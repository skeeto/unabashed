// libunabashed: runs of the shell inside a host process (unabashed.h)
//
// A run is the shell's whole life as a process, on a fiber of its own
// on the host's thread: setup on the host's fiber, sh_main on the run's
// shell fiber, then teardown back on the host's. Every resource the run
// takes is recorded the moment it exists (reservations, descriptors,
// programs), so that teardown can end the run from anywhere, without
// the core: an abort (out of memory, a cancel, a deadlock) switches
// straight back to the host's fiber, abandoning every fiber of the run
// where it stands, and teardown frees what the records list.
//
// The platform table is pltvtable with the entries below replaced
// (libvtable): refusals for what would change the host process (exec,
// signal dispositions, terminals, process groups, limits), and records
// around the entries that create resources.
// This is free and unencumbered software released into the public domain.

enum : i32 { E_CANCELED = 125 };

enum : u32 { REQ_HARD = 1 };

// A reservation of the run's, after a header that links it.
struct LibRes {
    LibRes *next;
    LibRes *prev;
    iz      len;   // the whole mapping, header included
};
enum : iz { RESHEAD = (iz)1<<16 };  // a page on every system

struct LibKid {
    i64 pid;
    b32 reaped;
};

enum : i32 { MAXKIDS = 1024, FDBITS = 1<<16 };

struct Lib {
    Os         os;  // first: an Os* is a Lib*
    ub_config  cfg;
    i32        running;   // a run is active (atomic)
    u32        req;       // REQ_HARD (atomic)
    b32        inrun;     // on a run fiber, so it can be aborted
    i32        why;       // the abort's error, 0 for a normal end
    i32        status;    // sh_main's
    Fiber     *shell;
    ub_cmd const *cmd;
    i64        pid;       // $$
    Arena      arena;     // the run's own: OsInfo
    char      *tmpdir;    // TMPDIR from the run's environment, or null
    i32        errfd;     // the run's stderr, once it has one
    sigset_t   mask;      // the thread's before the run
    b32        masked;

    // Memory
    LibRes    *res;
    iz         committed;
    iz         memlimit;  // -1 for none
    byte      *prestack;  // the shell fiber's, made during setup

    // Descriptors below FDBITS the run owns
    u64        fdbits[FDBITS/64];
    i32        nfds;
    i32        maxfds;
    DIR       *dir;       // plt_list's, while open

    // Programs, and the process groups they lead (each its own session)
    LibKid     kids[MAXKIDS];
    i32        nkids;
};

static __thread Lib *curlib;  // the run on this thread


// Hooks

static i32 libspin;

static void defaultlock(void *, int lock)
{
    if (!lock) {
        __atomic_store_n(&libspin, 0, __ATOMIC_RELEASE);
        return;
    }
    while (__atomic_exchange_n(&libspin, 1, __ATOMIC_ACQUIRE)) {
        sched_yield();
    }
}

static void lib_spawnlock(b32 lock)
{
    Lib *l = curlib;
    l->cfg.spawnlock(l->cfg.user, lock);
}


// Aborts and check points

[[noreturn]] static void lib_abort(Os *os, i32 err)
{
    Lib *l = (Lib *)os;
    if (!l->inrun || os->fibers.current==&os->fibers.main) {
        __builtin_trap();  // never on the host's fiber
    }
    l->why   = err;
    l->inrun = 0;
    os_switch(os, &os->fibers.main);
    __builtin_trap();  // never resumed
}

static void checkhard(Lib *l)
{
    if (l->inrun && (__atomic_load_n(&l->req, __ATOMIC_ACQUIRE) & REQ_HARD)) {
        lib_abort(&l->os, E_CANCELED);
    }
}

static void lib_step()
{
    checkhard(curlib);
}

static void wakeup(Lib *l)
{
    u8 c = 0;
    if (write(l->os.wakefd[1], &c, 1)) {}
}


// Memory: every reservation is listed, so teardown unmaps them all

