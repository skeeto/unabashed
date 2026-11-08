// Unit tests for b_printf.cpp against behavior recorded from bash 5.3
// $ g++ -std=c++20 -g3 -Wall -Wextra -Wconversion -Wno-sign-conversion
//       -fsanitize=undefined -fsanitize-trap -o printf_test.exe
//       t/unit/printf_test.cpp
// $ ./printf_test.exe [t/unit/printf.golden]
//
// The golden file is recorded from t/unit/printf.cases by
// t/unit/printf_record.sh, which documents its format. Run from the
// repository root, or pass the golden file's path.
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"  // base.cpp's spares
#include "../../src/base.cpp"
#pragma GCC diagnostic pop
#include "../../src/float.cpp"
#include "../../src/b_printf.cpp"

[[noreturn]] static void os_oom()
{
    fputs("printf_test: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}


// Golden file

struct Record {
    u8         kind;    // 'T' printf, 'X' echo -e
    b32        utf8;
    Str        fmt;
    Slice<Str> args;
    Str        out;
    Str        err;
    i32        status;
    Str        var;     // name=value after %n, or null
    iz         line;
};

static Str decode(Arena *a, Str s)
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        if (s[i]=='\\' && i+1<s.len && s[i+1]=='\\') {
            putbyte(&b, '\\');
            i++;
        } else if (s[i]=='\\' && i+3<s.len && s[i+1]=='x') {
            putbyte(&b, (u8)(hexdigit(s[i+2])<<4 | hexdigit(s[i+3])));
            i += 3;
        } else {
            putbyte(&b, s[i]);
        }
    }
    Str r = finish(&b);
    r.data = r.data ? r.data : (u8 *)"";
    return r;
}

static Slice<Record> parse(Arena *a, Str src)
{
    Slice<Record> r   = {};
    Record        cur = {};
    iz            n   = 0;
    for (Cut c = {.head={}, .tail=src, .ok=1}; c.ok;) {
        c = cut(c.tail, '\n');
        n++;
        Str line = c.head;
        if (line.len && line[line.len-1]=='\r') {
            line.len--;
        }
        if (!line.len || line[0]=='#') {
            continue;
        }
        Str val = line.len>2 ? cuthead(line, 2) : Str{};
        switch (line[0]) {
        case 'T':
        case 'X':
            cur      = {};
            cur.kind = line[0];
            cur.utf8 = val == "U";
            cur.line = n;
            break;
        case 'F': cur.fmt  = decode(a, val); break;
        case 'A': cur.args = push(a, cur.args, decode(a, val)); break;
        case 'O': cur.out  = decode(a, val); break;
        case 'E': cur.err  = decode(a, val); break;
        case 'V': cur.var  = decode(a, val); break;
        case 'S':
            cur.status = 0;
            for (iz i = 0; i < val.len; i++) {
                cur.status = cur.status*10 + val[i] - '0';
            }
            break;
        case '.':
            r = push(a, r, cur);
            break;
        }
    }
    return r;
}


// Test environment

struct Ctx {
    Buf *err;
    Buf *var;
    i64  now;
};

static void onerror(void *ctx, Str msg)
{
    Ctx *c = (Ctx *)ctx;
    print(c->err, msg);
    putbyte(c->err, '\n');
}

static void onsetint(void *ctx, Str name, i64 v)
{
    Ctx *c = (Ctx *)ctx;
    c->var->len = 0;
    print(c->var, name);
    putbyte(c->var, '=');
    print(c->var, v);
}

static i64 onnow(void *ctx)
{
    return ((Ctx *)ctx)->now;
}

struct Result {
    Str out;
    Str err;
    Str var;
    i32 status;
};

