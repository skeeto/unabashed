// Win32 platform layer: no CRT, no headers, runs on Windows XP and later
// $ g++ -std=c++20 -O2 -s -nostartfiles -static-libgcc -Wl,--stack,0x1000000
//       -o unabashed.exe main_win32.cpp
//
// Imports only kernel32.dll. Anything newer than XP must be resolved at
// run time with a fallback.
//
// Paths from the core are absolute, native (C:/x) or MSYS-style (/c/x),
// with either separator. Long paths get the extended-length prefix.
// Every handle the shell holds is non-inheritable; spawning makes
// inheritable duplicates of just the child's standard handles.
//
// This is free and unencumbered software released into the public domain.
#include "unabashed.cpp"
#include "wide.cpp"

enum : i32 {
    CREATE_NEW                   = 1,
    CREATE_UNICODE_ENVIRONMENT   = 0x400,
    DUPLICATE_SAME_ACCESS        = 2,
    ERROR_ACCESS_DENIED          = 5,
    ERROR_BROKEN_PIPE            = 109,
    ERROR_DIRECTORY              = 267,
    ERROR_FILE_NOT_FOUND         = 2,
    ERROR_INVALID_PARAMETER      = 87,
    ERROR_NO_DATA                = 232,
    ERROR_PATH_NOT_FOUND         = 3,
    FILE_APPEND_DATA             = 4,
    FILE_ATTRIBUTE_DIRECTORY     = 0x10,
    FILE_ATTRIBUTE_NORMAL        = 0x80,
    FILE_ATTRIBUTE_READONLY      = 1,
    FILE_ATTRIBUTE_REPARSE_POINT = 0x400,
    FILE_ATTRIBUTE_TEMPORARY     = 0x100,
    FILE_FLAG_DELETE_ON_CLOSE    = 0x04000000,
    FILE_FLAG_BACKUP_SEMANTICS   = 0x02000000,
    FILE_FLAG_OPEN_REPARSE_POINT = 0x00200000,
    FILE_READ_ATTRIBUTES         = 0x80,
    FILE_SHARE_ALL               = 7,  // read, write, delete
    FILE_TYPE_CHAR               = 2,
    FILE_TYPE_DISK               = 1,
    FILE_TYPE_PIPE               = 3,
    FIND_FIRST_EX_LARGE_FETCH    = 2,
    FindExInfoBasic              = 1,
    FindExInfoStandard           = 0,
    FindExSearchNameMatch        = 0,
    GENERIC_READ                 = (i32)0x80000000,
    GENERIC_WRITE                = 0x40000000,
    GetFileExInfoStandard        = 0,
    HANDLE_FLAG_INHERIT          = 1,
    INFINITE                     = -1,
    INVALID_FILE_ATTRIBUTES      = -1,
    IO_REPARSE_TAG_NAME_SURROGATE = 0x20000000,  // symlinks, junctions
    MEM_COMMIT                   = 0x1000,
    MEM_RESERVE                  = 0x2000,
    OPEN_ALWAYS                  = 4,
    OPEN_EXISTING                = 3,
    PAGE_NOACCESS                = 1,
    PAGE_READWRITE               = 4,
    STARTF_USESTDHANDLES         = 0x100,
    STD_ERROR_HANDLE             = -12,
};

enum : uz {
    INVALID_HANDLE_VALUE = (uz)-1,
};

struct Filetime {
    u32 lo;
    u32 hi;
};

struct FileAttributes {  // WIN32_FILE_ATTRIBUTE_DATA
    u32      attr;
    Filetime ctime;
    Filetime atime;
    Filetime mtime;
    u32      sizehi;
    u32      sizelo;
};
static_assert(sizeof(FileAttributes) == 36);

struct FileInfo {  // BY_HANDLE_FILE_INFORMATION
    u32      attr;
    Filetime ctime;
    Filetime atime;
    Filetime mtime;
    u32      volume;
    u32      sizehi;
    u32      sizelo;
    u32      nlink;
    u32      indexhi;
    u32      indexlo;
};
static_assert(sizeof(FileInfo) == 52);

struct FindData {  // WIN32_FIND_DATAW
    u32      attr;
    Filetime ctime;
    Filetime atime;
    Filetime mtime;
    u32      sizehi;
    u32      sizelo;
    u32      reparsetag;
    u32      reserved;
    c16      name[260];
    c16      altname[14];
};
static_assert(sizeof(FindData) == 592);

struct StartupInfo {  // STARTUPINFOW
    i32  cb;
    c16 *reserved;
    c16 *desktop;
    c16 *title;
    i32  x;
    i32  y;
    i32  xsize;
    i32  ysize;
    i32  xchars;
    i32  ychars;
    i32  fill;
    i32  flags;
    i32  show;  // wShowWindow and cbReserved2
    u8  *reserved2;
    uz   input;
    uz   output;
    uz   error;
};
static_assert(sizeof(StartupInfo) == (sizeof(void *)==8 ? 104 : 68));

struct ProcessInfo {  // PROCESS_INFORMATION
    uz  process;
    uz  thread;
    i32 pid;
    i32 tid;
};

struct BasicInfo {  // PROCESS_BASIC_INFORMATION
    uz exitstatus;
    uz peb;
    uz affinity;
    uz priority;
    uz pid;
    uz ppid;
};

#define W32(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
W32(b32,    CloseHandle(uz));
W32(uz,     CreateEventW(uz, b32, b32, c16 *));
W32(uz,     CreateThread(uz, uz, u32 (__stdcall *)(void *), void *, i32, i32 *));
W32(uz,     CreateFileW(c16 *, i32, i32, uz, i32, i32, uz));
W32(b32,    CreatePipe(uz *, uz *, uz, i32));
W32(b32,    CreateProcessW(c16 *, c16 *, uz, uz, b32, i32, c16 *, c16 *,
                           StartupInfo *, ProcessInfo *));