static byte *libreserve(Os *os, iz len)
{
    Lib *l = (Lib *)os;
    if (len<0 || len>(iz)(~(uz)0>>2) - RESHEAD) return 0;
    void *p = mmap(0, (uz)(len+RESHEAD), PROT_NONE,
                   MAP_PRIVATE|MAP_ANON|MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) return 0;
    if (mprotect(p, sizeof(LibRes), PROT_READ|PROT_WRITE)) {
        munmap(p, (uz)(len+RESHEAD));
        return 0;
    }
    LibRes *r = (LibRes *)p;
    r->len  = len + RESHEAD;
    r->prev = 0;
    r->next = l->res;
    if (l->res) l->res->prev = r;
    l->res = r;
    return (byte *)p + RESHEAD;
}

static void libunreserve(Lib *l, byte *p)
{
    LibRes *r = (LibRes *)(p - RESHEAD);
    if (r->prev) r->prev->next = r->next;
    else l->res = r->next;
    if (r->next) r->next->prev = r->prev;
    munmap(r, (uz)r->len);
}

static b32 libcommit(Os *os, byte *p, iz len)
{
    Lib *l = (Lib *)os;
    checkhard(l);
    if (l->memlimit>=0 && len>l->memlimit-l->committed) return 0;
    if (!plt_commit(os, p, len)) return 0;
    l->committed += len;
    return 1;
}

static void libdecommit(Os *os, byte *p, iz len)
{
    Lib *l = (Lib *)os;
    plt_decommit(os, p, len);
    l->committed -= len<l->committed ? len : l->committed;
}

static byte *lib_fiberstack(Os *os, iz size)
{
    Lib  *l = (Lib *)os;
    byte *p = l->prestack;
    if (p) {
        l->prestack = 0;
        return p;
    }
    p = libreserve(os, size);
    if (!p || mprotect(p, (uz)size, PROT_READ|PROT_WRITE)) {
        os_oom();
    }
    mprotect(p, 1<<12, PROT_NONE);  // guard page
    return p;
}

static void lib_fiberunstack(Os *os, byte *p, iz)
{
    libunreserve((Lib *)os, p);
}

// An arena as newarena makes one, but failing rather than aborting.
static b32 tryarena(Lib *l, Arena *a, iz reserve)
{
    byte *base = libreserve(&l->os, reserve);
    iz    head = (iz)1<<16;
    if (!base || !libcommit(&l->os, base, head)) return 0;
    Mem *m = (Mem *)base;
    m->os     = &l->os;
    m->commit = base + head;
    m->limit  = base + reserve;
    a->beg = base + sizeof(Mem);
    a->end = m->commit;
    a->mem = m;
    return 1;
}


// Descriptors: the run's own, within its budget

static b32 fdowned(Lib *l, i64 fd)
{
    return fd>=0 && fd<FDBITS && (l->fdbits[fd/64]>>(fd%64) & 1);
}

// Record a new descriptor, or close it if over budget (-1, E_MFILE).
static i32 fdtake(Lib *l, i32 fd)
{
    if (fd < 0) return fd;
    if (fd>=FDBITS || l->nfds>=l->maxfds) {
        close(fd);
        return -1;
    }
    l->fdbits[fd/64] |= (u64)1 << (fd%64);
    l->nfds++;
    return fd;
}

static void fdgive(Lib *l, i32 fd)
{
    if (!fdowned(l, fd)) return;
    l->fdbits[fd/64] &= ~((u64)1 << (fd%64));
    l->nfds--;
}

static Opened libopen(Os *os, Arena scratch, Str path, i32 flags, i32 mode)
{
    Opened r = plt_open(os, scratch, path, flags, mode);
    if (r.h!=BADHANDLE && fdtake((Lib *)os, (i32)r.h)<0) {
        r.h   = BADHANDLE;
        r.err = E_MFILE;
    }
    return r;
}

static Opened libtempfile(Os *os, Arena scratch)
{
    Opened r = plt_tempfile(os, scratch);
    if (r.h!=BADHANDLE && fdtake((Lib *)os, (i32)r.h)<0) {
        r.h   = BADHANDLE;
        r.err = E_MFILE;
    }
    return r;
}

static void libclose(Os *os, Handle h)
{
    Lib *l = (Lib *)os;
    if (!fdowned(l, (i64)h)) {
        flushw(os);
        return;  // never the host's
    }
    fdgive(l, (i32)h);
    plt_close(os, h);
}

