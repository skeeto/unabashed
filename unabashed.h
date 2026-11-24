// libunabashed: run bash scripts inside a host process
//
// One run is one "bash" process: ub_run(os, cmd) runs bash with cmd's
// arguments on the calling thread and returns its status. Programs it
// starts are real processes; everything else (subshells, pipelines,
// command substitutions) runs inside the run. When the run ends,
// whatever it started has ended too. See notes/library.md.
//
// Build: the platform's libunabashed_*.cpp (linux, darwin, win32) is a
// single translation unit, compiled with a C++20 compiler as an object
// to link into a C or C++ host (its header has the command). It exports
// only ub_*.
//
// This is free and unencumbered software released into the public domain.
#ifndef UNABASHED_H
#define UNABASHED_H

#include <stddef.h>
#include <stdint.h>

// The interface's version, 0xMMmmpp: the release that last changed it.
// Its structs only gain fields, at their ends, and in those the host
// fills (ub_config, ub_cmd) zero means the default, or off: a host
// written for an older version, its structs zeroed, builds and runs as
// before. Build the host with the header beside the library's source.
#define UB_VERSION 0x000700

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ub_os ub_os;

typedef struct {                // how a run ended, for ub_config.exit
    int                status;  // what ub_run returns: 0-255, UB_SIGNALED|(128+n)
    char const        *pwd;     // the logical directory, as pwd shows it; 0: none
    char const        *cwd;     // the native one programs start in; 0: none
                                // (Windows: a virtual one, as /proc)
    char const *const *env;     // the exports, "NAME=value", null-terminated,
                                // as ub_cmd.env takes them back (Windows:
                                // as a native program gets them)
    // What the run changed of env, as env (functions too, as
    // "BASH_FUNC_f%%=() {...}"), null-terminated: "NAME=value" for each
    // export set or changed, a bare "NAME" for each unset or unexported.
    // Against the exports as the command starts, after the startup
    // files: a host that applies carried exports to ub_cmd.env sees
    // neither one the profile sets again nor the library's own HOME
    // and PATH, by design. 0: unknown (makeenv escaped, or the run
    // ended within its startup files).
    char const *const *delta;
} ub_exit;

typedef struct {                // a file the script names, for ub_config.access
    int         op;             // UB_OPEN ... UB_TEMPFILE
    int         flags;          // UB_READ ... UB_NOFOLLOW
    char const *path;           // absolute, as the system is given it: POSIX
                                // joined, not resolved (., .. and links
                                // remain); Windows native, forward slashes,
                                // devices as //./NUL. UB_TEMPFILE: the
                                // directory it is made in
} ub_access;
enum {                          // ub_access.op
    UB_OPEN = 1,                // open, flags as asked; also a rename's
                                // target (UB_WRITE|UB_CREATE|UB_TRUNC)
    UB_STAT,                    // examine; UB_NOFOLLOW: not its last link
    UB_READLINK,                // read a link (UB_NOFOLLOW)
    UB_LIST,                    // list a directory (UB_READ)
    UB_TEMPFILE,                // make an anonymous file, as for a
                                // here-document (UB_READ|UB_WRITE|
                                // UB_CREATE|UB_EXCL)
};
enum {                          // ub_access.flags
    UB_READ = 1<<0, UB_WRITE = 1<<1, UB_CREATE = 1<<2, UB_TRUNC = 1<<3,
    UB_APPEND = 1<<4, UB_EXCL = 1<<5, UB_NOFOLLOW = 1<<6,
};

typedef struct {                // a program about to start, for ub_config.spawn
    char const        *path;    // absolute: the image (Windows: ComSpec for
                                // a batch file; in native mode busybox.exe
                                // for one of its alias stubs)
    int                argc;
    char const *const *argv;    // argv[0] first, null-terminated
    char const *const *env;     // "NAME=value", null-terminated
    char const        *cwd;     // absolute; 0: the host's
    char const        *cmdline; // Windows: the line it gets; POSIX: 0
    int64_t            trace;   // its command's ub_trace id, or 0
    // The hook's reply, copied as it returns:
    char const *const *wrap;    // 0, or null-terminated: start wrap[0]
                                // (absolute) instead, with wrap[1...], then
                                // path, then argv (POSIX), or path, then
                                // cmdline as one argument (Windows)
    char const        *reason;  // with a refusal: what the script sees, or 0
} ub_spawn;

