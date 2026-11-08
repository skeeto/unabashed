// Soft x87 long double: strtold and printf's floating point conversions
//
// bash's printf reads floating point arguments with strtold and prints
// them with the C library's %Lf %Le %Lg %La. On x86-64 glibc, the
// reference platform, long double is the x87 80-bit format: a 64-bit
// significand with an explicit integer bit and a 15-bit exponent. This
// module does both halves in software so that every platform prints
// what glibc prints: parsing is correctly rounded (half to even, with
// glibc's ERANGE rules) and output is the exact decimal expansion,
// rounded half to even, computed with arena bignums.
//
// This is free and unencumbered software released into the public domain.


// Interface

struct F80 {
    u64 m;    // significand, bit 63 the integer bit (clear if e is 0)
    i32 e;    // biased exponent: 0 zero or subnormal, 0x7fff inf or nan
    b32 neg;
};

struct F80Parse {
    F80 v;
    iz  end;    // just past the number, 0 if there is none
    b32 range;  // ERANGE: overflow, or an inexact subnormal or zero
};

struct F80Spec {
    b32 alt;
    b32 plus;
    b32 space;
    b32 left;
    b32 zero;
    i64 width;
    i64 prec;   // -1 if none
};

static F80Parse f80parse(Str, Arena);
static F80      f80fromint(i64);
static iz       f80bound(F80Spec *);
static void     f80format(Buf *, F80, u8 conv, F80Spec *, Arena);

enum : i32 {
    F80_BIAS = 16383,
    F80_MAXE = 0x7fff,
    F80_MAXDIGITS = 12000,  // more significant digits never decide rounding
};


// Bignums: little-endian 32-bit limbs, no leading zero limbs

struct Big {
    u32 *w;
    iz   len;
    iz   cap;
};

static void bigreserve(Big *b, Arena *a, iz need)
{
    if (need <= b->cap) {
        return;
    }
    iz   cap = b->cap*2>need ? b->cap*2 : need;
    u32 *w   = alloc<u32>(a, cap);
    copybytes(w, b->w, b->len*(iz)sizeof(u32));
    b->w   = w;
    b->cap = cap;
}

static Big bignew(Arena *a, iz cap, u64 v)
{
    Big r = {};
    bigreserve(&r, a, cap>2 ? cap : 2);
    for (; v; v >>= 32) {
        r.w[r.len++] = (u32)v;
    }
    return r;
}

// b = b*mul + add
static void bigmuladd(Big *b, Arena *a, u32 mul, u32 add)
{
    u64 carry = add;
    for (iz i = 0; i < b->len; i++) {
        u64 t = (u64)b->w[i]*mul + carry;
        b->w[i] = (u32)t;
        carry   = t >> 32;
    }
    if (carry) {
        bigreserve(b, a, b->len+1);
        b->w[b->len++] = (u32)carry;
    }
}

static void bigmulpow10(Big *b, Arena *a, i64 k)
{
    for (; k >= 9; k -= 9) {
        bigmuladd(b, a, 1000000000, 0);
    }
    u32 p = 1;
    for (; k > 0; k--) {
        p *= 10;
    }
    if (p > 1) {
        bigmuladd(b, a, p, 0);
    }
}

static void bigshl(Big *b, Arena *a, i64 bits)
{
    if (!b->len || bits<=0) {
        return;
    }
    iz  limbs = (iz)(bits / 32);
    i32 r     = (i32)(bits % 32);
    bigreserve(b, a, b->len+limbs+1);
    b->w[b->len+limbs] = 0;
    for (iz i = b->len-1; i >= 0; i--) {
        u32 v = b->w[i];
        if (r) {
            b->w[i+limbs+1] |= v >> (32 - r);
        }
        b->w[i+limbs] = v << r;
    }
    for (iz i = 0; i < limbs; i++) {
        b->w[i] = 0;
    }
    b->len += limbs + 1;
    for (; b->len && !b->w[b->len-1]; b->len--) {}
}

