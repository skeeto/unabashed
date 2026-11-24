// The host's hooks (unabashed.h), for both library modules: included
// by each root after its module, whose Lib it uses. A hook runs on the
// run's fiber that reached it, with what it is given converted here.
// This is free and unencumbered software released into the public domain.

// A NUL-terminated copy of s.
static char *hoststr(Arena *a, Str s)
{
    char *r = (char *)allocbytes(a, s.len+1);
    copybytes(r, s.data, s.len);
    r[s.len] = 0;
    return r;
}

// A null-terminated array of NUL-terminated copies.
static char const **hoststrs(Arena *a, Slice<Str> v)
{
    char const **r = alloc<char const *>(a, v.len+1);
    for (iz i = 0; i < v.len; i++) r[i] = hoststr(a, v[i]);
    r[v.len] = 0;
    return r;
}


// Exit

static b32 lib_exitwanted(Os *os)
{
    return ((Lib *)os)->cfg.exit != 0;
}

// The run's output is all written out first, for a host that reads it
// from the hook. Nothing allocates after the call, so the run ends as
// the hook was told.
static void lib_exit(Os *os, Arena a, i32 status, Str pwd, Str cwd, Slice<Str> env)
{
    Lib *l = (Lib *)os;
    ub_exit x = {};
    x.status = hoststatus(status);
    x.pwd    = pwd.len ? hoststr(&a, pwd) : 0;
    x.cwd    = cwd.len ? hoststr(&a, cwd) : 0;
    x.env    = hoststrs(&a, env);
    flushw(os);
    l->cfg.exit(l->cfg.user, &x);
}


// Access

enum : i32 { REASONMAX = 255 };

// The hooks' state in a run, from its arena (hooksetup)
struct HostErr {
    i32 err;        // the error it stands for
    i32 len;
    u8  text[REASONMAX];  // the host's reason, its first line
};

struct TraceRec;

struct LibHooks {
    HostErr   errs[NHOSTERR];  // a ring
    i32       nexterr;
    TraceRec *opentrace;  // the fibers' open commands, innermost first
    TraceRec *freetrace;
    i64       lasttrace;  // the last id given
};

static void hooksetup(Lib *l)
{
    l->hooks = alloc<LibHooks>(&l->arena, 1);
}

// The core's open flags are the hook's
static_assert((i32)OF_READ==UB_READ && (i32)OF_WRITE==UB_WRITE &&
              (i32)OF_CREATE==UB_CREATE && (i32)OF_TRUNC==UB_TRUNC &&
              (i32)OF_APPEND==UB_APPEND && (i32)OF_EXCL==UB_EXCL);

// A hook's refusal as the core's error: a negated UB_E*, or else EACCES
static i32 hookerr(i32 r)
{
    switch (r) {
    case -UB_EPERM: case -UB_ENOENT: case -UB_EBADF: case -UB_ECHILD:
    case -UB_ENOMEM: case -UB_EACCES: case -UB_EBUSY: case -UB_EEXIST:
    case -UB_ENOTDIR: case -UB_EISDIR: case -UB_EINVAL: case -UB_EROFS:
    case -UB_EDEADLK: case -UB_ECANCELED:
        return -r;
    }
    return E_ACCES;
}

// The first line of a host's reason, cut to fit a record at a whole
// character
static Str reasonline(char const *reason)
{
    Str r = {};
    r.data = (u8 *)reason;
    while (reason[r.len] && reason[r.len]!='\n' && r.len<=REASONMAX) {
        r.len++;
    }
    if (r.len > REASONMAX) {
        r.len = REASONMAX;
        while (r.len && (r.data[r.len]&0xc0)==0x80) r.len--;
    }
    if (r.len && r.data[r.len-1]=='\r') r.len--;
    return r;
}

// The core's error for a refusal: with a reason, or always if asked, a
// code of its own (E_HOST+k), reused while the ring holds the same, that
// errtext shows as the reason, if any; else the plain error, so that
// ENOENT still means absent.
static i32 hosterr(Lib *l, i32 err, char const *reason, b32 always)
{
    Str r = reason ? reasonline(reason) : Str{};
    if (!r.len && !always) return err;
    LibHooks *h = l->hooks;
    for (i32 k = 0; k < NHOSTERR; k++) {
        HostErr *e = h->errs + k;
        if (e->err==err && Str(e->text, e->len)==r) return E_HOST + k;
    }
    i32 k = h->nexterr;
    h->nexterr = (k + 1) % NHOSTERR;
    HostErr *e = h->errs + k;
    e->err = err;
    e->len = (i32)r.len;
    copybytes(e->text, r.data, r.len);
    return E_HOST + k;
}

