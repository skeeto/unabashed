// Portable half of the Win32 layer: UTF-16 conversion, command line
// splitting and quoting for the MSVCRT and Cygwin parsers, the CRT's
// inherited descriptor blob, a PE image sniffer, and extended-length
// paths. No Win32 calls, so the core, the vos and the unit tests
// (t/unit/wide_test.cpp) use it on every host.
// This is free and unencumbered software released into the public domain.

using c16 = decltype(u'0');

struct Str16 {
    c16 *data = {};
    iz   len  = {};
};

// Encode a code point as UTF-16, returning the length. Surrogate code
// points (from WTF-8) encode as themselves.
static iz utf16encode(c16 *s, i32 c)
{
    if (c > 0x10ffff) {
        c = 0xfffd;
    } else if (c >= 0x10000) {
        c -= 0x10000;
        s[0] = (c16)(0xd800 + (c >> 10));
        s[1] = (c16)(0xdc00 + (c & 0x3ff));
        return 2;
    }
    s[0] = (c16)c;
    return 1;
}

// Convert UTF-8 (or WTF-8) to null-terminated UTF-16.
static Str16 towide(Arena *a, Str s)
{
    Str16 r = {};
    r.data = alloc<c16>(a, s.len+1);  // never more units than bytes
    for (iz i = 0; i < s.len;) {
        r.len += utf16encode(r.data+r.len, utf8decode(s, &i));
    }
    return r;
}

// Convert UTF-16 to UTF-8. Unpaired surrogates become WTF-8, so that
// file names round-trip through towide().
static Str fromwide(Arena *a, c16 *s, iz len)
{
    if (len > (iz)((uz)-1>>2)/3) {
        os_oom();
    }
    Str r  = {};
    r.data = allocbytes(a, 3*len);  // worst case
    for (iz i = 0; i < len; i++) {
        i32 c = s[i];
        if (c>=0xd800 && c<=0xdbff && i+1<len && s[i+1]>=0xdc00 && s[i+1]<=0xdfff) {
            c = 0x10000 + ((c - 0xd800)<<10) + (s[++i] - 0xdc00);
        }
        r.len += utf8encode(r.data+r.len, c);
    }
    a->beg = (byte *)(r.data + r.len);  // return the unused tail
    return r;
}

// The MSVCRT-style command line parsers, which differ only in the
// program name and in a doubled quote inside quotes (t/unit/wide.golden).
enum : i32 {
    ARGV_SHELL32,  // CommandLineToArgvW
    ARGV_MSVCRT,   // msvcrt.dll and the CRTs before Visual C++ 2008
    ARGV_UCRT,     // UCRT and msvcr90 on
};

// Split a command line into arguments like the given parser, quirks
// included: backslashes are literal except before a quote, and the
// program name (first argument) has no escapes at all. A doubled quote
// inside quotes is a literal quote, which also ends the quoting except
// for UCRT. CommandLineToArgvW ends a quoted program name at its closing
// quote; the CRTs toggle quotes until an unquoted blank. Unescapes in
// place, so the arguments point into cmd.
static Slice<Str> splitcmdline(Arena *a, Str cmd, i32 parser = ARGV_SHELL32)
{
    Slice<Str> args = {};
    iz i = 0;  // read position
    iz w = 0;  // write position, never past i

    if (parser != ARGV_SHELL32) {
        b32 quoted = 0;
        for (; i<cmd.len && (quoted || (cmd[i]!=' ' && cmd[i]!='\t')); i++) {
            if (cmd[i] == '"') {
                quoted = !quoted;
            } else {
                cmd[w++] = cmd[i];
            }
        }
    } else if (i<cmd.len && cmd[i]=='"') {
        for (i++; i<cmd.len && cmd[i]!='"'; i++) {
            cmd[w++] = cmd[i];
        }
        i += i < cmd.len;
    } else {
        for (; i<cmd.len && cmd[i]!=' ' && cmd[i]!='\t'; i++) {
            cmd[w++] = cmd[i];
        }
    }
    args = push(a, args, takehead(cmd, w));

    for (;;) {
        for (; i<cmd.len && (cmd[i]==' ' || cmd[i]=='\t'); i++) {}
        if (i == cmd.len) {
            break;
        }

        iz  beg    = w;
        b32 quoted = 0;
        while (i<cmd.len && (quoted || (cmd[i]!=' ' && cmd[i]!='\t'))) {
            if (cmd[i] == '\\') {
                iz n = 0;
                for (; i<cmd.len && cmd[i]=='\\'; i++, n++) {}
                b32 quote = i<cmd.len && cmd[i]=='"';
                for (iz k = 0; k < (quote ? n/2 : n); k++) {
                    cmd[w++] = '\\';
                }
                if (quote && n%2) {
                    cmd[w++] = cmd[i++];  // escaped quote
                }
            } else if (cmd[i] == '"') {
                i++;
                if (quoted && i<cmd.len && cmd[i]=='"') {
                    cmd[w++] = cmd[i++];
                    quoted = parser == ARGV_UCRT;
                } else {
                    quoted = !quoted;
                }
            } else {
                cmd[w++] = cmd[i++];
            }
        }
        args = push(a, args, span(cmd.data+beg, cmd.data+w));
    }
    return args;
}

