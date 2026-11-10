// ubhost on Windows: a test host for libunabashed, with no C runtime
//
// As a drop-in, "ubhost [bash args]" behaves as unabashed.exe does: the
// run takes its command line, environment, directory, standard handles,
// CRT descriptors and pid; console events reach it as the console
// group's signals; it exits as the shell did. Programs share the
// host's console (UB_SHARECONSOLE), and the install is the host's own
// directory, the host itself its shell (root and self).
//
// "ubhost --check [--quick]" checks the host contract instead
// (ubcheckw.c), and "ubhost --fds" exits with 100 plus the number of
// inheritable handles it has, or 0 for none (the check's probe).
//
// $ cc -O2 -nostartfiles -nostdlib -o ubhost.exe tools/ubhost.c
//       libunabashed.obj -lkernel32 -lgcc
// Included by ubhost.c.
// This is free and unencumbered software released into the public domain.
#include <stddef.h>
#include <stdint.h>

#define W32(r, p) __declspec(dllimport) r __stdcall p
typedef struct {
    int       cb;
    void     *reserved, *desktop, *title;
    int       x, y, xsize, ysize, xchars, ychars, fill, flags;
    short     show, cbreserved2;
    uint8_t  *reserved2;
    uintptr_t input, output, error;
} StartupInfo;  // STARTUPINFOW
typedef struct { void *a, *b, *c, *d, *e, *f; } CriticalSection;

W32(void,      ExitProcess(unsigned)) __attribute((noreturn));
W32(uintptr_t, CreateThread(void *, size_t, unsigned (__stdcall *)(void *), void *, int, int *));
W32(int,       WaitForSingleObject(uintptr_t, int));
W32(int,       GetExitCodeThread(uintptr_t, unsigned *));
W32(uint16_t *, GetCommandLineW(void));
W32(int,       GetModuleFileNameW(uintptr_t, uint16_t *, int));
W32(uintptr_t, GetStdHandle(int));
W32(int,       SetHandleInformation(uintptr_t, int, int));
W32(int,       GetFileType(uintptr_t));
W32(void,      GetStartupInfoW(StartupInfo *));
W32(int,       GetCurrentProcessId(void));
W32(uintptr_t, GetCurrentProcess(void));
W32(int,       SetConsoleCtrlHandler(int (__stdcall *)(unsigned), int));
W32(void,      InitializeCriticalSection(CriticalSection *));
W32(void,      EnterCriticalSection(CriticalSection *));
W32(void,      LeaveCriticalSection(CriticalSection *));
W32(void,      Sleep(int));
W32(int,       WriteFile(uintptr_t, void const *, int, int *, void *));
W32(uintptr_t, GetModuleHandleW(uint16_t const *));
W32(void *,    GetProcAddress(uintptr_t, char const *));

// GCC calls these, and so does the library: no CRT here to give them.
// Their loops must not become calls to themselves.
#pragma GCC optimize("no-tree-loop-distribute-patterns")

void *memcpy(void *restrict d, void const *restrict s, size_t n)
{
    void *r = d;
    #if defined(__x86_64__) || defined(__i386__)
    __asm volatile ("rep movsb" : "+D"(d), "+S"(s), "+c"(n) : : "memory");
    #else
    unsigned char *dd = d;
    unsigned char const *ss = s;
    for (size_t i = 0; i < n; i++) dd[i] = ss[i];
    #endif
    return r;
}

void *memmove(void *d, void const *s, size_t n)
{
    unsigned char *dd = d;
    unsigned char const *ss = s;
    if (dd < ss) {
        for (size_t i = 0; i < n; i++) dd[i] = ss[i];
    } else {
        for (size_t i = n; i; i--) dd[i-1] = ss[i-1];
    }
    return d;
}

void *memset(void *d, int c, size_t n)
{
    void *r = d;
    #if defined(__x86_64__) || defined(__i386__)
    __asm volatile ("rep stosb" : "+D"(d), "+c"(n) : "a"(c) : "memory");
    #else
    unsigned char *dd = d;
    for (size_t i = 0; i < n; i++) dd[i] = (unsigned char)c;
    #endif
    return r;
}

int memcmp(void const *a, void const *b, size_t n)
{
    unsigned char const *x = a, *y = b;
    for (size_t i = 0; i < n; i++) {
        if (x[i] != y[i]) return x[i] - y[i];
    }
    return 0;
}

