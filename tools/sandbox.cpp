// sandbox: run a program so that it can only write inside one directory
// $ cc -nostartfiles -o sandbox.exe sandbox.cpp -ladvapi32
//
// usage: sandbox [-L] [-E NAME=VALUE]... [-m MIB] [-p COUNT] [-t SECS]
//                DIR -- PROGRAM [ARGS...]
//
// Creates DIR (with parents) and gives it a Low mandatory integrity label,
// then runs PROGRAM in DIR with a Low integrity, privilege-stripped copy of
// the caller's token, inside a job object that kills the whole process
// tree when the sandbox exits for any reason. Low integrity processes may
// still read anything the user can, but writes are limited to Low labeled
// places: DIR, %USERPROFILE%\AppData\LocalLow, HKCU\Software\AppDataLow.
//
// Everything after "--" passes to PROGRAM verbatim, quoting included.
// PROGRAM is found relative to the caller's directory and PATH, not DIR.
// Standard handles are inherited, and the exit status is PROGRAM's, or
// 124 on timeout, 125 on a sandbox error, 126 if PROGRAM cannot run, 127
// if it cannot be found. Background processes die when PROGRAM exits.
//
// With -E the environment is empty except for the -E variables (sorted,
// the last of a repeated name winning), so runs are reproducible. -L
// keeps the caller's integrity level: the token is still restricted and
// the job, limits and timeout still apply, but writes are no longer
// confined to DIR. It exists only to record curated scripts with
// programs that cannot start at Low integrity (MSYS2, see t/msys).
//
// Windows XP has no integrity levels, so there it warns and relies on the
// job object and the restricted token (no Administrators group) alone.
//
// This is free and unencumbered software released into the public domain.

using u8   = unsigned char;
using u16  = unsigned short;
using b32  = decltype(0);
using i32  = decltype(0);
using u32  = decltype(0u);
using i64  = decltype(0ll);
using u64  = decltype(0ull);
using iz   = decltype((u8 *)0 - (u8 *)0);
using uz   = decltype(sizeof(0));
using byte = decltype('0');
using c16  = decltype(u'0');

#define assert(c)   while (!(c)) __builtin_unreachable()
#define countof(a)  (iz)(sizeof(a) / sizeof(*(a)))

template<typename T>
void *operator new(uz, T *p) noexcept { return p; }

struct Arena {
    byte *beg = {};
    byte *end = {};
};

[[noreturn]] static void os_oom();

template<typename T>
static T *alloc(Arena *a, iz count = 1)
{
    iz size = sizeof(T);
    iz pad  = -(uz)a->beg & (alignof(T) - 1);
    if (count >= (a->end - a->beg - pad)/size) {
        os_oom();
    }
    T *r = (T *)(a->beg + pad);
    a->beg += pad + count*size;
    for (iz i = 0; i < count; i++) {
        new (r+i) T();
    }
    return r;
}

struct Str16 {
    c16 *data = {};
    iz   len  = {};
};

static iz c16len(c16 const *s)
{
    iz len = 0;
    for (; s[len]; len++) {}
    return len;
}

static b32 equals(Str16 s, c16 const *z)
{
    iz i = 0;
    for (; i<s.len && z[i] && s.data[i]==z[i]; i++) {}
    return i==s.len && !z[i];
}


// Win32