// Given a full path, as written by GetFullPathNameW at buf+6, prepend
// the extended-length prefix in the space before it: C:\x becomes
// \\?\C:\x, and \\host\share becomes \\?\UNC\host\share. This lifts
// the MAX_PATH limit. Paths already in that form are left alone.
static Str16 extendpath(c16 *buf, iz len)
{
    Str16 r = {buf+6, len};
    c16  *p = r.data;
    if (len>=4 && p[0]=='\\' && p[1]=='\\' && (p[2]=='?' || p[2]=='.') && p[3]=='\\') {
        return r;
    }
    b32 unc = len>=2 && p[0]=='\\' && p[1]=='\\';
    c16 prefix[] = {'\\', '\\', '?', '\\', 'U', 'N', 'C'};
    r.data -= unc ? 6 : 4;
    r.len  += unc ? 6 : 4;
    for (iz i = 0; i < (unc ? 7 : 4); i++) {
        r.data[i] = prefix[i];  // for UNC, replaces the first backslash
    }
    return r;
}

// Length of the root of an extended-length path, e.g. \\?\C:\ or
// \\?\UNC\host\share\, including its trailing separator. Zero for other
// paths.
static iz rootlen(Str16 p)
{
    c16 *s = p.data;
    if (p.len<4 || s[0]!='\\' || s[1]!='\\' || s[3]!='\\') {
        return 0;
    }
    iz  i     = 4;
    i32 parts = 1;  // drive, volume, or device
    if (p.len>=8 && s[4]=='U' && s[5]=='N' && s[6]=='C' && s[7]=='\\') {
        i     = 8;
        parts = 2;  // host and share
    }
    for (; parts; parts--) {
        for (; i<p.len && s[i]!='\\'; i++) {}
        i += i < p.len;
    }
    return i;
}

// Native spelling of a platform path: MSYS-style /c/x becomes C:\x and
// forward slashes become backslashes.
static Str nativepath(Arena *a, Str path)
{
    Buf b(a, path.len+2);
    iz  i = 0;
    if (path.len>=2 && path[0]=='/' && letter(path[1]) && (path.len==2 || path[2]=='/')) {
        putbyte(&b, (u8)(path[1] & 0xdf));
        putbyte(&b, ':');
        if (path.len == 2) putbyte(&b, '\\');
        i = 2;
    }
    for (; i < path.len; i++) {
        putbyte(&b, path[i]=='/' ? '\\' : path[i]);
    }
    return finish(&b);
}

// Append a program name, the first argument of a command line, which
// the parsers split without escapes: quote it if needed, and drop any
// quotes since they cannot be represented. Cygwin also splits on CR and
// LF, and takes ' as a quote.
static void quoteprogram(Buf *b, Str name)
{
    b32 quote = !name.len;
    for (iz i = 0; i < name.len; i++) {
        switch (name[i]) {
        case ' ': case '\t': case '\n': case '\r': case '\'':
            quote = 1;
        }
    }
    if (quote) putbyte(b, '"');
    for (iz i = 0; i < name.len; i++) {
        if (name[i] != '"') putbyte(b, name[i]);
    }
    if (quote) putbyte(b, '"');
}

