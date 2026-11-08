// printf builtin: formatting, escapes and shell quoting
//
// printfmt runs one printf invocation: it walks the format, converting
// arguments, and reuses the format while arguments remain. Conversions
// that bash hands to the C library (%c %s %d %i %o %u %x %X) follow
// glibc's formatting, quirks included; %b %q %Q, %(...)T and the wide
// %lc %ls follow bash's own padding. Floating point conversions read
// and print x87 long doubles in software (float.cpp), as glibc does.
//
// This is free and unencumbered software released into the public domain.


// Interface

struct PfZone;

struct PrintfEnv {
    void *ctx;
    b32   utf8;                             // UTF-8 locale, else C
    u8    radix;                            // decimal point, 0 for '.'
    i64   (*now)(void *ctx);                // seconds since the epoch
    i64   shellstart;                       // for %(...)T with -2
    Str   tz;                               // TZ if exported, else null
    PfZone *zone;                           // local time's zone, else from tz
    void  (*error)(void *ctx, Str msg);     // "printf: ..." diagnostics
    void  (*setint)(void *ctx, Str, i64);   // %n assignment, may be null
    void  (*flush)(void *ctx, Str);         // line-buffered output, may be null
    iz    flushed;                          // output bytes already flushed
    Buf  *out;                              // (set by printfmt)
};

static i32 printfmt(PrintfEnv *, Buf *, Str fmt, Str *args, iz nargs);
static Str echoescape(Arena *, Str, b32 percentb, b32 utf8, b32 *stop);
static Str shellquote(Arena *, Str, b32 utf8);


// Characters

static b32 octdigit(u8 c)
{
    return c>='0' && c<='7';
}

static i32 hexdigit(u8 c)  // -1 if not a hex digit
{
    if (digit(c)) {
        return c - '0';
    }
    c = lowercase(c);
    return c>='a' && c<='f' ? c - 'a' + 10 : -1;
}

// Give back the unused tail of a buffer that ends at the arena's front.
static Str shrinkbuf(Buf *b)
{
    if ((byte *)(b->data+b->cap) == b->a->beg) {
        b->a->beg = (byte *)(b->data + b->len);
        b->cap    = b->len;
    }
    return finish(b);
}


// Unicode, as glibc's C.UTF-8 locale sees it

struct Mbchar {
    i32 c;
    i32 len;  // 0 if invalid or truncated
};

// Decode the character at s[i] as glibc's mbrtowc does: the original
// 31-bit UTF-8, without overlong forms or surrogates.
static Mbchar mbcharat(Str s, iz i)
{
    static i32 mins[] = {0, 0x80, 0x800, 0x10000, 0x200000, 0x4000000};

    Mbchar r = {};
    u8     b = s[i];
    if (b < 0x80) {
        r.c   = b;
        r.len = 1;
        return r;
    } else if (b<0xc0 || b>0xfd) {
        return r;
    }

    i32 n = 1;  // continuation bytes
    for (; b<<(n+1) & 0x80; n++) {}
    if (n > s.len-i-1) {
        return r;
    }
    r.c = b & (0x3f >> n);
    for (i32 k = 1; k <= n; k++) {
        u8 t = s[i+k];
        if ((t&0xc0) != 0x80) {
            return {};
        }
        r.c = r.c<<6 | (t&0x3f);
    }
    if (r.c<mins[n] || (r.c>=0xd800 && r.c<=0xdfff)) {
        return {};
    }
    r.len = n + 1;
    return r;
}

