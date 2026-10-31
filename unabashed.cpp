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




#include "parse.cpp"
#include "print.cpp"


// Shell
//
// M1 executes only what the parser tests need: function definitions,
// declare -f, echo, exit, alias, and shopt. The interpreter proper
// arrives in M2.

struct Shell {
    Os         *os;
    Output     *out;
    Output     *err;
    Arena      *perm;
    Str         name;   // $0
    Str         iname;  // input name for parser errors: -c or the script
    Lexer      *lexer;
    Map<Node *> *funcs;
    Alias      *aliases;
    i32         status;
    b32         exiting;
};

static void printmsgs(Shell *sh)
{
    Lexer *L = sh->lexer;
    for (Message *m = L->msgs; m; m = m->next) {
        print(sh->err, sh->name);
        if (m->kind == MSG_ERROR) {
            if (!(sh->name == sh->iname)) {
                print(sh->err, ": ");
                print(sh->err, sh->iname);
            }
            print(sh->err, ": line ");
            print(sh->err, (i64)m->line);
            print(sh->err, ": ");
        } else {
            print(sh->err, ": line ");
            print(sh->err, (i64)m->line);
            print(sh->err, ": warning: ");
        }
        print(sh->err, m->text);
        print(sh->err, "\n");
    }
    L->msgs    = 0;
    L->msgtail = &L->msgs;
    flush(sh->err);
}

struct FuncEntry {
    Str   name;
    Node *body;
};

static void collectfuncs(Map<Node *> *m, Slice<FuncEntry> *s, Arena *a)
{
    if (!m) return;
    if (m->val) {
        FuncEntry e = {m->key, m->val};
        *s = push(a, *s, e);
    }
    for (i32 i = 0; i < 4; i++) {
        collectfuncs(m->child[i], s, a);
    }
}

static b32 funcless(FuncEntry *a, FuncEntry *b)
{
    return compare(a->name, b->name) < 0;
}

static void printfunc(Shell *sh, Arena scratch, Str name, Node *body)
{
    print(sh->out, printfunction(&scratch, name, body, sh->lexer->posix));
    print(sh->out, "\n");
}

static i32 builtin_declare(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    if (nargs<2 || !(args[1] == "-f")) {
        return 0;  // M1: only declare -f
    }
    i32 status = 0;
    if (nargs == 2) {
        Slice<FuncEntry> all = {};
        collectfuncs(sh->funcs, &all, &scratch);
        sort(all.data, all.len, funcless, scratch);
        for (iz i = 0; i < all.len; i++) {
            printfunc(sh, scratch, all[i].name, all[i].body);
        }
        return 0;
    }
    for (iz i = 2; i < nargs; i++) {
        Node **f = upsert(&sh->funcs, args[i], 0);
        if (f && *f) {
            printfunc(sh, scratch, args[i], *f);
        } else {
            status = 1;
        }
    }
    return status;
}

static i32 builtin_alias(Shell *sh, Str *args, iz nargs)
{
    for (iz i = 1; i < nargs; i++) {
        Cut c = cut(args[i], '=');
        if (!c.ok) continue;
        Alias **slot = &sh->aliases;
        Str name = clone(sh->perm, c.head);
        for (u64 h = hash(name); *slot; h <<= 2) {
            if ((*slot)->name == name) break;
            slot = &(*slot)->child[h>>62];
        }
        if (!*slot) {
            *slot = alloc<Alias>(sh->perm);
            (*slot)->name = name;
        }
        (*slot)->value = clone(sh->perm, c.tail);
    }
    return 0;
}

static i32 builtin_shopt(Shell *sh, Str *args, iz nargs)
{
    if (nargs < 3) return 0;
    b32 on = args[1] == "-s";
    for (iz i = 2; i < nargs; i++) {
        if (args[i] == "expand_aliases") {
            sh->lexer->expandaliases = sh->lexer->aliasopt = on;
        } else if (args[i] == "extglob") {
            sh->lexer->extglob = sh->lexer->extglobopt = on;
        }
    }
    return 0;
}