static Result run(Arena *a, b32 utf8, Str fmt, Str *args, iz nargs)
{
    Result r   = {};
    Buf   *out = alloc<Buf>(a);
    Buf   *err = alloc<Buf>(a);
    Buf   *var = alloc<Buf>(a);
    *out = Buf(a, 1<<10);
    *err = Buf(a, 1<<10);
    *var = Buf(a, 1<<6);

    Ctx ctx = {};
    ctx.err = err;
    ctx.var = var;
    ctx.now = 1700000000;

    PrintfEnv env  = {};
    env.ctx        = &ctx;
    env.utf8       = utf8;
    env.now        = onnow;
    env.shellstart = 1600000000;
    env.error      = onerror;
    env.setint     = onsetint;

    r.status = printfmt(&env, out, fmt, args, nargs);
    r.out = finish(out);
    r.err = finish(err);
    r.var = finish(var);
    return r;
}


// Reporting

static i32 ntests;
static i32 nfailed;

static void show(Str label, Str s)
{
    fprintf(stderr, "    %-8.*s \"", (int)label.len, label.cdata);
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c == '\\') {
            fputs("\\\\", stderr);
        } else if (c>=0x20 && c<0x7f) {
            fputc(c, stderr);
        } else {
            fprintf(stderr, "\\x%02x", c);
        }
    }
    fputs("\"\n", stderr);
}

static void check(b32 ok, iz line, Str what)
{
    ntests++;
    if (!ok) {
        nfailed++;
        fprintf(stderr, "FAIL line %td: %.*s\n", line, (int)what.len, what.cdata);
    }
}

static void testrecord(Arena perm, Record *t)
{
    Arena *a = &perm;
    if (t->kind == 'X') {
        b32 stop = 0;
        Str got  = echoescape(a, t->args[0], 0, t->utf8, &stop);
        if (!stop) {
            got = concat(a, got, " X\n");
        }
        b32 ok = got == t->out;
        check(ok, t->line, "echo -e");
        if (!ok) {
            show("arg", t->args[0]);
            show("want", t->out);
            show("got", got);
        }
        return;
    }

    Result r  = run(a, t->utf8, t->fmt, t->args.data, t->args.len);
    b32    ok = r.out==t->out && r.err==t->err && r.status==t->status;
    if (t->var.data) {
        if (!r.var.len) {
            r.var = concat(a, cut(t->var, '=').head, "=");  // never assigned
        }
        ok &= r.var == t->var;
    }
    check(ok, t->line, "printf");
    if (!ok) {
        show("format", t->fmt);
        for (iz i = 0; i < t->args.len; i++) {
            show("arg", t->args[i]);
        }
        show("want out", t->out);
        show("got out",  r.out);
        show("want err", t->err);
        show("got err",  r.err);
        fprintf(stderr, "    status   want %d got %d\n", t->status, r.status);
        if (t->var.data) {
            show("want var", t->var);
            show("got var",  r.var);
        }
    }

    // %b and %q of one argument also exercise the standalone functions
    if (t->args.len==1 && t->err.len==0 && t->fmt=="%b") {
        b32 stop = 0;
        Str got  = echoescape(a, t->args[0], 1, t->utf8, &stop);
        check(got == t->out, t->line, "echoescape %b");
    }
    if (t->args.len==1 && t->fmt=="%q") {
        Str got = shellquote(a, t->args[0], t->utf8);
        check(got == t->out, t->line, "shellquote");
    }
}


// Cases that cannot be recorded: the clock, and deferred floats

static void expect(Arena perm, Str fmt, Str arg, Str want)
{
    Str    args[] = {arg};
    Result r      = run(&perm, 0, fmt, args, arg.data ? 1 : 0);
    b32    ok     = r.out==want && !r.err.len && !r.status;
    check(ok, 0, fmt);
    if (!ok) {
        show("format", fmt);
        show("want", want);
        show("got", r.out);
        show("err", r.err);
    }
}

static void testunclosed(Arena perm, Str unit, Str want)
{
    iz  n = 100000;
    Buf f(&perm, n*unit.len);
    for (iz i = 0; i < n; i++) {
        print(&f, unit);
    }
    Result r  = run(&perm, 0, finish(&f), 0, 0);
    b32    ok = !r.status && r.out.len==n*want.len;
    for (iz i = 0; ok && i < n; i++) {
        ok = takehead(cuthead(r.out, i*want.len), want.len) == want;
    }
    check(ok, 0, unit);
}

