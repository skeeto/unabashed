// Tests: a virtual OS layer, so no test touches real files or processes
// $ g++ -std=c++20 -g3 -fsanitize=undefined -fsanitize-trap -o tests main_test.cpp
// On Windows, add -Wl,--stack,0x2000000 so that oracle runs, which nest
// shells on one stack, have room for bash's recursion depths.
//
// The shell runs repeatedly in one process. Memory comes from a fixed
// heap that is poisoned and reset between runs, and output is captured.
//
// This is free and unencumbered software released into the public domain.
#include <dirent.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#define EDIT_CHECK 1
#include "src/unabashed.cpp"
#include "src/fibers.cpp"
#include "src/check.cpp"
#include "src/fakes.cpp"
#include "src/vos.cpp"

// A nested shell's share of the heap, released when it exits if
// nothing reserved since then outlives it.
struct HeapMark {
    iz  save;
    i32 proc;     // its vos process
    b32 dead;
};

// A reservation of a nested shell's own, for reuse once the shell has
// exited if the heap could not be rewound past it (a job outliving its
// parent's next reservation): loops of jobs run in bounded memory.
struct HeapBlock {
    byte *base;
    iz    len;
    i32   proc;  // the vos process of the shell that reserved it
};

struct Os {
    Fibers   fibers;
    byte    *heap;     // backs every reservation
    iz       heapcap;
    iz       heapused;
    iz       pinned;   // reserved by others while nested shells run
    HeapMark marks[64];
    i32      nmarks;
    HeapBlock blocks[1024];  // of live nested shells
    i32       nblocks;
    HeapBlock spare[1024];   // of ended ones, below heapused
    i32       nspare;
    u64      chaos;    // seed for the next run, 0 for none
    Vos      vos;
};

static Fibers *osfibers(Os *os)
{
    return &os->fibers;
}

static Vos *osvos(Os *os)
{
    return &os->vos;
}

static byte *osheap(Os *os, iz *len)
{
    *len = os->heapcap;
    return os->heap;
}

static jmp_buf *oomjmp;  // test-only: where os_oom lands

[[noreturn]] static void os_oom()
{
    if (oomjmp) {
        longjmp(*oomjmp, 1);
    }
    fprintf(stderr, "tests: unexpected out of memory\n");
    __builtin_trap();
}

static iz reservelimit(Os *);

// Empty the heap: every reservation is released.
static void heapreset(Os *os)
{
    os->nblocks  = 0;
    os->nspare   = 0;
    os->heapused = 0;
    os->nmarks   = 0;
    os->pinned   = 0;
}

// The nested shell running now, as an index into marks, or -1.
static i32 heapowner(Os *os)
{
    for (i32 m = os->nmarks-1; m >= 0; m--) {
        if (!os->marks[m].dead && os->marks[m].proc==os->vos.cur) return m;
    }
    return -1;
}

static byte *os_reserve(Os *os, iz len)
{
    // An image's region the vos recycles itself (os_imagenew)
    b32 own = !os->vos.imaging && heapowner(os)>=0;
    for (i32 i = 0; own && i<os->nspare; i++) {
        if (os->spare[i].len == len) {
            byte *r = os->spare[i].base;
            os->spare[i] = os->spare[--os->nspare];
            if (os->nblocks < countof(os->blocks)) {
                os->blocks[os->nblocks++] = {r, len, os->vos.cur};
            }
            memset(r, 0x55, (uz)len);
            return r;
        }
    }
    iz pad = -os->heapused & 63;
    if (len > reservelimit(os) - os->heapused - pad) {
        return 0;
    }
    byte *r = os->heap + os->heapused + pad;
    os->heapused += pad + len;
    if (os->nmarks && (os->marks[os->nmarks-1].proc!=os->vos.cur ||
                       os->vos.imaging)) {
        os->pinned = os->heapused;
    }
    if (own && os->nblocks<countof(os->blocks)) {
        os->blocks[os->nblocks++] = {r, len, os->vos.cur};
    }
    memset(r, 0x55, (uz)len);  // expose reads of uninitialized memory
    return r;
}

static b32 os_commit(Os *os, byte *p, iz len)
{
    assert(p>=os->heap && p+len<=os->heap+os->heapcap);
    return 1;
}

// Poisoned, as the contents are gone.
static void os_decommit(Os *os, byte *p, iz len)
{
    assert(p>=os->heap && p+len<=os->heap+os->heapcap);
    memset(p, 0x55, (uz)len);
}

// Reservations come from the lower half of the heap; the vos owns the
// upper half.
static iz reservelimit(Os *os)
{
    return os->heapcap/2;
}

static byte *fiberstack(Os *, iz size)
{
    byte *p = (byte *)malloc((uz)size);
    if (!p) {
        os_oom();
    }
    return p;
}

static void fiberunstack(Os *, byte *p, iz)
{
    free(p);
}


// Harness

struct Test {
    Os  *os;
    i32  run;
    i32  failed;
};

struct Result {
    Str out;
    Str err;
    Str merged;
    i32 status;
};

static void printstr(FILE *f, Str s)
{
    if (s.len) fwrite(s.data, 1, (uz)s.len, f);  // data may be null
}

// Compare strings, printing both on mismatch.
static b32 expect(Test *t, Str name, Str got, Str want)
{
    t->run++;
    if (got == want) {
        return 1;
    }
    t->failed++;
    fprintf(stderr, "FAIL: ");
    printstr(stderr, name);
    fprintf(stderr, "\n  got:  [");
    printstr(stderr, got);
    fprintf(stderr, "]\n  want: [");
    printstr(stderr, want);
    fprintf(stderr, "]\n");
    return 0;
}

static b32 expectint(Test *t, Str name, i64 got, i64 want)
{
    t->run++;
    if (got == want) {
        return 1;
    }
    t->failed++;
    fprintf(stderr, "FAIL: ");
    printstr(stderr, name);
    fprintf(stderr, "\n  got:  %lld\n  want: %lld\n", got, want);
    return 0;
}

// The thread's stack, for oracle runs, which give each shell what is
// left of it rather than a fixed budget.
static byte *stackbase;
static b32   stackrest;
#if _WIN32
enum : iz { STACKSIZE = (iz)1<<25 };  // see the build command
#else
enum : iz { STACKSIZE = (iz)1<<23 };  // the usual main thread limit
#endif

static iz stackleft()
{
    byte here = 0;
    iz used = stackbase - &here;
    iz left = STACKSIZE - used - ((iz)1<<18);
    return stackrest && left>((iz)1<<19) ? left : 0;
}

// The vos program for bash itself: a nested shell in the same process,
// on the fiber of its vos process, whose memory is released when it
// returns unless something reserved meanwhile still lives.
static i32 fake_bash(Os *os, FakeCtx *ctx)
{
    i32 m = os->nmarks;
    if (m == countof(os->marks)) return 126;
    os->marks[m].save = os->heapused;
    os->marks[m].proc = os->vos.cur;
    os->marks[m].dead = 0;
    os->nmarks++;
    OsInfo info = {};
    info.stack   = stackrest ? vstack(os) - ((iz)1<<20) : 0;
    info.chaos   = os->chaos;
    info.args    = ctx->argv;
    info.nargs   = ctx->argc;
    info.env     = ctx->env;
    info.nenv    = ctx->nenv;
    info.cwd     = ctx->cwd;
    if (ctx->win) {
        // As main_win32.cpp's startup sees a Windows process
        info.windows = 1;
        info.exe     = ctx->exe;
        info.cmdline = ctx->cmdline;
        info.windir  = "C:/Windows";
        info.env     = vwinenv(&ctx->scratch, ctx->env, ctx->nenv);
    }
    info.pid     = osvos(os)->procs[osvos(os)->cur].pid;
    info.pgid    = osvos(os)->procs[osvos(os)->cur].pgid;
    info.sigign  = osvos(os)->procs[osvos(os)->cur].sigign;
    info.ppid    = osvos(os)->procs[osvos(os)->cur].ppid;
    info.uid     = info.euid = 1000;
    info.gid     = info.egid = 1000;
    info.host    = "oracle";
    // The process's descriptors become the shell's own, its only
    // references, as in a real process: closing one closes it (a pipe
    // a background job's redirection replaces is closed at once)
    for (i32 i = 0; i < 3; i++) {
        info.std[i] = ctx->fd[i];
        ctx->fd[i]  = BADHANDLE;
    }
    // The rest it was given, as a POSIX shell inherits them, or a
    // Windows one through lpReserved2 (os_spawn passes them only then)
    InheritFd inherit[countof(ctx->fd)];
    for (i32 fd = 3; fd < countof(ctx->fd); fd++) {
        if (ctx->fd[fd] == BADHANDLE) continue;
        InheritFd *e = inherit + info.ninherit++;
        e->fd    = fd;
        e->h     = ctx->fd[fd];
        e->flags = vaccess(osvos(os), ctx->fd[fd]);
        ctx->fd[fd] = BADHANDLE;
    }
    info.inherit = inherit;
    info.ownstd  = 1;
    info.reserve = (iz)1<<22;
    info.image   = ctx->image;
    i32 status = sh_main(os, &info);
    os->marks[m].dead = 1;
    while (os->nmarks && os->marks[os->nmarks-1].dead) {
        iz save = os->marks[--os->nmarks].save;
        os->heapused = save>os->pinned ? save : os->pinned;
    }
    // What the rewind did not release becomes spare; nothing spare may
    // lie past the heap's end
    byte *end = os->heap + os->heapused;
    for (i32 i = os->nblocks-1; i >= 0; i--) {
        HeapBlock b = os->blocks[i];
        if (b.proc != os->vos.cur) continue;
        os->blocks[i] = os->blocks[--os->nblocks];
        if (b.base<end && os->nspare<countof(os->spare)) {
            os->spare[os->nspare++] = b;
        }
    }
    for (i32 i = os->nspare-1; i >= 0; i--) {
        if (os->spare[i].base >= end) os->spare[i] = os->spare[--os->nspare];
    }
    return status;
}

static b32 images = 1;  // os_imagenew self-spawns (--fibers: not)

static Fake testfakes[256];
static iz   ntestfakes;

static void initfakes()
{
    ntestfakes = 0;
    testfakes[ntestfakes++] = {"bash", fake_bash};
    for (iz i = 0; i < countof(fakes) && ntestfakes < countof(testfakes); i++) {
        testfakes[ntestfakes++] = fakes[i];
    }
    for (iz i = 0; i < countof(vwinfakes) && ntestfakes < countof(testfakes); i++) {
        testfakes[ntestfakes++] = vwinfakes[i];
    }
}

// Personalities: the system a run's shell finds itself on.
enum : i32 {
    PERS_LINUX,   // the oracle container
    PERS_NATIVE,  // Windows, the shell as w64devkit's sh.exe
    PERS_MSYS,    // Windows, the shell as an MSYS2 install's bash.exe
    PERS_W64BASH, // Windows, the shell as w64devkit's bash.exe (no fstab)
};