enum : i32 {
    CP_UTF8                          = 65001,
    CREATE_BREAKAWAY_FROM_JOB        = 0x01000000,
    CREATE_SUSPENDED                 = 0x00000004,
    CREATE_UNICODE_ENVIRONMENT       = 0x00000400,
    DISABLE_MAX_PRIVILEGE            = 1,
    DUPLICATE_SAME_ACCESS            = 2,
    ERROR_FILE_NOT_FOUND             = 2,
    ERROR_PATH_NOT_FOUND             = 3,
    FILE_ATTRIBUTE_DIRECTORY         = 0x10,
    INFINITE                         = -1,
    INVALID_FILE_ATTRIBUTES          = -1,
    JOB_OBJECT_LIMIT_ACTIVE_PROCESS  = 0x0008,
    JOB_OBJECT_LIMIT_JOB_MEMORY      = 0x0200,
    JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION = 0x0400,
    JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x2000,
    JobObjectExtendedLimitInformation = 9,
    LABEL_SECURITY_INFORMATION       = 0x10,
    MEM_COMMIT                       = 0x1000,
    MEM_RESERVE                      = 0x2000,
    PAGE_READWRITE                   = 4,
    SDDL_REVISION_1                  = 1,
    SE_FILE_OBJECT                   = 1,
    SE_GROUP_INTEGRITY               = 0x20,
    SEM_FAILCRITICALERRORS           = 0x0001,
    SEM_NOGPFAULTERRORBOX            = 0x0002,
    STARTF_USESTDHANDLES             = 0x100,
    STD_ERROR_HANDLE                 = -12,
    STD_INPUT_HANDLE                 = -10,
    STD_OUTPUT_HANDLE                = -11,
    TOKEN_ASSIGN_PRIMARY             = 0x0001,
    TOKEN_DUPLICATE                  = 0x0002,
    TOKEN_QUERY                      = 0x0008,
    TOKEN_ADJUST_DEFAULT             = 0x0080,
    TokenIntegrityLevel              = 25,
    WAIT_TIMEOUT                     = 0x102,
};

enum : uz {
    INVALID_HANDLE_VALUE = (uz)-1,
};

struct StartupInfo {  // STARTUPINFOW
    u32  cb;
    c16 *lpReserved;
    c16 *lpDesktop;
    c16 *lpTitle;
    u32  dwX, dwY, dwXSize, dwYSize;
    u32  dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;
    u16  wShowWindow, cbReserved2;
    void *lpReserved2;
    uz   hStdInput, hStdOutput, hStdError;
};

struct ProcessInfo {  // PROCESS_INFORMATION
    uz  hProcess;
    uz  hThread;
    u32 dwProcessId;
    u32 dwThreadId;
};

struct JobLimits {  // JOBOBJECT_EXTENDED_LIMIT_INFORMATION
    i64 proctime, jobtime;
    u32 flags;
    uz  minws, maxws;
    u32 activeprocs;
    uz  affinity;
    u32 priority, scheduling;
    u64 io[6];
    uz  procmem, jobmem, peakprocmem, peakjobmem;
};
static_assert(sizeof(JobLimits) == (sizeof(uz)==4 ? 112 : 144), "");

struct SidAndAttributes {  // SID_AND_ATTRIBUTES, TOKEN_MANDATORY_LABEL
    void *sid;
    u32   attrs;
};

struct SidAuthority {  // SID_IDENTIFIER_AUTHORITY
    u8 value[6];
};

#define W32(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
W32(b32,    AllocateAndInitializeSid(SidAuthority *, u8, u32, u32, u32, u32,
                                     u32, u32, u32, u32, void **));
W32(b32,    AssignProcessToJobObject(uz, uz));
W32(b32,    CloseHandle(uz));
W32(b32,    ConvertStringSecurityDescriptorToSecurityDescriptorW(
                c16 const *, u32, void **, u32 *));
W32(b32,    CreateDirectoryW(c16 const *, void *));
W32(uz,     CreateJobObjectW(void *, c16 const *));
W32(b32,    CreateProcessAsUserW(uz, c16 const *, c16 *, void *, void *, b32,
                                 u32, void *, c16 const *, StartupInfo *,
                                 ProcessInfo *));
W32(b32,    CreateRestrictedToken(uz, u32, u32, SidAndAttributes *, u32,
                                  void *, u32, void *, uz *));
W32(b32,    DuplicateHandle(uz, uz, uz, uz *, u32, b32, u32));
W32(void,   ExitProcess[[noreturn]](u32));
W32(c16 *,  GetCommandLineW());
W32(b32,    GetConsoleMode(uz, u32 *));
W32(uz,     GetCurrentProcess());
W32(b32,    GetExitCodeProcess(uz, u32 *));
W32(u32,    GetFileAttributesW(c16 const *));
W32(u32,    GetFullPathNameW(c16 const *, u32, c16 *, c16 **));
W32(u32,    GetLastError());
W32(u32,    GetLengthSid(void *));
W32(b32,    GetSecurityDescriptorSacl(void *, b32 *, void **, b32 *));
W32(uz,     GetStdHandle(i32));
W32(u32,    GetVersion());
W32(b32,    OpenProcessToken(uz, u32, uz *));
W32(u32,    ResumeThread(uz));
W32(b32,    SetConsoleCtrlHandler(b32 (__stdcall *)(u32), b32));
W32(u32,    SetErrorMode(u32));
W32(b32,    SetInformationJobObject(uz, i32, void *, u32));
W32(u32,    SetNamedSecurityInfoW(c16 *, i32, u32, void *, void *, void *,
                                  void *));
