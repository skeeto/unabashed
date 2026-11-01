// Tests for pattern.cpp against results recorded from bash 5.3
// $ g++ -std=c++20 -g3 -Wall -Wextra -Wconversion -Wno-sign-conversion
//       -fsanitize=undefined -fsanitize-trap
//       -o pattern_test.exe t/unit/pattern_test.cpp
// $ ./pattern_test.exe [t/unit/pattern.golden]
//
// Run from the repository root. pattern_record.sh writes pattern.golden
// by running pattern_oracle.bash in bash 5.3 on the inputs in
// pattern.cases. Besides the golden cases, this checks the two-pointer
// matcher against the set matcher, the fast substring search against
// probing, and that pathological and hostile patterns (deep nesting,
// thousands of stars, runs of optional groups, nested negations) finish
// quickly within the recursion limits. Build with -Wl,--stack,1048576
// to check that the limits fit a 1 MiB stack, as on a shell fiber.
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../base.cpp"
#include "../../pattern.cpp"

[[noreturn]] static void os_oom()
{
    fputs("pattern_test: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}

static i32 ntests;
static i32 nfailed;

static void showstr(FILE *f, Str s)
{
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c<0x21 || c>0x7e || c=='%') {
            fprintf(f, "%%%02X", c);
        } else {
            fputc(c, f);
        }
    }
}

static void fail(Str line, Str got)
{
    nfailed++;
    fputs("FAIL: ", stderr);
    showstr(stderr, line);
    fputs("  got: ", stderr);
    showstr(stderr, got);
    fputc('\n', stderr);
}

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
        switch (s[i]) {
        case 'e': f |= PM_EXTGLOB; break;
        case 'i': f |= PM_NOCASE;  break;
        case 'u': f |= PM_UTF8;    break;
        }
    }
    return f;
}

static Str catstr(Arena *a, Str x, Str y)
{
    Str r = {allocbytes(a, x.len+y.len), x.len+y.len};
    copybytes(r.data, x.data, x.len);
    copybytes(r.data+x.len, y.data, y.len);
    return r;
}

// Expand ${s OP pat} with a replacement of <>, following bash's loop in
// pat_subst for // and its special cases for empty strings and patterns.
static Str expand(Arena *a, Str op, Str pat, Str s, i32 flags)
{
    Str rep = "<>";
    if (op=="#" || op=="##" || op=="%" || op=="%%") {
        flags &= ~PM_NOCASE;  // bash ignores nocasematch here
        i32 mode = op=="#"  ? PF_SHORTPREFIX : op=="##" ? PF_LONGPREFIX :
                   op=="%"  ? PF_SHORTSUFFIX : PF_LONGSUFFIX;
        PatSpan m = patfind(pat, s, 0, mode, flags, *a);
        if (!m.ok) return s;
        return mode<=PF_LONGPREFIX ? cuthead(s, m.end) : takehead(s, m.beg);
    }
    if (op=="/#" || op=="/%") {
        i32 mode = op=="/#" ? PF_ANCHORSTART : PF_ANCHOREND;
        PatSpan m = patfind(pat, s, 0, mode, flags, *a);
        if (!m.ok) return s;
        Str r = catstr(a, takehead(s, m.beg), rep);
        return catstr(a, r, cuthead(s, m.end));
    }
    b32 global = op == "//";
    if (!s.len) {
        PatSpan m = patfind(pat, s, 0, PF_FIRST, flags, *a);
        return m.ok ? rep : s;
    }
    Str r   = {};
    iz  str = 0;
    while (str < s.len) {
        PatSpan m = patfind(pat, s, str, PF_FIRST, flags, *a);
        if (!m.ok) break;
        r   = catstr(a, r, span(s.data+str, s.data+m.beg));
        r   = catstr(a, r, rep);
        str = m.end;
        if (!global) break;
        if (m.beg==m.end && str<s.len) {
            iz len = 1;
            if (flags & PM_UTF8) {
                len = glibclen(s, str);
                len = len ? len : 1;
            }
            r = catstr(a, r, span(s.data+str, s.data+str+len));
            str += len;
        }
    }
    return catstr(a, r, cuthead(s, str));
}

