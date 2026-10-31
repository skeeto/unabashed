// Win32 platform layer: no CRT, no headers, runs on Windows XP and later
// $ g++ -std=c++20 -O2 -s -nostartfiles -static-libgcc -o unabashed.exe main_win32.cpp
//
// Imports only kernel32.dll. Anything newer than XP must be resolved at
// run time with a fallback.
//
// This is free and unencumbered software released into the public domain.
#include "unabashed.cpp"
#include "wide.cpp"

enum : i32 {
    MEM_COMMIT        = 0x1000,
    MEM_RESERVE       = 0x2000,
    PAGE_NOACCESS     = 1,
    PAGE_READWRITE    = 4,
    STD_ERROR_HANDLE  = -12,
};

#define W32(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
W32(void,   ExitProcess[[noreturn]](i32));
W32(b32,    FreeEnvironmentStringsW(c16 *));
W32(c16 *,  GetCommandLineW());
W32(b32,    GetConsoleMode(uz, i32 *));
W32(c16 *,  GetEnvironmentStringsW());
W32(uz,     GetStdHandle(i32));
W32(b32,    ReadFile(uz, u8 *, i32, i32 *, uz));
W32(byte *, VirtualAlloc(byte *, iz, i32, i32));
W32(b32,    VirtualFree(byte *, iz, i32));
W32(b32,    WriteConsoleW(uz, c16 *, i32, i32 *, uz));
W32(b32,    WriteFile(uz, u8 *, i32, i32 *, uz));

// GCC emits calls to these even without a CRT. Defining them keeps
// msvcrt.dll out of the imports.
#if defined(__x86_64__) || defined(__i386__)
extern "C" void *memcpy(void *dst, void const *src, uz len)
{
    void *r = dst;
    __asm volatile ("rep movsb" : "+D"(dst), "+S"(src), "+c"(len) : : "memory");
    return r;
}

extern "C" void *memset(void *dst, i32 c, uz len)
{
    void *r = dst;
    __asm volatile ("rep stosb" : "+D"(dst), "+c"(len) : "a"(c) : "memory");
    return r;
}

extern "C" void *memmove(void *dst, void const *src, uz len)
{
    u8 *d = (u8 *)dst;
    u8 *s = (u8 *)src;
    if (d <= s || d >= s+len) {
        return memcpy(dst, src, len);
    }
    for (uz i = len; i; i--) {
        d[i-1] = s[i-1];
    }
    return dst;
}

extern "C" void *memchr(void const *s, i32 c, uz len)
{
    u8 *p = (u8 *)s;
    for (uz i = 0; i < len; i++) {
        if (p[i] == (u8)c) {
            return p + i;
        }
    }
    return 0;
}

extern "C" i32 memcmp(void const *a, void const *b, uz len)
{
    u8 *x = (u8 *)a;
    u8 *y = (u8 *)b;
    for (uz i = 0; i < len; i++) {
        if (x[i] != y[i]) {
            return x[i] - y[i];
        }
    }
    return 0;
}
#endif

#include "fibers.cpp"

struct Os {
    Fibers fibers;
};

static Fibers *osfibers(Os *os)
{
    return &os->fibers;
}

static i32 chunk(iz len)  // bound a single ReadFile/WriteFile
{
    i32 max = 1<<21;
    return len<max ? (i32)len : max;
}

static b32 writeall(uz h, u8 *buf, iz len)
{
    for (iz off = 0; off < len;) {
        i32 n = chunk(len - off);
        if (!WriteFile(h, buf+off, n, &n, 0) || !n) {
            return 0;
        }
        off += n;
    }
    return 1;
}

[[noreturn]] static void os_oom()
{
    Str msg = "bash: out of memory\n";
    writeall(GetStdHandle(STD_ERROR_HANDLE), msg.data, msg.len);
    ExitProcess(2);
}

static byte *os_reserve(Os *, iz len)
{
    return VirtualAlloc(0, len, MEM_RESERVE, PAGE_NOACCESS);
}

static b32 os_commit(Os *, byte *p, iz len)
{
    return !!VirtualAlloc(p, len, MEM_COMMIT, PAGE_READWRITE);
}

static b32 os_write(Os *, Handle h, u8 *buf, iz len)
{
    i32 mode = 0;
    if (!GetConsoleMode(h, &mode)) {
        return writeall(h, buf, len);
    }

    // Consoles display UTF-16 correctly regardless of code page.
    // TODO: carry partial UTF-8 sequences across calls.
    Str s = {buf, len};
    c16 tmp[512];
    i32 n = 0;
    for (iz i = 0; i < s.len;) {
        n += (i32)utf16encode(tmp+n, utf8decode(s, &i));
        if (n>countof(tmp)-2 || i==s.len) {
            i32 dummy;
            if (!WriteConsoleW(h, tmp, n, &dummy, 0)) {
                return 0;
            }
            n = 0;
        }
    }
    return 1;
}

static iz os_read(Os *, Handle h, u8 *buf, iz len)
{
    i32 n = 0;
    if (!ReadFile(h, buf, chunk(len), &n, 0)) {
        return -1;
    }
    return n;
}

static byte *fiberstack(Os *, iz) { return 0; }    // unused on Win32
static void  fiberunstack(Os *, byte *, iz) {}

extern "C" __attribute((force_align_arg_pointer))
void mainCRTStartup()
{
    static Os os;
    OsInfo info = {};
    info.reserve = (iz)1 << (sizeof(void *)==8 ? 34 : 28);

    // Startup data lives in its own small arena.
    Arena a = newarena(&os, (iz)1<<26);

    c16 *cmd = GetCommandLineW();
    iz   len = 0;
    for (; cmd[len]; len++) {}
    Slice<Str> args = splitcmdline(&a, fromwide(&a, cmd, len));
    info.args  = args.data;
    info.nargs = args.len;

    Slice<Str> env = {};
    c16 *block = GetEnvironmentStringsW();
    for (c16 *p = block; p && *p;) {
        iz n = 0;
        for (; p[n]; n++) {}
        env = push(&a, env, fromwide(&a, p, n));
        p += n + 1;
    }
    FreeEnvironmentStringsW(block);
    info.env  = env.data;
    info.nenv = env.len;

    for (i32 i = 0; i < 3; i++) {
        info.std[i] = GetStdHandle(-10 - i);
    }
    ExitProcess(sh_main(&os, &info));
}
