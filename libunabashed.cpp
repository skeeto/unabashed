// libunabashed: the shell as a library (unabashed.h), POSIX
// $ c++ -std=c++20 -O2 -fno-exceptions -fno-rtti -fno-threadsafe-statics \n//       -c libunabashed.cpp
// Link the object into a C or C++ host with the C compiler: it needs
// only libc, and exports only ub_*.
//
// Every name of the shell's sits in an anonymous namespace inside
// unabashed, so none merges with a host's, not even the inline members
// of its types. Outside it: system headers, and the placement new,
// which must be global.
// This is free and unencumbered software released into the public domain.
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#if __APPLE__
#include <libproc.h>
#include <sys/random.h>  // getentropy
#endif
#if FIBER_UCONTEXT || !(__x86_64__ || (__APPLE__ && __aarch64__))
#include <ucontext.h>    // fibers.cpp's, here outside the namespace
#endif
#include "unabashed.h"
extern "C" int rename(char const *, char const *);  // stdio.h, whose macros collide
extern char **environ;

template<typename T>
[[gnu::always_inline]] inline void *operator new(decltype(sizeof(0)), T *p) noexcept
{
    return p;
}

#define UB_LIBRARY 1
namespace unabashed {
namespace {
#include "src/unabashed.cpp"
#include "src/fibers.cpp"
#include "src/posix.cpp"
#include "src/library.cpp"

[[noreturn]] static void os_oom()
{
    Lib *l = curlib;
    if (!l) __builtin_trap();
    lib_abort(&l->os, E_NOMEM);
}

static OsVtable const *osvtable(Os *)
{
    return &libvtable;
}
}
}

using namespace unabashed;

ub_os *ub_new(ub_config const *c)
{
    return (ub_os *)libnew(c);
}

int ub_run(ub_os *os, ub_cmd const *cmd)
{
    return librun((Lib *)os, cmd);
}

int ub_signal(ub_os *os, int sig)
{
    return libsignal((Lib *)os, sig);
}

void ub_cancel(ub_os *os)
{
    libcancel((Lib *)os);
}

void *ub_user(ub_os *os)
{
    return ((Lib *)os)->cfg.user;
}

void ub_free(ub_os *os)
{
    libfree((Lib *)os);
}

int ub_system(char const *cmd)
{
    ub_os *os = ub_new(0);
    if (!os) return -UB_ENOMEM;
    char const *argv[] = {"bash", "-c", cmd, 0};
    ub_cmd c = {};
    c.argc   = 3;
    c.argv   = argv;
    c.std[0] = 0;
    c.std[1] = 1;
    c.std[2] = 2;
    int r = ub_run(os, &c);
    ub_free(os);
    return r;
}