// Ranges of printable code points above ASCII, recorded from glibc 2.41
// (Unicode 16): assigned, not controls, not line or paragraph separators.
static u32 printable[][2] = {
    {0xa0, 0x377}, {0x37a, 0x37f}, {0x384, 0x38a}, {0x38c, 0x38c},
    {0x38e, 0x3a1}, {0x3a3, 0x52f}, {0x531, 0x556}, {0x559, 0x58a},
    {0x58d, 0x58f}, {0x591, 0x5c7}, {0x5d0, 0x5ea}, {0x5ef, 0x5f4},
    {0x600, 0x70d}, {0x70f, 0x74a}, {0x74d, 0x7b1}, {0x7c0, 0x7fa},
    {0x7fd, 0x82d}, {0x830, 0x83e}, {0x840, 0x85b}, {0x85e, 0x85e},
    {0x860, 0x86a}, {0x870, 0x88e}, {0x890, 0x891}, {0x897, 0x983},
    {0x985, 0x98c}, {0x98f, 0x990}, {0x993, 0x9a8}, {0x9aa, 0x9b0},
    {0x9b2, 0x9b2}, {0x9b6, 0x9b9}, {0x9bc, 0x9c4}, {0x9c7, 0x9c8},
    {0x9cb, 0x9ce}, {0x9d7, 0x9d7}, {0x9dc, 0x9dd}, {0x9df, 0x9e3},
    {0x9e6, 0x9fe}, {0xa01, 0xa03}, {0xa05, 0xa0a}, {0xa0f, 0xa10},
    {0xa13, 0xa28}, {0xa2a, 0xa30}, {0xa32, 0xa33}, {0xa35, 0xa36},
    {0xa38, 0xa39}, {0xa3c, 0xa3c}, {0xa3e, 0xa42}, {0xa47, 0xa48},
    {0xa4b, 0xa4d}, {0xa51, 0xa51}, {0xa59, 0xa5c}, {0xa5e, 0xa5e},
    {0xa66, 0xa76}, {0xa81, 0xa83}, {0xa85, 0xa8d}, {0xa8f, 0xa91},
    {0xa93, 0xaa8}, {0xaaa, 0xab0}, {0xab2, 0xab3}, {0xab5, 0xab9},
    {0xabc, 0xac5}, {0xac7, 0xac9}, {0xacb, 0xacd}, {0xad0, 0xad0},
    {0xae0, 0xae3}, {0xae6, 0xaf1}, {0xaf9, 0xaff}, {0xb01, 0xb03},
    {0xb05, 0xb0c}, {0xb0f, 0xb10}, {0xb13, 0xb28}, {0xb2a, 0xb30},
    {0xb32, 0xb33}, {0xb35, 0xb39}, {0xb3c, 0xb44}, {0xb47, 0xb48},
    {0xb4b, 0xb4d}, {0xb55, 0xb57}, {0xb5c, 0xb5d}, {0xb5f, 0xb63},
    {0xb66, 0xb77}, {0xb82, 0xb83}, {0xb85, 0xb8a}, {0xb8e, 0xb90},
    {0xb92, 0xb95}, {0xb99, 0xb9a}, {0xb9c, 0xb9c}, {0xb9e, 0xb9f},
    {0xba3, 0xba4}, {0xba8, 0xbaa}, {0xbae, 0xbb9}, {0xbbe, 0xbc2},
    {0xbc6, 0xbc8}, {0xbca, 0xbcd}, {0xbd0, 0xbd0}, {0xbd7, 0xbd7},
    {0xbe6, 0xbfa}, {0xc00, 0xc0c}, {0xc0e, 0xc10}, {0xc12, 0xc28},
    {0xc2a, 0xc39}, {0xc3c, 0xc44}, {0xc46, 0xc48}, {0xc4a, 0xc4d},
    {0xc55, 0xc56}, {0xc58, 0xc5a}, {0xc5d, 0xc5d}, {0xc60, 0xc63},
    {0xc66, 0xc6f}, {0xc77, 0xc8c}, {0xc8e, 0xc90}, {0xc92, 0xca8},
    {0xcaa, 0xcb3}, {0xcb5, 0xcb9}, {0xcbc, 0xcc4}, {0xcc6, 0xcc8},
    {0xcca, 0xccd}, {0xcd5, 0xcd6}, {0xcdd, 0xcde}, {0xce0, 0xce3},
    {0xce6, 0xcef}, {0xcf1, 0xcf3}, {0xd00, 0xd0c}, {0xd0e, 0xd10},
    {0xd12, 0xd44}, {0xd46, 0xd48}, {0xd4a, 0xd4f}, {0xd54, 0xd63},
    {0xd66, 0xd7f}, {0xd81, 0xd83}, {0xd85, 0xd96}, {0xd9a, 0xdb1},
    {0xdb3, 0xdbb}, {0xdbd, 0xdbd}, {0xdc0, 0xdc6}, {0xdca, 0xdca},
    {0xdcf, 0xdd4}, {0xdd6, 0xdd6}, {0xdd8, 0xddf}, {0xde6, 0xdef},
    {0xdf2, 0xdf4}, {0xe01, 0xe3a}, {0xe3f, 0xe5b}, {0xe81, 0xe82},
    {0xe84, 0xe84}, {0xe86, 0xe8a}, {0xe8c, 0xea3}, {0xea5, 0xea5},
    {0xea7, 0xebd}, {0xec0, 0xec4}, {0xec6, 0xec6}, {0xec8, 0xece},
    {0xed0, 0xed9}, {0xedc, 0xedf}, {0xf00, 0xf47}, {0xf49, 0xf6c},
    {0xf71, 0xf97}, {0xf99, 0xfbc}, {0xfbe, 0xfcc}, {0xfce, 0xfda},
    {0x1000, 0x10c5}, {0x10c7, 0x10c7}, {0x10cd, 0x10cd}, {0x10d0, 0x1248},
    {0x124a, 0x124d}, {0x1250, 0x1256}, {0x1258, 0x1258}, {0x125a, 0x125d},
    {0x1260, 0x1288}, {0x128a, 0x128d}, {0x1290, 0x12b0}, {0x12b2, 0x12b5},
    {0x12b8, 0x12be}, {0x12c0, 0x12c0}, {0x12c2, 0x12c5}, {0x12c8, 0x12d6},
    {0x12d8, 0x1310}, {0x1312, 0x1315}, {0x1318, 0x135a}, {0x135d, 0x137c},
    {0x1380, 0x1399}, {0x13a0, 0x13f5}, {0x13f8, 0x13fd}, {0x1400, 0x169c},
    {0x16a0, 0x16f8}, {0x1700, 0x1715}, {0x171f, 0x1736}, {0x1740, 0x1753},
    {0x1760, 0x176c}, {0x176e, 0x1770}, {0x1772, 0x1773}, {0x1780, 0x17dd},
    {0x17e0, 0x17e9}, {0x17f0, 0x17f9}, {0x1800, 0x1819}, {0x1820, 0x1878},
    {0x1880, 0x18aa}, {0x18b0, 0x18f5}, {0x1900, 0x191e}, {0x1920, 0x192b},
    {0x1930, 0x193b}, {0x1940, 0x1940}, {0x1944, 0x196d}, {0x1970, 0x1974},
    {0x1980, 0x19ab}, {0x19b0, 0x19c9}, {0x19d0, 0x19da}, {0x19de, 0x1a1b},
    {0x1a1e, 0x1a5e}, {0x1a60, 0x1a7c}, {0x1a7f, 0x1a89}, {0x1a90, 0x1a99},
    {0x1aa0, 0x1aad}, {0x1ab0, 0x1ace}, {0x1b00, 0x1b4c}, {0x1b4e, 0x1bf3},
    {0x1bfc, 0x1c37}, {0x1c3b, 0x1c49}, {0x1c4d, 0x1c8a}, {0x1c90, 0x1cba},
    {0x1cbd, 0x1cc7}, {0x1cd0, 0x1cfa}, {0x1d00, 0x1f15}, {0x1f18, 0x1f1d},
    {0x1f20, 0x1f45}, {0x1f48, 0x1f4d}, {0x1f50, 0x1f57}, {0x1f59, 0x1f59},
    {0x1f5b, 0x1f5b}, {0x1f5d, 0x1f5d}, {0x1f5f, 0x1f7d}, {0x1f80, 0x1fb4},
    {0x1fb6, 0x1fc4}, {0x1fc6, 0x1fd3}, {0x1fd6, 0x1fdb}, {0x1fdd, 0x1fef},
    {0x1ff2, 0x1ff4}, {0x1ff6, 0x1ffe}, {0x2000, 0x2027}, {0x202a, 0x2064},
    {0x2066, 0x2071}, {0x2074, 0x208e}, {0x2090, 0x209c}, {0x20a0, 0x20c0},
    {0x20d0, 0x20f0}, {0x2100, 0x218b}, {0x2190, 0x2429}, {0x2440, 0x244a},
    {0x2460, 0x2b73}, {0x2b76, 0x2b95}, {0x2b97, 0x2cf3}, {0x2cf9, 0x2d25},
    {0x2d27, 0x2d27}, {0x2d2d, 0x2d2d}, {0x2d30, 0x2d67}, {0x2d6f, 0x2d70},
    {0x2d7f, 0x2d96}, {0x2da0, 0x2da6}, {0x2da8, 0x2dae}, {0x2db0, 0x2db6},
    {0x2db8, 0x2dbe}, {0x2dc0, 0x2dc6}, {0x2dc8, 0x2dce}, {0x2dd0, 0x2dd6},
    {0x2dd8, 0x2dde}, {0x2de0, 0x2e5d}, {0x2e80, 0x2e99}, {0x2e9b, 0x2ef3},
    {0x2f00, 0x2fd5}, {0x2ff0, 0x303f}, {0x3041, 0x3096}, {0x3099, 0x30ff},
    {0x3105, 0x312f}, {0x3131, 0x318e}, {0x3190, 0x31e5}, {0x31ef, 0x321e},
    {0x3220, 0xa48c}, {0xa490, 0xa4c6}, {0xa4d0, 0xa62b}, {0xa640, 0xa6f7},
    {0xa700, 0xa7cd}, {0xa7d0, 0xa7d1}, {0xa7d3, 0xa7d3}, {0xa7d5, 0xa7dc},
    {0xa7f2, 0xa82c}, {0xa830, 0xa839}, {0xa840, 0xa877}, {0xa880, 0xa8c5},
    {0xa8ce, 0xa8d9}, {0xa8e0, 0xa953}, {0xa95f, 0xa97c}, {0xa980, 0xa9cd},
    {0xa9cf, 0xa9d9}, {0xa9de, 0xa9fe}, {0xaa00, 0xaa36}, {0xaa40, 0xaa4d},
    {0xaa50, 0xaa59}, {0xaa5c, 0xaac2}, {0xaadb, 0xaaf6}, {0xab01, 0xab06},
    {0xab09, 0xab0e}, {0xab11, 0xab16}, {0xab20, 0xab26}, {0xab28, 0xab2e},
    {0xab30, 0xab6b}, {0xab70, 0xabed}, {0xabf0, 0xabf9}, {0xac00, 0xd7a3},
    {0xd7b0, 0xd7c6}, {0xd7cb, 0xd7fb}, {0xe000, 0xfa6d}, {0xfa70, 0xfad9},
    {0xfb00, 0xfb06}, {0xfb13, 0xfb17}, {0xfb1d, 0xfb36}, {0xfb38, 0xfb3c},
    {0xfb3e, 0xfb3e}, {0xfb40, 0xfb41}, {0xfb43, 0xfb44}, {0xfb46, 0xfbc2},
    {0xfbd3, 0xfd8f}, {0xfd92, 0xfdc7}, {0xfdcf, 0xfdcf}, {0xfdf0, 0xfe19},
    {0xfe20, 0xfe52}, {0xfe54, 0xfe66}, {0xfe68, 0xfe6b}, {0xfe70, 0xfe74},
    {0xfe76, 0xfefc}, {0xfeff, 0xfeff}, {0xff01, 0xffbe}, {0xffc2, 0xffc7},
    {0xffca, 0xffcf}, {0xffd2, 0xffd7}, {0xffda, 0xffdc}, {0xffe0, 0xffe6},
    {0xffe8, 0xffee}, {0xfff9, 0xfffd}, {0x10000, 0x1000b}, {0x1000d, 0x10026},
    {0x10028, 0x1003a}, {0x1003c, 0x1003d}, {0x1003f, 0x1004d}, {0x10050, 0x1005d},
    {0x10080, 0x100fa}, {0x10100, 0x10102}, {0x10107, 0x10133}, {0x10137, 0x1018e},
    {0x10190, 0x1019c}, {0x101a0, 0x101a0}, {0x101d0, 0x101fd}, {0x10280, 0x1029c},
    {0x102a0, 0x102d0}, {0x102e0, 0x102fb}, {0x10300, 0x10323}, {0x1032d, 0x1034a},
    {0x10350, 0x1037a}, {0x10380, 0x1039d}, {0x1039f, 0x103c3}, {0x103c8, 0x103d5},
    {0x10400, 0x1049d}, {0x104a0, 0x104a9}, {0x104b0, 0x104d3}, {0x104d8, 0x104fb},
    {0x10500, 0x10527}, {0x10530, 0x10563}, {0x1056f, 0x1057a}, {0x1057c, 0x1058a},
    {0x1058c, 0x10592}, {0x10594, 0x10595}, {0x10597, 0x105a1}, {0x105a3, 0x105b1},
    {0x105b3, 0x105b9}, {0x105bb, 0x105bc}, {0x105c0, 0x105f3}, {0x10600, 0x10736},
    {0x10740, 0x10755}, {0x10760, 0x10767}, {0x10780, 0x10785}, {0x10787, 0x107b0},
    {0x107b2, 0x107ba}, {0x10800, 0x10805}, {0x10808, 0x10808}, {0x1080a, 0x10835},
    {0x10837, 0x10838}, {0x1083c, 0x1083c}, {0x1083f, 0x10855}, {0x10857, 0x1089e},
    {0x108a7, 0x108af}, {0x108e0, 0x108f2}, {0x108f4, 0x108f5}, {0x108fb, 0x1091b},
    {0x1091f, 0x10939}, {0x1093f, 0x1093f}, {0x10980, 0x109b7}, {0x109bc, 0x109cf},
    {0x109d2, 0x10a03}, {0x10a05, 0x10a06}, {0x10a0c, 0x10a13}, {0x10a15, 0x10a17},
    {0x10a19, 0x10a35}, {0x10a38, 0x10a3a}, {0x10a3f, 0x10a48}, {0x10a50, 0x10a58},
    {0x10a60, 0x10a9f}, {0x10ac0, 0x10ae6}, {0x10aeb, 0x10af6}, {0x10b00, 0x10b35},
    {0x10b39, 0x10b55}, {0x10b58, 0x10b72}, {0x10b78, 0x10b91}, {0x10b99, 0x10b9c},
    {0x10ba9, 0x10baf}, {0x10c00, 0x10c48}, {0x10c80, 0x10cb2}, {0x10cc0, 0x10cf2},
    {0x10cfa, 0x10d27}, {0x10d30, 0x10d39}, {0x10d40, 0x10d65}, {0x10d69, 0x10d85},
    {0x10d8e, 0x10d8f}, {0x10e60, 0x10e7e}, {0x10e80, 0x10ea9}, {0x10eab, 0x10ead},
    {0x10eb0, 0x10eb1}, {0x10ec2, 0x10ec4}, {0x10efc, 0x10f27}, {0x10f30, 0x10f59},
    {0x10f70, 0x10f89}, {0x10fb0, 0x10fcb}, {0x10fe0, 0x10ff6}, {0x11000, 0x1104d},
    {0x11052, 0x11075}, {0x1107f, 0x110c2}, {0x110cd, 0x110cd}, {0x110d0, 0x110e8},
    {0x110f0, 0x110f9}, {0x11100, 0x11134}, {0x11136, 0x11147}, {0x11150, 0x11176},
    {0x11180, 0x111df}, {0x111e1, 0x111f4}, {0x11200, 0x11211}, {0x11213, 0x11241},
    {0x11280, 0x11286}, {0x11288, 0x11288}, {0x1128a, 0x1128d}, {0x1128f, 0x1129d},
    {0x1129f, 0x112a9}, {0x112b0, 0x112ea}, {0x112f0, 0x112f9}, {0x11300, 0x11303},
    {0x11305, 0x1130c}, {0x1130f, 0x11310}, {0x11313, 0x11328}, {0x1132a, 0x11330},
    {0x11332, 0x11333}, {0x11335, 0x11339}, {0x1133b, 0x11344}, {0x11347, 0x11348},
    {0x1134b, 0x1134d}, {0x11350, 0x11350}, {0x11357, 0x11357}, {0x1135d, 0x11363},
    {0x11366, 0x1136c}, {0x11370, 0x11374}, {0x11380, 0x11389}, {0x1138b, 0x1138b},
    {0x1138e, 0x1138e}, {0x11390, 0x113b5}, {0x113b7, 0x113c0}, {0x113c2, 0x113c2},
    {0x113c5, 0x113c5}, {0x113c7, 0x113ca}, {0x113cc, 0x113d5}, {0x113d7, 0x113d8},
    {0x113e1, 0x113e2}, {0x11400, 0x1145b}, {0x1145d, 0x11461}, {0x11480, 0x114c7},
    {0x114d0, 0x114d9}, {0x11580, 0x115b5}, {0x115b8, 0x115dd}, {0x11600, 0x11644},
    {0x11650, 0x11659}, {0x11660, 0x1166c}, {0x11680, 0x116b9}, {0x116c0, 0x116c9},
    {0x116d0, 0x116e3}, {0x11700, 0x1171a}, {0x1171d, 0x1172b}, {0x11730, 0x11746},
    {0x11800, 0x1183b}, {0x118a0, 0x118f2}, {0x118ff, 0x11906}, {0x11909, 0x11909},
    {0x1190c, 0x11913}, {0x11915, 0x11916}, {0x11918, 0x11935}, {0x11937, 0x11938},
    {0x1193b, 0x11946}, {0x11950, 0x11959}, {0x119a0, 0x119a7}, {0x119aa, 0x119d7},
    {0x119da, 0x119e4}, {0x11a00, 0x11a47}, {0x11a50, 0x11aa2}, {0x11ab0, 0x11af8},
    {0x11b00, 0x11b09}, {0x11bc0, 0x11be1}, {0x11bf0, 0x11bf9}, {0x11c00, 0x11c08},
    {0x11c0a, 0x11c36}, {0x11c38, 0x11c45}, {0x11c50, 0x11c6c}, {0x11c70, 0x11c8f},
    {0x11c92, 0x11ca7}, {0x11ca9, 0x11cb6}, {0x11d00, 0x11d06}, {0x11d08, 0x11d09},
    {0x11d0b, 0x11d36}, {0x11d3a, 0x11d3a}, {0x11d3c, 0x11d3d}, {0x11d3f, 0x11d47},
    {0x11d50, 0x11d59}, {0x11d60, 0x11d65}, {0x11d67, 0x11d68}, {0x11d6a, 0x11d8e},
    {0x11d90, 0x11d91}, {0x11d93, 0x11d98}, {0x11da0, 0x11da9}, {0x11ee0, 0x11ef8},
    {0x11f00, 0x11f10}, {0x11f12, 0x11f3a}, {0x11f3e, 0x11f5a}, {0x11fb0, 0x11fb0},
    {0x11fc0, 0x11ff1}, {0x11fff, 0x12399}, {0x12400, 0x1246e}, {0x12470, 0x12474},
    {0x12480, 0x12543}, {0x12f90, 0x12ff2}, {0x13000, 0x13455}, {0x13460, 0x143fa},
    {0x14400, 0x14646}, {0x16100, 0x16139}, {0x16800, 0x16a38}, {0x16a40, 0x16a5e},
    {0x16a60, 0x16a69}, {0x16a6e, 0x16abe}, {0x16ac0, 0x16ac9}, {0x16ad0, 0x16aed},
    {0x16af0, 0x16af5}, {0x16b00, 0x16b45}, {0x16b50, 0x16b59}, {0x16b5b, 0x16b61},
    {0x16b63, 0x16b77}, {0x16b7d, 0x16b8f}, {0x16d40, 0x16d79}, {0x16e40, 0x16e9a},
    {0x16f00, 0x16f4a}, {0x16f4f, 0x16f87}, {0x16f8f, 0x16f9f}, {0x16fe0, 0x16fe4},
    {0x16ff0, 0x16ff1}, {0x17000, 0x187f7}, {0x18800, 0x18cd5}, {0x18cff, 0x18d08},
    {0x1aff0, 0x1aff3}, {0x1aff5, 0x1affb}, {0x1affd, 0x1affe}, {0x1b000, 0x1b122},
    {0x1b132, 0x1b132}, {0x1b150, 0x1b152}, {0x1b155, 0x1b155}, {0x1b164, 0x1b167},
    {0x1b170, 0x1b2fb}, {0x1bc00, 0x1bc6a}, {0x1bc70, 0x1bc7c}, {0x1bc80, 0x1bc88},
    {0x1bc90, 0x1bc99}, {0x1bc9c, 0x1bca3}, {0x1cc00, 0x1ccf9}, {0x1cd00, 0x1ceb3},
    {0x1cf00, 0x1cf2d}, {0x1cf30, 0x1cf46}, {0x1cf50, 0x1cfc3}, {0x1d000, 0x1d0f5},
    {0x1d100, 0x1d126}, {0x1d129, 0x1d1ea}, {0x1d200, 0x1d245}, {0x1d2c0, 0x1d2d3},
    {0x1d2e0, 0x1d2f3}, {0x1d300, 0x1d356}, {0x1d360, 0x1d378}, {0x1d400, 0x1d454},
    {0x1d456, 0x1d49c}, {0x1d49e, 0x1d49f}, {0x1d4a2, 0x1d4a2}, {0x1d4a5, 0x1d4a6},
    {0x1d4a9, 0x1d4ac}, {0x1d4ae, 0x1d4b9}, {0x1d4bb, 0x1d4bb}, {0x1d4bd, 0x1d4c3},
    {0x1d4c5, 0x1d505}, {0x1d507, 0x1d50a}, {0x1d50d, 0x1d514}, {0x1d516, 0x1d51c},
    {0x1d51e, 0x1d539}, {0x1d53b, 0x1d53e}, {0x1d540, 0x1d544}, {0x1d546, 0x1d546},
    {0x1d54a, 0x1d550}, {0x1d552, 0x1d6a5}, {0x1d6a8, 0x1d7cb}, {0x1d7ce, 0x1da8b},
    {0x1da9b, 0x1da9f}, {0x1daa1, 0x1daaf}, {0x1df00, 0x1df1e}, {0x1df25, 0x1df2a},
    {0x1e000, 0x1e006}, {0x1e008, 0x1e018}, {0x1e01b, 0x1e021}, {0x1e023, 0x1e024},
    {0x1e026, 0x1e02a}, {0x1e030, 0x1e06d}, {0x1e08f, 0x1e08f}, {0x1e100, 0x1e12c},
    {0x1e130, 0x1e13d}, {0x1e140, 0x1e149}, {0x1e14e, 0x1e14f}, {0x1e290, 0x1e2ae},
    {0x1e2c0, 0x1e2f9}, {0x1e2ff, 0x1e2ff}, {0x1e4d0, 0x1e4f9}, {0x1e5d0, 0x1e5fa},
    {0x1e5ff, 0x1e5ff}, {0x1e7e0, 0x1e7e6}, {0x1e7e8, 0x1e7eb}, {0x1e7ed, 0x1e7ee},
    {0x1e7f0, 0x1e7fe}, {0x1e800, 0x1e8c4}, {0x1e8c7, 0x1e8d6}, {0x1e900, 0x1e94b},
    {0x1e950, 0x1e959}, {0x1e95e, 0x1e95f}, {0x1ec71, 0x1ecb4}, {0x1ed01, 0x1ed3d},
    {0x1ee00, 0x1ee03}, {0x1ee05, 0x1ee1f}, {0x1ee21, 0x1ee22}, {0x1ee24, 0x1ee24},
    {0x1ee27, 0x1ee27}, {0x1ee29, 0x1ee32}, {0x1ee34, 0x1ee37}, {0x1ee39, 0x1ee39},
    {0x1ee3b, 0x1ee3b}, {0x1ee42, 0x1ee42}, {0x1ee47, 0x1ee47}, {0x1ee49, 0x1ee49},
    {0x1ee4b, 0x1ee4b}, {0x1ee4d, 0x1ee4f}, {0x1ee51, 0x1ee52}, {0x1ee54, 0x1ee54},
    {0x1ee57, 0x1ee57}, {0x1ee59, 0x1ee59}, {0x1ee5b, 0x1ee5b}, {0x1ee5d, 0x1ee5d},
    {0x1ee5f, 0x1ee5f}, {0x1ee61, 0x1ee62}, {0x1ee64, 0x1ee64}, {0x1ee67, 0x1ee6a},
    {0x1ee6c, 0x1ee72}, {0x1ee74, 0x1ee77}, {0x1ee79, 0x1ee7c}, {0x1ee7e, 0x1ee7e},
    {0x1ee80, 0x1ee89}, {0x1ee8b, 0x1ee9b}, {0x1eea1, 0x1eea3}, {0x1eea5, 0x1eea9},
    {0x1eeab, 0x1eebb}, {0x1eef0, 0x1eef1}, {0x1f000, 0x1f02b}, {0x1f030, 0x1f093},
    {0x1f0a0, 0x1f0ae}, {0x1f0b1, 0x1f0bf}, {0x1f0c1, 0x1f0cf}, {0x1f0d1, 0x1f0f5},
    {0x1f100, 0x1f1ad}, {0x1f1e6, 0x1f202}, {0x1f210, 0x1f23b}, {0x1f240, 0x1f248},
    {0x1f250, 0x1f251}, {0x1f260, 0x1f265}, {0x1f300, 0x1f6d7}, {0x1f6dc, 0x1f6ec},
    {0x1f6f0, 0x1f6fc}, {0x1f700, 0x1f776}, {0x1f77b, 0x1f7d9}, {0x1f7e0, 0x1f7eb},
    {0x1f7f0, 0x1f7f0}, {0x1f800, 0x1f80b}, {0x1f810, 0x1f847}, {0x1f850, 0x1f859},
    {0x1f860, 0x1f887}, {0x1f890, 0x1f8ad}, {0x1f8b0, 0x1f8bb}, {0x1f8c0, 0x1f8c1},
    {0x1f900, 0x1fa53}, {0x1fa60, 0x1fa6d}, {0x1fa70, 0x1fa7c}, {0x1fa80, 0x1fa89},
    {0x1fa8f, 0x1fac6}, {0x1face, 0x1fadc}, {0x1fadf, 0x1fae9}, {0x1faf0, 0x1faf8},
    {0x1fb00, 0x1fb92}, {0x1fb94, 0x1fbf9}, {0x20000, 0x2a6df}, {0x2a700, 0x2b739},
    {0x2b740, 0x2b81d}, {0x2b820, 0x2cea1}, {0x2ceb0, 0x2ebe0}, {0x2ebf0, 0x2ee5d},
    {0x2f800, 0x2fa1d}, {0x30000, 0x3134a}, {0x31350, 0x323af}, {0xe0001, 0xe0001},
    {0xe0020, 0xe007f}, {0xe0100, 0xe01ef}, {0xf0000, 0xffffd}, {0x100000, 0x10fffd},
};