static i32 personality(Str name)  // -1 if unknown
{
    if (name == "linux")  return PERS_LINUX;
    if (name == "native") return PERS_NATIVE;
    if (name == "msys")   return PERS_MSYS;
    if (name == "w64bash") return PERS_W64BASH;
    return -1;
}

static Str speclocaltz;  // a spec's "# localtz:" line: the vos's zone

// A fresh heap and vos for a run: the vos in the upper half of the
// heap, the shell below.
static void startrun(Os *os, b32 win, Str input)
{
    heapreset(os);
    Arena va = {};
    va.beg = os->heap + os->heapcap/2;
    va.end = os->heap + os->heapcap;
    vreset(&os->vos, va, testfakes, ntestfakes, win);
    os->vos.images = images;
    os->vos.in    = input;
    os->vos.chaos = os->chaos;
    os->vos.localtz = speclocaltz;
}

static OsInfo baseinfo(Os *os, Str *args, iz nargs)
{
    OsInfo info = {};
    info.args    = args;
    info.nargs   = nargs;
    info.pid     = 2;
    info.ppid    = 1;
    info.pgid    = 2;
    info.uid     = info.euid = 1000;
    info.gid     = info.egid = 1000;
    info.host    = "oracle";
    info.std[0]  = 0;
    info.std[1]  = 1;
    info.std[2]  = 2;
    info.reserve = (iz)1<<22;
    info.stack   = (iz)1<<20;
    info.chaos   = os->chaos;
    return info;
}

static Result finishrun(Os *os, OsInfo *info)
{
    Result r = {};
    r.status = sh_main(os, info) & 255;
    vfinish(os);
    r.out    = os->vos.out;
    r.err    = os->vos.err;
    r.merged = os->vos.merged;
    return r;
}

// The command line a native parent gives a program: MSVCRT quoting, or
// with cygwin Cygwin's, which an msys shell splits back to args.
static Str wincmdline(Arena *a, Str *args, iz nargs, b32 cygwin)
{
    Buf b(a);
    for (iz i = 0; i < nargs; i++) {
        if (!i) {
            quoteprogram(&b, args[i]);
            continue;
        }
        putbyte(&b, ' ');
        if (cygwin) {
            quotecygwin(&b, args[i]);
        } else {
            quotearg(&b, args[i]);
        }
    }
    return finish(&b);
}

// The Windows half of an OsInfo, as main_win32.cpp's startup fills it
// in (vwinenv renames Path), for a shell at exe with the given
// environment and working directory.
static void wininfo(Os *os, OsInfo *info, Str exe, Str *env, iz nenv, Str cwd,
                    b32 cygwin = 0)
{
    Arena *a = &os->vos.a;
    info->windows = 1;
    info->exe     = exe;
    info->windir  = "C:/Windows";
    info->cmdline = wincmdline(a, info->args, info->nargs, cygwin);
    info->env     = vwinenv(a, env, nenv);
    info->nenv    = nenv;
    info->cwd     = cwd;
}

// Run the shell with a fresh virtual OS state. The first argument is
// $0, conventionally "bash".
// A Windows personality may give the raw command line in place of the
// one quoted from args.
static Result runshell(Test *t, Str *args, iz nargs, Str input = {},
                       i32 pers = PERS_LINUX, Str cmdline = {})
{
    Os *os = t->os;
    startrun(os, pers!=PERS_LINUX, input);
    OsInfo info = baseinfo(os, args, nargs);
    if (pers == PERS_LINUX) {
        static Str env[] = {
            "LANG=C", "TZ=UTC", "HOME=/home/oracle",
            "PATH=/usr/local/bin:/opt/fakes/bin",
        };
        info.env  = env;
        info.nenv = countof(env);
        info.cwd  = "/home/oracle";
    } else {
        // As a Windows parent hands it over, sorted, with LANG and TZ
        // pinned as for the oracle
        static Str env[] = {
            "=C:=C:\\work",
            "ComSpec=C:\\Windows\\System32\\cmd.exe",
            "LANG=C",
            "Path=C:\\w64\\bin;C:\\Windows\\System32",
            "ProgramFiles(x86)=C:\\Program Files (x86)",
            "SystemRoot=C:\\Windows",
            "TEMP=C:\\Users\\oracle\\AppData\\Local\\Temp",
            "TMP=C:\\Users\\oracle\\AppData\\Local\\Temp",
            "TZ=UTC",
            "USERPROFILE=C:\\Users\\oracle",
        };
        static Str msysenv[countof(env)];
        Str *e = env;
        if (pers == PERS_MSYS) {
            for (iz i = 0; i < countof(env); i++) msysenv[i] = env[i];
            msysenv[3] = "Path=C:\\msys\\usr\\bin;C:\\w64\\bin;C:\\Windows\\System32";
            e = msysenv;
        }
        Str exe = pers==PERS_MSYS    ? Str("C:/msys/usr/bin/bash.exe")
                : pers==PERS_W64BASH ? Str("C:/w64/bin/bash.exe")
                :                      Str("C:/w64/bin/sh.exe");
        // a parent that quotes for Cygwin, so that a script given with
        // -c arrives as written in the msys shell's own split
        wininfo(os, &info, exe, e, countof(env), "C:/work", pers!=PERS_NATIVE);
        if (cmdline.data) info.cmdline = cmdline;
    }
    return finishrun(os, &info);
}

static Result runc(Test *t, Str script)
{
    Str args[] = {"bash", "-c", script};
    return runshell(t, args, countof(args));
}


// Tests

static void test_m0(Test *t)
{
    Result r = runc(t, "echo hi there; echo  x;exit 3");
    expect(t, "echo out", r.out, "hi there\nx\n");
    expect(t, "echo err", r.err, "");
    expectint(t, "exit status", r.status, 3);

    r = runc(t, "foo\necho $x; nope");
    expect(t, "not found", r.err,
        "bash: line 1: foo: command not found\n"
        "bash: line 2: nope: command not found\n");
    expectint(t, "not found status", r.status, 127);

    r = runc(t, "exit 300");
    expectint(t, "exit wraps", r.status, 44);

    Str args[] = {"bash", "-c"};
    r = runshell(t, args, countof(args));
    expect(t, "-c missing", r.err, "bash: -c: option requires an argument\n");
    expectint(t, "-c missing status", r.status, 2);
}

static void test_fibers(Test *t)
{
    Str args[] = {"bash", "--unabashed-selftest"};
    Result r = runshell(t, args, countof(args));
    expect(t, "fiber selftest", r.out, "fibers: ok\n");
    expectint(t, "fiber selftest status", r.status, 0);
}

// Path modes at startup: -O/+O with the hidden msyspath, and the msys
// shell's own split of its raw command line (globbing, quoting).
// The terminal model: wrapping with and without deferred wrap, wide
// characters at the margin, erasing, insert mode, scrolling.
static void test_vterm(Test *t)
{
    heapreset(t->os);
    Arena a = newarena(t->os, (iz)1<<20);
    struct {
        i32         rows, cols;
        b32         xenl;
        char const *in;
        char const *want;
    } cases[] = {
        {3, 5, 1, "abcde",                        "abcde\n@ 0,4+\n"},
        {3, 5, 1, "abcdef",                       "abcde\nf\n@ 1,1\n"},
        {3, 5, 1, "abcde\r\n",                    "abcde\n\n@ 1,0\n"},
        {3, 5, 1, "abcde\b",                      "abcde\n@ 0,3\n"},
        {3, 5, 0, "abcde",                        "abcde\n\n@ 1,0\n"},
        {3, 5, 1, "abcd\xe4\xb8\xad",             "abcd\n\xe4\xb8\xad\n@ 1,2\n"},
        {3, 5, 1, "hello\r\x1b[2Cx\x1b[K",        "hex\n@ 0,3\n"},
        {3, 5, 1, "hello\r\x1b[4hXY\x1b[4l",      "XYhel\n@ 0,2\n"},
        {3, 5, 1, "ab\x1b[1P",                    "ab\n@ 0,2\n"},
        {3, 5, 1, "ab\b\x1b[1P",                  "a\n@ 0,1\n"},
        {2, 5, 1, "a\r\nb\r\nc",                  "b\nc\n@ 1,1\n"},
        {3, 5, 1, "\x1b[7mab\x1b[27mc",           "abc\n~rr\n@ 0,3\n"},
        {3, 5, 1, "x\x1b]0;title\x07y\a",         "xy\n@ 0,2 bell 1\n"},
        {3, 9, 1, "a\tb",                         "a       b\n@ 0,8+\n"},
        {3, 5, 1, "e\xcc\x81x",                   "e\xcc\x81x\n@ 0,2\n"},
    };
    for (iz i = 0; i < countof(cases); i++) {
        Arena  tmp = a;
        VTerm *v   = vtnew(&tmp, cases[i].rows, cases[i].cols, cases[i].xenl);
        Str    in  = {(u8 *)cases[i].in, (iz)strlen(cases[i].in)};
        vtfeed(v, in.data, in.len);
        Str want = {(u8 *)cases[i].want, (iz)strlen(cases[i].want)};
        expect(t, in, vtdump(v, &tmp), want);
    }
}

