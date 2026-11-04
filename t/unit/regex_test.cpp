// Tests for regex.cpp against results recorded from glibc
// $ g++ -std=c++20 -g3 -Wall -Wextra -Wconversion -Wno-sign-conversion
//       -fsanitize=undefined -fsanitize-trap
//       -o regex_test.exe t/unit/regex_test.cpp
// $ ./regex_test.exe [t/unit/regex.golden]
//
// Run from the repository root. regex.golden is regex.cases with the
// result glibc's regcomp/regexec gave each case appended, written by
// t/unit/regex_diff.cpp --record on Linux. Besides the golden cases,
// this checks that long subjects match in linear time, that hostile
// patterns (deep nesting, intervals of intervals, long alternations,
// nested stars before a back-reference) finish or fail cleanly within
// the limits, and the error messages. Build with -Wl,--stack,1048576 to
// check that the limits fit a 1 MiB stack, as on a shell fiber.
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../base.cpp"
#include "../../pattern.cpp"
#include "../../regex.cpp"

[[noreturn]] static void os_oom()
{
    fputs("regex_test: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}

static i32 ntests;
static i32 nfailed;

static i32 hexval(u8 c)
{
    if (c>='0' && c<='9') return c - '0';
    if (c>='A' && c<='F') return c - 'A' + 10;
    if (c>='a' && c<='f') return c - 'a' + 10;
    return -1;
}

// Decode %HH escapes; %00 stands for nothing (an empty field).
static Str decode(Arena *a, Str s)
{
    Str r = {allocbytes(a, s.len), 0};
    for (iz i = 0; i < s.len; i++) {
        if (s[i]=='%' && i+2<s.len && hexval(s[i+1])>=0 && hexval(s[i+2])>=0) {
            u8 c = (u8)(hexval(s[i+1])<<4 | hexval(s[i+2]));
            i += 2;
            if (c) r.data[r.len++] = c;
        } else {
            r.data[r.len++] = s[i];
        }
    }
    return r;
}

static i32 parseflags(Str s)
{
    i32 f = 0;
    for (iz i = 0; i < s.len; i++) {
        if (s[i] == 'i') f |= REX_ICASE;
        if (s[i] == 'u') f |= REX_UTF8;
    }
    return f;
}

static void printnum(Buf *b, i64 v)
{
    char tmp[24];
    snprintf(tmp, sizeof(tmp), "%lld", (long long)v);
    print(b, Str((u8 *)tmp, (iz)strlen(tmp)));
}

// The result in regex_diff's notation, null if the budget ran out
static Str run(Arena *a, Str pat, Str s, i32 flags, i64 budget = -1)
{
    Buf    b(a, 256);
    RxComp c = rxcompile(pat, flags, a);
    if (!c.re) {
        print(&b, "err ");
        printnum(&b, c.err);
        return finish(&b);
    }
    RxMatch *m = alloc<RxMatch>(a, c.re->nsub+1);
    i32      r = rxexec(c.re, s, m, *a, budget);
    if (r < 0) return {};
    if (!r) return "nomatch";
    for (iz i = 0; i <= c.re->nsub; i++) {
        putbyte(&b, '(');
        printnum(&b, m[i].so);
        putbyte(&b, ',');
        printnum(&b, m[i].eo);
        putbyte(&b, ')');
    }
    return finish(&b);
}

static void expect(b32 ok, char const *what, Str got)
{
    ntests++;
    if (ok) return;
    nfailed++;
    fprintf(stderr, "FAIL: %s  got: %.*s\n", what, (int)got.len, got.cdata);
}

static void rungolden(char const *path, Arena perm)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "regex_test: cannot open %s\n", path);
        exit(2);
    }
    iz  cap = (iz)1 << 20;
    Str src = {allocbytes(&perm, cap), 0};
    src.len = (iz)fread(src.data, 1, (uz)cap, f);
    fclose(f);

    for (Cut c = cut(src, '\n'); c.head.len || c.tail.len; c = cut(c.tail, '\n')) {
        Str line = c.head;
        if (line.len && line[line.len-1]=='\r') line.len--;
        if (!line.len || line[0]=='#') continue;
        Str fields[4] = {};
        iz  nf = 0;
        for (Cut t = cut(line, '\t'); nf < countof(fields); t = cut(t.tail, '\t')) {
            fields[nf++] = t.head;
            if (!t.ok) break;
        }
        if (nf != 4) continue;
        Arena a   = perm;
        Str   pat = decode(&a, fields[1]);
        Str   s   = decode(&a, fields[2]);
        Str   got = run(&a, pat, s, parseflags(fields[0]));
        ntests++;
        if (got != fields[3]) {
            nfailed++;
            fprintf(stderr, "FAIL: %.*s\n  want: %.*s\n  got:  %.*s\n",
                    (int)line.len, line.cdata, (int)fields[3].len,
                    fields[3].cdata, (int)got.len, got.cdata);
        }
    }
}

static Str repeat(Arena *a, Str s, iz n)
{
    Str r = {allocbytes(a, s.len*n), s.len*n};
    for (iz i = 0; i < n; i++) copybytes(r.data+i*s.len, s.data, s.len);
    return r;
}

