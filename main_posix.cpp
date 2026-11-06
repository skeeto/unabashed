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
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "unabashed.cpp"
#include "fibers.cpp"

extern char **environ;

struct Os {
    Fibers fibers;
    Io    *pending;     // submitted operations, oldest first
    i32    wakefd[2];   // self-pipe written by the SIGCHLD handler
    i32    peekfd[2];   // private pipe for os_peek, once needed
    b32    pipeign;     // SIGPIPE was ignored on entry, so stays ignored
    u64    entryign;    // signals ignored on entry: they stay ignored
    u64    changed;     // signals whose disposition the shell changed
    iz     reserve;     // OsInfo.reserve, for a forked image child
    iz     stack;       // OsInfo.stack, likewise
    i32    ttyraw;      // os_tty: the mode set (TTY_*), 0 if never changed
    termios ttysave;    // and its mode before
};

static u64 volatile sigbits;  // caught signals not yet taken

// Level signals from another of our shells (os_siglevel), queued by
// the handler of a real-time signal: one producer (the handler, which
// cannot nest), one consumer (os_levelsig).
enum : u32 { NLEVELQ = 32 };
static LevelSig levelq[NLEVELQ];
static u32 volatile levelhead, leveltail;

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

static i32 os_unlink(Os *, Arena scratch, Str path)
{
    return unlink(cpath(&scratch, path)) ? errno : 0;
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

static NamedPipe os_namedpipe(Os *, Arena *, Str, i32)
{
    NamedPipe r = {};
    r.h   = BADHANDLE;
    r.err = E_NOSYS;
    return r;
}

static iz os_pipecap(Os *)
{
#ifdef __linux__
    return 1<<16;
#else
    return 1<<14;  // macOS starts small
#endif
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

#ifdef __linux__
// Look into a pipe without consuming: tee(2) copies what it holds into
// a private pipe, which is then drained. Zero when it holds nothing.
static iz os_peek(Os *os, Handle h, u8 *buf, iz len)
{
    struct stat st;
    if (fstat((i32)h, &st) || !S_ISFIFO(st.st_mode)) {
        return -1;
    } else if (!len) {
        return 0;
    }
    if (!os->peekfd[1]) {
        i32 fds[2];
        if (pipe(fds)) {
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
#else
static iz os_peek(Os *, Handle, u8 *, iz)
{
    return -1;  // pipes cannot be peeked
}
#endif

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
    r.ctime = (i64)s->st_ctimespec.tv_sec*1000000000 + s->st_ctimespec.tv_nsec;
    #else
    r.mtime = (i64)s->st_mtim.tv_sec*1000000000 + s->st_mtim.tv_nsec;
    r.atime = (i64)s->st_atim.tv_sec*1000000000 + s->st_atim.tv_nsec;
    r.ctime = (i64)s->st_ctim.tv_sec*1000000000 + s->st_ctim.tv_nsec;
    #endif
    r.blocks = (i64)s->st_blocks;
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

static void onsignal(int);

static b32 os_con(Os *, Handle, ConOp *)
{
    return 0;  // no legacy consoles here
}

static Tty os_tty(Os *os, Handle in, Handle out, i32 how)
{
    Tty     r = {};
    termios t;
    if (!isatty((i32)in) || !isatty((i32)out) || tcgetattr((i32)in, &t)) {
        r.err = E_NOTTY;
        return r;
    }
    if (how==TTY_RAW && os->ttyraw==TTY_RAW) {
        t = os->ttysave;  // already
    } else if (how!=TTY_QUERY && how!=TTY_RESTORE) {
        if (!os->ttyraw) os->ttysave = t;
        os->ttyraw = how;
        termios m = os->ttysave;
        switch (how) {
        case TTY_RAW:
            m.c_lflag &= ~(tcflag_t)(ICANON|ECHO|ISIG|IEXTEN);
            m.c_iflag &= ~(tcflag_t)(ICRNL|INLCR|IGNCR|IXON);
            break;
        case TTY_NOECHO:
            m.c_lflag &= ~(tcflag_t)(ECHO|ECHOE|ECHOK|ECHONL);
            break;
        case TTY_CHARS:
            m.c_lflag &= ~(tcflag_t)ICANON;
            break;
        case TTY_CHARSNOECHO:
            m.c_lflag &= ~(tcflag_t)(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
            break;
        }
        m.c_cc[VMIN]  = 1;
        m.c_cc[VTIME] = 0;
        tcsetattr((i32)in, TCSADRAIN, &m);
        t = os->ttysave;
        // A resize is a signal (SIGWINCH), unless something else
        // already handles it (a trap)
        struct sigaction sa = {};
        if (how==TTY_RAW && !sigaction(SIGWINCH, 0, &sa) && sa.sa_handler==SIG_DFL) {
            sigemptyset(&sa.sa_mask);
            sa.sa_flags   = SA_RESTART;
            sa.sa_handler = onsignal;
            sigaction(SIGWINCH, &sa, 0);
        }
    } else if (how==TTY_RESTORE && os->ttyraw) {
        os->ttyraw = 0;
        tcsetattr((i32)in, TCSADRAIN, &os->ttysave);
        t = os->ttysave;
    } else if (os->ttyraw) {
        t = os->ttysave;
    }
    winsize ws = {};
    if (!ioctl((i32)out, TIOCGWINSZ, &ws)) {
        r.rows = ws.ws_row;
        r.cols = ws.ws_col;
    }
    r.xenl = 1;  // as xterm and its kin
    u8 off = _POSIX_VDISABLE;
    r.intr   = t.c_cc[VINTR]==off  ? 0 : t.c_cc[VINTR];
    r.eof    = t.c_cc[VEOF]==off   ? 0 : t.c_cc[VEOF];
    r.erase  = t.c_cc[VERASE]==off ? 0 : t.c_cc[VERASE];
    r.kill   = t.c_cc[VKILL]==off  ? 0 : t.c_cc[VKILL];
    r.werase = t.c_cc[VWERASE]==off ? 0 : t.c_cc[VWERASE];
    r.lnext  = t.c_cc[VLNEXT]==off ? 0 : t.c_cc[VLNEXT];
    return r;
}

static b32 os_isatty(Os *, Handle h)
{
    return isatty((i32)h);
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
static Spawned forkimage(Os *, Arena, Spawn *);

static Spawned os_spawn(Os *os, Arena scratch, Spawn *s)
{
    if (s->image.base) return forkimage(os, scratch, s);
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

// Self-spawning with a state image: a plain reservation, which a forked
// child (forkimage) already has at the same address.
static ImageRef os_imagenew(Os *os, iz cap)
{
    ImageRef r = {};
    r.base = os_reserve(os, cap);
    r.cap  = r.base ? cap : 0;
    return r;
}

static void os_imagefree(Os *, ImageRef im)
{
    if (im.base) munmap(im.base, (uz)im.cap);
}

static Slice<InheritFd> inherited(Arena *);
static void initasync(Os *);
static void exitwith(i32 status);

// A forked image child's own fiber: the shell from its image (sh_main
// takes it to imagemain), on a fresh stack rather than wherever in the
// parent's call chain (perhaps a fiber's) the fork happened.
static void imagefiber(Os *os, void *arg)
{
    exitwith(sh_main(os, (OsInfo *)arg));
}

// os_spawn with an image: fork, and in the child give the descriptors
// listed their numbers, close the rest, start the async machinery
// afresh (the self-pipe was the parent's), reset the signals the shell
// changed but those it ignores, and run the image.
static Spawned forkimage(Os *os, Arena scratch, Spawn *s)
{
    Spawned r = {};
    r.proc = BADHANDLE;
    char *cwd = cpath(&scratch, s->cwd);
    i32  *src = alloc<i32>(&scratch, s->nfds ? s->nfds : 1);
    i32  *dst = alloc<i32>(&scratch, s->nfds ? s->nfds : 1);
    for (iz i = 0; i < s->nfds; i++) {
        src[i] = (i32)s->fds[i].h;
        dst[i] = s->fds[i].fd;
    }
    i32 nfds = (i32)s->nfds;

    sigset_t all, old;
    sigfillset(&all);
    sigprocmask(SIG_SETMASK, &all, &old);
    pid_t pid = fork();
    if (!pid) {
        for (i32 i = 0; i < nfds; i++) {
            src[i] = fcntl(src[i], F_DUPFD, 256);
        }
        closefds(0, 255);
        for (i32 i = 0; i < nfds; i++) dup2(src[i], dst[i]);
        closefds(256, -1);
        if (chdir(cwd)) {
            // as for a program: it runs anyway
        }
        for (i32 sig = 1; sig < NSIGNAL && sig < NSIG; sig++) {
            u64 bit = (u64)1 << (sig-1);
            if (sig==SIGKILL || sig==SIGSTOP || sig==SIGCHLD) continue;
            if ((s->sigign | os->entryign) & bit) {
                signal(sig, SIG_IGN);
            } else if (os->changed & bit) {
                signal(sig, sig==SIGPIPE ? SIG_IGN : SIG_DFL);
            }
        }
        os->changed = s->sigign & ~os->entryign;
        os->pending = 0;
        os->peekfd[0] = os->peekfd[1] = 0;
        sigbits = 0;
        levelhead = 0;
        leveltail = 0;
        static OsInfo info;
        Arena a = newarena(os, (iz)1<<24);
        Slice<InheritFd> in = inherited(&a);  // before the new self-pipe
        u64 entryign = os->entryign;
        initasync(os);
        os->entryign = entryign;  // as the parent had them
        sigprocmask(SIG_SETMASK, &old, 0);

        info.image    = s->image;
        info.inherit  = in.data;
        info.ninherit = in.len;
        info.pid      = getpid();
        info.ppid     = getppid();
        info.reserve  = os->reserve;
        info.stack    = os->stack;
        info.sigign   = os->entryign | s->sigign;
        for (i32 i = 0; i < 3; i++) info.std[i] = (Handle)i;
        Fiber *f = os_fiber(os, &a, os->stack, imagefiber, &info);
        os_switch(os, f);
        _exit(127);  // never reached
    }
    i32 e = pid<0 ? errno : 0;
    sigprocmask(SIG_SETMASK, &old, 0);
    if (e) {
        r.err = e;
        return r;
    }
    munmap(s->image.base, (uz)s->image.cap);  // the child's now
    r.proc = (Handle)pid;
    r.pid  = pid;
    return r;
}

// Fork, and in the original exec: the copy, which keeps the other
// tasks, learns how the exec went from a close-on-exec pipe, closed by
// a successful exec, written to on failure (the copy then vanishes).
static i32 os_forkexec(Os *os, Arena scratch, Spawn *s)
{
    i32 sync[2];
    if (pipe(sync)) return errno;
    for (i32 i = 0; i < 2; i++) sync[i] = highfd(sync[i]);
    if (sync[0]<0 || sync[1]<0) {
        if (sync[0] >= 0) close(sync[0]);
        if (sync[1] >= 0) close(sync[1]);
        return E_MFILE;
    }
    sigset_t all, old;
    sigfillset(&all);
    sigprocmask(SIG_SETMASK, &all, &old);
    pid_t pid = fork();
    if (!pid) {
        close(sync[1]);
        u8 c;
        iz n;
        do {
            n = read(sync[0], &c, 1);
        } while (n<0 && errno==EINTR);
        if (n) _exit(0);  // the original carries on
        close(sync[0]);
        sigbits = 0;  // signals were for the original's pid
        levelhead = 0;
        leveltail = 0;
        sigprocmask(SIG_SETMASK, &old, 0);
        return 0;
    }
    i32 err = pid<0 ? errno : 0;
    sigprocmask(SIG_SETMASK, &old, 0);
    close(sync[0]);
    if (!err) {
        err = os_exec(os, scratch, s);  // returns only on failure
        u8 c = 1;
        if (write(sync[1], &c, 1)) {}
        i32 status;
        while (waitpid(pid, &status, 0)<0 && errno==EINTR) {}
    }
    close(sync[1]);
    return err ? err : E_INVAL;
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

#ifdef SIGRTMAX
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

static i32 os_siglevel(Os *, i64 pid, i64 level, i32 sig)
{
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
#else
static i32 os_siglevel(Os *, i64, i64, i32)
{
    return E_NOSYS;
}
#endif

static LevelSig os_levelsig(Os *)
{
    LevelSig r = {};
    u32 h = levelhead;
    if (h != __atomic_load_n(&leveltail, __ATOMIC_ACQUIRE)) {
        r = levelq[h%NLEVELQ];
        levelhead = h + 1;
    }
    return r;
}

static u64 os_sigpending(Os *)
{
    return __atomic_exchange_n(&sigbits, (u64)0, __ATOMIC_RELAXED);
}

static u64 os_siggroup(Os *)
{
    return 0;  // terminal signals arrive as the process's own
}

static void os_tasktag(Os *, u64, iz, u64)
{
}

static void os_racy(Os *)
{
}

static void os_sigmode(Os *os, i32 sig, i32 mode)
{
    u64 bit = (u64)1 << (sig-1);
    if (sig<1 || sig>=NSIG || sig==SIGKILL || sig==SIGSTOP) return;
    if ((os->entryign & bit) && mode!=SIGM_CLAIM) return;
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
    case SIGM_CLAIM:
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
#ifdef SIGRTMAX
    sa = {};
    sa.sa_sigaction = onlevel;
    sa.sa_flags     = SA_RESTART | SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(siglevelnum(), &sa, 0);
#endif
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
        if (sigbits || levelhead!=leveltail) return 0;  // the core first
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

// glibc's default zone is the file /etc/localtime, which the core
// reads whole (transitions and footer), else UTC.
static Str os_localtz(Os *, Arena *)
{
    struct stat st;
    if (stat("/etc/localtime", &st) || !S_ISREG(st.st_mode)) {
        return "UTC0";
    }
    return ":/etc/localtime";
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

// The descriptors above 2 the shell inherited, found with one poll of
// them all (POLLNVAL marks a closed one) before the shell opens any,
// each moved out of the way as the shell's own: at 10 and up, and
// close-on-exec.
static Slice<InheritFd> inherited(Arena *a)
{
    Slice<InheritFd> r = {};
    struct rlimit rl;
    i32 max = FDMAX;
    if (!getrlimit(RLIMIT_NOFILE, &rl) && rl.rlim_cur<(rlim_t)max) {
        max = (i32)rl.rlim_cur;
    }
    if (max <= 3) return r;
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
        if (fds[i].revents & POLLNVAL) continue;
        i32 fl = fcntl(fds[i].fd, F_GETFL);
        InheritFd e = {};
        e.fd    = fds[i].fd;
        e.flags = (fl&O_ACCMODE)==O_RDONLY ? OF_READ :
                  (fl&O_ACCMODE)==O_WRONLY ? OF_WRITE : OF_READ|OF_WRITE;
        r = push(a, r, e);
    }
    // All found first: a duplicate may land on a number not yet seen
    iz n = 0;
    for (iz i = 0; i < r.len; i++) {
        i32 h = fcntl(r[i].fd, F_DUPFD_CLOEXEC, 10);
        close(r[i].fd);
        if (h < 0) continue;
        r[n] = r[i];
        r[n++].h = (Handle)h;
    }
    r.len = n;
    return r;
}

int main(int argc, char **argv)
{
    static Os os;
    OsInfo info = {};
    info.reserve = (iz)1 << (sizeof(void *)==8 ? 34 : 28);
    Arena a = newarena(&os, (iz)1<<26);
    Slice<InheritFd> in = inherited(&a);
    info.inherit  = in.data;
    info.ninherit = in.len;
    initasync(&os);
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
    char *exe = (char *)allocbytes(&a, 4096);
    ssize_t exelen = readlink("/proc/self/exe", exe, 4095);
    if (exelen > 0) {
        info.exe = {(u8 *)exe, (iz)exelen};  // Linux; elsewhere unknown
    }
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
    os.reserve  = info.reserve;
    os.stack    = info.stack;
    exitwith(sh_main(&os, &info));
}

static void exitwith(i32 status)
{
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
    _exit(status & 255);
}
