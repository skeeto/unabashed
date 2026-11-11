// Fibers for the roots built on every platform (main_test.cpp,
// main_fuzz.cpp). The platform roots name their own (main_linux.cpp,
// main_darwin.cpp, src/win32.cpp).
// This is free and unencumbered software released into the public domain.
#if _WIN32
#include "fibers_win32.cpp"
#else
#if __x86_64__ && !FIBER_UCONTEXT
#include "fibers_amd64.cpp"
#elif __APPLE__ && __aarch64__
#include "fibers_arm64.cpp"
#else
#include <ucontext.h>
#include "fibers_ucontext.cpp"
#endif
#include "fibers_posix.cpp"
#endif
