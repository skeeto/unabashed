// stdrun: run a program with chosen standard handles, for tests
// $ cc -nostartfiles -o stdrun.exe stdrun.cpp
//
// usage: stdrun [-d] [-t SECS] IN OUT ERR -- PROGRAM [ARGS...]
//
// Starts PROGRAM with each of its standard handles (input, output,
// error) given by a word: 0, 1 or 2 for stdrun's own handle of that
// number, the same value for every word naming it (as MSDN's
// redirection sample passes one pipe as both output and error), or -
// for none (a null handle). With -d the program starts detached, with
// no console, so that the programs it starts get consoles of their own.
// The exit status is PROGRAM's, or 124 if it is still running after
// SECS (default 30), when it is killed, 125 on an error. Everything
// after "--" passes to PROGRAM verbatim. Runs on Windows XP and later.
// Used by t/win32 to test the shell's handling of its standard handles.
//
// This is free and unencumbered software released into the public domain.

using u8   = unsigned char;
using b32  = decltype(0);
using i32  = decltype(0);
using u32  = decltype(0u);
using iz   = decltype((u8 *)0 - (u8 *)0);
using uz   = decltype(sizeof(0));
using c16  = decltype(u'0');

#define countof(a)  (iz)(sizeof(a) / sizeof(*(a)))

enum : i32 {
    DETACHED_PROCESS      = 0x00000008,
    HANDLE_FLAG_INHERIT   = 1,
    STARTF_USESTDHANDLES  = 0x100,
    STATUS_TIMEOUT        = 124,
    STATUS_FAIL           = 125,
    WAIT_TIMEOUT          = 0x102,
    INFINITE              = -1,
};

struct StartupInfo {  // STARTUPINFOW
    i32  cb;
    c16 *reserved;
    c16 *desktop;
    c16 *title;
    i32  x, y, xsize, ysize, xchars, ychars, fill, flags;
    i32  show;  // wShowWindow and cbReserved2
    u8  *reserved2;
    uz   input, output, error;
};

struct ProcessInfo {  // PROCESS_INFORMATION
    uz  process;
    uz  thread;
    i32 pid;
    i32 tid;
};

#define W32(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
W32(b32,  CloseHandle(uz));
W32(b32,  CreateProcessW(c16 const *, c16 *, void *, void *, b32, i32, void *,
                         c16 const *, StartupInfo *, ProcessInfo *));
W32(void, ExitProcess[[noreturn]](u32));
W32(c16 *,GetCommandLineW());
W32(b32,  GetExitCodeProcess(uz, i32 *));
W32(uz,   GetStdHandle(i32));
W32(b32,  SetHandleInformation(uz, i32, i32));
W32(b32,  TerminateProcess(uz, u32));
W32(i32,  WaitForSingleObject(uz, i32));
W32(b32,  WriteFile(uz, void const *, i32, i32 *, void *));

#if defined(__x86_64__) || defined(__i386__)
extern "C" void *memset(void *dst, i32 c, uz len)
{
    void *r = dst;
    __asm volatile ("rep stosb" : "+D"(dst), "+c"(len) : "a"(c) : "memory");
    return r;
}
#endif

[[noreturn]] static void fail(char const *msg)
{
    char buf[256];
    i32  len = 0;
    for (char const *s = "stdrun: "; *s; s++) buf[len++] = *s;
    for (; *msg && len<countof(buf)-1; msg++) buf[len++] = *msg;
    buf[len++] = '\n';
    WriteFile(GetStdHandle(-12), buf, len, &len, 0);
    ExitProcess(STATUS_FAIL);
}

// The next word of the command line at or after cmd[i]: its start in
// *word, null-terminated in place, and the offset past it. Words have
// no quotes (flags, numbers and -- only).
static iz nextword(c16 *cmd, iz i, c16 **word)
{
    for (; cmd[i]==' ' || cmd[i]=='\t'; i++) {}
    *word = cmd + i;
    for (; cmd[i] && cmd[i]!=' ' && cmd[i]!='\t'; i++) {}
    return i;
}

static b32 equal(c16 *s, iz len, char const *z)
{
    iz i = 0;
    for (; i<len && z[i] && s[i]==z[i]; i++) {}
    return i==len && !z[i];
}

extern "C" __attribute((force_align_arg_pointer))
void mainCRTStartup()
{
    c16 *cmd = GetCommandLineW();
    iz   i   = 0;
    if (cmd[i] == '"') {  // the program's name
        for (i++; cmd[i] && cmd[i]!='"'; i++) {}
        i += !!cmd[i];
    } else {
        for (; cmd[i] && cmd[i]!=' ' && cmd[i]!='\t'; i++) {}
    }

    i32 flags = 0;
    i32 secs  = 30;
    uz  std[3] = {};
    i32 nstd  = 0;
    c16 *line = 0;
    for (;;) {
        c16 *w   = 0;
        iz   end = nextword(cmd, i, &w);
        iz   len = cmd + end - w;
        i = end;
        if (!len) {
            fail("missing --");
        } else if (equal(w, len, "--")) {
            for (; cmd[i]==' ' || cmd[i]=='\t'; i++) {}
            line = cmd + i;
            break;
        } else if (equal(w, len, "-d")) {
            flags |= DETACHED_PROCESS;
        } else if (equal(w, len, "-t")) {
            i = nextword(cmd, i, &w);
            secs = 0;
            for (; w < cmd+i; w++) {
                if (*w<'0' || *w>'9' || secs>100000) fail("bad timeout");
                secs = secs*10 + (*w - '0');
            }
        } else if (len==1 && nstd<3 && (*w=='-' || (*w>='0' && *w<='2'))) {
            uz h = *w=='-' ? 0 : GetStdHandle(-10 - (*w - '0'));
            if (h) SetHandleInformation(h, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            std[nstd++] = h;
        } else {
            fail("usage: stdrun [-d] [-t SECS] IN OUT ERR -- PROGRAM [ARGS...]");
        }
    }
    if (nstd < 3) fail("missing a handle");
    if (!*line) fail("missing program");

    StartupInfo si = {};
    si.cb     = sizeof(si);
    si.flags  = STARTF_USESTDHANDLES;
    si.input  = std[0];
    si.output = std[1];
    si.error  = std[2];
    ProcessInfo pi = {};
    if (!CreateProcessW(0, line, 0, 0, 1, flags, 0, 0, &si, &pi)) {
        fail("cannot run the program");
    }
    CloseHandle(pi.thread);
    i32 status = STATUS_FAIL;
    if (WaitForSingleObject(pi.process, secs*1000) == WAIT_TIMEOUT) {
        TerminateProcess(pi.process, STATUS_TIMEOUT);
        WaitForSingleObject(pi.process, INFINITE);
        status = STATUS_TIMEOUT;
    } else if (!GetExitCodeProcess(pi.process, &status)) {
        status = STATUS_FAIL;
    }
    ExitProcess((u32)status);
}
