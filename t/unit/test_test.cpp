// Unit tests for b_test.cpp against cases recorded from bash 5.3
//   $ g++ -std=c++20 -g3 -Wall -Wextra -Wconversion -Wno-sign-conversion
//         -fsanitize=undefined -fsanitize-trap -o test_test.exe
//         t/unit/test_test.cpp
//   $ ./test_test.exe [t/unit/test.golden]
//
// test.golden begins with the oracle's file tree ("@stat" lines, from
// stat with and without following links) and ids ("@id"), which the
// fake TestEnv serves, then pairs of lines: a test or [ command in
// shell quoting, and "STATUS[ MESSAGE]". Regenerate with test_record.sh.
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"  // unused foundations
#include "../../src/base.cpp"
#include "../../src/api.cpp"
#pragma GCC diagnostic pop
#include "../../src/b_test.cpp"

[[noreturn]] static void os_oom()
{
    fprintf(stderr, "out of memory\n");
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}


// Fake environment

struct FakeStat {
    Str  name;
    b32  follow;
    Stat st;
};

struct Fake {
    FakeStat stats[128];
    iz       nstats;
    b32      posix;
    b32      tty1;   // fd 1 is a terminal
};

static Stat fakestat(void *ctx, Str path, b32 follow)
{
    Fake *f = (Fake *)ctx;
    for (iz i = 0; i < f->nstats; i++) {
        if (f->stats[i].name==path && f->stats[i].follow==follow) {
            return f->stats[i].st;
        }
    }
    Stat st = {};
    st.err = E_NOENT;
    return st;
}

static i64 fakeeuid = -1;
static i64 fakeegid = -1;

// Permission check for a non-root user, as the kernel does it.
static b32 fakeaccess(void *ctx, Str path, i32 mode)
{
    Stat st = fakestat(ctx, path, 1);
    if (st.err) {
        return 0;
    }
    i32 bits = st.mode;
    if (st.uid == fakeeuid) {
        bits >>= 6;
    } else if (st.gid == fakeegid) {
        bits >>= 3;
    }
    return (bits & mode) == mode;
}

static b32 fakeisatty(void *ctx, i64 fd)
{
    Fake *f = (Fake *)ctx;
    return f->tty1 && fd==1;
}

// Options as "set -o" reports them in a non-interactive bash -c.
static i32 fakeoption(void *ctx, Str name)
{
    Fake *f = (Fake *)ctx;
    static Str on[] = {"braceexpand", "hashall", "interactive-comments"};
    static Str off[] = {
        "allexport", "emacs", "errexit", "errtrace", "functrace",
        "histexpand", "history", "ignoreeof", "keyword", "monitor",
        "noclobber", "noexec", "noglob", "nolog", "notify", "nounset",
        "onecmd", "physical", "pipefail", "privileged", "verbose", "vi",
        "xtrace",
    };
    if (name == "posix") return f->posix;
    for (iz i = 0; i < countof(on); i++) {
        if (name == on[i]) return 1;
    }
    for (iz i = 0; i < countof(off); i++) {
        if (name == off[i]) return 0;
    }
    return -1;
}

// Variables from test_oracle.sh's setup, with no positional parameters.
static b32 fakevarset(void *, Str name)
{
    static Str set[] = {"x", "empty", "arr", "arr[0]", "arr[1]", "as[k]", "ref"};
    for (iz i = 0; i < countof(set); i++) {
        if (name == set[i]) return 1;
    }
    TestNum n = testparseint(name);
    return n.ok && n.value==0;
}

static b32 fakenameref(void *, Str name)
{
    return name=="ref" || name=="badref";
}

static TestEnv fakeenv(Fake *f)
{
    TestEnv env = {};
    env.ctx     = f;
    env.stat    = fakestat;
    env.access  = fakeaccess;
    env.isatty  = fakeisatty;
    env.option  = fakeoption;
    env.varset  = fakevarset;
    env.nameref = fakenameref;
    env.euid    = fakeeuid;
    env.egid    = fakeegid;
    return env;
}


// Golden file parsing

static Str nextline(Str *s)
{
    Cut c = cut(*s, '\n');
    *s = c.tail;
    return c.head;
}