static b32 wprintable(i32 c)
{
    if (c < 0x80) {
        return c>=0x20 && c<0x7f;
    }
    iz lo = 0;
    iz hi = countof(printable);
    while (lo < hi) {
        iz mid = lo + (hi - lo)/2;
        if ((u32)c < printable[mid][0]) {
            hi = mid;
        } else if ((u32)c > printable[mid][1]) {
            lo = mid + 1;
        } else {
            return 1;
        }
    }
    return 0;
}

// Append code point c (above 0x7f) as bash's \u and \U escapes produce
// it: UTF-8 extended to 31 bits, or in the C locale an ISO C escape.
static void putucs(Buf *b, u32 c, b32 utf8)
{
    if (c > 0x7fffffff) {
        return;  // not representable at all
    }

    if (!utf8) {
        i32 n = c<0x10000 ? 4 : 8;
        putbyte(b, '\\');
        putbyte(b, n==4 ? 'u' : 'U');
        for (i32 i = n-1; i >= 0; i--) {
            putbyte(b, "0123456789ABCDEF"[c>>(4*i) & 15]);
        }
        return;
    }

    i32 n = c<0x800 ? 2 : c<0x10000 ? 3 : c<0x200000 ? 4 : c<0x4000000 ? 5 : 6;
    putbyte(b, (u8)(0xff00>>n | c>>(6*(n-1))));
    for (i32 i = n-2; i >= 0; i--) {
        putbyte(b, (u8)(0x80 | (c>>(6*i) & 0x3f)));
    }
}


// Escapes

enum : i32 {
    ESC_FORMAT,  // in the format string
    ESC_B,       // in a %b argument
};

// Before a diagnostic, flush the complete lines of output, as bash's
// line-buffered standard output has written them by then.
static void pfflush(PrintfEnv *env)
{
    Buf *out = env->out;
    if (!env->flush || !out) {
        return;
    }
    iz nl = out->len;
    for (; nl>env->flushed && out->data[nl-1]!='\n'; nl--) {}
    if (nl > env->flushed) {
        env->flush(env->ctx, span(out->data+env->flushed, out->data+nl));
        env->flushed = nl;
    }
}

static void escerror(PrintfEnv *env, Str msg)
{
    if (env->error) {
        pfflush(env);
        env->error(env->ctx, msg);
    }
}

// Translate the escape whose backslash is at s[i-1], appending it, and
// return the number of bytes consumed after the backslash. Zero means
// the backslash stands for itself (it was appended) and s[i] is plain.
// In a %b argument, \c sets *stop.
static iz tescape(PrintfEnv *env, Buf *b, Str s, iz i, i32 mode, b32 *stop)
{
    u8 c = byteat(s, i);
    iz p = i + 1;
    switch (c) {
    case 'a': putbyte(b, '\a'); break;
    case 'b': putbyte(b, '\b'); break;
    case 'e':
    case 'E': putbyte(b, 0x1b); break;
    case 'f': putbyte(b, '\f'); break;
    case 'n': putbyte(b, '\n'); break;
    case 'r': putbyte(b, '\r'); break;
    case 't': putbyte(b, '\t'); break;
    case 'v': putbyte(b, '\v'); break;

    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': {
        // %b allows a leading 0 before three more digits
        i32 v = c - '0';
        i32 n = 2 + (!v && mode==ESC_B);
        for (; p<s.len && octdigit(s[p]) && n-- > 0; p++) {
            v = v*8 + s[p] - '0';
        }
        putbyte(b, (u8)v);
    } break;

    case 'x': {
        i32 v = 0;
        for (i32 n = 2; p<s.len && hexdigit(s[p])>=0 && n-- > 0; p++) {
            v = v*16 + hexdigit(s[p]);
        }
        if (p == i+1) {
            escerror(env, "printf: missing hex digit for \\x");
            putbyte(b, '\\');
            return 0;
        }
        putbyte(b, (u8)v);
    } break;

    case 'u':
    case 'U': {
        u32 v = 0;
        for (i32 n = c=='u' ? 4 : 8; p<s.len && hexdigit(s[p])>=0 && n-- > 0; p++) {
            v = v*16 + (u32)hexdigit(s[p]);
        }
        if (p == i+1) {
            escerror(env, c=='u' ? Str("printf: missing unicode digit for \\u")
                                 : Str("printf: missing unicode digit for \\U"));
            putbyte(b, '\\');
            return 0;
        }
        if (v < 0x80) {
            putbyte(b, (u8)v);
        } else {
            putucs(b, v, env->utf8);
        }
    } break;

    case '\\': putbyte(b, '\\'); break;

    case '\'':
    case '"':
    case '?':
        if (mode == ESC_B) {
            putbyte(b, '\\');
            return 0;
        }
        putbyte(b, c);
        break;

    case 'c':
        if (mode == ESC_B) {
            *stop = 1;
            break;
        }
        putbyte(b, '\\');
        return 0;

    default:
        putbyte(b, '\\');
        return 0;
    }
    return p - i;
}