static void testinline(Arena perm)
{
    expect(perm, "%(%s)T",   {},     "1700000000");
    expect(perm, "%(%s)T",   "-1",   "1700000000");
    expect(perm, "%(%s)T",   "-2",   "1600000000");
    expect(perm, "%(%s)T",   "-3",   "-3");
    expect(perm, "%()T",     {},     "22:13:20");
    expect(perm, "%f|%s",    "1.5",  "1.500000|");
    expect(perm, "%9.2e|",   "1",    " 1.00e+00|");
    expect(perm, "%a%A%E%F%g%G", {}, "0x0p+00X0P+00.000000E+000.00000000");

    // Standalone quoting entry points
    Arena *a = &perm;
    check(shellquote(a, "", 0) == "''", 0, "shellquote empty");
    check(shellquote(a, "a b", 1) == "a\\ b", 0, "shellquote blank");
    check(shellquote(a, "~x#", 0) == "\\~x#", 0, "shellquote tilde");
    b32 stop = 0;
    check(echoescape(a, "a\\cb", 0, 0, &stop)=="a" && stop, 0, "echo \\c");
    check(echoescape(a, "a\\xg", 1, 0, &stop)=="a\\xg" && !stop, 0, "%b \\x");

    // Unclosed %( must not rescan the rest of the format each time,
    // including after bash's in-place rewrite of "%l(" to "%((".
    testunclosed(perm, "%(",  "%(");
    testunclosed(perm, "%l(", "%((");
}


// Time zones: Windows zones as POSIX TZ strings, and zoneinfo files

static WinDate rule(i32 month, i32 week, i32 dow, i32 hour, i32 minute = 0)
{
    WinDate d = {};
    d.month  = month;
    d.day    = week;
    d.dow    = dow;
    d.hour   = hour;
    d.minute = minute;
    return d;
}

// The zone's offset and name at t, as "-0400 EDT"
static Str zoneat(Arena *a, PfZone *z, i64 t)
{
    PfTm tm = localtm(t, z);
    Buf  b(a, 32);
    i64  off = tm.gmtoff / 60;
    putbyte(&b, off<0 ? '-' : '+');
    off = off<0 ? -off : off;
    putbyte(&b, (u8)('0' + off/600%10));
    putbyte(&b, (u8)('0' + off/60%10));
    putbyte(&b, (u8)('0' + off%60/10));
    putbyte(&b, (u8)('0' + off%10));
    putbyte(&b, ' ');
    print(&b, tm.zone);
    return finish(&b);
}

