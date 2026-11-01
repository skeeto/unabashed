// Virtual OS: an in-memory file system, pipes, and in-process programs
//
// The test and fuzz layers include this file after unabashed.cpp. They
// embed a Vos in their struct Os and define osvos() to return it. Nothing
// here touches the host: file names resolve against the virtual tree, and
// "processes" are fake programs (fakes.cpp) or nested shells.
//
// Each spawned process runs on a fiber of its own, scheduled by the vos
// deterministically: a process runs until it blocks (an empty or full
// pipe, waiting for a child, or os_wait), then the next runnable one in
// round-robin order runs. Pipes hold 64 KiB, as on Linux; a fake that
// writes to a pipe without readers dies as if by SIGPIPE. When nothing
// can run, the vos reports a deadlock and fails the blocked call
// instead of hanging. A chaos seed randomizes the choices.
//
// The initial tree mirrors the oracle container (tools/record) so that
// recorded specs and the vos agree: cwd and HOME /home/oracle, a /tmp,
// bash's test helpers in /usr/local/bin and the other fake programs in
// /opt/fakes/bin, with PATH=/usr/local/bin:/opt/fakes/bin.
//
// This is free and unencumbered software released into the public domain.

struct Vos;
static Vos *osvos(Os *);

struct VNode {
    VNode *child;   // directory entries, newest first
    VNode *next;
    VNode *parent;
    Str    name;
    i32    type;    // FT_*
    i32    mode;
    i32    nlink;
    i64    ino;
    i64    mtime;
    i32    uid;     // owner and group: 0 for the initial tree
    Str    data;    // file contents or link target
    iz     cap;
    i32    prog;    // index+1 into the program table, or 0
};

enum : i32 { VK_FREE, VK_FILE, VK_DIR, VK_PIPEREAD, VK_PIPEWRITE, VK_NULL };

enum { VPIPECAP = 1<<16 };

struct VPipe {
    u8  *buf;      // VPIPECAP bytes once written
    iz   pos;      // next unread byte
    iz   len;      // end of the buffered bytes
    i32  readers;
    i32  writers;
};

struct VFile {      // open file description
    i32    kind;
    i32    refs;
    i32    flags;
    VNode *node;
    VPipe *pipe;
    i64    pos;
};

enum { VOS_HANDLES = 1024 };

enum : i32 { VP_FREE, VP_RUN, VP_BLOCK, VP_EXIT };
enum : i32 { VW_NONE, VW_READ, VW_WRITE, VW_PROC, VW_IO, VW_YIELD };

// A process: slot 0 is the outermost shell, the rest are spawned.
struct VProc {
    i32     state;    // VP_*
    i32     status;   // once exited
    i32     prog;
    b32     shell;    // a shell: a broken pipe is an error, not a kill
    b32     failed;   // resumed to fail its blocked call (deadlock)
    Fiber  *fiber;    // where to resume it
    Fiber  *own;      // its own fiber, freed once reaped
    iz      stack;
    i32     wait;     // VW_*: what it is blocked on
    VPipe  *wpipe;
    Handle  wproc;
    Io     *ios;      // submitted and not yet completed
    FakeCtx ctx;
    Arena   mem;      // a fixed block: the program's memory
};

struct Vos {
    Arena   a;           // everything the vos allocates
    VNode  *root;
    VFile   files[VOS_HANDLES];
    VProc   procs[256];
    i32     nprocs;      // slots in use, including the outermost shell
    i32     cur;         // the running process
    i32     live;        // spawned processes not yet exited
    u64     chaos;       // seed for random scheduling, 0 for none
    u8     *freebufs;    // released pipe buffers
    i64     nextino;
    i32     user;        // owner of new nodes
    i64     clock;       // virtual realtime, ns; advances on each query
    Fake   *fakes;
    iz      nfakes;
    // Captured standard streams of the outermost shell
    Str     out;
    Str     err;
    Str     merged;
    iz      outcap;
    Str     in;          // its standard input
    iz      inpos;
};

static VNode *vnew(Vos *v, VNode *dir, Str name, i32 type, i32 mode)
{
    VNode *n = alloc<VNode>(&v->a);
    n->name   = clone(&v->a, name);
    n->type   = type;
    n->mode   = mode;
    n->nlink  = 1;
    n->ino    = ++v->nextino;
    n->mtime  = v->clock;
    n->uid    = v->user;
    n->parent = dir ? dir : n;
    if (dir) {
        n->next    = dir->child;
        dir->child = n;
    }
    return n;
}

static VNode *vlookup(VNode *dir, Str name)
{
    if (name == ".") return dir;
    if (name == "..") return dir->parent;
    for (VNode *n = dir->child; n; n = n->next) {
        if (n->name == name) return n;
    }
    return 0;
}

struct VResolved {
    VNode *node;    // the final node, or null if absent
    VNode *dir;     // its parent directory, when that exists
    Str    base;    // final component
    i32    err;
};