static i64 bigbits(Big b)
{
    if (!b.len) {
        return 0;
    }
    return 32*(i64)b.len - __builtin_clz(b.w[b.len-1]);
}

static i32 bigcmp(Big x, Big y)
{
    if (x.len != y.len) {
        return x.len<y.len ? -1 : 1;
    }
    for (iz i = x.len-1; i >= 0; i--) {
        if (x.w[i] != y.w[i]) {
            return x.w[i]<y.w[i] ? -1 : 1;
        }
    }
    return 0;
}

static void bigsub(Big *x, Big y)  // x >= y
{
    i64 borrow = 0;
    for (iz i = 0; i < x->len; i++) {
        i64 t = (i64)x->w[i] - (i<y.len ? y.w[i] : 0) - borrow;
        borrow  = t < 0;
        x->w[i] = (u32)t;
    }
    for (; x->len && !x->w[x->len-1]; x->len--) {}
}

static u32 bigdivsmall(Big *b, u32 d)  // returns the remainder
{
    u64 rem = 0;
    for (iz i = b->len-1; i >= 0; i--) {
        u64 t = rem<<32 | b->w[i];
        b->w[i] = (u32)(t / d);
        rem     = t % d;
    }
    for (; b->len && !b->w[b->len-1]; b->len--) {}
    return (u32)rem;
}


// Rounding to the format

struct F80Round {
    F80 v;
    b32 range;
};

// Round m * 2^(x-63), m normalized (bit 63 set), plus a half bit and a
// sticky bit below m, to the nearest long double, ties to even. As x87
// glibc, tininess is detected after rounding.
static F80Round f80round(b32 neg, u64 m, b32 half, b32 sticky, i64 x)
{
    F80Round r = {};
    r.v.neg = neg;
    i64 e = x + F80_BIAS;
    if (e >= F80_MAXE) {
        r.v.e   = F80_MAXE;
        r.v.m   = 1ull << 63;
        r.range = 1;
        return r;
    }

    if (e >= 1) {
        if (half && (sticky || (m&1))) {
            if (!++m) {
                m = 1ull << 63;
                e++;
            }
        }
        if (e >= F80_MAXE) {
            r.v.e   = F80_MAXE;
            r.v.m   = 1ull << 63;
            r.range = 1;
            return r;
        }
        r.v.m = m;
        r.v.e = (i32)e;
        return r;
    }

    // Subnormal: shift into place, keeping the bits shifted out
    i64 shift = 1 - e;
    u64 nm = 0;
    b32 nh = 0, ns = 0;
    if (shift > 64) {
        ns = 1;
    } else if (shift == 64) {
        nh = (b32)(m >> 63);
        ns = (m<<1) || half || sticky;
    } else {
        nm = m >> shift;
        nh = (b32)(m >> (shift-1) & 1);
        ns = (m & ((1ull<<(shift-1)) - 1)) || half || sticky;
    }
    b32 tiny = !(e==0 && m==~0ull && half);
    if (nh && (ns || (nm&1))) {
        nm++;
    }
    r.v.m   = nm;
    r.v.e   = nm>>63 ? 1 : 0;
    r.range = tiny && (nh || ns);
    return r;
}

static F80 f80fromint(i64 v)
{
    F80 r = {};
    r.neg = v < 0;
    u64 m = v<0 ? -(u64)v : (u64)v;
    if (m) {
        i32 z = __builtin_clzll(m);
        r.m = m << z;
        r.e = F80_BIAS + 63 - z;
    }
    return r;
}


// Parsing (strtold in the C locale)

static b32 matchci(Str s, iz i, Str word)
{
    if (s.len-i < word.len) {
        return 0;
    }
    for (iz k = 0; k < word.len; k++) {
        if (lowercase(s[i+k]) != word[k]) {
            return 0;
        }
    }
    return 1;
}

static i32 f80hexval(u8 c)  // -1 if not a hex digit
{
    if (c>='0' && c<='9') return c - '0';
    if (c>='a' && c<='f') return c - 'a' + 10;
    if (c>='A' && c<='F') return c - 'A' + 10;
    return -1;
}