// Console key events as xterm bytes (vtcon.cpp, conkey)
static void test_conkey(Test *t)
{
    enum { RA = CK_RALT, LA = CK_LALT, LC = CK_LCTRL, SH = CK_SHIFT };
    struct {
        b32         down;
        i32         vk, ch, state;
        char const *want;
        i32         len;  // for a NUL, else 0
    } cases[] = {
        {1, 'A',  'a',  0,     "a", 0},
        {1, 'A',  'A',  SH,    "A", 0},
        {1, 'A',  'a',  LA,    "\x1b" "a", 0},
        {1, 'A',  0x01, LC,    "\x01", 0},
        {1, 'A',  0x01, LC|LA, "\x1b\x01", 0},
        {1, 'Q',  '@',  RA|LC, "@", 0},            // AltGr: text
        {1, 0x26, 0,    0,     "\x1b[A", 0},
        {1, 0x25, 0,    LC,    "\x1b[1;5D", 0},
        {1, 0x27, 0,    LA,    "\x1b[1;3C", 0},
        {1, 0x24, 0,    SH,    "\x1b[1;2H", 0},
        {1, 0x23, 0,    0,     "\x1b[F", 0},
        {1, 0x2e, 0,    0,     "\x1b[3~", 0},
        {1, 0x2e, 0,    LC,    "\x1b[3;5~", 0},
        {1, 0x2d, 0,    0,     "\x1b[2~", 0},
        {1, 0x21, 0,    0,     "\x1b[5~", 0},
        {1, 0x22, 0,    0,     "\x1b[6~", 0},
        {1, 0x70, 0,    0,     "\x1bOP", 0},
        {1, 0x71, 0,    LC,    "\x1b[1;5Q", 0},
        {1, 0x74, 0,    0,     "\x1b[15~", 0},
        {1, 0x7b, 0,    SH,    "\x1b[24;2~", 0},
        {1, 0x08, 0x08, 0,     "\x7f", 0},
        {1, 0x08, 0x7f, LC,    "\x08", 0},
        {1, 0x08, 0x08, LA,    "\x1b\x7f", 0},
        {1, 0x09, '\t', 0,     "\t", 0},
        {1, 0x09, '\t', SH,    "\x1b[Z", 0},
        {1, 0x0d, '\r', 0,     "\r", 0},
        {1, 0x1b, 0x1b, 0,     "\x1b", 0},
        {1, 0x20, ' ',  LC,    "", 1},          // C-SPC: NUL
        {1, '2',  0,    LC|SH, "", 1},          // C-@
        {1, 0xbd, 0,    LC,    "\x1f", 0},
        {1, 0x10, 0,    SH,    "", 0},             // shift alone
        {0, 'A',  'a',  0,     "", 0},             // key up
        {0, 0x12, 0xe9, 0,     "\xc3\xa9", 0},     // Alt+numpad on release
        {1, 0xe7, 0x4e2d, 0,   "\xe4\xb8\xad", 0}, // VK_PACKET
    };
    for (iz i = 0; i < countof(cases); i++) {
        ConKey k = {};
        k.down  = cases[i].down;
        k.vk    = cases[i].vk;
        k.ch    = cases[i].ch;
        k.state = cases[i].state;
        u8  out[16];
        i32 hi = 0;
        i32 n  = conkey(k, &hi, out);
        Str want = {(u8 *)cases[i].want, (iz)strlen(cases[i].want)};
        if (cases[i].len) want = {(u8 *)"\0", cases[i].len};
        char name[32];
        snprintf(name, sizeof(name), "conkey %d", (int)i);
        expect(t, {(u8 *)name, (iz)strlen(name)}, {out, n}, want);
    }
    // A surrogate pair, one event each
    i32 hi = 0;
    u8  out[16];
    ConKey k = {};
    k.down = 1;
    k.vk   = 0xe7;
    k.ch   = 0xd83d;
    i32 n  = conkey(k, &hi, out);
    k.ch   = 0xde00;
    n     += conkey(k, &hi, out+n);
    expect(t, "conkey surrogates", {out, n}, "\xf0\x9f\x98\x80");
}

static void test_pathmodes(Test *t)
{
    Str on[] = {"sh", "-O", "msyspath", "-c", "echo $PATH; shopt -p msyspath"};
    Result r = runshell(t, on, countof(on), {}, PERS_NATIVE);
    expect(t, "-O msyspath", r.out,
        "/bin:/c/Windows/System32\nshopt -s msyspath\n");

    Str off[] = {"bash", "+O", "msyspath", "-c", "echo $PATH $PWD"};
    r = runshell(t, off, countof(off), {}, PERS_MSYS);
    expect(t, "+O msyspath", r.out,
        "C:/msys/usr/bin;C:/w64/bin;C:/Windows/System32 C:/work\n");

    Str quoted[] = {"bash", "-c", "echo \"$@\"", "x", "a\\\\b", "/etc/*", "C:\\x"};
    r = runshell(t, quoted, countof(quoted), {}, PERS_MSYS);
    expect(t, "msys args intact", r.out, "a\\\\b /etc/* C:\\x\n");

    Str raw[] = {"bash"};
    r = runshell(t, raw, countof(raw), {}, PERS_MSYS,
        "bash -c 'echo \"$@\"' x /etc/f* '/etc/f*' {a,b}c");
    expect(t, "msys glob", r.out, "/etc/fstab /etc/f* ac bc\n");
    expect(t, "msys glob err", r.err, "");
}

// skipplainbrace, where it decides, ends a ${...} where the lexer does:
// every string of up to five characters over an alphabet of the ones
// that matter to lexing, after "${".
static void test_plainbrace(Test *t)
{
    Os *os = t->os;
    heapreset(os);
    Arena perm = newarena(os, (iz)1<<24);
    static u8 alpha[] = "a{}(#:-| \n$'\\<\"`)";
    iz    nalpha  = countof(alpha) - 1;
    u8    buf[8]  = {'$', '{'};
    i32   decided = 0;
    i32   bad     = 0;
    for (i32 len = 0; len <= 5; len++) {
        iz total = 1;
        for (i32 k = 0; k < len; k++) total *= nalpha;
        for (iz n = 0; n < total; n++) {
            iz m = n;
            for (i32 k = 0; k < len; k++, m /= nalpha) buf[2+k] = alpha[m%nalpha];
            Str s = {buf, 2+len};
            iz  e = skipplainbrace(s, 0);
            if (e < 0) continue;
            decided++;
            Arena  a = perm;
            Lexer *L = newlexer(&a, {});
            if (skipexpansion(L, s, 0, 0)!=e || skipexpansion(L, s, 0, 1)!=e) {
                if (!bad++) fprintf(stderr, "plainbrace: [%.*s]\n", (int)s.len, s.data);
            }
        }
    }
    expectint(t, "plainbrace decides", decided > 10000, 1);
    expectint(t, "plainbrace agrees", bad, 0);
}

static void test_arena(Test *t)
{
    Os *os = t->os;
    heapreset(os);

    // Grows past the initial commit, then fails at the reservation.
    Arena a = newarena(os, (iz)1<<21);
    alloc<u8>(&a, (iz)1<<20);
    expectint(t, "arena grew", a.end - (byte *)a.mem > (iz)1<<20, 1);

    jmp_buf jb;
    i32 hit = 0;
    oomjmp = &jb;
    if (!setjmp(jb)) {
        alloc<u8>(&a, (iz)1<<20);
    } else {
        hit = 1;
    }
    oomjmp = 0;
    expectint(t, "arena oom", hit, 1);
}

// RANDOM's PCG64: pcg-c's published outputs for pcg64_srandom_r(42, 54),
// and the 64-bit halves against 128-bit integers for other seeds.
static void test_pcg(Test *t)
{
    Pcg g = pcgseed(42, 54);
    u64 want[] = {
        0x86b1da1d72062b68, 0x1304aa46c9853d39, 0xa3670e9e0dd50358,
        0xf9090e529a7dae00, 0xc85b9fd837996f2c, 0x606121f8e3919196,
    };
    i32 ok = 1;
    for (i32 i = 0; i < countof(want); i++) {
        ok &= pcgnext(&g) == want[i];
    }
    expectint(t, "pcg64 known answers", ok, 1);

    #if __SIZEOF_INT128__
    typedef unsigned __int128 u128;
    u128 mul = (u128)0x2360ed051fc65da4<<64 | 0x4385df649fccf645;
    ok = 1;
    for (u64 k = 1; k < 64; k++) {
        u64  seed   = k * 0x9e3779b97f4a7c15u;
        u64  stream = ~seed ^ k<<60;
        Pcg  p      = pcgseed(seed, stream);
        u128 inc    = (u128)stream<<1 | 1;
        u128 s      = 0;
        s = s*mul + inc;
        s += seed;
        s = s*mul + inc;
        for (i32 i = 0; i < 100; i++) {
            s = s*mul + inc;
            u64 x = (u64)(s>>64) ^ (u64)s;
            i32 r = (i32)(s >> 122);
            ok &= pcgnext(&p) == (x>>r | x<<(-r & 63));
        }
    }
    expectint(t, "pcg64 halves match 128-bit", ok, 1);
    #endif
}

// /dev/urandom's ChaCha: the all-zero key and nonce's first block at
// 20 rounds (the classic vector) and at 8, and the stream across block
// boundaries in odd sizes, keyed lazily, with nothing given twice.
static void test_chacha(Test *t)
{
    static u8 want20[] = {
        0x76,0xb8,0xe0,0xad,0xa0,0xf1,0x3d,0x90,0x40,0x5d,0x6a,0xe5,0x53,0x86,0xbd,0x28,
        0xbd,0xd2,0x19,0xb8,0xa0,0x8d,0xed,0x1a,0xa8,0x36,0xef,0xcc,0x8b,0x77,0x0d,0xc7,
        0xda,0x41,0x59,0x7c,0x51,0x57,0x48,0x8d,0x77,0x24,0xe0,0x3f,0xb8,0xd8,0x4a,0x37,
        0x6a,0x43,0xb8,0xf4,0x15,0x18,0xa1,0x1c,0xc3,0x87,0xb6,0x69,0xb2,0xee,0x65,0x86,
    };
    static u8 want8[] = {
        0x3e,0x00,0xef,0x2f,0x89,0x5f,0x40,0xd6,0x7f,0x5b,0xb8,0xe8,0x1f,0x09,0xa5,0xa1,
        0x2c,0x84,0x0e,0xc3,0xce,0x9a,0x7f,0x3b,0x18,0x1b,0xe1,0x88,0xef,0x71,0x1a,0x1e,
        0x98,0x4c,0xe1,0x72,0xb9,0x21,0x6f,0x41,0x9f,0x44,0x53,0x67,0x45,0x6d,0x56,0x19,
        0x31,0x4a,0x42,0xa3,0xda,0x86,0xb0,0x01,0x38,0x7b,0xfd,0xb8,0x0e,0x0c,0xfe,0x42,
    };
    u32 key[8] = {};
    u8  out[64];
    chachablock(out, key, 0, 0, 20);
    expectint(t, "chacha20 zero block", !__builtin_memcmp(out, want20, 64), 1);
    chachablock(out, key, 0, 0, 8);
    expectint(t, "chacha8 zero block", !__builtin_memcmp(out, want8, 64), 1);

    // The stream is the blocks in order, keyed from os_entropy on use
    Sched s = {};
    s.os = t->os;
    u8 a[200], b[200], ref[256];
    chachafill(&s, a, 7);
    chachafill(&s, a+7, 100);
    chachafill(&s, a+107, 93);
    expectint(t, "chacha keyed lazily", s.chacha.keyed, 1);
    for (i32 i = 0; i < 4; i++) {
        chachablock(ref+64*i, s.chacha.key, (u64)i, 0, 8);
    }
    expectint(t, "chacha stream in blocks", !__builtin_memcmp(a, ref, 200), 1);
    s.chacha = {};  // a fork's reset
    chachafill(&s, b, 200);
    expectint(t, "chacha rekeyed", __builtin_memcmp(a, b, 200) != 0, 1);
}

// The scheduler on its own: pipes between tasks, joins, EPIPE, and
// deadlock recovery.

struct PipeTest {
    Pipe *p;
    iz    got;
};

static void pipereader(Task *t)
{
    PipeTest *x = (PipeTest *)t->arg;
    u8 buf[100];
    iz n;
    while ((n = piperead(t->s, x->p, buf, countof(buf))) > 0) x->got += n;
    pipeclose(t->s, x->p, 0);
    t->status = n<0 ? 99 : 0;
}

