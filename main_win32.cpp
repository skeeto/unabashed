// Win32 platform layer: no CRT, no headers, runs on Windows XP and later
// $ g++ -std=c++20 -O2 -s -nostartfiles -static-libgcc
//       -o unabashed.exe main_win32.cpp
// The i686 build also takes -Wl,--large-address-aware (4 GiB on x64
// Windows) and subsystem version 5.01 for XP: see the Makefile.
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
#include "src/unabashed.cpp"

enum : i32 {
    CREATE_BREAKAWAY_FROM_JOB    = 0x01000000,
    CREATE_NEW                   = 1,
    CREATE_NO_WINDOW             = 0x08000000,
    CREATE_SUSPENDED             = 4,
    CREATE_UNICODE_ENVIRONMENT   = 0x400,
    DUPLICATE_SAME_ACCESS        = 2,
    ERROR_ACCESS_DENIED          = 5,
    ERROR_BROKEN_PIPE            = 109,
    ERROR_DIRECTORY              = 267,
    ERROR_FILE_NOT_FOUND         = 2,
    ERROR_INVALID_PARAMETER      = 87,
    ERROR_MORE_DATA              = 234,
    ERROR_NO_DATA                = 232,
    ERROR_OPERATION_ABORTED      = 995,
    ERROR_PATH_NOT_FOUND         = 3,
    ERROR_PIPE_CONNECTED         = 535,
    FILE_APPEND_DATA             = 4,
    FILE_ATTRIBUTE_DIRECTORY     = 0x10,
    FILE_ATTRIBUTE_NORMAL        = 0x80,
    FILE_ATTRIBUTE_READONLY      = 1,
    FILE_ATTRIBUTE_REPARSE_POINT = 0x400,
    FILE_ATTRIBUTE_TEMPORARY     = 0x100,
    FILE_FLAG_DELETE_ON_CLOSE    = 0x04000000,
    FILE_FLAG_FIRST_PIPE_INSTANCE = 0x00080000,
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
    JOB_OBJECT_LIMIT_BREAKAWAY_OK = 0x800,
    JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x2000,
    JOB_OBJECT_MSG_NEW_PROCESS   = 6,
    JOB_OBJECT_MSG_EXIT_PROCESS  = 7,  // an exit status not an exception's
    JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS = 8,  // 0xC000013A among them
    JobObjectAssociateCompletionPortInformation = 7,
    JobObjectBasicAccountingInformation = 1,
    JobObjectBasicProcessIdList  = 3,
    JobObjectExtendedLimitInformation = 9,
    MEM_COMMIT                   = 0x1000,
    MEM_DECOMMIT                 = 0x4000,
    MEM_RESERVE                  = 0x2000,
    MEM_PRIVATE                  = 0x20000,
    MEM_RELEASE                  = 0x8000,
    OPEN_ALWAYS                  = 4,
    OPEN_EXISTING                = 3,
    PAGE_NOACCESS                = 1,
    PAGE_READWRITE               = 4,
    FILE_MAP_WRITE               = 2,
    EVENT_MODIFY_STATE           = 2,
    PIPE_ACCESS_INBOUND          = 1,
    PIPE_ACCESS_OUTBOUND         = 2,
    PIPE_REJECT_REMOTE_CLIENTS   = 8,
    PIPE_UNLIMITED_INSTANCES     = 255,
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

struct InputRecord {  // INPUT_RECORD, read as a key event
    short type;       // 1 KEY_EVENT, 4 WINDOW_BUFFER_SIZE_EVENT
    short pad;
    i32   down;
    short repeat;
    short vk;
    short scan;
    c16   ch;
    i32   state;
};

struct ConsoleInfo {  // CONSOLE_SCREEN_BUFFER_INFO
    short width;
    short height;
    short x;
    short y;
    short attr;
    short left;
    short top;
    short right;
    short bottom;
    short maxwidth;
    short maxheight;
};

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
static_assert(sizeof(JobLimits) == (sizeof(void *)==8 ? 144 : 112));

struct JobPort {  // JOBOBJECT_ASSOCIATE_COMPLETION_PORT
    uz key;
    uz port;
};

struct JobPids {  // JOBOBJECT_BASIC_PROCESS_ID_LIST
    u32 assigned;
    u32 listed;
    uz  pids[64];
};

struct JobCounts {  // JOBOBJECT_BASIC_ACCOUNTING_INFORMATION
    i64 utime, ktime, putime, pktime;
    u32 faults;
    u32 total;   // processes ever in the job
    u32 active;
    u32 terminated;
};
static_assert(sizeof(JobCounts) == 48);

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

struct MemInfo {  // MEMORY_BASIC_INFORMATION
    byte *base;
    byte *allocbase;
    i32   allocprotect;
    uz    size;
    i32   state;
    i32   protect;
    i32   type;
};

struct SysTime {  // SYSTEMTIME, WORDs
    c16 year;
    c16 month;
    c16 dow;
    c16 day;
    c16 hour;
    c16 minute;
    c16 second;
    c16 ms;
};

// TIME_ZONE_INFORMATION, and the DYNAMIC_ form's extra fields
struct TzInfo {
    i32     bias;
    c16     stdname[32];
    SysTime stddate;
    i32     stdbias;
    c16     dstname[32];
    SysTime dstdate;
    i32     dstbias;
    c16     key[128];
    u8      nodst;
};
static_assert(sizeof(TzInfo) == 432);

#define W32(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
W32(b32,    AssignProcessToJobObject(uz, uz));
W32(b32,    CloseHandle(uz));
W32(uz,     CreateIoCompletionPort(uz, uz, uz, i32));
W32(uz,     CreateJobObjectW(uz, c16 *));
W32(uz,     CreateEventW(uz, b32, b32, c16 *));
W32(uz,     CreateFileMappingW(uz, uz, i32, i32, i32, c16 *));
W32(uz,     OpenFileMappingW(i32, b32, c16 *));
W32(void *, MapViewOfFile(uz, i32, i32, i32, iz));
W32(b32,    UnmapViewOfFile(void *));
W32(uz,     OpenEventW(i32, b32, c16 *));
W32(uz,     CreateThread(uz, uz, u32 (__stdcall *)(void *), void *, i32, i32 *));
W32(uz,     CreateFileW(c16 *, i32, i32, uz, i32, i32, uz));
W32(uz,     CreateNamedPipeW(c16 *, i32, i32, i32, i32, i32, i32, uz));
W32(b32,    ConnectNamedPipe(uz, uz));
W32(b32,    CreatePipe(uz *, uz *, uz, i32));
W32(b32,    DeleteFileW(c16 *));
W32(b32,    MoveFileExW(c16 *, c16 *, i32));
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
W32(b32,    GetConsoleScreenBufferInfo(uz, ConsoleInfo *));
W32(i32,    GetCurrentDirectoryW(i32, c16 *));
W32(uz,     GetCurrentProcess());
W32(uz,     GetCurrentThread());
W32(b32,    GetNumberOfConsoleInputEvents(uz, i32 *));
W32(i32,    GetCurrentProcessId());
W32(uz,     LoadLibraryW(c16 const *));
W32(uz,     GlobalAlloc(i32, iz));
W32(uz,     GlobalFree(uz));
W32(void *, GlobalLock(uz));
W32(iz,     GlobalSize(uz));
W32(b32,    GlobalUnlock(uz));
W32(i32,    GetTempPathW(i32, c16 *));
W32(c16 *,  GetEnvironmentStringsW());
W32(void,   GetStartupInfoW(StartupInfo *));
W32(b32,    GetExitCodeProcess(uz, i32 *));
W32(b32,    GetFileAttributesExW(c16 *, i32, FileAttributes *));
W32(i32,    GetFileAttributesW(c16 *));
W32(b32,    GetFileInformationByHandle(uz, FileInfo *));
W32(i32,    GetFileType(uz));
W32(u32,    GetTickCount());
W32(i32,    GetFullPathNameW(c16 *, i32, c16 *, c16 **));
W32(i32,    GetLastError());
W32(i32,    GetModuleFileNameW(uz, c16 *, i32));
W32(uz,     GetModuleHandleW(c16 const *));
W32(void *, GetProcAddress(uz, char const *));
W32(uz,     GetStdHandle(i32));
W32(i32,    GetSystemWindowsDirectoryW(c16 *, i32));
W32(void,   GetSystemTimeAsFileTime(Filetime *));
W32(i32,    GetTimeZoneInformation(TzInfo *));
W32(b32,    ReadConsoleW(uz, c16 *, i32, i32 *, uz));
W32(b32,    ReadConsoleInputW(uz, InputRecord *, i32, i32 *));
W32(i32,    GetConsoleProcessList(i32 *, i32));
W32(b32,    GetProcessTimes(uz, Filetime *, Filetime *, Filetime *, Filetime *));
W32(b32,    GetQueuedCompletionStatus(uz, i32 *, uz *, void **, i32));
W32(b32,    GetThreadTimes(uz, Filetime *, Filetime *, Filetime *, Filetime *));
W32(b32,    QueryPerformanceCounter(i64 *));
W32(b32,    QueryPerformanceFrequency(i64 *));
W32(b32,    QueryInformationJobObject(uz, i32, void *, i32, i32 *));
W32(b32,    ReadProcessMemory(uz, void *, void *, iz, iz *));
W32(b32,    PeekNamedPipe(uz, u8 *, i32, i32 *, i32 *, i32 *));
W32(b32,    ReadFile(uz, u8 *, i32, i32 *, uz));
W32(b32,    RegisterWaitForSingleObject(uz *, uz, void (__stdcall *)(void *, u8),
                                        void *, i32, i32));
W32(i32,    ResumeThread(uz));
W32(void,   SetLastError(i32));
W32(b32,    SetInformationJobObject(uz, i32, void *, i32));
W32(b32,    UnregisterWaitEx(uz, uz));
W32(b32,    SetEndOfFile(uz));
W32(b32,    SetFilePointerEx(uz, i64, i64 *, i32));
W32(b32,    SetHandleInformation(uz, i32, i32));
W32(uz,     OpenProcess(i32, b32, i32));
W32(b32,    TerminateProcess(uz, u32));
W32(b32,    TerminateJobObject(uz, u32));
W32(b32,    SetConsoleCtrlHandler(void *, b32));
W32(b32,    SetConsoleMode(uz, i32));
W32(b32,    SetConsoleTitleW(c16 *));
W32(byte *, VirtualAlloc(byte *, iz, i32, i32));
W32(byte *, VirtualAllocEx(uz, byte *, iz, i32, i32));
W32(b32,    VirtualFree(byte *, iz, i32));
W32(iz,     VirtualQuery(byte *, MemInfo *, iz));
W32(b32,    WriteProcessMemory(uz, void *, void *, iz, iz *));
W32(b32,    SetEvent(uz));
W32(void,   Sleep(i32));
W32(i32,    WaitForMultipleObjects(i32, uz *, b32, i32));
W32(i32,    WaitForSingleObject(uz, i32));
W32(b32,    WriteConsoleW(uz, c16 *, i32, i32 *, uz));
// A COORD passes as one 32-bit value: x low, y high
W32(b32,    SetConsoleCursorPosition(uz, i32));
W32(b32,    SetConsoleTextAttribute(uz, i32));
W32(b32,    FillConsoleOutputCharacterW(uz, i32, i32, i32, i32 *));
W32(b32,    FillConsoleOutputAttribute(uz, i32, i32, i32, i32 *));
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

#include "src/fibers.cpp"

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
    H_CONNECT,   // a named pipe's client (IO_CONNECT)
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
    b32        freed;    // on the free list (maybefree): never twice
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
    b32        pipeout;  // a named pipe's server: writes to its client
    b32        mine;     // a pipe's read end that this shell made
    i32        streak;   // reads since a child last took it (fillahead)
    c16        pname[64];  // a named pipe's server: its name, else empty
};

struct Hent {  // handle registration
    Hent *next;
    uz    h;
    Wobj *obj;
};

struct Recent {  // a program started lately (see judge)
    uz  proc;
    uz  job;        // its own job, holding its descendants (Windows 8)
    uz  key;        // the job's completion key, unique
    u64 at;         // watched: the Ctrl-C (creation times)
    i64 time;       // started, in QueryPerformanceCounter ticks
    i64 until;      // watched until, likewise
    i32 pid;
    i32 state;      // R_*, after a Ctrl-C
    i32 busy;       // handler threads judging it unlocked: kept till 0
    b32 bound;      // the job reports to Os.jobport
    b32 late;       // watched: a program it started exited by itself
    i32 unseen[4];  // watched: started, gone before it could be opened
};
enum : i32 { R_MISSED = 1, R_WATCHED };

