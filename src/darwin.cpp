// macOS's part of the POSIX layer: what it does that Linux does
// otherwise (linux.cpp), each declared in posix.cpp, after which a
// root includes this module.
// This is free and unencumbered software released into the public domain.

// A pipe. In the library both ends are close-on-exec from the start,
// since another thread of the host may spawn a program at any moment:
// made inside the host's spawn lock, then marked.
static i32 cloexecpipe(i32 fds[2])
{
    if constexpr (LIBMODE) {
        lib_spawnlock(1);
        i32 r = pipe(fds);
        if (!r) {
            fcntl(fds[0], F_SETFD, FD_CLOEXEC);
            fcntl(fds[1], F_SETFD, FD_CLOEXEC);
        }
        lib_spawnlock(0);
        return r;
    }
    return pipe(fds);
}

static iz plt_pipecap(Os *os)
{
    flushw(os);
    return 1<<14;  // macOS starts small
}

static iz plt_peek(Os *os, Handle, u8 *, iz)
{
    flushw(os);
    return -1;  // pipes cannot be peeked
}

static void stattimes(Stat *r, struct stat *s)
{
    r->mtime = (i64)s->st_mtimespec.tv_sec*1000000000 + s->st_mtimespec.tv_nsec;
    r->atime = (i64)s->st_atimespec.tv_sec*1000000000 + s->st_atimespec.tv_nsec;
    r->ctime = (i64)s->st_ctimespec.tv_sec*1000000000 + s->st_ctimespec.tv_nsec;
}

static b32 closerange(i32, i32)
{
    return 0;  // no such call: one at a time
}

// The core numbers signals as Linux x86-64 does (trap.cpp), and macOS
// numbers them otherwise. Every number that crosses is translated:
// hostsig for one going out, coresig for one coming in, each 0 where
// the other side has no such signal; corebit and hostin take a host
// number into the core's masks.
static i32 const hostsigs[32] = {
    0,       SIGHUP,  SIGINT,  SIGQUIT,   SIGILL,  SIGTRAP,  SIGABRT, SIGBUS,
    SIGFPE,  SIGKILL, SIGUSR1, SIGSEGV,   SIGUSR2, SIGPIPE,  SIGALRM, SIGTERM,
    0,       SIGCHLD, SIGCONT, SIGSTOP,   SIGTSTP, SIGTTIN,  SIGTTOU, SIGURG,
    SIGXCPU, SIGXFSZ, SIGVTALRM, SIGPROF, SIGWINCH, SIGIO,   0,       SIGSYS,
};  // 16 (STKFLT), 30 (PWR) and the real-time signals are Linux's own

static i32 hostsig(i32 sig)
{
    return sig>0 && sig<32 ? hostsigs[sig] : 0;
}

static i32 coresig(i32 sig)
{
    for (i32 i = 1; sig>0 && i<32; i++) {
        if (hostsigs[i] == sig) return i;
    }
    return 0;
}

static u64 corebit(i32 sig)
{
    i32 c = coresig(sig);
    return c ? (u64)1<<(c-1) : 0;
}

static b32 hostin(u64 set, i32 sig)
{
    return (set & corebit(sig)) != 0;
}

// posix_spawn beyond POSIX: a chdir action (10.15, named without _np
// from 26), and close-on-exec for all but the map
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 260000
#  define posix_spawn_file_actions_addchdir_np posix_spawn_file_actions_addchdir
#endif

// Whether posix_spawn can carry out the plan (trispawn): not handing a
// foreground group the terminal
static b32 spawnable(SpawnPlan *p)
{
    return !(p->pgid && p->fg);
}

// Before the map: nothing but the map open. The shell's own descriptors
// are all close-on-exec, but for the standard three it was given.
// Returns the flags to add.
static short spawnpre(posix_spawnattr_t *, posix_spawn_file_actions_t *,
                      SpawnPlan *, i32 *, i32)
{
    return POSIX_SPAWN_CLOEXEC_DEFAULT;
}

// After the map: the directory
static void spawnpost(posix_spawn_file_actions_t *fa, SpawnPlan *p, i32)
{
    posix_spawn_file_actions_addchdir_np(fa, p->cwd);
}

// Whether the terminal sent the signal to its group: macOS cannot tell
static b32 fromterminal(siginfo_t *)
{
    return 0;
}

