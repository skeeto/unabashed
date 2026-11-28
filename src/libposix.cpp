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

// A hangup's SIGHUP, one per hangup (hangup, hupdup): the host forwarded
// one in this run (SEEN), the run made one (MADE), and the host's next
// is the made one's duplicate (DUP).
enum : u32 { HUP_SEEN = 1, HUP_MADE = 2, HUP_DUP = 4 };

// A reservation of the run's, after a header that links it.
struct LibRes {
    LibRes *next;
    LibRes *prev;
    iz      len;   // the whole mapping, header included
};
enum : iz { RESHEAD = (iz)1<<16 };  // a page on every system

struct TraceRec;

struct LibKid {
    i64       pid;
    i32       watch;   // Linux: its pidfd, or -1; macOS: in the kqueue
    b32       reaped;
    i32       status;  // once reaped by the core: as os_waitproc returns it
    TraceRec *trace;   // the command that started it, while it waits
    b32       held;    // exited, its zombie kept: its group still ours
    b32       owed;    // its CPU time is counted only once it is reaped
    i32       wstatus; // while held: its wait status
    b32       termed;  // its group was sent SIGTERM (a group signal)
    u32       ttysig[2];  // Lib.ttysig as it started (terminal mode)
};

// A program reaped and not held is forgotten: its pid, and so its
// group's id, may already be another's, so it is never signalled.
static b32 ours(LibKid *k)
{
    return !k->reaped || k->held;
}

enum : i32 { MAXKIDS = 1024, MAXHELD = 64, FDBITS = 1<<16 };

struct LibHooks;

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

    // Terminal mode (UB_TERMINAL with a terminal found): the host's
    // descriptor of its session's terminal, its mode as the run began,
    // and the devices a hangup is seen on (that terminal's, /dev/tty's)
    b32        term;
    i32        ttyfd;
    termios    ttystart;
    dev_t      ttydev[2];
    u32        hup;       // HUP_* (atomic)
    u64        kgrp;      // group signals of the script's own (kill 0)
    u32        ttysig[2]; // INTs and QUITs to every level so far (atomic):
                          // the terminal's, or the script's kill 0

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
    i32        nheld;     // kids held as zombies
    i32        waitfd;    // Linux: the run's epoll; macOS: a kqueue

    // CPU time (os_cputime): the run thread's as the run began, and
    // what the run's reaped programs used (cuser, csys)
    CpuTimes   cpu;

    // The hooks' state (libhooks.cpp), from the run's arena
    LibHooks  *hooks;
};

// The OS's own (liblinux.cpp, libdarwin.cpp), included after this
// module. Program exits come with no SIGCHLD. Linux watches each
// program's pidfd in one epoll descriptor, macOS its exit in a kqueue,
// and waits poll that one descriptor. A program not watched (an old
// kernel, the descriptor budget spent, already gone) is polled with
// WNOHANG, backing off.
static void watchkid(Lib *, LibKid *);
static b32  watching(LibKid *);
static void unwatchkid(Lib *, LibKid *);
static void drainwatch(Lib *);
static void unwatchall(Lib *);
static void rewatch(Lib *);
static i32  newwatch(Lib *);
static b32  hostthread(i64 tid);
static b32  blockraised(Lib *);
static void unraise();
static CpuTimes threadcpu();  // the calling thread's own

static __thread Lib *curlib;  // the run on this thread

// The hooks (libhooks.cpp, included after this module): hooksetup makes
// a run's state, and askhost asks the access hook, if any, of a file
// the script names, 0 or the error to fail with. The wrappers below ask
// before each platform call, but not of the shell's own (OF_OWN,
// ST_OWN).
static void hooksetup(Lib *);
static i32  askhost(Lib *, Arena, i32 op, i32 flags, Str path);
static i32  askspawn(Lib *, Arena *, Spawn *, i64 trace, Slice<Str> *wrap);
// The trace hook's records of each fiber's commands (TraceRec): its
// innermost open one (tracetop), whose START may be held (traceheld)
// until it spawns (tracespawning), linked to its program (tracelink)
// until the reap (tracereaped); a fiber's end (traceunfiber), and the
// run's (traceleave).
static TraceRec *tracetop(Lib *);
static b32  traceheld(TraceRec *);
static i64  tracespawning(Lib *, Arena, TraceRec *);
static void tracelink(Lib *, TraceRec *, LibKid *);
static void tracereaped(Lib *, LibKid *, i32 status);
static void traceunfiber(Lib *, uz fiber);
static void traceleave(Lib *);

static Lib *runlib()
{
    return curlib;
}


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
    os_switchfinal(os, &os->fibers.main);
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

static Str lib_root(Os *)
{
    return {};  // Windows only
}

static b32 lib_sysroot(Os *)
{
    return 0;  // Windows only
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
    traceunfiber((Lib *)os, (uz)p);
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
    if (!(flags & OF_OWN)) {
        i32 err = askhost((Lib *)os, scratch, UB_OPEN, flags&~(OF_OWN|OF_EXACT), path);
        if (err) return {BADHANDLE, err};
    }
    Opened r = plt_open(os, scratch, path, flags&~OF_OWN, mode);
    if (r.h!=BADHANDLE && fdtake((Lib *)os, (i32)r.h)<0) {
        r.h   = BADHANDLE;
        r.err = E_MFILE;
    }
    return r;
}

