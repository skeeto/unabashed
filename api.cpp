// Platform API: implemented by each platform layer (main_*.cpp, vos.cpp)
// This is free and unencumbered software released into the public domain.

using Handle = uz;  // fd, HANDLE, or virtual object
enum : Handle { BADHANDLE = (Handle)-1 };

struct Fiber;

// A descriptor above 2 the shell inherited from its parent.
struct InheritFd {
    i32    fd;
    Handle h;      // the shell's own reference to it
    i32    flags;  // OF_READ, OF_WRITE (both where the platform cannot tell)
};

// A region holding a state image, which a self-spawned child shell
// continues from (`&`, see notes/m5b.md): it sits at the same address
// in parent and child, and the child's store goes on in it.
struct ImageRef {
    byte  *base;  // null: none
    iz     cap;   // reserved from base
    iz     len;   // filled from base
};

struct OsInfo {
    ImageRef image;  // from a self-spawning parent: start from it
    b32    windows;  // drive letters, ; in PATH, .exe suffixes
    Str   *args;
    iz     nargs;
    Str   *env;      // "NAME=value", UTF-8 (WTF-8 on Windows)
    iz     nenv;
    Str    cwd;      // absolute, native form
    Str    exe;      // absolute path of the shell's own image, native
                     // form ("C:/x/sh.exe"), empty if unknown
    Str    cmdline;  // Windows: the raw command line, UTF-8 (WTF-8)
    Str    windir;   // Windows: the Windows directory, native form
                     // ("C:/Windows"), for SYSTEMROOT and ComSpec
    i64    pid;
    i64    ppid;
    i64    uid;
    i64    euid;
    i64    gid;
    i64    egid;
    Str    host;
    Handle std[3];
    InheritFd *inherit;  // descriptors above 2 from the parent: those open
    iz     ninherit;     // (POSIX), or the CRT's lpReserved2 blob (Windows)
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

enum : i32 {
    NP_OUT   = 1,  // the server writes, its client reads
    NP_NOBUF = 2,  // NP_OUT: a write completes only as the client reads it
};

struct NamedPipe {
    Handle h;     // the server's end
    Str    name;  // canonical native path, //./pipe/NAME
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
    i64 ctime;   // status change
    i64 blocks;  // 512-byte units allocated
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
    Str      cmdline; // Windows: the whole command line, UTF-8, built by
                      // the core (spawn.cpp) and passed verbatim with path
                      // as the image (ComSpec for a batch file); the vos
                      // also takes it empty, quoting argv itself
    i32      crtslot; // Windows: handle slot width (4 or 8) for passing
                      // descriptors 3 and up in the CRT's lpReserved2
                      // blob, or 0 to pass only 0-2
    ImageRef image;   // with a base: start this shell's own program
                      // (path and cmdline ignored) continuing from the
                      // image, every descriptor listed passed; once
                      // started the child owns it (the parent's copy may
                      // be gone), else the caller frees it
};

struct LevelSig {
    i64 level;  // the virtual process's pid
    i32 sig;
};

struct Spawned {
    i32    err;
    Handle proc;
    i64    pid;      // for kill and $!
};

// Signal numbers are Linux's on every platform. A process killed by a
// signal has status 128+N with WSIGNALED set (os_waitproc, and sh_main
// when the shell itself dies by one); the core masks it off for $?.
// WMISSED (os_waitproc): a group signal (os_siggroup) came while the
// program ran that it could not receive (Windows: it was still
// starting), so the waiting level takes it as its own.
// WNOLIB (os_waitproc, Windows): the program could not start for a
// missing DLL; the status is 127 and the core reports it as Cygwin.
enum : i32 { NSIGNAL = 65, WSIGNALED = 0x100, WMISSED = 0x200, WNOLIB = 0x400 };

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
    IO_CONNECT,   // a named pipe's client opened it (os_namedpipe): n = 0
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
// Windows: an instance of a named pipe for one client, under a fresh
// name in the //./pipe/ namespace, or another instance of name (then
// returned as is). A client opening the name gets the oldest instance
// not yet taken. The server's end writes to its client (NP_OUT) or
// reads what it writes. IO_CONNECT on it completes once its client has
// opened it. E_NOSYS elsewhere.
static NamedPipe os_namedpipe(Os *, Arena *, Str name, i32 how);
// Bytes a fresh pipe accepts before a write blocks.
static iz      os_pipecap(Os *);
// Copy up to len bytes that a read would return now, without
// consuming them: the count, 0 when nothing is waiting (or at end of
// file), -1 where the platform cannot look ahead on this handle.
static iz      os_peek(Os *, Handle, u8 *, iz);
// Reposition; whence is 0 (set), 1 (current) or 2 (end). -1 on error.
static i64     os_seek(Os *, Handle, i64, i32 whence);
static Stat    os_stat(Os *, Arena scratch, Str path, b32 follow);
static Stat    os_fstat(Os *, Handle);
// A terminal, as isatty(3): a console on Windows (not a mintty pipe)
static b32     os_isatty(Os *, Handle);
// Directory entries in the platform's order, or null with *err set.
static Dirent *os_list(Os *, Arena *, Str dir, i32 *err);
// Start a program. The child's descriptors are exactly those listed.
static Spawned os_spawn(Os *, Arena scratch, Spawn *);
// Reserve a region for a child's state image (Spawn.image), where a
// fresh process of this shell can reserve it again: base null where
// the platform cannot self-spawn so. The core commits and fills it.
static ImageRef os_imagenew(Os *, iz cap);
// Release a region os_imagenew made that no child took.
static void    os_imagefree(Os *, ImageRef);
// Replace the process with a program, as os_spawn would start it (its
// descriptors exactly those listed). Returns only on failure, with the
// process as it was: an E_* error, E_NOSYS where the platform cannot.
static i32     os_exec(Os *, Arena scratch, Spawn *);
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
// Send signal sig to virtual process level (a subshell's BASHPID, not
// a process of its own) of our own shell process pid, through the
// signal channel: its os_levelsig returns it. Returns 0 or an E_*
// error: E_SRCH if no such shell, E_NOSYS where there is no channel.
static i32     os_siglevel(Os *, i64 pid, i64 level, i32 sig);
// The next signal sent to one of this process's levels (os_siglevel),
// taken, or sig 0. os_wait returns null early when one arrives.
static LevelSig os_levelsig(Os *);
// Signals sent to this process since the last call, as a mask (bit n-1
// for signal n), and cleared. os_wait returns null early when one
// arrives. The core decides what each does (traps, default actions).
static u64     os_sigpending(Os *);
// Signals the terminal sent to the whole foreground process group
// (Ctrl-C at a console) since the last call, as a mask, and cleared.
// Every level of the shell gets them, as every process of bash's
// foreground job would. os_wait returns null early when one arrives.
static u64     os_siggroup(Os *);
// How the process itself handles a real signal (SIGM_*): the main
// shell's trap table. Signals ignored on entry stay ignored. Layers
// that deliver every signal through os_sigpending ignore it.
static void    os_sigmode(Os *, i32 sig, i32 mode);
// Realtime clock, in ns since the epoch.
static i64     os_now(Os *);
// The system's time zone, for local time while TZ is not in the
// environment, in TZ's own syntax: a POSIX TZ string ("EST5EDT,M3.2.0,
// M11.1.0"), or ':' and the absolute path of a zoneinfo file.
static Str     os_localtz(Os *, Arena *);
// The currently running fiber, converting the thread on first use.
static Fiber  *os_self(Os *);
// Create a suspended fiber running fn(os, arg), which must never
// return. The record is allocated in the arena.
static Fiber  *os_fiber(Os *, Arena *, iz stack, void (*)(Os *, void *), void *);
// Switch from the current fiber to another.
static void    os_switch(Os *, Fiber *);
// Release a fiber other than the current one.
static void    os_fiberfree(Os *, Fiber *);
// Test layers, for the chaos fuzzer (no-ops elsewhere): the running
// task's identity, the same under every chaos seed, how many tasks the
// scheduler has, and its writer group (the tag of the nearest task not
// waited for in place, as a procsub, or 0: writes in different groups
// are not ordered with each other); called when any changes.
static void    os_tasktag(Os *, u64 tag, iz ntasks, u64 group);
// Test layers: what the shell just did may legitimately depend on the
// schedule (a write that found no reader), so runs under different
// chaos seeds need not agree.
static void    os_racy(Os *);
// Entry point, called by the platform after it fills in OsInfo.
static i32     sh_main(Os *, OsInfo *);
