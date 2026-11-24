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