// Append an argument quoted for CommandLineToArgvW and the MSVCRT
// parser, the inverse of splitcmdline. Quoting glob and quote characters
// too keeps Cygwin and MSYS children from expanding them again.
static void quotearg(Buf *b, Str arg)
{
    b32 quote = !arg.len;
    for (iz i = 0; i < arg.len; i++) {
        switch (arg[i]) {
        case ' ': case '\t': case '\n': case '\v': case '"':
        case '*': case '?':  case '[':  case '{':  case '\'':
            quote = 1;
        }
    }
    if (!quote) {
        print(b, arg);
        return;
    }

    putbyte(b, '"');
    iz slashes = 0;
    for (iz i = 0; i < arg.len; i++) {
        if (arg[i] == '\\') {
            slashes++;
        } else {
            if (arg[i] == '"') {
                for (iz k = 0; k <= slashes; k++) {
                    putbyte(b, '\\');  // double the run, then escape
                }
            }
            slashes = 0;
        }
        putbyte(b, arg[i]);
    }
    for (; slashes; slashes--) {
        putbyte(b, '\\');  // the closing quote must not be escaped
    }
    putbyte(b, '"');
}

// Append an argument for a batch file run with cmd /s /c, as in Rust's
// fix for CVE-2024-24576 ("BatBadBut"): quoted unless plainly safe,
// quotes doubled, and % broken up with an empty substring expansion so
// cmd cannot expand variables. Returns false for arguments cmd cannot
// carry (NUL, CR, LF).
static b32 quotebatarg(Buf *b, Str arg, b32 quote)
{
    Str safe = "#$*+-./:?@\\_";
    quote |= !arg.len || arg[arg.len-1]=='\\';
    for (iz i = 0; i < arg.len; i++) {
        u8 c = arg[i];
        if (!c || c=='\r' || c=='\n') {
            return 0;
        }
        if (c<0x80 && !alnum(c) && find(safe, {&c, 1})<0) {
            quote = 1;
        }
    }

    if (quote) putbyte(b, '"');
    iz slashes = 0;
    for (iz i = 0; i < arg.len; i++) {
        u8 c = arg[i];
        if (c == '\\') {
            slashes++;
        } else {
            if (c == '"') {
                for (; slashes; slashes--) putbyte(b, '\\');
                putbyte(b, '"');
            } else if (c == '%') {
                print(b, "%%cd:~,");
            }
            slashes = 0;
        }
        putbyte(b, c);
    }
    if (quote) {
        for (; slashes; slashes--) putbyte(b, '\\');
        putbyte(b, '"');
    }
    return 1;
}


// Cygwin command lines

// Cygwin's startup splits the command line of a native parent itself:
// blanks, tabs, CR and LF separate; " and ' quote, the quotes kept for
// glob(3) to remove. Inside either quote, \\ and \<quote> escape, and ""
// inside double quotes is a literal quote. A word with a quote, a
// wildcard, a brace, a parenthesis or a leading ~ goes through glob with
// GLOB_NOCHECK|GLOB_BRACE|GLOB_TILDE|GLOB_QUOTE, so its unquoted
// backslashes escape too, except in a word starting with a drive letter
// (C: or "C:). An unquoted @ starts a response file name, with quotes
// removed but no escapes. The program name is only unquoted that way.
// Details and limits in t/unit/wide.golden and notes/m5a.md.
enum : i32 {
    CYG_GLOB   = 1<<0,  // unquoted wildcard, brace or leading ~: glob pat
    CYG_ATFILE = 1<<1,  // leading @: try file as a response file
};

struct CygArg {
    Str arg;    // the argument without globbing or response files
    Str pat;    // for CYG_GLOB, a pattern with backslash quoting
    Str file;   // for CYG_ATFILE, the response file name
    iz  end;    // offset in cmd past the word and its separator
    i32 flags;
};

static b32 cygsep(u8 c)
{
    return c==' ' || c=='\t' || c=='\n' || c=='\r';
}

static b32 cygdrive(Str w)
{
    return w.len>=2 && letter(w[0]) && w[1]==':';
}