// Resolve an absolute path. Symbolic links are followed in every
// component, and in the last one only when follow is set.
static VResolved vresolve(Vos *v, Str path, b32 follow, i32 depth = 0)
{
    VResolved r = {};
    if (!path.len || path[0]!='/') {
        r.err = E_NOENT;
        return r;
    }
    if (depth > 40) {
        r.err = E_LOOP;
        return r;
    }
    VNode *cur = v->root;
    Str    rest = cuthead(path, 1);
    for (;;) {
        for (; rest.len && rest[0]=='/'; rest = cuthead(rest, 1)) {}
        if (!rest.len) {
            r.node = cur;
            r.dir  = cur->parent;
            r.base = cur->name;
            return r;
        }
        Cut c = cut(rest, '/');
        if (cur->type != FT_DIR) {
            r.err = E_NOTDIR;
            return r;
        }
        VNode *n = vlookup(cur, c.head);
        b32 last = 1;
        for (iz i = 0; i < c.tail.len; i++) {
            if (c.tail[i] != '/') last = 0;
        }
        if (!n) {
            if (last) {
                r.dir  = cur;
                r.base = c.head;
            }
            r.err = E_NOENT;
            return r;
        }
        // A trailing slash follows a final link and requires a directory
        if (n->type==FT_LINK && (!last || follow || c.ok)) {
            // Splice the target in front of the remaining components
            Arena *a = &v->a;
            Buf b(a, n->data.len + c.tail.len + 64);
            if (n->data.len && n->data[0]!='/') {
                // relative to the link's directory
                Buf d(a);
                for (VNode *p = cur; p != v->root; p = p->parent) {
                    Buf t(a);
                    print(&t, "/");
                    print(&t, p->name);
                    print(&t, finish(&d));
                    d = t;
                }
                print(&b, finish(&d));
                print(&b, "/");
            }
            print(&b, n->data);
            if (c.ok) {
                print(&b, "/");
                print(&b, c.tail);
            }
            return vresolve(v, finish(&b), follow, depth+1);
        }
        if (last && c.ok && n->type!=FT_DIR) {
            r.err = E_NOTDIR;
            return r;
        }
        if (last) {
            r.node = n;
            r.dir  = cur;
            r.base = c.head;
            return r;
        }
        cur  = n;
        rest = c.tail;
    }
}

static VNode *vmkdirs(Vos *v, Str path)
{
    VNode *cur = v->root;
    for (Str rest = path; rest.len;) {
        Cut c = cut(rest, '/');
        rest = c.tail;
        if (!c.head.len) continue;
        VNode *n = vlookup(cur, c.head);
        cur = n ? n : vnew(v, cur, c.head, FT_DIR, 0755);
    }
    return cur;
}

static void vwritenode(Vos *v, VNode *n, i64 pos, u8 *buf, iz len)
{
    iz end = (iz)pos + len;
    if (end > n->cap) {
        iz cap = n->cap ? n->cap : 64;
        while (cap < end) cap *= 2;
        u8 *data = allocbytes(&v->a, cap);
        copybytes(data, n->data.data, n->data.len);
        n->data.data = data;
        n->cap = cap;
    }
    if (pos > n->data.len) {
        __builtin_memset(n->data.data+n->data.len, 0, (uz)(pos-n->data.len));
    }
    copybytes(n->data.data+pos, buf, len);
    n->data.len = n->data.len>end ? n->data.len : end;
    n->mtime = v->clock;
}

// Append what fits; the caller checked for space.
static iz vpipewrite(Vos *v, VPipe *p, u8 *buf, iz len)
{
    if (!p->buf) {
        p->buf = v->freebufs;
        if (p->buf) {
            __builtin_memcpy(&v->freebufs, p->buf, sizeof(u8 *));
        } else {
            p->buf = allocbytes(&v->a, VPIPECAP);
        }
    }
    iz space = VPIPECAP - (p->len - p->pos);
    iz n = len<space ? len : space;
    if (p->len+n > VPIPECAP) {
        __builtin_memmove(p->buf, p->buf+p->pos, (uz)(p->len-p->pos));
        p->len -= p->pos;
        p->pos  = 0;
    }
    copybytes(p->buf+p->len, buf, n);
    p->len += n;
    return n;
}

static iz vpiperead(VPipe *p, u8 *buf, iz len)
{
    iz n = p->len - p->pos;
    n = n<len ? n : len;
    copybytes(buf, p->buf+p->pos, n);
    p->pos += n;
    if (p->pos == p->len) p->pos = p->len = 0;
    return n;
}

static u32 vrand(Vos *v)
{
    u64 x = v->chaos;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    v->chaos = x;
    return (u32)((x * 0x2545f4914f6cdd1dull) >> 32);
}


static Handle vnewhandle(Vos *v)
{
    for (iz i = 3; i < VOS_HANDLES; i++) {
        if (v->files[i].kind == VK_FREE) {
            v->files[i] = {};
            v->files[i].refs = 1;
            return (Handle)i;
        }
    }
    return BADHANDLE;
}

static VFile *vfile(Vos *v, Handle h)
{
    if (h>=3 && h<VOS_HANDLES && v->files[h].kind!=VK_FREE) {
        return v->files + h;
    }
    return 0;
}


// Processes

static b32 vioready(Vos *, Io *);