static Str nextfield(Str *s)
{
    Str t = trimleft(*s);
    Cut c = cut(t, ' ');
    *s = c.tail;
    return c.head;
}

static i64 parsenum(Str s, i32 base)
{
    i64 v   = 0;
    b32 neg = s.len && s[0]=='-';
    for (iz i = neg; i < s.len; i++) {
        v = v*base + (s[i] - '0');
    }
    return neg ? -v : v;
}

static i32 hexval(u8 c)
{
    if (digit(c)) return c - '0';
    c = lowercase(c);
    if (c>='a' && c<='f') return c - 'a' + 10;
    return -1;
}

// Split a line of shell words: plain, '...', "..." and $'...'.
static Slice<Str> splitwords(Arena *a, Str s)
{
    Slice<Str> words = {};
    iz i = 0;
    for (;;) {
        for (; i<s.len && (s[i]==' ' || s[i]=='\t'); i++) {}
        if (i == s.len) break;
        Buf b(a, 64);
        for (; i<s.len && s[i]!=' ' && s[i]!='\t';) {
            u8 c = s[i++];
            if (c == '\'') {
                for (; i<s.len && s[i]!='\''; i++) putbyte(&b, s[i]);
                i++;
            } else if (c == '"') {
                for (; i<s.len && s[i]!='"'; i++) {
                    if (s[i]=='\\' && i+1<s.len && find("\"\\$`", takehead(cuthead(s, i+1), 1))>=0) i++;
                    putbyte(&b, s[i]);
                }
                i++;
            } else if (c=='$' && i<s.len && s[i]=='\'') {
                for (i++; i<s.len && s[i]!='\''; i++) {
                    if (s[i]!='\\' || i+1==s.len) {
                        putbyte(&b, s[i]);
                        continue;
                    }
                    u8 e = s[++i];
                    switch (e) {
                    case 'n': putbyte(&b, '\n'); break;
                    case 't': putbyte(&b, '\t'); break;
                    case 'v': putbyte(&b, '\v'); break;
                    case 'f': putbyte(&b, '\f'); break;
                    case 'r': putbyte(&b, '\r'); break;
                    case 'a': putbyte(&b, '\a'); break;
                    case 'e': putbyte(&b, 27);   break;
                    case 'x': {
                        i32 v = 0;
                        for (i32 k = 0; k<2 && i+1<s.len && hexval(s[i+1])>=0; k++) {
                            v = v*16 + hexval(s[++i]);
                        }
                        putbyte(&b, (u8)v);
                    } break;
                    default: putbyte(&b, e);
                    }
                }
                i++;
            } else if (c=='\\' && i<s.len) {
                putbyte(&b, s[i++]);
            } else {
                putbyte(&b, c);
            }
        }
        words = push(a, words, finish(&b));
    }
    return words;
}

static i32 typecode(Str s)
{
    if (s == "reg")  return FT_FILE;
    if (s == "dir")  return FT_DIR;
    if (s == "lnk")  return FT_LINK;
    if (s == "fifo") return FT_FIFO;
    if (s == "sock") return FT_SOCK;
    if (s == "chr")  return FT_CHAR;
    if (s == "blk")  return FT_BLOCK;
    return -1;
}

static void parsestat(Fake *f, Str line)
{
    nextfield(&line);  // @stat
    FakeStat *fs = f->stats + f->nstats++;
    assert(f->nstats <= countof(f->stats));
    fs->name   = nextfield(&line);
    fs->follow = (b32)parsenum(nextfield(&line), 10);
    Str type   = nextfield(&line);
    if (type == "none") {
        fs->st.err = E_NOENT;
        return;
    }
    fs->st.type  = typecode(type);
    fs->st.mode  = (i32)parsenum(nextfield(&line), 8);
    fs->st.uid   = parsenum(nextfield(&line), 10);
    fs->st.gid   = parsenum(nextfield(&line), 10);
    fs->st.size  = parsenum(nextfield(&line), 10);
    fs->st.mtime = parsenum(nextfield(&line), 10) * 1000000000;
    fs->st.atime = parsenum(nextfield(&line), 10) * 1000000000;
    fs->st.dev   = parsenum(nextfield(&line), 10);
    fs->st.ino   = parsenum(nextfield(&line), 10);
}