// Fill in arg, pat and CYG_GLOB for word w as Cygwin's globify does.
static void cygglob(Arena *a, CygArg *r, Str w)
{
    b32 globify = w.len && w[0]=='~';
    for (iz i = 0; i < w.len; i++) {
        switch (w[i]) {
        case '?': case '*': case '[': case '"': case '\'':
        case '(': case ')': case '{': case '}':
            globify = 1;
        }
    }
    if (!globify) {
        return;
    }

    b32 quote = w.len && (w[0]=='"' || w[0]=='\'');
    b32 dos   = cygdrive(w) || (quote && cygdrive(cuthead(w, 1)));
    Buf p(a, 2*w.len);
    for (iz i = 0; i < w.len; i++) {
        u8 q = w[i];
        if (q!='"' && q!='\'') {
            if (dos && q=='\\') putbyte(&p, '\\');
            putbyte(&p, q);
            continue;
        }
        for (i++; i<w.len && w[i]!=q; i++) {
            if (w[i]=='\\' && i+1<w.len && (w[i+1]==q || w[i+1]=='\\')) {
                i++;
            }
            putbyte(&p, '\\');
            putbyte(&p, w[i]);
        }
    }
    r->pat = finish(&p);

    // GLOB_NOCHECK's result: the pattern unquoted
    Buf u(a, r->pat.len);
    for (iz i = 0; i < r->pat.len; i++) {
        u8 c = r->pat[i];
        if (c=='\\' && i+1<r->pat.len) {
            putbyte(&u, r->pat[++i]);
            continue;
        }
        switch (c) {
        case '*': case '?': case '[': case '{':
            r->flags |= CYG_GLOB;
        }
        putbyte(&u, c);
    }
    if (r->pat.len && r->pat[0]=='~') {
        r->flags |= CYG_GLOB;
    }
    r->arg = finish(&u);
}

// Split a command line as Cygwin does for a native parent, the program
// name first if program is set. The caller globs CYG_GLOB arguments
// (NOCHECK: arg when nothing matches), and replaces a CYG_ATFILE one,
// when its file can be read, with the arguments of the file's contents,
// a space and the rest of cmd from end, split without a program name.
// Cygwin reads at most 10 response files per command line and dies on
// the 11th. Then it globs that argument as usual.
static Slice<CygArg> splitcygwin(Arena *a, Str cmd, b32 program)
{
    for (iz i = 0; i < cmd.len; i++) {
        if (!cmd[i]) {
            cmd = takehead(cmd, i);  // a C string to Cygwin
            break;
        }
    }

    Slice<CygArg> args = {};
    u8 *buf = allocbytes(a, cmd.len);  // every word fits, in total
    iz  used = 0;
    for (iz i = 0;;) {
        for (; i<cmd.len && cygsep(cmd[i]); i++) {}
        if (i == cmd.len) {
            break;
        }

        b32 name  = program && !args.len;
        b32 strip = name || cmd[i]=='@';  // quotes removed, no escapes
        u8 *word  = buf + used;
        iz  len   = 0;
        while (i<cmd.len && !cygsep(cmd[i])) {
            u8 q = cmd[i++];
            if (q!='"' && q!='\'') {
                word[len++] = q;
            } else if (strip) {
                for (; i<cmd.len && cmd[i]!=q; i++) {
                    word[len++] = cmd[i];
                }
                i += i < cmd.len;
            } else {
                word[len++] = q;
                while (i < cmd.len) {
                    u8 c = cmd[i++];
                    if (q=='"' && c=='"' && i<cmd.len && cmd[i]=='"') {
                        word[len++] = '\\';  // "" becomes \"
                        word[len++] = cmd[i++];
                    } else if (q=='"' && c=='\\' && i<cmd.len) {
                        word[len++] = c;
                        word[len++] = cmd[i++];
                    } else {
                        word[len++] = c;
                        if (c == q) break;
                    }
                }
            }
        }
        used += len;

        CygArg r = {};
        Str    w = {word, len};
        r.arg = w;
        r.end = i + (i < cmd.len);
        i = r.end;
        if (!name) {
            if (len>1 && w[0]=='@') {
                r.flags |= CYG_ATFILE;
                r.file   = cuthead(w, 1);
            }
            cygglob(a, &r, w);
        }
        args = push(a, args, r);
    }
    return args;
}

