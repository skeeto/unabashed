// POSIX platform layer (Linux, macOS)
// $ g++ -std=c++20 -O2 -o unabashed main_posix.cpp
// This is free and unencumbered software released into the public domain.
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include "unabashed.cpp"
#include "fibers.cpp"

extern char **environ;

struct Os {
    Fibers fibers;
};

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
    for (i32 i = 0; i < 3; i++) {
        info.std[i] = (Handle)i;
    }
    return sh_main(&os, &info);
}
