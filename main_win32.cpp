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
    ERROR_OPERATION_ABORTED      = 995,
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
    MEM_RELEASE                  = 0x8000,
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
W32(uz,     GetCurrentThread());
W32(b32,    GetNumberOfConsoleInputEvents(uz, i32 *));
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
W32(b32,    RegisterWaitForSingleObject(uz *, uz, void (__stdcall *)(void *, u8),
                                        void *, i32, i32));
W32(void,   SetLastError(i32));
W32(b32,    UnregisterWaitEx(uz, uz));
W32(b32,    SetEndOfFile(uz));
W32(b32,    SetFilePointerEx(uz, i64, i64 *, i32));
W32(b32,    SetHandleInformation(uz, i32, i32));
W32(uz,     OpenProcess(i32, b32, i32));
W32(b32,    TerminateProcess(uz, u32));
W32(b32,    SetConsoleCtrlHandler(void *, b32));
W32(byte *, VirtualAlloc(byte *, iz, i32, i32));
W32(b32,    VirtualFree(byte *, iz, i32));
W32(b32,    SetEvent(uz));
W32(void,   Sleep(i32));
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

// Asynchronous I/O (M3b, notes/m3b.md). Every handle the shell holds
// to a pipe or a console is registered with a Wobj, the state of its
// file object, shared by its duplicates. A synchronous file object
// serializes I/O: while a helper thread blocks in a read on it, a
// peek, a second read or the last CloseHandle would block too. So
// while a helper operation is in flight, every operation on the Wobj
// goes through its state machine and never touches the handle:
// - reads take read-ahead bytes first, then PeekNamedPipe and read
//   exactly what is there (never more than asked); with nothing there,
//   a helper reads one byte (a console line) into the read-ahead, so
//   that an abandoned read (read -t, a signal) loses nothing;
// - other operations queue behind the one in flight;
// - closing the last handle hands the private duplicate to the helper
//   side, closed once the operation ends;
// - a child given the object while a read is in flight or bytes are
//   read ahead reads through a fresh pipe (Interpose) fed by a pump,
//   and what it leaves is drained back into the read-ahead when it
//   exits; a child given it directly (lent) may block in its own reads
//   meanwhile, so the shell then reads it only through helpers.
// Helpers, process wait callbacks and the console control handler
// push onto the wake queue and set the wake event, the only thing the
// main thread waits on.

struct Wnode {  // an entry of the wake queue
    Wnode *next;
    i32    kind;  // WN_*
};
enum : i32 { WN_HELPER = 1, WN_PROC };

enum : i32 {
    K_PIPE = 1,  // either end of a pipe
    K_CONIN,     // console input
    K_CONOUT,    // console output
    K_CHAR,      // another character device (NUL)
};

enum : i32 {  // Helper operations
    H_READ = 1,  // one byte from a pipe
    H_CONREAD,   // a console line
    H_WRITE,     // the core's bytes
    H_PUMP,      // the object into an Interpose's pipe
};

enum : i32 { P_READING = 1, P_WRITING, P_DONE };  // a pump's state

struct Wobj;
struct Os;

struct Helper {
    Wnode   node;    // pushed when its operation ends
    Helper *next;    // idle list
    Os     *os;
    Wobj   *obj;
    i32     op;      // H_*
    u8     *wbuf;    // H_WRITE: the core's bytes
    iz      wlen;
    iz      n;       // result: bytes in buf, or written
    i32     code;    // GetLastError of a failure, else 0
    b32     eof;     // the source reached its end
    uz      iw;      // H_PUMP: the interposed pipe's write end
    uz      thread;
    uz      go;      // auto-reset: an operation is ready
    i32     live;    // atomic: issued and not yet pushed
    i32     inio;    // atomic: in a call CancelSynchronousIo may end
    i32     stop;    // atomic: H_PUMP should stop
    i32     pstate;  // atomic: P_*
    u8      buf[1<<12];
};

struct Interpose {
    Interpose *next;      // free list
    uz         ir;        // the shell's read end, drained when unused
    uz         iw;        // write end, until the pump takes it
    i32        children;  // live children reading through it
    b32        started;   // the pump runs (else after the read in flight)
};

struct Lend {  // a child holding a registered object
    Lend *next;
    uz    proc;  // the core's handle of the child process
    Wobj *obj;
    b32   ip;    // through obj->ip
};

struct Wobj {
    Wobj      *next;     // free list
    i32        refs;     // registered handles
    i32        kind;     // K_*
    b32        closing;  // no handle left: release once unused
    uz         own;      // private duplicate the helpers use, or 0
    Helper    *helper;   // its operation in flight, or null
    Io        *active;   // the Io it serves, null once abandoned
    Io        *queue;    // waiting for it to be idle (linked by next)
    u8        *ahead;    // read but not yet consumed
    iz         aoff;
    iz         alen;
    iz         acap;
    b32        eof;      // an end of file to report once
    i32        lent;     // children holding it directly
    Interpose *ip;
};

struct Hent {  // handle registration
    Hent *next;
    uz    h;
    Wobj *obj;
};

struct Pwait {  // an IO_PROC waited for by the thread pool
    Wnode  node;
    Pwait *next;   // free list
    Os    *os;
    Io    *io;     // null once withdrawn
    uz     wait;
    i32    fired;  // atomic: the callback ran
    b32    gone;   // the wait was unregistered already
};

enum : uz {  // Io.plt[0]: where an Io is
    S_NONE, S_ACTIVE, S_QUEUED, S_DONE, S_PROC, S_ZERO,
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
    b32  (__stdcall *cancelsync)(uz);    // CancelSynchronousIo, Vista
    Arena      perm;       // platform records, recycled through free lists
    uz         wake;       // auto-reset event for the main thread
    Wnode     *wakeq;      // atomic stack pushed by other threads
    Io        *done;       // completed, not yet returned by os_wait
    Io       **donetail;
    Io        *zero;       // zero-length reads not ready (read -t 0)
    i32        nwait;      // Ios that will complete by themselves
    Helper    *idle;
    i32        nhelpers;
    Hent      *htab[64];
    Hent      *freehent;
    Wobj      *freeobj;
    Lend      *lends;
    Lend      *freelend;
    Pwait     *freepwait;
    Interpose *freeip;
    u64        sigown;     // atomic: signals for the process
    u64        siggrp;     // atomic: signals for the console group
    uz         mainthread;
    i32        mainio;     // atomic: odd while the main thread blocks in
                           // a read Ctrl-C may cancel
};

