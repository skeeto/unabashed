// POSIX platform layer (Linux, macOS)
// $ g++ -std=c++20 -O2 -o unabashed main_posix.cpp
// This is free and unencumbered software released into the public domain.
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "unabashed.cpp"
#include "fibers.cpp"

extern char **environ;

struct Os {
    Fibers fibers;
    Io    *pending;     // submitted operations, oldest first
    i32    wakefd[2];   // self-pipe written by the SIGCHLD handler
    b32    pipeign;     // SIGPIPE was ignored on entry, so stays ignored
    u64    entryign;    // signals ignored on entry: they stay ignored
    u64    changed;     // signals whose disposition the shell changed
};

static u64 volatile sigbits;  // caught signals not yet taken

static Os *sigos;  // for the signal handler

static Fibers *osfibers(Os *os)
{
    return &os->fibers;
}

static b32 writeall(i32 fd, u8 *buf, iz len)
{
    for (iz off = 0; off < len;) {
        iz n = write(fd, buf+off, (uz)(len-off));
        if (n < 0) {
            if (errno == EINTR) continue;
            return 0;
        }
        off += n;
    }
    return 1;
}

[[noreturn]] static void os_oom()
{
    Str msg = "bash: out of memory\n";
    writeall(2, msg.data, msg.len);
    _exit(2);
}

static byte *os_reserve(Os *, iz len)
{
    void *p = mmap(0, (uz)len, PROT_NONE, MAP_PRIVATE|MAP_ANON|MAP_NORESERVE, -1, 0);
    return p==MAP_FAILED ? 0 : (byte *)p;
}

static b32 os_commit(Os *, byte *p, iz len)
{
    return !mprotect(p, (uz)len, PROT_READ|PROT_WRITE);
}

static b32 os_write(Os *, Handle h, u8 *buf, iz len)
{
    return writeall((i32)h, buf, len);
}

static iz os_read(Os *, Handle h, u8 *buf, iz len)
{
    iz max = (iz)1<<30;  // macOS rejects reads over INT_MAX
    for (;;) {
        iz n = read((i32)h, buf, (uz)(len<max ? len : max));
        if (n<0 && errno==EINTR) continue;
        return n;
    }
}

// A null-terminated copy for the system call.
static char *cpath(Arena *a, Str s)
{
    char *r = (char *)allocbytes(a, s.len+1);
    copybytes(r, s.data, s.len);
    r[s.len] = 0;
    return r;
}

// Keep shell-held descriptors away from the low numbers that scripts
// redirect, and out of children.
static i32 highfd(i32 fd)
{
    if (fd < 0) return fd;
    i32 r = fcntl(fd, F_DUPFD_CLOEXEC, 10);
    close(fd);
    return r;
}

static Opened os_open(Os *, Arena scratch, Str path, i32 flags, i32 mode)
{
    Opened r = {};
    i32 oflags = O_CLOEXEC;
    if ((flags&OF_READ) && (flags&OF_WRITE)) oflags |= O_RDWR;
    else if (flags & OF_WRITE) oflags |= O_WRONLY;
    else oflags |= O_RDONLY;
    if (flags & OF_CREATE) oflags |= O_CREAT;
    if (flags & OF_TRUNC)  oflags |= O_TRUNC;
    if (flags & OF_APPEND) oflags |= O_APPEND;
    if (flags & OF_EXCL)   oflags |= O_EXCL;
    i32 fd;
    do {
        fd = open(cpath(&scratch, path), oflags, mode);
    } while (fd<0 && errno==EINTR);
    if (fd < 0) {
        r.h   = BADHANDLE;
        r.err = errno;
        return r;
    }
    r.h = (Handle)highfd(fd);
    return r;
}

static Opened os_tempfile(Os *os, Arena scratch)
{
    // As bash: created in TMPDIR (or /tmp), then unlinked, readable
    // only by the user
    Str dir = {};
    char *t = getenv("TMPDIR");
    if (t && *t) {
        dir.data = (u8 *)t;
        for (; t[dir.len]; dir.len++) {}
    }
    dir = dir.len ? dir : Str("/tmp");
    static u32 counter;
    Opened r = {};
    for (i32 try_ = 0; try_ < 100; try_++) {
        Buf b(&scratch, dir.len+64);
        print(&b, dir);
        print(&b, "/sh-thd.");
        print(&b, (i64)getpid());
        print(&b, ".");
        print(&b, (i64)(++counter ^ (u32)os_now(os)));
        char *path = cpath(&scratch, finish(&b));
        i32 fd = open(path, O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC, 0600);
        if (fd < 0 && errno==EEXIST) continue;
        if (fd < 0) {
            r.h   = BADHANDLE;
            r.err = errno;
            return r;
        }
        unlink(path);
        fchmod(fd, 0400);
        r.h = (Handle)highfd(fd);
        return r;
    }
    r.h   = BADHANDLE;
    r.err = EEXIST;
    return r;
}