static u8 f80byte(Str s, iz i)  // zero past the end
{
    return i<s.len ? s[i] : 0;
}

// An exponent's digits at s[*i], clamped, consumed only when present.
static i64 f80exponent(Str s, iz *i)
{
    iz  k   = *i + 1;
    b32 neg = 0;
    if (f80byte(s, k)=='+' || f80byte(s, k)=='-') {
        neg = s[k++] == '-';
    }
    if (!digit(f80byte(s, k))) {
        return 0;
    }
    i64 v = 0;
    for (; digit(f80byte(s, k)); k++) {
        v = v<(1ll<<40) ? v*10 + (s[k] - '0') : v;
    }
    *i = k;
    return neg ? -v : v;
}

static F80Parse f80hex(Str s, iz i, b32 neg)
{
    F80Parse r = {};
    // Up to 18 significant nibbles, then a sticky bit for the rest
    u8  nib[18] = {};
    i32 nn      = 0;
    b32 sticky  = 0;
    i64 bexp    = 0;
    b32 any     = 0;
    b32 point   = 0;
    for (;; i++) {
        u8 c = f80byte(s, i);
        if (c=='.' && !point) {
            point = 1;
            continue;
        }
        i32 d = f80hexval(c);
        if (d < 0) {
            break;
        }
        any = 1;
        if (!nn && !d) {
            bexp -= point ? 4 : 0;
        } else if (nn < countof(nib)) {
            nib[nn++] = (u8)d;
            bexp -= point ? 4 : 0;
        } else {
            sticky |= d != 0;
            bexp   += point ? 0 : 4;
        }
    }
    if (!any) {
        return r;
    }
    r.end = i;
    if (f80byte(s, i)=='p' || f80byte(s, i)=='P') {
        bexp += f80exponent(s, &r.end);
    }
    r.v.neg = neg;
    if (!nn) {
        return r;  // zero
    }

    // Normalize the nibbles' bits: 64 bits, a half bit, a sticky bit
    i32 lead = 0;
    for (; !(nib[0] & (8>>lead)); lead++) {}
    i32 nbits = 4*nn - lead;
    u64 m     = 0;
    b32 half  = 0;
    for (i32 k = 0; k < nbits; k++) {
        i32 bit = lead + k;
        u32 b   = nib[bit/4] >> (3 - bit%4) & 1;
        if (k < 64) {
            m |= (u64)b << (63 - k);
        } else if (k == 64) {
            half = (b32)b;
        } else {
            sticky |= (b32)b;
        }
    }
    F80Round fr = f80round(neg, m, half, sticky, bexp + 4*nn - lead - 1);
    r.v     = fr.v;
    r.range = fr.range;
    return r;
}