W32(b32,    SetTokenInformation(uz, i32, void *, u32));
W32(b32,    TerminateJobObject(uz, u32));
W32(b32,    TerminateProcess(uz, u32));
W32(byte *, VirtualAlloc(uz, uz, u32, u32));
W32(u32,    WaitForSingleObject(uz, u32));
W32(i32,    WideCharToMultiByte(u32, u32, c16 const *, i32, char *, i32,
                                char const *, b32 *));
W32(b32,    WriteConsoleW(uz, c16 const *, u32, u32 *, void *));
W32(b32,    WriteFile(uz, void const *, u32, u32 *, void *));

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
#endif


// Messages

enum : i32 {
    STATUS_TIMEOUT = 124,
    STATUS_FAIL    = 125,
    STATUS_NOEXEC  = 126,
    STATUS_NOENT   = 127,
};

struct Msg {
    c16 *buf = {};
    iz   len = {};
    iz   cap = {};
};

static Msg newmsg(Arena *a)
{
    Msg m = {};
    m.cap = 1<<16;
    m.buf = alloc<c16>(a, m.cap);
    return m;
}

static void print(Msg *m, c16 const *s)
{
    for (; *s && m->len<m->cap; s++) {
        m->buf[m->len++] = *s;
    }
}

static void print(Msg *m, u32 x)
{
    c16  tmp[16];
    c16 *end = tmp + countof(tmp);
    c16 *beg = end;
    *--beg = 0;
    do {
        *--beg = (c16)(u'0' + x%10);
    } while (x /= 10);
    print(m, beg);
}

// Write to standard error: UTF-16 to a console, otherwise UTF-8.
static void flush(Msg *m, Arena scratch)
{
    uz  h    = GetStdHandle(STD_ERROR_HANDLE);
    u32 mode = 0;
    u32 n    = 0;
    if (GetConsoleMode(h, &mode)) {
        WriteConsoleW(h, m->buf, (u32)m->len, &n, 0);
    } else if (m->len) {
        i32 len = WideCharToMultiByte(CP_UTF8, 0, m->buf, (i32)m->len,
                                      0, 0, 0, 0);
        char *buf = alloc<char>(&scratch, len);
        WideCharToMultiByte(CP_UTF8, 0, m->buf, (i32)m->len, buf, len, 0, 0);
        WriteFile(h, buf, (u32)len, &n, 0);
    }
    m->len = 0;
}

// Print "sandbox: MSG[: ARG][ (error ERR)]" on its own line.
static void report(Arena scratch, c16 const *msg, c16 const *arg, u32 err)
{
    Msg m = newmsg(&scratch);
    print(&m, u"sandbox: ");
    print(&m, msg);
    if (arg) {
        print(&m, u": ");
        print(&m, arg);
    }
    if (err) {
        print(&m, u" (error ");
        print(&m, err);
        print(&m, u")");
    }
    print(&m, u"\n");
    flush(&m, scratch);
}

[[noreturn]] static void fail(Arena scratch, c16 const *msg, c16 const *arg,
                              u32 err, i32 status = STATUS_FAIL)
{
    report(scratch, msg, arg, err);
    ExitProcess((u32)status);
}

[[noreturn]] static void os_oom()
{
    static char const msg[] = "sandbox: out of memory\n";
    u32 n = 0;
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg)-1, &n, 0);
    ExitProcess(STATUS_FAIL);
}

