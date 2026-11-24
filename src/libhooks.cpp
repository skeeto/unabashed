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

struct LibHooks {
    HostErr errs[NHOSTERR];  // a ring
    i32     nexterr;
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

// Ask the spawn hook, if any, of a program about to start: 0 to start
// it, with *wrap the wrapper to start instead, if any, or the error. A
// refusal is always a host's error (E_HOST), which the core reports in
// the hook's words, not guessing at the system's reasons.
static i32 askspawn(Lib *l, Arena *a, Spawn *s, Slice<Str> *wrap)
{
    if (!l->cfg.spawn) return 0;
    ub_spawn q = {};
    q.path    = hoststr(a, s->path);
    q.argc    = (int)s->argc;
    q.argv    = hoststrs(a, {s->argv, s->argc});
    q.env     = hoststrs(a, {s->env, s->nenv});
    q.cwd     = s->cwd.len ? hoststr(a, s->cwd) : 0;
    q.cmdline = s->cmdline.len ? hoststr(a, s->cmdline) : 0;
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
