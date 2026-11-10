// Win32 platform layer: the executable, no CRT, no headers, runs on
// Windows XP and later
// $ g++ -std=c++20 -O2 -s -nostartfiles -static-libgcc
//       -o unabashed.exe main_win32.cpp
// The i686 build also takes -Wl,--large-address-aware (4 GiB on x64
// Windows) and, for XP, subsystem version 5.01:
// -Wl,--major-subsystem-version=5,--minor-subsystem-version=1
// This is free and unencumbered software released into the public domain.
#include "src/unabashed.cpp"
#include "src/win32.cpp"

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

static Os *ctrlos;  // the one Os, for the console handler and os_oom

[[noreturn]] static void os_oom()
{
    if (ctrlos) flushw(ctrlos);
    Str msg = "bash: out of memory\n";
    writeall(GetStdHandle(STD_ERROR_HANDLE), msg.data, msg.len);
    ExitProcess(2);
}

static b32 __stdcall onctrl(u32 type)
{
    return ctrlevent(ctrlos, type);
}

static OsVtable const *osvtable(Os *)
{
    return &pltvtable;
}

// Was the process started with Ctrl-C ignored (inherited from
// SetConsoleCtrlHandler(0, 1) in its parent)? The flag is bit 0 of
// RTL_USER_PROCESS_PARAMETERS.ConsoleFlags, reached through the PEB.
static b32 ctrlcignored()
{
    byte *peb = 0;
    #if defined(__x86_64__)
    __asm volatile ("mov %%gs:0x60, %0" : "=r"(peb));
    byte *params = 0;
    __builtin_memcpy(&params, peb+0x20, sizeof(params));
    u32 flags = 0;
    __builtin_memcpy(&flags, params+0x18, 4);
    return flags & 1;
    #elif defined(__i386__)
    __asm volatile ("mov %%fs:0x30, %0" : "=r"(peb));
    byte *params = 0;
    __builtin_memcpy(&params, peb+0x10, sizeof(params));
    u32 flags = 0;
    __builtin_memcpy(&flags, params+0x14, 4);
    return flags & 1;
    #else
    return 0;
    #endif
}

// Runs the shell on its own fiber (see SHELLSTACK). A shell killed by
// SIGINT exits as Ctrl-C ends a native program, which Cygwin's
// processes also take for death by SIGINT; by another signal, with
// SIGEXIT.
static void shellmain(Os *os, void *arg)
{
    i32 status = sh_main(os, (OsInfo *)arg);
    flushw(os);
    u32 code   = (u32)(status & 255);
    if (status == (WSIGNALED|(128+2))) {
        code = 0xc000013a;
    } else if (status & WSIGNALED) {
        code |= SIGEXIT;
    }
    ExitProcess(code);
}