[[noreturn]] static void usage(Arena scratch, i32 status)
{
    static c16 const text[] =
    u"usage: sandbox [-L] [-E NAME=VALUE]... [-m MIB] [-p COUNT] [-t SECS]\n"
    u"               DIR -- PROGRAM [ARGS...]\n"
    u"  -E N=V    run with an empty environment plus each N=V given\n"
    u"  -L        keep the caller's integrity level (curated scripts only)\n"
    u"  -m MIB    limit the committed memory of the process tree\n"
    u"  -p COUNT  limit the number of live processes in the tree\n"
    u"  -t SECS   kill the process tree after SECS seconds (status 124)\n"
    u"PROGRAM runs in DIR at Low integrity, so it may write only there\n"
    u"and in other Low places (LocalLow), and dies with the sandbox.\n";
    Msg m = newmsg(&scratch);
    print(&m, text);
    flush(&m, scratch);
    ExitProcess((u32)status);
}


// Command line

// Find the end of the program name, which has no escapes.
static iz skipprogname(Str16 cmd)
{
    iz i = 0;
    if (i<cmd.len && cmd.data[i]=='"') {
        for (i++; i<cmd.len && cmd.data[i]!='"'; i++) {}
        i += i < cmd.len;
    } else {
        for (; i<cmd.len && cmd.data[i]!=' ' && cmd.data[i]!='\t'; i++) {}
    }
    return i;
}

struct Arg {
    Str16 val;       // unescaped and null-terminated, null data if none
    iz    beg = {};  // offset of the raw argument in the command line
    iz    end = {};  // offset just past the raw argument
};

// Parse the argument at or after cmd[i] like CommandLineToArgvW (and
// ssg's splitcmdline), leaving cmd intact so the rest can pass verbatim.
static Arg nextarg(Arena *a, Str16 cmd, iz i)
{
    Arg r = {};
    for (; i<cmd.len && (cmd.data[i]==' ' || cmd.data[i]=='\t'); i++) {}
    r.beg = r.end = i;
    if (i == cmd.len) {
        return r;
    }

    c16 *out    = alloc<c16>(a, cmd.len-i+1);
    iz   w      = 0;
    b32  quoted = 0;
    while (i<cmd.len && (quoted || (cmd.data[i]!=' ' && cmd.data[i]!='\t'))) {
        if (cmd.data[i] == '\\') {
            iz n = 0;
            for (; i<cmd.len && cmd.data[i]=='\\'; i++, n++) {}
            b32 quote = i<cmd.len && cmd.data[i]=='"';
            for (iz k = 0; k < (quote ? n/2 : n); k++) {
                out[w++] = '\\';
            }
            if (quote && n%2) {
                out[w++] = cmd.data[i++];  // escaped quote
            }
        } else if (cmd.data[i] == '"') {
            i++;
            if (quoted && i<cmd.len && cmd.data[i]=='"') {
                out[w++] = cmd.data[i++];
            }
            quoted = !quoted;
        } else {
            out[w++] = cmd.data[i++];
        }
    }
    r.val.data = out;
    r.val.len  = w;
    r.end      = i;
    return r;
}

// Parse a decimal integer in [0, max], or return -1.
static i32 parseint(Str16 s, i32 max)
{
    if (!s.len) {
        return -1;
    }
    i32 r = 0;
    for (iz i = 0; i < s.len; i++) {
        i32 d = s.data[i] - '0';
        if (d<0 || d>9 || r>(max - d)/10) {
            return -1;
        }
        r = r*10 + d;
    }
    return r;
}

struct EnvVar {
    EnvVar *next = {};
    Str16   var  = {};  // NAME=VALUE
};

struct Config {
    c16    *dir     = {};
    c16    *line    = {};  // PROGRAM and ARGS, raw
    c16    *program = {};  // unescaped, for messages
    EnvVar *env     = {};  // -E, most recent first
    b32     setenv  = {};  // any -E: the environment is exactly env
    b32     medium  = {};  // -L: keep the integrity level
    i32     mib     = {};
    i32     procs   = {};
    i32     secs    = {};
};

static Str16 copy(Arena *a, Str16 s)
{
    Str16 r = {};
    r.data = alloc<c16>(a, s.len+1);
    r.len  = s.len;
    for (iz i = 0; i < s.len; i++) {
        r.data[i] = s.data[i];
    }
    return r;
}

