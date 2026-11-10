// libunabashed: run bash scripts inside a host process
//
// One run is one "bash" process: ub_run(os, cmd) runs bash with cmd's
// arguments on the calling thread and returns its status. Programs it
// starts are real processes; everything else (subshells, pipelines,
// command substitutions) runs inside the run. When the run ends,
// whatever it started has ended too. See notes/library.md.
//
// Build: libunabashed.cpp is a single translation unit, compiled with a
// C++20 compiler as an object to link into a C or C++ host (its header
// has the command). It needs only libc, and exports only ub_*.
//
// This is free and unencumbered software released into the public domain.
#ifndef UNABASHED_H
#define UNABASHED_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ub_os ub_os;

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
    char const *root;     // Windows: the install root; 0: none
    char const *self;     // Windows: an unabashed executable; 0: none
    ptrdiff_t   reserve;  // address space per arena; 0: 1<<34, or 1<<26 on 32-bit
    ptrdiff_t   stack;    // the shell fiber; 0: 16 MiB, or 8 MiB on 32-bit
    ptrdiff_t   memlimit; // committed bytes per run; 0: min(1 GiB, RAM/4),
                          // 256 MiB on 32-bit; negative: no limit
    int         fds;      // descriptors per run; 0: min(256, RLIMIT_NOFILE/4)
    int         flags;    // UB_SHARECONSOLE
} ub_config;

typedef struct { int fd; intptr_t h; } ub_fd;

typedef struct {
    int                argc;
    char const *const *argv;    // bash's argv, UTF-8: argv[0] is $0
    char const        *cmdline; // Windows: a raw command line; replaces argv
    char const *const *env;     // "NAME=value", null-terminated; 0: the host's
    char const        *cwd;     // absolute; 0: the host's
    intptr_t           std[3];  // fd or HANDLE; -1: the null device
    int                nfds;    // more descriptors, as if inherited
    ub_fd const       *fds;     // (fd 3 and up)
    int64_t            pid;     // $$; 0: a unique virtual pid
    int64_t            ppid;    // $PPID; 0: the host's pid
    uint64_t           sigign;  // bit n-1: signal n ignored at entry
} ub_cmd;

enum { UB_GROUP = 0x100 };      // or'd into ub_signal's sig
enum { UB_SIGNALED = 0x100 };   // status: the shell died of signal (s&0xff)-128
enum { UB_SHARECONSOLE = 1 };   // ub_config.flags
enum {                          // errors, negated; Linux numbering everywhere
    UB_EBADF = 9, UB_ECHILD = 10, UB_ENOMEM = 12, UB_EBUSY = 16,
    UB_EINVAL = 22, UB_EDEADLK = 35, UB_ECANCELED = 125,
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
