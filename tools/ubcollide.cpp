// ubcollide: a C++ host defining names the library uses inside, which
// must link beside it without clashing (make check-lib).
// This is free and unencumbered software released into the public domain.
#include <signal.h>
#include "../unabashed.h"

template<typename T>
void *operator new(decltype(sizeof(0)), T *p) noexcept { return p; }

struct Str   { char const *data; long len; long size() { return len; } };
struct Arena { char *beg, *end; };
struct Os    { int x; };

extern "C" void fiberswap(void **, void *) {}
extern "C" void fiberstart() {}
extern "C" void fiberentry() {}
extern "C" int  jmpsetasm(void *) { return 0; }
extern "C" void jmpbackasm(void *) {}
void os_oom() {}
int  sh_main(Os *, void *) { return 0; }

int main()
{
    signal(SIGPIPE, SIG_IGN);  // macOS: the host ignores these
    signal(SIGXFSZ, SIG_IGN);
    Str   s = {"true", 4};
    Arena a = {};
    Os    o = {};
    new (&o) Os{};
    fiberswap(0, 0);
    os_oom();
    return ub_system(s.data) + (int)s.size() - 4 + sh_main(&o, &a);
}