static void test_sched(Test *t)
{
    Os *os = t->os;
    heapreset(os);
    Arena va = {};
    va.beg = os->heap + os->heapcap/2;
    va.end = os->heap + os->heapcap;
    vreset(&os->vos, va, testfakes, ntestfakes);
    os->vos.images = images;
    Arena  a = newarena(os, (iz)1<<22);
    Sched *s = newsched(os, &a, (iz)1<<22, 0, 2);

    // Three pipes' worth through one pipe: the writer blocks, the
    // reader drains, and closing the write end is end of file
    PipeTest x = {};
    x.p = newpipe(s);
    Task *r = taskspawn(s, pipereader, &x, SMALLSTACK);
    u8 *big = allocbytes(&a, 3*PIPECAP);
    memset(big, 'x', 3*PIPECAP);
    expectint(t, "sched pipewrite", pipewrite(s, x.p, big, 3*PIPECAP), 0);
    pipeclose(s, x.p, 1);
    expectint(t, "sched join", taskjoin(s, r), 0);
    expectint(t, "sched bytes", x.got, 3*PIPECAP);
    expectint(t, "sched tasks", s->ntasks, 1);

    // No readers left: E_PIPE
    Pipe *p = newpipe(s);
    pipeclose(s, p, 0);
    expectint(t, "sched epipe", pipewrite(s, p, big, 10), E_PIPE);
    pipeclose(s, p, 1);

    // The reader waits on a pipe whose writer is the joining task:
    // reported, and the read fails
    x = {};
    x.p = newpipe(s);
    r = taskspawn(s, pipereader, &x, SMALLSTACK);
    expectint(t, "sched deadlock status", taskjoin(s, r), 99);
    expect(t, "sched deadlock message", os->vos.err,
           "unabashed: deadlock: every task is blocked\n");
    pipeclose(s, x.p, 1);
    freesched(s);
}

// A stage sent SIGINT dies of it, not of SIGPIPE, even when its reader
// died first and its next write finds no reader: in bash the terminal
// signals every stage at once. Not a spec, as bash itself races here
// when the signal comes from kill. Lines longer than a pipe holds keep
// the writer inside its write while its reader goes away.
static void test_sigpipe(Test *t)
{
    Str script =
        "big=x\n"
        "for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17; do big=$big$big; done\n"
        "f() { echo $BASHPID; while :; do echo \"$big\"; done; }\n"
        "f | { read w; kill -INT $w; }\n"
        "echo \"${PIPESTATUS[*]}\"\n"
        "f | { read w; kill -INT $w $BASHPID; }\n"
        "echo \"${PIPESTATUS[*]}\"\n"
        "g() { echo $BASHPID; while :; do printf '%s\\n' \"$big\" y; done; }\n"
        "g | { read w; kill -INT $w; exit 3; }\n"
        "echo \"${PIPESTATUS[*]}\"\n"
        "(echo $BASHPID; echo \"$big\") | { read w; kill -INT $w; }\n"
        "echo \"${PIPESTATUS[*]}\"\n"
        "{ trap '' INT; echo $BASHPID; while :; do echo \"$big\"; done; } |\n"
        "    { read w; kill -INT $w; }\n"
        "echo \"${PIPESTATUS[*]}\"\n";
    // Each run moves megabytes through the vos and every tests.exe run
    // pays for it, so take only a few seeds, scrambled as specs are
    for (u64 seed = 0; seed <= 3; seed++) {
        t->os->chaos = seed * 0x9e3779b97f4a7c15u;
        Result r = runc(t, script);
        expect(t, "sigint before sigpipe", r.merged,
               "130 0\n130 130\n130 3\n130 0\n141 0\n");
    }
    t->os->chaos = 0;
}

// SpecFile tests: run a script recorded by tools/record and compare all
// of its outputs. The harness reads the files; the shell never does.

static Str readfile(char *path)
{
    Str   r = {};
    FILE *f = fopen(path, "rb");
    if (!f) return r;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    r.data = (u8 *)malloc((uz)len + 1);
    r.len  = (iz)fread(r.data, 1, (uz)len, f);
    fclose(f);
    return r;
}

struct SpecFile {
    Str out;
    Str err;
    Str merged;
    i32 status;
    b32 ok;
};

static SpecFile parsespec(Str s)
{
    SpecFile r = {};
    for (i32 n = 0; n < 4; n++) {
        Cut line = cut(s, '\n');
        if (!startswith(line.head, "== ")) return r;
        Cut name = cut(cuthead(line.head, 3), ' ');
        i64 v = 0;
        for (iz i = 0; i < name.tail.len; i++) {
            v = v*10 + (name.tail[i] - '0');
        }
        s = line.tail;
        if (name.head == "status") {
            r.status = (i32)v;
            continue;
        }
        if (v > s.len) return r;
        Str body = takehead(s, (iz)v);
        s = cuthead(s, (iz)v);
        if (s.len && s[0]=='\n') s = cuthead(s, 1);
        if (name.head == "stdout") r.out    = body;
        if (name.head == "stderr") r.err    = body;
        if (name.head == "merged") r.merged = body;
    }
    r.ok = 1;
    return r;
}

static i32 chaosruns;  // --chaos N: also run each spec with seeds 1..N

// A script's "# personality: NAME" line, else PERS_LINUX (-1 if the
// name is unknown).
static i32 scriptpersona(Str script)
{
    Str key = "# personality: ";
    for (Cut c = {{}, script, 1}; c.ok;) {
        c = cut(c.tail, '\n');
        if (startswith(c.head, key)) {
            return personality(trim(cuthead(c.head, key.len)));
        }
    }
    return PERS_LINUX;
}

// A script's "# localtz: TZ" line, the system zone the vos reports
// (os_localtz), else null for its UTC.
static Str scriptlocaltz(Str script)
{
    Str key = "# localtz: ";
    for (Cut c = {{}, script, 1}; c.ok;) {
        c = cut(c.tail, '\n');
        if (startswith(c.head, key)) return trim(cuthead(c.head, key.len));
    }
    return {};
}

static b32 test_spec(Test *t, char *path)
{
    Str  p        = {(u8 *)path, (iz)strlen(path)};
    char specpath[1024];
    if (p.len+3 > countof(specpath)) return 1;
    memcpy(specpath, path, (uz)p.len - 3);
    strcpy(specpath + p.len - 3, ".spec");

    Str script = readfile(path);
    Str spec   = readfile(specpath);
    if (!script.data || !spec.data) {
        fprintf(stderr, "SKIP: %s (missing script or spec)\n", path);
        return 1;
    }
    SpecFile want = parsespec(spec);
    if (!want.ok) {
        fprintf(stderr, "SKIP: %s (bad spec)\n", path);
        return 1;
    }
    i32 pers = scriptpersona(script);
    speclocaltz = scriptlocaltz(script);
    if (pers < 0) {
        fprintf(stderr, "SKIP: %s (unknown personality)\n", path);
        return 1;
    }

    // A Windows personality runs as w64devkit's sh or MSYS2's bash
    Str    argv0  = pers==PERS_NATIVE ? Str("sh") : Str("bash");
    Str    args[] = {argv0, "-c", script, "bash"};
    Result r      = runshell(t, args, countof(args), {}, pers);
    b32    ok     = 1;
    ok &= expect(t, p, r.out, want.out);
    ok &= expect(t, p, r.err, want.err);
    ok &= expect(t, p, r.merged, want.merged);
    ok &= expectint(t, p, r.status, want.status);

    // Under chaos scheduling, each writer's output must be the same
    // (the merged stream may interleave differently)
    i32 nchaos = find(script, "# nochaos")<0 ? chaosruns : 0;
    for (i32 seed = 1; ok && seed <= nchaos; seed++) {
        t->os->chaos = (u64)seed * 0x9e3779b97f4a7c15u;
        r = runshell(t, args, countof(args), {}, pers);
        t->os->chaos = 0;
        char name[1200];
        snprintf(name, sizeof(name), "%s (chaos seed %d)", path, seed);
        Str n = {(u8 *)name, (iz)strlen(name)};
        ok &= expect(t, n, r.out, want.out);
        ok &= expect(t, n, r.err, want.err);
        ok &= expectint(t, n, r.status, want.status);
    }

    // The script's commands must also print and reparse to a fixed
    // point (scripts with aliases are skipped: the reparse lacks them).
    if (find(script, "alias")<0 && find(script, "# nofixedpoint")<0) {
        heapreset(t->os);
        Arena a = newarena(t->os, (iz)1<<22);
        Fixed f = fixedpoint(&a, script, find(script, "extglob")>=0);
        t->run++;
        if (f.status==FIX_REPARSE || f.status==FIX_DIFFER) {
            t->failed++;
            ok = 0;
            fprintf(stderr, "FAIL: %s fixed point (%s)\n  first:  [", path,
                    f.status==FIX_REPARSE ? "reparse" : "differ");
            printstr(stderr, f.first);
            fprintf(stderr, "]\n  second: [");
            printstr(stderr, f.second);
            fprintf(stderr, "]\n");
        }
    }
    free(script.data);
    free(spec.data);
    return ok;
}

// Editor sessions (t/edit/*.ed, format in vterm.cpp): an interactive
// shell on the vos's scripted terminal, its snapshots compared with
// the .screens file recorded from bash (tools/ptyrun). The shell runs
// as ptyrun runs bash, with the same environment as specs plus TERM
// and a UTF-8 locale.
static Str edenv[] = {
    "LANG=C.UTF-8", "TZ=UTC", "HOME=/home/oracle", "TERM=xterm",
    "PATH=/usr/local/bin:/opt/fakes/bin",
};

static void putvfile(Os *, Arena, Str path, Str data);

static Str runedit(Test *t, EdScript *ed, b32 nowrap, b32 legacy)
{
    static u8 snapbuf[1<<20];
    Os *os = t->os;
    startrun(os, 0, {});
    Str *env  = alloc<Str>(&os->vos.a, countof(edenv)+ed->nenv);
    iz   nenv = 0;
    for (iz i = 0; i < countof(edenv); i++) env[nenv++] = edenv[i];
    for (iz i = 0; i < ed->nenv; i++) env[nenv++] = ed->env[i];
    putvfile(os, os->vos.a, "/home/oracle/.edrc", ed->rc);
    vttyinit(&os->vos, ed->rows, ed->cols, !nowrap, ed->keys, ed->nkeys,
             snapbuf, countof(snapbuf));
    os->vos.tty.legacy = legacy;
    static Str args[] = {"bash", "--noprofile", "--rcfile", "/home/oracle/.edrc", "-i"};
    OsInfo info = baseinfo(os, args, countof(args));
    info.env  = env;
    info.nenv = nenv;
    info.cwd  = "/home/oracle";
    finishrun(os, &info);
    if (!os->vos.tty.ended) vsnap(&os->vos);  // the shell exited
    return os->vos.tty.snaps;
}

