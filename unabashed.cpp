// unabashed: a faithful clone of GNU bash 5.3
//
// Unity build: a platform layer (main_win32.cpp, main_posix.cpp,
// main_test.cpp, main_fuzz.cpp) includes this file and implements the
// os_* API. Design notes are in notes/, starting with notes/plan.md.
//
// This is free and unencumbered software released into the public domain.
#include "base.cpp"


#include "api.cpp"


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
#include "arith.cpp"
#include "pattern.cpp"
#include "brace.cpp"
#include "b_test.cpp"
#include "b_printf.cpp"
#include "sched.cpp"
#include "shell.cpp"
#include "array.cpp"
#include "vars.cpp"
#include "expand.cpp"
#include "exec.cpp"
#include "proc.cpp"
#include "builtins.cpp"
#include "trap.cpp"
#include "jobs.cpp"


// Startup

static void varchanged(Shell *sh, Str name)
{
    if (name == "LC_ALL" || name == "LC_CTYPE" || name == "LANG") {
        Arena none = {};
        Str v = getvar(sh, &none, "LC_ALL").value;
        if (!v.len) v = getvar(sh, &none, "LC_CTYPE").value;
        if (!v.len) v = getvar(sh, &none, "LANG").value;
        b32 utf8 = 0;
        for (iz i = 0; i+4 < v.len+1; i++) {
            Str t = cuthead(v, i);
            if (startswith(t, "UTF-8") || startswith(t, "utf8") ||
                    startswith(t, "UTF8") || startswith(t, "utf-8")) {
                utf8 = 1;
            }
        }
        if (sh->lexer) sh->lexer->utf8 = utf8;
        sh->utf8 = utf8;
    } else if (name == "_") {
        sh->lastargdirty = 1;  // no longer the dynamic $_
    } else if (name == "BASH_XTRACEFD") {
        // Traces go to an open descriptor it names, else to stderr
        Arena none = {};
        Got g = getvar(sh, &none, name);
        i64 fd = 0;
        sh->tracefd = 2;
        if (g.value.len && parsei64(g.value, &fd) && fd>=0 && fd<FDMAX && sh->fds->fd[fd]) {
            sh->tracefd = (i32)fd;
        } else if (g.value.len) {
            byte  mem[512];
            Arena scratch = {mem, mem+sizeof(mem)};
            if (g.value.len > 256) g.value = takehead(g.value, 256);
            shellerror(sh, scratch, concat3(&scratch, "BASH_XTRACEFD: ", g.value,
                                            Str(": invalid value for trace file descriptor")));
        }
    } else if (name == "PIPESTATUS") {
        sh->pstatdirty = 1;
    } else if (name == "PATH") {
        sh->cmdhash = 0;  // bash forgets remembered paths
    } else if (name == "OPTIND") {
        // Assigning or unsetting OPTIND restarts getopts at a word start
        if (!sh->ingetopts) sh->optchar = 1;
    } else if (name == "POSIXLY_CORRECT") {
        Arena none = {};
        if (getvar(sh, &none, name).set) sh->opt[O_POSIX] = 1;
    } else if (name == "GLOBIGNORE") {
        // A non-empty value turns dotglob on, and unsetting turns it off
        Arena none = {};
        Got g = getvar(sh, &none, name);
        if (g.value.len) {
            sh->shopt[S_DOTGLOB] = 1;
        } else if (!g.set) {
            sh->shopt[S_DOTGLOB] = 0;
        }
    }
}

static void dynamic(Shell *sh, Str name, i32 dyn, u32 attrs = 0)
{
    Value *v = setattrs(sh, name, attrs & ~(V_ARRAY|V_ASSOC), 0);
    v->dyn    = dyn;
    v->attrs |= attrs;
}