static F80Parse f80parse(Str s, Arena scratch)
{
    F80Parse r = {};
    iz i = 0;
    for (; i<s.len && whitespace(s[i]); i++) {}
    b32 neg = 0;
    if (f80byte(s, i)=='+' || f80byte(s, i)=='-') {
        neg = s[i++] == '-';
    }
    r.v.neg = neg;

    if (matchci(s, i, "inf")) {
        r.end = i + (matchci(s, i, "infinity") ? 8 : 3);
        r.v.e = F80_MAXE;
        r.v.m = 1ull << 63;
        return r;
    } else if (matchci(s, i, "nan")) {
        r.end = i + 3;
        if (f80byte(s, r.end) == '(') {
            iz k = r.end + 1;
            for (; alnum(f80byte(s, k)) || f80byte(s, k)=='_'; k++) {}
            r.end = f80byte(s, k)==')' ? k+1 : r.end;
        }
        r.v.e = F80_MAXE;
        r.v.m = 3ull << 62;
        return r;
    }

    if (f80byte(s, i)=='0' && lowercase(f80byte(s, i+1))=='x') {
        F80Parse h = f80hex(s, i+2, neg);
        if (h.end) {
            return h;
        }
        r.end = i + 1;  // just the 0
        return r;
    }

    // Significant digits as a decimal string, D * 10^q
    u8 *d      = allocbytes(&scratch, F80_MAXDIGITS+1);
    iz  nd     = 0;
    i64 q      = 0;
    b32 any    = 0;
    b32 point  = 0;
    b32 sticky = 0;
    for (;; i++) {
        u8 c = f80byte(s, i);
        if (c=='.' && !point) {
            point = 1;
            continue;
        } else if (!digit(c)) {
            break;
        }
        any = 1;
        if (!nd && c=='0') {
            q -= point;
        } else if (nd < F80_MAXDIGITS) {
            d[nd++] = (u8)(c - '0');
            q -= point;
        } else {
            sticky |= c != '0';
            q += !point;
        }
    }
    if (!any) {
        r.v.neg = 0;
        return r;
    }
    r.end = i;
    if (f80byte(s, i)=='e' || f80byte(s, i)=='E') {
        q += f80exponent(s, &r.end);
    }
    if (!nd) {
        return r;  // zero
    }
    if (sticky) {
        d[nd++] = 1;
        q--;
    }

    // Out of range either way: 10^4933 > LDBL_MAX, 10^-4952 < half of
    // the smallest subnormal
    if (nd-1+q > 4933) {
        F80Round fr = f80round(neg, 1ull<<63, 0, 0, F80_MAXE);
        r.v     = fr.v;
        r.range = 1;
        return r;
    } else if (nd+q < -4952) {
        r.range = 1;
        return r;
    }

    // value = num/den exactly
    Big num = bignew(&scratch, (iz)(nd/9 + (q>0 ? q/9 : 0) + 4), 0);
    for (iz k = 0; k < nd;) {
        u32 chunk = 0, mul = 1;
        for (iz n = 0; n<9 && k<nd; n++, k++) {
            chunk = chunk*10 + d[k];
            mul  *= 10;
        }
        bigmuladd(&num, &scratch, mul, chunk);
    }
    Big den = bignew(&scratch, (iz)((q<0 ? -q/9 : 0) + 4), 1);
    if (q > 0) {
        bigmulpow10(&num, &scratch, q);
    } else {
        bigmulpow10(&den, &scratch, -q);
    }

    // Scale so that den <= num < 2*den, then divide out 64 bits
    i64 k = bigbits(num) - bigbits(den);
    i64 x = k;
    if (k > 0) {
        bigshl(&den, &scratch, k);
    } else {
        bigshl(&num, &scratch, -k);
    }
    if (bigcmp(num, den) < 0) {
        bigshl(&num, &scratch, 1);
        x--;
    }
    bigreserve(&num, &scratch, den.len+2);
    u64 m = 0;
    for (i32 bit = 0; bit < 64; bit++) {
        m <<= 1;
        if (bigcmp(num, den) >= 0) {
            bigsub(&num, den);
            m |= 1;
        }
        bigshl(&num, &scratch, 1);
    }
    i32 c    = bigcmp(num, den);
    b32 half = c >= 0;
    b32 rest = c>0 || (c<0 && num.len);
    F80Round fr = f80round(neg, m, half, rest, x);
    r.v     = fr.v;
    r.range = fr.range;
    return r;
}


// Formatting (glibc's printf_fp and printf_fphex)


// A source of the exact decimal digits of a value, most significant
// first: the integer part's, then the fraction's, nine at a time by
// multiplying by 10^9 and taking what rises past the fraction's top.
struct F80Gen {
    u8 *ints;      // integer digits, least significant first
    iz  nint;      // ... still unread
    Big frac;      // fraction bits, aligned to fill flimbs limbs
    iz  flimbs;
    u8  chunk[9];
    i32 cpos;      // next digit in chunk, 9 when empty
};

