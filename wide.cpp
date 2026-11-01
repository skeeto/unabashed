// Portable half of the Win32 layer: UTF-16 conversion, command line
// splitting, and extended-length paths. No Win32 calls, so the unit
// tests cover it on every host.
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

// Split a command line into arguments like CommandLineToArgvW, quirks
// included: backslashes are literal except before a quote, a doubled
// quote inside quotes is a literal quote that also ends the quoting, and
// the program name (first argument) has no escapes at all. Unescapes in
// place, so the arguments point into cmd.
static Slice<Str> splitcmdline(Arena *a, Str cmd)
{
    Slice<Str> args = {};
    iz i = 0;  // read position
    iz w = 0;  // write position, never past i

    if (i<cmd.len && cmd[i]=='"') {
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
                }
                quoted = !quoted;
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
// quotes since they cannot be represented.
static void quoteprogram(Buf *b, Str name)
{
    b32 quote = !name.len;
    for (iz i = 0; i < name.len; i++) {
        quote |= name[i]==' ' || name[i]=='\t';
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
