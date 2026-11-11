// macOS's part of the library module: what it does that Linux does
// otherwise (liblinux.cpp), each declared in libposix.cpp, after which
// a root includes this module.
// This is free and unencumbered software released into the public domain.

static void watchkid(Lib *l, LibKid *k)
{
    struct kevent e;
    EV_SET(&e, (uintptr_t)k->pid, EVFILT_PROC, EV_ADD|EV_ONESHOT, NOTE_EXIT, 0, 0);
    k->watch = l->waitfd>=0 && !kevent(l->waitfd, &e, 1, 0, 0, 0);  // ESRCH: gone
}

static b32 watching(LibKid *k)
{
    return k->watch;
}

static void unwatchkid(Lib *, LibKid *k)
{
    k->watch = 0;  // its event was one-shot
}

// Take what the watch descriptor says. A program that ended with no
// wait yet pending is watched no more, lest it wake every wait: its
// wait, once submitted, finds it ended at once.
static void drainwatch(Lib *l)
{
    // XNU posts NOTE_EXIT before the program can be reaped, and the
    // event is one-shot: its program is polled, backing off, until then
    struct kevent e[64];
    struct timespec zero = {};
    for (i32 n = countof(e); n == countof(e);) {
        n = kevent(l->waitfd, 0, 0, e, countof(e), &zero);
        for (i32 i = 0; i < n; i++) {
            LibKid *k = findkid(l, (i64)e[i].ident);
            if (k) unwatchkid(l, k);
        }
    }
}

// A thread of the host's, whose id kill takes for the host: macOS's
// kill takes no thread ids
static b32 hostthread(i64)
{
    return 0;
}

// Every program watched no more (the kqueue stays, for the next run)
static void unwatchall(Lib *l)
{
    for (i32 i = 0; i < l->nkids; i++) {
        l->kids[i].watch = 0;
    }
}

// The signals a run raises on itself, SIGPIPE and SIGXFSZ: macOS sends
// a write's to the process, not the thread, so the host must ignore
// them (false otherwise)
static b32 blockraised(Lib *)
{
    i32 raised[] = {SIGPIPE, SIGXFSZ};
    for (i32 i = 0; i < countof(raised); i++) {
        struct sigaction sa = {};
        sigaction(raised[i], 0, &sa);
        if (sa.sa_handler != SIG_IGN) return 0;
    }
    return 1;
}

static void unraise() {}

// Before a run: the kqueue's events since the last
static void rewatch(Lib *l)
{
    drainwatch(l);
}

// The handle's watch descriptor, a kqueue for its life: not inherited
// across fork, and close-on-exec
static i32 newwatch(Lib *l)
{
    l->cfg.spawnlock(l->cfg.user, 1);
    i32 kq = kqueue();
    if (kq >= 0) fcntl(kq, F_SETFD, FD_CLOEXEC);
    l->cfg.spawnlock(l->cfg.user, 0);
    return kq;
}