static void runsimple(Shell *sh, Node *n, Arena scratch)
{
    iz nargs = 0;
    for (Word *w = n->words; w; w = w->next) nargs++;
    Str *args = alloc<Str>(&scratch, nargs);
    iz i = 0;
    for (Word *w = n->words; w; w = w->next) {
        args[i++] = quoteremoval(&scratch, w->text);
    }
    if (!nargs) {
        sh->status = 0;
        return;
    }

    Str name = args[0];
    if (name == "echo") {
        for (iz j = 1; j < nargs; j++) {
            if (j > 1) print(sh->out, " ");
            print(sh->out, args[j]);
        }
        print(sh->out, "\n");
        flush(sh->out);
        sh->status = 0;
    } else if (name == "exit") {
        if (nargs > 1) {
            i64 v = 0;
            for (iz j = 0; j < args[1].len && digit(args[1][j]); j++) {
                v = (v*10 + (args[1][j] - '0')) & 255;
            }
            sh->status = (i32)v;
        }
        sh->exiting = 1;
    } else if (name == "declare" || name == "typeset") {
        sh->status = builtin_declare(sh, args, nargs, scratch);
        flush(sh->out);
    } else if (name == "alias") {
        sh->status = builtin_alias(sh, args, nargs);
    } else if (name == "shopt") {
        sh->status = builtin_shopt(sh, args, nargs);
    } else {
        print(sh->err, sh->name);
        print(sh->err, ": line ");
        print(sh->err, (i64)n->line);
        print(sh->err, ": ");
        print(sh->err, name);
        print(sh->err, ": command not found\n");
        flush(sh->err);
        sh->status = 127;
    }
}

static void execute(Shell *sh, Node *n, Arena scratch)
{
    if (!n || sh->exiting) {
        return;
    }
    switch (n->kind) {
    case N_CONNECT:
        execute(sh, n->a, scratch);
        switch (n->connector) {
        case T_ANDAND:
            if (!sh->status) execute(sh, n->b, scratch);
            break;
        case T_OROR:
            if (sh->status) execute(sh, n->b, scratch);
            break;
        default:
            execute(sh, n->b, scratch);
        }
        break;
    case N_FUNCDEF: {
        if (n->name->flags & (W_HASDOLLAR|W_QUOTED)) {
            print(sh->err, sh->name);
            print(sh->err, ": line ");
            print(sh->err, (i64)n->line);
            print(sh->err, ": `");
            print(sh->err, n->name->text);
            print(sh->err, "': not a valid identifier\n");
            flush(sh->err);
            sh->status = 1;
            break;
        }
        Str name = clone(sh->perm, n->name->text);
        *upsert(&sh->funcs, name, sh->perm) = copynode(sh->perm, n->a);
        sh->status = 0;
    } break;
    case N_SIMPLE:
        runsimple(sh, n, scratch);
        break;
    default:
        sh->status = 0;  // M1 does not execute compound commands
    }
    if (n->flags & CMD_INVERT) {
        sh->status = !sh->status;
    }
}

static void runstring(Shell *sh, Str src, Arena scratch)
{
    Lexer *L = newlexer(&scratch, src);
    L->aliases = &sh->aliases;
    sh->lexer  = L;
    for (;;) {
        Parsed r = parseunit(L);
        printmsgs(sh);
        if (r.status == PARSE_EOF) {
            break;
        }
        if (r.status == PARSE_ERROR) {
            // A non-interactive shell exits. Under -c, an error inside
            // a command substitution exits with 127.
            // An end of input inside a quote or similar only prints its
            // own message and keeps a nonzero status (bash's error
            // yacc_EOF rule); reported errors give 2.
            if (L->comsuberr && sh->iname=="-c") {
                sh->status = 127;
            } else if (L->reported || !sh->status) {
                sh->status = 2;
            }
            break;
        }
        if (r.status == PARSE_DISCARD) {
            sh->status = 1;
            continue;
        }
        execute(sh, r.cmd, scratch);
        if (sh->exiting) {
            break;
        }
    }
}

static i32 sh_main(Os *os, OsInfo *info)
{
    Arena   perm    = newarena(os, info->reserve);
    Arena   scratch = newarena(os, info->reserve);
    Output *out     = newoutput(os, &perm, info->std[1]);
    Output *err     = newoutput(os, &perm, info->std[2]);

    Shell *sh = alloc<Shell>(&perm);
    sh->os    = os;
    sh->out   = out;
    sh->err   = err;
    sh->perm  = &perm;
    sh->name  = info->nargs ? info->args[0] : Str("bash");
    sh->iname = "-c";

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
            runstring(sh, info->args[2], scratch);
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
