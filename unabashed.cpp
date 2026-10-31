// unabashed: a faithful clone of GNU bash 5.3
//
// Unity build: a platform layer (main_win32.cpp, main_posix.cpp,
// main_test.cpp, main_fuzz.cpp) includes this file and implements the
// os_* API. Design notes are in notes/, starting with notes/plan.md.
//
// This is free and unencumbered software released into the public domain.
#include "base.cpp"


// Platform API

using Handle = uz;  // fd, HANDLE, or virtual object
enum : Handle { BADHANDLE = (Handle)-1 };

struct Fiber;

struct OsInfo {
    Str   *args;
    iz     nargs;
    Str   *env;      // "NAME=value", UTF-8 (WTF-8 on Windows)
    iz     nenv;
    Handle std[3];
    iz     reserve;  // address space for each large arena
};

// Reserve address space. Returns null on failure.
static byte  *os_reserve(Os *, iz);
// Commit part of a reservation. Returns false on failure.
static b32    os_commit(Os *, byte *, iz);
// Write all bytes, returning false on error.
static b32    os_write(Os *, Handle, u8 *, iz);
// Read up to len bytes. Returns 0 at end of file, -1 on error.
static iz     os_read(Os *, Handle, u8 *, iz);
// The currently running fiber, converting the thread on first use.
static Fiber *os_self(Os *);
// Create a suspended fiber running fn(os, arg), which must never
// return. The record is allocated in the arena.
static Fiber *os_fiber(Os *, Arena *, iz stack, void (*)(Os *, void *), void *);
// Switch from the current fiber to another.
static void   os_switch(Os *, Fiber *);
// Release a fiber other than the current one.
static void   os_fiberfree(Os *, Fiber *);
// Entry point, called by the platform after it fills in OsInfo.
static i32    sh_main(Os *, OsInfo *);


// Memory

// A new arena on its own reservation, committed as it grows.
static Arena newarena(Os *os, iz reserve)
{
    byte *base = os_reserve(os, reserve);
    iz    head = (iz)1<<16;
    if (!base || !os_commit(os, base, head)) {
        os_oom();
    }
    Mem *m = (Mem *)base;
    m->os     = os;
    m->commit = base + head;
    m->limit  = base + reserve;
    Arena a = {};
    a.beg = base + sizeof(Mem);
    a.end = m->commit;
    a.mem = m;
    return a;
}


// Output: buffered with a sticky error flag

struct Output {
    Os    *os;
    Handle h;
    u8    *buf;
    iz     len;
    iz     cap;
    b32    err;
};

static Output *newoutput(Os *os, Arena *a, Handle h)
{
    Output *o = alloc<Output>(a);
    o->os  = os;
    o->h   = h;
    o->cap = 1<<12;
    o->buf = allocbytes(a, o->cap);
    return o;
}

static void flush(Output *o)
{
    if (!o->err && o->len) {
        o->err = !os_write(o->os, o->h, o->buf, o->len);
    }
    o->len = 0;
}

static void print(Output *o, Str s)
{
    for (iz off = 0; off < s.len;) {
        if (o->len == o->cap) {
            flush(o);
        }
        iz avail = o->cap - o->len;
        iz count = avail<s.len-off ? avail : s.len-off;
        copybytes(o->buf+o->len, s.data+off, count);
        o->len += count;
        off    += count;
    }
}

static void print(Output *o, i64 v)
{
    u8  tmp[32];
    u8 *end = tmp + countof(tmp);
    u8 *beg = end;
    i64 t   = v<0 ? v : -v;
    do {
        *--beg = (u8)('0' - t%10);
    } while (t /= 10);
    if (v < 0) {
        *--beg = '-';
    }
    print(o, span(beg, end));
}


// Self test: exercises platform primitives on a real host.

struct PingPong {
    Fiber *main;
    Fiber *a;
    Fiber *b;
    u8    *trace;
    iz     len;
    i32    rounds;
};

static void pingpong(Os *os, PingPong *p, u8 mark, Fiber *next)
{
    for (i32 i = 0; i < p->rounds; i++) {
        p->trace[p->len++] = mark;
        os_switch(os, next);
    }
    os_switch(os, p->main);
    assert(0);
}

static void ping(Os *os, void *arg)
{
    PingPong *p = (PingPong *)arg;
    pingpong(os, p, 'a', p->b);
}