static b32 vready(Vos *v, VProc *p)
{
    if (p->state == VP_RUN) return 1;
    if (p->state != VP_BLOCK) return 0;
    VPipe *q = p->wpipe;
    switch (p->wait) {
    case VW_READ:  return q->len>q->pos || !q->writers;
    case VW_WRITE: return q->len-q->pos<VPIPECAP || !q->readers;
    case VW_PROC:  return v->procs[p->wproc].state == VP_EXIT;
    case VW_YIELD: return 1;
    case VW_IO:
        for (Io *io = p->ios; io; io = io->next) {
            if (vioready(v, io)) return 1;
        }
        return 0;
    }
    return 0;
}

// The next process to run after the current one, which may be itself
// (last, unless chaos picks it), or -1 if none can run.
static i32 vpick(Vos *v, b32 selfok)
{
    i32 n    = v->nprocs;
    i32 self = v->cur;
    if (v->chaos) {
        i32 ready[256];
        i32 nready = 0;
        for (i32 i = 0; i < n; i++) {
            if ((i!=self || selfok) && vready(v, v->procs+i)) ready[nready++] = i;
        }
        return nready ? ready[vrand(v) % (u32)nready] : -1;
    }
    for (i32 i = 1; i <= n; i++) {
        i32 j = (self + i) % n;
        if ((j!=self || selfok) && vready(v, v->procs+j)) return j;
    }
    return -1;
}

static void vrun(Os *os, Vos *v, i32 next)
{
    VProc *me = v->procs + v->cur;
    VProc *p  = v->procs + next;
    p->state = VP_RUN;
    p->wait  = VW_NONE;
    if (next == v->cur) return;
    me->fiber = os_self(os);
    v->cur = next;
    os_switch(os, p->fiber);
}

// Block the current process on its wait condition until it holds.
// Returns false if nothing else can run, so that it never would.
static b32 vschedule(Os *os, Vos *v)
{
    VProc *me = v->procs + v->cur;
    me->state = VP_BLOCK;
    i32 next = vpick(v, 1);
    if (next < 0) {
        me->state = VP_RUN;
        me->wait  = VW_NONE;
        return 0;
    }
    vrun(os, v, next);
    b32 ok = !me->failed;
    me->failed = 0;
    return ok;
}

static void vdeadlock(Vos *v)
{
    Str msg = "vos: deadlock\n";
    for (i32 i = 0; i < 2; i++) {
        Str *dst = i ? &v->merged : &v->err;
        if (msg.len <= v->outcap - dst->len) {
            copybytes(dst->data+dst->len, msg.data, msg.len);
            dst->len += msg.len;
        }
    }
}

// Give the other runnable processes a turn.
static void vyield(Os *os, Vos *v)
{
    v->procs[v->cur].wait = VW_YIELD;
    vschedule(os, v);
}

static void vcloseall(Os *os, VProc *p)
{
    for (iz i = 0; i < countof(p->ctx.fd); i++) {
        os_close(os, p->ctx.fd[i]);
        p->ctx.fd[i] = BADHANDLE;
    }
}

// End the current process: it never runs again. If nothing else can
// run, resume a blocked process to fail its call.
[[noreturn]] static void vexit(Os *os, Vos *v, i32 status)
{
    VProc *me = v->procs + v->cur;
    vcloseall(os, me);
    me->status = status;
    me->state  = VP_EXIT;
    me->wait   = VW_NONE;
    v->live--;
    i32 next = vpick(v, 0);
    if (next < 0) {
        vdeadlock(v);
        for (i32 i = 0; i < v->nprocs; i++) {
            if (v->procs[i].state == VP_BLOCK) {
                next = i;
                v->procs[i].failed = 1;
                break;
            }
        }
    }
    assert(next >= 0);
    vrun(os, v, next);
    __builtin_trap();
}

// End another process, as by a signal.
static void vkill(Os *os, Vos *v, VProc *p, i32 status)
{
    if (p->state==VP_EXIT || p->state==VP_FREE) return;
    vcloseall(os, p);
    p->status = status;
    p->state  = VP_EXIT;
    p->wait   = VW_NONE;
    v->live--;
}

// Release an exited process's slot.
static i32 vreap(Os *os, Vos *v, Handle proc)
{
    VProc *p = v->procs + proc;
    if (p->own) os_fiberfree(os, p->own);
    p->own   = p->fiber = 0;
    p->state = VP_FREE;
    return p->status;
}

static void vprocentry(Os *os, void *arg)
{
    VProc *p = (VProc *)arg;
    Vos   *v = osvos(os);
    vexit(os, v, v->fakes[p->prog].main(os, &p->ctx));
}

// The running process's stack size, for nested shells.
static iz vstack(Os *os)
{
    Vos *v = osvos(os);
    return v->procs[v->cur].stack;
}

// Free the fibers of processes left over after a run.
static void vfinish(Os *os)
{
    Vos *v = osvos(os);
    for (i32 i = 1; i < v->nprocs; i++) {
        VProc *p = v->procs + i;
        if (p->own) os_fiberfree(os, p->own);
        p->own = p->fiber = 0;
    }
}


// Platform API over the vos