static Handle libdup(Os *os, Handle h)
{
    Lib *l = (Lib *)os;
    if (!fdowned(l, (i64)h)) return BADHANDLE;
    Handle r = plt_dup(os, h);
    if (r!=BADHANDLE && fdtake(l, (i32)r)<0) r = BADHANDLE;
    return r;
}

static OsPipe libpipe(Os *os)
{
    Lib   *l = (Lib *)os;
    OsPipe r = plt_pipe(os);
    if (r.err) return r;
    i32 rd = fdtake(l, (i32)r.r);
    i32 wr = rd<0 ? -1 : fdtake(l, (i32)r.w);
    if (wr < 0) {
        if (rd >= 0) {
            fdgive(l, rd);
            close(rd);
        } else {
            close((i32)r.w);
        }
        r.r = r.w = BADHANDLE;
        r.err = E_MFILE;
    }
    return r;
}

static void lib_dir(Os *os, DIR *d)
{
    ((Lib *)os)->dir = d;
}

static char *lib_tmpdir(Os *os)
{
    return ((Lib *)os)->tmpdir;
}


// Programs

static LibKid *findkid(Lib *l, i64 pid)
{
    for (i32 i = 0; i < l->nkids; i++) {
        if (l->kids[i].pid == pid) return l->kids + i;
    }
    return 0;
}

static void reapedkid(Lib *l, LibKid *k)
{
    if (k->reaped) return;
    k->reaped = 1;
    if (l->cfg.child) l->cfg.child(l->cfg.user, k->pid, 0);
}

static void lib_reaped(i64 pid)
{
    Lib    *l = curlib;
    LibKid *k = findkid(l, pid);
    if (k) reapedkid(l, k);
}

// Room for one more: reaped programs' groups are forgotten, oldest
// first, when the table fills.
static b32 kidroom(Lib *l)
{
    if (l->nkids < MAXKIDS) return 1;
    i32 n = 0;
    for (i32 i = 0; i < l->nkids; i++) {
        if (!l->kids[i].reaped || i>=l->nkids-MAXKIDS/2) {
            l->kids[n++] = l->kids[i];
        }
    }
    l->nkids = n;
    return n < MAXKIDS;
}

static Spawned libspawn(Os *os, Arena scratch, Spawn *s)
{
    Lib *l = (Lib *)os;
    checkhard(l);
    if (!kidroom(l)) {
        Spawned r = {};
        r.proc = BADHANDLE;
        r.err  = E_AGAIN;
        return r;
    }
    Spawned r = plt_spawn(os, scratch, s);
    if (!r.err) {
        LibKid *k = l->kids + l->nkids++;
        k->pid    = r.pid;
        k->reaped = 0;
        if (l->cfg.child) l->cfg.child(l->cfg.user, r.pid, 1);
    }
    return r;
}

static i32 libwaitproc(Os *os, Handle proc)
{
    Lib *l = (Lib *)os;
    flushw(os);
    for (i32 backoff = 1;; backoff = backoff<64 ? backoff*2 : 64) {
        checkhard(l);
        i32   status = 0;
        pid_t r = waitpid((pid_t)proc, &status, WNOHANG);
        if (r<0 && errno==EINTR) continue;
        if (r) {
            lib_reaped((i64)proc);
            return r<0 ? 127 : waitstatus(status);
        }
        struct pollfd p = {};
        p.fd     = os->wakefd[0];
        p.events = POLLIN;
        if (poll(&p, 1, backoff) > 0) {
            u8 buf[64];
            while (read(os->wakefd[0], buf, sizeof(buf)) > 0) {}
        }
    }
}

static i32 libkill(Os *os, i64 pid, i32 sig)
{
    Lib *l = (Lib *)os;
    flushw(os);
    if (!pid || pid==-l->pid) {
        // The run's own group: every level and program, at the next
        // check point
        if (sig) {
            if (sig<1 || sig>64) return E_INVAL;
            bitsor(&os->grpbits, (u64)1<<(sig-1));
            wakeup(l);
        }
        return 0;
    }
    i64 host = getpid();
    if (pid==-1 || pid==host || pid==-host || pid==-(i64)getpgrp() ||
        pid<-(i64)0x7fffffff || pid>(i64)0x7fffffff) {
        return sig ? E_PERM : pid==-1 ? E_PERM : 0;
    }
    return plt_kill(os, pid, sig);
}