// Expand the escapes in a %b argument. The final byte is always plain.
static void bexpand(PrintfEnv *env, Buf *b, Str s, b32 *stop)
{
    for (iz i = 0; i < s.len;) {
        if (i==s.len-1 || s[i]!='\\') {
            putbyte(b, s[i++]);
            continue;
        }
        iz n = tescape(env, b, s, i+1, ESC_B, stop);
        if (*stop) {
            return;
        }
        i += 1 + n;
    }
}

// Expand echo -e escapes, which differ from %b in the details: \1-\7
// are not octal, and \x or \u without digits pass through silently.
static void echoexpand(Buf *b, Str s, b32 utf8, b32 *stop)
{
    for (iz i = 0; i < s.len;) {
        u8 c = s[i++];
        if (c!='\\' || i==s.len) {
            putbyte(b, c);
            continue;
        }

        c = s[i++];
        switch (c) {
        case 'a': c = '\a'; break;
        case 'b': c = '\b'; break;
        case 'e':
        case 'E': c = 0x1b; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case 'v': c = '\v'; break;

        case '0': {
            i32 v = 0;
            for (i32 n = 3; i<s.len && octdigit(s[i]) && n-- > 0; i++) {
                v = v*8 + s[i] - '0';
            }
            c = (u8)v;
        } break;

        case 'x': {
            iz  beg = i;
            i32 v   = 0;
            for (i32 n = 2; i<s.len && hexdigit(s[i])>=0 && n-- > 0; i++) {
                v = v*16 + hexdigit(s[i]);
            }
            if (i == beg) {
                putbyte(b, '\\');
            } else {
                c = (u8)v;
            }
        } break;

        case 'u':
        case 'U': {
            iz  beg = i;
            u32 v   = 0;
            i32 n   = c=='u' ? 4 : 8;
            for (; i<s.len && hexdigit(s[i])>=0 && n-- > 0; i++) {
                v = v*16 + (u32)hexdigit(s[i]);
            }
            if (i == beg) {
                putbyte(b, '\\');
            } else if (v < 0x80) {
                c = (u8)v;
            } else {
                putucs(b, v, utf8);
                continue;
            }
        } break;

        case '\\':
            break;

        case 'c':
            *stop = 1;
            return;

        default:  // including \1-\7, \' \" \?
            putbyte(b, '\\');
            break;
        }
        putbyte(b, c);
    }
}

// Escape processing for echo -e (percentb=0) or printf %b (percentb=1,
// without its diagnostics). *stop is set when \c ended the output.
static Str echoescape(Arena *a, Str s, b32 percentb, b32 utf8, b32 *stop)
{
    Buf b(a, s.len+16);
    *stop = 0;
    if (percentb) {
        PrintfEnv env = {};
        env.utf8 = utf8;
        bexpand(&env, &b, s, stop);
    } else {
        echoexpand(&b, s, utf8, stop);
    }
    return shrinkbuf(&b);
}


// Quoting

// Does s need $'...' quoting: control characters, or anything that is
// not a printable character in the locale?
static b32 shouldquote(Str s, b32 utf8)
{
    for (iz i = 0; i < s.len;) {
        u8 c = s[i];
        if (c < 0x80) {
            if (c<0x20 || c==0x7f) {
                return 1;
            }
            i++;
        } else {
            Mbchar u = utf8 ? mbcharat(s, i) : Mbchar{};
            if (!u.len || !wprintable(u.c)) {
                return 1;
            }
            i += u.len;
        }
    }
    return 0;
}

static void ansicquote(Buf *b, Str s, b32 utf8)
{
    print(b, "$'");
    for (iz i = 0; i < s.len;) {
        u8 c = s[i];
        u8 e = 0;
        switch (c) {
        case 0x1b: e = 'E'; break;
        case '\a': e = 'a'; break;
        case '\v': e = 'v'; break;
        case '\b': e = 'b'; break;
        case '\f': e = 'f'; break;
        case '\n': e = 'n'; break;
        case '\r': e = 'r'; break;
        case '\t': e = 't'; break;
        case '\\':
        case '\'': e = c;   break;
        }
        if (e) {
            putbyte(b, '\\');
            putbyte(b, e);
            i++;
        } else if (c>=0x20 && c<0x7f) {
            putbyte(b, c);
            i++;
        } else {
            Mbchar u = utf8 && c>=0x80 ? mbcharat(s, i) : Mbchar{};
            if (u.len && wprintable(u.c)) {
                print(b, {s.data+i, u.len});
                i += u.len;
            } else {
                putbyte(b, '\\');
                putbyte(b, (u8)('0' + (c>>6)));
                putbyte(b, (u8)('0' + (c>>3 & 7)));
                putbyte(b, (u8)('0' + (c    & 7)));
                i++;
            }
        }
    }
    putbyte(b, '\'');
}

static void singlequote(Buf *b, Str s)
{
    if (s == "'") {
        print(b, "\\'");
        return;
    }
    putbyte(b, '\'');
    for (iz i = 0; i < s.len; i++) {
        putbyte(b, s[i]);
        if (s[i] == '\'') {
            print(b, "\\''");
        }
    }
    putbyte(b, '\'');
}

static b32 shellspecial(u8 c)
{
    switch (c) {
    case '\t': case '\n': case ' ': case '!': case '"': case '$':
    case '&':  case '\'': case '(': case ')': case '*': case ',':
    case ';':  case '<':  case '>': case '?': case '[': case '\\':
    case ']':  case '^':  case '`': case '{': case '|': case '}':
        return 1;
    }
    return 0;
}

static void backslashquote(Buf *b, Str s)
{
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (shellspecial(c) ||
            (c=='#' && !i) ||
            (c=='~' && (!i || s[i-1]==':' || s[i-1]=='='))) {
            putbyte(b, '\\');
        }
        putbyte(b, c);
    }
}

// Quote like printf %q (alt: %#q, single quotes instead of backslashes).
static void quote(Buf *b, Str s, b32 utf8, b32 alt)
{
    if (!s.len) {
        print(b, "''");
    } else if (shouldquote(s, utf8)) {
        ansicquote(b, s, utf8);
    } else if (alt) {
        singlequote(b, s);
    } else {
        backslashquote(b, s);
    }
}

static Str shellquote(Arena *a, Str s, b32 utf8)
{
    Buf b(a, 2*s.len+3);
    quote(&b, s, utf8, 0);
    return shrinkbuf(&b);
}


// Numeric arguments

struct PfNum {
    u64 bits;
    iz  end;    // just past the number, 0 without digits
    b32 range;  // out of range, bits clamped
};

// Parse like strtoimax or strtoumax with base 0, as glibc 2.38+ does
// under _GNU_SOURCE (C23 rules): 0x for hex, 0b for binary, 0 octal.
static PfNum scannum(Str s, b32 unsign)
{
    PfNum r = {};
    iz    i = 0;
    for (; i<s.len && whitespace(s[i]); i++) {}

    b32 neg = 0;
    if (i<s.len && (s[i]=='+' || s[i]=='-')) {
        neg = s[i++] == '-';
    }

    i32 base = 10;
    if (byteat(s, i) == '0') {
        base = 8;
        u8 x = lowercase(byteat(s, i+1));
        if (x=='x' || x=='b') {
            i32 d = hexdigit(byteat(s, i+2));
            if (d<0 || (x=='b' && d>1)) {
                r.end = i + 1;  // just the 0
                return r;
            }
            base = x=='x' ? 16 : 2;
            i += 2;
        }
    }

    iz  beg  = i;
    u64 mag  = 0;
    b32 over = 0;
    for (; i < s.len; i++) {
        i32 d = hexdigit(s[i]);
        if (d<0 || d>=base) {
            break;
        }
        if (mag > (~0ull - (u64)d)/(u64)base) {
            over = 1;
        } else {
            mag = mag*(u64)base + (u64)d;
        }
    }
    if (i == beg) {
        return r;
    }

    r.end = i;
    if (unsign) {
        r.range = over;
        r.bits  = over ? ~0ull : neg ? -mag : mag;
    } else {
        u64 limit = neg ? 1ull<<63 : (1ull<<63) - 1;
        r.range = over || mag>limit;
        r.bits  = r.range ? limit : neg ? -mag : mag;
    }
    return r;
}


// Time formatting: glibc's strftime in the C locale

struct PfTm {
    i32 sec;
    i32 min;
    i32 hour;
    i32 mday;
    i32 mon;
    i32 year;  // minus 1900
    i32 wday;
    i32 yday;
    i64 secs;    // for %s
    i64 gmtoff;  // seconds east of UTC
    Str zone;
    b32 ok;
};

static b32 leapyear(i64 y)
{
    return y%4==0 && (y%100!=0 || y%400==0);
}

