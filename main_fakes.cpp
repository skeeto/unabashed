// Fake programs as a Linux multi-call binary, for the oracle container
// $ g++ -std=c++20 -O2 -o fakes main_fakes.cpp
//
// Runs the fake named by the final component of argv[0], so each program
// name is a symbolic link to this binary. "fakes NAME ARGS..." also works,
// and "fakes --list" prints the program names, one per line.
//
// This is free and unencumbered software released into the public domain.
#include <dirent.h>
#include <errno.h>
#include <stdio.h>     // rename
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "base.cpp"
#include "api.cpp"
#include "fakes.cpp"

extern char **environ;

struct Os {
    Mem mem;
};

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
    Str msg = "fakes: out of memory\n";
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

// A null-terminated copy for a system call.
static char *cstr(Arena *a, Str s)
{
    char *r = (char *)allocbytes(a, s.len+1);
    copybytes(r, s.data, s.len);
    r[s.len] = 0;
    return r;
}

static Str fromcstr(char *s)
{
    Str r = {(u8 *)s, 0};
    for (; r.data[r.len]; r.len++) {}
    return r;
}


// Platform API

static b32 os_write(Os *, Handle h, u8 *buf, iz len)
{
    return writeall((i32)h, buf, len);
}

static iz os_read(Os *, Handle h, u8 *buf, iz len)
{
    for (;;) {
        iz n = read((i32)h, buf, (uz)len);
        if (n<0 && errno==EINTR) continue;
        return n;
    }
}

