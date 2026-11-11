// macOS platform root: the executable
// $ c++ -std=c++20 -O2 -fno-exceptions -fno-rtti -fno-threadsafe-statics
//       -nostdlib++ -o unabashed main_darwin.cpp
// (only libc: libc++ would cost half of startup, for nothing)
// This is free and unencumbered software released into the public domain.
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
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
#include <sys/random.h>  // getentropy
extern "C" int rename(char const *, char const *);  // stdio.h, whose macros collide
extern char **environ;
#include "src/unabashed.cpp"
#if __x86_64__
#include "src/fibers_amd64.cpp"
#else
#include "src/fibers_arm64.cpp"
#endif
#include "src/fibers_posix.cpp"
#include "src/posix.cpp"
#include "src/darwin.cpp"
#include "src/posixstart.cpp"
