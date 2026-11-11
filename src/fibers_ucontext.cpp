// Fibers' context switch by ucontext, for POSIX systems without one of
// our own (fibers_amd64.cpp, fibers_arm64.cpp), and on x86-64 Linux
// with -DFIBER_UCONTEXT. The root includes <ucontext.h>. Included before
// fibers_posix.cpp, which builds fibers on it.
// This is free and unencumbered software released into the public domain.

struct Fiber;
[[gnu::used]] static void fiberentry(Fiber *) asm("fiberentry");  // fibers_posix.cpp

struct FiberCtx {
    ucontext_t uc;
};

static void fiberbounce(u32 hi, u32 lo)
{
    fiberentry((Fiber *)((uz)hi<<16<<16 | lo));
}

// Set a fiber to start at fiberentry(f), at the top of its stack.
static void fiberctxinit(FiberCtx *c, byte *stack, iz size, Fiber *f)
{
    getcontext(&c->uc);
    c->uc.uc_stack.ss_sp   = stack;
    c->uc.uc_stack.ss_size = (uz)size;
    c->uc.uc_link          = 0;
    uz p = (uz)f;
    makecontext(&c->uc, (void (*)())fiberbounce, 2, (u32)(p>>16>>16), (u32)p);
}

static void fiberctxswap(FiberCtx *from, FiberCtx *to)
{
    swapcontext(&from->uc, &to->uc);
}