static Str lib_hosttext(i32 err)
{
    Lib *l = runlib();
    if (!l || err<E_HOST || err>=E_HOST+NHOSTERR) return {};
    HostErr *e = l->hooks->errs + (err - E_HOST);
    return Str(e->text, e->len);
}

static i32 lib_hosterrno(i32 err)
{
    Lib *l = runlib();
    if (!l || err<E_HOST || err>=E_HOST+NHOSTERR) return 0;
    return l->hooks->errs[err-E_HOST].err;
}

// Ask the access hook, if any, of a file the script names: 0 to go on,
// or the error to fail with
static i32 askhost(Lib *l, Arena scratch, i32 op, i32 flags, Str path)
{
    if (!l->cfg.access) return 0;
    ub_access q = {};
    q.op    = op;
    q.flags = flags;
    q.path  = hoststr(&scratch, path);
    char const *reason = 0;
    i32 r = l->cfg.access(l->cfg.user, &q, &reason);
    return r ? hosterr(l, hookerr(r), reason, 0) : 0;
}


// Spawn

static b32 hostabsolute(Str path)
{
    if (path.len && path.data[0]=='/') return 1;
    #if _WIN32
    if (path.len && path.data[0]=='\\') return 1;
    u8 c = path.len>2 ? path.data[0]|0x20 : 0;
    if (c>='a' && c<='z' && path.data[1]==':') {
        return path.data[2]=='/' || path.data[2]=='\\';
    }
    #endif
    return 0;
}

// Ask the spawn hook, if any, of a program about to start (trace: its
// command's id): 0 to start it, with *wrap the wrapper to start
// instead, if any, or the error. A refusal is always a host's error
// (E_HOST), which the core reports in the hook's words, not guessing
// at the system's reasons.
static i32 askspawn(Lib *l, Arena *a, Spawn *s, i64 trace, Slice<Str> *wrap)
{
    if (!l->cfg.spawn) return 0;
    ub_spawn q = {};
    q.path    = hoststr(a, s->path);
    q.argc    = (int)s->argc;
    q.argv    = hoststrs(a, {s->argv, s->argc});
    q.env     = hoststrs(a, {s->env, s->nenv});
    q.cwd     = s->cwd.len ? hoststr(a, s->cwd) : 0;
    q.cmdline = s->cmdline.len ? hoststr(a, s->cmdline) : 0;
    q.trace   = trace;
    i32 v = l->cfg.spawn(l->cfg.user, &q);
    if (v) return hosterr(l, hookerr(v), q.reason, 1);
    for (iz i = 0; q.wrap && q.wrap[i]; i++) {
        *wrap = push(a, *wrap, libclone(a, q.wrap[i]));
    }
    if (wrap->len && !hostabsolute((*wrap)[0])) {
        *wrap = {};
        return hosterr(l, E_INVAL, 0, 1);
    }
    return 0;
}


// Trace

// A simple command the trace hook was told of, or will be (held), on
// its fiber's stack of open commands until it ends. One that started a
// program and ended first waits off the stack, its program's LibKid
// holding it, for the reap.
struct TraceRec {
    TraceRec  *next;
    uz         fiber;   // fiberkey
    void      *key;     // the core's: its Simple
    void      *frame;   // where it started: deeper is lower
    Slice<Str> argv;    // while held
    i64        id;      // 0 while held
    i64        level;
    i32        kind;
    i64        pid;     // its program, if it started one
    i32        status;  // the program's, once reaped
    b32        reaped;
    b32        left;    // done, waiting for its program
};

// The running fiber, as the module's fiber records name it
static uz fiberkey(Os *os)
{
    #if _WIN32
    return (uz)os_self(os)->handle;
    #else
    return (uz)os_self(os)->stack;
    #endif
}

static b32 lib_tracing(Os *os)
{
    return ((Lib *)os)->cfg.trace != 0;
}

static void tracefree(Lib *l, TraceRec *t)
{
    t->next = l->hooks->freetrace;
    l->hooks->freetrace = t;
}

static void tracestart(Lib *l, Arena a, TraceRec *t, Slice<Str> argv)
{
    t->id = ++l->hooks->lasttrace;
    ub_trace x = {};
    x.phase = UB_TRACE_START;
    x.kind  = t->kind;
    x.id    = t->id;
    x.level = t->level;
    x.argc  = (int)argv.len;
    x.argv  = hoststrs(&a, argv);
    l->cfg.trace(l->cfg.user, &x);
}

// Allocates nothing: teardown's too
static void traceend(Lib *l, TraceRec *t, i32 status, i64 pid)
{
    ub_trace x = {};
    x.phase  = UB_TRACE_END;
    x.kind   = t->kind;
    x.id     = t->id;
    x.level  = t->level;
    x.pid    = pid;
    x.status = hoststatus(status);
    l->cfg.trace(l->cfg.user, &x);
}

