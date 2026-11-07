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
// Windows mode (Vos.win, see "Windows personality" below) replaces it
// with drives C: and D:, names looked up ignoring case, reserved DOS
// names as the null device, a //./pipe/ namespace, and fakes installed
// as synthetic PE images that split their command lines as the real
// programs would.
//
// This is free and unencumbered software released into the public domain.

#include "vterm.cpp"

struct Vos;
static Vos *osvos(Os *);
// The layer's memory, from which every reservation comes.
static byte *osheap(Os *, iz *len);

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
    i64    atime;   // last read, updated as relatime does
    i64    ctime;   // status change
    i32    uid;     // owner and group: 0 for the initial tree
    Str    data;    // file contents or link target
    iz     cap;
    i32    prog;    // index+1 into the program table, or 0
};

enum : i32 { VK_FREE, VK_FILE, VK_DIR, VK_PIPEREAD, VK_PIPEWRITE, VK_NULL, VK_TTY };

enum { VPIPECAP = 1<<16 };

struct VPipe {
    u8  *buf;      // VPIPECAP bytes once written
    iz   pos;      // next unread byte
    iz   len;      // end of the buffered bytes
    i32  readers;
    i32  writers;
    u64  rkey;     // tracing: the last reader's identity, or 0
    u64  wgroup;   // tracing: the last writer's group (vgroup), or 0
};

