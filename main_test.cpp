// Tests: a virtual OS layer, so no test touches real files or processes
// $ g++ -std=c++20 -g3 -fsanitize=undefined -fsanitize-trap -o tests main_test.cpp
//
// The shell runs repeatedly in one process. Memory comes from a fixed
// heap that is poisoned and reset between runs, and output is captured.
//
// This is free and unencumbered software released into the public domain.
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unabashed.cpp"
#include "fibers.cpp"
#include "check.cpp"

struct Os {
    Fibers   fibers;
    byte    *heap;     // backs every reservation
    iz       heapcap;
    iz       heapused;
    Str      out;      // captured standard output
    Str      err;      // captured standard error
    Str      merged;   // both, in write order
    iz       outcap;
};

static Fibers *osfibers(Os *os)
{
    return &os->fibers;
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

static byte *os_reserve(Os *os, iz len)
{
    iz pad = -os->heapused & 63;
    if (len > os->heapcap - os->heapused - pad) {
        return 0;
    }
    byte *r = os->heap + os->heapused + pad;
    os->heapused += pad + len;
    memset(r, 0x55, (uz)len);  // expose reads of uninitialized memory
    return r;
}

static b32 os_commit(Os *os, byte *p, iz len)
{
    assert(p>=os->heap && p+len<=os->heap+os->heapcap);
    return 1;
}

static b32 os_write(Os *os, Handle h, u8 *buf, iz len)
{
    Str *dst = h==1 ? &os->out : h==2 ? &os->err : 0;
    if (!dst || len > os->outcap - dst->len) {
        return 0;
    }
    memcpy(dst->data+dst->len, buf, (uz)len);
    dst->len += len;
    if (len <= os->outcap - os->merged.len) {
        memcpy(os->merged.data+os->merged.len, buf, (uz)len);
        os->merged.len += len;
    }
    return 1;
}

static iz os_read(Os *, Handle, u8 *, iz)
{
    return 0;
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

// Run the shell with a fresh virtual OS state. The first argument is
// $0, conventionally "bash".
static Result runshell(Test *t, Str *args, iz nargs)
{
    Os *os = t->os;
    os->heapused = 0;
    os->out.len    = 0;
    os->err.len    = 0;
    os->merged.len = 0;
    os->fibers     = {};

    OsInfo info = {};
    info.args    = args;
    info.nargs   = nargs;
    info.std[0]  = 0;
    info.std[1]  = 1;
    info.std[2]  = 2;
    info.reserve = (iz)1<<22;

    Result r = {};
    r.status = sh_main(os, &info);
    r.out    = os->out;
    r.err    = os->err;
    r.merged = os->merged;
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

// Spec tests: run a script recorded by tools/record and compare all
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

struct Spec {
    Str out;
    Str err;
    Str merged;
    i32 status;
    b32 ok;
};

static Spec parsespec(Str s)
{
    Spec r = {};
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
    Spec want = parsespec(spec);
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

int main(int argc, char **argv)
{
    Os os = {};
    os.heapcap     = (iz)1<<26;
    os.heap        = (byte *)malloc((uz)os.heapcap);
    os.outcap      = (iz)1<<20;
    os.out.data    = (u8 *)malloc((uz)os.outcap);
    os.err.data    = (u8 *)malloc((uz)os.outcap);
    os.merged.data = (u8 *)malloc((uz)os.outcap);

    Test t = {};
    t.os = &os;
    test_m0(&t);
    test_fibers(&t);
    test_arena(&t);

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