static void rungolden(char const *path, Arena perm)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "pattern_test: cannot open %s\n", path);
        exit(2);
    }
    iz  cap = (iz)1 << 24;
    Str src = {allocbytes(&perm, cap), 0};
    src.len = (iz)fread(src.data, 1, (uz)cap, f);
    fclose(f);

    for (Cut c = cut(src, '\n'); c.head.len || c.tail.len; c = cut(c.tail, '\n')) {
        Str line = c.head;
        if (line.len && line[line.len-1]=='\r') line.len--;
        if (!line.len || line[0]=='#') continue;
        Str fields[8] = {};
        iz  nf = 0;
        for (Cut t = cut(line, '\t'); nf < countof(fields); t = cut(t.tail, '\t')) {
            fields[nf++] = t.head;
            if (!t.ok) break;
        }

        Arena a = perm;
        ntests++;
        Str   kind  = fields[0];
        i32   flags = parseflags(fields[1]);
        b32   ok    = 0;
        Str   got   = {};
        if (kind=="m" && nf==5) {
            Str pat = decode(&a, fields[2]);
            Str s   = decode(&a, fields[3]);
            b32 r   = patmatch(pat, s, flags, a);
            PatProg p = patprog(pat, flags, &a);
            ok  = r==(fields[4]=="1") && r==progmatch(&p, s, a);
            got = r ? Str("1") : "0";
        } else if (kind=="i" && nf==5) {
            // GLOBIGNORE, as glob.cpp's globignored: a whole path with
            // PM_PATHNAME (no . or .. components here)
            Str pat  = decode(&a, fields[2]);
            Str path = decode(&a, fields[3]);
            b32 r    = patmatch(pat, path, flags|PM_PATHNAME, a);
            ok  = r == (fields[4]=="1");
            got = r ? Str("1") : "0";
        } else if (kind=="f" && nf==6) {
            Str pat  = decode(&a, fields[3]);
            Str s    = decode(&a, fields[4]);
            Str want = decode(&a, fields[5]);
            got = expand(&a, fields[2], pat, s, flags);
            ok  = got == want;
        } else if (kind=="g" && nf==5) {
            Str pat  = decode(&a, fields[2]);
            Str name = decode(&a, fields[3]);
            if (!hasglob(pat, flags)) {
                // bash looks such a name up directly instead of matching
                ntests--;
                continue;
            }
            i32 gf   = flags;
            gf |= find(fields[1], "D")<0 ? PM_PERIOD   : 0;
            gf |= find(fields[1], "S")<0 ? PM_SKIPDOTS : 0;
            b32 r = globmatch(pat, name, gf, a);
            PatProg p = globprog(pat, gf, &a);
            b32 r2 = !skipname(pat, name, gf, 0) && progmatch(&p, name, a);
            ok  = r==(fields[4]=="1") && r==r2;
            got = r ? Str("1") : "0";
        } else if (kind=="h" && nf==4) {
            Str pat = decode(&a, fields[2]);
            b32 r   = hasglob(pat, flags);
            ok  = r == (fields[3]=="1");
            got = r ? Str("1") : "0";
        } else {
            got = "malformed line";
        }
        if (!ok) fail(line, got);
    }
}


// Self-consistency checks with random patterns

static u64 rng = 0x9e3779b97f4a7c15;

static i32 randint(i32 n)
{
    rng = rng*0x3243f6a8885a308d + 1;
    return (i32)((rng>>33) % (u64)n);
}

static Str randstr(Arena *a, Str alphabet, iz maxlen)
{
    iz  len = randint((i32)maxlen+1);
    Str r   = {allocbytes(a, len), len};
    for (iz i = 0; i < len; i++) {
        r.data[i] = alphabet[randint((i32)alphabet.len)];
    }
    return r;
}

