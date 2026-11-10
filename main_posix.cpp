// POSIX platform layer (Linux, macOS)
// $ g++ -std=c++20 -O2 -fno-exceptions -fno-rtti -fno-threadsafe-statics \n//       -nostdlib++ -o unabashed main_posix.cpp
// (only libc: libstdc++ would cost half of startup, for nothing)
// This is free and unencumbered software released into the public domain.
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#if __APPLE__
#include <libproc.h>
#include <sys/random.h>  // getentropy
#endif
extern "C" int rename(char const *, char const *);  // stdio.h, whose macros collide
#include "src/unabashed.cpp"
#include "src/fibers.cpp"

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
    // Output to a regular file is coalesced (see os_write)
    i32    wpend;       // the file's descriptor, while wlen > 0
    i32    wlen;
    i64    wtime;       // CLOCK_MONOTONIC ns when the buffer began
    i32    werrfd;      // a flush to this descriptor failed, else -1
    u8     wkind[256];  // per descriptor: FK_* (fdkind)
    u8     wbuf[1<<14];
    u64    entropyn;    // os_entropy's fallback counter
};

static u64 volatile sigbits;  // caught signals not yet taken
static u64 volatile grpbits;  // the terminal's, sent to its foreground group

// Level signals from another of our shells (os_siglevel), queued by
// the handler of a real-time signal: one producer (the handler, which
// cannot nest), one consumer (os_levelsig).
enum : u32 { NLEVELQ = 32 };
static LevelSig levelq[NLEVELQ];
static u32 volatile levelhead, leveltail;


static Fibers *osfibers(Os *os)
{
    return &os->fibers;
}

// Used before their definitions
static decltype(os_now)  plt_now;
static decltype(os_exec) plt_exec;

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

static i64 monons()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (i64)ts.tv_sec*1000000000 + ts.tv_nsec;
}

// Write out the coalesced output. A failure shows at the descriptor's
// next write. Every platform call but os_write to that same file does
// this first, so whatever can observe the file (a program, a stat,
// another descriptor, a reader of a pipe waiting for a later write)
// sees it, and so do exits and forks.
static void flushw(Os *os)
{
    if (!os->wlen) return;
    if (!writeall(os->wpend, os->wbuf, os->wlen)) os->werrfd = os->wpend;
    os->wlen = 0;
}

static Os *sigos;  // for the signal handler, os_oom and exits

[[noreturn]] static void os_oom()
{
    if (sigos) flushw(sigos);
    Str msg = "bash: out of memory\n";
    writeall(2, msg.data, msg.len);
    _exit(2);
}

static byte *plt_reserve(Os *, iz len)
{
    void *p = mmap(0, (uz)len, PROT_NONE, MAP_PRIVATE|MAP_ANON|MAP_NORESERVE, -1, 0);
    return p==MAP_FAILED ? 0 : (byte *)p;
}

static b32 plt_commit(Os *, byte *p, iz len)
{
    return !mprotect(p, (uz)len, PROT_READ|PROT_WRITE);
}

// Mapping over the range drops its pages and their contents.
static void plt_decommit(Os *, byte *p, iz len)
{
    mmap(p, (uz)len, PROT_NONE, MAP_PRIVATE|MAP_ANON|MAP_NORESERVE|MAP_FIXED, -1, 0);
}

// Output to a regular file gathers in wbuf: a write costs about as much
// as a builtin's whole run. It goes out before any other call (flushw),
// when full, and at the first write 100 ms after it began. Whether a
// descriptor is a regular file is remembered until os_close frees it.
static b32 wfailed(Os *os, i32 fd)
{
    if (os->werrfd != fd) return 0;
    os->werrfd = -1;
    return 1;
}

// What a descriptor is, remembered until os_close frees its number.
enum : u8 { FK_UNKNOWN, FK_REG, FK_FIFO, FK_OTHER };

static u8 fdkind(Os *os, i32 fd)
{
    b32 cache = fd>=0 && fd<countof(os->wkind);
    if (cache && os->wkind[fd]) return os->wkind[fd];
    struct stat st;
    u8 kind = fstat(fd, &st) ? FK_OTHER :
              S_ISREG(st.st_mode) ? FK_REG :
              S_ISFIFO(st.st_mode) ? FK_FIFO : FK_OTHER;
    if (cache) os->wkind[fd] = kind;
    return kind;
}

static b32 plt_write(Os *os, Handle h, u8 *buf, iz len)
{
    i32 fd = (i32)h;
    if (fdkind(os, fd) != FK_REG) {
        flushw(os);
        return !wfailed(os, fd) && writeall(fd, buf, len);
    }
    if (os->wlen && os->wpend!=fd) flushw(os);
    if (wfailed(os, fd)) return 0;
    iz room = countof(os->wbuf) - os->wlen;
    if (len > room) {
        flushw(os);
        if (wfailed(os, fd)) return 0;
        if (len > countof(os->wbuf)/2) return writeall(fd, buf, len);
    }
    if (!os->wlen) {
        os->wpend = fd;
        os->wtime = monons();
    }
    copybytes(os->wbuf+os->wlen, buf, len);
    os->wlen += (i32)len;
    if (monons()-os->wtime >= 100000000) {
        flushw(os);
        if (wfailed(os, fd)) return 0;
    }
    return 1;
}