enum : i32 { MAXHELPERS = 64 };

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

// Asynchronous I/O machinery (see the comment on Wobj above)

static void wakepush(Os *os, Wnode *n)
{
    Wnode *head = __atomic_load_n(&os->wakeq, __ATOMIC_RELAXED);
    do {
        n->next = head;
    } while (!__atomic_compare_exchange_n(&os->wakeq, &head, n, 1,
                                          __ATOMIC_RELEASE, __ATOMIC_RELAXED));
    SetEvent(os->wake);
}

static b32 sigwaiting(Os *os)
{
    return (__atomic_load_n(&os->sigown, __ATOMIC_SEQ_CST) |
            __atomic_load_n(&os->siggrp, __ATOMIC_SEQ_CST)) != 0;
}

static Hent **hslot(Os *os, uz h)
{
    return os->htab + ((h>>2) & (countof(os->htab) - 1));
}

static Wobj *lookup(Os *os, uz h)
{
    for (Hent *e = *hslot(os, h); e; e = e->next) {
        if (e->h == h) return e->obj;
    }
    return 0;
}

static void reg(Os *os, uz h, Wobj *w)
{
    Hent *e = os->freehent;
    if (e) {
        os->freehent = e->next;
    } else {
        e = alloc<Hent>(&os->perm);
    }
    Hent **slot = hslot(os, h);
    e->h    = h;
    e->obj  = w;
    e->next = *slot;
    *slot   = e;
    w->refs++;
}

static Wobj *unreg(Os *os, uz h)
{
    for (Hent **p = hslot(os, h); *p; p = &(*p)->next) {
        Hent *e = *p;
        if (e->h == h) {
            *p = e->next;
            e->next = os->freehent;
            os->freehent = e;
            e->obj->refs--;
            return e->obj;
        }
    }
    return 0;
}

static Wobj *newobj(Os *os, i32 kind)
{
    Wobj *w = os->freeobj;
    if (w) {
        os->freeobj = w->next;
        u8 *ahead = w->ahead;
        iz  acap  = w->acap;
        *w = {};
        w->ahead = ahead;
        w->acap  = acap;
    } else {
        w = alloc<Wobj>(&os->perm);
    }
    w->kind = kind;
    return w;
}

static b32 busy(Wobj *w)
{
    return w->helper || w->ip;
}

// Release an object nothing refers to any more.
static void maybefree(Os *os, Wobj *w)
{
    if (w->refs || busy(w) || w->lent || w->queue || w->closing==2) return;
    if (w->own) CloseHandle(w->own);
    w->own     = 0;
    w->closing = 2;  // released: never twice
    w->next    = os->freeobj;
    os->freeobj = w;
}

// What kind of object a handle refers to: 0 for files, which never
// block and need no state.
static i32 classify(uz h)
{
    i32 mode = 0;
    switch (GetFileType(h)) {
    case FILE_TYPE_PIPE:
        return K_PIPE;
    case FILE_TYPE_CHAR:
        if (!GetConsoleMode(h, &mode)) return K_CHAR;
        return GetNumberOfConsoleInputEvents(h, &mode) ? K_CONIN : K_CONOUT;
    }
    return 0;
}

// Register a new handle that is not a duplicate of a registered one.
static Wobj *track(Os *os, uz h)
{
    i32 kind = h==INVALID_HANDLE_VALUE ? 0 : classify(h);
    if (!kind) return 0;
    Wobj *w = newobj(os, kind);
    reg(os, h, w);
    return w;
}

static void reserveahead(Wobj *w, iz extra)
{
    iz len = w->alen - w->aoff;
    if (w->aoff) {
        for (iz i = 0; i < len; i++) w->ahead[i] = w->ahead[w->aoff+i];
        w->aoff = 0;
        w->alen = len;
    }
    if (w->acap-len >= extra) return;
    iz cap = (len + extra + 0xffff) & ~(iz)0xffff;
    u8 *buf = (u8 *)VirtualAlloc(0, cap, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!buf) os_oom();
    copybytes(buf, w->ahead, len);
    if (w->ahead) VirtualFree((byte *)w->ahead, 0, MEM_RELEASE);
    w->ahead = buf;
    w->acap  = cap;
}

static void appendahead(Wobj *w, u8 *buf, iz len)
{
    if (!len) return;
    reserveahead(w, len);
    copybytes(w->ahead+w->alen, buf, len);
    w->alen += len;
}

static iz takeahead(Wobj *w, u8 *buf, iz len)
{
    iz avail = w->alen - w->aoff;
    iz n     = avail<len ? avail : len;
    copybytes(buf, w->ahead+w->aoff, n);
    w->aoff += n;
    if (w->aoff == w->alen) w->aoff = w->alen = 0;
    return n;
}

// Console input, converted to UTF-8 with line endings as LF, into out
// (3 bytes per UTF-16 unit at most). -1 if Ctrl-Z begins it (end of
// file).
static iz conconvert(c16 *tmp, i32 n, u8 *out)
{
    if (n && tmp[0]==0x1a) return -1;
    Str s = {out, 0};
    for (i32 i = 0; i < n; i++) {
        if (tmp[i]=='\r' && i+1<n && tmp[i+1]=='\n') continue;
        i32 c = tmp[i];
        if (c>=0xd800 && c<=0xdbff && i+1<n && tmp[i+1]>=0xdc00 && tmp[i+1]<=0xdfff) {
            c = 0x10000 + ((c - 0xd800)<<10) + (tmp[++i] - 0xdc00);
        }
        s.len += utf8encode(s.data+s.len, c);
    }
    return s.len;
}

enum : i32 { CONUNITS = 1024 };  // UTF-16 units per console read