// A redisplay left what a redraw would not (EDIT_CHECK)
static b32 edchecked(Test *t, Str name)
{
    Vos *v = &t->os->vos;
    if (!v->tty.edbad) return 1;
    t->run++;
    t->failed++;
    fprintf(stderr, "FAIL: %.*s redisplay\n", (int)name.len, (char *)name.data);
    printstr(stderr, v->tty.edwhy);
    return 0;
}

static b32 test_edit(Test *t, char *path)
{
    Str  p = {(u8 *)path, (iz)strlen(path)};
    char screenpath[1024];
    if (p.len+5 > countof(screenpath)) return 1;
    memcpy(screenpath, path, (uz)p.len - 3);
    strcpy(screenpath + p.len - 3, ".screens");

    Str src  = readfile(path);
    Str want = readfile(screenpath);
    if (!src.data) {
        fprintf(stderr, "SKIP: %s (unreadable)\n", path);
        return 1;
    }
    iz    cap = src.len*4 + (1<<16);
    byte *mem = (byte *)malloc((uz)cap);
    Arena a   = {mem, mem+cap};
    EdScript ed = edparse(&a, src);
    b32 ok = 1;
    if (ed.err.data) {
        fprintf(stderr, "SKIP: %s (%.*s)\n", path, (int)ed.err.len, (char *)ed.err.data);
    } else {
        Str got = runedit(t, &ed, ed.nowrap, 0);
        ok = edchecked(t, p);
        if (want.data) {
            ok &= expect(t, p, got, want);
            // Again on a console without VT, drawn through vtcon
            Str lp = concat(&a, p, " (vtcon)");
            got = runedit(t, &ed, ed.nowrap, 1);
            ok &= edchecked(t, lp);
            ok &= expect(t, lp, got, want);
        } else {
            // Not recorded (or vos only): printed for review
            fprintf(stderr, "SKIP: %s (no .screens)\n", path);
            printstr(stdout, got);
            fflush(stdout);
        }
    }
    free(mem);
    free(src.data);
    free(want.data);
    return ok;
}

// Git Bash differential specs (t/msys): its "# vos" scripts, run as
// t/msys/run runs them (tools/README.md, "Git Bash differential
// tests") but on the vos's Windows tree laid out like its scratch
// install: WORKDIR is C:/work, the install root C:/msys (@ROOT@) holds
// only usr/bin/bash.exe (the shell) and etc/fstab, Git's programs are
// in C:/git (@GIT@), w64devkit in C:/w64 (@W64@), argv and argv32 in
// C:/work/bin, and the test in C:/work/NAME. Outputs are normalized as
// the runner normalizes them, then compared with Git Bash's recording.

struct NormPair {
    Str spelling;
    Str token;
};

// Longest spelling first, ties by the bytes of "TOKEN SPELLING" (no
// token is a prefix of another, so by token, then by spelling).
static b32 normless(NormPair *a, NormPair *b)
{
    if (a->spelling.len != b->spelling.len) return a->spelling.len > b->spelling.len;
    i32 c = compare(a->token, b->token);
    return c ? c<0 : compare(a->spelling, b->spelling)<0;
}

// The 24 (spelling, token) pairs: for each directory X:R, the forms
// X:R x:R X:B x:B /xR /XR, B being R with backslashes.
static Slice<NormPair> normpairs(Arena *a)
{
    static Str dirs[][2] = {
        {"@ROOT@", "C:/msys"}, {"@WORK@", "C:/work"},
        {"@GIT@",  "C:/git"},  {"@W64@",  "C:/w64"},
    };
    Slice<NormPair> r = {};
    for (iz i = 0; i < countof(dirs); i++) {
        Str dir  = dirs[i][1];
        u8  up   = upcase(dir[0]);
        u8  lo   = lowercase(dir[0]);
        Str rest = cuthead(dir, 2);
        Str back = slashify(a, rest, '\\');
        Str forms[6] = {};
        for (i32 k = 0; k < 6; k++) {
            Buf b(a);
            if (k < 4) {
                putbyte(&b, k&1 ? lo : up);
                putbyte(&b, ':');
                print(&b, k<2 ? rest : back);
            } else {
                putbyte(&b, '/');
                putbyte(&b, k==4 ? lo : up);
                print(&b, rest);
            }
            forms[k] = finish(&b);
        }
        for (i32 k = 0; k < 6; k++) {
            NormPair p = {forms[k], dirs[i][0]};
            r = push(a, r, p);
        }
    }
    sort(r.data, r.len, normless, *a);
    return r;
}

// Replace each pair's spelling by its token, pair by pair, left to
// right without overlaps, byte-exact.
static Str normalize(Arena *a, Str s, Slice<NormPair> pairs)
{
    for (iz k = 0; k < pairs.len; k++) {
        Str from = pairs[k].spelling;
        Buf b(a, s.len);
        for (iz i = 0; i < s.len;) {
            if (startswith(cuthead(s, i), from)) {
                print(&b, pairs[k].token);
                i += from.len;
            } else {
                putbyte(&b, s[i++]);
            }
        }
        s = finish(&b);
    }
    return s;
}

static b32 varchar(u8 c)
{
    return alnum(c) || c=='_';
}

// The words of a "# shell:" line as t/msys/lib.sh's eval reads them,
// with $D, $DW and $DU the test directory as C:/x, C:\x and /c/x.
static Slice<Str> shellwords(Arena *a, Str line, Str *vars)
{
    static Str names[] = {"D", "DW", "DU"};
    Slice<Str> words = {};
    for (iz i = 0;;) {
        for (; i<line.len && (line[i]==' ' || line[i]=='\t'); i++) {}
        if (i == line.len) break;
        Buf w(a);
        b32 dq = 0;
        while (i<line.len && (dq || (line[i]!=' ' && line[i]!='\t'))) {
            u8 c = line[i];
            if (c=='\'' && !dq) {
                for (i++; i<line.len && line[i]!='\''; i++) putbyte(&w, line[i]);
                i++;
            } else if (c == '"') {
                dq = !dq;
                i++;
            } else if (c=='\\' && i+1<line.len && (!dq || find("\\\"$`", {line.data+i+1, 1})>=0)) {
                putbyte(&w, line[i+1]);
                i += 2;
            } else if (c=='$' && i+1<line.len && varchar(line[i+1])) {
                iz beg = ++i;
                for (; i<line.len && varchar(line[i]); i++) {}
                Str name = span(line.data+beg, line.data+i);
                for (iz k = 0; k < countof(names); k++) {
                    if (name == names[k]) print(&w, vars[k]);
                }
            } else {
                putbyte(&w, c);
                i++;
            }
        }
        words = push(a, words, finish(&w));
    }
    return words;
}

static void vdetachname(Vos *v, Str path)
{
    VNode *n = vwalk(v, path, 0).node;
    if (n) vdetach(n);
}

// Run a t/msys script on the vos as test.sh in C:/work/NAME.
static Result runmsys(Test *t, Str name, Str script)
{
    enum { X64 = 0x8664, I386 = 0x14c };
    Os *os = t->os;
    startrun(os, 1, {});
    Vos   *v = &os->vos;
    Arena *a = &v->a;

    // Git's install: its programs (MSYS2 builds of the fakes) and etc
    for (i32 i = 0; i < v->nfakes; i++) {
        Str fake = v->fakes[i].name;
        if (v->fakes[i].win || fake=="recho" || fake=="zecho" || fake=="printenv" || fake=="xcase") {
            continue;
        }
        vinstallpe(v, "C:/git/usr/bin", concat(a, fake, ".exe"), i, X64, "msys-2.0.dll", 0);
    }
    vputfile(v, "C:/git/etc", "fstab",
        "none / cygdrive binary,posix=0,noacl,user 0 0\n"
        "none /tmp usertemp binary,posix=0,noacl 0 0\n");

    // The scratch install: the shell and an fstab lending it Git's
    VNode *bin  = vwalk(v, "/C:/msys/usr/bin", 1).node;
    VNode *bash = vlookup(v, bin, "bash.exe");
    bin->child  = bash;
    bash->next  = 0;
    vdetachname(v, "/C:/msys/tmp");
    VNode *fstab = vwalk(v, "/C:/msys/etc/fstab", 1).node;
    fstab->data.len = 0;
    Str tab =
        "# t/msys scratch install: Git's programs, the shell under test\n"
        "C:/git/usr/bin /usr/bin ntfs binary,noacl,auto 0 0\n"
        "C:/git/usr/bin /bin ntfs binary,noacl,auto 0 0\n"
        "C:/git/etc /etc ntfs binary,noacl,auto 0 0\n"
        "none / cygdrive binary,posix=0,noacl,user 0 0\n"
        "none /tmp usertemp binary,posix=0,noacl 0 0\n";
    vwritenode(v, fstab, 0, tab.data, tab.len);

    // argv and argv32 in WORKDIR/bin, not w64devkit's
    vdetachname(v, "/C:/w64/bin/argv.exe");
    vdetachname(v, "/C:/w64/bin/argv32.exe");
    i32 argv = -1;
    for (i32 i = 0; i < v->nfakes; i++) argv = v->fakes[i].name=="argv" ? i : argv;
    vinstallpe(v, "C:/work/bin", "argv.exe", argv, X64, "msvcrt.dll", 0);
    vinstallpe(v, "C:/work/bin", "argv32.exe", argv, I386, "msvcrt.dll", 0);

    // As on the recording machine: C:\WINDOWS, and w64devkit's cc
    vwalk(v, "/C:/Windows", 0).node->name = "WINDOWS";
    vinstallpe(v, "C:/w64/bin", "cc.exe", argv, X64, "msvcrt.dll", 0);

    Str dir = concat(a, "C:/work/", name);
    vmkdirs(v, concat(a, dir, "/home"));
    vmkdirs(v, concat(a, dir, "/tmp"));
    vputfile(v, dir, "test.sh", script);

    Str dw = slashify(a, dir, '\\');
    Str du = concat(a, "/c", cuthead(dir, 2));
    Str env[] = {
        "COMSPEC=C:\\WINDOWS\\System32\\cmd.exe",
        concat(a, concat(a, "HOME=", dw), "\\home"),
        "LANG=C",
        "PATH=C:\\git\\usr\\bin;C:\\w64\\bin;C:\\WINDOWS\\System32;C:\\work\\bin",
        "SYSTEMROOT=C:\\WINDOWS",
        concat(a, concat(a, "TEMP=", dw), "\\tmp"),
        concat(a, concat(a, "TMP=", dw), "\\tmp"),
        "TZ=UTC",
    };
    Str *envs = alloc<Str>(a, countof(env));
    for (iz i = 0; i < countof(env); i++) envs[i] = env[i];

    Str exe = "C:/msys/usr/bin/bash.exe";
    Slice<Str> args = {};
    args = push(a, args, exe);
    Str key = "# shell: ";
    Str shell = {};
    for (Cut c = {{}, script, 1}; c.ok;) {
        c = cut(c.tail, '\n');
        if (startswith(c.head, key)) shell = cuthead(c.head, key.len);
    }
    if (shell.data) {
        if (shell.len && shell[shell.len-1]=='\r') shell.len--;
        Str vars[] = {dir, dw, du};
        Slice<Str> w = shellwords(a, shell, vars);
        for (iz i = 0; i < w.len; i++) args = push(a, args, w[i]);
    } else {
        args = push(a, args, Str("--norc"));
        args = push(a, args, Str("--noprofile"));
        args = push(a, args, Str("test.sh"));
    }

    OsInfo info = baseinfo(os, args.data, args.len);
    wininfo(os, &info, exe, envs, countof(env), dir);
    return finishrun(os, &info);
}