static Opened os_open(Os *os, Arena, Str path, i32 flags, i32 mode)
{
    Vos   *v = osvos(os);
    Opened r = {};
    r.h = BADHANDLE;
    v->clock += 1000;
    if (path == "/dev/null") {
        Handle h = vnewhandle(v);
        if (h == BADHANDLE) { r.err = E_MFILE; return r; }
        v->files[h].kind  = VK_NULL;
        v->files[h].flags = flags;
        r.h = h;
        return r;
    }
    VResolved res = vresolve(v, path, 1);
    VNode *n = res.node;
    if (!n) {
        if (res.err!=E_NOENT || !(flags&OF_CREATE) || !res.dir) {
            r.err = res.err;
            return r;
        }
        if (!(res.dir->mode & 0200)) {
            r.err = E_ACCES;
            return r;
        }
        n = vnew(v, res.dir, res.base, FT_FILE, mode & 0777 & ~022);
    } else if ((flags&OF_CREATE) && (flags&OF_EXCL)) {
        r.err = E_EXIST;
        return r;
    } else if (n->type==FT_DIR && (flags&OF_WRITE)) {
        r.err = E_ISDIR;
        return r;
    } else if ((flags&OF_WRITE) && !(n->mode&0200)) {
        r.err = E_ACCES;
        return r;
    } else if ((flags&OF_READ) && !(n->mode&0400)) {
        r.err = E_ACCES;
        return r;
    }
    Handle h = vnewhandle(v);
    if (h == BADHANDLE) {
        r.err = E_MFILE;
        return r;
    }
    VFile *f = v->files + h;
    f->kind  = n->type==FT_DIR ? VK_DIR : VK_FILE;
    f->flags = flags;
    f->node  = n;
    if ((flags&OF_TRUNC) && n->type==FT_FILE) {
        n->data.len = 0;
        n->mtime    = v->clock;
    }
    r.h = h;
    return r;
}

static void os_close(Os *os, Handle h)
{
    Vos   *v = osvos(os);
    VFile *f = vfile(v, h);
    if (!f || --f->refs) return;
    if (f->kind == VK_PIPEREAD)  f->pipe->readers--;
    if (f->kind == VK_PIPEWRITE) f->pipe->writers--;
    VPipe *p = f->pipe;
    if (p && !p->readers && !p->writers && p->buf) {
        __builtin_memcpy(p->buf, &v->freebufs, sizeof(u8 *));
        v->freebufs = p->buf;
        p->buf = 0;
    }
    f->kind = VK_FREE;
}

static Handle os_dup(Os *os, Handle h)
{
    Vos *v = osvos(os);
    if (h < 3) return h;  // the outermost standard streams
    VFile *f = vfile(v, h);
    if (f) f->refs++;
    return f ? h : BADHANDLE;
}

static iz os_pipecap(Os *)
{
    return VPIPECAP;
}

static OsPipe os_pipe(Os *os)
{
    Vos   *v = osvos(os);
    OsPipe r = {};
    r.r = vnewhandle(v);
    if (r.r != BADHANDLE) {
        v->files[r.r].kind = VK_PIPEREAD;  // claimed before the next search
    }
    r.w = vnewhandle(v);
    if (r.r==BADHANDLE || r.w==BADHANDLE) {
        if (r.r != BADHANDLE) v->files[r.r].kind = VK_FREE;
        r.err = E_MFILE;
        return r;
    }
    VPipe *p = alloc<VPipe>(&v->a);
    p->readers = p->writers = 1;
    v->files[r.r].kind = VK_PIPEREAD;
    v->files[r.r].pipe = p;
    v->files[r.w].kind = VK_PIPEWRITE;
    v->files[r.w].pipe = p;
    return r;
}

static b32 os_write(Os *os, Handle h, u8 *buf, iz len)
{
    Vos *v = osvos(os);
    if (h==1 || h==2) {
        Str *dst = h==1 ? &v->out : &v->err;
        if (len > v->outcap - dst->len) {
            return 0;
        }
        copybytes(dst->data+dst->len, buf, len);
        dst->len += len;
        if (len <= v->outcap - v->merged.len) {
            copybytes(v->merged.data+v->merged.len, buf, len);
            v->merged.len += len;
        }
        return 1;
    }
    VFile *f = vfile(v, h);
    if (!f) return 0;
    switch (f->kind) {
    case VK_NULL:
        return 1;
    case VK_FILE:
        if (!(f->flags & OF_WRITE)) return 0;
        if (f->flags & OF_APPEND) f->pos = f->node->data.len;
        if (len > ((iz)1<<26) - f->pos) return 0;  // file too large
        vwritenode(v, f->node, f->pos, buf, len);
        f->pos += len;
        return 1;
    case VK_PIPEWRITE: {
        VPipe *p  = f->pipe;
        VProc *me = v->procs + v->cur;
        if (v->chaos && vrand(v)%4==0) vyield(os, v);
        while (len > 0) {
            if (!p->readers) {
                if (!me->shell) vexit(os, v, 128+13);  // SIGPIPE
                return 0;
            }
            if (p->len-p->pos == VPIPECAP) {
                me->wait  = VW_WRITE;
                me->wpipe = p;
                if (!vschedule(os, v)) {
                    vdeadlock(v);
                    return 0;
                }
                continue;
            }
            iz n = vpipewrite(v, p, buf, len);
            buf += n;
            len -= n;
        }
        return 1;
    }
    }
    return 0;
}