// Define a function a parent exported as BASH_FUNC_name%%="() {...}".
// As in bash since Shellshock, the value must be a function definition
// and nothing more.
static void importfunc(Shell *sh, Arena scratch, Str var, Str value)
{
    Str name = cuttail(cuthead(var, 10), 2);
    if (!name.len || !startswith(value, "() {")) return;
    Lexer *L = newlexer(&scratch, concat3(&scratch, name, " ", value));
    L->stackmax = (iz)1<<20;
    Parsed p = parseunit(L);
    Node  *n = p.cmd;
    if (p.status!=PARSE_OK || !n || n->kind!=N_FUNCDEF || !(n->name->text == name) ||
            parseunit(L).status!=PARSE_EOF) {
        return;
    }
    Node *body = n->a;
    if (n->redirs) {
        Node *g = alloc<Node>(&scratch);
        *g = *body;
        g->redirs = n->redirs;
        body = g;
    }
    definefunc(sh, scratch, name, body);
    Func *f = findfunc(sh, name);
    if (f) f->attrs |= V_EXPORT;
}

// A non-interactive shell first runs the file BASH_ENV names, after
// expanding it, unless in POSIX mode.
static void sourcebashenv(Shell *sh, Arena scratch)
{
    Got g = getvar(sh, &scratch, "BASH_ENV");
    if (sh->opt[O_POSIX] || !g.value.len) return;
    Str file = expandstring(sh, &scratch, g.value);
    i32 err;
    Str src = readwholefile(sh, &scratch, file, &err);
    if (err) {
        shellerror(sh, scratch, concat3(&scratch, file, ": ", errtext(err)));
        return;
    }
    runsource(sh, src, file, scratch);
}