static void pong(Os *os, void *arg)
{
    PingPong *p = (PingPong *)arg;
    pingpong(os, p, 'b', p->a);
}

// Run a fiber ping-pong and report the result. Returns the exit status.
static i32 selftest(Os *os, Arena *perm, Output *out)
{
    PingPong *p = alloc<PingPong>(perm);
    p->rounds = 1000;
    p->trace  = allocbytes(perm, 2*p->rounds);
    p->main   = os_self(os);
    p->a      = os_fiber(os, perm, (iz)1<<20, ping, p);
    p->b      = os_fiber(os, perm, (iz)1<<20, pong, p);
    os_switch(os, p->a);
    os_fiberfree(os, p->a);
    os_fiberfree(os, p->b);

    b32 ok = p->len == 2*p->rounds;
    for (iz i = 0; ok && i < p->len; i++) {
        ok = p->trace[i] == "ab"[i&1];
    }
    print(out, ok ? Str("fibers: ok\n") : Str("fibers: FAIL\n"));
    return !ok;
}


// Stand-in interpreter for M0: runs "echo" and "exit" from a -c string
// split on ';' and newlines. Replaced by the parser in M1.

struct Shell {
    Os     *os;
    Output *out;
    Output *err;
    Str     name;  // $0
    i32     status;
};

static Str nextword(Str *s)
{
    Str t = *s;
    for (; t.len && (t[0]==' ' || t[0]=='\t'); t = cuthead(t, 1)) {}
    iz i = 0;
    for (; i<t.len && t[i]!=' ' && t[i]!='\t'; i++) {}
    *s = cuthead(t, i);
    return takehead(t, i);
}

// Run one command. Returns true if the shell should exit.
static b32 runsimple(Shell *sh, Str cmd, i64 lineno)
{
    Str name = nextword(&cmd);
    if (!name.len) {
        return 0;
    }

    if (name == "echo") {
        for (b32 first = 1;; first = 0) {
            Str arg = nextword(&cmd);
            if (!arg.len) break;
            if (!first) print(sh->out, " ");
            print(sh->out, arg);
        }
        print(sh->out, "\n");
        sh->status = 0;
        return 0;
    }

    if (name == "exit") {
        Str arg = nextword(&cmd);
        if (arg.len) {
            i64 v = 0;
            for (iz i = 0; i < arg.len && digit(arg[i]); i++) {
                v = (v*10 + (arg[i] - '0')) & 255;
            }
            sh->status = (i32)v;
        }
        return 1;
    }

    print(sh->err, sh->name);
    print(sh->err, ": line ");
    print(sh->err, lineno);
    print(sh->err, ": ");
    print(sh->err, name);
    print(sh->err, ": command not found\n");
    sh->status = 127;
    return 0;
}

static void runstring(Shell *sh, Str src)
{
    i64 lineno = 1;
    while (src.len) {
        iz i = 0;
        for (; i<src.len && src[i]!=';' && src[i]!='\n'; i++) {}
        b32 nl = i<src.len && src[i]=='\n';
        if (runsimple(sh, takehead(src, i), lineno)) {
            return;
        }
        lineno += nl;
        src = cuthead(src, i + (i<src.len));
    }
}

static i32 sh_main(Os *os, OsInfo *info)
{
    Arena   perm = newarena(os, info->reserve);
    Output *out  = newoutput(os, &perm, info->std[1]);
    Output *err  = newoutput(os, &perm, info->std[2]);

    Shell *sh = alloc<Shell>(&perm);
    sh->os   = os;
    sh->out  = out;
    sh->err  = err;
    sh->name = info->nargs ? info->args[0] : Str("bash");

    Str opt = info->nargs>1 ? info->args[1] : Str{};
    if (opt == "--unabashed-selftest") {
        sh->status = selftest(os, &perm, out);
    } else if (opt == "-c") {
        if (info->nargs < 3) {
            print(err, sh->name);
            print(err, ": -c: option requires an argument\n");
            sh->status = 2;
        } else {
            if (info->nargs > 3) {
                sh->name = info->args[3];
            }
            runstring(sh, info->args[2]);
        }
    } else {
        print(err, sh->name);
        print(err, ": only -c is implemented\n");
        sh->status = 2;
    }

    flush(out);
    flush(err);
    return sh->status;
}
