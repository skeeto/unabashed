// Linux's part of the POSIX layer: what it does that macOS does
// otherwise (darwin.cpp), each declared in posix.cpp, after which a
// root includes this module.
// This is free and unencumbered software released into the public domain.

// A pipe. In the library both ends are close-on-exec from the start,
// since another thread of the host may spawn a program at any moment.
static i32 cloexecpipe(i32 fds[2])
{
    if constexpr (LIBMODE) return pipe2(fds, O_CLOEXEC);
    return pipe(fds);
}

static iz plt_pipecap(Os *os)
{
    flushw(os);
    return 1<<16;
}

// Look into a pipe without consuming: tee(2) copies what it holds into
// a private pipe, which is then drained. Zero when it holds nothing.
static iz plt_peek(Os *os, Handle h, u8 *buf, iz len)
{
    flushw(os);
    if (fdkind(os, (i32)h) != FK_FIFO) {
        return -1;
    } else if (!len) {
        return 0;
    }
    if (!os->peekfd[1]) {
        i32 fds[2];
        if (cloexecpipe(fds)) {
            return -1;
        }
        os->peekfd[0] = highfd(fds[0]);
        os->peekfd[1] = highfd(fds[1]);
    }
    len = len<(1<<16) ? len : 1<<16;  // what the private pipe holds
    iz n = tee((i32)h, os->peekfd[1], (uz)len, SPLICE_F_NONBLOCK);
    if (n < 0) {
        return errno==EAGAIN ? 0 : -1;
    }
    for (iz got = 0; got < n;) {
        iz r = read(os->peekfd[0], buf+got, (uz)(n-got));
        if (r <= 0) {
            return -1;
        }
        got += r;
    }
    return n;
}

static void stattimes(Stat *r, struct stat *s)
{
    r->mtime = (i64)s->st_mtim.tv_sec*1000000000 + s->st_mtim.tv_nsec;
    r->atime = (i64)s->st_atim.tv_sec*1000000000 + s->st_atim.tv_nsec;
    r->ctime = (i64)s->st_ctim.tv_sec*1000000000 + s->st_ctim.tv_nsec;
}

// close_range(2), where the kernel has it (5.9)
static b32 closerange(i32 lo, i32 hi)
{
    #ifdef SYS_close_range
    return !syscall(SYS_close_range, (unsigned)lo, (unsigned)hi, 0);
    #else
    (void)lo; (void)hi;
    return 0;
    #endif
}

// The core numbers signals as Linux x86-64 does (trap.cpp), so every
// translation between its numbers and the host's (darwin.cpp) is the
// identity.
static_assert(SIGBUS==7 && SIGUSR1==10 && SIGCHLD==17 && SIGSTOP==19 &&
              SIGIO==29 && SIGSYS==31, "the generic Linux numbering");
static i32 hostsig(i32 sig) { return sig; }
static i32 coresig(i32 sig) { return sig; }
static u64 corebit(i32 sig) { return (u64)1 << (sig-1); }
static b32 hostin(u64 set, i32 sig) { return sigin(set, sig); }

// posix_spawn beyond POSIX, by glibc version: chdir actions (2.29),
// closefrom (2.34), and the terminal for a new group (2.35)
#if defined(__GLIBC_PREREQ)
#  if __GLIBC_PREREQ(2, 29)
#    define SPAWN_CHDIR 1
#  endif
#  if __GLIBC_PREREQ(2, 34)
#    define SPAWN_CLOSEFROM 1
#  endif
#endif

// Whether posix_spawn can carry out the plan (trispawn)
static b32 spawnable(SpawnPlan *p)
{
    #if !SPAWN_CHDIR
    (void)p;
    return 0;
    #elif !defined(POSIX_SPAWN_TCSETPGROUP)
    return !(p->pgid && p->fg);
    #else
    (void)p;
    return 1;
    #endif
}

// Before the map: the terminal for a foreground group, and nothing but
// the map open. The shell's own descriptors are all close-on-exec, but
// for the standard three it was given. Returns the flags to add.
static short spawnpre(posix_spawnattr_t *at, posix_spawn_file_actions_t *fa,
                      SpawnPlan *p, i32 *src, i32 base)
{
    short flags = 0;
    #ifdef POSIX_SPAWN_TCSETPGROUP
    if (p->pgid && p->fg) {
        flags |= POSIX_SPAWN_TCSETPGROUP;
        posix_spawnattr_tcsetpgrp_np(at, p->tty);
    }
    #else
    (void)at;
    #endif
    for (i32 fd = 0; fd < base; fd++) {
        b32 target = 0;
        for (i32 i = 0; i < p->nfds; i++) {
            target |= p->dst[i]==fd && src[i]>=0;
        }
        i32 fl = target ? -1 : fcntl(fd, F_GETFD);
        if (fl>=0 && !(fl&FD_CLOEXEC)) {
            posix_spawn_file_actions_addclose(fa, fd);
        }
    }
    return flags;
}

// Before anything else: the directory, by its pin while that is open
static void spawndir(posix_spawn_file_actions_t *fa, SpawnPlan *p)
{
    #if SPAWN_CHDIR
    if (p->cwdfd >= 0) {
        posix_spawn_file_actions_addfchdir_np(fa, p->cwdfd);
    } else {
        posix_spawn_file_actions_addchdir_np(fa, p->cwd);
    }
    #else
    (void)fa; (void)p;
    #endif
}