static iz os_read(Os *os, Handle h, u8 *buf, iz len)
{
    Vos *v = osvos(os);
    if (h == 0) {
        iz n = v->in.len - v->inpos;
        n = n<len ? n : len;
        copybytes(buf, v->in.data+v->inpos, n);
        v->inpos += n;
        return n;
    }
    VFile *f = vfile(v, h);
    if (!f) return -1;
    switch (f->kind) {
    case VK_NULL:
        return 0;
    case VK_FILE: {
        if (!(f->flags & OF_READ)) return -1;
        iz avail = f->node->data.len - (iz)f->pos;
        iz n = avail<len ? avail : len;
        n = n<0 ? 0 : n;
        copybytes(buf, f->node->data.data+f->pos, n);
        f->pos += n;
        return n;
    }
    case VK_PIPEREAD: {
        VPipe *p  = f->pipe;
        VProc *me = v->procs + v->cur;
        if (v->chaos && vrand(v)%4==0) vyield(os, v);
        while (p->len==p->pos && p->writers) {
            me->wait  = VW_READ;
            me->wpipe = p;
            if (!vschedule(os, v)) {
                vdeadlock(v);
                return -1;
            }
        }
        return vpiperead(p, buf, len);
    }
    }
    return -1;
}

static i64 os_seek(Os *os, Handle h, i64 off, i32 whence)
{
    Vos   *v = osvos(os);
    VFile *f = vfile(v, h);
    if (!f || f->kind!=VK_FILE) return -1;
    i64 base = whence==0 ? 0 : whence==1 ? f->pos : f->node->data.len;
    if (off < -base) return -1;
    f->pos = base + off;
    return f->pos;
}

static Stat vstat(VNode *n)
{
    Stat r = {};
    r.type  = n->type;
    r.mode  = n->mode;
    r.nlink = n->nlink;
    r.size  = n->data.len;
    r.mtime = n->mtime;
    r.atime = n->mtime;  // never "modified since last read"
    r.dev   = 1;
    r.ino   = n->ino;
    r.uid   = n->uid;
    r.gid   = n->uid;
    return r;
}

static Stat os_stat(Os *os, Arena, Str path, b32 follow)
{
    Vos *v = osvos(os);
    if (path == "/dev/null") {
        Stat r = {};
        r.type = FT_CHAR;
        r.mode = 0666;
        r.dev  = 2;
        r.ino  = 1;
        return r;
    }
    VResolved res = vresolve(v, path, follow);
    if (!res.node) {
        Stat r = {};
        r.err = res.err;
        return r;
    }
    return vstat(res.node);
}

static Stat os_fstat(Os *os, Handle h)
{
    Vos *v = osvos(os);
    Stat r = {};
    if (h < 3) {
        r.type = FT_PIPE;
        r.mode = 0600;
        return r;
    }
    VFile *f = vfile(v, h);
    if (!f) {
        r.err = E_BADF;
    } else if (f->node) {
        r = vstat(f->node);
    } else if (f->kind == VK_NULL) {
        r.type = FT_CHAR;
        r.mode = 0666;
    } else {
        r.type = FT_PIPE;
        r.mode = 0600;
    }
    return r;
}

// Directory entries in creation order, as a fresh tmpfs lists them.
static Dirent *os_list(Os *os, Arena *a, Str dir, i32 *err)
{
    Vos *v = osvos(os);
    VResolved res = vresolve(v, dir, 1);
    *err = res.err;
    if (!res.node) return 0;
    if (res.node->type != FT_DIR) {
        *err = E_NOTDIR;
        return 0;
    }
    Dirent *head = 0;
    for (VNode *n = res.node->child; n; n = n->next) {
        Dirent *d = alloc<Dirent>(a);
        d->name = clone(a, n->name);
        d->type = n->type;
        d->next = head;
        head = d;
    }
    return head;
}

static i64 os_now(Os *os)
{
    Vos *v = osvos(os);
    return v->clock += 1000;
}

enum { VIMAGE = 1024 };  // size of a fake program's file, mostly NULs
static u8 const vmagic[12] = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0, 'f', 'a', 'k', 'e'};

// The program a file holds: installed as such, or a copy of one.
static i32 vimageprog(Vos *v, VNode *n)
{
    if (n->prog) return n->prog;
    Str  d = n->data;
    if (d.len<14 || !(takehead(d, 12) == Str((u8 *)vmagic, 12))) {
        return 0;
    }
    i32 prog = d[12] | d[13]<<8;
    return prog<=v->nfakes ? prog : 0;
}

// Look up a program by its path's final component when the path names
// an executable fake in the tree.
static i32 vprogram(Vos *v, Str path, i32 *err)
{
    VResolved res = vresolve(v, path, 1);
    if (!res.node) {
        *err = res.err;
        return -1;
    }
    VNode *n = res.node;
    if (n->type == FT_DIR) {
        *err = E_ACCES;
        return -1;
    }
    if (!(n->mode & 0100)) {
        *err = E_ACCES;
        return -1;
    }
    i32 prog = vimageprog(v, n);
    if (!prog) {
        *err = E_NOEXEC;
        return -1;
    }
    return prog - 1;
}