static Str randpat(Arena *a, b32 ext, i32 depth)
{
    static Str atoms[] = {
        "a", "b", "c", "*", "?", "[ab]", "[!a]", "[a-c]", "\\*", "\\a",
        "[[:alpha:]]", "[", "]", "**", "a*", "*b",
    };
    static Str ops[] = {"?(", "*(", "+(", "@(", "!("};
    Buf b(a, 64);
    i32 n = randint(5);
    for (i32 i = 0; i < n; i++) {
        if (ext && depth<3 && randint(4)==0) {
            print(&b, ops[randint(countof(ops))]);
            i32 nalt = 1 + randint(3);
            for (i32 j = 0; j < nalt; j++) {
                if (j) putbyte(&b, '|');
                print(&b, randpat(a, ext, depth+1));
            }
            putbyte(&b, ')');
        } else {
            print(&b, atoms[randint(countof(atoms))]);
        }
    }
    return clone(a, finish(&b));
}

static void check(b32 ok, char const *what, Str pat, Str s)
{
    ntests++;
    if (!ok) {
        nfailed++;
        fprintf(stderr, "FAIL: %s: pattern ", what);
        showstr(stderr, pat);
        fputs(" string ", stderr);
        showstr(stderr, s);
        fputc('\n', stderr);
    }
}

// The set matcher must agree with the two-pointer matcher on plain globs.
static void selfplain(Arena perm)
{
    for (i32 iter = 0; iter < 20000; iter++) {
        Arena a     = perm;
        Str   pat   = randpat(&a, 0, 0);
        Str   s     = randstr(&a, "abc*[]", 8);
        i32   flags = randint(2) ? PM_NOCASE : 0;
        PatComp  k     = {};
        PatSeq   q     = patcompile(&k, pat, flags, 0, &a);
        PatCtx   x     = makectx(s, flags, 0, &a);
        b32   want  = twoptr(&x, q);
        x.fixedend  = s.len;
        PosSet   e     = evalseq(&x, q, 0, setone(&a, 0), 0, &a);
        check(want == sethas(e, s.len), "twoptr vs sets", pat, s);
    }
}

// The fast substring search must agree with probing whole matches.
static void selffind(Arena perm)
{
    for (i32 iter = 0; iter < 20000; iter++) {
        Arena a     = perm;
        b32   ext   = randint(2);
        b32   wide  = randint(3) == 0;
        Str   pat   = randpat(&a, ext, 0);
        Str   s     = randstr(&a, "abc*\\", 10);
        if (wide) {
            Buf b(&a, 64);
            for (i32 i = randint(8); i > 0; i--) {
                static Str units[] = {"a", "b", "*", "\\", "\xc3\xa9", "\xe2\x82\xac"};
                print(&b, units[randint(countof(units))]);
            }
            s = clone(&a, finish(&b));
        }
        i32   mode  = randint(7);
        i32   flags = (ext ? PM_EXTGLOB : 0) | (wide ? PM_UTF8 : 0);
        if (!pat.len || !s.len) continue;
        if (wide && randint(2)) pat = catstr(&a, pat, "\xc3\xa9");
        wide = wide && glibcvalid(s) && !allascii(s);
        if (!wide) flags &= ~PM_UTF8;
        PatComp k = {};
        patcompile(&k, pat, flags, wide, &a);
        if (k.quirk) continue;
        PatSpan x = findspan(pat, s, mode, flags, a);
        PatSpan y = probespan(pat, s, mode, flags, wide, a);
        b32 same = x.ok==y.ok && (!x.ok || (x.beg==y.beg && x.end==y.end));
        check(same, "fast search vs probing", pat, s);
    }
}

// unquoteglob follows bash's dequoting of pathnames.
static void selfunquote(Arena perm)
{
    struct {
        Str in;
        Str out;
    } cases[] = {
        {"",         ""},
        {"abc",      "abc"},
        {"a\\*b",    "a*b"},
        {"\\\\",     "\\"},
        {"\\a\\b",   "ab"},
        {"a\\",      "a"},
        {"\\",       ""},
        {"[\\]]",    "[]]"},
        {"\\\\\\*",  "\\*"},
    };
    for (iz i = 0; i < countof(cases); i++) {
        Arena a = perm;
        check(unquoteglob(&a, cases[i].in)==cases[i].out, "unquoteglob", cases[i].in, "");
    }
}

