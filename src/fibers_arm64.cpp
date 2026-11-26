// Fibers' context switch for Apple arm64 (macOS): a small one of our
// own, since macOS's ucontext is deprecated and crashes there. Linux
// arm64 keeps ucontext (fibers_ucontext.cpp): glibc's swapcontext knows
// about guarded control stacks, which a hand-rolled switch would break
// (see JmpBuf in base.cpp for the rest of the ABI reasoning). Included
// before fibers_posix.cpp, which builds fibers on it.
// This is free and unencumbered software released into the public domain.

struct Fiber;
static void fiberentry(Fiber *);  // fibers_posix.cpp

struct FiberCtx {
    void *sp;  // saved stack pointer while suspended
};

// A suspended fiber's stack ends in this frame, popped on resuming
struct FiberFrame {
    void *fp;  // x29
    void *pc;  // where to resume
    void *x0;  // a new fiber's argument
    void *lr;  // x30, null for a new fiber
};

// Set a fiber to start at fiberentry(f), at the top of its stack, with
// a null frame pointer and link register ending the frame chain.
static void fiberctxinit(FiberCtx *c, byte *stack, iz size, Fiber *f)
{
    uz         top = (uz)(stack + size) & -(uz)16;
    FiberFrame *fr = (FiberFrame *)top - 1;
    fr->fp = 0;
    fr->pc = (void *)fiberentry;
    fr->x0 = f;
    fr->lr = 0;
    c->sp  = fr;
}

// Extended asm in an ordinary function, rather than asm functions of
// their own: GCC's LTO puts top-level asm in a single partition, out of
// reach of calls from the others, and GCC has no naked functions here.
// The clobbers have the compiler save the callee-saved registers
// (AAPCS64: x19-x28, d8-d15) around the switch. x29 and sp it cannot
// be told of, so the asm keeps them, in a FiberFrame; x18, Apple's
// platform register, is left alone. Not inlined, so that only a switch
// pays for the saves.
[[gnu::noinline]] static void fiberctxswap(FiberCtx *from, FiberCtx *to)
{
    register void **save asm("x0") = &from->sp;
    register void  *sp   asm("x1") = to->sp;
    asm volatile (
        "        sub   sp, sp, #32\n"
        "        adr   x2, 1f\n"
        "        stp   x29, x2, [sp]\n"
        "        mov   x3, sp\n"
        "        str   x3, [x0]\n"
        "        mov   sp, x1\n"
        "        ldp   x29, x2, [sp]\n"
        "        ldp   x0, x30, [sp, #16]\n"
        "        add   sp, sp, #32\n"
        "        br    x2\n"
        "1:\n"
        : "+r"(save), "+r"(sp)
        :
        : "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
          "x12", "x13", "x14", "x15", "x16", "x17", "x19", "x20", "x21",
          "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x30",
          "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9",
          "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18",
          "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",
          "v28", "v29", "v30", "v31", "cc", "memory"
    );
}