// After the map: the shell's copies above it closed
static void spawnpost(posix_spawn_file_actions_t *fa, SpawnPlan *, i32 base)
{
    #if SPAWN_CLOSEFROM
    posix_spawn_file_actions_addclosefrom_np(fa, base);
    #else
    (void)fa; (void)base;
    #endif
}

// Whether the kernel sent the signal: the terminal's, to its group
static b32 fromterminal(siginfo_t *info)
{
    return info && info->si_code==SI_KERNEL;
}

// RL_* as the system numbers them, -1 where it has no such limit
static i32 rlres(i32 res)
{
    switch (res) {
    case RL_RTTIME:     return RLIMIT_RTTIME;
    case RL_CORE:       return RLIMIT_CORE;
    case RL_DATA:       return RLIMIT_DATA;
    case RL_NICE:       return RLIMIT_NICE;
    case RL_FSIZE:      return RLIMIT_FSIZE;
    case RL_SIGPENDING: return RLIMIT_SIGPENDING;
    case RL_MEMLOCK:    return RLIMIT_MEMLOCK;
    case RL_RSS:        return RLIMIT_RSS;
    case RL_NOFILE:     return RLIMIT_NOFILE;
    case RL_MSGQUEUE:   return RLIMIT_MSGQUEUE;
    case RL_RTPRIO:     return RLIMIT_RTPRIO;
    case RL_STACK:      return RLIMIT_STACK;
    case RL_CPU:        return RLIMIT_CPU;
    case RL_NPROC:      return RLIMIT_NPROC;
    case RL_AS:         return RLIMIT_AS;
    case RL_LOCKS:      return RLIMIT_LOCKS;
    }
    return -1;
}

// A level signal travels as a queued real-time signal whose value
// holds the signal and the level's serial: a vpid of a process is
// 2^22*(serial+1) + its pid (newvpid), which the receiver rebuilds.
static i32 siglevelnum()
{
    return SIGRTMAX;
}

static void onlevel(int, siginfo_t *si, void *)
{
    i32 saved = errno;
    u32 t = leveltail;
    if (t-levelhead < NLEVELQ) {
        i32 v = si->si_value.sival_int;
        LevelSig *ls = levelq + t%NLEVELQ;
        ls->level = ((i64)(v>>8) + 1)<<22 | getpid();
        ls->sig   = v & 255;
        __atomic_store_n(&leveltail, t+1, __ATOMIC_RELEASE);
    }
    u8 c = 0;
    if (write(sigos->wakefd[1], &c, 1)) {}
    errno = saved;
}

// Whether pid runs this same program, so the real-time signal, deadly
// to anything else, reaches only another of our shells.
static b32 ourprocess(i64 pid)
{
    char path[64] = "/proc/";
    i32  len = 6;
    char digits[24];
    i32  nd = 0;
    for (i64 v = pid; v; v /= 10) digits[nd++] = (char)('0' + v%10);
    while (nd) path[len++] = digits[--nd];
    __builtin_memcpy(path+len, "/exe", 5);
    char a[4096], b[4096];
    ssize_t alen = readlink("/proc/self/exe", a, sizeof(a));
    ssize_t blen = readlink(path, b, sizeof(b));
    return alen>0 && alen==blen && !__builtin_memcmp(a, b, (uz)alen);
}

static i32 plt_siglevel(Os *os, i64 pid, i64 level, i32 sig)
{
    flushw(os);
    i64 serial = (level>>22) - 1;
    if (pid<=0 || pid>(i64)0x7fffffff || serial<0 || serial>=(i64)1<<23 ||
        sig<0 || sig>255 || (level & (((i64)1<<22) - 1))!=pid) {
        return E_SRCH;
    }
    if (!ourprocess(pid)) return E_SRCH;
    union sigval v = {};
    v.sival_int = (i32)(serial<<8 | sig);
    return sigqueue((pid_t)pid, siglevelnum(), v) ? errno : 0;
}

static void initlevel()
{
    struct sigaction sa = {};
    sa.sa_sigaction = onlevel;
    sa.sa_flags     = SA_RESTART | SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(siglevelnum(), &sa, 0);
}

// A named FIFO is a FIFO like a pipe
static u8 fifokind(struct stat *)
{
    return FK_FIFO;
}

// poll sees a named FIFO's end: no read waits on it (plt_wait)
static Io *fifoends(Os *, b32 *any)
{
    *any = 0;
    return 0;
}

// MACHTYPE's os, by C library, and its vendor (machtype)
static Str machos(Str *vendor, b32 *release)
{
    #if __x86_64__ || __i386__
    *vendor = "pc";
    #endif
    *release = 0;
    #if __ANDROID__
    return "linux-android";
    #elif __GLIBC__
    return "linux-gnu";
    #else
    return "linux-musl";
    #endif
}

// The open descriptors from 3 to below max (inherited), found with one
// poll of them all (POLLNVAL marks a closed one)
static Slice<InheritFd> openfds(Arena *a, i32 max)
{
    Slice<InheritFd> r = {};
    struct pollfd *fds = alloc<struct pollfd>(a, max-3);
    for (i32 fd = 3; fd < max; fd++) {
        fds[fd-3].fd = fd;
    }
    i32 ready;
    do {
        ready = poll(fds, (nfds_t)(max-3), 0);
    } while (ready<0 && errno==EINTR);
    if (ready < 0) return r;
    for (i32 i = 0; i < max-3; i++) {
        if (!(fds[i].revents & POLLNVAL)) r = pushinherit(a, r, fds[i].fd);
    }
    return r;
}