static void os_close(Os *, Handle h)
{
    if (h != BADHANDLE) close((i32)h);
}

static Handle os_dup(Os *, Handle h)
{
    i32 fd = fcntl((i32)h, F_DUPFD_CLOEXEC, 10);
    return fd<0 ? BADHANDLE : (Handle)fd;
}

static iz os_pipecap(Os *)
{
    return 1<<14;  // macOS starts small; Linux has 64 KiB
}

static OsPipe os_pipe(Os *)
{
    OsPipe r = {};
    i32 fds[2];
    if (pipe(fds)) {
        r.err = errno;
        r.r = r.w = BADHANDLE;
        return r;
    }
    r.r = (Handle)highfd(fds[0]);
    r.w = (Handle)highfd(fds[1]);
    return r;
}

static iz os_peek(Os *, Handle, u8 *, iz)
{
    return -1;  // pipes cannot be peeked
}

static i64 os_seek(Os *, Handle h, i64 off, i32 whence)
{
    return lseek((i32)h, off, whence==0 ? SEEK_SET : whence==1 ? SEEK_CUR : SEEK_END);
}

static Stat tostat(struct stat *s)
{
    Stat r = {};
    switch (s->st_mode & S_IFMT) {
    case S_IFREG:  r.type = FT_FILE;  break;
    case S_IFDIR:  r.type = FT_DIR;   break;
    case S_IFLNK:  r.type = FT_LINK;  break;
    case S_IFIFO:  r.type = FT_FIFO;  break;
    case S_IFCHR:  r.type = FT_CHAR;  break;
    case S_IFBLK:  r.type = FT_BLOCK; break;
    case S_IFSOCK: r.type = FT_SOCK;  break;
    }
    r.mode  = (i32)(s->st_mode & 07777);
    r.nlink = (i32)s->st_nlink;
    r.size  = (i64)s->st_size;
    #if __APPLE__
    r.mtime = (i64)s->st_mtimespec.tv_sec*1000000000 + s->st_mtimespec.tv_nsec;
    r.atime = (i64)s->st_atimespec.tv_sec*1000000000 + s->st_atimespec.tv_nsec;
    #else
    r.mtime = (i64)s->st_mtim.tv_sec*1000000000 + s->st_mtim.tv_nsec;
    r.atime = (i64)s->st_atim.tv_sec*1000000000 + s->st_atim.tv_nsec;
    #endif
    r.dev   = (i64)s->st_dev;
    r.ino   = (i64)s->st_ino;
    r.uid   = (i64)s->st_uid;
    r.gid   = (i64)s->st_gid;
    return r;
}

static Stat os_stat(Os *, Arena scratch, Str path, b32 follow)
{
    struct stat s;
    char *p = cpath(&scratch, path);
    if (follow ? stat(p, &s) : lstat(p, &s)) {
        Stat r = {};
        r.err = errno;
        return r;
    }
    return tostat(&s);
}

static Stat os_fstat(Os *, Handle h)
{
    struct stat s;
    if (fstat((i32)h, &s)) {
        Stat r = {};
        r.err = errno;
        return r;
    }
    return tostat(&s);
}

static Dirent *os_list(Os *, Arena *a, Str dir, i32 *err)
{
    Arena tmp = *a;
    DIR *d = opendir(cpath(&tmp, dir));
    *err = d ? 0 : errno;
    if (!d) return 0;
    Dirent  *head = 0;
    Dirent **tail = &head;
    for (struct dirent *e; (e = readdir(d));) {
        Dirent *n = alloc<Dirent>(a);
        Str name = {(u8 *)e->d_name, 0};
        for (; e->d_name[name.len]; name.len++) {}
        n->name = clone(a, name);
        switch (e->d_type) {
        case DT_REG: n->type = FT_FILE; break;
        case DT_DIR: n->type = FT_DIR;  break;
        case DT_LNK: n->type = FT_LINK; break;
        default:     n->type = FT_NONE; break;
        }
        *tail = n;
        tail  = &n->next;
    }
    closedir(d);
    return head;
}