// Read a console line into out (3*CONUNITS bytes): the byte count, 0
// at end of file, -1 on error with *code set (ERROR_OPERATION_ABORTED
// for Ctrl-C, which ends the read with nothing).
static iz conline(uz h, u8 *out, i32 *code)
{
    c16 tmp[CONUNITS];
    i32 n = 0;
    SetLastError(0);
    b32 ok = ReadConsoleW(h, tmp, countof(tmp), &n, 0);
    *code = GetLastError();
    if (!ok || (!n && *code==ERROR_OPERATION_ABORTED)) {
        *code = ok ? ERROR_OPERATION_ABORTED : *code;
        return -1;
    }
    *code = 0;
    iz r = conconvert(tmp, n, out);
    return r<0 ? 0 : r;
}

// One read of the object by a helper, into its buffer: bytes, 0 at
// the end, -1 on failure (code set).
static iz srcread(Helper *hp, iz len)
{
    Wobj *w = hp->obj;
    iz    r = 0;
    __atomic_store_n(&hp->inio, 1, __ATOMIC_SEQ_CST);
    if (w->kind == K_CONIN) {
        r = conline(w->own, hp->buf, &hp->code);
    } else {
        i32 n = 0;
        if (ReadFile(w->own, hp->buf, (i32)len, &n, 0)) {
            r = n;
        } else {
            hp->code = GetLastError();
            r = hp->code==ERROR_BROKEN_PIPE ? 0 : -1;
            hp->code = r ? hp->code : 0;
        }
    }
    __atomic_store_n(&hp->inio, 0, __ATOMIC_SEQ_CST);
    return r;
}

// Copy the object into the interposed pipe until it ends or the shell
// asks to stop (stopinterpose). Bytes read after the request stay in
// buf for the read-ahead. The write end is closed at the end, so that
// the children see end of file.
static void pumploop(Helper *hp)
{
    for (;;) {
        __atomic_store_n(&hp->pstate, P_READING, __ATOMIC_SEQ_CST);
        if (__atomic_load_n(&hp->stop, __ATOMIC_SEQ_CST)) break;
        iz n = srcread(hp, countof(hp->buf));
        if (n <= 0) {
            hp->eof = !n;
            break;
        }
        // Announce the write before the last look at stop, so that
        // the shell, having asked, never sees P_READING while a write
        // is still to come
        __atomic_store_n(&hp->pstate, P_WRITING, __ATOMIC_SEQ_CST);
        if (__atomic_load_n(&hp->stop, __ATOMIC_SEQ_CST)) {
            hp->n = n;
            break;
        }
        if (!writeall(hp->iw, hp->buf, n)) {
            hp->n = n;  // cannot happen while the shell holds the read end
            break;
        }
    }
    CloseHandle(hp->iw);
    hp->iw = 0;
    __atomic_store_n(&hp->pstate, P_DONE, __ATOMIC_SEQ_CST);
}

static u32 __stdcall helpermain(void *arg)
{
    Helper *hp = (Helper *)arg;
    Os     *os = hp->os;
    for (;;) {
        WaitForSingleObject(hp->go, INFINITE);
        hp->n    = 0;
        hp->code = 0;
        hp->eof  = 0;
        switch (hp->op) {
        case H_READ:
        case H_CONREAD: {
            iz n = srcread(hp, 1);
            hp->n   = n<0 ? 0 : n;
            hp->eof = !n;
        } break;
        case H_WRITE: {
            i32 n = 0;
            if (WriteFile(hp->obj->own, hp->wbuf, chunk(hp->wlen), &n, 0) && n) {
                hp->n = n;
            } else {
                hp->code = GetLastError();
                hp->code = hp->code ? hp->code : ERROR_NO_DATA;
            }
        } break;
        case H_PUMP:
            pumploop(hp);
            break;
        }
        __atomic_store_n(&hp->live, 0, __ATOMIC_SEQ_CST);
        wakepush(os, &hp->node);  // the last use of hp
    }
}

// An idle helper, or null if none can be had.
static Helper *gethelper(Os *os)
{
    Helper *hp = os->idle;
    if (hp) {
        os->idle = hp->next;
        return hp;
    }
    if (os->nhelpers == MAXHELPERS) return 0;
    hp = alloc<Helper>(&os->perm);
    hp->os        = os;
    hp->node.kind = WN_HELPER;
    hp->go        = CreateEventW(0, 0, 0, 0);
    hp->thread    = CreateThread(0, 1<<16, helpermain, hp, 0x10000, 0);
    if (!hp->go || !hp->thread) return 0;  // leaked, as it is rare
    os->nhelpers++;
    return hp;
}

// Hand an operation on w to a helper. False if no helper or private
// duplicate can be had.
static b32 issue(Os *os, Wobj *w, uz h, i32 op, Io *io, uz iw = 0)
{
    if (!w->own) {
        uz self = GetCurrentProcess();
        if (!DuplicateHandle(self, h, self, &w->own, 0, 0, DUPLICATE_SAME_ACCESS)) {
            w->own = 0;
            return 0;
        }
    }
    Helper *hp = gethelper(os);
    if (!hp) return 0;
    hp->obj    = w;
    hp->op     = op;
    hp->stop   = 0;
    hp->pstate = 0;
    hp->iw     = iw;
    if (io && op==H_WRITE) {
        hp->wbuf = io->buf;
        hp->wlen = io->len;
    }
    w->helper = hp;
    w->active = io;
    if (io) {
        io->plt[0] = S_ACTIVE;
        io->plt[1] = (uz)w;
        os->nwait++;
    }
    __atomic_store_n(&hp->live, 1, __ATOMIC_SEQ_CST);
    SetEvent(hp->go);
    return 1;
}

// Try for a while to end a helper's blocking call, where the system
// can (Vista and later). A cancel can come before the call starts, so
// it is repeated while the operation lasts, for at most about 20 ms;
// otherwise the operation simply runs on, abandoned.
static void cancelhelper(Os *os, Helper *hp)
{
    if (!os->cancelsync) return;
    for (i32 i = 0; i<20 && __atomic_load_n(&hp->live, __ATOMIC_SEQ_CST); i++) {
        if (__atomic_load_n(&hp->inio, __ATOMIC_SEQ_CST) && os->cancelsync(hp->thread)) {
            return;
        }
        Sleep(1);
    }
}

