// argv: show how a native program receives its command line
// $ g++ -std=c++20 -municode -O2 -s -o argv.exe argv.cpp
// $ i686-w64-mingw32-g++ -std=c++20 -municode -O2 -s -o argv32.exe argv.cpp
//
// Unlike the other tools this one keeps the normal CRT startup, so the
// arguments are split by msvcrt.dll exactly as a typical native program
// splits them (mingw-w64: __wgetmainargs, no wildcard expansion).
//
// Output, on standard output, UTF-8, each line ending in a bare \n:
//
//   cmdline = <RAW>        GetCommandLineW() as received
//   argv[I] = <ARG>        one line per CRT argument, I from 0
//   NAME=VALUE             for each name in $ARGV_SHOW, in order
//   NAME unset             ... when that variable is not set
//   fd N open              with ARGV_FD=N, if CRT descriptor N is open;
//                          then "argv: fd N\n" is written to it
//   fd N closed            ... if it is not
//
// The text between < and > is raw: no escaping, so it may hold any
// byte, newlines included. ARGV_SHOW holds names separated by spaces or
// commas, looked up as Windows does (case-insensitively). The status is
// 0, or 1 if writing to standard output failed.
//
// This is free and unencumbered software released into the public domain.

using u8   = unsigned char;
using b32  = decltype(0);
using i32  = decltype(0);
using u32  = decltype(0u);
using iz   = decltype((u8 *)0 - (u8 *)0);
using uz   = decltype(sizeof(0));
using byte = decltype('0');
using c16  = decltype(u'0');

#define countof(a)  (iz)(sizeof(a) / sizeof(*(a)))

#define W32(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
W32(c16 *, GetCommandLineW());
W32(u32,   GetEnvironmentVariableW(c16 const *, c16 *, u32));
W32(uz,    GetStdHandle(i32));
W32(u32,   GetLastError());
W32(void,  SetLastError(u32));
W32(b32,   WriteFile(uz, void const *, u32, u32 *, void *));

extern "C" iz  _get_osfhandle(i32);
extern "C" i32 _write(i32, void const *, u32);

enum : i32 {
    STD_OUTPUT_HANDLE = -11,
};


// Output

struct Out {
    byte buf[1<<14];
    iz   len;
    b32  err;
};

static void flush(Out *o)
{
    if (!o->err && o->len) {
        u32 n = 0;
        uz  h = GetStdHandle(STD_OUTPUT_HANDLE);
        o->err = !WriteFile(h, o->buf, (u32)o->len, &n, 0) || n!=(u32)o->len;
    }
    o->len = 0;
}

static void putbyte(Out *o, byte c)
{
    if (o->len == countof(o->buf)) {
        flush(o);
    }
    o->buf[o->len++] = c;
}

static void print(Out *o, char const *s)
{
    for (; *s; s++) {
        putbyte(o, *s);
    }
}

static void print(Out *o, i32 x)
{
    byte  tmp[16];
    byte *end = tmp + countof(tmp);
    byte *beg = end;
    u32   u   = x<0 ? -(u32)x : (u32)x;
    do {
        *--beg = (byte)('0' + u%10);
    } while (u /= 10);
    if (x < 0) {
        *--beg = '-';
    }
    for (; beg < end; beg++) {
        putbyte(o, *beg);
    }
}

// Print UTF-16 as UTF-8; a lone surrogate becomes U+FFFD.
static void print(Out *o, c16 const *s, iz len)
{
    for (iz i = 0; i < len; i++) {
        u32 c = s[i];
        if (c>=0xd800 && c<=0xdbff && i+1<len &&
                s[i+1]>=0xdc00 && s[i+1]<=0xdfff) {
            c = 0x10000 + ((c - 0xd800)<<10) + (s[++i] - 0xdc00);
        } else if (c>=0xd800 && c<=0xdfff) {
            c = 0xfffd;
        }
        if (c < 0x80) {
            putbyte(o, (byte)c);
        } else if (c < 0x800) {
            putbyte(o, (byte)(0xc0 | c>>6));
            putbyte(o, (byte)(0x80 | (c&63)));
        } else if (c < 0x10000) {
            putbyte(o, (byte)(0xe0 | c>>12));
            putbyte(o, (byte)(0x80 | (c>>6 & 63)));
            putbyte(o, (byte)(0x80 | (c&63)));
        } else {
            putbyte(o, (byte)(0xf0 | c>>18));
            putbyte(o, (byte)(0x80 | (c>>12 & 63)));
            putbyte(o, (byte)(0x80 | (c>>6 & 63)));
            putbyte(o, (byte)(0x80 | (c&63)));
        }
    }
}

static iz c16len(c16 const *s)
{
    iz len = 0;
    for (; s[len]; len++) {}
    return len;
}

static void print(Out *o, c16 const *s)
{
    print(o, s, c16len(s));
}


// Probes

static c16 envbuf[1<<15];

// Print NAME=VALUE or "NAME unset" for each name in the list.
static void showenv(Out *o, c16 const *list)
{
    c16 name[256];
    for (iz i = 0; list[i];) {
        for (; list[i]==' ' || list[i]==','; i++) {}
        iz len = 0;
        for (; list[i] && list[i]!=' ' && list[i]!=','; i++) {
            if (len < countof(name)-1) {
                name[len++] = list[i];
            }
        }
        if (!len) {
            continue;
        }
        name[len] = 0;
        print(o, name, len);
        SetLastError(0);
        u32 n = GetEnvironmentVariableW(name, envbuf, countof(envbuf));
        if (n>=(u32)countof(envbuf) || (!n && GetLastError())) {
            print(o, " unset\n");  // or too long, which cannot happen
        } else {
            print(o, "=");
            print(o, envbuf, n);
            print(o, "\n");
        }
    }
}

// Report whether CRT descriptor fd is open, and write to it if so.
static void probefd(Out *o, c16 const *s)
{
    i32 fd = 0;
    for (; *s>='0' && *s<='9' && fd<1000000; s++) {
        fd = fd*10 + (*s - '0');
    }
    print(o, "fd ");
    print(o, fd);
    if (_get_osfhandle(fd) == -1) {
        print(o, " closed\n");
        return;
    }
    print(o, " open\n");
    flush(o);  // keep the order when fd shares standard output

    byte msg[32];
    iz   len = 0;
    for (char const *p = "argv: fd "; *p; p++) {
        msg[len++] = *p;
    }
    Out tmp = {};
    print(&tmp, fd);
    for (iz i = 0; i < tmp.len; i++) {
        msg[len++] = tmp.buf[i];
    }
    msg[len++] = '\n';
    _write(fd, msg, (u32)len);
}

extern "C" i32 wmain(i32 argc, c16 **argv)
{
    static Out o;
    print(&o, "cmdline = <");
    print(&o, GetCommandLineW());
    print(&o, ">\n");
    for (i32 i = 0; i < argc; i++) {
        print(&o, "argv[");
        print(&o, i);
        print(&o, "] = <");
        print(&o, argv[i]);
        print(&o, ">\n");
    }

    if (GetEnvironmentVariableW(u"ARGV_SHOW", envbuf, countof(envbuf))) {
        static c16 list[countof(envbuf)];
        for (iz i = 0; i < countof(list); i++) {
            list[i] = envbuf[i];
        }
        showenv(&o, list);
    }
    if (GetEnvironmentVariableW(u"ARGV_FD", envbuf, countof(envbuf))) {
        probefd(&o, envbuf);
    }
    flush(&o);
    return o.err;
}
