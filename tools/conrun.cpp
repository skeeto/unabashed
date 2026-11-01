// conrun: run a program on a console of its own and act on it, for tests
// $ cc -nostartfiles -o conrun.exe conrun.cpp
//
// usage: conrun [-t SECS] [ACTION...] -- PROGRAM [ARGS...]
//
// Starts PROGRAM on a new console without a window, its standard input
// the console and its output conrun's own, then performs the actions in
// order and waits for PROGRAM to exit:
//   sleep:MS     wait MS milliseconds
//   await:PATH   wait until PATH exists (the program created it)
//   type:TEXT    type TEXT into the console; \n is Enter, \\ a backslash
//   ctrl-c       send Ctrl-C to every process on the console
//   ctrl-break   send Ctrl-Break likewise
// The exit status is PROGRAM's, or 124 if it is still running SECS
// (default 30) after the actions, when it is killed, 125 on an error.
// Everything after "--" passes to PROGRAM verbatim. Runs on Windows XP
// and later. Used by t/win32 to test Ctrl-C and console input.
//
// This is free and unencumbered software released into the public domain.

using u8   = unsigned char;
using u16  = unsigned short;
using b32  = decltype(0);
using i32  = decltype(0);
using u32  = decltype(0u);
using iz   = decltype((u8 *)0 - (u8 *)0);
using uz   = decltype(sizeof(0));
using c16  = decltype(u'0');

#define countof(a)  (iz)(sizeof(a) / sizeof(*(a)))

enum : i32 {
    CREATE_NO_WINDOW      = 0x08000000,
    CTRL_BREAK_EVENT      = 1,
    CTRL_C_EVENT          = 0,
    FILE_SHARE_ALL        = 7,
    GENERIC_READ          = (i32)0x80000000,
    GENERIC_WRITE         = 0x40000000,
    INFINITE              = -1,
    INVALID_FILE_ATTRIBUTES = -1,
    KEY_EVENT             = 1,
    OPEN_EXISTING         = 3,
    STARTF_USESTDHANDLES  = 0x100,
    STATUS_TIMEOUT        = 124,
    STATUS_FAIL           = 125,
    WAIT_TIMEOUT          = 0x102,
};

enum : uz { INVALID_HANDLE_VALUE = (uz)-1 };

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

struct SecurityAttributes {
    i32   len;
    void *desc;
    b32   inherit;
};

struct KeyEvent {  // INPUT_RECORD holding a KEY_EVENT_RECORD
    u16 type;
    u16 pad;
    b32 down;
    u16 repeat;
    u16 vk;
    u16 scan;
    c16 ch;
    u32 state;
};
static_assert(sizeof(KeyEvent) == 20, "");

#define W32(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
W32(b32,  CloseHandle(uz));
W32(uz,   CreateFileW(c16 const *, i32, i32, SecurityAttributes *, i32, i32, uz));
W32(b32,  CreateProcessW(c16 const *, c16 *, void *, void *, b32, i32, void *,
                         c16 const *, StartupInfo *, ProcessInfo *));
W32(void, ExitProcess[[noreturn]](u32));
W32(b32,  GenerateConsoleCtrlEvent(i32, i32));
W32(c16 *,GetCommandLineW());
W32(b32,  GetExitCodeProcess(uz, i32 *));
W32(i32,  GetFileAttributesW(c16 const *));
W32(i32,  GetModuleFileNameW(uz, c16 *, i32));
W32(uz,   GetStdHandle(i32));
W32(b32,  SetConsoleCtrlHandler(b32 (__stdcall *)(u32), b32));
W32(b32,  SetHandleInformation(uz, i32, i32));
W32(void, Sleep(i32));
W32(b32,  TerminateProcess(uz, u32));
W32(i32,  WaitForSingleObject(uz, i32));
W32(b32,  WriteConsoleInputW(uz, KeyEvent *, i32, i32 *));
W32(b32,  WriteFile(uz, void const *, i32, i32 *, void *));

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
#endif