static double now()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

// Pathological cases must stay fast.
static void selfspeed(Arena perm)
{
    Arena a = perm;
    iz    n = 100000;
    Str   s = {allocbytes(&a, n), n};
    memset(s.data, 'a', (uz)n);

    struct {
        Str pat;
        i32 flags;
        b32 want;
    } cases[] = {
        {"*a*a*a*a*a*a*a*a*b",        0,           0},
        {"*a*a*a*a*a*a*a*a*",         0,           1},
        {"*?*?*?*?*?*?*?*?*?*?b",     0,           0},
        {"+(a|aa)",                    PM_EXTGLOB,  1},
        {"+(a|aa)b",                   PM_EXTGLOB,  0},
        {"*(a*)b",                     PM_EXTGLOB,  0},
        {"@(*a|a*)*(a)",               PM_EXTGLOB,  1},
        {"!(*b)",                      PM_EXTGLOB,  1},
        {"*(+(a))",                    PM_EXTGLOB,  1},
    };
    for (iz i = 0; i < countof(cases); i++) {
        double t0 = now();
        b32    r  = patmatch(cases[i].pat, s, cases[i].flags, a);
        double dt = now() - t0;
        check(r==cases[i].want && dt<2, "speed", cases[i].pat, "a...a");
    }

    double t0 = now();
    PatSpan m = patfind("a", s, 0, PF_FIRST, 0, a);
    for (iz i = 1; i < 2000 && m.ok; i++) {
        m = patfind("a", s, m.end, PF_FIRST, 0, a);
    }
    check(m.ok && now()-t0<2, "speed", "a (repeated)", "a...a");

    t0 = now();
    m = patfind("+(a)", s, 0, PF_LONGSUFFIX, PM_EXTGLOB, a);
    check(m.ok && m.beg==0 && now()-t0<2, "speed", "%%+(a)", "a...a");
    t0 = now();
    m = patfind("*a", s, 0, PF_SHORTSUFFIX, 0, a);
    check(m.ok && m.beg==n-1 && now()-t0<2, "speed", "%*a", "a...a");
}

static Str repeat(Arena *a, Str pre, iz n, Str mid, Str post)
{
    Buf b(a, 1<<10);
    for (iz i = 0; i < n; i++) print(&b, pre);
    print(&b, mid);
    for (iz i = 0; i < n; i++) print(&b, post);
    return clone(a, finish(&b));
}