// t, off its stack, done: its END now, or once its program is reaped
// (the kid holds it). An abandoned command (no status) ends only by
// its program.
static void tracedone(Lib *l, TraceRec *t, b32 abandoned, i32 status)
{
    if (t->pid && !t->reaped) {
        t->left = 1;
        return;
    }
    if (t->pid) {
        traceend(l, t, t->status, t->pid);
    } else if (!abandoned && t->id) {
        traceend(l, t, status, 0);
    }
    tracefree(l, t);
}

static void lib_tracestart(Os *os, Arena a, void *key, void *frame, Slice<Str> argv,
                           i64 level, i32 kind, b32 held)
{
    Lib      *l = (Lib *)os;
    LibHooks *h = l->hooks;
    uz        f = fiberkey(os);
    // Its fiber's commands at its depth or deeper were abandoned by an
    // escape. Those above, still open, are its callers: done there.
    for (TraceRec **p = &h->opentrace; *p;) {
        TraceRec *t = *p;
        if (t->fiber != f) {
            p = &t->next;
        } else if ((uz)t->frame <= (uz)frame) {
            *p = t->next;
            tracedone(l, t, 1, 0);
        } else {
            break;
        }
    }
    TraceRec *t = h->freetrace;
    if (t) {
        h->freetrace = t->next;
    } else {
        t = alloc<TraceRec>(&l->arena);
    }
    *t = {};
    t->fiber = f;
    t->key   = key;
    t->frame = frame;
    t->level = level;
    t->kind  = kind;
    t->next  = h->opentrace;
    h->opentrace = t;
    if (held) {
        t->argv = argv;  // the core's, until the command ends
    } else {
        tracestart(l, a, t, argv);
    }
}

static void lib_traceend(Os *os, Arena a, void *key, i32 status, b32 retracted)
{
    Lib      *l = (Lib *)os;
    LibHooks *h = l->hooks;
    uz        f = fiberkey(os);
    TraceRec *t = h->opentrace;
    for (; t && !(t->fiber==f && t->key==key); t = t->next) {}
    if (!t) return;
    // Its fiber's commands above it were abandoned
    for (TraceRec **p = &h->opentrace;;) {
        TraceRec *o = *p;
        if (o->fiber != f) {
            p = &o->next;
            continue;
        }
        *p = o->next;
        if (o == t) break;
        tracedone(l, o, 1, 0);
    }
    if (!t->id && !retracted) tracestart(l, a, t, t->argv);  // never spawned
    t->argv = {};
    tracedone(l, t, retracted, status);
}

// The running fiber's command, about to start a program: a held START
// is told now. Its id, or 0.
static i64 tracespawning(Lib *l, Arena a, TraceRec *t)
{
    if (!t) return 0;
    if (!t->id) tracestart(l, a, t, t->argv);
    return t->id;
}

// A START not yet told, as the command may turn out to be shell code
static b32 traceheld(TraceRec *t)
{
    return t && !t->id;
}

// The running fiber's innermost open command, or null
static TraceRec *tracetop(Lib *l)
{
    if (!l->cfg.trace) return 0;
    uz f = fiberkey(&l->os);
    for (TraceRec *t = l->hooks->opentrace; t; t = t->next) {
        if (t->fiber == f) return t;
    }
    return 0;
}

// t started k's program. A command starts one at most, but should it
// start another, it ends with the later.
static void tracelink(Lib *l, TraceRec *t, LibKid *k)
{
    if (!t) return;
    if (t->pid && !t->reaped) {
        for (i32 i = 0; i < l->nkids; i++) {
            if (l->kids[i].trace == t) l->kids[i].trace = 0;
        }
    }
    k->trace  = t;
    t->pid    = k->pid;
    t->reaped = 0;
}

// k's program was reaped, its status as os_waitproc gives it
static void tracereaped(Lib *l, LibKid *k, i32 status)
{
    TraceRec *t = k->trace;
    if (!t) return;
    k->trace  = 0;
    t->reaped = 1;
    t->status = status;
    if (t->left) {
        traceend(l, t, status, t->pid);
        tracefree(l, t);
    }
}

// A fiber's end: its open commands were abandoned
static void traceunfiber(Lib *l, uz fiber)
{
    if (!l->cfg.trace) return;
    for (TraceRec **p = &l->hooks->opentrace; *p;) {
        TraceRec *t = *p;
        if (t->fiber == fiber) {
            *p = t->next;
            tracedone(l, t, 1, 0);
        } else {
            p = &t->next;
        }
    }
}

// Teardown, before its kills: every command still open was abandoned.
// Allocates nothing.
static void traceleave(Lib *l)
{
    if (!l->cfg.trace) return;
    while (TraceRec *t = l->hooks->opentrace) {
        l->hooks->opentrace = t->next;
        tracedone(l, t, 1, 0);
    }
}