// Read what a pipe holds now, without blocking, at most len bytes.
static iz peekread(uz h, u8 *buf, iz len)
{
    i32 avail = 0;
    if (len<=0 || !PeekNamedPipe(h, 0, 0, 0, &avail, 0) || !avail) return 0;
    i32 n = 0;
    if (!ReadFile(h, buf, chunk(avail<len ? avail : len), &n, 0)) return 0;
    return n;
}

static void pushdone(Os *os, Io *io)
{
    io->plt[0] = S_DONE;
    io->next   = 0;
    *os->donetail = io;
    os->donetail  = &io->next;
}

// Complete a read from the read-ahead, and whatever the pipe holds
// beyond it while that is safe (h), or with the pending end of file.
static void readahead(Wobj *w, uz h, Io *io)
{
    iz n = takeahead(w, io->buf, io->len);
    if (!n && w->eof) {
        w->eof = 0;
    } else if (n && h && w->kind==K_PIPE && !busy(w) && !w->lent) {
        n += peekread(h, io->buf+n, io->len-n);
    }
    io->n   = n;
    io->err = 0;
}

// Perform a read or write on an idle object, completing it, or start a
// helper for it. False if it is in flight now.
static b32 startio(Os *os, Wobj *w, Io *io)
{
    uz h = io->h;
    if (io->op == IO_READ) {
        if (w->alen>w->aoff || w->eof) {
            readahead(w, h, io);
            return 1;
        }
        if (w->kind==K_PIPE && !w->lent) {
            i32 avail = 0;
            if (!PeekNamedPipe(h, 0, 0, 0, &avail, 0)) {
                io->n   = 0;
                io->err = GetLastError()==ERROR_BROKEN_PIPE ? 0 : E_IO;
                return 1;
            }
            if (avail) {
                i32 n = 0;
                if (!ReadFile(h, io->buf, chunk(avail<io->len ? avail : io->len), &n, 0)) {
                    io->err = GetLastError()==ERROR_BROKEN_PIPE ? 0 : E_IO;
                }
                io->n = n;
                return 1;
            }
        }
        if (!io->len) {
            // read -t 0: not ready, and nothing to wait for
            io->plt[0] = S_ZERO;
            io->next   = os->zero;
            os->zero   = io;
            return 0;
        }
        if (issue(os, w, h, w->kind==K_CONIN ? H_CONREAD : H_READ, io)) return 0;
        io->n   = os_read(os, h, io->buf, io->len);  // no helper: block
        io->err = io->n<0 ? E_IO : 0;
        io->n   = io->n<0 ? 0 : io->n;
        return 1;
    }

    if (w->kind == K_PIPE) {
        if (!w->lent && os->queryfile) {
            IoStatusBlock iosb = {};
            PipeLocalInfo info = {};
            if (!os->queryfile(h, &iosb, &info, sizeof(info), 24) && info.writequota) {
                i32 n = 0;
                iz  len = io->len<(iz)info.writequota ? io->len : (iz)info.writequota;
                if (WriteFile(h, io->buf, chunk(len), &n, 0) && n) {
                    io->n = n;
                } else {
                    i32 code = GetLastError();
                    io->err = code==ERROR_NO_DATA || code==ERROR_BROKEN_PIPE ? E_PIPE : E_IO;
                }
                return 1;
            }
        }
        if (issue(os, w, h, H_WRITE, io)) return 0;
    }
    io->n   = io->len;  // a console, or no helper: block
    io->err = os_write(os, h, io->buf, io->len) ? 0 : E_PIPE;
    return 1;
}

// Start what waits for an object that became idle, or release it.
static void runqueue(Os *os, Wobj *w)
{
    while (w->queue && !busy(w)) {
        Io *io = w->queue;
        w->queue = io->next;
        os->nwait--;
        io->plt[0] = S_NONE;
        if (startio(os, w, io) && io->plt[0]!=S_ZERO) pushdone(os, io);
    }
    maybefree(os, w);
}

// A helper's operation ended (main thread).
static void helperdone(Os *os, Helper *hp)
{
    Wobj *w  = hp->obj;
    Io   *io = w->active;
    w->helper = 0;
    w->active = 0;
    if (io) os->nwait--;
    b32 reuse = 0;
    switch (hp->op) {
    case H_READ:
    case H_CONREAD:
        if (hp->n && !io && w->ip && !w->ip->started && w->ip->iw) {
            // An interposition waited for this read: its byte goes
            // through the pipe, then the pump takes over
            writeall(w->ip->iw, hp->buf, hp->n);
            hp->n = 0;
        }
        appendahead(w, hp->buf, hp->n);
        w->eof |= hp->eof;
        if (io) {
            readahead(w, w->own, io);
            io->err = !io->n && !hp->eof && hp->code ? E_INTR : 0;
            pushdone(os, io);
        }
        if (w->ip && !w->ip->started && w->ip->iw) {
            Interpose *ip = w->ip;
            ip->started = 1;
            hp->op     = H_PUMP;
            hp->iw     = ip->iw;
            hp->stop   = 0;
            hp->pstate = 0;
            ip->iw     = 0;
            w->helper  = hp;
            __atomic_store_n(&hp->live, 1, __ATOMIC_SEQ_CST);
            SetEvent(hp->go);
            reuse = 1;
        }
        break;
    case H_WRITE:
        io->n   = hp->n;
        io->err = !hp->code ? 0 :
                  hp->code==ERROR_NO_DATA || hp->code==ERROR_BROKEN_PIPE ? E_PIPE : E_IO;
        pushdone(os, io);
        break;
    case H_PUMP:
        appendahead(w, hp->buf, hp->n);  // read after the stop
        break;
    }
    if (!reuse) {
        hp->next = os->idle;
        os->idle = hp;
    }
    runqueue(os, w);
}

static void releaselends(Os *os, uz proc);
static i32  exitstatus(u32 code);

// A process's exit status, releasing its handle.
static i32 procexit(Os *os, uz proc)
{
    i32 code = 127;
    if (!GetExitCodeProcess(proc, &code)) code = 127;
    CloseHandle(proc);
    releaselends(os, proc);
    return exitstatus((u32)code);
}