static Str cat(Arena *a, Str x, Str y)
{
    Str r = {allocbytes(a, x.len+y.len), x.len+y.len};
    copybytes(r.data, x.data, x.len);
    copybytes(r.data+x.len, y.data, y.len);
    return r;
}

static double seconds()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

static void hostile(Arena perm)
{
    {
        // Linear matching: a match at the end of a long subject
        Arena  a = perm;
        Str    s = cat(&a, repeat(&a, "ab", 200000), "abc");
        double t = seconds();
        Str    r = run(&a, "(a|b)*c", s, 0);
        expect(r=="(0,400003)(400001,400002)", "long (a|b)*c", r);
        r = run(&a, "([[:alpha:]]+)c$", s, 0);
        expect(r=="(0,400003)(0,400002)", "long class run", r);
        r = run(&a, "x", s, 0);
        expect(r=="nomatch", "long no match", r);
        expect(seconds()-t < 5, "long subjects take linear time", {});
    }
    {
        // Nesting at the limit, and past it
        Arena a = perm;
        Str   p = cat(&a, repeat(&a, "(", 400), "a");
        p = cat(&a, p, repeat(&a, ")", 400));
        Str r = run(&a, p, "a", 0);
        expect(r.len>6 && startswith(r, "(0,1)(0,1)"), "400 nested groups", r);
        p = cat(&a, repeat(&a, "(", 600), repeat(&a, ")", 600));
        r = run(&a, p, "a", 0);
        expect(r=="err 12", "600 nested groups", r);
    }
    {
        // Intervals of intervals: too big rather than slow
        Arena a = perm;
        Str   r = run(&a, "((a{100}){100}){200}", "a", 0);
        expect(r=="err 12", "intervals of intervals", r);
        r = run(&a, "a{1000}b", cat(&a, repeat(&a, "a", 1000), "b"), 0);
        expect(r=="(0,1001)", "a{1000}b", r);
    }
    {
        // A long alternation and a long concatenation
        Arena a = perm;
        Str   p = repeat(&a, "x|", 20000);
        p = cat(&a, p, "abc");
        Str r = run(&a, p, "zabc", 0);
        expect(r=="(1,4)", "20000 alternatives", r);
        p = repeat(&a, "a", 3000);
        r = run(&a, p, p, 0);
        expect(r=="(0,3000)", "3000 literals", r);
    }
    {
        // Back-references after repetition: exponentially many paths,
        // searched in polynomial time (each took minutes at 40 a's)
        Arena  a = perm;
        Str    s = repeat(&a, "a", 300);
        double t = seconds();
        Str    r = run(&a, "((a|aa)*)\\1\\1c", cat(&a, s, "c"), 0);
        expect(r=="(0,301)(0,100)(99,100)", "((a|aa)*)\\1\\1c", r);
        r = run(&a, "((a|aa)*)\\1\\1c", s, 0);
        expect(r=="nomatch", "((a|aa)*)\\1\\1c without c", r);
        r = run(&a, "((a|aa)*)\\1\\1", cuthead(s, 1), 0);
        expect(r=="(0,297)(0,99)(98,99)", "((a|aa)*)\\1\\1", r);
        r = run(&a, "^(a|aa)+\\1$", cat(&a, s, "b"), 0);
        expect(r=="nomatch", "^(a|aa)+\\1$", r);
        r = run(&a, "(a|b)*\\1\\1x", repeat(&a, "ab", 300), 0);
        expect(r=="nomatch", "(a|b)*\\1\\1x", r);
        r = run(&a, "(a*)*\\1b", cuthead(s, 200), 0);
        expect(r=="nomatch", "(a*)*\\1b", r);
        expect(seconds()-t < 5, "back-reference paths take polynomial time", {});
    }
    {
        // The search's frames take nearly all of 256 KiB, so the memo
        // gives its room back (it ran out of memory when it did not)
        Arena a = perm;
        a.end = a.beg + ((iz)1<<18);
        Str s = repeat(&a, "a", 2400);
        Str r = run(&a, "(b)\\1|a*c", s, 0);
        expect(r=="nomatch", "(b)\\1|a*c in 256 KiB", r);
    }
    {
        // Back-references search every path: a budget stops it
        Arena a = perm;
        Str   s = repeat(&a, "a", 30);
        Str   r = run(&a, "((a*)*)*\\1x", s, 0, 100000);
        expect(!r.data, "budget for ((a*)*)*\\1x", r);
    }
    {
        // Messages
        expect(rxerrors[RXE_EBRACK]=="Unmatched [, [^, [:, [., or [=", "EBRACK", {});
        expect(rxerrors[RXE_ERPAREN]=="Unmatched ) or \\)", "ERPAREN", {});
        expect(countof(rxerrors) == RXE_ERPAREN+1, "messages", {});
    }
}

int main(int argc, char **argv)
{
    iz    cap  = (iz)1 << 30;
    Arena perm = {};
    perm.beg = (byte *)malloc((uz)cap);
    perm.end = perm.beg + cap;
    rungolden(argc>1 ? argv[1] : "t/unit/regex.golden", perm);
    hostile(perm);
    printf("%d tests, %d failed\n", ntests, nfailed);
    return !!nfailed;
}