// Spawn a fake program as a process of its own, runnable at once. Its
// arguments are copied into its memory, since it outlives the call.
static Spawned os_spawn(Os *os, Arena, Spawn *s)
{
    Vos    *v = osvos(os);
    Spawned r = {};
    r.proc = BADHANDLE;
    i32 prog = vprogram(v, s->path, &r.err);
    if (prog < 0) {
        return r;
    }
    i32 id = 1;
    for (; id<v->nprocs && v->procs[id].state!=VP_FREE; id++) {}
    if (id==countof(v->procs) || v->live>=64) {
        r.err = E_AGAIN;
        return r;
    }
    v->nprocs += id == v->nprocs;
    VProc *p = v->procs + id;
    if (!p->mem.beg) {
        enum { VPMEM = 1<<22 };
        p->mem.beg = (byte *)allocbytes(&v->a, VPMEM);
        p->mem.end = p->mem.beg + VPMEM;
    }
    Arena mem = {};
    mem.beg = p->mem.beg;
    mem.end = p->mem.end;
    FakeCtx ctx = {};
    ctx.argc = s->argc;
    ctx.argv = alloc<Str>(&mem, s->argc+1);
    for (iz i = 0; i < s->argc; i++) ctx.argv[i] = clone(&mem, s->argv[i]);
    ctx.nenv = s->nenv;
    ctx.env  = alloc<Str>(&mem, s->nenv+1);
    for (iz i = 0; i < s->nenv; i++) ctx.env[i] = clone(&mem, s->env[i]);
    ctx.cwd  = clone(&mem, s->cwd);
    for (iz i = 0; i < countof(ctx.fd); i++) {
        ctx.fd[i] = BADHANDLE;
    }
    for (iz i = 0; i < s->nfds; i++) {
        if (s->fds[i].fd < countof(ctx.fd)) {
            ctx.fd[s->fds[i].fd] = os_dup(os, s->fds[i].h);
        }
    }
    ctx.scratch = mem;

    p->state  = VP_RUN;
    p->status = 0;
    p->prog   = prog;
    p->shell  = v->fakes[prog].name == "bash";
    p->failed = 0;
    p->wait   = VW_NONE;
    p->ios    = 0;
    p->ctx    = ctx;
    p->stack  = p->shell ? (iz)1<<25 : (iz)1<<20;
    p->own    = p->fiber = os_fiber(os, &v->a, p->stack, vprocentry, p);
    v->live++;
    r.proc = (Handle)id;
    return r;
}

static i32 os_waitproc(Os *os, Handle proc)
{
    Vos *v = osvos(os);
    if (!proc || proc>=(Handle)v->nprocs || v->procs[proc].state==VP_FREE) {
        return 127;
    }
    VProc *me = v->procs + v->cur;
    while (v->procs[proc].state != VP_EXIT) {
        me->wait  = VW_PROC;
        me->wproc = proc;
        if (!vschedule(os, v)) {
            vdeadlock(v);
            vkill(os, v, v->procs+proc, 128+9);
        }
    }
    return vreap(os, v, proc);
}

// Can a submitted operation complete now? Then with perform set, do it.
static b32 vio(Os *os, Vos *v, Io *io, b32 perform)
{
    VFile *f = io->op==IO_PROC ? 0 : vfile(v, io->h);
    switch (io->op) {
    case IO_READ:
        if (f && f->kind==VK_PIPEREAD) {
            VPipe *p = f->pipe;
            if (p->len==p->pos && p->writers) return 0;
            if (perform) io->n = vpiperead(p, io->buf, io->len);
        } else if (perform) {
            io->n = os_read(os, io->h, io->buf, io->len);
            io->err = io->n<0 ? E_BADF : 0;
        }
        return 1;
    case IO_WRITE:
        if (f && f->kind==VK_PIPEWRITE) {
            VPipe *p = f->pipe;
            if (!p->readers) {
                io->err = E_PIPE;
                return 1;
            }
            if (p->len-p->pos == VPIPECAP) return 0;
            if (perform) io->n = vpipewrite(v, p, io->buf, io->len);
        } else if (perform) {
            io->n   = io->len;
            io->err = os_write(os, io->h, io->buf, io->len) ? 0 : E_BADF;
        }
        return 1;
    case IO_PROC:
        if (!io->h || io->h>=(Handle)v->nprocs || v->procs[io->h].state==VP_FREE) {
            io->err = E_CHILD;
            return 1;
        }
        if (v->procs[io->h].state != VP_EXIT) return 0;
        if (perform) io->n = vreap(os, v, io->h);
        return 1;
    }
    io->err = E_INVAL;
    return 1;
}

static b32 vioready(Vos *v, Io *io)
{
    return vio(0, v, io, 0);
}

static void os_submit(Os *os, Io *io)
{
    Vos   *v  = osvos(os);
    VProc *me = v->procs + v->cur;
    io->done = 0;
    io->next = 0;
    if (!v->chaos || vrand(v)%2) {
        if (vio(os, v, io, 1)) {
            io->done = 1;
            return;
        }
    }
    Io **tail = &me->ios;
    while (*tail) tail = &(*tail)->next;
    *tail = io;
}

