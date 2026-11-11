// Fibers for the POSIX layers, built on a context switch the root
// includes first: fibers_amd64.cpp (x86-64), fibers_arm64.cpp (Apple
// arm64) or fibers_ucontext.cpp (elsewhere), each defining FiberCtx,
// fiberctxinit and fiberctxswap. The including layer stores a Fibers in
// its struct Os and defines osfibers() to return it.
// This is free and unencumbered software released into the public domain.

// AddressSanitizer follows only stacks it is told about: each switch
// names the stack it goes to, and the first one away from the thread's
// own stack learns that stack's bounds (POSIX; a host built with ASan
// builds the library with it too).
#if UB_ASAN  // (base.cpp)
#define FIBER_ASAN 1
#endif
#if FIBER_ASAN
extern "C" void __sanitizer_start_switch_fiber(void **, void const *, decltype(sizeof(0)));
extern "C" void __sanitizer_finish_switch_fiber(void *, void const **, decltype(sizeof(0)) *);
#define FIBER_ASANFIELDS        \
    void const *asanbottom;     \
    uz          asansize;       \
    void       *asanfake;       \
    Fiber      *asanfrom;       \
    Fiber      *asannext;
#else
#define FIBER_ASANFIELDS
#endif

struct Fiber {
    FiberCtx ctx;
    byte    *stack;  // 0 for the thread's own fiber
    iz       size;
    Os      *os;
    void   (*fn)(Os *, void *);
    void    *arg;
    FIBER_ASANFIELDS
};

struct Fibers {
    Fiber *current;
    Fiber  main;
    #if FIBER_ASAN
    Fiber *asanall;  // every fiber os_fiber made, for os_fibersend
    #endif
};

// Defined by the including layer.
static Fibers *osfibers(Os *);
static byte   *fiberstack(Os *, iz size);  // POSIX stack memory
static void    fiberunstack(Os *, byte *, iz size);

#if FIBER_ASAN
// On a fiber just switched to: finish the switch, and learn the bounds
// of the stack it came from if they were unknown (the thread's own).
// Its fake stack is the thread's again, no longer saved with it.
static void fiberarrived(Fiber *f)
{
    void const *bottom = 0;
    uz          size   = 0;
    __sanitizer_finish_switch_fiber(f->asanfake, &bottom, &size);
    f->asanfake = 0;
    Fiber *from = f->asanfrom;
    if (from && !from->asansize) {
        from->asanbottom = bottom;
        from->asansize   = size;
    }
}
#endif

static void fiberentry(Fiber *f)
{
    #if FIBER_ASAN
    fiberarrived(f);
    #endif
    f->fn(f->os, f->arg);
    __builtin_trap();  // fibers never return
}

static Fiber *os_self(Os *os)
{
    Fibers *fs = osfibers(os);
    if (!fs->current) {
        fs->main.os = os;
        fs->current = &fs->main;
    }
    return fs->current;
}

// Set a fiber to start at fiberentry, at the top of its stack.
static void fiberinit(Fiber *f, iz stack)
{
    fiberctxinit(&f->ctx, f->stack, stack, f);
}

static Fiber *os_fiber(Os *os, Arena *a, iz stack, void (*fn)(Os *, void *), void *arg)
{
    os_self(os);
    Fiber *f = alloc<Fiber>(a);
    f->os    = os;
    f->fn    = fn;
    f->arg   = arg;
    f->size  = stack;
    f->stack = fiberstack(os, stack);
    #if FIBER_ASAN
    f->asanbottom = f->stack;
    f->asansize   = (uz)stack;
    f->asanfake   = 0;
    Fibers *fs = osfibers(os);
    f->asannext = fs->asanall;
    fs->asanall = f;
    #endif
    fiberinit(f, stack);
    return f;
}

static void os_switchto(Os *os, Fiber *f, b32 final)
{
    Fiber *from = osfibers(os)->current;
    osfibers(os)->current = f;
    #if FIBER_ASAN
    // A fiber left for good frees its fake stack (a null save)
    f->asanfrom = from;
    __sanitizer_start_switch_fiber(final ? 0 : &from->asanfake, f->asanbottom, f->asansize);
    #else
    (void)final;
    #endif
    fiberctxswap(&from->ctx, &f->ctx);
    #if FIBER_ASAN
    fiberarrived(from);  // resumed
    #endif
}

static void os_switch(Os *os, Fiber *f)
{
    os_switchto(os, f, 0);
}

// A switch away for good: the fiber left is never resumed (the library).
[[maybe_unused]] static void os_switchfinal(Os *os, Fiber *f)
{
    os_switchto(os, f, 1);
}

#if FIBER_ASAN
// ASan frees a fiber's fake stack only as the fiber is left for good. A
// suspended fiber that holds one is restarted on its own stack to do
// just that, returning at once to the fiber that retired it.
static void fiberretired(Os *os, void *back)
{
    os_switchfinal(os, (Fiber *)back);
}

static void fiberretire(Os *os, Fiber *f)
{
    Fibers *fs = osfibers(os);
    for (Fiber **p = &fs->asanall; *p; p = &(*p)->asannext) {
        if (*p == f) {
            *p = f->asannext;
            break;
        }
    }
    if (f->asanfake) {
        f->fn  = fiberretired;
        f->arg = fs->current;
        fiberinit(f, f->size);
        os_switch(os, f);
    }
}
#endif

// Retire every fiber but the current one: ASan's fake stacks, before a
// library run's stacks are released all at once. Nothing otherwise.
[[maybe_unused]] static void os_fibersend(Os *os)
{
    #if FIBER_ASAN
    Fibers *fs = osfibers(os);
    for (Fiber *f = fs->asanall, *next; f; f = next) {
        next = f->asannext;
        if (f != fs->current) fiberretire(os, f);
    }
    #else
    (void)os;
    #endif
}

static void os_fiberfree(Os *os, Fiber *f)
{
    assert(f != osfibers(os)->current);
    #if FIBER_ASAN
    fiberretire(os, f);
    #endif
    fiberunstack(os, f->stack, f->size);
}
