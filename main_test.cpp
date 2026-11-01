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
#include "unabashed.cpp"
#include "fibers.cpp"
#include "check.cpp"
#include "fakes.cpp"
#include "vos.cpp"

// A nested shell's share of the heap, released when it exits if
// nothing reserved since then outlives it.
struct HeapMark {
    iz  save;
    i32 proc;     // its vos process
    b32 dead;
};

struct Os {
    Fibers   fibers;
    byte    *heap;     // backs every reservation
    iz       heapcap;
    iz       heapused;
    iz       pinned;   // reserved by others while nested shells run
    HeapMark marks[64];
    i32      nmarks;
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

static byte *os_reserve(Os *os, iz len)
{
    iz pad = -os->heapused & 63;
    if (len > reservelimit(os) - os->heapused - pad) {
        return 0;
    }
    byte *r = os->heap + os->heapused + pad;
    os->heapused += pad + len;
    if (os->nmarks && os->marks[os->nmarks-1].proc!=os->vos.cur) {
        os->pinned = os->heapused;
    }
    memset(r, 0x55, (uz)len);  // expose reads of uninitialized memory
    return r;
}

static b32 os_commit(Os *os, byte *p, iz len)
{
    assert(p>=os->heap && p+len<=os->heap+os->heapcap);
    return 1;
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
    info.pid     = osvos(os)->procs[osvos(os)->cur].pid;
    info.sigign  = osvos(os)->procs[osvos(os)->cur].sigign;
    info.ppid    = 2;
    info.uid     = info.euid = 1000;
    info.gid     = info.egid = 1000;
    info.host    = "oracle";
    // Its own references: exec may close them before the process exits
    for (i32 i = 0; i < 3; i++) info.std[i] = os_dup(os, ctx->fd[i]);
    info.ownstd  = 1;
    info.reserve = (iz)1<<22;
    i32 status = sh_main(os, &info);
    os->marks[m].dead = 1;
    while (os->nmarks && os->marks[os->nmarks-1].dead) {
        iz save = os->marks[--os->nmarks].save;
        os->heapused = save>os->pinned ? save : os->pinned;
    }
    return status;
}

static Fake testfakes[256];
static iz   ntestfakes;

static void initfakes()
{
    ntestfakes = 0;
    testfakes[ntestfakes++] = {"bash", fake_bash};
    for (iz i = 0; i < countof(fakes) && ntestfakes < countof(testfakes); i++) {
        testfakes[ntestfakes++] = fakes[i];
    }
}

// Run the shell with a fresh virtual OS state. The first argument is
// $0, conventionally "bash".
static Result runshell(Test *t, Str *args, iz nargs, Str input = {})
{
    Os *os = t->os;
    os->heapused = 0;
    os->pinned   = 0;
    os->nmarks   = 0;

    // The vos lives in the upper half of the heap, the shell below
    Arena va = {};
    va.beg = os->heap + os->heapcap/2;
    va.end = os->heap + os->heapcap;
    vreset(&os->vos, va, testfakes, ntestfakes);
    os->vos.in    = input;
    os->vos.chaos = os->chaos;

    static Str env[] = {
        "LANG=C", "TZ=UTC", "HOME=/home/oracle",
        "PATH=/usr/local/bin:/opt/fakes/bin",
    };
    OsInfo info = {};
    info.args    = args;
    info.nargs   = nargs;
    info.env     = env;
    info.nenv    = countof(env);
    info.cwd     = "/home/oracle";
    info.pid     = 2;
    info.ppid    = 1;
    info.uid     = info.euid = 1000;
    info.gid     = info.egid = 1000;
    info.host    = "oracle";
    info.std[0]  = 0;
    info.std[1]  = 1;
    info.std[2]  = 2;
    info.reserve = (iz)1<<22;
    info.stack   = (iz)1<<20;
    info.chaos   = os->chaos;

    Result r = {};
    r.status = sh_main(os, &info) & 255;
    vfinish(os);
    r.out    = os->vos.out;
    r.err    = os->vos.err;
    r.merged = os->vos.merged;
    return r;
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

static void test_arena(Test *t)
{
    Os *os = t->os;
    os->heapused = 0;

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
    os->heapused = 0;
    Arena va = {};
    va.beg = os->heap + os->heapcap/2;
    va.end = os->heap + os->heapcap;
    vreset(&os->vos, va, testfakes, ntestfakes);
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

    Str    args[] = {"bash", "-c", script, "bash"};
    Result r      = runshell(t, args, countof(args));
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
        r = runshell(t, args, countof(args));
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
        t->os->heapused = 0;
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
    os->heapused = 0;
    os->pinned   = 0;
    os->nmarks   = 0;
    Arena va = {};
    va.beg = os->heap + os->heapcap/2;
    va.end = os->heap + os->heapcap;
    vreset(&os->vos, va, testfakes, ntestfakes);
    // run-all runs from a terminal, where nobody types: some tests time
    // out reads from it
    os->vos.ttyin = 1;
    // The suite expects a populated /bin, as on a real system: the
    // fakes stand in for it, and the shell for /bin/sh
    for (iz i = 0; i < ntestfakes; i++) {
        Str name = testfakes[i].name;
        if (name=="recho" || name=="zecho" || name=="xcase") continue;
        vinstall(&os->vos, "usr/bin", name, (i32)i);
        if (name == "bash") vinstall(&os->vos, "usr/bin", "sh", (i32)i);
    }
    VNode *home = vmkdirs(&os->vos, "home/oracle/tests");
    vimport(&os->vos, home, suite, 0);
    VNode *build = home->parent;  // BUILD_DIR
    vbuildfile(&os->vos, build, "y.tab.c",   100000);
    vbuildfile(&os->vos, build, "config.h",  20000);
    vbuildfile(&os->vos, build, "version.h", 2000);

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
    os.heapcap         = (iz)1<<29;  // half for the shells: room for background vprocs
    os.heap            = (byte *)malloc((uz)os.heapcap);
    os.vos.outcap      = (iz)1<<20;
    os.vos.out.data    = (u8 *)malloc((uz)os.vos.outcap);
    os.vos.err.data    = (u8 *)malloc((uz)os.vos.outcap);
    os.vos.merged.data = (u8 *)malloc((uz)os.vos.outcap);
    initfakes();

    Test t = {};
    t.os = &os;
    test_m0(&t);
    test_fibers(&t);
    test_arena(&t);
    test_sched(&t);

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
        // Run one script on the vos and show what it printed, with an
        // optional chaos seed
        Str script = readfile(argv[2]);
        os.chaos = argc>3 ? (u64)atoll(argv[3]) : 0;
        Str args[] = {"bash", "-c", script, "bash"};
        Result r = runshell(&t, args, countof(args));
        printstr(stdout, r.merged);
        printf("[status %d]\n", r.status);
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
        if (endswith(a, ".sh")) {
            specs++;
            specfail += !test_spec(&t, argv[i]);
        }
    }

    if (specs) {
        printf("%d specs, %d failed\n", specs, specfail);
    }
    printf("%d tests, %d failed\n", t.run, t.failed);
    return !!t.failed;
}