static Opened libtempfile(Os *os, Arena scratch)
{
    i32 flags = UB_READ|UB_WRITE|UB_CREATE|UB_EXCL;
    i32 err   = askhost((Lib *)os, scratch, UB_TEMPFILE, flags, tempdir(os));
    if (err) return {BADHANDLE, err};
    Opened r = plt_tempfile(os, scratch);
    if (r.h!=BADHANDLE && fdtake((Lib *)os, (i32)r.h)<0) {
        r.h   = BADHANDLE;
        r.err = E_MFILE;
    }
    return r;
}

// A rename is asked as an open of its target that replaces it
static i32 librename(Os *os, Arena scratch, Str from, Str to)
{
    i32 err = askhost((Lib *)os, scratch, UB_OPEN, UB_WRITE|UB_CREATE|UB_TRUNC, to);
    return err ? err : plt_rename(os, scratch, from, to);
}

static Stat libstat(Os *os, Arena scratch, Str path, b32 follow)
{
    b32 own = follow & ST_OWN;
    follow &= ~ST_OWN;
    if (!own) {
        i32 err = askhost((Lib *)os, scratch, UB_STAT, follow ? 0 : UB_NOFOLLOW, path);
        if (err) {
            Stat s = {};
            s.err = err;
            return s;
        }
    }
    return plt_stat(os, scratch, path, follow);
}

static Str libreadlink(Os *os, Arena *a, Str path)
{
    if (askhost((Lib *)os, *a, UB_READLINK, UB_NOFOLLOW, path)) return {};
    return plt_readlink(os, a, path);
}

static Dirent *liblist(Os *os, Arena *a, Str dir, i32 *err)
{
    *err = askhost((Lib *)os, *a, UB_LIST, UB_READ, dir);
    return *err ? 0 : plt_list(os, a, dir, err);
}

