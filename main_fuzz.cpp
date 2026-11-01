// Fuzz test: runs each input as a -c script on a virtual OS
// $ afl-clang-fast++ -std=c++20 -g3 -fsanitize=undefined -fsanitize-trap -o fuzz main_fuzz.cpp
// $ afl-clang-fast++ -DFUZZ_PARSE ...    (print/reparse fixed point)
// $ afl-clang-fast++ -DFUZZ_CHAOS ...    (same output under every schedule)
// $ afl-clang-fast++ -DFUZZ_ISOLATE ...  (a subshell changes nothing)
// $ afl-fuzz -i fuzzin -o fuzzout ./fuzz
//
// Built with a plain compiler, it runs the files named on the command
// line once each instead, to replay what AFL found (tools/fuzz).
//
// The fuzzed shell can never reach the real file system or start a
// process: everything goes through the vos (vos.cpp), whose files and
// programs are in memory, and output is discarded.
//
// Each mode traps when its property fails, so that AFL records a crash:
// - run (default): no crash, no assertion, and the OOM paths work.
// - FUZZ_PARSE: printing and reparsing reaches a fixed point.
// - FUZZ_CHAOS: the script runs once with the plain schedule and then
//   under CHAOS_SEEDS chaos seeds. What each writer (a vos process and
//   a task within it, identified the same way under every seed) writes
//   to stdout and stderr, the exit status, and whether a deadlock was
//   reported must agree. A deadlock under one schedule only is a bug;
//   one under every schedule is a script that would hang under bash
//   too (it reads a terminal nobody types at, or a pipe whose writer
//   waits for it), which the vos reports instead of hanging. Runs are not compared when the script does
//   something whose outcome legitimately depends on the schedule: see
//   chaosexempt for the constructs, and vos.cpp's tracing (Vos.racy)
//   for what is caught as it happens (a write finding its reader gone,
//   readers sharing a pipe, the file system changing while tasks run
//   concurrently).
// - FUZZ_ISOLATE: in one shell, the state dump (isolatedump) printed
//   before "( eval S )" equals the one printed after it, except for
//   variables that change on their own (isolatenoise).
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unabashed.cpp"
#include "fibers.cpp"
#include "check.cpp"
#include "fakes.cpp"
#include "vos.cpp"

#ifdef __AFL_FUZZ_TESTCASE_LEN
#define FUZZ_REPLAY 0
__AFL_FUZZ_INIT();
#else
// Replay: one input per command line argument
#define FUZZ_REPLAY 1
static u8  *replaybuf;
static iz   replaylen;
#define __AFL_INIT()
#define __AFL_FUZZ_TESTCASE_BUF replaybuf
#define __AFL_FUZZ_TESTCASE_LEN replaylen
#define __AFL_LOOP(n) replaynext()
static char **replayargs;
static int    replayleft;
static int replaynext()
{
    while (replayleft > 0) {
        replayleft--;
        char *path = *replayargs++;
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        free(replaybuf);
        replaybuf = 0;
        replaylen = 0;
        for (iz cap = 0;;) {
            if (replaylen == cap) {
                cap = cap ? cap*2 : 4096;
                replaybuf = (u8 *)realloc(replaybuf, (uz)cap);
            }
            iz n = (iz)fread(replaybuf+replaylen, 1, (uz)(cap-replaylen), f);
            if (!n) break;
            replaylen += n;
        }
        fclose(f);
        fprintf(stderr, "%s\n", path);
        return 1;
    }
    return 0;
}
#endif

