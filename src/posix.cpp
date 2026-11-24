// POSIX platform layer (Linux, macOS)
// A root includes the system headers, the core and the fibers, then
// this module and the OS's own (linux.cpp, darwin.cpp), then its
// startup code (main_linux.cpp, main_darwin.cpp for the executable).
// This is free and unencumbered software released into the public domain.

// A directory the shell entered (os_cwd), held open (plt_cwd)
struct Pin {
    i32   fd;    // 0: free (a pin is above 2, highfd)
    u32   used;  // Os.pinclock when last entered or started in
    dev_t dev;
    ino_t ino;
    iz    len;
    char  path[PATH_MAX];
};
enum : i32 { NPIN = 8 };

struct Os {
    Fibers fibers;
    Pin    pins[NPIN];
    u32    pinclock;
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
    i32    ttyin;       // the terminal, while its mode is changed
    i32    ttyout;
    b32    ttypaste;    // TTY_PASTE
    // Output to a regular file is coalesced (see plt_write)
    i32    wpend;       // the file's descriptor, while wlen > 0
    i32    wlen;
    i32    wmax;        // bytes the buffer may hold for it (wroom)
    i64    wtime;       // CLOCK_MONOTONIC ns when the buffer began
    i32    wrun;        // the last call wrote to this descriptor, and
                        // succeeded, else -1
    i32    werrfd;      // a flush to this descriptor failed, else -1
    i32    werr;        // why the last write failed (os_writeerr)
    b32    wcatch;      // the deadly signals are caught (wcatchsigs)
    u64    refault;     // host signals onsignal left at default (faultsure)
    i32    readyfd;     // os_ready: the descriptor it counted, and the
    iz     readyn;      // bytes waiting there that no read took since
    u8     wkind[256];  // per descriptor: FK_* (fdkind)
    u8     wbuf[1<<14];
    u64    entropyn;    // os_entropy's fallback counter
    struct pollfd *pollv;  // os_wait's records past pollfix (pollroom)
    iz     pollcap;
    struct pollfd pollfix[64];
    u64 volatile sigbits;  // caught signals not yet taken
    u64 volatile grpbits;  // the terminal's, sent to its foreground group
};

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
static decltype(os_monotime) plt_monotime;
static decltype(os_exec) plt_exec;
static i32 coreerr(i32);

