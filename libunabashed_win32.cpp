// libunabashed: the shell as a library (unabashed.h), on Windows
// $ c++ -std=c++20 -O2 -fno-exceptions -fno-rtti -fno-threadsafe-statics
//       -c libunabashed_win32.cpp
// Link the object into a C or C++ host with the C compiler. It needs
// only kernel32 (and advapi32 at run time), plus memcpy, memmove,
// memset, memcmp and strlen from the host's CRT or its own
// (tools/ubhostw.c), and libgcc on i686. It exports only ub_*.
//
// Every name of the shell's sits in an anonymous namespace inside
// unabashed, so none merges with a host's, not even the inline members
// of its types. Outside it: system headers, and the placement new,
// which must be global.
// This is free and unencumbered software released into the public domain.
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
#include "src/win32.cpp"      // and fibers_win32.cpp, after its declarations
#include "src/libwin32.cpp"
}
}
#include "src/ub.cpp"