// GLOB_BRACE on a CYG_GLOB pattern: every alternative of the first
// brace group, recursively ("{}" stays). At most limit results, since
// a command line can ask for 2^5000 of them.
static Slice<Str> cygbraces(Arena *a, Slice<Str> out, Str p, iz limit)
{
    if (out.len >= limit) return out;
    iz open = -1, close = -1, depth = 0;
    for (iz i = 0; i < p.len; i++) {
        if (p[i] == '\\') {
            i++;
        } else if (p[i]=='{' && !(i+1<p.len && p[i+1]=='}')) {
            if (!depth++ && open<0) open = i;
        } else if (p[i]=='}' && depth && !--depth && close<0) {
            close = i;
        }
    }
    if (open<0 || close<0) {
        return push(a, out, p);
    }
    Str head = takehead(p, open);
    Str tail = cuthead(p, close+1);
    iz  beg  = open + 1;
    depth = 0;
    for (iz i = beg; i <= close; i++) {
        if (p[i] == '\\') {
            i++;
        } else if (p[i] == '{') {
            depth++;
        } else if (i==close || (p[i]==',' && !depth)) {
            Str alt = span(p.data+beg, p.data+i);
            out = cygbraces(a, out, concat(a, concat(a, head, alt), tail), limit);
            beg = i + 1;
        } else if (p[i] == '}') {
            depth--;
        }
    }
    return out;
}

static Str cygunquote(Arena *a, Str s)  // backslash escapes removed
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        putbyte(&b, s[i]=='\\' && i+1<s.len ? s[++i] : s[i]);
    }
    return finish(&b);
}

// Append an argument quoted for splitcygwin, which gives it back
// unchanged, never globbed nor read as a response file. Command lines
// cannot carry NUL.
static void quotecygwin(Buf *b, Str arg)
{
    b32 quote = !arg.len || arg[0]=='~' || arg[0]=='@';
    for (iz i = 0; i < arg.len; i++) {
        switch (arg[i]) {
        case ' ': case '\t': case '\n': case '\r': case '"': case '\'':
        case '?': case '*':  case '[':  case '(':  case ')': case '{':
        case '}':
            quote = 1;
        }
    }
    if (!quote) {
        print(b, arg);
        return;
    }

    putbyte(b, '"');
    for (iz i = 0; i < arg.len; i++) {
        if (arg[i]=='\\' || arg[i]=='"') {
            putbyte(b, '\\');
        }
        putbyte(b, arg[i]);
    }
    putbyte(b, '"');
}


// Inherited CRT descriptors

// The Microsoft CRTs pass descriptors to a child in STARTUPINFO's
// lpReserved2: a 32-bit count n, n flag bytes, then n handles of the
// child's pointer width, all little endian, at most 65535 bytes.
enum : u8 {
    CRTFD_OPEN = 0x01,  // FOPEN
    CRTFD_PIPE = 0x08,  // FPIPE
    CRTFD_DEV  = 0x40,  // FDEV: a character device, such as a console
};

struct CrtFd {
    i64 handle;  // -1 when closed
    u8  flags;   // CRTFD_*, zero when closed
};

// Encode descriptors 0 to n-1 with handle slots of width 4 or 8 bytes.
// Trailing closed descriptors are left out, as are any past the size
// limit (13106 at width 4, 7281 at 8). Empty when none is open.
static Str encodecrtfds(Arena *a, CrtFd *fds, iz n, i32 width)
{
    assert(width==4 || width==8);
    iz max = (0xffff - 4) / (1 + width);
    n = n<max ? n : max;
    for (; n>0 && !(fds[n-1].flags & CRTFD_OPEN); n--) {}
    if (!n) {
        return {};
    }

    Str r  = {};
    r.len  = 4 + n*(1 + width);
    r.data = alloc<u8>(a, r.len);
    for (i32 b = 0; b < 4; b++) {
        r[b] = (u8)(n >> (8*b));
    }
    for (iz i = 0; i < n; i++) {
        b32 open = fds[i].flags & CRTFD_OPEN;
        u64 h    = open ? (u64)fds[i].handle : (u64)-1;
        r[4+i] = open ? fds[i].flags : 0;
        for (i32 b = 0; b < width; b++) {
            r[4 + n + i*width + b] = (u8)(h >> (8*b));
        }
    }
    return r;
}