static void testwintz(Arena perm)
{
    struct {
        i32     bias, stdbias, dstbias;
        WinDate stddate, dstdate;
        Str     stdname, dstname;
        b32     nodst;
        Str     want;
        i64     t;       // a time to check
        Str     wantat;  // the zone then
    } cases[] = {
        {300, 0, -60, rule(11, 1, 0, 2), rule(3, 2, 0, 2),
         "Eastern Standard Time", "Eastern Daylight Time", 0,
         "EST5EDT,M3.2.0,M11.1.0", 1690000000, "-0400 EDT"},
        {300, 0, -60, rule(11, 1, 0, 2), rule(3, 2, 0, 2),
         "Eastern Standard Time", "Eastern Daylight Time", 1,
         "EST5", 1690000000, "-0500 EST"},
        {0, 0, 0, {}, {}, "Coordinated Universal Time", "Coordinated Universal Time", 0,
         "CUT0", 1690000000, "+0000 CUT"},
        {-330, 0, 0, {}, {}, "India Standard Time", "India Daylight Time", 0,
         "IST-5:30", 0, "+0530 IST"},
        {-345, 0, -60, {}, {}, "Nepal Standard Time", "Nepal Daylight Time", 0,
         "NST-5:45", 0, "+0545 NST"},
        {-765, 0, -60, rule(4, 1, 0, 3, 45), rule(9, 5, 0, 2, 45),
         "Chatham Islands Standard Time", "Chatham Islands Daylight Time", 0,
         "CIST-12:45CIDT,M9.5.0/2:45,M4.1.0/3:45", 1700000000, "+1345 CIDT"},
        {-630, 0, -30, rule(4, 1, 0, 2), rule(10, 1, 0, 2),
         "Lord Howe Standard Time", "Lord Howe Daylight Time", 0,
         "LHST-10:30LHDT-11,M10.1.0,M4.1.0", 1690000000, "+1030 LHST"},
        {-600, 0, -60, rule(4, 1, 0, 3), rule(10, 1, 0, 2),
         "AUS Eastern Standard Time", "AUS Eastern Daylight Time", 0,
         "AUSEST-10AUSEDT,M10.1.0,M4.1.0/3", 1700000000, "+1100 AUSEDT"},
        {-60, 0, -60, rule(10, 5, 0, 3), rule(3, 5, 0, 2),
         "Mitteleurop\xc3\xa4ische Zeit", "Mitteleurop\xc3\xa4ische Sommerzeit", 0,
         "<+0100>-1<+0200>,M3.5.0,M10.5.0/3", 1690000000, "+0200 +0200"},
        {180, 0, -60, rule(2, 3, 6, 23, 59), rule(11, 1, 6, 23, 59),
         "E. South America Standard Time", "E. South America Daylight Time", 0,
         "ESAST3ESADT,M11.1.6/23:59,M2.3.6/23:59", 1690000000, "-0300 ESAST"},
        {0, 0, -60, rule(10, 5, 0, 2), rule(3, 5, 0, 1),
         "GMT Standard Time", "GMT Daylight Time", 0,
         "GMTST0GMTDT,M3.5.0/1,M10.5.0", 1690000000, "+0100 GMTDT"},
        {-120, 0, -60, {}, {}, "", "", 0, "<+0200>-2", 0, "+0200 +0200"},
    };
    for (iz i = 0; i < countof(cases); i++) {
        Arena   a = perm;
        WinZone w = {};
        w.bias    = cases[i].bias;
        w.stdbias = cases[i].stdbias;
        w.dstbias = cases[i].dstbias;
        w.stddate = cases[i].stddate;
        w.dstdate = cases[i].dstdate;
        w.stdname = cases[i].stdname;
        w.dstname = cases[i].dstname;
        w.nodst   = cases[i].nodst;
        Str got = wintz(&a, &w);
        check(got==cases[i].want, i, "wintz");
        if (got != cases[i].want) {
            show("want", cases[i].want);
            show("got", got);
        }
        PfZone z  = parsezone(got);
        Str    at = zoneat(&a, &z, cases[i].t);
        check(z.ok && at==cases[i].wantat, i, "wintz parses back");
        if (at != cases[i].wantat) {
            show("want", cases[i].wantat);
            show("got", at);
        }
    }

    // Fixed dates (wYear set) count days without February 29, and a
    // change at 23:59:59.999 rounds to the next day's start
    Arena   a = perm;
    WinZone w = {};
    w.bias    = 300;
    w.dstbias = -60;
    w.stdname = "Eastern Standard Time";
    w.dstname = "Eastern Daylight Time";
    w.dstdate = rule(3, 29, 0, 2);
    w.dstdate.year = 2026;
    w.stddate = rule(10, 25, 0, 3);
    w.stddate.year = 2026;
    check(wintz(&a, &w)=="EST5EDT,J88,J298/3", 0, "wintz fixed dates");
    w.dstdate = rule(3, 5, 6, 23, 59);
    w.dstdate.second = 59;
    w.dstdate.ms     = 999;
    w.stddate = rule(10, 5, 6, 23, 59);
    w.stddate.second = 59;
    check(wintz(&a, &w)=="EST5EDT,M3.5.6/24,M10.5.6/23:59:59", 0, "wintz end of day");
    w.stddate.month = 13;
    check(wintz(&a, &w)=="EST5", 0, "wintz bad month");
}