// Write all bytes: 0, or why not (E_*).
static i32 writeall(i32 fd, u8 *buf, iz len)
{
    for (iz off = 0; off < len;) {
        iz n = write(fd, buf+off, (uz)(len-off));
        if (n < 0) {
            if (errno == EINTR) continue;
            return coreerr(errno);
        }
        off += n;
    }
    return 0;
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
// sees it, and so do exits and forks; and so does the poll for signals
// between commands, once the output is WDELAY old, and a deadly signal
// (wcatchsigs). Called from a signal handler too: the buffer is emptied
// before the write, so that a handler cut in never writes it twice.
static void flushw(Os *os)
{
    os->wrun = -1;
    i32 len = os->wlen;
    if (!len) return;
    os->wlen = 0;
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
    i32 err = writeall(os->wpend, os->wbuf, len);
    if (err) {
        os->werrfd = os->wpend;
        os->werr   = err;
    }
}

static Os *sigos;  // for the signal handler, os_oom and exits

// Signal masks, set by a handler (or by another thread, in the library)
// and taken by the shell. Where a 64-bit atomic would take a lock
// (32-bit PowerPC), each 32-bit half is atomic on its own, which is all
// a mask of independent bits needs.
static void bitsor(u64 volatile *p, u64 bits)
{
    if constexpr (__atomic_always_lock_free(8, 0)) {
        __atomic_fetch_or(p, bits, __ATOMIC_RELAXED);
    } else {
        u32 volatile *h = (u32 volatile *)p;
        if ((u32)bits) __atomic_fetch_or(h+0, (u32)bits, __ATOMIC_RELAXED);
        if (bits>>32) __atomic_fetch_or(h+1, (u32)(bits>>32), __ATOMIC_RELAXED);
    }
}

// A plain load first spares the common case a locked exchange, and a
// bit set meanwhile waits for the next call.
static u64 bitstake(u64 volatile *p)
{
    if constexpr (__atomic_always_lock_free(8, 0)) {
        if (!__atomic_load_n(p, __ATOMIC_RELAXED)) return 0;
        return __atomic_exchange_n(p, (u64)0, __ATOMIC_RELAXED);
    } else {
        u32 volatile *h = (u32 volatile *)p;
        u64 r = 0;
        for (i32 i = 0; i < 2; i++) {
            if (__atomic_load_n(h+i, __ATOMIC_RELAXED)) {
                r |= (u64)__atomic_exchange_n(h+i, 0u, __ATOMIC_RELAXED) << (32*i);
            }
        }
        return r;
    }
}

// The library's own (libposix.cpp), named only in "if constexpr
// (LIBMODE)" branches: its fiber stacks are among the run's
// reservations, spawnlock brackets descriptors not yet close-on-exec,
// lib_reaped tracks the run's programs, with each status as
// os_waitproc gives it, lib_wait4 waits for them without reaping an
// exit (its zombie keeps its group's id), lib_dir the directory
// plt_list holds open, and lib_tmpdir is TMPDIR from the run's
// environment.
static byte *lib_fiberstack(Os *, iz size);
static void  lib_fiberunstack(Os *, byte *, iz size);
static void  lib_spawnlock(b32 lock);
static void  lib_reaped(i64 pid, i32 status, struct rusage const *);
static pid_t lib_wait4(pid_t, i32 *status, i32 flags, struct rusage *);
static void  lib_dir(Os *, DIR *);
static char *lib_tmpdir(Os *);

// The OS's own (linux.cpp, darwin.cpp), included after this module:
// what Linux and macOS do differently.
struct SpawnPlan;
static i32   cloexecpipe(i32 fds[2]);
static iz    plt_pipecap(Os *);
static iz    plt_peek(Os *, Handle, u8 *, iz);
static void  stattimes(Stat *, struct stat *);
static b32   closerange(i32 lo, i32 hi);
static i32   hostsig(i32 sig);
static i32   coresig(i32 sig);
static i32   coreerr(i32 err);
static iz    mainstack(byte *here, iz limit);
static Str   fromcstr(char *);
static u64   corebit(i32 sig);
static b32   hostin(u64 set, i32 sig);
static b32   spawnable(SpawnPlan *);
static void  spawndir(posix_spawn_file_actions_t *, SpawnPlan *);
static short spawnpre(posix_spawnattr_t *, posix_spawn_file_actions_t *,
                      SpawnPlan *, i32 *src, i32 base);
static void  spawnpost(posix_spawn_file_actions_t *, SpawnPlan *, i32 base);
static b32   fromterminal(siginfo_t *);
static b32   faultsure(int sig, siginfo_t *);
static i32   rlres(i32 res);
static i32   plt_siglevel(Os *, i64 pid, i64 level, i32 sig);
static void  initlevel();
static u8    fifokind(struct stat *);
static Io   *fifoends(Os *, b32 *any);
static Str   machos(Str *vendor, b32 *release);
static Slice<InheritFd> openfds(Arena *, i32 max);


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

// What a descriptor is, remembered until os_close frees its number.
// FK_NFIFO, a named FIFO, only on macOS, whose poll misses its end
// (fifoends); elsewhere it is an FK_FIFO like a pipe (fifokind).
enum : u8 { FK_UNKNOWN, FK_REG, FK_FIFO, FK_OTHER, FK_NFIFO };

static u8 fdkind(Os *os, i32 fd)
{
    b32 cache = fd>=0 && fd<countof(os->wkind);
    if (cache && os->wkind[fd]) return os->wkind[fd];
    struct stat st;
    u8 kind = fstat(fd, &st) ? FK_OTHER :
              S_ISREG(st.st_mode) ? FK_REG :
              S_ISFIFO(st.st_mode) ? FK_FIFO : FK_OTHER;
    if (kind == FK_FIFO) kind = fifokind(&st);
    if (cache) os->wkind[fd] = kind;
    return kind;
}

// Output to a regular file gathers in wbuf: a write costs about as much
// as a builtin's whole run (a loop of echos to a file takes a fifth of
// the time). But a builtin reports its failed write, as bash's does once
// it flushes its stdout, so only a write that should not fail waits:
// never the first of a run of writes to the descriptor with no other
// call between them (a lone `printf >file`), nor one after a failure,
// nor one past what wroom allows. What fails later all the same (an
// I/O error, a quota, a disk others filled meanwhile) shows at the
// descriptor's next write.
enum : i64 { WDELAY = 100000000 };  // ns before waiting output goes out
enum : u64 { WSPARE = 1<<20 };      // free bytes below which none waits

// The error of an earlier write to fd that failed as it went out, once.
static i32 wfailed(Os *os, i32 fd)
{
    if (os->werrfd != fd) return 0;
    os->werrfd = -1;
    return os->werr;
}

// What the buffer may hold for fd, worked out as it begins: nothing on
// a nearly full file system, and no more than the file may still grow
// by under RLIMIT_FSIZE, so that a write that would fail is made at once.
static i32 wroom(i32 fd, i32 cap)
{
    struct statvfs fs;
    u64 bsize = 0;
    if (!fstatvfs(fd, &fs)) bsize = fs.f_frsize ? fs.f_frsize : fs.f_bsize;
    if (!bsize || fs.f_bavail<WSPARE/bsize) return 0;
    struct rlimit rl;
    if (getrlimit(RLIMIT_FSIZE, &rl) || rl.rlim_cur==RLIM_INFINITY) return cap;
    off_t at = lseek(fd, 0, SEEK_CUR);
    i32   fl = fcntl(fd, F_GETFL);
    struct stat st;
    if (fl>=0 && (fl&O_APPEND) && !fstat(fd, &st)) at = st.st_size;
    if (at<0 || (rlim_t)at>=rl.rlim_cur) return 0;
    rlim_t left = rl.rlim_cur - (rlim_t)at;
    return left<(rlim_t)cap ? (i32)left : cap;
}

static void wcatchsigs(Os *);

static b32 plt_write(Os *os, Handle h, u8 *buf, iz len)
{
    i32 fd  = (i32)h;
    b32 run = os->wrun==fd && fdkind(os, fd)==FK_REG;
    if (os->wlen && (os->wpend!=fd || len>os->wmax-os->wlen)) flushw(os);
    i32 err = wfailed(os, fd);
    if (run && !err) {
        if (!os->wlen) {
            os->wpend = fd;
            os->wmax  = wroom(fd, countof(os->wbuf));
            os->wtime = monons();
        }
        if (len <= os->wmax-os->wlen) {
            copybytes(os->wbuf+os->wlen, buf, len);
            __atomic_signal_fence(__ATOMIC_SEQ_CST);  // see flushw
            os->wlen += (i32)len;
            if (!os->wcatch) wcatchsigs(os);
            if (monons()-os->wtime >= WDELAY) {
                flushw(os);
                err = wfailed(os, fd);
            }
            os->wrun = err ? -1 : fd;
            return !err;
        }
    }
    flushw(os);
    if (!err) err = wfailed(os, fd);
    i32 now = writeall(fd, buf, len);
    err = now ? now : err;
    os->wrun = err ? -1 : fd;
    if (err) os->werr = err;
    return !err;
}

static i32 plt_writeerr(Os *os)
{
    return os->werr;
}

static iz plt_read(Os *os, Handle h, u8 *buf, iz len)
{
    flushw(os);
    iz max = (iz)1<<30;  // macOS rejects reads over INT_MAX
    for (;;) {
        iz n = read((i32)h, buf, (uz)(len<max ? len : max));
        if (n<0 && errno==EINTR) continue;
        // another descriptor may be the same pipe
        b32 same = os->readyfd==(i32)h && n>0 && n<=os->readyn;
        os->readyn = same ? os->readyn-n : 0;
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
// move from there up first, out of the targets' way (fdaside). Not a
// fixed high number, which a low `ulimit -n` would put out of reach.
static i32 fdbase(i32 *dst, i32 n)
{
    i32 base = 3;
    for (i32 i = 0; i < n; i++) base = dst[i]>=base ? dst[i]+1 : base;
    return base;
}

// Keep shell-held descriptors away from the low numbers that scripts
// redirect, and out of children. Where no number from 10 up is free (a
// low `ulimit -n`), one above 2 stays where it is, as bash's would.
// Fails (-1, errno) only with no room at all, the original closed.
static i32 highfd(i32 fd)
{
    if (fd < 0) return fd;
    i32 r = fcntl(fd, F_DUPFD_CLOEXEC, 10);
    if (r<0 && fd>2 && !fcntl(fd, F_SETFD, FD_CLOEXEC)) return fd;
    if (r < 0) r = fcntl(fd, F_DUPFD_CLOEXEC, 3);
    i32 err = errno;
    close(fd);
    errno = err;
    return r;
}

// Directories the shell entered, held open: a program starts in the
// directory itself, as bash's forked child inherits it, so that it runs
// there even once the directory is renamed or removed, where its name
// leads elsewhere or nowhere. The least recently used goes first, but
// one its name still reaches before one only the descriptor can.
// Without its pin a program starts at the name, or not at all.

#ifdef O_PATH
enum : i32 { O_PIN = O_PATH|O_DIRECTORY|O_CLOEXEC };  // only search access
#else
enum : i32 { O_PIN = O_RDONLY|O_DIRECTORY|O_CLOEXEC };
#endif

static b32 reachable(Pin *p)  // does its name still lead to it?
{
    struct stat st;
    return !stat(p->path, &st) && st.st_dev==p->dev && st.st_ino==p->ino;
}

static Pin *findpin(Os *os, Str dir)
{
    for (i32 i = 0; i < NPIN; i++) {
        Pin *p = os->pins + i;
        if (p->fd && p->len==dir.len && dir.len &&
            !__builtin_memcmp(p->path, dir.data, (uz)dir.len)) {
            return p;
        }
    }
    return 0;
}

// Pin directory fd (taken) under name dir, replacing any pin of dir.
static void pinfd(Os *os, Str dir, i32 fd)
{
    Pin *p = findpin(os, dir);
    if (!p) {
        Pin *reach = 0;
        Pin *any   = 0;
        for (i32 i = 0; i<NPIN && !p; i++) {
            Pin *q = os->pins + i;
            if (!q->fd) {
                p = q;
            } else {
                any = !any || q->used<any->used ? q : any;
                if (reachable(q) && (!reach || q->used<reach->used)) reach = q;
            }
        }
        p = p ? p : reach ? reach : any;
    }
    if (p->fd) close(p->fd);
    p->fd = 0;
    struct stat st;
    fd = highfd(fd);
    if (fd<0 || fstat(fd, &st) || dir.len>=PATH_MAX) {
        if (fd >= 0) close(fd);
        return;
    }
    p->fd   = fd;
    p->used = ++os->pinclock;
    p->dev  = st.st_dev;
    p->ino  = st.st_ino;
    p->len  = dir.len;
    copybytes(p->path, dir.data, dir.len);
    p->path[dir.len] = 0;
}

static void plt_cwd(Os *os, Arena scratch, Str dir)
{
    flushw(os);
    if constexpr (LIBMODE) return;  // no descriptors held between runs
    Pin *p = findpin(os, dir);
    if (p && reachable(p)) {
        p->used = ++os->pinclock;  // still the same directory
        return;
    }
    if (dir.len >= PATH_MAX) return;
    pinfd(os, dir, open(cpath(&scratch, dir), O_PIN));
}

// The descriptor to start a program in dir with, or -1 for its name.
static i32 pinned(Os *os, Str dir)
{
    Pin *p = findpin(os, dir);
    if (!p) return -1;
    p->used = ++os->pinclock;
    return p->fd;
}

// Enter the directory: by its pin, else by name, where an empty name
// is the process's own (OsInfo.cwderr). 0 or an errno.
static i32 enterdir(i32 fd, char *path)
{
    if (fd<0 && !*path) return 0;
    return (fd>=0 ? fchdir(fd) : chdir(path)) ? errno : 0;
}

// getcwd's name for the process's directory, however long, or null
// with errno set.
static char *getcwdgrow(Arena *a)
{
    for (iz cap = PATH_MAX; cap <= (iz)1<<20; cap *= 2) {
        Arena save = *a;
        char *buf = (char *)allocbytes(a, cap);
        if (getcwd(buf, (uz)cap)) return buf;
        *a = save;
        if (errno != ERANGE) break;
    }
    return 0;
}

// A pinned directory's name, from getcwd once in it, so ENOENT once it
// is removed; the process returns to its own at once. Without
// a pin (always in the library, which so never moves the host's), the
// name's own resolution.
static Str plt_getcwd(Os *os, Arena *a, Str dir, i32 *err)
{
    flushw(os);
    *err = 0;
    Pin *p = findpin(os, dir);
    if (!p) {
        char *buf = (char *)allocbytes(a, PATH_MAX);
        if (dir.len < PATH_MAX && realpath(cpath(a, dir), buf)) return fromcstr(buf);
        *err = dir.len<PATH_MAX ? coreerr(errno) : E_NAMETOOLONG;
        return {};
    }
    i32 dot = open(".", O_PIN);
    if (dot < 0) {
        *err = coreerr(errno);
        return {};
    }
    Str name = {};
    if (fchdir(p->fd)) {
        *err = coreerr(errno);
    } else {
        char *buf = getcwdgrow(a);
        *err = buf ? 0 : coreerr(errno);
        name = buf ? fromcstr(buf) : Str{};
        if (fchdir(dot)) {}
    }
    close(dot);
    return name;
}

// The working directory's name for OsInfo.cwd: getcwd's, however long,
// if it leads back, else an inherited absolute $PWD that names it. Empty,
// the reason in *err, when it has no name (removed, or a parent cannot
// be read); getcwd's name, but *err set, when too long to resolve.
static Str cwdname(Arena *a, Str *env, iz nenv, i32 *err)
{
    *err = 0;
    Str name = {};
    if (char *buf = getcwdgrow(a)) {
        struct stat st;
        name = fromcstr(buf);
        if (!stat(buf, &st)) return name;
    }
    i32 e = errno;
    struct stat dot, st;
    for (iz i = 0; i < nenv; i++) {
        if (startswith(env[i], "PWD=/") && !stat(".", &dot)) {
            char *pwd = cpath(a, cuthead(env[i], 4));
            if (!stat(pwd, &st) && st.st_dev==dot.st_dev && st.st_ino==dot.st_ino) {
                return fromcstr(pwd);
            }
        }
    }
    *err = coreerr(e);
    return name;
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
        r.err = coreerr(errno);
        return r;
    }
    fd = highfd(fd);
    r.h   = fd<0 ? BADHANDLE : (Handle)fd;
    r.err = fd<0 ? coreerr(errno) : 0;
    return r;
}

static i32 plt_unlink(Os *os, Arena scratch, Str path)
{
    flushw(os);
    return unlink(cpath(&scratch, path)) ? coreerr(errno) : 0;
}

static i32 plt_rename(Os *os, Arena scratch, Str from, Str to)
{
    flushw(os);
    char *f = cpath(&scratch, from);
    char *t = cpath(&scratch, to);
    return rename(f, t) ? coreerr(errno) : 0;
}

// The directory plt_tempfile makes its files in: TMPDIR (the run's, in
// the library) if it can, else a system one
static Str tempdir(Os *os)
{
    Str dir = {};
    char *t;
    if constexpr (LIBMODE) {
        t = lib_tmpdir(os);
    } else {
        t = getenv("TMPDIR");
    }
    // As bash's sh_get_tmpdir: TMPDIR only if a directory it can write,
    // else the first of the system's that is
    char const *sys[] = {"/tmp", "/var/tmp", "/usr/tmp"};
    for (i32 i = -1; i < countof(sys); i++) {
        char const *d = i<0 ? t : sys[i];
        struct stat st;
        if (d && *d && !stat(d, &st) && S_ISDIR(st.st_mode) && !access(d, W_OK|X_OK)) {
            dir.data = (u8 *)d;
            for (; d[dir.len]; dir.len++) {}
            return dir;
        }
    }
    return Str(".");
}

static Opened plt_tempfile(Os *os, Arena scratch)
{
    flushw(os);
    // As bash: created in TMPDIR (or /tmp, see tempdir), then unlinked,
    // readable only by the user
    Str dir = tempdir(os);
    static u32 counter;
    Opened r = {};
    for (i32 try_ = 0; try_ < 100; try_++) {
        Buf b(&scratch, dir.len+64);
        print(&b, dir);
        print(&b, "/sh-thd.");
        print(&b, (i64)getpid());
        print(&b, ".");
        print(&b, (i64)(__atomic_add_fetch(&counter, 1u, __ATOMIC_RELAXED) ^ (u32)plt_now(os)));
        char *path = cpath(&scratch, finish(&b));
        i32 fd = open(path, O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC, 0600);
        if (fd < 0 && errno==EEXIST) continue;
        if (fd < 0) {
            r.h   = BADHANDLE;
            r.err = coreerr(errno);
            return r;
        }
        unlink(path);
        fchmod(fd, 0400);
        fd = highfd(fd);
        r.h   = fd<0 ? BADHANDLE : (Handle)fd;
        r.err = fd<0 ? coreerr(errno) : 0;
        return r;
    }
    r.h   = BADHANDLE;
    r.err = E_EXIST;
    return r;
}

// Not close's own error: bash ignores it too, its output already
// written by then.
static i32 plt_close(Os *os, Handle h)
{
    flushw(os);
    if (h == BADHANDLE) return 0;
    i32 fd = (i32)h;
    if (fd>=0 && fd<countof(os->wkind)) os->wkind[fd] = 0;  // the number may come back
    os->readyn = 0;  // likewise
    i32 err = wfailed(os, fd);
    close((i32)h);
    return err;
}

static Handle plt_dup(Os *os, Handle h)
{
    flushw(os);
    i32 fd = fcntl((i32)h, F_DUPFD_CLOEXEC, 10);
    if (fd < 0) fd = fcntl((i32)h, F_DUPFD_CLOEXEC, 3);  // as highfd
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

static OsPipe plt_pipe(Os *os)
{
    flushw(os);
    OsPipe r = {};
    i32 fds[2];
    if (cloexecpipe(fds)) {
        r.err = coreerr(errno);
        r.r = r.w = BADHANDLE;
        return r;
    }
    fds[0] = highfd(fds[0]);
    if (fds[0] < 0) {
        r.err = coreerr(errno);
        close(fds[1]);
        r.r = r.w = BADHANDLE;
        return r;
    }
    fds[1] = highfd(fds[1]);
    if (fds[1] < 0) {
        r.err = coreerr(errno);
        close(fds[0]);
        r.r = r.w = BADHANDLE;
        return r;
    }
    r.r = (Handle)fds[0];
    r.w = (Handle)fds[1];
    return r;
}

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
    stattimes(&r, s);
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
        r.err = coreerr(errno);
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
        r.err = coreerr(errno);
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

// The terminal as it was before os_tty changed it, and bracketed paste
// off, for an end of the process that skips TTY_RESTORE. Called from
// a signal handler too.
static void ttyabandon(Os *os)
{
    if (!os->ttyraw) return;
    os->ttyraw = 0;
    if (os->ttypaste && write(os->ttyout, "\x1b[?2004l", 8)) {}
    tcsetattr(os->ttyin, TCSANOW, &os->ttysave);
}

// While the terminal's mode is changed, or once output has waited in
// wbuf, the signals whose default action would end the process are
// caught instead, to restore the terminal and write out the output
// first, then die of them as readline does.
static int const ttydeadly[] = {
    SIGHUP, SIGINT, SIGQUIT, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2, SIGXCPU,
    SIGXFSZ, SIGVTALRM,
};

static void ondeadly(int sig)
{
    ttyabandon(sigos);
    flushw(sigos);
    struct sigaction sa = {};
    sa.sa_handler = SIG_DFL;
    sigaction(sig, &sa, 0);
    raise(sig);  // delivered on return
}

static b32 ttyisdeadly(int sig)
{
    for (iz i = 0; i < countof(ttydeadly); i++) {
        if (ttydeadly[i] == sig) return 1;
    }
    return 0;
}

// Catch the deadly signals left at their default action, or put back
// those caught so.
static void ttycatch(b32 on)
{
    for (iz i = 0; i < countof(ttydeadly); i++) {
        struct sigaction sa = {};
        if (sigaction(ttydeadly[i], 0, &sa)) continue;
        if (sa.sa_handler != (on ? SIG_DFL : ondeadly)) continue;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags   = 0;
        sa.sa_handler = on ? ondeadly : SIG_DFL;
        sigaction(ttydeadly[i], &sa, 0);
    }
}

// Output waits in wbuf from now on: a deadly signal writes it out, as
// bash's would be out already. Not in the library, whose host owns the
// signals; its output goes out at the end of the run.
static void wcatchsigs(Os *os)
{
    if constexpr (LIBMODE) return;
    os->wcatch = 1;
    ttycatch(1);
}

static Tty plt_tty(Os *os, Handle in, Handle out, i32 how)
{
    flushw(os);
    Tty     r = {};
    termios t;
    b32 paste = !!(how & TTY_PASTE);
    how &= ~TTY_PASTE;
    if (!isatty((i32)in) || !isatty((i32)out) || tcgetattr((i32)in, &t)) {
        r.err = E_NOTTY;
        return r;
    }
    if (how!=TTY_QUERY && how!=TTY_RESTORE) {
        os->ttypaste = paste && (how==TTY_RAW || how==TTY_RAWSIG);
    }
    if (how==TTY_RAW && os->ttyraw==TTY_RAW) {
        t = os->ttysave;  // already
    } else if (how!=TTY_QUERY && how!=TTY_RESTORE) {
        b32 first = !os->ttyraw;
        if (first) os->ttysave = t;
        os->ttyin  = (i32)in;
        os->ttyout = (i32)out;
        os->ttyraw = how;
        if (first) ttycatch(1);
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
        case TTY_RAWSIG:
            // readline's prepare_terminal_settings, with ^O bound
            m.c_lflag &= ~(tcflag_t)(ICANON|ECHO);
            m.c_lflag |= ISIG;
            m.c_iflag &= ~(tcflag_t)(ICRNL|INLCR);
            m.c_cc[VLNEXT]   = _POSIX_VDISABLE;
            m.c_cc[VDISCARD] = _POSIX_VDISABLE;
            #ifdef VDSUSP
            m.c_cc[VDSUSP]   = _POSIX_VDISABLE;
            #endif
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
        os->ttyraw   = 0;
        os->ttypaste = 0;
        tcsetattr((i32)in, TCSADRAIN, &os->ttysave);
        if (!os->wcatch) ttycatch(0);
        t = os->ttysave;
    } else if (os->ttyraw) {
        t = os->ttysave;
    }
    r.mode = os->ttyraw;
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
    *err = d ? 0 : coreerr(errno);
    if (!d) return 0;
    if constexpr (LIBMODE) lib_dir(os, d);  // closed if the run ends here
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
    if constexpr (LIBMODE) lib_dir(os, 0);
    closedir(d);
    return head;
}

// Close descriptors lo to hi, or lo up with hi -1, in one system call
// where the kernel has one (closerange).
static void closefds(i32 lo, i32 hi)
{
    if (closerange(lo, hi)) return;
    i32 max = 1023;
    if constexpr (LIBMODE) {
        // the host's may go as high as its limit
        struct rlimit rl;
        if (!getrlimit(RLIMIT_NOFILE, &rl)) {
            max = rl.rlim_cur>(1<<16) ? (1<<16) - 1 : (i32)rl.rlim_cur - 1;
        }
    }
    for (i32 fd = lo; fd <= (hi<0 ? max : hi); fd++) close(fd);
}

// Close every descriptor but the n in keep and also (negatives ignored).
static void closeexcept(i32 *keep, i32 n, i32 also = -1)
{
    for (i32 lo = 0;;) {
        i32 next = also>=lo ? also : -1;  // the lowest kept from lo up
        for (i32 i = 0; i < n; i++) {
            if (keep[i]>=lo && (next<0 || keep[i]<next)) next = keep[i];
        }
        if (next < 0) {
            closefds(lo, -1);
            return;
        }
        if (next > lo) closefds(lo, next-1);
        lo = next + 1;
    }
}

// A copy of fd out of the way of a spawn's n targets, by fcntl's cmd:
// from base (fdbase) up, or where a low `ulimit -n` leaves no room there
// (a target at 63, a process substitution's), the lowest free number
// that is no target. -1 with none.
static i32 fdaside(i32 fd, i32 base, i32 *dst, i32 n, i32 cmd)
{
    i32 r = fcntl(fd, cmd, base);
    for (i32 lo = 3; r<0 && lo<base;) {
        r = fcntl(fd, cmd, lo);
        if (r < 0) return r;
        b32 target = 0;
        for (i32 i = 0; i < n; i++) target |= dst[i]==r;
        if (!target) return r;
        close(r);
        lo = r + 1;
        r  = -1;
    }
    return r;
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
    i32      cwdfd;    // its pin, or -1 (plt_cwd)
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

// With a fork: the child has its own memory, and reports a failed exec
// through a close-on-exec pipe, which a successful exec closes. For
// what posix_spawn cannot set up (trispawn).
static i32 forkspawn(SpawnPlan *p, pid_t *out)
{
    i32 sync[2];
    if (cloexecpipe(sync)) return errno;
    for (i32 i = 0; i < 2; i++) sync[i] = highfd(sync[i]);
    if (sync[0]<0 || sync[1]<0) {
        if (sync[0] >= 0) close(sync[0]);
        if (sync[1] >= 0) close(sync[1]);
        return EMFILE;
    }
    pid_t pid = fork();
    if (!pid) {
        close(sync[0]);
        // The directory first, while its pin is open and not yet
        // replaced by a target of the map
        i32 err = enterdir(p->cwdfd, p->cwd);
        if (err) {
            if (write(sync[1], &err, sizeof(err))) {}
            _exit(127);
        }
        if constexpr (LIBMODE) setsid();  // away from the host's terminal
        joingroup(0, p->pgid, p->fg, p->tty);
        // Move the pipe and the sources out of the way first, then
        // install the map; one already at its own number stays, as no
        // other target is it. Where a low `ulimit -n` leaves no room it
        // fails as an exec would, the map not yet touched.
        i32 n    = p->nfds;
        i32 base = fdbase(p->dst, n);
        i32 w    = fdaside(sync[1], base, p->dst, n, F_DUPFD_CLOEXEC);
        for (i32 i = 0; i < n; i++) {
            if (p->src[i]>=0 && p->src[i]<base && p->src[i]!=p->dst[i]) {
                p->src[i] = fdaside(p->src[i], base, p->dst, n, F_DUPFD);
                if (p->src[i] < 0) {
                    err = errno;
                    if (write(w>=0 ? w : sync[1], &err, sizeof(err))) {}
                    _exit(127);
                }
            }
        }
        closeexcept(p->src, n, w);
        for (i32 i = 0; i < n; i++) {
            if (p->src[i] == p->dst[i]) {
                fcntl(p->src[i], F_SETFD, 0);  // as dup2 would leave it
            } else if (p->src[i] >= 0) {
                dup2(p->src[i], p->dst[i]);
            }
        }
        closeexcept(p->dst, n, w);
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
        sigset_t *mask = &p->mask;
        sigset_t  none;
        if constexpr (LIBMODE) {
            sigemptyset(&none);  // not the host thread's
            mask = &none;
        }
        sigprocmask(SIG_SETMASK, mask, 0);
        execve(p->path, p->argv, p->envp);
        err = errno ? errno : ENOEXEC;
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

// Where the C library's posix_spawn can change directory (spawnable),
// it makes the process: glibc runs the child on a stack of its own in
// the shell's memory, macOS has a system call, either way no copy of
// the shell's memory. Returns -1, having done nothing, where
// it cannot set up the plan:
// - limits, which no attribute sets;
// - a signal to ignore that the shell does not: the child may only
//   reset signals to their default, and ignoring it in the shell for
//   the while would throw away one pending;
// - the terminal for a foreground group, without an attribute for it
//   (the parent hands it over too, but only after the program may
//   already have run into the terminal).
static i32 trispawn(SpawnPlan *p, Arena scratch, pid_t *out)
{
    if (p->rlimits || !spawnable(p)) return -1;
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
        src[i] = fdaside(p->src[i], base, p->dst, p->nfds, F_DUPFD_CLOEXEC);
        if (src[i] < 0) err = errno;
    }

    posix_spawnattr_t at;
    posix_spawn_file_actions_t fa;
    if (!err) {
        posix_spawnattr_init(&at);
        posix_spawn_file_actions_init(&fa);
        short flags = POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF;
        sigset_t *mask = &p->mask;
        sigset_t  none;
        if constexpr (LIBMODE) {
            sigemptyset(&none);  // not the host thread's
            mask = &none;
            flags |= POSIX_SPAWN_SETSID;
        }
        posix_spawnattr_setsigmask(&at, mask);
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
        }
        spawndir(&fa, p);  // first, as forkspawn
        flags |= spawnpre(&at, &fa, p, src, base);
        for (i32 i = 0; i < p->nfds; i++) {
            if (src[i] >= 0) posix_spawn_file_actions_adddup2(&fa, src[i], p->dst[i]);
        }
        spawnpost(&fa, p, base);
        posix_spawnattr_setflags(&at, flags);

        err = posix_spawn(out, p->path, &fa, &at, p->argv, p->envp);
        posix_spawn_file_actions_destroy(&fa);
        posix_spawnattr_destroy(&at);
    }

    for (i32 i = 0; i < p->nfds; i++) {
        if (src[i]>=0 && src[i]!=p->src[i]) close(src[i]);
    }
    return err;
}

// The calling thread's signal mask. The executable has one thread; the
// library runs beside the host's, and macOS's sigprocmask, racing in
// several threads, leaves others' masks changed.
static void threadmask(i32 how, sigset_t *set, sigset_t *old)
{
    if constexpr (LIBMODE) {
        pthread_sigmask(how, set, old);
    } else {
        sigprocmask(how, set, old);
    }
}

static Spawned plt_spawn(Os *os, Arena scratch, Spawn *s)
{
    flushw(os);
    os->readyn = 0;  // the child may read what os_ready counted
    if (s->image.base) return forkimage(os, scratch, s);
    Spawned r = {};
    r.proc = BADHANDLE;
    SpawnPlan p = {};
    p.path  = cpath(&scratch, s->path);
    p.cwd   = cpath(&scratch, s->cwd);
    p.cwdfd = pinned(os, s->cwd);
    p.argv  = alloc<char *>(&scratch, s->argc+1);
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
    u64 changed = os->changed;
    if (!os->pipeign) changed |= corebit(SIGPIPE);
    changed |= corebit(SIGCHLD);
    p.ign     = s->sigign | os->entryign;
    p.dfl     = changed & ~p.ign;
    p.rlimits = s->rlimits;
    p.pgid    = s->pgid;
    p.fg      = s->fg;
    p.tty     = (i32)s->tty;
    if constexpr (LIBMODE) {
        p.dfl  = ~p.ign;  // whatever the host does with them
        p.pgid = 0;       // a session of its own instead
    }

    sigset_t all;
    sigfillset(&all);
    threadmask(SIG_SETMASK, &all, &p.mask);
    pid_t pid = -1;
    i32 e = trispawn(&p, scratch, &pid);
    if (e < 0) e = forkspawn(&p, &pid);
    if (!e) joingroup(pid, p.pgid, p.fg, p.tty);
    threadmask(SIG_SETMASK, &p.mask, 0);
    if (e) {
        r.err = coreerr(e);
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
static i32  fdaccess(i32);
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
    char *cwd   = cpath(&scratch, s->cwd);
    i32   cwdfd = pinned(os, s->cwd);
    i32  *src   = alloc<i32>(&scratch, s->nfds ? s->nfds : 1);
    i32  *dst = alloc<i32>(&scratch, s->nfds ? s->nfds : 1);
    for (iz i = 0; i < s->nfds; i++) {
        src[i] = (i32)s->fds[i].h;
        dst[i] = s->fds[i].fd;
    }
    i32 nfds = (i32)s->nfds;

    // Sources below a target move above them all first, here, where
    // a low `ulimit -n` leaving no room fails the spawn (the caller then
    // runs it otherwise) rather than the child losing the descriptor;
    // one already at its own number stays, as no other target is it
    i32 base = fdbase(dst, nfds);
    i32 err  = 0;
    for (i32 i = 0; i < nfds; i++) {
        if (src[i]<0 || src[i]>=base || src[i]==dst[i] || err) continue;
        src[i] = fdaside(src[i], base, dst, nfds, F_DUPFD_CLOEXEC);
        if (src[i] < 0) err = coreerr(errno);
    }
    if (err) {
        for (i32 i = 0; i < nfds; i++) {
            if (src[i]>=0 && src[i]!=(i32)s->fds[i].h) close(src[i]);
        }
        r.err = err;
        return r;
    }

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
        // The shell keeps its directory by name, so it runs on where it
        // cannot enter it, but its own pin is only good while the map
        // leaves it: it holds the directory again from inside
        b32 entered = !enterdir(cwdfd, cwd);
        closeexcept(src, nfds);
        for (i32 i = 0; i < nfds; i++) {
            if (src[i] == dst[i]) {
                fcntl(src[i], F_SETFD, 0);  // as dup2 would leave it
            } else if (src[i] >= 0) {
                dup2(src[i], dst[i]);
            }
        }
        closeexcept(dst, nfds);
        for (i32 i = 0; i < NPIN; i++) os->pins[i].fd = 0;
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
        if (os->ttyraw || os->wcatch) {
            ttycatch(0);  // the parent's terminal, not this process's
            os->ttyraw = 0;
            os->wcatch = 0;
        }
        // Its descriptors are new under the old numbers: what each is,
        // and a failure found writing to one, are the parent's (plt_spawn
        // wrote out the buffer)
        for (i32 i = 0; i < countof(os->wkind); i++) os->wkind[i] = 0;
        os->wlen   = 0;
        os->wrun   = -1;
        os->werrfd = -1;
        os->changed = s->sigign & ~os->entryign;
        os->pending = 0;
        os->peekfd[0] = os->peekfd[1] = 0;
        os->sigbits = 0;
        os->grpbits = 0;
        levelhead = 0;
        leveltail = 0;
        constinit static OsInfo info = {};  // in .bss, no guard
        Arena a = newarena(os, (iz)1<<24);
        Slice<InheritFd> in = inherited(&a);  // before the new self-pipe
        u64 entryign = os->entryign;
        initasync(os);
        os->entryign = entryign;  // as the parent had them
        // Pinned only now: taken before, inherited() would count the pin
        // as the program's and move it, leaving the record stale
        if (entered && s->cwd.len) pinfd(os, s->cwd, open(".", O_PIN));
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
        for (i32 i = 0; i < 3; i++) {
            info.stdflags[i] = fdaccess(i);
            info.std[i]      = info.stdflags[i] ? (Handle)i : BADHANDLE;  // closed
        }
        Fiber *f = os_fiber(os, &a, os->stack, imagefiber, &info);
        os_switch(os, f);
        _exit(127);  // never reached
    }
    i32 e = pid<0 ? coreerr(errno) : 0;
    if (pid > 0) joingroup(pid, s->pgid, s->fg, (i32)s->tty);
    sigprocmask(SIG_SETMASK, &old, 0);
    for (i32 i = 0; i < nfds; i++) {
        if (src[i]>=0 && src[i]!=(i32)s->fds[i].h) close(src[i]);
    }
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
    os->readyn = 0;  // as for plt_spawn
    i32 sync[2];
    if (pipe(sync)) return coreerr(errno);
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
        os->sigbits = 0;  // signals were for the original's pid
        os->grpbits = 0;
        levelhead = 0;
        leveltail = 0;
        sigprocmask(SIG_SETMASK, &old, 0);
        return 0;
    }
    i32 err = pid<0 ? coreerr(errno) : 0;
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
    i32 dot = open(".", O_PIN);
    if (dot>=0 && dot<base) {  // above the targets, as the saves
        i32 d = fcntl(dot, F_DUPFD_CLOEXEC, base);
        close(dot);
        dot = d;
    }
    if (!err && dot<0) err = errno;
    if (!err) err = enterdir(pinned(os, s->cwd), cwd);  // as os_spawn
    if (err) {
        for (iz i = 0; i < n; i++) {
            if (src[i] >= 0) close(src[i]);
            if (save[i] >= 0) close(save[i]);
        }
        if (dot >= 0) close(dot);
        return coreerr(err);
    }

    for (i32 fd = 0; fd < 1024; fd++) {
        b32 target = 0;
        for (iz i = 0; i < n; i++) target |= s->fds[i].fd == fd;
        if (!target) fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
    for (iz i = 0; i < n; i++) dup2(src[i], s->fds[i].fd);
    // Signal actions as os_spawn's child gets them
    struct sigaction old[NSIGNAL] = {};
    u64 touched = 0;
    u64 changed = os->changed;
    u64 ign     = s->sigign | os->entryign;
    if (!os->pipeign) changed |= corebit(SIGPIPE);
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
    return coreerr(err);
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
// A fault kills the shell as if it were never caught. One that may have
// been sent (faultsure) is noted as sent, the default action left for
// a fault of its own to meet when the instruction runs again, and the
// handler back once the shell takes the signal (plt_sigpending).
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
        if (faultsure(sig, info)) {
            raise(sig);  // delivered on return, if returning does not fault again
            return;
        }
        bitsor(&sigos->refault, (u64)1<<(sig-1));
    }
    if (sig==SIGPIPE && (!info || !info->si_pid || info->si_pid==getpid())) {
        return;
    }
    i32 saved = errno;
    b32 tty   = sig==SIGINT || sig==SIGQUIT || sig==SIGTSTP;
    b32 grp   = tty && fromterminal(info);
    bitsor(grp ? &sigos->grpbits : &sigos->sigbits, corebit(sig));
    u8 c = 0;
    if (write(sigos->wakefd[1], &c, 1)) {}
    errno = saved;
}

static i64 fromrlim(rlim_t v)
{
    return v==RLIM_INFINITY ? RL_INFINITY : (i64)v;
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
    return setrlimit(n, &l) ? coreerr(errno) : 0;
}

static i32 plt_kill(Os *os, i64 pid, i32 sig)
{
    flushw(os);
    if (pid<-(i64)0x7fffffff || pid>(i64)0x7fffffff) return E_SRCH;
    i32 hsig = hostsig(sig);
    if (sig && !hsig) return E_INVAL;  // one of Linux's this host lacks
    return kill((pid_t)pid, hsig) ? coreerr(errno) : 0;
}

static i32 plt_setpgid(Os *os, i64 pid, i64 pgid)
{
    flushw(os);
    return setpgid((pid_t)pid, (pid_t)pgid) ? coreerr(errno) : 0;
}

static i64 plt_tcpgrp(Os *os, Handle tty, i64 pgid)
{
    flushw(os);
    if (!pgid) {
        pid_t r = tcgetpgrp((i32)tty);
        return r<0 ? -coreerr(errno) : r;
    }
    sigset_t ttou, old;
    sigemptyset(&ttou);
    sigaddset(&ttou, SIGTTOU);
    sigprocmask(SIG_BLOCK, &ttou, &old);
    i32 r = tcsetpgrp((i32)tty, (pid_t)pgid);
    i32 e = coreerr(errno);
    sigprocmask(SIG_SETMASK, &old, 0);
    return r ? -e : pgid;
}

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

// Polled between commands: output that waited long enough goes out,
// though the shell runs on without another call
static u64 plt_sigpending(Os *os)
{
    if (os->wlen && monons()-os->wtime>=WDELAY) flushw(os);
    for (u64 b = bitstake(&os->refault); b; b &= b-1) {
        struct sigaction sa = {};  // as plt_sigmode caught it
        sigemptyset(&sa.sa_mask);
        sa.sa_flags     = SA_RESTART | SA_SIGINFO;
        sa.sa_sigaction = onsignal;
        sigaction(__builtin_ctzll(b) + 1, &sa, 0);
    }
    return bitstake(&os->sigbits);
}

static u64 plt_siggroup(Os *os)
{
    return bitstake(&os->grpbits);
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
    u64 refault = bitstake(&os->refault);  // not to be put back over this
    bitsor(&os->refault, refault & ~((u64)1<<(hsig-1)));
    if (mode==SIGM_DEFAULT && hsig==SIGPIPE) {
        quietpipe(os);  // its in-process subshells' broken pipes
        return;
    }
    struct sigaction sa = {};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    switch (mode) {
    case SIGM_DEFAULT:
        sa.sa_handler = (os->ttyraw || os->wcatch) && ttyisdeadly(hsig) ? ondeadly : SIG_DFL;
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
    if (os->wakefd[0]<0 || os->wakefd[1]<0) {
        if (os->wakefd[0] >= 0) close(os->wakefd[0]);
        if (os->wakefd[1] >= 0) close(os->wakefd[1]);
        os->wakefd[0] = os->wakefd[1] = -1;
        return;
    }
    sa = {};
    sa.sa_handler = onsigchld;
    sa.sa_flags   = SA_RESTART;  // stops too, for job control
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, 0);
    initlevel();
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
        io->err = n<0 ? coreerr(errno) : 0;
    } return 1;
    case IO_WRITE: {
        if (!revents) return 0;
        iz len = io->len<PIPE_BUF ? io->len : PIPE_BUF;
        iz n;
        do {
            n = write((i32)io->h, io->buf, (uz)len);
        } while (n<0 && errno==EINTR);
        io->n   = n<0 ? 0 : n;
        io->err = n<0 ? coreerr(errno) : 0;
    } return 1;
    case IO_PROC:
    case IO_PROCSTOP: {
        i32   status = 0;
        i32   flags  = WNOHANG | (io->op==IO_PROCSTOP ? WUNTRACED|WCONTINUED : 0);
        pid_t r;
        struct rusage ru = {};  // the library's os_cputime
        do {
            if constexpr (LIBMODE) {
                r = lib_wait4((pid_t)io->h, &status, flags & ~WNOHANG, &ru);
            } else {
                r = wait4((pid_t)io->h, &status, flags, &ru);
            }
        } while (r<0 && errno==EINTR);
        if (!r) return 0;
        i32 st = r<0 ? 127 : waitstatus(status);
        if constexpr (LIBMODE) {
            if (r<0 || WIFEXITED(status) || WIFSIGNALED(status)) {
                lib_reaped((pid_t)io->h, st, r<0 ? 0 : &ru);
            }
        }
        io->n   = st;
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

// Room for n records for poll, one for every pending operation however
// many: pollfix, or past that a mapping, kept for later waits.
static struct pollfd *pollroom(Os *os, iz n)
{
    if (n <= countof(os->pollfix)) return os->pollfix;
    if (n > os->pollcap) {
        iz cap = os->pollcap ? os->pollcap : countof(os->pollfix);
        for (; cap < n; cap *= 2) {}
        uz size = (uz)cap * sizeof(struct pollfd);
        void *p = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
        if (p == MAP_FAILED) os_oom();
        if (os->pollv) munmap(os->pollv, (uz)os->pollcap*sizeof(struct pollfd));
        os->pollv   = (struct pollfd *)p;
        os->pollcap = cap;
    }
    return os->pollv;
}

static void plt_submit(Os *os, Io *io)
{
    flushw(os);
    if (io->op == IO_READ) os->readyn = 0;  // its read is not plt_read's
    io->done = 0;
    io->next = 0;
    if (!isproc(io) && (i32)io->h<0) {
        io->n    = 0;  // poll would pass over it for ever
        io->err  = E_BADF;
        io->done = 1;
        return;
    }
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
    i32 backoff = 1;  // named FIFOs: 1 to 64 ms (fifoends)
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
        b32 named = 0;
        Io *ended = fifoends(os, &named);
        if (ended) return ended;
        iz max = 1;
        for (Io *io = os->pending; io; io = io->next) max += !isproc(io);
        struct pollfd *fds = pollroom(os, max);
        iz n = 0;
        fds[n].fd      = os->wakefd[0];
        fds[n].events  = POLLIN;
        fds[n++].revents = 0;
        for (Io *io = os->pending; io; io = io->next) {
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
        b32 bounded = named && (timeout<0 || timeout>backoff);
        if (bounded) {
            timeout = backoff;
            backoff = backoff<64 ? backoff*2 : 64;
        }
        i32 r = poll(fds, (nfds_t)n, timeout);
        if (r<0 && errno!=EINTR) return 0;
        if (fds[0].revents) {
            u8 buf[64];
            while (read(os->wakefd[0], buf, sizeof(buf)) > 0) {}
        }
        if (os->sigbits || os->grpbits || levelhead!=leveltail) return 0;  // the core first
        iz k = 1;
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
        if (bounded) {
            if (deadline>0 && plt_now(os)>=deadline) return 0;
            continue;  // only the bound passed
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

static i64 plt_monotime(Os *)
{
    return monons();
}

static i64 tvns(struct timeval t)
{
    return (i64)t.tv_sec*1000000000 + (i64)t.tv_usec*1000;
}

static CpuTimes plt_cputime(Os *)
{
    CpuTimes c = {};
    struct rusage u = {};
    if (!getrusage(RUSAGE_SELF, &u)) {
        c.user = tvns(u.ru_utime);
        c.sys  = tvns(u.ru_stime);
    }
    u = {};
    if (!getrusage(RUSAGE_CHILDREN, &u)) {
        c.cuser = tvns(u.ru_utime);
        c.csys  = tvns(u.ru_stime);
    }
    return c;
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

// getpwuid's name, as bash takes it: macOS keeps its accounts out of
// /etc/passwd, and Linux may keep some elsewhere (NSS)
static UserName plt_username(Os *, Arena *a)
{
    struct passwd pw, *p = 0;
    for (iz cap = 1<<10; cap <= 1<<20; cap *= 2) {
        char *buf = (char *)allocbytes(a, cap);
        if (getpwuid_r(getuid(), &pw, buf, (size_t)cap, &p) != ERANGE) break;
    }
    if (!p || !p->pw_name) return {{}, E_NOENT};
    return {fromcstr(p->pw_name), 0};
}

// FIONREAD answers for pipes, FIFOs, sockets and regular files on Linux
// and macOS alike. The count stands until a read of another descriptor
// (perhaps the same pipe), a close (the number may come back) or a
// spawn. Not for a terminal: its INT, QUIT or TSTP discards its queue,
// and a read counted on would then block.
static iz plt_ready(Os *os, Handle h)
{
    i32 fd = (i32)h;
    if (os->readyn>0 && os->readyfd==fd) return os->readyn;
    i32 avail = 0;
    if (ioctl(fd, FIONREAD, &avail) || avail<0) return -1;
    if (avail && isatty(fd)) return -1;
    os->readyfd = fd;
    os->readyn  = avail;
    return avail;
}

// The platform table (api.cpp, OsVtable)
static constexpr OsVtable pltvtable = {
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
    .cwd        = plt_cwd,
    .getcwd     = plt_getcwd,
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
    .cputime    = plt_cputime,
    .entropy    = plt_entropy,
    .clipget    = plt_clipget,
    .clipset    = plt_clipset,
    .localtz    = plt_localtz,
    .tasktag    = plt_tasktag,
    .racy       = plt_racy,
    .writeerr   = plt_writeerr,
    .username   = plt_username,
    .ready      = plt_ready,
    .monotime   = plt_monotime,
};


static byte *fiberstack(Os *os, iz size)
{
    if constexpr (LIBMODE) return lib_fiberstack(os, size);
    void *p = mmap(0, (uz)size, PROT_READ|PROT_WRITE,
                   MAP_PRIVATE|MAP_ANON|MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        os_oom();
    }
    mprotect(p, 1<<12, PROT_NONE);  // guard page
    return (byte *)p;
}

static void fiberunstack(Os *os, byte *p, iz size)
{
    if constexpr (LIBMODE) {
        lib_fiberunstack(os, p, size);
        return;
    }
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
// gives. The os and vendor are the OS's (machos), and the os may carry
// the kernel release, as config.guess appends it (darwin24.4.0).
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
    Str os = machos(&vendor, &release);

    Str r = concat3(a, cpu, "-", vendor);
    r = concat3(a, r, "-", os);
    struct utsname u;
    if (release && !uname(&u)) {
        r = concat(a, r, cut(fromcstr(u.release), '-').head);  // 14.1-RELEASE
    }
    return r;
}

// How a descriptor is open: OF_READ, OF_WRITE, or 0 if it is not.
static i32 fdaccess(i32 fd)
{
    i32 fl = fcntl(fd, F_GETFL);
    if (fl < 0) return 0;
    return (fl&O_ACCMODE)==O_RDONLY ? OF_READ :
           (fl&O_ACCMODE)==O_WRONLY ? OF_WRITE : OF_READ|OF_WRITE;
}

static Slice<InheritFd> pushinherit(Arena *a, Slice<InheritFd> r, i32 fd)
{
    i32 flags = fdaccess(fd);
    if (!flags) return r;
    InheritFd e = {};
    e.fd    = fd;
    e.flags = flags;
    return push(a, r, e);
}

// The descriptors above 2 the shell inherited, found before the shell
// opens any (openfds), each moved out of the way as the shell's own: at
// 10 and up where it can be (highfd), and close-on-exec.
static Slice<InheritFd> inherited(Arena *a)
{
    Slice<InheritFd> r = {};
    struct rlimit rl;
    i32 max = FDMAX;
    if (!getrlimit(RLIMIT_NOFILE, &rl) && rl.rlim_cur<(rlim_t)max) {
        max = (i32)rl.rlim_cur;
    }
    if (max <= 3) return r;
    r = openfds(a, max);
    // All found first: a duplicate may land on a number not yet seen
    iz n = 0;
    for (iz i = 0; i < r.len; i++) {
        i32 h = highfd(r[i].fd);
        if (h < 0) continue;
        r[n] = r[i];
        r[n++].h = (Handle)h;
    }
    r.len = n;
    return r;
}


static void exitwith(i32 status)
{
    if (sigos) flushw(sigos);
    if (sigos) ttyabandon(sigos);
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