static i32 libclose(Os *os, Handle h)
{
    Lib *l = (Lib *)os;
    if (!fdowned(l, (i64)h)) {
        flushw(os);
        return h==BADHANDLE ? 0 : wfailed(os, (i32)h);  // never the host's
    }
    fdgive(l, (i32)h);
    return plt_close(os, h);
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

// Programs start sessions of their own, apart from the host's terminal,
// but in terminal mode, where they stay in the host's session and group,
// as under bash +m: /dev/tty opens, and the terminal's ^C reaches them.
static b32 lib_ownsession()
{
    Lib *l = curlib;
    return !l || !l->term;
}


// Programs

static LibKid *findkid(Lib *l, i64 pid)
{
    for (i32 i = 0; i < l->nkids; i++) {
        if (l->kids[i].pid == pid) return l->kids + i;
    }
    return 0;
}

// A process wait pending for pid?
static b32 procpending(Os *os, i64 pid)
{
    for (Io *io = os->pending; io; io = io->next) {
        if (isproc(io) && (i64)io->h==pid) return 1;
    }
    return 0;
}

static void reapedkid(Lib *l, LibKid *k)
{
    if (k->reaped) return;
    k->reaped = 1;
    unwatchkid(l, k);
    if (l->cfg.child) l->cfg.child(l->cfg.user, k->pid, 0);
}

// Programs that outlived their handle, killed but not yet reapable
// (stuck in the kernel): any later call of any handle reaps them.
static i64 graves[64];
static i32 gravelock;

static void lockgraves(b32 lock)
{
    if (lock) {
        while (__atomic_exchange_n(&gravelock, 1, __ATOMIC_ACQUIRE)) sched_yield();
    } else {
        __atomic_store_n(&gravelock, 0, __ATOMIC_RELEASE);
    }
}

static void reapgraves()
{
    lockgraves(1);
    for (i32 i = 0; i < countof(graves); i++) {
        i32 status;
        if (graves[i] && waitpid((pid_t)graves[i], &status, WNOHANG)) graves[i] = 0;
    }
    lockgraves(0);
}

// Also terminal mode's leftovers (leavekids), which run on. Past 64 at
// once, the rest are never reaped here: zombies until the host exits.
static void bury(i64 pid)
{
    lockgraves(1);
    for (i32 i = 0; i < countof(graves); i++) {
        if (!graves[i]) {
            graves[i] = pid;
            break;
        }
    }  // else a zombie left for good: 64 programs stuck at once
    lockgraves(0);
}

static void countcpu(Lib *l, struct rusage const *ru)
{
    l->cpu.cuser += tvns(ru->ru_utime);
    l->cpu.csys  += tvns(ru->ru_stime);
}

// Terminal mode: a program that died of the terminal's INT or QUIT,
// which the kernel sent it as it sent the host, whose forwarder has yet
// to pass it on. The level waiting for the program acts on such a
// signal only if it has it as the program ends (waitdone), as bash,
// which the kernel signals at once, always has. So the run waits for it
// a little (but for a cancel): it comes at once from a host that keeps
// up, and never if another sent the signal, after which the run goes on
// without it.
static void awaittty(Lib *l, LibKid *k, i32 status)
{
    i32 i = status==(WSIGNALED|(128+2)) ? 0 : status==(WSIGNALED|(128+3)) ? 1 : -1;
    if (!l->term || i<0) return;
    i64 until = monons() + 250000000;
    while (__atomic_load_n(l->ttysig+i, __ATOMIC_ACQUIRE) == k->ttysig[i]) {
        i64 left = (until - monons() + 999999) / 1000000;
        if (left<=0 || (__atomic_load_n(&l->req, __ATOMIC_ACQUIRE) & REQ_HARD)) break;
        struct pollfd p = {};
        p.fd     = l->os.wakefd[0];
        p.events = POLLIN;
        if (poll(&p, 1, (i32)left) > 0) {
            u8 buf[64];
            while (read(l->os.wakefd[0], buf, sizeof(buf)) > 0) {}
        }
    }
}

static void lib_reaped(i64 pid, i32 status, struct rusage const *ru)
{
    Lib    *l = curlib;
    LibKid *k = findkid(l, pid);
    if (k) {
        awaittty(l, k, status);
        k->status = status;
        reapedkid(l, k);
        tracereaped(l, k, status);
    }
    if (ru) countcpu(l, ru);
}

// An exit is read without reaping (WNOWAIT): the zombie keeps its pid,
// and with it its group's id, from reuse while the run may yet signal
// the group (libsiggroup, teardown's kill). Zombies are reaped after
// teardown's kill, or oldest first past MAXHELD, their groups then
// forgotten (ours). Stops and continues are consumed, as wait4 does.

// Reap a held zombie, its group forgotten.
static void release(Lib *l, LibKid *k)
{
    if (!k->held) return;
    k->held = 0;
    l->nheld--;
    i32 status;
    struct rusage ru = {};
    pid_t r;
    do {
        r = wait4((pid_t)k->pid, &status, WNOHANG, &ru);
    } while (r<0 && errno==EINTR);
    if (r>0 && k->owed) countcpu(l, &ru);
    k->owed = 0;
}

// Is pid alone in the group it leads? Then its zombie need not be held.
static b32 alone(pid_t pid)
{
    #if __APPLE__
    pid_t pids[64];
    i32 len = proc_listpids(PROC_PGRP_ONLY, (u32)pid, pids, sizeof(pids));
    if (len<=0 || len>=(i32)sizeof(pids)) return 0;
    for (i32 i = 0; i < len/(i32)sizeof(pid_t); i++) {
        if (pids[i] && pids[i]!=pid) return 0;
    }
    return 1;
    #else
    (void)pid;
    return 0;
    #endif
}

// A program's wait status as the siginfo of waitid gives it
static i32 wstatusof(siginfo_t const *si)
{
    switch (si->si_code) {
    case CLD_EXITED:    return (si->si_status & 0xff) << 8;
    case CLD_KILLED:    return si->si_status & 0x7f;
    case CLD_DUMPED:    return (si->si_status & 0x7f) | 0x80;
    case CLD_STOPPED:
    case CLD_TRAPPED:   return (si->si_status & 0xff)<<8 | 0x7f;
    #if __APPLE__
    case CLD_CONTINUED: return SIGCONT<<8 | 0x7f;
    #else
    case CLD_CONTINUED: return 0xffff;
    #endif
    }
    return 0;
}

// wait4(pid, status, WNOHANG|flags, ru) for the run's programs, but an
// exit leaves pid a zombie held (above). *ru is zero when the program's
// time is counted only once it is reaped.
static pid_t lib_wait4(pid_t pid, i32 *status, i32 flags, struct rusage *ru)
{
    Lib    *l = curlib;
    LibKid *k = findkid(l, pid);
    *ru = {};
    if (!k || !ours(k)) return wait4(pid, status, WNOHANG|flags, ru);
    if (k->held) {
        *status = k->wstatus;
        return pid;
    }

    siginfo_t si = {};
    #if __linux__ && __LP64__
    // The system call fills in rusage, as wait4 would; glibc's does not
    b32  owed = 0;
    long r    = syscall(SYS_waitid, P_PID, (long)pid, &si, WEXITED|WNOHANG|WNOWAIT, ru);
    #else
    b32  owed = 1;
    long r    = waitid(P_PID, (id_t)pid, &si, WEXITED|WNOHANG|WNOWAIT);
    #endif
    if (r < 0) return -1;
    if (si.si_pid == pid) {
        *status = wstatusof(&si);
        if (owed && alone(pid)) {
            // Nothing else holds the group's id: reaped now, its time
            // counted at once
            return wait4(pid, status, WNOHANG, ru);
        }
        if (l->nheld == MAXHELD) {
            for (i32 i = 0; i < l->nkids; i++) {
                if (l->kids[i].held) {
                    release(l, l->kids+i);
                    break;
                }
            }
        }
        k->held    = 1;
        k->owed    = owed;
        k->wstatus = *status;
        l->nheld++;
        return pid;
    }

    if (!(flags & WUNTRACED)) return 0;
    si = {};
    do {
        r = waitid(P_PID, (id_t)pid, &si, WSTOPPED|WCONTINUED|WNOHANG);
    } while (r<0 && errno==EINTR);
    if (r < 0) return -1;
    if (si.si_pid != pid) return 0;
    *status = wstatusof(&si);
    return pid;
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
        } else {
            release(l, l->kids+i);
        }
    }
    l->nkids = n;
    return n < MAXKIDS;
}