struct Os {
    Fibers fibers;
    byte  *heap;
    iz     heapcap;
    iz     heapused;
    iz     pinned;    // reserved by others while nested shells run
    i32    shells;    // nested shells running
    i32    owner;     // vos process of the innermost one
    u64    chaos;     // seed for the current run, 0 for none
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

// Out of memory ends the run, from whichever fiber it happens on
static void *oomjmp[5];

[[noreturn]] static void os_oom()
{
    #if FUZZ_REPLAY && _WIN32
    // A Win32 fiber cannot jump to another fiber's stack: stop here
    fprintf(stderr, "  out of memory\n");
    exit(0);
    #endif
    __builtin_longjmp(oomjmp, 1);
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

// Fiber stacks still allocated, freed after a run that ran out of
// memory and so never released them.
static byte *stacks[1024];
static iz    nstacks;

static byte *fiberstack(Os *, iz size)
{
    if (nstacks == countof(stacks)) os_oom();
    byte *p = (byte *)malloc((uz)size);
    if (!p) os_oom();
    stacks[nstacks++] = p;
    return p;
}

static void fiberunstack(Os *, byte *p, iz)
{
    for (iz i = 0; i < nstacks; i++) {
        if (stacks[i] == p) {
            stacks[i] = stacks[--nstacks];
            break;
        }
    }
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
    info.chaos   = os->chaos;
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

// Run the shell once on a fresh vos: bash -c SCRIPT bash ARG...
static i32 runonce(Os *os, Str script, Str *extra, iz nextra, u64 chaos)
{
    os->heapused = 0;
    os->pinned   = 0;
    os->shells   = 0;
    os->chaos    = chaos;
    Arena va = {};
    va.beg = os->heap + os->heapcap/2;
    va.end = os->heap + os->heapcap;
    vreset(&os->vos, va, fuzzfakes, nfuzzfakes);
    os->vos.chaos = chaos;
    os->vos.trace = 1;
    static Str env[] = {"LANG=C", "HOME=/home/oracle", "PATH=/usr/local/bin:/opt/fakes/bin"};
    Str args[8] = {"bash", "-c", script, "bash"};
    iz  nargs   = 4;
    for (iz i = 0; i<nextra && nargs<countof(args); i++) args[nargs++] = extra[i];
    OsInfo info = {};
    info.args    = args;
    info.nargs   = nargs;
    info.env     = env;
    info.nenv    = countof(env);
    info.cwd     = "/home/oracle";
    info.pid     = 2;
    info.std[1]  = 1;
    info.std[2]  = 2;
    info.reserve = (iz)1<<20;
    info.chaos   = chaos;
    i32 status = sh_main(os, &info);
    vfinish(os);
    return status;
}


// Replay builds say what became of each input.
static void note(char const *what)
{
    #if FUZZ_REPLAY
    fprintf(stderr, "  %s\n", what);
    #else
    (void)what;
    #endif
}


// Chaos

enum { CHAOS_SEEDS = 3 };

// Constructs whose results depend on timing or on which task runs
// first, in bash as here: background jobs and what observes them, the
// clock, timeouts, fifos, and nested shells' pids ($$ is a vos pid, and
// so is $! for programs, handed out in start order across tasks).
static b32 chaosexempt(Str s)
{
    static Str words[] = {
        "coproc", "wait", "jobs", "kill", "disown", "fg", "bg", "suspend",
        "sleep", "SECONDS", "EPOCH", "TMOUT", "-t", "time", "date",
        "mkfifo", "$$", "-nt", "-ot", "-N",
    };
    for (iz i = 0; i < countof(words); i++) {
        if (find(s, words[i]) >= 0) return 1;
    }
    // & alone: a background job (not &&, >&, <&, |&, &>), where an
    // escaped character is no operator (\|& is | then &)
    u8 prev = 0;  // the previous character if unescaped, else 0
    for (iz i = 0; i < s.len; i++) {
        if (s[i] == '\\') {
            i++;
            prev = 0;
            continue;
        }
        u8 next = i+1<s.len ? s[i+1] : 0;
        if (s[i]=='&' && prev!='&' && prev!='>' && prev!='<' && prev!='|' &&
                next!='&' && next!='>') {
            return 1;
        }
        // && is one operator: its second & is no |& or >& prefix
        prev = s[i]=='&' && prev=='&' ? 0 : s[i];
    }
    return 0;
}

struct ChaosRun {
    i32     status;
    b32     racy;
    b32     deadlock;
    i32     nwriters;
    VWriter writers[256];
};

static void chaoscapture(Os *os, ChaosRun *r, i32 status)
{
    Vos *v = &os->vos;
    r->status   = status;
    r->racy     = v->racy;
    r->deadlock = find(v->err, "deadlock")>=0;
    r->nwriters = v->nwriters;
    // Sorted by key, so that the order writers first wrote in is not
    // compared
    for (i32 i = 0; i < v->nwriters; i++) {
        VWriter w = v->writers[i];
        i32 j = i;
        for (; j>0 && r->writers[j-1].key>w.key; j--) {
            r->writers[j] = r->writers[j-1];
        }
        r->writers[j] = w;
    }
}

static b32 chaossame(ChaosRun *a, ChaosRun *b)
{
    if (a->status!=b->status || a->deadlock!=b->deadlock) return 0;
    if (a->nwriters != b->nwriters) return 0;
    for (i32 i = 0; i < a->nwriters; i++) {
        VWriter *x = a->writers + i;
        VWriter *y = b->writers + i;
        if (x->key!=y->key || x->hash[0]!=y->hash[0] || x->hash[1]!=y->hash[1]) {
            return 0;
        }
    }
    return 1;
}

static void chaosreport(char const *what, i32 seed, ChaosRun *r)
{
    #if FUZZ_REPLAY
    fprintf(stderr, "  %s, seed %d: status %d%s%s\n", what, seed, r->status,
            r->racy ? " racy" : "", r->deadlock ? " deadlock" : "");
    for (i32 i = 0; i < r->nwriters; i++) {
        fprintf(stderr, "    %016llx %016llx %016llx\n",
                (unsigned long long)r->writers[i].key,
                (unsigned long long)r->writers[i].hash[0],
                (unsigned long long)r->writers[i].hash[1]);
    }
    #else
    (void)what; (void)seed; (void)r;
    #endif
}

static void fuzzchaos(Os *os, Str src)
{
    if (chaosexempt(src)) return note("exempt");
    static ChaosRun first, other;
    first = {};
    chaoscapture(os, &first, runonce(os, src, 0, 0, 0));
    if (first.racy) return note("racy");
    for (i32 seed = 1; seed <= CHAOS_SEEDS; seed++) {
        other = {};
        u64 chaos = (u64)seed * 0x9e3779b97f4a7c15u;
        chaoscapture(os, &other, runonce(os, src, 0, 0, chaos));
        if (other.racy) return note("racy");
        if (!chaossame(&first, &other)) {
            chaosreport("plain", 0, &first);
            chaosreport("chaos", seed, &other);
            __builtin_trap();
        }
    }
    note("same");
}


// Isolate

// Everything a subshell might leak into its parent, printed by the
// parent before and after the subshell. pwd is left out: the subshell
// may remove the directory, which the parent then notices.
static Str isolatedump =
    "declare -p; declare -f; alias; trap -p; set +o; shopt -p; umask";

// Variables that change between two dumps on their own.
static Str isolatenoise[] = {
    "_", "RANDOM", "SRANDOM", "SECONDS", "EPOCHSECONDS", "EPOCHREALTIME",
    "LINENO", "BASH_COMMAND", "PIPESTATUS", "BASH_LINENO", "HISTCMD",
};

static b32 isolatenoisy(Str line)
{
    // declare -FLAGS NAME[=VALUE]
    if (!startswith(line, "declare -")) return 0;
    Cut c = cut(cuthead(line, 9), ' ');
    if (!c.ok) return 0;
    Str name = cut(c.tail, '=').head;
    for (iz i = 0; i < countof(isolatenoise); i++) {
        if (name == isolatenoise[i]) return 1;
    }
    return 0;
}

// Compare two dumps line by line, skipping the noisy variables.
static b32 isolatesame(Str a, Str b)
{
    for (;;) {
        Cut x = {}, y = {};
        do x = cut(a, '\n'), a = x.tail; while (x.ok && isolatenoisy(x.head));
        do y = cut(b, '\n'), b = y.tail; while (y.ok && isolatenoisy(y.head));
        if (!x.ok || !y.ok) return x.ok==y.ok && x.head==y.head;
        if (x.head != y.head) return 0;
    }
}

static void fuzzisolate(Os *os, Arena scratch, Str src)
{
    Str sep    = "\x01isolate\x01\n";
    Str dump   = isolatedump;
    Str script = concat(&scratch, dump, "\necho $'\\x01isolate\\x01'\n"
        "(eval \"$1\") </dev/null >/dev/null 2>&1\n"
        "echo $'\\x01isolate\\x01'\n");
    script = concat(&scratch, script, dump);
    runonce(os, script, &src, 1, 0);
    Str out = os->vos.out;
    iz  i   = find(out, sep);
    if (i < 0) return note("no dump");
    Str before = takehead(out, i);
    Str rest   = cuthead(out, i+sep.len);
    iz  j      = find(rest, sep);
    if (j < 0) return note("parent died");  // kill $$
    Str after = cuthead(rest, j+sep.len);
    if (!isolatesame(before, after)) {
        #if FUZZ_REPLAY
        fprintf(stderr, "--- before\n%.*s--- after\n%.*s",
                (int)before.len, (char *)before.data,
                (int)after.len, (char *)after.data);
        #endif
        __builtin_trap();
    }
    note("same");
}


int main(int argc, char **argv)
{
    #if FUZZ_REPLAY
    replayargs = argv + 1;
    replayleft = argc - 1;
    #else
    (void)argc; (void)argv;
    #endif
    __AFL_INIT();
    static Os os = {};
    // Small, so that pathological inputs hit OOM fast, yet room for the
    // arenas of a shell (four or more of info.reserve, plus a pool
    // arena per capture depth) and a few concurrent vprocs
    os.heapcap = (iz)1<<26;
    os.heap    = (byte *)malloc((uz)os.heapcap);
    os.vos.outcap      = (iz)1<<16;
    os.vos.out.data    = (u8 *)malloc((uz)os.vos.outcap);
    os.vos.err.data    = (u8 *)malloc((uz)os.vos.outcap);
    os.vos.merged.data = (u8 *)malloc((uz)os.vos.outcap);
    fuzzfakes[nfuzzfakes++] = {"bash", fake_bash};
    for (iz i = 0; i < countof(fakes) && nfuzzfakes < countof(fuzzfakes); i++) {
        fuzzfakes[nfuzzfakes++] = fakes[i];
    }
    static u8 scratchmem[1<<16];

    os_self(&os);  // the thread's own fiber, which each run starts on
    u8 *buf = __AFL_FUZZ_TESTCASE_BUF;
    u8 *src = 0;
    while (__AFL_LOOP(10000)) {
        buf = __AFL_FUZZ_TESTCASE_BUF;
        iz len = __AFL_FUZZ_TESTCASE_LEN;
        src = (u8 *)realloc(src, (uz)len);  // copy so overreads are caught
        if (len) memcpy(src, buf, (uz)len);

        os.heapused = 0;
        os.pinned   = 0;
        os.shells   = 0;
        os.fibers.current = &os.fibers.main;
        if (__builtin_setjmp(oomjmp)) {
            #if FUZZ_REPLAY
            fprintf(stderr, "  out of memory\n");
            #endif
            os.fibers.current = &os.fibers.main;
            while (nstacks) free(stacks[--nstacks]);
            continue;
        }
        #if FUZZ_PARSE
        // Printing and reparsing every command must reach a fixed point.
        Arena a = newarena(&os, (iz)1<<21);
        Fixed f = fixedpoint(&a, {src, len}, len && src[0]&1);
        if (f.status==FIX_REPARSE || f.status==FIX_DIFFER) {
            __builtin_trap();
        }
        #elif FUZZ_CHAOS
        fuzzchaos(&os, {src, len});
        #elif FUZZ_ISOLATE
        Arena scratch = {};
        scratch.beg = (byte *)scratchmem;
        scratch.end = scratch.beg + countof(scratchmem);
        if (len > countof(scratchmem)/2) continue;
        fuzzisolate(&os, scratch, {src, len});
        #else
        runonce(&os, {src, len}, 0, 0, 0);
        #endif
    }
    (void)scratchmem;
}
