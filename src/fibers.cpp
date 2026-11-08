// Fibers for the platform layers: Win32 fibers (XP and later), a small
// x86-64 System V context switch, or ucontext elsewhere. The including
// layer stores a Fibers in its struct Os and defines osfibers() to
// return it.
// This is free and unencumbered software released into the public domain.

#if _WIN32
#define FIBERAPI(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
FIBERAPI(void *, ConvertThreadToFiber(void *));
FIBERAPI(void *, CreateFiberEx(uz, uz, i32, void (__stdcall *)(void *), void *));
FIBERAPI(void,   DeleteFiber(void *));
FIBERAPI(void,   SwitchToFiber(void *));

struct Fiber {
    void  *handle;
    Os    *os;
    void (*fn)(Os *, void *);
    void  *arg;
};

#elif __x86_64__ && !FIBER_UCONTEXT
struct Fiber {
    void  *sp;     // saved stack pointer while suspended
    byte  *stack;  // 0 for the thread's own fiber
    iz     size;
    Os    *os;
    void (*fn)(Os *, void *);
    void  *arg;
};

// Labels pin the symbol names, which otherwise vary by platform.
extern "C" void fiberswap(void **save, void *sp) asm("fiberswap");
extern "C" void fiberstart() asm("fiberstart");
asm (
    "        .text\n"
    "        .globl fiberswap\n"
    "fiberswap:\n"
    "        push  %rbp\n"
    "        push  %rbx\n"
    "        push  %r12\n"
    "        push  %r13\n"
    "        push  %r14\n"
    "        push  %r15\n"
    "        mov   %rsp, (%rdi)\n"
    "        mov   %rsi, %rsp\n"
    "        pop   %r15\n"
    "        pop   %r14\n"
    "        pop   %r13\n"
    "        pop   %r12\n"
    "        pop   %rbx\n"
    "        pop   %rbp\n"
    "        ret\n"
    "        .globl fiberstart\n"
    "fiberstart:\n"
    "        mov   %r12, %rdi\n"
    "        call  fiberentry\n"
    "        ud2\n"
);

#else
#include <ucontext.h>
struct Fiber {
    ucontext_t ctx;
    byte      *stack;
    iz         size;
    Os        *os;
    void     (*fn)(Os *, void *);
    void      *arg;
};
#endif

struct Fibers {
    Fiber *current;
    Fiber  main;
};

// Defined by the including layer.
static Fibers *osfibers(Os *);
static byte   *fiberstack(Os *, iz size);  // POSIX stack memory
static void    fiberunstack(Os *, byte *, iz size);

#if _WIN32
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
    return f;
}

static void os_switch(Os *os, Fiber *f)
{
    osfibers(os)->current = f;
    SwitchToFiber(f->handle);
}

static void os_fiberfree(Os *os, Fiber *f)
{
    assert(f != osfibers(os)->current);
    DeleteFiber(f->handle);
}

#else
extern "C" [[gnu::used]] void fiberentry(Fiber *f) asm("fiberentry");
extern "C" void fiberentry(Fiber *f)
{
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

#if !__x86_64__ || FIBER_UCONTEXT
static void fiberbounce(u32 hi, u32 lo)
{
    fiberentry((Fiber *)((uz)hi<<16<<16 | lo));
}
#endif

static Fiber *os_fiber(Os *os, Arena *a, iz stack, void (*fn)(Os *, void *), void *arg)
{
    os_self(os);
    Fiber *f = alloc<Fiber>(a);
    f->os    = os;
    f->fn    = fn;
    f->arg   = arg;
    f->size  = stack;
    f->stack = fiberstack(os, stack);

    #if __x86_64__ && !FIBER_UCONTEXT
    // Initial frame popped by fiberswap: r15 r14 r13 r12 rbx rbp, then
    // "return" into fiberstart with r12 = f and a 16-byte aligned stack.
    uz  *top = (uz *)((uz)(f->stack + stack) & -(uz)16);
    uz  *sp  = top - 7;
    for (i32 i = 0; i < 7; i++) sp[i] = 0;
    sp[3] = (uz)f;
    sp[6] = (uz)fiberstart;
    f->sp = sp;
    #else
    getcontext(&f->ctx);
    f->ctx.uc_stack.ss_sp   = f->stack;
    f->ctx.uc_stack.ss_size = (uz)stack;
    f->ctx.uc_link          = 0;
    uz p = (uz)f;
    makecontext(&f->ctx, (void (*)())fiberbounce, 2, (u32)(p>>16>>16), (u32)p);
    #endif
    return f;
}

static void os_switch(Os *os, Fiber *f)
{
    Fiber *from = osfibers(os)->current;
    osfibers(os)->current = f;
    #if __x86_64__ && !FIBER_UCONTEXT
    fiberswap(&from->sp, f->sp);
    #else
    swapcontext(&from->ctx, &f->ctx);
    #endif
}

static void os_fiberfree(Os *os, Fiber *f)
{
    assert(f != osfibers(os)->current);
    fiberunstack(os, f->stack, f->size);
}
#endif
