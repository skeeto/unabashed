// Fake programs for the virtual OS and the oracle container
//
// Each program is written against the platform API only, so the same
// source runs in-process on the vos (vos.cpp) and as a Linux multi-call
// binary in the oracle image (main_fakes.cpp), which keeps recorded
// specs and vos runs in agreement. Behavior follows the real tools for
// the options implemented.
//
// This is free and unencumbered software released into the public domain.

// What a fake program sees: its arguments, environment, working
// directory and descriptors.
struct FakeCtx {
    Str    *argv;
    iz      argc;
    Str    *env;
    iz      nenv;
    Str     cwd;
    Handle  fd[64];  // BADHANDLE when closed; procsubs use up to 63
    Arena   scratch;
    // The vos's Windows mode (Vos.win): the raw command line, the
    // image's own native path, and that drive paths are absolute
    Str     cmdline;
    Str     exe;
    b32     win;
};

struct Fake {
    Str  name;
    i32 (*main)(Os *, FakeCtx *);
    b32  win = 0;  // a Windows program, only in the vos's Windows tree
};


// Platform extensions
//
// The fakes change the file system in ways the shell itself never does,
// so these calls live here rather than in api.cpp. vos.cpp and
// main_fakes.cpp implement them. Paths are absolute. Each call returns
// 0 or an E_* code.

struct OsIds {
    i64 uid;
    i64 gid;
    i64 euid;
    i64 egid;
};

enum : i32 { UN_SYSNAME, UN_NODENAME, UN_RELEASE, UN_VERSION, UN_MACHINE };

// Accumulate a decimal digit, saturating at max so that long digit
// strings cannot overflow.
static i64 fkaccum(i64 n, u8 c, i64 max = (i64)1<<40)
{
    n = n*10 + (c - '0');
    return n<max ? n : max;
}

static i32   os_mkdir(Os *, Arena, Str path, i32 mode);   // mode less umask
static i32   os_rmdir(Os *, Arena, Str path);
static i32   os_unlink(Os *, Arena, Str path);            // not directories
static i32   os_rename(Os *, Arena, Str from, Str to);
static i32   os_link(Os *, Arena, Str from, Str to);      // hard link
static i32   os_symlink(Os *, Arena, Str target, Str path);
static i32   os_chmod(Os *, Arena, Str path, i32 mode);   // follows links
static i32   os_utime(Os *, Arena, Str path, i64 mtime);  // ns, also atime
static i32   os_mkfifo(Os *, Arena, Str path, i32 mode);  // mode less umask
static Str   os_readlink(Os *, Arena *, Str path);        // null data on error
static i32   os_umask(Os *);
static OsIds os_ids(Os *);
static Str   os_uname(Os *, Arena *, i32 field);          // UN_*
static void  os_sleep(Os *, i64 ns);


// Characters in a UTF-8 locale
//
// Most of the programs work on bytes in every locale, as the real ones
// do. Those that are multibyte-aware in glibc's C.UTF-8 (diagnostics
// quoting, wc, expr, and grep, sed and expr regular expressions) use
// these approximations of its character classes and widths.

struct FkChar {
    i32 c;    // code point, or -1 for an invalid byte
    i32 len;  // bytes consumed, at least 1
};

// Decode one character at s[i] as glibc's mbrtowc does: overlong forms,
// surrogates and code points past 0x10ffff are invalid.
static FkChar fkchar(Str s, iz i)
{
    FkChar r = {-1, 1};
    u8     c = s[i];
    if (c < 0x80) {
        r.c = c;
        return r;
    }
    i32 n   = c>=0xf0 ? 3 : c>=0xe0 ? 2 : c>=0xc0 ? 1 : 0;
    i32 min = n==3 ? 0x10000 : n==2 ? 0x800 : 0x80;
    if (!n || c>0xf4 || n>s.len-i-1) return r;
    i32 v = c & (0x3f >> n);
    for (i32 k = 1; k <= n; k++) {
        u8 b = s[i+k];
        if ((b & 0xc0) != 0x80) return r;
        v = v<<6 | (b & 0x3f);
    }
    if (v<min || v>0x10ffff || (v>=0xd800 && v<=0xdfff)) return r;
    r.c   = v;
    r.len = n + 1;
    return r;
}

// The character ending just before s[i], -1 at the start.
static i32 fkcharprev(Str s, iz i)
{
    for (iz j = i-1; j>=0 && j>=i-4; j--) {
        if ((s[j] & 0xc0) != 0x80) {
            FkChar r = fkchar(s, j);
            return j+r.len==i ? r.c : -1;
        }
    }
    return -1;
}

static iz fkcharcount(Str s)  // invalid bytes count one each, as mbslen
{
    iz n = 0;
    for (iz i = 0; i < s.len; i += fkchar(s, i).len) n++;
    return n;
}

struct FkCase {
    i32 lo;    // first upper case letter
    i32 hi;    // last
    i32 step;  // 1 for every code point, 2 for alternating pairs
    i32 delta; // lower minus upper
};

// Simple case mappings for the common alphabets.
static FkCase fkcases[] = {
    {0x0041, 0x005a, 1,  32}, {0x00c0, 0x00d6, 1,  32}, {0x00d8, 0x00de, 1,  32},
    {0x0100, 0x012e, 2,   1}, {0x0132, 0x0136, 2,   1}, {0x0139, 0x0147, 2,   1},
    {0x014a, 0x0176, 2,   1}, {0x0178, 0x0178, 1,-121}, {0x0179, 0x017d, 2,   1},
    {0x0386, 0x0386, 1,  38}, {0x0388, 0x038a, 1,  37}, {0x038c, 0x038c, 1,  64},
    {0x038e, 0x038f, 1,  63}, {0x0391, 0x03a1, 1,  32}, {0x03a3, 0x03ab, 1,  32},
    {0x0400, 0x040f, 1,  80}, {0x0410, 0x042f, 1,  32}, {0x0460, 0x0480, 2,   1},
    {0x048a, 0x04be, 2,   1}, {0x0531, 0x0556, 1,  48}, {0x1e00, 0x1e94, 2,   1},
    {0x1ea0, 0x1efe, 2,   1}, {0xff21, 0xff3a, 1,  32},
};

static i32 fklower(i32 c)
{
    if (c < 0x80) return c>='A' && c<='Z' ? c+32 : c;
    for (iz i = 0; i < countof(fkcases); i++) {
        FkCase *k = fkcases + i;
        if (c>=k->lo && c<=k->hi && (c-k->lo)%k->step==0) return c + k->delta;
    }
    return c;
}

static i32 fkupper(i32 c)
{
    if (c < 0x80) return c>='a' && c<='z' ? c-32 : c;
    if (c == 0x00ff) return 0x0178;
    if (c == 0x03c2) return 0x03a3;  // final sigma
    for (iz i = 0; i < countof(fkcases); i++) {
        FkCase *k = fkcases + i;
        i32 u = c - k->delta;
        if (k->delta!=-121 && u>=k->lo && u<=k->hi && (u-k->lo)%k->step==0) {
            return u;
        }
    }
    return c;
}

static b32 fkwspace(i32 c)
{
    return c==' ' || (c>=0x09 && c<=0x0d) || c==0x1680 || (c>=0x2000 && c<=0x2006)
        || (c>=0x2008 && c<=0x200a) || c==0x2028 || c==0x2029 || c==0x205f
        || c==0x3000;
}

static b32 fkwprint(i32 c)
{
    if (c < 0x80) return c>=0x20 && c<0x7f;
    return c>=0xa0 && c!=0x2028 && c!=0x2029 && !(c>=0xfff9 && c<=0xfffb)
        && (c&0xfffe)!=0xfffe && !(c>=0xd800 && c<=0xdfff);
}

static b32 fkwalpha(i32 c)
{
    if (c < 0x80) return letter((u8)c);
    return c==0xaa || c==0xb5 || c==0xba || (c>=0xc0 && c<=0x2ff && c!=0xd7 && c!=0xf7)
        || (c>=0x370 && c<=0x3ff && c!=0x375 && c!=0x37e && c!=0x384 && c!=0x385
            && c!=0x387)
        || (c>=0x400 && c<=0x52f && !(c>=0x482 && c<=0x489))
        || (c>=0x531 && c<=0x556) || (c>=0x561 && c<=0x587)
        || (c>=0x5d0 && c<=0x5ea) || (c>=0x620 && c<=0x64a)
        || (c>=0x1e00 && c<=0x1fff) || (c>=0x3041 && c<=0x30ff && c!=0x30fb)
        || (c>=0x3400 && c<=0x4dbf) || (c>=0x4e00 && c<=0x9fff)
        || (c>=0xac00 && c<=0xd7a3) || (c>=0xf900 && c<=0xfaff)
        || (c>=0xff21 && c<=0xff3a) || (c>=0xff41 && c<=0xff5a);
}

// Display width as wcwidth gives it: -1 when unprintable.
static i32 fkwidth(i32 c)
{
    if (!fkwprint(c)) return -1;
    if (c < 0x300) return 1;
    if ((c>=0x300 && c<=0x36f) || (c>=0x483 && c<=0x489) || (c>=0x591 && c<=0x5bd)
            || (c>=0x610 && c<=0x61a) || (c>=0x64b && c<=0x65f)
            || (c>=0x1160 && c<=0x11ff) || (c>=0x200b && c<=0x200f)
            || (c>=0x20d0 && c<=0x20ff) || (c>=0xfe00 && c<=0xfe0f)
            || (c>=0xfe20 && c<=0xfe2f)) {
        return 0;
    }
    if ((c>=0x1100 && c<=0x115f) || (c>=0x2e80 && c<=0x303e) || (c>=0x3041 && c<=0x33ff)
            || (c>=0x3400 && c<=0x4dbf) || (c>=0x4e00 && c<=0x9fff)
            || (c>=0xa000 && c<=0xa4cf) || (c>=0xac00 && c<=0xd7a3)
            || (c>=0xf900 && c<=0xfaff) || (c>=0xfe30 && c<=0xfe4f)
            || (c>=0xff00 && c<=0xff60) || (c>=0xffe0 && c<=0xffe6)
            || (c>=0x1f300 && c<=0x1f64f) || (c>=0x1f900 && c<=0x1f9ff)
            || (c>=0x20000 && c<=0x3fffd)) {
        return 2;
    }
    return 1;
}

enum : i32 {  // character classes, as [:NAME:] spells them
    FC_ALPHA, FC_UPPER, FC_LOWER, FC_DIGIT, FC_XDIGIT, FC_ALNUM, FC_PUNCT,
    FC_SPACE, FC_BLANK, FC_PRINT, FC_GRAPH, FC_CNTRL, FC_WORD,
};

static Str fkclassnames[] = {
    "alpha", "upper", "lower", "digit", "xdigit", "alnum", "punct",
    "space", "blank", "print", "graph", "cntrl",
};

static i32 fkclassid(Str name)  // -1 if unknown
{
    for (iz i = 0; i < countof(fkclassnames); i++) {
        if (name == fkclassnames[i]) return (i32)i;
    }
    return -1;
}

// Whether a character is in a class, as the isw* functions say.
static b32 fkwclass(i32 cls, i32 c)
{
    switch (cls) {
    case FC_ALPHA:  return fkwalpha(c);
    case FC_UPPER:  return fklower(c) != c;
    case FC_LOWER:  return fkupper(c)!=c || c==0xdf;
    case FC_DIGIT:  return c>='0' && c<='9';
    case FC_XDIGIT: return (c>='0' && c<='9') || ((c|32)>='a' && (c|32)<='f');
    case FC_ALNUM:  return fkwalpha(c) || (c>='0' && c<='9');
    case FC_PUNCT:  return fkwprint(c) && !fkwspace(c) && !fkwalpha(c)
                        && !(c>='0' && c<='9');
    case FC_SPACE:  return fkwspace(c);
    case FC_BLANK:  return c==' ' || c=='\t' || (fkwspace(c) && c>0x7f && c!=0x2028
                                                  && c!=0x2029);
    case FC_PRINT:  return fkwprint(c);
    case FC_GRAPH:  return fkwprint(c) && !fkwspace(c);
    case FC_CNTRL:  return c<0x20 || (c>=0x7f && c<0xa0) || c==0x2028 || c==0x2029;
    case FC_WORD:   return fkwalpha(c) || (c>='0' && c<='9') || c=='_';
    }
    return 0;
}


// Buffered output

struct FkOut {
    Os    *os;
    Handle h;
    u8    *buf;
    iz     len;
    iz     cap;
    b32    err;
    b32    utf8;  // quote for a UTF-8 locale
};

static FkOut fkout(Os *os, Arena *a, Handle h, iz cap)
{
    FkOut o = {};
    o.os  = os;
    o.h   = h;
    o.buf = allocbytes(a, cap);
    o.cap = cap;
    o.err = h == BADHANDLE;
    return o;
}

static void flush(FkOut *o)
{
    if (!o->err && o->len) {
        o->err = !os_write(o->os, o->h, o->buf, o->len);
    }
    o->len = 0;
}

static void print(FkOut *o, Str s)
{
    if (s.len > o->cap - o->len) {
        flush(o);
        if (s.len >= o->cap) {
            if (!o->err) o->err = !os_write(o->os, o->h, s.data, s.len);
            return;
        }
    }
    copybytes(o->buf+o->len, s.data, s.len);
    o->len += s.len;
}

static void putbyte(FkOut *o, u8 c)
{
    if (o->len == o->cap) flush(o);
    o->buf[o->len++] = c;
}

static Str fkfmtint(u8 *end, i64 v)  // digits ending at end
{
    u8 *beg = end;
    i64 t   = v<0 ? v : -v;
    do {
        *--beg = (u8)('0' - t%10);
    } while (t /= 10);
    if (v < 0) *--beg = '-';
    return span(beg, end);
}

static void print(FkOut *o, i64 v)
{
    u8 tmp[24];
    print(o, fkfmtint(tmp+countof(tmp), v));
}

// Right-align in width with the pad byte, as printf %*d or %0*d.
static void printpad(FkOut *o, i64 v, i32 width, u8 pad)
{
    u8  tmp[24];
    Str s = fkfmtint(tmp+countof(tmp), v);
    if (pad=='0' && v<0) {
        putbyte(o, '-');
        s = cuthead(s, 1);
        width--;
    }
    for (iz i = s.len; i < width; i++) putbyte(o, pad);
    print(o, s);
}

static void printoct(FkOut *o, u64 v, i32 digits)
{
    for (i32 i = digits-1; i >= 0; i--) {
        putbyte(o, (u8)('0' + (v>>(3*i) & 7)));
    }
}


// Program context and diagnostics

struct FkProg {
    Os      *os;
    FakeCtx *ctx;
    Arena   *a;
    Str      name;   // argv[0], as diagnostics show it
    FkOut    out;
    FkOut    err;
    b32      utf8;   // the locale's character set is UTF-8
};

static FkProg fkprog(Os *os, FakeCtx *ctx)
{
    FkProg p = {};
    p.os   = os;
    p.ctx  = ctx;
    p.a    = &ctx->scratch;
    p.name = ctx->argc ? ctx->argv[0] : Str("?");
    p.out  = fkout(os, p.a, ctx->fd[1], 1<<12);
    p.err  = fkout(os, p.a, ctx->fd[2], 1<<10);

    // LC_ALL, else LC_CTYPE, else LANG names the character set
    Str vars[] = {"LC_ALL=", "LC_CTYPE=", "LANG="};
    for (iz v = 0; v < countof(vars); v++) {
        Str val = {};
        for (iz i = 0; i < ctx->nenv; i++) {
            if (startswith(ctx->env[i], vars[v])) val = cuthead(ctx->env[i], vars[v].len);
        }
        if (!val.len) continue;
        Str cs = cut(cut(val, '.').tail, '@').head;
        Str lc = clone(p.a, cs);
        for (iz i = 0; i < lc.len; i++) lc.data[i] = lowercase(lc[i]);
        p.utf8 = cut(val, '.').head=="C" && (lc=="utf-8" || lc=="utf8");
        break;  // the image installs no other UTF-8 locale
    }
    p.out.utf8 = p.err.utf8 = p.utf8;
    return p;
}

static Str fkerrtext(i32 e)  // glibc's strerror text
{
    switch (e) {
    case E_PERM:        return "Operation not permitted";
    case E_NOENT:       return "No such file or directory";
    case E_SRCH:        return "No such process";
    case E_INTR:        return "Interrupted system call";
    case E_IO:          return "Input/output error";
    case E_NXIO:        return "No such device or address";
    case E_2BIG:        return "Argument list too long";
    case E_NOEXEC:      return "Exec format error";
    case E_BADF:        return "Bad file descriptor";
    case E_CHILD:       return "No child processes";
    case E_AGAIN:       return "Resource temporarily unavailable";
    case E_NOMEM:       return "Cannot allocate memory";
    case E_ACCES:       return "Permission denied";
    case E_FAULT:       return "Bad address";
    case E_BUSY:        return "Device or resource busy";
    case E_EXIST:       return "File exists";
    case E_XDEV:        return "Invalid cross-device link";
    case E_NODEV:       return "No such device";
    case E_NOTDIR:      return "Not a directory";
    case E_ISDIR:       return "Is a directory";
    case E_INVAL:       return "Invalid argument";
    case E_NFILE:       return "Too many open files in system";
    case E_MFILE:       return "Too many open files";
    case E_NOTTY:       return "Inappropriate ioctl for device";
    case E_TXTBSY:      return "Text file busy";
    case E_FBIG:        return "File too large";
    case E_NOSPC:       return "No space left on device";
    case E_SPIPE:       return "Illegal seek";
    case E_ROFS:        return "Read-only file system";
    case E_MLINK:       return "Too many links";
    case E_PIPE:        return "Broken pipe";
    case E_DOM:         return "Numerical argument out of domain";
    case E_RANGE:       return "Numerical result out of range";
    case E_NAMETOOLONG: return "File name too long";
    case E_NOSYS:       return "Function not implemented";
    case E_NOTEMPTY:    return "Directory not empty";
    case E_LOOP:        return "Too many levels of symbolic links";
    }
    return {};
}

static void printerr(FkOut *o, i32 e)
{
    Str s = fkerrtext(e);
    if (s.data) {
        print(o, s);
    } else {
        print(o, "Unknown error ");
        print(o, (i64)e);
    }
}

// Start a diagnostic line, "NAME: ".
static FkOut *diag(FkProg *p)
{
    print(&p->err, p->name);
    print(&p->err, ": ");
    return &p->err;
}

static void enddiag(FkProg *p)
{
    putbyte(&p->err, '\n');
    flush(&p->err);
}

// Finish ": strerror" and the line.
static void enddiag(FkProg *p, i32 e)
{
    print(&p->err, ": ");
    printerr(&p->err, e);
    enddiag(p);
}

// Flush standard output and produce the exit status. A failed write
// to a pipe stands for death by SIGPIPE, as the vos has no signals.
static i32 fkexit(FkProg *p, i32 status)
{
    flush(&p->out);
    flush(&p->err);
    if (p->out.err) {
        Stat st = os_fstat(p->os, p->out.h);
        if (p->out.h==BADHANDLE || st.err) {
            print(diag(p), "write error");
            enddiag(p, E_BADF);
            return 1;
        } else if (st.type == FT_PIPE) {
            return 128 + 13;
        }
        print(diag(p), "write error");
        enddiag(p, E_NOSPC);
        return 1;
    }
    return status;
}

// diffutils' form: "NAME: Try 'NAME --help' for more information."
static i32 fktry(FkProg *p, i32 status)
{
    print(diag(p), "Try '");
    print(&p->err, p->name);
    print(&p->err, " --help' for more information.");
    enddiag(p);
    return fkexit(p, status);
}

static i32 fkusage(FkProg *p, i32 status)
{
    print(&p->err, "Try '");
    print(&p->err, p->name);
    print(&p->err, " --help' for more information.\n");
    flush(&p->err);
    return fkexit(p, status);
}



// Quoting, as coreutils diagnostics do it

static b32 fkctl(u8 c)
{
    return c<0x20 || c>=0x7f;
}

// The character at s[i] and whether it must be escaped: unprintable
// bytes in the C locale, and invalid or unprintable characters in UTF-8.
static FkChar fkqchar(FkOut *o, Str s, iz i, b32 *ctl)
{
    if (!o->utf8 || s[i]<0x80) {
        *ctl = fkctl(s[i]);
        return {s[i], 1};
    }
    FkChar r = fkchar(s, i);
    *ctl = r.c<0 || !fkwprint(r.c);
    return r;
}

static void fkcescape(FkOut *o, u8 c)
{
    switch (c) {
    case '\a': print(o, "\\a"); return;
    case '\b': print(o, "\\b"); return;
    case '\f': print(o, "\\f"); return;
    case '\n': print(o, "\\n"); return;
    case '\r': print(o, "\\r"); return;
    case '\t': print(o, "\\t"); return;
    case '\v': print(o, "\\v"); return;
    }
    putbyte(o, '\\');
    printoct(o, c, 3);
}

// Shell-escape quoting of a file name: quoted only when needed (quotef)
// unless always (quoteaf).
static void fkquotename(FkOut *o, Str s, b32 always)
{
    b32 special = always || !s.len;
    b32 squote  = 0;
    b32 compat  = 1;  // could be written in double quotes instead
    for (iz i = 0; i < s.len;) {
        b32    ctl;
        FkChar ch = fkqchar(o, s, i, &ctl);
        i += ch.len;
        if (ctl) {
            special = 1;
            compat  = 0;
            continue;
        }
        switch (ch.c) {
        case '{': case '}':
            special |= s.len == 1;
            break;
        case '#': case '~':
            special |= i == 1;
            break;
        case ' ': case '"': case '&': case '(': case ')': case '*':
        case ';': case '<': case '=': case '>': case '?': case '[':
        case '^': case '|': case ':':
            special = 1;
            break;
        case '!': case '$': case '\\': case '`':
            special = 1;
            compat  = 0;
            break;
        case '\'':
            special = squote = 1;
            break;
        }
    }
    if (!special) {
        print(o, s);
    } else if (squote && compat) {
        putbyte(o, '"');
        for (iz i = 0; i < s.len; i++) {
            if (s[i] == '"') putbyte(o, '\\');
            putbyte(o, s[i]);
        }
        putbyte(o, '"');
    } else {
        b32 dollar = 0;
        putbyte(o, '\'');
        for (iz i = 0; i < s.len;) {
            b32    ctl;
            FkChar ch = fkqchar(o, s, i, &ctl);
            if (ctl) {
                if (!dollar) print(o, "'$'");
                dollar = 1;
                for (iz k = 0; k < ch.len; k++) fkcescape(o, s[i+k]);
            } else {
                if (dollar) print(o, "''");
                dollar = 0;
                if (ch.c == '\'') {
                    print(o, "'\\''");
                } else {
                    print(o, {s.data+i, ch.len});
                }
            }
            i += ch.len;
        }
        putbyte(o, '\'');
    }
}

static void quotef(FkOut *o, Str s)  { fkquotename(o, s, 0); }
static void quoteaf(FkOut *o, Str s) { fkquotename(o, s, 1); }

// Locale-style quoting for values in messages: 'x' with C escapes, or
// in a UTF-8 locale, curly quotes.
static void quote(FkOut *o, Str s)
{
    print(o, o->utf8 ? Str("\xe2\x80\x98") : Str("'"));
    for (iz i = 0; i < s.len;) {
        b32    ctl;
        FkChar ch = fkqchar(o, s, i, &ctl);
        if (ctl) {
            for (iz k = 0; k < ch.len; k++) fkcescape(o, s[i+k]);
        } else if (ch.c=='\\' || (ch.c=='\'' && !o->utf8)) {
            putbyte(o, '\\');
            putbyte(o, (u8)ch.c);
        } else {
            print(o, {s.data+i, ch.len});
        }
        i += ch.len;
    }
    print(o, o->utf8 ? Str("\xe2\x80\x99") : Str("'"));
}



// Option parsing in the manner of GNU getopt

struct FkOpt {
    FkProg *p;
    Str    *args;
    iz      nargs;
    iz      ind;
    iz      pos;      // offset in a cluster of options, or 0
    Str     arg;      // argument of the option just returned
    b32     permute;  // operands may precede options
    Str    *rest;     // operands
    iz      nrest;
};

static FkOpt fkopts(FkProg *p, b32 permute)
{
    FkOpt o = {};
    o.p       = p;
    o.args    = p->ctx->argv;
    o.nargs   = p->ctx->argc;
    o.ind     = 1;
    o.permute = permute;
    o.rest    = alloc<Str>(p->a, o.nargs);
    return o;
}

static void fkoptrest(FkOpt *o)
{
    for (; o->ind < o->nargs; o->ind++) {
        o->rest[o->nrest++] = o->args[o->ind];
    }
}

// Next option letter from spec ("ab:c::"), -1 when options end, or '?'
// after a diagnostic. Operands collect in rest.
static i32 fkgetopt(FkOpt *o, Str spec)
{
    FkProg *p = o->p;
    if (!o->pos) {
        for (;;) {
            if (o->ind >= o->nargs) return -1;
            Str a = o->args[o->ind];
            if (a == "--") {
                o->ind++;
                fkoptrest(o);
                return -1;
            }
            if (a.len>=2 && a[0]=='-') break;
            if (!o->permute) {
                fkoptrest(o);
                return -1;
            }
            o->rest[o->nrest++] = a;
            o->ind++;
        }
        Str a = o->args[o->ind];
        if (a[1] == '-') {
            print(diag(p), "unrecognized option '");
            print(&p->err, a);
            putbyte(&p->err, '\'');
            enddiag(p);
            o->ind++;
            return '?';
        }
        o->pos = 1;
    }

    Str a = o->args[o->ind];
    u8  c = a[o->pos++];
    iz  i = 0;
    for (; i<spec.len && (spec[i]!=c || c==':'); i++) {}
    b32 end = o->pos == a.len;
    if (i == spec.len) {
        print(diag(p), "invalid option -- '");
        putbyte(&p->err, c);
        putbyte(&p->err, '\'');
        enddiag(p);
        if (end) {
            o->ind++;
            o->pos = 0;
        }
        return '?';
    }

    o->arg = {};
    if (i+1<spec.len && spec[i+1]==':') {
        b32 optional = i+2<spec.len && spec[i+2]==':';
        if (!end) {
            o->arg = cuthead(a, o->pos);
        } else if (!optional) {
            if (o->ind+1 >= o->nargs) {
                print(diag(p), "option requires an argument -- '");
                putbyte(&p->err, c);
                putbyte(&p->err, '\'');
                enddiag(p);
                o->ind++;
                o->pos = 0;
                return '?';
            }
            o->arg = o->args[++o->ind];
        }
        o->ind++;
        o->pos = 0;
    } else if (end) {
        o->ind++;
        o->pos = 0;
    }
    return c;
}


// Files

// Linux's PATH_MAX. The vos has no such limit, so the recursive walks
// enforce it, which also bounds their recursion on deep trees.
enum { FK_PATHMAX = 4096 };

// Absolute form of a file name, relative to the working directory.
static Str fkpath(FkProg *p, Str name)
{
    if (!name.len || name[0]=='/') return name;
    if (p->ctx->win && (name[0]=='\\' ||(name.len>1 && name[1]==':'))) {
        return name;
    }
    Str cwd = p->ctx->cwd;
    b32 sep = !cwd.len || cwd[cwd.len-1]!='/';
    Str r   = {allocbytes(p->a, cwd.len+sep+name.len), 0};
    copybytes(r.data, cwd.data, cwd.len);
    r.len = cwd.len;
    if (sep) r.data[r.len++] = '/';
    copybytes(r.data+r.len, name.data, name.len);
    r.len += name.len;
    return r;
}

// Join a directory and a name with one slash.
static Str fkjoin(Arena *a, Str dir, Str name)
{
    b32 sep = dir.len && dir[dir.len-1]!='/';
    Str r   = {allocbytes(a, dir.len+sep+name.len), 0};
    copybytes(r.data, dir.data, dir.len);
    r.len = dir.len;
    if (sep) r.data[r.len++] = '/';
    copybytes(r.data+r.len, name.data, name.len);
    r.len += name.len;
    return r;
}

static Str fkbasename(Str s)  // final component, ignoring trailing slashes
{
    for (; s.len>1 && s[s.len-1]=='/'; s.len--) {}
    iz i = s.len;
    for (; i>0 && s[i-1]!='/'; i--) {}
    return s.len==1 ? s : cuthead(s, i);
}

struct FkIn {
    Handle h;
    i32    err;
    b32    std;   // standard input: not to be closed
    b32    dir;   // opened a directory
};

static FkIn fkopenin(FkProg *p, Str name)
{
    FkIn r = {};
    if (name == "-") {
        r.h   = p->ctx->fd[0];
        r.std = 1;
        r.err = r.h==BADHANDLE ? E_BADF : 0;
        return r;
    }
    Opened o = os_open(p->os, *p->a, fkpath(p, name), OF_READ, 0);
    r.h   = o.h;
    r.err = o.err;
    if (!r.err) {
        Stat st = os_fstat(p->os, r.h);
        r.dir = !st.err && st.type==FT_DIR;
    }
    return r;
}

static void fkclosein(FkProg *p, FkIn *in)
{
    if (!in->err && !in->std) os_close(p->os, in->h);
}

// Read all remaining input from a handle. Returns null data on error.
static Str fkreadall(FkProg *p, Handle h)
{
    Buf b(p->a, 1<<12);
    for (;;) {
        reserve(&b, 1<<12);
        iz n = os_read(p->os, h, b.data+b.len, b.cap-b.len);
        if (n < 0) return {};
        if (!n) break;
        b.len += n;
    }
    return finish(&b);
}

// Read a whole named input ("-" for standard input), reporting errors
// as "NAME: FILE: error". Returns null data on failure.
static Str fkslurp(FkProg *p, Str name)
{
    FkIn in = fkopenin(p, name);
    if (!in.err && in.dir) {
        fkclosein(p, &in);
        in.err = E_ISDIR;
    }
    Str r = {};
    if (!in.err) {
        r = fkreadall(p, in.h);
        if (!r.data) in.err = E_IO;
        fkclosein(p, &in);
    }
    if (in.err) {
        quotef(diag(p), name);
        enddiag(p, in.err);
    }
    return r;
}

static Str fkgetenv(FakeCtx *ctx, Str name)  // null data if unset
{
    for (iz i = 0; i < ctx->nenv; i++) {
        Str e = ctx->env[i];
        if (e.len>name.len && startswith(e, name) && e[name.len]=='=') {
            return cuthead(e, name.len+1);
        }
    }
    return {};
}


// Numbers

struct FkNum {
    i64 v;
    b32 ok;
};

// A decimal integer, optional sign, no surrounding junk.
static FkNum fkparseint(Str s)
{
    FkNum r = {};
    iz    i = 0;
    b32   neg = 0;
    if (i<s.len && (s[i]=='-' || s[i]=='+')) neg = s[i++]=='-';
    if (i == s.len) return r;
    u64 v = 0;
    for (; i < s.len; i++) {
        if (!digit(s[i])) return r;
        if (v > ((u64)1<<63)/10) return r;
        v = v*10 + (u64)(s[i]-'0');
        if (v > ((u64)1<<63) - !neg) return r;
    }
    r.v  = neg ? (i64)(0 - v) : (i64)v;
    r.ok = 1;
    return r;
}

// Whether s is a decimal integer, as expr and grep -m accept them.
static b32 fkisint(Str s)
{
    iz i = s.len && s[0]=='-';
    if (i == s.len) return 0;
    for (; i < s.len; i++) {
        if (!digit(s[i])) return 0;
    }
    return 1;
}

// Lines: the next line, without its newline, and whether it had one.
struct FkLine {
    Str line;
    Str rest;
    b32 nl;
};

static FkLine fknextline(Str s, u8 delim = '\n')
{
    FkLine r = {};
    Cut    c = cut(s, delim);
    r.line = c.head;
    r.rest = c.tail;
    r.nl   = c.ok;
    return r;
}

// Split into lines, dropping the final newline's empty remainder.
static Slice<Str> fklines(Arena *a, Str s, u8 delim = '\n')
{
    Slice<Str> r = {};
    while (s.len) {
        FkLine l = fknextline(s, delim);
        r = push(a, r, l.line);
        s = l.rest;
    }
    return r;
}


// bash's test helpers (support/*.c)

static i32 fake_recho(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    for (iz i = 1; i < ctx->argc; i++) {
        print(&p.out, "argv[");
        print(&p.out, (i64)i);
        print(&p.out, "] = <");
        Str s = ctx->argv[i];
        for (iz j = 0; j<s.len && s[j]; j++) {
            u8 c = s[j];
            if (c < ' ') {
                putbyte(&p.out, '^');
                putbyte(&p.out, (u8)(c + 64));
            } else if (c == 127) {
                print(&p.out, "^?");
            } else {
                putbyte(&p.out, c);
            }
        }
        print(&p.out, ">\n");
    }
    return fkexit(&p, 0);
}

static i32 fake_zecho(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    for (iz i = 1; i < ctx->argc; i++) {
        if (i > 1) putbyte(&p.out, ' ');
        print(&p.out, ctx->argv[i]);
    }
    putbyte(&p.out, '\n');
    return fkexit(&p, 0);
}

static i32 fake_printenv(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    if (ctx->argc < 2) {
        for (iz i = 0; i < ctx->nenv; i++) {
            print(&p.out, ctx->env[i]);
            putbyte(&p.out, '\n');
        }
        return fkexit(&p, 0);
    }
    Str name = ctx->argv[1];
    for (iz i = 0; name.len && i<ctx->nenv; i++) {
        Str e = ctx->env[i];
        if (e.len>name.len && startswith(e, name) && e[name.len]=='=') {
            print(&p.out, cuthead(e, name.len+1));
            putbyte(&p.out, '\n');
            return fkexit(&p, 0);
        }
    }
    return fkexit(&p, 1);
}

static i32 fake_xcase(Os *os, FakeCtx *ctx)
{
    FkProg p  = fkprog(os, ctx);
    FkOpt  o  = fkopts(&p, 1);
    i32    op = 0;
    b32    unbuffered = 0;
    for (i32 c; (c = fkgetopt(&o, "lnu")) != -1;) {
        switch (c) {
        case 'l': op = 'l'; break;
        case 'u': op = 'u'; break;
        case 'n': unbuffered = 1; break;
        default:
            print(&p.err, "casemod: usage: casemod [-lnu] [file]\n");
            flush(&p.err);
            return fkexit(&p, 2);
        }
    }
    FkIn in = {};
    in.h   = ctx->fd[0];
    in.std = 1;
    if (o.nrest && o.rest[0]!="-") {
        Opened f = os_open(os, *p.a, fkpath(&p, o.rest[0]), OF_READ, 0);
        if (f.err) {
            print(&p.err, "casemod: ");
            print(&p.err, o.rest[0]);
            print(&p.err, ": cannot open");
            enddiag(&p, f.err);
            return fkexit(&p, 1);
        }
        in.h   = f.h;
        in.std = 0;
    }
    u8 *buf = allocbytes(p.a, 1<<12);
    for (iz n; (n = os_read(os, in.h, buf, 1<<12)) > 0;) {
        for (iz i = 0; i < n; i++) {
            u8 c = buf[i];
            if (op=='u' && c>='a' && c<='z') c = (u8)(c - 32);
            if (op=='l') c = lowercase(c);
            putbyte(&p.out, c);
        }
        if (unbuffered) flush(&p.out);  // -n: as it is read, for coprocesses
    }
    fkclosein(&p, &in);
    return fkexit(&p, 0);
}


// Small coreutils

static i32 fake_true(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    return fkexit(&p, 0);
}

static i32 fake_false(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    return fkexit(&p, 1);
}

// getconf NAME: the few system variables scripts ask for, with the
// oracle container's values (ARG_MAX is what os_spawn enforces).
static i32 fake_getconf(Os *os, FakeCtx *ctx)
{
    static struct { Str name; i64 value; } vars[] = {
        {"ARG_MAX",   2097152},
        {"PAGESIZE",  4096},
        {"PAGE_SIZE", 4096},
        {"PATH_MAX",  4096},
    };
    FkProg p = fkprog(os, ctx);
    if (ctx->argc != 2) {
        print(diag(&p), "usage: getconf NAME");
        enddiag(&p);
        return fkexit(&p, 1);
    }
    for (iz i = 0; i < countof(vars); i++) {
        if (ctx->argv[1] == vars[i].name) {
            print(&p.out, vars[i].value);
            putbyte(&p.out, '\n');
            return fkexit(&p, 0);
        }
    }
    print(diag(&p), "Unrecognized variable '");
    print(&p.err, ctx->argv[1]);
    putbyte(&p.err, '\'');
    enddiag(&p);
    return fkexit(&p, 1);
}

// yes: its arguments (or "y") on a line, until the output fails.
static i32 fake_yes(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    iz     i = ctx->argc>1 && ctx->argv[1]=="--" ? 2 : 1;
    Buf    line(p.a, 64);
    for (iz k = i; k < ctx->argc; k++) {
        if (k > i) putbyte(&line, ' ');
        print(&line, ctx->argv[k]);
    }
    if (i >= ctx->argc) putbyte(&line, 'y');
    putbyte(&line, '\n');
    Str s = finish(&line);
    while (!p.out.err) print(&p.out, s);
    return fkexit(&p, 1);
}

static i32 fake_echo(Os *os, FakeCtx *ctx)
{
    FkProg p  = fkprog(os, ctx);
    b32    nl = 1;
    b32    esc = 0;
    iz     i  = 1;
    for (; i < ctx->argc; i++) {
        Str a = ctx->argv[i];
        if (a.len<2 || a[0]!='-') break;
        b32 ok = 1;
        for (iz j = 1; j < a.len; j++) {
            ok &= a[j]=='n' || a[j]=='e' || a[j]=='E';
        }
        if (!ok) break;
        for (iz j = 1; j < a.len; j++) {
            switch (a[j]) {
            case 'n': nl  = 0; break;
            case 'e': esc = 1; break;
            case 'E': esc = 0; break;
            }
        }
    }
    for (iz first = i; i < ctx->argc; i++) {
        if (i > first) putbyte(&p.out, ' ');
        Str a = ctx->argv[i];
        if (!esc) {
            print(&p.out, a);
            continue;
        }
        for (iz j = 0; j < a.len; j++) {
            u8 c = a[j];
            if (c!='\\' || j+1==a.len) {
                putbyte(&p.out, c);
                continue;
            }
            c = a[++j];
            switch (c) {
            case 'a': c = '\a';   break;
            case 'b': c = '\b';   break;
            case 'c': return fkexit(&p, 0);
            case 'e': c = 0x1b;   break;
            case 'f': c = '\f';   break;
            case 'n': c = '\n';   break;
            case 'r': c = '\r';   break;
            case 't': c = '\t';   break;
            case 'v': c = '\v';   break;
            case '\\': break;
            case 'x': {
                i32 v = 0, n = 0;
                for (; n<2 && j+1<a.len; n++) {
                    u8 h = a[j+1];
                    i32 d = digit(h) ? h-'0' : (h|32)>='a' && (h|32)<='f' ? (h|32)-'a'+10 : -1;
                    if (d < 0) break;
                    v = v*16 + d;
                    j++;
                }
                if (!n) {
                    putbyte(&p.out, '\\');
                    break;
                }
                c = (u8)v;
            } break;
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7': {
                i32 v = c=='0' ? 0 : c-'0';
                i32 max = c=='0' ? 3 : 2;
                for (i32 n = 0; n<max && j+1<a.len && a[j+1]>='0' && a[j+1]<='7'; n++) {
                    v = v*8 + a[++j] - '0';
                }
                c = (u8)v;
            } break;
            default:
                putbyte(&p.out, '\\');
            }
            putbyte(&p.out, c);
        }
    }
    if (nl) putbyte(&p.out, '\n');
    return fkexit(&p, 0);
}

static i32 fake_pwd(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 0);
    for (i32 c; (c = fkgetopt(&o, "LP")) != -1;) {
        if (c == '?') return fkusage(&p, 1);
    }
    if (o.nrest) {
        print(diag(&p), "ignoring non-option arguments");
        enddiag(&p);
    }
    print(&p.out, ctx->cwd);
    putbyte(&p.out, '\n');
    return fkexit(&p, 0);
}

static i32 fake_sleep(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    for (i32 c; (c = fkgetopt(&o, "")) != -1;) {
        return fkusage(&p, 1);
    }
    if (!o.nrest) {
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    b32 bad = 0;
    i64 ns  = 0;
    for (iz i = 0; i < o.nrest; i++) {
        Str s = o.rest[i];
        iz  j = 0, d = 0;
        i64 whole = 0, frac = 0, scale = 1000000000;
        for (; j<s.len && digit(s[j]); j++, d++) {
            whole = whole<10000 ? whole*10 + (s[j]-'0') : whole;
        }
        if (j<s.len && s[j]=='.') {
            for (j++; j<s.len && digit(s[j]); j++, d++) {
                if (scale > 1) {
                    scale /= 10;
                    frac += scale * (s[j]-'0');
                }
            }
        }
        i64 unit = 1;
        if (j<s.len && (s[j]=='s' || s[j]=='m' || s[j]=='h' || s[j]=='d')) {
            unit = s[j]=='m' ? 60 : s[j]=='h' ? 3600 : s[j]=='d' ? 86400 : 1;
            j++;
        }
        ns += (whole*1000000000 + frac) * unit;
        ns  = ns>(i64)1<<60 ? (i64)1<<60 : ns;  // bounded, never wraps
        b32 inf = s=="inf" || s=="infinity";
        if (!inf && (!d || j!=s.len)) {
            print(diag(&p), "invalid time interval ");
            quote(&p.err, s);
            enddiag(&p);
            bad = 1;
        }
    }
    if (bad) return fkusage(&p, 1);
    os_sleep(os, ns);
    return fkexit(&p, 0);
}

static i32 fake_basename(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    multi = 0;
    Str    suffix = {};
    u8     eol = '\n';
    for (i32 c; (c = fkgetopt(&o, "as:z")) != -1;) {
        switch (c) {
        case 'a': multi = 1; break;
        case 's': multi = 1; suffix = o.arg; break;
        case 'z': eol = 0; break;
        default:  return fkusage(&p, 1);
        }
    }
    if (!o.nrest) {
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    iz n = o.nrest;
    if (!multi) {
        if (n > 2) {
            print(diag(&p), "extra operand ");
            quote(&p.err, o.rest[2]);
            enddiag(&p);
            return fkusage(&p, 1);
        }
        if (n == 2) suffix = o.rest[1];
        n = 1;
    }
    for (iz i = 0; i < n; i++) {
        Str s = fkbasename(o.rest[i]);
        if (s.len && s!="/" && suffix.len && suffix.len<s.len && endswith(s, suffix)) {
            s = cuttail(s, suffix.len);
        }
        print(&p.out, s);
        putbyte(&p.out, eol);
    }
    return fkexit(&p, 0);
}

static Str fkdirname(Str s)
{
    iz i = s.len;
    for (; i>1 && s[i-1]=='/'; i--) {}   // trailing slashes
    for (; i>0 && s[i-1]!='/'; i--) {}   // final component
    for (; i>1 && s[i-1]=='/'; i--) {}   // separating slashes
    if (!i) return ".";
    return takehead(s, i);
}

static i32 fake_dirname(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    u8     eol = '\n';
    for (i32 c; (c = fkgetopt(&o, "z")) != -1;) {
        if (c == '?') return fkusage(&p, 1);
        eol = 0;
    }
    if (!o.nrest) {
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    for (iz i = 0; i < o.nrest; i++) {
        print(&p.out, fkdirname(o.rest[i]));
        putbyte(&p.out, eol);
    }
    return fkexit(&p, 0);
}

static i32 fake_uname(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    i32    want = 0;  // bit per field in output order
    for (i32 c; (c = fkgetopt(&o, "asnrvmpio")) != -1;) {
        switch (c) {
        case 'a': want |= 0x100; break;
        case 's': want |= 1<<0;  break;
        case 'n': want |= 1<<1;  break;
        case 'r': want |= 1<<2;  break;
        case 'v': want |= 1<<3;  break;
        case 'm': want |= 1<<4;  break;
        case 'p': want |= 1<<5;  break;
        case 'i': want |= 1<<6;  break;
        case 'o': want |= 1<<7;  break;
        default:  return fkusage(&p, 1);
        }
    }
    if (o.nrest) {
        print(diag(&p), "extra operand ");
        quote(&p.err, o.rest[0]);
        enddiag(&p);
        return fkusage(&p, 1);
    }
    if (want & 0x100) want = 0x9f;
    if (!want) want = 1;
    b32 first = 1;
    for (i32 i = 0; i < 8; i++) {
        if (!(want & 1<<i)) continue;
        Str s = {};
        switch (i) {
        case 0: s = os_uname(os, p.a, UN_SYSNAME);  break;
        case 1: s = os_uname(os, p.a, UN_NODENAME); break;
        case 2: s = os_uname(os, p.a, UN_RELEASE);  break;
        case 3: s = os_uname(os, p.a, UN_VERSION);  break;
        case 4: s = os_uname(os, p.a, UN_MACHINE);  break;
        case 5:
        case 6: s = "unknown";   break;
        case 7: s = "GNU/Linux"; break;
        }
        if (!first) putbyte(&p.out, ' ');
        first = 0;
        print(&p.out, s);
    }
    putbyte(&p.out, '\n');
    return fkexit(&p, 0);
}


// User and group names from /etc/passwd and /etc/group

struct FkPw {
    Str name;
    i64 uid;
    i64 gid;
    b32 ok;
};

static Str fkreadquiet(FkProg *p, Str path)  // null data on any error
{
    Opened f = os_open(p->os, *p->a, path, OF_READ, 0);
    if (f.err) return {};
    Str r = fkreadall(p, f.h);
    os_close(p->os, f.h);
    return r;
}

// Field i of a colon-separated line.
static Str fkfield(Str line, i32 i)
{
    Cut c = {};
    c.tail = line;
    for (i32 k = 0; k <= i; k++) {
        c = cut(c.tail, ':');
    }
    return c.head;
}

// Look up a user by name (when name has data) or else by uid.
static FkPw fkgetpw(FkProg *p, Str name, i64 uid)
{
    FkPw r = {};
    Str  s = fkreadquiet(p, "/etc/passwd");
    while (s.len) {
        FkLine l = fknextline(s);
        s = l.rest;
        FkNum u = fkparseint(fkfield(l.line, 2));
        FkNum g = fkparseint(fkfield(l.line, 3));
        Str   n = fkfield(l.line, 0);
        if (!u.ok || !g.ok) continue;
        if (name.data ? n==name : u.v==uid) {
            r.name = n;
            r.uid  = u.v;
            r.gid  = g.v;
            r.ok   = 1;
            return r;
        }
    }
    return r;
}

static Str fkgroupname(FkProg *p, i64 gid)  // null data if unknown
{
    Str s = fkreadquiet(p, "/etc/group");
    while (s.len) {
        FkLine l = fknextline(s);
        s = l.rest;
        FkNum g = fkparseint(fkfield(l.line, 2));
        if (g.ok && g.v==gid) return fkfield(l.line, 0);
    }
    return {};
}

// Primary group first, then groups that list the user as a member.
static Slice<i64> fkgroups(FkProg *p, Str user, i64 gid)
{
    Slice<i64> r = {};
    r = push(p->a, r, gid);
    Str s = fkreadquiet(p, "/etc/group");
    while (user.len && s.len) {
        FkLine l = fknextline(s);
        s = l.rest;
        FkNum g = fkparseint(fkfield(l.line, 2));
        if (!g.ok || g.v==gid) continue;
        for (Cut m = {{}, fkfield(l.line, 3), 1}; m.tail.len;) {
            m = cut(m.tail, ',');
            if (m.head == user) {
                r = push(p->a, r, g.v);
                break;
            }
        }
    }
    return r;
}

static void fkprintid(FkProg *p, Str key, i64 id, Str name)
{
    print(&p->out, key);
    print(&p->out, (i64)id);
    if (name.data) {
        putbyte(&p->out, '(');
        print(&p->out, name);
        putbyte(&p->out, ')');
    }
}

struct FkId {
    FkProg *p;
    i32     what;    // 0, 'u', 'g' or 'G'
    b32     names;
    b32     real;
    b32     zero;
};

// Report one user, or the process when user has no data.
static i32 fkid(FkId *c, Str user)
{
    FkProg *p = c->p;
    OsIds   ids = {};
    if (user.data) {
        FkPw pw = fkgetpw(p, user, 0);
        if (!pw.ok) {
            print(diag(p), "");
            quote(&p->err, user);
            print(&p->err, ": no such user");
            enddiag(p);
            return 1;
        }
        ids.uid = ids.euid = pw.uid;
        ids.gid = ids.egid = pw.gid;
        user = pw.name;
    } else {
        ids  = os_ids(p->os);
        user = fkgetpw(p, {}, ids.uid).name;
    }
    u8  end = c->zero ? 0 : '\n';
    i64 uid = c->real ? ids.uid : ids.euid;
    i64 gid = c->real ? ids.gid : ids.egid;
    switch (c->what) {
    case 'u':
        if (c->names) {
            Str n = fkgetpw(p, {}, uid).name;
            if (!n.data) {
                print(diag(p), "cannot find name for user ID ");
                print(&p->err, uid);
                enddiag(p);
                print(&p->out, uid);
                putbyte(&p->out, end);
                return 1;
            }
            print(&p->out, n);
        } else {
            print(&p->out, uid);
        }
        putbyte(&p->out, end);
        return 0;
    case 'g':
    case 'G': {
        Slice<i64> gs = {};
        if (c->what == 'g') {
            gs = push(p->a, gs, gid);
        } else {
            gs = fkgroups(p, user, gid);
        }
        i32 status = 0;
        for (iz i = 0; i < gs.len; i++) {
            if (i) putbyte(&p->out, c->zero ? 0 : ' ');
            Str n = c->names ? fkgroupname(p, gs[i]) : Str{};
            if (c->names && !n.data) {
                print(diag(p), "cannot find name for group ID ");
                print(&p->err, gs[i]);
                enddiag(p);
                status = 1;
            }
            if (n.data) {
                print(&p->out, n);
            } else {
                print(&p->out, gs[i]);
            }
        }
        putbyte(&p->out, end);
        return status;
    }
    }

    fkprintid(p, "uid=", ids.uid, fkgetpw(p, {}, ids.uid).name);
    fkprintid(p, " gid=", ids.gid, fkgroupname(p, ids.gid));
    if (ids.euid != ids.uid) {
        fkprintid(p, " euid=", ids.euid, fkgetpw(p, {}, ids.euid).name);
    }
    if (ids.egid != ids.gid) {
        fkprintid(p, " egid=", ids.egid, fkgroupname(p, ids.egid));
    }
    Slice<i64> gs = fkgroups(p, user, ids.gid);
    for (iz i = 0; i < gs.len; i++) {
        fkprintid(p, i ? Str(",") : Str(" groups="), gs[i], fkgroupname(p, gs[i]));
    }
    putbyte(&p->out, '\n');
    return 0;
}

static i32 fake_id(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    FkId   c = {};
    c.p = &p;
    for (i32 ch; (ch = fkgetopt(&o, "agGnruz")) != -1;) {
        switch (ch) {
        case 'a': break;
        case 'u': case 'g': case 'G':
            if (c.what && c.what!=ch) {
                print(diag(&p), "cannot print \"only\" of more than one choice");
                enddiag(&p);
                return fkexit(&p, 1);
            }
            c.what = ch;
            break;
        case 'n': c.names = 1; break;
        case 'r': c.real  = 1; break;
        case 'z': c.zero  = 1; break;
        default:  return fkusage(&p, 1);
        }
    }
    if (!c.what && (c.names || c.real)) {
        print(diag(&p), "printing only names or real IDs requires -u, -g, or -G");
        enddiag(&p);
        return fkexit(&p, 1);
    }
    if (!c.what && c.zero) {
        print(diag(&p), "option --zero not permitted in default format");
        enddiag(&p);
        return fkexit(&p, 1);
    }
    if (!o.nrest) return fkexit(&p, fkid(&c, {}));
    i32 status = 0;
    for (iz i = 0; i < o.nrest; i++) status |= fkid(&c, o.rest[i]);
    return fkexit(&p, status);
}

static i32 fake_whoami(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    for (i32 c; (c = fkgetopt(&o, "")) != -1;) {
        return fkusage(&p, 1);
    }
    if (o.nrest) {
        print(diag(&p), "extra operand ");
        quote(&p.err, o.rest[0]);
        enddiag(&p);
        return fkusage(&p, 1);
    }
    i64 uid = os_ids(os).euid;
    Str n   = fkgetpw(&p, {}, uid).name;
    if (!n.data) {
        print(diag(&p), "cannot find name for user ID ");
        print(&p.err, uid);
        enddiag(&p);
        return fkexit(&p, 1);
    }
    print(&p.out, n);
    putbyte(&p.out, '\n');
    return fkexit(&p, 0);
}


// seq: decimal fixed point, scaled to the operands' precision

struct FkDec {
    i64 v;     // value times 10^prec
    i32 prec;  // digits after the decimal point
    b32 ok;
};

static FkDec fkparsedec(Str s)
{
    FkDec r = {};
    iz    i = 0;
    b32   neg = 0;
    if (i<s.len && (s[i]=='-' || s[i]=='+')) neg = s[i++]=='-';
    i64 v = 0;
    i32 nd = 0;
    for (; i<s.len && digit(s[i]); i++, nd++) {
        if (v > ((i64)1<<62)/10) return r;
        v = v*10 + s[i] - '0';
    }
    if (i<s.len && s[i]=='.') {
        for (i++; i<s.len && digit(s[i]); i++, nd++) {
            if (v > ((i64)1<<62)/10) return r;
            v = v*10 + s[i] - '0';
            r.prec++;
        }
    }
    if (!nd || i!=s.len) return r;
    r.v  = neg ? -v : v;
    r.ok = 1;
    return r;
}

// A seq operand as strtold reads it: leading blanks, decimal with an
// optional exponent, 0x hex integers, inf and nan. Values are fixed
// point, so precision beyond 18 digits is out of reach.
struct FkSeqNum {
    FkDec d;
    b32   inf;   // the sign is d.v's
    b32   nan;
};

static FkSeqNum fkseqnum(Str s)
{
    FkSeqNum r = {};
    s = trimleft(s);
    b32 neg = s.len && s[0]=='-';
    Str t   = cuthead(s, s.len && (s[0]=='-' || s[0]=='+'));
    Str lc  = {};
    u8  low[8];
    if (t.len>=3 && t.len<=8) {
        for (iz i = 0; i < t.len; i++) low[i] = lowercase(t[i]);
        lc = {low, t.len};
    }
    if (lc=="inf" || lc=="infinity") {
        r.inf  = 1;
        r.d.v  = neg ? -1 : 1;
        r.d.ok = 1;
        return r;
    }
    if (lc == "nan") {
        r.nan = 1;
        return r;
    }
    if (t.len>2 && t[0]=='0' && (t[1]|32)=='x') {
        i64 v = 0;
        iz  i = 2;
        for (; i < t.len; i++) {
            u8  c = t[i];
            i32 d = digit(c) ? c-'0' : (c|32)>='a' && (c|32)<='f' ? (c|32)-'a'+10 : -1;
            if (d<0 || v>((i64)1<<62)/16) return r;
            v = v*16 + d;
        }
        r.d.v  = neg ? -v : v;
        r.d.ok = 1;
        return r;
    }
    // Mantissa digits, then an exponent shifting the decimal point
    i64 v = 0;
    i32 prec = 0, nd = 0;
    iz  i = 0;
    for (; i<t.len && digit(t[i]); i++, nd++) {
        if (__builtin_mul_overflow(v, 10, &v) || __builtin_add_overflow(v, t[i]-'0', &v)) {
            return r;
        }
    }
    if (i<t.len && t[i]=='.') {
        for (i++; i<t.len && digit(t[i]); i++, nd++) {
            if (v > ((i64)1<<62)/10) return r;
            v = v*10 + t[i] - '0';
            prec++;
        }
    }
    if (!nd) return r;
    if (i<t.len && (t[i]|32)=='e') {
        iz  j  = i + 1;
        b32 en = j<t.len && t[j]=='-';
        j += j<t.len && (t[j]=='-' || t[j]=='+');
        i32 e = 0;
        iz  k = j;
        for (; k<t.len && digit(t[k]); k++) e = e>99 ? e : e*10 + t[k] - '0';
        if (k > j) {
            i = k;
            prec += en ? e : -e;
        }
    }
    if (i != t.len) return r;
    for (; prec < 0; prec++) {
        if (v > ((i64)1<<62)/10) return r;
        v *= 10;
    }
    if (prec > 18) return r;
    r.d.v    = neg ? -v : v;
    r.d.prec = prec;
    r.d.ok   = 1;
    return r;
}

static b32 fkscale(FkDec d, i32 prec, i64 *v)  // false on overflow
{
    *v = d.v;
    for (i32 i = d.prec; i < prec; i++) {
        if (__builtin_mul_overflow(*v, 10, v)) return 0;
    }
    return 1;
}

static void fkprintdec(Buf *b, i64 v, i32 prec, i32 width)
{
    u8  tmp[48];
    u8 *end = tmp + countof(tmp);
    u8 *beg = end;
    i64 t   = v<0 ? v : -v;  // negative avoids overflow
    for (i32 i = 0; i < prec; i++) {
        *--beg = (u8)('0' - t%10);
        t /= 10;
    }
    if (prec) *--beg = '.';
    do {
        *--beg = (u8)('0' - t%10);
    } while (t /= 10);
    iz len = end - beg + (v < 0);
    if (v < 0) putbyte(b, '-');
    for (iz i = len; i < width; i++) putbyte(b, '0');
    print(b, span(beg, end));
}

static i32 fake_seq(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    Str    sep = "\n";
    b32    equal = 0;
    Str   *ops = alloc<Str>(p.a, ctx->argc);
    iz     nops = 0;
    for (iz i = 1; i < ctx->argc; i++) {
        Str a = ctx->argv[i];
        b32 opt = !nops && a.len>1 && a[0]=='-' && !digit(a[1]) && a[1]!='.';
        if (opt && a=="--") {
            for (i++; i < ctx->argc; i++) ops[nops++] = ctx->argv[i];
            break;
        }
        if (!opt) {
            ops[nops++] = a;
            continue;
        }
        for (iz j = 1; j < a.len; j++) {
            if (a[j] == 'w') {
                equal = 1;
            } else if (a[j] == 's') {
                if (j+1 < a.len) {
                    sep = cuthead(a, j+1);
                } else if (i+1 < ctx->argc) {
                    sep = ctx->argv[++i];
                } else {
                    print(diag(&p), "option requires an argument -- 's'");
                    enddiag(&p);
                    return fkusage(&p, 1);
                }
                break;
            } else {
                print(diag(&p), "invalid option -- '");
                putbyte(&p.err, a[j]);
                putbyte(&p.err, '\'');
                enddiag(&p);
                return fkusage(&p, 1);
            }
        }
    }
    if (!nops) {
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    if (nops > 3) {
        print(diag(&p), "extra operand ");
        quote(&p.err, ops[3]);
        enddiag(&p);
        return fkusage(&p, 1);
    }
    FkSeqNum d[3] = {};
    for (iz i = 0; i < nops; i++) {
        d[i] = fkseqnum(ops[i]);
        if (d[i].nan) {
            print(diag(&p), "invalid 'not-a-number' argument: ");
            quote(&p.err, ops[i]);
            enddiag(&p);
            return fkusage(&p, 1);
        }
        if (!d[i].d.ok) {
            print(diag(&p), "invalid floating point argument: ");
            quote(&p.err, ops[i]);
            enddiag(&p);
            return fkusage(&p, 1);
        }
    }
    FkSeqNum one = {{1, 0, 1}, 0, 0};
    FkSeqNum first = one, inc = one, last = {};
    switch (nops) {
    case 1: last = d[0]; break;
    case 2: first = d[0]; last = d[1]; break;
    case 3: first = d[0]; inc = d[1]; last = d[2]; break;
    }
    if (!inc.d.v) {
        print(diag(&p), "invalid Zero increment value: ");
        quote(&p.err, ops[1]);
        enddiag(&p);
        return fkusage(&p, 1);
    }
    // Infinities: nothing at all when the sequence starts past its end,
    // and otherwise, as many numbers as fit
    b32 up = inc.d.v > 0;
    if (first.inf || inc.inf) {
        b32 none = (first.d.v>0) == up || inc.inf;
        if (first.inf && none) return fkexit(&p, 0);
        if (inc.inf) {
            if ((up && first.d.v>last.d.v) || (!up && first.d.v<last.d.v)) {
                return fkexit(&p, 0);
            }
            Buf b(p.a, 32);
            fkprintdec(&b, first.d.v, first.d.prec, 0);
            putbyte(&b, '\n');
            print(&p.out, finish(&b));
            return fkexit(&p, 0);
        }
    }
    if (last.inf) {
        last.d.v = last.d.v>0 ? (i64)0x7fffffffffffffff : -(i64)0x7fffffffffffffff;
        last.d.prec = 0;
    }

    i32 prec = first.d.prec>inc.d.prec ? first.d.prec : inc.d.prec;
    i32 cmp  = prec>last.d.prec ? prec : last.d.prec;
    i64 x, step, end;
    if (!fkscale(first.d, cmp, &x) || !fkscale(inc.d, cmp, &step)) {
        print(diag(&p), "invalid floating point argument: ");
        quote(&p.err, ops[0]);
        enddiag(&p);
        return fkusage(&p, 1);
    }
    if (!fkscale(last.d, cmp, &end)) {
        end = last.d.v<0 ? -(i64)0x7fffffffffffffff : (i64)0x7fffffffffffffff;
    }
    i64 div = 1;
    for (i32 i = prec; i < cmp; i++) div *= 10;

    i32 width = 0;
    b32 hex   = 0;
    for (iz i = 0; i < nops; i++) {
        Str t = trimleft(ops[i]);
        t   = cuthead(t, t.len && (t[0]=='-' || t[0]=='+'));
        hex |= t.len>1 && t[0]=='0' && (t[1]|32)=='x';
    }
    if (equal && !hex) {
        Arena t = *p.a;
        Buf b1(&t, 32), b2(&t, 32);
        i64 f;
        fkscale(first.d, prec, &f);
        fkprintdec(&b1, f, prec, 0);
        fkprintdec(&b2, end/div, prec, 0);
        width = (i32)(b1.len>b2.len ? b1.len : b2.len);
    }
    b32 any = 0;
    for (; step>0 ? x<=end : x>=end;) {
        Arena t = *p.a;
        Buf   b(&t, 64);
        if (any) print(&b, sep);
        fkprintdec(&b, x/div, prec, width);
        print(&p.out, finish(&b));
        any = 1;
        if (__builtin_add_overflow(x, step, &x)) break;
    }
    if (any) putbyte(&p.out, '\n');
    return fkexit(&p, 0);
}


// date: UTC only

struct FkTm {
    i64 year;
    i32 mon;   // 1-12
    i32 mday;
    i32 hour;
    i32 min;
    i32 sec;
    i32 wday;  // 0 = Sunday
    i32 yday;  // 0-based
};

static i64 fkfloordiv(i64 a, i64 b)
{
    i64 q = a / b;
    return q - (a%b && (a<0)!=(b<0));
}

static i64 fkdaysfromcivil(i64 y, i64 m, i64 d)
{
    y -= m <= 2;
    i64 era = fkfloordiv(y, 400);
    i64 yoe = y - era*400;
    i64 doy = (153*(m>2 ? m-3 : m+9) + 2)/5 + d - 1;
    i64 doe = yoe*365 + yoe/4 - yoe/100 + doy;
    return era*146097 + doe - 719468;
}

static FkTm fkgmtime(i64 t)
{
    FkTm r = {};
    i64 days = fkfloordiv(t, 86400);
    i64 secs = t - days*86400;
    i64 z    = days + 719468;
    i64 era  = fkfloordiv(z, 146097);
    i64 doe  = z - era*146097;
    i64 yoe  = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    i64 doy  = doe - (365*yoe + yoe/4 - yoe/100);
    i64 mp   = (5*doy + 2)/153;
    r.mday = (i32)(doy - (153*mp + 2)/5 + 1);
    r.mon  = (i32)(mp<10 ? mp+3 : mp-9);
    r.year = yoe + era*400 + (r.mon <= 2);
    r.hour = (i32)(secs / 3600);
    r.min  = (i32)(secs / 60 % 60);
    r.sec  = (i32)(secs % 60);
    r.wday = (i32)(days - fkfloordiv(days+4, 7)*7 + 4);
    r.yday = (i32)(days - fkdaysfromcivil(r.year, 1, 1));
    return r;
}

static Str fkdays[]   = {"Sunday", "Monday", "Tuesday", "Wednesday",
                         "Thursday", "Friday", "Saturday"};
static Str fkmonths[] = {"January", "February", "March", "April", "May",
                         "June", "July", "August", "September", "October",
                         "November", "December"};

// ISO 8601 week number and week-based year.
static i32 fkisoweek(FkTm *tm, i64 *year)
{
    i32 wday = (tm->wday + 6) % 7;  // Monday = 0
    i32 week = (tm->yday - wday + 10) / 7;
    *year = tm->year;
    if (week < 1) {
        i64 y = tm->year - 1;
        b32 leap = (y%4==0 && y%100!=0) || y%400==0;
        week  = (tm->yday + 365 + leap - wday + 10) / 7;
        *year = y;
    } else if (week == 53) {
        i64 y = tm->year;
        b32 leap = (y%4==0 && y%100!=0) || y%400==0;
        if (tm->yday - wday + 3 >= 365 + leap) {  // Thursday in next year
            week  = 1;
            *year = y + 1;
        }
    }
    return week;
}

static void fkstrftime(Buf *b, Str fmt, i64 t, i64 nsec)
{
    FkTm tm = fkgmtime(t);
    for (iz i = 0; i < fmt.len; i++) {
        if (fmt[i]!='%' || i+1==fmt.len) {
            putbyte(b, fmt[i]);
            continue;
        }
        iz  start = i++;
        u8  flag  = 0;
        b32 upper = 0, swap = 0;
        for (; i < fmt.len; i++) {
            u8 c = fmt[i];
            if (c=='-' || c=='_' || c=='0' || c=='+') {
                flag = c;
            } else if (c == '^') {
                upper = 1;
            } else if (c == '#') {
                swap = 1;
            } else {
                break;
            }
        }
        i32 width = -1;
        for (; i<fmt.len && digit(fmt[i]); i++) {
            width = (width<0 ? 0 : width*10) + fmt[i] - '0';
        }
        i32 colons = 0;
        for (; i<fmt.len && fmt[i]==':'; i++) colons++;
        for (; i<fmt.len && (fmt[i]=='E' || fmt[i]=='O'); i++) {}
        if (i == fmt.len) {
            print(b, cuthead(fmt, start));
            break;
        }

        Arena scratch = *b->a;
        Buf   sub(&scratch, 64);
        b32   num = 1;
        i64   v   = 0;
        i32   w   = 2;
        u8    pad = '0';
        Str   s   = {};
        i64   isoyear = 0;
        switch (fmt[i]) {
        case 'a': num = 0; s = takehead(fkdays[tm.wday], 3); break;
        case 'A': num = 0; s = fkdays[tm.wday]; break;
        case 'b':
        case 'h': num = 0; s = takehead(fkmonths[tm.mon-1], 3); break;
        case 'B': num = 0; s = fkmonths[tm.mon-1]; break;
        case 'c': num = 0; fkstrftime(&sub, "%a %b %e %H:%M:%S %Y", t, nsec); break;
        case 'C': v = fkfloordiv(tm.year, 100); break;
        case 'd': v = tm.mday; break;
        case 'D': num = 0; fkstrftime(&sub, "%m/%d/%y", t, nsec); break;
        case 'e': v = tm.mday; pad = ' '; break;
        case 'F': num = 0; fkstrftime(&sub, "%+4Y-%m-%d", t, nsec); break;
        case 'g': fkisoweek(&tm, &isoyear); v = isoyear % 100; break;
        case 'G': fkisoweek(&tm, &isoyear); v = isoyear; w = 1; break;
        case 'H': v = tm.hour; break;
        case 'I': v = (tm.hour+11)%12 + 1; break;
        case 'j': v = tm.yday + 1; w = 3; break;
        case 'k': v = tm.hour; pad = ' '; break;
        case 'l': v = (tm.hour+11)%12 + 1; pad = ' '; break;
        case 'm': v = tm.mon; break;
        case 'M': v = tm.min; break;
        case 'n': num = 0; s = "\n"; break;
        case 'N': v = nsec; w = 9; break;
        case 'p': num = 0; s = tm.hour<12 ? Str("AM") : Str("PM"); swap = swap ? 2 : 0; break;
        case 'P': num = 0; s = tm.hour<12 ? Str("am") : Str("pm"); break;
        case 'q': v = (tm.mon+2)/3; w = 1; break;
        case 'r': num = 0; fkstrftime(&sub, "%I:%M:%S %p", t, nsec); break;
        case 'R': num = 0; fkstrftime(&sub, "%H:%M", t, nsec); break;
        case 's': v = t; w = 1; break;
        case 'S': v = tm.sec; break;
        case 't': num = 0; s = "\t"; break;
        case 'T': num = 0; fkstrftime(&sub, "%H:%M:%S", t, nsec); break;
        case 'u': v = tm.wday ? tm.wday : 7; w = 1; break;
        case 'U': v = (tm.yday + 7 - tm.wday) / 7; break;
        case 'V': v = fkisoweek(&tm, &isoyear); break;
        case 'w': v = tm.wday; w = 1; break;
        case 'W': v = (tm.yday + 7 - (tm.wday+6)%7) / 7; break;
        case 'x': num = 0; fkstrftime(&sub, "%m/%d/%y", t, nsec); break;
        case 'X': num = 0; fkstrftime(&sub, "%H:%M:%S", t, nsec); break;
        case 'y': v = tm.year % 100; v = v<0 ? v+100 : v; break;
        case 'Y': v = tm.year; w = 1; break;
        case 'z':
            num = 0;
            switch (colons) {
            case 0:  s = "+0000";     break;
            case 1:  s = "+00:00";    break;
            case 2:  s = "+00:00:00"; break;
            default: s = "+00";       break;
            }
            break;
        case 'Z': num = 0; s = "UTC"; swap = swap ? 2 : 0; break;
        case '%': num = 0; s = "%"; break;
        default:
            print(b, span(fmt.data+start, fmt.data+i+1));
            continue;
        }
        if (sub.len) s = finish(&sub);

        if (num) {
            u8  tmp[24];
            Str d = fkfmtint(tmp+countof(tmp), v<0 ? -v : v);
            if (flag == '_') pad = ' ';
            if (flag=='0' || flag=='+') pad = '0';
            i32 tw = width>=0 ? width : flag=='-' ? 0 : w;
            if (flag == '-') tw = 0;
            i32 plus = fmt[i]=='Y' || fmt[i]=='G' ? 4 : w;  // natural width
            if (flag=='+' && v>=0 && (d.len>plus || tw>plus)) {
                putbyte(b, '+');  // wider than the year's four digits
                tw--;
            }
            if (v < 0) {
                if (pad == '0') {
                    putbyte(b, '-');
                    tw--;
                }
            }
            iz len = d.len + (v<0 && pad!='0');
            for (iz k = len; k < tw; k++) putbyte(b, pad);
            if (v<0 && pad!='0') putbyte(b, '-');
            print(b, d);
        } else {
            u8 spad = flag=='0' ? '0' : ' ';
            for (iz k = s.len; k<width && flag!='-'; k++) putbyte(b, spad);
            for (iz k = 0; k < s.len; k++) {
                u8 c = s[k];
                if (upper || swap==1) {
                    c = c>='a' && c<='z' ? (u8)(c-32) : c;
                } else if (swap == 2) {
                    c = lowercase(c);
                }
                putbyte(b, c);
            }
        }
    }
}

static i32 fake_date(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    Str    when = {};
    Str    fixed = {};  // from -R or -I
    for (i32 c; (c = fkgetopt(&o, "d:uRI::")) != -1;) {
        switch (c) {
        case 'd': when = o.arg; break;
        case 'u': break;
        case 'R': fixed = "%a, %d %b %Y %H:%M:%S %z"; break;
        case 'I': {
            Str specs[] = {"date", "hours", "minutes", "seconds", "ns"};
            Str fmts[]  = {"%Y-%m-%d", "%Y-%m-%dT%H%:z", "%Y-%m-%dT%H:%M%:z",
                           "%Y-%m-%dT%H:%M:%S%:z", "%Y-%m-%dT%H:%M:%S,%N%:z"};
            iz k = 0;
            for (; o.arg.len && k<countof(specs) && !startswith(specs[k], o.arg); k++) {}
            if (k == countof(specs)) {
                print(diag(&p), "invalid argument ");
                quote(&p.err, o.arg);
                print(&p.err, " for '--iso-8601'");
                enddiag(&p);
                print(&p.err, "Valid arguments are:\n  - 'hours'\n  - 'minutes'\n"
                              "  - 'date'\n  - 'seconds'\n  - 'ns'\n");
                return fkusage(&p, 1);
            }
            fixed = fmts[k];
        } break;
        default:  return fkusage(&p, 1);
        }
    }
    Str fmt = fixed.data ? fixed : Str("%a %b %e %H:%M:%S %Z %Y");
    if (o.nrest) {
        if (fixed.data && o.rest[0].len && o.rest[0][0]=='+') {
            print(diag(&p), "multiple output formats specified");
            enddiag(&p);
            return fkexit(&p, 1);
        }
        if (o.rest[0].len && o.rest[0][0]=='+') {
            fmt = cuthead(o.rest[0], 1);
        } else if (when.data) {
            print(diag(&p), "the argument ");
            quote(&p.err, o.rest[0]);
            print(&p.err, " lacks a leading '+';\n"
                          "when using an option to specify date(s), any non-option\n"
                          "argument must be a format string beginning with '+'");
            enddiag(&p);
            return fkusage(&p, 1);
        } else {
            print(diag(&p), "invalid date ");
            quote(&p.err, o.rest[0]);
            enddiag(&p);
            return fkexit(&p, 1);
        }
        if (o.nrest > 1) {
            print(diag(&p), "extra operand ");
            quote(&p.err, o.rest[1]);
            enddiag(&p);
            return fkusage(&p, 1);
        }
    }
    i64 now  = os_now(os);
    i64 t    = fkfloordiv(now, 1000000000);
    i64 nsec = now - t*1000000000;
    if (when.data) {
        Str   s = trim(when);
        FkDec d = {};
        if (s.len && s[0]=='@') d = fkparsedec(trimleft(cuthead(s, 1)));
        if (!d.ok || d.prec>9) {
            print(diag(&p), "invalid date ");
            quote(&p.err, when);
            enddiag(&p);
            return fkexit(&p, 1);
        }
        i64 scale = 1;
        for (i32 i = 0; i < d.prec; i++) scale *= 10;
        t    = fkfloordiv(d.v, scale);
        nsec = (d.v - t*scale) * (1000000000/scale);
    }
    Buf b(p.a, 256);
    fkstrftime(&b, fmt, t, nsec);
    putbyte(&b, '\n');
    print(&p.out, finish(&b));
    return fkexit(&p, 0);
}


// env and tee

static i32 fake_env(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 0);
    b32    clear = 0, nul = 0;
    Str    dir = {};
    Slice<Str> unset = {};
    for (i32 c; (c = fkgetopt(&o, "iu:0C:")) != -1;) {
        switch (c) {
        case 'i': clear = 1; break;
        case 'u': unset = push(p.a, unset, o.arg); break;
        case '0': nul = 1; break;
        case 'C': dir = o.arg; break;
        default:  return fkusage(&p, 125);
        }
    }
    iz k = 0;
    if (k<o.nrest && o.rest[k]=="-") {
        clear = 1;
        k++;
    }

    Slice<Str> env = {};
    for (iz i = 0; !clear && i<ctx->nenv; i++) {
        env = push(p.a, env, ctx->env[i]);
    }
    for (iz u = 0; u < unset.len; u++) {
        Str name = unset[u];
        if (!name.len || cut(name, '=').ok) {
            print(diag(&p), "cannot unset ");
            quote(&p.err, name);
            enddiag(&p, E_INVAL);
            return fkexit(&p, 125);
        }
        iz n = 0;
        for (iz i = 0; i < env.len; i++) {
            Str e = env[i];
            if (!(startswith(e, name) && e.len>name.len && e[name.len]=='=')) {
                env[n++] = e;
            }
        }
        env.len = n;
    }
    for (; k<o.nrest && cut(o.rest[k], '=').ok; k++) {
        Str a = o.rest[k];
        Str name = cut(a, '=').head;
        iz  i = 0;
        for (; i < env.len; i++) {
            if (cut(env[i], '=').head == name) break;
        }
        if (i < env.len) {
            env[i] = a;
        } else {
            env = push(p.a, env, a);
        }
    }

    Str cwd = ctx->cwd;
    if (dir.data) {
        cwd = fkpath(&p, dir);
        Stat st = os_stat(os, *p.a, cwd, 1);
        i32 err = st.err ? st.err : st.type!=FT_DIR ? E_NOTDIR : 0;
        if (err) {
            print(diag(&p), "cannot change directory to ");
            quote(&p.err, dir);
            enddiag(&p, err);
            return fkexit(&p, 125);
        }
    }

    if (k == o.nrest) {
        if (dir.data) {
            print(diag(&p), "must specify command with --chdir (-C)");
            enddiag(&p);
            return fkusage(&p, 125);
        }
        for (iz i = 0; i < env.len; i++) {
            print(&p.out, env[i]);
            putbyte(&p.out, nul ? 0 : '\n');
        }
        return fkexit(&p, 0);
    }

    Spawn s = {};
    s.argv = o.rest + k;
    s.argc = o.nrest - k;
    s.env  = env.data;
    s.nenv = env.len;
    s.cwd  = cwd;
    s.fds  = alloc<SpawnFd>(p.a, countof(ctx->fd));
    for (i32 i = 0; i < countof(ctx->fd); i++) {
        if (ctx->fd[i] != BADHANDLE) {
            s.fds[s.nfds].fd  = i;
            s.fds[s.nfds++].h = ctx->fd[i];
        }
    }

    Str name = s.argv[0];
    Str path = {};
    for (iz i = 0; i < env.len; i++) {
        if (startswith(env[i], "PATH=")) path = cuthead(env[i], 5);
    }
    if (!path.data) path = "/bin:/usr/bin";
    b32 slash = cut(name, '/').ok;
    i32 err   = E_NOENT;
    b32 access = 0;
    flush(&p.out);
    for (Cut c = {{}, path, 1}; c.ok;) {
        Str cand = {};
        if (slash) {
            cand = fkpath(&p, name);
            c.ok = 0;
        } else {
            c = cut(c.tail, ':');
            cand = fkpath(&p, fkjoin(p.a, c.head, name));
        }
        s.path = cand;
        Spawned r = os_spawn(os, *p.a, &s);
        if (!r.err) {
            return fkexit(&p, os_waitproc(os, r.proc));
        }
        err = r.err;
        if (err == E_ACCES) {
            access = 1;
        } else if (err!=E_NOENT && err!=E_NOTDIR) {
            break;
        }
    }
    if (access && (err==E_NOENT || err==E_NOTDIR)) err = E_ACCES;
    print(diag(&p), "");
    quote(&p.err, name);
    enddiag(&p, err);
    return fkexit(&p, err==E_NOENT ? 127 : 126);
}

static i32 fake_tee(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    append = 0;
    for (i32 c; (c = fkgetopt(&o, "aip")) != -1;) {
        switch (c) {
        case 'a': append = 1; break;
        case 'i': break;
        case 'p': break;
        default:  return fkusage(&p, 1);
        }
    }
    i32     status = 0;
    Handle *hs = alloc<Handle>(p.a, o.nrest);
    i32     flags = OF_WRITE | OF_CREATE | (append ? OF_APPEND : OF_TRUNC);
    for (iz i = 0; i < o.nrest; i++) {
        Opened f = os_open(os, *p.a, fkpath(&p, o.rest[i]), flags, 0666);
        hs[i] = f.h;
        if (f.err) {
            hs[i] = BADHANDLE;
            quotef(diag(&p), o.rest[i]);
            enddiag(&p, f.err);
            status = 1;
        }
    }
    u8 *buf = allocbytes(p.a, 1<<13);
    for (iz n; (n = os_read(os, ctx->fd[0], buf, 1<<13)) > 0;) {
        print(&p.out, {buf, n});
        flush(&p.out);
        for (iz i = 0; i < o.nrest; i++) {
            if (hs[i]!=BADHANDLE && !os_write(os, hs[i], buf, n)) {
                quotef(diag(&p), o.rest[i]);
                enddiag(&p, E_IO);
                os_close(os, hs[i]);
                hs[i] = BADHANDLE;
                status = 1;
            }
        }
    }
    for (iz i = 0; i < o.nrest; i++) {
        if (hs[i] != BADHANDLE) os_close(os, hs[i]);
    }
    return fkexit(&p, status);
}


// cat

struct FkCat {
    b32 number;    // -n
    b32 nonblank;  // -b
    b32 squeeze;   // -s
    b32 ends;      // -E
    b32 tabs;      // -T
    b32 visible;   // -v
    i64 line;
    b32 bol;
    i32 blanks;
};

static void fkcatnum(FkOut *o, FkCat *c)
{
    printpad(o, ++c->line, 6, ' ');
    putbyte(o, '\t');
}

static void fkcatbytes(FkOut *o, FkCat *c, u8 *buf, iz len)
{
    for (iz i = 0; i < len; i++) {
        u8 b = buf[i];
        if (c->bol) {
            if (b == '\n') {
                c->blanks++;
                if (c->squeeze && c->blanks>1) continue;
                if (c->number && !c->nonblank) fkcatnum(o, c);
                if (c->ends) putbyte(o, '$');
                putbyte(o, '\n');
                continue;
            }
            c->blanks = 0;
            if (c->number) fkcatnum(o, c);
            c->bol = 0;
        }
        if (b == '\n') {
            if (c->ends) putbyte(o, '$');
            putbyte(o, '\n');
            c->bol = 1;
        } else if (b == '\t') {
            print(o, c->tabs ? Str("^I") : Str("\t"));
        } else if (!c->visible) {
            putbyte(o, b);
        } else {
            if (b >= 128) {
                print(o, "M-");
                b = (u8)(b - 128);
            }
            if (b < 32) {
                putbyte(o, '^');
                putbyte(o, (u8)(b + 64));
            } else if (b == 127) {
                print(o, "^?");
            } else {
                putbyte(o, b);
            }
        }
    }
}

static i32 fake_cat(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    FkCat  c = {};
    c.bol = 1;
    for (i32 ch; (ch = fkgetopt(&o, "AbeEnstTuv")) != -1;) {
        switch (ch) {
        case 'A': c.visible = c.ends = c.tabs = 1; break;
        case 'b': c.number = c.nonblank = 1; break;
        case 'e': c.visible = c.ends = 1; break;
        case 'E': c.ends = 1; break;
        case 'n': c.number = 1; break;
        case 's': c.squeeze = 1; break;
        case 't': c.visible = c.tabs = 1; break;
        case 'T': c.tabs = 1; break;
        case 'u': break;
        case 'v': c.visible = 1; break;
        default:  return fkusage(&p, 1);
        }
    }
    b32 plain = !(c.number || c.squeeze || c.ends || c.tabs || c.visible);
    if (!o.nrest) o.rest[o.nrest++] = "-";
    i32 status = 0;
    u8 *buf = allocbytes(p.a, 1<<16);
    for (iz i = 0; i < o.nrest; i++) {
        FkIn in = fkopenin(&p, o.rest[i]);
        if (in.err) {
            quotef(diag(&p), o.rest[i]);
            enddiag(&p, in.err);
            status = 1;
            continue;
        }
        if (in.dir) {
            quotef(diag(&p), o.rest[i]);
            enddiag(&p, E_ISDIR);
            fkclosein(&p, &in);
            status = 1;
            continue;
        }
        for (;;) {
            iz n = os_read(os, in.h, buf, 1<<16);
            if (n < 0) {
                quotef(diag(&p), o.rest[i]);
                enddiag(&p, E_IO);
                status = 1;
                break;
            }
            if (!n) break;
            if (plain) {
                print(&p.out, {buf, n});
            } else {
                fkcatbytes(&p.out, &c, buf, n);
            }
            flush(&p.out);
        }
        fkclosein(&p, &in);
    }
    return fkexit(&p, status);
}


// Permission modes, in the manner of gnulib's modechange

enum : i32 { FK_MODEBITS = 07777 };

struct FkModeOp {
    u8  op;         // = + -
    i32 flag;       // 0, 'c' copy existing, 'X' x if any x
    i32 affected;   // who bits, or 0 if not given
    i32 value;
    i32 mentioned;
};

struct FkMode {
    FkModeOp *ops;
    iz        nops;
    b32       ok;
};

static FkMode fkparsemode(Arena *a, Str s)
{
    FkMode r = {};
    r.ops = alloc<FkModeOp>(a, s.len+1);
    if (!s.len) return r;

    b32 octal = 1;
    i32 v = 0;
    for (iz i = 0; i < s.len; i++) {
        if (s[i]<'0' || s[i]>'7') {
            octal = 0;
            break;
        }
        v = v*8 + s[i] - '0';
        if (v > FK_MODEBITS) return r;
    }
    if (octal) {
        FkModeOp *op = r.ops + r.nops++;
        op->op        = '=';
        op->affected  = FK_MODEBITS;
        op->value     = v;
        op->mentioned = s.len<5 ? (v&06000) | 01777 : FK_MODEBITS;
        r.ok = 1;
        return r;
    }

    iz i = 0;
    for (;;) {
        i32 who = 0;
        for (; i < s.len; i++) {
            u8 c = s[i];
            if      (c == 'u') who |= 04700;
            else if (c == 'g') who |= 02070;
            else if (c == 'o') who |= 01007;
            else if (c == 'a') who |= FK_MODEBITS;
            else break;
        }
        if (i==s.len || (s[i]!='=' && s[i]!='+' && s[i]!='-')) return r;
        while (i<s.len && (s[i]=='=' || s[i]=='+' || s[i]=='-')) {
            FkModeOp *op = r.ops + r.nops++;
            op->op       = s[i++];
            op->affected = who;
            if (i<s.len && (s[i]=='u' || s[i]=='g' || s[i]=='o')) {
                op->flag  = 'c';
                op->value = s[i]=='u' ? 0700 : s[i]=='g' ? 070 : 07;
                i++;
            } else {
                for (; i < s.len; i++) {
                    u8 c = s[i];
                    if      (c == 'r') op->value |= 0444;
                    else if (c == 'w') op->value |= 0222;
                    else if (c == 'x') op->value |= 0111;
                    else if (c == 'X') op->flag = 'X';
                    else if (c == 's') op->value |= 06000;
                    else if (c == 't') op->value |= 01000;
                    else break;
                }
            }
            op->mentioned = who ? who&op->value : op->value;
        }
        if (i == s.len) break;
        if (s[i++] != ',') return r;
    }
    r.ok = 1;
    return r;
}

static i32 fkmodeadjust(i32 mode, b32 dir, i32 umask, FkMode m)
{
    i32 r = mode & FK_MODEBITS;
    for (iz i = 0; i < m.nops; i++) {
        FkModeOp *op = m.ops + i;
        i32 omit  = dir ? 06000 & ~op->mentioned : 0;
        i32 value = op->value;
        if (op->flag == 'c') {
            value &= r;
            value  = (value&0444 ? 0444 : 0) | (value&0222 ? 0222 : 0)
                   | (value&0111 ? 0111 : 0);
        } else if (op->flag=='X' && ((r&0111) || dir)) {
            value |= 0111;
        }
        value &= (op->affected ? op->affected : ~umask) & ~omit;
        switch (op->op) {
        case '=': {
            i32 keep = (op->affected ? ~op->affected : 0) | omit;
            r = (r & keep) | value;
        } break;
        case '+': r |= value;  break;
        case '-': r &= ~value; break;
        }
    }
    return r & FK_MODEBITS;
}

static void fkmodestr(FkOut *o, i32 m)  // nine characters, as ls shows
{
    u8 s[9];
    for (i32 i = 0; i < 9; i++) {
        s[i] = m & 0400>>i ? "rwxrwxrwx"[i] : '-';
    }
    if (m & 04000) s[2] = s[2]=='x' ? 's' : 'S';
    if (m & 02000) s[5] = s[5]=='x' ? 's' : 'S';
    if (m & 01000) s[8] = s[8]=='x' ? 't' : 'T';
    print(o, {s, 9});
}


// File system utilities

static b32 fkdotname(Str name)  // final component is . or ..
{
    Str b = fkbasename(name);
    return b=="." || b=="..";
}

static b32 fkrm(FkProg *p, Str name, b32 force, b32 rec, b32 dirs)
{
    Str  path = fkpath(p, name);
    Stat st   = os_stat(p->os, *p->a, path, 0);
    if (path.len >= FK_PATHMAX) st.err = E_NAMETOOLONG;
    if (st.err) {
        if (force && st.err==E_NOENT) return 1;
        print(diag(p), "cannot remove ");
        quoteaf(&p->err, name);
        enddiag(p, st.err);
        return 0;
    }
    i32 err = 0;
    if (st.type != FT_DIR) {
        err = os_unlink(p->os, *p->a, path);
        if (force && err==E_NOENT) err = 0;
    } else if (!rec && !dirs) {
        err = E_ISDIR;
    } else if (!rec) {
        err = os_rmdir(p->os, *p->a, path);
    } else {
        Arena   t = *p->a;
        i32     lerr = 0;
        Dirent *list = os_list(p->os, &t, path, &lerr);
        b32     ok = 1;
        for (Dirent *d = list; d; d = d->next) {
            if (d->name=="." || d->name=="..") continue;
            Arena save = *p->a;
            *p->a = t;
            ok &= fkrm(p, fkjoin(p->a, name, d->name), force, rec, dirs);
            *p->a = save;
        }
        if (lerr) err = lerr;
        if (!ok) return 0;
        if (!err) err = os_rmdir(p->os, *p->a, path);
    }
    if (err) {
        print(diag(p), "cannot remove ");
        quoteaf(&p->err, name);
        enddiag(p, err);
        return 0;
    }
    return 1;
}

static i32 fake_rm(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    force = 0, rec = 0, dirs = 0;
    for (i32 c; (c = fkgetopt(&o, "fdrRv")) != -1;) {
        switch (c) {
        case 'f': force = 1; break;
        case 'd': dirs  = 1; break;
        case 'r':
        case 'R': rec = 1; break;
        case 'v': break;
        default:  return fkusage(&p, 1);
        }
    }
    if (!o.nrest) {
        if (force) return fkexit(&p, 0);
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    b32 ok = 1;
    for (iz i = 0; i < o.nrest; i++) {
        Str name = o.rest[i];
        if (rec && fkdotname(name)) {
            print(diag(&p), "refusing to remove '.' or '..' directory: skipping ");
            quoteaf(&p.err, name);
            enddiag(&p);
            ok = 0;
            continue;
        }
        if (rec && fkpath(&p, name)=="/") {
            print(diag(&p), "it is dangerous to operate recursively on '/'");
            enddiag(&p);
            print(diag(&p), "use --no-preserve-root to override this failsafe");
            enddiag(&p);
            ok = 0;
            continue;
        }
        Arena save = *p.a;
        ok &= fkrm(&p, name, force, rec, dirs);
        *p.a = save;
    }
    return fkexit(&p, !ok);
}

static i32 fake_mkdir(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    parents = 0, verbose = 0;
    Str    modearg = {};
    for (i32 c; (c = fkgetopt(&o, "pm:v")) != -1;) {
        switch (c) {
        case 'p': parents = 1; break;
        case 'm': modearg = o.arg; break;
        case 'v': verbose = 1; break;
        default:  return fkusage(&p, 1);
        }
    }
    i32 umask = os_umask(os);
    i32 mode  = 0777;
    if (modearg.data) {
        FkMode m = fkparsemode(p.a, modearg);
        if (!m.ok) {
            print(diag(&p), "invalid mode ");
            quote(&p.err, modearg);
            enddiag(&p);
            return fkexit(&p, 1);
        }
        mode = fkmodeadjust(0777, 1, umask, m);
    }
    if (!o.nrest) {
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    i32 status = 0;
    for (iz i = 0; i < o.nrest; i++) {
        Str name = o.rest[i];
        b32 fail = 0;
        if (parents) {
            // Create each ancestor in turn
            for (iz j = 1; j < name.len; j++) {
                if (name[j]!='/' || name[j-1]=='/') continue;
                Str  sub  = takehead(name, j);
                Str  path = fkpath(&p, sub);
                i32  err  = os_mkdir(os, *p.a, path, 0777);
                if (err == E_EXIST) {
                    Stat st = os_stat(os, *p.a, path, 1);
                    err = st.err ? st.err : st.type!=FT_DIR ? E_NOTDIR : 0;
                } else if (!err && verbose) {
                    print(&p.out, p.name);
                    print(&p.out, ": created directory ");
                    FkOut *out = &p.out;
                    quoteaf(out, sub);
                    putbyte(out, '\n');
                }
                if (err) {
                    print(diag(&p), "cannot create directory ");
                    quoteaf(&p.err, sub);
                    enddiag(&p, err);
                    fail = 1;
                    break;
                }
            }
        }
        if (fail) {
            status = 1;
            continue;
        }
        Str path = fkpath(&p, name);
        i32 err  = os_mkdir(os, *p.a, path, mode);
        if (err==E_EXIST && parents) {
            Stat st = os_stat(os, *p.a, path, 1);
            if (!st.err && st.type==FT_DIR) continue;
        }
        if (err) {
            print(diag(&p), "cannot create directory ");
            quoteaf(&p.err, name);
            enddiag(&p, err);
            status = 1;
            continue;
        }
        if (modearg.data) os_chmod(os, *p.a, path, mode);
        if (verbose) {
            print(&p.out, p.name);
            print(&p.out, ": created directory ");
            quoteaf(&p.out, name);
            putbyte(&p.out, '\n');
        }
    }
    return fkexit(&p, status);
}

static i32 fake_rmdir(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    parents = 0, verbose = 0;
    for (i32 c; (c = fkgetopt(&o, "pv")) != -1;) {
        switch (c) {
        case 'p': parents = 1; break;
        case 'v': verbose = 1; break;
        default:  return fkusage(&p, 1);
        }
    }
    if (!o.nrest) {
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    i32 status = 0;
    for (iz i = 0; i < o.nrest; i++) {
        Str name = o.rest[i];
        for (;;) {
            if (verbose) {
                print(&p.out, p.name);
                print(&p.out, ": removing directory, ");
                quoteaf(&p.out, name);
                putbyte(&p.out, '\n');
            }
            i32 err = os_rmdir(os, *p.a, fkpath(&p, name));
            if (err) {
                print(diag(&p), "failed to remove ");
                quoteaf(&p.err, name);
                enddiag(&p, err);
                status = 1;
                break;
            }
            if (!parents) break;
            for (; name.len>1 && name[name.len-1]=='/'; name.len--) {}
            Str up = fkdirname(name);
            if (up=="." || up=="/" || up==name) break;
            name = up;
        }
    }
    return fkexit(&p, status);
}

// Parse touch -t's [[CC]YY]MMDDhhmm[.ss] into seconds.
static FkNum fkparsestamp(Str s)
{
    FkNum r = {};
    Cut   c = cut(s, '.');
    Str   d = c.head;
    i64   sec = 0;
    if (c.ok) {
        if (c.tail.len!=2 || !digit(c.tail[0]) || !digit(c.tail[1])) return r;
        sec = (c.tail[0]-'0')*10 + c.tail[1] - '0';
    }
    if (d.len!=8 && d.len!=10 && d.len!=12) return r;
    i64 f[6] = {};
    for (iz i = 0; i < d.len; i++) {
        if (!digit(d[i])) return r;
    }
    iz n = d.len / 2;
    for (iz i = 0; i < n; i++) {
        f[i] = (d[2*i]-'0')*10 + d[2*i+1] - '0';
    }
    i64 year = 0;
    iz  k = 0;
    if (n == 6) {
        year = f[0]*100 + f[1];
        k = 2;
    } else if (n == 5) {
        year = f[0] + (f[0]<69 ? 2000 : 1900);
        k = 1;
    } else {
        year = 1970;  // current year is unknown here
    }
    i64 mon = f[k], day = f[k+1], hour = f[k+2], min = f[k+3];
    if (mon<1 || mon>12 || day<1 || day>31 || hour>23 || min>59 || sec>60) {
        return r;
    }
    r.v  = fkdaysfromcivil(year, mon, day)*86400 + hour*3600 + min*60 + sec;
    r.ok = 1;
    return r;
}

static i32 fake_touch(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    nocreate = 0;
    b32    given = 0;
    i64    mtime = 0;
    for (i32 c; (c = fkgetopt(&o, "acd:fhmr:t:")) != -1;) {
        switch (c) {
        case 'a': case 'f': case 'h': case 'm': break;
        case 'c': nocreate = 1; break;
        case 'd': {
            Str   s = trim(o.arg);
            FkDec d = {};
            if (s.len && s[0]=='@') d = fkparsedec(cuthead(s, 1));
            i64 lim = (i64)0x7fffffffffffffff;
            for (i32 i = d.prec; i < 9; i++) lim /= 10;
            if (!d.ok || d.prec>9 || d.v>lim || d.v<-lim) {  // ns in i64
                print(diag(&p), "invalid date format ");
                quote(&p.err, o.arg);
                enddiag(&p);
                return fkexit(&p, 1);
            }
            mtime = d.v;
            for (i32 i = d.prec; i < 9; i++) mtime *= 10;
            given = 1;
        } break;
        case 'r': {
            Stat st = os_stat(os, *p.a, fkpath(&p, o.arg), 1);
            if (st.err) {
                print(diag(&p), "failed to get attributes of ");
                quoteaf(&p.err, o.arg);
                enddiag(&p, st.err);
                return fkexit(&p, 1);
            }
            mtime = st.mtime;
            given = 1;
        } break;
        case 't': {
            FkNum t = fkparsestamp(o.arg);
            i64 lim = (i64)0x7fffffffffffffff / 1000000000;
            if (!t.ok || t.v>lim || t.v<-lim) {
                print(diag(&p), "invalid date format ");
                quote(&p.err, o.arg);
                enddiag(&p);
                return fkexit(&p, 1);
            }
            mtime = t.v * 1000000000;
            given = 1;
        } break;
        default:
            return fkusage(&p, 1);
        }
    }
    if (!o.nrest) {
        print(diag(&p), "missing file operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    if (!given) mtime = os_now(os);
    i32 status = 0;
    for (iz i = 0; i < o.nrest; i++) {
        Str  name = o.rest[i];
        Str  path = fkpath(&p, name);
        Stat st   = os_stat(os, *p.a, path, 1);
        if (st.err == E_NOENT) {
            if (nocreate) continue;
            Opened f = os_open(os, *p.a, path, OF_WRITE|OF_CREATE, 0666);
            if (f.err) {
                print(diag(&p), "cannot touch ");
                quoteaf(&p.err, name);
                enddiag(&p, f.err);
                status = 1;
                continue;
            }
            os_close(os, f.h);
            if (!given) continue;
        } else if (st.err) {
            print(diag(&p), "cannot touch ");
            quoteaf(&p.err, name);
            enddiag(&p, st.err);
            status = 1;
            continue;
        }
        i32 err = os_utime(os, *p.a, path, mtime);
        if (err) {
            print(diag(&p), "setting times of ");
            quoteaf(&p.err, name);
            enddiag(&p, err);
            status = 1;
        }
    }
    return fkexit(&p, status);
}

struct FkChmod {
    FkProg *p;
    FkMode  mode;
    i32     umask;
    b32     rec;
    b32     quiet;
    i32     verbose;  // 1 for -c, 2 for -v
};

static b32 fkchmod(FkChmod *c, Str name, b32 top)
{
    FkProg *p    = c->p;
    Str     path = fkpath(p, name);
    Stat    st   = os_stat(p->os, *p->a, path, top);
    if (path.len >= FK_PATHMAX) st.err = E_NAMETOOLONG;
    if (st.err==E_NOENT && os_stat(p->os, *p->a, path, 0).type==FT_LINK) {
        if (!c->quiet) {
            print(diag(p), "cannot operate on dangling symlink ");
            quoteaf(&p->err, name);
            enddiag(p);
        }
        return 0;
    }
    if (st.err) {
        if (!c->quiet) {
            print(diag(p), "cannot access ");
            quoteaf(&p->err, name);
            enddiag(p, st.err);
        }
        if (c->verbose == 2) {
            quoteaf(&p->out, name);
            print(&p->out, " could not be accessed\n");
        }
        return 0;
    }
    if (st.type == FT_LINK) return 1;  // not followed while recursing
    b32 dir   = st.type == FT_DIR;
    i32 mode  = fkmodeadjust(st.mode, dir, c->umask, c->mode);
    i32 err   = os_chmod(p->os, *p->a, path, mode);
    b32 ok    = 1;
    if (err) {
        if (!c->quiet) {
            print(diag(p), "changing permissions of ");
            quoteaf(&p->err, name);
            enddiag(p, err);
        }
        ok = 0;
    } else if (c->verbose==2 || (c->verbose && (mode&07777)!=(st.mode&07777))) {
        FkOut *o = &p->out;
        print(o, "mode of ");
        quoteaf(o, name);
        if ((mode&07777) == (st.mode&07777)) {
            print(o, " retained as ");
        } else {
            print(o, " changed from ");
            printoct(o, (u64)(st.mode & 07777), 4);
            print(o, " (");
            fkmodestr(o, st.mode);
            print(o, ") to ");
        }
        printoct(o, (u64)(mode & 07777), 4);
        print(o, " (");
        fkmodestr(o, mode);
        print(o, ")\n");
    }
    if (!err) {
        i32 naive = fkmodeadjust(st.mode, dir, 0, c->mode);
        if (mode & ~naive) {
            if (!c->quiet) {
                quotef(diag(p), name);
                print(&p->err, ": new permissions are ");
                fkmodestr(&p->err, mode);
                print(&p->err, ", not ");
                fkmodestr(&p->err, naive);
                enddiag(p);
            }
            ok = 0;
        }
    }
    if (dir && c->rec) {
        Arena   t = *p->a;
        i32     lerr = 0;
        Dirent *list = os_list(p->os, &t, path, &lerr);
        for (Dirent *d = list; d; d = d->next) {
            if (d->name=="." || d->name=="..") continue;
            Arena save = *p->a;
            *p->a = t;
            ok &= fkchmod(c, fkjoin(p->a, name, d->name), 0);
            *p->a = save;
        }
    }
    return ok;
}

static b32 fkmodeopt(Str a)  // looks like "-w" or "-rwx,+x"
{
    if (a.len<2 || a[0]!='-') return 0;
    for (iz i = 1; i < a.len; i++) {
        u8 c = a[i];
        if (!(c=='r' || c=='w' || c=='x' || c=='X' || c=='s' || c=='t' ||
              c=='u' || c=='g' || c=='o' || c=='a' || c==',' || c=='+' ||
              c=='-' || c=='=' || digit(c))) {
            return 0;
        }
    }
    return 1;
}

// chgrp [-cfhRv] GROUP FILE...: files have no group of their own here
// (their gid is their owner's), so only checks: a numeric group, files
// that exist, and only the process's own group unless root.
static i32 fake_chgrp(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    for (i32 c; (c = fkgetopt(&o, "cfhRv")) != -1;) {
        if (c == '?') return fkusage(&p, 1);
    }
    if (o.nrest < 2) {
        print(diag(&p), o.nrest ? Str("missing operand after ") : Str("missing operand"));
        if (o.nrest) quote(&p.err, o.rest[0]);
        enddiag(&p);
        return fkusage(&p, 1);
    }
    FkNum g = fkparseint(o.rest[0]);
    if (!g.ok || g.v<0) {
        print(diag(&p), "invalid group: ");
        quote(&p.err, o.rest[0]);
        enddiag(&p);
        return fkexit(&p, 1);
    }
    OsIds ids = os_ids(os);
    i32 status = 0;
    for (iz i = 1; i < o.nrest; i++) {
        Str  name = o.rest[i];
        Stat st   = os_stat(os, *p.a, fkpath(&p, name), 1);
        if (st.err) {
            print(diag(&p), "cannot access ");
            quoteaf(&p.err, name);
            enddiag(&p, st.err);
            status = 1;
        } else if (ids.euid && (g.v!=ids.egid || st.uid!=ids.euid)) {
            print(diag(&p), "changing group of ");
            quoteaf(&p.err, name);
            enddiag(&p, E_PERM);
            status = 1;
        }
    }
    return fkexit(&p, status);
}

static i32 fake_chmod(Os *os, FakeCtx *ctx)
{
    FkProg  p = fkprog(os, ctx);
    FkChmod c = {};
    c.p     = &p;
    c.umask = os_umask(os);
    Str  modearg = {};
    Str *ops = alloc<Str>(p.a, ctx->argc);
    iz   nops = 0;
    b32  dashdash = 0;
    for (iz i = 1; i < ctx->argc; i++) {
        Str a = ctx->argv[i];
        if (dashdash || a.len<2 || a[0]!='-') {
            ops[nops++] = a;
        } else if (a == "--") {
            dashdash = 1;
        } else if (!modearg.data && fkmodeopt(a)) {
            modearg = a;
        } else if (a[1] == '-') {
            print(diag(&p), "unrecognized option ");
            quote(&p.err, a);
            enddiag(&p);
            return fkusage(&p, 1);
        } else {
            for (iz j = 1; j < a.len; j++) {
                switch (a[j]) {
                case 'R': c.rec   = 1; break;
                case 'f': c.quiet = 1; break;
                case 'v': c.verbose = 2; break;
                case 'c': if (!c.verbose) c.verbose = 1; break;
                default:
                    print(diag(&p), "invalid option -- '");
                    putbyte(&p.err, a[j]);
                    putbyte(&p.err, '\'');
                    enddiag(&p);
                    return fkusage(&p, 1);
                }
            }
        }
    }
    if (!modearg.data) {
        if (!nops) {
            print(diag(&p), "missing operand");
            enddiag(&p);
            return fkusage(&p, 1);
        }
        modearg = ops[0];
        ops++;
        nops--;
    }
    if (!nops) {
        print(diag(&p), "missing operand after ");
        quote(&p.err, modearg);
        enddiag(&p);
        return fkusage(&p, 1);
    }
    c.mode = fkparsemode(p.a, modearg);
    if (!c.mode.ok) {
        print(diag(&p), "invalid mode: ");
        quote(&p.err, modearg);
        enddiag(&p);
        return fkusage(&p, 1);
    }
    b32 ok = 1;
    for (iz i = 0; i < nops; i++) {
        Arena save = *p.a;
        ok &= fkchmod(&c, ops[i], 1);
        *p.a = save;
    }
    return fkexit(&p, !ok);
}

// Destination for each source: inside the last operand when it is a
// directory, else the last operand itself. Returns null data and
// reports an error when that is impossible.
struct FkTarget {
    Str  dir;     // target directory, or null data
    Str  dest;    // single destination
    b32  ok;
};

static FkTarget fktarget(FkProg *p, Str *ops, iz nops)
{
    FkTarget r = {};
    Str  last = ops[nops-1];
    Stat st   = os_stat(p->os, *p->a, fkpath(p, last), 1);
    if (!st.err && st.type==FT_DIR) {
        r.dir = last;
    } else if (nops > 2) {
        print(diag(p), "target ");
        quoteaf(&p->err, last);
        enddiag(p, st.err ? st.err : E_NOTDIR);
        return r;
    } else {
        r.dest = last;
    }
    r.ok = 1;
    return r;
}

static Str fkdest(FkProg *p, FkTarget *t, Str src)
{
    return t->dir.data ? fkjoin(p->a, t->dir, fkbasename(src)) : t->dest;
}

static b32 fkmissing(FkProg *p, iz nops, Str *ops)
{
    if (!nops) {
        print(diag(p), "missing file operand");
        enddiag(p);
        return 1;
    }
    if (nops == 1) {
        print(diag(p), "missing destination file operand after ");
        quoteaf(&p->err, ops[0]);
        enddiag(p);
        return 1;
    }
    return 0;
}

static b32 fksamefile(Stat a, Stat b)
{
    return !a.err && !b.err && a.dev==b.dev && a.ino==b.ino;
}

// Whether the directory dir is path's parent or one of its ancestors.
static b32 fkinside(FkProg *p, Stat dir, Str path)
{
    Arena t = *p->a;
    Str   d = fkdirname(path);
    for (i32 k = 0; k < 256; k++) {
        Stat a = os_stat(p->os, t, d, 1);
        if (a.err) return 0;
        if (a.dev==dir.dev && a.ino==dir.ino) return 1;
        d = fkjoin(&t, d, "..");
        Stat up = os_stat(p->os, t, d, 1);
        if (up.err || (up.dev==a.dev && up.ino==a.ino)) return 0;
    }
    return 0;
}

struct FkCp {
    FkProg *p;
    b32     rec;
    b32     preserve;
    b32     force;
    u8      links;    // P L H, or 0 to follow them unless recursive
    b32     nested;   // below an operand
};

static b32 fkcopy(FkCp *c, Str src, Str dst)
{
    FkProg *p    = c->p;
    Os     *os   = p->os;
    Str     sp   = fkpath(p, src);
    Str     dp   = fkpath(p, dst);
    b32     follow = c->links=='L' || (c->links=='H' && !c->nested)
                  || (!c->links && !c->rec);
    Stat    st   = os_stat(os, *p->a, sp, follow);
    if (sp.len>=FK_PATHMAX || dp.len>=FK_PATHMAX) st.err = E_NAMETOOLONG;
    if (st.err) {
        print(diag(p), "cannot stat ");
        quoteaf(&p->err, src);
        enddiag(p, st.err);
        return 0;
    }
    if (st.type == FT_LINK) {
        // Recursive copies keep symbolic links as links
        Str target = os_readlink(os, p->a, sp);
        i32 err    = target.data ? os_symlink(os, *p->a, target, dp) : E_INVAL;
        if (err) {
            print(diag(p), "cannot create symbolic link ");
            quoteaf(&p->err, dst);
            enddiag(p, err);
            return 0;
        }
        return 1;
    }
    Stat dt = os_stat(os, *p->a, dp, 1);
    if (fksamefile(st, dt)) {
        print(diag(p), "");
        quoteaf(&p->err, src);
        print(&p->err, " and ");
        quoteaf(&p->err, dst);
        print(&p->err, " are the same file");
        enddiag(p);
        return 0;
    }

    if (st.type == FT_DIR) {
        if (!c->rec) {
            print(diag(p), "-r not specified; omitting directory ");
            quoteaf(&p->err, src);
            enddiag(p);
            return 0;
        }
        if (fkinside(p, st, dp)) {
            print(diag(p), "cannot copy a directory, ");
            quoteaf(&p->err, src);
            print(&p->err, ", into itself, ");
            quoteaf(&p->err, dst);
            enddiag(p);
            return 0;
        }
        if (dt.err) {
            i32 err = os_mkdir(os, *p->a, dp, st.mode|0700);
            if (err) {
                print(diag(p), "cannot create directory ");
                quoteaf(&p->err, dst);
                enddiag(p, err);
                return 0;
            }
        } else if (dt.type != FT_DIR) {
            print(diag(p), "cannot overwrite non-directory ");
            quoteaf(&p->err, dst);
            print(&p->err, " with directory ");
            quoteaf(&p->err, src);
            enddiag(p);
            return 0;
        }
        Arena   t = *p->a;
        i32     lerr = 0;
        Dirent *list = os_list(os, &t, sp, &lerr);
        b32     ok = 1;
        for (Dirent *d = list; d; d = d->next) {
            if (d->name=="." || d->name=="..") continue;
            Arena save = *p->a;
            *p->a = t;
            b32 nested = c->nested;
            c->nested = 1;
            ok &= fkcopy(c, fkjoin(p->a, src, d->name), fkjoin(p->a, dst, d->name));
            c->nested = nested;
            *p->a = save;
        }
        i32 mode = c->preserve ? st.mode : st.mode & ~os_umask(os);
        if (dt.err || c->preserve) os_chmod(os, *p->a, dp, mode & FK_MODEBITS);
        if (c->preserve) os_utime(os, *p->a, dp, st.mtime);
        return ok;
    }

    if (!dt.err && dt.type==FT_DIR) {
        print(diag(p), "cannot overwrite directory ");
        quoteaf(&p->err, dst);
        print(&p->err, " with non-directory");
        enddiag(p);
        return 0;
    }
    Opened in = os_open(os, *p->a, sp, OF_READ, 0);
    if (in.err) {
        print(diag(p), "cannot open ");
        quoteaf(&p->err, src);
        print(&p->err, " for reading");
        enddiag(p, in.err);
        return 0;
    }
    Opened out = os_open(os, *p->a, dp, OF_WRITE|OF_CREATE|OF_TRUNC, st.mode&0777);
    if (out.err && !dt.err && c->force) {
        os_unlink(os, *p->a, dp);
        out = os_open(os, *p->a, dp, OF_WRITE|OF_CREATE|OF_TRUNC, st.mode&0777);
    }
    if (out.err) {
        print(diag(p), "cannot create regular file ");
        quoteaf(&p->err, dst);
        enddiag(p, out.err);
        os_close(os, in.h);
        return 0;
    }
    Arena t = *p->a;
    u8   *buf = allocbytes(&t, 1<<16);
    b32   ok = 1;
    for (iz n; (n = os_read(os, in.h, buf, 1<<16)) > 0;) {
        if (!os_write(os, out.h, buf, n)) {
            print(diag(p), "error writing ");
            quoteaf(&p->err, dst);
            enddiag(p, E_NOSPC);
            ok = 0;
            break;
        }
    }
    os_close(os, in.h);
    os_close(os, out.h);
    if (c->preserve) {
        os_chmod(os, *p->a, dp, st.mode & FK_MODEBITS);
        os_utime(os, *p->a, dp, st.mtime);
    }
    return ok;
}

static i32 fake_cp(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    FkCp   c = {};
    c.p = &p;
    for (i32 ch; (ch = fkgetopt(&o, "afpPrRLHdv")) != -1;) {
        switch (ch) {
        case 'a': c.rec = c.preserve = 1; c.links = 'P'; break;
        case 'f': c.force = 1; break;
        case 'p': c.preserve = 1; break;
        case 'r':
        case 'R': c.rec = 1; break;
        case 'd':
        case 'P': c.links = 'P'; break;
        case 'L':
        case 'H': c.links = (u8)ch; break;
        case 'v': break;
        default:  return fkusage(&p, 1);
        }
    }
    if (fkmissing(&p, o.nrest, o.rest)) return fkusage(&p, 1);
    FkTarget t = fktarget(&p, o.rest, o.nrest);
    if (!t.ok) return fkexit(&p, 1);
    b32 ok = 1;
    for (iz i = 0; i < o.nrest-1; i++) {
        Arena save = *p.a;
        ok &= fkcopy(&c, o.rest[i], fkdest(&p, &t, o.rest[i]));
        *p.a = save;
    }
    return fkexit(&p, !ok);
}

static i32 fake_mv(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    for (i32 ch; (ch = fkgetopt(&o, "fintuv")) != -1;) {
        if (ch == '?') return fkusage(&p, 1);
    }
    if (fkmissing(&p, o.nrest, o.rest)) return fkusage(&p, 1);
    FkTarget t = fktarget(&p, o.rest, o.nrest);
    if (!t.ok) return fkexit(&p, 1);
    b32 ok = 1;
    for (iz i = 0; i < o.nrest-1; i++) {
        Arena save = *p.a;
        Str   src  = o.rest[i];
        Str   dst  = fkdest(&p, &t, src);
        Str   sp   = fkpath(&p, src);
        Str   dp   = fkpath(&p, dst);
        Stat  st   = os_stat(os, *p.a, sp, 0);
        Stat  dt   = os_stat(os, *p.a, dp, 0);
        if (st.err) {
            print(diag(&p), "cannot stat ");
            quoteaf(&p.err, src);
            enddiag(&p, st.err);
            ok = 0;
        } else if (fksamefile(st, dt) ||
                   fksamefile(os_stat(os, *p.a, sp, 1), os_stat(os, *p.a, dp, 1))) {
            print(diag(&p), "");
            quoteaf(&p.err, src);
            print(&p.err, " and ");
            quoteaf(&p.err, dst);
            print(&p.err, " are the same file");
            enddiag(&p);
            ok = 0;
        } else if (!dt.err && dt.type==FT_DIR && st.type!=FT_DIR) {
            print(diag(&p), "cannot overwrite directory ");
            quoteaf(&p.err, dst);
            print(&p.err, " with non-directory");
            enddiag(&p);
            ok = 0;
        } else if (!dt.err && dt.type!=FT_DIR && st.type==FT_DIR) {
            print(diag(&p), "cannot overwrite non-directory ");
            quoteaf(&p.err, dst);
            print(&p.err, " with directory ");
            quoteaf(&p.err, src);
            enddiag(&p);
            ok = 0;
        } else if (fkdotname(src)) {
            print(diag(&p), "cannot move ");
            quoteaf(&p.err, src);
            print(&p.err, " to ");
            quoteaf(&p.err, dst);
            enddiag(&p, E_BUSY);  // as rename(2) refuses
            ok = 0;
        } else if (st.type==FT_DIR && fkinside(&p, os_stat(os, *p.a, sp, 1), dp)) {
            print(diag(&p), "cannot move ");
            quoteaf(&p.err, src);
            print(&p.err, " to a subdirectory of itself, ");
            quoteaf(&p.err, dst);
            enddiag(&p);
            ok = 0;
        } else {
            i32 err = os_rename(os, *p.a, sp, dp);
            if (err) {
                print(diag(&p), "cannot move ");
                quoteaf(&p.err, src);
                print(&p.err, " to ");
                quoteaf(&p.err, dst);
                enddiag(&p, err);
                ok = 0;
            }
        }
        *p.a = save;
    }
    return fkexit(&p, !ok);
}

static i32 fake_ln(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    sym = 0, force = 0, noderef = 0;
    for (i32 ch; (ch = fkgetopt(&o, "sfnrTv")) != -1;) {
        switch (ch) {
        case 's': sym = 1; break;
        case 'f': force = 1; break;
        case 'n': noderef = 1; break;
        case 'r': case 'T': case 'v': break;
        default:  return fkusage(&p, 1);
        }
    }
    if (!o.nrest) {
        print(diag(&p), "missing file operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    FkTarget t = {};
    iz nsrc = o.nrest - 1;
    if (o.nrest == 1) {
        t.dir = ".";
        nsrc  = 1;
    } else {
        Str  last = o.rest[o.nrest-1];
        Stat st   = os_stat(os, *p.a, fkpath(&p, last), !noderef);
        if (!st.err && st.type==FT_DIR) {
            t.dir = last;
        } else if (o.nrest > 2) {
            print(diag(&p), "target ");
            quoteaf(&p.err, last);
            enddiag(&p, st.err ? st.err : E_NOTDIR);
            return fkexit(&p, 1);
        } else {
            t.dest = last;
        }
    }
    b32 ok = 1;
    for (iz i = 0; i < nsrc; i++) {
        Arena save = *p.a;
        Str   src = o.rest[i];
        Str   dst = fkdest(&p, &t, src);
        Str   dp  = fkpath(&p, dst);
        if (!sym) {
            Stat st = os_stat(os, *p.a, fkpath(&p, src), 0);
            if (st.err) {
                print(diag(&p), "failed to access ");
                quoteaf(&p.err, src);
                enddiag(&p, st.err);
                ok = 0;
                *p.a = save;
                continue;
            }
            if (st.type == FT_DIR) {
                quotef(diag(&p), src);
                print(&p.err, ": hard link not allowed for directory");
                enddiag(&p);
                ok = 0;
                *p.a = save;
                continue;
            }
        }
        if (force) {
            Stat dt = os_stat(os, *p.a, dp, 0);
            // The source as the link would resolve it
            Str  rs = !sym || (src.len && src[0]=='/') ? fkpath(&p, src)
                    : fkjoin(p.a, fkdirname(dp), src);
            Stat ss = os_stat(os, *p.a, rs, sym);
            if (!dt.err && !ss.err && fksamefile(ss, dt)) {
                print(diag(&p), "");
                quoteaf(&p.err, src);
                print(&p.err, " and ");
                quoteaf(&p.err, dst);
                print(&p.err, " are the same file");
                enddiag(&p);
                ok = 0;
                *p.a = save;
                continue;
            }
            if (!dt.err && dt.type==FT_DIR) {
                print(diag(&p), "");
                quoteaf(&p.err, dst);
                print(&p.err, ": cannot overwrite directory");
                enddiag(&p);
                ok = 0;
                *p.a = save;
                continue;
            }
            if (!dt.err) os_unlink(os, *p.a, dp);
        }
        i32 err = sym ? os_symlink(os, *p.a, src, dp)
                      : os_link(os, *p.a, fkpath(&p, src), dp);
        if (err) {
            if (sym) {
                print(diag(&p), "failed to create symbolic link ");
                quoteaf(&p.err, dst);
            } else if (err==E_EXIST || err==E_NOSPC || err==E_ROFS) {
                print(diag(&p), "failed to create hard link ");
                quoteaf(&p.err, dst);
            } else if (err == E_MLINK) {
                print(diag(&p), "failed to create hard link to ");
                quoteaf(&p.err, src);
            } else {
                print(diag(&p), "failed to create hard link ");
                quoteaf(&p.err, dst);
                print(&p.err, " => ");
                quoteaf(&p.err, src);
            }
            enddiag(&p, err);
            ok = 0;
        }
        *p.a = save;
    }
    return fkexit(&p, !ok);
}

static i32 fake_mkfifo(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    Str    modearg = {};
    for (i32 ch; (ch = fkgetopt(&o, "m:")) != -1;) {
        if (ch == '?') return fkusage(&p, 1);
        modearg = o.arg;
    }
    i32 mode = 0666;
    if (modearg.data) {
        FkMode m = fkparsemode(p.a, modearg);
        if (!m.ok) {
            print(diag(&p), "invalid mode");
            enddiag(&p);
            return fkexit(&p, 1);
        }
        mode = fkmodeadjust(0666, 0, os_umask(os), m);
    }
    if (!o.nrest) {
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    i32 status = 0;
    for (iz i = 0; i < o.nrest; i++) {
        Str path = fkpath(&p, o.rest[i]);
        i32 err  = os_mkfifo(os, *p.a, path, mode);
        if (err) {
            print(diag(&p), "cannot create fifo ");
            quoteaf(&p.err, o.rest[i]);
            enddiag(&p, err);
            status = 1;
        } else if (modearg.data) {
            os_chmod(os, *p.a, path, mode);
        }
    }
    return fkexit(&p, status);
}

static i32 fake_mktemp(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    dir = 0, quiet = 0, dry = 0, topt = 0;
    Str    pdir = {};
    b32    pgiven = 0;
    for (i32 ch; (ch = fkgetopt(&o, "dqup:t")) != -1;) {
        switch (ch) {
        case 'd': dir = 1; break;
        case 'q': quiet = 1; break;
        case 'u': dry = 1; break;
        case 'p': pgiven = 1; pdir = o.arg; break;
        case 't': topt = 1; break;
        default:  return fkusage(&p, 1);
        }
    }
    if (o.nrest > 1) {
        print(diag(&p), "too many templates");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    Str tmpl   = o.nrest ? o.rest[0] : Str("tmp.XXXXXXXXXX");
    iz  lastx  = tmpl.len;
    for (; lastx>0 && tmpl[lastx-1]!='X'; lastx--) {}
    Str suffix = cuthead(tmpl, lastx ? lastx : tmpl.len);
    if (cut(suffix, '/').ok) {
        print(diag(&p), "invalid suffix ");
        quote(&p.err, suffix);
        print(&p.err, ", contains directory separator");
        enddiag(&p);
        return fkexit(&p, 1);
    }
    b32 usedir = pgiven || !o.nrest || topt;
    Str prefix = {};
    if (usedir) {
        Str env = fkgetenv(ctx, "TMPDIR");
        if (topt) {
            prefix = env.len ? env : pdir.len ? pdir : Str("/tmp");
        } else {
            prefix = pdir.len ? pdir : env.len ? env : Str("/tmp");
        }
        if (cut(tmpl, '/').ok && (topt || tmpl[0]=='/')) {
            print(diag(&p), "invalid template, ");
            quote(&p.err, tmpl);
            print(&p.err, topt ? Str(", contains directory separator")
                               : Str("; with --tmpdir, it may not be absolute"));
            enddiag(&p);
            return fkexit(&p, 1);
        }
        tmpl = fkjoin(p.a, prefix, tmpl);
    }
    // The last run of X's, and after it, an implied suffix
    iz end = tmpl.len - suffix.len;
    iz nx = 0;
    for (; nx<end && tmpl[end-1-nx]=='X'; nx++) {}
    if (nx < 3) {
        print(diag(&p), "too few X's in template ");
        quote(&p.err, o.nrest ? o.rest[0] : tmpl);
        enddiag(&p);
        return fkexit(&p, 1);
    }
    // Deterministic on the vos's clock, random-looking on a real one
    Str name = clone(p.a, tmpl);
    u64 rng  = (u64)os_now(os) ^ hash(tmpl);
    i32 err  = E_EXIST;
    for (i32 tries = 0; tries<1000 && err==E_EXIST; tries++) {
        for (iz i = end-nx; i < end; i++) {
            rng = rng*0x3243f6a8885a308d + 1111111111111111111u;
            name[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"[(rng>>33) % 62];
        }
        Str path = fkpath(&p, name);
        if (dry) {
            err = os_stat(os, *p.a, path, 0).err ? 0 : E_EXIST;
        } else if (dir) {
            err = os_mkdir(os, *p.a, path, 0700);
        } else {
            Opened f = os_open(os, *p.a, path, OF_WRITE|OF_CREATE|OF_EXCL, 0600);
            err = f.err;
            if (!err) os_close(os, f.h);
        }
    }
    if (err) {
        if (!quiet) {
            print(diag(&p), dir ? Str("failed to create directory via template ")
                                : Str("failed to create file via template "));
            quote(&p.err, tmpl);
            enddiag(&p, err);
        }
        return fkexit(&p, 1);
    }
    print(&p.out, name);
    putbyte(&p.out, '\n');
    return fkexit(&p, 0);
}


// ls: one name per line, or -l

struct FkEnt {
    Str  name;
    Str  path;   // for stat and readlink
    Stat st;
};

static b32 fkentless(FkEnt *a, FkEnt *b)
{
    return compare(a->name, b->name) < 0;
}

static b32 fkentnewer(FkEnt *a, FkEnt *b)
{
    if (a->st.mtime != b->st.mtime) return a->st.mtime > b->st.mtime;
    return compare(a->name, b->name) < 0;
}

struct FkLs {
    FkProg *p;
    b32     all;      // -a
    b32     almost;   // -A
    b32     dirs;     // -d
    b32     rev;      // -r
    b32     bytime;   // -t
    b32     lng;      // -l
    b32     numeric;  // -n
    b32     nogroup;  // -o
    b32     noowner;  // -g
    u8      format;   // '1', or C and x for columns
    b32     qmark;    // -q
    i64     cols;     // line width for columns
    i64     now;      // seconds
    i64     uid;      // last name lookups
    Str     uname;
    i64     gid;
    Str     gname;
};

static void fklssort(FkLs *ls, Slice<FkEnt> e)
{
    sort(e.data, e.len, ls->bytime ? fkentnewer : fkentless, *ls->p->a);
    if (ls->rev) {
        for (iz i = 0, j = e.len-1; i < j; i++, j--) {
            FkEnt t = e[i];
            e[i] = e[j];
            e[j] = t;
        }
    }
}

static Str fklsuser(FkLs *ls, i64 uid)
{
    if (!ls->uname.data || ls->uid!=uid) {
        ls->uid   = uid;
        ls->uname = ls->numeric ? Str{} : fkgetpw(ls->p, {}, uid).name;
        if (!ls->uname.data) {
            u8 tmp[24];
            ls->uname = clone(ls->p->a, fkfmtint(tmp+countof(tmp), uid));
        }
    }
    return ls->uname;
}

static Str fklsgroup(FkLs *ls, i64 gid)
{
    if (!ls->gname.data || ls->gid!=gid) {
        ls->gid   = gid;
        ls->gname = ls->numeric ? Str{} : fkgroupname(ls->p, gid);
        if (!ls->gname.data) {
            u8 tmp[24];
            ls->gname = clone(ls->p->a, fkfmtint(tmp+countof(tmp), gid));
        }
    }
    return ls->gname;
}

static iz fkdigits(i64 v)
{
    iz n = 1;
    for (; v >= 10; v /= 10) n++;
    return n;
}

// A name as ls prints it, with -q's question marks.
static Str fklsname(FkLs *ls, Str name)
{
    if (!ls->qmark) return name;
    Str r = clone(ls->p->a, name);
    for (iz i = 0; i < r.len;) {
        FkChar ch = ls->p->utf8 && r[i]>=0x80 ? fkchar(r, i) : FkChar{r[i], 1};
        if (ch.c<0 || !fkwprint(ch.c) || (!ls->p->utf8 && ch.c>=0x80)) r[i] = '?';
        i += ch.len;
    }
    return r;
}

static iz fklswidth(FkLs *ls, Str name)
{
    if (!ls->p->utf8) return name.len;
    iz w = 0;
    for (iz i = 0; i < name.len;) {
        FkChar ch = fkchar(name, i);
        i32    cw = ch.c>=0 ? fkwidth(ch.c) : 1;
        w += cw>0 ? cw : 0;
        i += ch.len;
    }
    return w;
}

// Pad from one column to another, with tabs where they fit, as GNU ls.
static void fklsindent(FkOut *o, i64 from, i64 to)
{
    while (from < to) {
        if (to/8 > (from+1)/8) {
            putbyte(o, '\t');
            from += 8 - from%8;
        } else {
            putbyte(o, ' ');
            from++;
        }
    }
}

// Names in columns, down (-C) or across (-x), in GNU's layout.
static void fklscolumns(FkLs *ls, Slice<FkEnt> ents)
{
    FkOut *o = &ls->p->out;
    iz     n = ents.len;
    if (!n) return;
    Str *names = alloc<Str>(ls->p->a, n);
    iz  *lens  = alloc<iz>(ls->p->a, n);
    for (iz i = 0; i < n; i++) {
        names[i] = fklsname(ls, ents[i].name);
        lens[i]  = fklswidth(ls, names[i]);
    }
    if (!ls->cols) {
        // Unlimited width: one line, two spaces apart
        for (iz i = 0; i < n; i++) {
            if (i) print(o, "  ");
            print(o, names[i]);
        }
        putbyte(o, '\n');
        return;
    }
    b32 down = ls->format == 'C';
    iz  maxcols = ls->cols/3 > 1 ? (iz)(ls->cols/3) : 1;
    maxcols = maxcols<n ? maxcols : n;
    maxcols = maxcols<2048 ? maxcols : 2048;  // the layout table is quadratic
    iz  *widths = alloc<iz>(ls->p->a, maxcols*maxcols);
    i64 *linelen = alloc<i64>(ls->p->a, maxcols);
    b32 *valid   = alloc<b32>(ls->p->a, maxcols);
    for (iz c = 0; c < maxcols; c++) {
        valid[c]   = 1;
        linelen[c] = (i64)(c+1) * 3;
        for (iz k = 0; k <= c; k++) widths[c*maxcols+k] = 3;
    }
    for (iz f = 0; f < n; f++) {
        for (iz c = 0; c < maxcols; c++) {
            if (!valid[c]) continue;
            iz  idx  = down ? f / ((n + c) / (c+1)) : f % (c+1);
            iz  real = lens[f] + (idx==c ? 0 : 2);
            iz *w    = widths + c*maxcols + idx;
            if (*w < real) {
                linelen[c] += real - *w;
                *w = real;
                valid[c] = linelen[c] < ls->cols;
            }
        }
    }
    iz cols = 1;
    for (iz c = maxcols; c > 0; c--) {
        if (valid[c-1]) {
            cols = c;
            break;
        }
    }
    iz *w = widths + (cols-1)*maxcols;
    if (down) {
        iz rows = (n + cols - 1) / cols;
        for (iz r = 0; r < rows; r++) {
            i64 pos = 0;
            iz  col = 0;
            for (iz f = r;;) {
                print(o, names[f]);
                iz max = w[col++];
                iz len = lens[f];
                f += rows;
                if (f >= n) break;
                fklsindent(o, pos+len, pos+max);
                pos += max;
            }
            putbyte(o, '\n');
        }
        return;
    }
    i64 pos = 0;
    print(o, names[0]);
    for (iz f = 1; f < n; f++) {
        iz col = f % cols;
        if (!col) {
            putbyte(o, '\n');
            pos = 0;
        } else {
            fklsindent(o, pos+lens[f-1], pos+w[col-1]);
            pos += w[col-1];
        }
        print(o, names[f]);
    }
    putbyte(o, '\n');
}

static void fklsprint(FkLs *ls, Slice<FkEnt> ents, b32 total)
{
    FkProg *p = ls->p;
    FkOut  *o = &p->out;
    if (!ls->lng && ls->format!='1') {
        Arena save = *p->a;
        fklscolumns(ls, ents);
        *p->a = save;
        return;
    }
    if (!ls->lng) {
        for (iz i = 0; i < ents.len; i++) {
            print(o, fklsname(ls, ents[i].name));
            putbyte(o, '\n');
        }
        return;
    }
    iz  wlink = 1, wuser = 1, wgroup = 1, wsize = 1;
    i64 blocks = 0;
    for (iz i = 0; i < ents.len; i++) {
        Stat *st = &ents[i].st;
        iz n = fkdigits(st->nlink);
        wlink  = n>wlink ? n : wlink;
        n = fklsuser(ls, st->uid).len;
        wuser  = n>wuser ? n : wuser;
        n = fklsgroup(ls, st->gid).len;
        wgroup = n>wgroup ? n : wgroup;
        n = fkdigits(st->size);
        wsize  = n>wsize ? n : wsize;
        if (st->type == FT_FILE) blocks += (st->size + 4095) / 4096 * 4;
    }
    if (total) {
        print(o, "total ");
        print(o, blocks);
        putbyte(o, '\n');
    }
    for (iz i = 0; i < ents.len; i++) {
        Stat *st = &ents[i].st;
        u8 type = '-';
        switch (st->type) {
        case FT_DIR:   type = 'd'; break;
        case FT_LINK:  type = 'l'; break;
        case FT_FIFO:
        case FT_PIPE:  type = 'p'; break;
        case FT_CHAR:  type = 'c'; break;
        case FT_BLOCK: type = 'b'; break;
        case FT_SOCK:  type = 's'; break;
        }
        putbyte(o, type);
        fkmodestr(o, st->mode);
        putbyte(o, ' ');
        printpad(o, st->nlink, (i32)wlink, ' ');
        if (!ls->noowner) {
            Str u = fklsuser(ls, st->uid);
            putbyte(o, ' ');
            print(o, u);
            for (iz k = u.len; k < wuser; k++) putbyte(o, ' ');
        }
        if (!ls->nogroup) {
            Str g = fklsgroup(ls, st->gid);
            putbyte(o, ' ');
            print(o, g);
            for (iz k = g.len; k < wgroup; k++) putbyte(o, ' ');
        }
        putbyte(o, ' ');
        printpad(o, st->size, (i32)wsize, ' ');
        putbyte(o, ' ');
        i64 t = fkfloordiv(st->mtime, 1000000000);
        b32 recent = t>ls->now-31556952/2 && t<=ls->now;
        Arena scratch = *p->a;
        Buf   b(&scratch, 32);
        fkstrftime(&b, recent ? Str("%b %e %H:%M") : Str("%b %e  %Y"), t, 0);
        print(o, finish(&b));
        putbyte(o, ' ');
        print(o, fklsname(ls, ents[i].name));
        if (st->type == FT_LINK) {
            Str target = os_readlink(p->os, &scratch, ents[i].path);
            if (target.data) {
                print(o, " -> ");
                print(o, target);
            }
        }
        putbyte(o, '\n');
    }
}

static i32 fake_ls(Os *os, FakeCtx *ctx)
{
    FkProg p  = fkprog(os, ctx);
    FkOpt  o  = fkopts(&p, 1);
    FkLs   ls = {};
    ls.p = &p;
    ls.format = '1';
    ls.cols   = 80;
    Str   colenv = fkgetenv(ctx, "COLUMNS");
    FkNum cols   = fkparseint(colenv);
    if (cols.ok && cols.v>=0 && colenv[0]!='+' && colenv[0]!='-') {
        ls.cols = cols.v;
    } else if (colenv.len) {
        print(diag(&p), "ignoring invalid width in environment variable COLUMNS: ");
        quote(&p.err, colenv);
        enddiag(&p);
    }
    for (i32 ch; (ch = fkgetopt(&o, "1aACdfglnoqrtUx")) != -1;) {
        switch (ch) {
        case '1':
        case 'C':
        case 'x':
            ls.format = (u8)ch;
            ls.lng    = 0;
            break;
        case 'q': ls.qmark = 1; break;
        case 'a': ls.all = 1; break;
        case 'A': ls.almost = 1; break;
        case 'd': ls.dirs = 1; break;
        case 'f': ls.all = 1; break;
        case 'g': ls.lng = ls.noowner = 1; break;
        case 'l': ls.lng = 1; break;
        case 'n': ls.lng = ls.numeric = 1; break;
        case 'o': ls.lng = ls.nogroup = 1; break;
        case 'r': ls.rev = 1; break;
        case 't': ls.bytime = 1; break;
        case 'U': break;
        default:  return fkusage(&p, 2);
        }
    }
    ls.now = fkfloordiv(os_now(os), 1000000000);
    if (!o.nrest) o.rest[o.nrest++] = ".";
    i32 status = 0;
    Slice<FkEnt> files = {};
    Slice<FkEnt> dirs  = {};
    for (iz i = 0; i < o.nrest; i++) {
        Str  name = o.rest[i];
        Str  path = fkpath(&p, name);
        Stat st   = os_stat(os, *p.a, path, !ls.dirs && !ls.lng);
        if (st.err && !ls.dirs) {
            Stat lt = os_stat(os, *p.a, path, 0);
            if (!lt.err) st = lt;
        }
        if (st.err) {
            print(diag(&p), "cannot access ");
            quoteaf(&p.err, name);
            enddiag(&p, st.err);
            status = 2;
            continue;
        }
        if (ls.lng && st.type==FT_LINK && !ls.dirs) {
            // -l lists a link to a directory as the directory
            Stat ft = os_stat(os, *p.a, path, 1);
            if (!ft.err && ft.type==FT_DIR && endswith(name, "/")) st = ft;
        }
        FkEnt e = {name, path, st};
        if (st.type==FT_DIR && !ls.dirs) {
            dirs = push(p.a, dirs, e);
        } else {
            files = push(p.a, files, e);
        }
    }
    fklssort(&ls, files);
    fklssort(&ls, dirs);
    fklsprint(&ls, files, 0);
    b32 headers = o.nrest > 1;
    for (iz i = 0; i < dirs.len; i++) {
        Arena save = *p.a;
        Str   name = dirs[i].name;
        Str   path = dirs[i].path;
        i32   err  = 0;
        Dirent *list = os_list(os, p.a, path, &err);
        if (err) {
            print(diag(&p), "cannot open directory ");
            quoteaf(&p.err, name);
            enddiag(&p, err);
            status = 2;
            *p.a = save;
            continue;
        }
        if (files.len || i) putbyte(&p.out, '\n');
        if (headers) {
            print(&p.out, name);
            print(&p.out, ":\n");
        }
        Slice<FkEnt> ents = {};
        if (ls.all) {
            ents = push(p.a, ents, FkEnt{".", path, {}});
            ents = push(p.a, ents, FkEnt{"..", fkjoin(p.a, path, ".."), {}});
        }
        for (Dirent *d = list; d; d = d->next) {
            if (d->name=="." || d->name=="..") continue;
            if (d->name.len && d->name[0]=='.' && !ls.all && !ls.almost) continue;
            ents = push(p.a, ents, FkEnt{d->name, fkjoin(p.a, path, d->name), {}});
        }
        if (ls.bytime || ls.lng) {
            for (iz j = 0; j < ents.len; j++) {
                ents[j].st = os_stat(os, *p.a, ents[j].path, 0);
            }
        }
        fklssort(&ls, ents);
        fklsprint(&ls, ents, 1);
        *p.a = save;
    }
    return fkexit(&p, status);
}


// wc

struct FkCounts {
    i64 lines;
    i64 words;
    i64 chars;
    i64 bytes;
    i64 maxlen;
};

// Count all of s. In UTF-8, invalid bytes are no characters but join
// words, and no-break spaces separate words, as in GNU's wc.
static FkCounts fkcount(Str s, b32 utf8)
{
    FkCounts c = {};
    b32      inword = 0;
    i64      col = 0;
    c.bytes = s.len;
    for (iz i = 0; i < s.len;) {
        FkChar ch = utf8 && s[i]>=0x80 ? fkchar(s, i) : FkChar{s[i], 1};
        i32    k  = ch.c;
        i += ch.len;
        c.chars += k >= 0;
        if (k=='\n' || k=='\r' || k=='\f') {
            c.lines += k == '\n';
            c.maxlen = col>c.maxlen ? col : c.maxlen;
            col = 0;
        } else if (k == '\t') {
            col = (col/8 + 1) * 8;
        } else if (k>=0x80 && utf8) {
            i32 w = fkwidth(k);
            col += w>0 ? w : 0;
        } else if (k>=' ' && k<0x7f) {
            col++;
        }
        b32 space = k>=0 && k<0x80 ? whitespace((u8)k)
                  : utf8 && k>=0 && (fkwspace(k) || k==0xa0 || k==0x2007
                                     || k==0x202f || k==0x2060);
        if (space) {
            inword = 0;
        } else if (!inword) {
            inword = 1;
            c.words++;
        }
    }
    c.maxlen = col>c.maxlen ? col : c.maxlen;
    return c;
}

static i32 fake_wc(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    show[5] = {};  // lines words chars bytes maxlen
    for (i32 ch; (ch = fkgetopt(&o, "clmwL")) != -1;) {
        switch (ch) {
        case 'l': show[0] = 1; break;
        case 'w': show[1] = 1; break;
        case 'm': show[2] = 1; break;
        case 'c': show[3] = 1; break;
        case 'L': show[4] = 1; break;
        default:  return fkusage(&p, 1);
        }
    }
    i32 nshow = show[0] + show[1] + show[2] + show[3] + show[4];
    if (!nshow) {
        show[0] = show[1] = show[3] = 1;
        nshow = 3;
    }
    b32 named = o.nrest > 0;
    if (!named) o.rest[o.nrest++] = "-";

    // Column width from the sizes of regular files, as GNU computes it
    i32 width = 1;
    if (!(o.nrest==1 && nshow==1)) {
        i32 minimum = 1;
        i64 total = 0;
        for (iz i = 0; i < o.nrest; i++) {
            Stat st = {};
            if (o.rest[i] == "-") {
                st = os_fstat(os, ctx->fd[0]);
            } else {
                st = os_stat(os, *p.a, fkpath(&p, o.rest[i]), 1);
            }
            if (st.err) continue;
            if (st.type != FT_FILE) {
                minimum = 7;
            } else {
                total += st.size;
            }
        }
        for (; total >= 10; total /= 10) width++;
        width = width>minimum ? width : minimum;
    }

    i32      status = 0;
    FkCounts sum = {};
    for (iz i = 0; i < o.nrest; i++) {
        FkCounts c = {};
        FkIn in = fkopenin(&p, o.rest[i]);
        if (in.err) {
            quotef(diag(&p), o.rest[i]);
            enddiag(&p, in.err);
            status = 1;
            continue;
        }
        if (in.dir) {
            quotef(diag(&p), o.rest[i]);
            enddiag(&p, E_ISDIR);
            status = 1;
        } else {
            Arena scratch = *p.a;
            Str   data = fkreadall(&p, in.h);
            c = fkcount(data, p.utf8);
            *p.a = scratch;
        }
        fkclosein(&p, &in);
        i64 v[5] = {c.lines, c.words, c.chars, c.bytes, c.maxlen};
        b32 first = 1;
        for (i32 k = 0; k < 5; k++) {
            if (!show[k]) continue;
            if (!first) putbyte(&p.out, ' ');
            first = 0;
            printpad(&p.out, v[k], width, ' ');
        }
        if (named) {
            putbyte(&p.out, ' ');
            print(&p.out, o.rest[i]);
        }
        putbyte(&p.out, '\n');
        sum.lines  += c.lines;
        sum.words  += c.words;
        sum.chars  += c.chars;
        sum.bytes  += c.bytes;
        sum.maxlen  = c.maxlen>sum.maxlen ? c.maxlen : sum.maxlen;
    }
    if (o.nrest > 1) {
        i64 v[5] = {sum.lines, sum.words, sum.chars, sum.bytes, sum.maxlen};
        b32 first = 1;
        for (i32 k = 0; k < 5; k++) {
            if (!show[k]) continue;
            if (!first) putbyte(&p.out, ' ');
            first = 0;
            printpad(&p.out, v[k], width, ' ');
        }
        print(&p.out, " total\n");
    }
    return fkexit(&p, status);
}


// head and tail

struct FkCount {
    i64 n;
    b32 neg;    // leading - (head: all but the last n)
    b32 plus;   // leading + (tail: starting at n)
    b32 ok;
};

static FkCount fkparsecount(Str s)
{
    FkCount r = {};
    i64     lim = (i64)1 << 62;
    s = trimleft(s);
    iz i = 0;
    if (i<s.len && s[i]=='-') {
        r.neg = 1;
        i++;
    } else if (i<s.len && s[i]=='+') {
        r.plus = 1;
        i++;
    }
    if (i == s.len) return r;
    i64 v = 0;
    for (; i<s.len && digit(s[i]); i++) {
        v = v>lim/10 ? lim : v*10 + s[i] - '0';  // huge is unlimited
    }
    i64 mult = 1;
    if (i < s.len) {
        Str suf = cuthead(s, i);
        if      (suf == "b")  mult = 512;
        else if (suf == "kB") mult = 1000;
        else if (suf == "K" || suf == "k" || suf == "KiB") mult = 1024;
        else if (suf == "MB") mult = 1000*1000;
        else if (suf == "M" || suf == "MiB") mult = 1024*1024;
        else return r;
    }
    r.n  = v>lim/mult ? lim : v*mult;
    r.ok = 1;
    return r;
}

static void fkheader(FkProg *p, Str name, b32 first)
{
    if (!first) putbyte(&p->out, '\n');
    print(&p->out, "==> ");
    print(&p->out, name=="-" ? Str("standard input") : name);
    print(&p->out, " <==\n");
}

// Rewrite an obsolete leading "-N" or "+N" option as "-n N" or "-n +N".
static b32 fkobsolete(FkProg *p, b32 tail)  // false after a diagnostic
{
    FakeCtx *ctx = p->ctx;
    if (ctx->argc < 2) return 1;
    Str a = ctx->argv[1];
    if (a.len<2 || (a[0]!='-' && !(tail && a[0]=='+'))) return 1;
    iz nd = 1;
    for (; nd<a.len && digit(a[nd]); nd++) {}
    Str num  = span(a.data+1, a.data+nd);
    Str rest = cuthead(a, nd);
    u8  unit = 'l';
    if (rest.len && (rest[0]=='c' || rest[0]=='l' || rest[0]=='b')) {
        unit = rest[0];
        rest = cuthead(rest, 1);
    }
    if (rest.len) return 1;
    if (!num.len && (!tail || (a[0]=='-' && unit=='c'))) return 1;
    if (tail) {
        // Only with at most one file operand, as POSIX had it
        Str *v = ctx->argv;
        b32 alone = ctx->argc==2 || (ctx->argc==3 && !(v[2].len>1 && v[2][0]=='-'))
                 || (ctx->argc<=4 && v[2]=="--");
        if (!alone) {
            if (a[0] == '+') return 1;
            print(diag(p), "option used in invalid context -- ");
            putbyte(&p->err, a[1]);
            enddiag(p);
            return 0;
        }
    }
    Str count = num.len ? num : Str("10");
    if (unit == 'b') count = concat(p->a, count, Str("b"));
    if (a[0] == '+') count = concat(p->a, Str("+"), count);
    Str *argv = alloc<Str>(p->a, ctx->argc+1);
    argv[0] = ctx->argv[0];
    argv[1] = unit=='l' ? Str("-n") : Str("-c");
    argv[2] = count;
    for (iz i = 2; i < ctx->argc; i++) argv[i+1] = ctx->argv[i];
    ctx->argv = argv;
    ctx->argc++;
    if (!tail) {
        for (iz i = 3; i<ctx->argc && !(argv[i]=="--"); i++) {
            if (argv[i].len>1 && argv[i][0]=='-' && digit(argv[i][1])) {
                print(diag(p), "invalid trailing option -- ");
                putbyte(&p->err, argv[i][1]);
                enddiag(p);
                fkusage(p, 1);
                return 0;
            }
        }
    }
    return 1;
}

static i32 fake_head(Os *os, FakeCtx *ctx)
{
    FkProg  p = fkprog(os, ctx);
    if (!fkobsolete(&p, 0)) return fkexit(&p, 1);
    FkOpt   o = fkopts(&p, 1);
    FkCount count = {10, 0, 0, 1};
    b32     bytes = 0;
    i32     headers = 0;  // 0 auto, 1 never, 2 always
    u8      eol = '\n';
    for (i32 ch; (ch = fkgetopt(&o, "n:c:qvz")) != -1;) {
        switch (ch) {
        case 'n':
        case 'c':
            count = fkparsecount(o.arg);
            bytes = ch == 'c';
            if (!count.ok) {
                print(diag(&p), bytes ? Str("invalid number of bytes: ")
                                      : Str("invalid number of lines: "));
                quote(&p.err, o.arg);
                enddiag(&p);
                return fkexit(&p, 1);
            }
            break;
        case 'q': headers = 1; break;
        case 'v': headers = 2; break;
        case 'z': eol = 0; break;
        default:  return fkusage(&p, 1);
        }
    }
    if (!o.nrest) o.rest[o.nrest++] = "-";
    b32 show = headers==2 || (headers==0 && o.nrest>1);
    i32 status = 0;
    b32 first = 1;
    u8 *buf = allocbytes(p.a, 1<<16);
    for (iz i = 0; i < o.nrest; i++) {
        FkIn in = fkopenin(&p, o.rest[i]);
        if (!in.err && in.dir) {
            fkclosein(&p, &in);
            in.err = E_ISDIR;
            if (show) fkheader(&p, o.rest[i], first);
            first = 0;
            print(diag(&p), "error reading ");
            quoteaf(&p.err, o.rest[i]);
            enddiag(&p, E_ISDIR);
            status = 1;
            continue;
        }
        if (in.err) {
            print(diag(&p), "cannot open ");
            quoteaf(&p.err, o.rest[i]);
            print(&p.err, " for reading");
            enddiag(&p, in.err);
            status = 1;
            continue;
        }
        if (show) fkheader(&p, o.rest[i], first);
        first = 0;
        if (count.neg) {
            Str s = fkreadall(&p, in.h);
            iz  end = s.len;
            if (bytes) {
                end = count.n<s.len ? s.len-(iz)count.n : 0;
            } else {
                i64 total = 0;
                for (iz j = 0; j < s.len; j++) total += s[j]==eol;
                if (s.len && s[s.len-1]!=eol) total++;
                i64 keep = total - count.n;
                end = 0;
                for (; end<s.len && keep>0; end++) {
                    if (s[end] == eol) keep--;
                }
            }
            print(&p.out, takehead(s, end));
        } else {
            i64 left = count.n;
            while (left > 0) {
                iz n = os_read(os, in.h, buf, 1<<16);
                if (n <= 0) break;
                iz take = 0;
                if (bytes) {
                    take = left<n ? (iz)left : n;
                    left -= take;
                } else {
                    for (; take<n && left; take++) {
                        if (buf[take] == eol) left--;
                    }
                }
                print(&p.out, {buf, take});
                if (take < n) os_seek(os, in.h, take-n, 1);
            }
        }
        fkclosein(&p, &in);
    }
    return fkexit(&p, status);
}

static i32 fake_tail(Os *os, FakeCtx *ctx)
{
    FkProg  p = fkprog(os, ctx);
    if (!fkobsolete(&p, 1)) return fkexit(&p, 1);
    FkOpt   o = fkopts(&p, 1);
    FkCount count = {10, 0, 0, 1};
    b32     bytes = 0;
    i32     headers = 0;
    u8      eol = '\n';
    for (i32 ch; (ch = fkgetopt(&o, "n:c:qvz")) != -1;) {
        switch (ch) {
        case 'n':
        case 'c':
            count = fkparsecount(o.arg);
            bytes = ch == 'c';
            if (!count.ok) {
                print(diag(&p), bytes ? Str("invalid number of bytes: ")
                                      : Str("invalid number of lines: "));
                quote(&p.err, o.arg);
                enddiag(&p);
                return fkexit(&p, 1);
            }
            break;
        case 'q': headers = 1; break;
        case 'v': headers = 2; break;
        case 'z': eol = 0; break;
        default:  return fkusage(&p, 1);
        }
    }
    if (!o.nrest) o.rest[o.nrest++] = "-";
    b32 show = headers==2 || (headers==0 && o.nrest>1);
    i32 status = 0;
    b32 first = 1;
    for (iz i = 0; i < o.nrest; i++) {
        FkIn in = fkopenin(&p, o.rest[i]);
        if (!in.err && in.dir) {
            fkclosein(&p, &in);
            if (show) fkheader(&p, o.rest[i], first);
            first = 0;
            print(diag(&p), "error reading ");
            quoteaf(&p.err, o.rest[i]);
            enddiag(&p, E_ISDIR);
            status = 1;
            continue;
        }
        if (in.err) {
            print(diag(&p), "cannot open ");
            quoteaf(&p.err, o.rest[i]);
            print(&p.err, " for reading");
            enddiag(&p, in.err);
            status = 1;
            continue;
        }
        if (show) fkheader(&p, o.rest[i], first);
        first = 0;
        Str s = fkreadall(&p, in.h);
        fkclosein(&p, &in);
        iz  beg = 0;
        if (bytes) {
            if (count.plus) {
                beg = count.n>1 ? (count.n-1<s.len ? (iz)count.n-1 : s.len) : 0;
            } else {
                beg = count.n<s.len ? s.len-(iz)count.n : 0;
            }
        } else if (count.plus) {
            i64 k = count.n>1 ? count.n-1 : 0;
            for (; beg<s.len && k; beg++) {
                if (s[beg] == eol) k--;
            }
        } else {
            i64 k = count.n;
            iz  j = s.len;
            if (j && s[j-1]==eol) j--;
            beg = 0;
            if (!k) {
                beg = s.len;
            }
            for (; k && j > 0; j--) {
                if (s[j-1] == eol && !--k) {
                    beg = j;
                    break;
                }
            }
        }
        print(&p.out, cuthead(s, beg));
    }
    return fkexit(&p, status);
}


// sort

struct FkKey {
    i64 sword;   // starting field, from 0
    i64 schar;   // starting character in it, from 0
    i64 eword;   // ending field, or -1 for the end of line
    i64 echar;   // ending character, 0 for the end of the field
    b32 skipsb;  // b on the start
    b32 skipeb;  // b on the end
    b32 numeric;
    b32 fold;
    b32 reverse;
    b32 plain;   // no options of its own
};

struct FkSort {
    FkKey *keys;
    iz     nkeys;
    FkKey  global;
    i32    tab;      // -1 for blank-separated fields
    b32    unique;
    b32    stable;
};

static FkSort *fksortctx;  // comparison context for the merge sort

static b32 fkblank(u8 c)
{
    return c==' ' || c=='\t';
}

static Str fkkeytext(Str line, FkKey *k, i32 tab)
{
    iz ptr = 0, lim = line.len;
    for (i64 w = k->sword; ptr<lim && w; w--) {
        if (tab >= 0) {
            for (; ptr<lim && line[ptr]!=tab; ptr++) {}
            if (ptr < lim) ptr++;
        } else {
            for (; ptr<lim && fkblank(line[ptr]); ptr++) {}
            for (; ptr<lim && !fkblank(line[ptr]); ptr++) {}
        }
    }
    if (k->skipsb) {
        for (; ptr<lim && fkblank(line[ptr]); ptr++) {}
    }
    iz beg = ptr+k->schar<lim ? ptr+(iz)k->schar : lim;

    iz end = lim;
    if (k->eword >= 0) {
        iz  e = 0;
        i64 w = k->eword + (k->echar == 0);
        for (; e<lim && w; w--) {
            if (tab >= 0) {
                for (; e<lim && line[e]!=tab; e++) {}
                if (e<lim && (w>1 || k->echar)) e++;
            } else {
                for (; e<lim && fkblank(line[e]); e++) {}
                for (; e<lim && !fkblank(line[e]); e++) {}
            }
        }
        if (k->echar) {
            if (k->skipeb) {
                for (; e<lim && fkblank(line[e]); e++) {}
            }
            e = e+k->echar<lim ? e+(iz)k->echar : lim;
        }
        end = e;
    }
    return beg<end ? span(line.data+beg, line.data+end) : Str{line.data+beg, 0};
}

// Compare leading decimal numbers, as sort -n does in the C locale.
static i32 fknumcmp(Str a, Str b)
{
    Str s[2] = {a, b};
    Str ip[2], fp[2];
    i32 sign[2];
    for (i32 k = 0; k < 2; k++) {
        Str t = s[k];
        iz  i = 0;
        for (; i<t.len && fkblank(t[i]); i++) {}
        b32 neg = i<t.len && t[i]=='-';
        i += neg;
        for (; i<t.len && t[i]=='0'; i++) {}
        iz j = i;
        for (; j<t.len && digit(t[j]); j++) {}
        ip[k] = span(t.data+i, t.data+j);
        fp[k] = {};
        if (j<t.len && t[j]=='.') {
            iz f = j + 1;
            iz e = f;
            for (; e<t.len && digit(t[e]); e++) {}
            for (; e>f && t[e-1]=='0'; e--) {}
            fp[k] = span(t.data+f, t.data+e);
        }
        b32 zero = !ip[k].len && !fp[k].len;
        sign[k] = zero ? 0 : neg ? -1 : 1;
    }
    if (sign[0] != sign[1]) return sign[0]<sign[1] ? -1 : 1;
    i32 r = 0;
    if (ip[0].len != ip[1].len) {
        r = ip[0].len<ip[1].len ? -1 : 1;
    } else {
        r = compare(ip[0], ip[1]);
        if (!r) {
            iz n = fp[0].len>fp[1].len ? fp[0].len : fp[1].len;
            for (iz i = 0; i<n && !r; i++) {
                u8 x = i<fp[0].len ? fp[0][i] : '0';
                u8 y = i<fp[1].len ? fp[1][i] : '0';
                r = x<y ? -1 : x>y;
            }
        }
    }
    r = r<0 ? -1 : r>0;
    return sign[0]<0 ? -r : r;
}

static i32 fkfoldcmp(Str a, Str b)
{
    iz n = a.len<b.len ? a.len : b.len;
    for (iz i = 0; i < n; i++) {
        u8 x = a[i]>='a' && a[i]<='z' ? (u8)(a[i]-32) : a[i];
        u8 y = b[i]>='a' && b[i]<='z' ? (u8)(b[i]-32) : b[i];
        if (x != y) return x<y ? -1 : 1;
    }
    return a.len<b.len ? -1 : a.len>b.len;
}

static i32 fkkeycmp(FkSort *s, Str a, Str b)
{
    FkKey whole = s->global;
    whole.eword = -1;
    FkKey *keys = s->nkeys ? s->keys : &whole;
    iz     n    = s->nkeys ? s->nkeys : 1;
    for (iz i = 0; i < n; i++) {
        FkKey *k = keys + i;
        Str x = s->nkeys ? fkkeytext(a, k, s->tab) : a;
        Str y = s->nkeys ? fkkeytext(b, k, s->tab) : b;
        if (!s->nkeys && k->skipsb) {
            for (; x.len && fkblank(x[0]); x = cuthead(x, 1)) {}
            for (; y.len && fkblank(y[0]); y = cuthead(y, 1)) {}
        }
        i32 r = k->numeric ? fknumcmp(x, y)
              : k->fold    ? fkfoldcmp(x, y)
              :              compare(x, y);
        if (r) return k->reverse ? -r : r;
    }
    return 0;
}

static i32 fklinecmp(Str a, Str b)
{
    FkSort *s = fksortctx;
    i32 r = fkkeycmp(s, a, b);
    if (r || s->unique || s->stable) return r;
    r = compare(a, b);
    return s->global.reverse ? -r : r;
}

static b32 fklineless(Str *a, Str *b)
{
    return fklinecmp(*a, *b) < 0;
}

// Parse a key field spec "F[.C][opts]", returning the rest, or else a
// message and the text it concerns, as GNU reports them.
struct FkKeyErr {
    Str msg;
    Str at;     // "invalid count at start of" this, or null data
};

static Str fkkeypos(Str s, i64 *field, i64 *chr, FkKey *k, b32 end, FkKeyErr *err)
{
    iz  i = 0;
    i64 f = 0, c = 0;
    for (; i<s.len && digit(s[i]); i++) {
        f = f>(i64)1<<40 ? f : f*10 + s[i] - '0';
    }
    if (!i) {
        err->msg = end ? Str("invalid number after ','") : Str("invalid number at field start");
        err->at  = s;
        return s;
    }
    if (!f) {
        err->msg = "field number is zero";
        return s;
    }
    if (i<s.len && s[i]=='.') {
        iz j = ++i;
        for (; i<s.len && digit(s[i]); i++) {
            c = c>(i64)1<<40 ? c : c*10 + s[i] - '0';
        }
        if (i == j) {
            err->msg = "invalid number after '.'";
            err->at  = cuthead(s, i);
            return s;
        }
        if (!c && !end) {
            err->msg = "character offset is zero";
            return s;
        }
    }
    for (; i<s.len && s[i]!=','; i++) {
        switch (s[i]) {
        case 'b': if (end) k->skipeb = 1; else k->skipsb = 1; break;
        case 'n': k->numeric = 1; break;
        case 'f': k->fold = 1; break;
        case 'r': k->reverse = 1; break;
        default:
            err->msg = "stray character in field spec";
            return s;
        }
        k->plain = 0;
    }
    *field = f;
    *chr   = c;
    return cuthead(s, i);
}

static i32 fake_sort(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    FkSort s = {};
    s.tab  = -1;
    s.keys = alloc<FkKey>(p.a, ctx->argc);
    Str  outname = {};
    b32  check = 0, merge = 0;
    u8   eol = '\n';
    for (i32 ch; (ch = fkgetopt(&o, "bcfk:mno:rst:uz")) != -1;) {
        switch (ch) {
        case 'm': merge = 1; break;
        case 'b': s.global.skipsb = s.global.skipeb = 1; break;
        case 'c': check = 1; break;
        case 'f': s.global.fold = 1; break;
        case 'n': s.global.numeric = 1; break;
        case 'r': s.global.reverse = 1; break;
        case 's': s.stable = 1; break;
        case 'u': s.unique = 1; break;
        case 'z': eol = 0; break;
        case 'o': outname = o.arg; break;
        case 't':
            if (o.arg.len != 1) {
                print(diag(&p), o.arg.len ? Str("multi-character tab ") : Str("empty tab"));
                if (o.arg.len) quote(&p.err, o.arg);
                enddiag(&p);
                return fkexit(&p, 2);
            }
            s.tab = o.arg[0];
            break;
        case 'k': {
            FkKey *k = s.keys + s.nkeys++;
            k->plain = 1;
            k->eword = -1;
            FkKeyErr err = {};
            Str r = fkkeypos(o.arg, &k->sword, &k->schar, k, 0, &err);
            if (!err.msg.data && r.len && r[0]==',') {
                i64 ef = 0, ec = 0;
                r = fkkeypos(cuthead(r, 1), &ef, &ec, k, 1, &err);
                k->eword = ef - 1;
                k->echar = ec;
            }
            if (!err.msg.data && r.len) err.msg = "stray character in field spec";
            if (err.msg.data) {
                print(diag(&p), err.msg);
                if (err.at.data) {
                    print(&p.err, ": invalid count at start of ");
                    quote(&p.err, err.at);
                } else {
                    print(&p.err, ": invalid field specification ");
                    quote(&p.err, o.arg);
                }
                enddiag(&p);
                return fkexit(&p, 2);
            }
            k->sword--;
            k->schar = k->schar ? k->schar-1 : 0;
        } break;
        default:
            return fkusage(&p, 2);
        }
    }
    for (iz i = 0; i < s.nkeys; i++) {
        FkKey *k = s.keys + i;
        if (k->plain) {
            k->numeric = s.global.numeric;
            k->fold    = s.global.fold;
            k->reverse = s.global.reverse;
            k->skipsb  = s.global.skipsb;
            k->skipeb  = s.global.skipeb;
        }
    }
    if (!o.nrest) o.rest[o.nrest++] = "-";
    Slice<Str> lines  = {};
    iz        *starts = alloc<iz>(p.a, o.nrest+1);
    for (iz i = 0; i < o.nrest; i++) {
        starts[i] = lines.len;
        FkIn in = fkopenin(&p, o.rest[i]);
        Str  data = {};
        if (!in.err && in.dir) {
            fkclosein(&p, &in);
            in.err = E_ISDIR;
        }
        if (!in.err) {
            data = fkreadall(&p, in.h);
            fkclosein(&p, &in);
        }
        if (in.err) {
            print(diag(&p), "cannot read: ");
            quotef(&p.err, o.rest[i]);
            enddiag(&p, in.err);
            return fkexit(&p, 2);
        }
        while (data.len) {
            FkLine l = fknextline(data, eol);
            lines = push(p.a, lines, l.line);
            data  = l.rest;
        }
    }
    fksortctx = &s;
    if (check) {
        for (iz i = 1; i < lines.len; i++) {
            i32 r = fklinecmp(lines[i-1], lines[i]);
            if (r>0 || (s.unique && !r)) {
                print(diag(&p), o.rest[0]);
                putbyte(&p.err, ':');
                print(&p.err, (i64)i+1);
                print(&p.err, ": disorder: ");
                print(&p.err, lines[i]);
                enddiag(&p);
                return fkexit(&p, 1);
            }
        }
        return fkexit(&p, 0);
    }
    starts[o.nrest] = lines.len;
    if (merge) {
        // Inputs are already sorted: take the least head each time,
        // the earliest input's on a tie
        Slice<Str> m = {};
        iz *at = alloc<iz>(p.a, o.nrest);
        for (iz i = 0; i < o.nrest; i++) at[i] = starts[i];
        for (;;) {
            iz best = -1;
            for (iz i = 0; i < o.nrest; i++) {
                if (at[i] == starts[i+1]) continue;
                if (best<0 || fklinecmp(lines[at[i]], lines[at[best]])<0) best = i;
            }
            if (best < 0) break;
            m = push(p.a, m, lines[at[best]++]);
        }
        lines = m;
    } else {
        sort(lines.data, lines.len, fklineless, *p.a);
    }

    FkOut *out = &p.out;
    FkOut  file = {};
    if (outname.data) {
        Opened f = os_open(os, *p.a, fkpath(&p, outname), OF_WRITE|OF_CREATE|OF_TRUNC, 0666);
        if (f.err) {
            print(diag(&p), "open failed: ");
            quotef(&p.err, outname);
            enddiag(&p, f.err);
            return fkexit(&p, 2);
        }
        file = fkout(os, p.a, f.h, 1<<12);
        out  = &file;
    }
    for (iz i = 0; i < lines.len; i++) {
        if (s.unique && i && !fkkeycmp(&s, lines[i-1], lines[i])) continue;
        print(out, lines[i]);
        putbyte(out, eol);
    }
    if (outname.data) {
        flush(&file);
        os_close(os, file.h);
    }
    return fkexit(&p, 0);
}


// uniq

struct FkUniq {
    i64 skipfields;
    i64 skipchars;
    i64 maxchars;   // -1 for all
    b32 fold;
};

static Str fkuniqkey(FkUniq *u, Str s)
{
    iz i = 0;
    for (i64 f = 0; f < u->skipfields; f++) {
        for (; i<s.len && fkblank(s[i]); i++) {}
        for (; i<s.len && !fkblank(s[i]); i++) {}
    }
    i = i+u->skipchars<s.len ? i+(iz)u->skipchars : s.len;
    s = cuthead(s, i);
    if (u->maxchars>=0 && u->maxchars<s.len) s = takehead(s, (iz)u->maxchars);
    return s;
}

static i32 fake_uniq(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    FkUniq u = {};
    u.maxchars = -1;
    b32    count = 0, dups = 0, uniqs = 0, all = 0;
    u8     eol = '\n';
    for (i32 ch; (ch = fkgetopt(&o, "cdDf:is:uw:z")) != -1;) {
        FkNum n = {};
        switch (ch) {
        case 'D': all = 1; break;
        case 'z': eol = 0; break;
        case 'c': count = 1; break;
        case 'd': dups  = 1; break;
        case 'u': uniqs = 1; break;
        case 'i': u.fold = 1; break;
        case 'f':
        case 's':
        case 'w':
            n = fkparseint(o.arg);
            if (!n.ok || n.v<0) {
                print(diag(&p), "");
                print(&p.err, o.arg);
                print(&p.err, ch=='w' ? Str(": invalid number of bytes to compare")
                            : ch=='s' ? Str(": invalid number of bytes to skip")
                            :           Str(": invalid number of fields to skip"));
                enddiag(&p);
                return fkexit(&p, 1);
            }
            if (ch == 'f') u.skipfields = n.v;
            if (ch == 's') u.skipchars  = n.v;
            if (ch == 'w') u.maxchars   = n.v;
            break;
        default:
            return fkusage(&p, 1);
        }
    }
    if (o.nrest > 2) {
        print(diag(&p), "extra operand ");
        quote(&p.err, o.rest[2]);
        enddiag(&p);
        return fkusage(&p, 1);
    }
    if (all && count) {
        print(diag(&p), "printing all duplicated lines and repeat counts is meaningless");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    Str data = fkslurp(&p, o.nrest ? o.rest[0] : Str("-"));
    if (!data.data) return fkexit(&p, 1);
    FkOut *out = &p.out;
    FkOut  file = {};
    if (o.nrest == 2) {
        Opened f = os_open(os, *p.a, fkpath(&p, o.rest[1]), OF_WRITE|OF_CREATE|OF_TRUNC, 0666);
        if (f.err) {
            quotef(diag(&p), o.rest[1]);
            enddiag(&p, f.err);
            return fkexit(&p, 1);
        }
        file = fkout(os, p.a, f.h, 1<<12);
        out  = &file;
    }
    Slice<Str> lines = fklines(p.a, data, eol);
    for (iz i = 0; i < lines.len;) {
        iz  j = i + 1;
        Str k = fkuniqkey(&u, lines[i]);
        for (; j < lines.len; j++) {
            Str k2 = fkuniqkey(&u, lines[j]);
            if (u.fold ? fkfoldcmp(k, k2) : compare(k, k2)) break;
        }
        i64 n = j - i;
        if (all) {
            for (iz k = i+uniqs; k<j && n>1; k++) {
                print(out, lines[k]);
                putbyte(out, eol);
            }
        } else if ((!dups || n>1) && (!uniqs || n==1)) {
            if (count) {
                printpad(out, n, 7, ' ');
                putbyte(out, ' ');
            }
            print(out, lines[i]);
            putbyte(out, eol);
        }
        i = j;
    }
    if (o.nrest == 2) {
        flush(&file);
        os_close(os, file.h);
    }
    return fkexit(&p, 0);
}


// cut

struct FkRanges {
    u8  *sel;    // selected positions 1..max
    i64  max;
    i64  from;   // everything from here on, or 0
    b32  ok;
};

static FkRanges fkparselist(FkProg *p, Str s, b32 fields)
{
    FkRanges r = {};
    Str      what = fields ? Str("field") : Str("byte/character position");
    // First pass: find the largest explicit endpoint
    i64 max = 0;
    for (Cut c = {{}, s, 1}; c.ok;) {
        c = cut(c.tail, ',');
        Cut d = cut(c.head, '-');
        FkNum a = fkparseint(d.head);
        FkNum b = fkparseint(d.tail);
        if (a.ok && a.v>max) max = a.v;
        if (b.ok && b.v>max) max = b.v;
    }
    if (max > 1<<24) max = 1<<24;
    r.sel = alloc<u8>(p->a, max+2);
    r.max = max;
    for (Cut c = {{}, s, 1}; c.ok;) {
        c = cut(c.tail, ',');
        Str item = c.head;
        Cut d = cut(item, '-');
        i64 lo = 1, hi = 0;
        b32 bad = 0;
        if (d.head.len) {
            FkNum a = fkparseint(d.head);
            bad |= !a.ok || d.head[0]=='+' || d.head[0]=='-';
            lo = a.v;
        }
        if (d.ok && d.tail.len) {
            FkNum b = fkparseint(d.tail);
            bad |= !b.ok || d.tail[0]=='+' || d.tail[0]=='-';
            hi = b.v;
        } else if (!d.ok) {
            hi = lo;
        }
        if (d.ok && !d.head.len && !d.tail.len) {
            print(diag(p), "invalid range with no endpoint: -");
            enddiag(p);
            return r;
        }
        if (d.ok && cut(d.tail, '-').ok) {
            print(diag(p), fields ? Str("invalid field range")
                                  : Str("invalid byte or character range"));
            enddiag(p);
            return r;
        }
        for (i32 k = 0; k<2 && bad; k++) {
            Str num = k ? d.tail : d.head;
            b32 all = num.len > 0;
            for (iz i = 0; i < num.len; i++) all &= digit(num[i]);
            if (all && !fkparseint(num).ok) {
                print(diag(p), fields ? Str("field number ") : Str("byte/character offset "));
                quote(&p->err, num);
                print(&p->err, " is too large");
                enddiag(p);
                return r;
            }
        }
        if (bad || !item.len) {
            if (!item.len) {
                print(diag(p), fields ? Str("fields are numbered from 1")
                                      : Str("byte/character positions are numbered from 1"));
            } else {
                print(diag(p), "invalid ");
                print(&p->err, what);
                print(&p->err, fields ? Str(" value ") : Str(" "));
                quote(&p->err, item);
            }
            enddiag(p);
            return r;
        }
        if ((d.head.len && lo<1) || (d.ok && d.tail.len && hi<1)) {
            print(diag(p), fields ? Str("fields are numbered from 1")
                                  : Str("byte/character positions are numbered from 1"));
            enddiag(p);
            return r;
        }
        if (d.ok && d.tail.len && hi<lo) {
            print(diag(p), "invalid decreasing range");
            enddiag(p);
            return r;
        }
        if (d.ok && !d.tail.len) {
            r.from = r.from && r.from<lo ? r.from : lo;
            continue;
        }
        for (i64 i = lo; i<=hi && i<=max; i++) r.sel[i] = 1;
    }
    r.ok = 1;
    return r;
}

static b32 fkselected(FkRanges *r, i64 i)
{
    return (r->from && i>=r->from) || (i<=r->max && r->sel[i]);
}

static i32 fake_cut(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    Str    list = {};
    b32    fields = 0, suppress = 0, nlists = 0;
    Str    delim = {};
    u8     eol = '\n';
    for (i32 ch; (ch = fkgetopt(&o, "b:c:d:f:nsz")) != -1;) {
        switch (ch) {
        case 'b':
        case 'c':
        case 'f':
            if (nlists++) {
                print(diag(&p), "only one list may be specified");
                enddiag(&p);
                return fkusage(&p, 1);
            }
            list   = o.arg;
            fields = ch == 'f';
            break;
        case 'd': delim = o.arg; break;
        case 's': suppress = 1; break;
        case 'n': break;
        case 'z': eol = 0; break;
        default:  return fkusage(&p, 1);
        }
    }
    if (!nlists) {
        print(diag(&p), "you must specify a list of bytes, characters, or fields");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    if (delim.data && !fields) {
        print(diag(&p), "an input delimiter may be specified only when operating on fields");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    if (suppress && !fields) {
        print(diag(&p), "suppressing non-delimited lines makes sense\n\tonly when operating on fields");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    if (delim.data && delim.len>1) {
        print(diag(&p), "the delimiter must be a single character");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    u8 d = delim.data ? (delim.len ? delim[0] : 0) : '\t';  // -d '' is NUL
    FkRanges r = fkparselist(&p, list, fields);
    if (!r.ok) return fkusage(&p, 1);
    if (!o.nrest) o.rest[o.nrest++] = "-";
    i32 status = 0;
    for (iz f = 0; f < o.nrest; f++) {
        Arena save = *p.a;
        Str data = fkslurp(&p, o.rest[f]);
        if (!data.data) {
            status = 1;
            continue;
        }
        while (data.len) {
            FkLine l = fknextline(data, eol);
            data = l.rest;
            Str s = l.line;
            if (fields && d==eol) {
                // The delimiter ends lines too: the input is one record
                s    = concat(p.a, s, data.len ? concat(p.a, Str(&eol, 1), data) : Str{});
                s    = s.len && s[s.len-1]==eol ? cuttail(s, 1) : s;
                data = {};
            }
            if (!fields) {
                for (iz i = 0; i < s.len; i++) {
                    if (fkselected(&r, i+1)) putbyte(&p.out, s[i]);
                }
                putbyte(&p.out, eol);
                continue;
            }
            if (!cut(s, d).ok) {
                if (!suppress) {
                    print(&p.out, s);
                    putbyte(&p.out, eol);
                }
                continue;
            }
            b32 any = 0;
            i64 n   = 1;
            for (Cut c = {{}, s, 1}; c.ok; n++) {
                c = cut(c.tail, d);
                if (!fkselected(&r, n)) continue;
                if (any) putbyte(&p.out, d);
                print(&p.out, c.head);
                any = 1;
            }
            putbyte(&p.out, eol);
        }
        *p.a = save;
    }
    return fkexit(&p, status);
}


// tr

struct FkSetClass {  // a [:class:] in a set, for tr's alignment checks
    iz  pos;  // index of its first character
    i32 cls;
};

struct FkSet {
    u8  *c;       // expanded characters
    iz   len;
    iz   fill;    // index of a [c*] to stretch, or -1
    b32  ok;
    b32  equiv;   // holds a [=c=]
    b32  endclass;  // ends with a [:class:]
    Slice<FkSetClass> classes;
};

static b32 fkclass(Str name, u8 c)
{
    if (name == "alnum")  return alnum(c);
    if (name == "alpha")  return letter(c);
    if (name == "blank")  return c==' ' || c=='\t';
    if (name == "cntrl")  return c<32 || c==127;
    if (name == "digit")  return digit(c);
    if (name == "graph")  return c>32 && c<127;
    if (name == "lower")  return c>='a' && c<='z';
    if (name == "print")  return c>=32 && c<127;
    if (name == "punct")  return c>32 && c<127 && !alnum(c);
    if (name == "space")  return whitespace(c);
    if (name == "upper")  return c>='A' && c<='Z';
    if (name == "xdigit") return digit(c) || ((c|32)>='a' && (c|32)<='f');
    return 0;
}

static b32 fkclassname(Str name)
{
    Str names[] = {"alnum", "alpha", "blank", "cntrl", "digit", "graph",
                   "lower", "print", "punct", "space", "upper", "xdigit"};
    for (iz i = 0; i < countof(names); i++) {
        if (names[i] == name) return 1;
    }
    return 0;
}

// Decode one possibly escaped character at s[*i].
static u8 fktrchar(FkProg *p, Str s, iz *i)
{
    u8 c = s[(*i)++];
    if (c != '\\') return c;
    if (*i == s.len) {
        print(diag(p), "warning: an unescaped backslash at end of string is not portable");
        enddiag(p);
        return '\\';
    }
    c = s[(*i)++];
    switch (c) {
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    }
    if (c>='0' && c<='7') {
        i32 v = c - '0';
        for (i32 n = 1; n<3 && *i<s.len && s[*i]>='0' && s[*i]<='7'; n++) {
            i32 t = v*8 + s[*i] - '0';
            if (t > 255) {
                print(diag(p), "warning: the ambiguous octal escape \\");
                print(&p->err, Str(s.data+*i-2, 3));
                print(&p->err, " is being\n\tinterpreted as the 2-byte sequence \\0");
                print(&p->err, Str(s.data+*i-2, 2));
                print(&p->err, ", ");
                putbyte(&p->err, s[*i]);
                enddiag(p);
                break;
            }
            v = t;
            (*i)++;
        }
        return (u8)v;
    }
    return c;
}

static FkSet fkparseset(FkProg *p, Str s, b32 second)
{
    FkSet r = {};
    r.fill = -1;
    r.c    = alloc<u8>(p->a, s.len*256 + 256);
    for (iz i = 0; i < s.len;) {
        r.endclass = 0;
        if (s[i]=='[' && i+1<s.len && s[i+1]==':') {
            iz j = i + 2;
            for (; j+1<s.len && !(s[j]==':' && s[j+1]==']'); j++) {}
            Str name = span(s.data+i+2, s.data+j);
            if (j+1 < s.len) {
                if (!fkclassname(name)) {
                    print(diag(p), "invalid character class ");
                    quote(&p->err, name);
                    enddiag(p);
                    return r;
                }
                FkSetClass k = {r.len, fkclassid(name)};
                r.classes = push(p->a, r.classes, k);
                for (i32 c = 0; c < 256; c++) {
                    if (fkclass(name, (u8)c)) r.c[r.len++] = (u8)c;
                }
                i = j + 2;
                r.endclass = i == s.len;
                continue;
            }
        }
        if (s[i]=='[' && i+2<s.len && s[i+1]=='=') {
            iz  j = i + 2;
            u8  c = fktrchar(p, s, &j);
            if (j+1<s.len && s[j]=='=' && s[j+1]==']') {
                r.c[r.len++] = c;
                r.equiv = 1;
                i = j + 2;
                continue;
            }
        }
        if (s[i]=='[' && i+2<s.len) {
            iz j = i + 1;
            u8 c = fktrchar(p, s, &j);
            iz k = j + 1;
            for (; k<s.len && s[k]!=']'; k++) {}
            if (j<s.len && s[j]=='*' && k<s.len) {
                Str num = span(s.data+j+1, s.data+k);
                i64 n   = 0;
                b32 oct = num.len && num[0]=='0';
                for (iz m = 0; m < num.len; m++) {
                    if (!digit(num[m]) || (oct && num[m]>'7')) {
                        print(diag(p), "invalid repeat count ");
                        quote(&p->err, num);
                        print(&p->err, " in [c*n] construct");
                        enddiag(p);
                        return r;
                    }
                    n = n*(oct ? 8 : 10) + num[m] - '0';
                    if (n > 1<<20) n = 1<<20;
                }
                if (!second && !n) {
                    print(diag(p), "the [c*] repeat construct may not appear in string1");
                    enddiag(p);
                    return r;
                }
                if (!n) {
                    r.fill = r.len;
                    r.c[r.len++] = c;
                } else {
                    u8 *c2 = alloc<u8>(p->a, r.len + n + s.len*256 + 256);
                    copybytes(c2, r.c, r.len);
                    r.c = c2;
                    for (i64 m = 0; m < n; m++) r.c[r.len++] = c;
                }
                i = k + 1;
                continue;
            }
        }
        iz j = i;
        u8 c = fktrchar(p, s, &j);
        if (j+1<s.len && s[j]=='-') {
            iz k = j + 1;
            u8 e = fktrchar(p, s, &k);
            if (e < c) {
                print(diag(p), "range-endpoints of ");
                quote(&p->err, span(s.data+i, s.data+k));
                print(&p->err, " are in reverse collating sequence order");
                enddiag(p);
                return r;
            }
            for (i32 x = c; x <= e; x++) r.c[r.len++] = (u8)x;
            i = k;
            continue;
        }
        r.c[r.len++] = c;
        i = j;
    }
    r.ok = 1;
    return r;
}

static i32 fake_tr(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    comp = 0, del = 0, squeeze = 0, trunc = 0;
    for (i32 ch; (ch = fkgetopt(&o, "cCdst")) != -1;) {
        switch (ch) {
        case 'c': case 'C': comp = 1; break;
        case 'd': del = 1; break;
        case 's': squeeze = 1; break;
        case 't': trunc = 1; break;
        default:  return fkusage(&p, 1);
        }
    }
    iz want = del&&!squeeze ? 1 : !del&&squeeze ? 1 : 2;
    if (!o.nrest) {
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 1);
    }
    if (o.nrest<want || (!del && !squeeze && o.nrest<2)) {
        print(diag(&p), "missing operand after ");
        quote(&p.err, o.rest[o.nrest-1]);
        enddiag(&p);
        print(&p.err, del ? Str("Two strings must be given when both deleting and squeezing repeats.\n")
                          : Str("Two strings must be given when translating.\n"));
        return fkusage(&p, 1);
    }
    iz maxops = del && !squeeze ? 1 : 2;
    if (o.nrest > maxops) {
        print(diag(&p), "extra operand ");
        quote(&p.err, o.rest[maxops]);
        enddiag(&p);
        if (del && !squeeze) {
            print(&p.err, "Only one string may be given when deleting without squeezing repeats.\n");
        }
        return fkusage(&p, 1);
    }
    FkSet s1 = fkparseset(&p, o.rest[0], 0);
    if (!s1.ok) return fkexit(&p, 1);
    FkSet s2 = {};
    if (o.nrest > 1) {
        s2 = fkparseset(&p, o.rest[1], 1);
        if (!s2.ok) return fkexit(&p, 1);
    }
    if (!del && o.nrest==2) {
        if (s2.equiv) {
            print(diag(&p), "[=c=] expressions may not appear in string2 when translating");
            enddiag(&p);
            return fkexit(&p, 1);
        }
        for (iz i = 0; i < s2.classes.len; i++) {
            i32 cls = s2.classes[i].cls;
            if (cls!=FC_UPPER && cls!=FC_LOWER) {
                print(diag(&p), "when translating, the only character classes that may appear in\n"
                                "string2 are 'upper' and 'lower'");
                enddiag(&p);
                return fkexit(&p, 1);
            }
        }
        iz n1 = comp ? 256 - s1.len : s1.len;  // the complement, roughly
        if (!trunc && n1>s2.len && s2.endclass) {
            print(diag(&p), "when translating with string1 longer than string2,\n"
                            "the latter string must not end with a character class");
            enddiag(&p);
            return fkexit(&p, 1);
        }
        // [:upper:] and [:lower:] in string2 pair with one of them in string1
        for (iz i = 0; i < s2.classes.len; i++) {
            b32 ok = 0;
            for (iz k = 0; k < s1.classes.len; k++) {
                i32 cls = s1.classes[k].cls;
                ok |= !comp && s1.classes[k].pos==s2.classes[i].pos
                      && (cls==FC_UPPER || cls==FC_LOWER);
            }
            if (!ok) {
                print(diag(&p), "misaligned [:upper:] and/or [:lower:] construct");
                enddiag(&p);
                return fkexit(&p, 1);
            }
        }
    }
    u8 in1[256] = {};
    for (iz i = 0; i < s1.len; i++) in1[s1.c[i]] = 1;
    if (comp) {
        iz n = 0;
        for (i32 c = 0; c < 256; c++) {
            if (!in1[c]) s1.c[n++] = (u8)c;
        }
        s1.len = n;
        for (i32 c = 0; c < 256; c++) in1[c] = !in1[c];
    }

    u8 map[256];
    for (i32 c = 0; c < 256; c++) map[c] = (u8)c;
    b32 translate = !del && o.nrest==2;
    if (translate) {
        if (!s2.len) {
            print(diag(&p), "when not truncating set1, string2 must be non-empty");
            enddiag(&p);
            return fkexit(&p, 1);
        }
        if (s2.fill >= 0) {
            iz extra = s1.len - s2.len;
            u8 *c2 = alloc<u8>(p.a, s1.len + s2.len + 1);
            iz  n  = 0;
            for (iz i = 0; i < s2.len; i++) {
                c2[n++] = s2.c[i];
                if (i == s2.fill) {
                    for (iz k = 0; k < extra; k++) c2[n++] = s2.c[i];
                }
            }
            s2.c   = c2;
            s2.len = n;
        }
        if (trunc && s1.len>s2.len) s1.len = s2.len;
        for (iz i = 0; i < s1.len; i++) {
            map[s1.c[i]] = s2.c[i<s2.len ? i : s2.len-1];
        }
    }
    u8 sq[256] = {};
    if (squeeze) {
        FkSet *s = o.nrest==2 ? &s2 : &s1;
        if (o.nrest == 2) {
            for (iz i = 0; i < s->len; i++) sq[s->c[i]] = 1;
        } else {
            for (i32 c = 0; c < 256; c++) sq[c] = in1[c];
        }
    }

    u8 *buf  = allocbytes(p.a, 1<<16);
    i32 last = -1;
    for (iz n; (n = os_read(os, ctx->fd[0], buf, 1<<16)) > 0;) {
        for (iz i = 0; i < n; i++) {
            u8 c = buf[i];
            if (del && in1[c]) continue;
            c = map[c];
            if (squeeze && sq[c] && last==c) continue;
            last = c;
            putbyte(&p.out, c);
        }
    }
    return fkexit(&p, 0);
}


// od

struct FkOdSpec {
    u8  type;    // a c d o u x
    i32 size;    // bytes per value
    i32 width;   // field width, without the separator
    i32 pad;     // extra padding per line
    b32 text;    // z suffix: the printable bytes after the line
};

static i32 fkodwidth(u8 type, i32 size)
{
    static i32 w[4][4] = {
        {3, 6, 11, 22},   // o
        {2, 4, 8, 16},    // x
        {4, 6, 11, 20},   // d
        {3, 5, 10, 20},   // u
    };
    i32 k = size==1 ? 0 : size==2 ? 1 : size==4 ? 2 : 3;
    switch (type) {
    case 'o': return w[0][k];
    case 'x': return w[1][k];
    case 'd': return w[2][k];
    case 'u': return w[3][k];
    }
    return 3;
}

static b32 fkodparse(FkProg *p, Str s, Slice<FkOdSpec> *specs)
{
    for (iz i = 0; i < s.len;) {
        u8 t = s[i++];
        FkOdSpec sp = {};
        sp.type = t;
        switch (t) {
        case 'a': case 'c':
            sp.size = 1;
            break;
        case 'd': case 'o': case 'u': case 'x':
            sp.size = 4;
            if (i < s.len) {
                u8 c = s[i];
                if      (c == 'C') { sp.size = 1; i++; }
                else if (c == 'S') { sp.size = 2; i++; }
                else if (c == 'I') { sp.size = 4; i++; }
                else if (c == 'L') { sp.size = 8; i++; }
                else if (digit(c)) {
                    i32 n = 0;
                    for (; i<s.len && digit(s[i]); i++) n = (i32)fkaccum(n, s[i], 1<<20);
                    if (n!=1 && n!=2 && n!=4 && n!=8) {
                        print(diag(p), "invalid type string ");
                        quote(&p->err, s);
                        print(&p->err, ";\nthis system doesn't provide a ");
                        print(&p->err, (i64)n);
                        print(&p->err, "-byte integral type");
                        enddiag(p);
                        return 0;
                    }
                    sp.size = n;
                }
            }
            break;
        default:
            print(diag(p), "invalid character '");
            putbyte(&p->err, t);
            print(&p->err, "' in type string ");
            quote(&p->err, s);
            enddiag(p);
            return 0;
        }
        if (i<s.len && s[i]=='z') {
            sp.text = 1;
            i++;
        }
        sp.width = fkodwidth(t, sp.size);
        *specs = push(p->a, *specs, sp);
    }
    return 1;
}

static Str fkodnames[] = {
    "nul", "soh", "stx", "etx", "eot", "enq", "ack", "bel", "bs", "ht",
    "nl", "vt", "ff", "cr", "so", "si", "dle", "dc1", "dc2", "dc3", "dc4",
    "nak", "syn", "etb", "can", "em", "sub", "esc", "fs", "gs", "rs", "us",
    "sp",
};

static void fkodfield(FkOut *o, FkOdSpec *sp, u8 *b, i32 width)
{
    u64 v = 0;
    for (i32 k = sp->size-1; k >= 0; k--) v = v<<8 | b[k];
    u8  tmp[32];
    u8 *end = tmp + countof(tmp);
    u8 *beg = end;
    switch (sp->type) {
    case 'a': {
        u8  c = b[0] & 0x7f;
        Str s = c<=32 ? fkodnames[c] : c==127 ? Str("del") : Str{b, 1};
        if (c>32 && c<127) {
            *--beg = c;
        } else {
            beg -= s.len;
            copybytes(beg, s.data, s.len);
        }
    } break;
    case 'c': {
        u8 c = b[0];
        Str e = {};
        switch (c) {
        case 0:    e = "\\0"; break;
        case '\a': e = "\\a"; break;
        case '\b': e = "\\b"; break;
        case '\f': e = "\\f"; break;
        case '\n': e = "\\n"; break;
        case '\r': e = "\\r"; break;
        case '\t': e = "\\t"; break;
        case '\v': e = "\\v"; break;
        }
        if (e.data) {
            beg -= e.len;
            copybytes(beg, e.data, e.len);
        } else if (c>=32 && c<127) {
            *--beg = c;
        } else {
            for (i32 k = 0; k < 3; k++) {
                *--beg = (u8)('0' + (c>>(3*k) & 7));
            }
        }
    } break;
    case 'o':
        for (i32 k = 0; k < sp->width; k++) {
            *--beg = (u8)('0' + (v & 7));
            v >>= 3;
        }
        break;
    case 'x':
        for (i32 k = 0; k < sp->width; k++) {
            *--beg = "0123456789abcdef"[v & 15];
            v >>= 4;
        }
        break;
    case 'u':
        do {
            *--beg = (u8)('0' + v%10);
        } while (v /= 10);
        break;
    case 'd': {
        i64 sv = (i64)v;
        if (sp->size < 8) {
            i32 shift = 64 - 8*sp->size;
            sv = (i64)(v << shift) >> shift;
        }
        beg = fkfmtint(end, sv).data;
    } break;
    }
    for (iz k = end-beg; k < width; k++) putbyte(o, ' ');
    print(o, span(beg, end));
}

static void fkodaddr(FkOut *o, u8 radix, i64 addr)
{
    switch (radix) {
    case 'o': printoct(o, (u64)addr, addr>=(1<<21) ? 11 : 7); break;
    case 'd': printpad(o, addr, 7, '0'); break;
    case 'x': {
        i32 d = 6;
        for (; d<16 && ((u64)addr>>(4*d)); d++) {}
        for (i32 i = d-1; i >= 0; i--) {
            putbyte(o, "0123456789abcdef"[addr>>(4*i) & 15]);
        }
    } break;
    }
}

// A count for -j or -N: decimal, 0x hex or 0 octal, with an optional
// multiplier suffix. Sets bad to 1 for no number, 2 for a bad suffix.
static i64 fkodnum(Str s, i32 *bad)
{
    i64 lim  = (i64)1 << 62;
    i32 base = 10;
    iz  i    = 0;
    if (s.len>2 && s[0]=='0' && (s[1]|32)=='x') {
        base = 16;
        i = 2;
    } else if (s.len>1 && s[0]=='0') {
        base = 8;
    }
    i64 v  = 0;
    iz  d0 = i;
    for (; i < s.len; i++) {
        u8  c = s[i];
        i32 d = digit(c) ? c-'0' : (c|32)>='a' && (c|32)<='f' ? (c|32)-'a'+10 : 99;
        if (d >= base) break;
        v = v>lim/16 ? lim : v*base + d;
    }
    if (i == d0) {
        *bad = 1;
        return 0;
    }
    Str suf = cuthead(s, i);
    Str sufs[]  = {"", "b", "k", "K", "kB", "KiB", "m", "M", "MB", "MiB", "G", "GB", "GiB"};
    i64 mults[] = {1, 512, 1024, 1024, 1000, 1024, 1<<20, 1<<20, 1000000, 1<<20,
                   1<<30, 1000000000, 1<<30};
    for (iz k = 0; k < countof(sufs); k++) {
        if (suf == sufs[k]) return v>lim/mults[k] ? lim : v*mults[k];
    }
    *bad = 2;
    return 0;
}

static i32 fake_od(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    Slice<FkOdSpec> specs = {};
    u8  radix = 'o';
    i64 skip = 0, limit = -1;
    i32 perline = 16;
    b32 verbose = 0;
    for (i32 ch; (ch = fkgetopt(&o, "A:bcdilj:N:ost:vw::x")) != -1;) {
        b32 ok = 1;
        switch (ch) {
        case 'A':
            if (o.arg.len!=1 || !cut("doxn", o.arg[0]).ok) {
                print(diag(&p), "invalid output address radix '");
                print(&p.err, o.arg);
                print(&p.err, "'; it must be one character from [doxn]");
                enddiag(&p);
                return fkexit(&p, 1);
            }
            radix = o.arg[0];
            break;
        case 'b': ok = fkodparse(&p, "o1", &specs); break;
        case 'c': ok = fkodparse(&p, "c",  &specs); break;
        case 'd': ok = fkodparse(&p, "u2", &specs); break;
        case 'i': ok = fkodparse(&p, "d4", &specs); break;
        case 'l': ok = fkodparse(&p, "d8", &specs); break;
        case 'o': ok = fkodparse(&p, "o2", &specs); break;
        case 's': ok = fkodparse(&p, "d2", &specs); break;
        case 'x': ok = fkodparse(&p, "x2", &specs); break;
        case 't': ok = fkodparse(&p, o.arg, &specs); break;
        case 'v': verbose = 1; break;
        case 'j':
        case 'N': {
            i32 bad = 0;
            i64 v   = fkodnum(o.arg, &bad);
            if (bad) {
                print(diag(&p), bad==2 ? Str("invalid suffix in ") : Str("invalid "));
                print(&p.err, ch=='j' ? Str("-j") : Str("-N"));
                print(&p.err, " argument ");
                quote(&p.err, o.arg);
                enddiag(&p);
                return fkexit(&p, 1);
            }
            if (ch == 'j') skip  = v;
            if (ch == 'N') limit = v;
        } break;
        case 'w':
            perline = 32;
            if (o.arg.data) {
                FkNum n = fkparseint(o.arg);
                if (!n.ok || n.v<1 || n.v>4096) {
                    print(diag(&p), "invalid -w argument ");
                    quote(&p.err, o.arg);
                    enddiag(&p);
                    return fkexit(&p, 1);
                }
                perline = (i32)n.v;
            }
            break;
        default:
            return fkusage(&p, 1);
        }
        if (!ok) return fkexit(&p, 1);
    }
    if (!specs.len) fkodparse(&p, "o2", &specs);
    i32 lcm = 1;
    for (iz i = 0; i < specs.len; i++) {
        i32 s = specs[i].size;
        i32 a = lcm, b = s;
        while (b) {
            i32 t = a % b;
            a = b;
            b = t;
        }
        lcm = lcm / a * s;
    }
    if (perline % lcm) {
        print(diag(&p), "warning: invalid width ");
        print(&p.err, (i64)perline);
        print(&p.err, "; using ");
        print(&p.err, (i64)lcm);
        print(&p.err, " instead");
        enddiag(&p);
        perline = lcm;
    }
    i32 blockwidth = 0;
    for (iz i = 0; i < specs.len; i++) {
        i32 w = (specs[i].width + 1) * (perline / specs[i].size);
        blockwidth = w>blockwidth ? w : blockwidth;
    }
    for (iz i = 0; i < specs.len; i++) {
        specs[i].pad = blockwidth - specs[i].width*(perline/specs[i].size);
    }

    // The traditional offset operand: od [FILE] [+]OFFSET[b]
    if ((o.nrest==1 && o.rest[0].len && o.rest[0][0]=='+') || o.nrest==2) {
        Str s   = o.rest[o.nrest-1];
        Str t   = s.len && s[0]=='+' ? cuthead(s, 1) : s;
        i64 mul = 1;
        if (t.len && t[t.len-1]=='b') {
            mul = 512;
            t = cuttail(t, 1);
        }
        b32 hex = t.len>2 && t[0]=='0' && (t[1]|32)=='x';
        t = cuthead(t, hex ? 2 : 0);
        i64 v  = 0;
        b32 ok = t.len > 0;
        for (iz i = 0; i<t.len && ok; i++) {
            u8  c = t[i];
            i32 d = digit(c) ? c-'0' : (c|32)>='a' && (c|32)<='f' ? (c|32)-'a'+10 : 99;
            ok &= d < (hex ? 16 : 8);
            v = v>(i64)1<<50 ? v : v*(hex ? 16 : 8) + d;
        }
        if (ok) {
            skip = v * mul;
            o.nrest--;
        }
    }

    // Gather the input
    if (!o.nrest) o.rest[o.nrest++] = "-";
    i32 status = 0;
    iz  nopened = 0;
    Buf all(p.a, 1<<12);
    for (iz i = 0; i < o.nrest; i++) {
        FkIn in = fkopenin(&p, o.rest[i]);
        if (!in.err && in.dir) {
            fkclosein(&p, &in);
            print(diag(&p), "read error");
            enddiag(&p, E_ISDIR);
            status = 1;
            continue;
        }
        if (in.err) {
            quotef(diag(&p), o.rest[i]);
            enddiag(&p, in.err);
            status = 1;
            continue;
        }
        Str s = fkreadall(&p, in.h);
        fkclosein(&p, &in);
        print(&all, s);
        nopened++;
    }
    if (!nopened) return fkexit(&p, status);
    Str data = finish(&all);
    i64 addr = skip;
    if (skip > data.len) {
        print(diag(&p), "cannot skip past end of combined input");
        enddiag(&p);
        return fkexit(&p, 1);
    }
    data = cuthead(data, skip<data.len ? (iz)skip : data.len);
    if (limit>=0 && limit<data.len) data = takehead(data, (iz)limit);

    u8 *block = alloc<u8>(p.a, perline + 8);
    i32 addrwidth = radix=='n' ? 0 : radix=='x' ? 6 : 7;
    b32 star = 0;
    for (iz off = 0; off < data.len; off += perline) {
        iz n = data.len-off<perline ? data.len-off : perline;
        if (!verbose && off && n==perline &&
            !__builtin_memcmp(data.data+off, data.data+off-perline, (uz)perline)) {
            if (!star) print(&p.out, "*\n");
            star = 1;
            continue;
        }
        star = 0;
        __builtin_memset(block, 0, (uz)perline+8);
        copybytes(block, data.data+off, n);
        for (iz s = 0; s < specs.len; s++) {
            FkOdSpec *sp = specs.data + s;
            if (!s) {
                fkodaddr(&p.out, radix, addr+off);
            } else {
                for (i32 k = 0; k < addrwidth; k++) putbyte(&p.out, ' ');
            }
            i32 fields = perline / sp->size;
            i32 used   = (i32)((n + sp->size - 1) / sp->size);
            i32 padleft = sp->pad;
            i32 printed = 0;
            for (i32 f = 0; f < used; f++) {
                i32 next = sp->pad * (fields - f - 1) / fields;
                i32 w    = padleft - next + sp->width;
                padleft  = next;
                fkodfield(&p.out, sp, block + f*sp->size, w);
                printed += w;
            }
            if (sp->text) {
                for (i32 k = printed; k < blockwidth; k++) putbyte(&p.out, ' ');
                print(&p.out, "  >");
                for (iz k = 0; k < n; k++) {
                    u8 c = block[k];
                    putbyte(&p.out, c>=0x20 && c<0x7f ? c : '.');
                }
                putbyte(&p.out, '<');
            }
            putbyte(&p.out, '\n');
        }
    }
    if (radix != 'n') {
        fkodaddr(&p.out, radix, addr+data.len);
        putbyte(&p.out, '\n');
    }
    return fkexit(&p, status);
}


// hexdump

static void fkhex7(FkOut *o, iz v)
{
    for (i32 k = 6; k >= 0; k--) {
        putbyte(o, (u8)"0123456789abcdef"[v>>(4*k) & 15]);
    }
}

// Only util-linux's one-byte octal display, -b, for the bash tests
// that use it. Lines hold 16 bytes after a 7-digit hex offset, a short
// last line is padded to full width, and repeated lines fold into "*"
// unless -v.
static i32 fake_hexdump(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    octal = 0, verbose = 0;
    for (i32 ch; (ch = fkgetopt(&o, "bv")) != -1;) {
        switch (ch) {
        case 'b': octal   = 1; break;
        case 'v': verbose = 1; break;
        default:  return fkusage(&p, 1);
        }
    }
    if (!octal) {
        print(diag(&p), "only -b is supported");
        enddiag(&p);
        return fkexit(&p, 1);
    }
    if (!o.nrest) o.rest[o.nrest++] = "-";
    i32 status = 0;
    Buf all(p.a, 1<<12);
    for (iz i = 0; i < o.nrest; i++) {
        Str s = fkslurp(&p, o.rest[i]);
        if (!s.data) {
            status = 1;
            continue;
        }
        print(&all, s);
    }
    Str data = finish(&all);
    b32 star = 0;
    for (iz off = 0; off < data.len; off += 16) {
        iz n = data.len-off<16 ? data.len-off : 16;
        if (!verbose && off && n==16 &&
            !__builtin_memcmp(data.data+off, data.data+off-16, 16)) {
            if (!star) print(&p.out, "*\n");
            star = 1;
            continue;
        }
        star = 0;
        fkhex7(&p.out, off);
        for (iz k = 0; k < 16; k++) {
            putbyte(&p.out, ' ');
            if (k < n) {
                u8 c = data[off+k];
                putbyte(&p.out, (u8)('0' + (c>>6)));
                putbyte(&p.out, (u8)('0' + (c>>3&7)));
                putbyte(&p.out, (u8)('0' + (c&7)));
            } else {
                print(&p.out, "   ");
            }
        }
        putbyte(&p.out, '\n');
    }
    if (data.len) {
        fkhex7(&p.out, data.len);
        putbyte(&p.out, '\n');
    }
    return fkexit(&p, status);
}


// cmp and diff

// A byte as cmp -b shows it, in the manner of cat -v.
static void fkcmpbyte(FkOut *o, u8 c)
{
    if (c >= 0x80) {
        print(o, "M-");
        c &= 0x7f;
    }
    if (c < 0x20) {
        putbyte(o, '^');
        putbyte(o, (u8)(c + 64));
    } else if (c == 0x7f) {
        print(o, "^?");
    } else {
        putbyte(o, c);
    }
}

// A skip count for -i or the operands after the files: decimal with an
// optional multiplier suffix. Returns -1 when invalid.
static i64 fkcmpskip(Str s)
{
    i32 bad = 0;
    i64 v   = fkodnum(s, &bad);
    return bad || (s.len>1 && s[0]=='0') ? -1 : v;
}

static i32 fake_cmp(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    silent = 0, list = 0, bytes = 0;
    i64    limit = -1;
    i64    skip[2] = {};
    for (i32 ch; (ch = fkgetopt(&o, "bi:ln:s")) != -1;) {
        switch (ch) {
        case 'b': bytes  = 1; break;
        case 'l': list   = 1; break;
        case 's': silent = 1; break;
        case 'n': {
            i32 bad = 0;
            limit = fkodnum(o.arg, &bad);
            if (bad) {
                print(diag(&p), "invalid --bytes value ");
                quote(&p.err, o.arg);
                enddiag(&p);
                return fktry(&p, 2);
            }
        } break;
        case 'i': {
            Cut c = cut(o.arg, ':');
            skip[0] = fkcmpskip(c.head);
            skip[1] = c.ok ? fkcmpskip(c.tail) : skip[0];
            if (skip[0]<0 || skip[1]<0) {
                print(diag(&p), "invalid --ignore-initial value ");
                quote(&p.err, o.arg);
                enddiag(&p);
                return fktry(&p, 2);
            }
        } break;
        default:
            return fktry(&p, 2);
        }
    }
    if (list && silent) {
        print(diag(&p), "options -l and -s are incompatible");
        enddiag(&p);
        return fktry(&p, 2);
    }
    if (!o.nrest) {
        print(diag(&p), "missing operand after ");
        quote(&p.err, p.name);
        enddiag(&p);
        return fktry(&p, 2);
    }
    if (o.nrest > 4) {
        print(diag(&p), "extra operand ");
        quote(&p.err, o.rest[4]);
        enddiag(&p);
        return fktry(&p, 2);
    }
    for (iz k = 2; k < o.nrest; k++) {
        skip[k-2] = fkcmpskip(o.rest[k]);
        if (skip[k-2] < 0) {
            print(diag(&p), "invalid --ignore-initial value ");
            quote(&p.err, o.rest[k]);
            enddiag(&p);
            return fktry(&p, 2);
        }
    }
    Str name[2] = {o.rest[0], o.nrest>1 ? o.rest[1] : Str("-")};
    Str data[2] = {};
    for (i32 k = 0; k < 2; k++) {
        FkIn in = fkopenin(&p, name[k]);
        if (!in.err && in.dir) {
            fkclosein(&p, &in);
            in.err = E_ISDIR;
        }
        if (in.err) {
            if (!silent) {
                print(diag(&p), name[k]);
                enddiag(&p, in.err);
            }
            return fkexit(&p, 2);
        }
        data[k] = fkreadall(&p, in.h);
        fkclosein(&p, &in);
        data[k] = cuthead(data[k], skip[k]<data[k].len ? (iz)skip[k] : data[k].len);
        if (limit>=0 && limit<data[k].len) data[k] = takehead(data[k], (iz)limit);
    }
    iz n = data[0].len<data[1].len ? data[0].len : data[1].len;
    if (list) {
        i32 width = 1;
        for (iz m = n; m >= 10; m /= 10) width++;
        b32 differ = 0;
        for (iz i = 0; i < n; i++) {
            u8 x = data[0][i], y = data[1][i];
            if (x == y) continue;
            differ = 1;
            printpad(&p.out, (i64)i+1, width, ' ');
            putbyte(&p.out, ' ');
            printpad(&p.out, (x>>6)*100 + (x>>3&7)*10 + (x&7), 3, ' ');
            if (bytes) {
                putbyte(&p.out, ' ');
                iz before = p.out.len;
                fkcmpbyte(&p.out, x);
                for (iz k = p.out.len-before; k < 4; k++) putbyte(&p.out, ' ');
            }
            putbyte(&p.out, ' ');
            printpad(&p.out, (y>>6)*100 + (y>>3&7)*10 + (y&7), 3, ' ');
            if (bytes) {
                putbyte(&p.out, ' ');
                fkcmpbyte(&p.out, y);
            }
            putbyte(&p.out, '\n');
        }
        if (data[0].len != data[1].len) {
            i32 k = data[0].len<data[1].len ? 0 : 1;
            flush(&p.out);
            print(diag(&p), "EOF on ");
            print(&p.err, name[k]);
            if (!n) {
                print(&p.err, " which is empty");
            } else {
                print(&p.err, " after byte ");
                print(&p.err, (i64)n);
            }
            enddiag(&p);
            return fkexit(&p, 1);
        }
        return fkexit(&p, differ);
    }

    iz i = 0;
    i64 line = 1;
    for (; i<n && data[0][i]==data[1][i]; i++) {
        line += data[0][i] == '\n';
    }
    if (i < n) {
        if (!silent) {
            print(&p.out, name[0]);
            putbyte(&p.out, ' ');
            print(&p.out, name[1]);
            print(&p.out, bytes ? Str(" differ: byte ") : Str(" differ: char "));
            print(&p.out, (i64)i+1);
            print(&p.out, ", line ");
            print(&p.out, line);
            if (bytes) {
                u8 x = data[0][i], y = data[1][i];
                print(&p.out, " is ");
                printoct(&p.out, x, 3);
                putbyte(&p.out, ' ');
                fkcmpbyte(&p.out, x);
                putbyte(&p.out, ' ');
                printoct(&p.out, y, 3);
                putbyte(&p.out, ' ');
                fkcmpbyte(&p.out, y);
            }
            putbyte(&p.out, '\n');
        }
        return fkexit(&p, 1);
    }
    if (data[0].len == data[1].len) return fkexit(&p, 0);
    if (!silent) {
        i32 k = data[0].len<data[1].len ? 0 : 1;
        flush(&p.out);
        print(diag(&p), "EOF on ");
        print(&p.err, name[k]);
        if (!n) {
            print(&p.err, " which is empty");
        } else {
            print(&p.err, " after byte ");
            print(&p.err, (i64)n);
            if (data[k][n-1] == '\n') {
                print(&p.err, ", line ");
                print(&p.err, line-1);
            } else {
                print(&p.err, ", in line ");
                print(&p.err, line);
            }
        }
        enddiag(&p);
    }
    return fkexit(&p, 1);
}

struct FkDiffLines {
    Str *s;
    iz   n;
    b32  lastnl;   // final line ends with a newline
};

static FkDiffLines fkdifflines(Arena *a, Str data)
{
    FkDiffLines r = {};
    Slice<Str>  l = {};
    r.lastnl = 1;
    while (data.len) {
        FkLine x = fknextline(data);
        l = push(a, l, x.line);
        data = x.rest;
        if (!x.nl) r.lastnl = 0;
    }
    r.s = l.data;
    r.n = l.len;
    return r;
}

static b32 fkdiffeq(FkDiffLines *a, iz i, FkDiffLines *b, iz j)
{
    b32 an = i<a->n-1 || a->lastnl;
    b32 bn = j<b->n-1 || b->lastnl;
    return an==bn && a->s[i]==b->s[j];
}

static void fkdiffrange(FkOut *o, iz a, iz b)  // 1-based lines a..b
{
    print(o, (i64)a);
    if (b > a) {
        putbyte(o, ',');
        print(o, (i64)b);
    }
}

static void fkdiffline(FkOut *o, Str prefix, FkDiffLines *d, iz i)
{
    print(o, prefix);
    print(o, d->s[i]);
    putbyte(o, '\n');
    if (i==d->n-1 && !d->lastnl) print(o, "\\ No newline at end of file\n");
}

// Compare two files, printing the differences. Returns diff's status.
static i32 fkdifffiles(FkProg *p, Str n0, Str n1, b32 brief, b32 same, b32 text)
{
    Str data[2] = {};
    Str names[2] = {n0, n1};
    for (i32 k = 0; k < 2; k++) {
        data[k] = fkslurp(p, names[k]);
        if (!data[k].data) return 2;
    }
    if (data[0] == data[1]) {
        if (same) {
            print(&p->out, "Files ");
            print(&p->out, n0);
            print(&p->out, " and ");
            print(&p->out, n1);
            print(&p->out, " are identical\n");
        }
        return 0;
    }
    b32 binary = 0;
    for (i32 k = 0; k < 2; k++) {
        for (iz i = 0; i < data[k].len; i++) binary |= !data[k][i];
    }
    if (brief || (binary && !text)) {
        print(&p->out, binary ? Str("Binary files ") : Str("Files "));
        print(&p->out, n0);
        print(&p->out, " and ");
        print(&p->out, n1);
        print(&p->out, " differ\n");
        return 1;
    }

    FkDiffLines a = fkdifflines(p->a, data[0]);
    FkDiffLines b = fkdifflines(p->a, data[1]);
    // Common prefix and suffix, then a longest common subsequence table
    iz pre = 0;
    for (; pre<a.n && pre<b.n && fkdiffeq(&a, pre, &b, pre); pre++) {}
    iz suf = 0;
    for (; suf<a.n-pre && suf<b.n-pre && fkdiffeq(&a, a.n-1-suf, &b, b.n-1-suf); suf++) {}
    iz n = a.n - pre - suf;
    iz m = b.n - pre - suf;
    i32 *t = 0;
    b32 table = (n+1) <= ((iz)1<<24)/(m+1);
    if (table) {
        t = alloc<i32>(p->a, (n+1)*(m+1));
        for (iz i = n-1; i >= 0; i--) {
            for (iz j = m-1; j >= 0; j--) {
                i32 *r = t + i*(m+1) + j;
                if (fkdiffeq(&a, pre+i, &b, pre+j)) {
                    *r = r[m+2] + 1;
                } else {
                    *r = r[m+1]>r[1] ? r[m+1] : r[1];
                }
            }
        }
    }
    iz i = 0, j = 0;
    while (i<n || j<m) {
        if (i<n && j<m && table && fkdiffeq(&a, pre+i, &b, pre+j) &&
            t[i*(m+1)+j] == t[(i+1)*(m+1)+j+1]+1) {
            i++;
            j++;
            continue;
        }
        // A hunk: advance until the next match on the chosen path
        iz i0 = i, j0 = j;
        for (;;) {
            if (i<n && j<m && table && fkdiffeq(&a, pre+i, &b, pre+j) &&
                t[i*(m+1)+j] == t[(i+1)*(m+1)+j+1]+1) {
                break;
            }
            if (i==n && j==m) break;
            if (!table) {
                i = n;
                j = m;
                break;
            }
            if (j==m || (i<n && t[(i+1)*(m+1)+j] >= t[i*(m+1)+j+1])) {
                i++;
            } else {
                j++;
            }
        }
        iz ai = pre + i0, aj = pre + i;
        iz bi = pre + j0, bj = pre + j;
        if (ai == aj) {
            print(&p->out, (i64)ai);
            putbyte(&p->out, 'a');
            fkdiffrange(&p->out, bi+1, bj);
        } else if (bi == bj) {
            fkdiffrange(&p->out, ai+1, aj);
            putbyte(&p->out, 'd');
            print(&p->out, (i64)bi);
        } else {
            fkdiffrange(&p->out, ai+1, aj);
            putbyte(&p->out, 'c');
            fkdiffrange(&p->out, bi+1, bj);
        }
        putbyte(&p->out, '\n');
        for (iz k = ai; k < aj; k++) fkdiffline(&p->out, "< ", &a, k);
        if (ai!=aj && bi!=bj) print(&p->out, "---\n");
        for (iz k = bi; k < bj; k++) fkdiffline(&p->out, "> ", &b, k);
    }
    return 1;
}

struct FkDiffEnt {
    Str name;
    i32 side;   // 1, 2, or 3 for both
};

static b32 fkdiffentless(FkDiffEnt *a, FkDiffEnt *b)
{
    return compare(a->name, b->name) < 0;
}

static Slice<FkDiffEnt> fkdifflist(FkProg *p, Str dir, i32 side)
{
    Slice<FkDiffEnt> r = {};
    i32 err = 0;
    for (Dirent *d = os_list(p->os, p->a, fkpath(p, dir), &err); d; d = d->next) {
        if (d->name=="." || d->name=="..") continue;
        FkDiffEnt e = {d->name, side};
        r = push(p->a, r, e);
    }
    return r;
}

// Compare two directories without recursing, as diff without -r.
static i32 fkdiffdirs(FkProg *p, Str d0, Str d1, b32 brief, b32 same, b32 text)
{
    Slice<FkDiffEnt> a = fkdifflist(p, d0, 1);
    Slice<FkDiffEnt> b = fkdifflist(p, d1, 2);
    sort(a.data, a.len, fkdiffentless, *p->a);
    sort(b.data, b.len, fkdiffentless, *p->a);
    i32 status = 0;
    for (iz i = 0, j = 0; i<a.len || j<b.len;) {
        i32 c = i==a.len ? 1 : j==b.len ? -1 : compare(a[i].name, b[j].name);
        if (c) {
            Str dir  = c<0 ? d0 : d1;
            Str name = c<0 ? a[i++].name : b[j++].name;
            print(&p->out, "Only in ");
            print(&p->out, dir);
            print(&p->out, ": ");
            print(&p->out, name);
            putbyte(&p->out, '\n');
            status = status>1 ? status : 1;
            continue;
        }
        Str  x  = fkjoin(p->a, d0, a[i].name);
        Str  y  = fkjoin(p->a, d1, b[j].name);
        Stat sx = os_stat(p->os, *p->a, fkpath(p, x), 1);
        Stat sy = os_stat(p->os, *p->a, fkpath(p, y), 1);
        i++;
        j++;
        b32 dx = !sx.err && sx.type==FT_DIR;
        b32 dy = !sy.err && sy.type==FT_DIR;
        if (dx && dy) {
            print(&p->out, "Common subdirectories: ");
            print(&p->out, x);
            print(&p->out, " and ");
            print(&p->out, y);
            putbyte(&p->out, '\n');
        } else if (dx != dy) {
            print(&p->out, "File ");
            print(&p->out, x);
            print(&p->out, dx ? Str(" is a directory while file ") : Str(" is a regular file while file "));
            print(&p->out, y);
            print(&p->out, dy ? Str(" is a directory\n") : Str(" is a regular file\n"));
            status = status>1 ? status : 1;
        } else {
            Arena save = *p->a;
            Str   u = fkslurp(p, x);
            Str   v = fkslurp(p, y);
            if (!u.data || !v.data) {
                status = 2;
            } else if (!(u == v) || same) {
                if (!(u == v) && !brief) {
                    print(&p->out, "diff ");
                    print(&p->out, x);
                    putbyte(&p->out, ' ');
                    print(&p->out, y);
                    putbyte(&p->out, '\n');
                }
                i32 s = fkdifffiles(p, x, y, brief, same, text);
                status = status>s ? status : s;
            }
            *p->a = save;
        }
    }
    return status;
}

static i32 fake_diff(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    b32    brief = 0, same = 0, text = 0;
    for (i32 ch; (ch = fkgetopt(&o, "aqs")) != -1;) {
        switch (ch) {
        case 'a': text  = 1; break;
        case 'q': brief = 1; break;
        case 's': same  = 1; break;
        default:
            print(diag(&p), "Try '");
            print(&p.err, p.name);
            print(&p.err, " --help' for more information.");
            enddiag(&p);
            return fkexit(&p, 2);
        }
    }
    if (o.nrest != 2) {
        if (o.nrest < 2) {
            print(diag(&p), "missing operand after ");
            quote(&p.err, o.nrest ? o.rest[0] : p.name);
        } else {
            print(diag(&p), "extra operand ");
            quote(&p.err, o.rest[2]);
        }
        enddiag(&p);
        print(diag(&p), "Try '");
        print(&p.err, p.name);
        print(&p.err, " --help' for more information.");
        enddiag(&p);
        return fkexit(&p, 2);
    }
    // A directory operand stands for the file of the other's name in it
    Str  n[2] = {o.rest[0], o.rest[1]};
    b32  dir[2] = {};
    Stat st[2] = {};
    for (i32 k = 0; k < 2; k++) {
        st[k]  = n[k]=="-" ? Stat{} : os_stat(os, *p.a, fkpath(&p, n[k]), 1);
        dir[k] = !st[k].err && st[k].type==FT_DIR;
    }
    if ((dir[0] && n[1]=="-") || (dir[1] && n[0]=="-")) {
        print(diag(&p), "cannot compare '-' to a directory");
        enddiag(&p);
        return fkexit(&p, 2);
    }
    if (dir[0] && dir[1]) {
        if (fksamefile(st[0], st[1])) return fkexit(&p, 0);
        return fkexit(&p, fkdiffdirs(&p, n[0], n[1], brief, same, text));
    }
    for (i32 k = 0; k < 2; k++) {
        if (dir[k]) n[k] = fkjoin(p.a, n[k], fkbasename(n[1-k]));
    }
    return fkexit(&p, fkdifffiles(&p, n[0], n[1], brief, same, text));
}



// Regular expressions
//
// POSIX basic and extended syntax with the GNU extensions, parsed to a
// tree and compiled to a backtracking program. A search finds the
// leftmost match and, among the matches starting there, the longest.
// The search memoizes visited branch states unless the pattern has back
// references, so it stays polynomial on ordinary patterns. In a UTF-8
// locale, "." and bracket expressions match whole characters, and never
// an invalid byte.

// sed's escapes for bytes by number, \dNNN \oNNN \xHH, and for controls,
// \cX. s[*i] is the letter after the backslash. Returns -1 when this is
// no such escape, else the byte, with *i advanced past the escape.
static i32 fksedchar(Str s, iz *i)
{
    iz j = *i;
    u8 k = s[j++];
    if (k == 'c') {
        if (j >= s.len) return -1;
        *i = j + 1;
        return (s[j]>='a' && s[j]<='z' ? s[j]-32 : s[j]) ^ 0x40;
    }
    i32 base = k=='d' ? 10 : k=='o' ? 8 : k=='x' ? 16 : 0;
    i32 max  = k=='x' ? 2 : 3;
    i32 v = 0, n = 0;
    for (; base && n<max && j<s.len; n++, j++) {
        u8  c = s[j];
        i32 d = digit(c) ? c-'0' : (c|32)>='a' && (c|32)<='f' ? (c|32)-'a'+10 : 99;
        if (d >= base) break;
        v = v*base + d;
    }
    if (!n) return -1;
    *i = j;
    return v & 0xff;
}

enum : i32 {
    RX_ERE   = 1<<0,
    RX_ICASE = 1<<1,
    RX_SED   = 1<<2,  // sed's syntax: \n, \t, \x41 and its stricter rules
    RX_FIXED = 1<<3,  // every byte is literal
    RX_GREP  = 1<<4,  // grep's syntax: its warnings and leniencies
    RX_UTF8  = 1<<5,  // characters are UTF-8
    RX_MULTI = 1<<6,  // ^ and $ also match at newlines, as sed's M
};

enum : u8 {  // tree nodes
    RN_EMPTY, RN_CHAR, RN_ANY, RN_SET, RN_WSET, RN_ASSERT, RN_BACKREF,
    RN_GROUP, RN_CAT, RN_ALT, RN_REPEAT,
};

enum : u8 {  // program instructions
    RI_CHAR, RI_ANY, RI_ANYMB, RI_SET, RI_WSET, RI_BOL, RI_EOL, RI_WORDB,
    RI_NWORDB, RI_WBEG, RI_WEND, RI_NOTWPREV, RI_NOTWNEXT, RI_SPLIT, RI_JMP,
    RI_SAVE, RI_BACKREF, RI_MARK, RI_CHECK, RI_MATCH,
};

struct FkRxNode {
    u8        kind;
    i32       c;      // byte, set, assertion, or group number
    i32       min;
    i32       max;    // -1 for unbounded
    FkRxNode *a;
    FkRxNode *b;
};

struct FkInst {
    u8  op;
    i32 x;
    i32 y;
};

struct FkRxSet {
    u8 bits[32];
};

// A bracket expression over characters, for UTF-8.
struct FkRxWide {
    FkRxSet    ascii;    // members below 0x80, before negation
    Slice<i32> ranges;   // pairs of code points
    u32        classes;  // 1<<FC_*
    b32        neg;
    b32        icase;
};

struct FkRx {
    FkInst   *prog;
    i32       len;
    FkRxSet  *sets;
    FkRxWide *wides;
    iz       *sub;      // each search's group bounds
    i32       ngroups;
    i32       nloops;
    b32       icase;
    b32       utf8;
    b32       multi;
    b32       backrefs;
    Str       err;
    Str       warn;
};

struct FkRxParse {
    Arena *a;
    Str    s;
    iz     i;
    i32    flags;
    Slice<FkRxSet>  sets;
    Slice<FkRxWide> wides;
    i32    ngroups;
    u32    closed;   // groups closed in this branch, for back references
    i32    depth;
    Str    err;
    Str    warn;
};

// Group nesting limit, which bounds the parser's recursion. GNU has no
// limit short of its stack size, so it accepts somewhat deeper groups.
enum { FKRX_DEPTH = 1000 };

static FkRxNode *fkrxnode(FkRxParse *p, u8 kind, i32 c)
{
    FkRxNode *n = alloc<FkRxNode>(p->a);
    n->kind = kind;
    n->c    = c;
    return n;
}

static FkRxNode *fkrxcat(FkRxParse *p, FkRxNode *x, FkRxNode *y)
{
    if (!x) return y;
    FkRxNode *n = fkrxnode(p, RN_CAT, 0);
    n->a = x;
    n->b = y;
    return n;
}

static FkRxNode *fkrxfail(FkRxParse *p, Str msg)
{
    if (!p->err.data) p->err = msg;
    return 0;
}

static void fkrxwarn(FkRxParse *p, Str msg)
{
    if ((p->flags&RX_GREP) && !p->warn.data) p->warn = msg;
}

static void fksetadd(FkRxSet *s, u8 c)
{
    s->bits[c>>3] |= (u8)(1 << (c&7));
}

static b32 fksethas(FkRxSet *s, u8 c)
{
    return s->bits[c>>3]>>(c&7) & 1;
}

static i32 fkrxnewset(FkRxParse *p)
{
    FkRxSet z = {};
    p->sets = push(p->a, p->sets, z);
    return (i32)p->sets.len - 1;
}

static i32 fkrxnewwide(FkRxParse *p)
{
    FkRxWide z = {};
    z.icase  = (p->flags & RX_ICASE) != 0;
    p->wides = push(p->a, p->wides, z);
    return (i32)p->wides.len - 1;
}

static b32 fkwidehas(FkRxWide *w, i32 c)
{
    b32 in = 0;
    if (c < 0x80) {
        in = fksethas(&w->ascii, (u8)c);
    } else {
        i32 alt[] = {c, w->icase ? fklower(c) : c, w->icase ? fkupper(c) : c};
        for (iz k = 0; k<countof(alt) && !in; k++) {
            for (iz i = 0; i<w->ranges.len && !in; i += 2) {
                in = alt[k]>=w->ranges[i] && alt[k]<=w->ranges[i+1];
            }
            for (i32 cls = 0; cls<=FC_WORD && !in; cls++) {
                in = (w->classes>>cls & 1) && fkwclass(cls, alt[k]);
            }
        }
    }
    return in != w->neg;
}

static b32 fkwordc(u8 c)
{
    return alnum(c) || c=='_';
}

// A set node for \w \W \s \S.
static FkRxNode *fkrxclassnode(FkRxParse *p, u8 k)
{
    b32 word = k=='w' || k=='W';
    b32 neg  = k=='W' || k=='S';
    if (p->flags & RX_UTF8) {
        i32 id = fkrxnewwide(p);
        FkRxWide *w = p->wides.data + id;
        for (i32 c = 0; c < 0x80; c++) {
            if (word ? fkwordc((u8)c) : whitespace((u8)c)) fksetadd(&w->ascii, (u8)c);
        }
        w->classes = 1u << (word ? FC_WORD : FC_SPACE);
        w->neg     = neg;
        return fkrxnode(p, RN_WSET, id);
    }
    i32 id = fkrxnewset(p);
    FkRxSet *s = p->sets.data + id;
    for (i32 c = 0; c < 256; c++) {
        b32 in = word ? fkwordc((u8)c) : whitespace((u8)c);
        if (in != neg) fksetadd(s, (u8)c);
    }
    return fkrxnode(p, RN_SET, id);
}

// One literal character, which in UTF-8 may span several bytes that
// must repeat together.
static FkRxNode *fkrxliteral(FkRxParse *p, u8 c)
{
    if ((p->flags&RX_UTF8) && c>=0x80) {
        FkChar ch = fkchar(p->s, p->i);
        if (ch.c >= 0) {
            p->i += ch.len;
            if (p->flags & RX_ICASE) {
                i32 id = fkrxnewwide(p);
                FkRxWide *w = p->wides.data + id;
                w->ranges = push(p->a, push(p->a, w->ranges, ch.c), ch.c);
                return fkrxnode(p, RN_WSET, id);
            }
            FkRxNode *r = 0;
            for (iz k = ch.len; k > 0; k--) {
                r = fkrxcat(p, r, fkrxnode(p, RN_CHAR, p->s[p->i-k]));
            }
            return r;
        }
    }
    p->i++;
    return fkrxnode(p, RN_CHAR, c);
}

static FkRxNode *fkrxbracket(FkRxParse *p)
{
    Str s    = p->s;
    b32 utf8 = p->flags & RX_UTF8;
    FkRxSet    set     = {};
    Slice<i32> ranges  = {};
    u32        classes = 0;
    b32        neg     = 0;
    if (p->i == s.len) {
        return fkrxfail(p, "Invalid regular expression");
    }
    if (s[p->i] == '^') {
        neg = 1;
        p->i++;
    }
    iz first = p->i;
    if ((p->flags&RX_GREP) && p->i<s.len && s[p->i]==':') {
        iz j = p->i + 1;
        for (; j<s.len && s[j]!=']'; j++) {}
        if (j<s.len && j>p->i+1 && s[j-1]==':') {
            return fkrxfail(p, "character class syntax is [[:space:]], not [:space:]");
        }
    }
    b32 afterrange = 0;  // a range or class just ended
    for (;;) {
        if (p->i >= s.len) {
            return fkrxfail(p, "Unmatched [, [^, [:, [., or [=");
        }
        u8 c = s[p->i];
        if (c==']' && p->i>first) {
            p->i++;
            break;
        }
        if (c=='-' && afterrange && p->i+1<s.len && s[p->i+1]!=']') {
            return fkrxfail(p, "Invalid range end");
        }
        afterrange = 0;
        i32 lo = -1;
        if (c=='[' && p->i+1<s.len && (s[p->i+1]==':' || s[p->i+1]=='=' || s[p->i+1]=='.')) {
            u8 kind = s[p->i+1];
            iz j = p->i + 2;
            for (; j+1<s.len && !(s[j]==kind && s[j+1]==']'); j++) {}
            if (j+1 >= s.len) {
                return fkrxfail(p, "Unmatched [, [^, [:, [., or [=");
            }
            Str name = span(s.data+p->i+2, s.data+j);
            p->i = j + 2;
            if (kind == ':') {
                i32 cls = fkclassid(name);
                if (cls < 0) {
                    return fkrxfail(p, "Invalid character class name");
                }
                for (i32 k = 0; k < 256; k++) {
                    if (fkclass(name, (u8)k)) fksetadd(&set, (u8)k);
                }
                if ((p->flags&RX_ICASE) && (cls==FC_UPPER || cls==FC_LOWER)) {
                    for (i32 k = 0; k < 26; k++) {
                        fksetadd(&set, (u8)('a'+k));
                        fksetadd(&set, (u8)('A'+k));
                    }
                    cls = FC_ALPHA;  // close enough beyond ASCII
                }
                classes |= 1u << cls;
                afterrange = 1;
                continue;
            }
            if (name.len == 1) {
                lo = name[0];
            } else {
                return fkrxfail(p, "Invalid collation character");
            }
        } else if ((p->flags&RX_SED) && c=='\\' && p->i+1<s.len
                   && (s[p->i+1]=='n' || s[p->i+1]=='t')) {
            lo = s[p->i+1]=='n' ? '\n' : '\t';  // sed converts these first
            p->i += 2;
        } else if (utf8 && c>=0x80) {
            FkChar ch = fkchar(s, p->i);
            lo = ch.c>=0 ? ch.c : 0x110000;  // an invalid byte matches nothing
            p->i += ch.len;
        } else {
            lo = c;
            p->i++;
        }
        i32 hi = lo;
        if (p->i+1<s.len && s[p->i]=='-' && s[p->i+1]!=']') {
            p->i++;
            u8 e = s[p->i];
            if (e=='[' && p->i+1<s.len && s[p->i+1]=='.') {
                iz j = p->i + 2;
                for (; j+1<s.len && !(s[j]=='.' && s[j+1]==']'); j++) {}
                if (j+1>=s.len || j!=p->i+3) {
                    return fkrxfail(p, "Invalid range end");
                }
                hi = s[p->i+2];
                p->i = j + 2;
            } else if (utf8 && e>=0x80) {
                FkChar ch = fkchar(s, p->i);
                hi = ch.c>=0 ? ch.c : 0x110000;
                p->i += ch.len;
            } else {
                hi = e;
                p->i++;
            }
            if (utf8 && (lo>=0x80 || hi>=0x80)) {
                return fkrxfail(p, "Invalid collation character");  // as C.UTF-8
            }
            if (hi < lo) {
                return fkrxfail(p, "Invalid range end");
            }
            afterrange = 1;
        }
        if (lo >= 0x80 && utf8) {
            ranges = push(p->a, push(p->a, ranges, lo), hi);
            continue;
        }
        for (i32 k = lo; k <= hi; k++) {
            fksetadd(&set, (u8)k);
            if (p->flags & RX_ICASE) {
                if (k>='a' && k<='z') fksetadd(&set, (u8)(k-32));
                if (k>='A' && k<='Z') fksetadd(&set, (u8)(k+32));
            }
        }
    }
    if (utf8 && (neg || classes || ranges.len)) {
        i32 id = fkrxnewwide(p);
        FkRxWide *w = p->wides.data + id;
        for (i32 k = 0; k < 16; k++) w->ascii.bits[k] = set.bits[k];
        w->ranges  = ranges;
        w->classes = classes;
        w->neg     = neg;
        return fkrxnode(p, RN_WSET, id);
    }
    if (neg) {
        for (i32 k = 0; k < 32; k++) set.bits[k] = (u8)~set.bits[k];
    }
    i32 id = fkrxnewset(p);
    p->sets[id] = set;
    return fkrxnode(p, RN_SET, id);
}

// Try to read an interval at s[i] (after "{" or "\{"). Returns -2 if it
// is not an interval at all, -1 on error.
static i32 fkrxinterval(FkRxParse *p, i32 *min, i32 *max)
{
    Str s   = p->s;
    b32 ere = p->flags & RX_ERE;
    iz  i   = p->i;
    i32 lo  = -1, hi = -1;
    b32 big = 0;
    if (i<s.len && digit(s[i])) {
        lo = 0;
        for (; i<s.len && digit(s[i]); i++) {
            lo = lo*10 + s[i] - '0';
            if (lo > 0x7fff) { big = 1; lo = 0x8000; }
        }
    }
    b32 comma = i<s.len && s[i]==',';
    if (comma) {
        i++;
        if (i<s.len && digit(s[i])) {
            hi = 0;
            for (; i<s.len && digit(s[i]); i++) {
                hi = hi*10 + s[i] - '0';
                if (hi > 0x7fff) { big = 1; hi = 0x8000; }
            }
        }
        if (ere && i<s.len && s[i]==',') {
            fkrxfail(p, "Invalid content of \\{\\}");  // as "a{1,2,3}"
            return -1;
        }
    } else {
        hi = lo;
    }
    b32 close = ere ? i<s.len && s[i]=='}'
                    : i+1<s.len && s[i]=='\\' && s[i+1]=='}';
    if (!close) {
        if (ere) return -2;
        if (i >= s.len || (i+1>=s.len && s[i]=='\\')) {
            fkrxfail(p, "Unmatched \\{");
        } else {
            fkrxfail(p, "Invalid content of \\{\\}");
        }
        return -1;
    }
    if (lo<0 && !comma) {
        if (ere) return -2;
        fkrxfail(p, "Invalid content of \\{\\}");
        return -1;
    }
    if (lo < 0) lo = 0;
    if (big) {
        fkrxfail(p, "Regular expression too big");
        return -1;
    }
    if (hi>=0 && hi<lo) {
        fkrxfail(p, "Invalid content of \\{\\}");
        return -1;
    }
    p->i = i + (ere ? 1 : 2);
    *min = lo;
    *max = hi;
    return 0;
}

static FkRxNode *fkrxalt(FkRxParse *p);

// At an alternation or group end, which ends a concatenation.
static b32 fkrxatend(FkRxParse *p)
{
    Str s = p->s;
    iz  i = p->i;
    if (i >= s.len) return 1;
    if (p->flags & RX_ERE) {
        return s[i]=='|' || (s[i]==')' && p->depth);
    }
    return s[i]=='\\' && i+1<s.len && (s[i+1]=='|' || s[i+1]==')');
}

static FkRxNode *fkrxgroup(FkRxParse *p, b32 ere)
{
    if (p->depth >= FKRX_DEPTH) return fkrxfail(p, "Regular expression too big");
    Str s = p->s;
    i32 g = ++p->ngroups;
    p->depth++;
    FkRxNode *inner = fkrxalt(p);
    p->depth--;
    if (p->err.data) return 0;
    if (ere ? p->i>=s.len || s[p->i]!=')'
            : p->i+1>=s.len || s[p->i]!='\\' || s[p->i+1]!=')') {
        return fkrxfail(p, "Unmatched ( or \\(");
    }
    p->i += ere ? 1 : 2;
    if (g < 32) p->closed |= 1u << g;
    FkRxNode *atom = fkrxnode(p, RN_GROUP, g);
    atom->a = inner;
    return atom;
}

static FkRxNode *fkrxcatexpr(FkRxParse *p)
{
    Str       s     = p->s;
    b32       ere   = p->flags & RX_ERE;
    b32       sed   = p->flags & RX_SED;
    b32       grep  = p->flags & RX_GREP;
    FkRxNode *r     = 0;
    b32       start = 1;   // at the start of this expression
    while (!fkrxatend(p)) {
        u8        c    = s[p->i];
        FkRxNode *atom = 0;
        b32       anchor = 0;
        if (p->flags & RX_FIXED) {
            atom = fkrxnode(p, RN_CHAR, c);
            p->i++;
        } else if (c=='^' && (ere || start)) {
            atom = fkrxnode(p, RN_ASSERT, RI_BOL);
            anchor = 1;
            p->i++;
        } else if (c == '$') {
            p->i++;
            b32 last = ere || fkrxatend(p);
            atom = last ? fkrxnode(p, RN_ASSERT, RI_EOL) : fkrxnode(p, RN_CHAR, '$');
            anchor = last;
        } else if (c=='*' && start && !ere) {
            atom = fkrxnode(p, RN_CHAR, '*');
            p->i++;
        } else if ((c=='*' || c=='+' || c=='?') && start && ere) {
            if (sed) return fkrxfail(p, "Invalid preceding regular expression");
            fkrxwarn(p, c=='*' ? Str("* at start of expression")
                      : c=='+' ? Str("+ at start of expression")
                               : Str("? at start of expression"));
            p->i++;
            continue;
        } else if (c == '.') {
            atom = fkrxnode(p, RN_ANY, 0);
            p->i++;
        } else if (c == '[') {
            p->i++;
            atom = fkrxbracket(p);
            if (!atom) return 0;
        } else if (ere && c=='(') {
            p->i++;
            atom = fkrxgroup(p, 1);
            if (!atom) return 0;
        } else if (c=='{' && ere && start) {
            if (sed) return fkrxfail(p, "Invalid preceding regular expression");
            iz  save = p->i++;
            i32 min, max;
            if (fkrxinterval(p, &min, &max) == 0) {
                fkrxwarn(p, "{...} at start of expression");
            }
            p->err = {};
            p->i   = save + 1;
            atom = fkrxnode(p, RN_CHAR, '{');
        } else if (c == '\\') {
            if (p->i+1 >= s.len) {
                return fkrxfail(p, "Trailing backslash");
            }
            u8 e = s[p->i+1];
            p->i += 2;
            if (!ere && e=='(') {
                atom = fkrxgroup(p, 0);
                if (!atom) return 0;
            } else if (!ere && e==')') {
                return fkrxfail(p, "Unmatched ) or \\)");
            } else if (!ere && e=='{') {
                if (start && !grep) {
                    return fkrxfail(p, "Invalid preceding regular expression");
                }
                atom = fkrxnode(p, RN_CHAR, '{');
            } else if (e>='1' && e<='9') {
                i32 g = e - '0';
                if (!(p->closed>>g & 1)) {
                    return fkrxfail(p, "Invalid back reference");
                }
                atom = fkrxnode(p, RN_BACKREF, g);
            } else if (e=='w' || e=='W' || e=='s' || e=='S') {
                atom = fkrxclassnode(p, e);
            } else if (e == '<') {
                atom = fkrxnode(p, RN_ASSERT, RI_WBEG);
                anchor = 1;
            } else if (e == '>') {
                atom = fkrxnode(p, RN_ASSERT, RI_WEND);
                anchor = 1;
            } else if (e == 'b') {
                atom = fkrxnode(p, RN_ASSERT, RI_WORDB);
                anchor = 1;
            } else if (e == 'B') {
                atom = fkrxnode(p, RN_ASSERT, RI_NWORDB);
                anchor = 1;
            } else if (e == '`') {
                atom = fkrxnode(p, RN_ASSERT, RI_BOL);
                anchor = 1;
            } else if (e == '\'') {
                atom = fkrxnode(p, RN_ASSERT, RI_EOL);
                anchor = 1;
            } else if (sed && e=='n') {
                atom = fkrxnode(p, RN_CHAR, '\n');
            } else if (sed && e=='t') {
                atom = fkrxnode(p, RN_CHAR, '\t');
            } else if (sed && (e=='d' || e=='o' || e=='x' || e=='c')) {
                iz  j = p->i - 1;
                i32 v = fksedchar(s, &j);
                if (v < 0) {
                    p->i--;
                    atom = fkrxliteral(p, e);
                } else {
                    p->i = j;
                    atom = fkrxnode(p, RN_CHAR, v);
                }
            } else {
                p->i--;
                atom = fkrxliteral(p, e);
            }
        } else {
            atom = fkrxliteral(p, c);
        }

        // Postfix repetition
        b32 dup = 0;
        for (;;) {
            if (p->flags & RX_FIXED) break;
            if (p->i >= s.len) break;
            u8  q   = s[p->i];
            i32 min = 0, max = -1;
            if (dup && sed && !ere
                    && (q=='*' || (q=='\\' && p->i+1<s.len && s[p->i+1]=='{'))) {
                // sed's syntax rejects stacked * and \{\} in BRE
                return fkrxfail(p, "Invalid preceding regular expression");
            }
            dup = 1;
            if (q == '*') {
                if (anchor && atom->c==RI_BOL && atom->kind==RN_ASSERT) {
                    if (!ere) break;  // BRE: literal after a leading ^
                    if (sed) return fkrxfail(p, "Invalid preceding regular expression");
                    fkrxwarn(p, "* at start of expression");
                }
                p->i++;
            } else if (ere && q=='+') {
                if (anchor && atom->c==RI_BOL && atom->kind==RN_ASSERT) {
                    if (sed) return fkrxfail(p, "Invalid preceding regular expression");
                    fkrxwarn(p, "+ at start of expression");
                }
                min = 1;
                p->i++;
            } else if (ere && q=='?') {
                if (anchor && atom->c==RI_BOL && atom->kind==RN_ASSERT) {
                    if (sed) return fkrxfail(p, "Invalid preceding regular expression");
                    fkrxwarn(p, "? at start of expression");
                }
                max = 1;
                p->i++;
            } else if (!ere && q=='\\' && p->i+1<s.len && (s[p->i+1]=='+' || s[p->i+1]=='?')) {
                if (s[p->i+1] == '+') min = 1; else max = 1;
                p->i += 2;
            } else if (ere && q=='{') {
                iz save = p->i++;
                i32 k = fkrxinterval(p, &min, &max);
                if (k == -1) return 0;
                if (k == -2) {
                    p->i = save;
                    break;
                }
                if (anchor && atom->c==RI_BOL && atom->kind==RN_ASSERT) {
                    if (sed) return fkrxfail(p, "Invalid preceding regular expression");
                    fkrxwarn(p, "{...} at start of expression");
                }
            } else if (!ere && q=='\\' && p->i+1<s.len && s[p->i+1]=='{') {
                p->i += 2;
                if (fkrxinterval(p, &min, &max) < 0) return 0;
            } else {
                break;
            }
            if (atom->kind == RN_REPEAT) {
                // Fold directly nested repeats where the result is a
                // single repeat, like a** or a+?, which bounds the
                // tree depth on hostile patterns
                i32 imin = atom->min, imax = atom->max;
                if ((imax<0 || max<0) && imax && max && imin<=1 && min<=1) {
                    atom->min = imin * min;
                    atom->max = -1;
                    continue;
                } else if (imax>=0 && imax<=1 && max>=0 && max<=1) {
                    atom->min = imin * min;
                    atom->max = imax * max;
                    continue;
                }
            }
            FkRxNode *rep = fkrxnode(p, RN_REPEAT, 0);
            rep->a   = atom;
            rep->min = min;
            rep->max = max;
            atom = rep;
        }
        r = fkrxcat(p, r, atom);
        start = c=='^' && anchor;  // what follows a leading ^ is at the start
    }
    return r ? r : fkrxnode(p, RN_EMPTY, 0);
}

static FkRxNode *fkrxalt(FkRxParse *p)
{
    // Back references see only the groups closed in their own branch
    u32 initial = p->closed;
    u32 all     = 0;
    FkRxNode *r = fkrxcatexpr(p);
    while (!p->err.data && p->i<p->s.len) {
        Str s = p->s;
        if (p->flags & RX_ERE) {
            if (s[p->i] != '|') break;
            p->i++;
        } else {
            if (!(s[p->i]=='\\' && p->i+1<s.len && s[p->i+1]=='|')) break;
            p->i += 2;
        }
        all |= p->closed;
        p->closed = initial;
        FkRxNode *n = fkrxnode(p, RN_ALT, 0);
        n->a = r;
        n->b = fkrxcatexpr(p);
        r = n;
    }
    p->closed |= all;
    return p->err.data ? 0 : r;
}

struct FkRxGen {
    Slice<FkInst> prog;
    Arena        *a;
    i32           nloops;
    b32           icase;
    b32           utf8;
    b32           backrefs;
    b32           big;
};

static i32 fkrxemit(FkRxGen *g, u8 op, i32 x, i32 y)
{
    if (g->prog.len > 1<<17) {
        g->big = 1;
        return (i32)g->prog.len;
    }
    FkInst in = {op, x, y};
    g->prog = push(g->a, g->prog, in);
    return (i32)g->prog.len - 1;
}

static void fkrxgen(FkRxGen *g, FkRxNode *n, FkRxParse *p)
{
    if (g->big) return;
    switch (n->kind) {
    case RN_EMPTY:
        break;
    case RN_CHAR:
        if (g->icase && letter((u8)n->c)) {
            i32 id = fkrxnewset(p);
            fksetadd(p->sets.data+id, lowercase((u8)n->c));
            fksetadd(p->sets.data+id, (u8)(lowercase((u8)n->c) - 32));
            fkrxemit(g, RI_SET, id, 0);
        } else {
            fkrxemit(g, RI_CHAR, n->c, 0);
        }
        break;
    case RN_ANY:     fkrxemit(g, g->utf8 ? RI_ANYMB : RI_ANY, 0, 0); break;
    case RN_SET:     fkrxemit(g, RI_SET, n->c, 0);                   break;
    case RN_WSET:    fkrxemit(g, RI_WSET, n->c, 0);                  break;
    case RN_ASSERT:  fkrxemit(g, (u8)n->c, 0, 0);                    break;
    case RN_BACKREF:
        g->backrefs = 1;
        fkrxemit(g, RI_BACKREF, n->c, 0);
        break;
    case RN_GROUP:
        fkrxemit(g, RI_SAVE, 2*n->c, 0);
        fkrxgen(g, n->a, p);
        fkrxemit(g, RI_SAVE, 2*n->c+1, 0);
        break;
    case RN_CAT:
    case RN_ALT: {
        // The parser builds these left-deep, as long as the pattern, so
        // walk the left spine instead of recursing down it
        u8 kind = n->kind;
        iz len  = 1;
        for (FkRxNode *m = n; m->kind == kind; m = m->a) len++;
        FkRxNode **parts = alloc<FkRxNode *>(g->a, len);
        iz i = len;
        FkRxNode *m = n;
        for (; m->kind == kind; m = m->a) parts[--i] = m->b;
        parts[0] = m;

        if (kind == RN_CAT) {
            for (i = 0; i<len && !g->big; i++) fkrxgen(g, parts[i], p);
            break;
        }
        i32 *jmps = alloc<i32>(g->a, len);
        for (i = 0; i<len-1 && !g->big; i++) {
            i32 split = fkrxemit(g, RI_SPLIT, 0, 0);
            fkrxgen(g, parts[i], p);
            jmps[i] = fkrxemit(g, RI_JMP, 0, 0);
            if (g->big) return;
            g->prog[split].x = split + 1;
            g->prog[split].y = (i32)g->prog.len;
        }
        fkrxgen(g, parts[len-1], p);
        if (g->big) return;
        for (i = 0; i < len-1; i++) g->prog[jmps[i]].x = (i32)g->prog.len;
    } break;
    case RN_REPEAT: {
        for (i32 i = 0; i < n->min && !g->big; i++) {
            fkrxgen(g, n->a, p);
        }
        if (n->max < 0) {
            i32 loop  = g->nloops++;
            i32 split = fkrxemit(g, RI_SPLIT, 0, 0);
            fkrxemit(g, RI_MARK, loop, 0);
            fkrxgen(g, n->a, p);
            fkrxemit(g, RI_CHECK, loop, 0);
            fkrxemit(g, RI_JMP, split, 0);
            if (g->big) return;
            g->prog[split].x = split + 1;
            g->prog[split].y = (i32)g->prog.len;
        } else {
            i32 nopt = n->max - n->min;
            i32 *splits = alloc<i32>(g->a, nopt+1);
            for (i32 i = 0; i < nopt && !g->big; i++) {
                splits[i] = fkrxemit(g, RI_SPLIT, 0, 0);
                fkrxgen(g, n->a, p);
            }
            if (g->big) return;
            for (i32 i = 0; i < nopt; i++) {
                g->prog[splits[i]].x = splits[i] + 1;
                g->prog[splits[i]].y = (i32)g->prog.len;
            }
        }
    } break;
    }
}

// Compile one pattern. Check err for failure. With wrap, -x or -w
// style anchors surround the whole pattern.
static FkRx fkrxcompile(Arena *a, Str pat, i32 flags, u8 wrap = 0)
{
    FkRx      r = {};
    FkRxParse p = {};
    p.a     = a;
    p.s     = pat;
    p.flags = flags;
    FkRxNode *root = fkrxalt(&p);
    if (!p.err.data && p.i<pat.len) {
        fkrxfail(&p, "Unmatched ) or \\)");
    }
    r.warn = p.warn;
    if (p.err.data) {
        r.err = p.err;
        return r;
    }
    if (wrap == 'x') {
        root = fkrxcat(&p, fkrxnode(&p, RN_ASSERT, RI_BOL),
                       fkrxcat(&p, root, fkrxnode(&p, RN_ASSERT, RI_EOL)));
    } else if (wrap == 'w') {
        root = fkrxcat(&p, fkrxnode(&p, RN_ASSERT, RI_NOTWPREV),
                       fkrxcat(&p, root, fkrxnode(&p, RN_ASSERT, RI_NOTWNEXT)));
    }
    FkRxGen g = {};
    g.a     = a;
    g.icase = flags & RX_ICASE;
    g.utf8  = flags & RX_UTF8;
    fkrxgen(&g, root, &p);
    fkrxemit(&g, RI_MATCH, 0, 0);
    if (g.big) {
        r.err = "Regular expression too big";
        return r;
    }
    r.prog     = g.prog.data;
    r.len      = (i32)g.prog.len;
    r.sets     = p.sets.data;
    r.wides    = p.wides.data;
    r.sub      = alloc<iz>(a, 2*p.ngroups+2);
    r.ngroups  = p.ngroups;
    r.nloops   = g.nloops;
    r.icase    = g.icase;
    r.utf8     = g.utf8;
    r.multi    = (flags & RX_MULTI) != 0;
    r.backrefs = g.backrefs;
    return r;
}

struct FkMatch {
    b32 ok;
    iz  beg;
    iz  end;
    iz *sub;     // 2 per group, from group 1; -1 when unset
    i32 nsub;
};

struct FkRxFrame {
    i32 kind;    // 0 branch, 1 restore a capture, 2 restore a loop mark
    i32 pc;
    iz  pos;
};

// Search s from start for the leftmost match, the longest one there
// when longest is set. With anchored, only a match at start counts.
// The result's sub array belongs to rx and the next search reuses it.
static FkMatch fkrxexec(FkRx *rx, Str s, iz start, Arena scratch, b32 longest,
                        b32 anchored = 0)
{
    FkMatch r = {};
    r.nsub = rx->ngroups;
    r.sub  = rx->sub;
    iz *cap   = alloc<iz>(&scratch, 2*rx->ngroups+4);
    iz *loops = alloc<iz>(&scratch, rx->nloops+1);
    for (i32 i = 0; i < 2*rx->ngroups+4; i++) cap[i] = -1;

    // Memo of visited branch states, when it is sound and affordable
    u8 *memo = 0;
    iz  slots = (iz)rx->len * (s.len+1);
    if (!rx->backrefs && slots < ((iz)1<<26)) {
        memo = alloc<u8>(&scratch, (slots>>3) + 1);
    }

    iz         stackcap = 256;
    FkRxFrame *stack = alloc<FkRxFrame>(&scratch, stackcap);
    iz         sp = 0;
    i64        budget = (i64)1<<26;
    iz         last = anchored ? start : s.len;

    for (iz st = start; st <= last; st++) {
        i32 pc  = 0;
        iz  pos = st;
        b32 found = 0;
        for (;;) {
            FkInst in = rx->prog[pc];
            b32 fail = 0;
            if (--budget < 0) {
                fail = 1;
                sp = 0;
            }
            switch (fail ? (u8)255 : in.op) {
            case RI_CHAR:
                if (pos<s.len && s[pos]==(u8)in.x) {
                    pos++;
                    pc++;
                } else {
                    fail = 1;
                }
                break;
            case RI_ANY:
                if (pos < s.len) {
                    pos++;
                    pc++;
                } else {
                    fail = 1;
                }
                break;
            case RI_ANYMB:
            case RI_WSET: {
                FkChar ch = pos<s.len ? fkchar(s, pos) : FkChar{-1, 0};
                if (ch.c>=0 && (in.op==RI_ANYMB || fkwidehas(rx->wides+in.x, ch.c))) {
                    pos += ch.len;
                    pc++;
                } else {
                    fail = 1;
                }
            } break;
            case RI_SET:
                if (pos<s.len && fksethas(rx->sets+in.x, s[pos])) {
                    pos++;
                    pc++;
                } else {
                    fail = 1;
                }
                break;
            case RI_BOL:
                if (pos==0 || (rx->multi && s[pos-1]=='\n')) pc++; else fail = 1;
                break;
            case RI_EOL:
                if (pos==s.len || (rx->multi && s[pos]=='\n')) pc++; else fail = 1;
                break;
            case RI_WORDB:
            case RI_NWORDB:
            case RI_WBEG:
            case RI_WEND:
            case RI_NOTWPREV:
            case RI_NOTWNEXT: {
                b32 prev = pos>0 && fkwordc(s[pos-1]);
                b32 next = pos<s.len && fkwordc(s[pos]);
                if (rx->utf8) {
                    i32 pc0 = fkcharprev(s, pos);
                    i32 nc0 = pos<s.len ? fkchar(s, pos).c : -1;
                    prev = pc0>=0 && fkwclass(FC_WORD, pc0);
                    next = nc0>=0 && fkwclass(FC_WORD, nc0);
                }
                b32 ok   = 0;
                switch (in.op) {
                case RI_WORDB:    ok = prev != next;   break;
                case RI_NWORDB:   ok = prev == next;   break;
                case RI_WBEG:     ok = !prev && next;  break;
                case RI_WEND:     ok = prev && !next;  break;
                case RI_NOTWPREV: ok = !prev;          break;
                case RI_NOTWNEXT: ok = !next;          break;
                }
                if (ok) pc++; else fail = 1;
            } break;
            case RI_SPLIT:
                if (memo) {
                    iz k = (iz)pc*(s.len+1) + pos;
                    if (memo[k>>3]>>(k&7) & 1) {
                        fail = 1;
                        break;
                    }
                    memo[k>>3] |= (u8)(1 << (k&7));
                }
                if (sp == stackcap) {
                    FkRxFrame *n = alloc<FkRxFrame>(&scratch, 2*stackcap);
                    copybytes(n, stack, sp*(iz)sizeof(*stack));
                    stack = n;
                    stackcap *= 2;
                }
                stack[sp++] = {0, in.y, pos};
                pc = in.x;
                break;
            case RI_JMP:
                pc = in.x;
                break;
            case RI_SAVE:
            case RI_MARK:
                if (sp+1 >= stackcap) {
                    FkRxFrame *n = alloc<FkRxFrame>(&scratch, 2*stackcap);
                    copybytes(n, stack, sp*(iz)sizeof(*stack));
                    stack = n;
                    stackcap *= 2;
                }
                if (in.op == RI_SAVE) {
                    stack[sp++] = {1, in.x, cap[in.x]};
                    cap[in.x] = pos;
                } else {
                    stack[sp++] = {2, in.x, loops[in.x]};
                    loops[in.x] = pos;
                }
                pc++;
                break;
            case RI_CHECK:
                if (loops[in.x] == pos) fail = 1; else pc++;
                break;
            case RI_BACKREF: {
                iz b = cap[2*in.x], e = cap[2*in.x+1];
                if (b<0 || e<0 || e-b > s.len-pos) {
                    fail = 1;
                    break;
                }
                for (iz k = 0; k<e-b && !fail; k++) {
                    u8 x = s[b+k], y = s[pos+k];
                    fail = rx->icase ? lowercase(x)!=lowercase(y) : x!=y;
                }
                if (!fail) {
                    pos += e - b;
                    pc++;
                }
            } break;
            case RI_MATCH:
                if (!found || pos>r.end) {
                    found = 1;
                    r.beg = st;
                    r.end = pos;
                    for (i32 k = 0; k < 2*rx->ngroups; k++) r.sub[k] = cap[k+2];
                }
                fail = 1;
                if (!longest) sp = 0;
                break;
            default:
                fail = 1;
            }
            if (!fail) continue;
            // Backtrack
            b32 resumed = 0;
            while (sp) {
                FkRxFrame f = stack[--sp];
                if (f.kind == 1) {
                    cap[f.pc] = f.pos;
                } else if (f.kind == 2) {
                    loops[f.pc] = f.pos;
                } else {
                    pc  = f.pc;
                    pos = f.pos;
                    resumed = 1;
                    break;
                }
            }
            if (!resumed) break;
        }
        if (found) {
            r.ok = 1;
            return r;
        }
        if (budget < 0) break;
        if (rx->utf8 && !anchored && st<s.len) st += fkchar(s, st).len - 1;
    }
    return r;
}


// grep

struct FkGrep {
    FkProg *p;
    FkRx   *rx;
    iz      nrx;
    b32     invert;
    b32     count;
    b32     quiet;
    b32     lineno;
    b32     names;
    b32     only;
    b32     listmatch;
    b32     listnon;
    b32     silent;
    b32     text;      // -a: binary files are text
    b32     offset;    // -b
    i64     before;    // -B context lines
    i64     after;     // -A
    b32     context;   // any of -A -B -C, even 0
    b32     anyout;    // a group was printed, for "--" separators
    i64     maxcount;
    u8      eol;
};

static b32 fkgrepline(FkGrep *g, Str line, Arena scratch)
{
    for (iz i = 0; i < g->nrx; i++) {
        if (fkrxexec(g->rx+i, line, 0, scratch, 0).ok) return 1;
    }
    return 0;
}

struct FkGrepLine {
    Str line;
    iz  off;   // byte offset in the file
};

// The prefix of an output line: name, number and offset, each followed
// by ':' for a selected line or '-' for context.
static void fkgrepprefix(FkGrep *g, Str shown, i64 n, iz off, u8 sep)
{
    FkOut *o = &g->p->out;
    if (g->names) {
        print(o, shown);
        putbyte(o, sep);
    }
    if (g->lineno) {
        print(o, n);
        putbyte(o, sep);
    }
    if (g->offset) {
        print(o, (i64)off);
        putbyte(o, sep);
    }
}

// Returns 1 if any line was selected.
static b32 fkgrepfile(FkGrep *g, Str name, Str data)
{
    FkProg *p = g->p;
    FkOut  *o = &p->out;
    Str     shown = name=="-" ? Str("(standard input)") : name;
    b32     binary = 0;
    for (iz i = 0; i<data.len && g->eol && !g->text; i++) {
        if (!data[i]) {
            binary = 1;
            break;
        }
    }

    // Lines, which in a binary file also end at each NUL, as GNU zaps
    // NULs into line ends when it matches binary data
    Slice<FkGrepLine> lines = {};
    for (iz i = 0; i < data.len;) {
        iz j = i;
        for (; j<data.len && data[j]!=g->eol && !(binary && !data[j]); j++) {}
        FkGrepLine l = {span(data.data+i, data.data+j), i};
        lines = push(p->a, lines, l);
        i = j + 1;
    }

    b32 listing = g->quiet || g->listmatch || g->listnon;
    b32 showctx = g->context && !g->count && !g->only && !listing;
    i64 selected = 0;
    i64 printed  = 0;   // lines up to here have been output
    i64 pending  = 0;   // after-context lines still to print
    b32 any      = 0;   // anything printed from this file
    for (iz i = 0; i < lines.len; i++) {
        Str  line = lines[i].line;
        i64  n    = i + 1;
        b32  done = g->maxcount>=0 && selected>=g->maxcount;
        Arena scratch = *p->a;
        b32  m = !done && fkgrepline(g, line, scratch)!=g->invert;
        if (!m) {
            if (done && !pending) break;
            if (pending) {
                fkgrepprefix(g, shown, n, lines[i].off, '-');
                print(o, line);
                putbyte(o, g->eol);
                printed = n;
                pending--;
            }
            continue;
        }
        selected++;
        if (listing) break;
        if (g->count) continue;
        if (binary) {
            flush(o);
            print(diag(p), shown);
            print(&p->err, ": binary file matches");
            enddiag(p);
            break;
        }
        if (showctx) {
            i64 from = n - g->before;
            if (from <= printed) from = printed + 1;
            if (g->anyout && (!any || from>printed+1)) print(o, "--\n");
            for (i64 k = from; k < n; k++) {
                fkgrepprefix(g, shown, k, lines[k-1].off, '-');
                print(o, lines[k-1].line);
                putbyte(o, g->eol);
            }
            pending   = g->after;
            printed   = n;
            any       = 1;
            g->anyout = 1;
        }
        if (g->only) {
            if (g->invert) continue;
            for (iz pos = 0; pos <= line.len;) {
                iz bb = -1, be = -1;
                for (iz k = 0; k < g->nrx; k++) {
                    FkMatch mm = fkrxexec(g->rx+k, line, pos, scratch, 1);
                    if (mm.ok && (bb<0 || mm.beg<bb || (mm.beg==bb && mm.end>be))) {
                        bb = mm.beg;
                        be = mm.end;
                    }
                }
                if (bb < 0) break;
                if (be == bb) {
                    pos = bb + 1;
                    continue;
                }
                fkgrepprefix(g, shown, n, lines[i].off+bb, ':');
                print(o, span(line.data+bb, line.data+be));
                putbyte(o, g->eol);
                pos = be;
            }
            continue;
        }
        fkgrepprefix(g, shown, n, lines[i].off, ':');
        print(o, line);
        putbyte(o, g->eol);
    }
    if (g->count && !listing) {
        if (g->names) {
            print(o, shown);
            putbyte(o, ':');
        }
        print(o, selected);
        putbyte(o, '\n');
    }
    if (g->listmatch && selected) {
        print(o, shown);
        putbyte(o, '\n');
    }
    if (g->listnon && !selected) {
        print(o, shown);
        putbyte(o, '\n');
    }
    return selected > 0;
}

// A context length for -A, -B and -C.
static b32 fkgrepctx(FkProg *p, Str arg, i64 *n)
{
    FkNum v = fkparseint(arg);
    if (!fkisint(arg) || arg[0]=='-') {
        print(diag(p), arg);
        print(&p->err, ": invalid context length argument");
        enddiag(p);
        return 0;
    }
    *n = v.ok ? v.v : (i64)1<<62;
    return 1;
}

static i32 fake_grep(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 1);
    FkGrep g = {};
    g.p        = &p;
    g.maxcount = -1;
    g.eol      = '\n';
    i32 flags  = RX_GREP | (p.utf8 ? RX_UTF8 : 0);
    u8  wrap   = 0;
    i32 names  = 0;  // 0 auto, 1 never, 2 always
    Slice<Str> pats = {};
    b32 given = 0;
    i64 before = -1, after = -1, both = -1;
    for (i32 ch; (ch = fkgetopt(&o, "EFGe:f:icvlLnqshHoxwm:yaIzbA:B:C:")) != -1;) {
        switch (ch) {
        case 'z': g.eol = 0; break;
        case 'E': flags = (flags & ~RX_FIXED) | RX_ERE; break;
        case 'F': flags = (flags & ~RX_ERE) | RX_FIXED; break;
        case 'G': flags &= ~(RX_ERE|RX_FIXED); break;
        case 'e':
            pats  = push(p.a, pats, o.arg);
            given = 1;
            break;
        case 'f': {
            Str data = fkslurp(&p, o.arg);
            if (!data.data) return fkexit(&p, 2);
            b32 any = data.len > 0;
            if (data.len && data[data.len-1]=='\n') data = cuttail(data, 1);
            if (any) pats = push(p.a, pats, data);
            given = 1;
        } break;
        case 'i':
        case 'y': flags |= RX_ICASE; break;
        case 'c': g.count = 1; break;
        case 'v': g.invert = 1; break;
        case 'l': g.listmatch = 1; break;
        case 'L': g.listnon = 1; break;
        case 'n': g.lineno = 1; break;
        case 'q': g.quiet = 1; break;
        case 's': g.silent = 1; break;
        case 'h': names = 1; break;
        case 'H': names = 2; break;
        case 'o': g.only = 1; break;
        case 'x': wrap = 'x'; break;
        case 'w': if (wrap != 'x') wrap = 'w'; break;
        case 'b': g.offset = 1; break;
        case 'a': g.text = 1; break;
        case 'A': if (!fkgrepctx(&p, o.arg, &after))  return fkexit(&p, 2); break;
        case 'B': if (!fkgrepctx(&p, o.arg, &before)) return fkexit(&p, 2); break;
        case 'C': if (!fkgrepctx(&p, o.arg, &both))   return fkexit(&p, 2); break;
        case 'm': {
            FkNum n = fkparseint(o.arg);
            if (!fkisint(o.arg)) {
                print(diag(&p), "invalid max count");
                enddiag(&p);
                return fkexit(&p, 2);
            }
            g.maxcount = !n.ok ? (o.arg[0]=='-' ? -1 : (i64)1<<62) : n.v<0 ? -1 : n.v;
        } break;
        case 'I': break;
        default:
            print(&p.err, "Usage: ");
            print(&p.err, p.name);
            print(&p.err, " [OPTION]... PATTERNS [FILE]...\n");
            return fkusage(&p, 2);
        }
    }
    g.after   = after>=0 ? after : both>=0 ? both : 0;
    g.before  = before>=0 ? before : both>=0 ? both : 0;
    g.context = after>=0 || before>=0 || both>=0;
    iz k = 0;
    if (!given) {
        if (!o.nrest) {
            print(&p.err, "Usage: ");
            print(&p.err, p.name);
            print(&p.err, " [OPTION]... PATTERNS [FILE]...\n");
            return fkusage(&p, 2);
        }
        pats = push(p.a, pats, o.rest[k++]);
    }
    // Each pattern may hold several, one per line
    Slice<FkRx> rxs = {};
    iz total = 0;
    for (iz i = 0; i < pats.len; i++) {
        total += pats[i].len;
        for (Cut c = {{}, pats[i], 1}; c.ok;) {
            c = cut(c.tail, '\n');
            FkRx rx = fkrxcompile(p.a, c.head, flags, wrap);
            if (rx.warn.data) {
                print(diag(&p), "warning: ");
                print(&p.err, rx.warn);
                enddiag(&p);
            }
            if (rx.err.data) {
                print(diag(&p), rx.err);
                enddiag(&p);
                return fkexit(&p, 2);
            }
            rxs = push(p.a, rxs, rx);
        }
    }
    // GNU gives up at once when nothing could ever be selected
    if ((!g.maxcount || (pats.len==1 && !total && g.invert && !wrap)) && !g.listnon) {
        return fkexit(&p, 1);
    }
    g.rx  = rxs.data;
    g.nrx = rxs.len;
    iz nfiles = o.nrest - k;
    g.names = names==2 || (names==0 && nfiles>1);
    if (!nfiles) {
        o.rest[o.nrest++] = "-";
        nfiles = 1;
    }
    b32 matched = 0, error = 0;
    for (iz i = k; i < o.nrest; i++) {
        Arena save = *p.a;
        Str   name = o.rest[i];
        FkIn  in   = fkopenin(&p, name);
        if (!in.err && in.dir) {
            fkclosein(&p, &in);
            in.err = E_ISDIR;
        }
        Str data = {};
        if (!in.err) {
            data = fkreadall(&p, in.h);
            fkclosein(&p, &in);
            if (!data.data) in.err = E_IO;
        }
        if (in.err) {
            if (!g.silent) {
                print(diag(&p), name);
                enddiag(&p, in.err);
            }
            if (in.err == E_ISDIR) {
                // As GNU reads a directory: an error, and no lines
                fkgrepfile(&g, name, Str((u8 *)"", 0));
            }
            error = 1;
            *p.a = save;
            continue;
        }
        matched |= fkgrepfile(&g, name, data);
        *p.a = save;
        if (matched && g.quiet) return fkexit(&p, 0);
    }
    return fkexit(&p, error && !(matched && g.quiet) ? 2 : !matched);
}


// sed

enum : i32 {
    SA_NONE, SA_LINE, SA_LAST, SA_RE, SA_STEP, SA_ZERO, SA_PLUS, SA_MULT,
};

struct FkSedFile {   // a w file, opened once for the whole run
    Str    name;
    FkOut *out;
};

struct FkSedAddr {
    i32   kind;
    i64   n;
    i64   step;
    FkRx *rx;    // null for the last regex used
};

struct FkSedPart {   // replacement text pieces
    i32 group;       // -1 literal, 0 whole match, 1-9 groups
    Str text;
    u8  casec;       // L U E l u, or 0
};

struct FkSedCmd {
    FkSedAddr  a1;
    FkSedAddr  a2;
    i32        naddr;
    b32        neg;
    u8         cmd;
    b32        active;
    i64        rangeend;
    FkRx      *rx;
    FkSedPart *repl;
    iz         nrepl;
    b32        global;
    i64        nth;
    b32        print;
    u8        *map;
    Slice<Str> yfrom;    // y for UTF-8: characters and replacements
    Slice<Str> yto;
    Str        text;
    i32        status;
    Str        label;
    iz         jump;
    FkOut     *wfile;    // w, or s with the w flag
};

struct FkSedParse {
    FkProg *p;
    Str     s;
    iz      i;
    iz     *chunks;   // start offset of each -e chunk
    iz      nchunks;
    i32     flags;
    b32     failed;
    b32     panic;    // exits 4 rather than 1
    b32     quiet;    // "#n" on the first line
    b32     anyre;    // a regex came before, for an empty one to reuse
    Slice<FkSedFile> files;
};

struct FkSed {
    FkProg    *p;
    FkSedCmd  *cmds;
    iz         ncmds;
    FkOut     *out;
    b32        quiet;
    FkRx      *lastrx;
    Slice<Str> lines;
    Slice<b32> nls;
    Slice<Str> names;     // the file each line came from
    iz         next;      // next line to read
    i64        lineno;
    Buf        ps;
    Buf        hs;
    b32        curnl;     // the last line read had a newline
    b32        holdnl;    // likewise for the hold space
    b32        missingnl; // output is owed a newline
    b32        tflag;
    b32        quit;
    i32        status;
    u8         eol;       // newline, or with -z, NUL
    Str        filename;  // for F
    Slice<Str> append;
};

static void fksederr(FkSedParse *sp, Str msg, b32 atzero = 0)
{
    if (sp->failed) return;
    sp->failed = 1;
    FkProg *p = sp->p;
    iz k = 0;
    for (; k+1<sp->nchunks && sp->chunks[k+1]<=sp->i; k++) {}
    iz end = k+1<sp->nchunks ? sp->chunks[k+1]-1 : sp->s.len;
    iz pos = (sp->i<end ? sp->i : end) - sp->chunks[k];
    if (atzero) pos = 0;
    print(diag(p), "-e expression #");
    print(&p->err, (i64)k+1);
    print(&p->err, ", char ");
    print(&p->err, (i64)pos);
    print(&p->err, ": ");
    print(&p->err, msg);
    enddiag(p);
}

static b32 fksedeof(FkSedParse *sp)
{
    return sp->i >= sp->s.len;
}

static void fksedspaces(FkSedParse *sp)
{
    for (; !fksedeof(sp) && (sp->s[sp->i]==' ' || sp->s[sp->i]=='\t'); sp->i++) {}
}

// Read delimited text up to an unescaped delim, turning \delim into
// delim and \n into a newline (for regexes, only when delim is not n).
static Str fkseddelimited(FkSedParse *sp, u8 delim, b32 regex, b32 *ok)
{
    FkProg *p = sp->p;
    Buf b(p->a, 64);
    Str special = sp->flags&RX_ERE ? Str(".*[]^$\\+?(){}|") : Str(".*[]^$\\");
    *ok = 0;
    while (!fksedeof(sp)) {
        u8 c = sp->s[sp->i++];
        if (c == delim) {
            *ok = 1;
            return finish(&b);
        }
        if (c == '\n') {
            sp->i--;
            return finish(&b);
        }
        if (regex && c=='[') {
            // A bracket expression may hold the delimiter
            Str s = sp->s;
            iz  j = sp->i;
            if (j<s.len && s[j]=='^') j++;
            if (j<s.len && s[j]==']') j++;
            for (; j<s.len && s[j]!='\n' && s[j]!=']'; j++) {
                if (s[j]=='[' && j+1<s.len && (s[j+1]==':' || s[j+1]=='=' || s[j+1]=='.')) {
                    u8 k = s[j+1];
                    iz m = j + 2;
                    for (; m+1<s.len && !(s[m]==k && s[m+1]==']'); m++) {}
                    if (m+1 < s.len) j = m + 1;
                }
            }
            putbyte(&b, '[');
            if (j<s.len && s[j]==']') {
                print(&b, span(s.data+sp->i, s.data+j+1));
                sp->i = j + 1;
            }
            continue;
        }
        if (c=='\\' && !fksedeof(sp)) {
            u8 e = sp->s[sp->i++];
            if (e == delim) {
                // A literal delimiter
                if (regex && cut(special, delim).ok) putbyte(&b, '\\');
                if (!regex && (delim=='&' || delim=='\\')) putbyte(&b, '\\');
                putbyte(&b, e);
            } else if (e == '\n') {
                putbyte(&b, '\n');
            } else {
                putbyte(&b, '\\');
                putbyte(&b, e);
            }
            continue;
        }
        putbyte(&b, c);
    }
    return finish(&b);
}

static b32 fksedcmdend(FkSedParse *sp)
{
    fksedspaces(sp);
    if (fksedeof(sp)) return 1;
    u8 c = sp->s[sp->i];
    if (c==';' || c=='\n' || c=='}' || c=='#') return 1;
    sp->i++;
    fksederr(sp, "extra characters after command");
    return 0;
}

static FkRx *fksedregex(FkSedParse *sp, Str re, i32 flags)
{
    if (!re.len) {
        if (!sp->anyre) fksederr(sp, "no previous regular expression", 1);
        return 0;
    }
    sp->anyre = 1;
    FkRx *rx = alloc<FkRx>(sp->p->a);
    *rx = fkrxcompile(sp->p->a, re, flags);
    if (rx->err.data) {
        fksederr(sp, rx->err);
        return 0;
    }
    return rx;
}

static FkSedAddr fksedaddr(FkSedParse *sp, b32 second)
{
    FkSedAddr a = {};
    Str s = sp->s;
    if (fksedeof(sp)) return a;
    u8 c = s[sp->i];
    if (digit(c)) {
        i64 n = 0;
        for (; !fksedeof(sp) && digit(s[sp->i]); sp->i++) n = fkaccum(n, s[sp->i]);
        a.kind = SA_LINE;
        a.n    = n;
        if (!fksedeof(sp) && s[sp->i]=='~') {
            sp->i++;
            i64 st = 0;
            for (; !fksedeof(sp) && digit(s[sp->i]); sp->i++) st = fkaccum(st, s[sp->i]);
            a.kind = SA_STEP;
            a.step = st;
        } else if (!n && !second) {
            a.kind = SA_ZERO;
        }
    } else if ((c=='+' || c=='~') && second) {
        sp->i++;
        i64 n = 0;
        for (; !fksedeof(sp) && digit(s[sp->i]); sp->i++) n = fkaccum(n, s[sp->i]);
        a.kind = c=='+' ? SA_PLUS : SA_MULT;
        a.n    = n;
    } else if (c == '$') {
        sp->i++;
        a.kind = SA_LAST;
    } else if (c=='/' || c=='\\') {
        sp->i++;
        u8 delim = '/';
        if (c == '\\') {
            if (fksedeof(sp)) {
                fksederr(sp, "unexpected `,'");
                return a;
            }
            delim = s[sp->i++];
        }
        b32 ok = 0;
        Str re = fkseddelimited(sp, delim, 1, &ok);
        if (!ok) {
            fksederr(sp, "unterminated address regex");
            return a;
        }
        i32 flags = sp->flags;
        for (; !fksedeof(sp) && (s[sp->i]=='I' || s[sp->i]=='M'); sp->i++) {
            flags |= s[sp->i]=='I' ? RX_ICASE : RX_MULTI;
        }
        a.kind = SA_RE;
        a.rx   = fksedregex(sp, re, flags);
    }
    return a;
}

// Text argument of a, i, c in either the classic or one-line form.
static Str fksedtext(FkSedParse *sp)
{
    FkProg *p = sp->p;
    Str     s = sp->s;
    fksedspaces(sp);
    if (fksedeof(sp)) {
        fksederr(sp, "expected \\ after `a', `c' or `i'");
        return {};
    }
    b32 line = 1;  // there is a line of text, even if empty
    if (s[sp->i] == '\\') {
        sp->i++;
        line = !fksedeof(sp);
        if (line && s[sp->i]=='\n') sp->i++;
    }
    Buf b(p->a, 64);
    while (!fksedeof(sp)) {
        u8 c = s[sp->i++];
        if (c == '\n') break;
        if (c=='\\' && !fksedeof(sp)) {
            c = s[sp->i++];
        }
        putbyte(&b, c);
    }
    return line ? finish(&b) : Str{};
}

static b32 fksedrepl(FkSedParse *sp, FkSedCmd *cmd, Str r, i32 ngroups)
{
    FkProg *p = sp->p;
    Slice<FkSedPart> parts = {};
    Buf lit(p->a, 64);
    u8  pending = 0;
    for (iz i = 0; i < r.len; i++) {
        u8 c = r[i];
        i32 group = -1;
        u8  casec = 0;
        if (c == '&') {
            group = 0;
        } else if (c=='\\' && i+1<r.len) {
            u8 e = r[++i];
            if (digit(e)) {
                group = e - '0';
                if (group > ngroups) {
                    Buf m(p->a, 64);
                    print(&m, "invalid reference \\");
                    putbyte(&m, e);
                    print(&m, " on `s' command's RHS");
                    fksederr(sp, finish(&m));
                    return 0;
                }
            } else if (e=='L' || e=='U' || e=='E' || e=='l' || e=='u') {
                casec = e;
            } else if (e=='d' || e=='o' || e=='x' || e=='c') {
                iz  j = i;
                i32 v = fksedchar(r, &j);
                putbyte(&lit, v<0 ? e : (u8)v);
                if (v >= 0) i = j - 1;
                continue;
            } else {
                switch (e) {
                case 'n': e = '\n'; break;
                case 't': e = '\t'; break;
                case 'a': e = '\a'; break;
                case 'f': e = '\f'; break;
                case 'v': e = '\v'; break;
                case 'r': e = '\r'; break;
                }
                putbyte(&lit, e);
                continue;
            }
        } else {
            putbyte(&lit, c);
            continue;
        }
        if (lit.len || pending) {
            FkSedPart t = {-1, clone(p->a, finish(&lit)), pending};
            parts = push(p->a, parts, t);
            lit.len = 0;
            pending = 0;
        }
        if (casec) {
            pending = casec;
            continue;
        }
        FkSedPart g = {group, {}, pending};
        parts = push(p->a, parts, g);
        pending = 0;
    }
    if (lit.len || pending) {
        FkSedPart t = {-1, clone(p->a, finish(&lit)), pending};
        parts = push(p->a, parts, t);
    }
    cmd->repl  = parts.data;
    cmd->nrepl = parts.len;
    return 1;
}

// The file name of w or s///w, to the end of the line, opened now as
// GNU opens them all before reading any input.
static FkOut *fksedwfile(FkSedParse *sp)
{
    FkProg *p = sp->p;
    Str     s = sp->s;
    fksedspaces(sp);
    iz b = sp->i;
    for (; !fksedeof(sp) && s[sp->i]!='\n'; sp->i++) {}
    Str name = span(s.data+b, s.data+sp->i);
    if (!name.len) {
        fksederr(sp, "missing filename in r/R/w/W commands");
        return 0;
    }
    if (name == "/dev/stdout") return &p->out;
    if (name == "/dev/stderr") return &p->err;
    for (iz i = 0; i < sp->files.len; i++) {
        if (sp->files[i].name == name) return sp->files[i].out;
    }
    Opened f = os_open(p->os, *p->a, fkpath(p, name), OF_WRITE|OF_CREATE|OF_TRUNC, 0666);
    if (f.err) {
        print(diag(p), "couldn't open file ");
        print(&p->err, name);
        enddiag(p, f.err);
        sp->failed = sp->panic = 1;
        return 0;
    }
    FkSedFile file = {name, alloc<FkOut>(p->a)};
    *file.out = fkout(p->os, p->a, f.h, 1<<12);
    sp->files = push(p->a, sp->files, file);
    return file.out;
}

static Slice<FkSedCmd> fksedparse(FkSedParse *sp)
{
    FkProg *p = sp->p;
    Str     s = sp->s;
    Slice<FkSedCmd> cmds = {};
    Slice<iz> blocks = {};
    if (s.len>=2 && s[0]=='#' && s[1]=='n' && (s.len==2 || s[2]=='\n')) {
        sp->quiet = 1;
    }
    for (;;) {
        for (; !fksedeof(sp); sp->i++) {
            u8 c = s[sp->i];
            if (!(c==' ' || c=='\t' || c=='\n' || c==';')) break;
        }
        if (fksedeof(sp)) break;
        if (s[sp->i] == '#') {
            for (; !fksedeof(sp) && s[sp->i]!='\n'; sp->i++) {}
            continue;
        }
        FkSedCmd c = {};
        c.a1 = fksedaddr(sp, 0);
        if (sp->failed) return cmds;
        if (c.a1.kind) {
            c.naddr = 1;
            fksedspaces(sp);
            if (!fksedeof(sp) && s[sp->i]==',') {
                sp->i++;
                fksedspaces(sp);
                c.a2 = fksedaddr(sp, 1);
                if (sp->failed) return cmds;
                if (!c.a2.kind) {
                    fksederr(sp, "unexpected `,'");
                    return cmds;
                }
                c.naddr = 2;
            }
        }
        fksedspaces(sp);
        if (!fksedeof(sp) && s[sp->i]=='!') {
            c.neg = 1;
            sp->i++;
            fksedspaces(sp);
            if (!fksedeof(sp) && s[sp->i]=='!') {
                sp->i++;
                fksederr(sp, "multiple `!'s");
                return cmds;
            }
        }
        if (fksedeof(sp)) {
            fksederr(sp, "missing command");
            return cmds;
        }
        c.cmd = s[sp->i++];
        if (c.a1.kind==SA_ZERO && (c.naddr!=2 || c.a2.kind!=SA_RE)) {
            fksederr(sp, "invalid usage of line address 0");
            return cmds;
        }
        switch (c.cmd) {
        case '{':
            blocks = push(p->a, blocks, cmds.len);
            break;
        case '}':
            if (!blocks.len) {
                fksederr(sp, "unexpected `}'");
                return cmds;
            }
            if (c.naddr) {
                fksederr(sp, "unexpected `}'");
                return cmds;
            }
            cmds[blocks[blocks.len-1]].jump = cmds.len;
            blocks.len--;
            if (!fksedcmdend(sp)) return cmds;
            break;
        case '=': case 'd': case 'D': case 'g': case 'G': case 'h': case 'H':
        case 'n': case 'N': case 'p': case 'P': case 'x': case 'z': case 'F':
            if (!fksedcmdend(sp)) return cmds;
            break;
        case 'l':
            fksedspaces(sp);
            for (; !fksedeof(sp) && digit(s[sp->i]); sp->i++) {
                c.nth = c.nth*10 + s[sp->i] - '0';
                if (c.nth > 1<<20) c.nth = 1<<20;
            }
            if (!fksedcmdend(sp)) return cmds;
            break;
        case 'v':
            for (; !fksedeof(sp) && !cut(Str(";\n}#"), s[sp->i]).ok; sp->i++) {}
            break;
        case 'w':
            c.wfile = fksedwfile(sp);
            if (sp->failed) return cmds;
            break;
        case 'q': case 'Q':
            fksedspaces(sp);
            for (; !fksedeof(sp) && digit(s[sp->i]); sp->i++) {
                c.status = (i32)fkaccum(c.status, s[sp->i], (i64)1<<30);
            }
            if (!fksedcmdend(sp)) return cmds;
            break;
        case 'a': case 'i': case 'c':
            c.text = fksedtext(sp);
            if (sp->failed) return cmds;
            break;
        case ':': {
            if (c.naddr) {
                fksederr(sp, ": doesn't want any addresses");
                return cmds;
            }
            fksedspaces(sp);
            iz b = sp->i;
            for (; !fksedeof(sp) && s[sp->i]!='\n' && s[sp->i]!=';'; sp->i++) {}
            c.label = trimright(span(s.data+b, s.data+sp->i));
            if (!c.label.len) {
                fksederr(sp, "\":\" lacks a label");
                return cmds;
            }
        } break;
        case 'b': case 't': case 'T': {
            fksedspaces(sp);
            iz b = sp->i;
            for (; !fksedeof(sp) && s[sp->i]!='\n' && s[sp->i]!=';' && s[sp->i]!='}'; sp->i++) {}
            c.label = trimright(span(s.data+b, s.data+sp->i));
        } break;
        case 's': {
            if (fksedeof(sp) || s[sp->i]=='\n' || s[sp->i]=='\\') {
                fksederr(sp, "unterminated `s' command");
                return cmds;
            }
            u8  delim = s[sp->i++];
            b32 ok = 0;
            Str re = fkseddelimited(sp, delim, 1, &ok);
            if (!ok) {
                fksederr(sp, "unterminated `s' command");
                return cmds;
            }
            Str rep = fkseddelimited(sp, delim, 0, &ok);
            if (!ok) {
                fksederr(sp, "unterminated `s' command");
                return cmds;
            }
            i32 flags = sp->flags;
            b32 more = 1;
            while (more && !fksedeof(sp)) {
                u8 f = s[sp->i];
                switch (f) {
                case 'g':
                    sp->i++;
                    if (c.global) {
                        fksederr(sp, "multiple `g' options to `s' command");
                        return cmds;
                    }
                    c.global = 1;
                    break;
                case 'p':
                    sp->i++;
                    if (c.print) {
                        fksederr(sp, "multiple `p' options to `s' command");
                        return cmds;
                    }
                    c.print = 1;
                    break;
                case 'i': case 'I':
                    sp->i++;
                    flags |= RX_ICASE;
                    break;
                case 'm': case 'M':
                    sp->i++;
                    flags |= RX_MULTI;
                    break;
                case 'w':
                    sp->i++;
                    c.wfile = fksedwfile(sp);
                    if (sp->failed) return cmds;
                    more = 0;
                    break;
                default:
                    if (digit(f)) {
                        if (c.nth) {
                            sp->i++;
                            fksederr(sp, "multiple number options to `s' command");
                            return cmds;
                        }
                        i64 n = 0;
                        for (; !fksedeof(sp) && digit(s[sp->i]); sp->i++) {
                            n = fkaccum(n, s[sp->i]);
                        }
                        if (!n) {
                            fksederr(sp, "number option to `s' command may not be zero");
                            return cmds;
                        }
                        c.nth = n;
                    } else if (f==';' || f=='\n' || f=='}' || f=='#' || f==' ' || f=='\t') {
                        more = 0;
                    } else {
                        sp->i++;
                        fksederr(sp, "unknown option to `s'");
                        return cmds;
                    }
                }
            }
            c.rx = fksedregex(sp, re, flags);
            if (sp->failed) return cmds;
            i32 ngroups = c.rx ? c.rx->ngroups : 9;
            if (!fksedrepl(sp, &c, rep, ngroups)) return cmds;
            if (!fksedcmdend(sp)) return cmds;
        } break;
        case 'y': {
            if (fksedeof(sp)) {
                fksederr(sp, "unterminated `y' command");
                return cmds;
            }
            u8  delim = s[sp->i++];
            Str parts[2] = {};
            for (i32 k = 0; k < 2; k++) {
                Buf b(p->a, 64);
                b32 ok = 0;
                while (!fksedeof(sp)) {
                    u8 ch = s[sp->i++];
                    if (ch == delim) {
                        ok = 1;
                        break;
                    }
                    if (ch=='\\' && !fksedeof(sp)) {
                        u8  e = s[sp->i];
                        iz  j = sp->i;
                        i32 v = fksedchar(s, &j);
                        sp->i++;
                        switch (e) {
                        case 'n': ch = '\n'; break;
                        case 't': ch = '\t'; break;
                        case 'a': ch = '\a'; break;
                        case 'f': ch = '\f'; break;
                        case 'v': ch = '\v'; break;
                        case 'r': ch = '\r'; break;
                        default:
                            if (v >= 0) {
                                ch    = (u8)v;
                                sp->i = j;
                            } else if (e=='\\' || e==delim) {
                                ch = e;
                            } else {
                                putbyte(&b, '\\');
                                ch = e;
                            }
                        }
                    }
                    putbyte(&b, ch);
                }
                if (!ok) {
                    fksederr(sp, "unterminated `y' command");
                    return cmds;
                }
                parts[k] = finish(&b);
            }
            // Characters, which in UTF-8 may span several bytes
            Slice<Str> chars[2] = {};
            for (i32 k = 0; k < 2; k++) {
                for (iz i = 0; i < parts[k].len;) {
                    iz n = p->utf8 ? fkchar(parts[k], i).len : 1;
                    chars[k] = push(p->a, chars[k], Str(parts[k].data+i, n));
                    i += n;
                }
            }
            if (chars[0].len != chars[1].len) {
                fksederr(sp, "strings for `y' command are different lengths");
                return cmds;
            }
            c.map = alloc<u8>(p->a, 256);
            for (i32 k = 0; k < 256; k++) c.map[k] = (u8)k;
            if (parts[0].len==chars[0].len && parts[1].len==chars[1].len) {
                for (iz k = 0; k < parts[0].len; k++) c.map[parts[0][k]] = parts[1][k];
            } else {
                c.yfrom = chars[0];
                c.yto   = chars[1];
            }
            if (!fksedcmdend(sp)) return cmds;
        } break;
        default: {
            Buf m(p->a, 32);
            print(&m, "unknown command: `");
            putbyte(&m, c.cmd);
            putbyte(&m, '\'');
            fksederr(sp, finish(&m));
            return cmds;
        }
        }
        cmds = push(p->a, cmds, c);
    }
    if (blocks.len) {
        fksederr(sp, "unmatched `{'", 1);
        return cmds;
    }
    // Resolve branch labels
    for (iz i = 0; i < cmds.len; i++) {
        FkSedCmd *c = cmds.data + i;
        if (c->cmd!='b' && c->cmd!='t' && c->cmd!='T') continue;
        c->jump = cmds.len;
        if (!c->label.len) continue;
        iz j = 0;
        for (; j<cmds.len && !(cmds[j].cmd==':' && cmds[j].label==c->label); j++) {}
        if (j == cmds.len) {
            print(diag(p), "can't find label for jump to `");
            print(&p->err, c->label);
            putbyte(&p->err, '\'');
            enddiag(p);
            sp->failed = 1;
            sp->panic  = 1;
            return cmds;
        }
        c->jump = j;
    }
    return cmds;
}

static void fksedout(FkSed *s, Str text, b32 nl)
{
    if (s->missingnl) {
        putbyte(s->out, s->eol);
        s->missingnl = 0;
    }
    print(s->out, text);
    if (nl) {
        putbyte(s->out, s->eol);
    } else {
        s->missingnl = 1;
    }
}

static b32 fksedread(FkSed *s, b32 append)
{
    if (s->next >= s->lines.len) return 0;
    if (append) {
        putbyte(&s->ps, s->eol);
    } else {
        s->ps.len = 0;
    }
    print(&s->ps, s->lines[s->next]);
    s->curnl = s->nls[s->next];
    s->filename = s->names[s->next];
    s->next++;
    s->lineno++;
    return 1;
}

// Text of a, i or c. No text at all, as from "a\" ending the script,
// adds nothing but the newline that output may be owed.
static void fksedtextout(FkSed *s, Str text)
{
    if (text.data) {
        fksedout(s, text, 1);
    } else if (s->missingnl) {
        putbyte(s->out, s->eol);
        s->missingnl = 0;
    }
}

static void fkseddump(FkSed *s)
{
    for (iz i = 0; i < s->append.len; i++) fksedtextout(s, s->append[i]);
    s->append.len = 0;
}

static b32 fksedre(FkSed *s, FkRx *rx, Str text)
{
    if (!rx) rx = s->lastrx;
    if (!rx) return 0;
    s->lastrx = rx;
    return fkrxexec(rx, text, 0, *s->p->a, 0).ok;
}

static b32 fksedaddrmatch(FkSed *s, FkSedAddr *a)
{
    switch (a->kind) {
    case SA_LINE: return s->lineno == a->n;
    case SA_LAST: return s->next >= s->lines.len;
    case SA_RE:   return fksedre(s, a->rx, finish(&s->ps));
    case SA_STEP:
        if (a->step <= 0) return s->lineno == a->n;
        return s->lineno>=a->n && (s->lineno - a->n)%a->step == 0;
    }
    return 0;
}

static b32 fksedselect(FkSed *s, FkSedCmd *c)
{
    b32 r = 0;
    if (c->naddr == 0) {
        r = 1;
    } else if (c->naddr == 1) {
        r = fksedaddrmatch(s, &c->a1);
    } else if (!c->active) {
        b32 start = c->a1.kind==SA_ZERO ? s->lineno==1 : fksedaddrmatch(s, &c->a1);
        if (start) {
            r = 1;
            switch (c->a2.kind) {
            case SA_LINE:
                c->active = c->a2.n > s->lineno;
                break;
            case SA_PLUS:
                c->rangeend = s->lineno + c->a2.n;
                c->active   = c->a2.n > 0;
                break;
            case SA_MULT:
                c->active = c->a2.n > 0;  // even when addr1 is a multiple
                break;
            case SA_LAST:
                c->active = s->next < s->lines.len;
                break;
            case SA_STEP:
                c->active = 1;
                if (c->a2.step>0 && s->lineno%c->a2.step==0) c->active = 0;
                break;
            case SA_RE:
                c->active = 1;
                if (c->a1.kind==SA_ZERO && fksedre(s, c->a2.rx, finish(&s->ps))) {
                    c->active = 0;
                }
                break;
            }
        }
    } else {
        r = 1;
        switch (c->a2.kind) {
        case SA_LINE:
            if (s->lineno >= c->a2.n) c->active = 0;
            break;
        case SA_PLUS:
            if (s->lineno >= c->rangeend) c->active = 0;
            break;
        case SA_MULT:
            if (s->lineno%c->a2.n == 0) c->active = 0;
            break;
        case SA_LAST:
            if (s->next >= s->lines.len) c->active = 0;
            break;
        case SA_STEP:
            if (c->a2.step<=0 || s->lineno%c->a2.step==0) c->active = 0;
            break;
        case SA_RE:
            if (fksedre(s, c->a2.rx, finish(&s->ps))) c->active = 0;
            break;
        }
    }
    return r != c->neg;
}

static void fksedcase(Buf *b, Str t, u8 *mode, u8 *one, b32 utf8)
{
    for (iz i = 0; i < t.len; i++) {
        u8 c = t[i];
        u8 m = *one ? *one : *mode;
        FkChar ch = utf8 && c>=0x80 ? fkchar(t, i) : FkChar{-1, 1};
        if (m && ch.c>=0) {
            u8 tmp[4];
            i32 to = m=='U' || m=='u' ? fkupper(ch.c) : fklower(ch.c);
            print(b, {tmp, utf8encode(tmp, to)});
            i += ch.len - 1;
            *one = 0;
            continue;
        }
        if (m=='U' || m=='u') {
            c = c>='a' && c<='z' ? (u8)(c-32) : c;
        } else if (m=='L' || m=='l') {
            c = lowercase(c);
        }
        *one = 0;
        putbyte(b, c);
    }
}

static b32 fksedsubst(FkSed *s, FkSedCmd *c)
{
    FkRx *rx = c->rx ? c->rx : s->lastrx;
    if (!rx) return 0;
    s->lastrx = rx;
    Arena scratch = *s->p->a;
    Str   in  = clone(&scratch, finish(&s->ps));
    Buf   out(&scratch, in.len + 64);
    iz    pos = 0, prevend = -1;
    i64   count = 0;
    b32   did = 0;
    while (pos <= in.len) {
        FkMatch m = fkrxexec(rx, in, pos, scratch, 1);
        if (!m.ok) break;
        if (m.beg==m.end && m.beg==prevend) {
            print(&out, span(in.data+pos, in.data+m.beg));
            if (m.beg < in.len) putbyte(&out, in[m.beg]);
            pos = m.beg + 1;
            continue;
        }
        count++;
        print(&out, span(in.data+pos, in.data+m.beg));
        b32 replace = c->global ? count >= (c->nth ? c->nth : 1)
                                : count == (c->nth ? c->nth : 1);
        if (replace) {
            u8 mode = 0, one = 0;
            for (iz k = 0; k < c->nrepl; k++) {
                FkSedPart *pt = c->repl + k;
                if (pt->casec == 'L' || pt->casec == 'U') {
                    mode = pt->casec;
                } else if (pt->casec == 'E') {
                    mode = 0;
                } else if (pt->casec) {
                    one = pt->casec;
                }
                Str t = pt->text;
                if (pt->group == 0) {
                    t = span(in.data+m.beg, in.data+m.end);
                } else if (pt->group > 0) {
                    iz b = pt->group<=m.nsub ? m.sub[2*pt->group-2] : -1;
                    iz e = pt->group<=m.nsub ? m.sub[2*pt->group-1] : -1;
                    t = b>=0 && e>=b ? span(in.data+b, in.data+e) : Str{};
                }
                fksedcase(&out, t, &mode, &one, s->p->utf8);
            }
            did = 1;
        } else {
            print(&out, span(in.data+m.beg, in.data+m.end));
        }
        prevend = m.end;
        if (m.end == m.beg) {
            if (m.beg < in.len) putbyte(&out, in[m.beg]);
            pos = m.end + 1;
        } else {
            pos = m.end;
        }
        if (did && !c->global) break;
    }
    if (!did) return 0;
    if (pos < in.len) print(&out, cuthead(in, pos));
    s->ps.len = 0;
    print(&s->ps, finish(&out));
    return 1;
}

// y over characters that span several bytes.
static void fksedmap(FkSed *s, FkSedCmd *c)
{
    Str in = clone(s->p->a, finish(&s->ps));
    s->ps.len = 0;
    for (iz i = 0; i < in.len;) {
        iz  n = fkchar(in, i).len;
        Str ch = {in.data+i, n};
        iz  k = 0;
        for (; k<c->yfrom.len && !(c->yfrom[k]==ch); k++) {}
        print(&s->ps, k<c->yfrom.len ? c->yto[k] : ch);
        i += n;
    }
}

// The l command: escapes, and lines wrapped to width with a backslash.
// Like GNU's, it works on bytes even in UTF-8.
static void fksedlist(FkSed *s, Str t, i64 width)
{
    Arena scratch = *s->p->a;
    Buf   line(&scratch, 128);
    for (iz i = 0; i < t.len; i++) {
        u8  tmp[4];
        Str esc = {};
        u8  c = t[i];
        if (c == '\\') {
            esc = "\\\\";
        } else if (c>=0x20 && c<0x7f) {
            esc = {t.data+i, 1};
        } else {
            u8 const *names = (u8 const *)"\aa\bb\ff\nn\rr\tt\vv";
            tmp[0] = '\\';
            esc = {tmp, 4};
            for (i32 k = 0; k < 14; k += 2) {
                if (c == names[k]) {
                    tmp[1] = names[k+1];
                    esc.len = 2;
                }
            }
            if (esc.len == 4) {
                tmp[1] = (u8)('0' + (c>>6));
                tmp[2] = (u8)('0' + (c>>3 & 7));
                tmp[3] = (u8)('0' + (c & 7));
            }
        }
        if (width>0 && line.len+esc.len > width-1) {
            print(&line, "\\");
            fksedout(s, finish(&line), 1);
            line.len = 0;
        }
        print(&line, esc);
    }
    print(&line, "$");
    fksedout(s, finish(&line), 1);
}

// Run the script over the loaded lines.
static void fksedrun(FkSed *s)
{
    while (!s->quit && fksedread(s, 0)) {
        b32 autoprint = !s->quiet;
        b32 restart   = 0;
        s->tflag = 0;
        for (;;) {
            restart = 0;
            iz pc = 0;
            for (; pc<s->ncmds && !s->quit; pc++) {
                FkSedCmd *c = s->cmds + pc;
                if (c->cmd == '}') continue;
                if (!fksedselect(s, c)) {
                    if (c->cmd == '{') pc = c->jump;
                    continue;
                }
                switch (c->cmd) {
                case '{': case ':': break;
                case '=': {
                    u8 tmp[24];
                    fksedout(s, fkfmtint(tmp+countof(tmp), s->lineno), 1);
                } break;
                case 'a':
                    s->append = push(s->p->a, s->append, c->text);
                    break;
                case 'i':
                    fksedtextout(s, c->text);
                    break;
                case 'c':
                    if (c->naddr<2 || !c->active || c->neg) fksedtextout(s, c->text);
                    autoprint = 0;
                    pc = s->ncmds;
                    break;
                case 'd':
                    autoprint = 0;
                    pc = s->ncmds;
                    break;
                case 'D': {
                    Cut k = cut(finish(&s->ps), s->eol);
                    if (!k.ok) {
                        autoprint = 0;
                        pc = s->ncmds;
                        break;
                    }
                    Str rest = clone(s->p->a, k.tail);
                    s->ps.len = 0;
                    print(&s->ps, rest);
                    fkseddump(s);
                    restart = 1;
                    pc = s->ncmds;
                } break;
                // Each space knows whether its text ended in a newline,
                // and copies carry that along
                case 'g':
                    s->ps.len = 0;
                    print(&s->ps, finish(&s->hs));
                    s->curnl = s->holdnl;
                    break;
                case 'G':
                    putbyte(&s->ps, s->eol);
                    print(&s->ps, finish(&s->hs));
                    s->curnl = s->holdnl;
                    break;
                case 'h':
                    s->hs.len = 0;
                    print(&s->hs, finish(&s->ps));
                    s->holdnl = s->curnl;
                    break;
                case 'H':
                    putbyte(&s->hs, s->eol);
                    print(&s->hs, finish(&s->ps));
                    s->holdnl = s->curnl;
                    break;
                case 'x': {
                    Buf t = s->ps;
                    s->ps = s->hs;
                    s->hs = t;
                    b32 nl = s->curnl;
                    s->curnl  = s->holdnl;
                    s->holdnl = nl;
                } break;
                case 'z':
                    s->ps.len = 0;
                    break;
                case 'n':
                    if (s->next >= s->lines.len) {
                        s->quit = 1;
                        pc = s->ncmds;
                        break;
                    }
                    if (!s->quiet) fksedout(s, finish(&s->ps), s->curnl);
                    fkseddump(s);
                    fksedread(s, 0);
                    break;
                case 'N':
                    if (s->next >= s->lines.len) {
                        s->quit = 1;
                        pc = s->ncmds;
                        break;
                    }
                    fkseddump(s);
                    fksedread(s, 1);
                    break;
                case 'p':
                    fksedout(s, finish(&s->ps), s->curnl);
                    break;
                case 'P': {
                    Cut k = cut(finish(&s->ps), s->eol);
                    fksedout(s, k.head, k.ok || s->curnl);
                } break;
                case 'q':
                    s->status = c->status;
                    s->quit   = 1;
                    break;
                case 'Q':
                    s->status = c->status;
                    s->quit   = 1;
                    autoprint = 0;
                    break;
                case 's':
                    if (fksedsubst(s, c)) {
                        s->tflag = 1;
                        if (c->print) fksedout(s, finish(&s->ps), s->curnl);
                        if (c->wfile) {
                            print(c->wfile, finish(&s->ps));
                            if (s->curnl) putbyte(c->wfile, s->eol);
                        }
                    }
                    break;
                case 'y':
                    if (c->yfrom.len) {
                        fksedmap(s, c);
                        break;
                    }
                    for (iz k = 0; k < s->ps.len; k++) s->ps.data[k] = c->map[s->ps.data[k]];
                    break;
                case 'l':
                    fksedlist(s, finish(&s->ps), c->nth ? c->nth : 70);
                    break;
                case 'F':
                    fksedout(s, s->filename, 1);
                    break;
                case 'w':
                    print(c->wfile, finish(&s->ps));
                    if (s->curnl) putbyte(c->wfile, s->eol);
                    break;
                case 'b':
                    pc = c->jump - 1;
                    break;
                case 't':
                    if (s->tflag) {
                        s->tflag = 0;
                        pc = c->jump - 1;
                    }
                    break;
                case 'T':
                    if (!s->tflag) {
                        pc = c->jump - 1;
                    } else {
                        s->tflag = 0;
                    }
                    break;
                }
            }
            if (!restart) break;
        }
        if (autoprint) fksedout(s, finish(&s->ps), s->curnl);
        fkseddump(s);
    }
}

static void fksedload(FkSed *s, Str data, Str name)
{
    while (data.len) {
        FkLine l = fknextline(data, s->eol);
        s->lines = push(s->p->a, s->lines, l.line);
        s->nls   = push(s->p->a, s->nls, l.nl);
        s->names = push(s->p->a, s->names, name);
        data = l.rest;
    }
}

// Rewrite sed's long options as short ones, dropping those that change
// nothing here.
static void fksedlong(FkProg *p)
{
    FakeCtx *ctx  = p->ctx;
    Str     *argv = alloc<Str>(p->a, 2*ctx->argc+1);
    iz       argc = 0;
    b32      done = 0;
    Str longs[] = {
        "--expression", "--file", "--in-place", "--quiet", "--silent",
        "--regexp-extended", "--separate", "--null-data", "--unbuffered",
        "--posix", "--follow-symlinks", "--sandbox", "--debug",
        "--line-length", "--help", "--version",
    };
    for (iz i = 0; i < ctx->argc; i++) {
        Str a = ctx->argv[i];
        Cut eq = cut(a, '=');
        if (!done && i && startswith(a, "--") && eq.head.len>2) {
            // A unique abbreviation stands for the whole name
            iz  hits = 0;
            Str full = {};
            for (iz k = 0; k < countof(longs); k++) {
                if (startswith(longs[k], eq.head)) {
                    hits++;
                    full = longs[k];
                }
                if (longs[k] == eq.head) {
                    hits = 1;
                    full = longs[k];
                    break;
                }
            }
            if (hits == 1) {
                eq.head = full;
                a = eq.ok ? concat(p->a, concat(p->a, full, Str("=")), eq.tail) : full;
            }
        }
        if (done || i==0 || !startswith(a, "--") || a=="--") {
            done |= a == "--";
            argv[argc++] = a;
        } else if (eq.head=="--expression" || eq.head=="--file") {
            argv[argc++] = eq.head=="--file" ? Str("-f") : Str("-e");
            if (eq.ok) argv[argc++] = eq.tail;
        } else if (eq.head == "--in-place") {
            argv[argc++] = eq.ok ? concat(p->a, Str("-i"), eq.tail) : Str("-i");
        } else if (a=="--quiet" || a=="--silent") {
            argv[argc++] = "-n";
        } else if (a == "--regexp-extended") {
            argv[argc++] = "-E";
        } else if (a == "--separate") {
            argv[argc++] = "-s";
        } else if (a == "--null-data") {
            argv[argc++] = "-z";
        } else if (a == "--unbuffered") {
            argv[argc++] = "-u";
        } else if (!(a=="--posix" || a=="--follow-symlinks" || a=="--sandbox")) {
            argv[argc++] = a;
        }
    }
    ctx->argv = argv;
    ctx->argc = argc;
}

static i32 fake_sed(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    fksedlong(&p);
    FkOpt  o = fkopts(&p, 1);
    b32    quiet = 0, inplace = 0, separate = 0;
    Str    suffix = {};
    u8     eol = '\n';
    i32    flags = RX_SED | (p.utf8 ? RX_UTF8 : 0);
    Buf    script(p.a, 256);
    Slice<iz> chunks = {};
    for (i32 ch; (ch = fkgetopt(&o, "ne:f:i::Ersuz")) != -1;) {
        switch (ch) {
        case 'n': quiet = 1; break;
        case 'e':
            if (chunks.len) putbyte(&script, '\n');
            chunks = push(p.a, chunks, script.len);
            print(&script, o.arg);
            break;
        case 'f': {
            FkIn in = fkopenin(&p, o.arg);
            Str  data = {};
            if (!in.err) {
                data = fkreadall(&p, in.h);
                fkclosein(&p, &in);
                if (!data.data) in.err = E_IO;
            }
            if (in.err) {
                print(diag(&p), "couldn't open file ");
                print(&p.err, o.arg);
                enddiag(&p, in.err);
                return fkexit(&p, 4);
            }
            if (data.len && data[data.len-1]=='\n') data = cuttail(data, 1);
            if (chunks.len) putbyte(&script, '\n');
            chunks = push(p.a, chunks, script.len);
            print(&script, data);
        } break;
        case 'i':
            inplace = separate = 1;
            suffix  = o.arg;
            break;
        case 'E':
        case 'r': flags |= RX_ERE; break;
        case 's': separate = 1; break;
        case 'u': break;
        case 'z': eol = 0; break;
        default:  return fkusage(&p, 1);
        }
    }
    iz k = 0;
    if (!chunks.len) {
        if (!o.nrest) {
            print(&p.err, "Usage: ");
            print(&p.err, p.name);
            print(&p.err, " [OPTION]... {script-only-if-no-other-script} [input-file]...\n");
            flush(&p.err);
            return fkexit(&p, 1);
        }
        chunks = push(p.a, chunks, (iz)0);
        print(&script, o.rest[k++]);
    }

    FkSedParse sp = {};
    sp.p       = &p;
    sp.s       = finish(&script);
    sp.chunks  = chunks.data;
    sp.nchunks = chunks.len;
    sp.flags   = flags;
    Slice<FkSedCmd> cmds = fksedparse(&sp);
    if (sp.failed) return fkexit(&p, sp.panic ? 4 : 1);

    FkSed s = {};
    s.p     = &p;
    s.cmds  = cmds.data;
    s.ncmds = cmds.len;
    s.out   = &p.out;
    s.quiet = quiet || sp.quiet;
    s.ps    = Buf(p.a, 256);
    s.hs    = Buf(p.a, 64);
    s.holdnl = 1;
    s.eol    = eol;
    if (inplace && k==o.nrest) {
        print(diag(&p), "no input files");
        enddiag(&p);
        return fkexit(&p, 4);
    }
    if (k == o.nrest) o.rest[o.nrest++] = "-";

    i32 status = 0;
    if (!separate) {
        for (iz i = k; i < o.nrest; i++) {
            FkIn in = fkopenin(&p, o.rest[i]);
            if (!in.err && in.dir) {
                fkclosein(&p, &in);
                print(diag(&p), "read error on ");
                print(&p.err, o.rest[i]);
                enddiag(&p, E_ISDIR);
                status = 2;
                continue;
            }
            if (in.err) {
                print(diag(&p), "can't read ");
                print(&p.err, o.rest[i]);
                enddiag(&p, in.err);
                status = 2;
                continue;
            }
            fksedload(&s, fkreadall(&p, in.h), o.rest[i]);
            fkclosein(&p, &in);
        }
        fksedrun(&s);
    } else {
        for (iz i = k; i<o.nrest && !s.quit; i++) {
            Str  name = o.rest[i];
            Str  path = fkpath(&p, name);
            Stat st   = os_stat(os, *p.a, path, 1);
            if (inplace && !st.err && st.type!=FT_FILE) {
                print(diag(&p), "couldn't edit ");
                print(&p.err, name);
                print(&p.err, ": not a regular file");
                enddiag(&p);
                status = 4;
                continue;
            }
            FkIn in = fkopenin(&p, name);
            if (in.err) {
                print(diag(&p), "can't read ");
                print(&p.err, name);
                enddiag(&p, in.err);
                status = 2;
                continue;
            }
            Str data = fkreadall(&p, in.h);
            fkclosein(&p, &in);
            s.lines  = {};
            s.nls    = {};
            s.next   = 0;
            s.lineno = 0;
            s.names  = {};
            fksedload(&s, data, name);
            FkOut file = {};
            if (inplace && suffix.len) {
                Opened b = os_open(os, *p.a, concat(p.a, path, suffix),
                                   OF_WRITE|OF_CREATE|OF_TRUNC, st.mode & 0777);
                if (!b.err) {
                    os_write(os, b.h, data.data, data.len);
                    os_close(os, b.h);
                }
            }
            if (inplace) {
                Opened f = os_open(os, *p.a, path, OF_WRITE|OF_TRUNC, 0);
                if (f.err) {
                    print(diag(&p), "couldn't open file ");
                    print(&p.err, name);
                    enddiag(&p, f.err);
                    status = 4;
                    continue;
                }
                file = fkout(os, p.a, f.h, 1<<12);
                s.out = &file;
                s.missingnl = 0;
            }
            fksedrun(&s);
            if (inplace) {
                flush(&file);
                os_close(os, file.h);
                s.out = &p.out;
                s.missingnl = 0;
            }
        }
    }
    for (iz i = 0; i < sp.files.len; i++) {
        flush(sp.files[i].out);
        os_close(os, sp.files[i].out->h);
    }
    return fkexit(&p, s.quit && s.status ? s.status : status);
}


// expr

struct FkExpr {
    FkProg *p;
    Str    *args;
    iz      n;
    iz      i;
    i32     depth;    // nesting, bounded as GNU's stack bounds it
    i32     noeval;   // inside an operand that | or & skips
    b32     fail;     // a diagnostic was printed
    i32     status;   // its exit status
};

struct FkVal {
    Str s;
};

// Arbitrary precision integers, as GNU expr has them: a sign and the
// decimal digits of the magnitude, most significant first, no leading
// zeros (zero is the empty magnitude).
struct FkBig {
    b32 neg;
    Str mag;
};

static FkBig fkbig(Str s)  // from an integer string
{
    FkBig r = {};
    r.neg = s.len && s[0]=='-';
    Str m = cuthead(s, r.neg);
    while (m.len && m[0]=='0') m = cuthead(m, 1);
    r.mag = m;
    r.neg &= m.len > 0;
    return r;
}

static i32 fkmagcmp(Str a, Str b)
{
    if (a.len != b.len) return a.len<b.len ? -1 : 1;
    for (iz i = 0; i < a.len; i++) {
        if (a[i] != b[i]) return a[i]<b[i] ? -1 : 1;
    }
    return 0;
}

static i32 fkbigcmp(FkBig a, FkBig b)
{
    if (a.neg != b.neg) return a.neg ? -1 : 1;
    i32 c = fkmagcmp(a.mag, b.mag);
    return a.neg ? -c : c;
}

static Str fkmagtrim(u8 *beg, u8 *end)
{
    while (beg<end && *beg=='0') beg++;
    return span(beg, end);
}

static Str fkmagadd(Arena *a, Str x, Str y)
{
    iz  n = (x.len>y.len ? x.len : y.len) + 1;
    u8 *r = alloc<u8>(a, n);
    i32 carry = 0;
    for (iz i = 0; i < n; i++) {
        i32 d = carry;
        if (i < x.len) d += x[x.len-1-i] - '0';
        if (i < y.len) d += y[y.len-1-i] - '0';
        r[n-1-i] = (u8)('0' + d%10);
        carry = d / 10;
    }
    return fkmagtrim(r, r+n);
}

static Str fkmagsub(Arena *a, Str x, Str y)  // x >= y
{
    u8 *r = alloc<u8>(a, x.len+1);
    i32 borrow = 0;
    for (iz i = 0; i < x.len; i++) {
        i32 d = x[x.len-1-i] - '0' - borrow;
        if (i < y.len) d -= y[y.len-1-i] - '0';
        borrow = d < 0;
        r[x.len-1-i] = (u8)('0' + d + 10*borrow);
    }
    return fkmagtrim(r, r+x.len);
}

static Str fkmagmul(Arena *a, Str x, Str y)
{
    iz   n = x.len + y.len;
    i32 *t = alloc<i32>(a, n+1);
    for (iz i = 0; i < x.len; i++) {
        for (iz j = 0; j < y.len; j++) {
            t[i+j+1] += (x[i]-'0') * (y[j]-'0');
        }
    }
    for (iz k = n; k > 0; k--) {
        t[k-1] += t[k] / 10;
        t[k]   %= 10;
    }
    u8 *r = alloc<u8>(a, n);
    for (iz k = 0; k < n; k++) r[k] = (u8)('0' + t[k]);
    return fkmagtrim(r, r+n);
}

// Long division of magnitudes, quotient and remainder, the remainder
// worked in place so that memory stays linear.
static Str fkmagdiv(Arena *a, Str x, Str y, Str *rem)
{
    u8 *q = alloc<u8>(a, x.len+1);
    u8 *r = alloc<u8>(a, y.len+2);
    iz  n = 0;  // remainder digits, no leading zeros
    for (iz i = 0; i < x.len; i++) {
        if (n || x[i]!='0') r[n++] = x[i];
        u8 d = '0';
        while (fkmagcmp({r, n}, y) >= 0) {
            i32 borrow = 0;
            for (iz k = 0; k < n; k++) {
                i32 v = r[n-1-k] - '0' - borrow - (k<y.len ? y[y.len-1-k]-'0' : 0);
                borrow = v < 0;
                r[n-1-k] = (u8)('0' + v + 10*borrow);
            }
            iz z = 0;
            for (; z<n && r[z]=='0'; z++) {}
            for (iz k = z; k < n; k++) r[k-z] = r[k];
            n -= z;
            d++;
        }
        q[i] = d;
    }
    *rem = {r, n};
    return fkmagtrim(q, q+x.len);
}

static Str fkbigstr(Arena *a, b32 neg, Str mag)
{
    if (!mag.len) return "0";
    return neg ? concat(a, Str("-"), mag) : mag;
}

static FkVal fkexprnum(FkExpr *e, i64 v)
{
    u8 tmp[24];
    return {clone(e->p->a, fkfmtint(tmp+countof(tmp), v))};
}

static b32 fkexprnull(FkVal v)
{
    if (!v.s.len) return 1;
    if (!fkisint(v.s)) return 0;
    for (iz i = 0; i < v.s.len; i++) {
        if (v.s[i]!='-' && v.s[i]!='0') return 0;
    }
    return 1;
}

static void fkexprsyntax(FkExpr *e, Str msg, Str arg)
{
    if (e->fail) return;
    e->fail   = 1;
    e->status = 2;
    print(diag(e->p), "syntax error: ");
    print(&e->p->err, msg);
    if (arg.data) quote(&e->p->err, arg);
    enddiag(e->p);
}

static void fkexprerr(FkExpr *e, Str msg, i32 status)
{
    if (e->fail) return;
    e->fail   = 1;
    e->status = status;
    print(diag(e->p), msg);
    enddiag(e->p);
}

static b32 fkexprat(FkExpr *e, Str tok)
{
    return e->i<e->n && e->args[e->i]==tok;
}

static FkVal fkexpr0(FkExpr *e);

static FkVal fkexprmissing(FkExpr *e)
{
    fkexprsyntax(e, "missing argument after ", e->args[e->i-1]);
    return {};
}

static FkVal fkexpr7(FkExpr *e)
{
    if (e->fail) return {};
    if (e->i >= e->n) return fkexprmissing(e);
    if (fkexprat(e, "(")) {
        e->i++;
        if (e->i >= e->n) return fkexprmissing(e);
        FkVal v = fkexpr0(e);
        if (e->fail) return {};
        if (!fkexprat(e, ")")) {
            if (e->i >= e->n) {
                fkexprsyntax(e, "expecting ')' after ", e->args[e->i-1]);
            } else {
                fkexprsyntax(e, "expecting ')' instead of ", e->args[e->i]);
            }
            return {};
        }
        e->i++;
        return v;
    }
    if (fkexprat(e, ")")) {
        fkexprsyntax(e, "unexpected ')'", {});
        return {};
    }
    return {e->args[e->i++]};
}

// Characters, which are bytes outside a UTF-8 locale.
static FkChar fkexprch(FkExpr *e, Str s, iz i)
{
    return e->p->utf8 ? fkchar(s, i) : FkChar{s[i], 1};
}

static iz fkexprlen(FkExpr *e, Str s)
{
    return e->p->utf8 ? fkcharcount(s) : s.len;
}

static FkVal fkexprmatch(FkExpr *e, FkVal str, FkVal pat)
{
    if (e->noeval) return {Str((u8 *)"", 0)};
    FkRx rx = fkrxcompile(e->p->a, pat.s, e->p->utf8 ? RX_UTF8 : 0);
    if (rx.err.data) {
        fkexprerr(e, rx.err, 2);
        return {};
    }
    FkMatch m = fkrxexec(&rx, str.s, 0, *e->p->a, 1, 1);
    if (rx.ngroups) {
        if (!m.ok || m.sub[0]<0 || m.sub[1]<0) return {Str((u8 *)"", 0)};
        return {span(str.s.data+m.sub[0], str.s.data+m.sub[1])};
    }
    return fkexprnum(e, m.ok ? fkexprlen(e, takehead(str.s, m.end)) : 0);
}

// A position or length for substr: huge values are as good as infinite.
static i64 fkexprpos(Str s)
{
    if (!fkisint(s)) return -1;
    FkNum n = fkparseint(s);
    return n.ok ? n.v : s[0]=='-' ? -1 : (i64)1<<62;
}

static FkVal fkexpr6(FkExpr *e);

static FkVal fkexpr6x(FkExpr *e)
{
    if (e->fail) return {};
    if (e->i >= e->n) return fkexprmissing(e);
    if (fkexprat(e, "+")) {
        e->i++;
        if (e->i >= e->n) return fkexprmissing(e);
        return {e->args[e->i++]};
    }
    if (fkexprat(e, "length")) {
        e->i++;
        FkVal v = fkexpr6(e);
        return e->fail ? FkVal{} : fkexprnum(e, fkexprlen(e, v.s));
    }
    if (fkexprat(e, "match")) {
        e->i++;
        FkVal a = fkexpr6(e);
        FkVal b = fkexpr6(e);
        return e->fail ? FkVal{} : fkexprmatch(e, a, b);
    }
    if (fkexprat(e, "index")) {
        e->i++;
        FkVal a = fkexpr6(e);
        FkVal b = fkexpr6(e);
        if (e->fail) return {};
        iz k = 1;
        for (iz i = 0; i < a.s.len; k++) {
            FkChar x = fkexprch(e, a.s, i);
            for (iz j = 0; j < b.s.len;) {
                FkChar y = fkexprch(e, b.s, j);
                if (x.len==y.len && Str(a.s.data+i, x.len)==Str(b.s.data+j, y.len)) {
                    return fkexprnum(e, k);
                }
                j += y.len;
            }
            i += x.len;
        }
        return fkexprnum(e, 0);
    }
    if (fkexprat(e, "substr")) {
        e->i++;
        FkVal s = fkexpr6(e);
        FkVal a = fkexpr6(e);
        FkVal b = fkexpr6(e);
        if (e->fail) return {};
        i64 pos = fkexprpos(a.s);
        i64 len = fkexprpos(b.s);
        if (pos<1 || len<1) return {Str((u8 *)"", 0)};
        iz beg = 0;
        for (i64 k = 1; k<pos && beg<s.s.len; k++) beg += fkexprch(e, s.s, beg).len;
        iz end = beg;
        for (i64 k = 0; k<len && end<s.s.len; k++) end += fkexprch(e, s.s, end).len;
        return {span(s.s.data+beg, s.s.data+end)};
    }
    return fkexpr7(e);
}

// Every level of nesting, by parentheses or prefix keywords, passes
// through here. GNU expr recurses without a limit and dies of stack
// overflow when nesting is deep enough, so emulate that death.
static FkVal fkexpr6(FkExpr *e)
{
    if (e->depth >= 1000) {
        if (!e->fail) {
            e->fail   = 1;
            e->status = 128 + 11;  // SIGSEGV
        }
        return {};
    }
    e->depth++;
    FkVal v = fkexpr6x(e);
    e->depth--;
    return v;
}

static FkVal fkexpr5(FkExpr *e)
{
    FkVal l = fkexpr6(e);
    while (!e->fail && fkexprat(e, ":")) {
        e->i++;
        FkVal r = fkexpr6(e);
        if (e->fail) break;
        l = fkexprmatch(e, l, r);
    }
    return l;
}

static FkVal fkexprarith(FkExpr *e, FkVal l, FkVal r, u8 op)
{
    if (e->noeval) return {"0"};
    if (!fkisint(l.s) || !fkisint(r.s)) {
        fkexprerr(e, "non-integer argument", 2);
        return {};
    }
    Arena *a = e->p->a;
    FkBig  x = fkbig(l.s);
    FkBig  y = fkbig(r.s);
    switch (op) {
    case '-':
        y.neg = !y.neg && y.mag.len;
        [[fallthrough]];
    case '+':
        if (x.neg == y.neg) return {fkbigstr(a, x.neg, fkmagadd(a, x.mag, y.mag))};
        if (fkmagcmp(x.mag, y.mag) >= 0) {
            return {fkbigstr(a, x.neg, fkmagsub(a, x.mag, y.mag))};
        }
        return {fkbigstr(a, y.neg, fkmagsub(a, y.mag, x.mag))};
    }
    if (op!='*' && !y.mag.len) {
        fkexprerr(e, "division by zero", 2);
        return {};
    }
    if (x.mag.len>20000 || y.mag.len>20000) {
        // Quadratic here, where GMP is not: refuse rather than stall
        fkexprerr(e, "memory exhausted", 3);
        return {};
    }
    if (op == '*') {
        return {fkbigstr(a, x.neg!=y.neg, fkmagmul(a, x.mag, y.mag))};
    }
    Str rem;
    Str quo = fkmagdiv(a, x.mag, y.mag, &rem);
    if (op == '/') return {fkbigstr(a, x.neg!=y.neg, quo)};
    return {fkbigstr(a, x.neg, rem)};  // truncating, as C and GMP's tdiv
}

static FkVal fkexpr4(FkExpr *e)
{
    FkVal l = fkexpr5(e);
    while (!e->fail && (fkexprat(e, "*") || fkexprat(e, "/") || fkexprat(e, "%"))) {
        u8 op = e->args[e->i++][0];
        FkVal r = fkexpr5(e);
        if (e->fail) break;
        l = fkexprarith(e, l, r, op);
    }
    return l;
}

static FkVal fkexpr3(FkExpr *e)
{
    FkVal l = fkexpr4(e);
    while (!e->fail && (fkexprat(e, "+") || fkexprat(e, "-"))) {
        u8 op = e->args[e->i++][0];
        FkVal r = fkexpr4(e);
        if (e->fail) break;
        l = fkexprarith(e, l, r, op);
    }
    return l;
}

static FkVal fkexpr2(FkExpr *e)
{
    FkVal l = fkexpr3(e);
    for (;;) {
        if (e->fail || e->i>=e->n) return l;
        Str op = e->args[e->i];
        if (!(op=="<" || op=="<=" || op=="=" || op=="==" || op=="!=" ||
              op==">=" || op==">")) {
            return l;
        }
        e->i++;
        FkVal r = fkexpr3(e);
        if (e->fail) return l;
        i32 c = 0;
        if (fkisint(l.s) && fkisint(r.s)) {
            c = fkbigcmp(fkbig(l.s), fkbig(r.s));
        } else {
            c = compare(l.s, r.s);
        }
        b32 t = 0;
        if      (op == "<")  t = c <  0;
        else if (op == "<=") t = c <= 0;
        else if (op == ">")  t = c >  0;
        else if (op == ">=") t = c >= 0;
        else if (op == "!=") t = c != 0;
        else                 t = c == 0;
        l = fkexprnum(e, t);
    }
}

static FkVal fkexpr1(FkExpr *e)
{
    FkVal l = fkexpr2(e);
    while (!e->fail && fkexprat(e, "&")) {
        e->i++;
        b32 skip = fkexprnull(l);
        e->noeval += skip;
        FkVal r = fkexpr2(e);
        e->noeval -= skip;
        if (e->fail) break;
        if (skip || fkexprnull(r)) l = fkexprnum(e, 0);
    }
    return l;
}

static FkVal fkexpr0(FkExpr *e)
{
    FkVal l = fkexpr1(e);
    while (!e->fail && fkexprat(e, "|")) {
        e->i++;
        b32 skip = !fkexprnull(l);
        e->noeval += skip;
        FkVal r = fkexpr1(e);
        e->noeval -= skip;
        if (e->fail) break;
        if (!skip) l = fkexprnull(r) ? fkexprnum(e, 0) : r;
    }
    return l;
}

static i32 fake_expr(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkExpr e = {};
    e.p    = &p;
    e.args = ctx->argv + 1;
    e.n    = ctx->argc - 1;
    if (e.n && e.args[0]=="--") {
        e.args++;
        e.n--;
    }
    if (!e.n) {
        print(diag(&p), "missing operand");
        enddiag(&p);
        return fkusage(&p, 2);
    }
    FkVal v = fkexpr0(&e);
    if (!e.fail && e.i<e.n) {
        fkexprsyntax(&e, "unexpected argument ", e.args[e.i]);
    }
    if (e.fail) return fkexit(&p, e.status);
    print(&p.out, v.s);
    putbyte(&p.out, '\n');
    return fkexit(&p, fkexprnull(v));
}


// awk: only the tiny programs bash's test suite runs. A program is a
// list of "[NR==N|BEGIN|END] { statements }", where a statement is
// "print", "print $K[, $K...]" or "printf("%c", [0+]N)". Fields split on
// blanks, or on the -F character.

struct FkAwkRule {
    i32 when;   // 0: every record, 1: BEGIN, 2: END, 3: NR==nr
    i64 nr;
    Str action;
};

static Str fkawkfield(Str rec, i64 k, u8 fs)
{
    if (k == 0) return rec;
    i64 n = 0;
    if (fs) {
        for (Cut c = {{}, rec, 1}; c.ok;) {
            c = cut(c.tail, fs);
            if (++n == k) return c.head;
        }
        return {};
    }
    for (iz i = 0; i < rec.len;) {
        for (; i<rec.len && (rec[i]==' ' || rec[i]=='\t'); i++) {}
        if (i == rec.len) break;
        iz beg = i;
        for (; i<rec.len && rec[i]!=' ' && rec[i]!='\t'; i++) {}
        if (++n == k) return span(rec.data+beg, rec.data+i);
    }
    return {};
}

static Str fkawktrim(Str s)
{
    while (s.len && (s[0]==' ' || s[0]=='\t' || s[0]=='\n')) s = cuthead(s, 1);
    while (s.len && (s[s.len-1]==' ' || s[s.len-1]=='\t' || s[s.len-1]=='\n')) s = cuttail(s, 1);
    return s;
}

// Run one rule's statements on a record; false for an unknown statement.
static b32 fkawkrun(FkProg *p, Str action, Str rec, u8 fs)
{
    for (Cut st = {{}, action, 1}; st.ok;) {
        st = cut(st.tail, ';');
        Str s = fkawktrim(st.head);
        if (!s.len) continue;
        if (startswith(s, "printf")) {
            // printf("%c", 0+N)
            Str a = fkawktrim(cuthead(s, 6));
            if (!a.len || a[0]!='(' || a[a.len-1]!=')') return 0;
            a = fkawktrim(cuttail(cuthead(a, 1), 1));
            Cut c = cut(a, ',');
            if (!c.ok || fkawktrim(c.head)!="\"%c\"") return 0;
            Str v = fkawktrim(c.tail);
            if (startswith(v, "0+")) v = cuthead(v, 2);
            i64 n = 0;
            if (!v.len) return 0;
            for (iz i = 0; i < v.len; i++) {
                if (v[i]<'0' || v[i]>'9') return 0;
                n = n*10 + (v[i] - '0');
            }
            putbyte(&p->out, (u8)n);
            continue;
        }
        if (s!="print" && !startswith(s, "print ") && !startswith(s, "print$")) return 0;
        Str args = fkawktrim(cuthead(s, 5));
        if (!args.len) args = "$0";
        b32 first = 1;
        for (Cut c = {{}, args, 1}; c.ok; first = 0) {
            c = cut(c.tail, ',');
            Str f = fkawktrim(c.head);
            if (f.len<2 || f[0]!='$') return 0;
            i64 k = 0;
            for (iz i = 1; i < f.len; i++) {
                if (f[i]<'0' || f[i]>'9') return 0;
                k = k*10 + (f[i] - '0');
            }
            if (!first) putbyte(&p->out, ' ');
            print(&p->out, fkawkfield(rec, k, fs));
        }
        putbyte(&p->out, '\n');
    }
    return 1;
}

static i32 fake_awk(Os *os, FakeCtx *ctx)
{
    FkProg p = fkprog(os, ctx);
    FkOpt  o = fkopts(&p, 0);
    u8     fs = 0;
    for (i32 ch; (ch = fkgetopt(&o, "F:")) != -1;) {
        switch (ch) {
        case 'F': fs = o.arg.len ? o.arg[0] : 0; break;
        default:
            print(diag(&p), "usage: awk [-F fs] 'prog' [file ...]");
            enddiag(&p);
            return fkexit(&p, 2);
        }
    }
    if (!o.nrest) {
        print(diag(&p), "usage: awk [-F fs] 'prog' [file ...]");
        enddiag(&p);
        return fkexit(&p, 2);
    }

    // Parse the rules
    FkAwkRule rules[16];
    i32       nrules = 0;
    Str       prog   = o.rest[0];
    for (;;) {
        prog = fkawktrim(prog);
        if (!prog.len) break;
        Cut open = cut(prog, '{');
        Cut body = cut(open.tail, '}');
        if (!open.ok || !body.ok || nrules==countof(rules)) {
            print(diag(&p), "unsupported program");
            enddiag(&p);
            return fkexit(&p, 2);
        }
        FkAwkRule r = {};
        Str pat = fkawktrim(open.head);
        if (pat == "BEGIN") {
            r.when = 1;
        } else if (pat == "END") {
            r.when = 2;
        } else if (startswith(pat, "NR==")) {
            r.when = 3;
            for (iz i = 4; i < pat.len; i++) r.nr = r.nr*10 + (pat[i] - '0');
        } else if (pat.len) {
            print(diag(&p), "unsupported pattern");
            enddiag(&p);
            return fkexit(&p, 2);
        }
        r.action = body.head;
        rules[nrules++] = r;
        prog = body.tail;
    }

    b32 bad = 0;
    for (i32 i = 0; i < nrules; i++) {
        if (rules[i].when == 1) bad |= !fkawkrun(&p, rules[i].action, {}, fs);
    }
    b32 input = 0;
    for (i32 i = 0; i < nrules; i++) input |= rules[i].when != 1;
    i64 nr  = 0;
    Str last = {};
    i32 status = 0;
    if (!input) o.nrest = 1;  // BEGIN alone reads nothing
    else if (o.nrest == 1) o.rest[o.nrest++] = "-";
    for (iz f = 1; f < o.nrest; f++) {
        Str data = fkslurp(&p, o.rest[f]);
        if (!data.data) {
            status = 2;
            continue;
        }
        while (data.len) {
            FkLine l = fknextline(data);
            data = l.rest;
            last = l.line;
            nr++;
            for (i32 i = 0; i < nrules; i++) {
                FkAwkRule *r = rules + i;
                if (r->when==0 || (r->when==3 && r->nr==nr)) {
                    bad |= !fkawkrun(&p, r->action, l.line, fs);
                }
            }
        }
    }
    for (i32 i = 0; i < nrules; i++) {
        if (rules[i].when == 2) bad |= !fkawkrun(&p, rules[i].action, last, fs);
    }
    if (bad) {
        print(diag(&p), "unsupported statement");
        enddiag(&p);
        status = 2;
    }
    return fkexit(&p, status);
}


// Program table

static Fake fakes[] = {
    {"awk",      fake_awk},
    {"basename", fake_basename},
    {"cat",      fake_cat},
    {"chgrp",    fake_chgrp},
    {"chmod",    fake_chmod},
    {"cmp",      fake_cmp},
    {"cp",       fake_cp},
    {"cut",      fake_cut},
    {"date",     fake_date},
    {"diff",     fake_diff},
    {"dirname",  fake_dirname},
    {"echo",     fake_echo},
    {"env",      fake_env},
    {"expr",     fake_expr},
    {"false",    fake_false},
    {"getconf",  fake_getconf},
    {"grep",     fake_grep},
    {"head",     fake_head},
    {"hexdump",  fake_hexdump},
    {"id",       fake_id},
    {"ln",       fake_ln},
    {"ls",       fake_ls},
    {"mkdir",    fake_mkdir},
    {"mkfifo",   fake_mkfifo},
    {"mktemp",   fake_mktemp},
    {"mv",       fake_mv},
    {"od",       fake_od},
    {"printenv", fake_printenv},
    {"pwd",      fake_pwd},
    {"recho",    fake_recho},
    {"rm",       fake_rm},
    {"rmdir",    fake_rmdir},
    {"sed",      fake_sed},
    {"seq",      fake_seq},
    {"sleep",    fake_sleep},
    {"sort",     fake_sort},
    {"tail",     fake_tail},
    {"tee",      fake_tee},
    {"touch",    fake_touch},
    {"tr",       fake_tr},
    {"true",     fake_true},
    {"uname",    fake_uname},
    {"uniq",     fake_uniq},
    {"wc",       fake_wc},
    {"whoami",   fake_whoami},
    {"yes",      fake_yes},
    {"xcase",    fake_xcase},
    {"zecho",    fake_zecho},
};