// Hostile patterns must finish quickly, within the recursion limits and
// memory, giving bash's answer where it is known (recorded from bash on
// shorter strings, where its answer does not depend on the length).
// Nesting beyond PAT_MAXNEST deliberately never matches.
static void selfhostile(Arena perm)
{
    Arena a  = perm;
    iz    n  = 3000;
    Str   as = repeat(&a, "a", n, "", "");
    Str   bs = repeat(&a, "b", 600, "", "");  // negations cost n^3/64
    Str   ab = repeat(&a, "a", 40, "b", "");

    struct {
        Str pat;
        Str s;
        i32 flags;
        b32 want;
    } cases[] = {
        {repeat(&a, "@(", 30, "a", ")"),       "a",  PM_EXTGLOB,  1},
        {repeat(&a, "@(", 33, "a", ")"),       "a",  PM_EXTGLOB,  0},
        {repeat(&a, "@(", 50000, "a", ")"),    "a",  PM_EXTGLOB,  0},
        {repeat(&a, "!(", 50000, "a", ")"),    "a",  PM_EXTGLOB,  0},
        {repeat(&a, "*(", 50000, "a", ")"),    as,   PM_EXTGLOB,  0},
        {repeat(&a, "*(", 50000, "", ""),      as,   PM_EXTGLOB,  0},
        {repeat(&a, "[", 100000, "", ""),      "[",  0,           0},
        {repeat(&a, "[", 100000, "]", ""),     "[",  0,           1},
        {repeat(&a, "*a", n, "", ""),          as,   PM_EXTGLOB,  1},
        {repeat(&a, "*a", n, "@(a)", ""),      as,   PM_EXTGLOB,  0},
        {repeat(&a, "*a", n, "", ""),          as,   PM_PATHNAME, 1},
        {repeat(&a, "*a", n, "b", ""),         as,   PM_PATHNAME, 0},
        {repeat(&a, "*?(a)", 40, "", ""),      ab,   PM_EXTGLOB,  1},
        {repeat(&a, "*?(a)", 40, "c", ""),     ab,   PM_EXTGLOB,  0},
        {repeat(&a, "*?(a)", 40, "b", ""),     ab,   PM_EXTGLOB,  1},
        {repeat(&a, "**(a)", 40, "c", ""),     ab,   PM_EXTGLOB,  0},
        {repeat(&a, "*?(a)c", 40, "", ""),     ab,   PM_EXTGLOB,  0},
        {"!(*!(*!(*a)))",                      bs,   PM_EXTGLOB,  1},
        {"!(*!(*!(*b)))",                      bs,   PM_EXTGLOB,  0},
        {"@(*!(b)|!(*b)*)",                    bs,   PM_EXTGLOB,  1},
        {"+(!(a)|b)b",                         bs,   PM_EXTGLOB,  1},
        {repeat(&a, "[x#-[:alpha:]]", 40, "", ""), "x", 0,        0},
    };
    for (iz i = 0; i < countof(cases); i++) {
        double t0 = now();
        b32    r  = patmatch(cases[i].pat, cases[i].s, cases[i].flags, a);
        double dt = now() - t0;
        Str    p  = cases[i].pat;
        check(r==cases[i].want && dt<2, "hostile", takehead(p, p.len<40 ? p.len : 40), "");
    }

    // Leading-dot checks follow chains of groups without recursing.
    Str chain = repeat(&a, "?(a)", 2000, "b", "");
    check(globmatch(chain, "ab", PM_EXTGLOB|PM_PERIOD, a), "hostile", "?(a)...b", "ab");
    check(!globmatch(chain, ".ab", PM_EXTGLOB|PM_PERIOD, a), "hostile", "?(a)...b", ".ab");
    Str deep = repeat(&a, "@(", 50000, ".a", ")");
    check(!globmatch(deep, ".a", PM_EXTGLOB|PM_PERIOD, a), "hostile", "@(@(...", ".a");

    // Substring searches with hostile patterns
    double  t0 = now();
    PatSpan m  = patfind(repeat(&a, "@(", 50000, "a", ")"), as, 0, PF_FIRST, PM_EXTGLOB, a);
    check(!m.ok && now()-t0<2, "hostile", "/@(@(...", "a...a");
    t0 = now();
    m  = patfind(repeat(&a, "*a", n, "", ""), as, 0, PF_LONGSUFFIX, 0, a);
    check(m.ok && m.beg==0 && now()-t0<2, "hostile", "%%*a*a...", "a...a");
    t0 = now();
    m  = patfind(repeat(&a, "*?(a)", 40, "", ""), ab, 0, PF_ANCHORSTART, PM_EXTGLOB, a);
    check(m.ok && m.end==ab.len && now()-t0<2, "hostile", "/#*?(a)...", "a...ab");
}

int main(int argc, char **argv)
{
    iz    cap  = (iz)1 << 28;
    Arena perm = {};
    perm.beg = (byte *)malloc((uz)cap);
    perm.end = perm.beg + cap;
    if (!perm.beg) os_oom();

    rungolden(argc>1 ? argv[1] : "t/unit/pattern.golden", perm);
    selfplain(perm);
    selffind(perm);
    selfunquote(perm);
    selfspeed(perm);
    selfhostile(perm);

    fprintf(stderr, "%d tests, %d failed\n", ntests, nfailed);
    return nfailed != 0;
}