static Opened os_open(Os *, Arena scratch, Str path, i32 flags, i32 mode)
{
    Opened r = {};
    i32 oflags = O_CLOEXEC;
    if ((flags&OF_READ) && (flags&OF_WRITE)) {
        oflags |= O_RDWR;
    } else if (flags & OF_WRITE) {
        oflags |= O_WRONLY;
    } else {
        oflags |= O_RDONLY;
    }
    if (flags & OF_CREATE) oflags |= O_CREAT;
    if (flags & OF_TRUNC)  oflags |= O_TRUNC;
    if (flags & OF_APPEND) oflags |= O_APPEND;
    if (flags & OF_EXCL)   oflags |= O_EXCL;
    i32 fd = open(cstr(&scratch, path), oflags, mode);
    r.h   = fd<0 ? BADHANDLE : (Handle)fd;
    r.err = fd<0 ? errno : 0;
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

static OsPipe os_pipe(Os *)
{
    OsPipe r = {};
    i32 fds[2];
    if (pipe2(fds, O_CLOEXEC)) {
        r.r = r.w = BADHANDLE;
        r.err = errno;
        return r;
    }
    r.r = (Handle)fds[0];
    r.w = (Handle)fds[1];
    return r;
}

static i64 os_seek(Os *, Handle h, i64 off, i32 whence)
{
    i32 w = whence==0 ? SEEK_SET : whence==1 ? SEEK_CUR : SEEK_END;
    return lseek((i32)h, off, w);
}

static Stat fromstat(struct stat *st)
{
    Stat r = {};
    switch (st->st_mode & S_IFMT) {
    case S_IFREG:  r.type = FT_FILE;  break;
    case S_IFDIR:  r.type = FT_DIR;   break;
    case S_IFLNK:  r.type = FT_LINK;  break;
    case S_IFIFO:  r.type = FT_FIFO;  break;
    case S_IFCHR:  r.type = FT_CHAR;  break;
    case S_IFBLK:  r.type = FT_BLOCK; break;
    case S_IFSOCK: r.type = FT_SOCK;  break;
    }
    r.mode  = (i32)(st->st_mode & 07777);
    r.nlink = (i32)st->st_nlink;
    r.size  = st->st_size;
    r.mtime = (i64)st->st_mtim.tv_sec*1000000000 + st->st_mtim.tv_nsec;
    r.atime = (i64)st->st_atim.tv_sec*1000000000 + st->st_atim.tv_nsec;
    r.dev   = (i64)st->st_dev;
    r.ino   = (i64)st->st_ino;
    r.uid   = st->st_uid;
    r.gid   = st->st_gid;
    return r;
}

static Stat os_stat(Os *, Arena scratch, Str path, b32 follow)
{
    struct stat st;
    char *p = cstr(&scratch, path);
    if (follow ? stat(p, &st) : lstat(p, &st)) {
        Stat r = {};
        r.err = errno;
        return r;
    }
    return fromstat(&st);
}

static Stat os_fstat(Os *, Handle h)
{
    struct stat st;
    if (fstat((i32)h, &st)) {
        Stat r = {};
        r.err = errno;
        return r;
    }
    Stat r = fromstat(&st);
    if (r.type == FT_FIFO) r.type = FT_PIPE;
    return r;
}

static Dirent *os_list(Os *, Arena *a, Str dir, i32 *err)
{
    Arena t = *a;
    DIR *d = opendir(cstr(&t, dir));
    *err = 0;
    if (!d) {
        *err = errno;
        return 0;
    }
    Dirent  *head = 0;
    Dirent **tail = &head;
    for (struct dirent *e; (e = readdir(d));) {
        Str name = fromcstr(e->d_name);
        if (name=="." || name=="..") continue;
        Dirent *n = alloc<Dirent>(a);
        n->name = clone(a, name);
        switch (e->d_type) {
        case DT_REG:  n->type = FT_FILE;  break;
        case DT_DIR:  n->type = FT_DIR;   break;
        case DT_LNK:  n->type = FT_LINK;  break;
        case DT_FIFO: n->type = FT_FIFO;  break;
        case DT_CHR:  n->type = FT_CHAR;  break;
        case DT_BLK:  n->type = FT_BLOCK; break;
        case DT_SOCK: n->type = FT_SOCK;  break;
        }
        *tail = n;
        tail  = &n->next;
    }
    closedir(d);
    return head;
}

static Spawned os_spawn(Os *, Arena scratch, Spawn *s)
{
    Spawned r = {};
    r.proc = BADHANDLE;
    char  *path = cstr(&scratch, s->path);
    char  *cwd  = cstr(&scratch, s->cwd);
    char **argv = alloc<char *>(&scratch, s->argc+1);
    char **envp = alloc<char *>(&scratch, s->nenv+1);
    for (iz i = 0; i < s->argc; i++) argv[i] = cstr(&scratch, s->argv[i]);
    for (iz i = 0; i < s->nenv; i++) envp[i] = cstr(&scratch, s->env[i]);

    i32 errpipe[2];
    if (pipe2(errpipe, O_CLOEXEC)) {
        r.err = errno;
        return r;
    }
    pid_t pid = fork();
    if (pid < 0) {
        r.err = errno;
        close(errpipe[0]);
        close(errpipe[1]);
        return r;
    }
    if (!pid) {
        // Move every source above the targets, then place them
        i32 tmp[16];
        for (iz i = 0; i<s->nfds && i<16; i++) {
            tmp[i] = fcntl((i32)s->fds[i].h, F_DUPFD, 64);
        }
        b32 keep[10] = {};
        for (iz i = 0; i<s->nfds && i<16; i++) {
            if (tmp[i] < 0) continue;
            dup2(tmp[i], s->fds[i].fd);
            close(tmp[i]);
            if (s->fds[i].fd < 10) keep[s->fds[i].fd] = 1;
        }
        for (i32 fd = 0; fd < 10; fd++) {
            if (!keep[fd] && fd!=errpipe[1]) close(fd);
        }
        i32 e = 0;
        if (chdir(cwd)) {
            e = errno;
        } else {
            execve(path, argv, envp);
            e = errno;
        }
        writeall(errpipe[1], (u8 *)&e, sizeof(e));
        _exit(127);
    }
    close(errpipe[1]);
    i32 e = 0;
    iz  n = 0;
    for (;;) {
        n = read(errpipe[0], &e, sizeof(e));
        if (n<0 && errno==EINTR) continue;
        break;
    }
    close(errpipe[0]);
    if (n == sizeof(e)) {
        while (waitpid(pid, 0, 0)<0 && errno==EINTR) {}
        r.err = e;
        return r;
    }
    r.proc = (Handle)pid;
    return r;
}

static i32 os_waitproc(Os *, Handle proc)
{
    i32 status = 0;
    while (waitpid((pid_t)proc, &status, 0) < 0) {
        if (errno != EINTR) return 127;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 127;
}

static i64 os_now(Os *)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (i64)ts.tv_sec*1000000000 + ts.tv_nsec;
}


// Platform extensions for the fakes

static i32 os_mkdir(Os *, Arena scratch, Str path, i32 mode)
{
    return mkdir(cstr(&scratch, path), (mode_t)mode) ? errno : 0;
}

static i32 os_rmdir(Os *, Arena scratch, Str path)
{
    return rmdir(cstr(&scratch, path)) ? errno : 0;
}

static i32 os_unlink(Os *, Arena scratch, Str path)
{
    return unlink(cstr(&scratch, path)) ? errno : 0;
}

static i32 os_rename(Os *, Arena scratch, Str from, Str to)
{
    return rename(cstr(&scratch, from), cstr(&scratch, to)) ? errno : 0;
}

static i32 os_link(Os *, Arena scratch, Str from, Str to)
{
    return link(cstr(&scratch, from), cstr(&scratch, to)) ? errno : 0;
}

static i32 os_symlink(Os *, Arena scratch, Str target, Str path)
{
    return symlink(cstr(&scratch, target), cstr(&scratch, path)) ? errno : 0;
}

static i32 os_chmod(Os *, Arena scratch, Str path, i32 mode)
{
    return chmod(cstr(&scratch, path), (mode_t)mode) ? errno : 0;
}

static i32 os_utime(Os *, Arena scratch, Str path, i64 mtime)
{
    struct timespec ts[2];
    ts[0].tv_sec  = (time_t)(mtime / 1000000000);
    ts[0].tv_nsec = (long)(mtime % 1000000000);
    if (ts[0].tv_nsec < 0) {
        ts[0].tv_sec--;
        ts[0].tv_nsec += 1000000000;
    }
    ts[1] = ts[0];
    return utimensat(AT_FDCWD, cstr(&scratch, path), ts, 0) ? errno : 0;
}

static i32 os_mkfifo(Os *, Arena scratch, Str path, i32 mode)
{
    return mkfifo(cstr(&scratch, path), (mode_t)mode) ? errno : 0;
}

static Str os_readlink(Os *, Arena *a, Str path)
{
    char *p = cstr(a, path);
    for (iz cap = 256; cap < 1<<20; cap *= 2) {
        char *buf = (char *)allocbytes(a, cap);
        iz    n   = readlink(p, buf, (uz)cap);
        if (n < 0) return {};
        if (n < cap) return {(u8 *)buf, n};
    }
    return {};
}

static i32 os_umask(Os *)
{
    mode_t m = umask(0);
    umask(m);
    return (i32)m;
}

static OsIds os_ids(Os *)
{
    OsIds r = {};
    r.uid  = getuid();
    r.gid  = getgid();
    r.euid = geteuid();
    r.egid = getegid();
    return r;
}

static void os_sleep(Os *, i64 ns)
{
    struct timespec ts = {(time_t)(ns/1000000000), (long)(ns%1000000000)};
    while (nanosleep(&ts, &ts) && errno==EINTR) {}
}

static Str os_uname(Os *, Arena *a, i32 field)
{
    struct utsname u;
    if (uname(&u)) return "unknown";
    char *s = u.sysname;
    switch (field) {
    case UN_NODENAME: s = u.nodename; break;
    case UN_RELEASE:  s = u.release;  break;
    case UN_VERSION:  s = u.version;  break;
    case UN_MACHINE:  s = u.machine;  break;
    }
    return clone(a, fromcstr(s));
}


int main(int argc, char **argv)
{
    static Os os;
    iz    size = (iz)1 << (sizeof(void *)==8 ? 34 : 29);
    byte *base = os_reserve(&os, size);
    if (!base) os_oom();
    os.mem.os     = &os;
    os.mem.commit = base;
    os.mem.limit  = base + size;
    Arena a = {base, base, &os.mem};

    FakeCtx ctx = {};
    ctx.argc = argc;
    ctx.argv = alloc<Str>(&a, argc+1);
    for (i32 i = 0; i < argc; i++) ctx.argv[i] = fromcstr(argv[i]);
    for (; environ[ctx.nenv]; ctx.nenv++) {}
    ctx.env = alloc<Str>(&a, ctx.nenv);
    for (iz i = 0; i < ctx.nenv; i++) ctx.env[i] = fromcstr(environ[i]);
    for (iz cap = 256;; cap *= 2) {
        Arena t = a;
        char *buf = (char *)allocbytes(&t, cap);
        if (getcwd(buf, (uz)cap)) {
            ctx.cwd = fromcstr(buf);
            a = t;
            break;
        }
        if (errno != ERANGE) {
            ctx.cwd = "/";
            break;
        }
    }
    for (i32 i = 0; i < countof(ctx.fd); i++) {
        ctx.fd[i] = fcntl(i, F_GETFD)<0 ? BADHANDLE : (Handle)i;
    }

    Str name = argc ? fkbasename(ctx.argv[0]) : Str("fakes");
    if (name == "fakes") {
        if (argc>1 && ctx.argv[1]=="--list") {
            for (iz i = 0; i < countof(fakes); i++) {
                writeall(1, fakes[i].name.data, fakes[i].name.len);
                writeall(1, (u8 *)"\n", 1);
            }
            return 0;
        }
        if (argc < 2) {
            Str msg = "usage: fakes NAME [ARGS]...\n";
            writeall(2, msg.data, msg.len);
            return 2;
        }
        ctx.argv++;
        ctx.argc--;
        name = fkbasename(ctx.argv[0]);
    }
    ctx.scratch = a;
    for (iz i = 0; i < countof(fakes); i++) {
        if (fakes[i].name == name) return fakes[i].main(&os, &ctx);
    }
    Str msg = "fakes: unknown program\n";
    writeall(2, msg.data, msg.len);
    return 127;
}