// What the kernel would make of a program, for the spawn hook: E_ACCES
// for no regular file, E_NOEXEC for text without #!, which the core then
// runs as shell code itself; else 0, for the kernel to decide, as for a
// file it cannot read. A raw open: no yield, no descriptor from the
// run's budget, nothing asked of the access hook.
static i32 kernelsniff(Arena scratch, Str path)
{
    char *p = cpath(&scratch, path);
    if (access(p, X_OK)) return 0;
    i32 fd = open(p, O_RDONLY|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
    if (fd < 0) return 0;
    struct stat st;
    u8 head[80];
    iz n   = 0;
    b32 reg = !fstat(fd, &st) && S_ISREG(st.st_mode);
    if (reg) n = read(fd, head, sizeof(head));
    close(fd);
    if (!reg) return E_ACCES;
    if (n<0 || (n>=2 && head[0]=='#' && head[1]=='!')) return 0;
    for (iz i = 0; i < n; i++) {
        if (!head[i]) return 0;
    }
    return E_NOEXEC;
}

// The spawn hook's, and a held START's (the sniff decides whether it
// is shell code after all): the program it asked of, or a wrapper that
// starts it, wrap[0] with wrap[1...], the path, and its argv
static Spawn *asked(Lib *l, Arena *a, Spawn *s, TraceRec *t, i32 *err)
{
    *err = kernelsniff(*a, s->path);
    if (*err) return s;
    i64 id = tracespawning(l, *a, t);
    Slice<Str> wrap = {};
    *err = askspawn(l, a, s, id, &wrap);
    if (*err || !wrap.len) return s;
    Spawn *w = alloc<Spawn>(a);
    *w = *s;
    iz n = wrap.len;
    w->path = wrap[0];
    w->argc = n + 1 + s->argc;
    w->argv = alloc<Str>(a, w->argc);
    for (iz i = 0; i < n; i++) w->argv[i] = wrap[i];
    w->argv[n] = s->path;
    for (iz i = 0; i < s->argc; i++) w->argv[n+1+i] = s->argv[i];
    return w;
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
    TraceRec *t = tracetop(l);
    if (l->cfg.spawn || traceheld(t)) {
        i32 err = 0;
        s = asked(l, &scratch, s, t, &err);
        if (err) {
            Spawned r = {};
            r.proc = BADHANDLE;
            r.err  = err;
            return r;
        }
    }
    Spawned r = plt_spawn(os, scratch, s);
    if (!r.err) {
        LibKid *k = l->kids + l->nkids++;
        *k        = {};
        k->pid    = r.pid;
        for (i32 i = 0; i < 2; i++) {
            k->ttysig[i] = __atomic_load_n(l->ttysig+i, __ATOMIC_ACQUIRE);
        }
        tracelink(l, t, k);
        watchkid(l, k);
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
        struct rusage ru = {};
        pid_t r = lib_wait4((pid_t)proc, &status, 0, &ru);
        if (r<0 && errno==EINTR) continue;
        if (r) {
            i32 st = r<0 ? 127 : waitstatus(status);
            lib_reaped((i64)proc, st, r<0 ? 0 : &ru);
            return st;
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
        // check point (libsiggroup), not the host's group
        if (sig) {
            if (sig<1 || sig>64) return E_INVAL;
            bitsor(&os->grpbits, (u64)1<<(sig-1));
            l->kgrp |= (u64)1<<(sig-1);
            if (sig==2 || sig==3) {
                __atomic_add_fetch(l->ttysig+(sig-2), 1, __ATOMIC_RELEASE);
            }
            wakeup(l);
        }
        return 0;
    }
    i64 host = getpid();
    if (pid==-1 || pid==host || pid==-host || pid==-(i64)getpgrp() ||
        pid<-(i64)0x7fffffff || pid>(i64)0x7fffffff || hostthread(pid)) {
        return sig ? E_PERM : pid==-1 ? E_PERM : 0;
    }
    return plt_kill(os, pid, sig);
}


// Waiting: the wake pipe, the descriptors of submitted reads and
// writes, and process exits, which come with no SIGCHLD: those are
// polled, backing off from 1 to 64 ms while any is pending.

// Terminal mode: the run's terminal hung up (its master closed, its
// line dropped), as a read or write of it sees, submitted (libsubmit)
// or waited for (libwait): Linux's hung up descriptors poll POLLHUP, as
// do macOS's (TS_ZOMBIE, EV_EOF). That is taken as the kernel's SIGHUP
// to the session's leader, the host, so that the shell takes it before
// the read's end of file, however the host's forwarded one races it.
// One per hangup: none if the host forwarded one already in this run,
// and the host's next is this one's duplicate (hupdup). True if made.
static b32 hangup(Lib *l, i32 fd, i32 revents)
{
    struct stat st;
    if (!(revents & POLLHUP) ||
        (__atomic_load_n(&l->hup, __ATOMIC_RELAXED) & (HUP_SEEN|HUP_MADE)) ||
        fstat(fd, &st) || !S_ISCHR(st.st_mode) ||
        (st.st_rdev!=l->ttydev[0] && st.st_rdev!=l->ttydev[1])) {
        return 0;
    }
    u32 v = __atomic_load_n(&l->hup, __ATOMIC_RELAXED);
    do {
        if (v & (HUP_SEEN|HUP_MADE)) return 0;
    } while (!__atomic_compare_exchange_n(&l->hup, &v, v|HUP_MADE|HUP_DUP, 0,
                                          __ATOMIC_ACQ_REL, __ATOMIC_RELAXED));
    bitsor(&l->os.sigbits, corebit(SIGHUP));
    return 1;
}

static void libsubmit(Os *os, Io *io)
{
    Lib *l = (Lib *)os;
    if (io->op==IO_READ || io->op==IO_WRITE) {
        if (fdkind(os, (i32)io->h) == FK_REG) {
            // A file never blocks: done at once (and coalesced)
            io->done = 1;
            io->next = 0;
            if (io->op == IO_WRITE) {
                b32 ok  = plt_write(os, io->h, io->buf, io->len);
                io->n   = ok ? io->len : 0;
                io->err = ok ? 0 : os->werr;
            } else {
                iz n    = plt_read(os, io->h, io->buf, io->len);
                io->n   = n<0 ? 0 : n;
                io->err = n<0 ? E_IO : 0;
            }
            return;
        }
        u8 kind = FK_UNKNOWN;
        if (io->op==IO_READ && io->len) kind = fdkind(os, (i32)io->h);
        if (kind==FK_FIFO || kind==FK_NFIFO) {
            // read takes a pipe a byte at a time. While data is waiting,
            // skip plt_submit's poll: FIONREAD then read, as in tryio,
            // and nothing between them in which another reader could
            // empty the pipe (pending writes go out first)
            flushw(os);
            i32 avail = 0;
            if (!ioctl((i32)io->h, FIONREAD, &avail) && avail>0) {
                iz n     = plt_read(os, io->h, io->buf, io->len<avail ? io->len : avail);
                io->done = 1;
                io->next = 0;
                io->n    = n<0 ? 0 : n;
                io->err  = n<0 ? coreerr(errno) : 0;
                return;
            }
        }
    }
    if (l->term && (io->op==IO_READ || io->op==IO_WRITE) &&
        !(__atomic_load_n(&l->hup, __ATOMIC_RELAXED) & (HUP_SEEN|HUP_MADE)) &&
        hangup(l, (i32)io->h, pollnow(io, 0))) {
        // Not tried, its end of file the core's only after the SIGHUP
        flushw(os);
        if (io->op == IO_READ) os->readyn = 0;
        io->done = 0;
        io->next = 0;
        Io **tail = &os->pending;
        while (*tail) tail = &(*tail)->next;
        *tail = io;
        return;
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
    for (i32 backoff = 1;; backoff = backoff<64 ? backoff*2 : 64) {
        checkhard(l);
        Io *done = takeproc(os);
        if (done) return done;
        b32 named = 0;  // a pending read of a named FIFO (macOS)
        done = fifoends(os, &named);
        if (done) return done;

        b32 procs   = 0;  // a pending program not watched
        b32 watched = 0;
        iz  max     = 2;
        for (Io *io = os->pending; io; io = io->next) max += !isproc(io);
        struct pollfd *fds = pollroom(os, max);
        iz n = 0;
        fds[n].fd        = os->wakefd[0];
        fds[n].events    = POLLIN;
        fds[n++].revents = 0;
        fds[n].fd        = l->waitfd;
        fds[n].events    = POLLIN;
        fds[n++].revents = 0;
        for (Io *io = os->pending; io; io = io->next) {
            if (isproc(io)) {
                LibKid *k = findkid(l, (i64)io->h);
                b32 w = k && watching(k);
                watched |= w;
                procs   |= !w;
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
        if ((procs || named) && (timeout<0 || timeout>backoff)) {
            timeout = backoff;
        }
        if (!watched) fds[1].fd = -1;  // ignored
        i32 r = poll(fds, (nfds_t)n, timeout);
        if (r<0 && errno!=EINTR) return 0;
        if (fds[0].revents) {
            u8 buf[64];
            while (read(os->wakefd[0], buf, sizeof(buf)) > 0) {}
        }
        if (fds[1].revents) drainwatch(l);
        checkhard(l);
        for (iz i = 2; l->term && r>0 && i<n; i++) hangup(l, fds[i].fd, fds[i].revents);
        if (os->sigbits || os->grpbits) return 0;  // the core first

        iz k = 2;
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

// In terminal mode the INT, QUIT and TSTP the host forwards are the
// terminal's, which the kernel sent to its foreground group: the host's,
// as the run never moves the terminal, and so its programs too. They
// are group signals, which the core takes next (libsiggroup).
static u64 libsigpending(Os *os)
{
    Lib *l = (Lib *)os;
    checkhard(l);
    u64 bits = plt_sigpending(os);
    if (l->term) {
        u64 tty = corebit(SIGINT) | corebit(SIGQUIT) | corebit(SIGTSTP);
        if (bits & tty) bitsor(&os->grpbits, bits & tty);
        bits &= ~tty;
    }
    return bits;
}

// The group a program leads, should it have one: every program does, a
// session's leader, but in terminal mode, where only one that made its
// own leads one. Never the host's.
static void killgroup(LibKid *k, i32 sig)
{
    pid_t pg = (pid_t)k->pid;
    if (pg>1 && pg!=getpgrp()) kill(-pg, sig);
}

// A group signal the core takes goes to every program the run started,
// with its group, then to every level (ossignals). In terminal mode the
// terminal's (and the host's UB_GROUP) reached them already, or never
// will: only the script's own (kill 0) go to each, by its pid.
static u64 libsiggroup(Os *os)
{
    Lib *l    = (Lib *)os;
    u64  bits = plt_siggroup(os);
    u64  own  = l->kgrp;
    l->kgrp = 0;
    for (u64 b = l->term ? own : bits; b; b &= b-1) {
        i32 hsig = hostsig(__builtin_ctzll(b) + 1);
        if (!hsig) continue;
        for (i32 i = 0; i < l->nkids; i++) {
            LibKid *k = l->kids + i;
            if (!ours(k)) continue;
            if (l->term) {
                kill((pid_t)k->pid, hsig);
            } else {
                killgroup(k, hsig);
            }
            k->termed |= hsig == SIGTERM;
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

// The terminal of the host's session, which /dev/tty opens too
static b32 sessiontty(i32 fd)
{
    pid_t sid = tcgetsid(fd);
    return sid>0 && sid==getsid(0);
}

// In terminal mode, the modes of the host's session's terminal, which
// teardown gives back; no other terminal's, ever
static Tty libtty(Os *os, Handle in, Handle out, i32 how)
{
    Lib *l = (Lib *)os;
    if (l->term && sessiontty((i32)in) && sessiontty((i32)out)) {
        return plt_tty(os, in, out, how);
    }
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

// The run's: its thread's CPU time since it began, not the host's
// other threads', and its own programs' as it reaped them, not the
// host's children's
static CpuTimes libcputime(Os *os)
{
    Lib     *l = (Lib *)os;
    CpuTimes c = threadcpu();
    c.user -= l->cpu.user;
    c.sys  -= l->cpu.sys;
    c.cuser = l->cpu.cuser;
    c.csys  = l->cpu.csys;
    return c;
}

static constexpr OsVtable libtable(OsVtable t)
{
    t.reserve    = libreserve;
    t.commit     = libcommit;
    t.decommit   = libdecommit;
    t.open       = libopen;
    t.tempfile   = libtempfile;
    t.rename     = librename;
    t.stat       = libstat;
    t.readlink   = libreadlink;
    t.list       = liblist;
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
    t.cputime    = libcputime;
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

// The run's own close-on-exec copy of a host descriptor, from 10 up
// where it can, as highfd: -1 with no descriptor to give. Ends the run
// with E_BADF if the host's is not open.
static i32 hostcopy(Os *os, i32 fd)
{
    if (!fdaccess(fd)) lib_abort(os, E_BADF);
    i32 r = fcntl(fd, F_DUPFD_CLOEXEC, 10);
    return r<0 ? fcntl(fd, F_DUPFD_CLOEXEC, 3) : r;
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
        info.cwd = cwdname(a, info.env, info.nenv, &info.cwderr);
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

    // The run's own copies of the host's descriptors, which it closes. A
    // host descriptor that is not open is the host's error. One the run
    // has no descriptor to copy to (a low ulimit -n, its budget) it goes
    // without, as the executable goes without one its parent closed
    for (i32 i = 0; i < 3; i++) {
        i32 fd;
        if (cmd->std[i] < 0) {
            fd = highfd(open("/dev/null", O_RDWR|O_CLOEXEC));
        } else {
            fd = hostcopy(os, (i32)cmd->std[i]);
        }
        fd = fdtake(l, fd);
        info.std[i]      = fd<0 ? BADHANDLE : (Handle)fd;
        info.stdflags[i] = fd<0 ? 0 : fdaccess(fd);
    }
    l->errfd = (i32)info.std[2];
    info.inherit = alloc<InheritFd>(a, cmd->nfds);
    for (i32 i = 0; i < cmd->nfds; i++) {
        i32 src = (i32)cmd->fds[i].h;
        i32 fl  = fdaccess(src);
        i32 fd  = fdtake(l, hostcopy(os, src));
        if (fd < 0) continue;
        InheritFd *e = info.inherit + info.ninherit++;
        e->fd    = cmd->fds[i].fd;
        e->h     = (Handle)fd;
        e->flags = fl;
    }

    l->status = sh_main(os, &info);
    l->inrun  = 0;
    os_switchfinal(os, &os->fibers.main);
    __builtin_trap();  // never resumed
}

// With ub_config.linger, the programs still running get a SIGTERM (and
// a SIGCONT, should one be stopped), as do the groups still held, and
// up to linger ms to exit before teardown's kill. Exits are read with
// WNOWAIT, so the zombies keep the groups' ids for that kill. Each
// group gets one SIGTERM per stop: not another after a group signal
// sent it one, as a trap that runs once per TERM (dash's) would run
// twice. A program that left its group is sent its own, as is each in
// terminal mode, in the host's group: once, as for a group.
static void lingerkids(Lib *l)
{
    if (l->cfg.linger <= 0) return;
    b32 any = 0;
    for (i32 i = 0; i < l->nkids; i++) any |= !l->kids[i].reaped;
    if (!any) return;
    for (i32 i = 0; i < l->nkids; i++) {
        LibKid *k = l->kids + i;
        if (!ours(k)) continue;
        b32 apart = !k->reaped && getpgid((pid_t)k->pid)>0 &&
                    getpgid((pid_t)k->pid)!=(pid_t)k->pid;
        if (!k->termed) killgroup(k, SIGTERM);
        if (apart && !(l->term && k->termed)) kill((pid_t)k->pid, SIGTERM);
        k->termed = 1;
        killgroup(k, SIGCONT);
        if (apart) kill((pid_t)k->pid, SIGCONT);
    }
    i64 until = monons() + (i64)l->cfg.linger*1000000;
    for (;;) {
        b32 left = 0;
        for (i32 i = 0; i<l->nkids && !left; i++) {
            LibKid *k = l->kids + i;
            if (k->reaped) continue;
            siginfo_t si = {};
            i32 r = waitid(P_PID, (id_t)k->pid, &si, WEXITED|WNOHANG|WNOWAIT);
            left = !r && si.si_pid!=(pid_t)k->pid;
        }
        if (!left || monons()>=until) break;
        poll(0, 0, 1);
    }
}

// Terminal mode, ended other than by a cancel (exit, end of file, out of
// memory, a deadlock, a signal's death): the programs still running stay,
// as bash's would on its terminal. Under +m they are in the host's
// group, the terminal's in front, so the kernel's SIGHUP takes them as
// the host exits, but for those that ignore it or left. A shell dead of
// a HUP (a hangup's, say) was the terminal's controlling process, as
// bash would be, whose exit the kernel follows with SIGHUP and SIGCONT
// to the foreground group: those still in it get them now. The held
// zombies are reaped, as are those exited, and the rest buried, so that
// no later run signals or waits for them, and the child hook told they
// are no longer the run's (live 0): their pids may later be another's.
static void leavekids(Lib *l)
{
    b32 hup = !l->why && l->status==(WSIGNALED|(128+1));
    for (i32 i = 0; i < l->nkids; i++) {
        LibKid *k = l->kids + i;
        release(l, k);
        if (k->reaped) continue;
        i32   status;
        pid_t r;
        do {
            r = waitpid((pid_t)k->pid, &status, WNOHANG);
        } while (r<0 && errno==EINTR);
        if (r) {
            reapedkid(l, k);
            tracereaped(l, k, r<0 ? 127 : waitstatus(status));
            continue;
        }
        if (hup && getpgid((pid_t)k->pid)==getpgrp()) {
            kill((pid_t)k->pid, SIGHUP);
            kill((pid_t)k->pid, SIGCONT);
        }
        unwatchkid(l, k);
        bury(k->pid);
        k->reaped = 1;
        if (l->cfg.child) l->cfg.child(l->cfg.user, k->pid, 0);
    }
    l->nkids = 0;
}

// Terminal mode: one run at a time per process holds the terminal
static i32 termclaim;

// UB_TERMINAL: the terminal is std[2]'s, else std[0]'s, and with neither
// the flag does nothing. The host leads that terminal's session with its
// own group in front, and gives its own pid as $$; else E_INVAL, and
// E_BUSY while another run of the process holds it. SIGTTIN and SIGTTOU
// are blocked on the run's thread, so that teardown can take the
// terminal back from a group that took it.
static i32 termsetup(Lib *l, ub_cmd const *cmd)
{
    if (!(l->cfg.flags & UB_TERMINAL)) return 0;
    i32 fd = -1;
    if (cmd->std[2]>=0 && isatty((i32)cmd->std[2])) {
        fd = (i32)cmd->std[2];
    } else if (cmd->std[0]>=0 && isatty((i32)cmd->std[0])) {
        fd = (i32)cmd->std[0];
    }
    if (fd < 0) return 0;
    pid_t self = getpid();
    if (getsid(0)!=self || tcgetsid(fd)!=self || tcgetpgrp(fd)!=getpgrp() ||
        cmd->pid!=(i64)self) {
        return -E_INVAL;
    }
    termios     start;
    struct stat st, cty;
    if (tcgetattr(fd, &start) || fstat(fd, &st)) return -E_INVAL;
    if (stat("/dev/tty", &cty)) cty.st_rdev = st.st_rdev;
    i32 idle = 0;
    if (!__atomic_compare_exchange_n(&termclaim, &idle, 1, 0,
                                     __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        return -E_BUSY;
    }
    l->term      = 1;
    l->ttyfd     = fd;
    l->ttystart  = start;
    l->ttydev[0] = st.st_rdev;
    l->ttydev[1] = cty.st_rdev;
    sigset_t tty;
    sigemptyset(&tty);
    sigaddset(&tty, SIGTTIN);
    sigaddset(&tty, SIGTTOU);
    pthread_sigmask(SIG_BLOCK, &tty, l->masked ? 0 : &l->mask);
    l->masked = 1;
    return 0;
}

// The terminal back to the host, however the run ended: its group in
// front, should a program have taken it, and its mode. After a cancel,
// the mode the run began with; otherwise the run's own change undone
// (the line editor's, read's), and a program's (stty) kept. Errors are
// ignored: a terminal hung up has nothing to give back.
static void termback(Lib *l)
{
    Os *os = &l->os;
    i32 fd = l->ttyfd;
    if (tcgetpgrp(fd) != getpgrp()) tcsetpgrp(fd, getpgrp());
    if (os->ttyraw && os->ttypaste && write(fd, "\x1b[?2004l", 8)) {}
    if (l->why == E_CANCELED) {
        tcsetattr(fd, TCSANOW, &l->ttystart);
    } else if (os->ttyraw) {
        tcsetattr(fd, TCSANOW, &os->ttysave);
    }
    os->ttyraw   = 0;
    os->ttypaste = 0;
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

    // Terminal mode leaves the programs running, but after a cancel
    b32 leave = l->term && l->why!=E_CANCELED;
    if (!leave) lingerkids(l);
    traceleave(l);  // what is open now ends only with its program
    if (leave) leavekids(l);

    // Every program, with whatever it started in its session (or in a
    // group it made, in terminal mode), while the held zombies keep
    // their groups' ids ours; then those reaped
    for (i32 i = 0; i < l->nkids; i++) {
        LibKid *k = l->kids + i;
        if (!ours(k)) continue;
        killgroup(k, SIGKILL);
        if (!k->reaped) kill((pid_t)k->pid, SIGKILL);
    }
    for (i32 i = 0; i < l->nkids; i++) release(l, l->kids+i);
    i64 until = monons() + 1000000000;
    for (;;) {
        b32 left = 0;
        for (i32 i = 0; i < l->nkids; i++) {
            LibKid *k = l->kids + i;
            if (k->reaped) continue;
            i32   status;
            pid_t r = waitpid((pid_t)k->pid, &status, WNOHANG);
            if (r || errno!=EINTR) {
                if (r) {
                    reapedkid(l, k);
                    tracereaped(l, k, r<0 ? 127 : waitstatus(status));
                }
            }
            left |= !k->reaped;
        }
        if (!left || monons()>=until) break;
        poll(0, 0, 1);
    }
    i32 n = 0;
    for (i32 i = 0; i < l->nkids; i++) {
        if (!l->kids[i].reaped) {
            l->kids[i].trace = 0;  // no END after the run
            l->kids[n++] = l->kids[i];  // later
        }
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
    unwatchall(l);  // the descriptors closed with the rest

    if (l->term) termback(l);

    os_fibersend(os);  // ASan: the fibers' fake stacks, on their stacks
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
        unraise();
        pthread_sigmask(SIG_SETMASK, &l->mask, 0);
        l->masked = 0;
    }
    if (l->term) {
        l->term = 0;
        __atomic_store_n(&termclaim, 0, __ATOMIC_RELEASE);
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
    if (!blockraised(l)) return -E_INVAL;
    curlib = l;

    // Reaped by now, perhaps, are the stragglers of earlier runs
    reapgraves();
    rewatch(l);
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
    l->kgrp     = 0;
    l->status   = 0;
    l->tmpdir   = 0;
    l->errfd    = -1;
    l->pid      = cmd->pid ? cmd->pid : virtualpid();
    l->cpu      = threadcpu();
    l->cpu.cuser = l->cpu.csys = 0;
    os->fibers  = {};
    os->pending = 0;
    os->wlen    = 0;
    os->wrun    = -1;
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
    // A HUP the host forwarded before the run is this run's (hupdup),
    // while a hangup's duplicate is still owed
    __atomic_and_fetch(&l->hup, HUP_DUP, __ATOMIC_ACQ_REL);
    if (os->sigbits & corebit(SIGHUP)) {
        __atomic_or_fetch(&l->hup, HUP_SEEN, __ATOMIC_ACQ_REL);
    }
    i32 err = termsetup(l, cmd);
    if (err) return err;

    if (!tryarena(l, &l->arena, l->cfg.reserve)) return -E_NOMEM;
    hooksetup(l);  // within the arena's first commit
    l->prestack = libreserve(os, l->cfg.stack);
    if (!l->prestack || mprotect(l->prestack, (uz)l->cfg.stack, PROT_READ|PROT_WRITE)) {
        return -E_NOMEM;
    }
    mprotect(l->prestack, 1<<12, PROT_NONE);  // guard page
    l->shell = os_fiber(os, &l->arena, l->cfg.stack, runmain, l);
    return 0;
}

// A run's status as ub_run returns it, from sh_main's. A signal death
// is in host numbering, or, for one of Linux's the host lacks (the core
// numbers as Linux), the plain status that the executable, unable to
// die by it, exits with (exitwith).
static i32 hoststatus(i32 status)
{
    if (!(status & WSIGNALED)) return status & 255;
    i32 hsig = hostsig((status & 255) - 128);
    return hsig ? WSIGNALED | (128 + hsig) : status & 255;
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
        r = l->why ? -l->why : hoststatus(l->status);
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
    l->waitfd = newwatch(l);
    reapgraves();
    return l;
}

static void libfree(Lib *l)
{
    if (!l) return;
    for (i32 i = 0; i < l->nkids; i++) {
        i32 status;
        if (l->kids[i].reaped) continue;
        if (waitpid((pid_t)l->kids[i].pid, &status, WNOHANG)) {
            reapedkid(l, l->kids+i);
        } else {
            bury(l->kids[i].pid);
        }
    }
    reapgraves();
    close(l->os.wakefd[0]);
    close(l->os.wakefd[1]);
    if (l->waitfd >= 0) close(l->waitfd);  // macOS's kqueue
    if (l->os.pollv) munmap(l->os.pollv, (uz)l->os.pollcap*sizeof(struct pollfd));
    munmap(l, sizeof(Lib));
}

// A HUP the host forwards: dropped as the duplicate of one the run made
// of a hangup (hangup), else noted, so that the run makes none.
static b32 hupdup(Lib *l)
{
    u32 v = __atomic_load_n(&l->hup, __ATOMIC_RELAXED);
    for (;;) {
        u32 w = v&HUP_DUP ? v&~HUP_DUP : v|HUP_SEEN;
        if (__atomic_compare_exchange_n(&l->hup, &v, w, 0, __ATOMIC_ACQ_REL,
                                        __ATOMIC_RELAXED)) {
            return (v & HUP_DUP) != 0;
        }
    }
}

static i32 libsignal(Lib *l, i32 sig)
{
    i32 c = coresig(sig & 0xff);
    if (c<1 || c>64 || (sig & ~(0xff|UB_GROUP))) return -E_INVAL;
    if (sig==SIGHUP && hupdup(l)) return 0;
    u64 volatile *bits = sig&UB_GROUP ? &l->os.grpbits : &l->os.sigbits;
    bitsor(bits, (u64)1<<(c-1));
    if (c==2 || c==3) __atomic_add_fetch(l->ttysig+(c-2), 1, __ATOMIC_RELEASE);
    wakeup(l);
    return 0;
}

static void libcancel(Lib *l)
{
    __atomic_fetch_or(&l->req, REQ_HARD, __ATOMIC_RELEASE);
    wakeup(l);
}

// ub_system's standard handles: the host's own descriptors
static intptr_t lib_hoststd(i32 i)
{
    return i;
}
