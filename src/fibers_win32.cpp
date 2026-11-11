// Fibers for the Win32 layer: Win32 fibers (XP and later). The layer
// stores a Fibers in its struct Os and defines osfibers() to return it.
// This is free and unencumbered software released into the public domain.

#define FIBERAPI(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
FIBERAPI(void *, ConvertThreadToFiber(void *));
FIBERAPI(b32,    ConvertFiberToThread());
FIBERAPI(void *, CreateFiberEx(uz, uz, i32, void (__stdcall *)(void *), void *));
FIBERAPI(void,   DeleteFiber(void *));
FIBERAPI(void,   SwitchToFiber(void *));

struct Fiber {
    void  *handle;
    Os    *os;
    void (*fn)(Os *, void *);
    void  *arg;
};

// The library's (libwin32.cpp): the run's fibers, made and deleted
static void lib_fiber(Os *, void *handle);
static void lib_unfiber(Os *, void *handle);

struct Fibers {
    Fiber *current;
    Fiber  main;
};

// Defined by the including layer.
static Fibers *osfibers(Os *);

__attribute((force_align_arg_pointer))
static void __stdcall fiberentry(void *arg)
{
    Fiber *f = (Fiber *)arg;
    f->fn(f->os, f->arg);
    __builtin_trap();  // fibers never return
}

static Fiber *os_self(Os *os)
{
    Fibers *fs = osfibers(os);
    if (!fs->current) {
        if constexpr (LIBMODE) __builtin_trap();  // the run's setup made it
        fs->main.handle = ConvertThreadToFiber(0);
        fs->main.os     = os;
        fs->current     = &fs->main;
    }
    return fs->current;
}

static Fiber *os_fiber(Os *os, Arena *a, iz stack, void (*fn)(Os *, void *), void *arg)
{
    os_self(os);
    Fiber *f = alloc<Fiber>(a);
    f->os  = os;
    f->fn  = fn;
    f->arg = arg;
    f->handle = CreateFiberEx(1<<16, (uz)stack, 0, fiberentry, f);
    if (!f->handle) {
        os_oom();
    }
    if constexpr (LIBMODE) lib_fiber(os, f->handle);  // deleted if the run ends
    return f;
}

static void os_switch(Os *os, Fiber *f)
{
    osfibers(os)->current = f;
    SwitchToFiber(f->handle);
}

// A switch away for good: the fiber left is never resumed (the library).
[[maybe_unused]] static void os_switchfinal(Os *os, Fiber *f)
{
    os_switch(os, f);
}

[[maybe_unused]] static void os_fibersend(Os *) {}

static void os_fiberfree(Os *os, Fiber *f)
{
    assert(f != osfibers(os)->current);
    if constexpr (LIBMODE) lib_unfiber(os, f->handle);
    DeleteFiber(f->handle);
}
