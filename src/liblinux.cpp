// Linux's part of the library module: what it does that macOS does
// otherwise (libdarwin.cpp), each declared in libposix.cpp, after which
// a root includes this module.
// This is free and unencumbered software released into the public domain.

static void watchkid(Lib *l, LibKid *k)
{
    k->watch = -1;
    #ifdef SYS_pidfd_open
    if (l->waitfd < 0) {
        l->waitfd = fdtake(l, epoll_create1(EPOLL_CLOEXEC));
        if (l->waitfd < 0) return;
    }
    i32 fd = fdtake(l, (i32)syscall(SYS_pidfd_open, (pid_t)k->pid, 0));
    if (fd < 0) return;
    struct epoll_event e = {};
    e.events   = EPOLLIN;
    e.data.u64 = (u64)k->pid;
    if (epoll_ctl(l->waitfd, EPOLL_CTL_ADD, fd, &e)) {
        fdgive(l, fd);
        close(fd);
        return;
    }
    k->watch = fd;
    #endif
}

static b32 watching(LibKid *k)
{
    return k->watch >= 0;
}

static void unwatchkid(Lib *l, LibKid *k)
{
    if (watching(k)) {
        fdgive(l, k->watch);
        close(k->watch);  // and so out of the epoll set
    }
    k->watch = -1;
}

// Take what the watch descriptor says. A program that ended with no
// wait yet pending is watched no more, lest it wake every wait: its
// wait, once submitted, finds it ended at once.
static void drainwatch(Lib *l)
{
    struct epoll_event e[64];
    i32 n = epoll_wait(l->waitfd, e, countof(e), 0);
    for (i32 i = 0; i < n; i++) {
        LibKid *k = findkid(l, (i64)e[i].data.u64);
        if (k && !procpending(&l->os, k->pid)) unwatchkid(l, k);
    }
}

// A thread of the host's, whose id kill takes for the host.
static b32 hostthread(i64 tid)
{
    char path[48] = "/proc/self/task/";
    i32  len = 16;
    char digits[24];
    i32  nd = 0;
    for (i64 v = tid; v > 0; v /= 10) digits[nd++] = (char)('0' + v%10);
    while (nd) path[len++] = digits[--nd];
    path[len] = 0;
    return tid>0 && !access(path, F_OK);
}

// Every program watched no more, and the epoll gone: their descriptors
// closed with the rest
static void unwatchall(Lib *l)
{
    for (i32 i = 0; i < l->nkids; i++) {
        l->kids[i].watch = -1;
    }
    l->waitfd = -1;
}

// The signals a run raises on itself, SIGPIPE and SIGXFSZ, blocked in
// the run's thread, to which Linux sends them; true
static b32 blockraised(Lib *l)
{
    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGPIPE);
    sigaddset(&block, SIGXFSZ);
    pthread_sigmask(SIG_BLOCK, &block, &l->mask);
    l->masked = 1;
    return 1;
}

// A SIGPIPE or SIGXFSZ the run raised on itself goes unseen
static void unraise()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigaddset(&set, SIGXFSZ);
    struct timespec zero = {};
    while (sigtimedwait(&set, 0, &zero) > 0) {}
}

// Before a run: nothing to take, the epoll gone with the last run
static void rewatch(Lib *) {}

// The handle's watch descriptor: none until a run's first program
// (watchkid)
static i32 newwatch(Lib *)
{
    return -1;
}