// Like gmtime: fails when the year does not fit struct tm.
static PfTm breaktime(i64 t)
{
    PfTm r    = {};
    i64  days = t / 86400;
    i64  rem  = t % 86400;
    if (rem < 0) {
        rem  += 86400;
        days -= 1;
    }
    r.hour = (i32)(rem / 3600);
    r.min  = (i32)(rem / 60 % 60);
    r.sec  = (i32)(rem % 60);
    r.wday = (i32)((days%7 + 11) % 7);  // 1970-01-01 was a Thursday

    // Civil date from days since the epoch (era-based algorithm)
    i64 z   = days + 719468;
    i64 era = (z>=0 ? z : z-146096) / 146097;
    i64 doe = z - era*146097;
    i64 yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    i64 doy = doe - (365*yoe + yoe/4 - yoe/100);
    i64 mp  = (5*doy + 2) / 153;
    i64 d   = doy - (153*mp + 2)/5 + 1;
    i64 m   = mp<10 ? mp+3 : mp-9;
    i64 y   = yoe + era*400 + (m <= 2);

    static i32 cumul[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    i64 ty = y - 1900;
    if (ty<-0x7fffffffll-1 || ty>0x7fffffffll) {
        return r;
    }
    r.year = (i32)ty;
    r.mon  = (i32)(m - 1);
    r.mday = (i32)d;
    r.yday = cumul[r.mon] + r.mday - 1 + (r.mon>1 && leapyear(y));
    r.secs = t;
    r.zone = "UTC";
    r.ok   = 1;
    return r;
}

// Time zones: POSIX TZ strings, as glibc reads them when no zoneinfo
// file has the name. A string with daylight saving time but no rules
// gets glibc's built-in default, the United States rules since 2007.
// A zoneinfo (TZif) file adds its transitions: glibc uses them up to
// the last one and the file's footer string after it.

struct PfRule {
    u8  kind;  // 'J' Jn, 'N' n, 'M' Mm.w.d
    i32 d;
    i32 m;
    i32 w;
    i64 secs;  // local time of day of the change
};

struct PfType {
    i64 off;   // seconds east of UTC
    b32 dst;
    Str name;
};

struct PfZone {
    Str     name[2];  // standard, daylight
    i64     off[2];   // seconds east of UTC
    b32     dst;
    PfRule  rule[2];  // daylight saving time starts, ends
    b32     ok;       // the string had a name and an offset
    // From a zoneinfo file, else ntypes is zero
    i64    *times;    // transitions, ascending
    u8     *index;    // the type each transition starts
    iz      ntimes;
    PfType *types;
    iz      ntypes;
    b32     hasrule;  // the footer gave the rule above
};

static b32 zonename(Str s, iz *i, Str *name)
{
    iz k = *i;
    if (byteat(s, k) == '<') {
        iz beg = ++k;
        for (; alnum(byteat(s, k)) || byteat(s, k)=='+' || byteat(s, k)=='-'; k++) {}
        if (k-beg<3 || byteat(s, k)!='>') {
            return 0;
        }
        *name = span(s.data+beg, s.data+k);
        *i    = k + 1;
        return 1;
    }
    for (; letter(byteat(s, k)); k++) {}
    if (k-*i < 3) {
        return 0;
    }
    *name = span(s.data+*i, s.data+k);
    *i    = k;
    return 1;
}

// [+-]hh[:mm[:ss]] in seconds, the hours clamped to maxh, minutes and
// seconds to 59. False without digits.
static b32 tzclock(Str s, iz *i, i64 maxh, i64 *secs)
{
    iz  k   = *i;
    i64 neg = 1;
    if (byteat(s, k)=='+' || byteat(s, k)=='-') {
        neg = s[k++]=='-' ? -1 : 1;
    }
    if (!digit(byteat(s, k))) {
        return 0;
    }
    i64 part[3] = {};
    for (i32 n = 0; n < 3; n++) {
        for (; digit(byteat(s, k)); k++) {
            part[n] = part[n]<100000 ? part[n]*10 + s[k] - '0' : part[n];
        }
        if (n==2 || byteat(s, k)!=':' || !digit(byteat(s, k+1))) {
            break;
        }
        k++;
    }
    i64 h = part[0]<maxh ? part[0] : maxh;
    i64 m = part[1]<59 ? part[1] : 59;
    i64 c = part[2]<59 ? part[2] : 59;
    *secs = neg * (h*3600 + m*60 + c);
    *i    = k;
    return 1;
}

static b32 tzrule(Str s, iz *i, PfRule *r)
{
    iz k = *i;
    r->secs = 7200;
    if (byteat(s, k) == 'J') {
        r->kind = 'J';
        k++;
    } else if (byteat(s, k) == 'M') {
        r->kind = 'M';
        k++;
    } else {
        r->kind = 'N';
    }
    i32 v[3] = {};
    for (i32 n = 0; n < (r->kind=='M' ? 3 : 1); n++) {
        if (n && byteat(s, k++)!='.') {
            return 0;
        }
        if (!digit(byteat(s, k))) {
            return 0;
        }
        for (; digit(byteat(s, k)); k++) {
            v[n] = v[n]<100000 ? v[n]*10 + s[k] - '0' : v[n];
        }
    }
    if (r->kind=='J' && (v[0]<1 || v[0]>365)) return 0;
    if (r->kind=='N' && v[0]>365) return 0;
    if (r->kind=='M' && (v[0]<1 || v[0]>12 || v[1]<1 || v[1]>5 || v[2]>6)) return 0;
    r->d = r->kind=='M' ? v[2] : v[0];
    r->m = v[0];
    r->w = v[1];
    if (byteat(s, k) == '/') {
        k++;
        if (!tzclock(s, &k, 167, &r->secs)) {
            return 0;
        }
    }
    *i = k;
    return 1;
}

// Parse TZ; an empty value is glibc's "Universal".
static PfZone parsezone(Str tz)
{
    PfZone z = {};
    iz i = byteat(tz, 0)==':';
    if (i == tz.len) {
        z.name[0] = z.name[1] = "Universal";
        return z;
    }
    if (!zonename(tz, &i, &z.name[0])) {
        return z;  // UTC without a name
    }
    z.name[1] = z.name[0];
    i64 off;
    if (!tzclock(tz, &i, 24, &off)) {
        return z;
    }
    z.ok     = 1;
    z.off[0] = z.off[1] = -off;
    if (!zonename(tz, &i, &z.name[1])) {
        z.name[1] = z.name[0];
        return z;
    }
    z.dst    = 1;
    z.off[1] = tzclock(tz, &i, 24, &off) ? -off : z.off[0]+3600;
    b32 ok = byteat(tz, i) == ',';
    if (ok) {
        i++;
        ok = tzrule(tz, &i, &z.rule[0]) && byteat(tz, i)==',';
    }
    if (ok) {
        i++;
        ok = tzrule(tz, &i, &z.rule[1]);
    }
    if (!ok) {
        PfRule start = {'M', 0, 3, 2, 7200};
        PfRule end   = {'M', 0, 11, 1, 7200};
        z.rule[0] = start;
        z.rule[1] = end;
    }
    return z;
}

static i64 daysfromcivil(i64 y, i64 m, i64 d)
{
    y -= m <= 2;
    i64 era = (y>=0 ? y : y-399) / 400;
    i64 yoe = y - era*400;
    i64 doy = (153*(m>2 ? m-3 : m+9) + 2)/5 + d - 1;
    i64 doe = yoe*365 + yoe/4 - yoe/100 + doy;
    return era*146097 + doe - 719468;
}

// When a rule changes the clock in a year, in UTC. As glibc, years up
// to 1970 count their days from the start of 1970.
static i64 rulechange(PfRule *r, i64 year, i64 off)
{
    static i32 cumul[2][13] = {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366},
    };
    b32 leap = leapyear(year);
    i64 t    = year>1970 ? daysfromcivil(year, 1, 1)*86400 : 0;
    switch (r->kind) {
    case 'J':
        t += (r->d - 1 + (r->d>=60 && leap)) * (i64)86400;
        break;
    case 'N':
        t += r->d * (i64)86400;
        break;
    case 'M': {
        i64 first = daysfromcivil(year, r->m, 1);
        i64 dow   = (first%7 + 11) % 7;  // 1970-01-01 was a Thursday
        i64 len   = cumul[leap][r->m] - cumul[leap][r->m-1];
        i64 d     = (r->d - dow + 7) % 7;
        for (i32 n = 1; n<r->w && d+7<len; n++) {
            d += 7;
        }
        t += (cumul[leap][r->m-1] + d) * 86400;
    } break;
    }
    return t - off + r->secs;
}

// Like localtime in the zone. As glibc, fails unless both the UTC and
// the local time fit struct tm, which also keeps t+off from overflow.
static PfTm localtm(i64 t, PfZone *z)
{
    PfTm u = breaktime(t);
    if (!u.ok) {
        return u;
    }
    b32 ruled = !z->ntypes || (z->hasrule && z->ntimes && t>=z->times[z->ntimes-1]);
    i64 off   = 0;
    Str name  = {};
    if (ruled) {
        b32 isdst = 0;
        if (z->dst) {
            i64 year  = u.year + (i64)1900;
            i64 start = rulechange(&z->rule[0], year, z->off[0]);
            i64 end   = rulechange(&z->rule[1], year, z->off[1]);
            isdst = start>end ? t<end || t>=start : t>=start && t<end;
        }
        off  = z->off[isdst];
        name = z->name[isdst];
    } else {
        // Before the first transition the first standard type, as
        // glibc, else the type the last transition up to t started
        PfType *ty = z->types;
        if (!z->ntimes || t<z->times[0]) {
            for (iz i = z->ntypes-1; i >= 0; i--) {
                ty = z->types[i].dst ? ty : z->types+i;
            }
        } else {
            iz lo = 0, hi = z->ntimes;  // times[lo] <= t < times[hi]
            while (hi-lo > 1) {
                iz mid = lo + (hi-lo)/2;
                if (z->times[mid] <= t) {
                    lo = mid;
                } else {
                    hi = mid;
                }
            }
            ty = z->types + z->index[lo];
        }
        off  = ty->off;
        name = ty->name;
    }
    PfTm r   = breaktime(t + off);
    r.secs   = t;
    r.gmtoff = off;
    r.zone   = name;
    return r;
}

static u32 tzifu32(u8 *p)
{
    return (u32)p[0]<<24 | (u32)p[1]<<16 | (u32)p[2]<<8 | p[3];
}

// Read a zoneinfo (TZif) file: its transitions and types, and the
// footer's rule if it has one (version 2 and later). Not ok unless the
// file is well formed; a footer that does not parse is ignored.
static PfZone parsetzif(Arena *a, Str s)
{
    PfZone z = {};
    if (s.len<44 || !startswith(s, "TZif")) {
        return z;
    }
    b32 v2 = s[4] >= '2';
    iz  at = 0;
    for (i32 pass = 0; pass < 1+v2; pass++) {
        if (s.len-at<44 || !startswith(cuthead(s, at), "TZif")) {
            return z;
        }
        iz n[6];  // isutcnt isstdcnt leapcnt timecnt typecnt charcnt
        for (i32 k = 0; k < 6; k++) {
            u32 v = tzifu32(s.data + at + 20 + 4*k);
            if (v > (u32)s.len) return z;
            n[k] = (iz)v;
        }
        iz nleap = n[2], ntimes = n[3], ntypes = n[4], nchars = n[5];
        if (!ntypes || ntypes>256 || !nchars) {
            return z;
        }
        iz tlen = pass ? 8 : 4;  // bytes per time: 8 in the v2 block
        iz size = ntimes*tlen + ntimes + ntypes*6 + nchars + nleap*(tlen+4) + n[1] + n[0];
        at += 44;
        if (size > s.len-at) {
            return z;
        }
        if (v2 && !pass) {
            at += size;  // the v1 block: skip to the 64-bit one
            continue;
        }
        u8 *p = s.data + at;
        z.times  = alloc<i64>(a, ntimes);
        z.index  = alloc<u8>(a, ntimes);
        z.types  = alloc<PfType>(a, ntypes);
        z.ntimes = ntimes;
        z.ntypes = ntypes;
        for (iz i = 0; i < ntimes; i++, p += tlen) {
            u64 v = tlen==8 ? (u64)tzifu32(p)<<32 | tzifu32(p+4)
                            : (u64)(i64)(i32)tzifu32(p);
            z.times[i] = (i64)v;
            if (i && z.times[i]<=z.times[i-1]) return {};
        }
        for (iz i = 0; i < ntimes; i++, p++) {
            if (*p >= ntypes) return {};
            z.index[i] = *p;
        }
        u8 *chars = p + ntypes*6;
        for (iz i = 0; i < ntypes; i++, p += 6) {
            if (p[5] >= nchars) return {};
            z.types[i].off = (i32)tzifu32(p);
            z.types[i].dst = p[4] != 0;
            Str name = {chars+p[5], nchars-p[5]};
            z.types[i].name = cut(name, 0).head;
        }
        at += size;
    }
    if (v2) {
        // The footer: a newline, a TZ string, a newline
        Str rest = cuthead(s, at);
        Cut c    = rest.len ? cut(cuthead(rest, 1), '\n') : Cut{};
        if (rest.len && rest[0]=='\n' && c.ok && c.head.len) {
            PfZone f = parsezone(c.head);
            if (f.ok) {
                for (i32 k = 0; k < 2; k++) {
                    z.name[k] = f.name[k];
                    z.off[k]  = f.off[k];
                    z.rule[k] = f.rule[k];
                }
                z.dst     = f.dst;
                z.hasrule = 1;
            }
        }
    }
    z.ok = 1;
    return z;
}

// TIME_ZONE_INFORMATION's parts, with names in UTF-8 (wintz)
struct WinDate {
    i32 year;    // 0: a rule, day is the week (1-5, 5 the last)
    i32 month;   // 0: no daylight saving time
    i32 dow;
    i32 day;
    i32 hour;
    i32 minute;
    i32 second;
    i32 ms;
};

struct WinZone {
    i32     bias;     // minutes: UTC is local time plus bias
    i32     stdbias;
    i32     dstbias;
    WinDate stddate;  // when standard time starts
    WinDate dstdate;  // when daylight saving time starts
    Str     stdname;
    Str     dstname;
    b32     nodst;    // DynamicDaylightTimeDisabled
};

// A zone abbreviation from a Windows zone name, as Cygwin makes one:
// its capital letters ("Eastern Standard Time" is EST). With fewer than
// three, as a localized name may have, the offset in angle brackets.
static void wintzname(Buf *b, Str name, i32 west)
{
    iz n = 0;
    for (iz i = 0; i < name.len; i++) n += name[i]>='A' && name[i]<='Z';
    if (n >= 3) {
        for (iz i = 0; i < name.len; i++) {
            if (name[i]>='A' && name[i]<='Z') putbyte(b, name[i]);
        }
        return;
    }
    i32 east = -west;
    print(b, east<0 ? Str("<-") : Str("<+"));
    east = east<0 ? -east : east;
    putbyte(b, (u8)('0' + east/600%10));
    putbyte(b, (u8)('0' + east/60%10));
    putbyte(b, (u8)('0' + east%60/10));
    putbyte(b, (u8)('0' + east%10));
    putbyte(b, '>');
}

