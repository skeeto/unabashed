// Platform API: implemented by each platform layer (main_*.cpp, vos.cpp)
// This is free and unencumbered software released into the public domain.

using Handle = uz;  // fd, HANDLE, or virtual object
enum : Handle { BADHANDLE = (Handle)-1 };

struct Fiber;

struct OsInfo {
    b32    windows;  // drive letters, ; in PATH, .exe suffixes
    Str   *args;
    iz     nargs;
    Str   *env;      // "NAME=value", UTF-8 (WTF-8 on Windows)
    iz     nenv;
    Str    cwd;      // absolute, native form
    i64    pid;
    i64    ppid;
    i64    uid;
    i64    euid;
    i64    gid;
    i64    egid;
    Str    host;
    Handle std[3];
    iz     reserve;  // address space for each large arena
    iz     stack;    // usable bytes of the calling thread's stack, 0 if unknown
    u64    chaos;    // test layers: seed for random yields, 0 for none
    u64    sigign;   // signals ignored on entry (bit n-1 for signal n)
    b32    ownstd;   // test layers: std are the shell's own references,
                     // closed (with every descriptor) as it returns
};

// Errors are Linux errno numbers on every platform.
enum : i32 {
    E_PERM = 1, E_NOENT = 2, E_SRCH = 3, E_INTR = 4, E_IO = 5,
    E_NXIO = 6, E_2BIG = 7, E_NOEXEC = 8, E_BADF = 9, E_CHILD = 10,
    E_AGAIN = 11, E_NOMEM = 12, E_ACCES = 13, E_FAULT = 14, E_BUSY = 16,
    E_EXIST = 17, E_XDEV = 18, E_NODEV = 19, E_NOTDIR = 20, E_ISDIR = 21,
    E_INVAL = 22, E_NFILE = 23, E_MFILE = 24, E_NOTTY = 25, E_TXTBSY = 26,
    E_FBIG = 27, E_NOSPC = 28, E_SPIPE = 29, E_ROFS = 30, E_MLINK = 31,
    E_PIPE = 32, E_DOM = 33, E_RANGE = 34, E_DEADLK = 35,
    E_NAMETOOLONG = 36, E_NOSYS = 38, E_NOTEMPTY = 39, E_LOOP = 40,
};

enum : i32 {  // os_open flags
    OF_READ   = 1<<0,
    OF_WRITE  = 1<<1,
    OF_CREATE = 1<<2,
    OF_TRUNC  = 1<<3,
    OF_APPEND = 1<<4,
    OF_EXCL   = 1<<5,
};

enum : i32 {  // file types
    FT_NONE, FT_FILE, FT_DIR, FT_LINK, FT_FIFO, FT_CHAR, FT_BLOCK,
    FT_SOCK, FT_PIPE,
};

struct Opened {
    Handle h;
    i32    err;
};

struct OsPipe {
    Handle r;
    Handle w;
    i32    err;
};

struct Stat {
    i32 err;
    i32 type;    // FT_*
    i32 mode;    // permission bits, including setuid, setgid, sticky
    i32 nlink;
    i64 size;
    i64 mtime;   // ns since the epoch
    i64 atime;
    i64 dev;
    i64 ino;
    i64 uid;
    i64 gid;
};

struct Dirent {
    Dirent *next;
    Str     name;
    i32     type;  // FT_*, or FT_NONE if unknown
};

struct SpawnFd {
    i32    fd;     // descriptor number in the child
    Handle h;      // parent handle it refers to
};

struct Spawn {
    Str      path;   // absolute path of the program
    Str     *argv;
    iz       argc;
    Str     *env;    // "NAME=value"
    iz       nenv;
    Str      cwd;
    SpawnFd *fds;
    iz       nfds;
    u64      sigign; // signals the child starts out ignoring (bit n-1
                     // for signal n); others it starts at their default
};

struct Spawned {
    i32    err;
    Handle proc;
    i64    pid;      // for kill and $!
};

// Signal numbers are Linux's on every platform. A process killed by a
// signal has status 128+N with WSIGNALED set (os_waitproc, and sh_main
// when the shell itself dies by one); the core masks it off for $?.
enum : i32 { NSIGNAL = 65, WSIGNALED = 0x100 };

enum : i32 {  // os_sigmode
    SIGM_DEFAULT, SIGM_IGNORE, SIGM_CATCH,
};