static b32 hasline(Str s, Str line)
{
    for (Cut c = {{}, s, 1}; c.ok;) {
        c = cut(c.tail, '\n');
        Str h = c.head;
        if (h.len && h[h.len-1]=='\r') h.len--;
        if (h == line) return 1;
    }
    return 0;
}

static i32  msyspass;
static i32  msysfail;
static i32  msysskip;    // scripts the vos cannot run (no "# vos")
static b32  msysdiff;    // --msys-diff: show what differs

static void test_msys(Test *t, char *path)
{
    Str p = {(u8 *)path, (iz)strlen(path)};
    char specpath[1024];
    if (p.len+3 > countof(specpath)) return;
    memcpy(specpath, path, (uz)p.len - 3);
    strcpy(specpath + p.len - 3, ".spec");
    Str script = readfile(path);
    Str spec   = readfile(specpath);
    if (!script.data || !spec.data) {
        fprintf(stderr, "SKIP: %s (missing script or spec)\n", path);
        msysskip++;
        return;
    }
    SpecFile want = parsespec(spec);
    if (!want.ok || !hasline(script, "# vos")) {
        msysskip++;
        free(script.data);
        free(spec.data);
        return;
    }

    iz  slash = p.len;
    for (; slash>0 && p[slash-1]!='/' && p[slash-1]!='\\'; slash--) {}
    Str name = span(p.data+slash, p.data+p.len-3);
    Result r = runmsys(t, name, script);

    Arena a = t->os->vos.a;
    Slice<NormPair> pairs = normpairs(&a);
    Str out    = normalize(&a, r.out, pairs);
    Str err    = normalize(&a, r.err, pairs);
    Str merged = normalize(&a, r.merged, pairs);
    b32 ok = out==want.out && err==want.err && merged==want.merged && r.status==want.status;
    msyspass += ok;
    msysfail += !ok;
    if (!ok) {
        printf("FAIL %.*s\n", (int)name.len, (char *)name.data);
        if (msysdiff) {
            printf("  want (status %d): [", want.status);
            printstr(stdout, want.merged);
            printf("]\n  got  (status %d): [", r.status);
            printstr(stdout, merged);
            printf("]\n");
        }
    }
    free(script.data);
    free(spec.data);
}

// The vos's Windows mode on its own: path resolution, case folding,
// reserved names, named pipes, the PE images through pesniff, and
// command lines split by each kind of target.

static Str runspawn(Os *os, Arena a, Spawn *s, i32 *status)
{
    Vos *v = &os->vos;
    v->out.len = v->err.len = v->merged.len = 0;
    static SpawnFd fds[] = {{1, 1}, {2, 2}};
    s->fds  = fds;
    s->nfds = countof(fds);
    s->cwd  = s->cwd.len ? s->cwd : Str("C:/work");
    Spawned r = os_spawn(os, a, s);
    *status = r.err ? -r.err : os_waitproc(os, r.proc);
    return v->merged;
}

static void putvfile(Os *os, Arena a, Str path, Str data)
{
    Opened o = os_open(os, a, path, OF_WRITE|OF_CREATE|OF_TRUNC, 0644);
    os_write(os, o.h, data.data, data.len);
    os_close(os, o.h);
}

// Process groups and stops on the vos: a group signal reaches each
// member, stops and continues are reported to IO_PROCSTOP (an exit
// still ends the process), a fatal signal to a stopped program waits
// for its continue, and SIGSTOP stops even a program ignoring SIGTSTP.
static Spawned spawnsleep(Os *os, Arena a, Str secs, i64 pgid, u64 sigign = 0)
{
    static SpawnFd fds[] = {{1, 1}, {2, 2}};
    Spawn s  = {};
    Str argv[] = {"sleep", secs};
    s.fds    = fds;
    s.nfds   = countof(fds);
    s.path   = "/opt/fakes/bin/sleep";
    s.argv   = argv;
    s.argc   = countof(argv);
    s.cwd    = "/home/oracle";
    s.pgid   = pgid;
    s.sigign = sigign;
    return os_spawn(os, a, &s);
}

static i32 procstop(Os *os, Handle proc)
{
    Io io = {};
    io.op = IO_PROCSTOP;
    io.h  = proc;
    os_submit(os, &io);
    if (!io.done) {
        os_cancel(os, &io);
        return -1;  // nothing to report
    }
    return (i32)io.n;
}

static void test_jobctl(Test *t)
{
    Os  *os = t->os;
    startrun(os, 0, {});
    Arena a = newarena(os, (iz)1<<20);

    Spawned p = spawnsleep(os, a, "5", -1);
    expectint(t, "jobctl spawn", p.err, 0);
    expectint(t, "jobctl stop", os_kill(os, -p.pid, 19), 0);
    expectint(t, "jobctl stopped", procstop(os, p.proc), WSTOP|19);
    expectint(t, "jobctl once", procstop(os, p.proc), -1);
    expectint(t, "jobctl held", os_kill(os, p.pid, 15), 0);
    expectint(t, "jobctl still", procstop(os, p.proc), -1);
    expectint(t, "jobctl cont", os_kill(os, p.pid, 18), 0);
    expectint(t, "jobctl killed", procstop(os, p.proc), WSIGNALED|(128+15));

    Spawned l = spawnsleep(os, a, "1", -1);
    Spawned m = spawnsleep(os, a, "1", l.pid);
    expectint(t, "jobctl tstp", os_kill(os, -l.pid, 20), 0);
    expectint(t, "jobctl leader", procstop(os, l.proc), WSTOP|20);
    expectint(t, "jobctl member", procstop(os, m.proc), WSTOP|20);
    expectint(t, "jobctl group cont", os_kill(os, -l.pid, 18), 0);
    expectint(t, "jobctl leader cont", procstop(os, l.proc), WCONT);
    expectint(t, "jobctl member cont", procstop(os, m.proc), WCONT);
    expectint(t, "jobctl leader exit", os_waitproc(os, l.proc), 0);
    expectint(t, "jobctl member exit", os_waitproc(os, m.proc), 0);
    expectint(t, "jobctl gone", os_kill(os, -l.pid, 15), E_SRCH);
    expectint(t, "jobctl zero", os_kill(os, 0, 0), 0);  // own group

    Spawned g = spawnsleep(os, a, "1", 0, (u64)1<<(20-1));
    expectint(t, "jobctl own group", os_kill(os, -2, 0), 0);
    expectint(t, "jobctl ignored", os_kill(os, g.pid, 20), 0);
    expectint(t, "jobctl ignoring", procstop(os, g.proc), -1);
    expectint(t, "jobctl setpgid", os_setpgid(os, g.pid, 0), 0);
    expectint(t, "jobctl sigstop", os_kill(os, -g.pid, 19), 0);
    expectint(t, "jobctl sigstopped", procstop(os, g.proc), WSTOP|19);
    os_kill(os, g.pid, 18);
    expectint(t, "jobctl plain wait", os_waitproc(os, g.proc), 0);
    expectint(t, "jobctl no tty", (i32)os_tcpgrp(os, 1, 0), -E_NOTTY);
    expect(t, "jobctl no deadlock", os->vos.err, "");
}