[[noreturn]] static void fail(char const *msg)
{
    char buf[256];
    i32  len = 0;
    for (char const *s = "conrun: "; *s; s++) buf[len++] = *s;
    for (; *msg && len<countof(buf)-1; msg++) buf[len++] = *msg;
    buf[len++] = '\n';
    WriteFile(GetStdHandle(-12), buf, len, &len, 0);
    ExitProcess(STATUS_FAIL);
}

static b32 __stdcall survive(u32)
{
    return 1;  // the console's Ctrl-C is for the program
}


// Command line

struct Arg {
    c16 *val;  // unescaped, null-terminated, null if none
    iz   end;  // offset past the raw argument
};

static c16 argbuf[1<<15];
static iz  argused;

// Parse the argument at or after cmd[i] like CommandLineToArgvW.
static Arg nextarg(c16 *cmd, iz i)
{
    Arg r = {};
    for (; cmd[i]==' ' || cmd[i]=='\t'; i++) {}
    r.end = i;
    if (!cmd[i]) return r;
    c16 *out    = argbuf + argused;
    iz   w      = 0;
    b32  quoted = 0;
    while (cmd[i] && (quoted || (cmd[i]!=' ' && cmd[i]!='\t'))) {
        if (cmd[i] == '\\') {
            iz n = 0;
            for (; cmd[i]=='\\'; i++, n++) {}
            b32 quote = cmd[i] == '"';
            for (iz k = 0; k < (quote ? n/2 : n); k++) out[w++] = '\\';
            if (quote && n%2) out[w++] = cmd[i++];
        } else if (cmd[i] == '"') {
            i++;
            if (quoted && cmd[i]=='"') out[w++] = cmd[i++];
            quoted = !quoted;
        } else {
            out[w++] = cmd[i++];
        }
    }
    out[w++] = 0;
    argused += w;
    r.val = out;
    r.end = i;
    return r;
}

static iz skipprogname(c16 *cmd)
{
    iz i = 0;
    if (cmd[i] == '"') {
        for (i++; cmd[i] && cmd[i]!='"'; i++) {}
        i += !!cmd[i];
    } else {
        for (; cmd[i] && cmd[i]!=' ' && cmd[i]!='\t'; i++) {}
    }
    return i;
}

static b32 prefixed(c16 *s, char const *p, c16 **rest)
{
    for (; *p; p++, s++) {
        if (*s != *p) return 0;
    }
    *rest = s;
    return 1;
}

static i32 parseint(c16 *s)
{
    i32 r = 0;
    if (!*s) fail("bad number");
    for (; *s; s++) {
        if (*s<'0' || *s>'9' || r>100000000) fail("bad number");
        r = r*10 + (*s - '0');
    }
    return r;
}


// Actions

static void type(uz conin, c16 *text)
{
    for (; *text; text++) {
        c16 c  = *text;
        u16 vk = 0;
        if (c=='\\' && text[1]=='n') {
            c  = '\r';
            vk = 0x0d;  // VK_RETURN
            text++;
        } else if (c=='\\' && text[1]=='\\') {
            text++;
        }
        KeyEvent ev[2] = {};
        for (i32 k = 0; k < 2; k++) {
            ev[k].type   = KEY_EVENT;
            ev[k].down   = !k;
            ev[k].repeat = 1;
            ev[k].vk     = vk;
            ev[k].ch     = c;
        }
        i32 n = 0;
        if (!WriteConsoleInputW(conin, ev, 2, &n)) fail("cannot type");
    }
}