// An operation submitted for asynchronous completion, so that a task
// can block on it while other tasks run (see notes/m3a.md). The core
// fills op, h, buf and len and keeps the record in place until it
// completes; the platform may use the rest.
enum : i32 {
    IO_READ = 1,  // at least one byte, or end of file (n = 0)
    IO_WRITE,     // at least one byte (n), perhaps fewer than len
    IO_PROC,      // a spawned program's exit: n is os_waitproc's status
};

struct Io {
    i32    op;       // IO_*
    Handle h;        // descriptor, or the process for IO_PROC
    u8    *buf;
    iz     len;
    iz     n;        // result
    i32    err;      // E_* on failure, such as E_PIPE
    b32    done;
    void  *ctx;      // the core's: the waiting task
    Io    *next;     // the platform's: pending list
    uz     plt[4];   // the platform's
};

enum : i64 { WAIT_FOREVER = -1, WAIT_POLL = 0 };

// Reserve address space. Returns null on failure.
static byte   *os_reserve(Os *, iz);
// Commit part of a reservation. Returns false on failure.
static b32     os_commit(Os *, byte *, iz);
// Write all bytes, returning false on error.
static b32     os_write(Os *, Handle, u8 *, iz);
// Read up to len bytes. Returns 0 at end of file, -1 on error.
static iz      os_read(Os *, Handle, u8 *, iz);
// Open a file by absolute path. Mode applies when creating.
static Opened  os_open(Os *, Arena scratch, Str path, i32 flags, i32 mode);
// A new empty file, open for reading and writing, with no name: it is
// deleted once its last handle closes (large here-documents).
static Opened  os_tempfile(Os *, Arena scratch);
static void    os_close(Os *, Handle);
// A second handle to the same open file.
static Handle  os_dup(Os *, Handle);
static OsPipe  os_pipe(Os *);
// Bytes a fresh pipe accepts before a write blocks.
static iz      os_pipecap(Os *);
// Reposition; whence is 0 (set), 1 (current) or 2 (end). -1 on error.
static i64     os_seek(Os *, Handle, i64, i32 whence);
static Stat    os_stat(Os *, Arena scratch, Str path, b32 follow);
static Stat    os_fstat(Os *, Handle);
// Directory entries in the platform's order, or null with *err set.
static Dirent *os_list(Os *, Arena *, Str dir, i32 *err);
// Start a program. The child's descriptors are exactly those listed.
static Spawned os_spawn(Os *, Arena scratch, Spawn *);
// Wait for a spawned program: 0-255 on exit, 128+N|WSIGNALED on signal N.
static i32     os_waitproc(Os *, Handle proc);
// Begin an operation without blocking. It may complete at once (done
// set), else a later os_wait returns it. A pending IO_PROC releases
// the process when it completes, as os_waitproc does.
static void    os_submit(Os *, Io *);
// Block until a submitted operation completes and return it, or null
// at the deadline (WAIT_POLL to poll, or an os_now time). The virtual
// OS also returns null when nothing can ever complete (deadlock).
static Io     *os_wait(Os *, i64 deadline);
// Withdraw a pending operation. False if it cannot be withdrawn (it
// may still complete) or already completed.
static b32     os_cancel(Os *, Io *);
// Send signal sig to process pid, or with 0 check that it exists.
// Returns 0 or an E_* error (E_SRCH, E_PERM, E_INVAL).
static i32     os_kill(Os *, i64 pid, i32 sig);
// Signals sent to this process since the last call, as a mask (bit n-1
// for signal n), and cleared. os_wait returns null early when one
// arrives. The core decides what each does (traps, default actions).
static u64     os_sigpending(Os *);
// How the process itself handles a real signal (SIGM_*): the main
// shell's trap table. Signals ignored on entry stay ignored. Layers
// that deliver every signal through os_sigpending ignore it.
static void    os_sigmode(Os *, i32 sig, i32 mode);
// Realtime clock, in ns since the epoch.
static i64     os_now(Os *);
// The currently running fiber, converting the thread on first use.
static Fiber  *os_self(Os *);
// Create a suspended fiber running fn(os, arg), which must never
// return. The record is allocated in the arena.
static Fiber  *os_fiber(Os *, Arena *, iz stack, void (*)(Os *, void *), void *);
// Switch from the current fiber to another.
static void    os_switch(Os *, Fiber *);
// Release a fiber other than the current one.
static void    os_fiberfree(Os *, Fiber *);
// Entry point, called by the platform after it fills in OsInfo.
static i32     sh_main(Os *, OsInfo *);