typedef struct {                // a simple command, for ub_config.trace
    int                phase;   // UB_TRACE_START, UB_TRACE_END
    int                kind;    // UB_TRACE_BUILTIN ... UB_TRACE_PROGRAM
    int64_t            id;      // 1, 2, ... in START order; END repeats it
    int64_t            level;   // $BASHPID where it runs
    int                argc;    // START: its words, expanded, as set -x
    char const *const *argv;    // shows them; END: 0
    int64_t            pid;     // END: the program it started, or 0
    int                status;  // END: as $? (the program's, if one),
                                // UB_SIGNALED|(128+n) for a signal death
} ub_trace;
enum { UB_TRACE_START = 1, UB_TRACE_END };
enum { UB_TRACE_BUILTIN = 1, UB_TRACE_FUNCTION, UB_TRACE_PROGRAM };

// Hooks run on ub_run's thread, inside the run, and must be brief: one
// that blocks stalls the run, and a cancel takes effect only once it
// returns. Deep in a recursion a hook has only part of the 64 KiB of
// stack the shell keeps in reserve. A hook must not call the library,
// nor unwind out. The strings it is given are the library's, for the
// call only.
typedef struct {
    void       *user;     // the host's: ub_user returns it, hooks receive it
    // Brackets every moment the library holds a handle a spawned program
    // could inherit by accident: a host that spawns under the same lock
    // never leaks one into its own programs. 0: a library spin lock.
    void      (*spawnlock)(void *user, int lock);
    // Optional: each program the run starts (live 1), and later reaped
    // (live 0). POSIX hosts may feed a keeper that kills these if the
    // host dies.
    void      (*child)(void *user, int64_t pid, int live);
    // Optional, Windows: deliver signal sig to program pid, 0 if done.
    int       (*softsig)(void *user, int64_t pid, int sig);
    // Windows: the directory of the install's shell (its bin, as
    // C:/w64devkit/bin), for the persona, fstab, profile, default PATH
    // and command -p; 0: none, built-in defaults
    char const *root;
    // Windows: an unabashed executable, for $BASH, $SHELL and sh or
    // bash scripts; 0: none, and such scripts run in-process
    char const *self;
    ptrdiff_t   reserve;  // address space per arena; 0: 1<<34, or 1<<26 on 32-bit
    ptrdiff_t   stack;    // the shell fiber; 0: 16 MiB, or 8 MiB on 32-bit
    ptrdiff_t   memlimit; // committed bytes per run; 0: min(1 GiB, RAM/4),
                          // 256 MiB on 32-bit; negative: no limit
    int         fds;      // descriptors per run; 0: min(256, RLIMIT_NOFILE/4),
                          // 256 on Windows
    int         flags;    // UB_SHARECONSOLE, UB_SYSROOT
    // Optional: as the run ends, after its EXIT trap, with the state to
    // start the next run from. Not called if it was aborted (ub_run
    // returning a negated error), as by a cancel or out of memory.
    void      (*exit)(void *user, ub_exit const *);
    // Optional: each access to a file the script names (a redirection,
    // source, test, cd, a glob, a PATH search, a script read), but not
    // those the shell makes on its own (a program's first bytes,
    // /dev/null, the password file). 0 allows it; a negated UB_E*
    // refuses it, and any other value refuses it as -UB_EACCES. A
    // refusal fails as that error would, with *reason, if set, as its
    // message: its first line, up to 255 bytes. -UB_ENOENT without a
    // reason looks as if the file is not there.
    int       (*access)(void *user, ub_access const *, char const **reason);
    // Optional: each program the run starts, as it starts. 0 starts it,
    // or a wrapper if it sets wrap (see notes/library.md for what one
    // must do); a negated UB_E* refuses it, and any other value refuses
    // it as -UB_EACCES. A refused program fails as if the system had
    // refused it, with the reason, if set, as the message: status 126,
    // or 127 for -UB_ENOENT. A script without #!, which the shell runs
    // itself, is not asked (the access hook sees it read).
    int       (*spawn)(void *user, ub_spawn *);
    // Optional: each simple command that runs, START once its words
    // expand and END with its status, before ERR and set -e. A command
    // that started a program ends when both it and the program have,
    // with the program's status: a pipeline's stages end as they are
    // reaped, and a program killed as the run ends after the exit hook.
    // A command an escape abandons (exit, set -e, a fatal error) has no
    // END unless it started a program. No START for assignments or
    // redirections alone; a pipeline stage that turns out to be shell
    // code starts once, as that.
    void      (*trace)(void *user, ub_trace const *);
    // Milliseconds the programs still running as a run ends (however it
    // ends) get to exit before they are killed: first sent SIGTERM, and
    // SIGCONT, with their groups (POSIX), or SIGTERM through softsig
    // (Windows; without softsig, none). It waits for the programs the
    // run started, not for what they started. 0: killed at once.
    int         linger;
} ub_config;