// Runner

static i32 ntests;
static i32 nfailed;

static void printstr(FILE *out, Str s)
{
    fwrite(s.data, 1, (size_t)s.len, out);
}

static void check(Str what, TestResult got, i32 status, Str err)
{
    ntests++;
    if (got.status==status && got.err==err) {
        return;
    }
    nfailed++;
    fputs("FAIL: ", stderr);
    printstr(stderr, what);
    fprintf(stderr, "\n  want %d ", status);
    printstr(stderr, err);
    fprintf(stderr, "\n  got  %d ", got.status);
    printstr(stderr, got.err);
    fputs("\n", stderr);
}

static void checkb(char const *what, b32 got, b32 want)
{
    ntests++;
    if (!got == !want) {
        return;
    }
    nfailed++;
    fprintf(stderr, "FAIL: %s: want %d, got %d\n", what, !!want, !!got);
}

static void rungolden(Arena scratch, Str golden)
{
    Fake *f = alloc<Fake>(&scratch);
    while (golden.len) {
        Str line = nextline(&golden);
        if (startswith(line, "@stat ")) {
            parsestat(f, line);
            continue;
        } else if (startswith(line, "@id ")) {
            nextfield(&line);
            fakeeuid = parsenum(nextfield(&line), 10);
            fakeegid = parsenum(nextfield(&line), 10);
            continue;
        } else if (!line.len) {
            continue;
        }

        Arena tmp = scratch;
        Str   cmd = line;
        Str   res = nextline(&golden);
        f->posix  = startswith(line, "#posix ");
        if (f->posix) {
            cmd = cuthead(cmd, 7);
        }

        Cut st = cut(res, ' ');
        i32 status = (i32)parsenum(st.head, 10);
        Str err = st.tail;

        Slice<Str> w = splitwords(&tmp, cmd);
        TestEnv env = fakeenv(f);
        TestResult got = {};
        if (w.len && w[0]=="test") {
            got = testcmd(&env, &tmp, w.data+1, w.len-1);
        } else if (w.len && w[0]=="[") {
            got = testbracket(&env, &tmp, w.data+1, w.len-1);
        } else {
            nfailed++;
            fputs("bad case: ", stderr);
            printstr(stderr, line);
            fputs("\n", stderr);
            continue;
        }
        check(line, got, status, err);
    }
}

