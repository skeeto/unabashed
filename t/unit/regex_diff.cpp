// Differential tests of regex.cpp against glibc's regcomp/regexec
// $ g++ -std=c++20 -g3 -O1 -fsanitize=undefined -fsanitize-trap
//       -o regex_diff t/unit/regex_diff.cpp
// $ ./regex_diff --random [SEED [COUNT]]   compare on generated cases
// $ ./regex_diff --record <cases >golden   glibc's results for regex_test
// $ ./regex_diff <case                     one case (AFL), trap on a difference
//
// Linux only: glibc is the reference. Everything happens in memory; the
// only input is the case text on stdin. A case is one line of three
// tab-separated fields, flags (i: REG_ICASE, u: C.UTF-8, else C),
// pattern and subject, bytes outside ! to ~ and % written %HH (%00 is
// an empty field). A result is "err N", "nomatch", or the registers as
// (so,eo) pairs. AFL input is raw: a flags byte (bit 0 icase, bit 1
// UTF-8), the pattern, a newline, the subject; NUL bytes are dropped.
//
// Known differences (skipped): in UTF-8, classes, \w \s, word
// boundaries and case folding of non-ASCII characters, which glibc
// takes from its Unicode tables and regex.cpp treats as non-letters.
//
// This is free and unencumbered software released into the public domain.
#include <locale.h>
#include <regex.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../base.cpp"
#include "../../pattern.cpp"
#include "../../regex.cpp"

[[noreturn]] static void os_oom()
{
    fputs("regex_diff: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}

static i32 hexval(u8 c)
{
    if (c>='0' && c<='9') return c - '0';
    if (c>='A' && c<='F') return c - 'A' + 10;
    if (c>='a' && c<='f') return c - 'a' + 10;
    return -1;
}

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

static void encode(Buf *b, Str s)
{
    static char hex[] = "0123456789ABCDEF";
    if (!s.len) print(b, "%00");
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c<0x21 || c>0x7e || c=='%') {
            putbyte(b, '%');
            putbyte(b, hex[c>>4]);
            putbyte(b, hex[c&15]);
        } else {
            putbyte(b, c);
        }
    }
}

static char *cstr(Arena *a, Str s)
{
    char *r = (char *)allocbytes(a, s.len+1);
    copybytes(r, s.data, s.len);
    r[s.len] = 0;
    return r;
}

static void printnum(Buf *b, i64 v)
{
    char tmp[24];
    snprintf(tmp, sizeof(tmp), "%lld", (long long)v);
    print(b, Str((u8 *)tmp, (iz)strlen(tmp)));
}

static i32 curlocale = -1;

static void uselocale(i32 utf8)
{
    if (curlocale == utf8) return;
    curlocale = utf8;
    if (!setlocale(LC_ALL, utf8 ? "C.UTF-8" : "C")) {
        fputs("regex_diff: no C.UTF-8 locale\n", stderr);
        exit(2);
    }
}

// glibc overflows its stack on some patterns, (|b)[a-]\1{,2}+ for
// one, where bash crashes too. Such cases are skipped: the overflow
// lands on an alternate stack, which jumps back.
static sigjmp_buf crashjmp;

static void oncrash(int)
{
    siglongjmp(crashjmp, 1);
}

static void catchcrashes()
{
    static char altstack[1<<16];
    stack_t ss = {};
    ss.ss_sp   = altstack;
    ss.ss_size = sizeof(altstack);
    sigaltstack(&ss, 0);
    struct sigaction sa = {};
    sa.sa_handler = oncrash;
    sa.sa_flags   = SA_ONSTACK | SA_NODEFER;
    sigaction(SIGSEGV, &sa, 0);
}