// A zoneinfo file: a version 1 block (32-bit times) and, from version
// 2, a 64-bit block and the footer.
struct Tzif {
    i64 *times;
    u8  *index;
    iz   ntimes;
    i32 *offs;
    u8  *dsts;
    u8  *abbr;    // index of each type's name in chars
    iz   ntypes;
    Str  chars;
    Str  footer;  // null: version 1
};

static void put32(Buf *b, u32 v)
{
    for (i32 s = 24; s >= 0; s -= 8) putbyte(b, (u8)(v >> s));
}

static void tzifblock(Buf *b, Tzif *z, b32 wide, u8 version)
{
    print(b, "TZif");
    putbyte(b, version);
    for (i32 i = 0; i < 15; i++) putbyte(b, 0);
    u32 counts[] = {0, 0, 0, (u32)z->ntimes, (u32)z->ntypes, (u32)z->chars.len};
    for (i32 i = 0; i < 6; i++) put32(b, counts[i]);
    for (iz i = 0; i < z->ntimes; i++) {
        if (wide) put32(b, (u32)((u64)z->times[i] >> 32));
        put32(b, (u32)z->times[i]);
    }
    for (iz i = 0; i < z->ntimes; i++) putbyte(b, z->index[i]);
    for (iz i = 0; i < z->ntypes; i++) {
        put32(b, (u32)z->offs[i]);
        putbyte(b, z->dsts[i]);
        putbyte(b, z->abbr[i]);
    }
    print(b, z->chars);
}

static Str maketzif(Arena *a, Tzif *z)
{
    Buf b(a, 256);
    u8  version = z->footer.data ? '2' : 0;
    tzifblock(&b, z, 0, version);
    if (version) {
        tzifblock(&b, z, 1, version);
        putbyte(&b, '\n');
        print(&b, z->footer);
        putbyte(&b, '\n');
    }
    return finish(&b);
}