W32(b32,    DuplicateHandle(uz, uz, uz, uz *, i32, b32, i32));
W32(void,   ExitProcess[[noreturn]](i32));
W32(b32,    FindClose(uz));
W32(uz,     FindFirstFileExW(c16 *, i32, FindData *, i32, uz, i32));
W32(b32,    FindNextFileW(uz, FindData *));
W32(b32,    FreeEnvironmentStringsW(c16 *));
W32(c16 *,  GetCommandLineW());
W32(b32,    GetComputerNameW(c16 *, i32 *));
W32(b32,    GetConsoleMode(uz, i32 *));
W32(i32,    GetCurrentDirectoryW(i32, c16 *));
W32(uz,     GetCurrentProcess());
W32(i32,    GetCurrentProcessId());
W32(i32,    GetTempPathW(i32, c16 *));
W32(c16 *,  GetEnvironmentStringsW());
W32(b32,    GetExitCodeProcess(uz, i32 *));
W32(b32,    GetFileAttributesExW(c16 *, i32, FileAttributes *));
W32(i32,    GetFileAttributesW(c16 *));
W32(b32,    GetFileInformationByHandle(uz, FileInfo *));
W32(i32,    GetFileType(uz));
W32(i32,    GetFullPathNameW(c16 *, i32, c16 *, c16 **));
W32(i32,    GetLastError());
W32(uz,     GetModuleHandleW(c16 const *));
W32(void *, GetProcAddress(uz, char const *));
W32(uz,     GetStdHandle(i32));
W32(i32,    GetSystemDirectoryW(c16 *, i32));
W32(void,   GetSystemTimeAsFileTime(Filetime *));
W32(b32,    ReadConsoleW(uz, c16 *, i32, i32 *, uz));
W32(b32,    PeekNamedPipe(uz, u8 *, i32, i32 *, i32 *, i32 *));
W32(b32,    ReadFile(uz, u8 *, i32, i32 *, uz));
W32(b32,    SetEndOfFile(uz));
W32(b32,    SetFilePointerEx(uz, i64, i64 *, i32));
W32(b32,    SetHandleInformation(uz, i32, i32));
W32(byte *, VirtualAlloc(byte *, iz, i32, i32));
W32(b32,    VirtualFree(byte *, iz, i32));
W32(b32,    SetEvent(uz));
W32(i32,    WaitForMultipleObjects(i32, uz *, b32, i32));
W32(i32,    WaitForSingleObject(uz, i32));
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

// A thread that performs one blocking read or write at a time for the
// main thread, which waits on its done event (M3a; notes/m3a.md).
struct Helper {
    Helper *next;    // idle list
    Os     *os;
    Io     *io;
    uz      thread;
    uz      go;      // auto-reset: an operation is ready
    uz      done;    // auto-reset: it completed
};

struct IoStatusBlock {
    uz status;
    uz info;
};

struct PipeLocalInfo {  // FILE_PIPE_LOCAL_INFORMATION
    u32 type, config, maxinst, curinst, inquota, readavail;
    u32 outquota, writequota, state, end;
};

using NtQueryInformationFileFn = i32 (__stdcall *)(uz, IoStatusBlock *, void *, u32, i32);

struct Os {
    Fibers fibers;
    void (__stdcall *now)(Filetime *);  // precise clock where available
    NtQueryInformationFileFn queryfile;  // ntdll, resolved at run time
    Io     *pending;  // submitted, oldest first
    Helper *idle;
    Helper  helpers[64];
    i32     nhelpers;