// Waiting: the wake pipe, the descriptors of submitted reads and
// writes, and process exits, which come with no SIGCHLD: those are
// polled, backing off from 1 to 64 ms while any is pending.

static void libsubmit(Os *os, Io *io)
{
    if (io->op==IO_READ || io->op==IO_WRITE) {
        if (fdkind(os, (i32)io->h) == FK_REG) {
            // A file never blocks: done at once (and coalesced)
            io->done = 1;
            io->next = 0;
            if (io->op == IO_WRITE) {
                b32 ok  = plt_write(os, io->h, io->buf, io->len);
                io->n   = ok ? io->len : 0;
                io->err = ok ? 0 : E_IO;
            } else {
                iz n    = plt_read(os, io->h, io->buf, io->len);
                io->n   = n<0 ? 0 : n;
                io->err = n<0 ? E_IO : 0;
            }
            return;
        }
    }
    plt_submit(os, io);
}

static Io *takeproc(Os *os)
{
    for (Io **p = &os->pending; *p; p = &(*p)->next) {
        Io *io = *p;
        if (isproc(io) && tryio(io, 1)) {
            *p = io->next;
            io->done = 1;
            return io;
        }
    }
    return 0;
}

static Io *libwait(Os *os, i64 deadline)
{
    Lib *l = (Lib *)os;
    if (deadline != WAIT_POLL) flushw(os);
    enum { MAXPOLL = 256 };
    struct pollfd fds[MAXPOLL];
    for (i32 backoff = 1;; backoff = backoff<64 ? backoff*2 : 64) {
        checkhard(l);
        Io *done = takeproc(os);
        if (done) return done;

        b32 procs = 0;
        b32 trunc = 0;
        i32 n = 0;
        fds[n].fd        = os->wakefd[0];
        fds[n].events    = POLLIN;
        fds[n++].revents = 0;
        for (Io *io = os->pending; io; io = io->next) {
            if (isproc(io)) {
                procs = 1;
                continue;
            }
            if (n == MAXPOLL) {
                trunc = 1;
                continue;
            }
            fds[n].fd        = (i32)io->h;
            fds[n].events    = io->op==IO_READ ? POLLIN : POLLOUT;
            fds[n++].revents = 0;
        }

        i32 timeout = -1;
        if (deadline == WAIT_POLL) {
            timeout = 0;
        } else if (deadline > 0) {
            i64 left = (deadline - plt_now(os) + 999999) / 1000000;
            timeout = left<0 ? 0 : left>(1<<30) ? (1<<30) : (i32)left;
        }
        if ((procs || trunc) && (timeout<0 || timeout>backoff)) {
            timeout = backoff;
        }
        i32 r = poll(fds, (nfds_t)n, timeout);
        if (r<0 && errno!=EINTR) return 0;
        if (fds[0].revents) {
            u8 buf[64];
            while (read(os->wakefd[0], buf, sizeof(buf)) > 0) {}
        }
        checkhard(l);
        if (os->sigbits || os->grpbits) return 0;  // the core first

        i32 k = 1;
        for (Io **p = &os->pending; *p && k<n; p = &(*p)->next) {
            Io *io = *p;
            if (isproc(io)) continue;
            if (tryio(io, fds[k++].revents)) {
                *p = io->next;
                io->done = 1;
                return io;
            }
        }
        done = takeproc(os);
        if (done) return done;
        if (deadline == WAIT_POLL) return 0;
        if (deadline>0 && plt_now(os)>=deadline) return 0;
    }
}


// Signals: ub_signal sets the bits, as the executable's handler does

static u64 libsigpending(Os *os)
{
    checkhard((Lib *)os);
    return plt_sigpending(os);
}

// A group signal the core takes goes to every program the run started,
// each the leader of its own session, then to every level (ossignals).
static u64 libsiggroup(Os *os)
{
    Lib *l = (Lib *)os;
    u64 bits = plt_siggroup(os);
    for (i32 sig = 1; bits>>(sig-1); sig++) {
        if (!(bits>>(sig-1) & 1)) continue;
        i32 hsig = hostsig(sig);
        if (!hsig) continue;
        for (i32 i = 0; i < l->nkids; i++) {
            kill(-(pid_t)l->kids[i].pid, hsig);
        }
    }
    return bits;
}