static void __stdcall procfired(void *arg, u8)
{
    Pwait *pw = (Pwait *)arg;
    __atomic_store_n(&pw->fired, 1, __ATOMIC_SEQ_CST);
    wakepush(pw->os, &pw->node);
}

static void procwaited(Os *os, Pwait *pw)
{
    if (!pw->gone) UnregisterWaitEx(pw->wait, INVALID_HANDLE_VALUE);
    Io *io = pw->io;
    if (io) {
        os->nwait--;
        io->n = procexit(os, io->h);
        pushdone(os, io);
    }
    pw->next = os->freepwait;
    os->freepwait = pw;
}

// Act on everything the other threads pushed, oldest first.
static void drainwake(Os *os)
{
    Wnode *list = __atomic_exchange_n(&os->wakeq, (Wnode *)0, __ATOMIC_ACQUIRE);
    Wnode *fifo = 0;
    while (list) {
        Wnode *next = list->next;
        list->next = fifo;
        fifo = list;
        list = next;
    }
    while (fifo) {
        Wnode *n = fifo;
        fifo = n->next;
        switch (n->kind) {
        case WN_HELPER: helperdone(os, (Helper *)n); break;
        case WN_PROC:   procwaited(os, (Pwait *)n);  break;
        }
    }
}

// Wait until an object is idle, or a signal comes: false then.
static b32 waitidle(Os *os, Wobj *w)
{
    for (;;) {
        drainwake(os);
        if (!busy(w)) return 1;
        if (sigwaiting(os)) return 0;
        WaitForSingleObject(os->wake, INFINITE);
    }
}

// Read a pipe on the main thread, blocking, where Ctrl-C can cancel it
// (onctrl): -1 then, or on error.
static iz mainread(Os *os, uz h, u8 *buf, iz len)
{
    __atomic_add_fetch(&os->mainio, 1, __ATOMIC_SEQ_CST);  // odd
    if (sigwaiting(os)) {
        __atomic_add_fetch(&os->mainio, 1, __ATOMIC_SEQ_CST);
        return -1;
    }
    i32 n  = 0;
    b32 ok = ReadFile(h, buf, chunk(len), &n, 0);
    i32 code = ok ? 0 : GetLastError();
    __atomic_add_fetch(&os->mainio, 1, __ATOMIC_SEQ_CST);  // even
    if (ok) return n;
    return code==ERROR_BROKEN_PIPE ? 0 : -1;
}

// Read a console line into the read-ahead on the main thread: its
// length, 0 at end of file, -1 for Ctrl-C or an error.
static iz conread(Os *os, Wobj *w, uz h)
{
    reserveahead(w, 3*CONUNITS);
    i32 code = 0;
    iz  n    = conline(h, w->ahead+w->alen, &code);
    if (n<0 && code==ERROR_OPERATION_ABORTED) {
        // The control handler runs on a thread of its own, perhaps not
        // yet: give it a moment to note the signal
        for (i32 i = 0; i<100 && !sigwaiting(os); i++) Sleep(1);
    }
    if (n > 0) w->alen += n;
    return n;
}

// Drain an interposed pipe into the read-ahead.
static void drainpipe(Wobj *w, uz ir)
{
    for (;;) {
        reserveahead(w, 1<<16);
        iz n = peekread(ir, w->ahead+w->alen, w->acap-w->alen);
        if (!n) return;
        w->alen += n;
    }
}

// The children reading through an interposition are gone: stop the
// pump and drain what they left into the read-ahead.
static void stopinterpose(Os *os, Wobj *w)
{
    Interpose *ip = w->ip;
    Helper    *hp = w->helper && w->helper->op==H_PUMP ? w->helper : 0;
    if (hp) {
        __atomic_store_n(&hp->stop, 1, __ATOMIC_SEQ_CST);
        for (i32 i = 0;; i++) {
            drainpipe(w, ip->ir);  // lets a blocked write finish
            if (__atomic_load_n(&hp->pstate, __ATOMIC_SEQ_CST) != P_WRITING) break;
            Sleep(i<4 ? 0 : 1);
        }
        cancelhelper(os, hp);  // if blocked reading the object
    } else if (ip->iw) {
        CloseHandle(ip->iw);  // the read in flight is not forwarded
        ip->iw = 0;
    }
    drainpipe(w, ip->ir);
    CloseHandle(ip->ir);
    w->ip    = 0;
    ip->next = os->freeip;
    os->freeip = ip;
}

// A child is to inherit h. If the shell read bytes ahead the child
// must see first, or a read on it is in flight, the child gets an
// interposed pipe instead (the returned handle). Otherwise the child
// holds the object directly (lent). *out is the object, if any.
static uz lend(Os *os, uz h, Wobj **out, b32 *viaip)
{
    *out   = 0;
    *viaip = 0;
    Wobj *w = lookup(os, h);
    if (!w || (w->kind!=K_PIPE && w->kind!=K_CONIN)) return h;
    drainwake(os);
    *out = w;
    if (!w->ip && w->helper && w->helper->op!=H_WRITE && !w->active && os->cancelsync) {
        // An abandoned read: end it if possible, so that the child can
        // have the object itself
        Helper *hp = w->helper;
        cancelhelper(os, hp);
        for (i32 i = 0; i<20 && __atomic_load_n(&hp->live, __ATOMIC_SEQ_CST); i++) {
            Sleep(1);
        }
        drainwake(os);
    }
    b32 reading = w->helper && w->helper->op!=H_WRITE;
    if (!w->ip && !reading && w->alen==w->aoff && !w->eof) {
        return h;
    }
    if (!w->ip) {
        uz ir = 0;
        uz iw = 0;
        iz pending = w->alen - w->aoff;
        if (!CreatePipe(&ir, &iw, 0, (i32)(pending + (1<<16)))) {
            return h;  // as good as it gets
        }
        writeall(iw, w->ahead+w->aoff, pending);  // fits the pipe
        w->aoff = w->alen = 0;
        Interpose *ip = os->freeip;
        if (ip) {
            os->freeip = ip->next;
            *ip = {};
        } else {
            ip = alloc<Interpose>(&os->perm);
        }
        ip->ir = ir;
        ip->iw = iw;
        w->ip  = ip;
        if (w->eof) {
            // The children see the end of file the shell would have
            w->eof = 0;
            CloseHandle(iw);
            ip->iw = 0;
            ip->started = 1;
        } else if (!reading) {
            if (issue(os, w, h, H_PUMP, 0, iw)) {
                ip->iw = 0;
                ip->started = 1;
            }
        }
        // else the pump starts once the read in flight ends (helperdone)
    }
    w->ip->children++;
    *viaip = 1;
    return w->ip->ir;
}