[[noreturn]] static void inner(c16 *cmd, iz i)
{
    // Every process on this console gets the Ctrl-C sent below. The
    // program must not inherit an ignored Ctrl-C from conrun.
    SetConsoleCtrlHandler(0, 0);
    SetConsoleCtrlHandler(survive, 1);

    SecurityAttributes sa = {sizeof(sa), 0, 1};
    uz conin = CreateFileW(u"CONIN$", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_ALL,
                           &sa, OPEN_EXISTING, 0, 0);
    if (conin == INVALID_HANDLE_VALUE) fail("no console input");

    // First pass: find the program and the timeout
    i32 secs = 30;
    iz  acts = i;
    c16 *line = 0;
    for (;;) {
        Arg a = nextarg(cmd, i);
        if (!a.val) fail("missing --");
        i = a.end;
        if (a.val[0]=='-' && a.val[1]=='-' && !a.val[2]) {
            for (; cmd[i]==' ' || cmd[i]=='\t'; i++) {}
            line = cmd + i;
            break;
        }
        if (a.val[0]=='-' && a.val[1]=='t' && !a.val[2]) {
            Arg v = nextarg(cmd, i);
            if (!v.val) fail("missing timeout");
            i = v.end;
            secs = parseint(v.val);
        }
    }
    if (!*line) fail("missing program");

    StartupInfo si = {};
    si.cb     = sizeof(si);
    si.flags  = STARTF_USESTDHANDLES;
    si.input  = conin;
    si.output = GetStdHandle(-11);
    si.error  = GetStdHandle(-12);
    SetHandleInformation(si.output, 1, 1);
    SetHandleInformation(si.error, 1, 1);
    ProcessInfo pi = {};
    if (!CreateProcessW(0, line, 0, 0, 1, 0, 0, 0, &si, &pi)) {
        fail("cannot run the program");
    }
    CloseHandle(pi.thread);

    for (i = acts;;) {
        Arg  a = nextarg(cmd, i);
        c16 *rest;
        i = a.end;
        if (a.val[0]=='-' && a.val[1]=='-' && !a.val[2]) {
            break;
        } else if (a.val[0]=='-' && a.val[1]=='t' && !a.val[2]) {
            i = nextarg(cmd, i).end;
        } else if (prefixed(a.val, "sleep:", &rest)) {
            Sleep(parseint(rest));
        } else if (prefixed(a.val, "await:", &rest)) {
            for (i32 t = 0; GetFileAttributesW(rest)==INVALID_FILE_ATTRIBUTES; t += 10) {
                if (t > secs*1000) fail("await timed out");
                Sleep(10);
            }
        } else if (prefixed(a.val, "type:", &rest)) {
            type(conin, rest);
        } else if (prefixed(a.val, "ctrl-c", &rest) && !*rest) {
            GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
        } else if (prefixed(a.val, "ctrl-break", &rest) && !*rest) {
            GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);
        } else {
            fail("unknown action");
        }
    }

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

extern "C" __attribute((force_align_arg_pointer))
void mainCRTStartup()
{
    c16 *cmd = GetCommandLineW();
    iz   i   = skipprogname(cmd);
    Arg  a   = nextarg(cmd, i);
    if (a.val && a.val[0]=='-' && a.val[1]=='-' && a.val[2]=='i' && !a.val[3]) {
        inner(cmd, a.end);
    }

    // Run again as the inner conrun, on a new console with no window
    static c16 self[1024];
    static c16 line[1<<15];
    i32 len = GetModuleFileNameW(0, self, countof(self));
    if (len<=0 || len>=countof(self)) fail("cannot find itself");
    iz n = 0;
    line[n++] = '"';
    for (i32 k = 0; k < len; k++) line[n++] = self[k];
    for (char const *s = "\" --i"; *s; s++) line[n++] = (c16)*s;
    for (; cmd[i] && n<countof(line)-1; i++) line[n++] = cmd[i];
    line[n] = 0;

    StartupInfo si = {};
    si.cb     = sizeof(si);
    si.flags  = STARTF_USESTDHANDLES;
    si.input  = 0;
    si.output = GetStdHandle(-11);
    si.error  = GetStdHandle(-12);
    SetHandleInformation(si.output, 1, 1);
    SetHandleInformation(si.error, 1, 1);
    ProcessInfo pi = {};
    if (!CreateProcessW(self, line, 0, 0, 1, CREATE_NO_WINDOW, 0, 0, &si, &pi)) {
        fail("cannot start itself");
    }
    CloseHandle(pi.thread);
    WaitForSingleObject(pi.process, INFINITE);
    i32 status = STATUS_FAIL;
    GetExitCodeProcess(pi.process, &status);
    ExitProcess((u32)status);
}