static Config parseargs(Arena *perm, Arena scratch)
{
    Config r = {};
    Str16  cmd = {};
    cmd.data = GetCommandLineW();
    cmd.len  = c16len(cmd.data);

    iz i = skipprogname(cmd);
    for (;;) {
        Arg arg = nextarg(&scratch, cmd, i);
        i = arg.end;
        if (!arg.val.data) {
            usage(scratch, STATUS_FAIL);
        } else if (r.dir) {
            if (!equals(arg.val, u"--")) {
                usage(scratch, STATUS_FAIL);
            }
            break;
        } else if (equals(arg.val, u"-h")) {
            usage(scratch, 0);
        } else if (equals(arg.val, u"-L")) {
            r.medium = 1;
        } else if (equals(arg.val, u"-E")) {
            Arg val = nextarg(&scratch, cmd, i);
            i = val.end;
            iz eq = 0;
            for (; eq<val.val.len && val.val.data[eq]!='='; eq++) {}
            if (!eq || eq==val.val.len) {
                fail(scratch, u"-E wants NAME=VALUE", val.val.data, 0);
            }
            EnvVar *v = alloc<EnvVar>(perm);
            v->var  = copy(perm, val.val);
            v->next = r.env;
            r.env   = v;
            r.setenv = 1;
        } else if (equals(arg.val, u"-m") || equals(arg.val, u"-p") ||
                   equals(arg.val, u"-t")) {
            Arg val = nextarg(&scratch, cmd, i);
            i = val.end;
            i32 n = 0;
            switch (arg.val.data[1]) {
            case 'm': n = r.mib   = parseint(val.val, sizeof(uz)==4 ? 4095 : 1<<30); break;
            case 'p': n = r.procs = parseint(val.val, 1<<20); break;
            case 't': n = r.secs  = parseint(val.val, 1<<22); break;
            }
            if (n < 1) {
                fail(scratch, u"invalid option value", arg.val.data, 0);
            }
        } else if (arg.val.len>1 && arg.val.data[0]=='-') {
            fail(scratch, u"unknown option", arg.val.data, 0);
        } else {
            r.dir = alloc<c16>(perm, arg.val.len+1);
            for (iz k = 0; k < arg.val.len; k++) {
                r.dir[k] = arg.val.data[k];
            }
        }
    }

    Arg prog = nextarg(perm, cmd, i);
    if (!prog.val.data) {
        usage(scratch, STATUS_FAIL);
    }
    r.program = prog.val.data;
    iz len = cmd.len - prog.beg;
    r.line = alloc<c16>(perm, len+1);  // CreateProcess wants it writable
    for (iz k = 0; k < len; k++) {
        r.line[k] = cmd.data[prog.beg+k];
    }
    return r;
}


// Confinement

static b32 hasintegrity()
{
    return (u8)GetVersion() >= 6;  // Vista
}

// Create the directory and its parents, returning its full path.
static c16 *makedir(Arena *perm, Arena scratch, c16 *dir)
{
    u32 len = GetFullPathNameW(dir, 0, 0, 0);
    if (!len) {
        fail(scratch, u"invalid directory", dir, GetLastError());
    }
    c16 *path = alloc<c16>(perm, len);
    len = GetFullPathNameW(dir, len, path, 0);

    // Failures are fine here: roots, existing parents, and so on.
    for (u32 i = 1; i < len; i++) {
        if (path[i]=='\\' || path[i]=='/') {
            c16 save = path[i];
            path[i] = 0;
            CreateDirectoryW(path, 0);
            path[i] = save;
        }
    }
    CreateDirectoryW(path, 0);

    u32 attr = GetFileAttributesW(path);
    if (attr==(u32)INVALID_FILE_ATTRIBUTES || !(attr&FILE_ATTRIBUTE_DIRECTORY)) {
        fail(scratch, u"cannot create directory", path, GetLastError());
    }
    return path;
}

// Allow Low integrity writes in the directory, inherited by its contents,
// including existing files.
static void labeldir(Arena scratch, c16 *path)
{
    void *sd = 0;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            u"S:(ML;OICI;NW;;;LW)", SDDL_REVISION_1, &sd, 0)) {
        fail(scratch, u"cannot build integrity label", 0, GetLastError());
    }
    b32   present   = 0;
    b32   defaulted = 0;
    void *sacl      = 0;
    GetSecurityDescriptorSacl(sd, &present, &sacl, &defaulted);
    u32 err = SetNamedSecurityInfoW(path, SE_FILE_OBJECT,
                                    LABEL_SECURITY_INFORMATION,
                                    0, 0, 0, sacl);
    if (err) {
        fail(scratch, u"cannot label directory (NTFS required)", path, err);
    }
}

