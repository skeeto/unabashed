// Fibers' context switch for x86-64 System V (Linux, macOS): a small
// one of our own, faster than ucontext, which saves the signal mask with
// a system call. Included before fibers_posix.cpp, which builds fibers
// on it.
// This is free and unencumbered software released into the public domain.

struct Fiber;
static void fiberentry(Fiber *);  // fibers_posix.cpp

struct FiberCtx {
    void *sp;  // saved stack pointer while suspended
};

// Naked functions, not top-level asm: GCC's LTO puts top-level asm in
// one partition, out of reach of calls from the others. Being static,
// they stay local to the object, so that two copies of these layers,
// such as a library linked beside its host, never clash.
[[gnu::naked]] static void fiberswap(void ** /*save*/, void * /*sp*/)
{
    asm (
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
    );
}

// The first return into a fiber: fiberentry(f), r13 and r12 set up by
// fiberctxinit
[[gnu::naked]] static void fiberstart()
{
    asm (
    "        mov   %r12, %rdi\n"
    "        call  *%r13\n"
    "        ud2\n"
    );
}

// Set a fiber to start at fiberentry(f), at the top of its stack.
static void fiberctxinit(FiberCtx *c, byte *stack, iz size, Fiber *f)
{
    // Initial frame popped by fiberswap: r15 r14 r13 r12 rbx rbp, then
    // "return" into fiberstart with r12 = f, r13 = fiberentry, and a
    // 16-byte aligned stack.
    uz  *top = (uz *)((uz)(stack + size) & -(uz)16);
    uz  *sp  = top - 7;
    for (i32 i = 0; i < 7; i++) sp[i] = 0;
    sp[2] = (uz)fiberentry;
    sp[3] = (uz)f;
    sp[6] = (uz)fiberstart;
    c->sp = sp;
}

static void fiberctxswap(FiberCtx *from, FiberCtx *to)
{
    fiberswap(&from->sp, to->sp);
}
