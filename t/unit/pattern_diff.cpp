// Differential tests of pattern.cpp against glibc's fnmatch
// $ g++ -std=c++20 -g3 -O1 -fsanitize=undefined -fsanitize-trap
//       -o pattern_diff t/unit/pattern_diff.cpp
// $ ./pattern_diff --random [SEED [COUNT]]   compare on generated cases
// $ ./pattern_diff --all [-v] <cases         show every difference, gaps too
//                                           (with -v every result)
// $ ./pattern_diff <case                     one case (AFL), trap on a difference
//
// Linux only: glibc is the reference where it agrees with bash, whose
// matcher is its own (lib/glob/strmatch.c) and not fnmatch. Everything
// happens in memory; the only input is the case text on stdin. A case
// is one line of three tab-separated fields, flags, pattern and string,
// bytes outside ! to ~ and % written %HH (%00 is an empty field). Flag
// letters: e FNM_EXTMATCH (PM_EXTGLOB), p FNM_PATHNAME (PM_PATHNAME),
// d FNM_PERIOD (PM_PERIOD), i FNM_CASEFOLD (PM_NOCASE), u C.UTF-8
// (else C), - none. The generated cases are those bash can show: case
// and [[ ]] (extglob), a pathname component (pathname and period, no
// slash in the string) and GLOBIGNORE (pathname). AFL input is raw: a
// flags byte (bit 0 extglob, bit 1 pathname, bit 2 period, bit 3
// nocase, bit 4 UTF-8), the pattern, a newline, the string; NUL bytes
// are dropped.
//
// Where bash and glibc differ (knowngap below), the case is skipped.
// Each class was found here and its cases checked against bash 5.3
// (pattern_oracle.bash, which bash agreed with pattern.cpp on); see
// notes/m4.md:
// - a pattern ending in an unpaired backslash: bash matches a literal
//   backslash, glibc nothing;
// - extglob groups: glibc's FNM_EXTMATCH matcher is not bash's (star
//   runs before groups, negations, empty and unclosed groups);
// - under FNM_PATHNAME, bash lets a final run of * and ? holding a *
//   match slashes, and fails a run of two or more wildcards starting
//   with a * where the string has a slash;
// - a bracket whose end depends on the character it matches (bash
//   finds the end by a rescan after a match; a fork in pattern.cpp)
//   between stars: bash's star loop commits to the first place the
//   part before the next star matches, which pattern.cpp does not
//   model (notes/m4.md);
// - [:x:] with an unknown name, unterminated, or next to a range dash:
//   bash skips it and reads the bracket on, glibc gives up on it;
// - brackets: under FNM_PATHNAME bash takes one holding a slash
//   literally; bash reads [. .] and [= =] its own way (folding them
//   under FNM_CASEFOLD, and when unterminated); glibc fails a bracket
//   after a run of * and ? holding both under FNM_PERIOD when the
//   string holds a dot;
// - in UTF-8 with non-ASCII characters: classes and case folding
//   (glibc uses its Unicode tables, pattern.cpp is ASCII-only),
//   brackets, invalid sequences, and ? (glibc lets ?? match é).
//
// This is free and unencumbered software released into the public domain.
#include <fnmatch.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../base.cpp"
#include "../../pattern.cpp"