static void addlend(Os *os, uz proc, Wobj *w, b32 viaip)
{
    Lend *l = os->freelend;
    if (l) {
        os->freelend = l->next;
    } else {
        l = alloc<Lend>(&os->perm);
    }
    l->proc = proc;
    l->obj  = w;
    l->ip   = viaip;
    l->next = os->lends;
    os->lends = l;
    w->lent += !viaip;
}

static void droplend(Os *os, Lend *l)
{
    Wobj *w = l->obj;
    if (!l->ip) {
        w->lent--;
    } else if (!--w->ip->children) {
        stopinterpose(os, w);
    }
    l->next = os->freelend;
    os->freelend = l;
    runqueue(os, w);
}

// A child exited (its handle, now closed): its holds end.
static void releaselends(Os *os, uz proc)
{
    for (Lend **p = &os->lends; *p;) {
        Lend *l = *p;
        if (l->proc == proc) {
            *p = l->next;
            droplend(os, l);
        } else {
            p = &l->next;
        }
    }
}

// Console control events, on a thread the system creates: Ctrl-C is
// SIGINT and Ctrl-Break SIGQUIT for every level of the shell (the
// whole console group gets them), closing the console SIGHUP. A main
// thread blocked in a pipe read is woken with CancelSynchronousIo.
static Os *ctrlos;