static void testtzif(Arena perm)
{
    // As t/exec/874's hist: XST before and after XDT, then YST3
    i64 times[] = {1000000000, 1100000000};
    u8  index[] = {1, 0};
    i32 offs[]  = {-18000, -14400};
    u8  dsts[]  = {0, 1};
    u8  abbr[]  = {0, 4};
    Tzif hist   = {times, index, 2, offs, dsts, abbr, 2, Str("XST\0XDT\0"), "YST3"};
    struct {
        i64 t;
        Str want;
    } at[] = {
        {900000000,  "-0500 XST"},
        {999999999,  "-0500 XST"},
        {1000000000, "-0400 XDT"},
        {1050000000, "-0400 XDT"},
        {1100000000, "-0300 YST"},  // from the last transition, the footer
        {1200000000, "-0300 YST"},
    };
    Arena  a = perm;
    PfZone z = parsetzif(&a, maketzif(&a, &hist));
    check(z.ok && z.hasrule && z.ntimes==2 && z.ntypes==2, 0, "tzif v2");
    for (iz i = 0; z.ok && i < countof(at); i++) {
        Str got = zoneat(&a, &z, at[i].t);
        check(got==at[i].want, i, "tzif v2 times");
        if (got != at[i].want) show("got", got);
    }

    // Version 1: no footer, the last transition's type stays
    Tzif v1 = hist;
    v1.footer = {};
    a = perm;
    z = parsetzif(&a, maketzif(&a, &v1));
    check(z.ok && !z.hasrule, 0, "tzif v1");
    check(z.ok && zoneat(&a, &z, 1200000000)=="-0500 XST", 0, "tzif v1 after");
    check(z.ok && zoneat(&a, &z, 0)=="-0500 XST", 0, "tzif v1 before");

    // No transitions: the first standard type, footer or not
    i32 offs2[] = {-14400, -18000};
    Tzif none = {0, 0, 0, offs2, dsts, abbr, 2, Str("XDT\0XST\0"), "EST5EDT,M3.2.0,M11.1.0"};
    u8   abbr2[] = {4, 0};
    u8   dsts2[] = {0, 1};
    none.abbr = abbr2;
    none.dsts = dsts2;
    a = perm;
    z = parsetzif(&a, maketzif(&a, &none));
    check(z.ok && zoneat(&a, &z, 1690000000)=="-0400 XST", 0, "tzif no transitions");

    // An empty or broken footer leaves the table in charge
    Tzif nofoot = hist;
    nofoot.footer = "";
    a = perm;
    z = parsetzif(&a, maketzif(&a, &nofoot));
    check(z.ok && !z.hasrule && zoneat(&a, &z, 1200000000)=="-0500 XST", 0, "tzif empty footer");
    nofoot.footer = "<bad";
    a = perm;
    z = parsetzif(&a, maketzif(&a, &nofoot));
    check(z.ok && !z.hasrule, 0, "tzif bad footer");

    // Malformed files
    a = perm;
    Str good = maketzif(&a, &hist);
    for (iz n = 0; n < 44+5+12+8; n++) {
        Arena t = a;
        check(!parsetzif(&t, takehead(good, n)).ok, n, "tzif truncated");
    }
    Tzif bad = hist;
    u8 badindex[] = {1, 2};
    bad.index = badindex;
    check(!parsetzif(&a, maketzif(&a, &bad)).ok, 0, "tzif bad index");
    bad = hist;
    i64 badtimes[] = {1100000000, 1000000000};
    bad.times = badtimes;
    check(!parsetzif(&a, maketzif(&a, &bad)).ok, 0, "tzif unordered");
    bad = hist;
    u8 badabbr[] = {0, 8};
    bad.abbr = badabbr;
    check(!parsetzif(&a, maketzif(&a, &bad)).ok, 0, "tzif bad abbreviation");
    bad = hist;
    bad.ntypes = 0;
    bad.ntimes = 0;
    check(!parsetzif(&a, maketzif(&a, &bad)).ok, 0, "tzif no types");
    check(!parsetzif(&a, "TZjf2").ok, 0, "tzif magic");

    // Random bytes after a valid header must never crash the parser
    u64 rng = 1;
    for (i32 i = 0; i < 100000; i++) {
        Arena t = a;
        Str   s = clone(&t, good);
        iz    n = (iz)(s.len * (rng>>33) / ((u64)1<<31));
        for (i32 k = 0; k < 4 && n<s.len; k++) {
            rng = rng*0x9e3779b97f4a7c15u + 1;
            s[(n + (iz)(rng>>59)) % s.len] = (u8)(rng >> 40);
        }
        rng = rng*0x9e3779b97f4a7c15u + 1;
        PfZone r = parsetzif(&t, takehead(s, (iz)((rng>>33) % (u64)(s.len+1))));
        if (r.ok) zoneat(&t, &r, (i64)(rng>>20) - ((i64)1<<43));
    }
    check(1, 0, "tzif random bytes");
}

int main(int argc, char **argv)
{
    char const *path = argc>1 ? argv[1] : "t/unit/printf.golden";

    iz    cap = (iz)1<<28;
    byte *mem = (byte *)malloc((size_t)cap);
    Arena perm = {mem, mem+cap, 0};

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "printf_test: cannot open %s\n", path);
        return 2;
    }
    Str src = {};
    src.data = (u8 *)perm.beg;
    src.len  = (iz)fread(src.data, 1, (size_t)(cap/4), f);
    fclose(f);
    perm.beg += src.len;

    Slice<Record> records = parse(&perm, src);
    for (iz i = 0; i < records.len; i++) {
        testrecord(perm, &records[i]);
    }
    testinline(perm);
    testwintz(perm);
    testtzif(perm);

    printf("%d tests, %d failed\n", ntests, nfailed);
    return nfailed != 0;
}