typedef struct { int fd; intptr_t h; } ub_fd;

typedef struct {
    int                argc;
    char const *const *argv;    // bash's argv, UTF-8: argv[0] is $0
    char const        *cmdline; // Windows: a raw command line, split as
                                // unabashed.exe splits its own; replaces argv
    char const *const *env;     // "NAME=value", null-terminated; 0: the host's
    char const        *cwd;     // absolute; 0: the host's
    intptr_t           std[3];  // fd or HANDLE; -1 (Windows also 0): the
                                // null device
    int                nfds;    // more descriptors, as if inherited
    ub_fd const       *fds;     // (fd 3 and up)
    int64_t            pid;     // $$; 0: a unique virtual pid; POSIX
                                // below 2^22, Windows below 2^47
    int64_t            ppid;    // $PPID; 0: the host's pid
    uint64_t           sigign;  // bit n-1: signal n ignored at entry
} ub_cmd;

enum { UB_GROUP = 0x100 };      // or'd into ub_signal's sig
enum { UB_SIGNALED = 0x100 };   // status: the shell died of signal (s&0xff)-128
// ub_config.flags, Windows. UB_SYSROOT: with no root, / is the system
// drive's root (the Windows directory's), not the start directory's, so
// a run started on another drive means the same by /
enum { UB_SHARECONSOLE = 1, UB_SYSROOT = 2 };
enum {                          // errors, negated; Linux numbering everywhere
    UB_EPERM = 1, UB_ENOENT = 2, UB_EBADF = 9, UB_ECHILD = 10,
    UB_ENOMEM = 12, UB_EACCES = 13, UB_EBUSY = 16, UB_EEXIST = 17,
    UB_ENOTDIR = 20, UB_EISDIR = 21, UB_EINVAL = 22, UB_EROFS = 30,
    UB_EDEADLK = 35, UB_ECANCELED = 125,
};

// A handle for runs, one at a time. A null config means the defaults.
// Returns null on failure.
ub_os *ub_new(ub_config const *);

// Run bash with cmd on this thread: 0-255 ($?), UB_SIGNALED|(128+n),
// or a negated UB_E* error.
int    ub_run(ub_os *, ub_cmd const *);

// From any thread, at any time, even a signal handler: send signal sig
// (host numbering) to the run, or with UB_GROUP to the run and every
// program it started, as a terminal would. 0, or -UB_EINVAL.
int    ub_signal(ub_os *, int sig);

// From any thread, at any time: end the run promptly, with every
// program it started; ub_run returns -UB_ECANCELED.
void   ub_cancel(ub_os *);

void  *ub_user(ub_os *);
void   ub_free(ub_os *);

// bash -c cmd with the host's environment, directory and std handles.
int    ub_system(char const *cmd);

#ifdef __cplusplus
}
#endif
#endif