static void test_vwin(Test *t)
{
    Os  *os = t->os;
    startrun(os, 1, {});
    Vos  *v = &os->vos;
    Arena a = newarena(os, (iz)1<<22);

    // Resolution: drive roots, MSYS-style drives, both separators, case,
    // dot-dot, rooted paths on C:, and what cannot exist
    struct { Str path; i32 type; i32 err; } paths[] = {
        {"C:/",                      FT_DIR,  0},
        {"c:",                       FT_DIR,  0},
        {"D:\\",                     FT_DIR,  0},
        {"E:/",                      0,       E_NOENT},
        {"C:/work",                  FT_DIR,  0},
        {"c:\\WORK\\",               FT_DIR,  0},
        {"/c/Work",                  FT_DIR,  0},
        {"/C",                       FT_DIR,  0},
        {"C:/work/../WINDOWS/system32/CMD.EXE", FT_FILE, 0},
        {"/Windows/System32/cmd.exe", FT_FILE, 0},
        {"\\\\?\\C:\\w64\\bin\\argv.exe", FT_FILE, 0},
        {"work",                     0,       E_NOENT},
        {"//srv/share/x",            0,       E_NOENT},
        {"\\\\srv\\share",           0,       E_NOENT},
        {"C:/work/x/",               0,       E_NOENT},
        {"C:/w64/bin/argv.exe/",     0,       E_NOTDIR},
        {"/dev/null",                FT_CHAR, 0},
        {"/dev/tty",                 0,       E_NOENT},
        {"//./NUL",                  FT_CHAR, 0},
        {"\\\\.\\nul",               FT_CHAR, 0},
        {"C:/work/nul",              FT_CHAR, 0},
        {"C:/nowhere/Con.txt",       FT_CHAR, 0},
        {"D:/aux.c.d. ",             FT_CHAR, 0},
        {"C:/work/nul/x",            0,       E_NOENT},
        {"C:/work/null",             0,       E_NOENT},
        {"//./foo",                  0,       E_NOENT},
        {"//./pipe/none",            0,       E_NOENT},
    };
    for (iz i = 0; i < countof(paths); i++) {
        Stat st = os_stat(os, a, paths[i].path, 1);
        Str  name = concat(&a, "vwin stat ", paths[i].path);
        expectint(t, name, st.err, paths[i].err);
        if (!paths[i].err) expectint(t, name, st.type, paths[i].type);
    }

    // Names keep their case and are found in any case, listed in NTFS
    // order; modes are synthesized as by the Win32 layer
    putvfile(os, a, "C:/work/Foo.TXT", "hi");
    putvfile(os, a, "c:/WORK/bar", "#!/bin/sh\n");
    putvfile(os, a, "C:/work/Baz.BAT", "");
    os_mkdir(os, a, "C:/work/dir", 0755);
    expectint(t, "vwin case size", os_stat(os, a, "c:/WORK/FOO.txt", 1).size, 2);
    Opened o = os_open(os, a, "C:/WORK/foo.txt", OF_WRITE|OF_CREATE|OF_EXCL, 0644);
    expectint(t, "vwin case exists", o.err, E_EXIST);
    o = os_open(os, a, "C:/work/NUL.txt", OF_WRITE|OF_CREATE, 0644);
    expectint(t, "vwin nul write", o.err || !os_write(os, o.h, (u8 *)"x", 1), 0);
    os_close(os, o.h);
    i32 err = 0;
    Buf names(&a);
    for (Dirent *d = os_list(os, &a, "C:/work", &err); d; d = d->next) {
        print(&names, d->name);
        putbyte(&names, ' ');
    }
    expect(t, "vwin list", finish(&names), "bar Baz.BAT dir Foo.TXT ");
    expectint(t, "vwin mode file", os_stat(os, a, "C:/work/Foo.TXT", 1).mode, 0644);
    expectint(t, "vwin mode #!", os_stat(os, a, "C:/work/bar", 1).mode, 0755);
    // extension rules belong to the core (winmode): content only here
    expectint(t, "vwin mode .bat", os_stat(os, a, "C:/work/baz.bat", 1).mode, 0644);
    expectint(t, "vwin mode dir", os_stat(os, a, "C:/work/dir", 1).mode, 0755);
    os_chmod(os, a, "C:/work/Foo.TXT", 0444);
    expectint(t, "vwin mode read-only", os_stat(os, a, "C:/work/Foo.TXT", 1).mode, 0444);

    // PE images, as pesniff classifies them
    struct { Str path; i32 machine, width, crt, posix; b32 ours; } pes[] = {
        {"C:/w64/bin/argv.exe",         0x8664, 8, CRT_MSVCRT, 0,          0},
        {"C:/w64/bin/argv32.exe",       0x14c,  4, CRT_MSVCRT, 0,          0},
        {"C:/w64/bin/busybox.exe",      0x8664, 8, CRT_MSVCRT, 0,          0},
        {"C:/w64/bin/ls.exe",           0x8664, 8, CRT_NONE,   0,          0},
        {"C:/w64/bin/sh.exe",           0x8664, 8, CRT_NONE,   0,          1},
        {"C:/msys/usr/bin/cat.exe",     0x8664, 8, CRT_NONE,   POSIX_MSYS, 0},
        {"C:/msys/usr/bin/bash.exe",    0x8664, 8, CRT_NONE,   0,          1},
        {"C:/Windows/System32/cmd.exe", 0x8664, 8, CRT_UCRT,   0,          0},
    };
    for (iz i = 0; i < countof(pes); i++) {
        VNode *n    = vresolve(v, &a, pes[i].path, 1).node;
        PeInfo pe   = vpesniff(n, a);
        Str    name = concat(&a, "vwin pe ", pes[i].path);
        expectint(t, name, pe.ok, 1);
        expectint(t, name, pe.machine, pes[i].machine);
        expectint(t, name, pe.width, pes[i].width);
        expectint(t, name, pe.subsystem, 3);
        expectint(t, name, pe.crt, pes[i].crt);
        expectint(t, name, pe.posix, pes[i].posix);
        expectint(t, name, pe.ours, pes[i].ours);
    }

    // A copy of an image runs the same program; an ELF-like fake does not
    VNode *argv = vresolve(v, &a, "C:/w64/bin/argv.exe", 1).node;
    putvfile(os, a, "C:/work/copy.exe", argv->data);
    putvfile(os, a, "C:/work/elf.exe", {(u8 *)vmagic, countof(vmagic)});
    expectint(t, "vwin copy", vprogram(v, &a, "c:\\work\\COPY.exe").prog, argv->prog-1);
    expectint(t, "vwin elf", vprogram(v, &a, "C:/work/elf.exe").err, E_NOEXEC);

    // Named pipes
    NamedPipe np = os_namedpipe(os, &a, {}, 0);
    expect(t, "vwin pipe name", np.name, "//./pipe/unabashed-2-1");
    expectint(t, "vwin pipe stat", os_stat(os, a, "//./pipe/Unabashed-2-1", 1).type, FT_PIPE);
    expectint(t, "vwin pipe access", os_open(os, a, np.name, OF_READ, 0).err, E_ACCES);
    Io io = {};
    io.op = IO_CONNECT;
    io.h  = np.h;
    expectint(t, "vwin pipe listening", vioready(osvos(os), &io), 0);
    o = os_open(os, a, "\\\\.\\PIPE\\UNABASHED-2-1", OF_WRITE, 0);
    expectint(t, "vwin pipe open", o.err, 0);
    expectint(t, "vwin pipe connected", vioready(osvos(os), &io), 1);
    expectint(t, "vwin pipe busy", os_open(os, a, np.name, OF_WRITE, 0).err, E_BUSY);
    NamedPipe np2 = os_namedpipe(os, &a, np.name, 0);
    expect(t, "vwin pipe instance", np2.name, np.name);
    Opened o2 = os_open(os, a, np.name, OF_WRITE, 0);
    expectint(t, "vwin pipe second", o2.err, 0);
    os_write(os, o2.h, (u8 *)"2nd", 3);
    os_close(os, o2.h);
    os_write(os, o.h, (u8 *)"hi", 2);
    os_close(os, o.h);
    u8 buf[8] = {};
    expectint(t, "vwin pipe read", os_read(os, np.h, buf, countof(buf)), 2);
    expectint(t, "vwin pipe eof", os_read(os, np.h, buf, countof(buf)), 0);
    expectint(t, "vwin pipe read second", os_read(os, np2.h, buf, countof(buf)), 3);
    os_close(os, np.h);
    os_close(os, np2.h);
    expectint(t, "vwin pipe gone", os_stat(os, a, np.name, 1).err, E_NOENT);
    np = os_namedpipe(os, &a, {}, NP_OUT);
    expect(t, "vwin pipe reuse", np.name, "//./pipe/unabashed-2-3");
    os_close(os, np.h);

    // Command lines: MSVCRT splitting for argv, Cygwin's (globs and
    // response files) for an MSYS2 program, an alias stub through
    // busybox, cmd's expansion, and a batch file through ComSpec
    i32 status = 0;
    Spawn s = {};
    s.path    = "C:/w64/bin/argv.exe";
    s.cmdline = "argv \"a b\" c\\\"d e\\\\f \"\"";
    expect(t, "vwin argv", runspawn(os, a, &s, &status),
           "cmdline = <argv \"a b\" c\\\"d e\\\\f \"\">\n"
           "argv[0] = <argv>\nargv[1] = <a b>\nargv[2] = <c\"d>\n"
           "argv[3] = <e\\\\f>\nargv[4] = <>\n");

    s = {};
    s.path = "C:/w64/bin/argv32.exe";
    Str args[] = {"argv32", "a b", ""};
    s.argv = args;
    s.argc = countof(args);
    expect(t, "vwin argv without cmdline", runspawn(os, a, &s, &status),
           "cmdline = <argv32 \"a b\" \"\">\n"
           "argv[0] = <argv32>\nargv[1] = <a b>\nargv[2] = <>\n");

    putvfile(os, a, "C:/work/a.txt", "");
    putvfile(os, a, "C:/work/b.txt", "");
    putvfile(os, a, "C:/work/resp", "r1 'r 2'");
    static Str home[] = {"HOME=C:\\work"};
    s = {};
    s.path    = "C:/msys/usr/bin/echo.exe";
    s.cmdline = "echo *.txt \"*.txt\" {x,y} '{x,y}' @resp ~ C:\\x";
    s.env     = home;
    s.nenv    = countof(home);
    expect(t, "vwin msys", runspawn(os, a, &s, &status),
           "a.txt b.txt *.txt x y {x,y} r1 r 2 C:\\work C:\\x\n");

    s = {};
    s.path    = "C:/w64/bin/echo.exe";
    s.cmdline = "echo \"a  b\" c";
    expect(t, "vwin alias", runspawn(os, a, &s, &status), "a  b c\n");

    static Str env[] = {
        "ComSpec=C:\\Windows\\System32\\cmd.exe", "Path=C:\\w64\\bin",
    };
    s = {};
    s.path    = "C:/Windows/System32/cmd.exe";
    s.cmdline = "cmd /d /c echo %path%^! %nope% \"^\" 100%%";
    s.env     = env;
    s.nenv    = countof(env);
    expect(t, "vwin cmd", runspawn(os, a, &s, &status),
           "C:\\w64\\bin! %nope% \"^\" 100%%\r\n");

    putvfile(os, a, "C:/work/t.bat",
        "@echo off\r\necho [%1] [%~1] [%2] [%*]\r\nexit /b 3\r\necho never\r\n");
    Str bat[] = {"t.bat", "a b", "%PATH%"};
    s = {};
    s.path = "C:/work/t.bat";
    s.argv = bat;
    s.argc = countof(bat);
    s.env  = env;
    s.nenv = countof(env);
    expect(t, "vwin batch", runspawn(os, a, &s, &status),
           "[\"a b\"] [a b] [\"%PATH%\"] [\"a b\" \"%PATH%\"]\r\n");
    expectint(t, "vwin batch status", status, 3);
    vfinish(os);
}

// Oracle runs: bash's own test suite, run read-only on the vos. The
// suite's directory is copied into the virtual tree for each test, so
// nothing it does reaches the host.

static void vimport(Vos *v, VNode *dir, char *host, i32 depth)
{
    if (depth > 8) return;
    DIR *d = opendir(host);
    if (!d) return;
    for (struct dirent *e; (e = readdir(d));) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", host, e->d_name);
        Str name = {(u8 *)e->d_name, (iz)strlen(e->d_name)};
        DIR *sub = opendir(path);
        if (sub) {
            closedir(sub);
            VNode *n = vnew(v, dir, name, FT_DIR, 0755);
            vimport(v, n, path, depth+1);
            continue;
        }
        Str data = readfile(path);
        if (!data.data) continue;
        // Windows checkouts lose file modes: these are the suite's
        // executable files in the bash 5.3 tarball
        static char const *exec[] = {"exec1.sub", "trap1.sub", "trap2.sub", "trap2a.sub"};
        b32 script = data.len>1 && data[0]=='#' && data[1]=='!';
        for (i32 i = 0; i < (i32)countof(exec); i++) {
            script |= !depth && !strcmp(e->d_name, exec[i]);
        }
        VNode *n = vnew(v, dir, name, FT_FILE, script ? 0755 : 0644);
        vwritenode(v, n, 0, data.data, data.len);
        free(data.data);
    }
    closedir(d);
}