[[noreturn]] static void os_oom()
{
    fputs("pattern_diff: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}

enum : i32 {
    DF_EXT    = 1<<0,
    DF_PATH   = 1<<1,
    DF_PERIOD = 1<<2,
    DF_CASE   = 1<<3,
    DF_UTF8   = 1<<4,
};

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

static i32 curlocale = -1;

static void uselocale(i32 utf8)
{
    if (curlocale == utf8) return;
    curlocale = utf8;
    if (!setlocale(LC_ALL, utf8 ? "C.UTF-8" : "C")) {
        fputs("pattern_diff: no C.UTF-8 locale\n", stderr);
        exit(2);
    }
}

static b32 glibcrun(Arena a, Str pat, Str s, i32 flags)
{
    uselocale(!!(flags & DF_UTF8));
    i32 f = 0;
    f |= flags&DF_EXT  ? FNM_EXTMATCH : 0;
    f |= flags&DF_PATH   ? FNM_PATHNAME : 0;
    f |= flags&DF_PERIOD ? FNM_PERIOD : 0;
    f |= flags&DF_CASE   ? FNM_CASEFOLD : 0;
    return !fnmatch(cstr(&a, pat), cstr(&a, s), f);
}

static b32 ourrun(Arena a, Str pat, Str s, i32 flags)
{
    i32 f = 0;
    f |= flags&DF_EXT  ? PM_EXTGLOB : 0;
    f |= flags&DF_PATH   ? PM_PATHNAME : 0;
    f |= flags&DF_PERIOD ? PM_PERIOD : 0;
    f |= flags&DF_CASE   ? PM_NOCASE : 0;
    f |= flags&DF_UTF8   ? PM_UTF8 : 0;
    return patmatch(pat, s, f, a);
}

// Does the pattern hold an extglob group (with extglob on)?
static b32 hasgroup(Str pat, i32 flags)
{
    if (!(flags & DF_EXT)) return 0;
    for (iz i = 0; i+1 < pat.len; i++) {
        if (pat[i] == '\\') {
            i++;
        } else if (isextop(pat[i]) && pat[i+1]=='(') {
            return 1;
        }
    }
    return 0;
}

static b32 hasbyte(Str s, u8 c)
{
    for (iz i = 0; i < s.len; i++) {
        if (s[i] == c) return 1;
    }
    return 0;
}

// Cases where bash and glibc differ, as listed at the top
static b32 knowngap(Str pat, Str s, i32 flags, Arena scratch)
{
    // A trailing unpaired backslash
    iz bs = 0;
    for (iz i = pat.len-1; i>=0 && pat[i]=='\\'; i--) bs++;
    if (bs & 1) return 1;

    // extglob groups, where glibc's matcher is not bash's
    if (hasgroup(pat, flags)) return 1;

    // A bracket whose end depends on the character it matches (a fork in
    // pattern.cpp) with stars around it: bash commits to the first place
    // the part before the next star matches, a cut that is exact only
    // when brackets end in one place. pattern.cpp tries every place.
    PatComp k = {};
    patcompile(&k, pat, flags&DF_PATH ? PM_PATHNAME : 0, 0, &scratch);
    iz stars = 0;
    for (iz i = 0; i < pat.len; i++) stars += pat[i] == '*';
    if (k.forks && stars>1) return 1;

    // Under FNM_PATHNAME, a final run of * and ? holding a * matches
    // slashes in bash
    // slashes in bash, and one of two or more wildcards starting with a
    // * fails where the string has a slash
    if ((flags&DF_PATH) && hasbyte(s, '/')) {
        b32 star = 0;
        for (iz i = pat.len; i>0 && (pat[i-1]=='*' || pat[i-1]=='?'); i--) {
            star |= pat[i-1] == '*';
        }
        if (star) return 1;
        for (iz i = 0; i+1 < pat.len; i++) {
            if (pat[i] == '\\') {
                i++;
            } else if (pat[i]=='*' && (pat[i+1]=='*' || pat[i+1]=='?')) {
                return 1;
            }
        }
    }

    // Brackets: under FNM_PATHNAME one holding a slash is literal in
    // bash; with case folding bash folds [. .] and [= =] too; in UTF-8
    // with non-ASCII characters, glibc matches brackets its own way
    // bash's own reading of [. .] and [= =], terminated or not, also
    // differs. glibc fails a bracket after a run of * and ? holding
    // both when FNM_PERIOD is on and the string holds a dot.
    b32 bracket = 0;
    b32 starq   = 0;
    for (iz i = 0; i < pat.len; i++) {
        if (pat[i] == '\\') {
            i++;
        } else if (pat[i] == '[') {
            bracket = 1;
            if ((flags&DF_PATH) && hasbyte(cuthead(pat, i), '/')) return 1;
            if (i+1<pat.len && (pat[i+1]=='.' || pat[i+1]=='=')) return 1;
        } else if (pat[i]=='*' || pat[i]=='?') {
            b32 star = 0, q = 0;
            for (; i<pat.len && (pat[i]=='*' || pat[i]=='?'); i++) {
                star |= pat[i] == '*';
                q    |= pat[i] == '?';
            }
            i--;
            starq |= star && q;
        }
    }
    if ((flags&DF_PERIOD) && bracket && starq && hasbyte(s, '.')) return 1;

    // [:x:] with an unknown name, unterminated, or as a range end:
    // bash skips an unknown class and reads the rest of the bracket on,
    // glibc gives up on the bracket
    static Str classes[] = {
        "alnum", "alpha", "blank", "cntrl", "digit", "graph", "lower",
        "print", "punct", "space", "upper", "xdigit",
    };
    for (iz i = 0; i+1 < pat.len; i++) {
        if (pat[i]!='[' || pat[i+1]!=':') continue;
        if (i>0 && pat[i-1]=='-') return 1;
        iz j = i + 2;
        for (; j+1<pat.len && !(pat[j]==':' && pat[j+1]==']'); j++) {}
        if (j+1 >= pat.len) return 1;
        Str  name = span(pat.data+i+2, pat.data+j);
        b32  known = 0;
        for (iz c = 0; c < countof(classes); c++) known |= name == classes[c];
        if (!known) return 1;
        if (j+2<pat.len && pat[j+2]=='-') return 1;
    }

    // Non-ASCII in UTF-8: classes, case, invalid sequences
    if ((flags&DF_UTF8) && !(allascii(pat) && allascii(s))) {
        if (flags & DF_CASE) return 1;
        if (bracket || hasbyte(pat, '?')) return 1;
        if (!glibcvalid(pat) || !glibcvalid(s)) return 1;
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
    if (flags & DF_EXT)    putbyte(&b, 'e');
    if (flags & DF_PATH)   putbyte(&b, 'p');
    if (flags & DF_PERIOD) putbyte(&b, 'd');
    if (flags & DF_CASE)   putbyte(&b, 'i');
    if (flags & DF_UTF8)   putbyte(&b, 'u');
    if (!flags) putbyte(&b, '-');
    putbyte(&b, '\t');
    encode(&b, pat);
    putbyte(&b, '\t');
    encode(&b, s);
    Str line = finish(&b);
    fwrite(line.data, 1, (uz)line.len, f);
}

// Compare one case; 0 on a difference.
static b32 check(Arena a, Str pat, Str s, i32 flags, b32 verbose, b32 gaps)
{
    if (!gaps && knowngap(pat, s, flags, a)) return 1;
    b32 want = glibcrun(a, pat, s, flags);
    b32 got  = ourrun(a, pat, s, flags);
    if (got == want) return 1;
    if (verbose) {
        showcase(stdout, pat, s, flags);
        printf("\tglibc %d ours %d\n", want, got);
        fflush(stdout);  // before the AFL mode traps
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

static Str randpattern(Arena *a, b32 ext)
{
    static Str atoms[] = {
        "a", "b", "c", "a", "b", "A", ".", ".", "/", "/", "-", "x",
        "*", "*", "*", "?", "?", "\\*", "\\?", "\\a", "\\", "\\\\", "\\/",
        "[ab]", "[!a]", "[^b]", "[a-c]", "[]a]", "[a-]", "[", "]", "[.]",
        "[/]", "[[:alpha:]]", "[[:upper:]]", "[[:punct:]]", "[![:lower:]]",
        "[[.a.]]", "[[=b=]]", "[\\]]", "[*?]",
        "\xc3\xa9", "\xff", "\xc3",
    };
    static Str groups[] = {
        "@(", "*(", "+(", "?(", "!(", "|", ")", "(", "@(a|b)", "*(a)",
        "!(a)", "?(.)", "+(b|*)", "@()", "!(*.c)",
    };
    i32 n = 1 + randn(8);
    Buf b(a, 64);
    for (i32 i = 0; i < n; i++) {
        if (ext && randn(3)==0) {
            print(&b, groups[randn(countof(groups))]);
        } else {
            print(&b, atoms[randn(countof(atoms))]);
        }
    }
    return finish(&b);
}

static Str randsubject(Arena *a, b32 noslash)
{
    static Str chars[] = {
        "a", "b", "c", "A", "B", ".", "/", "x", "-", "*", "?", "[", "]",
        "\\", "(", ")", "|", "\xc3\xa9", "\xff", "\xc3",
    };
    i32 n = randn(8);
    Buf b(a, 64);
    for (i32 i = 0; i < n; i++) {
        i32 lim = randn(4) ? 8 : countof(chars);
        Str c   = chars[randn(lim)];
        print(&b, noslash && c=="/" ? Str("y") : c);
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
        switch (s[i]) {
        case 'e': f |= DF_EXT;  break;
        case 'p': f |= DF_PATH; break;
        case 'd': f |= DF_PERIOD; break;
        case 'i': f |= DF_CASE; break;
        case 'u': f |= DF_UTF8; break;
        }
    }
    return f;
}

int main(int argc, char **argv)
{
    iz    cap = (iz)1 << 26;
    Arena perm = {};
    perm.beg = (byte *)malloc((uz)cap);
    perm.end = perm.beg + cap;

    if (argc>1 && !strcmp(argv[1], "--random")) {
        rng = argc>2 ? strtoull(argv[2], 0, 10) : 1;
        long count = argc>3 ? atol(argv[3]) : 100000;
        b32  gaps  = argc>4;
        long bad = 0;
        for (long i = 0; i < count; i++) {
            Arena a     = perm;
            i32   flags = randn(4)==0 ? DF_CASE : 0;
            flags |= randn(3)==0 ? DF_UTF8 : 0;
            switch (randn(3)) {
            case 0: flags |= DF_EXT;  break;                         // case
            case 1: flags |= DF_PATH | DF_PERIOD | randn(2)*DF_EXT; break;  // glob
            case 2: flags |= DF_PATH | randn(2)*DF_EXT; break;       // GLOBIGNORE
            }
            Str pat = randpattern(&a, flags & DF_EXT);
            Str s   = randsubject(&a, flags & DF_PERIOD);
            if (!check(a, pat, s, flags, gaps || bad<50, gaps)) bad++;
        }
        printf("%ld of %ld differ\n", bad, count);
        return !!bad;
    }

    if (argc>1 && !strcmp(argv[1], "--all")) {
        Str  src = readall(&perm, stdin);
        long bad = 0;
        for (Cut c = cut(src, '\n'); c.head.len || c.tail.len; c = cut(c.tail, '\n')) {
            Str line = c.head;
            if (line.len && line[line.len-1]=='\r') line.len--;
            if (!line.len || line[0]=='#') continue;
            Arena a   = perm;
            Cut   f1  = cut(line, '\t');
            Cut   f2  = cut(f1.tail, '\t');
            Str   pat = decode(&a, f2.head);
            Str   s   = decode(&a, f2.tail);
            i32   fl  = parseflags(f1.head);
            if (argc > 2) {  // every result
                showcase(stdout, pat, s, fl);
                printf("\tglibc %d ours %d\n", glibcrun(a, pat, s, fl), ourrun(a, pat, s, fl));
            }
            bad += !check(a, pat, s, fl, argc<=2, 1);
        }
        printf("%ld differ\n", bad);
        return 0;
    }

    // One AFL case from stdin
    Arena a  = perm;
    Str   in = readall(&a, stdin);
    if (!in.len) return 0;
    i32 flags = in[0] & 31;
    Cut c     = cut(dropnul(&a, cuthead(in, 1)), '\n');
    if (c.head.len>64 || c.tail.len>32) return 0;  // keep glibc fast
    if (!check(a, c.head, c.tail, flags, 1, 0)) __builtin_trap();
    return 0;
}