static u8 f80next(F80Gen *g)
{
    if (g->nint) {
        return g->ints[--g->nint];
    }
    if (g->cpos == 9) {
        Big *f     = &g->frac;
        u64  carry = 0;
        for (iz j = 0; j < f->len; j++) {
            u64 t = (u64)f->w[j]*1000000000 + carry;
            f->w[j] = (u32)t;
            carry   = t >> 32;
        }
        if (f->len < g->flimbs) {
            if (carry) {
                f->w[f->len++] = (u32)carry;
            }
            carry = 0;
        }
        for (; f->len && !f->w[f->len-1]; f->len--) {}
        u32 c = (u32)carry;
        for (i32 k = 8; k >= 0; k--) {
            g->chunk[k] = (u8)('0' + c%10);
            c /= 10;
        }
        g->cpos = 0;
    }
    return g->chunk[g->cpos++];
}

static b32 f80more(F80Gen *g)  // any nonzero digit left?
{
    for (iz k = 0; k < g->nint; k++) {
        if (g->ints[k] != '0') return 1;
    }
    for (i32 k = g->cpos; k < 9; k++) {
        if (g->chunk[k] != '0') return 1;
    }
    return g->frac.len > 0;
}

// Exact decimal digits: 0.d... * 10^exp.
struct F80Dec {
    u8 *d;
    iz  len;
    i64 exp;
    b32 carry;  // rounding added a digit in front
};

// The digits of finite v rounded half to even: n significant digits,
// or with fix, through the digit worth 10^-n. The count is exact,
// zeros included, and one more when fix carries into a new digit.
static F80Dec f80digits(F80 v, b32 fix, i64 n, Arena *a)
{
    F80Dec r = {};
    if (!v.m) {
        r.exp = 1;
        r.len = (iz)(fix ? n+1 : n);
        r.d   = allocbytes(a, r.len);
        for (iz k = 0; k < r.len; k++) {
            r.d[k] = '0';
        }
        return r;
    }

    F80Gen g = {};
    g.cpos   = 9;
    i64 x    = (v.e ? v.e : 1) - F80_BIAS - 63;  // v = m * 2^x

    Big ip = {};
    if (x >= 0) {
        ip = bignew(a, (iz)(x/32) + 4, v.m);
        bigshl(&ip, a, x);
    } else if (x > -64) {
        ip = bignew(a, 4, v.m >> -x);
    }
    g.ints = allocbytes(a, (iz)(bigbits(ip)*3/10) + 20);
    while (ip.len) {
        u32 c = bigdivsmall(&ip, 1000000000);
        for (i32 k = 0; k < 9; k++) {
            g.ints[g.nint++] = (u8)('0' + c%10);
            c /= 10;
        }
    }
    for (; g.nint && g.ints[g.nint-1]=='0'; g.nint--) {}

    i64 fbits = x<0 ? -x : 0;
    i64 align = (32 - fbits%32) % 32;
    g.flimbs  = (iz)((fbits + align) / 32);
    if (fbits) {
        u64 f = fbits<64 ? v.m & ((1ull<<fbits) - 1) : v.m;
        g.frac = bignew(a, g.flimbs+2, f);
        bigshl(&g.frac, a, align);
    }

    r.exp = g.nint;
    u8 first = f80next(&g);
    for (; first == '0'; r.exp--) {
        first = f80next(&g);
    }

    i64 keep = fix ? r.exp+n : n;
    if (keep < 0) {
        return r;  // below half a unit in the last place: zero
    }
    r.d   = allocbytes(a, (iz)keep+1);
    r.len = (iz)keep;
    u8 next = first;
    for (iz k = 0; k < r.len; k++) {
        r.d[k] = next;
        next   = f80next(&g);
    }
    b32 odd = r.len && (r.d[r.len-1]&1);
    if (next>'5' || (next=='5' && (odd || f80more(&g)))) {
        iz k = r.len - 1;
        for (; k>=0 && r.d[k]=='9'; k--) {
            r.d[k] = '0';
        }
        if (k >= 0) {
            r.d[k]++;
        } else {
            // 99...9 became 100...0, one more digit before the point
            r.d[0]  = '1';
            r.exp++;
            r.carry = 1;
            if (fix || !r.len) {
                r.d[r.len++] = '0';
                r.d[0]       = '1';
            }
        }
    }
    return r;
}

static void f80pad(Buf *b, i64 n, u8 c)
{
    for (; n > 0; n--) {
        putbyte(b, c);
    }
}