static iz plt_read(Os *os, Handle h, u8 *buf, iz len)
{
    flushw(os);
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

// The lowest descriptor above every target of a spawn's map: sources
// move there first, out of the targets' way. Not a fixed high number,
// which a low `ulimit -n` would put out of reach.
static i32 fdbase(i32 *dst, i32 n)
{
    i32 base = 3;
    for (i32 i = 0; i < n; i++) base = dst[i]>=base ? dst[i]+1 : base;
    return base;
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

static Opened plt_open(Os *os, Arena scratch, Str path, i32 flags, i32 mode)
{
    flushw(os);
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

static i32 plt_unlink(Os *os, Arena scratch, Str path)
{
    flushw(os);
    return unlink(cpath(&scratch, path)) ? errno : 0;
}

static i32 plt_rename(Os *os, Arena scratch, Str from, Str to)
{
    flushw(os);
    char *f = cpath(&scratch, from);
    char *t = cpath(&scratch, to);
    return rename(f, t) ? errno : 0;
}

static Opened plt_tempfile(Os *os, Arena scratch)
{
    flushw(os);
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
        print(&b, (i64)(++counter ^ (u32)plt_now(os)));
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

static void plt_close(Os *os, Handle h)
{
    flushw(os);
    if (h == BADHANDLE) return;
    if ((i32)h < countof(os->wkind)) os->wkind[h] = 0;  // the number may come back
    if (os->werrfd == (i32)h) os->werrfd = -1;
    close((i32)h);
}

static Handle plt_dup(Os *os, Handle h)
{
    flushw(os);
    i32 fd = fcntl((i32)h, F_DUPFD_CLOEXEC, 10);
    return fd<0 ? BADHANDLE : (Handle)fd;
}

static NamedPipe plt_namedpipe(Os *os, Arena *, Str, i32)
{
    flushw(os);
    NamedPipe r = {};
    r.h   = BADHANDLE;
    r.err = E_NOSYS;
    return r;
}

static NamedTemp plt_namedtemp(Os *, Arena *, Str)
{
    NamedTemp r = {};
    r.h   = BADHANDLE;
    r.err = E_NOSYS;
    return r;
}

static iz plt_pipecap(Os *os)
{
    flushw(os);
#ifdef __linux__
    return 1<<16;
#else
    return 1<<14;  // macOS starts small
#endif
}

static OsPipe plt_pipe(Os *os)
{
    flushw(os);
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
static iz plt_peek(Os *os, Handle, u8 *, iz)
{
    flushw(os);
    return -1;  // pipes cannot be peeked
}
#endif

static i64 plt_seek(Os *os, Handle h, i64 off, i32 whence)
{
    flushw(os);
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

static Str plt_readlink(Os *os, Arena *a, Str path)
{
    flushw(os);
    char *p = cpath(a, path);
    for (iz cap = 256; cap < 1<<20; cap *= 2) {
        char *buf = (char *)allocbytes(a, cap);
        iz    n   = readlink(p, buf, (uz)cap);
        if (n < 0) return {};
        if (n < cap) return {(u8 *)buf, n};
    }
    return {};
}

static Stat plt_stat(Os *os, Arena scratch, Str path, b32 follow)
{
    flushw(os);
    struct stat s;
    char *p = cpath(&scratch, path);
    if (follow ? stat(p, &s) : lstat(p, &s)) {
        Stat r = {};
        r.err = errno;
        return r;
    }
    return tostat(&s);
}

static Stat plt_fstat(Os *os, Handle h)
{
    flushw(os);
    struct stat s;
    if (fstat((i32)h, &s)) {
        Stat r = {};
        r.err = errno;
        return r;
    }
    return tostat(&s);
}

static void onsignal(int, siginfo_t *, void *);
static void quietpipe(Os *);

static b32 plt_con(Os *os, Handle, ConOp *)
{
    flushw(os);
    return 0;  // no legacy consoles here
}

static Tty plt_tty(Os *os, Handle in, Handle out, i32 how)
{
    flushw(os);
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
            sa.sa_flags     = SA_RESTART | SA_SIGINFO;
            sa.sa_sigaction = onsignal;
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
    r.susp   = t.c_cc[VSUSP]==off  ? 0 : t.c_cc[VSUSP];
    return r;
}

static b32 plt_isatty(Os *, Handle h)
{
    return isatty((i32)h);
}

static Dirent *plt_list(Os *os, Arena *a, Str dir, i32 *err)
{
    flushw(os);
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

static Spawned forkimage(Os *, Arena, Spawn *);
static i32     rlres(i32);
static rlim_t  torlim(i64);

// Put a new child in its process group (pid 0: in the child itself),
// and give its group the terminal: both parent and child do it, so the
// child runs only once it holds. Signals are blocked, SIGTTOU included.
static void joingroup(pid_t pid, i64 pgid, b32 fg, i32 tty)
{
    if (!pgid) return;
    pid_t self  = pid ? pid : getpid();
    pid_t group = pgid<0 ? self : (pid_t)pgid;
    setpgid(pid, group);
    if (fg) tcsetpgrp(tty, group);
}

// What a program's process starts out with, worked out before it
// exists. Signals stay blocked from before it is made until it has its
// dispositions, so no handler of the shell's runs in it.
struct SpawnPlan {
    char    *path;
    char    *cwd;
    char   **argv;
    char   **envp;
    i32     *src;      // dst[i] gets src[i] (none if negative)
    i32     *dst;
    i32      nfds;
    u64      ign;      // signals it starts out ignoring
    u64      dfl;      // and with their default action
    Rlimit  *rlimits;  // or null to keep the shell's
    i64      pgid;
    b32      fg;
    i32      tty;
    sigset_t mask;     // its signal mask (the shell's)
};

static b32 sigin(u64 set, i32 sig)
{
    return sig<=64 && set>>(sig-1) & 1;
}

// The core numbers signals as Linux x86-64 does (trap.cpp), and other
// hosts, macOS among them, number them otherwise. Every number that
// crosses is translated: hostsig for one going out, coresig for one
// coming in, each 0 where the other side has no such signal; corebit
// and hostin take a host number into the core's masks. On Linux all
// of them are the identity.
#if __linux__
static_assert(SIGBUS==7 && SIGUSR1==10 && SIGCHLD==17 && SIGSTOP==19 &&
              SIGIO==29 && SIGSYS==31, "the generic Linux numbering");
static i32 hostsig(i32 sig) { return sig; }
static i32 coresig(i32 sig) { return sig; }
static u64 corebit(i32 sig) { return (u64)1 << (sig-1); }
static b32 hostin(u64 set, i32 sig) { return sigin(set, sig); }
#else
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
#endif

// With a fork: the child has its own memory, and reports a failed exec
// through a close-on-exec pipe, which a successful exec closes. For
// what posix_spawn cannot set up (trispawn).
static i32 forkspawn(SpawnPlan *p, pid_t *out)
{
    i32 sync[2];
    if (pipe(sync)) return errno;
    for (i32 i = 0; i < 2; i++) sync[i] = highfd(sync[i]);
    if (sync[0]<0 || sync[1]<0) {
        if (sync[0] >= 0) close(sync[0]);
        if (sync[1] >= 0) close(sync[1]);
        return E_MFILE;
    }
    pid_t pid = fork();
    if (!pid) {
        close(sync[0]);
        joingroup(0, p->pgid, p->fg, p->tty);
        // Move the pipe and the sources out of the way first, then
        // install the map
        i32 base = fdbase(p->dst, p->nfds);
        i32 w = fcntl(sync[1], F_DUPFD_CLOEXEC, base);
        for (i32 i = 0; i < p->nfds; i++) {
            if (p->src[i] < base) p->src[i] = fcntl(p->src[i], F_DUPFD, base);
        }
        closefds(0, base-1);
        for (i32 i = 0; i < p->nfds; i++) dup2(p->src[i], p->dst[i]);
        if (w < 0) {
            closefds(base, -1);
        } else {
            closefds(base, w-1);
            closefds(w+1, -1);
        }
        if (chdir(p->cwd)) {
            // run anyway: bash reports nothing here
        }
        // Every handler of the shell's goes too, as an exec would take
        // them, so that none runs here before it
        for (i32 sig = 1; sig < NSIG; sig++) {
            struct sigaction sa = {};
            if (sig==SIGKILL || sig==SIGSTOP || sigaction(sig, 0, &sa)) continue;
            b32 caught = sa.sa_handler!=SIG_DFL && sa.sa_handler!=SIG_IGN;
            if (hostin(p->ign, sig)) {
                signal(sig, SIG_IGN);
            } else if (caught || hostin(p->dfl, sig)) {
                signal(sig, SIG_DFL);
            }
        }
        for (i32 i = 0; p->rlimits && i<RL_COUNT; i++) {
            i32 n = rlres(i);
            if (n<0 || p->rlimits[i].err) continue;
            rlimit l = {torlim(p->rlimits[i].soft), torlim(p->rlimits[i].hard)};
            setrlimit(n, &l);  // as bash's forked subshell had them
        }
        sigprocmask(SIG_SETMASK, &p->mask, 0);
        execve(p->path, p->argv, p->envp);
        i32 err = errno ? errno : E_NOEXEC;
        if (w>=0 && write(w, &err, sizeof(err))) {}
        _exit(127);
    }
    i32 err = pid<0 ? errno : 0;
    close(sync[1]);
    if (pid > 0) {
        i32 got = 0;
        iz  n;
        do {
            n = read(sync[0], &got, sizeof(got));
        } while (n<0 && errno==EINTR);
        if (n == sizeof(got)) {
            err = got;
            while (waitpid(pid, 0, 0)<0 && errno==EINTR) {}
        }
    }
    close(sync[0]);
    *out = pid;
    return err;
}

// Where the C library's posix_spawn can change directory (glibc 2.29,
// macOS 10.15), it makes the process: glibc runs the child on a stack
// of its own in the shell's memory, macOS has a system call, either way
// no copy of the shell's memory. Returns -1, having done nothing, where
// it cannot set up the plan:
// - limits, which no attribute sets;
// - a signal to ignore that the shell does not: the child may only
//   reset signals to their default, and ignoring it in the shell for
//   the while would throw away one pending;
// - the terminal for a foreground group, without glibc 2.35's
//   attribute (the parent hands it over too, but only after the
//   program may already have run into the terminal);
// - a directory it cannot enter, where bash runs the program anyway.
#if __APPLE__
#  define SPAWN_CHDIR 1
#  if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 260000
#    define posix_spawn_file_actions_addchdir_np posix_spawn_file_actions_addchdir
#  endif
#elif defined(__GLIBC_PREREQ)
#  if __GLIBC_PREREQ(2, 29)
#    define SPAWN_CHDIR 1
#  endif
#  if __GLIBC_PREREQ(2, 34)
#    define SPAWN_CLOSEFROM 1
#  endif
#endif

#if SPAWN_CHDIR
static b32 dirok(char *path)
{
    struct stat st;
    return !stat(path, &st) && S_ISDIR(st.st_mode) && !access(path, X_OK);
}

static i32 trispawn(SpawnPlan *p, Arena scratch, pid_t *out)
{
    if (p->rlimits) return -1;
    #ifndef POSIX_SPAWN_TCSETPGROUP
    if (p->pgid && p->fg) return -1;
    #endif
    for (i32 sig = 1; sig < NSIG; sig++) {
        if (!hostin(p->ign, sig) || sig==SIGKILL || sig==SIGSTOP) continue;
        struct sigaction sa = {};
        if (sigaction(sig, 0, &sa) || sa.sa_handler!=SIG_IGN) return -1;
    }

    // Sources below a target move above them all first, in the shell,
    // as close-on-exec copies
    i32  base = fdbase(p->dst, p->nfds);
    i32 *src  = alloc<i32>(&scratch, p->nfds ? p->nfds : 1);
    i32  err  = 0;
    for (i32 i = 0; i < p->nfds; i++) {
        src[i] = p->src[i];
        if (src[i]<0 || src[i]>=base || err) continue;
        src[i] = fcntl(p->src[i], F_DUPFD_CLOEXEC, base);
        if (src[i] < 0) err = errno;
    }

    posix_spawnattr_t at;
    posix_spawn_file_actions_t fa;
    if (!err) {
        posix_spawnattr_init(&at);
        posix_spawn_file_actions_init(&fa);
        short flags = POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF;
        posix_spawnattr_setsigmask(&at, &p->mask);
        sigset_t dfl;
        sigemptyset(&dfl);
        for (i32 sig = 1; sig < NSIG; sig++) {
            if (sig==SIGKILL || sig==SIGSTOP) continue;
            if (hostin(p->dfl, sig)) sigaddset(&dfl, sig);
        }
        posix_spawnattr_setsigdefault(&at, &dfl);
        if (p->pgid) {
            flags |= POSIX_SPAWN_SETPGROUP;
            posix_spawnattr_setpgroup(&at, p->pgid<0 ? 0 : (pid_t)p->pgid);
            #ifdef POSIX_SPAWN_TCSETPGROUP
            if (p->fg) {
                flags |= POSIX_SPAWN_TCSETPGROUP;
                posix_spawnattr_tcsetpgrp_np(&at, p->tty);
            }
            #endif
        }

        // Nothing but the map: the shell's own descriptors are all
        // close-on-exec, but for the standard three it was given
        #if __APPLE__
        flags |= POSIX_SPAWN_CLOEXEC_DEFAULT;
        #else
        for (i32 fd = 0; fd < base; fd++) {
            b32 target = 0;
            for (i32 i = 0; i < p->nfds; i++) {
                target |= p->dst[i]==fd && src[i]>=0;
            }
            i32 fl = target ? -1 : fcntl(fd, F_GETFD);
            if (fl>=0 && !(fl&FD_CLOEXEC)) {
                posix_spawn_file_actions_addclose(&fa, fd);
            }
        }
        #endif
        for (i32 i = 0; i < p->nfds; i++) {
            if (src[i] >= 0) posix_spawn_file_actions_adddup2(&fa, src[i], p->dst[i]);
        }
        #if SPAWN_CLOSEFROM
        posix_spawn_file_actions_addclosefrom_np(&fa, base);
        #endif
        posix_spawn_file_actions_addchdir_np(&fa, p->cwd);
        posix_spawnattr_setflags(&at, flags);

        err = posix_spawn(out, p->path, &fa, &at, p->argv, p->envp);
        posix_spawn_file_actions_destroy(&fa);
        posix_spawnattr_destroy(&at);
        if (err && !dirok(p->cwd)) err = -1;
    }

    for (i32 i = 0; i < p->nfds; i++) {
        if (src[i]>=0 && src[i]!=p->src[i]) close(src[i]);
    }
    return err;
}
#else
static i32 trispawn(SpawnPlan *, Arena, pid_t *)
{
    return -1;
}
#endif

static Spawned plt_spawn(Os *os, Arena scratch, Spawn *s)
{
    flushw(os);
    if (s->image.base) return forkimage(os, scratch, s);
    Spawned r = {};
    r.proc = BADHANDLE;
    SpawnPlan p = {};
    p.path = cpath(&scratch, s->path);
    p.cwd  = cpath(&scratch, s->cwd);
    p.argv = alloc<char *>(&scratch, s->argc+1);
    for (iz i = 0; i < s->argc; i++) p.argv[i] = cpath(&scratch, s->argv[i]);
    p.envp = alloc<char *>(&scratch, s->nenv+1);
    for (iz i = 0; i < s->nenv; i++) p.envp[i] = cpath(&scratch, s->env[i]);
    p.src = alloc<i32>(&scratch, s->nfds ? s->nfds : 1);
    p.dst = alloc<i32>(&scratch, s->nfds ? s->nfds : 1);
    for (iz i = 0; i < s->nfds; i++) {
        p.src[i] = (i32)s->fds[i].h;
        p.dst[i] = s->fds[i].fd;
    }
    p.nfds = (i32)s->nfds;
    // The shell ignores SIGPIPE for its in-process subshells, and
    // catches or ignores what its traps say: the child starts with
    // defaults, except what it was told to ignore (trap '' in the
    // spawning subshell) and what was ignored on entry
    u64 changed = sigos ? sigos->changed : 0;
    if (!sigos || !sigos->pipeign) changed |= corebit(SIGPIPE);
    changed |= corebit(SIGCHLD);
    p.ign     = s->sigign | (sigos ? sigos->entryign : 0);
    p.dfl     = changed & ~p.ign;
    p.rlimits = s->rlimits;
    p.pgid    = s->pgid;
    p.fg      = s->fg;
    p.tty     = (i32)s->tty;

    sigset_t all;
    sigfillset(&all);
    sigprocmask(SIG_SETMASK, &all, &p.mask);
    pid_t pid = -1;
    i32 e = trispawn(&p, scratch, &pid);
    if (e < 0) e = forkspawn(&p, &pid);
    if (!e) joingroup(pid, p.pgid, p.fg, p.tty);
    sigprocmask(SIG_SETMASK, &p.mask, 0);
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
static ImageRef plt_imagenew(Os *os, iz cap)
{
    flushw(os);
    ImageRef r = {};
    r.base = plt_reserve(os, cap);
    r.cap  = r.base ? cap : 0;
    return r;
}

static void plt_imagefree(Os *os, ImageRef im)
{
    flushw(os);
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
        joingroup(0, s->pgid, s->fg, (i32)s->tty);
        if (s->gated) {
            close((i32)s->gatew);
            char c;
            while (read((i32)s->gater, &c, 1)<0 && errno==EINTR) {}
        }
        i32 base = fdbase(dst, nfds);
        for (i32 i = 0; i < nfds; i++) {
            if (src[i] < base) src[i] = fcntl(src[i], F_DUPFD, base);
        }
        closefds(0, base-1);
        for (i32 i = 0; i < nfds; i++) dup2(src[i], dst[i]);
        closefds(base, -1);
        if (chdir(cwd)) {
            // as for a program: it runs anyway
        }
        for (i32 i = 0; s->rlimits && i<RL_COUNT; i++) {
            i32 k = rlres(i);
            if (k<0 || s->rlimits[i].err) continue;
            rlimit l = {torlim(s->rlimits[i].soft), torlim(s->rlimits[i].hard)};
            setrlimit(k, &l);
        }
        for (i32 sig = 1; sig < NSIGNAL && sig < NSIG; sig++) {
            u64 bit = corebit(sig);
            if (sig==SIGKILL || sig==SIGSTOP || sig==SIGCHLD) continue;
            if ((s->sigign | os->entryign) & bit) {
                signal(sig, SIG_IGN);
            } else if (sig == SIGPIPE) {
                quietpipe(os);
            } else if (os->changed & bit) {
                signal(sig, SIG_DFL);
            }
        }
        os->changed = s->sigign & ~os->entryign;
        os->pending = 0;
        os->peekfd[0] = os->peekfd[1] = 0;
        sigbits = 0;
        grpbits = 0;
        levelhead = 0;
        leveltail = 0;
        constinit static OsInfo info = {};  // in .bss, no guard
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
        info.pgid     = getpgrp();
        info.reserve  = os->reserve;
        info.stack    = os->stack;
        info.sigign   = os->entryign | s->sigign;
        for (i32 i = 0; i < 3; i++) info.std[i] = (Handle)i;
        Fiber *f = os_fiber(os, &a, os->stack, imagefiber, &info);
        os_switch(os, f);
        _exit(127);  // never reached
    }
    i32 e = pid<0 ? errno : 0;
    if (pid > 0) joingroup(pid, s->pgid, s->fg, (i32)s->tty);
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
static i32 plt_forkexec(Os *os, Arena scratch, Spawn *s)
{
    flushw(os);
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
        grpbits = 0;
        levelhead = 0;
        leveltail = 0;
        sigprocmask(SIG_SETMASK, &old, 0);
        return 0;
    }
    i32 err = pid<0 ? errno : 0;
    sigprocmask(SIG_SETMASK, &old, 0);
    close(sync[0]);
    if (!err) {
        err = plt_exec(os, scratch, s);  // returns only on failure
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
static i32 plt_exec(Os *os, Arena scratch, Spawn *s)
{
    flushw(os);
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
    i32 base = 0;
    for (iz i = 0; i < n; i++) base = s->fds[i].fd>=base ? s->fds[i].fd+1 : base;
    for (iz i = 0; i<n && !err; i++) {
        src[i] = fcntl((i32)s->fds[i].h, F_DUPFD_CLOEXEC, base);
        err = src[i]<0 ? errno : 0;
    }
    for (iz i = 0; i<n && !err; i++) {
        save[i] = fcntl(s->fds[i].fd, F_DUPFD_CLOEXEC, base);  // -1 if closed
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
    if (!sigos || !sigos->pipeign) changed |= corebit(SIGPIPE);
    changed |= corebit(SIGCHLD);
    for (i32 sig = 1; sig < NSIGNAL && sig < NSIG; sig++) {
        u64 bit = corebit(sig);
        if (sig==SIGKILL || sig==SIGSTOP || !((ign|changed) & bit)) continue;
        struct sigaction sa = {};
        sa.sa_handler = ign&bit ? SIG_IGN : SIG_DFL;
        if (!sigaction(sig, &sa, old+sig)) touched |= (u64)1 << (sig-1);
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
    if (WIFSIGNALED(status)) return WSIGNALED | (128 + coresig(WTERMSIG(status)));
    if (WIFSTOPPED(status)) return WSTOP | coresig(WSTOPSIG(status));
    if (WIFCONTINUED(status)) return WCONT;
    return 127;
}

static b32 isproc(Io *io)
{
    return io->op==IO_PROC || io->op==IO_PROCSTOP;
}

static i32 plt_waitproc(Os *os, Handle proc)
{
    flushw(os);
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

// A fault of the shell's own (a bad address, a trapped assertion) as
// opposed to a signal sent to it: the kernel's fault codes are small
// and positive, while kill's and sigqueue's are zero or negative
// (Linux, OpenBSD) or from 0x10001 (FreeBSD, macOS).
static b32 faulted(int sig, siginfo_t *info)
{
    switch (sig) {
    case SIGILL: case SIGTRAP: case SIGBUS: case SIGFPE: case SIGSEGV:
    case SIGSYS:
        return info && info->si_code>0 && info->si_code<0x10000;
    }
    return 0;
}

// A trapped signal: note it and wake os_wait. The terminal's INT, QUIT
// and TSTP, which the kernel sends to its whole foreground group, are
// told apart from those sent to this process (kill): every level of
// the shell gets them, as every process of bash's foreground job would.
// A fault kills the shell as if it were never caught.
// A SIGPIPE of the shell's own making (a write to a broken pipe: the
// kernel's, naming no sender or the shell itself) is only EPIPE, as if
// ignored; the level that wrote acts on it (sigpipe), so that a broken
// pipe in an in-process subshell ends only the subshell.
static void onsignal(int sig, siginfo_t *info, void *)
{
    if (faulted(sig, info)) {
        struct sigaction sa = {};
        sa.sa_handler = SIG_DFL;
        sigaction(sig, &sa, 0);
        raise(sig);  // delivered on return, if returning does not fault again
        return;
    }
    if (sig==SIGPIPE && (!info || !info->si_pid || info->si_pid==getpid())) {
        return;
    }
    i32 saved = errno;
    b32 tty   = sig==SIGINT || sig==SIGQUIT || sig==SIGTSTP;
    #ifdef SI_KERNEL
    b32 grp   = tty && info && info->si_code==SI_KERNEL;
    #else
    b32 grp   = 0;
    (void)tty;
    (void)info;
    #endif
    __atomic_fetch_or(grp ? &grpbits : &sigbits, corebit(sig), __ATOMIC_RELAXED);
    u8 c = 0;
    if (write(sigos->wakefd[1], &c, 1)) {}
    errno = saved;
}

// RL_* as the system numbers them, -1 where it has no such limit
static i32 rlres(i32 res)
{
    switch (res) {
    #ifdef RLIMIT_RTTIME
    case RL_RTTIME:     return RLIMIT_RTTIME;
    #endif
    case RL_CORE:       return RLIMIT_CORE;
    case RL_DATA:       return RLIMIT_DATA;
    #ifdef RLIMIT_NICE
    case RL_NICE:       return RLIMIT_NICE;
    #endif
    case RL_FSIZE:      return RLIMIT_FSIZE;
    #ifdef RLIMIT_SIGPENDING
    case RL_SIGPENDING: return RLIMIT_SIGPENDING;
    #endif
    case RL_MEMLOCK:    return RLIMIT_MEMLOCK;
    case RL_RSS:        return RLIMIT_RSS;
    case RL_NOFILE:     return RLIMIT_NOFILE;
    #ifdef RLIMIT_MSGQUEUE
    case RL_MSGQUEUE:   return RLIMIT_MSGQUEUE;
    #endif
    #ifdef RLIMIT_RTPRIO
    case RL_RTPRIO:     return RLIMIT_RTPRIO;
    #endif
    case RL_STACK:      return RLIMIT_STACK;
    case RL_CPU:        return RLIMIT_CPU;
    case RL_NPROC:      return RLIMIT_NPROC;
    case RL_AS:         return RLIMIT_AS;
    #ifdef RLIMIT_LOCKS
    case RL_LOCKS:      return RLIMIT_LOCKS;
    #endif
    }
    return -1;
}

static i64 fromrlim(rlim_t v)
{
    return v==RLIM_INFINITY || v>(rlim_t)0x7fffffffffffffff ? RL_INFINITY : (i64)v;
}

static rlim_t torlim(i64 v)
{
    return v==RL_INFINITY ? RLIM_INFINITY : (rlim_t)v;
}

static Rlimit plt_getrlimit(Os *, i32 res)
{
    Rlimit r = {};
    if (res == RL_PIPE) {
        r.soft = r.hard = PIPE_BUF;
        return r;
    }
    i32    n = rlres(res);
    rlimit l = {};
    if (n<0 || getrlimit(n, &l)) {
        r.err = E_NOSYS;
        return r;
    }
    r.soft = fromrlim(l.rlim_cur);
    r.hard = fromrlim(l.rlim_max);
    return r;
}

static i32 plt_setrlimit(Os *os, i32 res, Rlimit r)
{
    flushw(os);
    if (res == RL_PIPE) return E_INVAL;
    i32 n = rlres(res);
    if (n < 0) return E_NOSYS;
    rlimit l = {torlim(r.soft), torlim(r.hard)};
    return setrlimit(n, &l) ? errno : 0;
}

static i32 plt_kill(Os *os, i64 pid, i32 sig)
{
    flushw(os);
    if (pid<-(i64)0x7fffffff || pid>(i64)0x7fffffff) return E_SRCH;
    i32 hsig = hostsig(sig);
    if (sig && !hsig) return E_INVAL;  // one of Linux's this host lacks
    return kill((pid_t)pid, hsig) ? errno : 0;
}

static i32 plt_setpgid(Os *os, i64 pid, i64 pgid)
{
    flushw(os);
    return setpgid((pid_t)pid, (pid_t)pgid) ? errno : 0;
}

static i64 plt_tcpgrp(Os *os, Handle tty, i64 pgid)
{
    flushw(os);
    if (!pgid) {
        pid_t r = tcgetpgrp((i32)tty);
        return r<0 ? -errno : r;
    }
    sigset_t ttou, old;
    sigemptyset(&ttou);
    sigaddset(&ttou, SIGTTOU);
    sigprocmask(SIG_BLOCK, &ttou, &old);
    i32 r = tcsetpgrp((i32)tty, (pid_t)pgid);
    i32 e = errno;
    sigprocmask(SIG_SETMASK, &old, 0);
    return r ? -e : pgid;
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
#else
static i32 plt_siglevel(Os *os, i64, i64, i32)
{
    flushw(os);
    return E_NOSYS;
}
#endif

static LevelSig plt_levelsig(Os *)
{
    LevelSig r = {};
    u32 h = levelhead;
    if (h != __atomic_load_n(&leveltail, __ATOMIC_ACQUIRE)) {
        r = levelq[h%NLEVELQ];
        levelhead = h + 1;
    }
    return r;
}

// Polled between commands: a plain load first spares the common case a
// locked exchange, and a bit set meanwhile waits for the next poll.
static u64 plt_sigpending(Os *)
{
    if (!__atomic_load_n(&sigbits, __ATOMIC_RELAXED)) return 0;
    return __atomic_exchange_n(&sigbits, (u64)0, __ATOMIC_RELAXED);
}

static u64 plt_siggroup(Os *)
{
    if (!__atomic_load_n(&grpbits, __ATOMIC_RELAXED)) return 0;
    return __atomic_exchange_n(&grpbits, (u64)0, __ATOMIC_RELAXED);
}

static void plt_tasktag(Os *, u64, iz, u64)
{
}

static void plt_racy(Os *)
{
}

// SIGPIPE's action when untrapped: caught all the same (see onsignal),
// so that one from another process has its default action.
static void quietpipe(Os *)
{
    struct sigaction sa = {};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags     = SA_RESTART | SA_SIGINFO;
    sa.sa_sigaction = onsignal;
    sigaction(SIGPIPE, &sa, 0);
}

static void plt_sigmode(Os *os, i32 sig, i32 mode)
{
    flushw(os);
    u64 bit  = (u64)1 << (sig-1);
    i32 hsig = hostsig(sig);
    if (hsig<1 || hsig>=NSIG || hsig==SIGKILL || hsig==SIGSTOP) return;
    if ((os->entryign & bit) && mode!=SIGM_CLAIM) return;
    if (hsig == SIGCHLD) return;  // os_wait's own handler
    if (mode==SIGM_DEFAULT && !(os->changed & bit)) {
        return;  // still as on entry, which may be a profiler's SIGPROF
    }
    os->changed |= bit;
    if (mode==SIGM_DEFAULT && hsig==SIGPIPE) {
        quietpipe(os);  // its in-process subshells' broken pipes
        return;
    }
    struct sigaction sa = {};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    switch (mode) {
    case SIGM_DEFAULT:
        sa.sa_handler = SIG_DFL;
        break;
    case SIGM_IGNORE:
        sa.sa_handler = SIG_IGN;
        break;
    case SIGM_CATCH:
    case SIGM_CLAIM:
        sa.sa_flags     = SA_RESTART | SA_SIGINFO;
        sa.sa_sigaction = onsignal;
        break;
    }
    sigaction(hsig, &sa, 0);
}

static void initasync(Os *os)
{
    sigos = os;
    struct sigaction sa = {};
    sigaction(SIGPIPE, 0, &sa);
    os->pipeign = sa.sa_handler == SIG_IGN;
    for (i32 sig = 1; sig < NSIGNAL && sig < NSIG; sig++) {
        if (!sigaction(sig, 0, &sa) && sa.sa_handler==SIG_IGN) {
            os->entryign |= corebit(sig);
        }
    }
    if (!os->pipeign) quietpipe(os);  // else it stays ignored
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
    sa.sa_flags   = SA_RESTART;  // stops too, for job control
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
        iz len = io->len<(1<<30) ? io->len : (1<<30);
        i32 avail = 0;
        if (!(revents & (POLLHUP|POLLERR)) && !ioctl((i32)io->h, FIONREAD, &avail)) {
            // Another process reading the same pipe may have taken what
            // poll saw: then read would block the whole shell, so wait
            // again. Never ask for more than is there.
            struct stat st;
            if (!avail && !fstat((i32)io->h, &st) && S_ISFIFO(st.st_mode)) return 0;
            if (avail>0 && avail<len) len = avail;
        }
        iz n;
        do {
            n = read((i32)io->h, io->buf, (uz)len);
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
    case IO_PROC:
    case IO_PROCSTOP: {
        i32   status = 0;
        i32   flags  = WNOHANG | (io->op==IO_PROCSTOP ? WUNTRACED|WCONTINUED : 0);
        pid_t r;
        do {
            r = waitpid((pid_t)io->h, &status, flags);
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

static void plt_submit(Os *os, Io *io)
{
    flushw(os);
    io->done = 0;
    io->next = 0;
    if (tryio(io, isproc(io) ? 1 : pollnow(io, 0))) {
        io->done = 1;
        return;
    }
    Io **tail = &os->pending;
    while (*tail) tail = &(*tail)->next;
    *tail = io;
}

static Io *plt_wait(Os *os, i64 deadline)
{
    if (deadline != WAIT_POLL) flushw(os);  // it may block
    enum { MAXPOLL = 256 };
    struct pollfd fds[MAXPOLL];
    for (;;) {
        // Process exits first: the self-pipe only says "look again"
        for (Io **p = &os->pending; *p; p = &(*p)->next) {
            Io *io = *p;
            if (isproc(io) && tryio(io, 1)) {
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
            if (isproc(io)) continue;
            fds[n].fd      = (i32)io->h;
            fds[n].events  = io->op==IO_READ ? POLLIN : POLLOUT;
            fds[n++].revents = 0;
        }
        i32 timeout = -1;
        if (deadline == WAIT_POLL) {
            timeout = 0;
        } else if (deadline > 0) {
            i64 left = (deadline - plt_now(os) + 999999) / 1000000;
            timeout = left<0 ? 0 : left>(1<<30) ? (1<<30) : (i32)left;
        }
        i32 r = poll(fds, (nfds_t)n, timeout);
        if (r<0 && errno!=EINTR) return 0;
        if (fds[0].revents) {
            u8 buf[64];
            while (read(os->wakefd[0], buf, sizeof(buf)) > 0) {}
        }
        if (sigbits || grpbits || levelhead!=leveltail) return 0;  // the core first
        i32 k = 1;
        for (Io **p = &os->pending; *p; p = &(*p)->next) {
            Io *io = *p;
            if (isproc(io)) continue;
            if (k >= n) break;
            if (tryio(io, fds[k++].revents)) {
                *p = io->next;
                io->done = 1;
                return io;
            }
        }
        for (Io **p = &os->pending; *p; p = &(*p)->next) {
            Io *io = *p;
            if (isproc(io) && tryio(io, 1)) {
                *p = io->next;
                io->done = 1;
                return io;
            }
        }
        if (r==0 && timeout>=0) return 0;
    }
}

static b32 plt_cancel(Os *os, Io *io)
{
    flushw(os);
    for (Io **p = &os->pending; *p; p = &(*p)->next) {
        if (*p == io) {
            *p = io->next;
            return 1;
        }
    }
    return 0;
}

static i64 plt_now(Os *)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (i64)ts.tv_sec*1000000000 + ts.tv_nsec;
}

static u64 plt_entropy(Os *os)
{
    u64 r = 0;
    if (!getentropy(&r, sizeof(r))) {
        return r;
    }
    u64 x = (u64)plt_now(os) ^ (u64)getpid()<<40;
    x += ++os->entropyn * 0x9e3779b97f4a7c15u;
    x  = (x ^ x>>30) * 0xbf58476d1ce4e5b9u;
    x  = (x ^ x>>27) * 0x94d049bb133111ebu;
    return x ^ x>>31;
}

// glibc's default zone is the file /etc/localtime, which the core
// reads whole (transitions and footer), else UTC.
static ClipText plt_clipget(Os *, Arena *) { return {{}, E_NOSYS}; }
static i32 plt_clipset(Os *, Arena, Str) { return E_NOSYS; }

static Str plt_localtz(Os *, Arena *)
{
    struct stat st;
    if (stat("/etc/localtime", &st) || !S_ISREG(st.st_mode)) {
        return "UTC0";
    }
    return ":/etc/localtime";
}

// The platform table (api.cpp, OsVtable)
static OsVtable const pltvtable = {
    .version    = OSVTABLE_VERSION,
    .reserve    = plt_reserve,
    .commit     = plt_commit,
    .decommit   = plt_decommit,
    .write      = plt_write,
    .read       = plt_read,
    .open       = plt_open,
    .tempfile   = plt_tempfile,
    .namedtemp  = plt_namedtemp,
    .close      = plt_close,
    .unlink     = plt_unlink,
    .rename     = plt_rename,
    .dup        = plt_dup,
    .pipe       = plt_pipe,
    .namedpipe  = plt_namedpipe,
    .pipecap    = plt_pipecap,
    .peek       = plt_peek,
    .seek       = plt_seek,
    .stat       = plt_stat,
    .readlink   = plt_readlink,
    .fstat      = plt_fstat,
    .isatty     = plt_isatty,
    .tty        = plt_tty,
    .con        = plt_con,
    .list       = plt_list,
    .spawn      = plt_spawn,
    .imagenew   = plt_imagenew,
    .imagefree  = plt_imagefree,
    .exec       = plt_exec,
    .forkexec   = plt_forkexec,
    .waitproc   = plt_waitproc,
    .submit     = plt_submit,
    .wait       = plt_wait,
    .cancel     = plt_cancel,
    .kill       = plt_kill,
    .setpgid    = plt_setpgid,
    .tcpgrp     = plt_tcpgrp,
    .getrlimit  = plt_getrlimit,
    .setrlimit  = plt_setrlimit,
    .siglevel   = plt_siglevel,
    .levelsig   = plt_levelsig,
    .sigpending = plt_sigpending,
    .siggroup   = plt_siggroup,
    .sigmode    = plt_sigmode,
    .now        = plt_now,
    .entropy    = plt_entropy,
    .clipget    = plt_clipget,
    .clipset    = plt_clipset,
    .localtz    = plt_localtz,
    .tasktag    = plt_tasktag,
    .racy       = plt_racy,
};

static OsVtable const *osvtable(Os *)
{
    return &pltvtable;
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

// The build target as a configure triple, cpu-vendor-os, for MACHTYPE
// (and through it HOSTTYPE and OSTYPE), with the names config.guess
// gives. On macOS and the BSDs the os carries the kernel release, as
// config.guess appends it (darwin24.4.0, freebsd14.1).
static Str machtype(Arena *a)
{
    #if __aarch64__
    Str cpu = "aarch64";
    #elif __x86_64__
    Str cpu = "x86_64";
    #elif __i386__
    Str cpu = "i686";
    #elif __arm__
    Str cpu = "arm";
    #elif __powerpc64__ && __LITTLE_ENDIAN__
    Str cpu = "powerpc64le";
    #elif __powerpc64__
    Str cpu = "powerpc64";
    #elif __powerpc__
    Str cpu = "powerpc";
    #elif __riscv && __riscv_xlen==64
    Str cpu = "riscv64";
    #else
    Str cpu = "unknown";
    #endif

    Str vendor = "unknown";
    b32 release = 1;
    #if __APPLE__
    vendor = "apple";
    Str os = "darwin";
    #elif __FreeBSD__
    Str os = "freebsd";
    #elif __OpenBSD__
    Str os = "openbsd";
    #elif __NetBSD__
    Str os = "netbsd";
    #elif __linux__
    #if __x86_64__ || __i386__
    vendor = "pc";
    #endif
    #if __ANDROID__
    Str os = "linux-android";
    #elif __GLIBC__
    Str os = "linux-gnu";
    #else
    Str os = "linux-musl";
    #endif
    release = 0;
    #else
    Str os = "unknown";
    release = 0;
    #endif

    Str r = concat3(a, cpu, "-", vendor);
    r = concat3(a, r, "-", os);
    struct utsname u;
    if (release && !uname(&u)) {
        r = concat(a, r, cut(fromcstr(u.release), '-').head);  // 14.1-RELEASE
    }
    return r;
}

static Slice<InheritFd> pushinherit(Arena *a, Slice<InheritFd> r, i32 fd)
{
    i32 fl = fcntl(fd, F_GETFL);
    if (fl < 0) return r;
    InheritFd e = {};
    e.fd    = fd;
    e.flags = (fl&O_ACCMODE)==O_RDONLY ? OF_READ :
              (fl&O_ACCMODE)==O_WRONLY ? OF_WRITE : OF_READ|OF_WRITE;
    return push(a, r, e);
}

// The descriptors above 2 the shell inherited, found before the shell
// opens any, each moved out of the way as the shell's own: at 10 and
// up, and close-on-exec. Linux finds them with one poll of them all
// (POLLNVAL marks a closed one). macOS's poll cannot tell: asked for no
// events it reports nothing for a closed descriptor, and asked for any
// it reports POLLNVAL for an open device, so the kernel lists them.
static Slice<InheritFd> inherited(Arena *a)
{
    Slice<InheritFd> r = {};
    struct rlimit rl;
    i32 max = FDMAX;
    if (!getrlimit(RLIMIT_NOFILE, &rl) && rl.rlim_cur<(rlim_t)max) {
        max = (i32)rl.rlim_cur;
    }
    if (max <= 3) return r;
    #if __APPLE__
    i32 size = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, 0, 0);
    if (size <= 0) return r;
    iz cap = size / (iz)sizeof(proc_fdinfo);
    proc_fdinfo *list = alloc<proc_fdinfo>(a, cap ? cap : 1);
    size = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, list, (i32)(cap*(iz)sizeof(*list)));
    for (iz i = 0; size>0 && i<size/(iz)sizeof(*list); i++) {
        i32 fd = list[i].proc_fd;
        if (fd>=3 && fd<max) r = pushinherit(a, r, fd);
    }
    #else
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
    #endif
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
    os.werrfd = -1;
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
    info.pgid = getpgrp();
    info.uid  = getuid();
    info.euid = geteuid();
    info.gid  = getgid();
    info.egid = getegid();
    char *host = (char *)allocbytes(&a, 256);
    if (!gethostname(host, 255)) {
        host[255] = 0;
        info.host = fromcstr(host);
    }
    info.machtype = machtype(&a);
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
    if (sigos) flushw(sigos);
    #if PROFILE
    exit(status & 255);  // -DPROFILE -pg: exit writes gprof's gmon.out
    #endif
    if (status & WSIGNALED) {
        // Killed by a signal: die by it, so that the parent sees that
        i32 sig = hostsig((status & 255) - 128);
        signal(sig, SIG_DFL);
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, sig);
        sigprocmask(SIG_UNBLOCK, &set, 0);
        kill(getpid(), sig);
    }
    _exit(status & 255);
}