// Close descriptors lo to hi, or lo up with hi -1, as close_range(2)
// where the kernel has it.
static void closefds(i32 lo, i32 hi)
{
    #ifdef SYS_close_range
    if (!syscall(SYS_close_range, (unsigned)lo, (unsigned)hi, 0)) return;
    #endif
    for (i32 fd = lo; fd <= (hi<0 ? 1023 : hi); fd++) close(fd);
}

// Started with vfork: the child borrows the parent's memory until it
// execs, so it only makes system calls on what was prepared before, and
// reports a failed exec through err. Signals stay blocked until the
// child has reset its dispositions, so no handler of the shell's runs
// in it.
static Spawned os_spawn(Os *, Arena scratch, Spawn *s)
{
    Spawned r = {};
    r.proc = BADHANDLE;
    char *path = cpath(&scratch, s->path);
    char *cwd  = cpath(&scratch, s->cwd);
    char **argv = alloc<char *>(&scratch, s->argc+1);
    for (iz i = 0; i < s->argc; i++) argv[i] = cpath(&scratch, s->argv[i]);
    char **envp = alloc<char *>(&scratch, s->nenv+1);
    for (iz i = 0; i < s->nenv; i++) envp[i] = cpath(&scratch, s->env[i]);
    i32 *src = alloc<i32>(&scratch, s->nfds ? s->nfds : 1);
    i32 *dst = alloc<i32>(&scratch, s->nfds ? s->nfds : 1);
    for (iz i = 0; i < s->nfds; i++) {
        src[i] = (i32)s->fds[i].h;
        dst[i] = s->fds[i].fd;
    }
    i32 nfds = (i32)s->nfds;
    b32 pipedfl = !sigos || !sigos->pipeign;
    u64 changed = sigos ? sigos->changed : 0;
    u64 ign     = s->sigign | (sigos ? sigos->entryign : 0);

    sigset_t all, old;
    sigfillset(&all);
    sigprocmask(SIG_SETMASK, &all, &old);
    i32 volatile err = 0;
    pid_t pid = vfork();
    if (!pid) {
        // Move sources out of the way first, then install the map
        for (i32 i = 0; i < nfds; i++) {
            src[i] = fcntl(src[i], F_DUPFD, 256);
        }
        closefds(0, 255);
        for (i32 i = 0; i < nfds; i++) dup2(src[i], dst[i]);
        closefds(256, -1);
        if (chdir(cwd)) {
            // run anyway: bash reports nothing here
        }
        // The shell ignores SIGPIPE for its in-process subshells, and
        // catches or ignores what its traps say: the child starts with
        // defaults, except what it was told to ignore (trap '' in the
        // spawning subshell) and what was ignored on entry
        if (pipedfl) signal(SIGPIPE, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        for (i32 sig = 1; sig < NSIGNAL && sig < NSIG; sig++) {
            u64 bit = (u64)1 << (sig-1);
            if (ign & bit) {
                signal(sig, SIG_IGN);
            } else if (changed & bit) {
                signal(sig, SIG_DFL);
            }
        }
        sigprocmask(SIG_SETMASK, &old, 0);
        execve(path, argv, envp);
        err = errno ? errno : E_NOEXEC;
        _exit(127);
    }
    i32 e = pid<0 ? errno : err;
    sigprocmask(SIG_SETMASK, &old, 0);
    if (pid>=0 && e) {
        waitpid(pid, 0, 0);
    }
    if (e) {
        r.err = e;
        return r;
    }
    r.proc = (Handle)pid;
    r.pid  = pid;
    return r;
}

// As os_spawn's child, but in this process. Everything is undone if
// execve fails (a script without #!, which the shell then runs itself):
// the targets were saved first, the other descriptors are only marked
// close-on-exec, the directory and signal actions are put back.
static i32 os_exec(Os *, Arena scratch, Spawn *s)
{
    char *path = cpath(&scratch, s->path);
    char *cwd  = cpath(&scratch, s->cwd);
    char **argv = alloc<char *>(&scratch, s->argc+1);
    for (iz i = 0; i < s->argc; i++) argv[i] = cpath(&scratch, s->argv[i]);
    char **envp = alloc<char *>(&scratch, s->nenv+1);
    for (iz i = 0; i < s->nenv; i++) envp[i] = cpath(&scratch, s->env[i]);
    iz   n    = s->nfds;
    i32 *src  = alloc<i32>(&scratch, n ? n : 1);
    i32 *save = alloc<i32>(&scratch, n ? n : 1);
    i32  err  = 0;
    for (iz i = 0; i < n; i++) {
        src[i] = save[i] = -1;
    }
    for (iz i = 0; i<n && !err; i++) {
        src[i] = fcntl((i32)s->fds[i].h, F_DUPFD_CLOEXEC, 256);
        err = src[i]<0 ? errno : 0;
    }
    for (iz i = 0; i<n && !err; i++) {
        save[i] = fcntl(s->fds[i].fd, F_DUPFD_CLOEXEC, 256);  // -1 if closed
    }
    i32 dot = open(".", O_RDONLY|O_CLOEXEC);
    if (err || dot<0) {
        err = err ? err : errno;
        for (iz i = 0; i < n; i++) {
            if (src[i] >= 0) close(src[i]);
            if (save[i] >= 0) close(save[i]);
        }
        if (dot >= 0) close(dot);
        return err;
    }

    for (i32 fd = 0; fd < 1024; fd++) {
        b32 target = 0;
        for (iz i = 0; i < n; i++) target |= s->fds[i].fd == fd;
        if (!target) fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
    for (iz i = 0; i < n; i++) dup2(src[i], s->fds[i].fd);
    if (chdir(cwd)) {
        // run anyway, as os_spawn
    }
    // Signal actions as os_spawn's child gets them
    struct sigaction old[NSIGNAL] = {};
    u64 touched = 0;
    u64 changed = sigos ? sigos->changed : 0;
    u64 ign     = s->sigign | (sigos ? sigos->entryign : 0);
    if (!sigos || !sigos->pipeign) changed |= (u64)1 << (SIGPIPE-1);
    changed |= (u64)1 << (SIGCHLD-1);
    for (i32 sig = 1; sig < NSIGNAL && sig < NSIG; sig++) {
        u64 bit = (u64)1 << (sig-1);
        if (sig==SIGKILL || sig==SIGSTOP || !((ign|changed) & bit)) continue;
        struct sigaction sa = {};
        sa.sa_handler = ign&bit ? SIG_IGN : SIG_DFL;
        if (!sigaction(sig, &sa, old+sig)) touched |= bit;
    }

    execve(path, argv, envp);

    err = errno;
    for (i32 sig = 1; sig < NSIGNAL && sig < NSIG; sig++) {
        if (touched>>(sig-1) & 1) sigaction(sig, old+sig, 0);
    }
    if (fchdir(dot)) {
        // nothing better to do
    }
    close(dot);
    for (iz i = 0; i < n; i++) {
        if (save[i] >= 0) {
            dup2(save[i], s->fds[i].fd);
            close(save[i]);
        } else {
            close(s->fds[i].fd);
        }
        close(src[i]);
    }
    return err;
}

static i32 waitstatus(i32 status)
{
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return WSIGNALED | (128 + WTERMSIG(status));
    return 127;
}

static i32 os_waitproc(Os *, Handle proc)
{
    i32 status;
    while (waitpid((pid_t)proc, &status, 0) < 0) {
        if (errno != EINTR) return 127;
    }
    return waitstatus(status);
}


// Asynchronous operations: poll for readiness, then a single read or
// write that cannot block (at most PIPE_BUF bytes, which a pipe takes
// atomically once poll reports room). Process exits arrive through a
// self-pipe written by the SIGCHLD handler. No descriptor that another
// process may hold is made non-blocking.

static void onsigchld(int)
{
    i32 saved = errno;
    u8  c = 0;
    if (write(sigos->wakefd[1], &c, 1)) {}
    errno = saved;
}

// A trapped signal: note it and wake os_wait.
static void onsignal(int sig)
{
    i32 saved = errno;
    __atomic_fetch_or(&sigbits, (u64)1<<(sig-1), __ATOMIC_RELAXED);
    u8 c = 0;
    if (write(sigos->wakefd[1], &c, 1)) {}
    errno = saved;
}

static i32 os_kill(Os *, i64 pid, i32 sig)
{
    if (pid<=0 || pid>(i64)0x7fffffff) return E_SRCH;
    return kill((pid_t)pid, sig) ? errno : 0;
}

static u64 os_sigpending(Os *)
{
    return __atomic_exchange_n(&sigbits, (u64)0, __ATOMIC_RELAXED);
}

static u64 os_siggroup(Os *)
{
    return 0;  // terminal signals arrive as the process's own
}

static void os_tasktag(Os *, u64, iz)
{
}

static void os_racy(Os *)
{
}

static void os_sigmode(Os *os, i32 sig, i32 mode)
{
    u64 bit = (u64)1 << (sig-1);
    if (sig<1 || sig>=NSIG || sig==SIGKILL || sig==SIGSTOP || (os->entryign & bit)) {
        return;
    }
    if (sig == SIGCHLD) return;  // os_wait's own handler
    struct sigaction sa = {};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    switch (mode) {
    case SIGM_DEFAULT:
        // the shell keeps ignoring SIGPIPE for its in-process subshells
        sa.sa_handler = sig==SIGPIPE ? SIG_IGN : SIG_DFL;
        break;
    case SIGM_IGNORE:
        sa.sa_handler = SIG_IGN;
        break;
    case SIGM_CATCH:
        sa.sa_handler = onsignal;
        break;
    }
    sigaction(sig, &sa, 0);
    os->changed |= bit;
}

static void initasync(Os *os)
{
    sigos = os;
    struct sigaction sa = {};
    sigaction(SIGPIPE, 0, &sa);
    os->pipeign = sa.sa_handler == SIG_IGN;
    for (i32 sig = 1; sig < NSIGNAL && sig < NSIG; sig++) {
        if (!sigaction(sig, 0, &sa) && sa.sa_handler==SIG_IGN) {
            os->entryign |= (u64)1 << (sig-1);
        }
    }
    signal(SIGPIPE, SIG_IGN);
    if (pipe(os->wakefd)) {
        os->wakefd[0] = os->wakefd[1] = -1;
        return;
    }
    for (i32 i = 0; i < 2; i++) {
        os->wakefd[i] = highfd(os->wakefd[i]);
        fcntl(os->wakefd[i], F_SETFL, O_NONBLOCK);
    }
    sa = {};
    sa.sa_handler = onsigchld;
    sa.sa_flags   = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, 0);
}

// Complete an operation if it can be done now. For reads and writes,
// revents is poll's verdict on the descriptor.
static b32 tryio(Io *io, i32 revents)
{
    switch (io->op) {
    case IO_READ: {
        if (!revents) return 0;
        iz n;
        do {
            n = read((i32)io->h, io->buf, (uz)(io->len<(1<<30) ? io->len : (1<<30)));
        } while (n<0 && errno==EINTR);
        io->n   = n<0 ? 0 : n;
        io->err = n<0 ? errno : 0;
    } return 1;
    case IO_WRITE: {
        if (!revents) return 0;
        iz len = io->len<PIPE_BUF ? io->len : PIPE_BUF;
        iz n;
        do {
            n = write((i32)io->h, io->buf, (uz)len);
        } while (n<0 && errno==EINTR);
        io->n   = n<0 ? 0 : n;
        io->err = n<0 ? errno : 0;
    } return 1;
    case IO_PROC: {
        i32   status = 0;
        pid_t r;
        do {
            r = waitpid((pid_t)io->h, &status, WNOHANG);
        } while (r<0 && errno==EINTR);
        if (!r) return 0;
        io->n   = r<0 ? 127 : waitstatus(status);
        io->err = 0;
    } return 1;
    }
    io->err = E_INVAL;
    return 1;
}

static i32 pollnow(Io *io, i32 timeout)
{
    struct pollfd p = {};
    p.fd     = (i32)io->h;
    p.events = io->op==IO_READ ? POLLIN : POLLOUT;
    i32 r;
    do {
        r = poll(&p, 1, timeout);
    } while (r<0 && errno==EINTR);
    return r>0 ? p.revents : 0;
}

static void os_submit(Os *os, Io *io)
{
    io->done = 0;
    io->next = 0;
    if (tryio(io, io->op==IO_PROC ? 1 : pollnow(io, 0))) {
        io->done = 1;
        return;
    }
    Io **tail = &os->pending;
    while (*tail) tail = &(*tail)->next;
    *tail = io;
}

static Io *os_wait(Os *os, i64 deadline)
{
    enum { MAXPOLL = 256 };
    struct pollfd fds[MAXPOLL];
    for (;;) {
        // Process exits first: the self-pipe only says "look again"
        for (Io **p = &os->pending; *p; p = &(*p)->next) {
            Io *io = *p;
            if (io->op==IO_PROC && tryio(io, 1)) {
                *p = io->next;
                io->done = 1;
                return io;
            }
        }
        i32 n = 0;
        fds[n].fd      = os->wakefd[0];
        fds[n].events  = POLLIN;
        fds[n++].revents = 0;
        for (Io *io = os->pending; io && n<MAXPOLL; io = io->next) {
            if (io->op == IO_PROC) continue;
            fds[n].fd      = (i32)io->h;
            fds[n].events  = io->op==IO_READ ? POLLIN : POLLOUT;
            fds[n++].revents = 0;
        }
        i32 timeout = -1;
        if (deadline == WAIT_POLL) {
            timeout = 0;
        } else if (deadline > 0) {
            i64 left = (deadline - os_now(os) + 999999) / 1000000;
            timeout = left<0 ? 0 : left>(1<<30) ? (1<<30) : (i32)left;
        }
        i32 r = poll(fds, (nfds_t)n, timeout);
        if (r<0 && errno!=EINTR) return 0;
        if (fds[0].revents) {
            u8 buf[64];
            while (read(os->wakefd[0], buf, sizeof(buf)) > 0) {}
        }
        if (sigbits) return 0;  // the core takes the signal first
        i32 k = 1;
        for (Io **p = &os->pending; *p; p = &(*p)->next) {
            Io *io = *p;
            if (io->op == IO_PROC) continue;
            if (k >= n) break;
            if (tryio(io, fds[k++].revents)) {
                *p = io->next;
                io->done = 1;
                return io;
            }
        }
        for (Io **p = &os->pending; *p; p = &(*p)->next) {
            Io *io = *p;
            if (io->op==IO_PROC && tryio(io, 1)) {
                *p = io->next;
                io->done = 1;
                return io;
            }
        }
        if (r==0 && timeout>=0) return 0;
    }
}

static b32 os_cancel(Os *os, Io *io)
{
    for (Io **p = &os->pending; *p; p = &(*p)->next) {
        if (*p == io) {
            *p = io->next;
            return 1;
        }
    }
    return 0;
}

static i64 os_now(Os *)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (i64)ts.tv_sec*1000000000 + ts.tv_nsec;
}