// Pad sign, prefix and body to the width: zeros go between the prefix
// and the body, spaces outside. The body may sit in scratch just past
// b's capacity, where b grows in place when f80bound fell short (a
// width or precision near 2^30), so everything is reserved first and
// the body moved to its place before the padding is written.
static void f80emit(Buf *b, F80Spec *fs, Str sign, Str prefix, Str body, b32 zeros)
{
    b32 z     = zeros && fs->zero && !fs->left;
    iz  head  = sign.len + prefix.len;
    i64 pad   = fs->width - head - body.len;
    pad = pad>0 ? pad : 0;
    i64 total = head + body.len + pad;
    if (total > (i64)((uz)-1 >> 1)) os_oom();
    reserve(b, (iz)total);
    u8 *out = b->data + b->len;
    iz  at  = fs->left ? head : (iz)total - body.len;
    if (body.len) __builtin_memmove(out+at, body.data, (uz)body.len);
    iz i = 0;
    if (!fs->left && !z) for (; i < pad; i++) out[i] = ' ';
    copybytes(out+i, sign.data, sign.len);
    i += sign.len;
    copybytes(out+i, prefix.data, prefix.len);
    i += prefix.len;
    if (z) for (iz n = 0; n < pad; n++) out[i++] = '0';
    if (fs->left) for (i = at+body.len; i < total; i++) out[i] = ' ';
    b->len += (iz)total;
}

// Digit k of d, zero outside it.
static u8 f80dig(F80Dec *d, i64 k)
{
    return k>=0 && k<d->len ? d->d[k] : '0';
}

// %f: digits through 10^-prec.
static void f80fixed(Buf *t, F80Dec *d, i64 prec, b32 alt)
{
    if (d->exp > 0) {
        for (i64 k = 0; k < d->exp; k++) {
            putbyte(t, f80dig(d, k));
        }
    } else {
        putbyte(t, '0');
    }
    if (prec || alt) {
        putbyte(t, '.');
    }
    for (i64 j = 0; j < prec; j++) {
        putbyte(t, f80dig(d, d->exp + j));
    }
}

// %e: one digit, the point, prec digits, an exponent.
static void f80sci(Buf *t, F80Dec *d, i64 prec, b32 alt, u8 e)
{
    putbyte(t, f80dig(d, 0));
    if (prec || alt) {
        putbyte(t, '.');
    }
    for (i64 j = 1; j <= prec; j++) {
        putbyte(t, f80dig(d, j));
    }
    i64 x = d->exp - 1;
    putbyte(t, e);
    putbyte(t, x<0 ? '-' : '+');
    x = x<0 ? -x : x;
    if (x < 10) {
        putbyte(t, '0');
    }
    print(t, x);
}

// %a: glibc's layout for the x87 format, where the leading hex digit
// is the significand's top nibble and the exponent counts from there.
static void f80hexfmt(Buf *t, F80 v, F80Spec *fs, b32 upper)
{
    Str hex = upper ? Str("0123456789ABCDEF") : Str("0123456789abcdef");
    u8  digits[15];
    for (i32 k = 0; k < 15; k++) {
        digits[k] = hex[(iz)(v.m >> (56 - 4*k) & 15)];
    }
    u8  lead = hex[(iz)(v.m >> 60)];
    i64 exp  = !v.m ? 0 : v.e ? v.e-F80_BIAS-3 : 1-F80_BIAS-3;
    i64 n    = 15;
    for (; n && digits[n-1]=='0'; n--) {}

    i64 prec = fs->prec;
    if (prec<0 || !v.m) {
        prec = prec<0 ? (v.m ? n : 0) : prec;
        n    = v.m ? n : 0;
    } else if (prec < n) {
        u8  last  = prec ? digits[prec-1] : lead;
        i32 lastv = f80hexval(last);
        i32 nextv = f80hexval(digits[prec]);
        b32 more  = (nextv&7) || prec+1<n;
        if (nextv>=8 && ((lastv&1) || more)) {
            i64 k = prec - 1;
            for (; k >= 0; k--) {
                u8 c = digits[k];
                if (c == '9') {
                    digits[k] = upper ? 'A' : 'a';
                    break;
                } else if (lowercase(c) < 'f') {
                    digits[k]++;
                    break;
                }
                digits[k] = '0';
            }
            if (k < 0) {
                if (lead == '9') {
                    lead = upper ? 'A' : 'a';
                } else if (lowercase(lead) < 'f') {
                    lead++;
                } else {
                    lead = '1';
                    exp += 4;
                }
            }
        }
        n = prec;
    }

    putbyte(t, lead);
    if (prec || fs->alt) {
        putbyte(t, '.');
    }
    print(t, {digits, (iz)n});
    f80pad(t, prec-n, '0');
    putbyte(t, upper ? 'P' : 'p');
    putbyte(t, exp<0 ? '-' : '+');
    print(t, exp<0 ? -exp : exp);
}