// [-]h[:mm[:ss]]
static void wintzclock(Buf *b, i64 secs)
{
    if (secs < 0) {
        putbyte(b, '-');
        secs = -secs;
    }
    print(b, secs/3600);
    if (secs % 3600) {
        putbyte(b, ':');
        putbyte(b, (u8)('0' + secs/600%6));
        putbyte(b, (u8)('0' + secs/60%10));
        if (secs % 60) {
            putbyte(b, ':');
            putbyte(b, (u8)('0' + secs%60/10));
            putbyte(b, (u8)('0' + secs%10));
        }
    }
}

static void wintzrule(Buf *b, WinDate *d)
{
    static i32 cumul[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    putbyte(b, ',');
    if (d->year) {
        // A fixed date, counted without February 29
        putbyte(b, 'J');
        print(b, (i64)cumul[(d->month-1)%12] + d->day);
    } else {
        putbyte(b, 'M');
        print(b, (i64)d->month);
        putbyte(b, '.');
        print(b, (i64)d->day);
        putbyte(b, '.');
        print(b, (i64)d->dow);
    }
    i64 secs = d->hour*(i64)3600 + d->minute*60 + d->second + (d->ms>=500);
    if (secs != 7200) {
        putbyte(b, '/');
        wintzclock(b, secs);
    }
}

// The POSIX TZ string for a Windows zone: names from the zone's names,
// offsets from the biases, rules from the transition dates.
static Str wintz(Arena *a, WinZone *w)
{
    Buf b(a, 64);
    i32 std = w->bias + w->stdbias;  // minutes west, as POSIX counts
    wintzname(&b, w->stdname, std);
    wintzclock(&b, std*(i64)60);
    b32 valid = w->stddate.month>=1 && w->stddate.month<=12 &&
                w->dstdate.month>=1 && w->dstdate.month<=12;
    if (w->nodst || !valid) {
        return finish(&b);
    }
    i32 dst = w->bias + w->dstbias;
    wintzname(&b, w->dstname, dst);
    if (dst != std-60) {
        wintzclock(&b, dst*(i64)60);
    }
    wintzrule(&b, &w->dstdate);
    wintzrule(&b, &w->stddate);
    return finish(&b);
}

struct PfTout {
    u8  data[128];  // the buffer bash gives strftime
    iz  len;
    b32 full;       // strftime would return 0
};

enum : i32 {
    CASE_KEEP,
    CASE_LOWER,
    CASE_UPPER,
};

// glibc's add(): pad to width (zeros for pad '0'), then s.
static void tadd(PfTout *t, Str s, i32 width, i32 pad, i32 casing)
{
    i64 delta = (i64)width - s.len;
    i64 incr  = s.len + (delta>0 ? delta : 0);
    if (t->full || incr>=countof(t->data)-t->len) {
        t->full = 1;
        return;
    }
    for (; delta > 0; delta--) {
        t->data[t->len++] = pad=='0' ? '0' : ' ';
    }
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        c = casing==CASE_LOWER ? lowercase(c) : casing==CASE_UPPER ? upcase(c) : c;
        t->data[t->len++] = c;
    }
}

// Raw bytes with strftime's size check but no width.
static void tpad(PfTout *t, u8 c, i64 n)
{
    if (t->full || n>=countof(t->data)-t->len) {
        t->full = 1;
        return;
    }
    for (; n > 0; n--) {
        t->data[t->len++] = c;
    }
}

// A number padded as glibc's strftime pads it: to at least digits
// places and the width, with zeros by default or spaces for pad '_'.
// Pad '-' drops the digits minimum but keeps the width, with spaces.
// A sign goes before zeros and after spaces.
static void tnum(PfTout *t, i64 v, i32 digits, i32 pad, i32 width)
{
    u8  tmp[32];
    u8 *end = tmp + countof(tmp);
    u8 *beg = end;
    b32 neg = v < 0;
    u64 u   = neg ? -(u64)v : (u64)v;
    do {
        *--beg = (u8)('0' + u%10);
    } while (u /= 10);

    pad = pad ? pad : '0';
    i64 w    = width>0 ? width : 0;
    w        = pad!='-' && w<digits ? digits : w;
    i64 fill = w - neg - (end - beg);
    fill = fill>0 ? fill : 0;
    if (pad == '0') {
        tpad(t, '-', neg);
        tpad(t, '0', fill);
    } else {
        tpad(t, ' ', fill);
        tpad(t, '-', neg);
    }
    tadd(t, span(beg, end), -1, 0, CASE_KEEP);
}

static i32 isoweekdays(i32 yday, i32 wday)
{
    i32 big = (366/7 + 2) * 7;
    return yday - (yday - wday + 4 + big)%7 + 4 - 1;
}

static void tsub(PfTout *, Str, PfTm *, i32 width, i32 pad, i32 casing);

// Format tm like glibc's strftime in the C locale.
static void fmttime(PfTout *t, Str f, PfTm *tm)
{
    static Str days[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
        "Friday", "Saturday",
    };
    static Str months[] = {
        "January", "February", "March", "April", "May", "June", "July",
        "August", "September", "October", "November", "December",
    };

    for (iz i = 0; i<f.len && !t->full; i++) {
        if (f[i] != '%') {
            tadd(t, {f.data+i, 1}, -1, 0, CASE_KEEP);
            continue;
        }

        iz  pct    = i;
        i32 pad    = 0;
        i32 casing = CASE_KEEP;
        b32 change = 0;
        for (;; i++) {
            u8 c = byteat(f, i+1);
            if (c=='_' || c=='-' || c=='0') {
                pad = c;
            } else if (c == '^') {
                casing = CASE_UPPER;
            } else if (c == '#') {
                change = 1;
            } else {
                break;
            }
        }
        i++;

        i32 width = -1;
        if (digit(byteat(f, i))) {
            width = 0;
            for (; digit(byteat(f, i)); i++) {
                i32 d = f[i] - '0';
                if (width>0x7fffffff/10 || (width==0x7fffffff/10 && d>7)) {
                    width = 0x7fffffff;
                } else {
                    width = width*10 + d;
                }
            }
        }

        i32 mod = 0;
        if (byteat(f, i)=='E' || byteat(f, i)=='O') {
            mod = f[i++];
        }

        i32 hour12 = tm->hour%12 ? tm->hour%12 : 12;
        Str sub    = {};
        i32 digits = 0;
        i64 num    = 0;
        b32 space  = 0;  // pad numbers with spaces by default
        switch (byteat(f, i)) {
        case '%':
            // any modifier
            tadd(t, "%", width, pad, CASE_KEEP);
            continue;

        case 'a':
        case 'A':
            if (mod) goto bad;
            if (change) casing = CASE_UPPER;
            sub = days[tm->wday];
            if (f[i] == 'a') sub = takehead(sub, 3);
            tadd(t, sub, width, pad, casing);
            continue;

        case 'b':
        case 'h':
        case 'B':
            if (change) casing = CASE_UPPER;  // even for a bad format
            if (mod == 'E') goto bad;
            sub = months[tm->mon];
            if (f[i] != 'B') sub = takehead(sub, 3);
            tadd(t, sub, width, pad, casing);
            continue;

        case 'c':
            if (mod == 'O') goto bad;
            sub = "%a %b %e %H:%M:%S %Y";
            goto subformat;

        case 'C':
            // no era in the C locale
            {
                i32 y = (i32)((u32)tm->year + 1900u);
                num    = y/100 - (y%100 < 0);
                digits = 1;
            }
            goto number;

        case 'x':
            if (mod == 'O') goto bad;
            sub = "%m/%d/%y";
            goto subformat;

        case 'D':
            if (mod) goto bad;
            sub = "%m/%d/%y";
            goto subformat;

        case 'd':
            if (mod == 'E') goto bad;
            num = tm->mday; digits = 2;
            goto number;

        case 'e':
            if (mod == 'E') goto bad;
            num = tm->mday; digits = 2; space = 1;
            goto number;

        case 'F':
            if (mod) goto bad;
            sub = "%Y-%m-%d";
            goto subformat;

        case 'H':
            if (mod == 'E') goto bad;
            num = tm->hour; digits = 2;
            goto number;

        case 'I':
            if (mod == 'E') goto bad;
            num = hour12; digits = 2;
            goto number;

        case 'k':
            if (mod == 'E') goto bad;
            num = tm->hour; digits = 2; space = 1;
            goto number;

        case 'l':
            if (mod == 'E') goto bad;
            num = hour12; digits = 2; space = 1;
            goto number;

        case 'j':
            if (mod == 'E') goto bad;
            num = tm->yday + 1; digits = 3;
            goto number;

        case 'M':
            if (mod == 'E') goto bad;
            num = tm->min; digits = 2;
            goto number;

        case 'm':
            if (mod == 'E') goto bad;
            num = tm->mon + 1; digits = 2;
            goto number;

        case 'n':
            tadd(t, "\n", width, pad, CASE_KEEP);
            continue;

        case 't':
            tadd(t, "\t", width, pad, CASE_KEEP);
            continue;

        case 'P':
        case 'p': {
            b32 lower = f[i]=='P' || change;
            if (change) casing = CASE_KEEP;
            sub = tm->hour>11 ? Str("PM") : Str("AM");
            tadd(t, sub, width, pad, lower ? CASE_LOWER : casing);
        } continue;

        case 'R':
            sub = "%H:%M";
            goto subformat;

        case 'r':
            sub = "%I:%M:%S %p";
            goto subformat;

        case 'S':
            if (mod == 'E') goto bad;
            num = tm->sec; digits = 2;
            goto number;

        case 's': {
            // the full time_t, signed, at least one digit
            u8  tmp[32];
            u8 *end = tmp + countof(tmp);
            u8 *beg = end;
            i64 v   = tm->secs;
            do {
                i64 d = v % 10;
                *--beg = (u8)('0' + (d<0 ? -d : d));
            } while (v /= 10);
            if (tm->secs < 0) {
                *--beg = '-';
            }
            tadd(t, span(beg, end), width, pad, CASE_KEEP);
        } continue;

        case 'X':
            if (mod == 'O') goto bad;
            sub = "%H:%M:%S";
            goto subformat;

        case 'T':
            sub = "%H:%M:%S";
            goto subformat;

        case 'u':
            num = (tm->wday + 6)%7 + 1; digits = 1;
            goto number;

        case 'U':
            if (mod == 'E') goto bad;
            num = (tm->yday - tm->wday + 7) / 7; digits = 2;
            goto number;

        case 'V':
        case 'g':
        case 'G': {
            if (mod == 'E') goto bad;
            i32 year   = tm->year + (tm->year<0 ? 1900%400 : 1900%400 - 400);
            i32 adjust = 0;
            i32 wdays  = isoweekdays(tm->yday, tm->wday);
            if (wdays < 0) {
                adjust = -1;
                wdays  = isoweekdays(tm->yday + 365 + leapyear(year-1), tm->wday);
            } else {
                i32 d = isoweekdays(tm->yday - 365 - leapyear(year), tm->wday);
                if (d >= 0) {
                    adjust = 1;
                    wdays  = d;
                }
            }
            i32 iso = (i32)((u32)tm->year + 1900u + (u32)adjust);  // wraps
            if (f[i] == 'g') {
                num    = (iso%100 + 100) % 100;
                digits = 2;
            } else if (f[i] == 'G') {
                num    = iso;
                digits = 1;
            } else {
                num    = wdays/7 + 1;
                digits = 2;
            }
        } goto number;

        case 'W':
            if (mod == 'E') goto bad;
            num = (tm->yday - (tm->wday + 6)%7 + 7) / 7; digits = 2;
            goto number;

        case 'w':
            if (mod == 'E') goto bad;
            num = tm->wday; digits = 1;
            goto number;

        case 'Y':
            if (mod == 'O') goto bad;
            num = (i32)((u32)tm->year + 1900u); digits = 1;
            goto number;

        case 'y':
            num = (tm->year%100 + 100) % 100; digits = 2;
            goto number;

        case 'Z':
            if (change) casing = CASE_LOWER;
            tadd(t, tm->zone, width, pad, casing);
            continue;

        case 'z': {
            i64 diff = (tm->gmtoff<0 ? -tm->gmtoff : tm->gmtoff) / 60;
            tadd(t, tm->gmtoff<0 ? Str("-") : Str("+"), width, pad, CASE_KEEP);
            num = (i32)(diff/60*100 + diff%60); digits = 4;
        } goto number;

        case 0:
            i--;
            goto bad;

        default:
        bad:
            tadd(t, {f.data+pct, i-pct+1}, width, pad, casing);
            continue;
        }

    number:
        if (space && pad!='0' && pad!='-') {
            pad = '_';
        }
        tnum(t, num, digits, pad, width);
        continue;

    subformat:
        tsub(t, sub, tm, width, pad, casing);
    }
}