static byte *fiberstack(Os *, iz size)
{
    void *p = mmap(0, (uz)size, PROT_READ|PROT_WRITE,
                   MAP_PRIVATE|MAP_ANON|MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        os_oom();
    }
    mprotect(p, 1<<12, PROT_NONE);  // guard page
    return (byte *)p;
}

static void fiberunstack(Os *, byte *p, iz size)
{
    munmap(p, (uz)size);
}

static Str fromcstr(char *s)
{
    Str r = {(u8 *)s, 0};
    for (; r.data[r.len]; r.len++) {}
    return r;
}

int main(int argc, char **argv)
{
    static Os os;
    OsInfo info = {};
    info.reserve = (iz)1 << (sizeof(void *)==8 ? 34 : 28);
    initasync(&os);

    Arena a = newarena(&os, (iz)1<<26);
    info.nargs = argc;
    info.args  = alloc<Str>(&a, argc);
    for (i32 i = 0; i < argc; i++) {
        info.args[i] = fromcstr(argv[i]);
    }
    for (; environ[info.nenv]; info.nenv++) {}
    info.env = alloc<Str>(&a, info.nenv);
    for (iz i = 0; i < info.nenv; i++) {
        info.env[i] = fromcstr(environ[i]);
    }
    char *cwd = getcwd((char *)allocbytes(&a, 4096), 4096);
    info.cwd  = cwd ? fromcstr(cwd) : Str("/");
    info.pid  = getpid();
    info.ppid = getppid();
    info.uid  = getuid();
    info.euid = geteuid();
    info.gid  = getgid();
    info.egid = getegid();
    char *host = (char *)allocbytes(&a, 256);
    if (!gethostname(host, 255)) {
        host[255] = 0;
        info.host = fromcstr(host);
    }
    struct rlimit rl;
    if (!getrlimit(RLIMIT_STACK, &rl) && rl.rlim_cur!=RLIM_INFINITY) {
        info.stack = (iz)rl.rlim_cur;
    } else {
        info.stack = (iz)1<<23;
    }
    for (i32 i = 0; i < 3; i++) {
        info.std[i] = (Handle)i;
    }
    info.sigign = os.entryign;
    i32 status = sh_main(&os, &info);
    if (status & WSIGNALED) {
        // Killed by a signal: die by it, so that the parent sees that
        i32 sig = (status & 255) - 128;
        signal(sig, SIG_DFL);
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, sig);
        sigprocmask(SIG_UNBLOCK, &set, 0);
        kill(getpid(), sig);
    }
    return status & 255;
}
