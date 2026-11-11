// Fibers' context switch for x86-64 System V (Linux, macOS): a small
// one of our own, faster than ucontext, which saves the signal mask with
// a system call. Included before fibers_posix.cpp, which builds fibers
// on it.
// This is free and unencumbered software released into the public domain.

struct Fiber;
[[gnu::used]] static void fiberentry(Fiber *) asm("fiberentry");  // fibers_posix.cpp

struct FiberCtx {
    void *sp;  // saved stack pointer while suspended
};

// Labels pin the symbol names, which otherwise vary by platform. They
// stay local to the object, so that two copies of these layers, such as
// a library linked beside its host, never clash.
extern "C" void fiberswap(void **save, void *sp) asm("fiberswap");
extern "C" void fiberstart() asm("fiberstart");
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

// Set a fiber to start at fiberentry(f), at the top of its stack.
static void fiberctxinit(FiberCtx *c, byte *stack, iz size, Fiber *f)
{
    // Initial frame popped by fiberswap: r15 r14 r13 r12 rbx rbp, then
    // "return" into fiberstart with r12 = f and a 16-byte aligned stack.
    uz  *top = (uz *)((uz)(stack + size) & -(uz)16);
    uz  *sp  = top - 7;
    for (i32 i = 0; i < 7; i++) sp[i] = 0;
    sp[3] = (uz)f;
    sp[6] = (uz)fiberstart;
    c->sp = sp;
}

static void fiberctxswap(FiberCtx *from, FiberCtx *to)
{
    fiberswap(&from->sp, to->sp);
}
