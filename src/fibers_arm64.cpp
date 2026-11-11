// Fibers' context switch for Apple arm64 (macOS): a small one of our
// own, since macOS's ucontext is deprecated and crashes there. Linux
// arm64 keeps ucontext (fibers_ucontext.cpp): glibc's swapcontext knows
// about guarded control stacks, which a hand-rolled switch would break
// (see JmpBuf in base.cpp for the rest of the ABI reasoning). Included
// before fibers_posix.cpp, which builds fibers on it.
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

// Set a fiber to start at fiberentry(f), at the top of its stack.
static void fiberctxinit(FiberCtx *c, byte *stack, iz size, Fiber *f)
{
    // Initial frame loaded by fiberswap: x19-x30, d8-d15. It "returns"
    // via x30 into fiberstart with x19 = f, a zero frame pointer ending
    // the frame chain, and sp back at the 16-byte aligned top.
    uz  *top = (uz *)((uz)(stack + size) & -(uz)16);
    uz  *sp  = top - 20;
    for (i32 i = 0; i < 20; i++) sp[i] = 0;
    sp[0]  = (uz)f;
    sp[11] = (uz)fiberstart;
    c->sp = sp;
}

static void fiberctxswap(FiberCtx *from, FiberCtx *to)
{
    fiberswap(&from->sp, to->sp);
}