static Io *os_wait(Os *os, i64 deadline)
{
    Vos   *v  = osvos(os);
    VProc *me = v->procs + v->cur;
    b32 yielded = 0;
    for (;;) {
        for (Io **p = &me->ios; *p; p = &(*p)->next) {
            Io *io = *p;
            if (vio(os, v, io, 1)) {
                *p = io->next;
                io->done = 1;
                return io;
            }
        }
        if (deadline == WAIT_POLL) {
            // Polling: let the other processes make progress once
            if (yielded) return 0;
            vyield(os, v);
            yielded = 1;
            continue;
        }
        me->wait = VW_IO;
        if (!vschedule(os, v)) {
            if (deadline > 0) {
                v->clock = v->clock>deadline ? v->clock : deadline;
            } else {
                vdeadlock(v);
            }
            return 0;
        }
    }
}

static b32 os_cancel(Os *os, Io *io)
{
    Vos *v = osvos(os);
    for (i32 i = 0; i < v->nprocs; i++) {
        for (Io **p = &v->procs[i].ios; *p; p = &(*p)->next) {
            if (*p == io) {
                *p = io->next;
                return 1;
            }
        }
    }
    return 0;
}


// Setup

static void vinstall(Vos *v, Str dir, Str name, i32 prog)
{
    VNode *d = vmkdirs(v, dir);
    VNode *n = vnew(v, d, name, FT_FILE, 0755);
    n->prog = prog + 1;
    // Contents like an executable's, so that shells see a binary file,
    // naming the program so that copies of it run too
    u8 *image = allocbytes(&v->a, VIMAGE);
    __builtin_memset(image, 0, VIMAGE);
    copybytes(image, (u8 *)vmagic, 12);
    image[12] = (u8)(prog + 1);
    image[13] = (u8)((prog + 1) >> 8);
    n->data = {image, VIMAGE};
    n->cap  = VIMAGE;
}

// Reset the vos to the oracle container's initial state.
static void vreset(Vos *v, Arena a, Fake *fakes, iz nfakes)
{
    Str out    = v->out;
    Str err    = v->err;
    Str merged = v->merged;
    iz  outcap = v->outcap;
    *v = {};
    v->a       = a;
    v->out     = out;
    v->err     = err;
    v->merged  = merged;
    v->outcap  = outcap;
    v->out.len = v->err.len = v->merged.len = 0;
    v->clock   = (i64)1700000000 * 1000000000;
    v->fakes   = fakes;
    v->nfakes  = nfakes;
    v->nprocs  = 1;  // the outermost shell
    v->procs[0].state = VP_RUN;
    v->procs[0].shell = 1;

    v->root = vnew(v, 0, "", FT_DIR, 0755);
    vmkdirs(v, "home/oracle")->uid = 1000;
    VNode *tmp = vmkdirs(v, "tmp");
    tmp->mode = 01777;
    vmkdirs(v, "usr/local/bin");
    vmkdirs(v, "usr/bin");
    vmkdirs(v, "etc");
    vmkdirs(v, "dev");
    VNode *bin = vnew(v, v->root, "bin", FT_LINK, 0777);
    bin->data = "usr/bin";
    for (iz i = 0; i < nfakes; i++) {
        Str  name = fakes[i].name;
        b32  local = name=="bash" || name=="recho" || name=="zecho"
                  || name=="printenv" || name=="xcase";
        vinstall(v, local ? Str("usr/local/bin") : Str("opt/fakes/bin"), name, (i32)i);
    }
    v->user = 1000;  // everything made from here on is the user's
}


// Platform extensions for the fakes (declared in fakes.cpp)

static void vdetach(VNode *n)
{
    for (VNode **p = &n->parent->child; *p; p = &(*p)->next) {
        if (*p == n) {
            *p = n->next;
            break;
        }
    }
    n->next = 0;
}

// Check that a new name can be made, returning its directory.
static VNode *vcreatable(Vos *v, Str path, VResolved *r, i32 *err)
{
    *r = vresolve(v, path, 0);
    v->clock += 1000;
    if (r->node) {
        *err = E_EXIST;
    } else if (r->err!=E_NOENT || !r->dir) {
        *err = r->err;
    } else if (!(r->dir->mode & 0200)) {
        *err = E_ACCES;
    } else {
        *err = 0;
        return r->dir;
    }
    return 0;
}

static i32 os_mkdir(Os *os, Arena, Str path, i32 mode)
{
    Vos      *v = osvos(os);
    VResolved r;
    i32       err;
    VNode    *dir = vcreatable(v, path, &r, &err);
    if (!dir) return err;
    vnew(v, dir, r.base, FT_DIR, mode & 07777 & ~022)->nlink = 2;
    return 0;
}

static i32 os_mkfifo(Os *os, Arena, Str path, i32 mode)
{
    Vos      *v = osvos(os);
    VResolved r;
    i32       err;
    VNode    *dir = vcreatable(v, path, &r, &err);
    if (!dir) return err;
    vnew(v, dir, r.base, FT_FIFO, mode & 07777 & ~022);
    return 0;
}

