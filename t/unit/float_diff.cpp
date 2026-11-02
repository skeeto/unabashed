// Differential tests of float.cpp against glibc's strtold and printf
// $ g++ -std=c++20 -g3 -O1 -fsanitize=undefined -fsanitize-trap
//       -o float_diff t/unit/float_diff.cpp
// $ ./float_diff --random [SEED [COUNT]]   compare on generated cases
// $ ./float_diff <case                     one case (AFL), trap on a difference
//
// x86-64 Linux only: glibc's long double is the reference. Everything
// happens in memory. Two kinds of case are compared:
//   - formatting: random x87 bit patterns (normals, subnormals, zero,
//     inf, nan) through random %L conversions, flags, widths and
//     precisions, against snprintf;
//   - parsing: random numeric text through strtold, comparing the
//     bits, the end and ERANGE, and then printing the result.
// AFL input: a format byte (conversion, flags), a width byte, a
// precision byte, then the number's text.
//
// This is free and unencumbered software released into the public domain.
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../base.cpp"
#include "../../float.cpp"

[[noreturn]] static void os_oom()
{
    fputs("float_diff: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}

static u64 rng;

static u32 rand32()
{
    rng = rng*0x3243f6a8885a308d + 1;
    return (u32)(rng >> 32);
}

static long double told(F80 v)
{
    u8 b[16] = {};
    memcpy(b, &v.m, 8);
    unsigned short se = (unsigned short)(v.e | (v.neg ? 0x8000 : 0));
    memcpy(b+8, &se, 2);
    long double r;
    memcpy(&r, b, sizeof(r));
    return r;
}

static F80 fromld(long double x)
{
    u8 b[16];
    memcpy(b, &x, sizeof(x));
    F80 r = {};
    unsigned short se;
    memcpy(&r.m, b, 8);
    memcpy(&se, b+8, 2);
    r.e   = se & 0x7fff;
    r.neg = se >> 15;
    return r;
}

struct Spec {
    u8  conv;
    u8  flags;   // 1 alt, 2 plus, 4 space, 8 left, 16 zero
    i32 width;   // -1 none
    i32 prec;    // -1 none
};

static Str glibcfmt(Arena *a, Spec s, long double x)
{
    char f[32];
    char *p = f;
    *p++ = '%';
    if (s.flags & 1)  *p++ = '#';
    if (s.flags & 2)  *p++ = '+';
    if (s.flags & 4)  *p++ = ' ';
    if (s.flags & 8)  *p++ = '-';
    if (s.flags & 16) *p++ = '0';
    *p++ = '*';
    *p++ = '.';
    *p++ = '*';
    *p++ = 'L';
    *p++ = (char)s.conv;
    *p   = 0;
    int n = snprintf(0, 0, f, s.width<0 ? 0 : s.width, s.prec, x);
    u8 *buf = allocbytes(a, n+1);
    snprintf((char *)buf, (uz)n+1, f, s.width<0 ? 0 : s.width, s.prec, x);
    return {buf, n};
}

static Str ourfmt(Arena *a, Spec s, F80 v)
{
    F80Spec fs = {};
    fs.alt   = s.flags & 1;
    fs.plus  = !!(s.flags & 2);
    fs.space = !!(s.flags & 4);
    fs.left  = !!(s.flags & 8);
    fs.zero  = !!(s.flags & 16);
    fs.width = s.width<0 ? 0 : s.width;
    fs.prec  = s.prec;
    Buf b(a, f80bound(&fs));
    Arena t = *a;
    f80format(&b, v, s.conv, &fs, t);
    return finish(&b);
}

static void show(char const *what, Str s)
{
    fprintf(stderr, "%s: %.*s\n", what, (int)s.len, s.data);
}

static b32 checkfmt(Arena scratch, Spec s, F80 v)
{
    Str want = glibcfmt(&scratch, s, told(v));
    Str got  = ourfmt(&scratch, s, v);
    if (!(want == got)) {
        fprintf(stderr, "FORMAT m=%016llx e=%d neg=%d conv=%c flags=%d width=%d prec=%d\n",
                (unsigned long long)v.m, v.e, v.neg, s.conv, s.flags, s.width, s.prec);
        show("want", want);
        show("got ", got);
        return 0;
    }
    return 1;
}

static b32 checkparse(Arena scratch, Str text, Spec s)
{
    char *z = (char *)allocbytes(&scratch, text.len+1);
    memcpy(z, text.data, (uz)text.len);
    z[text.len] = 0;
    char *end;
    errno = 0;
    long double x = strtold(z, &end);
    b32 range = errno == ERANGE;
    F80 want  = fromld(x);
    iz  wend  = end - z;

    F80Parse got = f80parse(text, scratch);
    b32 nanboth  = want.e==0x7fff && got.v.e==0x7fff && want.m<<1 && got.v.m<<1;
    b32 same = got.end==wend && got.range==range && got.v.neg==want.neg &&
               (nanboth || (got.v.m==want.m && got.v.e==want.e));
    if (!same) {
        fprintf(stderr, "PARSE \"%s\"\n", z);
        fprintf(stderr, "want m=%016llx e=%d neg=%d end=%td range=%d\n",
                (unsigned long long)want.m, want.e, want.neg, wend, range);
        fprintf(stderr, "got  m=%016llx e=%d neg=%d end=%td range=%d\n",
                (unsigned long long)got.v.m, got.v.e, got.v.neg, got.end, got.range);
        return 0;
    }
    return checkfmt(scratch, s, want);
}

static Spec randspec()
{
    Spec s = {};
    s.conv  = (u8)"aAeEfFgG"[rand32()%8];
    s.flags = (u8)(rand32()%4 ? rand32()%32 : 0);
    s.width = rand32()%3 ? -1 : (i32)(rand32()%40);
    switch (rand32()%6) {
    case 0:  s.prec = -1; break;
    case 1:  s.prec = (i32)(rand32()%4); break;
    case 2:  s.prec = (i32)(rand32()%20); break;
    case 3:  s.prec = (i32)(rand32()%70); break;
    case 4:  s.prec = (i32)(rand32()%(s.conv=='f' || s.conv=='F' ? 6000 : 400)); break;
    default: s.prec = -1;
    }
    return s;
}

static F80 randf80()
{
    F80 v = {};
    v.neg = rand32() & 1;
    u64 m = (u64)rand32()<<32 | rand32();
    switch (rand32()%8) {
    case 0:  m &= ~0ull << (rand32()%64); break;     // short significands
    case 1:  m |= ~0ull >> (rand32()%64); break;     // runs of ones
    case 2:  m = (m & 0xff00000000000000) | (rand32()%2 ? 0x80 : 0x7f); break;
    }
    switch (rand32()%10) {
    case 0:  v.e = 0; m >>= rand32()%64; break;      // subnormal
    case 1:  v.e = 0x7fff; m = rand32()%2 ? 0 : m; break;
    case 2:  v.e = (i32)(rand32()%0x7ffe) + 1; break;
    case 3:  v.e = 1 + (i32)(rand32()%40); break;
    case 4:  v.e = 0x7ffe - (i32)(rand32()%40); break;
    default: v.e = 16383 + (i32)(rand32()%200) - 100;
    }
    if (v.e) {
        m |= 1ull << 63;
    } else {
        m &= ~(1ull << 63);
    }
    if (v.e == 0x7fff) {
        m |= 1ull << 63;
    }
    v.m = m;
    return v;
}

static Str randtext(Arena *a)
{
    Buf b(a, 64);
    if (rand32()%8 == 0) putbyte(&b, " \t\n"[rand32()%3]);
    if (rand32()%3 == 0) putbyte(&b, "+-"[rand32()%2]);
    switch (rand32()%12) {
    case 0: {
        Str words[] = {"inf", "INF", "infinity", "Infinit", "nan", "NaN(x_1)",
                       "nan(", "nan()", "in", "na", "0x", "0x.", "0x.p1",
                       ".", "e5", "", "0e", "1e+", "1e-x", "0xp3"};
        print(&b, words[rand32()%countof(words)]);
    } break;
    case 1:
    case 2: {
        print(&b, rand32()%2 ? Str("0x") : Str("0X"));
        i32 n = (i32)(rand32()%30) + 1;
        i32 dot = rand32()%2 ? (i32)(rand32()%(u32)n) : -1;
        for (i32 i = 0; i < n; i++) {
            if (i == dot) putbyte(&b, '.');
            u8 c = (u8)"0123456789abcdefABCDEF0000ffff"[rand32()%30];
            putbyte(&b, c);
        }
        if (rand32()%3) {
            putbyte(&b, "pP"[rand32()%2]);
            if (rand32()%2) putbyte(&b, "+-"[rand32()%2]);
            print(&b, (i64)(rand32()%(rand32()%2 ? 20 : 17000)));
        }
    } break;
    default: {
        i32 n = (i32)(rand32()%(rand32()%10 ? 25 : 200)) + 1;
        if (rand32()%50 == 0) n = 13000 + (i32)(rand32()%2000);
        i32 dot = rand32()%2 ? (i32)(rand32()%(u32)(n+1)) : -1;
        u32 style = rand32()%4;
        for (i32 i = 0; i < n; i++) {
            if (i == dot) putbyte(&b, '.');
            u8 c;
            switch (style) {
            case 0:  c = (u8)('0' + rand32()%10); break;
            case 1:  c = i==0 ? (u8)('1' + rand32()%9) : (rand32()%8 ? '0' : '5'); break;
            case 2:  c = rand32()%5 ? '9' : (u8)('0' + rand32()%10); break;
            default: c = i<n-1 ? (u8)('0' + rand32()%10) : '5';
            }
            putbyte(&b, c);
        }
        if (rand32()%2) {
            putbyte(&b, "eE"[rand32()%2]);
            if (rand32()%2) putbyte(&b, "+-"[rand32()%2]);
            u32 r = rand32()%4;
            i64 x = r==0 ? rand32()%40 : r==1 ? 4900 + rand32()%100 : r==2 ? rand32()%5000 : 4920 + rand32()%40;
            print(&b, x);
        }
    }
    }
    if (rand32()%10 == 0) putbyte(&b, "x .e"[rand32()%4]);
    return finish(&b);
}

static Arena newarena(iz cap)
{
    Arena a = {};
    a.beg = (byte *)malloc((uz)cap);
    a.end = a.beg + cap;
    return a;
}

int main(int argc, char **argv)
{
    Arena perm = newarena((iz)1<<28);

    if (argc>1 && !strcmp(argv[1], "--random")) {
        rng = argc>2 ? strtoull(argv[2], 0, 10) : 1;
        long count = argc>3 ? strtol(argv[3], 0, 10) : 100000;
        long fails = 0;
        for (long i = 0; i < count && fails < 20; i++) {
            Arena scratch = perm;
            if (rand32() % 2) {
                fails += !checkfmt(scratch, randspec(), randf80());
            } else {
                Str text = randtext(&scratch);
                fails += !checkparse(scratch, text, randspec());
            }
        }
        printf("%ld cases, %ld failed\n", count, fails);
        return !!fails;
    }

    // AFL: one case from stdin
    static u8 buf[1<<16];
    iz len = (iz)fread(buf, 1, sizeof(buf), stdin);
    if (len < 3) {
        return 0;
    }
    Spec s = {};
    s.conv  = (u8)"aAeEfFgG"[buf[0]&7];
    s.flags = (u8)(buf[0] >> 3);
    s.width = buf[1]==255 ? -1 : buf[1];
    s.prec  = buf[2]==255 ? -1 : buf[2]*4;
    Str text = {buf+3, len-3};
    for (iz i = 0; i < text.len; i++) {
        if (!text[i]) text.len = i;  // strtold stops at a null byte
    }
    if (!checkparse(perm, text, s)) {
        __builtin_trap();
    }
    return 0;
}