extern "C" __attribute((force_align_arg_pointer))
void mainCRTStartup()
{
    constinit static Os os = {};  // in .bss, no guard
    OsInfo info = {};
    info.reserve = (iz)1 << (sizeof(void *)==8 ? 34 : 28);

    dynamics(&os);
    if (os.lateattach) setupjobs(&os);
    os.perm     = newarena(&os, (iz)1<<24);
    os.wake     = CreateEventW(0, 0, 0, 0);
    os.donetail = &os.done;
    uz self = GetCurrentProcess();
    DuplicateHandle(self, GetCurrentThread(), self, &os.mainthread, 0, 0,
                    DUPLICATE_SAME_ACCESS);
    ctrlos = &os;
    SetConsoleCtrlHandler((void *)onctrl, 1);

    // Startup data lives in its own small arena.
    Arena a = newarena(&os, (iz)1<<26);

    info.cmdline = fromwidez(&a, GetCommandLineW());
    Slice<Str> args = splitcmdline(&a, clone(&a, info.cmdline));
    info.args  = args.data;
    info.nargs = args.len;
    Str flag = "--unabashed-image=";
    if (args.len==2 && startswith(args[1], flag)) {
        info.image = imagearg(cuthead(args[1], flag.len));
    }

    for (i32 cap = 260; cap <= 1<<15; cap *= 2) {
        c16 *buf = alloc<c16>(&a, cap);
        i32  len = GetModuleFileNameW(0, buf, cap);
        if (len>0 && len<cap) {
            Str s = fromwide(&a, buf, len);
            if (startswith(s, "\\\\?\\")) s = cuthead(s, 4);
            for (iz i = 0; i < s.len; i++) {
                s[i] = s[i]=='\\' ? '/' : s[i];
            }
            info.exe = os.exe = s;
            break;
        } else if (!len) {
            break;
        }
    }

    // As Windows spells it: Path tells the core (importnative) that a
    // Windows parent started it, and the core spells it PATH
    Slice<Str> env = {};
    c16 *block = GetEnvironmentStringsW();
    for (c16 *p = block; p && *p;) {
        iz n = 0;
        for (; p[n]; n++) {}
        Str var = fromwide(&a, p, n);
        env = push(&a, env, var);
        p += n + 1;
    }
    FreeEnvironmentStringsW(block);
    info.env  = env.data;
    info.nenv = env.len;
    xpio(&os, env.data, env.len);

    i32  cap = GetCurrentDirectoryW(0, 0);
    c16 *cwd = alloc<c16>(&a, cap+1);
    i32  len = cap>0 ? GetCurrentDirectoryW(cap, cwd) : 0;
    if (len>0 && len<cap) {
        Str s = fromwide(&a, cwd, len);
        if (startswith(s, "\\\\?\\UNC\\")) {
            s = cuthead(s, 6);
            s[0] = '\\';
        } else if (startswith(s, "\\\\?\\")) {
            s = cuthead(s, 4);
        }
        for (iz i = 0; i < s.len; i++) {
            s[i] = s[i]=='\\' ? '/' : s[i];
        }
        info.cwd = s;
    }

    info.windows = 1;
    info.pid  = GetCurrentProcessId();
    chanopen(&os, info.pid);
    hostinfo(&a, &info);

    // Inherited standard handles are often inheritable, and would leak
    // into every child even when redirected away.
    for (i32 i = 0; i < 3; i++) {
        uz h = GetStdHandle(-10 - i);
        h = h ? h : BADHANDLE;
        if (h != BADHANDLE) {
            SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0);
            Wobj *w = 0;
            for (i32 j = 0; j<i && !w; j++) {
                w = info.std[j]==h ? lookup(&os, h) : 0;
            }
            if (w) {
                reg(&os, h, w);  // the same handle twice: one object
            } else {
                track(&os, h);
            }
        }
        info.std[i] = h;
    }
    // Descriptors above 2 from a CRT parent (busybox, ourselves) in the
    // lpReserved2 blob, at this image's width: real handles of a known
    // type only, made the shell's own (not inheritable, registered), a
    // handle that another descriptor also names duplicated
    StartupInfo si = {};
    si.cb = sizeof(si);
    GetStartupInfoW(&si);
    Str blob = {si.reserved2, (iz)((u32)si.show >> 16)};
    Slice<CrtFd>     crt = blob.data ? decodecrtfds(&a, blob, sizeof(void *)) : Slice<CrtFd>{};
    Slice<InheritFd> in  = {};
    for (i32 fd = 3; fd < crt.len; fd++) {
        uz  h    = (uz)crt[fd].handle;
        i32 type = (crt[fd].flags & CRTFD_OPEN) ? GetFileType(h) : 0;
        if (type!=FILE_TYPE_DISK && type!=FILE_TYPE_CHAR && type!=FILE_TYPE_PIPE) {
            continue;
        }
        b32 seen = h==info.std[0] || h==info.std[1] || h==info.std[2];
        for (iz i = 0; i < in.len; i++) seen |= h == in[i].h;
        if (seen) {
            h = plt_dup(&os, h);
            if (h == BADHANDLE) continue;
        } else {
            SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0);
            track(&os, h);
        }
        InheritFd e = {};
        e.fd    = fd;
        e.h     = h;
        e.flags = OF_READ | OF_WRITE;
        in = push(&a, in, e);
    }
    info.inherit  = in.data;
    info.ninherit = in.len;

    if (ctrlcignored()) info.sigign |= (u64)1 << (2-1);
    info.stack = SHELLSTACK;
    os_switch(&os, os_fiber(&os, &a, SHELLSTACK, shellmain, &info));
    __builtin_unreachable();  // shellmain exits the process
}