// Decode a descriptor blob with handle slots of width 4 or 8 bytes, as
// the CRTs do: a count beyond the blob's size is cut to fit. Handles
// from 4-byte slots are sign extended, as Windows does for 32-bit
// handles.
static Slice<CrtFd> decodecrtfds(Arena *a, Str blob, i32 width)
{
    assert(width==4 || width==8);
    Slice<CrtFd> r = {};
    if (blob.len < 4) {
        return r;
    }
    u32 count = 0;
    for (i32 b = 3; b >= 0; b--) {
        count = count<<8 | blob[b];
    }
    i32 n   = (i32)count;
    iz  fit = (blob.len - 4) / (1 + width);
    if (n <= 0) {
        return r;
    }
    r.len  = n<fit ? n : fit;
    r.cap  = r.len;
    r.data = alloc<CrtFd>(a, r.len);
    for (iz i = 0; i < r.len; i++) {
        u64 h = 0;
        for (i32 b = width-1; b >= 0; b--) {
            h = h<<8 | blob[4 + r.len + i*width + b];
        }
        r[i].flags  = blob[4+i];
        r[i].handle = width==4 ? (i64)(i32)(u32)h : (i64)h;
    }
    return r;
}


// PE images

enum : i32 {
    CRT_NONE,
    CRT_MSVCRT,  // msvcrt.dll, msvcr70 to msvcr80: ARGV_MSVCRT
    CRT_UCRT,    // UCRT, msvcr90 to msvcr120: ARGV_UCRT
};

enum : i32 {
    POSIX_CYGWIN = 1,  // imports cygwin1.dll
    POSIX_MSYS,        // imports msys-2.0.dll
};

struct PeInfo {
    i32 machine;    // 0x14c i386, 0x8664 x64, 0xaa64 arm64
    i32 subsystem;  // 2 GUI, 3 console
    i32 width;      // pointer size: 4 for PE32, 8 for PE32+
    i32 crt;        // CRT_*, from the imports
    i32 posix;      // POSIX_*, or zero
    b32 dll;
    b32 ours;       // carries our .unabash section
    b32 ok;         // a PE image
    Slice<Str> dlls;  // imported DLLs as named, when asked for
};

// Read up to len bytes at off into buf, returning the count read, short
// at the end of the file or on errors.
using PeRead = iz (*)(void *ctx, i64 off, u8 *buf, iz len);

// Our own images carry this section, which pesniff recognizes. Only PE
// names sections so (Mach-O would want "segment,section").
#if _WIN32 && !UB_LIBRARY
[[gnu::used, gnu::section(".unabash")]]
static char const unabashedmark[] = "unabashed";
#endif

static u32 peget(u8 *p, i32 n)
{
    u32 r = 0;
    for (i32 i = n-1; i >= 0; i--) {
        r = r<<8 | p[i];
    }
    return r;
}

struct PeSection {
    u32 va;
    u32 vsize;
    u32 rawsize;
    u32 rawptr;
};

// File offset of an RVA, or -1 if no file data backs it.
static i64 peoffset(PeSection *s, iz n, u32 headers, u32 rva)
{
    for (iz i = 0; i < n; i++) {
        u32 size = s[i].vsize ? s[i].vsize : s[i].rawsize;
        u32 off  = rva - s[i].va;
        if (rva>=s[i].va && off<size) {
            return off<s[i].rawsize ? (i64)s[i].rawptr+off : -1;
        }
    }
    return rva<headers ? (i64)rva : -1;
}

static i32 pecrt(Str dll)
{
    Str msvcr = "msvcr";
    if (dll=="ucrtbase.dll" || dll=="ucrtbased.dll" || startswith(dll, "api-ms-win-crt-")) {
        return CRT_UCRT;
    } else if (startswith(dll, "msvcrt")) {
        return CRT_MSVCRT;  // also msvcrt20, msvcrt40, msvcrtd
    } else if (startswith(dll, msvcr)) {
        i32 version = 0;
        for (iz i = msvcr.len; i<dll.len && digit(dll[i]); i++) {
            version = version*10 + dll[i] - '0';
            if (version > 1000) break;
        }
        if (version) {
            return version<90 ? CRT_MSVCRT : CRT_UCRT;
        }
    }
    return CRT_NONE;
}