// RL_* as the system numbers them, -1 where it has no such limit
static i32 rlres(i32 res)
{
    switch (res) {
    case RL_CORE:       return RLIMIT_CORE;
    case RL_DATA:       return RLIMIT_DATA;
    case RL_FSIZE:      return RLIMIT_FSIZE;
    case RL_MEMLOCK:    return RLIMIT_MEMLOCK;
    case RL_RSS:        return RLIMIT_RSS;
    case RL_NOFILE:     return RLIMIT_NOFILE;
    case RL_STACK:      return RLIMIT_STACK;
    case RL_CPU:        return RLIMIT_CPU;
    case RL_NPROC:      return RLIMIT_NPROC;
    case RL_AS:         return RLIMIT_AS;
    }
    return -1;
}

// No real-time signals: a level signal reaches no other shell
static i32 plt_siglevel(Os *os, i64, i64, i32)
{
    flushw(os);
    return E_NOSYS;
}

static void initlevel() {}

static u8 fifokind(struct stat *st)
{
    return st->st_dev ? FK_NFIFO : FK_FIFO;  // a pipe's st_dev is 0
}

// macOS poll (and kqueue, on which it is built) never reports that a
// named FIFO's writers have all closed, and select does so only until a
// read first returns 0 there. A read waiting at the end would wait for
// ever, though read would return 0 at once. So while such a read is
// pending, waits are bounded and the read is tried through a descriptor
// of its own, opened by path without blocking: that reads what is
// there, 0 with no writer, or fails (EAGAIN) while a writer is idle.
// The shared descriptor's flags are never touched. Unlinked or moved,
// the FIFO is not reopened, and its end is not seen.
static b32 isnamedread(Os *os, Io *io)
{
    return io->op==IO_READ && io->len && fdkind(os, (i32)io->h)==FK_NFIFO;
}

static b32 fifoend(Io *io)
{
    // F_GETPATH still gives a path once the FIFO is unlinked, renamed
    // or replaced, and opening another FIFO would release its writer
    // only to close on it: look before opening, and again after
    struct stat a, b;
    char path[MAXPATHLEN];
    if (fstat((i32)io->h, &a) || !a.st_nlink) return 0;
    if (fcntl((i32)io->h, F_GETPATH, path)) return 0;
    if (lstat(path, &b) || !S_ISFIFO(b.st_mode) ||
        a.st_dev!=b.st_dev || a.st_ino!=b.st_ino) return 0;
    i32 fd = open(path, O_RDONLY|O_NONBLOCK|O_CLOEXEC);
    if (fd < 0) return 0;
    b32 same = !fstat(fd, &b) && a.st_dev==b.st_dev && a.st_ino==b.st_ino;
    iz n = -1;
    if (same) {
        iz len = io->len<(1<<30) ? io->len : (1<<30);
        do {
            n = read(fd, io->buf, (uz)len);
        } while (n<0 && errno==EINTR);
    }
    close(fd);
    if (n < 0) return 0;
    io->n   = n;
    io->err = 0;
    return 1;
}

// Complete a pending read of a named FIFO that has ended, or has data
// poll missed. *any says whether such a read is pending.
static Io *fifoends(Os *os, b32 *any)
{
    *any = 0;
    for (Io **p = &os->pending; *p; p = &(*p)->next) {
        Io *io = *p;
        if (!isnamedread(os, io)) continue;
        *any = 1;
        if (fifoend(io)) {
            *p = io->next;
            io->done = 1;
            return io;
        }
    }
    return 0;
}

// MACHTYPE's os, with the kernel release appended, and its vendor
// (machtype)
static Str machos(Str *vendor, b32 *)
{
    *vendor = "apple";
    return "darwin";
}

// The open descriptors from 3 to below max (inherited). macOS's poll
// cannot tell: asked for no events it reports nothing for a closed
// descriptor, and asked for any it reports POLLNVAL for an open device,
// so the kernel lists them.
static Slice<InheritFd> openfds(Arena *a, i32 max)
{
    Slice<InheritFd> r = {};
    i32 size = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, 0, 0);
    if (size <= 0) return r;
    iz cap = size / (iz)sizeof(proc_fdinfo);
    proc_fdinfo *list = alloc<proc_fdinfo>(a, cap ? cap : 1);
    size = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, list, (i32)(cap*(iz)sizeof(*list)));
    for (iz i = 0; size>0 && i<size/(iz)sizeof(*list); i++) {
        i32 fd = list[i].proc_fd;
        if (fd>=3 && fd<max) r = pushinherit(a, r, fd);
    }
    return r;
}