// Stand-ins for files of the bash build tree that the suite reads, such
// as heredoc5.sub's y.tab.c, config.h and version.h, which it feeds
// through here-documents for their sizes: synthetic C-like text of at
// least the given length, with tabs, quotes, backslashes and dollars.
static void vbuildfile(Vos *v, VNode *dir, Str name, iz size)
{
    static char const *lines[] = {
        "/* %s: generated stand-in, line %d */\n",
        "#define NAME_%d \"value \\\"%d\\\" with $HOME and `x`\"\n",
        "\tif (c == '\\\\' || c == '$') {\t/* %s */\n",
        "\t\treturn yytable[%d] + '\\n';\n",
        "\t}\n",
        "  static char *s%d = \"${var} $(cmd) %d\";\n",
    };
    char buf[256];
    VNode *n = vnew(v, dir, name, FT_FILE, 0644);
    for (i32 i = 0; n->data.len < size; i++) {
        char const *fmt = lines[i % countof(lines)];
        i32 len;
        if (strstr(fmt, "%s")) {
            len = snprintf(buf, sizeof(buf), fmt, (char *)"x", i);
        } else {
            len = snprintf(buf, sizeof(buf), fmt, i, i);
        }
        vwritenode(v, n, n->data.len, (u8 *)buf, len);
    }
}

// Run "bash run-NAME" in the suite's directory. Like run-all, any
// output means a failure.
static b32 oracle(Test *t, char *suite, char *name, b32 verbose)
{
    Os *os = t->os;
    heapreset(os);
    Arena va = {};
    va.beg = os->heap + os->heapcap/2;
    va.end = os->heap + os->heapcap;
    vreset(&os->vos, va, testfakes, ntestfakes);
    os->vos.images = images;
    // run-all runs from a terminal, where nobody types: some tests time
    // out reads from it
    os->vos.ttyin = 1;
    // The suite expects a populated /bin, as on a real system: the
    // fakes stand in for it, and the shell for /bin/sh
    for (iz i = 0; i < ntestfakes; i++) {
        Str name = testfakes[i].name;
        if (name=="recho" || name=="zecho" || name=="xcase") continue;
        if (testfakes[i].win) continue;
        vinstall(&os->vos, "usr/bin", name, (i32)i);
        if (name == "bash") vinstall(&os->vos, "usr/bin", "sh", (i32)i);
    }
    VNode *home = vmkdirs(&os->vos, "home/oracle/tests");
    vimport(&os->vos, home, suite, 0);
    VNode *build = home->parent;  // BUILD_DIR
    vbuildfile(&os->vos, build, "y.tab.c",   100000);
    vbuildfile(&os->vos, build, "config.h",  20000);
    vbuildfile(&os->vos, build, "version.h", 2000);
    // A system's account files, which a few tests read (the vos has
    // /etc/passwd already)
    VNode *etc = vmkdirs(&os->vos, "etc");
    static char const group[] = "root:x:0:\ndaemon:x:1:\noracle:x:1000:\n";
    VNode *gr = vnew(&os->vos, etc, "group", FT_FILE, 0644);
    vwritenode(&os->vos, gr, 0, (u8 *)group, (iz)sizeof(group)-1);

    static Str env[] = {
        "LANG=C", "TZ=UTC", "HOME=/home/oracle", "TMPDIR=/tmp",
        "PATH=.:/usr/local/bin:/opt/fakes/bin",
        "THIS_SH=/usr/local/bin/bash", "BUILD_DIR=/home/oracle",
        "BASH_TSTOUT=/tmp/bashtst-1",
    };
    char run[256];
    snprintf(run, sizeof(run), "run-%s", name);
    Str args[] = {"sh", {(u8 *)run, (iz)strlen(run)}};
    OsInfo info = {};
    info.args    = args;
    info.nargs   = countof(args);
    info.env     = env;
    info.nenv    = countof(env);
    info.cwd     = "/home/oracle/tests";
    info.pid     = 2;
    info.ppid    = 1;
    info.pgid    = 2;
    info.uid     = info.euid = 1000;
    info.gid     = info.egid = 1000;
    info.host    = "oracle";
    info.std[0]  = 0;
    info.std[1]  = 1;
    info.std[2]  = 2;
    info.reserve = (iz)1<<22;
    stackrest    = 1;
    info.stack   = stackleft();
    i32 status = sh_main(os, &info) & 255;
    vfinish(os);
    stackrest    = 0;
    // Some run-* scripts always print advisory "warning: ..." lines,
    // which run-all shows to a human. They are not differences.
    b32 pass = 1;
    for (Str rest = os->vos.merged; rest.len;) {
        iz  nl   = find(rest, "\n");
        Str line = nl<0 ? rest : takehead(rest, nl);
        rest     = nl<0 ? Str{} : cuthead(rest, nl+1);
        if (!startswith(line, "warning: ")) pass = 0;
    }
    printf("%-20s %s (status %d, %lld bytes)\n", name, pass ? "PASS" : "FAIL",
           status, (long long)os->vos.merged.len);
    if (!pass && verbose) {
        printstr(stdout, os->vos.merged);
        printf("\n");
    }
    return pass;
}

int main(int argc, char **argv)
{
    static Os os = {};
    byte base = 0;
    stackbase = &base;
    os.heapcap         = (iz)1<<30;  // half for the shells: room for background
                                     // vprocs, and the images of jobs not yet
                                     // started (read2.sub has some 50 at once)
    os.heap            = (byte *)malloc((uz)os.heapcap);
    os.vos.outcap      = (iz)1<<20;
    os.vos.out.data    = (u8 *)malloc((uz)os.vos.outcap);
    os.vos.err.data    = (u8 *)malloc((uz)os.vos.outcap);
    os.vos.merged.data = (u8 *)malloc((uz)os.vos.outcap);
    initfakes();
    if (argc>1 && !strcmp(argv[1], "--fibers")) {
        // Background shells as fiber vprocs, as before M5b and as where
        // a platform has no images, in every mode below
        images = 0;
        argv[1] = argv[0];
        argc--;
        argv++;
    }

    Test t = {};
    t.os = &os;
    test_m0(&t);
    test_fibers(&t);
    test_vterm(&t);
    test_conkey(&t);
    test_arena(&t);
    test_pcg(&t);
    test_chacha(&t);
    test_sched(&t);
    test_sigpipe(&t);
    test_vwin(&t);
    test_jobctl(&t);
    test_pathmodes(&t);
    test_plainbrace(&t);

    if (argc>3 && (!strcmp(argv[1], "--oracle") || !strcmp(argv[1], "--oracle-v"))) {
        // --oracle SUITEDIR NAME...: bash test suite runs on the vos
        b32 verbose = !strcmp(argv[1], "--oracle-v");
        i32 pass = 0;
        for (i32 i = 3; i < argc; i++) {
            pass += oracle(&t, argv[2], argv[i], verbose);
        }
        printf("%d of %d pass\n", pass, argc-3);
        return pass != argc-3;
    }

    if (argc>2 && !strcmp(argv[1], "--run")) {
        // --run [--personality NAME] [--spec] SCRIPT [SEED]: run one
        // script on the vos and show what it printed, or its spec, with
        // an optional chaos seed; a "# personality:" line also counts
        i32 i    = 2;
        i32 pers = -2;
        b32 spec = 0;
        for (; i+1 < argc; i++) {
            if (!strcmp(argv[i], "--personality")) {
                pers = personality({(u8 *)argv[i+1], (iz)strlen(argv[i+1])});
                i++;
            } else if (!strcmp(argv[i], "--spec")) {
                spec = 1;
            } else {
                break;
            }
        }
        Str script = readfile(argv[i]);
        pers = pers==-2 ? scriptpersona(script) : pers;
        speclocaltz = scriptlocaltz(script);
        if (!script.data || pers<0) {
            fprintf(stderr, "tests: --run [--personality linux|native|msys] [--spec] SCRIPT [SEED]\n");
            return 2;
        }
        os.chaos = i+1<argc ? (u64)atoll(argv[i+1]) : 0;
        Str argv0  = pers==PERS_NATIVE ? Str("sh") : Str("bash");
        Str args[] = {argv0, "-c", script, "bash"};
        Result r = runshell(&t, args, countof(args), {}, pers);
        if (spec) {
            #if _WIN32
            _setmode(1, _O_BINARY);  // the bytes exactly, CRs included
            #endif
            // The spec format (tools/README.md): the vos captures both
            // streams apart and merged in the one run
            Str names[] = {"stdout", "stderr", "merged"};
            Str parts[] = {r.out, r.err, r.merged};
            for (i32 k = 0; k < 3; k++) {
                printf("== %.*s %lld\n", (int)names[k].len, (char *)names[k].data,
                       (long long)parts[k].len);
                printstr(stdout, parts[k]);
                printf("\n");
            }
            printf("== status %d\n", r.status);
        } else {
            printstr(stdout, r.merged);
            printf("[status %d]\n", r.status);
        }
        return 0;
    }

    if (argc>1 && !strcmp(argv[1], "--fixed")) {
        // Replay fuzzer inputs through the print/reparse fixed point,
        // with extglob chosen by the first byte as in main_fuzz.cpp.
        i32 bad = 0;
        for (i32 i = 2; i < argc; i++) {
            Str src = readfile(argv[i]);
            if (!src.data) continue;
            os.heapused = 0;
            Arena a = newarena(&os, (iz)1<<24);
            Fixed f = fixedpoint(&a, src, src.len && src[0]&1);
            if (f.status==FIX_REPARSE || f.status==FIX_DIFFER) {
                bad++;
                printf("== %s (%s)\n--- first\n", argv[i],
                       f.status==FIX_REPARSE ? "reparse" : "differ");
                printstr(stdout, f.first);
                printf("\n--- second\n");
                printstr(stdout, f.second);
                printf("\n");
            }
            free(src.data);
        }
        printf("%d of %d fail\n", bad, argc-2);
        return !!bad;
    }

    i32 specs    = 0;
    i32 specfail = 0;
    for (i32 i = 1; i < argc; i++) {
        Str a = {(u8 *)argv[i], (iz)strlen(argv[i])};
        if (a=="--chaos" && i+1<argc) {
            chaosruns = atoi(argv[++i]);
            continue;
        }
        if (a == "--msys-diff") {
            msysdiff = 1;
            continue;
        }
        if (endswith(a, ".sh") && find(a, "msys/")>=0) {
            // Git Bash recordings, expected to fail until M5a's steps 5
            // to 7: counted on their own
            test_msys(&t, argv[i]);
            continue;
        }
        if (endswith(a, ".sh")) {
            specs++;
            specfail += !test_spec(&t, argv[i]);
        }
        if (endswith(a, ".ed")) {
            specs++;
            specfail += !test_edit(&t, argv[i]);
        }
    }

    if (specs) {
        printf("%d specs, %d failed\n", specs, specfail);
    }
    if (msyspass || msysfail) {
        printf("%d t/msys vos specs, %d passed, %d failed (%d not # vos)\n",
               msyspass+msysfail, msyspass, msysfail, msysskip);
    }
    printf("%d tests, %d failed\n", t.run, t.failed);
    return t.failed || msysfail;
}
