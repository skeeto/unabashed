// unigen: generate unicode.cpp's character class and case tables from
// glibc's C.UTF-8 (the oracle's locale), by enumerating every code
// point through iswctype, towupper and towlower.
//   $ cc -o unigen tools/unigen.c && ./unigen >tables.txt
// This is free and unencumbered software released into the public domain.
#include <locale.h>
#include <stdio.h>
#include <wctype.h>

typedef int b32;

enum {
    UC_ALPHA = 1<<0, UC_DIGIT = 1<<1, UC_SPACE = 1<<2, UC_PRINT = 1<<3,
    UC_CNTRL = 1<<4, UC_XDIGIT = 1<<5, UC_BLANK = 1<<6,
    UC_UPPER = 1<<7,  // iswupper differs from towlower(c)!=c
    UC_LOWER = 1<<8,  // iswlower differs from towupper(c)!=c
};

static unsigned classes(int c)
{
    if (c>=0xd800 && c<0xe000) return 0;
    return (iswalpha(c)  ? UC_ALPHA  : 0) | (iswdigit(c) ? UC_DIGIT : 0) |
           (iswspace(c)  ? UC_SPACE  : 0) | (iswprint(c) ? UC_PRINT : 0) |
           (iswcntrl(c)  ? UC_CNTRL  : 0) | (iswxdigit(c)? UC_XDIGIT: 0) |
           (iswblank(c)  ? UC_BLANK  : 0) |
           // upper and lower as exceptions to "has a case mapping", so
           // that alternating blocks are not one range per code point
           ((!iswupper(c)) != (towlower(c)==(wint_t)c) ? UC_UPPER : 0) |
           ((!iswlower(c)) != (towupper(c)==(wint_t)c) ? UC_LOWER : 0);
}

static long casedelta(int c, int upper)
{
    if (c>=0x110000 || (c>=0xd800 && c<0xe000)) return 0;
    return (long)(upper ? towupper(c) : towlower(c)) - c;
}

// Runs of code points whose mapping adds the same delta, spaced 1 or 2
// apart (the alternating upper/lower blocks). A run spaced 2 apart
// skips only code points without a mapping, so runs never overlap.
static void caseruns(char const *name, int upper)
{
    printf("static CaseRun const %s[] = {\n", name);
    int n = 0, lo = -1, hi = -1, step = 0;
    long delta = 0;
    for (int c = 0; c <= 0x110000; c++) {
        long d = casedelta(c, upper);
        if (!d) continue;
        b32 gap = c-hi==2 && !casedelta(c-1, upper);
        if (lo>=0 && d==delta && ((step==0 && (c-hi==1 || gap)) || (step==1 && c-hi==1) ||
                                  (step==2 && gap))) {
            if (!step) step = c - hi;
            hi = c;
            continue;
        }
        if (lo >= 0) {
            printf("    {0x%05x, 0x%05x, %d, %6ld},\n", lo, hi, step ? step : 1, delta);
            n++;
        }
        lo = hi = c;
        step = 0;
        delta = d;
    }
    if (lo >= 0) {
        printf("    {0x%05x, 0x%05x, %d, %6ld},\n", lo, hi, step ? step : 1, delta);
        n++;
    }
    printf("};  // %d runs\n\n", n);
}

int main(void)
{
    if (!setlocale(LC_ALL, "C.UTF-8")) return 1;
    // Each range's first code point and its classes, as c<<11 | mask
    printf("static u32 const classranges[] = {");
    unsigned prev = ~0u;
    int n = 0;
    for (int c = 0; c < 0x110000; c++) {
        unsigned m = classes(c);
        if (m == prev) continue;
        printf("%s0x%08x,", n%6 ? " " : "\n    ", (unsigned)c<<11 | m);
        prev = m;
        n++;
    }
    printf("\n};  // %d ranges\n\n", n);
    caseruns("upperruns", 1);
    caseruns("lowerruns", 0);
    return 0;
}
