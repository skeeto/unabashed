// Fibers for the platform layers: Win32 fibers (XP and later), a small
// context switch for x86-64 System V and for Apple arm64, or ucontext
// elsewhere. The including layer stores a Fibers in its struct Os and
// defines osfibers() to return it.
//
// Apple arm64 has its own switch because macOS's ucontext is deprecated
// and crashes there. Linux arm64 keeps ucontext: glibc's swapcontext
// knows about guarded control stacks, which a hand-rolled switch would
// break (see JmpBuf in base.cpp for the rest of the ABI reasoning).
// This is free and unencumbered software released into the public domain.

#if !_WIN32 && !FIBER_UCONTEXT && (__x86_64__ || (__APPLE__ && __aarch64__))
#define FIBER_ASM 1
#endif

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

#elif FIBER_ASM
struct Fiber {
    void  *sp;     // saved stack pointer while suspended
    byte  *stack;  // 0 for the thread's own fiber
    iz     size;
    Os    *os;
    void (*fn)(Os *, void *);
    void  *arg;
};

// Labels pin the symbol names, which otherwise vary by platform. They
// stay local to the object, so that two copies of these layers, such as
// a library linked beside its host, never clash.
extern "C" void fiberswap(void **save, void *sp) asm("fiberswap");
extern "C" void fiberstart() asm("fiberstart");
#if __x86_64__
asm (
    "        .text\n"
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
    "fiberstart:\n"
    "        mov   %r12, %rdi\n"
    "        call  fiberentry\n"
    "        ud2\n"
);
#else
// AAPCS64: the frame holds x19-x30 then d8-d15, 160 bytes, so sp stays
// 16-byte aligned. Control moves only by ret, never by br.
asm (
    "        .text\n"
    "        .p2align 2\n"
    "fiberswap:\n"
    "        sub   sp, sp, #160\n"
    "        stp   x19, x20, [sp, #0]\n"
    "        stp   x21, x22, [sp, #16]\n"
    "        stp   x23, x24, [sp, #32]\n"
    "        stp   x25, x26, [sp, #48]\n"
    "        stp   x27, x28, [sp, #64]\n"
    "        stp   x29, x30, [sp, #80]\n"
    "        stp   d8,  d9,  [sp, #96]\n"
    "        stp   d10, d11, [sp, #112]\n"
    "        stp   d12, d13, [sp, #128]\n"
    "        stp   d14, d15, [sp, #144]\n"
    "        mov   x16, sp\n"
    "        str   x16, [x0]\n"
    "        mov   sp, x1\n"
    "        ldp   x19, x20, [sp, #0]\n"
    "        ldp   x21, x22, [sp, #16]\n"
    "        ldp   x23, x24, [sp, #32]\n"
    "        ldp   x25, x26, [sp, #48]\n"
    "        ldp   x27, x28, [sp, #64]\n"
    "        ldp   x29, x30, [sp, #80]\n"
    "        ldp   d8,  d9,  [sp, #96]\n"
    "        ldp   d10, d11, [sp, #112]\n"
    "        ldp   d12, d13, [sp, #128]\n"
    "        ldp   d14, d15, [sp, #144]\n"
    "        add   sp, sp, #160\n"
    "        ret\n"
    "        .p2align 2\n"
    "fiberstart:\n"
    "        mov   x0, x19\n"
    "        bl    fiberentry\n"
    "        brk   #1\n"
);
#endif

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
[[gnu::used]] static void fiberentry(Fiber *f) asm("fiberentry");
static void fiberentry(Fiber *f)
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

#if !FIBER_ASM
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

    #if FIBER_ASM && __x86_64__
    // Initial frame popped by fiberswap: r15 r14 r13 r12 rbx rbp, then
    // "return" into fiberstart with r12 = f and a 16-byte aligned stack.
    uz  *top = (uz *)((uz)(f->stack + stack) & -(uz)16);
    uz  *sp  = top - 7;
    for (i32 i = 0; i < 7; i++) sp[i] = 0;
    sp[3] = (uz)f;
    sp[6] = (uz)fiberstart;
    f->sp = sp;
    #elif FIBER_ASM
    // Initial frame loaded by fiberswap: x19-x30, d8-d15. It "returns"
    // via x30 into fiberstart with x19 = f, a zero frame pointer ending
    // the frame chain, and sp back at the 16-byte aligned top.
    uz  *top = (uz *)((uz)(f->stack + stack) & -(uz)16);
    uz  *sp  = top - 20;
    for (i32 i = 0; i < 20; i++) sp[i] = 0;
    sp[0]  = (uz)f;
    sp[11] = (uz)fiberstart;
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
    #if FIBER_ASM
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