// Direct checks of the helpers and of paths the oracle cannot reach.
static void rundirect(Arena scratch)
{
    static struct { Str s; i64 v; b32 ok; } ints[] = {
        {"0", 0, 1}, {"-0", 0, 1}, {"+7", 7, 1}, {" \t12\n", 12, 1},
        {"9223372036854775807", 9223372036854775807, 1},
        {"-9223372036854775808", (i64)((u64)1<<63), 1},
        {"9223372036854775808", 0, 0}, {"-9223372036854775809", 0, 0},
        {"", 0, 0}, {" ", 0, 0}, {"-", 0, 0}, {"+ 1", 0, 0}, {"1 1", 0, 0},
        {"0x1", 0, 0}, {"1a", 0, 0}, {"\v\f\r5\v", 5, 1},
    };
    for (iz i = 0; i < countof(ints); i++) {
        TestNum n = testparseint(ints[i].s);
        checkb("testparseint ok", n.ok, ints[i].ok);
        checkb("testparseint value", n.value==ints[i].v, 1);
    }

    static Str unops[] = {
        "-a", "-b", "-c", "-d", "-e", "-f", "-g", "-h", "-k", "-n", "-o",
        "-p", "-r", "-s", "-t", "-u", "-v", "-w", "-x", "-z", "-G", "-L",
        "-N", "-O", "-R", "-S",
    };
    for (iz i = 0; i < countof(unops); i++) {
        checkb("testunaryop", testunaryop(unops[i]), 1);
    }
    static Str notunops[] = {"-", "--", "-i", "-E", "-ab", "a", "", "-nt", "!"};
    for (iz i = 0; i < countof(notunops); i++) {
        checkb("not testunaryop", testunaryop(notunops[i]), 0);
    }

    static Str binops[] = {
        "=", "==", "!=", "<", ">", "-eq", "-ne", "-lt", "-le", "-gt",
        "-ge", "-nt", "-ot", "-ef",
    };
    for (iz i = 0; i < countof(binops); i++) {
        checkb("testbinaryop", testbinaryop(binops[i], 0), 1);
        checkb("testbinaryop cond", testbinaryop(binops[i], 1), 1);
    }
    checkb("=~ test", testbinaryop("=~", 0), 0);
    checkb("=~ cond", testbinaryop("=~", 1), 1);
    static Str notbinops[] = {"-a", "-o", "===", "!~", "-EQ", "-e", "<=", ""};
    for (iz i = 0; i < countof(notbinops); i++) {
        checkb("not testbinaryop", testbinaryop(notbinops[i], 1), 0);
    }

    checkb("intcmp eq",  testintcmp("-eq", 3, 3), 1);
    checkb("intcmp ne",  testintcmp("-ne", 3, 3), 0);
    checkb("intcmp lt",  testintcmp("-lt", -1, 0), 1);
    checkb("intcmp le",  testintcmp("-le", 1, 0), 0);
    checkb("intcmp gt",  testintcmp("-gt", 1, 0), 1);
    checkb("intcmp ge",  testintcmp("-ge", 0, 0), 1);
    checkb("intcmp min", testintcmp("-lt", (i64)((u64)1<<63), 0), 1);

    Fake *f = alloc<Fake>(&scratch);
    f->tty1 = 1;
    TestEnv env = fakeenv(f);
    check("-t 1 on a tty", testunary(&env, &scratch, "-t", "1"), 0, "");
    check("-t 2 on a tty", testunary(&env, &scratch, "-t", "2"), 1, "");
    check("-t wraps", testunary(&env, &scratch, "-t", "4294967297"), 1, "");
    check("-t junk", testunary(&env, &scratch, "-t", "x"), 2, "x: integer expected");
    check("-q", testunary(&env, &scratch, "-q", "x"), 2, "-q: unary operator expected");
    check("=~", testbinary(&env, &scratch, "=~", "a", "b"), 2, "=~: binary operator expected");
    check("<", testbinary(&env, &scratch, "<", "a", "b"), 0, "");
    check("> high", testbinary(&env, &scratch, ">", "\xc3\xa9", "z"), 0, "");
    check("-eq", testbinary(&env, &scratch, "-eq", " 1", "1 "), 0, "");
    check("-eq rhs", testbinary(&env, &scratch, "-eq", "1", "y"), 2, "y: integer expected");

    Str a1[] = {"-t"};
    check("test -t", testcmd(&env, &scratch, a1, 1), 0, "");
    Str a2[] = {"-t", "-a", "x", "-o", "y"};
    check("test -t -a x -o y", testcmd(&env, &scratch, a2, 5), 0, "");
    Str a3[] = {"!", "-t", "1"};
    check("test ! -t 1", testcmd(&env, &scratch, a3, 3), 1, "");
    Str a4[] = {"x", "-a", "-t", "-o", "y"};
    check("test x -a -t -o y", testcmd(&env, &scratch, a4, 5), 0, "");
    Str a5[] = {"]"};
    check("[ ]", testbracket(&env, &scratch, a5, 1), 1, "");
    check("[", testbracket(&env, &scratch, 0, 0), 2, testnobracket);
}