static i32 os_symlink(Os *os, Arena, Str target, Str path)
{
    Vos      *v = osvos(os);
    VResolved r;
    i32       err;
    VNode    *dir = vcreatable(v, path, &r, &err);
    if (!dir) return err;
    vnew(v, dir, r.base, FT_LINK, 0777)->data = clone(&v->a, target);
    return 0;
}

// A hard link is a copy sharing the inode number: later writes to one
// do not show in the other.
static i32 os_link(Os *os, Arena, Str from, Str to)
{
    Vos      *v = osvos(os);
    VResolved s = vresolve(v, from, 0);
    if (!s.node) return s.err;
    if (s.node->type == FT_DIR) return E_PERM;
    VResolved r;
    i32       err;
    VNode    *dir = vcreatable(v, to, &r, &err);
    if (!dir) return err;
    VNode *n = vnew(v, dir, r.base, s.node->type, s.node->mode);
    n->ino   = s.node->ino;
    n->mtime = s.node->mtime;
    n->prog  = s.node->prog;
    n->data  = clone(&v->a, s.node->data);
    n->cap   = n->data.len;
    n->nlink = ++s.node->nlink;
    return 0;
}

static i32 os_unlink(Os *os, Arena, Str path)
{
    Vos      *v = osvos(os);
    VResolved r = vresolve(v, path, 0);
    v->clock += 1000;
    if (!r.node) return r.err;
    if (r.node->type == FT_DIR) return E_ISDIR;
    if (!(r.node->parent->mode & 0200)) return E_ACCES;
    vdetach(r.node);
    return 0;
}

static i32 os_rmdir(Os *os, Arena, Str path)
{
    Vos      *v = osvos(os);
    VResolved r = vresolve(v, path, 0);
    v->clock += 1000;
    if (!r.node) return r.err;
    VNode *n = r.node;
    if (n->type != FT_DIR) return E_NOTDIR;
    if (r.base == ".") return E_INVAL;
    if (r.base=="..") return E_NOTEMPTY;
    if (n == v->root) return E_BUSY;
    if (n->child) return E_NOTEMPTY;
    if (!(n->parent->mode & 0200)) return E_ACCES;
    vdetach(n);
    return 0;
}

static i32 os_rename(Os *os, Arena, Str from, Str to)
{
    Vos      *v = osvos(os);
    VResolved a = vresolve(v, from, 0);
    VResolved b = vresolve(v, to, 0);
    v->clock += 1000;
    if (!a.node) return a.err;
    if (!b.node && (b.err!=E_NOENT || !b.dir)) return b.err;
    VNode *src = a.node;
    if (src == v->root) return E_BUSY;
    if (b.node == src) return 0;
    VNode *dir = b.node ? b.node->parent : b.dir;
    if (src->type == FT_DIR) {
        for (VNode *d = dir;; d = d->parent) {
            if (d == src) return E_INVAL;
            if (d == v->root) break;
        }
    }
    if (!(src->parent->mode & 0200) || !(dir->mode & 0200)) return E_ACCES;
    Str name = b.node ? b.node->name : clone(&v->a, b.base);
    if (b.node) {
        if (src->type == FT_DIR) {
            if (b.node->type != FT_DIR) return E_NOTDIR;
            if (b.node->child) return E_NOTEMPTY;
        } else if (b.node->type == FT_DIR) {
            return E_ISDIR;
        }
        vdetach(b.node);
    }
    vdetach(src);
    src->name   = name;
    src->parent = dir;
    src->next   = dir->child;
    dir->child  = src;
    return 0;
}

static i32 os_chmod(Os *os, Arena, Str path, i32 mode)
{
    Vos      *v = osvos(os);
    VResolved r = vresolve(v, path, 1);
    if (!r.node) return r.err;
    r.node->mode = mode & 07777;
    return 0;
}

static i32 os_utime(Os *os, Arena, Str path, i64 mtime)
{
    Vos      *v = osvos(os);
    VResolved r = vresolve(v, path, 1);
    if (!r.node) return r.err;
    r.node->mtime = mtime;
    return 0;
}

static Str os_readlink(Os *os, Arena *a, Str path)
{
    Vos      *v = osvos(os);
    VResolved r = vresolve(v, path, 0);
    if (!r.node || r.node->type!=FT_LINK) return {};
    return clone(a, r.node->data);
}

static i32 os_umask(Os *)
{
    return 022;
}

static OsIds os_ids(Os *)
{
    OsIds r = {1000, 1000, 1000, 1000};
    return r;
}

// Virtual time passes instantly.
static void os_sleep(Os *os, i64 ns)
{
    osvos(os)->clock += ns;
}

static Str os_uname(Os *, Arena *, i32 field)
{
    switch (field) {
    case UN_SYSNAME:  return "Linux";
    case UN_NODENAME: return "oracle";
    case UN_RELEASE:  return "6.1.0";
    case UN_VERSION:  return "#1 SMP PREEMPT_DYNAMIC";
    case UN_MACHINE:  return "x86_64";
    }
    return "unknown";
}