struct Judging {  // one Ctrl-C's view (see judge)
    i32 pids[256];  // attached to the console, read after the event
    i32 npids;      // -1: unknown
    u64 at;         // the event reached this process (creation times)
    i64 now;        // likewise, in ticks
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
using NtQueryInformationProcessFn = i32 (__stdcall *)(uz, i32, void *, u32, u32 *);
using NtWow64ReadFn = i32 (__stdcall *)(uz, u64, void *, u64, u64 *);

// The signal channel (notes/m5b.md, step 6): every shell maps a
// section named for its pid, where another of our shells (32- or
// 64-bit alike) sets signals for it, and sets an event named likewise.
enum : u32 { CHANMAGIC = 0x75736967 };  // "usig"

struct ChanSig {
    i64 level;  // 0: the process itself
    i32 sig;
    i32 pad;
};

struct SigChan {
    u32     magic;
    u32     pid;
    u64     own;       // atomic: signals for the process
    i32     lock;      // atomic: spin lock over the queue
    i32     nlevels;   // atomic to look at
    ChanSig levels[32];
};

// A process os_kill ended, and the exit code it gave
struct Killed {
    i32 pid;
    u32 code;
};

// A background program's job (Windows 8), so that a signal sent to its
// "group" (os_kill with -pid) reaches what it started too, as one sent
// to a process group would: w64devkit's alias stubs and the busybox
// behind them.
struct BgJob {
    i32 pid;
    uz  job;
};

struct Os {
    Fibers fibers;
    void (__stdcall *now)(Filetime *);  // precise clock where available
    NtQueryInformationFileFn queryfile;  // ntdll, resolved at run time
    NtQueryInformationProcessFn queryproc;  // ntdll, likewise
    // A 32-bit build on 64-bit Windows (WOW64): ntdll's calls that read
    // a 64-bit process's memory, else null
    NtQueryInformationProcessFn queryproc64;
    NtWow64ReadFn               read64;
    b32 (__stdcall *iswow64)(uz, b32 *);  // IsWow64Process, XP SP2
    Str  exe;  // this program, for self-spawning (os_spawn with an image)
    b32  (__stdcall *cancelsync)(uz);    // CancelSynchronousIo, Vista
    // RtlGenRandom (XP), from advapi32 when entropy is first wanted, so
    // that shells that never ask do not load it
    u8   (__stdcall *genrandom)(void *, u32);
    b32  gentried;
    u64  entropyn;  // the fallback's counter
    // The clipboard functions, from user32 on first use, so that the
    // import table stays kernel32 and most shells never load it
    b32  (__stdcall *openclip)(uz);
    b32  (__stdcall *closeclip)();
    b32  (__stdcall *emptyclip)();
    uz   (__stdcall *getclip)(i32);
    uz   (__stdcall *setclip)(i32, uz);
    b32  cliptried;
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
    Rlimit     rl[RL_COUNT];  // os_getrlimit: modelled, as Cygwin's
    b32        rlinit;
    SigChan   *chan;       // this shell's signal channel, or null
    uz         chanwait;   // its registered wait
    u64        siggrp;     // atomic: signals for the console group
    uz         mainthread;
    i32        mainio;     // atomic: odd while the main thread blocks in
                           // a read Ctrl-C may cancel
    i32        nctrl;      // atomic: Ctrl-C events so far
    i32        nctrlseen;  // nctrl when the core last took siggrp
    u64        ctrlat;     // atomic: the last Ctrl-C's time (FILETIME)
    i32        recentlock; // atomic: guards recent
    b32        lateattach; // Windows 8: attach after CreateProcess
    b32        xp;         // Windows XP: 32767-character environments
    b32        noconsole;  // children get CREATE_NO_WINDOW
    i64        freq;       // QueryPerformanceCounter ticks per second
    i64        youngticks; // 100 ms: programs judged at a Ctrl-C
    Recent     recent[32];
    Killed     killed[16]; // os_kill's exit codes, for XP (procexit)
    BgJob      bgjobs[64]; // jobs of recent background programs (ring)
    i32        nbgjobs;
    i32        nkilled;
    uz         jobport;    // job messages for watched programs
    b32        ttyraw;     // os_tty: the console's modes were changed
    i32        ttykeys;    // atomic: reads give keys (TK_*), not lines
    i32        ttyinmode;  // and its modes before
    i32        ttyoutmode;
    uz         jobkeys;    // completion keys handed out
    u32        jobflags;   // limits of each program's job (see newjob)
    c16        self[512];  // this program's image path (see sameimage)
    i32        selflen;
    // Output to a regular file is coalesced (see os_write)
    uz         wpend;      // the file's handle, while wlen > 0
    i32        wlen;
    u32        wtick;      // GetTickCount when the buffer began
    uz         werrh;      // a flush to this handle failed
    u8         wbuf[1<<14];
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

static Os *ctrlos;  // the one Os, for the console handler and os_oom

// Write out the coalesced output. A failure shows at the handle's next
// write. Every platform call but os_write to that same file does this
// first, so whatever can observe the file (a program, a stat, another
// handle, a reader of a pipe waiting for a later write) sees it, and so
// do exits.
static void flushw(Os *os)
{
    if (!os->wlen) return;
    if (!writeall(os->wpend, os->wbuf, os->wlen)) os->werrh = os->wpend;
    os->wlen = 0;
}

[[noreturn]] static void os_oom()
{
    if (ctrlos) flushw(ctrlos);
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
    c16 *s = rel.data;
    if (rel.len>=4 && s[0]=='\\' && s[1]=='\\' && s[2]=='?' && s[3]=='\\') {
        return rel;  // GetFullPathNameW would strip a trailing dot
    }
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
    SigChan *c = os->chan;
    return (__atomic_load_n(&os->sigown, __ATOMIC_SEQ_CST) |
            __atomic_load_n(&os->siggrp, __ATOMIC_SEQ_CST)) != 0 ||
           (c && (__atomic_load_n(&c->own, __ATOMIC_SEQ_CST) ||
                  __atomic_load_n(&c->nlevels, __ATOMIC_SEQ_CST)));
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
    if (w->refs || busy(w) || w->lent || w->queue || w->freed) return;
    if (w->own) CloseHandle(w->own);
    w->own   = 0;
    w->freed = 1;
    w->next  = os->freeobj;
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

// Read a block ahead from a pipe this shell made, once the shell has
// read it some while with no child taking it. Bytes read ahead are
// still the shell's to hand on: a child that takes the pipe gets an
// interposed one, holding them first (lend), which is why a loop that
// starts a program on the pipe each time keeps reading it exactly.
// An inherited pipe may have other readers, so is never read ahead.
enum : i32 { AHEADSTREAK = 16, AHEADBLOCK = 1<<14 };

static void fillahead(Wobj *w, uz h)
{
    if (!w->mine || w->streak<AHEADSTREAK || w->alen>w->aoff || w->eof ||
            w->helper || w->lent || w->ip) {
        return;
    }
    i32 avail = 0;
    if (!PeekNamedPipe(h, 0, 0, 0, &avail, 0) || !avail) return;
    reserveahead(w, AHEADBLOCK);
    i32 n = 0;
    if (ReadFile(h, w->ahead+w->alen, chunk(avail<AHEADBLOCK ? avail : AHEADBLOCK), &n, 0)) {
        w->alen += n;
    }
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

// Os.ttykeys: how console input is read. TK_SILENT and TK_CHARS take
// Ctrl-C as a key (unprocessed input): a console read without line
// input is not ended by it, so the read would go on and take the next
// command's keys.
enum : i32 { TK_LINES, TK_VT, TK_EVENTS, TK_SILENT, TK_CHARS };

static void wakemain(Os *);
static b32 __stdcall onctrl(u32);

// Keys from a console in the editor's raw mode, as xterm bytes into
// out (3*CONUNITS bytes): with VT input the console's own characters,
// else key events turned into them (conkey). A resize raises SIGWINCH
// and the read goes on. -1 on error with *code set.
static iz conkeys(Os *os, uz h, u8 *out, i32 *code, b32 vtin)
{
    InputRecord recs[64];
    i32 hi = 0;
    for (;;) {
        i32 n = 0;
        if (!ReadConsoleInputW(h, recs, countof(recs), &n)) {
            *code = GetLastError();
            return -1;
        }
        iz len = 0;
        for (i32 i = 0; i < n; i++) {
            InputRecord *r = recs + i;
            if (r->type == 4) {
                __atomic_fetch_or(&os->sigown, (u64)1<<27, __ATOMIC_SEQ_CST);
                wakemain(os);
                continue;
            }
            if (r->type != 1) continue;
            ConKey k = {};
            k.down  = !!r->down;
            k.vk    = r->vk & 0xffff;
            k.ch    = r->ch;
            k.state = r->state;
            u8  b[16];
            i32 m   = vtin ? conchar(k, &hi, b) : conkey(k, &hi, b);
            i32 rep = r->repeat<1 ? 1 : r->repeat>4 ? 4 : r->repeat;
            for (i32 j = 0; m && j<rep; j++) {
                copybytes(out+len, b, m);
                len += m;
            }
        }
        if (len) {
            *code = 0;
            return len;
        }
    }
}

// Read a console line into out (3*CONUNITS bytes): the byte count, 0
// at end of file, -1 on error with *code set (ERROR_OPERATION_ABORTED
// for Ctrl-C, which ends the read with nothing).
static iz conline(Os *os, uz h, u8 *out, i32 *code)
{
    i32 keys = __atomic_load_n(&os->ttykeys, __ATOMIC_SEQ_CST);
    if (keys==TK_VT || keys==TK_EVENTS) {
        return conkeys(os, h, out, code, keys==TK_VT);
    }
    c16 tmp[CONUNITS];
    i32 n = 0;
    if (keys == TK_SILENT) {
        // A line not echoed (read -s): the console's line input writes
        // a newline at Enter even without echo, which a terminal does
        // not, so the line is gathered here a character at a time,
        // erasing with Backspace
        for (;;) {
            c16 c = 0;
            i32 got = 0;
            SetLastError(0);
            b32 ok = ReadConsoleW(h, &c, 1, &got, 0);
            *code = GetLastError();
            if (!ok || (!got && *code==ERROR_OPERATION_ABORTED)) {
                *code = ok ? ERROR_OPERATION_ABORTED : *code;
                return -1;
            }
            if (!got) continue;
            if (c == 3) {
                onctrl(0);  // as the console would have
                *code = ERROR_OPERATION_ABORTED;
                return -1;
            } else if (c=='\r' || c=='\n') {
                tmp[n++] = '\n';
                break;
            } else if (c==8 || c==0x7f) {
                n -= n>0;
                n -= n>0 && tmp[n]>=0xdc00 && tmp[n]<=0xdfff &&
                     tmp[n-1]>=0xd800 && tmp[n-1]<=0xdbff;
            } else if (n < CONUNITS-1) {
                tmp[n++] = c;
            }
        }
        *code = 0;
        iz r = conconvert(tmp, n, out);
        return r<0 ? 0 : r;
    }
    SetLastError(0);
    b32 ok = ReadConsoleW(h, tmp, countof(tmp), &n, 0);
    *code = GetLastError();
    if (!ok || (!n && *code==ERROR_OPERATION_ABORTED)) {
        *code = ok ? ERROR_OPERATION_ABORTED : *code;
        return -1;
    }
    for (i32 i = 0; keys==TK_CHARS && i<n; i++) {
        if (tmp[i] == 3) {
            onctrl(0);  // the keys with it go, as with line input
            *code = ERROR_OPERATION_ABORTED;
            return -1;
        }
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
        r = conline(hp->os, w->own, hp->buf, &hp->code);
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

// Wait for a named pipe's client: 0 once there is one, else the error.
// A client may have come and even gone already.
static i32 connectpipe(uz h)
{
    if (ConnectNamedPipe(h, 0)) return 0;
    i32 code = GetLastError();
    return code==ERROR_PIPE_CONNECTED || code==ERROR_NO_DATA ? 0 : code;
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
        case H_CONNECT:
            hp->code = connectpipe(hp->obj->own);
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
// otherwise the operation simply runs on, abandoned. Never a console
// read: the console would throw away the next line typed.
static void cancelhelper(Os *os, Helper *hp)
{
    if (!os->cancelsync || hp->obj->kind==K_CONIN) return;
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
    if (io->op == IO_CONNECT) {
        if (issue(os, w, h, H_CONNECT, io)) return 0;
        i32 code = connectpipe(h);  // no helper: block
        io->n   = 0;
        io->err = code ? errnum(code) : 0;
        return 1;
    }
    if (io->op == IO_READ) {
        if (w->kind == K_PIPE) {
            w->streak += w->streak < AHEADSTREAK;
            fillahead(w, h);
        }
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
            if (!io->n && !hp->eof && hp->code==ERROR_OPERATION_ABORTED && w->kind==K_PIPE) {
                // Ended by lend (an abandoned read, which a reader then
                // took up), not by the core: that reader reads again
                // once the object is free, as bash's would block on
                // the pipe it shares with the program
                io->next   = w->queue;
                w->queue   = io;
                io->plt[0] = S_QUEUED;
                io->plt[1] = (uz)w;
                os->nwait++;
            } else {
                io->err = !io->n && !hp->eof && hp->code ? E_INTR : 0;
                pushdone(os, io);
            }
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
    case H_CONNECT:
        if (io) {
            io->n   = 0;
            io->err = hp->code ? errnum(hp->code) : 0;
            pushdone(os, io);
        }
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
static b32  takemissed(Os *os, uz proc);

static i32 procpid(Os *os, uz proc);

// A process's exit status, releasing its handle. XP gives 0 for a
// process terminated before it finished starting, whatever code
// TerminateProcess passed, so a process os_kill ended that reads 0
// takes the code it was given.
static i32 procexit(Os *os, uz proc)
{
    i32 code = 127;
    if (!GetExitCodeProcess(proc, &code)) code = 127;
    if (!code && os->nkilled) {
        i32 pid = procpid(os, proc);
        for (i32 i = 0; pid && i<countof(os->killed); i++) {
            if (os->killed[i].pid == pid) {
                code = (i32)os->killed[i].code;
                os->killed[i] = {};
            }
        }
    }
    i32 missed = takemissed(os, proc) ? WMISSED : 0;
    CloseHandle(proc);
    releaselends(os, proc);
    return exitstatus((u32)code) | missed;
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
// (onctrl, CancelSynchronousIo): -1 then, or on error.
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
// length, 0 at end of file, -1 for Ctrl-C or an error. Ctrl-C typed at
// the console ends the read. A Ctrl-C event sent another way does not
// (GenerateConsoleCtrlEvent): the shell takes it once a line arrives.
// CancelSynchronousIo would end the read, but the console then throws
// away the next line typed.
static iz conread(Os *os, Wobj *w, uz h)
{
    reserveahead(w, 3*CONUNITS);
    i32 code = 0;
    iz  n    = conline(os, h, w->ahead+w->alen, &code);
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
    w->streak = 0;  // read exactly again for a while (fillahead)
    if (!w->ip && w->helper && w->helper->op==H_READ && !w->active && os->cancelsync) {
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

static void lockrecent(Os *os)
{
    while (__atomic_exchange_n(&os->recentlock, 1, __ATOMIC_ACQUIRE)) {
        Sleep(0);
    }
}

static void unlockrecent(Os *os)
{
    __atomic_store_n(&os->recentlock, 0, __ATOMIC_RELEASE);
}

static i64 ticks()
{
    i64 t = 0;
    QueryPerformanceCounter(&t);
    return t;
}

static u64 bornat(uz proc)  // creation time, 0 if unknown
{
    Filetime t[4] = {};
    if (!GetProcessTimes(proc, t, t+1, t+2, t+3)) return 0;
    return (u64)t[0].hi<<32 | t[0].lo;
}

static b32 exited(uz proc)
{
    return !WaitForSingleObject(proc, 0);
}

// Another process's memory, read as its own bitness lays it out: from
// a WOW64 build, a 64-bit process's through ntdll's Wow64 calls (its
// addresses may lie past 4 GiB, and its PEB is not the one
// NtQueryInformationProcess reports).
struct Remote {
    uz  proc;
    u64 peb;     // 0 if unknown
    b32 wide;    // 64-bit pointers
    b32 via64;   // read with Os.read64
};

static Remote remote(Os *os, uz proc)
{
    Remote r = {};
    r.proc = proc;
    r.wide = sizeof(uz) == 8;
    b32 wow64 = 1;
    if (os->queryproc64 && os->iswow64(proc, &wow64) && !wow64) {
        u64 pbi[6] = {};  // PROCESS_BASIC_INFORMATION64
        if (!os->queryproc64(proc, 0, pbi, sizeof(pbi), 0)) {
            r.peb   = pbi[1];
            r.wide  = 1;
            r.via64 = 1;
        }
        return r;
    }
    uz pbi[6] = {};  // PROCESS_BASIC_INFORMATION
    if (os->queryproc && !os->queryproc(proc, 0, pbi, sizeof(pbi), 0)) {
        r.peb = pbi[1];
    }
    return r;
}

static b32 rread(Os *os, Remote *r, u64 addr, void *buf, iz len)
{
    if (!addr) return 0;
    if (r->via64) {
        u64 n = 0;
        return !os->read64(r->proc, addr, buf, (u64)len, &n) && n==(u64)len;
    }
    iz n = 0;
    return ReadProcessMemory(r->proc, (void *)(uz)addr, buf, len, &n) && n==len;
}

// The pointer at addr in the process, 0 if unknown.
static u64 rptr(Os *os, Remote *r, u64 addr)
{
    u64 v = 0;
    u32 v32 = 0;
    if (r->wide) return rread(os, r, addr, &v, 8) ? v : 0;
    return rread(os, r, addr, &v32, 4) ? v32 : 0;
}

// The pointer in slot i of a process's PEB, 0 if unknown.
static u64 pebslot(Os *os, Remote *r, iz i)
{
    return r->peb ? rptr(os, r, r->peb + (u64)(i*(r->wide ? 8 : 4))) : 0;
}

// A process's id from its handle (GetProcessId is XP SP1), 0 if unknown.
static i32 procpid(Os *os, uz proc)
{
    uz pbi[6] = {};  // PROCESS_BASIC_INFORMATION
    if (!os->queryproc || os->queryproc(proc, 0, pbi, sizeof(pbi), 0)) {
        return 0;
    }
    return (i32)pbi[4];  // UniqueProcessId
}

// Is the process's image a console program? Read through its PEB, so
// it works before the process has run. False if unknown.
static b32 consoleimage(Os *os, uz proc)
{
    Remote r    = remote(os, proc);
    u64    base = pebslot(os, &r, 2);  // ImageBaseAddress
    if (!base) return 0;
    i32 nt = 0;  // IMAGE_DOS_HEADER.e_lfanew
    if (!rread(os, &r, base+0x3c, &nt, 4) || nt<0) return 0;
    c16 subsystem = 0;  // IMAGE_OPTIONAL_HEADER.Subsystem, PE32 or PE32+
    if (!rread(os, &r, base+(u64)nt+0x5c, &subsystem, 2)) return 0;
    return subsystem == 3;  // IMAGE_SUBSYSTEM_WINDOWS_CUI
}

// Offsets in RTL_USER_PROCESS_PARAMETERS, by pointer width
enum : iz {
    PARAMS_FLAGS = 8,
};
static iz paramsconsoleflags(b32 wide) { return wide ? 0x18 : 0x14; }
static iz paramsimagepath(b32 wide)    { return wide ? 0x60 : 0x38; }

struct Ustr {  // UNICODE_STRING
    c16  len;  // bytes
    c16  cap;
    c16 *data;
};

// Was the process started with Ctrl-C ignored (the inherited flag that
// ctrlcignored reads for this process)? A Ctrl-C did nothing to it,
// missed or not. False if unknown.
static b32 ignoresctrlc(Os *os, uz proc)
{
    Remote r      = remote(os, proc);
    u64    params = pebslot(os, &r, 4);  // ProcessParameters
    u32    flags  = 0;
    return rread(os, &r, params+(u64)paramsconsoleflags(r.wide), &flags, 4) && flags&1;
}

static i32 foldpath(c16 c)
{
    c = c=='/' ? '\\' : c;
    return c>='A' && c<='Z' ? c+32 : c;
}

// Does the process run this very program (its image path is Os.self)?
// Then it judges what it starts by these same rules. False if unknown.
static b32 sameimage(Os *os, uz proc)
{
    Remote r      = remote(os, proc);
    u64    params = pebslot(os, &r, 4);  // ProcessParameters
    u32    flags  = 0;
    c16    len    = 0;  // UNICODE_STRING.Length, bytes
    if (!os->selflen || !params) return 0;
    if (r.wide != (sizeof(uz)==8)) return 0;  // another build
    u64 at = params + (u64)paramsimagepath(r.wide);
    if (!rread(os, &r, params+PARAMS_FLAGS, &flags, 4) ||
        !rread(os, &r, at, &len, 2) || len/2!=os->selflen) {
        return 0;
    }
    u64 data = rptr(os, &r, at + (r.wide ? 8 : 4));  // .Buffer
    if (!(flags & 1)) {  // not yet normalized: an offset from params
        data += params;
    }
    for (i32 i = 0; i < os->selflen; i += 64) {
        c16 buf[64];
        i32 len = os->selflen-i<64 ? os->selflen-i : 64;
        if (!rread(os, &r, data+2*(u64)i, buf, 2*len)) return 0;
        for (i32 k = 0; k < len; k++) {
            if (foldpath(buf[k]) != foldpath(os->self[i+k])) return 0;
        }
    }
    return 1;
}

// Is the process still in its loader's initialization (the
// ProcessInitializing bit of PEB.CrossProcessFlags)? It has run none of
// its own code. False if unknown.
static b32 starting(Os *os, uz proc)
{
    Remote r     = remote(os, proc);
    u64    flags = pebslot(os, &r, 10);  // CrossProcessFlags
    return flags>>1 & 1;
}

enum : i32 { END_DONE, END_SPARED, END_FAILED };  // endmissed

// End, as a Ctrl-C would have, a process that missed one, once its
// initial thread runs (the loader has set PEB.Ldr): ended earlier, its
// parent's CreateProcess would fail. One started with Ctrl-C ignored is
// spared. END_DONE also if another level ended it, END_FAILED if it
// exited otherwise, or did not start within 50 ms. It waits, so call it
// without the lock.
static i32 endmissed(Os *os, uz proc)
{
    if (ignoresctrlc(os, proc)) return END_SPARED;
    for (i64 end = ticks() + os->youngticks/2; !exited(proc) && ticks()<end;) {
        Remote r = remote(os, proc);
        if (pebslot(os, &r, 3)) {  // Ldr
            if (ignoresctrlc(os, proc)) return END_SPARED;
            TerminateProcess(proc, 0xc000013a);
            WaitForSingleObject(proc, 1000);
            break;
        }
        Sleep(1);
    }
    i32 code = 0;
    b32 done = exited(proc) && GetExitCodeProcess(proc, &code) &&
               (u32)code==0xc000013au;
    return done ? END_DONE : END_FAILED;
}

// A job of its own for a program just created suspended, so that its
// descendants can be told at a Ctrl-C (see judge). Breaking away from
// it is allowed if the shell is in no job, or its own job allows it
// (Os.jobflags): what a program could do without this job. More limits
// in extra. 0 if unavailable.
static uz newjob(Os *os, uz proc, u32 extra)
{
    uz job = CreateJobObjectW(0, 0);
    if (!job) return 0;
    JobLimits l = {};
    l.flags = os->jobflags | extra;
    i32 info = JobObjectExtendedLimitInformation;
    if (!SetInformationJobObject(job, info, &l, sizeof(l)) ||
        !AssignProcessToJobObject(job, proc)) {
        CloseHandle(job);
        return 0;
    }
    return job;
}

// Prepare for jobs (Windows 8): the port, the breakaway rule and this
// program's image path.
static void setupjobs(Os *os)
{
    os->jobport = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1);

    JobLimits own  = {};
    i32       info = JobObjectExtendedLimitInformation;
    os->jobflags = JOB_OBJECT_LIMIT_BREAKAWAY_OK;
    if (QueryInformationJobObject(0, info, &own, sizeof(own), 0)) {
        os->jobflags &= own.flags;  // the shell's own job
    }

    Remote r      = remote(os, GetCurrentProcess());
    byte  *params = (byte *)(uz)pebslot(os, &r, 4);
    Ustr   path   = {};
    if (params) __builtin_memcpy(&path, params+paramsimagepath(sizeof(uz)==8), sizeof(path));
    if (path.data && path.len/2<=countof(os->self)) {
        __builtin_memcpy(os->self, path.data, path.len);
        os->selflen = path.len / 2;
    }
}

// Stop reporting a job's processes to the port. Where that fails, they
// go on reporting under a key that matches no watch (see notejob).
static void unbind(Recent *r)
{
    JobPort none = {};
    i32     info = JobObjectAssociateCompletionPortInformation;
    b32     done = SetInformationJobObject(r->job, info, &none, sizeof(none));
    r->bound = r->bound && !done;
}

// End a watch (see watchjob): the program missed the Ctrl-C if
// something it started exited by itself meanwhile.
static void settle(Recent *r)
{
    r->state = r->late ? R_MISSED : 0;
    r->late  = 0;
    __builtin_memset(r->unseen, 0, sizeof(r->unseen));
    unbind(r);
}

static void dropjob(Recent *r)
{
    if (r->bound) unbind(r);
    if (r->job) CloseHandle(r->job);
}

// Note a program just started with Ctrl-C enabled, and its job (or 0),
// in place of the oldest entry not in use. procexit removes the entry
// before the handle closes, so a handle here is live.
static void addrecent(Os *os, uz proc, i32 pid, uz job)
{
    i64 now = ticks();
    lockrecent(os);
    Recent *r = 0;
    for (i32 i = 0; i < countof(os->recent); i++) {
        Recent *c = os->recent + i;
        if (!c->proc) {
            r = c;
            break;
        }
        if (!c->busy && c->state!=R_WATCHED && (!r || c->time<r->time)) {
            r = c;
        }
    }
    if (!r) {
        unlockrecent(os);
        if (job) CloseHandle(job);  // all 32 judged right now
        return;
    }
    dropjob(r);
    *r = {};
    r->proc = proc;
    r->job  = job;
    r->key  = ++os->jobkeys;
    r->pid  = pid;
    r->time = now;
    unlockrecent(os);
}

// Forget a process. True if a Ctrl-C came that it may have missed,
// which waits for any judging or watch of it to settle.
static b32 takemissed(Os *os, uz proc)
{
    for (i32 n = 0;; n++) {
        b32 r    = 0;
        b32 wait = 0;
        lockrecent(os);
        for (i32 i = 0; i < countof(os->recent); i++) {
            Recent *e = os->recent + i;
            if (e->proc != proc) continue;
            wait = (e->busy || e->state==R_WATCHED) && n<1000;
            if (!wait) {
                r  = e->state == R_MISSED;
                dropjob(e);
                *e = {};
            }
        }
        unlockrecent(os);
        if (!wait) return r;
        Sleep(1);
    }
}

static b32 attached(Judging *j, i32 pid)
{
    b32 r = j->npids < 0;
    for (i32 i = 0; i < j->npids; i++) {
        r |= j->pids[i] == pid;
    }
    return r;
}

struct Verdict {  // see judge
    JobCounts before;  // the job's counts before its processes were listed
    b32       known;   // the job was listed
    b32       kids;    // it had descendants at the Ctrl-C
    b32       missed;  // one missed it and could not be ended
    b32       self;    // it runs this very program
};

// A Ctrl-C reaches only the processes attached to the console when it
// is sent, and since Windows 8 a process attaches in its own startup,
// after CreateProcess returns, so a program started just before can
// miss it, where bash's forked child would have it. Judge a young
// program by the console's process list, read as soon as the event
// reached this process:
// * Not attached, a console program: it cannot have had it, and has run
//   none of its own code, so it is ended as the Ctrl-C would have.
// * Attached but still in its loader's initialization (about 3 ms for
//   busybox): it may not act on the event, and has run none of its own
//   code, so the event could only end it. It is ended likewise.
// * Attached: it had it, and so did its descendants (its job's other
//   processes) created before the event and attached; the others, and
//   those still initializing, are ended likewise, once started, unless
//   started with Ctrl-C ignored. One without any may be a wrapper that
//   ignores Ctrl-C and waits for the real program (w64devkit's command
//   aliases for busybox), caught before starting it, so it is watched
//   (see decide).
// A missed program that cannot be ended is reported by os_waitproc
// (WMISSED), and the level waiting for it takes the signal as its own.
// Called unlocked, with the entry kept (Recent.busy).
static Verdict judge(Os *os, Judging *j, Recent *r)
{
    Verdict v = {};
    if (!attached(j, r->pid)) {
        if (consoleimage(os, r->proc)) TerminateProcess(r->proc, 0xc000013a);
        return v;
    }
    if (!os->lateattach) return v;  // XP: it had it with all it started
    if (starting(os, r->proc)) {
        v.missed = endmissed(os, r->proc) == END_FAILED;
        return v;
    }
    if (!r->job) return v;  // it had it, with all it started

    i32 info = JobObjectBasicAccountingInformation;
    if (!QueryInformationJobObject(r->job, info, &v.before, sizeof(v.before), 0)) {
        return v;
    }
    JobPids l  = {};
    b32     ok = QueryInformationJobObject(r->job, JobObjectBasicProcessIdList,
                                           &l, sizeof(l), 0);
    if (!ok && GetLastError()!=ERROR_MORE_DATA) return v;
    v.known = 1;
    v.kids  = l.listed < l.assigned;
    for (u32 i = 0; i < l.listed; i++) {
        i32 pid = (i32)l.pids[i];
        if (pid == r->pid) continue;
        uz h = OpenProcess(0x100411, 0, pid);  // query, read, end, wait
        if (!h) continue;
        if (bornat(h) < j->at) {
            v.kids = 1;
            b32 had = attached(j, pid) && !starting(os, h);
            if (!had && consoleimage(os, h)) {
                v.missed |= endmissed(os, h) == END_FAILED;
            }
        }
        CloseHandle(h);
    }
    v.self = sameimage(os, r->proc);
    return v;
}

// Take a verdict (lock held). A program childless at the Ctrl-C is
// watched, unless it runs this very program (it judges its own): its
// job is bound to the port, and a process that exited between the
// listing and the binding went unseen, so was missed.
static void decide(Os *os, Judging *j, Recent *r, Verdict v)
{
    if (v.missed) {
        if (r->state == R_WATCHED) {
            r->late = 1;  // watched since an earlier Ctrl-C
        } else {
            r->state = R_MISSED;
        }
    }
    b32 watch = v.known && !v.kids && !v.self && os->jobport;
    if (!watch || r->state==R_MISSED) return;

    if (!r->bound) {
        JobPort p = {};
        p.key    = r->key;
        p.port   = os->jobport;
        i32 info = JobObjectAssociateCompletionPortInformation;
        r->bound = SetInformationJobObject(r->job, info, &p, sizeof(p));
        if (!r->bound) return;
    }
    JobCounts after = {};
    i32       info  = JobObjectBasicAccountingInformation;
    QueryInformationJobObject(r->job, info, &after, sizeof(after), 0);
    if (exited(r->proc)) {  // it had it
        if (r->state != R_WATCHED) unbind(r);
        return;
    }
    u32 gone  = v.before.total - v.before.active;
    r->late  |= after.total-after.active > gone;
    r->state  = R_WATCHED;
    r->at     = j->at;
    r->until  = j->now + os->youngticks/2;
}

// Judge the young programs (see judge), on the control handler's thread
// before the signal is posted, so that nothing the shell does about it
// (a stage ending, a pipe closing) reaches a program first. True if
// any is to be watched.
static b32 notemissed(Os *os)
{
    Judging j = {};
    j.npids = GetConsoleProcessList(j.pids, countof(j.pids));
    j.npids = j.npids>0 && j.npids<=countof(j.pids) ? j.npids : -1;
    Filetime ft[4] = {};  // this thread's creation: when the event came
    GetThreadTimes(GetCurrentThread(), ft, ft+1, ft+2, ft+3);
    j.at  = (u64)ft[0].hi<<32 | ft[0].lo;
    j.now = ticks();
    __atomic_store_n(&os->ctrlat, j.at, __ATOMIC_SEQ_CST);

    Recent *young[countof(os->recent)];
    i32     n = 0;
    lockrecent(os);
    for (i32 i = 0; i < countof(os->recent); i++) {
        Recent *r = os->recent + i;
        if (r->proc && j.now-r->time<os->youngticks) {
            r->busy++;
            young[n++] = r;
        }
    }
    unlockrecent(os);

    b32 watch = 0;
    for (i32 i = 0; i < n; i++) {
        Recent *r = young[i];
        Verdict v = judge(os, &j, r);
        lockrecent(os);
        decide(os, &j, r, v);
        r->busy--;
        watch |= r->state == R_WATCHED;
        unlockrecent(os);
    }
    return watch;
}

// A message from a watched program's job: whatever the program starts
// within 50 ms of the Ctrl-C missed it and is ended, unless started
// with Ctrl-C ignored. One that exits by itself first cannot be, and
// the program is taken to have missed it. Its own exit ends the watch.
static void notejob(Os *os, i32 msg, uz key, i32 pid)
{
    lockrecent(os);
    Recent *r = 0;
    for (i32 i = 0; i < countof(os->recent); i++) {
        Recent *c = os->recent + i;
        r = c->state==R_WATCHED && c->key==key ? c : r;
    }
    if (!r) {
        unlockrecent(os);  // stale
        return;
    }

    switch (msg) {
    case JOB_OBJECT_MSG_NEW_PROCESS:
        if (pid != r->pid) {
            u64 at = r->at;
            r->busy++;
            unlockrecent(os);
            uz  h    = OpenProcess(0x100411, 0, pid);  // query, read, end, wait
            u64 born = h ? bornat(h) : 0;
            b32 late = 0;
            if (h && born>=at && born-at<500000) {  // 50 ms
                late = endmissed(os, h) == END_FAILED;
            }
            if (h) CloseHandle(h);
            lockrecent(os);
            r->busy--;
            if (r->state == R_WATCHED) {
                r->late |= late;
                if (!h) {  // gone already: its exit message tells
                    i32 i = 0;
                    for (; i<countof(r->unseen) && r->unseen[i]; i++) {}
                    if (i < countof(r->unseen)) {
                        r->unseen[i] = pid;
                    } else {
                        r->late = 1;
                    }
                }
            }
        }
        break;
    case JOB_OBJECT_MSG_EXIT_PROCESS:
    case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
        if (pid == r->pid) {
            r->until = 0;  // settled now, or once judging it is done
            if (!r->busy) settle(r);
        } else {
            for (i32 i = 0; i < countof(r->unseen); i++) {
                if (r->unseen[i] != pid) continue;
                r->unseen[i] = 0;
                r->late |= msg == JOB_OBJECT_MSG_EXIT_PROCESS;
            }
        }
        break;
    }
    unlockrecent(os);
}

// For 50 ms after a Ctrl-C, or until it exits, a watched program is
// taken to be a wrapper, and what it starts missed the Ctrl-C. Its job
// reports each process as it is created and as it exits, so none goes
// unseen, however briefly it runs, nor however late this starts.
static void watchjob(Os *os)
{
    for (;;) {
        i32   msg = 0;
        uz    key = 0;
        void *pid = 0;
        while (GetQueuedCompletionStatus(os->jobport, &msg, &key, &pid, 0)) {
            notejob(os, msg, key, (i32)(uz)pid);
        }

        i64 now  = ticks();
        i64 next = 0;
        lockrecent(os);
        for (i32 i = 0; i < countof(os->recent); i++) {
            Recent *r = os->recent + i;
            if (r->state != R_WATCHED) continue;
            if (r->until<=now && !r->busy) {
                settle(r);
            } else {
                i64 until = r->until>now ? r->until : now+1;
                next = !next || until<next ? until : next;
            }
        }
        unlockrecent(os);
        if (!next) return;

        i32 ms = (i32)((next - now)*1000/os->freq) + 1;
        if (GetQueuedCompletionStatus(os->jobport, &msg, &key, &pid, ms)) {
            notejob(os, msg, key, (i32)(uz)pid);
        }
    }
}

// A Ctrl-C came while os_spawn was in CreateProcess (nctrl moved): the
// new program missed it if not yet attached, or if created after it,
// since it may have attached by now (on XP too, in its own startup).
// Creation times tick coarsely (15.6 ms on XP), so a program created in
// the Ctrl-C's tick is judged by attachment alone.
static void notemissedspawn(Os *os, uz proc, i32 pid)
{
    Judging j = {};
    j.npids = GetConsoleProcessList(j.pids, countof(j.pids));
    j.npids = j.npids>0 && j.npids<=countof(j.pids) ? j.npids : -1;
    u64 at   = __atomic_load_n(&os->ctrlat, __ATOMIC_SEQ_CST);
    u64 born = bornat(proc);
    b32 late = at && born>at;
    if ((late || !attached(&j, pid)) && consoleimage(os, proc)) {
        TerminateProcess(proc, 0xc000013a);
    }
}

// From another thread: wake the main thread's wait, and end a blocking
// read it is in (where it can be cancelled), so that it takes signals.
static void wakemain(Os *os)
{
    SetEvent(os->wake);
    i32 t = __atomic_load_n(&os->mainio, __ATOMIC_SEQ_CST);
    if ((t&1) && os->cancelsync) {
        for (i32 i = 0; i<1000 && __atomic_load_n(&os->mainio, __ATOMIC_SEQ_CST)==t; i++) {
            if (os->cancelsync(os->mainthread)) break;
            Sleep(1);
        }
    }
}

// Another of our shells signalled this one through its channel.
static void __stdcall onchan(void *arg, u8)
{
    wakemain((Os *)arg);
}

// The channel's names for pid: the section's, or with ev its event's.
static c16 *channame(c16 *buf, i64 pid, b32 ev)
{
    c16 *p = buf;
    for (char const *s = ev ? "Local\\unabashed-sigev-" : "Local\\unabashed-sig-"; *s; s++) {
        *p++ = (c16)*s;
    }
    c16 digits[24];
    i32 n = 0;
    do {
        digits[n++] = (c16)('0' + pid%10);
    } while (pid /= 10);
    while (n) *p++ = digits[--n];
    *p = 0;
    return buf;
}

// Make this shell's signal channel; without one (a Low integrity
// process may not name objects) signals end it outright.
static void chanopen(Os *os, i64 pid)
{
    c16 name[64];
    uz  m = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0,
                               sizeof(SigChan), channame(name, pid, 0));
    if (!m) return;
    SigChan *c = (SigChan *)MapViewOfFile(m, FILE_MAP_WRITE, 0, 0, sizeof(SigChan));
    uz ev = c ? CreateEventW(0, 0, 0, channame(name, pid, 1)) : 0;
    if (!ev || !RegisterWaitForSingleObject(&os->chanwait, ev, onchan, os, INFINITE, 0)) {
        if (ev) CloseHandle(ev);
        if (c) UnmapViewOfFile(c);
        CloseHandle(m);
        return;
    }
    c->pid   = (u32)pid;
    c->magic = CHANMAGIC;
    os->chan = c;  // the mapping and event live as long as the process
}

// Send sig to pid's level (0: the process) through its channel.
// Returns E_SRCH where pid has none (not one of our shells).
static i32 chansend(i64 pid, i64 level, i32 sig)
{
    c16 name[64];
    uz  m = OpenFileMappingW(FILE_MAP_WRITE, 0, channame(name, pid, 0));
    if (!m) return E_SRCH;
    SigChan *c = (SigChan *)MapViewOfFile(m, FILE_MAP_WRITE, 0, 0, sizeof(SigChan));
    i32 err = E_SRCH;
    if (c && c->magic==CHANMAGIC && c->pid==(u32)pid) {
        err = 0;
        if (!level) {
            __atomic_fetch_or(&c->own, (u64)1<<(sig-1), __ATOMIC_SEQ_CST);
        } else {
            while (__atomic_exchange_n(&c->lock, 1, __ATOMIC_ACQUIRE)) Sleep(0);
            i32 n = c->nlevels;
            b32 dup = 0;
            for (i32 i = 0; i < n; i++) {
                dup |= c->levels[i].level==level && c->levels[i].sig==sig;
            }
            if (!dup && n<countof(c->levels)) {
                c->levels[n].level = level;
                c->levels[n].sig   = sig;
                __atomic_store_n(&c->nlevels, n+1, __ATOMIC_SEQ_CST);
            } else if (!dup) {
                err = E_AGAIN;
            }
            __atomic_store_n(&c->lock, 0, __ATOMIC_RELEASE);
        }
        uz ev = OpenEventW(EVENT_MODIFY_STATE, 0, channame(name, pid, 1));
        if (ev) {
            SetEvent(ev);
            CloseHandle(ev);
        }
    }
    if (c) UnmapViewOfFile(c);
    CloseHandle(m);
    return err;
}

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
    b32 watch = 0;
    if (sig != 1) {
        __atomic_fetch_add(&os->nctrl, 1, __ATOMIC_SEQ_CST);
        watch = notemissed(os);
    }
    __atomic_fetch_or(&os->siggrp, (u64)1<<(sig-1), __ATOMIC_SEQ_CST);
    wakemain(os);
    if (watch) watchjob(os);
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

static void os_decommit(Os *, byte *p, iz len)
{
    VirtualFree(p, len, MEM_DECOMMIT);
}

static b32 os_write(Os *os, Handle h, u8 *buf, iz len)
{
    Wobj *w    = lookup(os, h);
    i32   mode = 0;

    // Output to a regular file gathers in wbuf: a write costs about as
    // much as a builtin's whole run. It goes out before any other call
    // (flushw), when full, and at the first write 100 ms after it began.
    if (!w && h == os->werrh) {
        os->werrh = 0;
        return 0;
    }
    if (!w && (os->wlen ? h==os->wpend : GetFileType(h)==FILE_TYPE_DISK)) {
        iz room = countof(os->wbuf) - os->wlen;
        if (len > room) {
            flushw(os);
            if (os->werrh == h) {
                os->werrh = 0;
                return 0;
            }
            if (len > countof(os->wbuf)/2) return writeall(h, buf, len);
        }
        if (!os->wlen) {
            os->wpend = h;
            os->wtick = GetTickCount();
        }
        copybytes(os->wbuf+os->wlen, buf, len);
        os->wlen += (i32)len;
        if (GetTickCount()-os->wtick >= 100) {
            flushw(os);
            if (os->werrh == h) {
                os->werrh = 0;
                return 0;
            }
        }
        return 1;
    }
    flushw(os);
    // Only a character device can be a console: asking a file costs a
    // GetConsoleMode, a third of a write
    if (w ? w->kind!=K_CONOUT : GetFileType(h)!=FILE_TYPE_CHAR || !GetConsoleMode(h, &mode)) {
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
    flushw(os);
    Wobj *w = lookup(os, h);
    i32   mode = 0;
    if (!w && GetConsoleMode(h, &mode)) w = track(os, h);
    if (!w || w->kind==K_CONOUT || w->kind==K_CHAR) {
        return rawread(h, buf, len);
    }
    drainwake(os);
    if (w->kind == K_PIPE) {
        w->streak += w->streak < AHEADSTREAK;
        fillahead(w, h);
    }
    for (;;) {
        iz n = takeahead(w, buf, len);
        if (n) {
            if (w->kind==K_PIPE && !busy(w) && !w->lent && w->streak<AHEADSTREAK) {
                n += peekread(h, buf+n, len-n);
            }
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
    flushw(os);
    Wobj *w = lookup(os, h);
    if (!w || (w->kind!=K_PIPE && w->kind!=K_CONIN)) return -1;
    drainwake(os);
    if (w->kind == K_PIPE) fillahead(w, h);
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

static i32 os_unlink(Os *os, Arena scratch, Str path)
{
    flushw(os);
    Str16 w = widepath(&scratch, path);
    return DeleteFileW(w.data) ? 0 : errnum(GetLastError());
}

static i32 os_rename(Os *os, Arena scratch, Str from, Str to)
{
    flushw(os);
    enum { MOVEFILE_REPLACE_EXISTING = 1 };
    Str16 f = widepath(&scratch, from);
    Str16 t = widepath(&scratch, to);
    return MoveFileExW(f.data, t.data, MOVEFILE_REPLACE_EXISTING) ? 0 : errnum(GetLastError());
}

static Opened os_tempfile(Os *os, Arena)
{
    flushw(os);
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

static NamedTemp os_namedtemp(Os *os, Arena *a, Str dir)
{
    flushw(os);
    NamedTemp r = {};
    r.h = BADHANDLE;
    static u32 counter;
    for (i32 try_ = 0; try_ < 100; try_++) {
        u64 tag = ((u64)GetCurrentProcessId()<<32) ^ ++counter ^ (u64)os_now(os);
        tag *= 0x9e3779b97f4a7c15u;
        Buf b(a, dir.len+32);
        print(&b, dir);
        print(&b, "/sh-fd-");
        for (i32 i = 0; i < 16; i++) {
            putbyte(&b, (u8)"0123456789abcdef"[(tag >> (60 - 4*i)) & 15]);
        }
        Str   path = finish(&b);
        Arena tmp  = *a;
        Str16 w    = widepath(&tmp, path);
        uz h = CreateFileW(w.data, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_ALL, 0,
                           CREATE_NEW,
                           FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, 0);
        if (h != INVALID_HANDLE_VALUE) {
            r.h    = (Handle)h;
            r.path = path;
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
    flushw(os);
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
// An operation that ended meanwhile may release it in drainwake already.
static void os_close(Os *os, Handle h)
{
    flushw(os);
    if (h == BADHANDLE) return;
    if (h == os->werrh) os->werrh = 0;  // the handle value may come back
    Wobj *w = unreg(os, h);
    if (w) {
        drainwake(os);
        maybefree(os, w);
    }
    CloseHandle(h);
}

static Handle os_dup(Os *os, Handle h)
{
    flushw(os);
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
    flushw(os);
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
    Wobj *rw = newobj(os, K_PIPE);
    rw->mine = 1;
    reg(os, rd, rw);
    reg(os, wr, newobj(os, K_PIPE));
    r.r = rd;
    r.w = wr;
    return r;
}

// The name is unguessable, so that no other process can take an
// instance before the client it is meant for. Without a buffer
// (NP_NOBUF), a write waits for its client to read it, so that a
// client leaving unread (one that only looked) takes nothing with it.
static NamedPipe os_namedpipe(Os *os, Arena *a, Str name, i32 how)
{
    flushw(os);
    NamedPipe r = {};
    r.h = BADHANDLE;
    static u32 counter;
    b32 out   = how & NP_OUT;
    i32 pmode = PIPE_REJECT_REMOTE_CLIENTS;  // byte mode, blocking
    i32 bufs  = how&NP_NOBUF ? 0 : 1<<16;
    for (i32 try_ = 0; try_ < 100; try_++) {
        Str fresh = {};
        if (!name.data) {
            i64 qpc = 0;
            QueryPerformanceCounter(&qpc);
            u64 tag = (u64)os_now(os) ^ (u64)qpc<<20 ^ (u64)++counter<<48;
            tag *= 0x9e3779b97f4a7c15u;
            tag ^= tag >> 31;
            Buf b(a, 64);
            print(&b, "//./pipe/unabashed-");
            print(&b, (i64)GetCurrentProcessId());
            putbyte(&b, '-');
            for (i32 i = 0; i < 16; i++) {
                putbyte(&b, (u8)"0123456789abcdef"[(tag >> (60 - 4*i)) & 15]);
            }
            // busybox-w32's stat reads a file's first KiB to decide its
            // execute bit, which would take a bridge's data before the
            // program opens it to read (busybox diff saw an empty
            // procsub), except for a name ending in .dll
            print(&b, ".dll");
            fresh = finish(&b);
        }
        Str use = name.data ? name : fresh;
        c16 wname[64];
        if (use.len >= countof(wname)) {
            r.err = E_NAMETOOLONG;
            return r;
        }
        iz n = 0;
        for (; n < use.len; n++) wname[n] = use[n]=='/' ? '\\' : use[n];
        wname[n] = 0;
        i32 mode = out ? PIPE_ACCESS_OUTBOUND : PIPE_ACCESS_INBOUND;
        mode |= name.data ? 0 : FILE_FLAG_FIRST_PIPE_INSTANCE;
        uz h = CreateNamedPipeW(wname, mode, pmode, PIPE_UNLIMITED_INSTANCES,
                                bufs, bufs, 0, 0);
        i32 code = h==INVALID_HANDLE_VALUE ? GetLastError() : 0;
        if (code==ERROR_INVALID_PARAMETER && pmode) {
            pmode = 0;  // XP: no PIPE_REJECT_REMOTE_CLIENTS
            try_--;
            continue;
        } else if (code==ERROR_ACCESS_DENIED && !name.data) {
            continue;  // the name is taken
        } else if (code) {
            r.err = errnum(code);
            return r;
        }
        Wobj *w = newobj(os, K_PIPE);
        w->pipeout = out;
        copybytes(w->pname, wname, (n+1)*(iz)sizeof(c16));
        reg(os, h, w);
        r.h    = h;
        r.name = use;
        return r;
    }
    r.err = E_EXIST;
    return r;
}

static i64 os_seek(Os *os, Handle h, i64 off, i32 whence)
{
    flushw(os);
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
    r.ctime = r.mtime;  // no change time before FILE_BASIC_INFO (Vista)
    r.blocks = (size + 4095)/4096 * 8;  // whole 4 KiB clusters
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

// Reparse points are not read: callers treat a link as a plain file.
static Str os_readlink(Os *os, Arena *, Str)
{
    flushw(os);
    return {};
}

static Stat os_stat(Os *os, Arena scratch, Str path, b32 follow)
{
    flushw(os);
    Stat r = {};

    // A trailing slash demands a directory.
    b32 dironly = 0;
    while (path.len>1 && path[path.len-1]=='/') {
        if (path.len==3 && path[1]==':') break;  // C:/
        path.len--;
        dironly = 1;
    }

    Str16 w     = widepath(&scratch, path);
    b32   exe   = hassuffix(path, ".exe");
    i32   flags = FILE_FLAG_BACKUP_SEMANTICS;
    flags |= follow ? 0 : FILE_FLAG_OPEN_REPARSE_POINT;

    // The execute bits say what the content is: #!, MZ or ":\n" (the
    // core adds its extension rules, spawn.cpp's winmode). Read access
    // lets the first bytes decide, except for an .exe, taken to be MZ.
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
                b32 co = magic[0]==':' && magic[1]=='\n';
                r.mode |= mz||sb||co ? 0111 : 0;
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

enum : i32 {  // console modes
    CM_PROCESSED_INPUT = 0x0001, CM_LINE_INPUT = 0x0002, CM_ECHO_INPUT = 0x0004,
    CM_WINDOW_INPUT = 0x0008, CM_VT_INPUT = 0x0200,
    CM_PROCESSED_OUTPUT = 0x0001, CM_WRAP_AT_EOL = 0x0002, CM_VT_OUTPUT = 0x0004,
    CM_NO_AUTO_RETURN = 0x0008,
};

static Tty os_tty(Os *os, Handle in, Handle out, i32 how)
{
    flushw(os);
    Tty r = {};
    i32 imode = 0;
    i32 omode = 0;
    ConsoleInfo ci = {};
    b32 conin = GetConsoleMode(in, &imode);
    r.noctl = conin && !os->ttyraw;  // even if out is not the console
    if (!conin || !GetConsoleMode(out, &omode) || !GetConsoleScreenBufferInfo(out, &ci)) {
        r.err = E_NOTTY;
        return r;
    }
    if (how!=TTY_QUERY && how!=TTY_RESTORE) {
        if (!os->ttyraw) {
            os->ttyraw     = 1;
            os->ttyinmode  = imode;
            os->ttyoutmode = omode;
        }
        i32 base = os->ttyinmode;
        switch (how) {
        case TTY_RAW: {
            i32 raw = base & ~(CM_PROCESSED_INPUT|CM_LINE_INPUT|CM_ECHO_INPUT);
            raw |= CM_WINDOW_INPUT;
            i32 keys = TK_VT;
            if (!SetConsoleMode(in, raw|CM_VT_INPUT)) {
                SetConsoleMode(in, raw);
                keys = TK_EVENTS;
            }
            __atomic_store_n(&os->ttykeys, keys, __ATOMIC_SEQ_CST);
            i32 vt = omode | CM_PROCESSED_OUTPUT | CM_WRAP_AT_EOL | CM_VT_OUTPUT;
            if (!SetConsoleMode(out, vt|CM_NO_AUTO_RETURN) && !SetConsoleMode(out, vt)) {
                SetConsoleMode(out, omode);
            }
        } break;
        case TTY_NOECHO:
            __atomic_store_n(&os->ttykeys, TK_SILENT, __ATOMIC_SEQ_CST);
            SetConsoleMode(in, base & ~(CM_PROCESSED_INPUT|CM_LINE_INPUT|CM_ECHO_INPUT));
            break;
        case TTY_CHARS:
        case TTY_CHARSNOECHO:
            __atomic_store_n(&os->ttykeys, TK_CHARS, __ATOMIC_SEQ_CST);
            // The console echoes only whole lines
            SetConsoleMode(in, base & ~(CM_PROCESSED_INPUT|CM_LINE_INPUT|CM_ECHO_INPUT));
            r.noecho = how == TTY_CHARS;
            break;
        }
    } else if (how==TTY_RESTORE && os->ttyraw) {
        os->ttyraw = 0;
        __atomic_store_n(&os->ttykeys, TK_LINES, __ATOMIC_SEQ_CST);
        SetConsoleMode(in, os->ttyinmode);
        SetConsoleMode(out, os->ttyoutmode);
    }
    GetConsoleMode(out, &omode);
    r.cols   = ci.width;
    r.rows   = ci.bottom - ci.top + 1;
    r.legacy = !(omode & CM_VT_OUTPUT);
    r.xenl   = !!(omode & CM_NO_AUTO_RETURN);
    r.intr   = 0x03;
    r.eof    = 0x04;
    r.erase  = 0x7f;
    r.kill   = 0x15;
    r.werase = 0x17;
    r.lnext  = 0x16;
    return r;
}

static i32 concoord(i32 x, i32 y)
{
    return (i32)((u32)(x & 0xffff) | (u32)(y & 0xffff)<<16);
}

static b32 os_con(Os *os, Handle out, ConOp *o)
{
    flushw(os);
    ConsoleInfo ci = {};
    if (!GetConsoleScreenBufferInfo(out, &ci)) return 0;
    switch (o->op) {
    case CON_INFO:
        o->cols = ci.width;
        o->rows = ci.bottom - ci.top + 1;
        o->x    = ci.x;
        o->y    = ci.y - ci.top;
        o->attr = ci.attr & 0xffff;
        return 1;
    case CON_MOVE:
        return SetConsoleCursorPosition(out, concoord(o->x, ci.top+o->y));
    case CON_WRITE:
        return os_write(os, out, o->text.data, o->text.len);
    case CON_FILL: {
        i32 n = (i32)(o->n<0x7fffffff ? o->n : 0x7fffffff);
        i32 at = concoord(o->x, ci.top+o->y);
        i32 done = 0;
        return FillConsoleOutputCharacterW(out, ' ', n, at, &done)
            && FillConsoleOutputAttribute(out, o->attr, n, at, &done);
    }
    case CON_ATTR:
        return SetConsoleTextAttribute(out, o->attr);
    case CON_TITLE: {
        c16 title[512];
        i32 n = 0;
        for (iz i = 0; i<o->text.len && n<countof(title)-3;) {
            n += (i32)utf16encode(title+n, utf8decode(o->text, &i));
        }
        title[n] = 0;
        return SetConsoleTitleW(title);
    }
    }
    return 0;
}

static b32 os_isatty(Os *os, Handle h)
{
    Wobj *w = lookup(os, h);
    if (w) return w->kind==K_CONIN || w->kind==K_CONOUT;
    i32 mode = 0;
    return !!GetConsoleMode(h, &mode);
}

static Stat os_fstat(Os *os, Handle h)
{
    flushw(os);
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

static Dirent *os_list(Os *os, Arena *a, Str dir, i32 *err)
{
    flushw(os);
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

// Start a program: the image s->path with the command line s->cmdline
// (the core's, spawn.cpp), suspended until it is in its job where it
// gets one. With exec (os_exec), its job kills it if the shell dies:
// the M3b job on Windows 8, else one made here, out of the shell's own
// job if that allows breaking away, without one if not. *job is then
// that job, or 0.
static void printhexuz(Buf *b, uz x)
{
    u8  buf[16];
    i32 n = 0;
    do {
        buf[n++] = (u8)"0123456789abcdef"[x&15];
    } while (x >>= 4);
    while (n) putbyte(b, buf[--n]);
}

// Give a suspended child of this program the parent's image region at
// the same address, committed as far as it is here (the image's arena
// goes on from there), and its bytes. Returns a Win32 error, or 0.
static i32 imagecopy(uz proc, ImageRef im)
{
    MemInfo mi = {};
    if (!VirtualQuery(im.base, &mi, sizeof(mi)) || mi.state!=MEM_COMMIT ||
            (iz)mi.size<im.len) {
        return 87;  // ERROR_INVALID_PARAMETER
    }
    if (VirtualAllocEx(proc, im.base, im.cap, MEM_RESERVE, PAGE_NOACCESS) != im.base) {
        return 487;  // ERROR_INVALID_ADDRESS: taken in the child
    }
    if (!VirtualAllocEx(proc, im.base, (iz)mi.size, MEM_COMMIT, PAGE_READWRITE)) {
        return GetLastError();
    }
    iz n = 0;
    if (!WriteProcessMemory(proc, im.base, im.base, im.len, &n) || n!=im.len) {
        return GetLastError();
    }
    return 0;
}

static Spawned startproc(Os *os, Arena scratch, Spawn *s, b32 exec, uz *job)
{
    Spawned r = {};
    r.proc = BADHANDLE;

    Str  cmd  = s->cmdline;
    Str  path = s->path;
    i32  slot = s->crtslot;
    b32  self = !!s->image.base;
    if (self) {
        // This program again, told where its image is (see imagearg)
        Buf b(&scratch, 1<<10);
        quoteprogram(&b, s->argc ? s->argv[0] : os->exe);
        print(&b, " --unabashed-image=");
        printhexuz(&b, (uz)s->image.base);
        putbyte(&b, ',');
        printhexuz(&b, (uz)s->image.cap);
        putbyte(&b, ',');
        printhexuz(&b, (uz)s->image.len);
        cmd  = finish(&b);
        path = os->exe;
        slot = sizeof(void *);  // as this program decodes it
    } else if (!cmd.len) {
        Buf b(&scratch, 1<<10);
        quoteprogram(&b, s->argc ? s->argv[0] : s->path);
        for (iz i = 1; i < s->argc; i++) {
            putbyte(&b, ' ');
            quotearg(&b, s->argv[i]);
        }
        cmd = finish(&b);
    }
    Str16 line = towide(&scratch, cmd);
    if (line.len > 32766) {
        r.err = E_2BIG;
        return r;
    }
    Str16 app = widepath(&scratch, path);

    // Environment block: sorted, each entry null-terminated, and one
    // more null at the end (towide adds the last). XP takes at most
    // 32767 characters in all.
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
    if (os->xp && wenv.len>32767) {
        r.err = E_2BIG;
        return r;
    }

    Str16 cwd = s->cwd.len ? widepath(&scratch, s->cwd) : Str16{};

    // Inheritable duplicates of the child's descriptors, closed right
    // after creation: 0-2 as the standard handles, and with crtslot the
    // rest in the CRT's lpReserved2 blob, at the child's handle width.
    // The blob leaves 0-2 closed, so that the CRT sets them up from the
    // standard handles as it would without one (text mode); the others
    // are binary, as busybox-w32 passes them. A pipe or console the
    // child gets is lent to it (see Wobj), perhaps through an
    // interposed pipe.
    uz  cur = GetCurrentProcess();
    i32 top = 3;
    for (iz i = 0; i<s->nfds && slot; i++) {
        i32 fd = s->fds[i].fd;
        top = fd>=top && fd<FDMAX ? fd+1 : top;
    }
    uz    *dups  = alloc<uz>(&scratch, top);  // 0: none
    Wobj **objs  = alloc<Wobj *>(&scratch, top);
    b32   *viaip = alloc<b32>(&scratch, top);
    for (iz i = 0; i < s->nfds; i++) {
        i32 fd = s->fds[i].fd;
        uz  h  = s->fds[i].h;
        if (fd<0 || fd>=top || h==BADHANDLE || dups[fd]) continue;
        h = lend(os, h, objs+fd, viaip+fd);
        if (!DuplicateHandle(cur, h, cur, dups+fd, 0, 1, DUPLICATE_SAME_ACCESS)) {
            dups[fd] = 0;
        }
    }
    Str blob = {};
    if (top > 3) {
        CrtFd *c = alloc<CrtFd>(&scratch, top);
        for (i32 fd = 3; fd < top; fd++) {
            if (!dups[fd]) continue;
            i32 type = GetFileType(dups[fd]);
            c[fd].handle = (i64)dups[fd];
            c[fd].flags  = CRTFD_OPEN | (type==FILE_TYPE_PIPE ? CRTFD_PIPE : 0) |
                           (type==FILE_TYPE_CHAR ? CRTFD_DEV : 0);
        }
        // A 32-bit parent always writes 4-byte slots: for a 64-bit
        // child WOW64 widens them itself (zero-extended), and would
        // widen 8-byte slots again (observed on Windows 11)
        i32 width = sizeof(void *)==4 ? 4 : slot;
        blob = encodecrtfds(&scratch, c, top, width);
    }

    StartupInfo si = {};
    si.cb     = sizeof(si);
    si.flags  = STARTF_USESTDHANDLES;
    si.input  = dups[0];
    si.output = dups[1];
    si.error  = dups[2];
    si.show   = (i32)(blob.len << 16);  // cbReserved2
    si.reserved2 = blob.data;
    ProcessInfo pi = {};
    // trap '' INT in the spawning subshell: the child inherits the
    // ignoring of Ctrl-C, which is set only around its creation so
    // that the shell itself is unaffected
    b32 noint = s->sigign>>1 & 1;
    // Since Windows 8, a program runs in a job of its own (see judge),
    // entered before it runs; exec's job is for kill-on-close only, so
    // also when the program ignores Ctrl-C (as in background, &)
    b32 tojob = os->lateattach && (!noint || exec);
    b32 bgjob = os->lateattach && noint && !exec && !self;
    uz  ejob  = 0;  // exec's job before Windows 8
    i32 flags = CREATE_UNICODE_ENVIRONMENT;
    flags |= os->noconsole ? CREATE_NO_WINDOW : 0;
    if (exec && !os->lateattach) {
        JobLimits own  = {};
        i32       info = JobObjectExtendedLimitInformation;
        b32       injob = QueryInformationJobObject(0, info, &own, sizeof(own), 0);
        if (!injob || (own.flags & JOB_OBJECT_LIMIT_BREAKAWAY_OK)) {
            ejob   = CreateJobObjectW(0, 0);
            flags |= injob && ejob ? CREATE_BREAKAWAY_FROM_JOB : 0;
        }
    }
    // A Ctrl-C the core has not yet seen came before this program could
    // attach, so it missed it, where bash would not have started it or
    // its forked child would have had it: it is ended before it runs.
    // That also covers a Ctrl-C between the core's last look and here,
    // a window long enough on a slow machine to start a whole program.
    i32 nctrl  = __atomic_load_n(&os->nctrl, __ATOMIC_SEQ_CST);
    u64 ctrls  = (u64)1<<1 | (u64)1<<2;  // SIGINT, SIGQUIT (Ctrl-Break)
    b32 unseen = !noint && (nctrl!=os->nctrlseen ||
                 (__atomic_load_n(&os->siggrp, __ATOMIC_SEQ_CST) & ctrls));
    flags |= tojob || bgjob || ejob || unseen || self ? CREATE_SUSPENDED : 0;
    if (noint) SetConsoleCtrlHandler(0, 1);
    b32 ok = CreateProcessW(app.data, line.data, 0, 0, 1, flags, wenv.data,
                            cwd.data, &si, &pi);
    i32 code = ok ? 0 : GetLastError();
    if (!ok && code==ERROR_DIRECTORY && cwd.data) {
        // Unusable working directory (too long, or gone): run anyway
        ok = CreateProcessW(app.data, line.data, 0, 0, 1, flags, wenv.data,
                            0, &si, &pi);
        code = ok ? 0 : GetLastError();
    }
    if (noint) SetConsoleCtrlHandler(0, 0);
    if (ok && self) {
        i32 err = imagecopy(pi.process, s->image);
        if (err) {
            TerminateProcess(pi.process, 1);
            CloseHandle(pi.thread);
            CloseHandle(pi.process);
            ok   = 0;
            code = err;
        } else if (!tojob && !bgjob && !ejob && !unseen) {
            ResumeThread(pi.thread);
        }
    }
    if (ok && unseen) TerminateProcess(pi.process, 0xc000013a);
    if (ok && exec) {
        uz j = 0;
        if (tojob) {
            j = newjob(os, pi.process, JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE);
        } else if (ejob) {
            JobLimits l = {};
            l.flags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_BREAKAWAY_OK;
            i32 info = JobObjectExtendedLimitInformation;
            if (SetInformationJobObject(ejob, info, &l, sizeof(l)) &&
                    AssignProcessToJobObject(ejob, pi.process)) {
                j = ejob;
            }
        }
        if (ejob && j!=ejob) CloseHandle(ejob);
        if (flags & CREATE_SUSPENDED) ResumeThread(pi.thread);
        *job = j;
    } else if (ok && bgjob) {
        uz j = newjob(os, pi.process, 0);
        if (j) {
            BgJob *b = os->bgjobs + os->nbgjobs%countof(os->bgjobs);
            if (b->job) CloseHandle(b->job);  // the oldest: let it go
            b->pid = pi.pid;
            b->job = j;
            os->nbgjobs++;
        }
        if (!unseen) ResumeThread(pi.thread);
    } else if (ok && !noint) {
        uz j = tojob ? newjob(os, pi.process, 0) : 0;
        if (tojob) ResumeThread(pi.thread);
        addrecent(os, pi.process, pi.pid, j);
        if (__atomic_load_n(&os->nctrl, __ATOMIC_SEQ_CST) != nctrl) {
            notemissedspawn(os, pi.process, pi.pid);
        }
    } else if (ejob) {
        CloseHandle(ejob);
    }
    for (i32 i = 0; i < top; i++) {
        if (dups[i]) CloseHandle(dups[i]);
        if (!objs[i]) continue;
        addlend(os, ok ? pi.process : 0, objs[i], viaip[i]);
    }
    if (!ok) {
        releaselends(os, 0);
        r.err = errnum(code);
        return r;
    }
    CloseHandle(pi.thread);
    if (self) VirtualFree(s->image.base, 0, MEM_RELEASE);  // the child's now
    r.proc = pi.process;
    r.pid  = (i64)pi.pid;
    return r;
}

static Spawned os_spawn(Os *os, Arena scratch, Spawn *s)
{
    flushw(os);
    return startproc(os, scratch, s, 0, 0);
}

// A program or shell ended by signal N (os_kill, or a shell dying by
// one) exits with SIGEXIT|128+N, which os_waitproc takes for death by
// N: an error NTSTATUS with the customer bit set, whose low byte is the
// status for parents that keep only that.
enum : u32 { SIGEXIT = 0xe0550000u };

// Native crashes become the matching signal statuses, as in Cygwin,
// and so does SIGEXIT.
static i32 exitstatus(u32 code)
{
    i32 low = (i32)(code & 255);
    if ((code & ~0xffu)==SIGEXIT && low>128 && low<128+NSIGNAL) {
        return WSIGNALED | low;
    }
    switch (code) {
    case 0xc0000005u: return WSIGNALED | (128 + 11);  // access violation: SIGSEGV
    case 0xc00000fdu: return WSIGNALED | (128 + 11);  // stack overflow
    case 0xc000001du: return WSIGNALED | (128 + 4);   // illegal instruction: SIGILL
    case 0xc0000094u: return WSIGNALED | (128 + 8);   // divide by zero: SIGFPE
    case 0xc000013au: return WSIGNALED | (128 + 2);   // Ctrl-C: SIGINT
    case 0xc0000409u: return WSIGNALED | (128 + 6);   // fast fail: SIGABRT
    case 0xc0000135u: return WNOLIB | 127;            // a DLL is missing
    }
    return (i32)(code & 0xff);
}

// The shell runs on a fiber of this reservation, not on the main
// thread's stack, so that deep shell recursion fails as cleanly as on
// Linux whatever the PE header says, as vprocs' fibers do.
enum { SHELLSTACK = (iz)16<<20 };

// Signals: one of our shells gets it through its channel (but KILL,
// and STOP, which it could not act on); any other process is ended
// outright by it, as killed by it (see SIGEXIT).

// Resource limits: Windows has none of POSIX's, so these are Cygwin's
// (Git Bash's ulimit -a): a table only, which ulimit reads and sets.
// Only the core size, stack, address space and open files take a new
// value; the stack is the shell's own, its fiber's reservation.
static Rlimit *rltable(Os *os)
{
    if (!os->rlinit) {
        os->rlinit = 1;
        for (i32 i = 0; i < RL_COUNT; i++) os->rl[i].err = E_NOSYS;
        Rlimit inf = {RL_INFINITY, RL_INFINITY, 0};
        os->rl[RL_CORE]   = {0, RL_INFINITY, 0};
        os->rl[RL_DATA]   = inf;
        os->rl[RL_FSIZE]  = inf;
        os->rl[RL_NOFILE] = {3200, 3200, 0};
        os->rl[RL_PIPE]   = {4096, 4096, 0};
        os->rl[RL_CPU]    = inf;
        os->rl[RL_NPROC]  = {256, 256, 0};
        os->rl[RL_AS]     = inf;
        os->rl[RL_STACK]  = {SHELLSTACK, RL_INFINITY, 0};
    }
    return os->rl;
}

static Rlimit os_getrlimit(Os *os, i32 res)
{
    return rltable(os)[res];
}

static i32 os_setrlimit(Os *os, i32 res, Rlimit r)
{
    flushw(os);
    Rlimit *t = rltable(os) + res;
    if (t->err) return t->err;
    switch (res) {
    case RL_CORE: case RL_STACK: case RL_AS: case RL_NOFILE: break;
    default: return E_INVAL;
    }
    if (r.hard!=RL_INFINITY && (r.soft==RL_INFINITY || r.soft>r.hard)) return E_INVAL;
    if (t->hard!=RL_INFINITY && (r.hard==RL_INFINITY || r.hard>t->hard)) return E_PERM;
    t->soft = r.soft;
    t->hard = r.hard;
    return 0;
}

// No process groups here: job control on Windows is the core's.
static i32 os_setpgid(Os *, i64, i64)
{
    return E_NOSYS;
}

static i64 os_tcpgrp(Os *, Handle, i64)
{
    return -E_NOSYS;
}

// The job of a background program by its pid, 0 if none.
static uz bgjobof(Os *os, i64 pid)
{
    i32 n = os->nbgjobs<countof(os->bgjobs) ? os->nbgjobs : countof(os->bgjobs);
    for (i32 i = 0; i < n; i++) {
        BgJob *b = os->bgjobs + i;
        if (b->job && b->pid==pid) return b->job;
    }
    return 0;
}

// No process groups here: -pid is a background program and everything
// in its job, the program alone if it has none.
static i32 os_kill(Os *os, i64 pid, i32 sig)
{
    flushw(os);
    if (pid<0 && pid>=-(i64)0x7fffffff) {
        uz job = bgjobof(os, -pid);
        pid = -pid;
        b32 kills = sig>0 && sig!=17 && sig!=18 && !(sig>=19 && sig<=23) && sig!=28;
        if (job && kills) {
            if (!chansend(pid, 0, sig)) return 0;  // one of our shells
            // another program: end it and its kin
            u32 code = sig==2 ? 0xc000013au : SIGEXIT|(u32)(128+sig);
            if (TerminateJobObject(job, code)) {
                i32     i = os->nkilled % countof(os->killed);
                Killed *k = os->killed + i;
                k->pid  = (i32)pid;
                k->code = code;
                os->nkilled = i + 1;
                return 0;
            }
        }
    }
    if (pid<=0 || pid>(i64)0x7fffffff) return E_SRCH;
    if (sig<0 || sig>=NSIGNAL) return E_INVAL;
    if (sig && sig!=9 && sig!=19 && !chansend(pid, 0, sig)) {
        return 0;  // one of our shells: its traps decide
    }
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
    default: {
        u32 code = sig==2 ? 0xc000013au : SIGEXIT|(u32)(128+sig);
        ok = TerminateProcess(h, code);
        if (ok) {  // see procexit
            i32     i = os->nkilled % countof(os->killed);
            Killed *k = os->killed + i;
            k->pid  = (i32)pid;
            k->code = code;
            os->nkilled = i + 1;  // never 0 again
        }
    }
    }
    CloseHandle(h);
    return ok ? 0 : E_PERM;
}

static i32 os_siglevel(Os *, i64 pid, i64 level, i32 sig)
{
    if (pid<=0 || pid>(i64)0x7fffffff || level<=0) return E_SRCH;
    if (sig<0 || sig>=NSIGNAL) return E_INVAL;
    if (!sig) {
        c16 name[64];
        uz  m = OpenFileMappingW(FILE_MAP_WRITE, 0, channame(name, pid, 0));
        if (m) CloseHandle(m);
        return m ? 0 : E_SRCH;
    }
    return chansend(pid, level, sig);
}

static LevelSig os_levelsig(Os *os)
{
    LevelSig r = {};
    SigChan *c = os->chan;
    if (!c || !__atomic_load_n(&c->nlevels, __ATOMIC_SEQ_CST)) return r;
    while (__atomic_exchange_n(&c->lock, 1, __ATOMIC_ACQUIRE)) Sleep(0);
    i32 n = c->nlevels;
    if (n) {
        r.level = c->levels[0].level;
        r.sig   = c->levels[0].sig;
        for (i32 i = 1; i < n; i++) c->levels[i-1] = c->levels[i];
        __atomic_store_n(&c->nlevels, n-1, __ATOMIC_SEQ_CST);
    }
    __atomic_store_n(&c->lock, 0, __ATOMIC_RELEASE);
    return r;
}

static u64 os_sigpending(Os *os)
{
    u64 r = __atomic_exchange_n(&os->sigown, (u64)0, __ATOMIC_SEQ_CST);
    if (os->chan) r |= __atomic_exchange_n(&os->chan->own, (u64)0, __ATOMIC_SEQ_CST);
    return r;
}

// Also notes how many Ctrl-C events the core has seen, after taking
// them, so that one counted but not yet posted stays pending (see the
// unseen rule in os_spawn).
static u64 os_siggroup(Os *os)
{
    u64 r = __atomic_exchange_n(&os->siggrp, (u64)0, __ATOMIC_SEQ_CST);
    os->nctrlseen = __atomic_load_n(&os->nctrl, __ATOMIC_SEQ_CST);
    return r;
}

static void os_sigmode(Os *, i32 sig, i32 mode)
{
    // Ctrl-C ignored on entry (ConsoleFlags): taken again; spawns set
    // it back around each program that should ignore it
    if (sig==2 && mode==SIGM_CLAIM) SetConsoleCtrlHandler(0, 0);
}

static void os_tasktag(Os *, u64, iz, u64)
{
}

static void os_racy(Os *)
{
}

// Self-spawning with a state image: a plain reservation, which
// os_spawn copies into the suspended child at the same address
// (imagecopy) and then releases.
static ImageRef os_imagenew(Os *os, iz cap)
{
    flushw(os);
    ImageRef r = {};
    r.base = os_reserve(os, cap);
    r.cap  = r.base ? cap : 0;
    return r;
}

static void os_imagefree(Os *, ImageRef im)
{
    if (im.base) VirtualFree(im.base, 0, MEM_RELEASE);
}

// The image a self-spawning parent left (imagecopy), from its command
// line argument "BASE,CAP,LEN" in hex: only a region that is wholly
// this process's own private, committed memory as far as LEN, which
// the core then checks further (imagemain).
static ImageRef imagearg(Str s)
{
    ImageRef r = {};
    uz v[3] = {};
    iz i = 0;
    for (i32 k = 0; k < 3; k++) {
        if (k && (i>=s.len || s[i++]!=',')) return r;
        iz beg = i;
        for (; i < s.len && i-beg < (iz)sizeof(uz)*2; i++) {
            u8 c = s[i];
            i32 d = c>='0' && c<='9' ? c-'0' : c>='a' && c<='f' ? c-'a'+10 : -1;
            if (d < 0) break;
            v[k] = v[k]<<4 | (uz)d;
        }
        if (i == beg) return r;
    }
    iz cap = (iz)v[1], len = (iz)v[2];
    if (i<s.len || cap<=0 || len<=0 || len>cap) return r;
    MemInfo mi = {};
    byte   *base = (byte *)v[0];
    if (!VirtualQuery(base, &mi, sizeof(mi)) || mi.allocbase!=base ||
            mi.base!=base || mi.state!=MEM_COMMIT || mi.type!=MEM_PRIVATE ||
            (iz)mi.size<len) {
        return r;
    }
    r.base = base;
    r.cap  = cap;
    r.len  = len;
    return r;
}

// No exec on Windows: the program starts as a child in a job that ends
// it if the shell dies (startproc), and the shell waits and exits with
// its exit code. Its descendants outlive a normal exit, as after a real
// exec: the job lets them go first. A signal sent to the shell through
// its channel goes to the program, as it would to the program exec'd.
static i32 os_exec(Os *os, Arena scratch, Spawn *s)
{
    flushw(os);
    uz      job = 0;
    Spawned r   = startproc(os, scratch, s, 1, &job);
    if (r.err) return r.err;
    for (;;) {
        uz  hs[2] = {r.proc, os->wake};
        i32 w = WaitForMultipleObjects(2, hs, 0, INFINITE);
        if (w != 1) break;
        drainwake(os);
        u64 own = os->chan ? __atomic_exchange_n(&os->chan->own, (u64)0, __ATOMIC_SEQ_CST) : 0;
        for (i32 sig = 1; own; sig++, own >>= 1) {
            if (own & 1) os_kill(os, r.pid, sig);
        }
    }
    i32 code = 0;
    GetExitCodeProcess(r.proc, &code);
    if (job) {
        JobLimits l = {};
        l.flags = os->jobflags;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &l, sizeof(l));
    }
    ExitProcess(code);
}

static i32 os_forkexec(Os *, Arena, Spawn *)
{
    return E_NOSYS;  // no fork: the shell starts the program and waits
}

// Waits on the process and the wake queue together, so that helpers
// and interpositions keep going meanwhile. Signals do not end it.
static i32 os_waitproc(Os *os, Handle proc)
{
    flushw(os);
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
    flushw(os);
    io->done   = 0;
    io->next   = 0;
    io->plt[0] = S_NONE;
    io->plt[1] = 0;
    drainwake(os);
    if (io->op==IO_PROC || io->op==IO_PROCSTOP) {  // nothing stops here
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
    if (io->op==IO_CONNECT && (!w || !w->pname[0])) {
        io->err  = E_INVAL;  // not a named pipe's server
        io->done = 1;
        return;
    }
    if (!w || w->kind==K_CHAR || w->kind==K_CONOUT ||
            (w->kind==K_CONIN && io->op==IO_WRITE)) {
        // A file, a device, console output: at once (pipes in either
        // direction and console input go through the state machine)
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
    if (deadline != WAIT_POLL) flushw(os);  // it may block
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
    flushw(os);
    drainwake(os);  // perhaps completed meanwhile
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
        if (io->op == IO_CONNECT) {
            // Become the client, which ends the helper's wait, unless a
            // real one came first: then it completes by itself
            i32 access = w->pipeout ? GENERIC_READ : GENERIC_WRITE;
            uz  c = CreateFileW(w->pname, access, 0, 0, OPEN_EXISTING, 0, 0);
            if (c == INVALID_HANDLE_VALUE) return 0;
            CloseHandle(c);
            w->active = 0;
            os->nwait--;
            io->plt[0] = S_NONE;
            return 1;
        }
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

static u64 os_entropy(Os *os)
{
    if (!os->gentried) {
        os->gentried = 1;
        uz m = LoadLibraryW(u"advapi32.dll");
        if (m) {
            void *f = GetProcAddress(m, "SystemFunction036");
            os->genrandom = (u8 (__stdcall *)(void *, u32))f;
        }
    }
    u64 r = 0;
    if (os->genrandom && os->genrandom(&r, sizeof(r))) {
        return r;
    }
    u64 x = (u64)os_now(os) ^ (u64)GetCurrentProcessId()<<40;
    x += ++os->entropyn * 0x9e3779b97f4a7c15u;
    x  = (x ^ x>>30) * 0xbf58476d1ce4e5b9u;
    x  = (x ^ x>>27) * 0x94d049bb133111ebu;
    return x ^ x>>31;
}

// Open the clipboard, loading user32 first if need be: 0, E_NOSYS
// without it, or E_ACCES while another program keeps it open (about
// 150 ms of retries, as the holder usually lets go at once).
static i32 clipopen(Os *os)
{
    if (!os->cliptried) {
        os->cliptried = 1;
        uz m = LoadLibraryW(u"user32.dll");
        if (m) {
            os->openclip  = (b32 (__stdcall *)(uz))GetProcAddress(m, "OpenClipboard");
            os->closeclip = (b32 (__stdcall *)())GetProcAddress(m, "CloseClipboard");
            os->emptyclip = (b32 (__stdcall *)())GetProcAddress(m, "EmptyClipboard");
            os->getclip   = (uz (__stdcall *)(i32))GetProcAddress(m, "GetClipboardData");
            os->setclip   = (uz (__stdcall *)(i32, uz))GetProcAddress(m, "SetClipboardData");
        }
    }
    if (!os->openclip || !os->closeclip || !os->emptyclip || !os->getclip || !os->setclip) {
        return E_NOSYS;
    }
    for (i32 i = 0; i < 20; i++) {
        if (os->openclip(0)) return 0;
        Sleep(i<5 ? 1 : 10);
    }
    return E_ACCES;
}

enum { CF_UNICODETEXT = 13, GMEM_MOVEABLE = 2 };

static ClipText os_clipget(Os *os, Arena *a)
{
    ClipText r = {};
    r.err = clipopen(os);
    if (r.err) return r;
    r.text = Str("");  // present, though perhaps empty
    uz h = os->getclip(CF_UNICODETEXT);  // synthesized from CF_TEXT
    c16 *s = h ? (c16 *)GlobalLock(h) : 0;
    if (s) {
        iz max = GlobalSize(h) / 2;  // do not trust the terminator
        iz len = 0;
        for (; len<max && s[len]; len++) {}
        r.text = fromwide(a, s, len);
        GlobalUnlock(h);
    }
    os->closeclip();
    return r;
}

static i32 os_clipset(Os *os, Arena scratch, Str text)
{
    Str16 w = towide(&scratch, text);
    iz    n = 0;
    for (; n<w.len && w.data[n]; n++) {}  // readers stop at a NUL anyway
    uz h = GlobalAlloc(GMEM_MOVEABLE, (n+1)*2);
    c16 *d = h ? (c16 *)GlobalLock(h) : 0;
    if (!d) {
        if (h) GlobalFree(h);
        return E_NOMEM;
    }
    for (iz i = 0; i < n; i++) d[i] = w.data[i];
    d[n] = 0;
    GlobalUnlock(h);
    i32 err = clipopen(os);
    if (err) {
        GlobalFree(h);
        return err;
    }
    // Owned by the system once set; else still ours to free
    if (!os->emptyclip() || !os->setclip(CF_UNICODETEXT, h)) {
        GlobalFree(h);
        err = E_ACCES;
    }
    os->closeclip();
    return err;
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

static WinDate windate(SysTime t)
{
    WinDate d = {};
    d.year   = t.year;
    d.month  = t.month;
    d.dow    = t.dow;
    d.day    = t.day;
    d.hour   = t.hour;
    d.minute = t.minute;
    d.second = t.second;
    d.ms     = t.ms;
    return d;
}

// The system zone as a POSIX TZ string (the core's wintz). Vista and
// later report whether automatic daylight saving time is off, which
// GetTimeZoneInformation (XP's only call) leaves out.
static Str os_localtz(Os *, Arena *a)
{
    typedef i32 __stdcall Dynamic(TzInfo *);
    Dynamic *getdynamic = (Dynamic *)dynamic(u"kernel32.dll",
                                             "GetDynamicTimeZoneInformation");
    TzInfo t = {};
    i32 r = getdynamic ? getdynamic(&t) : GetTimeZoneInformation(&t);
    if (r == -1) {
        return "UTC0";  // TIME_ZONE_ID_INVALID
    }
    t.stdname[countof(t.stdname)-1] = 0;
    t.dstname[countof(t.dstname)-1] = 0;
    WinZone w = {};
    w.bias    = t.bias;
    w.stdbias = t.stdbias;
    w.dstbias = t.dstbias;
    w.stddate = windate(t.stddate);
    w.dstdate = windate(t.dstdate);
    w.stdname = fromwidez(a, t.stdname);
    w.dstname = fromwidez(a, t.dstname);
    w.nodst   = getdynamic && t.nodst;
    return wintz(a, &w);
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

    os.now = GetSystemTimeAsFileTime;
    os.queryfile = (NtQueryInformationFileFn)dynamic(u"ntdll.dll", "NtQueryInformationFile");
    void *precise = dynamic(u"kernel32.dll", "GetSystemTimePreciseAsFileTime");
    if (precise) {
        os.now = (void (__stdcall *)(Filetime *))precise;  // Windows 8
        os.lateattach = 1;
    }
    os.queryproc = (NtQueryInformationProcessFn)dynamic(u"ntdll.dll", "NtQueryInformationProcess");
    os.iswow64 = (b32 (__stdcall *)(uz, b32 *))dynamic(u"kernel32.dll", "IsWow64Process");
    b32 wow64 = 0;
    if (sizeof(uz)==4 && os.iswow64 && os.iswow64(GetCurrentProcess(), &wow64) && wow64) {
        os.queryproc64 = (NtQueryInformationProcessFn)dynamic(u"ntdll.dll", "NtWow64QueryInformationProcess64");
        os.read64      = (NtWow64ReadFn)dynamic(u"ntdll.dll", "NtWow64ReadVirtualMemory64");
        if (!os.queryproc64 || !os.read64) os.queryproc64 = 0;
    }
    os.xp = !dynamic(u"kernel32.dll", "GetTickCount64");  // Vista
    i32 conpid = 0;
    os.noconsole = !GetConsoleProcessList(&conpid, 1);
    i64 freq = 0;
    QueryPerformanceFrequency(&freq);
    os.freq       = freq;
    os.youngticks = freq / 10;
    if (os.lateattach) setupjobs(&os);
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

    c16 windir[260];
    i32 windirlen = GetSystemWindowsDirectoryW(windir, countof(windir));
    if (windirlen>0 && windirlen<countof(windir)) {
        Str s = fromwide(&a, windir, windirlen);
        for (iz i = 0; i < s.len; i++) {
            s[i] = s[i]=='\\' ? '/' : s[i];
        }
        info.windir = s;
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
    chanopen(&os, info.pid);
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
            h = os_dup(&os, h);
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
