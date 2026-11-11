// libunabashed: the shell as a library (unabashed.h), on macOS
// $ c++ -std=c++20 -O2 -fno-exceptions -fno-rtti -fno-threadsafe-statics
//       -c libunabashed_darwin.cpp
// Link the object into a C or C++ host with the C compiler. It needs
// only libc. It exports only ub_*.
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
#include <libproc.h>
#include <sys/event.h>
#include <sys/random.h>  // getentropy
extern "C" int rename(char const *, char const *);  // stdio.h, whose macros collide
extern char **environ;
#include "unabashed.h"

template<typename T>
[[gnu::always_inline]] inline void *operator new(decltype(sizeof(0)), T *p) noexcept
{
    return p;
}

#define UB_LIBRARY 1
namespace unabashed {
namespace {
#include "src/unabashed.cpp"
#if __x86_64__
#include "src/fibers_amd64.cpp"
#else
#include "src/fibers_arm64.cpp"
#endif
#include "src/fibers_posix.cpp"
#include "src/posix.cpp"
#include "src/darwin.cpp"
#include "src/libposix.cpp"
#include "src/libdarwin.cpp"
}
}
#include "src/ub.cpp"