    // Console input converted to UTF-8 but not yet consumed
    uz     conh;
    u8     conbuf[1024];
    iz     conlen;
    iz     conoff;
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

// Translate a system error code into a Linux errno number.
static i32 errnum(i32 code)
{
    static struct { i32 code; i32 err; } table[] = {
        {   2, E_NOENT},        // ERROR_FILE_NOT_FOUND
        {   3, E_NOENT},        // ERROR_PATH_NOT_FOUND
        {   4, E_MFILE},        // ERROR_TOO_MANY_OPEN_FILES
        {   5, E_ACCES},        // ERROR_ACCESS_DENIED
        {   6, E_BADF},         // ERROR_INVALID_HANDLE
        {   8, E_NOMEM},        // ERROR_NOT_ENOUGH_MEMORY
        {  11, E_NOEXEC},       // ERROR_BAD_FORMAT
        {  14, E_NOMEM},        // ERROR_OUTOFMEMORY
        {  15, E_NOENT},        // ERROR_INVALID_DRIVE
        {  17, E_XDEV},         // ERROR_NOT_SAME_DEVICE
        {  19, E_ROFS},         // ERROR_WRITE_PROTECT
        {  21, E_IO},           // ERROR_NOT_READY
        {  32, E_BUSY},         // ERROR_SHARING_VIOLATION
        {  33, E_BUSY},         // ERROR_LOCK_VIOLATION
        {  50, E_NOSYS},        // ERROR_NOT_SUPPORTED
        {  53, E_NOENT},        // ERROR_BAD_NETPATH
        {  65, E_ACCES},        // ERROR_NETWORK_ACCESS_DENIED
        {  67, E_NOENT},        // ERROR_BAD_NET_NAME
        {  80, E_EXIST},        // ERROR_FILE_EXISTS
        {  87, E_INVAL},        // ERROR_INVALID_PARAMETER
        { 109, E_PIPE},         // ERROR_BROKEN_PIPE
        { 112, E_NOSPC},        // ERROR_DISK_FULL
        { 123, E_NOENT},        // ERROR_INVALID_NAME
        { 131, E_INVAL},        // ERROR_NEGATIVE_SEEK
        { 145, E_NOTEMPTY},     // ERROR_DIR_NOT_EMPTY
        { 161, E_NOENT},        // ERROR_BAD_PATHNAME
        { 170, E_BUSY},         // ERROR_BUSY
        { 183, E_EXIST},        // ERROR_ALREADY_EXISTS
        { 191, E_NOEXEC},       // ERROR_INVALID_EXE_SIGNATURE
        { 192, E_NOEXEC},       // ERROR_EXE_MARKED_INVALID
        { 193, E_NOEXEC},       // ERROR_BAD_EXE_FORMAT
        { 206, E_NAMETOOLONG},  // ERROR_FILENAME_EXCED_RANGE
        { 216, E_NOEXEC},       // ERROR_EXE_MACHINE_TYPE_MISMATCH
        { 232, E_PIPE},         // ERROR_NO_DATA
        { 267, E_NOTDIR},       // ERROR_DIRECTORY
        { 740, E_ACCES},        // ERROR_ELEVATION_REQUIRED
        {1314, E_PERM},         // ERROR_PRIVILEGE_NOT_HELD
        {1920, E_ACCES},        // ERROR_CANT_ACCESS_FILE
        {1921, E_LOOP},         // ERROR_CANT_RESOLVE_FILENAME
    };
    for (iz i = 0; i < countof(table); i++) {
        if (table[i].code == code) return table[i].err;
    }
    return E_INVAL;
}

static i32 lasterr()
{
    return errnum(GetLastError());
}

static b32 notfound(i32 code)
{
    return code==ERROR_FILE_NOT_FOUND || code==ERROR_PATH_NOT_FOUND;
}

static u64 ticks(Filetime t)
{
    return (u64)t.hi<<32 | t.lo;
}

// FILETIME (100 ns since 1601) to ns since the Unix epoch.
static i64 unixns(Filetime t)
{
    i64 epoch = 116444736000000000;
    i64 max   = epoch + 0x7fffffffffffffff/100;
    u64 v     = ticks(t);
    i64 r     = v>(u64)max ? max : (i64)v;
    return (r - epoch) * 100;
}

// Absolute, null-terminated UTF-16 path for a system call: /dev/null is
// NUL, and long paths are extended-length so MAX_PATH never applies.
// Falls back to the plain conversion if the system rejects the path,
// leaving the error for the caller's system call to report.
static Str16 widepath(Arena *a, Str path)
{
    if (path == "/dev/null") {
        return towide(a, "NUL");
    }
    Str16 rel = towide(a, nativepath(a, path));
    for (i32 cap = 300;;) {
        c16 *buf = alloc<c16>(a, cap+6);
        i32  len = GetFullPathNameW(rel.data, cap, buf+6, 0);
        if (!len || len>0x8000) {
            return rel;
        } else if (len < cap) {
            Str16 r = {buf+6, len};
            return len<248 ? r : extendpath(buf, len);
        }
        cap = len;
    }
}

static b32 hassuffix(Str s, Str suffix)  // ASCII case-insensitive
{
    if (s.len < suffix.len) return 0;
    Str tail = taketail(s, suffix.len);
    for (iz i = 0; i < tail.len; i++) {
        if (lowercase(tail[i]) != suffix[i]) return 0;
    }
    return 1;
}

static b32 exename(Str path)
{
    static Str exts[] = {".exe", ".com", ".bat", ".cmd", ".sh"};
    for (iz i = 0; i < countof(exts); i++) {
        if (hassuffix(path, exts[i])) return 1;
    }
    return 0;
}


// Platform API

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

// Read a console as UTF-8 regardless of code page, with line endings
// as LF. Ctrl-Z at the start of a line is end of file, as with ReadFile.
static iz readconsole(Os *os, uz h, u8 *buf, iz len)
{
    if (os->conh!=h || os->conoff==os->conlen) {
        c16 tmp[256];  // UTF-8 is at most 3 bytes per unit: fits conbuf
        i32 n = 0;
        if (!ReadConsoleW(h, tmp, countof(tmp), &n, 0)) {
            return -1;
        }
        if (!n || tmp[0]==0x1a) {
            return 0;
        }
        Str s = {};
        s.data = os->conbuf;
        for (i32 i = 0; i < n; i++) {
            if (tmp[i]=='\r' && i+1<n && tmp[i+1]=='\n') continue;
            i32 c = tmp[i];
            if (c>=0xd800 && c<=0xdbff && i+1<n && tmp[i+1]>=0xdc00 && tmp[i+1]<=0xdfff) {
                c = 0x10000 + ((c - 0xd800)<<10) + (tmp[++i] - 0xdc00);
            }
            s.len += utf8encode(s.data+s.len, c);
        }
        os->conh   = h;
        os->conlen = s.len;
        os->conoff = 0;
    }
    iz avail = os->conlen - os->conoff;
    iz n     = len<avail ? len : avail;
    copybytes(buf, os->conbuf+os->conoff, n);
    os->conoff += n;
    return n;
}

static iz os_read(Os *os, Handle h, u8 *buf, iz len)
{
    i32 mode = 0;
    if (GetConsoleMode(h, &mode)) {
        return readconsole(os, h, buf, len);
    }

    i32 n = 0;
    if (!ReadFile(h, buf, chunk(len), &n, 0)) {
        return GetLastError()==ERROR_BROKEN_PIPE ? 0 : -1;
    }
    return n;
}

static Opened os_tempfile(Os *os, Arena)
{
    // A uniquely named file in the temporary directory, deleted by the
    // system when its last handle closes
    Opened r = {};
    r.h = BADHANDLE;
    c16 path[300];
    i32 len = GetTempPathW(260, path);
    if (len<=0 || len>=260) {
        r.err = E_NOENT;
        return r;
    }
    static u32 counter;
    for (i32 try_ = 0; try_ < 100; try_++) {
        u64 tag = ((u64)GetCurrentProcessId()<<32) ^ ++counter ^ (u64)os_now(os);
        i32 n = len;
        for (char c : "sh-thd-") if (c) path[n++] = (c16)c;
        for (i32 i = 0; i < 16; i++) {
            path[n++] = (c16)"0123456789abcdef"[(tag >> (60 - 4*i)) & 15];
        }
        path[n] = 0;
        uz h = CreateFileW(path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_ALL, 0,
                           CREATE_NEW,
                           FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, 0);
        if (h != INVALID_HANDLE_VALUE) {
            r.h = (Handle)h;
            return r;
        }
        i32 code = GetLastError();
        if (code != 80 && code != 183) {  // ERROR_FILE_EXISTS, ERROR_ALREADY_EXISTS
            r.err = errnum(code);
            return r;
        }
    }
    r.err = E_EXIST;
    return r;
}

static Opened os_open(Os *, Arena scratch, Str path, i32 flags, i32 mode)
{
    Opened r = {};
    r.h = BADHANDLE;

    i32 access = flags&OF_READ ? GENERIC_READ : 0;
    if (flags & OF_WRITE) {
        // Without FILE_WRITE_DATA, every write lands at the end, even
        // through handles inherited by children.
        b32 append = (flags&OF_APPEND) && !(flags&OF_TRUNC);
        access |= append ? FILE_APPEND_DATA : GENERIC_WRITE;
    }
    access = access ? access : GENERIC_READ;

    i32 disp = OPEN_EXISTING;
    if (flags & OF_CREATE) {
        disp = flags&OF_EXCL ? CREATE_NEW : OPEN_ALWAYS;
    }
    i32 attr = FILE_ATTRIBUTE_NORMAL;
    if ((flags&OF_CREATE) && !(mode&0200)) {
        attr = FILE_ATTRIBUTE_READONLY;
    }

    Str16 w = {};
    if (path == "/dev/tty") {
        w      = towide(&scratch, flags&OF_WRITE ? Str("CONOUT$") : "CONIN$");
        access = GENERIC_READ | GENERIC_WRITE;
        disp   = OPEN_EXISTING;
    } else {
        w = widepath(&scratch, path);
    }

    uz h = CreateFileW(w.data, access, FILE_SHARE_ALL, 0, disp, attr, 0);
    if (h == INVALID_HANDLE_VALUE) {
        i32 code = GetLastError();
        r.err = errnum(code);
        if (code == ERROR_ACCESS_DENIED) {
            i32 a = GetFileAttributesW(w.data);
            if (a!=INVALID_FILE_ATTRIBUTES && (a&FILE_ATTRIBUTE_DIRECTORY)) {
                r.err = E_ISDIR;
            }
        }
        return r;
    }

    // Truncate separately: CREATE_ALWAYS would fail on hidden files and
    // replace the file's identity.
    if ((flags&OF_TRUNC) && (flags&OF_WRITE) && GetFileType(h)==FILE_TYPE_DISK) {
        if (!SetEndOfFile(h)) {
            r.err = lasterr();
            CloseHandle(h);
            return r;
        }
    }
    r.h = h;
    return r;
}

static void os_close(Os *os, Handle h)
{
    if (h == os->conh) {
        os->conh = os->conlen = os->conoff = 0;
    }
    if (h != BADHANDLE) CloseHandle(h);
}

static Handle os_dup(Os *, Handle h)
{
    uz self = GetCurrentProcess();
    uz r    = 0;
    if (!DuplicateHandle(self, h, self, &r, 0, 0, DUPLICATE_SAME_ACCESS)) {
        return BADHANDLE;
    }
    return r;
}

static iz os_pipecap(Os *)
{
    return 1<<15;  // CreatePipe is asked for 64 KiB; stay well under
}

static OsPipe os_pipe(Os *)
{
    OsPipe r = {};
    uz rd = 0;
    uz wr = 0;
    // Match a Linux pipe's capacity, since the core may fill a pipe
    // before anyone reads it.
    if (!CreatePipe(&rd, &wr, 0, 1<<16)) {
        r.err = lasterr();
        r.r = r.w = BADHANDLE;
        return r;
    }
    r.r = rd;
    r.w = wr;
    return r;
}

static i64 os_seek(Os *, Handle h, i64 off, i32 whence)
{
    i64 pos = 0;
    if (GetFileType(h) != FILE_TYPE_DISK) {
        return -1;
    }
    return SetFilePointerEx(h, off, &pos, whence) ? pos : -1;
}

static Stat fromattrs(u32 attr, Filetime mtime, Filetime atime, i64 size)
{
    Stat r  = {};
    r.type  = attr&FILE_ATTRIBUTE_DIRECTORY ? FT_DIR : FT_FILE;
    r.mode  = r.type==FT_DIR ? 0755 : 0644;
    if (r.type==FT_FILE && (attr&FILE_ATTRIBUTE_READONLY)) {
        r.mode &= ~0222;
    }
    r.nlink = 1;
    r.size  = size;
    r.mtime = unixns(mtime);
    r.atime = unixns(atime);
    r.uid   = 1000;
    r.gid   = 1000;
    return r;
}

// Fill a Stat from an open handle. Permission bits are synthesized:
// directories 0755, files 0644, read-only files lose write bits.
static Stat handlestat(uz h)
{
    Stat r = {};
    FileInfo fi = {};
    if (GetFileInformationByHandle(h, &fi)) {
        i64 size = (i64)((u64)fi.sizehi<<32 | fi.sizelo);
        r = fromattrs(fi.attr, fi.mtime, fi.atime, size);
        r.nlink = (i32)fi.nlink;
        r.dev   = fi.volume;
        r.ino   = (i64)((u64)fi.indexhi<<32 | fi.indexlo);
        return r;
    }
    switch (GetFileType(h)) {
    case FILE_TYPE_CHAR: r.type = FT_CHAR; r.mode = 0666; break;
    case FILE_TYPE_PIPE: r.type = FT_PIPE; r.mode = 0600; break;
    default:             r.err  = E_BADF;  return r;
    }
    r.nlink = 1;
    r.uid   = 1000;
    r.gid   = 1000;
    return r;
}

static Stat os_stat(Os *, Arena scratch, Str path, b32 follow)
{
    Stat r = {};

    // A trailing slash demands a directory.
    b32 dironly = 0;
    while (path.len>1 && path[path.len-1]=='/') {
        if (path.len==3 && path[1]==':') break;  // C:/
        path.len--;
        dironly = 1;
    }

    Str16 w     = widepath(&scratch, path);
    b32   exe   = exename(path);
    i32   flags = FILE_FLAG_BACKUP_SEMANTICS;
    flags |= follow ? 0 : FILE_FLAG_OPEN_REPARSE_POINT;

    // Read access lets the magic number decide whether a file without
    // an executable extension is a program or script.
    i32 access = exe ? FILE_READ_ATTRIBUTES : GENERIC_READ;
    uz  h = CreateFileW(w.data, access, FILE_SHARE_ALL, 0, OPEN_EXISTING, flags, 0);
    if (h == INVALID_HANDLE_VALUE) {
        i32 code = GetLastError();
        if (notfound(code)) {
            r.err = errnum(code);  // the common case in PATH searches
            return r;
        }
    }
    if (h==INVALID_HANDLE_VALUE && access==GENERIC_READ) {
        access = FILE_READ_ATTRIBUTES;
        h = CreateFileW(w.data, access, FILE_SHARE_ALL, 0, OPEN_EXISTING, flags, 0);
    }

    if (h == INVALID_HANDLE_VALUE) {
        // Locked (pagefile.sys), unresolvable (app execution aliases),
        // or not found: the attributes alone still say enough.
        FileAttributes fa = {};
        if (!GetFileAttributesExW(w.data, GetFileExInfoStandard, &fa)) {
            r.err = lasterr();
            return r;
        }
        i64 size = (i64)((u64)fa.sizehi<<32 | fa.sizelo);
        r = fromattrs(fa.attr, fa.mtime, fa.atime, size);
    } else {
        r = handlestat(h);
        if (!r.err && r.type==FT_FILE && !exe && access==GENERIC_READ) {
            u8  magic[2] = {};
            i32 n = 0;
            if (ReadFile(h, magic, 2, &n, 0) && n==2) {
                b32 mz = magic[0]=='M' && magic[1]=='Z';
                b32 sb = magic[0]=='#' && magic[1]=='!';
                r.mode |= mz||sb ? 0111 : 0;
            }
        }
        CloseHandle(h);
        if (r.err) return r;
    }
    if (r.type==FT_FILE && exe) {
        r.mode |= 0111;
    }

    // Without following, a symbolic link or junction is a link.
    FindData fd = {};
    if (!follow && r.type!=FT_CHAR && r.type!=FT_PIPE) {
        i32 attr = GetFileAttributesW(w.data);
        if (attr!=INVALID_FILE_ATTRIBUTES && (attr&FILE_ATTRIBUTE_REPARSE_POINT)) {
            uz f = FindFirstFileExW(w.data, FindExInfoStandard, &fd,
                                    FindExSearchNameMatch, 0, 0);
            if (f != INVALID_HANDLE_VALUE) {
                FindClose(f);
                if (fd.reparsetag & IO_REPARSE_TAG_NAME_SURROGATE) {
                    r.type = FT_LINK;
                    r.mode = 0777;
                }
            }
        }
    }

    if (dironly && r.type!=FT_DIR && r.type!=FT_LINK) {
        r = {};
        r.err = E_NOTDIR;
    }
    return r;
}

static Stat os_fstat(Os *, Handle h)
{
    return handlestat(h);
}

static Dirent *os_list(Os *, Arena *a, Str dir, i32 *err)
{
    *err = 0;

    // Only the first call uses the pattern, so it may share memory with
    // the results that follow.
    Arena tmp = *a;
    Str   pat = concat(&tmp, dir, dir.len && dir[dir.len-1]=='/' ? Str("*") : "/*");
    Str16 w   = widepath(&tmp, pat);

    FindData fd = {};
    uz h = FindFirstFileExW(w.data, FindExInfoBasic, &fd, FindExSearchNameMatch,
                            0, FIND_FIRST_EX_LARGE_FETCH);
    if (h==INVALID_HANDLE_VALUE && GetLastError()==ERROR_INVALID_PARAMETER) {
        // Before Windows 7: no basic info level, no large fetch
        h = FindFirstFileExW(w.data, FindExInfoStandard, &fd,
                             FindExSearchNameMatch, 0, 0);
    }
    if (h == INVALID_HANDLE_VALUE) {
        i32 code = GetLastError();
        *err = code==ERROR_FILE_NOT_FOUND ? 0 : errnum(code);  // empty
        return 0;
    }

    Dirent  *head = 0;
    Dirent **tail = &head;
    do {
        iz len = 0;
        for (; len<countof(fd.name) && fd.name[len]; len++) {}
        Dirent *e = alloc<Dirent>(a);
        e->name = fromwide(a, fd.name, len);
        if ((fd.attr&FILE_ATTRIBUTE_REPARSE_POINT) &&
            (fd.reparsetag&IO_REPARSE_TAG_NAME_SURROGATE)) {
            e->type = FT_LINK;
        } else if (fd.attr & FILE_ATTRIBUTE_DIRECTORY) {
            e->type = FT_DIR;
        } else {
            e->type = FT_FILE;
        }
        *tail = e;
        tail  = &e->next;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return head;
}

// Look up a variable in a "NAME=value" list, case-insensitively.
static Str envget(Str *env, iz nenv, Str name)
{
    for (iz i = 0; i < nenv; i++) {
        Str n = envname(env[i]);
        if (n.len==name.len && hassuffix(n, name) && n.len<env[i].len) {
            return cuthead(env[i], n.len+1);
        }
    }
    return {};
}

static Spawned os_spawn(Os *, Arena scratch, Spawn *s)
{
    Spawned r = {};
    r.proc = BADHANDLE;

    // Command line: MSVCRT quoting, or cmd.exe for batch files, which
    // CreateProcess would otherwise run with no escaping at all.
    Str16 app = {};
    Buf   cmd(&scratch, 1<<10);
    if (hassuffix(s->path, ".bat") || hassuffix(s->path, ".cmd")) {
        Str comspec = envget(s->env, s->nenv, "comspec");
        if (!comspec.len) {
            c16 sys[260];
            i32 len = GetSystemDirectoryW(sys, countof(sys));
            if (!len || len>=countof(sys)) {
                r.err = lasterr();
                return r;
            }
            comspec = concat(&scratch, fromwide(&scratch, sys, len), "\\cmd.exe");
        }
        // cmd would report a missing script itself, with status 1
        Str16 wscript = widepath(&scratch, s->path);
        i32   attr    = GetFileAttributesW(wscript.data);
        if (attr == INVALID_FILE_ATTRIBUTES) {
            r.err = lasterr();
            return r;
        } else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            r.err = E_ACCES;
            return r;
        }
        Str script = fromwide(&scratch, wscript.data, wscript.len);
        quoteprogram(&cmd, comspec);
        print(&cmd, " /d /e:on /v:off /s /c \"");
        quotebatarg(&cmd, script, 1);
        for (iz i = 1; i < s->argc; i++) {
            putbyte(&cmd, ' ');
            if (!quotebatarg(&cmd, s->argv[i], 0)) {
                r.err = E_INVAL;
                return r;
            }
        }
        putbyte(&cmd, '"');
        app = widepath(&scratch, comspec);
    } else {
        quoteprogram(&cmd, s->argc ? s->argv[0] : s->path);
        for (iz i = 1; i < s->argc; i++) {
            putbyte(&cmd, ' ');
            quotearg(&cmd, s->argv[i]);
        }
        app = widepath(&scratch, s->path);
    }
    Str16 line = towide(&scratch, finish(&cmd));
    if (line.len > 32766) {
        r.err = E_2BIG;
        return r;
    }

    // Environment block: sorted, each entry null-terminated, and one
    // more null at the end (towide adds the last).
    Str *env = alloc<Str>(&scratch, s->nenv);
    iz   nenv = 0;
    for (iz i = 0; i < s->nenv; i++) {
        if (envname(s->env[i]).len < s->env[i].len) env[nenv++] = s->env[i];
    }
    sort(env, nenv, winenvless, scratch);
    Buf block(&scratch);
    for (iz i = 0; i < nenv; i++) {
        print(&block, env[i]);
        putbyte(&block, 0);
    }
    if (!nenv) putbyte(&block, 0);
    Str16 wenv = towide(&scratch, finish(&block));

    Str16 cwd = s->cwd.len ? widepath(&scratch, s->cwd) : Str16{};

    // Inheritable duplicates of the standard handles, closed right after
    // creation. Descriptors above 2 are not passed (yet).
    uz self   = GetCurrentProcess();
    uz std[3] = {};
    for (iz i = 0; i < s->nfds; i++) {
        i32 fd = s->fds[i].fd;
        uz  h  = s->fds[i].h;
        if (fd<0 || fd>2 || h==BADHANDLE || std[fd]) continue;
        if (!DuplicateHandle(self, h, self, std+fd, 0, 1, DUPLICATE_SAME_ACCESS)) {
            std[fd] = 0;
        }
    }

    StartupInfo si = {};
    si.cb     = sizeof(si);
    si.flags  = STARTF_USESTDHANDLES;
    si.input  = std[0];
    si.output = std[1];
    si.error  = std[2];
    ProcessInfo pi = {};
    b32 ok = CreateProcessW(app.data, line.data, 0, 0, 1,
                            CREATE_UNICODE_ENVIRONMENT, wenv.data,
                            cwd.data, &si, &pi);
    i32 code = ok ? 0 : GetLastError();
    if (!ok && code==ERROR_DIRECTORY && cwd.data) {
        // Unusable working directory (too long, or gone): run anyway
        ok = CreateProcessW(app.data, line.data, 0, 0, 1,
                            CREATE_UNICODE_ENVIRONMENT, wenv.data,
                            0, &si, &pi);
        code = ok ? 0 : GetLastError();
    }
    for (i32 i = 0; i < 3; i++) {
        if (std[i]) CloseHandle(std[i]);
    }
    if (!ok) {
        r.err = errnum(code);
        return r;
    }
    CloseHandle(pi.thread);
    r.proc = pi.process;
    return r;
}

// Native crashes become the matching signal statuses, as in Cygwin.
static i32 exitstatus(u32 code)
{
    switch (code) {
    case 0xc0000005u: return 128 + 11;  // access violation: SIGSEGV
    case 0xc00000fdu: return 128 + 11;  // stack overflow
    case 0xc000001du: return 128 + 4;   // illegal instruction: SIGILL
    case 0xc0000094u: return 128 + 8;   // divide by zero: SIGFPE
    case 0xc000013au: return 128 + 2;   // Ctrl-C: SIGINT
    case 0xc0000409u: return 128 + 6;   // fast fail: SIGABRT
    }
    return (i32)(code & 0xff);
}

static i32 os_waitproc(Os *, Handle proc)
{
    i32 code = 127;
    WaitForSingleObject(proc, INFINITE);
    if (!GetExitCodeProcess(proc, &code)) {
        code = 127;
    }
    CloseHandle(proc);
    return exitstatus((u32)code);
}

// Asynchronous operations (M3a): process exits through the process
// handle, pipe reads through PeekNamedPipe when data is waiting, pipe
// writes directly when the pipe has room, and otherwise a helper
// thread performs the blocking call. Disk files and consoles complete
// at once. The main thread waits on process handles and helpers' done
// events together (at most 64 at a time).

static u32 __stdcall helpermain(void *arg)
{
    Helper *h = (Helper *)arg;
    for (;;) {
        WaitForSingleObject(h->go, INFINITE);
        Io *io = h->io;
        if (io->op == IO_READ) {
            io->n   = os_read(h->os, io->h, io->buf, io->len);
            io->err = io->n<0 ? E_IO : 0;
            io->n   = io->n<0 ? 0 : io->n;
        } else {
            i32 n = 0;
            if (WriteFile(io->h, io->buf, chunk(io->len), &n, 0) && n) {
                io->n = n;
            } else {
                i32 code = GetLastError();
                io->err = code==ERROR_NO_DATA || code==ERROR_BROKEN_PIPE ? E_PIPE : E_IO;
            }
        }
        SetEvent(h->done);
    }
}

static b32 tohelper(Os *os, Io *io)
{
    Helper *h = os->idle;
    if (h) {
        os->idle = h->next;
    } else {
        if (os->nhelpers == countof(os->helpers)) return 0;
        h = os->helpers + os->nhelpers;
        h->os   = os;
        h->go   = CreateEventW(0, 0, 0, 0);
        h->done = CreateEventW(0, 0, 0, 0);
        h->thread = CreateThread(0, 1<<16, helpermain, h, 0x10000, 0);
        if (!h->go || !h->done || !h->thread) return 0;
        os->nhelpers++;
    }
    h->io = io;
    io->plt[0] = (uz)h;
    SetEvent(h->go);
    return 1;
}

static b32 procdone(Io *io, i32 timeout)
{
    if (WaitForSingleObject(io->h, timeout)) return 0;
    i32 code = 127;
    if (!GetExitCodeProcess(io->h, &code)) code = 127;
    CloseHandle(io->h);
    io->n = exitstatus((u32)code);
    return 1;
}

// Try to complete a read or write without blocking.
static b32 trypipeio(Os *os, Io *io)
{
    i32 type = GetFileType(io->h);
    if (type != FILE_TYPE_PIPE) {
        if (type==FILE_TYPE_CHAR && io->op==IO_READ) return 0;
        if (io->op == IO_READ) {
            io->n   = os_read(os, io->h, io->buf, io->len);
            io->err = io->n<0 ? E_IO : 0;
            io->n   = io->n<0 ? 0 : io->n;
        } else {
            io->n   = io->len;
            io->err = os_write(os, io->h, io->buf, io->len) ? 0 : E_IO;
        }
        return 1;
    }
    if (io->op == IO_READ) {
        i32 avail = 0;
        if (!PeekNamedPipe(io->h, 0, 0, 0, &avail, 0)) {
            io->n   = 0;
            io->err = GetLastError()==ERROR_BROKEN_PIPE ? 0 : E_IO;
            return 1;
        }
        if (!avail) return 0;
        i32 n = 0;
        if (!ReadFile(io->h, io->buf, chunk(avail<io->len ? avail : io->len), &n, 0)) {
            io->err = GetLastError()==ERROR_BROKEN_PIPE ? 0 : E_IO;
        }
        io->n = n;
        return 1;
    }
    if (!os->queryfile) return 0;
    IoStatusBlock iosb = {};
    PipeLocalInfo info = {};
    if (os->queryfile(io->h, &iosb, &info, sizeof(info), 24) || !info.writequota) {
        return 0;
    }
    i32 n = 0;
    iz  len = io->len<(iz)info.writequota ? io->len : (iz)info.writequota;
    if (WriteFile(io->h, io->buf, chunk(len), &n, 0) && n) {
        io->n = n;
    } else {
        i32 code = GetLastError();
        io->err = code==ERROR_NO_DATA || code==ERROR_BROKEN_PIPE ? E_PIPE : E_IO;
    }
    return 1;
}

static void os_submit(Os *os, Io *io)
{
    io->done   = 0;
    io->next   = 0;
    io->plt[0] = 0;
    b32 now = io->op==IO_PROC ? procdone(io, 0) : trypipeio(os, io);
    if (now) {
        io->done = 1;
        return;
    }
    if (io->op!=IO_PROC && !tohelper(os, io)) {
        // no helper: block here
        if (io->op == IO_READ) {
            io->n   = os_read(os, io->h, io->buf, io->len);
            io->err = io->n<0 ? E_IO : 0;
            io->n   = io->n<0 ? 0 : io->n;
        } else {
            io->n   = io->len;
            io->err = os_write(os, io->h, io->buf, io->len) ? 0 : E_PIPE;
        }
        io->done = 1;
        return;
    }
    Io **tail = &os->pending;
    while (*tail) tail = &(*tail)->next;
    *tail = io;
}

static Io *os_wait(Os *os, i64 deadline)
{
    uz  handles[64];
    Io *ios[64];
    i32 n = 0;
    for (Io *io = os->pending; io && n<countof(handles); io = io->next) {
        Helper *h = (Helper *)io->plt[0];
        handles[n] = h ? h->done : io->h;
        ios[n++]   = io;
    }
    if (!n) return 0;
    i32 timeout = INFINITE;
    if (deadline == WAIT_POLL) {
        timeout = 0;
    } else if (deadline > 0) {
        i64 left = (deadline - os_now(os) + 999999) / 1000000;
        timeout = left<0 ? 0 : left>(1<<30) ? (1<<30) : (i32)left;
    }
    i32 r = WaitForMultipleObjects(n, handles, 0, timeout);
    if (r<0 || r>=n) return 0;
    Io *io = ios[r];
    Helper *h = (Helper *)io->plt[0];
    if (h) {
        h->next  = os->idle;
        os->idle = h;
    } else {
        procdone(io, 0);
    }
    for (Io **p = &os->pending; *p; p = &(*p)->next) {
        if (*p == io) {
            *p = io->next;
            break;
        }
    }
    io->done = 1;
    return io;
}

static b32 os_cancel(Os *os, Io *io)
{
    if (io->plt[0]) return 0;  // a helper owns it until it completes
    for (Io **p = &os->pending; *p; p = &(*p)->next) {
        if (*p == io) {
            *p = io->next;
            return 1;
        }
    }
    return 0;
}

static i64 os_now(Os *os)
{
    Filetime t = {};
    os->now(&t);
    return unixns(t);
}

// The main thread's stack reservation, from this image's PE header,
// so that deep shell recursion fails as cleanly as on Linux.
extern "C" byte __ImageBase[];

static iz stackreserve()
{
    i32 lfanew = 0;
    __builtin_memcpy(&lfanew, __ImageBase+0x3c, 4);
    byte *opt   = __ImageBase + lfanew + 24;
    c16   magic = 0;
    __builtin_memcpy(&magic, opt, 2);
    u64 r64 = 0;
    u32 r32 = 0;
    if (magic == 0x20b) {
        __builtin_memcpy(&r64, opt+72, 8);
        return (iz)r64;
    }
    __builtin_memcpy(&r32, opt+72, 4);
    return (iz)r32;
}

static byte *fiberstack(Os *, iz) { return 0; }    // unused on Win32
static void  fiberunstack(Os *, byte *, iz) {}

// Resolve a function newer than XP, or null.
static void *dynamic(c16 const *dll, char const *name)
{
    uz m = GetModuleHandleW(dll);
    return m ? GetProcAddress(m, name) : 0;
}

static Str fromwidez(Arena *a, c16 *s)
{
    iz len = 0;
    for (; s[len]; len++) {}
    return fromwide(a, s, len);
}

extern "C" __attribute((force_align_arg_pointer))
void mainCRTStartup()
{
    static Os os;
    OsInfo info = {};
    info.reserve = (iz)1 << (sizeof(void *)==8 ? 34 : 28);

    os.now = GetSystemTimeAsFileTime;
    os.queryfile = (NtQueryInformationFileFn)dynamic(u"ntdll.dll", "NtQueryInformationFile");
    void *precise = dynamic(u"kernel32.dll", "GetSystemTimePreciseAsFileTime");
    if (precise) {
        os.now = (void (__stdcall *)(Filetime *))precise;  // Windows 8
    }

    // Startup data lives in its own small arena.
    Arena a = newarena(&os, (iz)1<<26);

    Slice<Str> args = splitcmdline(&a, fromwidez(&a, GetCommandLineW()));
    info.args  = args.data;
    info.nargs = args.len;

    // Windows spells it Path, but scripts expect PATH.
    Slice<Str> env = {};
    c16 *block = GetEnvironmentStringsW();
    for (c16 *p = block; p && *p;) {
        iz n = 0;
        for (; p[n]; n++) {}
        Str var = fromwide(&a, p, n);
        Str name = envname(var);
        if (name.len==4 && hassuffix(name, "path")) {
            copybytes(var.data, (u8 *)"PATH", 4);
        }
        env = push(&a, env, var);
        p += n + 1;
    }
    FreeEnvironmentStringsW(block);
    info.env  = env.data;
    info.nenv = env.len;

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
    info.uid  = info.euid = 1000;
    info.gid  = info.egid = 1000;
    using NtQueryInformationProcess = i32 (__stdcall *)(uz, i32, void *, i32, i32 *);
    NtQueryInformationProcess query =
        (NtQueryInformationProcess)dynamic(u"ntdll.dll", "NtQueryInformationProcess");
    BasicInfo bi = {};
    if (query && !query(GetCurrentProcess(), 0, &bi, sizeof(bi), 0)) {
        info.ppid = (i64)bi.ppid;
    }

    c16 host[256];
    i32 hostlen = countof(host);
    if (GetComputerNameW(host, &hostlen)) {
        info.host = fromwide(&a, host, hostlen);
    }

    // Inherited standard handles are often inheritable, and would leak
    // into every child even when redirected away.
    for (i32 i = 0; i < 3; i++) {
        uz h = GetStdHandle(-10 - i);
        h = h ? h : BADHANDLE;
        if (h != BADHANDLE) {
            SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0);
        }
        info.std[i] = h;
    }
    info.stack = stackreserve();
    ExitProcess(sh_main(&os, &info));
}