static void initvars(Shell *sh, OsInfo *info, Arena scratch)
{
    // The environment: exported variables
    for (iz i = 0; i < info->nenv; i++) {
        Cut c = cut(info->env[i], '=');
        if (c.ok && startswith(c.head, "BASH_FUNC_") && endswith(c.head, "%%")) {
            importfunc(sh, scratch, c.head, c.tail);
            continue;
        }
        if (!c.ok || !validvarname(c.head)) continue;
        setvar(sh, scratch, c.head, c.tail, SV_FORCE);
        setattrs(sh, c.head, V_EXPORT, 0);
    }
    Arena *a = &scratch;
    Got g;

    setvar(sh, scratch, "BASH", info->args && info->nargs ? Str("/usr/local/bin/bash") : Str("bash"));
    setvar(sh, scratch, "BASH_VERSION", "5.3.0(1)-release");
    setvar(sh, scratch, "HOSTTYPE", "x86_64");
    setvar(sh, scratch, "MACHTYPE", "x86_64-pc-linux-gnu");
    setvar(sh, scratch, "OSTYPE", "linux-gnu");
    setvar(sh, scratch, "OPTERR", "1");
    setvar(sh, scratch, "OPTIND", "1");
    setvar(sh, scratch, "IFS", " \t\n");
    setvar(sh, scratch, "PS4", "+ ");
    setvar(sh, scratch, "HOSTNAME", info->host.len ? info->host : Str("oracle"));
    setvar(sh, scratch, "PPID", fmti64(a, info->ppid));
    setattrs(sh, "PPID", V_INTEGER|V_READONLY, 0);
    setvar(sh, scratch, "UID", fmti64(a, info->uid));
    setattrs(sh, "UID", V_INTEGER|V_READONLY, 0);
    setvar(sh, scratch, "EUID", fmti64(a, info->euid));
    setattrs(sh, "EUID", V_INTEGER|V_READONLY, 0);
    g = getvar(sh, a, "TERM");
    if (!g.set) setvar(sh, scratch, "TERM", "dumb");
    g = getvar(sh, a, "SHELL");
    if (!g.set) setvar(sh, scratch, "SHELL", "/usr/local/bin/bash");
    g = getvar(sh, a, "PATH");
    if (!g.set) {
        setvar(sh, scratch, "PATH",
               "/usr/local/bin:/usr/local/sbin:/usr/bin:/usr/sbin:/bin:/sbin:.");
    }

    // PWD: inherited if it names the working directory
    g = getvar(sh, a, "PWD");
    Str pwd = info->cwd.len ? info->cwd : Str("/");
    if (g.set && isabsolute(sh, g.value)) {
        Stat x = os_stat(sh->os, scratch, g.value, 1);
        Stat y = os_stat(sh->os, scratch, pwd, 1);
        if (!x.err && !y.err && x.ino==y.ino && x.dev==y.dev) pwd = g.value;
    }
    setvar(sh, scratch, "PWD", pwd);
    setattrs(sh, "PWD", V_EXPORT, 0);
    sh->cwd = sclone(sh->store, info->cwd.len ? info->cwd : Str("/"));
    g = getvar(sh, a, "OLDPWD");
    if (!g.set) setattrs(sh, "OLDPWD", V_EXPORT, 0);

    // SHLVL counts nested shells
    g = getvar(sh, a, "SHLVL");
    i64 lvl = 0;
    if (g.set && !parsei64(g.value, &lvl)) lvl = 0;
    lvl = lvl<0 ? 0 : lvl+1;
    setvar(sh, scratch, "SHLVL", fmti64(a, lvl));
    setattrs(sh, "SHLVL", V_EXPORT, 0);

    dynamic(sh, "LINENO", DYN_LINENO, V_INTEGER);
    dynamic(sh, "RANDOM", DYN_RANDOM, V_INTEGER);
    dynamic(sh, "SRANDOM", DYN_SRANDOM, V_INTEGER);
    dynamic(sh, "SECONDS", DYN_SECONDS, V_INTEGER);
    dynamic(sh, "EPOCHSECONDS", DYN_EPOCHSECONDS);
    dynamic(sh, "EPOCHREALTIME", DYN_EPOCHREALTIME);
    dynamic(sh, "BASHPID", DYN_BASHPID, V_INTEGER);
    dynamic(sh, "BASH_SUBSHELL", DYN_BASH_SUBSHELL);
    dynamic(sh, "BASH_COMMAND", DYN_BASH_COMMAND);
    dynamic(sh, "FUNCNAME", DYN_FUNCNAME, V_ARRAY);
    dynamic(sh, "BASH_LINENO", DYN_BASH_LINENO, V_ARRAY);
    dynamic(sh, "BASH_SOURCE", DYN_BASH_SOURCE, V_ARRAY);
    dynamic(sh, "BASH_ARGV", DYN_BASH_ARGV, V_ARRAY);
    dynamic(sh, "BASH_ARGC", DYN_BASH_ARGC, V_ARRAY);
    dynamic(sh, "PIPESTATUS", DYN_PIPESTATUS, V_ARRAY);
    dynamic(sh, "DIRSTACK", DYN_DIRSTACK, V_ARRAY);
    dynamic(sh, "BASH_ALIASES", DYN_BASH_ALIASES, V_ASSOC);
    dynamic(sh, "BASH_CMDS", DYN_BASH_CMDS, V_ASSOC);
    dynamic(sh, "_", DYN_UNDERSCORE);
    sh->npstat = 1;
    static Str versinfo[] = {"5", "3", "0", "1", "release", "x86_64-pc-linux-gnu"};
    for (iz i = 0; i < countof(versinfo); i++) {
        setelem(sh, scratch, "BASH_VERSINFO", i, {}, versinfo[i]);
    }
    setattrs(sh, "BASH_VERSINFO", V_READONLY, 0);
    setelem(sh, scratch, "GROUPS", 0, {}, fmti64(a, info->gid));
    setattrs(sh, "GROUPS", V_NOASSIGN, 0);
    dynamic(sh, "BASH_ARGV0", DYN_BASH_ARGV0);
    dynamic(sh, "HISTCMD", DYN_HISTCMD, V_INTEGER);
    varchanged(sh, "LANG");
}

static File *stdfile(Shell *sh, Handle h, i32 flags)
{
    File *f = newfile(sh, F_OS, flags);
    f->h = h;
    return f;
}