// Sniff a PE image: its machine, subsystem, CRT and POSIX runtime by
// the import directory, and our own marker section. Reads through the
// callback, which sees no offset past what the headers point at. With
// keep, also the names of the imported DLLs, allocated there.
static PeInfo pesniff(PeRead read, void *ctx, Arena scratch, Arena *keep = 0)
{
    PeInfo r = {};
    u8     mz[64];
    if (read(ctx, 0, mz, countof(mz))<countof(mz) || mz[0]!='M' || mz[1]!='Z') {
        return r;
    }

    i64 pe = peget(mz+0x3c, 4);
    u8  nt[24+240] = {};  // signature, file header, optional header
    iz  got = read(ctx, pe, nt, countof(nt));
    if (got<24 || peget(nt, 4)!=0x4550) {
        return r;
    }
    iz nsect   = peget(nt+6, 2);
    iz optsize = peget(nt+20, 2);
    iz optlen  = optsize<got-24 ? optsize : got-24;
    u8 *opt    = nt + 24;
    iz  dirs   = 0;  // offset of the data directories
    switch (optlen<2 ? 0 : peget(opt, 2)) {
    case 0x10b: r.width = 4; dirs =  96; break;
    case 0x20b: r.width = 8; dirs = 112; break;
    default: return r;
    }
    if (optlen < dirs) {
        return r;
    }
    r.machine   = (i32)peget(nt+4, 2);
    r.dll       = !!(peget(nt+22, 2) & 0x2000);
    r.subsystem = (i32)peget(opt+68, 2);
    r.ok        = 1;
    u32 headers = peget(opt+60, 4);
    u32 imports = 0;
    if (optlen>=dirs+16 && peget(opt+dirs-4, 4)>=2) {
        imports = peget(opt+dirs+8, 4);
    }

    nsect = nsect<1024 ? nsect : 1024;
    u8 *raw = allocbytes(&scratch, 40*nsect);
    nsect = read(ctx, pe+24+optsize, raw, 40*nsect) / 40;
    PeSection *sects = alloc<PeSection>(&scratch, nsect);
    for (iz i = 0; i < nsect; i++) {
        u8 *s = raw + 40*i;
        r.ours |= Str{s, 8} == ".unabash";
        sects[i].vsize   = peget(s+ 8, 4);
        sects[i].va      = peget(s+12, 4);
        sects[i].rawsize = peget(s+16, 4);
        sects[i].rawptr  = peget(s+20, 4);
    }

    // Import descriptors: 20 bytes each, the DLL name's RVA at 12, up
    // to an all-zero entry
    for (u32 i = 0; imports && i < 1<<12; i++) {
        u8  desc[20];
        i64 off = peoffset(sects, nsect, headers, imports+20*i);
        if (off<0 || read(ctx, off, desc, countof(desc))<countof(desc)) {
            break;
        }
        u32 namerva = peget(desc+12, 4);
        if (!namerva) {
            break;
        }

        u8  name[64];
        i64 noff = peoffset(sects, nsect, headers, namerva);
        iz  len  = noff<0 ? 0 : read(ctx, noff, name, countof(name));
        iz  end  = 0;
        for (; end<len && name[end]; end++) {}
        if (end == len) {
            continue;  // unterminated or unreadable
        }
        if (keep) {
            r.dlls = push(keep, r.dlls, clone(keep, Str{name, end}));
        }
        for (iz k = 0; k < end; k++) {
            name[k] = lowercase(name[k]);
        }

        Str dll = {name, end};
        i32 crt = pecrt(dll);
        r.crt = crt>r.crt ? crt : r.crt;
        if (dll == "cygwin1.dll") {
            r.posix = POSIX_CYGWIN;
        } else if (dll == "msys-2.0.dll") {
            r.posix = POSIX_MSYS;
        }
    }
    return r;
}


// Environment blocks

// The NAME of a "NAME=value" string. A leading = belongs to the name,
// as in the hidden =C: variables.
static Str envname(Str s)
{
    iz i = 1;
    for (; i<s.len && s[i]!='='; i++) {}
    return takehead(s, i<s.len ? i : s.len);
}

// Order "NAME=value" strings as Windows wants an environment block:
// by name, case-insensitively by upper case.
static b32 winenvless(Str *a, Str *b)
{
    Str x = envname(*a);
    Str y = envname(*b);
    for (iz i = 0; i<x.len && i<y.len; i++) {
        i32 c = x[i]>='a' && x[i]<='z' ? x[i]-32 : x[i];
        i32 d = y[i]>='a' && y[i]<='z' ? y[i]-32 : y[i];
        if (c != d) return c < d;
    }
    return x.len < y.len;
}
