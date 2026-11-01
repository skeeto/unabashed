// Fuzz test: runs each input as a -c script on a virtual OS
// $ afl-clang-fast++ -std=c++20 -g3 -fsanitize=undefined -fsanitize-trap -o fuzz main_fuzz.cpp
// $ afl-clang-fast++ -DFUZZ_PARSE ... (print/reparse fixed point)
// $ afl-fuzz -i fuzzin -o fuzzout ./fuzz
//
// The fuzzed shell can never reach the real file system or start a
// process: everything goes through the vos (vos.cpp), whose files and
// programs are in memory, and output is discarded.
//
// This is free and unencumbered software released into the public domain.
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unabashed.cpp"
#include "fibers.cpp"
#include "check.cpp"
#include "fakes.cpp"
#include "vos.cpp"

__AFL_FUZZ_INIT();

struct Os {
    Fibers fibers;
    byte  *heap;
    iz     heapcap;
    iz     heapused;
    iz     pinned;    // reserved by others while nested shells run
    i32    shells;    // nested shells running
    i32    owner;     // vos process of the innermost one
    Vos    vos;
};

static Fibers *osfibers(Os *os)
{
    return &os->fibers;
}

static Vos *osvos(Os *os)
{
    return &os->vos;
}

static jmp_buf oomjmp;

[[noreturn]] static void os_oom()
{
    longjmp(oomjmp, 1);
}

static byte *os_reserve(Os *os, iz len)
{
    iz pad = -os->heapused & 63;
    if (len > os->heapcap/2 - os->heapused - pad) {
        return 0;
    }
    byte *r = os->heap + os->heapused + pad;
    os->heapused += pad + len;
    if (os->shells && os->owner!=os->vos.cur) os->pinned = os->heapused;
    return r;
}

static b32 os_commit(Os *, byte *, iz)
{
    return 1;
}

static byte *fiberstack(Os *, iz size)
{
    return (byte *)malloc((uz)size);
}

static void fiberunstack(Os *, byte *p, iz)
{
    free(p);
}

// Nested shells run in-process with their memory released afterward,
// unless another process reserved memory meanwhile.
static i32 fake_bash(Os *os, FakeCtx *ctx)
{
    iz save = os->heapused;
    os->shells++;
    os->owner = os->vos.cur;
    OsInfo info = {};
    info.args    = ctx->argv;
    info.nargs   = ctx->argc;
    info.env     = ctx->env;
    info.nenv    = ctx->nenv;
    info.cwd     = ctx->cwd;
    info.pid     = os->vos.procs[os->vos.cur].pid;
    info.sigign  = os->vos.procs[os->vos.cur].sigign;
    info.ppid    = 2;
    for (i32 i = 0; i < 3; i++) info.std[i] = os_dup(os, ctx->fd[i]);
    info.ownstd = 1;
    info.reserve = (iz)1<<18;
    i32 status = sh_main(os, &info);
    os->owner = -1;  // until the next nested shell
    if (os->shells-- == 1 && os->pinned < save) os->heapused = save;
    return status;
}

static Fake fuzzfakes[256];
static iz   nfuzzfakes;

int main()
{
    __AFL_INIT();
    static Os os = {};
    os.heapcap = (iz)1<<23;  // small: pathological inputs hit OOM fast
    os.heap    = (byte *)malloc((uz)os.heapcap);
    os.vos.outcap      = (iz)1<<16;
    os.vos.out.data    = (u8 *)malloc((uz)os.vos.outcap);
    os.vos.err.data    = (u8 *)malloc((uz)os.vos.outcap);
    os.vos.merged.data = (u8 *)malloc((uz)os.vos.outcap);
    fuzzfakes[nfuzzfakes++] = {"bash", fake_bash};
    for (iz i = 0; i < countof(fakes) && nfuzzfakes < countof(fuzzfakes); i++) {
        fuzzfakes[nfuzzfakes++] = fakes[i];
    }

    u8 *buf = __AFL_FUZZ_TESTCASE_BUF;
    u8 *src = 0;
    while (__AFL_LOOP(10000)) {
        iz len = __AFL_FUZZ_TESTCASE_LEN;
        src = (u8 *)realloc(src, (uz)len);  // copy so overreads are caught
        memcpy(src, buf, (uz)len);

        os.heapused = 0;
        os.pinned   = 0;
        os.shells   = 0;
        os.fibers.current = &os.fibers.main;
        if (setjmp(oomjmp)) {
            continue;
        }
        #if FUZZ_PARSE
        // Printing and reparsing every command must reach a fixed point.
        Arena a = newarena(&os, (iz)1<<21);
        Fixed f = fixedpoint(&a, {src, len}, len && src[0]&1);
        if (f.status==FIX_REPARSE || f.status==FIX_DIFFER) {
            __builtin_trap();
        }
        #else
        Arena va = {};
        va.beg = os.heap + os.heapcap/2;
        va.end = os.heap + os.heapcap;
        vreset(&os.vos, va, fuzzfakes, nfuzzfakes);
        static Str env[] = {"LANG=C", "HOME=/home/oracle", "PATH=/usr/local/bin:/opt/fakes/bin"};
        Str args[] = {"bash", "-c", {src, len}};
        OsInfo info = {};
        info.args    = args;
        info.nargs   = countof(args);
        info.env     = env;
        info.nenv    = countof(env);
        info.cwd     = "/home/oracle";
        info.pid     = 2;
        info.std[1]  = 1;
        info.std[2]  = 2;
        info.reserve = (iz)1<<20;
        sh_main(&os, &info);
        vfinish(&os);
        #endif
    }
}