void *memchr(void const *s, int c, size_t n)
{
    unsigned char const *p = s;
    for (size_t i = 0; i < n; i++) {
        if (p[i] == (unsigned char)c) return (void *)(p + i);
    }
    return 0;
}

size_t strlen(char const *s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

// A drop-in for unabashed.exe: shells take it for their own (wide.cpp),
// passing it CRT descriptors and quoting its command line their way
__attribute((used, section(".unabash")))
static char const unabashedmark[] = "unabashed";

static ub_os *runos;
static CriticalSection spawncs;

static void spawnlock(void *user, int lock)
{
    (void)user;
    if (lock) {
        EnterCriticalSection(&spawncs);
    } else {
        LeaveCriticalSection(&spawncs);
    }
}

// Console events as the console group's signals; closing the console
// waits here for the run to end the process (as the executable does).
static int __stdcall onctrl(unsigned type)
{
    switch (type) {
    case 0: ub_signal(runos, 2 | UB_GROUP); return 1;  // CTRL_C
    case 1: ub_signal(runos, 3 | UB_GROUP); return 1;  // CTRL_BREAK
    case 2:                                            // CTRL_CLOSE
        ub_signal(runos, 1 | UB_GROUP);
        for (;;) Sleep(1000);
    }
    return 0;
}

// UTF-16 to WTF-8 (lone surrogates kept), null-terminated, from buf.
static char *wtf8(char *buf, uint16_t const *s, ptrdiff_t len)
{
    char *p = buf;
    for (ptrdiff_t i = 0; i < len; i++) {
        uint32_t c = s[i];
        if (c>=0xd800 && c<0xdc00 && i+1<len && s[i+1]>=0xdc00 && s[i+1]<0xe000) {
            c = 0x10000 + ((c - 0xd800)<<10) + (s[++i] - 0xdc00);
        }
        if (c < 0x80) {
            *p++ = (char)c;
        } else if (c < 0x800) {
            *p++ = (char)(0xc0 | c>>6);
            *p++ = (char)(0x80 | (c & 63));
        } else if (c < 0x10000) {
            *p++ = (char)(0xe0 | c>>12);
            *p++ = (char)(0x80 | (c>>6 & 63));
            *p++ = (char)(0x80 | (c & 63));
        } else {
            *p++ = (char)(0xf0 | c>>18);
            *p++ = (char)(0x80 | (c>>12 & 63));
            *p++ = (char)(0x80 | (c>>6 & 63));
            *p++ = (char)(0x80 | (c & 63));
        }
    }
    *p = 0;
    return buf;
}

// Started with Ctrl-C ignored (bit 0 of the process parameters'
// ConsoleFlags, through the PEB)
static int ctrlcignored(void)
{
    unsigned char *peb = 0, *params = 0;
    unsigned flags = 0;
    #if defined(__x86_64__)
    __asm volatile ("mov %%gs:0x60, %0" : "=r"(peb));
    memcpy(&params, peb+0x20, sizeof(params));
    memcpy(&flags, params+0x18, 4);
    #elif defined(__i386__)
    __asm volatile ("mov %%fs:0x30, %0" : "=r"(peb));
    memcpy(&params, peb+0x10, sizeof(params));
    memcpy(&flags, params+0x14, 4);
    #endif
    return flags & 1;
}

static int64_t parentpid(void)
{
    typedef int (__stdcall *Query)(uintptr_t, int, void *, int, int *);
    Query q = (Query)GetProcAddress(GetModuleHandleW(u"ntdll.dll"),
                                    "NtQueryInformationProcess");
    uintptr_t bi[6] = {0};  // PROCESS_BASIC_INFORMATION
    if (q && !q(GetCurrentProcess(), 0, bi, sizeof(bi), 0)) return (int64_t)bi[5];
    return 0;
}

static ub_cmd    cmd;
static ub_config cfg;

static unsigned __stdcall runner(void *arg)
{
    (void)arg;
    return (unsigned)ub_run(runos, &cmd);
}

static void say(char const *s)
{
    int n = 0;
    WriteFile(GetStdHandle(-12), s, (int)strlen(s), &n, 0);
}

#include "ubcheckw.c"

// The command line's arguments after the program name, as written.
static uint16_t *arguments(uint16_t *s)
{
    if (*s == '"') {
        for (s++; *s && *s!='"'; s++) {}
        s += *s=='"';
    } else {
        for (; *s && *s!=' ' && *s!='\t'; s++) {}
    }
    for (; *s==' ' || *s=='\t'; s++) {}
    return s;
}

static int argsare(uint16_t const *a, char const *s)
{
    for (; *s && *a==(uint16_t)*s; a++, s++) {}
    for (; *a==' ' || *a=='\t'; a++) {}
    return !*s && !*a;
}

__attribute((force_align_arg_pointer))
void mainCRTStartup(void)
{
    static char line[3*32768];
    uint16_t *wl = GetCommandLineW();
    uint16_t *args = arguments(wl);
    if (argsare(args, "--check")) ExitProcess((unsigned)check(0));
    if (argsare(args, "--check --quick")) ExitProcess((unsigned)check(1));
    if (argsare(args, "--fds")) ExitProcess((unsigned)inheritedfds());
    ptrdiff_t n = 0;
    for (; wl[n]; n++) {}
    cmd.cmdline = wtf8(line, wl, n);

    // root, its own directory, and self, itself
    static uint16_t path[32768];
    static char self[3*32768], root[3*32768];
    int len = GetModuleFileNameW(0, path, 32768);
    if (len>0 && len<32768) {
        cfg.self = wtf8(self, path, len);
        int dir = len;
        for (; dir && path[dir-1]!='\\' && path[dir-1]!='/'; dir--) {}
        cfg.root = wtf8(root, path, dir);
    }

    // Standard handles and CRT descriptors, kept from programs
    for (int i = 0; i < 3; i++) {
        uintptr_t h = GetStdHandle(-10 - i);
        if (h && h!=(uintptr_t)-1) SetHandleInformation(h, 1, 0);
        cmd.std[i] = h ? (intptr_t)h : -1;
    }
    static ub_fd fds[256];
    StartupInfo si = {0};
    si.cb = sizeof(si);
    GetStartupInfoW(&si);
    if (si.reserved2 && si.cbreserved2 >= 4) {
        uint32_t count = 0;
        memcpy(&count, si.reserved2, 4);
        int width = sizeof(void *);
        uint32_t room = ((uint32_t)(uint16_t)si.cbreserved2 - 4) / (1 + width);
        count = count<room ? count : room;
        for (uint32_t fd = 3; fd<count && cmd.nfds<256; fd++) {
            uint8_t   flags = si.reserved2[4+fd];
            uintptr_t h = 0;
            memcpy(&h, si.reserved2 + 4 + count + fd*width, width);
            if (!(flags & 1) || !h || h==(uintptr_t)-1) continue;
            int type = GetFileType(h);
            if (type<1 || type>3) continue;
            SetHandleInformation(h, 1, 0);
            fds[cmd.nfds].fd  = (int)fd;
            fds[cmd.nfds++].h = (intptr_t)h;
        }
    }
    cmd.fds    = fds;
    cmd.pid    = GetCurrentProcessId();
    cmd.ppid   = parentpid();
    cmd.sigign = ctrlcignored() ? (uint64_t)1<<(2-1) : 0;

    InitializeCriticalSection(&spawncs);
    cfg.spawnlock = spawnlock;
    cfg.flags     = UB_SHARECONSOLE;
    runos = ub_new(&cfg);
    if (!runos) {
        say("ubhost: ub_new failed\n");
        ExitProcess(126);
    }
    SetConsoleCtrlHandler(onctrl, 1);

    uintptr_t t = CreateThread(0, 1<<16, runner, 0, 0, 0);
    unsigned  r = 0;
    if (!t || WaitForSingleObject(t, -1) || !GetExitCodeThread(t, &r)) {
        say("ubhost: no run thread\n");
        ExitProcess(126);
    }
    int status = (int)r;
    if (status < 0) {
        if (status != -UB_ENOMEM) say("ubhost: run failed\n");
        ExitProcess(status==-UB_ENOMEM ? 2 : 126);
    }
    unsigned code = (unsigned)(status & 255);
    if (status == (UB_SIGNALED|(128+2))) {
        code = 0xc000013a;  // as Ctrl-C ends a native program
    } else if (status & UB_SIGNALED) {
        code |= 0xe0550000u;  // SIGEXIT, as unabashed.exe
    }
    ExitProcess(code);
}