// Make a primary token like the caller's, but with no privileges beyond
// SeChangeNotifyPrivilege, Administrators as deny-only, and Low integrity
// when available, unless medium (-L), which keeps the caller's level.
static uz maketoken(Arena scratch, b32 medium)
{
    uz self = 0;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE |
                          TOKEN_QUERY | TOKEN_ADJUST_DEFAULT, &self)) {
        fail(scratch, u"cannot open process token", 0, GetLastError());
    }

    SidAuthority     nt     = {{0, 0, 0, 0, 0, 5}};
    SidAndAttributes admins = {};
    if (!AllocateAndInitializeSid(&nt, 2, 32, 544, 0, 0, 0, 0, 0, 0,
                                  &admins.sid)) {
        fail(scratch, u"cannot build Administrators SID", 0, GetLastError());
    }
    uz token = 0;
    if (!CreateRestrictedToken(self, DISABLE_MAX_PRIVILEGE, 1, &admins,
                               0, 0, 0, 0, &token)) {
        fail(scratch, u"cannot restrict token", 0, GetLastError());
    }
    CloseHandle(self);

    if (!medium && hasintegrity()) {
        SidAuthority     ml    = {{0, 0, 0, 0, 0, 16}};
        SidAndAttributes label = {};
        if (!AllocateAndInitializeSid(&ml, 1, 0x1000, 0, 0, 0, 0, 0, 0, 0,
                                      &label.sid)) {
            fail(scratch, u"cannot build Low integrity SID", 0, GetLastError());
        }
        label.attrs = SE_GROUP_INTEGRITY;
        u32 size = (u32)sizeof(label) + GetLengthSid(label.sid);
        if (!SetTokenInformation(token, TokenIntegrityLevel, &label, size)) {
            fail(scratch, u"cannot lower token integrity", 0, GetLastError());
        }
    }
    return token;
}

// A job that kills the tree when its last handle closes, i.e. when the
// sandbox exits, and keeps crashes from waiting on an error dialog.
static uz makejob(Arena scratch, Config *c)
{
    uz job = CreateJobObjectW(0, 0);
    if (!job) {
        fail(scratch, u"cannot create job object", 0, GetLastError());
    }
    JobLimits lim = {};
    lim.flags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
    if (c->mib) {
        lim.flags  |= JOB_OBJECT_LIMIT_JOB_MEMORY;
        lim.jobmem  = (uz)c->mib << 20;
    }
    if (c->procs) {
        lim.flags       |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
        lim.activeprocs  = (u32)c->procs;
    }
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                 &lim, sizeof(lim))) {
        fail(scratch, u"cannot set job limits", 0, GetLastError());
    }
    return job;
}

// Compare variable names as Windows sorts them: by uppercased code unit.
static i32 namecmp(Str16 a, Str16 b)
{
    for (iz i = 0;; i++) {
        c16 x = i<a.len && a.data[i]!='=' ? a.data[i] : 0;
        c16 y = i<b.len && b.data[i]!='=' ? b.data[i] : 0;
        x = x>='a' && x<='z' ? (c16)(x - 32) : x;
        y = y>='a' && y<='z' ? (c16)(y - 32) : y;
        if (x != y) return x<y ? -1 : +1;
        if (!x) return 0;
    }
}

// Build a sorted environment block from the -E variables. For a name
// given more than once the last one wins.
static c16 *makeenv(Arena *perm, Arena scratch, EnvVar *env)
{
    iz count = 0;
    iz units = 2;
    for (EnvVar *v = env; v; v = v->next) {
        count++;
        units += v->var.len + 1;
    }
    Str16 *vars = alloc<Str16>(&scratch, count);
    iz     n    = 0;
    for (EnvVar *v = env; v; v = v->next) {  // most recent first
        iz i = 0;
        for (; i<n && namecmp(vars[i], v->var)<0; i++) {}
        if (i<n && !namecmp(vars[i], v->var)) {
            continue;  // overridden by a later -E
        }
        for (iz k = n; k > i; k--) {
            vars[k] = vars[k-1];
        }
        vars[i] = v->var;
        n++;
    }

    c16 *block = alloc<c16>(perm, units);
    iz   len   = 0;
    for (iz i = 0; i < n; i++) {
        for (iz k = 0; k < vars[i].len; k++) {
            block[len++] = vars[i].data[k];
        }
        block[len++] = 0;
    }
    if (!n) {
        block[len++] = 0;  // an empty block still has two terminators
    }
    block[len] = 0;
    return block;
}