// A conversion defined by another format: the result is padded as a
// whole, then uppercased for '^' (padding included).
static void tsub(PfTout *t, Str sub, PfTm *tm, i32 width, i32 pad, i32 casing)
{
    PfTout tmp = {};
    fmttime(&tmp, sub, tm);
    iz beg = t->len;
    tadd(t, {tmp.data, tmp.len}, width, pad, CASE_KEEP);
    if (casing == CASE_UPPER) {
        for (iz i = beg; i < t->len; i++) {
            t->data[i] = upcase(t->data[i]);
        }
    }
}


// The printf builtin

struct Printf {
    PrintfEnv *env;
    Buf       *out;
    Str       *args;
    iz         nargs;
    iz         next;     // next argument
    b32        converr;  // a conversion error was reported
};

// A conversion specification as bash scans it.
struct PfSpec {
    b32 alt;
    b32 group;
    b32 plus;
    b32 space;
    b32 left;
    b32 zero;
    b32 starw;    // width from an argument
    i32 width;    // ... its value
    Str wdigits;  // width digits in the format
    b32 dot;
    b32 starp;    // precision from an argument
    i32 prec;     // ... its value (bash's "precision", else 0)
    b32 negprec;  // a '-' after the dot
    Str pdigits;  // precision digits in the format
    b32 longform;
    u8  conv;
};

static void diag(Printf *p, Str a, Str b = {}, Str c = {}, Str d = {})
{
    if (!p->env->error) {
        return;
    }
    Arena t = *p->out->a;
    Buf   m(&t, 16+a.len+b.len+c.len+d.len);
    print(&m, "printf: ");
    print(&m, a);
    print(&m, b);
    print(&m, c);
    print(&m, d);
    pfflush(p->env);
    p->env->error(p->env->ctx, finish(&m));
}

static b32 hasarg(Printf *p)
{
    return p->next < p->nargs;
}

static Str getstr(Printf *p)
{
    return hasarg(p) ? p->args[p->next++] : Str{};
}

static u8 getchr(Printf *p)
{
    Str s = getstr(p);
    return s.len ? s[0] : 0;
}

// The character code after a leading quote.
static i64 asciicode(Printf *p)
{
    Str s = p->args[p->next++];
    if (s.len < 2) {
        return 0;
    }
    if (p->env->utf8) {
        Mbchar u = mbcharat(s, 1);
        if (u.len) {
            return u.c;
        }
    }
    return s[1];
}

static void chkconv(Printf *p, Str s, iz end, b32 range)
{
    if (!end || end<s.len) {
        Str msg = ": invalid number";
        if (byteat(s, 0)=='0' && digit(byteat(s, 1))) {
            msg = ": invalid octal number";
        } else if (byteat(s, 0)=='0' && byteat(s, 1)=='x') {
            msg = ": invalid hex number";
        }
        diag(p, s, msg);
        p->converr = 1;
    } else if (range) {
        diag(p, s, ": Numerical result out of range");
        p->converr = 1;
    }
}

static u64 getnum(Printf *p, b32 unsign)
{
    if (!hasarg(p)) {
        return 0;
    }
    Str s = p->args[p->next];
    if (byteat(s, 0)=='\'' || byteat(s, 0)=='"') {
        return (u64)asciicode(p);
    }
    PfNum n = scannum(s, unsign);
    chkconv(p, s, n.end, n.range);
    p->next++;
    return n.bits;
}

// A floating point argument, as strtold reads it.
static F80 getfloat(Printf *p)
{
    if (!hasarg(p)) {
        return {};
    }
    Str s = p->args[p->next];
    if (byteat(s, 0)=='\'' || byteat(s, 0)=='"') {
        return f80fromint(asciicode(p));
    }
    u8  radix = p->env->radix;
    Str t     = s;
    if (radix && radix!='.') {
        // As strtold: the locale's point, and '.' ends the number
        t = clone(p->out->a, s);
        for (iz i = 0; i < t.len; i++) {
            if (t[i] == '.') {
                t = takehead(t, i);
                break;
            } else if (t[i] == radix) {
                t.data[i] = '.';
            }
        }
    }
    F80Parse r = f80parse(t, *p->out->a);
    chkconv(p, s, r.end, r.range);
    p->next++;
    return r.v;
}

// A * width or precision, with fallback when out of int range.
static i32 getint(Printf *p, i32 fallback)
{
    if (!hasarg(p)) {
        return 0;
    }
    Str s = p->args[p->next];
    if (byteat(s, 0)=='\'' || byteat(s, 0)=='"') {
        return (i32)asciicode(p);
    }
    PfNum n    = scannum(s, 0);
    i64   v    = (i64)n.bits;
    b32   over = n.range || v<-0x7fffffffll-1 || v>0x7fffffffll;
    chkconv(p, s, n.end, over);
    p->next++;
    return over ? fallback : (i32)v;
}

// Width or precision digits as bash's printstr reads them: overflow
// past INT_MAX is reported and yields the fallback.
static i64 decodeint(Printf *p, Str d, i64 fallback)
{
    i64 v = 0;
    for (iz i = 0; i < d.len; i++) {
        v = v*10 + d[i] - '0';
        if (v > 0x7fffffff) {
            diag(p, d, ": Numerical result out of range");
            p->converr = 1;
            return fallback;
        }
    }
    return v;
}

static void padspaces(Buf *b, i64 n)
{
    for (; n > 0; n--) {
        putbyte(b, ' ');
    }
}

struct PfLayout {
    b32 ljust;
    i64 width;
    i64 prec;  // -1 if none
};

// Width and precision as bash's printstr reads them from the spec.
static PfLayout layout(Printf *p, PfSpec *sp)
{
    PfLayout r = {};
    r.ljust = sp->left;
    if (sp->starw) {
        r.width = sp->width;
        if (r.width < 0) {
            r.width = -r.width;
            r.ljust = 1;
        }
    } else if (sp->wdigits.len) {
        r.width = decodeint(p, sp->wdigits, 0);
    }

    r.prec = -1;
    if (sp->dot) {
        if (sp->starp) {
            r.prec = sp->prec;
        } else if (sp->pdigits.len && !sp->negprec) {
            r.prec = decodeint(p, sp->pdigits, -1);
            r.prec = r.prec<sp->prec ? sp->prec : r.prec;
        } else {
            r.prec = 0;
        }
    }
    return r;
}

// bash's printstr: %b, %q, %Q and %(...)T.
static void bstr(Printf *p, PfSpec *sp, Str s)
{
    PfLayout l   = layout(p, sp);
    iz       nc  = l.prec>=0 && l.prec<=s.len ? (iz)l.prec : s.len;
    i64      pad = l.width - nc;
    if (!l.ljust) padspaces(p->out, pad);
    print(p->out, takehead(s, nc));
    if (l.ljust) padspaces(p->out, pad);
}

// bash's printwidestr: %lc and %ls in a UTF-8 locale, counting width
// and precision in characters. A string that does not decode counts
// its bytes and prints only an ASCII prefix within the precision.
static void wstr(Printf *p, PfSpec *sp, Str s, b32 single)
{
    b32 valid = 1;
    iz  n     = 0;
    iz  i     = 0;
    for (; i<s.len && (!single || !n); n++) {
        Mbchar u = mbcharat(s, i);
        if (!u.len) {
            valid = 0;
            break;
        }
        i += u.len;
    }
    if (!valid) {
        n = single ? 1 : s.len;
    }

    PfLayout l   = layout(p, sp);
    iz       nc  = l.prec>=0 && l.prec<=n ? (iz)l.prec : n;
    i64      pad = l.width - nc;
    if (!l.ljust) padspaces(p->out, pad);

    iz take = l.prec>=0 && l.prec<n ? (iz)l.prec : n;  // characters converted
    if (valid) {
        iz j = 0;
        for (iz k = 0; k < take; k++) {
            j += mbcharat(s, j).len;
        }
        print(p->out, takehead(s, j));
    } else if (l.prec >= 0) {
        b32 ascii = 1;
        for (iz k = 0; k < take; k++) {
            ascii &= s[k] < 0x80;
        }
        if (ascii) print(p->out, takehead(s, take));
    }

    if (l.ljust) padspaces(p->out, pad);
}

// How glibc's printf reads a spec that bash rebuilt.
struct PfLibc {
    b32 overflow;  // a width or precision past INT_MAX
    b32 unknown;   // "%.-": glibc echoes the spec back
    b32 alt;
    b32 group;
    b32 plus;
    b32 space;
    b32 left;
    b32 zero;
    i64 width;
    i64 prec;      // -1 if none
};

static i64 libcint(Str d)  // -1 past INT_MAX
{
    i64 v = 0;
    for (iz i = 0; i < d.len; i++) {
        v = v*10 + d[i] - '0';
        if (v > 0x7fffffff) {
            return -1;
        }
    }
    return v;
}

static PfLibc cspec(PfSpec *sp)
{
    PfLibc r = {};
    r.alt   = sp->alt;
    r.group = sp->group;
    r.plus  = sp->plus;
    r.space = sp->space;
    r.left  = sp->left;
    r.zero  = sp->zero && !sp->left;

    if (sp->starw) {
        r.width = sp->width;
        if (r.width < 0) {
            r.width = -r.width;
            r.left  = 1;  // but the 0 flag survives, as printf_unknown shows
        }
    } else if (sp->wdigits.len) {
        r.width    = libcint(sp->wdigits);
        r.overflow = r.width < 0;
    }

    r.prec = -1;
    if (r.overflow || !sp->dot) {
        return r;
    } else if (sp->starp) {
        r.prec = sp->prec<0 ? -1 : sp->prec;
    } else if (sp->negprec) {
        r.unknown = 1;
        r.prec    = 0;
    } else if (sp->pdigits.len) {
        r.prec     = libcint(sp->pdigits);
        r.overflow = r.prec < 0;
    } else {
        r.prec = 0;
    }
    return r;
}

static b32 isintconv(u8 c)
{
    return c=='d' || c=='i' || c=='o' || c=='u' || c=='x' || c=='X';
}

static b32 isfloatconv(u8 c)
{
    switch (c) {
    case 'a': case 'A': case 'e': case 'E':
    case 'f': case 'F': case 'g': case 'G':
        return 1;
    }
    return 0;
}

// glibc's printf_unknown, then the rest of bash's rebuilt spec.
static void cunknown(Buf *b, PfLibc *cs, PfSpec *sp)
{
    putbyte(b, '%');
    if (cs->alt)   putbyte(b, '#');
    if (cs->group) putbyte(b, '\'');
    if (cs->plus) {
        putbyte(b, '+');
    } else if (cs->space) {
        putbyte(b, ' ');
    }
    if (cs->left)  putbyte(b, '-');
    if (cs->zero)  putbyte(b, '0');
    if (cs->width) print(b, cs->width);
    print(b, ".0-");
    print(b, sp->pdigits);
    if (isintconv(sp->conv)) putbyte(b, 'l');
    if (isfloatconv(sp->conv)) putbyte(b, 'L');
    putbyte(b, sp->conv);
}

// Pad a C library conversion's text to its width.
static void cpad(Buf *b, PfLibc *cs, Str s)
{
    i64 pad = cs->width - s.len;
    if (!cs->left) padspaces(b, pad);
    print(b, s);
    if (cs->left) padspaces(b, pad);
}