static b32 __stdcall onctrl(u32 type)
{
    Os *os = ctrlos;
    i32 sig = 0;
    switch (type) {
    case 0:  sig = 2; break;  // CTRL_C_EVENT
    case 1:  sig = 3; break;  // CTRL_BREAK_EVENT
    case 2:  sig = 1; break;  // CTRL_CLOSE_EVENT
    default: return 0;
    }
    __atomic_fetch_or(&os->siggrp, (u64)1<<(sig-1), __ATOMIC_SEQ_CST);
    SetEvent(os->wake);
    i32 t = __atomic_load_n(&os->mainio, __ATOMIC_SEQ_CST);
    if ((t&1) && os->cancelsync) {
        for (i32 i = 0; i<1000 && __atomic_load_n(&os->mainio, __ATOMIC_SEQ_CST)==t; i++) {
            if (os->cancelsync(os->mainthread)) break;
            Sleep(1);
        }
    }
    if (type == 2) {
        Sleep(10000);  // the shell exits meanwhile, else the system ends it
    }
    return 1;
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

static b32 os_write(Os *os, Handle h, u8 *buf, iz len)
{
    Wobj *w    = lookup(os, h);
    i32   mode = 0;
    if (w ? w->kind!=K_CONOUT : !GetConsoleMode(h, &mode)) {
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

static iz rawread(uz h, u8 *buf, iz len)
{
    i32 n = 0;
    if (!ReadFile(h, buf, chunk(len), &n, 0)) {
        return GetLastError()==ERROR_BROKEN_PIPE ? 0 : -1;
    }
    return n;
}

// Pipes are read exactly: whatever was read ahead, then what the pipe
// holds, at most len bytes. With nothing there, the main thread blocks
// where Ctrl-C can cancel the read (Vista and later), else a helper
// waits for a byte while the main thread waits for it or a signal.
// Consoles are read a line at a time, as UTF-8 regardless of code page
// with line endings as LF; Ctrl-Z at the start of a line is end of
// file, as with ReadFile, and Ctrl-C ends the read (-1).
static iz os_read(Os *os, Handle h, u8 *buf, iz len)
{
    Wobj *w = lookup(os, h);
    i32   mode = 0;
    if (!w && GetConsoleMode(h, &mode)) w = track(os, h);
    if (!w || w->kind==K_CONOUT || w->kind==K_CHAR) {
        return rawread(h, buf, len);
    }
    drainwake(os);
    for (;;) {
        iz n = takeahead(w, buf, len);
        if (n) {
            if (w->kind==K_PIPE && !busy(w) && !w->lent) n += peekread(h, buf+n, len-n);
            return n;
        }
        if (w->eof) {
            w->eof = 0;
            return 0;
        }
        if (!len) return 0;
        if (busy(w) || w->lent) {
            // Its handle is off limits: a helper reads for us
            i32 op = w->kind==K_CONIN ? H_CONREAD : H_READ;
            if (!busy(w) && !issue(os, w, h, op, 0)) {
                return w->kind==K_PIPE ? rawread(h, buf, len) : conread(os, w, h);
            }
            if (!waitidle(os, w)) return -1;
            continue;
        }
        if (sigwaiting(os)) return -1;
        if (w->kind == K_CONIN) {
            n = conread(os, w, h);
            if (n <= 0) return n;
            continue;
        }
        i32 avail = 0;
        if (!PeekNamedPipe(h, 0, 0, 0, &avail, 0)) {
            return GetLastError()==ERROR_BROKEN_PIPE ? 0 : -1;
        }
        if (avail) return rawread(h, buf, avail<len ? avail : len);
        if (os->cancelsync) return mainread(os, h, buf, len);
        if (!issue(os, w, h, H_READ, 0)) return rawread(h, buf, len);
        if (!waitidle(os, w)) return -1;
    }
}

static iz os_peek(Os *os, Handle h, u8 *buf, iz len)
{
    Wobj *w = lookup(os, h);
    if (!w || (w->kind!=K_PIPE && w->kind!=K_CONIN)) return -1;
    drainwake(os);
    iz n = w->alen - w->aoff;
    n = n<len ? n : len;
    copybytes(buf, w->ahead+w->aoff, n);
    if (w->kind==K_PIPE && !busy(w) && !w->lent && !w->eof && n<len) {
        i32 got   = 0;
        i32 avail = 0;
        if (PeekNamedPipe(h, buf+n, chunk(len-n), &got, &avail, 0)) n += got;
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

static Opened os_open(Os *os, Arena scratch, Str path, i32 flags, i32 mode)
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
    track(os, h);
    r.h = h;
    return r;
}

// Never the last handle of an object in an operation: helpers use a
// private duplicate (Wobj.own), released once the object is unused.
static void os_close(Os *os, Handle h)
{
    if (h == BADHANDLE) return;
    Wobj *w = unreg(os, h);
    if (w) {
        drainwake(os);
        w->closing |= !w->refs;
        maybefree(os, w);
    }
    CloseHandle(h);
}

static Handle os_dup(Os *os, Handle h)
{
    uz self = GetCurrentProcess();
    uz r    = 0;
    if (!DuplicateHandle(self, h, self, &r, 0, 0, DUPLICATE_SAME_ACCESS)) {
        return BADHANDLE;
    }
    Wobj *w = lookup(os, h);
    if (w) {
        reg(os, r, w);  // the same file object
    } else {
        track(os, r);
    }
    return r;
}

static iz os_pipecap(Os *)
{
    return 1<<15;  // CreatePipe is asked for 64 KiB; stay well under
}

static OsPipe os_pipe(Os *os)
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
    reg(os, rd, newobj(os, K_PIPE));
    reg(os, wr, newobj(os, K_PIPE));
    r.r = rd;
    r.w = wr;
    return r;
}

static i64 os_seek(Os *os, Handle h, i64 off, i32 whence)
{
    i64 pos = 0;
    if (lookup(os, h) || GetFileType(h)!=FILE_TYPE_DISK) {
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

static Stat os_fstat(Os *os, Handle h)
{
    // Registered objects are described without touching the handle,
    // which an operation in flight may hold
    Wobj *w = lookup(os, h);
    if (!w) return handlestat(h);
    Stat r  = {};
    r.type  = w->kind==K_PIPE ? FT_PIPE : FT_CHAR;
    r.mode  = w->kind==K_PIPE ? 0600 : 0666;
    r.nlink = 1;
    r.uid   = 1000;
    r.gid   = 1000;
    return r;
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

static Spawned os_spawn(Os *os, Arena scratch, Spawn *s)
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
    // creation. Descriptors above 2 are not passed (yet). A pipe or
    // console the child gets is lent to it (see Wobj), perhaps through
    // an interposed pipe.
    uz    self    = GetCurrentProcess();
    uz    std[3]  = {};
    Wobj *objs[3] = {};
    b32   viaip[3] = {};
    for (iz i = 0; i < s->nfds; i++) {
        i32 fd = s->fds[i].fd;
        uz  h  = s->fds[i].h;
        if (fd<0 || fd>2 || h==BADHANDLE || std[fd]) continue;
        h = lend(os, h, objs+fd, viaip+fd);
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
    // trap '' INT in the spawning subshell: the child inherits the
    // ignoring of Ctrl-C, which is set only around its creation so
    // that the shell itself is unaffected
    b32 noint = s->sigign>>1 & 1;
    if (noint) SetConsoleCtrlHandler(0, 1);
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
    if (noint) SetConsoleCtrlHandler(0, 0);
    for (i32 i = 0; i < 3; i++) {
        if (std[i]) CloseHandle(std[i]);
        if (!objs[i]) continue;
        addlend(os, ok ? pi.process : 0, objs[i], viaip[i]);
    }
    if (!ok) {
        releaselends(os, 0);
        r.err = errnum(code);
        return r;
    }
    CloseHandle(pi.thread);
    r.proc = pi.process;
    r.pid  = (i64)pi.pid;
    return r;
}

// Native crashes become the matching signal statuses, as in Cygwin.
static i32 exitstatus(u32 code)
{
    switch (code) {
    case 0xc0000005u: return WSIGNALED | (128 + 11);  // access violation: SIGSEGV
    case 0xc00000fdu: return WSIGNALED | (128 + 11);  // stack overflow
    case 0xc000001du: return WSIGNALED | (128 + 4);   // illegal instruction: SIGILL
    case 0xc0000094u: return WSIGNALED | (128 + 8);   // divide by zero: SIGFPE
    case 0xc000013au: return WSIGNALED | (128 + 2);   // Ctrl-C: SIGINT
    case 0xc0000409u: return WSIGNALED | (128 + 6);   // fast fail: SIGABRT
    }
    return (i32)(code & 0xff);
}

// Signals (M3a's minimum; the signal channel to our own shells is
// M5b): a signal ends another process outright, with exit code 128+N.
// Nothing arrives through os_sigpending yet.
static i32 os_kill(Os *, i64 pid, i32 sig)
{
    if (pid<=0 || pid>(i64)0x7fffffff) return E_SRCH;
    if (sig<0 || sig>=NSIGNAL) return E_INVAL;
    i32 access = sig ? 0x0001 : 0x100000;  // PROCESS_TERMINATE, SYNCHRONIZE
    uz  h = OpenProcess(access, 0, (i32)pid);
    if (!h) {
        i32 code = GetLastError();
        return code==5 ? E_PERM : E_SRCH;  // ERROR_ACCESS_DENIED
    }
    b32 ok = 1;
    switch (sig) {
    case 0: case 17: case 18: case 19: case 20: case 21: case 22: case 23: case 28:
        break;  // existence check, or ignored by default
    default:
        ok = TerminateProcess(h, (u32)(128 + sig));
    }
    CloseHandle(h);
    return ok ? 0 : E_PERM;
}

static u64 os_sigpending(Os *os)
{
    return __atomic_exchange_n(&os->sigown, (u64)0, __ATOMIC_SEQ_CST);
}

static u64 os_siggroup(Os *os)
{
    return __atomic_exchange_n(&os->siggrp, (u64)0, __ATOMIC_SEQ_CST);
}

static void os_sigmode(Os *, i32, i32)
{
}

static void os_tasktag(Os *, u64, iz)
{
}

static void os_racy(Os *)
{
}

static i32 os_exec(Os *, Arena, Spawn *)
{
    return E_NOSYS;  // no exec on Windows: start a child instead
}

// Waits on the process and the wake queue together, so that helpers
// and interpositions keep going meanwhile. Signals do not end it.
static i32 os_waitproc(Os *os, Handle proc)
{
    for (;;) {
        uz  hs[2] = {proc, os->wake};
        i32 r = WaitForMultipleObjects(2, hs, 0, INFINITE);
        if (r != 1) break;
        drainwake(os);
    }
    return procexit(os, proc);
}

// Asynchronous operations (M3b): process exits through thread pool
// waits (RegisterWaitForSingleObject, no 64-handle limit), reads and
// writes on pipes and consoles through their Wobj (see above). Files
// and other devices complete at once.

static void os_submit(Os *os, Io *io)
{
    io->done   = 0;
    io->next   = 0;
    io->plt[0] = S_NONE;
    io->plt[1] = 0;
    drainwake(os);
    if (io->op == IO_PROC) {
        if (!WaitForSingleObject(io->h, 0)) {
            io->n    = procexit(os, io->h);
            io->done = 1;
            return;
        }
        Pwait *pw = os->freepwait;
        if (pw) {
            os->freepwait = pw->next;
        } else {
            pw = alloc<Pwait>(&os->perm);
        }
        pw->node.kind = WN_PROC;
        pw->os    = os;
        pw->io    = io;
        pw->fired = 0;
        pw->gone  = 0;
        io->plt[0] = S_PROC;
        io->plt[1] = (uz)pw;
        os->nwait++;
        // WT_EXECUTEONLYONCE|WT_EXECUTEINWAITTHREAD
        if (!RegisterWaitForSingleObject(&pw->wait, io->h, procfired, pw, INFINITE, 0xc)) {
            os->nwait--;
            pw->next = os->freepwait;
            os->freepwait = pw;
            io->n    = os_waitproc(os, io->h);  // block here
            io->done = 1;
        }
        return;
    }

    Wobj *w = lookup(os, io->h);
    if (!w || w->kind==K_CHAR || (w->kind==K_CONOUT) != (io->op==IO_WRITE)) {
        // A file or device: at once
        if (io->op == IO_READ) {
            io->n   = os_read(os, io->h, io->buf, io->len);
            io->err = io->n<0 ? E_IO : 0;
            io->n   = io->n<0 ? 0 : io->n;
        } else {
            io->n   = io->len;
            io->err = os_write(os, io->h, io->buf, io->len) ? 0 : E_IO;
        }
        io->done = 1;
        return;
    }
    if (busy(w) || w->queue) {
        Io **tail = &w->queue;
        while (*tail) tail = &(*tail)->next;
        *tail = io;
        io->plt[0] = S_QUEUED;
        io->plt[1] = (uz)w;
        os->nwait++;
        return;
    }
    if (startio(os, w, io)) io->done = 1;
}

static Io *os_wait(Os *os, i64 deadline)
{
    for (;;) {
        drainwake(os);
        Io *io = os->done;
        if (io) {
            os->done = io->next;
            if (!os->done) os->donetail = &os->done;
            io->next   = 0;
            io->plt[0] = S_NONE;
            io->done   = 1;
            return io;
        }
        if (sigwaiting(os) || deadline==WAIT_POLL) return 0;
        i32 timeout = INFINITE;
        if (deadline > 0) {
            i64 left = (deadline - os_now(os) + 999999) / 1000000;
            timeout = left<0 ? 0 : left>(1<<30) ? (1<<30) : (i32)left;
        } else if (!os->nwait) {
            return 0;  // nothing can complete
        }
        if (WaitForSingleObject(os->wake, timeout)) {
            drainwake(os);
            if (!os->done) return 0;  // the deadline
        }
    }
}

static b32 os_cancel(Os *os, Io *io)
{
    switch (io->plt[0]) {
    case S_QUEUED: {
        Wobj *w = (Wobj *)io->plt[1];
        for (Io **p = &w->queue; *p; p = &(*p)->next) {
            if (*p == io) {
                *p = io->next;
                break;
            }
        }
        os->nwait--;
        io->plt[0] = S_NONE;
        maybefree(os, w);
        return 1;
    }
    case S_ZERO:
        for (Io **p = &os->zero; *p; p = &(*p)->next) {
            if (*p == io) {
                *p = io->next;
                break;
            }
        }
        io->plt[0] = S_NONE;
        return 1;
    case S_ACTIVE: {
        Wobj *w = (Wobj *)io->plt[1];
        if (io->op != IO_READ) return 0;  // the core's bytes are in use
        // Abandoned: what the helper reads goes to the read-ahead
        w->active = 0;
        os->nwait--;
        io->plt[0] = S_NONE;
        cancelhelper(os, w->helper);
        return 1;
    }
    case S_PROC: {
        Pwait *pw = (Pwait *)io->plt[1];
        UnregisterWaitEx(pw->wait, INVALID_HANDLE_VALUE);  // after any callback
        os->nwait--;
        io->plt[0] = S_NONE;
        if (__atomic_load_n(&pw->fired, __ATOMIC_SEQ_CST)) {
            pw->io   = 0;  // queued: procwaited finds nothing to do
            pw->gone = 1;
        } else {
            pw->next = os->freepwait;
            os->freepwait = pw;
        }
        return 1;
    }
    }
    return 0;  // completed, or completing
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
    os.cancelsync = (b32 (__stdcall *)(uz))dynamic(u"kernel32.dll", "CancelSynchronousIo");
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
    if (envget(env.data, env.len, "unabashed_xpio").data) {
        os.cancelsync = 0;  // for tests: the XP code paths
    }

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
    if (ctrlcignored()) info.sigign |= (u64)1 << (2-1);
    info.stack = stackreserve();

    // A shell killed by SIGINT exits as Ctrl-C ends a native program,
    // which Cygwin's processes also take for death by SIGINT
    i32 status = sh_main(&os, &info);
    ExitProcess(status==(WSIGNALED|(128+2)) ? 0xc000013a : status&255);
}