static uz inheritable(i32 which)
{
    uz h = GetStdHandle(which);
    uz r = 0;
    if (!h || h==INVALID_HANDLE_VALUE) {
        return 0;
    }
    uz self = GetCurrentProcess();
    DuplicateHandle(self, h, self, &r, 0, 1, DUPLICATE_SAME_ACCESS);
    return r;
}

static b32 __stdcall ignorectrl(u32)
{
    return 1;  // the child gets the same event and decides
}

extern "C" __attribute((force_align_arg_pointer))
void mainCRTStartup()
{
    iz    cap = 1<<22;
    byte *mem = VirtualAlloc(0, (uz)cap, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!mem) {
        os_oom();
    }
    Arena perm    = {mem, mem + cap/2};
    Arena scratch = {mem + cap/2, mem + cap};

    Config c   = parseargs(&perm, scratch);
    c16  *dir  = makedir(&perm, scratch, c.dir);
    if (hasintegrity()) {
        labeldir(scratch, dir);
    } else {
        report(scratch, u"warning: no integrity levels before Vista, "
                        u"confining only the process tree", 0, 0);
    }
    uz   token = maketoken(scratch, c.medium);
    uz   job   = makejob(scratch, &c);
    c16 *env   = c.setenv ? makeenv(&perm, scratch, c.env) : 0;

    // Crashes should end the test, not open a dialog. Inherited.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    StartupInfo si = {};
    si.cb         = sizeof(si);
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = inheritable(STD_INPUT_HANDLE);
    si.hStdOutput = inheritable(STD_OUTPUT_HANDLE);
    si.hStdError  = inheritable(STD_ERROR_HANDLE);

    // Start suspended so the child joins the job before it can spawn
    // anything. Before Windows 8 a process cannot be in two jobs, so if
    // the sandbox itself runs in one, try again outside it.
    ProcessInfo pi    = {};
    u32         flags = CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT;
    for (;;) {
        if (!CreateProcessAsUserW(token, 0, c.line, 0, 0, 1, flags, env, dir,
                                  &si, &pi)) {
            u32 err = GetLastError();
            if (flags & CREATE_BREAKAWAY_FROM_JOB) {
                fail(scratch, u"cannot leave the enclosing job", 0, err);
            }
            b32 noent = err==ERROR_FILE_NOT_FOUND || err==ERROR_PATH_NOT_FOUND;
            fail(scratch, u"cannot run", c.program, err,
                 noent ? STATUS_NOENT : STATUS_NOEXEC);
        }
        if (AssignProcessToJobObject(job, pi.hProcess)) {
            break;
        }
        u32 err = GetLastError();
        TerminateProcess(pi.hProcess, STATUS_FAIL);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (flags & CREATE_BREAKAWAY_FROM_JOB) {
            fail(scratch, u"cannot assign process to job", 0, err);
        }
        flags |= CREATE_BREAKAWAY_FROM_JOB;
    }
    if (si.hStdInput)  CloseHandle(si.hStdInput);
    if (si.hStdOutput) CloseHandle(si.hStdOutput);
    if (si.hStdError)  CloseHandle(si.hStdError);

    SetConsoleCtrlHandler(ignorectrl, 1);
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    u32 ms = c.secs ? (u32)c.secs*1000 : (u32)INFINITE;
    if (WaitForSingleObject(pi.hProcess, ms) == WAIT_TIMEOUT) {
        TerminateJobObject(job, STATUS_TIMEOUT);
        fail(scratch, u"time limit exceeded", 0, 0, STATUS_TIMEOUT);
    }
    u32 status = STATUS_FAIL;
    GetExitCodeProcess(pi.hProcess, &status);
    ExitProcess(status);  // closing the job kills any stragglers
}