// glibc's %d %i %o %u %x %X on a 64-bit value.
static void cint(Buf *b, PfLibc *cs, u8 conv, u64 v)
{
    Str prefix = "";
    u32 base   = 10;
    switch (conv) {
    case 'd':
    case 'i':
        if ((i64)v < 0) {
            prefix = "-";
            v = -v;
        } else if (cs->plus) {
            prefix = "+";
        } else if (cs->space) {
            prefix = " ";
        }
        break;
    case 'o':
        base = 8;
        break;
    case 'x':
    case 'X':
        base = 16;
        if (cs->alt && v) {
            prefix = conv=='x' ? Str("0x") : Str("0X");
        }
        break;
    }

    u8  tmp[32];
    u8 *end = tmp + countof(tmp);
    u8 *beg = end;
    Str hex = conv=='X' ? Str("0123456789ABCDEF") : Str("0123456789abcdef");
    if (v || cs->prec) {
        do {
            *--beg = hex[(iz)(v%base)];
        } while (v /= base);
    }
    iz  ndigits = end - beg;
    i64 zeros   = cs->prec>ndigits ? cs->prec-ndigits : 0;
    if (conv=='o' && cs->alt && !zeros && (!ndigits || *beg!='0')) {
        zeros = 1;
    }

    i64 len = prefix.len + zeros + ndigits;
    i64 pad = cs->width - len;
    if (cs->zero && !cs->left && cs->prec<0 && pad>0) {
        zeros += pad;
        pad    = 0;
    }
    if (!cs->left) padspaces(b, pad);
    print(b, prefix);
    for (; zeros > 0; zeros--) {
        putbyte(b, '0');
    }
    print(b, span(beg, end));
    if (cs->left) padspaces(b, pad);
}

static b32 validname(Str s)
{
    if (!s.len || digit(s[0])) {
        return 0;
    }
    for (iz i = 0; i < s.len; i++) {
        if (!alnum(s[i]) && s[i]!='_') {
            return 0;
        }
    }
    return 1;
}

// For each '(' in fmt, the index of the ')' that bash's depth-counting
// scan would stop at, or -1 if none. One linear pass instead of a scan
// per %(, which is quadratic on a format of unclosed "%(".
static void matchparens(iz *match, Str fmt)
{
    iz top = -1;  // pending '(' form a stack linked through match
    for (iz i = 0; i < fmt.len; i++) {
        if (fmt[i] == '(') {
            match[i] = top;
            top      = i;
        } else if (fmt[i]==')' && top>=0) {
            iz up      = match[top];
            match[top] = i;
            top        = up;
        }
    }
    while (top >= 0) {
        iz up      = match[top];
        match[top] = -1;
        top        = up;
    }
}

// Format args with fmt, reusing fmt while arguments remain, appending
// to out. Returns the exit status: 1 after a reported error. Invalid
// numbers are reported without stopping the output; other errors stop
// it, keeping what was already appended. Working memory comes from
// out's arena, ahead of out, which then grows by moving past it.
static i32 printfmt(PrintfEnv *env, Buf *out, Str fmt, Str *args, iz nargs)
{
    if (!fmt.len) {
        return 0;
    }

    Printf  pf = {};
    Printf *p  = &pf;
    p->env     = env;
    env->out   = out;
    p->out     = out;
    p->args    = args;
    p->nargs   = nargs;

    iz     *match = 0;  // see matchparens, built on the first %(
    PfZone *zone  = env->zone;  // else from env->tz, on the first %(...)T
    b32 stale = 1;  // match predates a rewrite of fmt
    b32 owned = 0;  // fmt is our own copy, safe to rewrite

    do {
        iz pass = out->len;  // %n counts from here
        if (match && stale) {
            matchparens(match, fmt);
            stale = 0;
        }
        for (iz i = 0; i < fmt.len; i++) {
            u8 c = fmt[i];
            if (c == '\\') {
                i += tescape(env, out, fmt, i+1, ESC_FORMAT, 0);
                continue;
            } else if (c != '%') {
                putbyte(out, c);
                continue;
            }

            iz start = i++;
            if (byteat(fmt, i) == '%') {
                putbyte(out, '%');
                continue;
            }

            PfSpec sp = {};
            for (;; i++) {
                switch (byteat(fmt, i)) {
                case '#':  sp.alt   = 1; continue;
                case '\'': sp.group = 1; continue;
                case '-':  sp.left  = 1; continue;
                case '+':  sp.plus  = 1; continue;
                case ' ':  sp.space = 1; continue;
                case '0':  sp.zero  = 1; continue;
                }
                break;
            }

            if (byteat(fmt, i) == '*') {
                i++;
                sp.starw = 1;
                sp.width = getint(p, 0);
            } else {
                iz beg = i;
                for (; digit(byteat(fmt, i)); i++) {}
                sp.wdigits = span(fmt.data+beg, fmt.data+i);
            }

            if (byteat(fmt, i) == '.') {
                i++;
                sp.dot = 1;
                if (byteat(fmt, i) == '*') {
                    i++;
                    sp.starp = 1;
                    sp.prec  = getint(p, -1);
                } else {
                    if (byteat(fmt, i) == '-') {
                        i++;
                        sp.negprec = 1;
                    }
                    iz beg = i;
                    for (; digit(byteat(fmt, i)); i++) {}
                    sp.pdigits = span(fmt.data+beg, fmt.data+i);
                }
            }

            iz mods = i;
            for (;; i++) {
                u8 m = byteat(fmt, i);
                if (m!='h' && m!='j' && m!='l' && m!='L' && m!='t' && m!='z') {
                    break;
                }
                sp.longform |= m == 'l';
            }

            if (i == fmt.len) {
                diag(p, "`", cuthead(fmt, start), "': missing format character");
                return 1;
            }
            sp.conv = fmt[i];

            PfLibc cs = {};
            switch (sp.conv) {
            case 'c':
            case 'C':
                if ((sp.longform || sp.conv=='C') && env->utf8) {
                    Str s = getstr(p);
                    if (!s.len) {
                        bstr(p, &sp, {(u8 *)"", 1});  // a null byte
                    } else {
                        wstr(p, &sp, s, 1);
                    }
                    break;
                }
                c  = getchr(p);
                cs = cspec(&sp);
                if (cs.overflow) {
                    goto overflow;
                } else if (cs.unknown) {
                    cunknown(out, &cs, &sp);
                } else if (sp.conv=='C' && c>=0x80) {
                    goto badwide;
                } else {
                    cpad(out, &cs, {&c, 1});
                }
                break;

            case 's':
            case 'S':
                if ((sp.longform || sp.conv=='S') && env->utf8) {
                    wstr(p, &sp, getstr(p), 0);
                    break;
                } else {
                    Str s = getstr(p);
                    cs = cspec(&sp);
                    if (cs.overflow) {
                        goto overflow;
                    } else if (cs.unknown) {
                        cunknown(out, &cs, &sp);
                    } else if (sp.conv=='S' && cs.prec) {
                        // glibc would read the bytes as wchar_t
                        goto badwide;
                    } else if (sp.conv == 'S') {
                        cpad(out, &cs, "");  // precision 0 reads nothing
                    } else {
                        if (cs.prec>=0 && cs.prec<s.len) {
                            s = takehead(s, (iz)cs.prec);
                        }
                        cpad(out, &cs, s);
                    }
                }
                break;

            case 'b': {
                // Diagnostics allocate past the expansion
                Str   s    = getstr(p);
                Buf   x(out->a, s.len+16);
                b32   stop = 0;
                bexpand(env, &x, s, &stop);
                bstr(p, &sp, finish(&x));
                if (stop) {
                    return 0;  // bash returns before folding in conversion errors
                }
            } break;

            case '(': {
                if (!match) {
                    match = alloc<iz>(out->a, fmt.len);
                    matchparens(match, fmt);
                    stale = 0;
                }
                iz beg = i + 1;
                iz j   = match[i]<0 ? fmt.len : match[i];
                if (byteat(fmt, j)!=')' || byteat(fmt, j+1)!='T') {
                    u8 w = byteat(fmt, j)==')' ? byteat(fmt, j+1) : 0;
                    diag(p, "warning: `", {&w, 1},
                         "': invalid time format specification");
                    putbyte(out, '%');
                    if (mods < i) {
                        // bash rewrote the spec in place to strip the
                        // length modifiers, then bailed before undoing
                        // it: the first modifier is now '(' for good.
                        if (!owned) {
                            fmt   = clone(out->a, fmt);
                            owned = 1;
                        }
                        fmt[mods] = '(';
                        stale     = 1;  // later '(' are unaffected
                    }
                    i = start;
                    continue;
                }
                Str tf = span(fmt.data+beg, fmt.data+j);
                i = j + 1;
                if (!tf.len) {
                    tf = "%X";
                }
                i64 secs = hasarg(p) ? (i64)getnum(p, 0) : -1;
                if (secs == -1) {
                    secs = env->now ? env->now(env->ctx) : 0;
                } else if (secs == -2) {
                    secs = env->shellstart;
                }
                if (!zone) {
                    zone  = alloc<PfZone>(out->a);
                    *zone = parsezone(env->tz.data ? env->tz : Str("UTC"));
                }
                PfTm tm = localtm(secs, zone);
                if (!tm.ok) {
                    tm = localtm(0, zone);
                }
                PfTout t = {};
                fmttime(&t, tf, &tm);
                bstr(p, &sp, t.full ? Str{} : Str{t.data, t.len});
            } break;

            case 'n': {
                Str name = getstr(p);
                if (!name.len) {
                    break;
                } else if (!validname(name)) {
                    diag(p, "`", name, "': not a valid identifier");
                    return 1;
                }
                if (env->setint) {
                    env->setint(env->ctx, name, out->len - pass);
                }
            } break;

            case 'q':
            case 'Q': {
                Str s = getstr(p);
                if (sp.conv=='Q' && (sp.starp || sp.pdigits.len)) {
                    i64 prec = sp.prec;
                    if (sp.pdigits.len) {
                        prec = libcint(sp.pdigits);
                    }
                    if (prec>=0 && prec<s.len) {
                        s = takehead(s, (iz)prec);
                    }
                }
                Buf q(out->a, 2*s.len+3);
                quote(&q, s, env->utf8, sp.alt);
                if (sp.conv=='Q' && q.len>0x7fffffff) {
                    diag(p, "%Q: string length Numerical result out of range");
                    sp.prec = -1;
                } else if (sp.conv == 'Q') {
                    sp.prec = (i32)q.len;  // precision applied before quoting
                }
                bstr(p, &sp, finish(&q));
            } break;

            case 'd':
            case 'i':
            case 'o':
            case 'u':
            case 'x':
            case 'X': {
                b32 unsign = sp.conv!='d' && sp.conv!='i';
                u64 v      = getnum(p, unsign);
                cs = cspec(&sp);
                if (cs.overflow) {
                    goto overflow;
                } else if (cs.unknown) {
                    cunknown(out, &cs, &sp);
                } else {
                    cint(out, &cs, sp.conv, v);
                }
            } break;

            case 'a': case 'A':
            case 'e': case 'E':
            case 'f': case 'F':
            case 'g': case 'G': {
                F80 v = getfloat(p);
                cs = cspec(&sp);
                if (cs.overflow) {
                    goto overflow;
                } else if (cs.unknown) {
                    cunknown(out, &cs, &sp);
                } else {
                    F80Spec fs = {};
                    fs.alt   = cs.alt;
                    fs.plus  = cs.plus;
                    fs.space = cs.space;
                    fs.left  = cs.left;
                    fs.zero  = cs.zero;
                    fs.width = cs.width;
                    fs.prec  = cs.prec;
                    reserve(out, f80bound(&fs));  // working memory beyond
                    iz start = out->len;
                    f80format(out, v, sp.conv, &fs, *out->a);
                    u8 radix = p->env->radix;
                    for (iz i = start; radix && i<out->len; i++) {
                        if (out->data[i] == '.') {
                            out->data[i] = radix;
                            break;
                        }
                    }
                }
            } break;

            default:
                diag(p, "`", {&sp.conv, 1}, "': invalid format character");
                return 1;
            }
            continue;

        overflow:
            diag(p, "Value too large for defined data type");
            return 1;

        badwide:
            diag(p, "Invalid or incomplete multibyte or wide character");
            return 1;
        }
    } while (p->next<p->nargs && p->next);

    return p->converr;
}