// Refusals: nothing that would change the host process

static ImageRef libimagenew(Os *os, iz)
{
    flushw(os);
    return {};  // levels are fibers
}

static void libimagefree(Os *, ImageRef)
{
}

static i32 libexec(Os *os, Arena, Spawn *)
{
    flushw(os);
    return E_NOSYS;  // the core spawns and waits instead
}

static i32 libforkexec(Os *os, Arena, Spawn *)
{
    flushw(os);
    return E_NOSYS;
}

static i32 libsiglevel(Os *os, i64, i64, i32)
{
    flushw(os);
    return E_NOSYS;
}

static LevelSig liblevelsig(Os *)
{
    return {};
}

static void libsigmode(Os *os, i32, i32)
{
    flushw(os);
}

static Tty libtty(Os *os, Handle, Handle, i32)
{
    flushw(os);
    Tty r = {};
    r.err = E_NOTTY;
    return r;
}

static i32 libsetpgid(Os *os, i64, i64)
{
    flushw(os);
    return E_NOSYS;
}

static i64 libtcpgrp(Os *os, Handle, i64)
{
    flushw(os);
    return -E_NOSYS;
}

static i32 libsetrlimit(Os *os, i32, Rlimit)
{
    flushw(os);
    return E_PERM;  // the host's
}

static constexpr OsVtable libtable(OsVtable t)
{
    t.reserve    = libreserve;
    t.commit     = libcommit;
    t.decommit   = libdecommit;
    t.open       = libopen;
    t.tempfile   = libtempfile;
    t.close      = libclose;
    t.dup        = libdup;
    t.pipe       = libpipe;
    t.tty        = libtty;
    t.spawn      = libspawn;
    t.imagenew   = libimagenew;
    t.imagefree  = libimagefree;
    t.exec       = libexec;
    t.forkexec   = libforkexec;
    t.waitproc   = libwaitproc;
    t.submit     = libsubmit;
    t.wait       = libwait;
    t.kill       = libkill;
    t.setpgid    = libsetpgid;
    t.tcpgrp     = libtcpgrp;
    t.setrlimit  = libsetrlimit;
    t.siglevel   = libsiglevel;
    t.levelsig   = liblevelsig;
    t.sigpending = libsigpending;
    t.siggroup   = libsiggroup;
    t.sigmode    = libsigmode;
    return t;
}
static constexpr OsVtable libvtable = libtable(pltvtable);


// A run

static Str libclone(Arena *a, char const *s)
{
    Str r = {};
    if (!s) return r;
    for (; s[r.len]; r.len++) {}
    r.data = (u8 *)allocbytes(a, r.len);
    copybytes(r.data, (void *)s, r.len);
    return r;
}

// $$ for a run without one: never a real pid, never another run's, and
// owned (vpidowner) by the host's pid, so every vpid derived from it
// (newvpid) stays below 2^63.
static i64 virtualpid()
{
    static u32 serial;
    u32 n = __atomic_fetch_add(&serial, 1u, __ATOMIC_RELAXED) % (1u<<20);
    i64 f = 1 + (i64)n*((i64)1<<20);
    return ((i64)1<<22)*f + getpid();
}