// Commands from standard input, read as the parser asks for them, a
// line at a time, so that the commands it runs can read what follows
// (bash's buffered input). A seekable input is read in blocks and then
// rewound past the line; anything else a byte at a time. As in bash,
// it is whatever fd 0 is now, which exec may have changed.
struct StdinSource {
    Shell *sh;
    Buf    buf;
};

// Show PS1 or PS2 on standard error, expanded as ${PS1@P} would be. An
// expansion error shows nothing.
static void showprompt(Shell *sh, b32 first)
{
    Arena  a = *scratchat(sh, sh->capdepth+1);
    Escape e = {};
    e.up    = sh->esc;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    i32 status = sh->status;
    if (!__builtin_setjmp(e.jmp)) {
        Str ps = expandstring(sh, &a, first ? Str("${PS1@P}") : "${PS2@P}");
        fdwrite(sh, 2, ps);
    } else {
        unwind(sh, e.undo);
    }
    sh->esc    = e.up;
    sh->status = status;
}

// Read the next bytes of the command input. At an interactive shell's
// prompt, Ctrl-C discards the line: -2.
static iz readstdin(Shell *sh, File *f, u8 *buf, iz len)
{
    if (!sh->interactive) return fileread(sh, f, buf, len);
    Escape e = {};
    e.up    = sh->esc;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    if (__builtin_setjmp(e.jmp)) {
        sh->esc = e.up;
        if (e.why != ESC_INTR) escape(sh, e.why);
        return -2;
    }
    iz n = fileread(sh, f, buf, len);
    sh->esc = e.up;
    return n;
}

static b32 fillstdin(void *ctx, Lexer *L)
{
    StdinSource *in = (StdinSource *)ctx;
    Shell *sh    = in->sh;
    File  *f     = sh->fds->fd[0];
    Buf   *b     = &in->buf;
    iz     start = b->len;
    b32 seekable = f && f->kind==F_OS && os_seek(sh->os, f->h, 0, 1)>=0;
    if (sh->interactive) {
        showprompt(sh, L->newunit);
        L->newunit = 0;
    }
    for (b32 done = 0; !done;) {
        iz n = 0;
        if (seekable) {
            reserve(b, 4096);
            n = readstdin(sh, f, b->data+b->len, 4096);
        } else {
            u8 c;
            n = readstdin(sh, f, &c, 1);
            if (n == 1) {
                putbyte(b, c);
                done = c == '\n';
                continue;
            }
        }
        if (n == -2) {
            // Ctrl-C at the prompt: an empty line instead, status 130
            fdwrite(sh, 2, "\n");
            sh->status = 128 + 2;
            b->len = start;
            putbyte(b, '\n');
            break;
        }
        if (!seekable || n<=0) break;  // end of input, or an error
        iz keep = n;
        for (iz i = 0; i < n; i++) {
            if (b->data[b->len+i] == '\n') {
                keep = i + 1;
                done = 1;
                break;
            }
        }
        if (keep < n) os_seek(sh->os, f->h, keep-n, 1);
        b->len += keep;
    }
    L->src = {b->data, b->len};
    return b->len > start;
}