struct VNamedPipe {  // Windows mode: an instance of //./pipe/NAME
    Str    name;       // NAME
    VPipe *pipe;
    i32    seq;        // creation order
    b32    out;        // the server writes, its client reads
    b32    connected;  // its one client has opened it
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

struct VWriter {
    u64 key;
    u64 hash[2];   // FNV-1a of what it wrote to stdout and stderr
};

struct VCount {
    u64 key;
    u64 n;
};

enum : i32 { VP_FREE, VP_RUN, VP_BLOCK, VP_EXIT };
enum : i32 { VW_NONE, VW_READ, VW_WRITE, VW_PROC, VW_IO, VW_YIELD, VW_SLEEP, VW_TTY };

// A process: slot 0 is the outermost shell, the rest are spawned.
struct VProc {
    i32     state;    // VP_*
    i32     status;   // once exited
    i32     prog;
    b32     shell;    // a shell: a broken pipe is an error, not a kill
    b32     failed;   // resumed to fail its blocked call (deadlock)
    b32     intr;     // resumed by a signal (shells): fail a read or os_wait
    i64     pid;
    u64     sigpend;  // shells: signals not yet taken by os_sigpending
    u64     grpsig;   // shells: the terminal's, not yet taken by os_siggroup
    LevelSig lsigs[16];  // shells: os_siglevel's, oldest first
    i32     nlsigs;
    u64     sigign;   // fakes: signals they ignore (Spawn.sigign)
    i64     until;    // VW_SLEEP, VW_IO with a deadline: time to wake at
    Fiber  *fiber;    // where to resume it
    Fiber  *own;      // its own fiber, freed once reaped
    iz      stack;
    i32     wait;     // VW_*: what it is blocked on
    VPipe  *wpipe;
    Handle  wproc;
    Io     *ios;      // submitted and not yet completed
    u64     lid;      // tracing: identity, the same under every seed
    u64     tag;      // tracing: a shell's running task (os_tasktag)
    iz      ntasks;   // tracing: a shell's task count
    u64     group;    // tracing: a shell's running task's group (os_tasktag)
    u64     ingroup;  // tracing: the writer group it was started in
    FakeCtx ctx;
    Arena   mem;      // a fixed block: the program's memory
};

// The terminal behind the outermost shell's descriptors 0-2 and
// /dev/tty, when a test gives one (t/edit): writes draw on term, reads
// take the script's keys. A snapshot of the screen (vtdump) is taken
// at each VKEY_SNAP, and at the end of the script, which then reads as
// end of file. In its cooked mode it does what a line discipline does
// (echo, erase, kill, ^C, ^D); the other modes as os_tty describes.
struct VTty {
    VTerm *term;   // null: no such terminal
    VKey  *keys;
    iz     nkeys;
    iz     next;   // the step in progress
    iz     pos;    // within its text
    i64    until;  // a pause in progress: its end, or 0
    i32    mode;   // TTY_*, 0 for the cooked mode it starts in
    b32    ended;  // the script ran out: its snapshot is taken
    Str    snaps;  // the snapshots, each after a "== snap N" line
    iz     snapcap;
    i32    nsnaps;
    u8     line[1024];  // cooked: the line being typed
    iz     linelen;
    u8     ready[1024]; // cooked: lines finished, not yet read
    iz     readylen;
    b32    eof;         // cooked: ^D on an empty line, not yet read
    b32    settling;    // vothersready: leave the terminal out
    b32    legacy;      // a console without VT: drawn with os_con
    i64    active;      // the clock at its last output or read
    // The editor's check (edcheck)
    b32    edoff;       // the line started off the left margin
    b32    edattr;      // ... or with attributes on
    b32    edlost;      // it was taller than the screen
    b32    edforeign;   // ... or others wrote over it
    b32    edwriting;   // the editor is writing
    b32    edtrap;      // trap on a failure, else record it
    i32    edbad;       // failures
    Str    edwhy;       // the first one: both screens
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
    Str     localtz;     // os_localtz's answer, "UTC0" if null
    i64     nextpid;     // for the next spawned process
    Fake   *fakes;
    iz      nfakes;
    // Captured standard streams of the outermost shell
    Str     out;
    Str     err;
    Str     merged;
    iz      outcap;
    b32     outfull;     // a write to them was refused at outcap
    Str     in;          // its standard input
    iz      inpos;
    b32     ttyin;       // or else a terminal that never delivers input
    VTty    tty;         // or a terminal typed at by a script (term)
    // Tracing for the chaos fuzzer: output per writer, as hashes, and
    // whether the run did something whose outcome may depend on the
    // schedule, such as changing a file while tasks run concurrently
    b32     trace;
    b32     racy;
    VWriter writers[256];
    i32     nwriters;
    VCount  spawns[256];   // programs started per (process, task)
    i32     nspawns;
    b32     images;  // self-spawn background shells (os_imagenew)
    b32     imaging; // os_reserve is for an image: the layer pins it
    ImageRef freeimgs[64];  // regions of images whose shells have ended
    i32      nfreeimgs;
    // Windows mode
    b32        win;
    VNamedPipe npipes[64];
    i32        nnpipes;
    i32        npipeseq;  // named pipes made so far
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
    n->atime  = v->clock;
    n->ctime  = v->clock;
    n->uid    = v->user;
    n->parent = dir ? dir : n;
    if (dir) {
        n->next    = dir->child;
        dir->child = n;
    }
    return n;
}

static VNode *vlookup(Vos *v, VNode *dir, Str name)
{
    if (name == ".") return dir;
    if (name == "..") return dir->parent;
    for (VNode *n = dir->child; n; n = n->next) {
        if (v->win ? equalfold(n->name, name) : n->name==name) return n;
    }
    return 0;
}

// The user's permission bits (r 4, w 2, x 1) on a node: the owner's
// bits for the user's own nodes, the others' bits for the rest.
static i32 vperm(Vos *v, VNode *n)
{
    return (n->uid==v->user ? n->mode>>6 : n->mode) & 7;
}

struct VResolved {
    VNode *node;    // the final node, or null if absent
    VNode *dir;     // its parent directory, when that exists
    Str    base;    // final component
    i32    err;
};

// Resolve an absolute path in the tree's own form (in Windows mode
// "/C:/x", from vpath). Symbolic links are followed in every
// component, and in the last one only when follow is set.
static VResolved vwalk(Vos *v, Str path, b32 follow, i32 depth = 0)
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
        if (!(vperm(v, cur) & 1)) {
            r.err = E_ACCES;  // a name looked up needs search permission
            return r;
        }
        VNode *n = vlookup(v, cur, c.head);
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
            b32 drive = v->win && hasdrive(n->data);
            if (drive) {
                putbyte(&b, '/');  // a Windows link target: "C:/x"
            } else if (n->data.len && n->data[0]!='/') {
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
            return vwalk(v, finish(&b), follow, depth+1);
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

enum : i32 {  // VPath kinds
    VN_FILE,  // path names a node in the tree
    VN_NULL,  // the null device
    VN_PIPE,  // a named pipe: path is its name
    VN_NONE,  // nothing can exist there (relative, UNC, other devices)
};

struct VPath {
    Str path;    // for VN_FILE, the tree's own form
    Str native;  // Windows mode: the canonical native path
    i32 kind;
};

// The tree's form of a path given to the platform. In Linux mode that
// is the path itself. In Windows mode it is what the Win32 layer makes
// of it (main_win32.cpp's widepath): /dev/null is NUL, MSYS-style /c/x
// is C:/x, the rest is made absolute lexically as GetFullPathName does,
// on drive C:, and reserved DOS names in any directory are the null
// device. Drive paths become "/C:/x" in the tree, whose root holds the
// drives.
static VPath vpath(Vos *v, Arena *a, Str path)
{
    VPath r = {};
    r.path   = path;
    r.native = path;
    if (!v->win) return r;
    if (path == "/dev/null") {
        r.kind   = VN_NULL;
        r.native = "//./NUL";
        return r;
    }
    Str p = path;
    if (p.len>=2 && p[0]=='/' && letter(p[1]) && (p.len==2 || p[2]=='/')) {
        Buf b(a, p.len+2);
        putbyte(&b, upcase(p[1]));
        putbyte(&b, ':');
        print(&b, p.len==2 ? Str("/") : cuthead(p, 2));
        p = finish(&b);
    }
    Str n = winnorm(a, p, "C:/");
    r.native = n;
    if (!n.data) {
        r.kind = VN_NONE;
        return r;
    }
    if (startswith(n, "//./")) {
        Cut c = cut(cuthead(n, 4), '/');
        if (c.ok && c.tail.len && equalfold(c.head, "pipe")) {
            r.kind = VN_PIPE;
            r.path = c.tail;
        } else {
            r.kind = !c.ok && reservedname(c.head, 0).data ? VN_NULL : VN_NONE;
        }
        return r;
    }
    if (!hasdrive(n)) {
        r.kind = VN_NONE;  // a share on a server, or //?/ without a drive
        return r;
    }
    if (reservedname(lastpart(n), 1).data) {
        r.kind   = VN_NULL;
        r.native = "//./NUL";
        return r;
    }
    r.path = concat(a, "/", n);
    return r;
}

// Resolve a path as given to the platform (see vpath and vwalk). What
// is not a node in the tree resolves to nothing.
static VResolved vresolve(Vos *v, Arena *a, Str path, b32 follow)
{
    VPath p = vpath(v, a, path);
    if (p.kind != VN_FILE) {
        VResolved r = {};
        r.err = E_NOENT;
        return r;
    }
    return vwalk(v, p.path, follow);
}

static VNode *vmkdirs(Vos *v, Str path)
{
    VNode *cur = v->root;
    for (Str rest = path; rest.len;) {
        Cut c = cut(rest, '/');
        rest = c.tail;
        if (!c.head.len) continue;
        VNode *n = vlookup(v, cur, c.head);
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
    n->mtime = n->ctime = v->clock;
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

static u64 vkey(Vos *);

static iz vpiperead(Vos *v, VPipe *p, u8 *buf, iz len)
{
    iz n = p->len - p->pos;
    n = n<len ? n : len;
    if (v->trace && n) {
        // Readers sharing a pipe may split its bytes any way
        u64 key = vkey(v);
        if (p->rkey && p->rkey!=key) v->racy = 1;
        p->rkey = key;
    }
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


static u64 vmix(u64 a, u64 b)
{
    u64 h = (a ^ b*0x9e3779b97f4a7c15u) * 0xbf58476d1ce4e5b9u;
    return h ^ h>>31;
}

// Tracing: the identity of the running process and task.
static u64 vkey(Vos *v)
{
    VProc *p = v->procs + v->cur;
    return vmix(p->lid, p->tag);
}

// Tracing: record output to the outermost standard streams by writer.
static void vtraceout(Vos *v, Handle h, u8 *buf, iz len)
{
    u64 key = vkey(v);
    i32 i = 0;
    for (; i<v->nwriters && v->writers[i].key!=key; i++) {}
    if (i == countof(v->writers)) {
        v->racy = 1;  // too many to tell apart
        return;
    }
    if (i == v->nwriters) {
        v->nwriters++;
        v->writers[i].key     = key;
        v->writers[i].hash[0] = v->writers[i].hash[1] = 0xcbf29ce484222325u;
    }
    u64 *hash = v->writers[i].hash + (h==2);
    for (iz j = 0; j < len; j++) {
        *hash = (*hash ^ buf[j]) * 0x100000001b3u;
    }
}

// Tracing: the writer group of the running process and task, never 0.
// A shell's loose task (os_tasktag) starts a group of its own; else it
// writes in the group its process was started in, as programs do.
static u64 vgroup(Vos *v)
{
    VProc *p = v->procs + v->cur;
    return p->group ? vmix(p->lid, p->group) : p->ingroup;
}

// Tracing: the running process writes a pipe. Bytes of two writer
// groups may arrive in either order (sched.cpp's tracewrite).
static void vtracewrite(Vos *v, VPipe *p)
{
    if (!v->trace) return;
    u64 group = vgroup(v);
    if (p->wgroup && p->wgroup!=group) v->racy = 1;
    p->wgroup = group;
}

// Tracing: is more than one task able to run now? A process waiting
// for a program it started counts as that program.
static b32 vconcurrent(Vos *v)
{
    iz n = 0;
    for (i32 i = 0; i < v->nprocs; i++) {
        VProc *p = v->procs + i;
        if (p->state!=VP_RUN && p->state!=VP_BLOCK) continue;
        iz k = p->ntasks;
        if (p->state==VP_BLOCK && p->wait==VW_PROC) k--;
        if (p->state==VP_BLOCK && p->wait==VW_IO) {
            for (Io *io = p->ios; io; io = io->next) k -= io->op==IO_PROC;
        }
        n += k;
    }
    return n > 1;
}

// Tracing: the file system changes. Concurrent tasks may see it in
// either order.
static void vmutate(Vos *v)
{
    if (v->trace && vconcurrent(v)) v->racy = 1;
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

// How a handle is open (OF_READ, OF_WRITE), for InheritFd.
static i32 vaccess(Vos *v, Handle h)
{
    VFile *f = vfile(v, h);
    switch (f ? f->kind : VK_FREE) {
    case VK_PIPEREAD:  return OF_READ;
    case VK_PIPEWRITE: return OF_WRITE;
    case VK_FILE:
        if (f->flags & (OF_READ|OF_WRITE)) return f->flags & (OF_READ|OF_WRITE);
    }
    return OF_READ | OF_WRITE;
}


// Processes

static b32 vioready(Vos *, Io *);
static b32 vttyready(Vos *);

static b32 vready(Vos *v, VProc *p)
{
    if (p->state == VP_RUN) return 1;
    if (p->state != VP_BLOCK) return 0;
    VPipe *q = p->wpipe;
    switch (p->wait) {
    case VW_READ:  return q->len>q->pos || !q->writers || p->intr;
    case VW_WRITE: return q->len-q->pos<VPIPECAP || !q->readers;
    case VW_PROC:  return v->procs[p->wproc].state == VP_EXIT;
    case VW_YIELD: return 1;
    case VW_SLEEP: return v->clock >= p->until;
    case VW_TTY:   return p->intr || vttyready(v);
    case VW_IO:
        if (p->intr || (p->until && v->clock>=p->until)) return 1;
        for (Io *io = p->ios; io; io = io->next) {
            if (vioready(v, io)) return 1;
        }
        return 0;
    }
    return 0;
}

// When nothing else can run, virtual time jumps to the earliest
// sleeper's deadline. False if there is none.
static b32 vadvance(Vos *v)
{
    i64 when = -1;
    for (i32 i = 0; i < v->nprocs; i++) {
        VProc *p = v->procs + i;
        b32 timed = p->wait==VW_SLEEP || (p->wait==VW_IO && p->until);
        if (p->state==VP_BLOCK && timed && (when<0 || p->until<when)) {
            when = p->until;
        }
    }
    if (v->tty.until && (when<0 || v->tty.until<when)) when = v->tty.until;
    if (when < 0) return 0;
    v->clock = v->clock>when ? v->clock : when;
    return 1;
}

// The next process to run after the current one, which may be itself
// (last, unless chaos picks it), or -1 if none can run.
static b32 vtype(Os *, Vos *);

static i32 vpick(Os *os, Vos *v, b32 selfok)
{
    i32 n    = v->nprocs;
    i32 self = v->cur;
    if (v->chaos) {
        i32 ready[256];
        i32 nready = 0;
        for (i32 i = 0; i < n; i++) {
            if ((i!=self || selfok) && vready(v, v->procs+i)) ready[nready++] = i;
        }
        if (nready) return ready[vrand(v) % (u32)nready];
        if (vtype(os, v)) return vpick(os, v, selfok);
        return vadvance(v) ? vpick(os, v, selfok) : -1;
    }
    for (i32 i = 1; i <= n; i++) {
        i32 j = (self + i) % n;
        if ((j!=self || selfok) && vready(v, v->procs+j)) return j;
    }
    if (vtype(os, v)) return vpick(os, v, selfok);
    return vadvance(v) ? vpick(os, v, selfok) : -1;
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
    i32 next = vpick(os, v, 1);
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
    i32 next = vpick(os, v, 0);
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
    if (p == v->procs+v->cur) vexit(os, v, status);
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
    i32 status = v->fakes[p->prog].main(os, &p->ctx);
    ImageRef im = p->ctx.image;
    if (im.base && v->nfreeimgs<countof(v->freeimgs)) {
        v->freeimgs[v->nfreeimgs++] = im;  // nothing refers to it now
    }
    // A program's exit status is truncated to a byte, as by a kernel
    // (sed's "Q888" is 120), while a shell reports its own deaths
    vexit(os, v, p->shell ? status : status&255);
}

// The running process's stack size, for nested shells.
static iz vstack(Os *os)
{
    Vos *v = osvos(os);
    return v->procs[v->cur].stack;
}

// Free the fibers of processes left over after a run.
// After the top shell exits: the programs it left running finish, as
// they would on a real system, until none can run. A program that
// never ends (yes &) makes the run racy rather than endless.
static void vdrain(Os *os, Vos *v)
{
    VProc *me = v->procs + v->cur;
    vcloseall(os, me);  // EOF for whoever reads what it wrote
    for (i32 steps = 0;; steps++) {
        i32 next = vpick(os, v, 0);
        if (next < 0) return;
        if (steps == 1<<14) {
            v->racy = 1;
            return;
        }
        me->state = VP_BLOCK;
        me->wait  = VW_YIELD;
        vrun(os, v, next);
    }
}

static void vfinish(Os *os)
{
    Vos *v = osvos(os);
    vdrain(os, v);
    for (i32 i = 1; i < v->nprocs; i++) {
        VProc *p = v->procs + i;
        if (p->own) os_fiberfree(os, p->own);
        p->own = p->fiber = 0;
    }
}


// Platform API over the vos

// The descriptor named by /dev/fd/N or /dev/std*, or -1.
static i32 vdevfd(Str path)
{
    if (path == "/dev/stdin")  return 0;
    if (path == "/dev/stdout") return 1;
    if (path == "/dev/stderr") return 2;
    if (!startswith(path, "/dev/fd/") || path.len==8 || path.len>12) return -1;
    i32 n = 0;
    for (iz i = 8; i < path.len; i++) {
        if (!digit(path[i])) return -1;
        n = n*10 + (path[i] - '0');
    }
    return n;
}

// The scripted terminal (VTty)

// Draw output on the terminal, \n as \r\n (output processing).
static void vttyout(Vos *v, u8 *buf, iz len)
{
    VTerm *t = v->tty.term;
    v->tty.active = v->clock;
    if (!v->tty.edwriting) v->tty.edforeign = 1;
    for (iz i = 0; i < len; i++) {
        if (buf[i] == '\n') vtbyte(t, '\r');
        vtbyte(t, buf[i]);
    }
}

static void vsnap(Vos *v)
{
    VTty *y = &v->tty;
    Arena scratch = v->a;
    Buf b(&scratch, 1<<12);
    print(&b, "== snap ");
    print(&b, (i64)++y->nsnaps);
    print(&b, "\n");
    print(&b, vtdump(y->term, &scratch));
    Str s = finish(&b);
    if (s.len <= y->snapcap - y->snaps.len) {
        copybytes(y->snaps.data+y->snaps.len, s.data, s.len);
        y->snaps.len += s.len;
    }
}

// A signal the terminal sends to every process on it (no process
// groups here): shells take it through os_siggroup.
static void vttysignal(Os *os, Vos *v, i32 sig)
{
    for (i32 i = 0; i < v->nprocs; i++) {
        VProc *p = v->procs + i;
        if (p->state!=VP_RUN && p->state!=VP_BLOCK) continue;
        if (p->shell) {
            p->grpsig |= (u64)1 << (sig-1);
            p->intr = p != v->procs+v->cur;
        } else {
            os_kill(os, p->pid, sig);
        }
    }
}

static b32 vnocanon(VTty *y)
{
    return y->mode==TTY_RAW || y->mode==TTY_CHARS || y->mode==TTY_CHARSNOECHO;
}

static void vttyecho(Vos *v, u8 c)
{
    if (v->tty.mode == TTY_NOECHO) return;
    if (c<0x20 && c!='\t' && c!='\n') {
        u8 s[2] = {'^', (u8)(c + '@')};
        vttyout(v, s, 2);
    } else {
        vttyout(v, &c, 1);
    }
}

// Erase the last character of the line being typed, on screen too.
static void vttyrubout(Vos *v)
{
    VTty *y = &v->tty;
    if (!y->linelen) return;
    iz end = y->linelen;
    iz beg = end - 1;
    for (; beg>0 && (y->line[beg]&0xc0)==0x80; beg--) {}
    iz  i = beg;
    i32 c = utf8decode({y->line, end}, &i);
    i32 w = c<0x20 ? 2 : cpwidth(c);
    for (i32 k = 0; k<w && y->mode!=TTY_NOECHO; k++) vttyout(v, (u8 *)"\b \b", 3);
    y->linelen = beg;
}

// One byte typed at the terminal in its cooked mode.
static void vcooked(Os *os, Vos *v, u8 c)
{
    VTty *y = &v->tty;
    c = c=='\r' ? '\n' : c;
    switch (c) {
    case 0x03:
        vttyout(v, (u8 *)"^C", 2);
        y->linelen = 0;
        vttysignal(os, v, 2);
        return;
    case 0x04:
        if (!y->linelen) {
            y->eof = 1;
            return;
        }
        break;
    case 0x7f:
        vttyrubout(v);
        return;
    case 0x15:
        while (y->linelen) vttyrubout(v);
        return;
    case 0x17:
        while (y->linelen && y->line[y->linelen-1]==' ') vttyrubout(v);
        while (y->linelen && y->line[y->linelen-1]!=' ') vttyrubout(v);
        return;
    default:
        if (y->linelen < countof(y->line)) y->line[y->linelen++] = c;
        vttyecho(v, c);
        if (c != '\n') return;
    }
    iz n = y->linelen;
    n = n<countof(y->ready)-y->readylen ? n : countof(y->ready)-y->readylen;
    copybytes(y->ready+y->readylen, y->line, n);
    y->readylen += n;
    y->linelen   = 0;
}

static b32 vready(Vos *, VProc *);

// Whether a process other than the current one can run, the terminal
// left out (a snapshot waits for everything else to settle).
static b32 vothersready(Vos *v)
{
    v->tty.settling = 1;
    b32 r = 0;
    for (i32 i = 0; i<v->nprocs && !r; i++) {
        r = i!=v->cur && vready(v, v->procs+i);
    }
    v->tty.settling = 0;
    return r;
}

// Whether a read from the terminal has something to do: keys or end
// of input to deliver, a snapshot to take.
static b32 vttyready(Vos *v)
{
    VTty *y = &v->tty;
    if (!y->term || y->settling) return 0;
    if (!vnocanon(y) && (y->readylen || y->eof)) return 1;
    if (y->next >= y->nkeys) return 1;
    VKey *k = y->keys + y->next;
    if (k->kind != VKEY_PAUSE) return 1;
    if (!y->until) y->until = v->clock + k->ms*1000000;
    return v->clock >= y->until;
}

// Are typed bytes waiting now? A zero-length read asks (read -t 0, the
// editor's typeahead): keys still to come, a snapshot or a resize next,
// are not yet typed.
static b32 vttyhas(Vos *v)
{
    VTty *y = &v->tty;
    if (!y->term || y->settling) return 0;
    if (!vnocanon(y)) return y->readylen || y->eof;
    for (iz i = y->next; i < y->nkeys; i++) {
        VKey *k = y->keys + i;
        if (k->kind != VKEY_TEXT) return 0;
        if ((i==y->next ? y->pos : 0) < k->text.len) return 1;
    }
    return 0;
}

// Typing ahead: while no process reads the terminal, as when they all
// wait for something else or a shell is busy, its line discipline
// takes the script's keys in its cooked mode (echo, ^C), the pauses
// passing in virtual time, up to anything that needs a reader (a
// snapshot, a resize, other modes). True if it took any.
static b32 vtype(Os *os, Vos *v)
{
    VTty *y = &v->tty;
    if (!y->term || y->settling || (y->mode && y->mode!=TTY_NOECHO)) return 0;
    for (i32 i = 0; i < v->nprocs; i++) {
        VProc *p = v->procs + i;
        if (p->state==VP_BLOCK && p->wait==VW_TTY) return 0;
    }
    b32 took = 0;
    while (y->next < y->nkeys) {
        VKey *k = y->keys + y->next;
        if (k->kind == VKEY_PAUSE) {
            if (!y->until) y->until = v->clock + k->ms*1000000;
            if (v->clock < y->until) break;
            y->until = 0;
        } else if (k->kind == VKEY_TEXT) {
            while (y->pos < k->text.len) vcooked(os, v, k->text[y->pos++]);
            y->pos = 0;
        } else {
            break;
        }
        y->next++;
        took = 1;
    }
    return took;
}

// Read typed keys: the count (raw: as typed, cooked: finished lines),
// 0 at the end of input, -1 if none yet, -2 if others must run first.
static iz vttyget(Os *os, Vos *v, u8 *buf, iz len)
{
    VTty *y = &v->tty;
    y->active = v->clock;
    for (;;) {
        if (!vnocanon(y) && y->readylen) {
            iz n = len<y->readylen ? len : y->readylen;
            copybytes(buf, y->ready, n);
            y->readylen -= n;
            for (iz i = 0; i < y->readylen; i++) y->ready[i] = y->ready[i+n];
            return n;
        }
        if (!vnocanon(y) && y->eof) {
            y->eof = 0;
            return 0;
        }
        if (y->next >= y->nkeys) {
            if (!y->ended) {
                if (vothersready(v)) return -2;
                vsnap(v);
                y->ended = 1;
            }
            return 0;
        }
        VKey *k = y->keys + y->next;
        switch (k->kind) {
        case VKEY_TEXT:
            if (y->pos == k->text.len) {
                y->next++;
                y->pos = 0;
            } else if (y->mode == TTY_RAW) {
                iz n = k->text.len - y->pos;
                n = n<len ? n : len;
                copybytes(buf, k->text.data+y->pos, n);
                y->pos += n;
                return n;
            } else if (vnocanon(y)) {
                // A byte at a time, with ^C, CR as \n and the echo
                u8 c = k->text[y->pos++];
                c = c=='\r' ? '\n' : c;
                if (c == 0x03) {
                    vttyout(v, (u8 *)"^C", 2);
                    vttysignal(os, v, 2);
                    continue;
                }
                if (y->mode == TTY_CHARS) vttyecho(v, c);
                buf[0] = c;
                return 1;
            } else {
                vcooked(os, v, k->text[y->pos++]);
            }
            continue;
        case VKEY_PAUSE:
            if (!y->until) y->until = v->clock + k->ms*1000000;
            if (v->clock < y->until) return -1;
            y->until = 0;
            y->next++;
            continue;
        case VKEY_SNAP:
            if (vothersready(v)) return -2;
            vsnap(v);
            y->next++;
            continue;
        case VKEY_RESIZE:
            vtresize(y->term, &v->a, k->rows, k->cols);
            for (i32 i = 0; i < v->nprocs; i++) {
                VProc *p = v->procs + i;
                if (p->shell && (p->state==VP_RUN || p->state==VP_BLOCK)) {
                    os_kill(os, p->pid, 28);
                    p->intr = 1;  // the reader's own read too
                }
            }
            y->next++;
            return -2;  // the reader sees its signal before more keys
        }
    }
}

// A read from the terminal. Nobody types at one without a script: it
// blocks until a signal fails it, or the scheduler finds nothing else
// can ever run.
static iz vttyread(Os *os, Vos *v, u8 *buf, iz len)
{
    VProc *me = v->procs + v->cur;
    for (;;) {
        if (me->intr) {
            me->intr = 0;
            return -1;
        }
        if (v->tty.term) {
            iz n = vttyget(os, v, buf, len);
            if (n >= 0) return n;
            if (n == -2) {
                vyield(os, v);
                continue;
            }
        }
        me->wait = VW_TTY;
        if (!vschedule(os, v)) {
            vdeadlock(v);
            return -1;
        }
    }
}

static b32 vistty(Vos *v, Handle h)
{
    if (h < 3) return h==0 ? v->ttyin : !!v->tty.term;
    VFile *f = vfile(v, h);
    return f && f->kind==VK_TTY;
}

static Tty os_tty(Os *os, Handle in, Handle out, i32 how)
{
    Vos *v = osvos(os);
    Tty  r = {};
    VTty *y = &v->tty;
    if (!y->term || !vistty(v, in) || !vistty(v, out)) {
        r.err = E_NOTTY;
        return r;
    }
    if (how == TTY_RESTORE) {
        y->mode = 0;
    } else if (how != TTY_QUERY) {
        y->mode = how;
    }
    r.rows   = y->term->rows;
    r.cols   = y->term->cols;
    r.xenl   = y->term->xenl;
    r.legacy = y->legacy;
    r.intr   = 0x03;
    r.eof    = 0x04;
    r.erase  = 0x7f;
    r.kill   = 0x15;
    r.werase = 0x17;
    r.lnext  = 0x16;
    return r;
}

// The legacy console's primitives over the terminal's screen, which
// knows only reverse video and bold of the attributes
static b32 os_con(Os *os, Handle out, ConOp *o)
{
    Vos  *v = osvos(os);
    VTty *y = &v->tty;
    if (!y->term || !y->legacy || !vistty(v, out)) return 0;
    VTerm *t = y->term;
    y->active = v->clock;
    switch (o->op) {
    case CON_INFO:
        o->cols = t->cols;
        o->rows = t->rows;
        o->x    = t->col;
        o->y    = t->row;
        o->attr = 0x07;  // as at the start: vtcon reads it once
        break;
    case CON_MOVE:
        vtmoveto(t, o->y, o->x);
        break;
    case CON_WRITE:
        vttyout(v, o->text.data, o->text.len);
        break;
    case CON_FILL: {
        iz beg = (iz)o->y*t->cols + o->x;
        iz end = (iz)t->rows*t->cols;
        for (iz i = 0; i<o->n && beg+i<end; i++) t->cells[beg+i] = {};
    } break;
    case CON_ATTR: {
        // Black on a color is reverse video; the foreground's
        // intensity is bold
        b32 rev = !!(o->attr & 0x70);
        i32 fg  = rev ? o->attr>>4 : o->attr;
        t->attr = (u8)((rev ? VA_REVERSE : 0) | (fg&8 ? VA_BOLD : 0));
    } break;
    }
    return 1;
}

// The editor's check (EDIT_CHECK): what a redisplay leaves on the
// screen, from the prompt's row down, equals its layout drawn on a
// blank screen, with the cursor at point. Not checked: a line started
// past the left margin, over old text or with attributes on (as
// readline, it draws as though it had not), one that ever got taller
// than the screen (its
// top is lost) or that others wrote over (shell-expand-line's command
// substitutions), until a redraw from the margin; sideways scrolling.
static b32 edcellsame(VCell a, VCell b)
{
    if (a.c==' ' && !a.attr) a.c = 0;
    if (b.c==' ' && !b.attr) b.c = 0;
    return a.c==b.c && a.comb==b.comb && a.attr==b.attr;
}

static void edcheck(Editor *ed, Shell *sh, i32 how)
{
    Vos   *v = osvos(sh->os);
    VTty  *y = &v->tty;
    VTerm *t = y->term;
    if (!t || !vistty(v, ed->out)) return;
    switch (how) {
    case EDCHECK_START:
        // Off the margin, or over old text: output with escapes in it
        // (an error message quoting what was typed) can move the
        // cursor up, and the editor, as readline, draws a new line as
        // though on blank rows. A UTF-8 sequence cut short (a lead byte
        // typed ahead and echoed) becomes U+FFFD before the prompt.
        y->edoff = t->col!=0 || t->wrapnext || t->need;
        for (i32 i = t->row*t->cols; i < t->rows*t->cols; i++) {
            y->edoff |= t->cells[i].c > 0;
        }
        y->edattr    = t->attr != 0;
        y->edlost    = 0;
        y->edforeign = 0;
        return;
    case EDCHECK_MARGIN:
        // Drawn again from the margin (after a listing or a message),
        // as though on blank rows: unless something is left on them
        // (a line given up while the cursor was above its end)
        y->edoff = 0;
        for (i32 i = t->row*t->cols; i < t->rows*t->cols; i++) {
            y->edoff |= t->cells[i].c > 0;
        }
        y->edattr    = t->attr != 0;
        y->edlost    = 0;
        y->edforeign = 0;
        return;
    case EDCHECK_WRITE:
        y->edwriting = 1;
        return;
    case EDCHECK_WROTE:
        y->edwriting = 0;
        return;
    }
    Layout *l    = &ed->shown;
    i32     need = l->nrows;  // rows that must be on the screen
    while (need>1 && !l->rows[need-1].ncells && l->prow<need-1) {
        // Empty rows below the cursor (after a newline in the line, or
        // past a full row) are not drawn: the editor reaches them with
        // newlines, which scroll
        need--;
    }
    if (l->nrows > t->rows) y->edlost = 1;  // even an empty row may be drawn
    if (y->edoff || y->edattr || y->edlost || y->edforeign || ed->hscroll) return;
    if (t->rows!=ed->tty.rows || t->cols!=ed->tty.cols) return;  // unseen resize

    i32 top = t->row - ed->crow;
    if (top < 0) {
        // Its top scrolled away: as readline, a full last row's wrap
        // can take one more row than the screen has
        y->edlost = 1;
        return;
    }
    Arena  a = v->a;
    VTerm *r = vtnew(&a, l->nrows+t->rows, t->cols, t->xenl);
    for (i32 i = 0; i < l->nrows; i++) {
        vtmoveto(r, i, 0);
        r->wrapnext = 0;
        vtfeed(r, l->rows[i].text.data, l->rows[i].text.len);
    }
    if (y->legacy) {
        // vtcon draws no underline
        for (iz i = 0; i < (iz)r->rows*r->cols; i++) r->cells[i].attr &= ~VA_UNDER;
    }
    b32 ok = top+need <= t->rows;
    ok = ok && ed->crow==l->prow && ed->ccol==l->pcol && !ed->pending;
    ok = ok && t->col==l->pcol && !t->wrapnext;
    i32 badrow = -1;
    i32 badcol = -1;
    for (i32 row = top; ok && row < t->rows; row++) {
        for (i32 c = 0; ok && c < t->cols; c++) {
            ok = edcellsame(*vtcell(t, row, c), *vtcell(r, row-top, c));
            badrow = ok ? -1 : row;
            badcol = ok ? -1 : c;
        }
    }
    if (ok) return;
    if (y->edtrap) __builtin_trap();
    if (!y->edbad++) {
        v->a   = a;  // keeping r
        r->row = l->prow;
        r->col = l->pcol;
        Buf b(&v->a, 1<<12);
        print(&b, "-- screen (top ");
        print(&b, (i64)top);
        print(&b, ", editor at ");
        print(&b, (i64)ed->crow);
        print(&b, ",");
        print(&b, (i64)ed->ccol);
        print(&b, ed->pending ? Str("+") : "");
        if (badrow >= 0) {
            print(&b, ", cell ");
            print(&b, (i64)badrow);
            print(&b, ",");
            print(&b, (i64)badcol);
        }
        print(&b, ")\n");
        print(&b, vtdump(t, &v->a));
        print(&b, "-- redrawn\n");
        print(&b, vtdump(r, &v->a));
        y->edwhy = finish(&b);
    }
}

// Give the outermost shell a terminal typed at by a script, rows by
// cols, snapshots going to a buffer of snapcap bytes.
static void vttyinit(Vos *v, i32 rows, i32 cols, b32 xenl, VKey *keys, iz nkeys,
                     u8 *snapbuf, iz snapcap)
{
    VTty *y    = &v->tty;
    *y         = {};
    y->term    = vtnew(&v->a, rows, cols, xenl);
    y->keys    = keys;
    y->nkeys   = nkeys;
    y->snaps   = {snapbuf, 0};
    y->snapcap = snapcap;
    v->ttyin   = 1;
}

static Opened vnpopen(Vos *, Str name, i32 flags);

static Opened os_open(Os *os, Arena scratch, Str path, i32 flags, i32 mode)
{
    Vos   *v = osvos(os);
    Opened r = {};
    r.h = BADHANDLE;
    v->clock += 1000;
    if (v->win) {
        VPath p = vpath(v, &scratch, path);
        switch (p.kind) {
        case VN_NULL: path = "/dev/null"; break;
        case VN_PIPE: return vnpopen(v, p.path, flags);
        case VN_NONE: r.err = E_NOENT; return r;
        }
    }
    i32 fd = v->win ? -1 : vdevfd(path);
    if (fd>=0 && v->cur) {
        // As Linux, /dev/fd/N reopens the process's descriptor: a pipe
        // end as a duplicate, a file as a new description of it
        Handle h = fd<countof(v->procs[0].ctx.fd) ? v->procs[v->cur].ctx.fd[fd] : BADHANDLE;
        VFile *f = vfile(v, h);
        if (!f && h<3) {
            r.h = os_dup(os, h);
            return r;
        } else if (!f) {
            r.err = E_NOENT;
            return r;
        } else if (f->kind!=VK_FILE && f->kind!=VK_DIR) {
            r.h = os_dup(os, h);
            return r;
        }
        Handle n = vnewhandle(v);
        if (n == BADHANDLE) {
            r.err = E_MFILE;
            return r;
        }
        if (flags & (OF_WRITE|OF_TRUNC)) vmutate(v);
        VFile *nf = v->files + n;
        nf->kind  = f->kind;
        nf->flags = flags;
        nf->node  = f->node;
        if ((flags&OF_TRUNC) && f->node->type==FT_FILE) {
            f->node->data.len = 0;
            f->node->mtime    = v->clock;
            f->node->ctime    = v->clock;
        }
        r.h = n;
        return r;
    }
    if (path=="/dev/null" || (path=="/dev/tty" && !v->win)) {
        Handle h = vnewhandle(v);
        if (h == BADHANDLE) { r.err = E_MFILE; return r; }
        v->files[h].kind  = path=="/dev/tty" ? VK_TTY : VK_NULL;
        v->files[h].flags = flags;
        r.h = h;
        return r;
    }
    VResolved res = vresolve(v, &scratch, path, 1);
    VNode *n = res.node;
    if (flags & (OF_WRITE|OF_CREATE|OF_TRUNC)) vmutate(v);
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
        n->ctime    = v->clock;
    }
    r.h = h;
    return r;
}

static Opened os_tempfile(Os *os, Arena)
{
    // A node in no directory, as an unlinked file: bash's heredoc file
    // is left readable only by its owner
    Vos   *v = osvos(os);
    Opened r = {};
    v->clock += 1000;
    r.h = vnewhandle(v);
    if (r.h == BADHANDLE) {
        r.err = E_MFILE;
        return r;
    }
    VNode *n = vnew(v, 0, "", FT_FILE, 0400);
    n->nlink = 0;
    VFile *f = v->files + r.h;
    f->kind  = VK_FILE;
    f->flags = OF_READ|OF_WRITE;
    f->node  = n;
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
        if (v->trace) vtraceout(v, h, buf, len);
        if (v->tty.term) vttyout(v, buf, len);
        Str *dst = h==1 ? &v->out : &v->err;
        if (len > v->outcap - dst->len) {
            // As a reader gone: which task it cuts off is up to timing
            if (v->trace && vconcurrent(v)) v->racy = 1;
            v->outfull = 1;
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
    case VK_TTY:
        if (v->tty.term) vttyout(v, buf, len);
        return 1;
    case VK_NULL:
        return 1;
    case VK_FILE:
        if (!(f->flags & OF_WRITE)) return 0;
        if (f->flags & OF_APPEND) f->pos = f->node->data.len;
        if (len > ((iz)1<<26) - f->pos) return 0;  // file too large
        vmutate(v);
        vwritenode(v, f->node, f->pos, buf, len);
        f->pos += len;
        return 1;
    case VK_PIPEWRITE: {
        VPipe *p  = f->pipe;
        VProc *me = v->procs + v->cur;
        vtracewrite(v, p);
        if (v->chaos && vrand(v)%4==0) vyield(os, v);
        while (len > 0) {
            if (!p->readers) {
                v->racy = 1;  // whether the reader was gone yet
                if (!me->shell && !(me->sigign>>12 & 1)) {
                    vexit(os, v, WSIGNALED|(128+13));  // SIGPIPE
                }
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
    if (vistty(v, h)) {
        return vttyread(os, v, buf, len);
    }
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
        if (f->node->atime <= f->node->mtime) f->node->atime = v->clock;
        return n;
    }
    case VK_PIPEREAD: {
        VPipe *p  = f->pipe;
        VProc *me = v->procs + v->cur;
        if (v->chaos && vrand(v)%4==0) vyield(os, v);
        while (p->len==p->pos && p->writers) {
            if (me->intr) {
                me->intr = 0;  // a signal arrived: the shell takes it
                return -1;
            }
            me->wait  = VW_READ;
            me->wpipe = p;
            if (!vschedule(os, v)) {
                vdeadlock(v);
                return -1;
            }
        }
        return vpiperead(v, p, buf, len);
    }
    }
    return -1;
}

static iz os_peek(Os *os, Handle h, u8 *buf, iz len)
{
    Vos   *v = osvos(os);
    VFile *f = h ? vfile(v, h) : 0;
    if (!f || f->kind!=VK_PIPEREAD || vistty(v, h)) return -1;
    VPipe *p = f->pipe;
    iz     n = p->len - p->pos;
    n = n<len ? n : len;
    copybytes(buf, p->buf+p->pos, n);
    return n;
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

// Windows mode: names the Win32 layer marks executable by extension.
static b32 vexename(Str name)  // taken to start with MZ, as Win32's os_stat
{
    Str exe = ".exe";
    return name.len>=exe.len && equalfold(taketail(name, exe.len), exe);
}

static Stat vstat(Vos *v, VNode *n)
{
    Stat r = {};
    r.type  = n->type;
    r.mode  = n->mode;
    r.nlink = n->nlink;
    r.size  = n->data.len;
    r.mtime = n->mtime;
    r.atime = n->atime;
    r.ctime = n->ctime;
    r.blocks = (r.size + 4095)/4096 * 8;  // as ext4 with 4 KiB blocks
    r.dev   = 1;
    r.ino   = n->ino;
    r.uid   = n->uid;
    r.gid   = n->uid;
    if (v->win && (n->type==FT_FILE || n->type==FT_DIR)) {
        // Synthesized as the Win32 layer does (fromattrs, os_stat): a
        // read-only file loses its write bits, and an .exe name or a
        // leading MZ, #! or ":\n" makes a file executable (the shell
        // adds the rules of its path mode)
        r.mode = n->type==FT_DIR ? 0755 : 0644;
        if (n->type == FT_FILE) {
            Str d = n->data;
            b32 magic = d.len>=2 && ((d[0]=='M' && d[1]=='Z') || (d[0]=='#' && d[1]=='!')
                                     || (d[0]==':' && d[1]=='\n'));
            r.mode &= n->mode&0200 ? 0777 : ~0222;
            r.mode |= vexename(n->name) || magic ? 0111 : 0;
        }
        r.uid = r.gid = 1000;
    }
    return r;
}

static Stat os_stat(Os *os, Arena scratch, Str path, b32 follow)
{
    Vos *v = osvos(os);
    if (v->win) {
        VPath p = vpath(v, &scratch, path);
        Stat  r = {};
        switch (p.kind) {
        case VN_NULL:
            path = "/dev/null";
            break;
        case VN_PIPE:
            r.err = E_NOENT;
            for (i32 i = 0; i < v->nnpipes; i++) {
                VPipe *q = v->npipes[i].pipe;
                if (equalfold(v->npipes[i].name, p.path) && (q->readers || q->writers)) {
                    r = {};
                    r.type = FT_PIPE;
                    r.mode = 0600;
                    r.uid  = r.gid = 1000;
                }
            }
            return r;
        case VN_NONE:
            r.err = E_NOENT;
            return r;
        }
    }
    i32 fd = v->win ? -1 : vdevfd(path);
    if (fd>=0 && v->cur) {
        Handle h = fd<countof(v->procs[0].ctx.fd) ? v->procs[v->cur].ctx.fd[fd] : BADHANDLE;
        if (h<3 || vfile(v, h)) return os_fstat(os, h);
        Stat r = {};
        r.err = E_NOENT;
        return r;
    }
    if (path=="/dev/null" || (path=="/dev/tty" && !v->win)) {
        Stat r = {};
        r.type = FT_CHAR;
        r.mode = 0666;
        r.dev  = 2;
        r.ino  = path=="/dev/tty" ? 2 : 1;
        return r;
    }
    VResolved res = vresolve(v, &scratch, path, follow);
    if (!res.node) {
        Stat r = {};
        r.err = res.err;
        return r;
    }
    return vstat(v, res.node);
}

static b32 os_isatty(Os *os, Handle h)
{
    return vistty(osvos(os), h);
}

static Stat os_fstat(Os *os, Handle h)
{
    Vos *v = osvos(os);
    Stat r = {};
    if (h < 3) {
        r.type = vistty(v, h) ? FT_CHAR : FT_PIPE;
        r.mode = 0600;
        r.uid  = r.gid = v->user;  // the user's terminal or pipes
        return r;
    }
    VFile *f = vfile(v, h);
    if (!f) {
        r.err = E_BADF;
    } else if (f->node) {
        r = vstat(v, f->node);
    } else if (f->kind==VK_NULL || f->kind==VK_TTY) {
        r.type = FT_CHAR;
        r.mode = 0666;
    } else {
        r.type = FT_PIPE;
        r.mode = 0600;
        r.uid  = r.gid = v->user;
    }
    return r;
}

// NTFS order: by name, ignoring case as upper case.
static b32 vntfsless(Dirent **a, Dirent **b)
{
    Str x = (*a)->name;
    Str y = (*b)->name;
    for (iz i = 0; i<x.len && i<y.len; i++) {
        u8 c = upcase(x[i]);
        u8 d = upcase(y[i]);
        if (c != d) return c < d;
    }
    return x.len < y.len;
}

// Directory entries in creation order, as a fresh tmpfs lists them, or
// in Windows mode in NTFS's order.
static Dirent *os_list(Os *os, Arena *a, Str dir, i32 *err)
{
    Vos *v = osvos(os);
    VResolved res = vresolve(v, a, dir, 1);
    *err = res.err;
    if (!res.node) return 0;
    if (res.node->type != FT_DIR) {
        *err = E_NOTDIR;
        return 0;
    }
    if (!(vperm(v, res.node) & 4)) {
        *err = E_ACCES;
        return 0;
    }
    // Newest first, as tmpfs lists them, where specs are recorded
    Dirent  *head = 0;
    Dirent **tail = &head;
    for (VNode *n = res.node->child; n; n = n->next) {
        Dirent *d = alloc<Dirent>(a);
        d->name = clone(a, n->name);
        d->type = n->type;
        *tail = d;
        tail  = &d->next;
    }
    if (v->win && head) {
        iz n = 0;
        for (Dirent *d = head; d; d = d->next) n++;
        Dirent **all = alloc<Dirent *>(a, n);
        n = 0;
        for (Dirent *d = head; d; d = d->next) all[n++] = d;
        sort(all, n, vntfsless, *a);
        for (iz i = 0; i < n; i++) all[i]->next = i+1<n ? all[i+1] : 0;
        head = all[0];
    }
    return head;
}

static i64 os_now(Os *os)
{
    Vos *v = osvos(os);
    return v->clock += 1000;
}

// The system zone: UTC as in the oracle's container, unless a test
// configures another (a spec's "# localtz:" line).
static Str os_localtz(Os *os, Arena *)
{
    Vos *v = osvos(os);
    return v->localtz.data ? v->localtz : Str("UTC0");
}

// Signals. A shell receives every signal as a pending bit, and its core
// decides what it does: run a trap, ignore it, or die. A fake does what
// a program with default dispositions does, except for the signals its
// spawner made it ignore. Stopping is not modelled: stop signals, like
// the ones ignored by default, do nothing to a fake.
static i32 os_kill(Os *os, i64 pid, i32 sig)
{
    Vos *v = osvos(os);
    if (sig<0 || sig>=NSIGNAL) return E_INVAL;
    VProc *p = 0;
    for (i32 i = 0; i < v->nprocs; i++) {
        VProc *q = v->procs + i;
        if (q->pid==pid && (q->state==VP_RUN || q->state==VP_BLOCK)) p = q;
    }
    if (!p || pid<=0) return E_SRCH;
    if (!sig) return 0;
    if (p->shell) {
        p->sigpend |= (u64)1 << (sig-1);
        p->intr = p != v->procs+v->cur;  // fail its blocked read or wait
        return 0;
    }
    switch (sig) {
    case 17: case 18: case 19: case 20: case 21: case 22: case 23: case 28:
        return 0;  // CHLD CONT STOP TSTP TTIN TTOU URG WINCH
    }
    if (sig!=9 && (p->sigign>>(sig-1) & 1)) return 0;
    vkill(os, v, p, WSIGNALED|(128+sig));
    return 0;
}

static i32 os_siglevel(Os *os, i64 pid, i64 level, i32 sig)
{
    Vos *v = osvos(os);
    if (sig<0 || sig>=NSIGNAL) return E_INVAL;
    VProc *p = 0;
    for (i32 i = 0; i < v->nprocs; i++) {
        VProc *q = v->procs + i;
        if (q->pid==pid && (q->state==VP_RUN || q->state==VP_BLOCK)) p = q;
    }
    if (!p || !p->shell || pid<=0) return E_SRCH;
    if (!sig) return 0;
    for (i32 i = 0; i < p->nlsigs; i++) {
        if (p->lsigs[i].level==level && p->lsigs[i].sig==sig) return 0;
    }
    if (p->nlsigs == countof(p->lsigs)) return E_AGAIN;
    p->lsigs[p->nlsigs].level = level;
    p->lsigs[p->nlsigs].sig   = sig;
    p->nlsigs++;
    p->intr = p != v->procs+v->cur;  // fail its blocked read or wait
    return 0;
}

static LevelSig os_levelsig(Os *os)
{
    Vos     *v  = osvos(os);
    VProc   *me = v->procs + v->cur;
    LevelSig r  = {};
    if (me->nlsigs) {
        r = me->lsigs[0];
        me->nlsigs--;
        for (i32 i = 0; i < me->nlsigs; i++) me->lsigs[i] = me->lsigs[i+1];
    }
    return r;
}

static u64 os_sigpending(Os *os)
{
    Vos   *v  = osvos(os);
    VProc *me = v->procs + v->cur;
    u64    r  = me->sigpend;
    me->sigpend = 0;
    return r;
}

static u64 os_siggroup(Os *os)
{
    Vos   *v  = osvos(os);
    VProc *me = v->procs + v->cur;
    VTty  *y  = &v->tty;
    u64    r  = me->grpsig;
    me->grpsig = 0;
    if (y->term && !y->ended && !vothersready(v)) {
        // A busy shell: a millisecond passes at each look, and keys
        // are typed once the terminal has been quiet a while, as
        // tools/ptyrun waits for. Past the last of them, the final
        // snapshot, and the shell is killed, as ptyrun does.
        v->clock += 1000000;
        if (y->active && v->clock-y->active>=300*1000000ll) {
            if (y->next<y->nkeys && y->keys[y->next].kind==VKEY_SNAP) {
                vsnap(v);
                y->next++;
                y->active = v->clock;
            } else if (y->next < y->nkeys) {
                vtype(os, v);
            } else {
                vsnap(v);
                y->ended = 1;
                r |= (u64)1 << (9-1);
            }
        }
    }
    return r;
}

static void os_tasktag(Os *os, u64 tag, iz ntasks, u64 group)
{
    Vos *v = osvos(os);
    v->procs[v->cur].tag    = tag;
    v->procs[v->cur].ntasks = ntasks;
    v->procs[v->cur].group  = group;
}

static void os_racy(Os *os)
{
    osvos(os)->racy = 1;
}

static void os_sigmode(Os *, i32, i32)
{
    // shells take every signal through os_sigpending
}

enum { VIMAGE = 1024 };  // size of a fake program's file, mostly NULs
static u8 const vmagic[12] = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0, 'f', 'a', 'k', 'e'};

// Windows mode: a fake's file is a PE image (vpeimage) whose .text
// section starts with this mark and the program's number.
enum { VPEIMAGE = 0xc00, VPEMARK = 0x400 };
static u8 const vpemagic[8] = {'v', 'o', 's', 'f', 'a', 'k', 'e', 0};

// The program a file holds: installed as such, or a copy of one.
static i32 vimageprog(Vos *v, VNode *n)
{
    if (n->prog) return n->prog;
    Str  d = n->data;
    if (v->win) {
        Str mark = {(u8 *)vpemagic, countof(vpemagic)};
        if (d.len<VPEMARK+10 || d[0]!='M' || d[1]!='Z' ||
                !(Str{d.data+VPEMARK, mark.len} == mark)) {
            return 0;
        }
        i32 prog = d[VPEMARK+8] | d[VPEMARK+9]<<8;
        return prog<=v->nfakes ? prog : 0;
    }
    if (d.len<14 || !(takehead(d, 12) == Str((u8 *)vmagic, 12))) {
        return 0;
    }
    i32 prog = d[12] | d[13]<<8;
    return prog<=v->nfakes ? prog : 0;
}

struct VProgram {
    i32    prog;    // index into the program table, or -1
    i32    err;
    VNode *node;
    Str    native;  // Windows mode: the image's canonical path
};

// Look up a program by its path's final component when the path names
// an executable fake in the tree. Windows has no execute bit: any
// image runs.
static VProgram vprogram(Vos *v, Arena *a, Str path)
{
    VProgram  r   = {};
    r.prog = -1;
    VResolved res = vresolve(v, a, path, 1);
    if (!res.node) {
        r.err = res.err;
        return r;
    }
    VNode *n = res.node;
    if (n->type == FT_DIR) {
        r.err = E_ACCES;
        return r;
    }
    if (!v->win && !(n->mode & 0100)) {
        r.err = E_ACCES;
        return r;
    }
    i32 prog = vimageprog(v, n);
    if (!prog) {
        r.err = E_NOEXEC;
        return r;
    }
    r.prog   = prog - 1;
    r.node   = n;
    r.native = v->win ? vpath(v, a, path).native : path;
    return r;
}


// Windows personality: images, command lines and named pipes

static void vput(u8 *p, u64 x, i32 n)
{
    for (i32 i = 0; i < n; i++) {
        p[i] = (u8)(x >> (8*i));
    }
}

// A minimal PE image for a fake, as pesniff reads them: headers, a
// .text section holding the vos mark and the program number, .idata
// importing one DLL, and our own .unabash section when ours is set.
// Machine 0x14c makes a PE32 image, others PE32+.
static Str vpeimage(Arena *a, i32 machine, Str dll, b32 ours, i32 prog)
{
    u8 *img = alloc<u8>(a, VPEIMAGE);
    b32 pe64 = machine != 0x14c;
    img[0] = 'M';
    img[1] = 'Z';
    iz pe = 0x80;
    vput(img+0x3c, pe, 4);
    vput(img+pe, 0x4550, 4);
    i32 nsect   = ours ? 3 : 2;
    i32 optsize = pe64 ? 240 : 224;
    vput(img+pe+4,  (u64)machine, 2);
    vput(img+pe+6,  (u64)nsect, 2);
    vput(img+pe+20, (u64)optsize, 2);
    vput(img+pe+22, 0x0102, 2);  // executable, 32-bit words
    iz opt  = pe + 24;
    iz dirs = opt + (pe64 ? 112 : 96);
    vput(img+opt, pe64 ? 0x20b : 0x10b, 2);
    vput(img+opt+60, 0x400, 4);  // SizeOfHeaders
    vput(img+opt+68, 3, 2);      // console subsystem
    vput(img+dirs-4, 16, 4);     // NumberOfRvaAndSizes

    struct { Str name; u32 va, vsize, rawsize, rawptr; } sects[] = {
        {".text",    0x1000, 0x200, 0x200, 0x400},
        {".idata",   0x2000, 0x300, 0x400, 0x600},
        {".unabash", 0x3000, 0x10,  0x200, 0xa00},
    };
    iz sect = opt + optsize;
    for (i32 i = 0; i < nsect; i++) {
        u8 *s = img + sect + 40*i;
        copybytes(s, sects[i].name.data, sects[i].name.len);
        vput(s+ 8, sects[i].vsize, 4);
        vput(s+12, sects[i].va, 4);
        vput(s+16, sects[i].rawsize, 4);
        vput(s+20, sects[i].rawptr, 4);
    }

    // One import descriptor and a null one, then the DLL's name
    vput(img+dirs+8,  0x2000, 4);
    vput(img+dirs+12, 40, 4);
    vput(img+0x600+12, 0x2000+40, 4);
    copybytes(img+0x600+40, dll.data, dll.len<200 ? dll.len : 200);

    copybytes(img+VPEMARK, (u8 *)vpemagic, countof(vpemagic));
    vput(img+VPEMARK+8, (u64)prog, 2);
    if (ours) {
        copybytes(img+0xa00, (u8 *)"unabashed", 9);
    }
    Str r = {img, VPEIMAGE};
    return r;
}

static iz vperead(void *ctx, i64 off, u8 *buf, iz len)
{
    Str *s = (Str *)ctx;
    if (off<0 || off>=s->len) return 0;
    iz n = s->len-off<len ? s->len-(iz)off : len;
    copybytes(buf, s->data+off, n);
    return n;
}

static PeInfo vpesniff(VNode *n, Arena scratch)
{
    Str data = n->data;
    return pesniff(vperead, &data, scratch);
}

// The value of a variable in "NAME=value" strings, its name compared
// as Windows does (ignoring case), or null data.
static Str venvget(Str *env, iz nenv, Str name)
{
    for (iz i = 0; i < nenv; i++) {
        Str n = envname(env[i]);
        if (equalfold(n, name) && n.len<env[i].len) {
            return cuthead(env[i], n.len+1);
        }
    }
    return {};
}

static b32 vbatch(Str path)
{
    Str exts[] = {".bat", ".cmd"};
    for (iz i = 0; i < countof(exts); i++) {
        if (path.len>=4 && equalfold(taketail(path, 4), exts[i])) return 1;
    }
    return 0;
}

// Windows mode: the command line a spawned program gets, and the image
// that runs it.
struct VWinCmd {
    Str path;     // the program: ComSpec for a batch file
    Str cmdline;
    b32 split;    // the target splits cmdline, else it takes Spawn's argv
    i32 err;
};

// With Spawn.cmdline, that line verbatim. Without, as the Win32 layer
// builds one today (main_win32.cpp's os_spawn): batch files through
// ComSpec with BatBadBut quoting, others with MSVCRT quoting.
static VWinCmd vwincmd(Vos *v, Arena *a, Spawn *s)
{
    VWinCmd r = {};
    r.path = s->path;
    if (s->cmdline.len) {
        r.cmdline = s->cmdline;
        r.split   = 1;
    } else if (vbatch(s->path)) {
        Str comspec = venvget(s->env, s->nenv, "comspec");
        if (!comspec.len) {
            comspec = "C:\\Windows\\System32\\cmd.exe";  // GetSystemDirectory
        }
        // cmd would report a missing script itself, with status 1
        VResolved res = vresolve(v, a, s->path, 1);
        if (!res.node) {
            r.err = res.err;
            return r;
        } else if (res.node->type == FT_DIR) {
            r.err = E_ACCES;
            return r;
        }
        Str script = slashify(a, vpath(v, a, s->path).native, '\\');
        Buf b(a);
        quoteprogram(&b, comspec);
        print(&b, " /d /e:on /v:off /s /c \"");
        quotebatarg(&b, script, 1);
        for (iz i = 1; i < s->argc; i++) {
            putbyte(&b, ' ');
            if (!quotebatarg(&b, s->argv[i], 0)) {
                r.err = E_INVAL;
                return r;
            }
        }
        print(&b, " & exit\"");
        r.cmdline = finish(&b);
        r.path    = comspec;
        r.split   = 1;
    } else {
        Buf b(a);
        quoteprogram(&b, s->argc ? s->argv[0] : s->path);
        for (iz i = 1; i < s->argc; i++) {
            putbyte(&b, ' ');
            quotearg(&b, s->argv[i]);
        }
        r.cmdline = finish(&b);
    }
    if (towide(a, r.cmdline).len > 32766) {
        r.err = E_2BIG;
    }
    return r;
}

// A relative name made absolute against a native working directory.
static Str vabs(Arena *a, Str cwd, Str name)
{
    if (name.len && (pathsep(name[0]) || hasdrive(name))) return name;
    return concat(a, concat(a, cwd, "/"), name.len ? name : Str("."));
}

static b32 vwild(Str part)  // an unescaped wildcard
{
    for (iz i = 0; i < part.len; i++) {
        if (part[i] == '\\') {
            i++;
        } else if (part[i]=='*' || part[i]=='?' || part[i]=='[') {
            return 1;
        }
    }
    return 0;
}

static b32 vstrless(Str *a, Str *b)
{
    return compare(*a, *b) < 0;
}

// glob(3) below shown, the names as typed so far, for the rest of the
// pattern: each match in order. A match must exist.
static Slice<Str> vglobpart(Vos *v, Arena *a, Slice<Str> out, Str cwd, Str shown, Str rest)
{
    if (!rest.len) {
        if (vresolve(v, a, vabs(a, cwd, shown), 0).node) {
            out = push(a, out, shown);
        }
        return out;
    }
    Cut c   = cut(rest, '/');
    Str sep = c.ok ? Str("/") : Str("");
    if (!vwild(c.head)) {
        Str next = concat(a, concat(a, shown, cygunquote(a, c.head)), sep);
        return vglobpart(v, a, out, cwd, next, c.tail);
    }
    VResolved d = vresolve(v, a, vabs(a, cwd, shown), 1);
    if (!d.node || d.node->type!=FT_DIR) return out;
    Slice<Str> names = {};
    for (VNode *n = d.node->child; n; n = n->next) {
        if (patmatch(c.head, n->name, PM_PERIOD, *a)) {
            names = push(a, names, n->name);
        }
    }
    sort(names.data, names.len, vstrless, *a);
    for (iz i = 0; i < names.len; i++) {
        Str next = concat(a, concat(a, shown, names[i]), sep);
        out = vglobpart(v, a, out, cwd, next, c.tail);
    }
    return out;
}

// Cygwin's glob for a CYG_GLOB word: braces, a leading ~ (HOME, when
// it names a directory), wildcards over the tree, and the unquoted
// pattern when nothing matches (GLOB_NOCHECK).
static Slice<Str> vcygglob(Vos *v, Arena *a, Slice<Str> out, Str pat, Str cwd, Str home)
{
    Slice<Str> alts = cygbraces(a, {}, pat, (iz)1<<12);
    for (iz k = 0; k < alts.len; k++) {
        Str p = alts[k];
        if (p.len && p[0]=='~' && (p.len==1 || p[1]=='/') && home.len) {
            VResolved h = vresolve(v, a, home, 1);
            if (h.node && h.node->type==FT_DIR) {
                Buf b(a);
                for (iz i = 0; i < home.len; i++) {
                    if (find("\\*?[{}", {home.data+i, 1}) >= 0) putbyte(&b, '\\');
                    putbyte(&b, home[i]);
                }
                print(&b, cuthead(p, 1));
                p = finish(&b);
            }
        }
        iz before = out.len;
        out = vglobpart(v, a, out, cwd, {}, p);
        if (out.len == before) {
            out = push(a, out, cygunquote(a, p));
        }
    }
    return out;
}

// The arguments a Cygwin program makes of its command line: response
// files read from the tree (up to 10), then globbing.
static Slice<Str> vcygargs(Vos *v, Arena *a, Slice<Str> out, Str cmd, b32 program,
                           Str cwd, Str home, i32 *nfiles)
{
    Slice<CygArg> args = splitcygwin(a, cmd, program);
    for (iz i = 0; i < args.len; i++) {
        CygArg r = args[i];
        if (r.flags & CYG_ATFILE) {
            VResolved f = vresolve(v, a, vabs(a, cwd, r.file), 1);
            if (f.node && f.node->type==FT_FILE && (f.node->mode&0400) && ++*nfiles<=10) {
                Str more = concat(a, concat(a, f.node->data, " "), cuthead(cmd, r.end));
                return vcygargs(v, a, out, more, 0, cwd, home, nfiles);
            }
        }
        if (r.flags & CYG_GLOB) {
            out = vcygglob(v, a, out, r.pat, cwd, home);
        } else {
            out = push(a, out, r.arg);
        }
    }
    return out;
}

// What a Windows program sees: its command line, image path, working
// directory (POSIX for an MSYS2 program, where C:/x is /c/x) and its
// arguments, split by its own startup code when the command line is
// what it gets. Everything goes into the program's memory.
static void vwinctx(Vos *v, Arena *mem, FakeCtx *ctx, Spawn *s, VWinCmd *wc, VProgram *g)
{
    PeInfo pe = vpesniff(g->node, *mem);
    ctx->win     = 1;
    ctx->cmdline = clone(mem, wc->cmdline);
    ctx->exe     = clone(mem, g->native);
    VPath cwd = vpath(v, mem, s->cwd);
    Str   dir = cwd.kind==VN_FILE ? cwd.native : clone(mem, s->cwd);
    if (dir.len>3 && dir[dir.len-1]=='/') dir.len--;
    if (pe.posix && hasdrive(dir)) {
        Buf b(mem, dir.len+1);
        putbyte(&b, '/');
        putbyte(&b, lowercase(dir[0]));
        print(&b, dir.len>3 ? cuthead(dir, 2) : Str(""));
        ctx->cwd = finish(&b);
    } else {
        ctx->cwd = dir;
    }

    Slice<Str> argv = {};
    if (!wc->split) {
        for (iz i = 0; i < s->argc; i++) argv = push(mem, argv, clone(mem, s->argv[i]));
    } else if (pe.posix) {
        Str home   = venvget(s->env, s->nenv, "HOME");
        i32 nfiles = 0;
        argv = vcygargs(v, mem, argv, ctx->cmdline, 1, dir, home, &nfiles);
    } else {
        i32 parser = pe.crt==CRT_UCRT ? ARGV_UCRT : pe.crt==CRT_MSVCRT ? ARGV_MSVCRT : ARGV_SHELL32;
        argv = splitcmdline(mem, clone(mem, ctx->cmdline), parser);
    }
    ctx->argc = argv.len;
    ctx->argv = alloc<Str>(mem, argv.len+1);
    for (iz i = 0; i < argv.len; i++) ctx->argv[i] = argv[i];
}

// An instance of a pipe in the //./pipe/ namespace for one client, as
// Win32's CreateNamedPipe: the server holds one end, and the client
// opening the name (ignoring case) gets the other end of the oldest
// instance not yet taken. A fresh name is deterministic: the creator's
// pid and a sequence number. A slot is free again once both ends are
// closed. Unlike Win32's, an NP_NOBUF instance buffers what the server
// writes, like any pipe.
static NamedPipe os_namedpipe(Os *os, Arena *a, Str name, i32 how)
{
    b32 out = how & NP_OUT;
    Vos      *v = osvos(os);
    NamedPipe r = {};
    r.h = BADHANDLE;
    if (!v->win) {
        r.err = E_NOSYS;
        return r;
    }
    i32 i = 0;
    for (; i < v->nnpipes; i++) {
        VPipe *p = v->npipes[i].pipe;
        if (!p->readers && !p->writers) break;
    }
    if (i == countof(v->npipes)) {
        r.err = E_NFILE;
        return r;
    }
    OsPipe p = os_pipe(os);
    if (p.err) {
        r.err = p.err;
        return r;
    }
    os_close(os, out ? p.r : p.w);  // the client's, opened by name
    VNamedPipe *n = v->npipes + i;
    v->npipeseq++;
    if (name.data) {
        n->name = clone(&v->a, cuthead(name, Str("//./pipe/").len));
        r.name  = name;
    } else {
        Buf b(&v->a, 64);
        print(&b, "unabashed-");
        print(&b, v->procs[v->cur].pid);
        putbyte(&b, '-');
        print(&b, (i64)v->npipeseq);
        n->name = finish(&b);
        r.name  = concat(a, "//./pipe/", n->name);
    }
    n->pipe      = v->files[out ? p.w : p.r].pipe;
    n->seq       = v->npipeseq;
    n->out       = out;
    n->connected = 0;
    v->nnpipes  += i == v->nnpipes;
    r.h = out ? p.w : p.r;
    return r;
}

// The named pipe whose server end h is, or null.
static VNamedPipe *vserverpipe(Vos *v, Handle h)
{
    VFile *f = vfile(v, h);
    for (i32 i = 0; f && i<v->nnpipes; i++) {
        VNamedPipe *n = v->npipes + i;
        if (n->pipe==f->pipe && f->kind==(n->out ? VK_PIPEWRITE : VK_PIPEREAD)) {
            return n;
        }
    }
    return 0;
}

// A client opens a named pipe: the end its server does not hold of the
// oldest instance with none yet, for the access the pipe's direction
// allows (Win32: ERROR_PIPE_BUSY when all are taken,
// ERROR_ACCESS_DENIED).
static Opened vnpopen(Vos *v, Str name, i32 flags)
{
    Opened r = {};
    r.h   = BADHANDLE;
    r.err = E_NOENT;
    VNamedPipe *n = 0;
    for (i32 i = 0; i < v->nnpipes; i++) {
        VNamedPipe *c = v->npipes + i;
        VPipe      *p = c->pipe;
        if (!equalfold(c->name, name) || !(c->out ? p->writers : p->readers)) {
            continue;
        }
        r.err = E_BUSY;
        if (!c->connected && (!n || c->seq<n->seq)) n = c;
    }
    if (!n) return r;
    if (flags & (n->out ? OF_WRITE : OF_READ)) {
        r.err = E_ACCES;
        return r;
    }
    r.h = vnewhandle(v);
    if (r.h == BADHANDLE) {
        r.err = E_MFILE;
        return r;
    }
    r.err = 0;
    n->connected = 1;
    VFile *f = v->files + r.h;
    f->kind  = n->out ? VK_PIPEREAD : VK_PIPEWRITE;
    f->flags = flags;
    f->pipe  = n->pipe;
    if (n->out) {
        n->pipe->readers++;
    } else {
        n->pipe->writers++;
    }
    return r;
}

// Spawn a fake program as a process of its own, runnable at once. Its
// arguments are copied into its memory, since it outlives the call.
// The vos's processes share one address space: a child takes the
// region in place.
static ImageRef os_imagenew(Os *os, iz cap)
{
    ImageRef r = {};
    Vos     *v = osvos(os);
    if (!v->images) return r;
    // One that ended gives back its region, which the layer never
    // releases, so that loops of jobs run in bounded memory
    for (i32 i = 0; i < v->nfreeimgs; i++) {
        if (v->freeimgs[i].cap >= cap) {
            r = v->freeimgs[i];
            v->freeimgs[i] = v->freeimgs[--v->nfreeimgs];
            r.len = 0;
            __builtin_memset(r.base, 0x55, (uz)r.cap);  // as fresh from os_reserve
            return r;
        }
    }
    // The child runs from it after the nested shell that made it, and
    // whose memory the layer releases, has ended
    osvos(os)->imaging = 1;
    r.base = os_reserve(os, cap);
    osvos(os)->imaging = 0;
    r.cap  = r.base ? cap : 0;
    return r;
}

// An image whose spawn failed: its region goes back for reuse
static void os_imagefree(Os *os, ImageRef im)
{
    Vos *v = osvos(os);
    if (im.base && v->nfreeimgs<countof(v->freeimgs)) {
        v->freeimgs[v->nfreeimgs++] = im;
    }
}

#if _WIN32
extern "C" char __ImageBase;
#else
extern "C" char __executable_start, _end;
#endif

// An image must be closed (notes/m5b.md): no word in it may point into
// the shells' memory outside its own region, nor into this program's
// image (string constants, code), which a real child has elsewhere. A
// conservative scan of every aligned word: returns the offset of the
// first that does, or -1.
static iz vimageescape(Os *os, ImageRef im)
{
    iz    heaplen = 0;
    byte *heap    = osheap(os, &heaplen);
#if _WIN32
    byte *exe    = (byte *)&__ImageBase;
    i32   pe     = *(i32 *)(exe + 0x3c);
    byte *exeend = exe + *(u32 *)(exe + pe + 24 + 56);  // SizeOfImage
#else
    byte *exe    = (byte *)&__executable_start;
    byte *exeend = (byte *)&_end;
#endif
    uz lo = (uz)im.base, hi = (uz)im.base + (uz)im.cap;
    for (iz off = 0; off+(iz)sizeof(uz) <= im.len; off += (iz)sizeof(uz)) {
        uz w = *(uz *)(im.base + off);
        if (w>=lo && w<=hi) continue;  // the end included
        b32 inheap = w>=(uz)heap && w<(uz)heap+(uz)heaplen;
        b32 inexe  = w>=(uz)exe && w<(uz)exeend;
        if (inheap || inexe) return off;
    }
    return -1;
}

// Free the slots of the current process's children that have exited,
// their statuses kept for os_wait, as a shell's SIGCHLD handling reaps
// them however busy it is: a loop starting jobs from subshells, never
// blocking, would otherwise fill the table.
static void vreapexited(Os *os, Vos *v)
{
    for (Io *io = v->procs[v->cur].ios; io; io = io->next) {
        b32 child = io->op==IO_PROC && !io->plt[2] && io->h &&
                    io->h<(Handle)v->nprocs;
        if (child && v->procs[io->h].state==VP_EXIT) {
            io->n      = vreap(os, v, io->h);
            io->plt[2] = 1;
        }
    }
}

static Spawned os_spawn(Os *os, Arena scratch, Spawn *s)
{
    Vos    *v = osvos(os);
    Spawned r = {};
    r.proc = BADHANDLE;
    VWinCmd wc   = {};
    Str     path = s->path;
    if (v->win && !s->image.base) {
        wc = vwincmd(v, &scratch, s);
        if (wc.err) {
            r.err = wc.err;
            return r;
        }
        path = wc.path;
    }
    VProgram g = {};
    if (s->image.base) {
        iz off = vimageescape(os, s->image);
        if (off >= 0) {
            // A copier bug: fail loudly, in the spec's output
            Buf b(&scratch, 128);
            print(&b, "vos: image escapes its region at offset ");
            print(&b, fmti64(&scratch, off));
            print(&b, " to ");
            printhex(&b, *(uz *)(s->image.base + off), 16);
            print(&b, "\n");
            Str msg = finish(&b);
            for (i32 i = 0; i < 2; i++) {
                Str *dst = i ? &v->merged : &v->err;
                if (msg.len <= v->outcap - dst->len) {
                    copybytes(dst->data+dst->len, msg.data, msg.len);
                    dst->len += msg.len;
                }
            }
        }
        // This shell again, continuing from its image
        g.prog = -1;
        for (i32 i = 0; i < v->nfakes; i++) {
            if (v->fakes[i].name == "bash") g.prog = i;
        }
        g.err = E_NOENT;
    } else {
        g = vprogram(v, &scratch, path);
    }
    i32 prog = g.prog;
    if (prog < 0) {
        r.err = g.err;
        return r;
    }
    if (!v->win) {
        // Linux execve: each string at most MAX_ARG_STRLEN (32 pages),
        // all of them with their pointers within ARG_MAX (getconf)
        enum { STRMAX = 131072, ARGMAX = 2097152 };
        iz total = 0;
        for (iz i = 0; i < s->argc+s->nenv; i++) {
            Str a = i<s->argc ? s->argv[i] : s->env[i-s->argc];
            if (a.len >= STRMAX) total = ARGMAX;
            total += a.len + 1 + (iz)sizeof(void *);
        }
        if (total > ARGMAX) {
            r.err = E_2BIG;
            return r;
        }
    }
    // At the limit, as a kernel would, let time pass while the others
    // run and make room (jobs still starting sleep); E_AGAIN if none
    // does within a virtual second
    vreapexited(os, v);
    for (i32 tries = 0; v->live>=64 && tries<1000; tries++) {
        os_sleep(os, 1000000);
        vreapexited(os, v);
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
    ctx.image = s->image;
    if (v->win && !s->image.base) {
        vwinctx(v, &mem, &ctx, s, &wc, &g);
    } else {
        ctx.argc = s->argc;
        ctx.argv = alloc<Str>(&mem, s->argc+1);
        for (iz i = 0; i < s->argc; i++) ctx.argv[i] = clone(&mem, s->argv[i]);
    }
    ctx.nenv = s->nenv;
    ctx.env  = alloc<Str>(&mem, s->nenv+1);
    for (iz i = 0; i < s->nenv; i++) ctx.env[i] = clone(&mem, s->env[i]);
    if (!v->win) {
        ctx.cwd = clone(&mem, s->cwd);
    }
    for (iz i = 0; i < countof(ctx.fd); i++) {
        ctx.fd[i] = BADHANDLE;
    }
    for (iz i = 0; i < s->nfds; i++) {
        // Windows passes descriptors past 2 only through the CRT's blob
        b32 pass = !v->win || s->fds[i].fd<3 || s->crtslot || s->image.base;
        if (s->fds[i].fd<countof(ctx.fd) && pass) {
            ctx.fd[s->fds[i].fd] = os_dup(os, s->fds[i].h);
        }
    }
    ctx.scratch = mem;

    p->state  = VP_RUN;
    if (s->image.base) {
        // A shell continuing from an image takes a while to start, as a
        // forked or self-spawned one does: its parent goes on meanwhile
        p->state = VP_BLOCK;
    }
    p->status = 0;
    p->prog   = prog;
    p->shell  = v->fakes[prog].name == "bash";
    p->failed = 0;
    p->intr   = 0;
    p->pid    = v->nextpid++;
    p->sigpend = 0;
    p->nlsigs  = 0;
    p->sigign = s->sigign;
    p->wait   = VW_NONE;
    if (s->image.base) {
        p->wait  = VW_SLEEP;
        p->until = v->clock + 5000000;  // 5 ms
    }
    p->ios    = 0;
    p->ctx    = ctx;
    p->tag    = 0;
    p->ntasks = 1;
    p->group  = 0;
    p->ingroup = vgroup(v);
    if (v->trace) {
        // Identified by its starter and how many that task started
        VProc *me  = v->procs + v->cur;
        u64    key = vmix(me->lid, me->tag);
        i32    i   = 0;
        for (; i<v->nspawns && v->spawns[i].key!=key; i++) {}
        if (i == countof(v->spawns)) {
            v->racy = 1;
            i = 0;
        } else if (i == v->nspawns) {
            v->spawns[v->nspawns++].key = key;
        }
        p->lid = vmix(key, ++v->spawns[i].n);
    }
    p->stack  = p->shell ? (iz)1<<25 : (iz)1<<20;
    p->own    = p->fiber = os_fiber(os, &v->a, p->stack, vprocentry, p);
    v->live++;
    r.proc = (Handle)id;
    r.pid  = p->pid;
    return r;
}

static i32 os_exec(Os *, Arena, Spawn *)
{
    return E_NOSYS;  // a vos process is a fiber: start a child instead
}

static i32 os_forkexec(Os *, Arena, Spawn *)
{
    return E_NOSYS;
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
        if (vistty(v, io->h)) {
            if (!io->len) {
                if (!vttyhas(v)) return 0;
                if (perform) io->n = 0;
                return 1;
            }
            if (!vttyready(v)) return 0;
            if (perform) {
                iz n = vttyget(os, v, io->buf, io->len);
                if (n < 0) return 0;
                io->n = n;
            }
            return 1;
        }
        if (f && f->kind==VK_PIPEREAD) {
            VPipe *p = f->pipe;
            if (p->len==p->pos && p->writers) return 0;
            if (perform) io->n = vpiperead(v, p, io->buf, io->len);
        } else if (perform) {
            io->n = os_read(os, io->h, io->buf, io->len);
            io->err = io->n<0 ? E_BADF : 0;
        }
        return 1;
    case IO_WRITE:
        if (f && f->kind==VK_PIPEWRITE) {
            VPipe *p = f->pipe;
            if (!p->readers) {
                v->racy = 1;
                io->err = E_PIPE;
                return 1;
            }
            if (p->len-p->pos == VPIPECAP) return 0;
            if (perform) {
                vtracewrite(v, p);
                io->n = vpipewrite(v, p, io->buf, io->len);
            }
        } else if (perform) {
            // a failure as taskwrite reports it with no other tasks
            io->n = io->len;
            if (!os_write(os, io->h, io->buf, io->len)) {
                Stat st = os_fstat(os, io->h);
                io->err = st.type==FT_PIPE || st.type==FT_FIFO ? E_PIPE : E_IO;
            }
        }
        return 1;
    case IO_PROC:
        if (io->plt[2]) return 1;  // reaped early (vreapexited): n holds
        if (!io->h || io->h>=(Handle)v->nprocs || v->procs[io->h].state==VP_FREE) {
            io->err = E_CHILD;
            return 1;
        }
        if (v->procs[io->h].state != VP_EXIT) return 0;
        if (perform) io->n = vreap(os, v, io->h);
        return 1;
    case IO_CONNECT: {
        VNamedPipe *n = vserverpipe(v, io->h);
        if (!n) {
            io->err = E_INVAL;
            return 1;
        }
        if (perform) io->n = 0;
        return n->connected;
    }
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
    io->plt[0] = me->tag;  // tracing: done later, perhaps for another task
    io->plt[1] = me->group;
    io->plt[2] = 0;
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
        if (me->intr) {
            me->intr = 0;  // a signal arrived: the shell takes it
            return 0;
        }
        for (Io **p = &me->ios; *p; p = &(*p)->next) {
            Io *io = *p;
            u64 tag   = me->tag;
            u64 group = me->group;
            me->tag   = io->plt[0];  // tracing: as the task that submitted it
            me->group = io->plt[1];
            b32 ok = vio(os, v, io, 1);
            me->tag   = tag;
            me->group = group;
            if (ok) {
                *p = io->next;
                io->done = 1;
                return io;
            }
        }
        if (deadline == WAIT_POLL) {
            // Polling: let the other processes make progress once. A
            // shell polls from a busy loop's safe points: let that stand
            // for time passing (1 ms), or a sleeper it waits for, as in
            // { sleep 1; kill -USR1 $$; } & while :; do :; done, would
            // never wake, since time only jumps when nothing can run
            if (yielded) return 0;
            v->clock += 1000000;
            vyield(os, v);
            yielded = 1;
            return 0;  // what that did, the next poll sees
            continue;
        }
        if (deadline>0 && v->clock>=deadline) return 0;
        me->wait  = VW_IO;
        me->until = deadline>0 ? deadline : 0;
        b32 ok = vschedule(os, v);
        me->until = 0;
        if (!ok) {
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
    if (io->op==IO_CONNECT && vioready(v, io)) {
        return 0;  // a client came: it completes, as on Win32
    }
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


// Windows programs: vos-only fakes installed in the Windows tree

// The applet a program name stands for: its final component without
// an .exe suffix.
static Str vappletname(Str path)
{
    iz i = path.len;
    for (; i>0 && !pathsep(path[i-1]) && path[i-1]!=':'; i--) {}
    Str name = cuthead(path, i);
    if (name.len>4 && equalfold(taketail(name, 4), ".exe")) {
        name = cuttail(name, 4);
    }
    return name;
}

// argv: tools/argv.cpp's output, for the same probes (its header and
// tools/README.md have the format).
static i32 vfake_argv(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOut *o = &p.out;
    print(o, "cmdline = <");
    print(o, ctx->cmdline);
    print(o, ">\n");
    for (iz i = 0; i < ctx->argc; i++) {
        print(o, "argv[");
        print(o, i);
        print(o, "] = <");
        print(o, ctx->argv[i]);
        print(o, ">\n");
    }

    Str show = venvget(ctx->env, ctx->nenv, "ARGV_SHOW");
    for (iz i = 0; i < show.len;) {
        for (; i<show.len && (show[i]==' ' || show[i]==','); i++) {}
        iz beg = i;
        for (; i<show.len && show[i]!=' ' && show[i]!=','; i++) {}
        Str name = span(show.data+beg, show.data+i);
        if (!name.len) continue;
        Str value = venvget(ctx->env, ctx->nenv, name);
        print(o, name);
        if (value.data) {
            print(o, "=");
            print(o, value);
            print(o, "\n");
        } else {
            print(o, " unset\n");
        }
    }

    Str fdv = venvget(ctx->env, ctx->nenv, "ARGV_FD");
    if (fdv.len) {
        i64 fd = 0;
        for (iz i = 0; i<fdv.len && digit(fdv[i]) && fd<1000000; i++) {
            fd = fd*10 + (fdv[i] - '0');
        }
        print(o, "fd ");
        print(o, fd);
        Handle h = fd<countof(ctx->fd) ? ctx->fd[fd] : BADHANDLE;
        if (h == BADHANDLE) {
            print(o, " closed\n");
        } else {
            print(o, " open\n");
            flush(o);
            Buf b(p.a);
            print(&b, "argv: fd ");
            print(&b, fd);
            putbyte(&b, '\n');
            Str msg = finish(&b);
            os_write(os, h, msg.data, msg.len);
        }
    }
    flush(o);
    return o->err;
}

// The fake for a busybox applet name, ignoring case, or -1.
static i32 vapplet(Vos *v, Str name)
{
    for (iz i = 0; i < v->nfakes; i++) {
        Fake *f = v->fakes + i;
        if (!f->win && f->name!="bash" && equalfold(f->name, name)) return (i32)i;
    }
    return -1;
}

// busybox: busybox-w32's multi-call binary, whose applets are the
// fakes: the one argv[0] names (a copy's name), else argv[1].
static i32 vfake_busybox(Os *os, FakeCtx *ctx)
{
    Vos *v    = osvos(os);
    Str  name = vappletname(ctx->argc ? ctx->argv[0] : Str("busybox"));
    iz   skip = 0;
    if (equalfold(name, "busybox")) {
        if (ctx->argc < 2) {
            FkProg p = fkprog(os, ctx);
            print(&p.out, "BusyBox (vos) multi-call binary.\n");
            return fkexit(&p, 0);
        }
        skip = 1;
        name = ctx->argv[1];
    }
    i32 i = vapplet(v, name);
    if (i < 0) {
        FkProg p = fkprog(os, ctx);
        print(&p.err, name);
        print(&p.err, ": applet not found\n");
        flush(&p.err);
        return 127;
    }
    FakeCtx sub = *ctx;
    sub.argv = ctx->argv + skip;
    sub.argc = ctx->argc - skip;
    return v->fakes[i].main(os, &sub);
}

// The descriptors 0-2 of a program for its child.
static void vstdfds(FakeCtx *ctx, Spawn *s, Arena *a)
{
    s->fds = alloc<SpawnFd>(a, 3);
    for (i32 i = 0; i < 3; i++) {
        if (ctx->fd[i] != BADHANDLE) {
            s->fds[s->nfds].fd  = i;
            s->fds[s->nfds++].h = ctx->fd[i];
        }
    }
}

// The rest of a command line after its program name, which the CRTs
// end at the first blank outside quotes.
static Str vcmdrest(Str cmd)
{
    iz  i = 0;
    b32 q = 0;
    for (; i<cmd.len && (q || (cmd[i]!=' ' && cmd[i]!='\t')); i++) {
        q ^= cmd[i] == '"';
    }
    return cuthead(cmd, i);
}

// An alias stub, as w64devkit's: runs busybox.exe from its own
// directory as the applet named after itself, with the rest of its
// command line verbatim, and exits with its status.
static i32 vfake_alias(Os *os, FakeCtx *ctx)
{
    Arena *a   = &ctx->scratch;
    Str    exe = ctx->exe;
    iz     dir = exe.len;
    for (; dir>0 && !pathsep(exe[dir-1]); dir--) {}
    Str busybox = concat(a, takehead(exe, dir), "busybox.exe");
    Buf b(a);
    quoteprogram(&b, busybox);
    putbyte(&b, ' ');
    print(&b, vappletname(exe));
    print(&b, vcmdrest(ctx->cmdline));
    Spawn s = {};
    s.path    = busybox;
    s.cmdline = finish(&b);
    s.argv    = &busybox;
    s.argc    = 1;
    s.env     = ctx->env;
    s.nenv    = ctx->nenv;
    s.cwd     = ctx->cwd;
    vstdfds(ctx, &s, a);
    Spawned r = os_spawn(os, *a, &s);
    if (r.err) {
        FkProg p = fkprog(os, ctx);
        print(&p.err, "alias: cannot run busybox.exe\n");
        flush(&p.err);
        return 1;
    }
    return os_waitproc(os, r.proc);
}

// cmd.exe, as far as command lines and batch files need it: /c with
// /s quote stripping, %VAR% expansion (substrings %X:~N,M% included)
// and ^ escapes; echo, exit, set and rem; programs and batch files by
// PATH and PATHEXT. Batch lines take %0-%9, %~N, %* and %%. Operators
// (& | < > and parentheses), ECHO ON and setlocal are not modelled.
struct VCmd {
    Os        *os;
    FakeCtx   *ctx;
    FkProg     p;
    Arena     *a;
    Slice<Str> env;     // its own copy: set changes it
    i32        status;  // ERRORLEVEL
    b32        done;    // exit ran
};

static b32 vcmdblank(u8 c)
{
    return c==' ' || c=='\t';
}

static i64 vcmdint(Str s)
{
    s = trim(s);
    b32 neg = s.len && s[0]=='-';
    i64 n   = 0;
    for (iz i = neg; i<s.len && digit(s[i]) && n<1<<24; i++) n = n*10 + (s[i] - '0');
    return neg ? -n : n;
}

// %NAME% or %NAME:~N,M%, or null data if undefined.
static Str vcmdvalue(VCmd *c, Str name)
{
    Cut sub = cut(name, ':');
    Str val = {};
    if (equalfold(sub.head, "cd")) {
        val = slashify(c->a, c->ctx->cwd, '\\');
    } else {
        val = venvget(c->env.data, c->env.len, sub.head);
    }
    if (!val.data || !sub.ok || !startswith(sub.tail, "~")) return val;

    // ~N,M: from N (from the end if negative), M characters (all but
    // the last -M if negative, the rest if absent); empty numbers are 0
    Cut f   = cut(cuthead(sub.tail, 1), ',');
    iz  len = val.len;
    i64 n   = vcmdint(f.head);
    iz  beg = n<0 ? len+n : n;
    beg = beg<0 ? 0 : beg>len ? len : beg;
    i64 m   = vcmdint(f.tail);
    iz  end = !f.ok ? len : m<0 ? len+m : beg+m;
    end = end<beg ? beg : end>len ? len : end;
    return span(val.data+beg, val.data+end);
}

// Expand % references: on the command line (params null) an undefined
// variable stays as written; in a batch file it vanishes, %% is %, and
// %0-%9, %~N and %* are its parameters.
static Str vcmdexpand(VCmd *c, Str s, Slice<Str> *params, Str all)
{
    Buf b(c->a);
    for (iz i = 0; i < s.len; i++) {
        if (s[i] != '%') {
            putbyte(&b, s[i]);
            continue;
        }
        u8 next = byteat(s, i+1);
        if (params) {
            if (next == '%') {
                putbyte(&b, '%');
                i++;
                continue;
            } else if (digit(next)) {
                iz k = next - '0';
                if (k < params->len) print(&b, (*params)[k]);
                i++;
                continue;
            } else if (next=='~' && digit(byteat(s, i+2))) {
                iz  k = byteat(s, i+2) - '0';
                Str v = k<params->len ? (*params)[k] : Str("");
                if (v.len>=2 && v[0]=='"' && v[v.len-1]=='"') v = span(v.data+1, v.data+v.len-1);
                print(&b, v);
                i += 2;
                continue;
            } else if (next == '*') {
                print(&b, all);
                i++;
                continue;
            }
        }
        iz j = i + 1;
        for (; j<s.len && s[j]!='%'; j++) {}
        if (j == s.len) {
            if (!params) putbyte(&b, '%');  // a lone % stays, or vanishes
            continue;
        }
        Str name = span(s.data+i+1, s.data+j);
        if (!name.len) {
            putbyte(&b, '%');  // the second % may start a reference
            continue;
        }
        Str val = vcmdvalue(c, name);
        if (val.data) {
            print(&b, val);
        } else if (!params) {
            print(&b, span(s.data+i, s.data+j+1));
        }
        i = j;
    }
    return finish(&b);
}

// Phase two of cmd's parsing: ^ escapes the next character outside
// quotes, and quotes stay.
static Str vcmdcaret(Arena *a, Str s)
{
    Buf b(a, s.len);
    b32 q = 0;
    for (iz i = 0; i < s.len; i++) {
        if (s[i] == '"') {
            q = !q;
        } else if (!q && s[i]=='^') {
            if (++i == s.len) break;
        }
        putbyte(&b, s[i]);
    }
    return finish(&b);
}

static Str vcmdunquote(Arena *a, Str s)
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        if (s[i] != '"') putbyte(&b, s[i]);
    }
    return finish(&b);
}

static b32 vcmdisfile(VCmd *c, Str path)
{
    Stat st = os_stat(c->os, *c->a, path, 1);
    return !st.err && st.type==FT_FILE;
}

// A program as cmd finds it: by its path, else in the working
// directory and then each PATH entry, as named or with each PATHEXT
// extension. Null data if not found.
static Str vcmdfind(VCmd *c, Str name)
{
    static Str exts[] = {".COM", ".EXE", ".BAT", ".CMD"};
    Str  last  = lastpart(name);
    b32  ext   = cut(last, '.').ok;
    b32  where = last.len != name.len || hasdrive(name);
    Str  path  = venvget(c->env.data, c->env.len, "PATH");
    Str  dirs  = where ? Str("") : concat(c->a, concat(c->a, c->ctx->cwd, ";"), path);
    for (Cut d = {{}, dirs, 1}; d.ok;) {
        d = cut(d.tail, ';');
        if (!where && !d.head.len) continue;
        Str base = where ? fkpath(&c->p, name) : concat(c->a, concat(c->a, d.head, "/"), name);
        if (ext && vcmdisfile(c, base)) return base;
        for (iz i = 0; i < countof(exts); i++) {
            Str cand = concat(c->a, base, exts[i]);
            if (vcmdisfile(c, cand)) return cand;
        }
        if (where) break;
    }
    return {};
}

static void vcmdline(VCmd *c, Str line);

// Run a batch file: each line expanded with the parameters, which are
// split at blanks, commas, semicolons and equals signs outside quotes.
static void vcmdbatch(VCmd *c, Str path, Str arg0, Str rest)
{
    Str src = fkslurp(&c->p, path);
    if (!src.data) {
        c->status = 1;
        return;
    }
    Slice<Str> params = {};
    params = push(c->a, params, arg0);
    iz i = 0;
    for (;;) {
        for (; i<rest.len && find(" \t,;=", Str{rest.data+i, 1})>=0; i++) {}
        if (i == rest.len) break;
        iz  beg = i;
        b32 q   = 0;
        for (; i<rest.len && (q || find(" \t,;=", Str{rest.data+i, 1})<0); i++) {
            q ^= rest[i] == '"';
        }
        params = push(c->a, params, span(rest.data+beg, rest.data+i));
    }
    Str all = trimleft(rest);
    for (; all.len && find(",;=", Str{all.data, 1})>=0; all = trimleft(cuthead(all, 1))) {}

    for (Cut l = {{}, src, 1}; l.ok && !c->done;) {
        l = cut(l.tail, '\n');
        Str line = l.head;
        if (line.len && line[line.len-1]=='\r') line.len--;
        line = vcmdcaret(c->a, vcmdexpand(c, line, &params, all));
        vcmdline(c, line);
    }
    c->done = 0;  // exit /b leaves the batch file only
}

// Run one command after expansion.
static void vcmdline(VCmd *c, Str line)
{
    for (; line.len && (vcmdblank(line[0]) || line[0]=='@'); line = cuthead(line, 1)) {}
    if (!line.len || line[0]==':') return;

    iz  i = 0;
    b32 q = 0;
    for (; i<line.len && (q || (!vcmdblank(line[i]) && line[i]!='.' && line[i]!='/')); i++) {
        q ^= line[i] == '"';
    }
    Str word = takehead(line, i);
    Str rest = cuthead(line, i);
    FkOut *o = &c->p.out;
    if (equalfold(word, "echo")) {
        Str text = rest.len ? cuthead(rest, 1) : rest;
        if (!rest.len || (!trim(text).len && rest[0]!='.' && rest[0]!='/')) {
            print(o, "ECHO is off.\r\n");
        } else if (!equalfold(trim(text), "off") && !equalfold(trim(text), "on")) {
            print(o, text);
            print(o, "\r\n");
        }
        return;
    } else if (equalfold(word, "rem")) {
        return;
    } else if (equalfold(word, "exit")) {
        Str arg = trim(rest);
        if (startswith(arg, "/b") || startswith(arg, "/B")) arg = trim(cuthead(arg, 2));
        i32 n = 0;
        for (iz k = 0; k<arg.len && digit(arg[k]) && n<100000000; k++) n = n*10 + arg[k]-'0';
        c->status = arg.len ? n : c->status;
        c->done   = 1;
        return;
    } else if (equalfold(word, "set") && trim(rest).len) {
        Str def  = trim(rest);
        Cut nv   = cut(def, '=');
        Str name = nv.head;
        for (iz k = 0; k < c->env.len; k++) {
            if (equalfold(envname(c->env[k]), name)) {
                c->env[k] = c->env[--c->env.len];
                break;
            }
        }
        if (nv.tail.len) c->env = push(c->a, c->env, def);
        return;
    }

    // A program, its rest of the line verbatim
    for (; i<line.len && !vcmdblank(line[i]); i++) {
        q ^= line[i] == '"';
    }
    word = vcmdunquote(c->a, takehead(line, i));
    rest = cuthead(line, i);
    Str path = vcmdfind(c, word);
    if (!path.data) {
        print(&c->p.err, "'");
        print(&c->p.err, word);
        print(&c->p.err, "' is not recognized as an internal or external command,\r\n"
                         "operable program or batch file.\r\n");
        flush(&c->p.err);
        c->status = 9009;
        return;
    }
    if (vbatch(path)) {
        vcmdbatch(c, path, takehead(line, i), rest);
        return;
    }
    flush(o);
    Spawn s = {};
    s.path    = path;
    s.cmdline = line;
    s.argv    = &word;
    s.argc    = 1;
    s.env     = c->env.data;
    s.nenv    = c->env.len;
    s.cwd     = c->ctx->cwd;
    vstdfds(c->ctx, &s, c->a);
    Spawned r = os_spawn(c->os, *c->a, &s);
    c->status = r.err ? 1 : os_waitproc(c->os, r.proc) & 255;
}

static i32 vfake_cmd(Os *os, FakeCtx *ctx)
{
    VCmd c = {};
    c.os  = os;
    c.ctx = ctx;
    c.p   = fkprog(os, ctx);
    c.a   = &ctx->scratch;
    for (iz i = 0; i < ctx->nenv; i++) c.env = push(c.a, c.env, ctx->env[i]);

    Str cmd = vcmdrest(ctx->cmdline);
    b32 run = 0;
    for (;;) {
        cmd = trimleft(cmd);
        if (cmd.len<2 || cmd[0]!='/') break;
        u8 sw = lowercase(cmd[1]);
        if (sw=='c' || sw=='k') {
            run = 1;
            cmd = cuthead(cmd, 2);
            break;
        }
        iz i = 1;  // /d /s /q /e:on /v:off ...
        for (; i<cmd.len && !vcmdblank(cmd[i]) && cmd[i]!='/'; i++) {}
        cmd = cuthead(cmd, i);
    }
    if (!run) {
        return fkexit(&c.p, 0);  // interactive, with no console to read
    }

    // The first quote and the last go: always with /s, and the old
    // rules' exceptions without it are not modelled
    cmd = trimleft(cmd);
    if (cmd.len && cmd[0]=='"') {
        iz last = cmd.len - 1;
        for (; last>0 && cmd[last]!='"'; last--) {}
        if (last > 0) {
            Buf b(c.a, cmd.len);
            print(&b, span(cmd.data+1, cmd.data+last));
            print(&b, cuthead(cmd, last+1));
            cmd = finish(&b);
        }
    }
    // Commands joined by & outside quotes and carets, after % expansion;
    // the blanks before an & are not part of the command (%* drops them)
    cmd = vcmdexpand(&c, cmd, 0, {});
    for (b32 more = 1; more && !c.done;) {
        iz  i = 0;
        b32 q = 0;
        for (; i<cmd.len && (q || cmd[i]!='&'); i++) {
            q ^= cmd[i] == '"';
            i += !q && cmd[i]=='^';
        }
        more = i < cmd.len;
        Str one = takehead(cmd, more ? i : cmd.len);
        for (; more && one.len && vcmdblank(one[one.len-1]); one.len--) {}
        vcmdline(&c, vcmdcaret(c.a, one));
        cmd = cuthead(cmd, more ? i+1 : cmd.len);
    }
    return fkexit(&c.p, c.status);
}

static Fake vwinfakes[] = {
    {"alias",   vfake_alias,   1},
    {"argv",    vfake_argv,    1},
    {"busybox", vfake_busybox, 1},
    {"cmd",     vfake_cmd,     1},
};

// Windows mode: the environment as the Win32 layer hands it to the
// shell at startup (main_win32.cpp), Path renamed PATH.
static Str *vwinenv(Arena *a, Str *env, iz nenv)
{
    Str *r = alloc<Str>(a, nenv+1);
    for (iz i = 0; i < nenv; i++) {
        Str name = envname(env[i]);
        r[i] = env[i];
        if (name.len==4 && equalfold(name, "path")) {
            r[i] = concat(a, "PATH", cuthead(env[i], 4));
        }
    }
    return r;
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

static VNode *vinstallpe(Vos *v, Str dir, Str name, i32 prog, i32 machine, Str dll, b32 ours)
{
    VNode *d = vmkdirs(v, dir);
    VNode *n = vnew(v, d, name, FT_FILE, 0755);
    n->prog = prog + 1;
    n->data = vpeimage(&v->a, machine, dll, ours, prog+1);
    n->cap  = n->data.len;
    return n;
}

static VNode *vputfile(Vos *v, Str dir, Str name, Str data)
{
    VNode *n = vnew(v, vmkdirs(v, dir), name, FT_FILE, 0644);
    vwritenode(v, n, 0, data.data, data.len);
    return n;
}

// The Windows tree: drives C: and D:, cmd.exe in C:/Windows/System32,
// w64devkit in C:/w64 (busybox, its alias stubs, argv and argv32, and
// the shell as sh.exe), an MSYS2 install in C:/msys (the shell as
// bash.exe, the other fakes as MSYS2 programs), a profile with TEMP in
// C:/Users/oracle, and the working directory C:/work.
static void vwintree(Vos *v)
{
    enum { X64 = 0x8664, I386 = 0x14c };
    vmkdirs(v, "D:");
    vmkdirs(v, "C:/work");
    vmkdirs(v, "C:/Users/oracle/AppData/Local/Temp");
    vmkdirs(v, "C:/msys/tmp");
    vputfile(v, "C:/msys/etc", "fstab",
        "none / cygdrive binary,posix=0,noacl,user 0 0\n"
        "none /tmp usertemp binary,posix=0,noacl 0 0\n");
    i32 alias = -1;
    for (iz i = 0; i < v->nfakes; i++) {
        alias = v->fakes[i].name=="alias" ? (i32)i : alias;
    }
    for (i32 i = 0; i < v->nfakes; i++) {
        Str name = v->fakes[i].name;
        Str exe  = concat(&v->a, name, ".exe");
        if (name == "bash") {
            vinstallpe(v, "C:/msys/usr/bin", "bash.exe", i, X64, "kernel32.dll", 1);
            vinstallpe(v, "C:/w64/bin", "sh.exe", i, X64, "kernel32.dll", 1);
        } else if (name == "argv") {
            vinstallpe(v, "C:/w64/bin", "argv.exe", i, X64, "msvcrt.dll", 0);
            vinstallpe(v, "C:/w64/bin", "argv32.exe", i, I386, "msvcrt.dll", 0);
            // small and CRT-free, but no stub: as u-config or cc.exe
            vinstallpe(v, "C:/w64/bin", "argvk.exe", i, X64, "kernel32.dll", 0);
        } else if (name == "busybox") {
            vinstallpe(v, "C:/w64/bin", "busybox.exe", i, X64, "msvcrt.dll", 0);
        } else if (name == "cmd") {
            vinstallpe(v, "C:/Windows/System32", "cmd.exe", i, X64, "ucrtbase.dll", 0);
        } else if (!v->fakes[i].win && name!="recho" && name!="zecho"
                   && name!="printenv" && name!="xcase") {
            vinstallpe(v, "C:/msys/usr/bin", exe, i, X64, "msys-2.0.dll", 0);
            if (alias >= 0) {
                VNode *n = vinstallpe(v, "C:/w64/bin", exe, alias, X64, "kernel32.dll", 0);
                Str    w = Str("could not start busybox.exe");  // spawn.cpp's bbstub
                copybytes(n->data.data+0x800, w.data, w.len);
            }
        }
    }
}

// Reset the vos to the oracle container's initial state, or with win
// to the Windows tree (vwintree).
static void vreset(Vos *v, Arena a, Fake *fakes, iz nfakes, b32 win = 0)
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
    v->nextpid = 10000;
    v->procs[0].state = VP_RUN;
    v->procs[0].shell = 1;
    v->procs[0].pid   = 2;  // as the harnesses' OsInfo
    v->procs[0].lid   = 1;
    v->procs[0].ntasks  = 1;
    v->procs[0].ingroup = 1;  // tracing: vgroup is never 0

    v->root = vnew(v, 0, "", FT_DIR, 0755);
    if (win) {
        v->win = 1;
        vwintree(v);
        v->user = 1000;
        return;
    }
    vmkdirs(v, "home/oracle")->uid = 1000;
    VNode *tmp = vmkdirs(v, "tmp");
    tmp->mode = 01777;
    vmkdirs(v, "usr/local/bin");
    vmkdirs(v, "usr/bin");
    vmkdirs(v, "etc");
    // What login shells see of the oracle image's /etc/profile: the
    // PATH Debian gives a user other than root
    vputfile(v, "etc", "profile",
        "PATH=/usr/local/bin:/usr/bin:/bin:/usr/local/games:/usr/games\n"
        "export PATH\n");
    vmkdirs(v, "dev");
    // The terminal database entries the editor looks for (eddumb)
    vputfile(v, "usr/share/terminfo/x", "xterm", "");
    vputfile(v, "usr/share/terminfo/x", "xterm-256color", "");
    vputfile(v, "usr/share/terminfo/d", "dumb", "");
    vputfile(v, "usr/share/terminfo/v", "vt100", "");
    VNode *bin = vnew(v, v->root, "bin", FT_LINK, 0777);
    bin->data = "usr/bin";
    for (iz i = 0; i < nfakes; i++) {
        if (fakes[i].win) continue;
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
static VNode *vcreatable(Vos *v, Arena *a, Str path, VResolved *r, i32 *err)
{
    *r = vresolve(v, a, path, 0);
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

static i32 os_mkdir(Os *os, Arena scratch, Str path, i32 mode)
{
    Vos      *v = osvos(os);
    vmutate(v);
    VResolved r;
    i32       err;
    VNode    *dir = vcreatable(v, &scratch, path, &r, &err);
    if (!dir) return err;
    vnew(v, dir, r.base, FT_DIR, mode & 07777 & ~022)->nlink = 2;
    return 0;
}

static i32 os_mkfifo(Os *os, Arena scratch, Str path, i32 mode)
{
    Vos      *v = osvos(os);
    vmutate(v);
    VResolved r;
    i32       err;
    VNode    *dir = vcreatable(v, &scratch, path, &r, &err);
    if (!dir) return err;
    vnew(v, dir, r.base, FT_FIFO, mode & 07777 & ~022);
    return 0;
}

static i32 os_symlink(Os *os, Arena scratch, Str target, Str path)
{
    Vos      *v = osvos(os);
    vmutate(v);
    VResolved r;
    i32       err;
    VNode    *dir = vcreatable(v, &scratch, path, &r, &err);
    if (!dir) return err;
    vnew(v, dir, r.base, FT_LINK, 0777)->data = clone(&v->a, target);
    return 0;
}

// A hard link is a copy sharing the inode number: later writes to one
// do not show in the other.
static i32 os_link(Os *os, Arena scratch, Str from, Str to)
{
    Vos      *v = osvos(os);
    vmutate(v);
    VResolved s = vresolve(v, &scratch, from, 0);
    if (!s.node) return s.err;
    if (s.node->type == FT_DIR) return E_PERM;
    VResolved r;
    i32       err;
    VNode    *dir = vcreatable(v, &scratch, to, &r, &err);
    if (!dir) return err;
    VNode *n = vnew(v, dir, r.base, s.node->type, s.node->mode);
    n->ino   = s.node->ino;
    n->mtime = s.node->mtime;
    n->atime = s.node->atime;
    n->ctime = v->clock;
    n->prog  = s.node->prog;
    n->data  = clone(&v->a, s.node->data);
    n->cap   = n->data.len;
    n->nlink = ++s.node->nlink;
    return 0;
}

static i32 os_unlink(Os *os, Arena scratch, Str path)
{
    Vos      *v = osvos(os);
    vmutate(v);
    VResolved r = vresolve(v, &scratch, path, 0);
    v->clock += 1000;
    if (!r.node) return r.err;
    if (r.node->type == FT_DIR) return E_ISDIR;
    if (!(r.node->parent->mode & 0200)) return E_ACCES;
    vdetach(r.node);
    return 0;
}

static i32 os_rmdir(Os *os, Arena scratch, Str path)
{
    Vos      *v = osvos(os);
    vmutate(v);
    VResolved r = vresolve(v, &scratch, path, 0);
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

static i32 os_rename(Os *os, Arena scratch, Str from, Str to)
{
    Vos      *v = osvos(os);
    vmutate(v);
    VResolved a = vresolve(v, &scratch, from, 0);
    VResolved b = vresolve(v, &scratch, to, 0);
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

static i32 os_chmod(Os *os, Arena scratch, Str path, i32 mode)
{
    Vos      *v = osvos(os);
    vmutate(v);
    VResolved r = vresolve(v, &scratch, path, 1);
    if (!r.node) return r.err;
    r.node->mode  = mode & 07777;
    r.node->ctime = v->clock;
    return 0;
}

static i32 os_utime(Os *os, Arena scratch, Str path, i64 mtime)
{
    Vos      *v = osvos(os);
    vmutate(v);
    VResolved r = vresolve(v, &scratch, path, 1);
    if (!r.node) return r.err;
    r.node->mtime = mtime;
    r.node->atime = mtime;
    r.node->ctime = v->clock;
    return 0;
}

static Str os_readlink(Os *os, Arena *a, Str path)
{
    Vos      *v = osvos(os);
    VResolved r = vresolve(v, a, path, 0);
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

// Virtual time passes instantly once nothing else can run, so that a
// sleeping program can be killed meanwhile.
static void os_sleep(Os *os, i64 ns)
{
    Vos   *v  = osvos(os);
    VProc *me = v->procs + v->cur;
    me->until = v->clock + ns;
    while (v->clock < me->until) {
        me->wait = VW_SLEEP;
        if (!vschedule(os, v)) {
            v->clock = me->until;
        }
    }
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