// The run's shell fiber: the process's main.
static void runmain(Os *os, void *arg)
{
    Lib          *l   = (Lib *)arg;
    ub_cmd const *cmd = l->cmd;
    Arena        *a   = &l->arena;
    l->inrun = 1;
    checkhard(l);

    OsInfo info = {};
    info.nargs = cmd->argc;
    info.args  = alloc<Str>(a, cmd->argc);
    for (i32 i = 0; i < cmd->argc; i++) {
        info.args[i] = libclone(a, cmd->argv[i]);
    }
    char const *const *env = cmd->env ? cmd->env : environ;
    for (; env[info.nenv]; info.nenv++) {}
    info.env = alloc<Str>(a, info.nenv);
    for (iz i = 0; i < info.nenv; i++) {
        info.env[i] = libclone(a, env[i]);
        if (startswith(info.env[i], "TMPDIR=") && info.env[i].len>7) {
            l->tmpdir = (char *)cpath(a, cuthead(info.env[i], 7));
        }
    }
    if (cmd->cwd) {
        info.cwd = libclone(a, cmd->cwd);
    } else {
        char *cwd = getcwd((char *)allocbytes(a, 4096), 4096);
        info.cwd  = cwd ? fromcstr(cwd) : Str("/");
    }
    info.pid  = l->pid;
    info.ppid = cmd->ppid ? cmd->ppid : getpid();
    info.uid  = getuid();
    info.euid = geteuid();
    info.gid  = getgid();
    info.egid = getegid();
    char *host = (char *)allocbytes(a, 256);
    if (!gethostname(host, 255)) {
        host[255] = 0;
        info.host = fromcstr(host);
    }
    info.machtype = machtype(a);
    info.reserve  = l->cfg.reserve;
    info.stack    = l->cfg.stack;
    info.sigign   = os->entryign;

    // The run's own copies of the host's descriptors, which it closes
    for (i32 i = 0; i < 3; i++) {
        i32 fd;
        if (cmd->std[i] < 0) {
            fd = highfd(open("/dev/null", O_RDWR|O_CLOEXEC));
        } else {
            fd = fcntl((i32)cmd->std[i], F_DUPFD_CLOEXEC, 10);
        }
        if (fdtake(l, fd) < 0) lib_abort(os, E_BADF);
        info.std[i] = (Handle)fd;
    }
    l->errfd = (i32)info.std[2];
    info.inherit = alloc<InheritFd>(a, cmd->nfds);
    for (i32 i = 0; i < cmd->nfds; i++) {
        i32 src = (i32)cmd->fds[i].h;
        i32 fl  = fcntl(src, F_GETFL);
        i32 fd  = fl<0 ? -1 : fcntl(src, F_DUPFD_CLOEXEC, 10);
        if (fdtake(l, fd) < 0) lib_abort(os, E_BADF);
        InheritFd *e = info.inherit + info.ninherit++;
        e->fd    = cmd->fds[i].fd;
        e->h     = (Handle)fd;
        e->flags = (fl&O_ACCMODE)==O_RDONLY ? OF_READ :
                   (fl&O_ACCMODE)==O_WRONLY ? OF_WRITE : OF_READ|OF_WRITE;
    }

    l->status = sh_main(os, &info);
    l->inrun  = 0;
    os_switch(os, &os->fibers.main);
    __builtin_trap();  // never resumed
}

// Everything the run took, back: from any state setup or an abort left.
static void teardown(Lib *l)
{
    Os *os = &l->os;
    l->inrun = 0;
    flushw(os);

    // Out of memory: said on the run's stderr, if that cannot block
    if (l->why == E_NOMEM) {
        i32 err = l->errfd;
        struct pollfd p = {};
        p.fd     = err;
        p.events = POLLOUT;
        if (err>=0 && poll(&p, 1, 0)==1 && (p.revents & POLLOUT)) {
            Str msg = "bash: out of memory\n";
            if (write(err, msg.data, (uz)msg.len)) {}
        }
    }

    // Every program, with whatever it started in its session
    for (i32 i = 0; i < l->nkids; i++) {
        LibKid *k = l->kids + i;
        kill(-(pid_t)k->pid, SIGKILL);
        if (!k->reaped) kill((pid_t)k->pid, SIGKILL);
    }
    i64 until = monons() + 1000000000;
    for (;;) {
        b32 left = 0;
        for (i32 i = 0; i < l->nkids; i++) {
            LibKid *k = l->kids + i;
            if (k->reaped) continue;
            i32   status;
            pid_t r = waitpid((pid_t)k->pid, &status, WNOHANG);
            if (r || errno!=EINTR) {
                if (r) reapedkid(l, k);
            }
            left |= !k->reaped;
        }
        if (!left || monons()>=until) break;
        poll(0, 0, 1);
    }
    i32 n = 0;
    for (i32 i = 0; i < l->nkids; i++) {
        if (!l->kids[i].reaped) l->kids[n++] = l->kids[i];  // later
    }
    l->nkids = n;

    if (l->dir) closedir(l->dir);
    l->dir = 0;
    for (i32 i = 0; i < 2; i++) {
        if (os->peekfd[i] > 0) close(os->peekfd[i]);
        os->peekfd[i] = 0;
    }
    for (i32 w = 0; w < FDBITS/64; w++) {
        for (u64 b = l->fdbits[w]; b; b &= b-1) {
            close(w*64 + __builtin_ctzll(b));
        }
        l->fdbits[w] = 0;
    }
    l->nfds = 0;

    for (LibRes *r = l->res, *next; r; r = next) {
        next = r->next;
        munmap(r, (uz)r->len);
    }
    l->res       = 0;
    l->committed = 0;
    l->prestack  = 0;
    os->pending  = 0;
    os->fibers   = {};

    if (l->masked) {
        #if __linux__
        // A SIGPIPE or SIGXFSZ the run raised on itself goes unseen
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        sigaddset(&set, SIGXFSZ);
        struct timespec zero = {};
        while (sigtimedwait(&set, 0, &zero) > 0) {}
        #endif
        sigprocmask(SIG_SETMASK, &l->mask, 0);
        l->masked = 0;
    }
    curlib = 0;
    __atomic_and_fetch(&l->req, ~REQ_HARD, __ATOMIC_RELEASE);
}