static i32 sh_main(Os *os, OsInfo *info)
{
    Arena perm    = newarena(os, info->reserve);
    Arena scratch = newarena(os, info->reserve);

    Store *st = alloc<Store>(&perm);
    st->a    = newarena(os, info->reserve);
    st->base = st->a.beg;

    Shell *sh = alloc<Shell>(&perm);
    sh->os      = os;
    sh->sched   = newsched(os, &perm, info->reserve, info->chaos, info->std[2]);
    sh->store   = st;
    sh->fds     = alloc<Fdtab>(&perm);
    sh->reserve = info->reserve;
    sh->name    = info->nargs ? info->args[0] : Str("bash");
    sh->iname   = sh->name;
    sh->pid     = info->pid;
    sh->bashpid = info->pid;
    sh->lastpid = info->pid;
    sh->ppid    = info->ppid;
    sh->uid     = info->uid;
    sh->euid    = info->euid;
    sh->gid     = info->gid;
    sh->egid    = info->egid;
    sh->umask   = 022;
    sh->windows = info->windows;
    sh->start   = os_now(os);
    sh->seed    = (u32)(sh->start ^ info->pid);

    Escape top = {};
    sh->esc = &top;
    Sigproc *sig = alloc<Sigproc>(&perm);
    sigenter(sh, sig, sh->pid, sh->sched->cur);
    sh->sched->top = sig;
    sh->hardign = info->sigign;
    byte stackmark = 0;
    sh->stacktop = &stackmark;
    sh->stackmax = info->stack ? info->stack - ((iz)1<<16) : (iz)1<<19;

    for (i32 i = 0; i < 3; i++) {
        File *f = stdfile(sh, info->std[i], i ? OF_WRITE : OF_READ);
        sh->fds->fd[i] = f;
    }
    sh->fds->top = 3;

    sh->opt[O_BRACEEXPAND] = 1;
    sh->opt[O_HASHALL]     = 1;
    sh->opt[O_INTERACTIVECOMMENTS] = 1;
    static i32 shoptson[] = {
        S_CHECKWINSIZE, S_CMDHIST, S_COMPLETE_FULLQUOTE, S_EXTQUOTE,
        S_FORCE_FIGNORE, S_GLOBASCIIRANGES, S_GLOBSKIPDOTS, S_HOSTCOMPLETE,
        S_INTERACTIVE_COMMENTS, S_PATSUB_REPLACEMENT, S_PROGCOMP,
        S_PROMPTVARS, S_SOURCEPATH,
    };
    for (iz i = 0; i < countof(shoptson); i++) {
        sh->shopt[shoptson[i]] = 1;
    }

    if (!__builtin_setjmp(top.jmp)) {
        initvars(sh, info, scratch);
        entrytraps(sh);

        // Options before the command string or script
        iz  i = 1;
        Str cmd = {};
        b32 havecmd = 0;
        b32 selftest_ = 0;
        for (; i < info->nargs; i++) {
            Str a = info->args[i];
            if (a == "--unabashed-selftest") {
                selftest_ = 1;
                break;
            }
            if (a=="--" || a=="-") {
                i++;
                break;
            }
            if (a=="--norc" || a=="--noprofile" || a=="--noediting") continue;
            if (a == "--posix") {
                sh->opt[O_POSIX] = 1;
                continue;
            }
            if (a.len<2 || (a[0]!='-' && a[0]!='+') || startswith(a, "--")) break;
            b32 on = a[0] == '-';
            for (iz k = 1; k < a.len; k++) {
                u8 c = a[k];
                if (c == 'c') {
                    sh->cflag = 1;
                } else if (c == 's') {
                    sh->sflag = 1;
                } else if (c == 'i') {
                    sh->interactive = 1;
                } else if (c == 'o') {
                    if (i+1 < info->nargs) setoptionbyname(sh, info->args[++i], on);
                } else if (c=='l' || c=='r' || c=='O' || c=='D') {
                    // login, restricted: accepted
                } else {
                    for (i32 o = 0; o < O_COUNT; o++) {
                        if (setoptions[o].letter == c) sh->opt[o] = on;
                    }
                }
            }
        }

        if (sh->interactive) {
            sh->shopt[S_EXPAND_ALIASES] = 1;
            if (!getvar(sh, &scratch, "PS1").set) setvar(sh, scratch, "PS1", "\\s-\\v\\$ ");
            if (!getvar(sh, &scratch, "PS2").set) setvar(sh, scratch, "PS2", "> ");
        } else if (!selftest_) {
            sourcebashenv(sh, scratch);
        }

        if (selftest_) {
            Output *out = newoutput(os, &perm, info->std[1]);
            sh->status = selftest(os, &perm, out);
            flush(out);
        } else if (sh->cflag) {
            if (i >= info->nargs) {
                Buf b(&scratch, 64);
                print(&b, sh->name);
                print(&b, ": -c: option requires an argument\n");
                fdwrite(sh, 2, finish(&b));
                sh->status = 2;
            } else {
                cmd = info->args[i++];
                havecmd = 1;
                setvar(sh, scratch, "BASH_EXECUTION_STRING", cmd);
                if (i < info->nargs) sh->name = info->args[i++];
                sh->args.v = info->args + i;
                sh->args.n = info->nargs - i;
                sh->iname  = "-c";
                runsource(sh, cmd, "-c", scratch);
            }
        } else if (i<info->nargs && !sh->sflag) {
            // A script file
            Str file = info->args[i++];
            sh->name   = file;
            sh->iname  = file;
            sh->args.v = info->args + i;
            sh->args.n = info->nargs - i;
            i32  err;
            Str  src = readwholefile(sh, &scratch, file, &err);
            Stat st  = os_stat(os, scratch, absolute(sh, &scratch, file), 1);
            Str  bad = {};
            if (!err && !st.err && st.type==FT_DIR) {
                bad = errtext(E_ISDIR);
            } else if (!err && binaryfile(src)) {
                bad = "cannot execute binary file";
            }
            if (bad.data) {
                // with $0 already the script, as bash
                Buf b(&scratch, 64);
                print(&b, file);
                print(&b, ": ");
                print(&b, file);
                print(&b, ": ");
                print(&b, bad);
                putbyte(&b, '\n');
                fdwrite(sh, 2, finish(&b));
                sh->status = 126;
            } else if (err) {
                Buf b(&scratch, 64);
                print(&b, info->nargs ? info->args[0] : Str("bash"));
                print(&b, ": ");
                print(&b, file);
                print(&b, ": ");
                print(&b, errtext(err));
                putbyte(&b, '\n');
                fdwrite(sh, 2, finish(&b));
                sh->status = err==E_NOENT ? 127 : 126;
            } else {
                runsource(sh, src, file, scratch, 1);
            }
        } else {
            // Commands from standard input
            sh->sflag  = 1;
            sh->args.v = info->args + i;
            sh->args.n = info->nargs>i ? info->nargs-i : 0;
            StdinSource in = {};
            in.sh  = sh;
            Arena inarena = newarena(os, sh->reserve);
            in.buf = Buf(&inarena, 4096);
            runsource(sh, {}, sh->name, scratch, 1, 1, fillstdin, &in);
        }
        (void)havecmd;
    } else if (top.why==ESC_FATAL && sh->cflag) {
        sh->status = 127;  // as bash's -c reports a fatal error
    }
    if (!sh->execed && !sh->killsig) {
        // Signals still pending, then the EXIT trap
        if (!__builtin_setjmp(top.jmp)) {
            sh->exiting = 0;
            checksigs(sh, scratch);
            sh->exiting = 1;
            if (sh->errexited) unwindtemp(sh, 0);
            runexittrap(sh, scratch);
        }
    }
    if (sh->jobs) orphanjobs(sh);
    if (sh->psubs || sh->sched->orphans) {
        // Procsubs and background jobs still running end on EOF once
        // the shell's ends of their pipes close, as at a process exit,
        // and are waited for: a fiber cannot outlive its process. The
        // platform's own handles stay open.
        Fdtab *t = sh->fds;
        for (i32 fd = 0; fd < t->top; fd++) {
            File *f = t->fd[fd];
            b32 std = f && f->kind==F_OS &&
                      (f->h==info->std[0] || f->h==info->std[1] || f->h==info->std[2]);
            if (f && !std) {
                fileunref(sh, f);
                t->fd[fd] = 0;
            }
        }
        joinpsubs(sh);
        joinorphans(sh->sched);
    }
    if (info->ownstd) closeall(sh);
    freesched(sh->sched);
    if (sh->killsig) return WSIGNALED | (128 + sh->killsig);
    return sh->status & 255;
}
