// Fuzz test: runs each input as a -c script on a virtual OS
// $ afl-clang-fast++ -std=c++20 -g3 -fsanitize=undefined -fsanitize-trap -o fuzz main_fuzz.cpp
// $ afl-fuzz -i fuzzin -o fuzzout ./fuzz
//
// The fuzzed shell can never reach the real file system or start a
// process: output is discarded and anything else traps.
//
// This is free and unencumbered software released into the public domain.
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unabashed.cpp"
#include "fibers.cpp"

__AFL_FUZZ_INIT();

struct Os {
    Fibers fibers;
    byte  *heap;
    iz     heapcap;
    iz     heapused;
};

static Fibers *osfibers(Os *os)
{
    return &os->fibers;
}

static jmp_buf oomjmp;

[[noreturn]] static void os_oom()
{
    longjmp(oomjmp, 1);
}

static byte *os_reserve(Os *os, iz len)
{
    iz pad = -os->heapused & 63;
    if (len > os->heapcap - os->heapused - pad) {
        return 0;
    }
    byte *r = os->heap + os->heapused + pad;
    os->heapused += pad + len;
    return r;
}

static b32 os_commit(Os *, byte *, iz)
{
    return 1;
}

static b32 os_write(Os *, Handle, u8 *, iz)
{
    return 1;
}

static iz os_read(Os *, Handle, u8 *, iz)
{
    return 0;
}

static byte *fiberstack(Os *, iz size)
{
    return (byte *)malloc((uz)size);
}

static void fiberunstack(Os *, byte *p, iz)
{
    free(p);
}

int main()
{
    __AFL_INIT();
    Os os = {};
    os.heapcap = (iz)1<<22;  // small: pathological inputs hit OOM fast
    os.heap    = (byte *)malloc((uz)os.heapcap);

    u8 *buf = __AFL_FUZZ_TESTCASE_BUF;
    u8 *src = 0;
    while (__AFL_LOOP(10000)) {
        iz len = __AFL_FUZZ_TESTCASE_LEN;
        src = (u8 *)realloc(src, (uz)len);  // copy so overreads are caught
        memcpy(src, buf, (uz)len);

        Str args[] = {"bash", "-c", {src, len}};
        OsInfo info = {};
        info.args    = args;
        info.nargs   = countof(args);
        info.std[1]  = 1;
        info.std[2]  = 2;
        info.reserve = (iz)1<<20;
        os.heapused  = 0;
        os.fibers    = {};
        if (!setjmp(oomjmp)) {
            sh_main(&os, &info);
        }
    }
}