// An upper bound on the length of a conversion's output.
static iz f80bound(F80Spec *fs)
{
    i64 prec = fs->prec<0 ? 6 : fs->prec;
    i64 n    = fs->width + prec + 4980;
    return n<(1ll<<30) ? (iz)n : (iz)1<<30;
}

// Format v like glibc's printf with %L and conv (aAeEfFgG). Working
// memory comes from scratch, which must not overlap b's free space.
static void f80format(Buf *b, F80 v, u8 conv, F80Spec *fs, Arena scratch)
{
    b32 upper = conv>='A' && conv<='Z';
    u8  e     = upper ? 'E' : 'e';
    Str sign  = v.neg ? Str("-") : fs->plus ? Str("+") : fs->space ? Str(" ") : Str("");

    if (v.e == F80_MAXE) {
        Str body = v.m<<1 ? (upper ? Str("NAN") : Str("nan"))
                          : (upper ? Str("INF") : Str("inf"));
        f80emit(b, fs, sign, "", body, 0);
        return;
    }

    Buf t(&scratch, 64);
    switch (lowercase(conv)) {
    case 'a':
        f80hexfmt(&t, v, fs, upper);
        f80emit(b, fs, sign, upper ? Str("0X") : Str("0x"), finish(&t), 1);
        return;

    case 'f': {
        i64    prec = fs->prec<0 ? 6 : fs->prec;
        F80Dec d    = f80digits(v, 1, prec, &scratch);
        f80fixed(&t, &d, prec, fs->alt);
    } break;

    case 'e': {
        i64    prec = fs->prec<0 ? 6 : fs->prec;
        F80Dec d    = f80digits(v, 0, prec+1, &scratch);
        f80sci(&t, &d, prec, fs->alt, e);
    } break;

    case 'g': {
        i64    p = fs->prec<0 ? 6 : fs->prec ? fs->prec : 1;
        F80Dec d = f80digits(v, 0, p, &scratch);
        i64    x = d.exp - 1;
        if (x>=-4 && x<p) {
            f80fixed(&t, &d, p-1-x, fs->alt);
        } else if (d.carry && x==p) {
            // glibc planned %f without a fraction, then rounding made
            // the integer part too long: %e, still without a fraction
            f80sci(&t, &d, 0, fs->alt, e);
        } else {
            f80sci(&t, &d, p-1, fs->alt, e);
        }
        if (!fs->alt) {
            // Drop the fraction's trailing zeros, then a bare point
            Str all = finish(&t);
            iz  dot = 0;
            for (; dot<all.len && all[dot]!='.'; dot++) {}
            iz  end = dot;
            for (; end<all.len && all[end]!=e; end++) {}
            iz  cut = end;
            if (dot < end) {
                for (; all[cut-1] == '0'; cut--) {}
                cut -= all[cut-1] == '.';
            }
            Buf u(&scratch, all.len);
            print(&u, takehead(all, cut));
            print(&u, cuthead(all, end));
            f80emit(b, fs, sign, "", finish(&u), 1);
            return;
        }
    } break;
    }
    f80emit(b, fs, sign, "", finish(&t), 1);
}