// glibc's result for a case, or a null string if it crashed
static Str glibcrun(Arena *a, Str pat, Str s, i32 flags)
{
    uselocale(!!(flags & REX_UTF8));
    regex_t re = {};
    i32 cf = REG_EXTENDED | ((flags & REX_ICASE) ? REG_ICASE : 0);
    if (sigsetjmp(crashjmp, 1)) return {};
    i32 e  = regcomp(&re, cstr(a, pat), cf);
    Buf b(a, 256);
    if (e) {
        print(&b, "err ");
        printnum(&b, e);
        return finish(&b);
    }
    iz n = (iz)re.re_nsub + 1;
    regmatch_t *m = alloc<regmatch_t>(a, n);
    if (regexec(&re, cstr(a, s), (size_t)n, m, 0)) {
        regfree(&re);
        return "nomatch";
    }
    for (iz i = 0; i < n; i++) {
        putbyte(&b, '(');
        printnum(&b, m[i].rm_so);
        putbyte(&b, ',');
        printnum(&b, m[i].rm_eo);
        putbyte(&b, ')');
    }
    regfree(&re);
    return finish(&b);
}

// regex.cpp's result, or a null string when it gave up
static Str ourrun(Arena *a, Str pat, Str s, i32 flags)
{
    Buf    b(a, 256);
    RxComp c = rxcompile(pat, flags, a);
    if (!c.re) {
        print(&b, "err ");
        printnum(&b, c.err);
        return finish(&b);
    }
    RxMatch *m = alloc<RxMatch>(a, c.re->nsub+1);
    i32      r = rxexec(c.re, s, m, *a, 2000000);
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

// Cases where a difference is a documented gap
static b32 knowngap(Str pat, Str s, i32 flags)
{
    if (!(flags & REX_UTF8)) return 0;
    if (allascii(s) && allascii(pat)) return 0;
    if (flags & REX_ICASE) return 1;
    for (iz i = 0; i+1 < pat.len; i++) {
        if (pat[i]=='[' && pat[i+1]==':') return 1;
        if (pat[i] == '\\') {
            u8 c = pat[++i];
            if (c=='w' || c=='W' || c=='s' || c=='S' || c=='b' || c=='B' ||
                    c=='<' || c=='>') return 1;
        }
    }
    return 0;
}

static void showcase(FILE *f, Str pat, Str s, i32 flags)
{
    Arena a = {};
    static byte mem[1<<16];
    a.beg = mem;
    a.end = mem + sizeof(mem);
    Buf b(&a, 1024);
    if (flags & REX_ICASE) putbyte(&b, 'i');
    if (flags & REX_UTF8)  putbyte(&b, 'u');
    if (!(flags & (REX_ICASE|REX_UTF8))) putbyte(&b, '-');
    putbyte(&b, '\t');
    encode(&b, pat);
    putbyte(&b, '\t');
    encode(&b, s);
    Str line = finish(&b);
    fwrite(line.data, 1, (uz)line.len, f);
}

// Compare one case; 0 on a difference.
static b32 check(Arena a, Str pat, Str s, i32 flags, b32 verbose)
{
    if (knowngap(pat, s, flags)) return 1;
    Str want = glibcrun(&a, pat, s, flags);
    if (!want.data) return 1;
    Str got  = ourrun(&a, pat, s, flags);
    if (!got.data || got==want) return 1;
    if (verbose) {
        showcase(stderr, pat, s, flags);
        fprintf(stderr, "\n  glibc: %.*s\n  ours:  %.*s\n",
                (int)want.len, want.cdata, (int)got.len, got.cdata);
    }
    return 0;
}

static u64 rng;

static u32 rand32()
{
    rng = rng*0x3243f6a8885a308d + 1;
    return (u32)(rng >> 32);
}

static i32 randn(i32 n)
{
    return (i32)(((u64)rand32() * (u32)n) >> 32);
}

// A random pattern from pieces that exercise glibc's parser and matcher
static Str randpattern(Arena *a)
{
    static Str atoms[] = {
        "a", "b", "c", "a", "b", ".", "x", "_", " ", "-", "^", "$",
        "(", "(", ")", ")", "|", "|", "*", "*", "+", "?",
        "{", "}", "{2}", "{0,1}", "{1,}", "{,2}", "{0}", ",", "1",
        "[ab]", "[^a]", "[a-c]", "[]a]", "[^]b]", "[a-]", "[", "]",
        "[[:alpha:]]", "[[:digit:]]", "[[:space:]]", "[[.a.]]", "[[=b=]]",
        "[[:upper:]]", "[[:lower:]]", "\\w", "\\W", "\\s", "\\S",
        "\\b", "\\B", "\\<", "\\>", "\\`", "\\'", "\\1", "\\2", "\\.",
        "\\(", "\\a", "\\", "A", "B", "()", "(a|)", "(|b)", "a*", "(a*)*",
        "\xc3\xa9", "\xff",
    };
    i32 n = 1 + randn(12);
    Buf b(a, 64);
    for (i32 i = 0; i < n; i++) {
        i32 k = randn(countof(atoms));
        print(&b, atoms[k]);
    }
    return finish(&b);
}

static Str randsubject(Arena *a)
{
    static Str chars[] = {
        "a", "b", "c", "x", "A", "B", "_", " ", "-", ".", "\xc3\xa9",
        "\xff", "1", "\n",
    };
    i32 n = randn(10);
    Buf b(a, 64);
    for (i32 i = 0; i < n; i++) {
        i32 k = randn(countof(chars) - 4 + 4*(randn(4)==0));
        print(&b, chars[k]);
    }
    return finish(&b);
}

static Str readall(Arena *a, FILE *f)
{
    iz  cap = (iz)1 << 20;
    Str r   = {allocbytes(a, cap), 0};
    r.len = (iz)fread(r.data, 1, (uz)cap, f);
    return r;
}

static Str dropnul(Arena *a, Str s)
{
    Str r = {allocbytes(a, s.len), 0};
    for (iz i = 0; i < s.len; i++) {
        if (s[i]) r.data[r.len++] = s[i];
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

int main(int argc, char **argv)
{
    catchcrashes();
    iz    cap = (iz)1 << 28;
    Arena perm = {};
    perm.beg = (byte *)malloc((uz)cap);
    perm.end = perm.beg + cap;

    if (argc>1 && !strcmp(argv[1], "--random")) {
        rng = argc>2 ? strtoull(argv[2], 0, 10) : 1;
        long count = argc>3 ? atol(argv[3]) : 100000;
        long bad = 0;
        for (long i = 0; i < count; i++) {
            Arena a     = perm;
            Str   pat   = randpattern(&a);
            Str   s     = randsubject(&a);
            i32   flags = randn(4)==0 ? REX_ICASE : 0;
            flags |= randn(3)==0 ? REX_UTF8 : 0;
            if (argc > 4) {  // trace each case, to find a crash
                showcase(stderr, pat, s, flags);
                fputc('\n', stderr);
            }
            if (!check(a, pat, s, flags, bad < 50)) bad++;
        }
        printf("%ld of %ld differ\n", bad, count);
        return !!bad;
    }

    if (argc>1 && !strcmp(argv[1], "--record")) {
        Str src = readall(&perm, stdin);
        for (Cut c = cut(src, '\n'); c.head.len || c.tail.len; c = cut(c.tail, '\n')) {
            Str line = c.head;
            if (line.len && line[line.len-1]=='\r') line.len--;
            if (!line.len || line[0]=='#') {
                fwrite(line.data, 1, (uz)line.len, stdout);
                fputc('\n', stdout);
                continue;
            }
            Arena a  = perm;
            Cut   f1 = cut(line, '\t');
            Cut   f2 = cut(f1.tail, '\t');
            Str   pat = decode(&a, f2.head);
            Str   s   = decode(&a, f2.tail);
            Str   r   = glibcrun(&a, pat, s, parseflags(f1.head));
            fwrite(line.data, 1, (uz)line.len, stdout);
            printf("\t%.*s\n", (int)r.len, r.cdata);
        }
        return 0;
    }

    // One AFL case from stdin
    Arena a   = perm;
    Str   in  = readall(&a, stdin);
    if (!in.len) return 0;
    i32   flags = (in[0]&1 ? REX_ICASE : 0) | (in[0]&2 ? REX_UTF8 : 0);
    Cut   c     = cut(dropnul(&a, cuthead(in, 1)), '\n');
    if (c.head.len>48 || c.tail.len>24) return 0;  // keep glibc fast
    if (!check(a, c.head, c.tail, flags, 1)) __builtin_trap();
    return 0;
}