// Long and deeply nested expressions: results as recorded from bash 5.3,
// except past TEST_MAXNEST, where bash keeps recursing.
static void rundeep(Arena scratch)
{
    Fake   *f   = alloc<Fake>(&scratch);
    TestEnv env = fakeenv(f);

    // ! ( ! ( ... ! ( X ) ... ) )
    static struct { i32 n; Str x; i32 status; } nots[] = {
        {1023, "x", 1}, {1023, "", 0}, {1024, "x", 0}, {1024, "", 1},
        {1025, "x", 1}, {1025, "", 0},
    };
    for (iz i = 0; i < countof(nots); i++) {
        Arena tmp  = scratch;
        i32   n    = nots[i].n;
        iz    len  = 3*(iz)n + 1;
        Str  *args = alloc<Str>(&tmp, len);
        for (i32 k = 0; k < n; k++) {
            args[2*k]       = "!";
            args[2*k+1]     = "(";
            args[len-1-k]   = ")";
        }
        args[2*n] = nots[i].x;
        check("! ( nested", testcmd(&env, &tmp, args, len), nots[i].status, "");
    }

    // x -a ( ( ... ( '' -o y ) ... ) ), nested one group too deep at the end
    for (i32 n = 1022; n <= 1025; n++) {
        Arena tmp  = scratch;
        iz    len  = 2*(iz)n + 5;
        Str  *args = alloc<Str>(&tmp, len);
        args[0] = "x";
        args[1] = "-a";
        for (i32 k = 0; k < n; k++) {
            args[2+k]     = "(";
            args[len-1-k] = ")";
        }
        args[2+n] = "";
        args[3+n] = "-o";
        args[4+n] = "y";
        if (n <= TEST_MAXNEST) {
            check("( nested", testcmd(&env, &tmp, args, len), 0, "");
        } else {
            check("( too deep", testcmd(&env, &tmp, args, len), 2,
                  "expression recursion level exceeded");
        }
    }

    // A long flat expression must not recurse per operator.
    {
        Arena tmp  = scratch;
        iz    n    = 100000;
        iz    len  = 4*n + 3;
        Str  *args = alloc<Str>(&tmp, len);
        args[0] = "x";
        for (iz k = 0; k < n; k++) {
            args[1+4*k] = "-a";
            args[2+4*k] = "x";
            args[3+4*k] = "-o";
            args[4+4*k] = "";
        }
        check("flat", testcmd(&env, &tmp, args, len-2), 0, "");
        args[len-2] = "-a";
        args[len-1] = "";
        check("flat -a ''", testcmd(&env, &tmp, args, len), 0, "");
        args[len-1] = "1";
        args[len-2] = "-eq";  // x -a ... -o '' -eq 1: '' is not an integer
        check("flat error", testcmd(&env, &tmp, args, len), 2, ": integer expected");
    }

    // Many unclosed groups: the nesting bound also bounds the rescans.
    {
        Arena tmp  = scratch;
        iz    len  = 20000;
        Str  *args = alloc<Str>(&tmp, len);
        for (iz k = 0; k < len; k++) {
            args[k] = "(";
        }
        check("unclosed", testcmd(&env, &tmp, args, len), 2,
              "expression recursion level exceeded");
    }

    // A long run of ! is a loop.
    {
        Arena tmp  = scratch;
        iz    len  = 200000;
        Str  *args = alloc<Str>(&tmp, len);
        for (iz k = 0; k < len; k++) {
            args[k] = "!";
        }
        check("many !", testcmd(&env, &tmp, args, len), 2, "argument expected");
        args[len-1] = "x";  // an odd number of !
        check("many ! x", testcmd(&env, &tmp, args, len), 1, "");
    }
}

static Str readfile(Arena *a, char const *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return {};
    }
    iz  cap = 1<<22;
    u8 *buf = allocbytes(a, cap);
    iz  len = (iz)fread(buf, 1, (size_t)cap, fp);
    fclose(fp);
    if (len == cap) {
        return {};  // too large
    }
    Str s = {buf, len};
    // tolerate CRLF checkouts
    Buf b(a, len);
    for (iz i = 0; i < s.len; i++) {
        if (s[i]!='\r' || i+1==s.len || s[i+1]!='\n') putbyte(&b, s[i]);
    }
    return finish(&b);
}

int main(int argc, char **argv)
{
    static byte mem[1<<25];
    Arena a = {mem, mem+countof(mem)};

    char const *path = argc>1 ? argv[1] : "t/unit/test.golden";
    Str golden = readfile(&a, path);
    if (!golden.data) {
        fprintf(stderr, "test_test: cannot read %s\n", path);
        return 2;
    }

    rungolden(a, golden);
    rundirect(a);
    rundeep(a);

    fflush(stderr);
    printf("%d tests, %d failed\n", ntests, nfailed);
    fflush(stdout);
    return nfailed != 0;
}