static i32 setup(Lib *l, ub_cmd const *cmd)
{
    Os *os = &l->os;
    if (!cmd || cmd->argc<1 || !cmd->argv || cmd->nfds<0 ||
        (cmd->nfds && !cmd->fds) || cmd->pid<0 || cmd->pid>=(i64)1<<22) {
        return -E_INVAL;
    }
    for (i32 i = 0; i < cmd->argc; i++) {
        if (!cmd->argv[i]) return -E_INVAL;
    }
    for (i32 i = 0; i < cmd->nfds; i++) {
        if (cmd->fds[i].fd < 3) return -E_INVAL;
    }

    struct sigaction sa = {};
    sigaction(SIGCHLD, 0, &sa);
    if (sa.sa_handler==SIG_IGN || (sa.sa_flags & SA_NOCLDWAIT)) {
        return -E_CHILD;  // programs would vanish unwaited
    }
    #if __APPLE__
    // macOS sends a write's SIGPIPE to the process, not the thread
    i32 raised[] = {SIGPIPE, SIGXFSZ};
    for (i32 i = 0; i < countof(raised); i++) {
        sigaction(raised[i], 0, &sa);
        if (sa.sa_handler != SIG_IGN) return -E_INVAL;
    }
    #else
    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGPIPE);
    sigaddset(&block, SIGXFSZ);
    sigprocmask(SIG_BLOCK, &block, &l->mask);
    l->masked = 1;
    #endif
    curlib = l;

    // Reaped by now, perhaps, are the stragglers of earlier runs
    i32 n = 0;
    for (i32 i = 0; i < l->nkids; i++) {
        i32 status;
        if (!waitpid((pid_t)l->kids[i].pid, &status, WNOHANG)) {
            l->kids[n++] = l->kids[i];
        } else {
            reapedkid(l, l->kids+i);
        }
    }
    l->nkids = n;

    l->cmd      = cmd;
    l->why      = 0;
    l->status   = 0;
    l->tmpdir   = 0;
    l->errfd    = -1;
    l->pid      = cmd->pid ? cmd->pid : virtualpid();
    os->fibers  = {};
    os->pending = 0;
    os->wlen    = 0;
    os->werrfd  = -1;
    os->ttyraw  = 0;
    os->changed = 0;
    os->pipeign = 0;
    u64 ign = 0;
    for (i32 sig = 1; sig < 64 && sig < NSIG; sig++) {
        if (sig==SIGKILL || sig==SIGSTOP) continue;
        if (cmd->sigign>>(sig-1) & 1) ign |= corebit(sig);
    }
    os->entryign = ign;
    os->reserve  = l->cfg.reserve;
    os->stack    = l->cfg.stack;
    for (i32 i = 0; i < countof(os->wkind); i++) os->wkind[i] = 0;

    if (!tryarena(l, &l->arena, l->cfg.reserve)) return -E_NOMEM;
    l->prestack = libreserve(os, l->cfg.stack);
    if (!l->prestack || mprotect(l->prestack, (uz)l->cfg.stack, PROT_READ|PROT_WRITE)) {
        return -E_NOMEM;
    }
    mprotect(l->prestack, 1<<12, PROT_NONE);  // guard page
    l->shell = os_fiber(os, &l->arena, l->cfg.stack, runmain, l);
    return 0;
}

static i32 librun(Lib *l, ub_cmd const *cmd)
{
    i32 idle = 0;
    if (!__atomic_compare_exchange_n(&l->running, &idle, 1, 0,
                                     __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        return -E_BUSY;
    }
    i32 r = setup(l, cmd);
    if (!r) {
        os_switch(&l->os, l->shell);
        if (l->why) {
            r = -l->why;
        } else if (l->status & WSIGNALED) {
            r = WSIGNALED | (128 + hostsig((l->status & 255) - 128));
        } else {
            r = l->status & 255;
        }
    }
    teardown(l);
    __atomic_store_n(&l->running, 0, __ATOMIC_RELEASE);
    return r;
}

static Lib *libnew(ub_config const *c)
{
    void *p = mmap(0, sizeof(Lib), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if (p == MAP_FAILED) return 0;
    Lib *l = (Lib *)p;
    if (c) l->cfg = *c;
    b32 wide = sizeof(void *) == 8;
    if (!l->cfg.spawnlock) l->cfg.spawnlock = defaultlock;
    if (l->cfg.reserve <= 0) l->cfg.reserve = (iz)1 << (wide ? 34 : 26);
    if (l->cfg.stack <= 0) l->cfg.stack = (iz)1 << (wide ? 24 : 23);
    l->cfg.stack = (l->cfg.stack + 0xffff) & -(iz)0x10000;
    l->memlimit = l->cfg.memlimit;
    if (!l->memlimit) {
        iz ram = (iz)sysconf(_SC_PHYS_PAGES) * (iz)sysconf(_SC_PAGESIZE);
        iz max = wide ? (iz)1<<30 : (iz)1<<28;
        l->memlimit = ram>0 && ram/4<max ? ram/4 : max;
    } else if (l->memlimit < 0) {
        l->memlimit = -1;
    }
    l->maxfds = l->cfg.fds;
    if (l->maxfds <= 0) {
        struct rlimit rl;
        l->maxfds = 256;
        if (!getrlimit(RLIMIT_NOFILE, &rl) && rl.rlim_cur!=RLIM_INFINITY &&
            rl.rlim_cur/4 < 256) {
            l->maxfds = rl.rlim_cur/4<16 ? 16 : (i32)(rl.rlim_cur/4);
        }
    }

    // The wake pipe, written by ub_signal and ub_cancel from anywhere
    Lib *saved = curlib;
    curlib = l;
    i32 fds[2];
    i32 err = cloexecpipe(fds);
    curlib = saved;
    if (err) {
        munmap(l, sizeof(Lib));
        return 0;
    }
    for (i32 i = 0; i < 2; i++) {
        l->os.wakefd[i] = fds[i];
        fcntl(fds[i], F_SETFL, O_NONBLOCK);
    }
    return l;
}

static void libfree(Lib *l)
{
    if (!l) return;
    for (i32 i = 0; i < l->nkids; i++) {
        i32 status;
        if (waitpid((pid_t)l->kids[i].pid, &status, WNOHANG)) {
            reapedkid(l, l->kids+i);
        }
    }
    close(l->os.wakefd[0]);
    close(l->os.wakefd[1]);
    munmap(l, sizeof(Lib));
}

static i32 libsignal(Lib *l, i32 sig)
{
    i32 c = coresig(sig & 0xff);
    if (!c || (sig & ~(0xff|UB_GROUP))) return -E_INVAL;
    u64 volatile *bits = sig&UB_GROUP ? &l->os.grpbits : &l->os.sigbits;
    bitsor(bits, (u64)1<<(c-1));
    wakeup(l);
    return 0;
}

static void libcancel(Lib *l)
{
    __atomic_fetch_or(&l->req, REQ_HARD, __ATOMIC_RELEASE);
    wakeup(l);
}
