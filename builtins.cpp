// Builtin commands
// This is free and unencumbered software released into the public domain.

static void histoption(Shell *, i32 opt, b32 on);  // history.cpp
static b32  exitwarning(Shell *);                  // jobs.cpp

static Str helpsynopsis(Str name);  // help.cpp

static Str usage(Str name)
{
    static struct { Str name; Str text; } usages[] = {
        {"cd",       "cd [-L|[-P [-e]]] [-@] [dir]"},
        {"declare",  "declare [-aAfFgiIlnrtux] [name[=value] ...] or declare -p [-aAfFilnrtux] [name ...]"},
        {"typeset",  "typeset [-aAfFgiIlnrtux] name[=value] ... or typeset -p [-aAfFilnrtux] [name ...]"},
        {"local",    "local [option] name[=value] ..."},
        {"export",   "export [-fn] [name[=value] ...] or export -p [-f]"},
        {"readonly", "readonly [-aAf] [name[=value] ...] or readonly -p"},
        {"unset",    "unset [-f] [-v] [-n] [name ...]"},
        {"read",     "read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]"},
        {"mapfile",  "mapfile [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]"},
        {"readarray", "readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]"},
        {"shift",    "shift [n]"},
        {"set",      "set [-abefhkmnptuvxBCEHPT] [-o option-name] [--] [-] [arg ...]"},
        {"shopt",    "shopt [-pqsu] [-o] [optname ...]"},
        {"type",     "type [-afptP] name [name ...]"},
        {"command",  "command [-pVv] command [arg ...]"},
        {"compgen",  "compgen [-V varname] [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [word]"},
        {"pwd",      "pwd [-LP]"},
        {"alias",    "alias [-p] [name[=value] ... ]"},
        {"unalias",  "unalias [-a] name [name ...]"},
        {"umask",    "umask [-p] [-S] [mode]"},
        {"enable",   "enable [-a] [-dnps] [-f filename] [name ...]"},
        {"pushd",    "pushd [-n] [+N | -N | dir]"},
        {"popd",     "popd [-n] [+N | -N]"},
        {"dirs",     "dirs [-clpv] [+N] [-N]"},
        {"ulimit",   "ulimit [-SHabcdefiklmnpqrstuvxPRT] [limit]"},
        {"getopts",  "getopts optstring name [arg ...]"},
        {"hash",     "hash [-lr] [-p pathname] [-dt] [name ...]"},
        {"history",  "history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]"},
        {"fc",       "fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]"},
        {"trap",     "trap [-Plp] [[action] signal_spec ...]"},
        {"kill",     "kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]"},
        {"return",   "return [n]"},
        {"caller",   "caller [expr]"},
        {"exit",     "exit [n]"},
        {"source",   "source [-p path] filename [arguments]"},
        {".",        ". [-p path] filename [arguments]"},
        {"printf",   "printf [-v var] format [arguments]"},
        {"let",      "let arg [arg ...]"},
        {"eval",     "eval [arg ...]"},
        {"exec",     "exec [-cl] [-a name] [command [argument ...]] [redirection ...]"},
        {"wait",     "wait [-fn] [-p var] [id ...]"},
        {"jobs",     "jobs [-lnprs] [jobspec ...] or jobs -x command [args]"},
        {"disown",   "disown [-h] [-ar] [jobspec ... | pid ...]"},
        {"suspend",  "suspend [-f]"},
        {"builtin",  "builtin [shell-builtin [arg ...]]"},
    };
    for (iz i = 0; i < countof(usages); i++) {
        if (usages[i].name == name) return usages[i].text;
    }
    Str s = helpsynopsis(name);
    return s.data ? s : name;
}

// Bash prints the usage line without the "NAME: line N: " prefix.
static void usageerror(Shell *sh, Arena a, Str name)
{
    sh->bifatal = 1;
    flushbout(sh);
    Str msg = concat3(&a, name, ": usage: ", usage(name));
    fdwrite(sh, 2, concat(&a, msg, "\n"));
}

static void invalidoption(Shell *sh, Arena a, Str name, Str opt)
{
    builtinerror(sh, a, name, concat(&a, opt, ": invalid option"));
    usageerror(sh, a, name);
}

// Options as bash's builtins scan them: clusters of the letters in
// opts, up to "--" or the first word that is not an option. Each letter
// seen sets its bit (its index in opts) in *flags. The first operand's
// index, or -1 once an invalid option was reported with the usage.
static iz scanopts(Shell *sh, Arena a, Str name, Str *args, iz nargs,
                   Str opts, u32 *flags)
{
    *flags = 0;
    iz i = 1;
    for (; i < nargs; i++) {
        Str w = args[i];
        if (w.len<2 || w[0]!='-') break;
        if (w == "--") return i + 1;
        for (iz k = 1; k < w.len; k++) {
            iz at = 0;
            for (; at<opts.len && opts[at]!=w[k]; at++) {}
            if (at == opts.len) {
                u8 opt[2] = {'-', w[k]};
                invalidoption(sh, a, name, clone(&a, Str{opt, 2}));
                return -1;
            }
            *flags |= (u32)1 << at;
        }
    }
    return i;
}

static void notident(Shell *sh, Arena a, Str name, Str word)
{
    sh->bifatal = 1;
    builtinerror(sh, a, name, concat3(&a, "`", word, "': not a valid identifier"));
}

static b32 parsei64(Str s, i64 *out)
{
    s = trim(s);
    if (!s.len) return 0;
    b32 neg = 0;
    iz  i   = 0;
    if (s[0]=='-' || s[0]=='+') {
        neg = s[0]=='-';
        i = 1;
    }
    if (i == s.len) return 0;
    u64 v = 0;
    for (; i < s.len; i++) {
        if (!digit(s[i])) return 0;
        u64 nv = v*10 + (u64)(s[i] - '0');
        if (nv/10 != v) return 0;
        v = nv;
    }
    if (v > (u64)0x7fffffffffffffff + neg) return 0;
    *out = neg ? (i64)(0 - v) : (i64)v;
    return 1;
}

// Skip a leading "--" end-of-options marker.
static iz skipdashdash(Str *args, iz nargs, iz i)
{
    if (i<nargs && args[i]=="--") i++;
    return i;
}


// Trivial builtins

static i32 b_colon(Shell *, Str *, iz, Arena) { return 0; }
static i32 b_false(Shell *, Str *, iz, Arena) { return 1; }

static i32 b_echo(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 newline = 1;
    b32 escapes = sh->shopt[S_XPG_ECHO];
    iz  i = 1;
    for (; i < nargs; i++) {
        Str a = args[i];
        if (a.len<2 || a[0]!='-') break;
        b32 valid = 1;
        for (iz k = 1; k < a.len; k++) {
            valid &= a[k]=='n' || a[k]=='e' || a[k]=='E';
        }
        if (!valid) break;
        for (iz k = 1; k < a.len; k++) {
            if (a[k] == 'n') newline = 0;
            if (a[k] == 'e') escapes = 1;
            if (a[k] == 'E') escapes = 0;
        }
    }
    for (b32 first = 1; i < nargs; i++, first = 0) {
        if (!first) bprint(sh, " ");
        if (escapes) {
            b32 stop = 0;
            bprint(sh, echoescape(&scratch, args[i], 0, sh->utf8, &stop));
            if (stop) return 0;
        } else {
            bprint(sh, args[i]);
        }
    }
    if (newline) bprint(sh, "\n");
    return 0;
}

struct PfCtx {
    Shell *sh;
    Arena *a;
};

static i64 pfnow(void *ctx)
{
    PfCtx *c = (PfCtx *)ctx;
    return os_now(c->sh->os) / 1000000000;
}

static void pfflushout(void *ctx, Str s)
{
    PfCtx *c = (PfCtx *)ctx;
    bprint(c->sh, s);
}

static void pfsetint(void *ctx, Str name, i64 v)
{
    PfCtx *c = (PfCtx *)ctx;
    Arena  t = *c->a;  // past the output, which waits meanwhile
    Str    s = fmti64(&t, v);
    setvar(c->sh, t, name, s);
}

static void pferror(void *ctx, Str msg)
{
    PfCtx *c = (PfCtx *)ctx;
    builtinerror(c->sh, *c->a, "", msg);  // msg names printf
}

static i32 b_printf(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    iz  i   = 1;
    Str var = {};
    if (i<nargs && args[i]=="-v") {
        if (i+1 >= nargs) {
            builtinerror(sh, scratch, "printf", "-v: option requires an argument");
            usageerror(sh, scratch, "printf");
            return 2;
        }
        var = args[i+1];
        i += 2;
        Str rn, rs;
        if (!validvarname(var) && !splitrefonce(sh, var, &rn, &rs)) {
            notident(sh, scratch, "printf", var);  // before any output
            return 2;
        }
    } else if (i<nargs && args[i].len>1 && args[i][0]=='-' && args[i]!="--") {
        invalidoption(sh, scratch, "printf", takehead(args[i], 2));
        return 2;
    }
    i = skipdashdash(args, nargs, i);
    if (i >= nargs) {
        usageerror(sh, scratch, "printf");
        return 2;
    }
    PfCtx ctx = {sh, &scratch};
    PrintfEnv env = {};
    env.ctx        = &ctx;
    env.utf8       = sh->utf8;
    env.now        = pfnow;
    env.shellstart = sh->start / 1000000000;
    if (find(args[i], "%(") >= 0) {
        env.zone = shellzone(sh, &scratch);
    }
    env.error      = pferror;
    env.flush      = var.data ? 0 : pfflushout;
    env.setint     = pfsetint;
    Buf out(&scratch, 256);
    i32 status = printfmt(&env, &out, args[i], args+i+1, nargs-i-1);
    if (var.data) {
        if (!assignto(sh, scratch, var, finish(&out))) return 1;
    } else {
        bprint(sh, cuthead(finish(&out), env.flushed));
    }
    return status;
}

static i32 b_test(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    Str        name = args[0];
    TestCtx    tc   = {sh, &scratch, 0};
    TestEnv    env  = testenv(&tc);
    TestResult r    = name=="["
        ? testbracket(&env, &scratch, args+1, nargs-1)
        : testcmd(&env, &scratch, args+1, nargs-1);
    if (r.status == 2 && r.err.len) {
        builtinerror(sh, scratch, name, r.err);
    }
    return r.status;
}


// Control flow

static void logoutfiles(Shell *, Arena);  // unabashed.cpp
static i32  editline(Shell *, Str prompt, Buf *out, EdRead *);  // edit.cpp
static void sigsend(Shell *, Sigproc *, i32);  // trap.cpp

static i32 b_exit(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    if (sh->interactive && !sh->subshell) {
        fdwrite(sh, 2, sh->login ? Str("logout\n") : "exit\n");
        if (exitwarning(sh)) return 1;
    }
    // In a trap action, the status from before the action (POSIX)
    i32 status = sh->intrap ? sh->trapstatus : sh->status;
    iz  i = skipdashdash(args, nargs, 1);
    if (i < nargs) {
        i64 v;
        if (!parsei64(args[i], &v)) {
            // bash 5.3 reports it and carries on without exiting
            builtinerror(sh, scratch, "exit", concat(&scratch, args[i], ": numeric argument required"));
            sh->bifatal = 1;
            return 2;
        } else if (i+1 < nargs) {
            // As bash, abandon the -c, eval or source text
            builtinerror(sh, scratch, "exit", "too many arguments");
            sh->status = 2;
            escape(sh, ESC_ABORT);
        } else {
            status = (i32)(v & 255);
        }
    }
    sh->status = status;
    logoutfiles(sh, scratch);
    sh->status  = status;
    sh->exiting = 1;
    sh->exitcalled = 1;
    escape(sh, ESC_EXIT);
}

// logout: exit, in a login shell only
static i32 b_logout(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    if (!sh->login) {
        builtinerror(sh, scratch, "logout", "not login shell: use `exit'");
        return 1;
    }
    return b_exit(sh, args, nargs, scratch);
}

// Element i of an indexed (or dynamic) array variable, or null.
static Str arrayelem(Shell *sh, Arena *a, Str name, i64 i)
{
    Array *arr = varray(sh, a, lookup(sh, name));
    if (!arr || arr->bucket || i<0) return {};
    iz k = afind(arr, i);
    return k<0 ? Str{} : arr->e[k].val;
}

// caller [n]: "line file" of the current call, or "line func file" of
// frame n, from BASH_LINENO, FUNCNAME and BASH_SOURCE.
static i32 b_caller(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    // As bash, empty arrays fail before the options are looked at
    Str line0 = arrayelem(sh, &scratch, "BASH_LINENO", 0);
    Str src1  = arrayelem(sh, &scratch, "BASH_SOURCE", 0);
    if (!line0.data || !src1.data) return 1;
    iz i = 1;
    if (i<nargs && args[i]=="--") {
        i++;
    } else if (i<nargs && args[i].len>1 && args[i][0]=='-' && args[i]!="-") {
        invalidoption(sh, scratch, "caller", takehead(args[i], 2));
        return 2;
    }
    if (i == nargs) {
        Str src = arrayelem(sh, &scratch, "BASH_SOURCE", 1);
        bprint(sh, line0);
        bprint(sh, " ");
        bprint(sh, src.data ? src : Str("NULL"));
        bprint(sh, "\n");
        return 0;
    }
    if (!arrayelem(sh, &scratch, "FUNCNAME", 0).data) return 1;
    i64 n = 0;
    if (!parsei64(args[i], &n)) {
        builtinerror(sh, scratch, "caller", concat(&scratch, args[i], ": invalid number"));
        usageerror(sh, scratch, "caller");
        return 2;
    }
    Str line = arrayelem(sh, &scratch, "BASH_LINENO", n);
    Str src  = arrayelem(sh, &scratch, "BASH_SOURCE", n+1);
    Str func = arrayelem(sh, &scratch, "FUNCNAME", n+1);
    if (!line.data || !src.data || !func.data) return 1;
    bprint(sh, line);
    bprint(sh, " ");
    bprint(sh, func);
    bprint(sh, " ");
    bprint(sh, src);
    bprint(sh, "\n");
    return 0;
}

static i32 b_return(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    // As bash, the argument is checked first: a number, then alone
    // (too many abandons the -c, eval or source text), and only then
    // whether there is anything to return from
    iz  i   = skipdashdash(args, nargs, 1);
    b32 bad = 0;
    i64 v   = 0;
    if (i < nargs) {
        if (!parsei64(args[i], &v)) {
            builtinerror(sh, scratch, "return", concat(&scratch, args[i], ": numeric argument required"));
            sh->bifatal = 1;
            bad = 1;
        } else if (i+1 < nargs) {
            builtinerror(sh, scratch, "return", "too many arguments");
            sh->status = 2;
            escape(sh, ESC_ABORT);
        }
    }
    if (!sh->funcdepth && !sh->sourcedepth) {
        builtinerror(sh, scratch, "return", "can only `return' from a function or sourced script");
        sh->bifatal = 1;
        return 2;
    }
    // In a trap action itself, the status from before the action
    b32 intrap = sh->intrap && sh->funcdepth==sh->trapfuncs;
    i32 status = intrap ? sh->trapstatus : sh->status;
    sh->retprev = sh->status;
    if (bad) {
        status = 2;
    } else if (i < nargs) {
        status = (i32)(v & 255);
    }
    sh->flow = FLOW_RETURN;
    return status;
}

static i32 loopcontrol(Shell *sh, Str *args, iz nargs, Arena scratch, i32 flow)
{
    Str name = args[0];
    if (!sh->loopdepth) {
        // Outside a loop the arguments are never examined, and POSIX
        // mode does not complain
        if (!sh->opt[O_POSIX]) {
            builtinerror(sh, scratch, name, "only meaningful in a `for', `while', or `until' loop");
        }
        return 0;
    }
    i64 n = 1;
    iz  i = skipdashdash(args, nargs, 1);
    if (i < nargs) {
        if (!parsei64(args[i], &n)) {
            // Fatal in a non-interactive shell
            builtinerror(sh, scratch, name, concat(&scratch, args[i], ": numeric argument required"));
            sh->status  = 2;
            sh->exiting = 1;
            escape(sh, ESC_EXIT);
        }
        if (i+1 < nargs) {
            // As bash, abandon the -c, eval or source text
            builtinerror(sh, scratch, name, "too many arguments");
            sh->status = 2;
            escape(sh, ESC_ABORT);
        }
        if (n < 1) {
            builtinerror(sh, scratch, name, concat(&scratch, args[i], ": loop count out of range"));
            sh->flow = FLOW_BREAK;
            sh->flowcount = sh->loopdepth;
            return 1;
        }
    }
    sh->flow      = flow;
    sh->flowcount = (i32)(n<sh->loopdepth ? n : sh->loopdepth);
    return 0;
}

static i32 b_break(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    return loopcontrol(sh, args, nargs, scratch, FLOW_BREAK);
}

static i32 b_continue(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    return loopcontrol(sh, args, nargs, scratch, FLOW_CONTINUE);
}

static i32 b_eval(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    iz i = skipdashdash(args, nargs, 1);
    if (i==1 && nargs>1 && args[1].len>1 && args[1][0]=='-') {
        // eval takes no options
        invalidoption(sh, scratch, args[0], span(args[1].data, args[1].data+2));
        return 2;
    }
    if (i >= nargs) return 0;
    Buf b(&scratch, 256);
    for (iz k = i; k < nargs; k++) {
        if (k > i) putbyte(&b, ' ');
        print(&b, args[k]);
    }
    Str src = finish(&b);
    sh->evaldepth++;
    sh->status = 0;
    i32 line = sh->line;
    i32 status = runsource(sh, src, "eval", scratch, 0, line);
    sh->line = line;
    sh->evaldepth--;
    return status;
}

static i32 sourcetext(Shell *, Str *args, iz nargs, Str src, Arena, b32 startup);

// A file . cannot find: in POSIX mode this ends a non-interactive
// shell, unless `command` ran it
static i32 sourcenotfound(Shell *sh, Arena scratch, Str name, Str file, b32 cmd)
{
    builtinerror(sh, scratch, name, concat(&scratch, file, ": file not found"));
    if (sh->opt[O_POSIX] && !sh->interactive && !cmd) {
        sh->status  = 1;
        sh->exiting = 1;
        escape(sh, ESC_EXIT);
    }
    return 1;
}

// Restricted shells (rbash, -r, set -r)

// bash's "CMD: restricted", or "CMD: WHAT: restricted"; status 1
static i32 restricted(Shell *sh, Arena scratch, Str cmd, Str what = {})
{
    Str msg = what.data ? concat(&scratch, what, ": restricted") : Str("restricted");
    builtinerror(sh, scratch, cmd, msg);
    return 1;
}

static b32 restrictedhash(Shell *sh, Arena scratch, Str cmd, Str path)
{
    if (!sh->restricted) return 1;
    if (haspath(sh, path)) {
        if (cmd.data) {
            restricted(sh, scratch, cmd, path);
        } else {
            shellerror(sh, scratch, concat(&scratch, path, ": restricted"));
        }
        return 0;
    }
    if (!pathlookup(sh, &scratch, path).data) {
        Str msg = concat(&scratch, path, ": not found");
        if (cmd.data) {
            builtinerror(sh, scratch, cmd, msg);
        } else {
            shellerror(sh, scratch, msg);
        }
        return 0;
    }
    return 1;
}

// Make the shell restricted: the variables it may not change become
// readonly, as bash's maybe_make_restricted does.
static void makerestricted(Shell *sh)
{
    Str names[] = {"PATH", "SHELL", "ENV", "BASH_ENV", "HISTFILE"};
    for (iz i = 0; i < countof(names); i++) {
        setattrs(sh, names[i], V_READONLY, 0);
    }
    sh->restricted = 1;
}

static i32 b_source(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 cmd  = sh->viacommand;
    sh->viacommand = 0;
    Str name = args[0];
    iz  i    = 1;
    Got g    = {};  // the search path
    b32 only = 0;   // -p: search it alone, with no fallback
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") {
            i++;
            break;
        } else if (a[1] != 'p') {
            invalidoption(sh, scratch, name, span(a.data, a.data+2));
            return 2;
        } else if (a.len > 2) {
            g.value = span(a.data+2, a.data+a.len);
        } else if (i+1 < nargs) {
            g.value = args[++i];
        } else {
            builtinerror(sh, scratch, name, "-p: option requires an argument");
            usageerror(sh, scratch, name);
            return 2;
        }
        g.set = only = 1;
    }
    if (i >= nargs) {
        builtinerror(sh, scratch, name, "filename argument required");
        usageerror(sh, scratch, name);
        return 2;
    }
    if (sh->restricted && (only || haspath(sh, args[i]))) {
        return restricted(sh, scratch, name, args[i]);
    }
    Str file = args[i];
    Str path = file;
    b32 slash = 0;
    for (iz k = 0; k < file.len; k++) slash |= file[k]=='/';
    slash |= sh->pathmode!=PM_POSIX && haspath(sh, file);
    if (!slash && (only || sh->shopt[S_SOURCEPATH])) {
        if (!only) g = getvar(sh, &scratch, "PATH");
        b32 found = 0;
        for (Str rest = g.value; g.set;) {
            Cut c = cut(rest, listsep(sh));
            Str cand = joinpath(&scratch, c.head.len ? c.head : Str("."), file);
            Stat st = pathstat(sh, scratch, cand, 1);
            if (!st.err && st.type==FT_FILE) {
                path  = cand;
                found = 1;
                break;
            }
            if (!c.ok) break;
            rest = c.tail;
        }
        // Not found: the working directory next, except with -p and
        // in POSIX mode (bash's source_searches_cwd)
        if (!found && (only || sh->opt[O_POSIX])) {
            return sourcenotfound(sh, scratch, name, file, cmd);
        }
    } else if (!slash && sh->opt[O_POSIX]) {
        // as bash, never from the working directory, even without
        // sourcepath, which then searches nothing
        return sourcenotfound(sh, scratch, name, file, cmd);
    }
    Stat st = pathstat(sh, scratch, path, 1);
    if (!st.err && st.type==FT_DIR) {
        builtinerror(sh, scratch, name, concat(&scratch, file, ": is a directory"));
        return 1;
    }
    i32 err;
    Str src = readwholefile(sh, &scratch, path, &err);
    if (err) {
        shellerror(sh, scratch, concat3(&scratch, file, Str(": "), errtext(err)));
        if (sh->opt[O_POSIX] && !sh->interactive && !cmd) {
            sh->status  = 1;  // as a file not found
            sh->exiting = 1;
            escape(sh, ESC_EXIT);
        }
        return 1;
    }
    // As bash: many NUL bytes make a binary, and fewer are dropped
    iz nnul = 0;
    for (iz k = 0; k < src.len; k++) {
        src[k-nnul] = src[k];
        nnul += !src[k];
    }
    src.len -= nnul;
    if (nnul > 512) {
        builtinerror(sh, scratch, name, concat(&scratch, file, ": cannot execute binary file"));
        return 126;
    }
    args[i] = path;  // the name it goes by: "./f" when . found it
    return sourcetext(sh, args+i, nargs-i, src, scratch, 0);
}

// Run a sourced file's text: args[0] is its name, any others the
// positional parameters while it runs. A startup file (bash's own,
// not the source builtin's) ended by return leaves the status from
// before the return.
static i32 sourcetext(Shell *sh, Str *args, iz nargs, Str src, Arena scratch, b32 startup)
{
    Str  file  = args[0];
    Args saved = sh->args;
    b32  wasset = sh->argsset;
    sh->argsset = 0;
    if (nargs > 1) {
        sh->args.v = args + 1;
        sh->args.n = nargs - 1;
    }
    sh->sourcedepth++;
    sh->status = 0;
    i32 line = sh->line;
    Str srcname = sh->srcname;
    sh->srcname = file;
    Srcframe sf = {};
    sf.up    = sh->srcframes;
    sf.frame = sh->frame;
    sf.file  = file;
    sf.line  = line;
    if (nargs > 1) {
        sf.args.v    = args + 1;
        sf.args.n    = nargs - 1;
        sf.argpushed = sh->shopt[S_EXTDEBUG];
    } else {
        sf.args.v    = args;  // as bash: the name stands in
        sf.args.n    = 1;
        sf.argpushed = 1;
    }
    if (sh->traps) {
        for (i32 k = 0; k < 3; k++) sf.trapat[k] = sh->traps[TRAP_DEBUG+k].data;
    }
    sh->srcframes = &sf;
    // Pop the frame on the way out of an escape too (exit in the file,
    // then an EXIT action that reads BASH_SOURCE)
    Escape e = {};
    e.up    = sh->esc;
    e.bout  = sh->bout;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    i32 status = 0;
    if (__builtin_setjmp(e.jmp)) {
        sh->esc = e.up;
        sh->srcframes = sf.up;
        sh->retsrc    = 0;
        escape(sh, e.why);
    }
    status = runsource(sh, src, file, scratch);
    sh->lexeof    = 0;  // bash pretends the file ended in a newline
    sh->srcframes = sf.up;
    sh->srcname = srcname;
    if (sh->traps) {
        // As bash, RETURN runs where the file was sourced, though
        // DEBUG in its action follows the file's rules
        sh->line   = line;
        sh->retsrc = &sf;
        runreturntrap(sh, scratch);
        sh->retsrc = 0;
        status = sh->status;
    }
    sh->esc = e.up;
    if (sh->flow == FLOW_RETURN) {
        // Functions have taken their own returns: this one is the file's
        sh->flow = FLOW_NONE;
        if (startup) status = sh->retprev;
    }
    sh->line = line;
    sh->sourcedepth--;
    // As bash: arguments end with the file, unless it set its own
    // outside a function and was not sourced from one
    if (nargs>1 && (sh->funcdepth || !sh->argsset)) sh->args = saved;
    sh->argsset = nargs>1 ? 0 : wasset|sh->argsset;
    return status;
}


// Variables

static Str quotevalue(Arena *a, Str s)
{
    // declare -p style: double quotes, escaping " \ $ `, or $'...'
    // when unprintable
    if (ansicshouldquote(s)) return ansicquote(a, s);
    Buf b(a, s.len+2);
    putbyte(&b, '"');
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c=='"' || c=='\\' || c=='$' || c=='`') putbyte(&b, '\\');
        putbyte(&b, c);
    }
    putbyte(&b, '"');
    return finish(&b);
}

// A variable as declare -p shows it. Listed by export or readonly in
// POSIX mode, that command is named instead, with only -a or -A.
static void printdecl(Shell *sh, Arena *a, Str name, Value *v, Str cmd = {})
{
    if (cmd.len && sh->opt[O_POSIX]) {
        Str let = attrletters(a, v->attrs & (V_ARRAY|V_ASSOC));
        bprint(sh, cmd);
        bprint(sh, let.len ? concat3(a, " -", let, " ") : Str(" "));
    } else {
        Str let = attrletters(a, v->attrs & ~(V_LOCAL|V_TEMP));
        bprint(sh, "declare -");
        bprint(sh, let.len ? let : Str("-"));
        bprint(sh, " ");
    }
    bprint(sh, name);
    Array *arr = varray(sh, a, v);
    if (arr) {
        if (arr->isset) {
            bprint(sh, "=");
            bprint(sh, arrayassign(a, arr));
        }
        bprint(sh, "\n");
        return;
    }
    Str val = v->dyn ? dynvalue(sh, a, v) : v->str;
    if (val.data) {
        bprint(sh, "=");
        bprint(sh, quotevalue(a, val));
    }
    bprint(sh, "\n");
}

// A set variable as set and declare list it: name=value.
static void printassign(Shell *sh, Arena *a, Str name, Value *v)
{
    Array *arr = varray(sh, a, v);
    if (arr) {
        if (!arr->isset) return;
        bprint(sh, name);
        bprint(sh, "=");
        bprint(sh, arrayassign(a, arr));
        bprint(sh, "\n");
        return;
    }
    if (!v->str.data && !v->dyn) return;
    Str val = v->dyn ? dynvalue(sh, a, v) : v->str;
    if (!val.data) return;
    bprint(sh, name);
    bprint(sh, "=");
    if (ansicshouldquote(val)) {
        val = ansicquote(a, val);
    } else if (tracemetas(val)) {
        val = singlequote(a, val);  // as bash, only where it would be special
    }
    bprint(sh, val);
    bprint(sh, "\n");
}

struct VarEntry {
    Str    name;
    Value *v;
};

static void collectvars(Var *v, Slice<VarEntry> *out, Arena *a)
{
    if (!v) return;
    if (v->top) {
        VarEntry e = {v->name, v->top};
        *out = push(a, *out, e);
    }
    for (i32 i = 0; i < 4; i++) collectvars(v->child[i], out, a);
}

static b32 varless(VarEntry *a, VarEntry *b)
{
    return compare(a->name, b->name) < 0;
}

static Slice<VarEntry> sortedvars(Shell *sh, Arena *a)
{
    Slice<VarEntry> all = {};
    collectvars(sh->vars, &all, a);
    sort(all.data, all.len, varless, *a);
    return all;
}

struct FuncEntry {
    Str   name;
    Func *f;
};

static void collectfuncs(Func *f, Slice<FuncEntry> *out, Arena *a)
{
    if (!f) return;
    if (f->body) {
        FuncEntry e = {f->name, f};
        *out = push(a, *out, e);
    }
    for (i32 i = 0; i < 4; i++) collectfuncs(f->child[i], out, a);
}

static b32 funcless(FuncEntry *a, FuncEntry *b)
{
    return compare(a->name, b->name) < 0;
}

static void printfuncname(Shell *sh, Arena *a, Str name, Func *f)
{
    bprint(sh, "declare -f");
    bprint(sh, attrletters(a, (u32)f->attrs));
    bprint(sh, " ");
    bprint(sh, name);
    bprint(sh, "\n");
}

// A function's body, then its attributes if it has any.
static void printfunc(Shell *sh, Arena *a, Str name, Func *f)
{
    bprint(sh, printfunction(a, name, f->body, sh->opt[O_POSIX]));
    bprint(sh, "\n");
    if (f->attrs) printfuncname(sh, a, name, f);
}

// declare, typeset, local, export and readonly share this.
static i32 declarelike(Shell *sh, Str *args, iz nargs, Arena scratch, Str name,
                       u32 baseon, b32 local)
{
    u32 on = baseon, off = 0;
    b32 print = 0, funcs = 0, funcnames = 0, global = 0, inherit = 0;
    iz  i = 1;
    for (; i < nargs; i++) {
        Str a = args[i];
        if (a == "--") { i++; break; }
        if (a.len<2 || (a[0]!='-' && a[0]!='+')) break;
        b32 plus = a[0]=='+';
        if (plus && (name=="export" || name=="readonly")) break;  // a name
        for (iz k = 1; k < a.len; k++) {
            u32 bit = 0;
            if (name == "export") {
                // only -f, -n and -p
                switch (a[k]) {
                case 'f': funcs = 1; continue;
                case 'p': print = 1; continue;
                case 'n': off |= V_EXPORT; on &= ~V_EXPORT; continue;
                case 'a': on |= V_ARRAY; continue;
                case 'A': on |= V_ASSOC; continue;
                default:
                    invalidoption(sh, scratch, name, span(a.data+k-1, a.data+k+1));
                    return 2;
                }
            } else if (name == "readonly") {
                // only -a, -A, -f, -n (no readonly then) and -p
                switch (a[k]) {
                case 'f': funcs = 1; continue;
                case 'p': print = 1; continue;
                case 'n': on &= ~V_READONLY; continue;
                case 'a': on |= V_ARRAY; continue;
                case 'A': on |= V_ASSOC; continue;
                default:
                    invalidoption(sh, scratch, name, span(a.data+k-1, a.data+k+1));
                    return 2;
                }
            } else switch (a[k]) {
            case 'a': bit = V_ARRAY;    break;
            case 'A': bit = V_ASSOC;    break;
            case 'i': bit = V_INTEGER;  break;
            case 'l': bit = V_LOWER;    break;
            case 'u': bit = V_UPPER;    break;
            case 'c': bit = V_CAPITAL;  break;
            case 'n': bit = V_NAMEREF;  break;
            case 'r': bit = V_READONLY; break;
            case 't': bit = V_TRACE;    break;
            case 'x': bit = V_EXPORT;   break;
            case 'p': print = 1; continue;
            case 'f': funcs = 1; continue;
            case 'F': funcnames = 1; funcs = 1; continue;
            case 'g': global = 1; continue;
            case 'I': inherit = 1; continue;
            default:
                invalidoption(sh, scratch, name, span(a.data+k-1, a.data+k+1));
                return 2;
            }
            if (plus) {
                off |= bit;
                on &= ~bit;
            } else {
                on |= bit;
                // the case conversions replace one another
                u32 cases = V_LOWER|V_UPPER|V_CAPITAL;
                if (bit & cases) off |= cases & ~bit;
            }
        }
    }
    if ((on & V_LOWER) && (on & V_UPPER)) {
        on &= ~(V_LOWER|V_UPPER);
    }
    if (local && !sh->funcdepth) {
        builtinerror(sh, scratch, name, "can only be used in a function");
        return 1;
    }

    if (funcs) {
        u32 fattrs = V_EXPORT|V_READONLY|V_TRACE;
        if (i == nargs) {
            Slice<FuncEntry> all = {};
            collectfuncs(sh->funcs, &all, &scratch);
            sort(all.data, all.len, funcless, scratch);
            u32 want = on & fattrs;
            if (on & (V_INTEGER|V_NAMEREF|V_ARRAY|V_ASSOC)) return 0;  // none has
            for (iz k = 0; k < all.len; k++) {
                Func *f = all[k].f;
                if (want && ((u32)f->attrs & want) != want) continue;
                if (funcnames) {
                    printfuncname(sh, &scratch, all[k].name, f);
                } else {
                    printfunc(sh, &scratch, all[k].name, f);
                }
            }
            return 0;
        }
        if (i<nargs && name!="export" && name!="readonly") {
            // As bash, attributes no function can have (in its order)
            u32 bad[] = {V_INTEGER, V_NAMEREF, V_ARRAY, V_ASSOC};
            Str opt[] = {"-i", "-n", "-a", "-A"};
            for (i32 k = 0; k < countof(bad); k++) {
                if (on & bad[k]) {
                    builtinerror(sh, scratch, name, concat(&scratch, opt[k], ": invalid option"));
                    return 1;
                }
            }
        }
        i32 status = 0;
        for (; i < nargs; i++) {
            if (!print && find(args[i], "=")>=0 && !findfunc(sh, args[i])) {
                builtinerror(sh, scratch, name, "cannot use `-f' to make functions");
                status = 1;
                continue;
            }
            if (sh->opt[O_POSIX] && (name=="declare" || name=="typeset") &&
                    !validvarname(args[i])) {
                // POSIX mode cannot name such functions here, though it
                // defines and lists them
                notident(sh, scratch, name, args[i]);
                status = 1;
                continue;
            }
            Func *f = findfunc(sh, args[i]);
            if (!f) {
                if (name=="export" || name=="readonly") {
                    builtinerror(sh, scratch, name, concat(&scratch, args[i], ": not a function"));
                } else if (print && !funcnames) {
                    builtinerror(sh, scratch, name, concat(&scratch, args[i], ": not found"));
                }
                status = 1;
                continue;
            }
            b32 modify = !print && (on|off);  // attributes, not a listing
            if (modify && name=="export" && (on & V_EXPORT) &&
                    !exportablefunc(args[i])) {
                builtinerror(sh, scratch, name, concat(&scratch, args[i], ": cannot export"));
                status = 1;
            } else if (modify && (off & V_READONLY) && (f->attrs & V_READONLY)) {
                builtinerror(sh, scratch, name, concat(&scratch, args[i], ": readonly function"));
                status = 1;
            } else if (modify) {
                uz attrs = (f->attrs | (on & fattrs)) & ~(uz)(off & fattrs);
                jwrite(sh->store, &f->attrs, attrs);
            } else if (funcnames) {
                bprint(sh, args[i]);
                if (sh->shopt[S_EXTDEBUG]) {
                    // where it was defined, as bash's extdebug shows it
                    bprint(sh, " ");
                    bprint(sh, fmti64(&scratch, f->code->line));
                    bprint(sh, " ");
                    bprint(sh, f->code->src.len ? f->code->src : sh->name);
                }
                bprint(sh, "\n");
            } else if (print) {
                printfunc(sh, &scratch, args[i], f);
            } else {
                // as bash, the body alone when named without -p
                bprint(sh, printfunction(&scratch, args[i], f->body, sh->opt[O_POSIX]));
                bprint(sh, "\n");
            }
        }
        return status;
    }

    if (i==nargs && local) {
        // This function's locals, whatever the options
        Frame *f = sh->frame;
        if (f->opts) bprint(sh, "local -\n");
        // sorted by name, as bash lists them
        Slice<VarEntry> all = {};
        for (iz k = 0; k < f->nlocals; k++) {
            Var   *v = f->locals[k];
            Value *t = v->top;
            if (t && t->depth==sh->funcdepth && !(t->attrs & V_TEMP)) {
                VarEntry e = {v->name, t};
                all = push(&scratch, all, e);
            }
        }
        sort(all.data, all.len, varless, scratch);
        for (iz k = 0; k < all.len; k++) {
            printdecl(sh, &scratch, all[k].name, all[k].v);
        }
        return 0;
    }

    if (i == nargs) {
        // Listing
        Slice<VarEntry> all = sortedvars(sh, &scratch);
        sh->listing = 1;
        for (iz k = 0; k < all.len; k++) {
            Value *v = all[k].v;
            u32 want = on & ~V_LOCAL;
            if (want && (v->attrs & want) != want) continue;
            if (name=="readonly" && !(v->attrs & V_READONLY)) continue;
            if (name=="export" && !(v->attrs & V_EXPORT)) continue;
            if (name=="export" || name=="readonly") {
                printdecl(sh, &scratch, all[k].name, v, name);
            } else if (print || want) {
                printdecl(sh, &scratch, all[k].name, v);
            } else {
                printassign(sh, &scratch, all[k].name, v);
            }
        }
        sh->listing = 0;
        return 0;
    }

    i32 status = 0;
    u32 arrbits = on & (V_ARRAY|V_ASSOC);
    for (; i < nargs; i++) {
        Str a = args[i];
        if (local && a=="-") {
            // Save the set options for the function's return
            Frame *f = sh->frame;
            if (!f->opts) {
                f->opts = (b32 *)salloc(sh->store, O_COUNT*(iz)sizeof(b32));
                for (i32 k = 0; k < O_COUNT; k++) f->opts[k] = sh->opt[k];
            }
            continue;
        }
        if (print) {
            Value *v = rawlookup(sh, a);
            if (v && local && (v->depth!=sh->funcdepth || (v->attrs & V_TEMP))) {
                v = 0;  // local -p shows this function's own only
            }
            if (!v) {
                builtinerror(sh, scratch, name, concat(&scratch, a, ": not found"));
                status = 1;
                continue;
            }
            printdecl(sh, &scratch, a, v, name=="export" || name=="readonly" ? name : Str{});
            continue;
        }
        iz  eq = 0;
        for (; eq<a.len && a[eq]!='='; eq++) {}
        Str vname = takehead(a, eq);
        b32 append = vname.len && vname[vname.len-1]=='+' && eq<a.len;
        if (append) vname = cuttail(vname, 1);
        Str sub    = {};
        b32 hassub = 0;
        Str rn, rs;
        if (splitref(vname, &rn, &rs) || splitrefflat(sh, vname, &rn, &rs)) {
            vname  = rn;
            sub    = rs;
            hassub = 1;
        }
        if (!validvarname(vname) || (hassub && (name=="export" || name=="readonly"))) {
            notident(sh, scratch, name, a);
            status = 1;
            if (sh->opt[O_POSIX] && (name=="export" || name=="readonly")) {
                return 1;  // as bash, the rest left alone
            }
            continue;
        }
        Str  val      = eq<a.len ? cuthead(a, eq+1) : Str{};
        b32  wordcomp = i<sh->compargs.len && sh->compargs[i];
        if (name=="export" || name=="readonly") {
            // As bash, -a and -A make or convert an array only with a
            // value: export -a x and readonly -A x leave x as it is
            on = (on & ~(V_ARRAY|V_ASSOC)) | (eq<a.len ? arrbits : 0);
        }
        b32  mklocal  = local || (name!="export" && name!="readonly" && sh->funcdepth && !global);
        b32  onref    = !!(on & V_NAMEREF);
        if (onref && wordcomp && val.len>1 && val[0]=='(' && val[val.len-1]==')') {
            // declare -n r=(...): reported, then assigned as an array
            Str msg = concat(&scratch, vname, ": reference variable cannot be an array");
            builtinerror(sh, scratch, name, msg);
            on   &= ~V_NAMEREF;
            onref = 0;
            Value *old = rawlookup(sh, vname);
            if (old && (old->attrs & V_NAMEREF)) {
                Value *nv = setattrs(sh, vname, 0, V_NAMEREF);
                nv->str = {};
            }
        }
        if (onref && eq<a.len && !val.len && !append) {
            builtinerror(sh, scratch, name, "`': not a valid identifier");
            status = 1;
            continue;
        }
        if (onref) {
            if (hassub) {
                Str msg = concat(&scratch, takehead(a, eq), ": reference variable cannot be an array");
                builtinerror(sh, scratch, name, msg);
                status = 1;
                continue;
            }
            Str sn, ss;
            b32 self = eq<a.len && (val==vname || (splitref(val, &sn, &ss) && sn==vname));
            if (self && !sh->funcdepth) {
                Str msg = concat(&scratch, vname, ": nameref variable self references not allowed");
                builtinerror(sh, scratch, name, msg);
                status = 1;
                continue;
            } else if (self) {
                // and again as bash looks the new variable up
                Str msg = concat3(&scratch, "warning: ", vname, ": circular name reference");
                builtinerror(sh, scratch, name, msg);
                shellerror(sh, scratch, msg);
            }
            Value *old = rawlookup(sh, vname);
            if (append && old && (old->attrs & V_NAMEREF) && old->str.len) {
                // ref+=x appends to the name, which may come back to ref
                Str nv = concat(&scratch, old->str, val);
                if (nv==vname || (splitref(nv, &sn, &ss) && sn==vname)) {
                    shellerror(sh, scratch, concat(&scratch, vname, ": nameref variable self references not allowed"));
                    status = 1;
                    continue;
                }
            }
            if (val.len && !append && !validnameref(val)) {
                Str msg = concat3(&scratch, "`", val, "': invalid variable name for name reference");
                builtinerror(sh, scratch, name, msg);
                status = 1;
                continue;
            }
        }
        Str    refname = {};  // the nameref followed to vname, if any
        Value *raw = global ? globalvalue(sh, vname) : rawlookup(sh, vname);
        if (raw && (raw->attrs & V_NAMEREF) && hassub && eq<a.len) {
            // declare -n r; declare r[1]=x: r stops being a nameref
            shellerror(sh, scratch, concat3(&scratch, "warning: ", vname, ": removing nameref attribute"));
            Value *nv = setattrs(sh, vname, 0, V_NAMEREF);
            nv->str = {};
            raw = nv;
        } else if (raw && (raw->attrs & V_NAMEREF) && !onref) {
            // Otherwise declare acts on the variable a nameref names,
            // though in a function only through a nameref of its own
            if ((raw->attrs & V_EMPTYREF) && (off & V_READONLY)) {
                continue;  // as bash: +r finds no variable, silently
            }
            Ref r = resolveref(sh, vname, global);
            b32 offref = !!(off & V_NAMEREF);
            if (offref && r.last && r.last->str.data && (r.last->attrs & V_READONLY)) {
                builtinerror(sh, scratch, name, concat(&scratch, vname, ": readonly variable"));
                status = 1;
                continue;
            }
            if (offref && r.last && !(on & ~baseon) && eq==a.len && off==V_NAMEREF) {
                setattrs(sh, r.lastname, 0, V_NAMEREF);
                continue;
            }
            b32 follow = r.name.data && !r.loop && (!mklocal || (r.last && r.last->depth==sh->funcdepth));
            if (follow && r.hassub && (name=="export" || name=="readonly")) {
                notident(sh, scratch, name, r.last->str);
                status = 1;
                continue;
            }
            if (offref && r.last) setattrs(sh, r.lastname, 0, V_NAMEREF);
            if (follow) {
                refname = vname;
                vname   = r.name;
                if (r.hassub && !hassub) {
                    sub    = r.sub;
                    hassub = 1;
                }
            }
            raw = rawlookup(sh, vname);
            if (raw && (raw->attrs & V_NAMEREF) && hassub && eq<a.len) {
                shellerror(sh, scratch, concat3(&scratch, "warning: ", vname, ": removing nameref attribute"));
                Value *nv = setattrs(sh, vname, 0, V_NAMEREF);
                nv->str = {};
            }
        }
        Value *cur = global ? globalvalue(sh, vname) : rawlookup(sh, vname);
        b32 shadowing = mklocal && cur && cur->depth!=sh->funcdepth && cur->depth;
        if (wordcomp && cur && (cur->attrs & V_READONLY) && !shadowing) {
            // As bash, a literal name=(...) to a readonly variable is
            // first declared alone (with -aAiluc and the scope), while
            // the arguments expand. An error there is reported under the
            // stale command name, and abandons the command unless it
            // declares a local; otherwise the assignment itself fails.
            u32  bad    = V_ARRAY|V_ASSOC|V_INTEGER|V_LOWER|V_UPPER|V_CAPITAL;
            b32  shadow = mklocal && cur->depth!=sh->funcdepth && !cur->depth;
            Str  msg    = concat(&scratch, vname, ": readonly variable");
            if (((on|off) & bad) || shadow) {
                Str stale = {sh->cmdname, sh->cmdnamelen};
                if (stale.len) {
                    builtinerror(sh, scratch, stale, msg);
                } else {
                    shellerror(sh, scratch, msg);
                }
                if (!mklocal) {
                    sh->status = 1;
                    escape(sh, ESC_DISCARD);
                }
            } else {
                shellerror(sh, scratch, msg);
                status = 1;
            }
            a  = vname;  // then declared without its value
            eq = a.len;
        }
        if (cur && (cur->attrs & V_READONLY)) {
            // A new local may shadow a caller's readonly local, but
            // never a readonly global
            b32 shadow = mklocal && cur->depth!=sh->funcdepth;
            u32 bad    = V_ARRAY|V_ASSOC|V_INTEGER|V_LOWER|V_UPPER|V_CAPITAL;
            if (shadow ? !cur->depth : eq<a.len || (off & V_READONLY) || ((on|off) & bad)) {
                // As bash, an assignment names the nameref; a local of
                // it that failed, or attributes alone, its target
                b32 byref = refname.data && !shadow && eq<a.len && !(off & V_READONLY);
                if ((cur->attrs & V_NAMEREF) && !cur->str.data && !mklocal && eq<a.len) {
                    setattrs(sh, vname, V_EMPTYREF, 0);  // see V_EMPTYREF
                }
                Str msg   = concat(&scratch, byref ? refname : vname, ": readonly variable");
                // export -a and readonly -a run declare, which names itself
                if ((name=="export" || name=="readonly") && !(on & (V_ARRAY|V_ASSOC))) {
                    shellerror(sh, scratch, msg);
                } else {
                    builtinerror(sh, scratch, name, msg);
                }
                sh->bifatal = 1;  // a special builtin's assignment error
                status = 1;
                continue;
            }
        }
        if (cur && (cur->attrs & V_TEMP) && cur->depth==sh->funcdepth+1 &&
                !mklocal && !hassub && (on & (V_EXPORT|V_READONLY))) {
            // var=value export var: as bash, the temporary value and the
            // attribute reach the variable beneath, which outlives it
            Value *under = belowtemp(sh, vname, cur);
            Str    v     = eq<a.len && !append ? val : cur->str;
            Str    old   = under->str;
            under->str   = sclone(sh->store, v);
            under->dyn   = 0;
            under->attrs |= (cur->attrs & ~(V_TEMP|V_LOCAL)) | (on & (V_EXPORT|V_READONLY));
            if ((byte *)old.data >= sh->store->mark) sfreestr(sh->store, old);
            sh->exportgen++;
        }
        b32 fresh = !cur || (mklocal && cur->depth!=sh->funcdepth);
        if (mklocal) {
            makelocal(sh, &scratch, vname, inherit);
            cur = rawlookup(sh, vname);
            if (cur && !cur->str.data && !cur->arr) cur = 0;
        }
        b32 isarr   = cur && cur->arr && !cur->arr->bucket;
        b32 isassoc = cur && cur->arr && cur->arr->bucket;
        if (((on & V_ASSOC) && isarr) || ((on & V_ARRAY) && isassoc)) {
            Str msg = concat(&scratch, vname, isarr ? Str(": cannot convert indexed to associative array")
                                                    : Str(": cannot convert associative to indexed array"));
            if (wordcomp && eq<a.len) {
                // found while a literal name=(...) expands, as bash
                // names the stale command, a function's or none; at
                // top level that abandons the command, while in a
                // function it is reported again and the command goes on
                Str stale = {sh->cmdname, sh->cmdnamelen};
                builtinerror(sh, scratch, stale, msg);
                if (!sh->funcdepth) {
                    sh->status = 1;
                    escape(sh, ESC_DISCARD);
                }
                builtinerror(sh, scratch, name, msg);  // and again
            } else {
                builtinerror(sh, scratch, name, msg);
            }
            status = 1;
            continue;
        }
        if ((off & (V_ARRAY|V_ASSOC)) && (isarr || isassoc)) {
            builtinerror(sh, scratch, name, concat(&scratch, vname, ": cannot destroy array variables in this way"));
            status = 1;
            continue;
        }
        Value *self = global ? globalvalue(sh, vname) : rawlookup(sh, vname);
        if (onref && (isarr || isassoc)) {
            builtinerror(sh, scratch, name, concat(&scratch, vname, ": reference variable cannot be an array"));
            status = 1;
            continue;
        }
        if (onref && eq==a.len && self && !(self->attrs & V_NAMEREF) &&
                self->str.data && !validnameref(self->str)) {
            Str msg = concat3(&scratch, "`", self->str, "': invalid variable name for name reference");
            builtinerror(sh, scratch, name, msg);
            status = 1;
            continue;
        }
        b32 isref = self && (self->attrs & V_NAMEREF);
        if (isref && !onref && !(off & V_NAMEREF) && eq<a.len && !validnameref(val)) {
            // Outside a function, bash made the variable anew, and so
            // deletes it
            b32 anew = !self->str.len && !mklocal;
            Str why  = anew ? Str("': not a valid identifier")
                            : Str("': invalid variable name for name reference");
            Str msg  = concat3(&scratch, "`", val, why);
            builtinerror(sh, scratch, name, msg);
            if (anew) unsetvar(sh, vname);
            status = 1;
            continue;
        }
        if (isref && (on & (V_ARRAY|V_ASSOC))) {
            // a nameref made an array stops being one
            Value *nv = setattrs(sh, vname, 0, V_NAMEREF);
            nv->str = {};
            isref = 0;
        }
        if (onref && self && (self->attrs & V_READONLY)) {
            builtinerror(sh, scratch, name, concat(&scratch, vname, ": readonly variable"));
            status = 1;
            continue;
        }
        if (onref) {
            // as ksh93, a nameref loses -ilcu, unless given them again
            off |= (V_INTEGER|V_LOWER|V_UPPER|V_CAPITAL) & ~on;
        }
        if (onref && (on & V_INTEGER) && eq<a.len) {
            // the name evaluates to a number, never a valid name, and
            // bash quietly deletes the variable it made
            if (!self) unsetvar(sh, vname);
            status = 1;
            continue;
        }
        if ((on & V_ASSOC) && fresh) {
            toassoc(sh, setattrs(sh, vname, 0, 0), 1);
        }
        u32 aon = on & ~(V_READONLY|V_NAMEREF);
        if (hassub && !(on & V_ASSOC) && !isassoc) aon |= V_ARRAY;
        i32 sflags = (append ? SV_APPEND : 0) | (global ? SV_GLOBAL : 0);
        if (onref || isref) sflags |= SV_NOREF;
        // export and readonly without -a or -A assign a quoted
        // 'name=(...)' as a string, to element 0 of an existing array
        b32 plain    = (name=="export" || name=="readonly") && !(on & (V_ARRAY|V_ASSOC));
        b32 parens   = val.len>1 && val[0]=='(' && val[val.len-1]==')';
        b32 creating = !!(on & (V_ARRAY|V_ASSOC));
        b32 exists   = isarr || isassoc;
        b32 compound = parens && wordcomp;
        if (parens && !wordcomp && !plain && !(hassub && exists && !creating) &&
                (hassub || creating || exists)) {
            // a quoted name='(...)' is a list only for an array, and
            // name[sub]='(...)' for a new one is an element, as bash
            if (!exists && !creating) {
                shellerror(sh, scratch, concat3(&scratch, "warning: ", a, ": quoted compound array assignment deprecated"));
            }
            compound = exists || creating;
            if (compound) hassub = 0;
        }
        if (eq < a.len) {
            // set the attributes first so that -i and -l apply
            if (aon || off) setattrs(sh, vname, aon, off);
            if (sh->opt[O_XTRACE] && (name=="export" || name=="readonly")) {
                traceassign(sh, scratch, vname, append, val);  // as bash does
            }
            b32 ok = 0;
            if (compound && hassub) {
                Str msg = concat(&scratch, takehead(a, eq), ": cannot assign list to array member");
                builtinerror(sh, scratch, name, msg);
            } else if (compound) {
                i32 nl = mklocal && fresh ? SV_NEWLOCAL : 0;
                ok = assigncompound(sh, scratch, vname, cuttail(cuthead(val, 1), 1),
                                    sflags|nl, wordcomp);
            } else if (hassub) {
                i32 raw = sh->shopt[S_ARRAY_EXPAND_ONCE] ? SV_RAWKEY : 0;
                ok = assignelem(sh, scratch, vname, sub, takehead(a, eq), val, sflags|raw);
            } else {
                ok = setvar(sh, scratch, vname, val, sflags);
            }
            if (!ok) {
                status = 1;
                continue;
            }
            if (on & (V_READONLY|V_NAMEREF)) setattrs(sh, vname, on & (V_READONLY|V_NAMEREF), 0);
        } else if (on || off || local || hassub || !rawlookup(sh, vname)) {
            // Like bash, a new -i leaves the current value as it is:
            // only later assignments are evaluated
            setattrs(sh, vname, on | (aon & V_ARRAY), off);
        }
    }
    return status;
}

static i32 b_declare(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    return declarelike(sh, args, nargs, scratch, args[0], 0, 0);
}

static i32 b_local(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    return declarelike(sh, args, nargs, scratch, "local", 0, 1);
}

static i32 b_export(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    return declarelike(sh, args, nargs, scratch, "export", V_EXPORT, 0);
}

static i32 b_readonly(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    return declarelike(sh, args, nargs, scratch, "readonly", V_READONLY, 0);
}

// unset 'name[sub]': one element, or for an indexed array's @ and *,
// every element. Element 0 of a scalar is the scalar itself.
// BASH_COMPAT as a number like 51, or 53 when unset or not understood.
static i32 compatlevel(Shell *sh)
{
    Arena none = {};
    Str   v    = getvar(sh, &none, "BASH_COMPAT").value;
    i32   n    = 0;
    for (iz i = 0; i < v.len; i++) {
        if (v[i] == '.') continue;
        if (v[i]<'0' || v[i]>'9' || n>1000) return 53;
        n = n*10 + (v[i] - '0');
    }
    return n>=31 && n<=53 ? n : 53;
}

// With raw, an associative key is used as it is, not expanded
// (array_expand_once).
static i32 unsetelem(Shell *sh, Arena scratch, Str name, Str sub, b32 raw = 0, b32 quiet = 0)
{
    i32 rflags = 0;
    name = arraytarget(sh, scratch, name, &rflags);
    if (!name.data) return 1;
    name = clone(&scratch, name);  // the subscript may run code
    Var *v = findvar(sh, name);
    if (!v || !v->top) return 0;
    Value *cur = v->top;
    if (cur->attrs & V_READONLY) {
        builtinerror(sh, scratch, "unset", concat(&scratch, name, ": cannot unset: readonly variable"));
        sh->bifatal = 1;
        return 1;
    }
    X *x = newx(sh, &scratch);
    if (cur->dyn==DYN_BASH_ALIASES || cur->dyn==DYN_BASH_CMDS) {
        return 0;  // as bash, the tables stay as they are
    }
    if (cur->arr && cur->arr->bucket) {
        Str key = raw ? sub : subkey(x, sub);
        // the expansion (${ cmd; }) may have unset or replaced it
        if (!v->top || !v->top->arr || !v->top->arr->bucket) return 0;
        Value *val = ownvalue(sh, &v->top);
        hdel(sh->store, val->arr, key);
        varchanged(sh, name);
        return 0;
    }
    if (cur->arr && allsub(sub) && compatlevel(sh)<=51) {
        return unsetvar(sh, name) ? 0 : 1;  // the whole array, as bash 5.1
    }
    if (cur->arr && allsub(sub)) {
        Value *val = ownvalue(sh, &v->top);
        aclear(sh->store, val->arr);
        varchanged(sh, name);
        return 0;
    }
    i64 idx = 0;
    if (!allsub(sub) && raw) {
        idx = evalinteger(sh, &scratch, sub).value;  // array_expand_once
    } else if (!allsub(sub)) {
        idx = subindex(x, sub);
    }
    cur = v->top;  // the expansion may have unset or replaced it
    if (!cur) return 0;
    if (!cur->arr) {
        if (idx && quiet) {
            return 0;
        } else if (idx) {
            builtinerror(sh, scratch, "unset", concat(&scratch, name, ": not an array variable"));
            return 1;
        }
        return unsetvar(sh, name) ? 0 : 1;
    }
    if (idx < 0) idx = (i64)((u64)idx + (u64)amax(cur->arr) + 1);  // wraps, as bash
    if (idx < 0) {
        builtinerror(sh, scratch, "unset", concat3(&scratch, "[", sub, "]: bad array subscript"));
        return 1;
    }
    Value *val = ownvalue(sh, &v->top);
    adel(sh->store, val->arr, idx);
    varchanged(sh, name);
    return 0;
}

static i32 b_unset(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 funcs = 0, vars = 0, refs = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        if (args[i] == "--") { i++; break; }
        for (iz k = 1; k < args[i].len; k++) {
            switch (args[i][k]) {
            case 'f': funcs = 1; break;
            case 'v': vars = 1; break;
            case 'n': refs = 1; break;
            default:
                invalidoption(sh, scratch, "unset", span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }
    if (funcs && vars) {
        builtinerror(sh, scratch, "unset", "cannot simultaneously unset a function and a variable");
        return 1;
    }
    i32 status = 0;
    for (; i < nargs; i++) {
        Str name = args[i];
        Str base = name;
        Cut br = cut(name, '[');
        if (br.ok) base = br.head;
        Str rn0, rs0;
        b32 valid = validvarname(base) && (!br.ok || splitref(name, &rn0, &rs0));
        if (!valid && vars) {
            if (i<sh->compargs.len && sh->compargs[i]==2) {
                // its subscript was quoted as it expanded: show it bare
                Buf b(&scratch, name.len);
                for (iz k = 0; k < name.len; k++) {
                    k += name[k]==0x5c && k+1<name.len;
                    putbyte(&b, name[k]);
                }
                name = finish(&b);
            }
            notident(sh, scratch, "unset", name);
            status = 1;
            continue;
        }
        // Without a flag, an invalid name may be a function, as is a
        // name that is not a variable
        if (funcs || (!vars && (!valid || !rawlookup(sh, base)))) {
            Func *f = findfunc(sh, name);
            if (f && (f->attrs & V_READONLY)) {
                builtinerror(sh, scratch, "unset", concat(&scratch, name, ": cannot unset: readonly function"));
                sh->bifatal = 1;
                status = 1;
            } else if (f) {
                unsetfunc(sh, name);
            }
            if (funcs || f || !valid) continue;
        }
        Str rn, rs;
        if (br.ok && splitref(name, &rn, &rs) && !refs) {
            b32 raw = sh->shopt[S_ARRAY_EXPAND_ONCE] &&
                      !(i<sh->compargs.len && sh->compargs[i]==2);
            status |= unsetelem(sh, scratch, rn, rs, raw);
            continue;
        }
        Value *top = rawlookup(sh, base);
        if (refs && !funcs) {
            // unset -n: the nameref itself, and nothing else
            if (!top || !(top->attrs & V_NAMEREF)) continue;
        } else if (top && (top->attrs & V_NAMEREF)) {
            // what the nameref names, which may be an array element
            Ref r = resolveref(sh, base);
            if (r.hassub) {
                status |= unsetelem(sh, scratch, r.name, r.sub, 0, 1);
                continue;
            } else if (r.name.data) {
                base = r.name;
            }
        }
        Value *cur = rawlookup(sh, base);
        i32    dyn = cur ? cur->dyn : 0;
        if (dyn==DYN_BASH_LINENO || dyn==DYN_BASH_SOURCE ||
                dyn==DYN_BASH_ARGV || dyn==DYN_BASH_ARGC) {
            // As bash, the call stack's arrays are not unsettable
            builtinerror(sh, scratch, "unset", concat(&scratch, base, ": cannot unset"));
            sh->bifatal = 1;
            status = 1;
        } else if (!unsetvar(sh, base)) {
            builtinerror(sh, scratch, "unset", concat(&scratch, base, ": cannot unset: readonly variable"));
            sh->bifatal = 1;
            status = 1;
        }
    }
    return status;
}

static i32 b_shift(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    i64 n = 1;
    iz  i = skipdashdash(args, nargs, 1);
    if (i < nargs) {
        if (!parsei64(args[i], &n)) {
            builtinerror(sh, scratch, "shift", concat(&scratch, args[i], ": numeric argument required"));
            sh->bifatal = 1;
            return 2;
        }
        if (i+1 < nargs) {
            // As bash, abandon the -c, eval or source text
            builtinerror(sh, scratch, "shift", "too many arguments");
            sh->status = 2;
            escape(sh, ESC_ABORT);
        }
    }
    if (n < 0 || n > sh->args.n) {
        if (sh->shopt[S_SHIFT_VERBOSE] || n < 0) {
            Str msg = "shift count out of range";
            if (i < nargs) msg = concat3(&scratch, args[i], Str(": "), msg);
            builtinerror(sh, scratch, "shift", msg);
        }
        return 1;
    }
    sh->args.v += (iz)n;
    sh->args.n -= (iz)n;
    return 0;
}

// The shopt options that POSIX mode turns on, as bash's posix_initialize
static i32 posixshopts[] = {
    S_INTERACTIVE_COMMENTS, S_SOURCEPATH, S_EXPAND_ALIASES,
    S_INHERIT_ERREXIT, S_SHIFT_VERBOSE,
};

// Entering or leaving POSIX mode (POSIXLY_CORRECT changed): leaving
// restores the settings saved for a temporary binding, if any, else
// bash's defaults.
static void posixinit(Shell *sh, b32 on)
{
    if (on) {
        for (i32 i = 0; i < countof(posixshopts); i++) {
            sh->shopt[posixshopts[i]] = 1;
        }
    } else if (sh->posixsaved) {
        for (i32 i = 0; i < countof(posixshopts); i++) {
            sh->shopt[posixshopts[i]] = sh->posixshopts[i];
        }
        sh->posixsaved = 0;
    } else {
        sh->shopt[S_EXPAND_ALIASES] = sh->interactive;
        sh->shopt[S_SHIFT_VERBOSE]  = 0;
    }
}

// set -o posix and --posix: POSIXLY_CORRECT follows the option
static void setposix(Shell *sh, b32 on)
{
    if (sh->opt[O_POSIX] == on) return;
    byte  mem[256];
    Arena a = {mem, mem+sizeof(mem)};
    if (on) {
        setvar(sh, a, "POSIXLY_CORRECT", "y");
    } else {
        unsetvar(sh, "POSIXLY_CORRECT");
    }
    sh->opt[O_POSIX] = on;  // even if the variable is readonly
}

static i32 setoptionbyname(Shell *sh, Str name, b32 on)
{
    for (i32 k = 0; k < O_COUNT; k++) {
        if (setoptions[k].name==name && hasoption(sh, k)) {
            if (k == O_POSIX) {
                setposix(sh, on);
                return 1;
            }
            sh->opt[k] = on;
            histoption(sh, k, on);
            if (k==O_EMACS && on) sh->opt[O_VI] = 0;
            if (k==O_VI && on) sh->opt[O_EMACS] = 0;
            if (k == O_IGNOREEOF) {
                // as bash, the option and IGNOREEOF follow one another
                byte  mem[256];
                Arena a = {mem, mem+sizeof(mem)};
                if (on) {
                    setvar(sh, a, "IGNOREEOF", "10");
                } else {
                    unsetvar(sh, "IGNOREEOF");
                }
                sh->opt[k] = on;
            }
            return 1;
        }
    }
    return 0;
}

static void listsetoptions(Shell *sh, b32 plusform)
{
    for (i32 k = 0; k < O_COUNT; k++) {
        if (!hasoption(sh, k)) continue;
        if (plusform) {
            bprint(sh, sh->opt[k] ? Str("set -o ") : Str("set +o "));
            bprint(sh, setoptions[k].name);
            bprint(sh, "\n");
        } else {
            bprint(sh, setoptions[k].name);
            for (iz pad = setoptions[k].name.len; pad < 15; pad++) bprint(sh, " ");
            bprint(sh, "\t");
            bprint(sh, sh->opt[k] ? Str("on") : Str("off"));
            bprint(sh, "\n");
        }
    }
}

static i32 b_set(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    if (nargs == 1) {
        Slice<VarEntry> all = sortedvars(sh, &scratch);
        sh->listing = 1;
        for (iz k = 0; k < all.len; k++) {
            printassign(sh, &scratch, all[k].name, all[k].v);
        }
        sh->listing = 0;
        return 0;
    }
    iz i = 1;
    b32 setargs = 0;
    for (; i < nargs; i++) {
        Str a = args[i];
        if (a == "--") {
            i++;
            setargs = 1;
            break;
        }
        if (a == "-") {
            sh->opt[O_XTRACE] = sh->opt[O_VERBOSE] = 0;
            i++;
            setargs = i < nargs;
            break;
        }
        if (a.len<2 || (a[0]!='-' && a[0]!='+')) {
            setargs = 1;
            break;
        }
        b32 on = a[0] == '-';
        for (iz k = 1; k < a.len; k++) {
            u8 c = a[k];
            if (c == 'o') {
                if (i+1 >= nargs) {
                    listsetoptions(sh, !on);
                    continue;
                }
                Str oname = args[++i];
                if (!setoptionbyname(sh, oname, on)) {
                    builtinerror(sh, scratch, "set", concat(&scratch, oname, ": invalid option name"));
                    sh->bifatal = 1;
                    return 2;
                }
                continue;
            }
            b32 found = c=='r' && (on || !sh->restricted);
            if (c=='r' && on) makerestricted(sh);
            for (i32 o = 0; o < O_COUNT; o++) {
                if (setoptions[o].letter == c) {
                    sh->opt[o] = on;
                    histoption(sh, o, on);
                    found = 1;
                }
            }
            if (!found) {
                invalidoption(sh, scratch, "set", span(a.data+k-1, a.data+k+1));
                return c=='r' ? 1 : 2;  // +r: bash's change_flag failing
            }
        }
    }
    if (setargs) {
        iz  n = nargs - i;
        Str *v = (Str *)salloc(sh->store, (n ? n : 1)*(iz)sizeof(Str));
        for (iz k = 0; k < n; k++) v[k] = sclone(sh->store, args[i+k]);
        sh->args.v = v;
        sh->args.n = n;
        sh->argsset |= !sh->funcdepth;
    }
    return 0;
}

// The hidden shopt msyspath, on Windows only: msys mode when set,
// native when unset. It is in neither shopt's listings nor BASHOPTS.
static b32 msysshopt(Shell *sh, Str name)
{
    return sh->win32 && name=="msyspath";
}

// Switch between msys and native mode. The mode decides what "/x"
// means, PATH's separator and the form paths print in, so PATH and
// PWD are rendered again in the new form (msys2.md, "Coexistence").
static void setpathmode(Shell *sh, Arena scratch, PathMode mode)
{
    if (!sh->win32 || mode==sh->pathmode) return;
    PathCtx c    = pathctx(sh, &scratch, {});
    Got     path = getvar(sh, &scratch, "PATH");
    if (path.set) {
        Str p = mode==PM_MSYS ? toposixlist(&c, &scratch, path.value)
                              : towinlist(&c, &scratch, path.value, 0);
        setvar(sh, scratch, "PATH", p.data ? p : Str(""), SV_FORCE);
    }
    Str ncwd = spawncwd(sh, &scratch);
    Str wd   = mode==PM_MSYS ? toposix(&c, &scratch, ncwd) : ncwd;
    sh->pathmode = mode;
    sh->cwd = sclone(sh->store, ncwd);
    sh->wd  = sclone(sh->store, wd);
    setvar(sh, scratch, "PWD", wd, SV_FORCE);
    sh->cmdhash = 0;
}

static i32 b_shopt(Shell *, Str *, iz, Arena);

// shopt -s or -u by name, as the -O and +O options give it. Returns
// false, changing nothing, for an unknown name.
static b32 setshoptbyname(Shell *sh, Arena scratch, Str name, b32 on)
{
    b32 known = msysshopt(sh, name);
    for (i32 k = 0; k < S_COUNT; k++) {
        known |= hasshopt(sh, k) && shoptnames[k]==name;
    }
    if (!known) return 0;
    Str args[] = {"shopt", on ? Str("-s") : Str("-u"), "--", name};
    b_shopt(sh, args, countof(args), scratch);
    return 1;
}

static i32 b_shopt(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 set = 0, unset = 0, print = 0, quiet = 0, oflag = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        if (args[i] == "--") { i++; break; }
        for (iz k = 1; k < args[i].len; k++) {
            switch (args[i][k]) {
            case 's': set = 1; break;
            case 'u': unset = 1; break;
            case 'p': print = 1; break;
            case 'q': quiet = 1; break;
            case 'o': oflag = 1; break;
            default:
                invalidoption(sh, scratch, "shopt", span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }
    if (set && unset) {
        builtinerror(sh, scratch, "shopt", "cannot set and unset shell options simultaneously");
        return 1;
    }
    i32 count = oflag ? (i32)O_COUNT : (i32)S_COUNT;
    if (i == nargs) {
        for (i32 k = 0; k < count; k++) {
            b32 on = oflag ? sh->opt[k] : sh->shopt[k];
            Str nm = oflag ? setoptions[k].name : shoptnames[k];
            if ((set && !on) || (unset && on)) continue;
            if (!(oflag ? hasoption(sh, k) : hasshopt(sh, k))) continue;
            if (print) {
                bprint(sh, oflag ? Str(on ? "set -o " : "set +o ") : Str(on ? "shopt -s " : "shopt -u "));
                bprint(sh, nm);
            } else {
                bprint(sh, nm);
                for (iz pad = nm.len; pad < (oflag ? 15 : 20); pad++) bprint(sh, " ");
                bprint(sh, "\t");
                bprint(sh, on ? Str("on") : Str("off"));
            }
            bprint(sh, "\n");
        }
        return 0;
    }
    i32 status = 0;
    for (; i < nargs; i++) {
        i32 idx    = -1;
        b32 msys   = sh->pathmode == PM_MSYS;
        b32 hidden = !oflag && msysshopt(sh, args[i]);
        for (i32 k = 0; k < count; k++) {
            b32 has = oflag ? hasoption(sh, k) : hasshopt(sh, k);
            if (has && (oflag ? setoptions[k].name : shoptnames[k])==args[i]) idx = k;
        }
        if (idx<0 && !hidden) {
            builtinerror(sh, scratch, "shopt", concat(&scratch, args[i], oflag ? Str(": invalid option name") : Str(": invalid shell option name")));
            status = 1;
            continue;
        }
        b32 *slot = hidden ? &msys : oflag ? &sh->opt[idx] : &sh->shopt[idx];
        if (hidden && (set || unset)) {
            setpathmode(sh, scratch, set ? PM_MSYS : PM_NATIVE);
        } else if (set || unset) {
            if (oflag || (idx!=S_LOGIN_SHELL && idx!=S_RESTRICTED_SHELL)) {
                *slot = set;  // the others read-only, quietly
            }
            if (oflag) histoption(sh, idx, set);
            if (!oflag && (idx==S_ARRAY_EXPAND_ONCE || idx==S_ASSOC_EXPAND_ONCE)) {
                // two names for one option
                sh->shopt[S_ARRAY_EXPAND_ONCE] = sh->shopt[S_ASSOC_EXPAND_ONCE] = set;
            }
            if (!oflag && idx==S_EXTDEBUG) {
                // as bash: extdebug sets errtrace and functrace with it
                sh->opt[O_ERRTRACE] = sh->opt[O_FUNCTRACE] = set;
                if (set) initbashargv(sh);
            }
        } else {
            if (!*slot) status = 1;
            if (!quiet) {
                if (print) {
                    bprint(sh, oflag ? Str(*slot ? "set -o " : "set +o ") : Str(*slot ? "shopt -s " : "shopt -u "));
                    bprint(sh, args[i]);
                } else {
                    bprint(sh, args[i]);
                    // Named, the width is 20 even for set -o options
                    for (iz pad = args[i].len; pad < 20; pad++) bprint(sh, " ");
                    bprint(sh, "\t");
                    bprint(sh, *slot ? Str("on") : Str("off"));
                }
                bprint(sh, "\n");
            }
        }
    }
    return status;
}

static i32 b_let(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    iz i = skipdashdash(args, nargs, 1);
    if (i == nargs) {
        builtinerror(sh, scratch, "let", "expression expected");
        return 1;
    }
    i64 last = 0;
    for (; i < nargs; i++) {
        Arith r = evalarithcmd(sh, &scratch, args[i], "let");
        if (!r.ok) return 1;
        last = r.value;
    }
    return !last;
}


// Directories

// Canonicalize an absolute path textually: drop . and resolve .. but
// keep a drive prefix such as C:
static Str canonpath(Arena *a, Str path)
{
    Str drive = {};
    if (path.len>1 && letter(path[0]) && path[1]==':') {
        drive = takehead(path, 2);
        path  = cuthead(path, 2);
    }
    Str *parts = alloc<Str>(a, path.len/2 + 2);
    iz   n = 0;
    for (Str rest = path; rest.len;) {
        Cut c = cut(rest, '/');
        rest = c.tail;
        if (!c.head.len || c.head==".") continue;
        if (c.head == "..") {
            if (n) n--;
            continue;
        }
        parts[n++] = c.head;
    }
    Buf b(a, drive.len+path.len+1);
    print(&b, drive);
    if (!n) putbyte(&b, '/');
    for (iz i = 0; i < n; i++) {
        putbyte(&b, '/');
        print(&b, parts[i]);
    }
    return finish(&b);
}

// A directory for cd in msys mode, in POSIX form: Windows spellings
// through the mount table ("C:\x" is /c/x), backslashes as slashes.
static Str msysdir(Shell *sh, Arena *a, Str dir)
{
    if (dir.len && dir[0]=='/') {
        return find(dir, "\\")<0 ? dir : slashify(a, dir, '/');
    }
    PathCtx c = pathctx(sh, a, dir);
    Str     r = toposix(&c, a, dir);
    return r.data ? r : dir;
}

// The canonical form of cd's absolute target in the mode: textual in
// POSIX and msys mode (an msys // keeps naming a server), and the
// native path in native mode.
static Str cdcanon(Shell *sh, Arena *a, Str path)
{
    switch (sh->pathmode) {
    case PM_POSIX:
        break;
    case PM_MSYS:
        if (startswith(path, "//") && !startswith(path, "///")) {
            return concat(a, "/", canonpath(a, cuthead(path, 1)));
        }
        break;
    case PM_NATIVE: {
        PathRes r = resolve(sh, a, path);
        Str     n = r.native;
        if (!r.ok || !n.data) return path;
        if (startswith(n, "//?/") && hasdrive(cuthead(n, 4))) n = cuthead(n, 4);
        for (; n.len>1 && n[n.len-1]=='/' && !(n.len==3 && hasdrive(n)); n.len--) {}
        return n;
    }
    }
    return canonpath(a, path);
}

// Make dir, canonical in the mode's form, the working directory: PWD
// and OLDPWD, the native directory the platform sees (none for a
// virtual msys directory such as /proc), and its drive's =X: entry.
static b32 setcwd(Shell *sh, Arena scratch, Str dir, Str old)
{
    Str ncwd = dir;
    if (sh->pathmode == PM_MSYS) {
        PathRes r = resolve(sh, &scratch, dir);
        ncwd = r.ok && r.kind==PK_FILE ? r.native : Str{};
        if (startswith(ncwd, "//?/") && hasdrive(cuthead(ncwd, 4))) ncwd = cuthead(ncwd, 4);
        for (; ncwd.len>1 && ncwd[ncwd.len-1]=='/' && !(ncwd.len==3 && hasdrive(ncwd)); ncwd.len--) {}
    }
    // As bash, a readonly PWD or OLDPWD is reported and fails cd,
    // though the directory changes
    b32 ok = setvar(sh, scratch, "OLDPWD", old);
    ok &= setvar(sh, scratch, "PWD", dir);
    sh->wd  = sclone(sh->store, dir);
    sh->cwd = ncwd.data==dir.data ? sh->wd : sclone(sh->store, ncwd);
    if (sh->pathmode != PM_POSIX) setdrivecwd(sh, scratch, ncwd);
    return ok;
}

static i32 cdas(Shell *sh, Str name, Str *args, iz nargs, Arena scratch);

static i32 b_cd(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    return cdas(sh, "cd", args, nargs, scratch);
}

static i32 cdtry(Shell *, Arena, Str, b32, b32, b32, i32 *);
static Str spellpath(Shell *, Arena *, Str);

// cd, its messages under name (pushd and popd run it as bash does)
static i32 cdas(Shell *sh, Str name, Str *args, iz nargs, Arena scratch)
{
    if (sh->restricted) return restricted(sh, scratch, name);
    b32 physical = sh->opt[O_PHYSICAL];
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        if (args[i] == "--") { i++; break; }
        for (iz k = 1; k < args[i].len; k++) {
            switch (args[i][k]) {
            case 'L': physical = 0; break;
            case 'P': physical = 1; break;
            case 'e': case '@': break;
            default:
                invalidoption(sh, scratch, name, span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }
    Str dir;
    b32 printdir = 0;
    if (i == nargs) {
        Got g = getvar(sh, &scratch, "HOME");
        if (!g.set) {
            builtinerror(sh, scratch, name, "HOME not set");
            return 1;
        }
        dir = g.value;
    } else if (i+1 < nargs) {
        builtinerror(sh, scratch, name, "too many arguments");
        return 2;
    } else if (args[i] == "-") {
        Got g = getvar(sh, &scratch, "OLDPWD");
        if (!g.set) {
            builtinerror(sh, scratch, name, "OLDPWD not set");
            return 1;
        }
        dir = g.value;
        printdir = 1;
    } else {
        dir = args[i];
        if (!dir.len) {
            builtinerror(sh, scratch, name, "null directory");
            return 1;
        }
    }
    if (!dir.len) {
        return 0;
    }
    // As bash, a name that fails may be a variable holding one
    // (cdable_vars), then misspelled (cdspell, interactive only); those
    // print what they took
    i32 err = 0;
    i32 r   = cdtry(sh, scratch, dir, physical, 1, printdir, &err);
    if (r<0 && sh->shopt[S_CDABLE_VARS] && validvarname(dir)) {
        Got v = getvar(sh, &scratch, dir);
        if (v.set && (r = cdtry(sh, scratch, v.value, physical, 0, 0, &err))>=0) {
            bprint(sh, v.value);
            bprint(sh, "\n");
        }
    }
    if (r<0 && sh->shopt[S_CDSPELL] && sh->interactive) {
        Str fix = spellpath(sh, &scratch, dir);
        if (fix.data && (r = cdtry(sh, scratch, fix, physical, 0, 0, &err))>=0) {
            bprint(sh, fix);
            bprint(sh, "\n");
        }
    }
    if (r < 0) {
        builtinerror(sh, scratch, name, concat3(&scratch, dir, Str(": "), errtext(err)));
        return 1;
    }
    return r;
}

// cd's change to dir, with CDPATH when usecdpath: -1 if dir is no
// directory (*err), else 0, or 1 if setting the variables failed.
// printdir prints the new directory, as a CDPATH match does too.
static i32 cdtry(Shell *sh, Arena scratch, Str dir, b32 physical,
                 b32 usecdpath, b32 printdir, i32 *err)
{
    b32 msys  = sh->pathmode == PM_MSYS;
    if (msys) dir = msysdir(sh, &scratch, dir);

    // Relative to bash's the_current_working_directory, not $PWD; a
    // native path resolves through the drives' directories (topath)
    Got pwd    = getvar(sh, &scratch, "PWD");
    Str old    = pwd.set ? pwd.value : sh->wd;
    Str base   = sh->wd;
    if (msys && !startswith(base, "/")) base = msysdir(sh, &scratch, base);
    b32 native = sh->pathmode == PM_NATIVE;
    Str target = isabsolute(sh, dir) || native ? dir : joinpath(&scratch, base, dir);

    // CDPATH for relative names not starting with . or ..
    Got cdpath = usecdpath ? getvar(sh, &scratch, "CDPATH") : Got{};
    if (cdpath.set && !isabsolute(sh, dir) && dir[0]!='.') {
        for (Str rest = cdpath.value;;) {
            Cut c = cut(rest, listsep(sh));
            Str cand = c.head.len ? joinpath(&scratch, c.head, dir) : Str{};
            if (cand.len) {
                if (msys) cand = msysdir(sh, &scratch, cand);
                Str abs = isabsolute(sh, cand) || native ? cand : joinpath(&scratch, base, cand);
                Stat st = pathstat(sh, scratch, cdcanon(sh, &scratch, abs), 1);
                if (!st.err && st.type==FT_DIR) {
                    target = abs;
                    printdir = 1;
                    break;
                }
            }
            if (!c.ok) break;
            rest = c.tail;
        }
    }

    Str logical = cdcanon(sh, &scratch, target);
    Stat st = pathstat(sh, scratch, physical ? target : logical, 1);
    if (st.err || st.type!=FT_DIR) {
        *err = st.err ? st.err : E_NOTDIR;
        return -1;
    }
    b32 ok = setcwd(sh, scratch, physical ? cdcanon(sh, &scratch, target) : logical, old);
    if (printdir) {
        bprint(sh, sh->wd);
        bprint(sh, "\n");
    }
    return !ok;
}

// How far a directory entry's name is from what was typed, as cdspell
// judges it: 0 the same, 1 two neighbours swapped, 2 one character
// changed, missing or extra, else 3.
static i32 spelldist(Str name, Str typed)
{
    iz i = 0;
    for (; i<name.len && i<typed.len && name[i]==typed[i]; i++) {}
    if (i==name.len && i==typed.len) return 0;
    Str n = cuthead(name, i);
    Str t = cuthead(typed, i);
    if (n.len>=2 && t.len>=2 && n[0]==t[1] && n[1]==t[0] &&
            cuthead(n, 2)==cuthead(t, 2)) {
        return 1;
    }
    if (n.len && t.len && cuthead(n, 1)==cuthead(t, 1)) return 2;
    if (n.len && cuthead(n, 1)==t) return 2;  // typed lacks one
    if (t.len && n==cuthead(t, 1)) return 2;  // typed has one more
    return 3;
}

// A path with each component replaced by the closest name in its
// directory (spelldist below 3, a later entry winning a tie, as in
// bash's readdir loop), keeping the slashes; null if one has none.
static Str spellpath(Shell *sh, Arena *a, Str path)
{
    Str out = {};
    out.data = (u8 *)"";
    for (iz i = 0; i < path.len;) {
        iz e = i;
        for (; e<path.len && path[e]=='/'; e++) {}
        out = concat(a, out, span(path.data+i, path.data+e));
        if (e == path.len) break;
        for (i = e; e<path.len && path[e]!='/'; e++) {}
        Str comp = span(path.data+i, path.data+e);
        Str dir  = out.len ? out : Str(".");
        if (dir[0] == '~') dir = tildefilename(sh, a, dir);
        i32     derr = 0;
        Dirent *list = pathlist(sh, a, dir, &derr);
        if (derr) return {};
        b32 dot = 0, dotdot = 0;
        for (Dirent *d = list; d; d = d->next) {
            dot    |= d->name == ".";
            dotdot |= d->name == "..";
        }
        if (!dotdot) list = addentry(a, list, "..", FT_DIR);  // as readdir
        if (!dot)    list = addentry(a, list, ".",  FT_DIR);
        Str best = {};
        i32 dist = 3;
        for (Dirent *d = list; d && dist; d = d->next) {
            i32 x = spelldist(d->name, comp);
            if (x <= dist && x != 3) {
                best = d->name;
                dist = x;
            }
        }
        if (!best.data) return {};
        out = concat(a, out, best);
        i = e;
    }
    return out;
}


// pushd, popd and dirs: the stack is the working directory followed by
// sh->dirs from its top. Their number arguments are words of their own
// ("-n", "-l", "+2"), never grouped.

// A directory as dirs shows it: under $HOME as ~
static Str politedir(Shell *sh, Arena *a, Str dir)
{
    Got home = getvar(sh, a, "HOME");
    if (!home.set || home.value.len<2) return dir;
    iz n = homeprefix(sh, dir, home.value);
    return n<0 ? dir : concat(a, "~", cuthead(dir, n));
}

// dirs' listing: the whole stack, or entry i (DIR_CWD or an index)
enum { DIRS_LONG = 1, DIRS_LINES = 2, DIRS_NUMBERED = 4 };

static void printdirs(Shell *sh, Arena a, i32 flags, b32 one, iz which)
{
    iz  off = sh->ndirs;
    Buf b(&a, 256);
    if (!one || which==DIR_CWD) {
        Str wd = flags&DIRS_LONG ? sh->wd : politedir(sh, &a, sh->wd);
        if (flags & DIRS_NUMBERED) print(&b, " 0  ");
        print(&b, wd);
    }
    for (iz i = one ? which : off-1; i >= 0; i--) {
        Str d = flags&DIRS_LONG ? sh->dirs[i] : politedir(sh, &a, sh->dirs[i]);
        if (flags & DIRS_NUMBERED) {
            if (!one) putbyte(&b, '\n');
            Str n = fmti64(&a, off - i);
            if (n.len < 2) putbyte(&b, ' ');
            print(&b, n);
            print(&b, "  ");
        } else if (!one) {
            putbyte(&b, flags&DIRS_LINES ? '\n' : ' ');
        }
        print(&b, d);
        if (one) break;
    }
    putbyte(&b, '\n');
    bprint(sh, finish(&b));
}

static void direrror(Shell *sh, Arena a, Str name, Str arg)
{
    if (!sh->ndirs) {
        builtinerror(sh, a, name, "directory stack empty");
    } else {
        builtinerror(sh, a, name, concat(&a, arg, ": directory stack index out of range"));
    }
}

static i32 badnumber(Shell *sh, Arena a, Str name, Str word)
{
    builtinerror(sh, a, name, concat(&a, word, ": invalid number"));
    usageerror(sh, a, name);
    return 2;
}

static i32 cdto(Shell *sh, Str name, Str dir, Arena scratch)
{
    Str args[] = {name, "--", dir};
    return cdas(sh, name, args, countof(args), scratch);
}

static i32 b_dirs(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    i32 flags = 0;
    b32 clear = 0;
    b32 one   = 0;
    iz  which = 0;
    Str word  = "";
    for (iz i = 1; i < nargs; i++) {
        Str a = args[i];
        if (a == "-l") {
            flags |= DIRS_LONG;
        } else if (a == "-c") {
            clear = 1;
        } else if (a == "-v") {
            flags |= DIRS_NUMBERED;
        } else if (a == "-p") {
            flags |= DIRS_LINES;
        } else if (a == "--") {
            break;
        } else if (a.len && (a[0]=='+' || a[0]=='-')) {
            i64 n = 0;
            word = cuthead(a, 1);
            if (!parsei64(word, &n)) return badnumber(sh, scratch, "dirs", a);
            one   = 1;
            which = dirindex(sh, n, a[0]=='+' ? 1 : -1);
        } else {
            invalidoption(sh, scratch, "dirs", a);
            return 2;
        }
    }
    if (clear) {
        putdirs(sh, 0, 0);
        return 0;
    }
    if (one && which==DIR_NONE) {
        direrror(sh, scratch, "dirs", word);
        return 1;
    }
    printdirs(sh, scratch, flags, one, which);
    return 0;
}

static i32 b_pushd(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    Store *st  = sh->store;
    iz     off = sh->ndirs;
    iz     i   = 1;
    b32    skipopt = i<nargs && args[i]=="--";
    i += skipopt;
    if (i == nargs) {
        if (!off) {
            builtinerror(sh, scratch, "pushd", "no other directory");
            return 1;
        }
        // exchange the top two: the stack changes even if cd fails
        Str  top = sh->dirs[off-1];
        Str *v   = alloc<Str>(&scratch, off);
        for (iz k = 0; k < off; k++) v[k] = sh->dirs[k];
        v[off-1] = sclone(st, sh->wd);
        putdirs(sh, v, off);
        i32 r = cdto(sh, "pushd", top, scratch);
        if (!r) printdirs(sh, scratch, 0, 0, 0);
        return r;
    }

    b32 nocd   = 0;
    b32 rotate = 0;
    i64 num    = 0;
    for (; !skipopt && i<nargs; i++) {
        Str a = args[i];
        if (a == "-n") {
            nocd = 1;
        } else if (a == "--") {
            i++;
            break;
        } else if (a == "-") {
            break;
        } else if (a.len && (a[0]=='+' || a[0]=='-')) {
            if (!parsei64(cuthead(a, 1), &num)) return badnumber(sh, scratch, "pushd", a);
            if (a[0] == '-') num = off - num;
            if (num<0 || num>off) {
                direrror(sh, scratch, "pushd", a);
                return 1;
            }
            rotate = 1;
        } else {
            break;
        }
    }

    if (rotate) {
        // bring entry num to the top, the working directory going under
        // the bottom; with -n, only the stack turns (and the entry that
        // would have become the working directory is lost, as in bash)
        if (num) {
            Str *v   = alloc<Str>(&scratch, off);
            Str  cur = sclone(st, sh->wd);
            for (iz k = 0; k < off; k++) v[k] = sh->dirs[k];
            for (i64 r = 0; r < num; r++) {
                Str top = v[off-1];
                for (iz k = off-1; k > 0; k--) v[k] = v[k-1];
                v[0] = cur;
                cur  = top;
            }
            putdirs(sh, v, off);
            if (nocd) return 0;
            i32 r = cdto(sh, "pushd", cur, scratch);
            if (!r) printdirs(sh, scratch, 0, 0, 0);
            return r;
        }
        if (nocd) return 0;
        i32 r = cdto(sh, "pushd", sh->wd, scratch);
        if (!r) printdirs(sh, scratch, 0, 0, 0);
        return r;
    }
    if (i == nargs) return 0;

    Str cur = sclone(st, sh->wd);
    if (!nocd) {
        iz from = skipopt ? 0 : i-1;
        Str *cargs = alloc<Str>(&scratch, nargs-from);
        cargs[0] = "pushd";
        for (iz k = from+1; k < nargs; k++) cargs[k-from] = args[k];
        if (cdas(sh, "pushd", cargs, nargs-from, scratch)) return 1;
    }
    Str *v = alloc<Str>(&scratch, off+1);
    for (iz k = 0; k < off; k++) v[k] = sh->dirs[k];
    v[off] = nocd ? sclone(st, args[i]) : cur;
    putdirs(sh, v, off+1);
    printdirs(sh, scratch, 0, 0, 0);
    return 0;
}

static i32 b_popd(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    iz  off   = sh->ndirs;
    b32 nocd  = 0;
    i64 which = 0;
    u8  dir   = '+';
    Str word  = {};
    for (iz i = 1; i < nargs; i++) {
        Str a = args[i];
        if (a == "-n") {
            nocd = 1;
        } else if (a == "--") {
            break;
        } else if (a.len && (a[0]=='+' || a[0]=='-')) {
            dir = a[0];
            if (!parsei64(cuthead(a, 1), &which)) return badnumber(sh, scratch, "popd", a);
            word = a;
        } else if (a.len) {
            builtinerror(sh, scratch, "popd", concat(&scratch, a, ": invalid argument"));
            usageerror(sh, scratch, "popd");
            return 2;
        } else {
            break;
        }
    }
    if (which>off || which<-off || (!off && !which)) {
        direrror(sh, scratch, "popd", word.data ? word : Str(""));
        return 1;
    }
    Str *v = alloc<Str>(&scratch, off ? off : 1);
    for (iz k = 0; k < off; k++) v[k] = sh->dirs[k];
    if ((dir=='+' && !which) || (dir=='-' && which==off)) {
        if (!nocd) {
            i32 r = cdto(sh, "popd", sh->dirs[off-1], scratch);
            if (r) return r;
        }
        putdirs(sh, v, off-1);
    } else {
        iz k = dir=='+' ? off - (iz)which : (iz)which;
        if (k<0 || k>=off) {
            direrror(sh, scratch, "popd", word.data ? word : Str(""));
            return 1;
        }
        for (; k < off-1; k++) v[k] = v[k+1];
        putdirs(sh, v, off-1);
    }
    printdirs(sh, scratch, 0, 0, 0);
    return 0;
}

static i32 b_pwd(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 win = 0;
    for (iz i = 1; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        if (args[i] == "--") break;
        for (iz k = 1; k < args[i].len; k++) {
            switch (args[i][k]) {
            case 'L': case 'P': break;  // the logical path holds no links
            case 'W':
                if (sh->pathmode != PM_POSIX) {
                    win = 1;
                    break;
                }
                [[fallthrough]];
            default:
                invalidoption(sh, scratch, "pwd", span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }
    Str wd = sh->wd;
    if (win && sh->pathmode==PM_MSYS) {
        // MSYS2's pwd -W: the Windows form with forward slashes
        PathCtx c = pathctx(sh, &scratch, {});
        wd = towin(&c, &scratch, wd, 0);
        if (wd.len>1 && wd[wd.len-1]=='/' && !(wd.len==3 && hasdrive(wd))) wd.len--;
    }
    bprint(sh, wd);
    bprint(sh, "\n");
    return 0;
}

// cygpath, msys mode only (also the shell started as cygpath): MSYS2's
// path converter on the shell's own mount table (paths.cpp).
static i32 b_cygpath(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    static Str help =
        "usage: cygpath [-u|-w|-m] [-alps] NAME...\n"
        "       cygpath [-u|-w|-m] [-alps] -f FILE\n"
        "       cygpath [-u|-w|-m] -W|-S\n"
        "Convert paths between the POSIX (-u, the default), Windows (-w)\n"
        "and mixed (-m, forward slashes) forms, through the mount table.\n"
        "  -a  make relative names absolute\n"
        "  -p  convert path lists (':' on the POSIX side, ';' on Windows)\n"
        "  -f  convert each line of FILE (- for standard input)\n"
        "  -W  the Windows directory    -S  the system directory\n"
        "  -l  -s  accepted for long and short names; no effect\n";
    CygOpts o = cygpathopts(&scratch, args, nargs);
    if (!o.ok) {
        flushbout(sh);
        Str msg = concat3(&scratch, "cygpath: ", o.err, "\n");
        fdwrite(sh, 2, concat3(&scratch, msg, cut(help, '\n').head, "\n"));
        return 1;
    }
    if (o.help) {
        bprint(sh, help);
        return 0;
    }
    Str in = {};
    if (o.file.data) {
        i32 err = 0;
        in = readwholefile(sh, &scratch, o.file=="-" ? Str("/dev/stdin") : o.file, &err);
        if (err) {
            flushbout(sh);
            Str msg = concat3(&scratch, o.file, ": ", errtext(err));
            fdwrite(sh, 2, concat3(&scratch, "cygpath: ", msg, "\n"));
            return 1;
        }
    }
    PathCtx c      = pathctx(sh, &scratch, {});
    Str     windir = getvar(sh, &scratch, "SYSTEMROOT").value;
    windir = windir.len ? windir : getvar(sh, &scratch, "WINDIR").value;
    CygRun  r      = cygpathrun(&c, &scratch, &o, args, nargs, in, windir);
    // cygpath buffers its output until it exits, except with -f, where
    // it flushes each line: the error comes first or last
    if (o.file.data) bprint(sh, r.out);
    if (r.err.data) {
        flushbout(sh);
        fdwrite(sh, 2, r.err);
    }
    if (!o.file.data) bprint(sh, r.out);
    return r.status;
}


// Input

// Read one byte from a descriptor, -1 at end of input, or -2 on error.
static i32 readbyte(Shell *sh, i32 fd)
{
    u8 c;
    iz n = fdread(sh, fd, &c, 1);
    return n==1 ? c : n<0 ? -2 : -1;
}

// The next byte of read's input: from the block read ahead, else from
// the descriptor. -1 at the end, -2 on error, -3 on timeout.
static i32 readnext(Shell *sh, i32 fd, u8 *ahead, iz *aoff, iz alen, i64 deadline)
{
    if (*aoff < alen) return ahead[(*aoff)++];
    Task *self = sh->sched->cur;
    self->deadline = deadline;
    i32 c = readbyte(sh, fd);
    self->deadline = 0;
    if (self->timedout) {
        self->timedout = 0;
        return -3;
    }
    return c;
}

// Parse a read -t timeout, a non-negative decimal with an optional
// fraction of which microseconds count, into nanoseconds. False if
// invalid. As bash's timer, the seconds wrap at 2^32, and -1 when that
// leaves nothing of a timeout that was not zero: no timer at all.
static b32 parsetimeout(Str s, i64 *ns)
{
    b32 dot = 0;
    i64 sec = 0, usec = 0, scale = 100000;
    for (iz i = 0; i < s.len; i++) {
        if (s[i]=='.' && !dot) {
            dot = 1;
        } else if (!digit(s[i])) {
            return 0;
        } else if (!dot) {
            if (sec > (0x7fffffffffffffff - 9)/10) return 0;
            sec = sec*10 + (s[i] - '0');
        } else if (scale) {
            usec += (s[i] - '0') * scale;
            scale /= 10;
        }
    }
    i64 wrap = sec & 0xffffffff;
    *ns = !wrap && !usec && sec ? -1 : wrap*1000000000 + usec*1000;
    return 1;
}

// Is input waiting on a file, or its end? For read -t 0.
static b32 inputready(Shell *sh, File *f)
{
    if (!f || !(f->flags & OF_READ)) return 0;
    switch (f->kind) {
    case F_OS:
        return taskreadable(sh->sched, f->h);
    case F_MEM:
        return 1;
    case F_PIPE:
        if (f->pipe->osr != BADHANDLE) return taskreadable(sh->sched, f->pipe->osr);
        return f->pipe->len || !f->pipe->writers;
    }
    return 0;
}

static b32 ifsmember(Str ifs, u8 c)
{
    for (iz i = 0; i < ifs.len; i++) {
        if (ifs[i] == c) return 1;
    }
    return 0;
}

// Counting characters for read -n and -N in UTF-8: does byte c start
// one? A lead byte expects its continuation bytes, which do not count;
// any other byte is a character of its own.
static b32 charstart(u8 c, i32 *pend)
{
    if (*pend && c>=0x80 && c<0xc0) {
        (*pend)--;
        return 0;
    }
    *pend = c>=0xc2 && c<0xe0 ? 1 : c>=0xe0 && c<0xf0 ? 2 : c>=0xf0 && c<0xf5 ? 3 : 0;
    return 1;
}

static i32 b_read(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 raw = 0, silent = 0;
    i32 delim = '\n', fd = 0;
    i64 nchars = -1;
    b32 exact = 0;
    b32 timed = 0;
    i64 timeout = 0;
    Str prompt = {};
    Str arrname = {};
    Str itext = {};
    b32 edit = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") { i++; break; }
        for (iz k = 1; k < a.len; k++) {
            u8 c = a[k];
            Str val = {};
            if (c=='p' || c=='d' || c=='n' || c=='N' || c=='t' || c=='u' ||
                    c=='a' || c=='i') {
                if (k+1 < a.len) {
                    val = cuthead(a, k+1);
                } else if (i+1 < nargs) {
                    val = args[++i];
                } else {
                    builtinerror(sh, scratch, "read", concat3(&scratch, "-", Str{&a.data[k], 1}, ": option requires an argument"));
                    usageerror(sh, scratch, "read");
                    return 2;
                }
                k = a.len;
            }
            switch (c) {
            case 'r': raw = 1; break;
            case 's': silent = 1; break;
            case 'e': case 'E': edit = 1; break;
            case 'p': prompt = val; break;
            case 'd': delim = val.len ? val[0] : 0; break;
            case 'n':
            case 'N':
                if (!parsei64(val, &nchars) || nchars < 0 || nchars > 0x7fffffff) {
                    builtinerror(sh, scratch, "read", concat(&scratch, val, ": invalid number"));
                    return 1;
                }
                exact = c == 'N';
                break;
            case 't':
                if (!parsetimeout(val, &timeout)) {
                    builtinerror(sh, scratch, "read", concat(&scratch, val, ": invalid timeout specification"));
                    return 1;
                }
                timed = 1;
                break;
            case 'u': {
                i64 v;
                if (!parsei64(val, &v) || v<0 || v>0x7fffffff) {
                    builtinerror(sh, scratch, "read", concat(&scratch, val, ": invalid file descriptor specification"));
                    return 1;
                } else if (v>=FDMAX || !sh->fds->fd[v]) {
                    Str msg = concat(&scratch, fmti64(&scratch, v), ": invalid file descriptor: Bad file descriptor");
                    builtinerror(sh, scratch, "read", msg);
                    return 1;
                }
                fd = (i32)v;
            } break;
            case 'a': arrname = val; break;
            case 'i': itext = val; break;
            default:
                invalidoption(sh, scratch, "read", span(a.data+k-1, a.data+k+1));
                return 2;
            }
        }
    }
    // The prompt, -e, -i and -s apply only to a terminal
    File *in  = sh->fds->fd[fd];
    b32   tty = in && in->kind==F_OS && os_isatty(sh->os, in->h);
    if (!tty) {
        prompt = itext = {};
        edit = silent = 0;
    }
    if (timed && !timeout) {
        return !inputready(sh, in);  // -t 0: is there input?
    }
    if (!timed) {
        Got tm = getvar(sh, &scratch, "TMOUT");  // the default timeout
        if (tm.set && !parsetimeout(tm.value, &timeout)) timeout = 0;
    }
    timeout = timeout<0 ? 0 : timeout;
    if (timeout && in) {
        // Not for a regular file, nor text in memory, which never wait
        if (in->kind==F_MEM || (in->kind==F_OS && os_fstat(sh->os, in->h).type==FT_FILE)) {
            timeout = 0;
        }
    }
    Task *self = sh->sched->cur;
    i64 deadline = timeout ? os_now(sh->os) + timeout : 0;
    // The first name is checked now, the rest as they are assigned
    Str rn, rs;
    if (i<nargs && !validvarname(args[i]) && !splitrefonce(sh, args[i], &rn, &rs)) {
        notident(sh, scratch, "read", args[i]);
        return 1;
    }

    // On a terminal without the editor, its own line editing reads,
    // echoing or not; with -n or -d, a byte at a time
    i32    ttymode = 0;
    Handle ttyout  = 0;
    b32    selfecho = 0;  // a Windows console cannot echo single bytes
    if (tty && !edit) {
        if (nchars>0 || delim!='\n') {
            ttymode = silent ? TTY_CHARSNOECHO : TTY_CHARS;
        } else if (silent) {
            ttymode = TTY_NOECHO;
        }
        File *ferr = sh->fds->fd[2];
        ttyout = ferr && ferr->kind==F_OS ? ferr->h : in->h;
    }
    Escape te = {};
    if (ttymode) {
        Tty t = os_tty(sh->os, in->h, ttyout, ttymode);
        if (t.err) {
            ttymode = 0;
        } else {
            // restored on the way out, however it goes
            selfecho = t.noecho;
            te.up    = sh->esc;
            te.bout  = sh->bout;
            te.undo  = sh->undo;
            te.frame = sh->frame;
            sh->esc  = &te;
            if (__builtin_setjmp(te.jmp)) {
                sh->esc = te.up;
                os_tty(sh->os, in->h, ttyout, TTY_RESTORE);
                escape(sh, te.why);
            }
        }
    }
    if (prompt.len && !edit) fdwrite(sh, 2, prompt);
    EdRead eo = {};
    eo.init   = itext;
    eo.fd     = fd;
    eo.delim  = delim=='\n' ? -1 : delim;
    eo.nchars = nchars>0 ? nchars : 0;
    eo.silent = silent;
    eo.deadline = deadline;

    // Read the raw line, keeping track of which bytes were escaped
    Buf line(&scratch, 128);
    Buf esc(&scratch, 128);
    b32 eof = 0;
    b32 timedout = 0;
    i64 count = 0;
    Str s = {}, e = {};
    b32 done = 0;
    // CRs on Windows: igncr drops them all (Git Bash, which otherwise
    // keeps them), native mode a CR before a newline (busybox-w32,
    // whatever the delimiter)
    b32 nocr = igncr(sh);
    b32 crlf = sh->pathmode==PM_NATIVE && !exact;
    if (in && in->kind==F_MEM && (in->flags & OF_READ)) {
        // Text in memory (a here-document or here-string): scan it in
        // place, unless an escape needs the general loop
        Str rest = cuthead(in->mem, in->pos<in->mem.len ? in->pos : in->mem.len);
        iz  lim  = rest.len;
        b32 capped = nchars>=0 && nchars<=lim;  // stops at the count, not EOF
        lim = capped ? (iz)nchars : lim;
        if (nchars>=0 && sh->utf8) {
            // The count is of characters
            i64 c    = 0;
            i32 pend = 0;
            for (lim = 0; lim < rest.len; lim++) {
                if (charstart(rest[lim], &pend) && c++==nchars) break;
            }
            capped = c >= nchars;
        }
        iz  k = 0;
        b32 slash = 0;
        b32 cr = 0;
        for (; k<lim && (exact || rest[k]!=delim); k++) {
            slash |= rest[k]=='\\' || !rest[k];
            cr    |= rest[k] == '\r';
        }
        if ((raw || !slash) && !(cr && (nocr || crlf))) {
            b32 found = k < lim;
            s = clone(&scratch, takehead(rest, k));
            e = {alloc<u8>(&scratch, k), k};
            in->pos += k + found;
            eof  = !found && !capped;
            done = 1;
        }
    }
    // Read a line at a time where that consumes nothing past the
    // delimiter: a regular file in blocks, then seeking back to just
    // past it, as bash does, and a pipe the platform can look into
    // (os_peek), exactly through it. Otherwise a byte at a time.
    b32 peekable = in && in->kind==F_OS && (in->flags & OF_READ) && !edit;
    b32 seekable = 0;
    if (peekable && os_peek(sh->os, in->h, 0, 0)<0) {
        peekable = 0;
        seekable = os_seek(sh->os, in->h, 0, 1) >= 0;
    }
    u8 *ahead = 0;
    iz  alen = 0, aoff = 0;
    iz  look = 128;  // grows while lines are longer
    i32 pend = 0;  // continuation bytes of a UTF-8 character to come
    enum { NOHELD = -4 };
    i32 held = NOHELD;  // a byte read ahead past a CR
    for (; !done;) {
        if (nchars>=0 && count>=nchars && !pend) break;
        if (deadline && aoff==alen && os_now(sh->os)>=deadline) {
            // Input that never runs dry (/dev/zero) never blocks, so
            // the deadline is also checked here, as bash's alarm would
            timedout = 1;
            break;
        }
        if (aoff==alen && seekable) {
            ahead = ahead ? ahead : allocbytes(&scratch, 1<<12);
            iz n = fdread(sh, fd, ahead, look);
            n = n<0 ? 0 : n;
            iz want = n;
            for (iz k = 0; k<n && !exact; k++) {
                if (ahead[k] == delim) {
                    want = k + 1;
                    break;
                }
            }
            if (want==look && look<(1<<12)) look *= 2;
            if (nchars>=0 && want>nchars-count) want = (iz)(nchars - count);
            if (want < n) os_seek(sh->os, in->h, want-n, 1);
            alen = want;
            aoff = 0;
            seekable = n > 0;  // at the end: as before, for its status
        } else if (aoff==alen && peekable) {
            ahead = ahead ? ahead : allocbytes(&scratch, 1<<12);
            iz n = os_peek(sh->os, in->h, ahead, look);
            peekable = n >= 0;
            iz want = n;
            for (iz k = 0; k<n && !exact; k++) {
                if (ahead[k] == delim) {
                    want = k + 1;
                    break;
                }
            }
            if (want==look && look<(1<<12)) look *= 2;
            if (nchars>=0 && want>nchars-count) want = (iz)(nchars - count);
            alen = aoff = 0;
            if (want > 0) {
                self->deadline = deadline;
                alen = fdread(sh, fd, ahead, want);
                self->deadline = 0;
                alen = alen<0 ? 0 : alen;
            }
        }
        i32 c;
        if (held != NOHELD) {
            c    = held;
            held = NOHELD;
            if (c == -3) {
                timedout = 1;
                break;
            }
        } else if (aoff < alen) {
            c = ahead[aoff++];
        } else if (edit) {
            // read -e: the next line from the editor, as typed, and its
            // delimiter
            Buf got(&scratch, 128);
            i32 r = editline(sh, prompt, &got, &eo);
            eo.init = {};
            if (r == ED_INTR) {
                // as the terminal's SIGINT would
                sigsend(sh, sh->sig, 2);
                sigfatal(sh);
                runsigs(sh, scratch);
                return 128 + 2;
            }
            if (r == ED_TIMEOUT) {
                timedout = 1;
                break;
            } else if (r != ED_LINE) {
                c = -1;
            } else {
                if (delim) putbyte(&got, (u8)delim);
                ahead = got.data;
                alen  = got.len;
                aoff  = 0;
                c = alen ? ahead[aoff++] : delim;
            }
        } else {
            sh->intrread = self->introk = 1;
            self->deadline = deadline;
            c = readbyte(sh, fd);
            self->deadline = 0;
            sh->intrread = self->introk = 0;
            if (selfecho && c>=0) {
                u8 b = (u8)c;
                os_write(sh->os, ttyout, b=='\n' ? (u8 *)"\r\n" : &b, b=='\n' ? 2 : 1);
            }
        }
        if (self->timedout) {
            self->timedout = 0;
            timedout = 1;
            break;
        }
        if (c == -2) {
            if (ttymode) {
                sh->esc = te.up;
                os_tty(sh->os, in->h, ttyout, TTY_RESTORE);
            }
            Str msg = concat(&scratch, fmti64(&scratch, fd), ": read error: Bad file descriptor");
            builtinerror(sh, scratch, "read", msg);
            return 1;
        } else if (c < 0) {
            eof = 1;
            break;
        }
        if (crlf && c=='\r' && (nchars<0 || count+1<nchars)) {
            // Native mode, as busybox-w32: CR LF ends the line
            i32 d = readnext(sh, fd, ahead, &aoff, alen, deadline);
            if (d == -3) {
                timedout = 1;
                break;
            }
            if (d == '\n') {
                c = d;
            } else {
                held = d;
            }
        }
        if (c==delim && !exact) break;
        if (c=='\\' && !raw) {
            i32 d = readnext(sh, fd, ahead, &aoff, alen, deadline);
            if (crlf && d=='\r' && (nchars<0 || count+1<nchars)) {
                // and so does backslash CR LF, a line continuation
                i32 n = readnext(sh, fd, ahead, &aoff, alen, deadline);
                if (n == '\n') continue;
                held = n;
            }
            if (d == -3) {
                timedout = 1;
                break;
            }
            if (d < 0) {
                eof = 1;
                break;
            }
            if (d == '\n') {
                // a line continuation, prompted for as bash does
                if (sh->interactive && tty && !edit) {
                    fdwrite(sh, 2, getvar(sh, &scratch, "PS2").value);
                }
                continue;
            }
            putbyte(&line, (u8)d);
            putbyte(&esc, 1);
            count += !sh->utf8 || charstart((u8)d, &pend);
            continue;
        }
        if (!c) continue;  // bash drops null bytes
        if (nocr && c=='\r') continue;  // igncr: CRs vanish, uncounted
        putbyte(&line, (u8)c);
        putbyte(&esc, 0);
        count += !sh->utf8 || charstart((u8)c, &pend);
    }
    if (ttymode) {
        sh->esc = te.up;
        os_tty(sh->os, in->h, ttyout, TTY_RESTORE);
    }
    if (!done) {
        s = finish(&line);
        e = finish(&esc);
    }

    b32 ifsset;
    Str ifs = clone(&scratch, ifsvalue(sh, &ifsset));  // read may assign IFS
    if (exact) ifs = {};  // -N does not split
    if (arrname.data) {
        // Every field into the array's elements
        if (!validvarname(arrname)) {
            notident(sh, scratch, "read", arrname);
            return 1;
        }
        Slice<Str> fields = {};
        iz pos = 0;
        while (pos<s.len && !e[pos] && ifswhite(s[pos]) && ifsmember(ifs, s[pos])) pos++;
        while (pos < s.len) {
            iz start = pos;
            while (pos<s.len && (e[pos] || !ifsmember(ifs, s[pos]))) pos++;
            fields = push(&scratch, fields, span(s.data+start, s.data+pos));
            while (pos<s.len && !e[pos] && ifswhite(s[pos]) && ifsmember(ifs, s[pos])) pos++;
            if (pos<s.len && !e[pos] && !ifswhite(s[pos]) && ifsmember(ifs, s[pos])) {
                pos++;
                while (pos<s.len && !e[pos] && ifswhite(s[pos]) && ifsmember(ifs, s[pos])) pos++;
            }
        }
        i32 aflags = SV_FORCE;
        arrname = arraytarget(sh, scratch, arrname, &aflags, 1);  // namerefs
        if (!arrname.data) return 1;
        Value *val = assignslot(sh, scratch, arrname, aflags & ~SV_FORCE);
        if (!val) return 1;
        if (val->arr && val->arr->bucket) {
            builtinerror(sh, scratch, "read", concat(&scratch, arrname, ": not an indexed array"));
            return 1;
        }
        toarray(sh, val);
        aclear(sh->store, val->arr);
        val->arr->isset = 1;
        for (iz k = 0; k < fields.len; k++) {
            setelem(sh, scratch, arrname, k, {}, fields[k], aflags);
        }
        return timedout ? 128+14 : eof;
    }
    iz  nvars = nargs - i;
    if (!nvars) {
        if (!setvar(sh, scratch, "REPLY", s)) return 2;
        return timedout ? 128+14 : eof;
    }
    // Split into the variables; the last takes the rest of the line
    u8 isifs[256] = {};
    for (iz j = 0; j < ifs.len; j++) isifs[ifs[j]] = 1;
    iz pos = 0;
    for (iz v = 0; v < nvars; v++) {
        // skip leading IFS whitespace
        while (pos<s.len && !e[pos] && ifswhite(s[pos])) {
            b32 inifs = isifs[s[pos]];
            if (!inifs) break;
            pos++;
        }
        if (!validvarname(args[i+v]) && !splitrefonce(sh, args[i+v], &rn, &rs)) {
            notident(sh, scratch, "read", args[i+v]);
            return 1;
        }
        Str val;
        if (v == nvars-1) {
            // One more field ending the line, less its delimiter...
            iz end = pos;
            while (end < s.len) {
                b32 inifs = !e[end] && isifs[s[end]];
                if (inifs) break;
                end++;
            }
            iz next = end;
            while (next<s.len && !e[next] && ifswhite(s[next])) {
                b32 inifs = isifs[s[next]];
                if (!inifs) break;
                next++;
            }
            if (next < s.len && !e[next] && !ifswhite(s[next])) {
                b32 inifs = isifs[s[next]];
                if (inifs) next++;
            }
            while (next<s.len && !e[next] && ifswhite(s[next])) {
                b32 inifs = isifs[s[next]];
                if (!inifs) break;
                next++;
            }
            if (next == s.len) {
                val = span(s.data+pos, s.data+end);
                if (!assignto(sh, scratch, args[i+v], val)) return 1;
                break;
            }
            // ...or else the rest, minus trailing IFS whitespace
            end = s.len;
            while (end > pos) {  // escaped or not
                b32 inifs = isifs[s[end-1]];
                if (!inifs || !ifswhite(s[end-1])) break;
                end--;
            }
            val = span(s.data+pos, s.data+end);
            pos = s.len;
        } else {
            iz start = pos;
            while (pos < s.len) {
                b32 inifs = !e[pos] && isifs[s[pos]];
                if (inifs) break;
                pos++;
            }
            val = span(s.data+start, s.data+pos);
            // consume one delimiter with surrounding IFS whitespace
            while (pos<s.len && !e[pos] && ifswhite(s[pos])) {
                b32 inifs = isifs[s[pos]];
                if (!inifs) break;
                pos++;
            }
            if (pos < s.len && !e[pos]) {
                b32 inifs = isifs[s[pos]];
                if (inifs && !ifswhite(s[pos])) pos++;
            }
        }
        if (!assignto(sh, scratch, args[i+v], val)) return v==nvars-1 ? 1 : 2;
    }
    return timedout ? 128+14 : eof;
}

// mapfile's input. Lines are read in blocks where that consumes nothing
// a later reader would miss: text in memory, a seekable file (seeking
// back to just past the last line taken, as bash), or any input when
// every line is taken. With a count or a callback, a pipe is read
// through each delimiter exactly, by peeking where the platform can,
// else a byte at a time, as bash.
enum : i32 {
    MF_MEM,    // f->mem, in place
    MF_SEEK,   // blocks, then seek back
    MF_BLOCK,  // blocks, up to the end
    MF_PEEK,   // exactly through each delimiter
    MF_BYTE,   // a byte at a time
};

struct MfIn {
    Shell *sh;
    File  *f;
    i32    fd;
    i32    mode;
    u8     delim;
    u8    *buf;
    iz     cap;
    iz     len;
    iz     pos;
};

static b32 mfrefill(MfIn *in)
{
    Shell *sh = in->sh;
    iz     n  = 0;
    switch (in->mode) {
    case MF_MEM:
        return 0;
    case MF_SEEK:
    case MF_BLOCK:
        n = fdread(sh, in->fd, in->buf, in->cap);
        break;
    case MF_PEEK: {
        n = os_peek(sh->os, in->f->h, in->buf, in->cap);
        iz want = n<0 ? 1 : n;
        for (iz k = 0; k < n; k++) {
            if (in->buf[k] == in->delim) {
                want = k + 1;
                break;
            }
        }
        n = fdread(sh, in->fd, in->buf, want ? want : 1);
    } break;
    case MF_BYTE:
        n = fdread(sh, in->fd, in->buf, 1);
        break;
    }
    in->pos = 0;
    in->len = n>0 ? n : 0;
    return n > 0;
}

// Give back what was read past the lines taken, before someone else
// reads the input (a callback) and when done.
static void mfsync(MfIn *in)
{
    if (in->mode == MF_MEM) {
        in->f->pos = in->pos;
    } else if (in->mode==MF_SEEK && in->pos<in->len) {
        os_seek(in->sh->os, in->f->h, in->pos-in->len, 1);
        in->pos = in->len = 0;
    }
}

// After a callback, which may have read the input too.
static void mfresync(MfIn *in)
{
    if (in->mode == MF_MEM) {
        in->pos = in->f->pos;
        in->len = in->f->mem.len;
    }
}

// The next line, with its delimiter if it has one, or null data at the
// end. It may point into the input's buffer, valid until the next call.
static Str mfline(MfIn *in, Arena *a)
{
    Buf part = {};
    for (;;) {
        if (in->pos==in->len && !mfrefill(in)) {
            break;
        }
        iz beg = in->pos;
        iz k   = beg;
        for (; k<in->len && in->buf[k]!=in->delim; k++) {}
        if (k < in->len) {
            in->pos = k + 1;
            Str s = span(in->buf+beg, in->buf+k+1);
            if (!part.data) return s;
            print(&part, s);
            return finish(&part);
        }
        if (!part.data) part = Buf(a, 2*(k-beg) + 64);
        print(&part, span(in->buf+beg, in->buf+k));
        in->pos = k;
    }
    return part.len ? finish(&part) : Str{};
}

static b32 mfcount(Shell *sh, Arena scratch, Str name, Str arg, i64 min, Str what, i64 *v)
{
    if (!parsei64(arg, v) || *v<min || *v>0xffffffffll) {
        builtinerror(sh, scratch, name, concat3(&scratch, arg, ": invalid ", what));
        return 0;
    }
    return 1;
}

static i32 b_mapfile(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    Str name     = args[0];
    u8  delim    = '\n';
    i64 count    = 0;
    i64 origin   = 0;
    b32 isorigin = 0;
    i64 skip     = 0;
    b32 chop     = 0;
    i32 fd       = 0;
    Str callback = {};
    i64 quantum  = 5000;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") {
            i++;
            break;
        }
        for (iz k = 1; k < a.len; k++) {
            u8  c   = a[k];
            Str val = {};
            if (c=='d' || c=='n' || c=='O' || c=='s' || c=='u' || c=='C' || c=='c') {
                if (k+1 < a.len) {
                    val = cuthead(a, k+1);
                } else if (i+1 < nargs) {
                    val = args[++i];
                } else {
                    Str opt = span(a.data+k-1, a.data+k+1);
                    builtinerror(sh, scratch, name, concat(&scratch, opt, ": option requires an argument"));
                    usageerror(sh, scratch, name);
                    return 2;
                }
                k = a.len;
            }
            switch (c) {
            case 'd':
                delim = val.len ? val[0] : 0;
                break;
            case 'n':
                if (!mfcount(sh, scratch, name, val, 0, "line count", &count)) return 1;
                break;
            case 'O':
                if (!parsei64(val, &origin) || origin<0 || origin>0xffffffffll) {
                    builtinerror(sh, scratch, name, concat(&scratch, val, ": invalid array origin"));
                    return 1;
                }
                isorigin = 1;
                break;
            case 's':
                if (!mfcount(sh, scratch, name, val, 0, "line count", &skip)) return 1;
                break;
            case 't':
                chop = 1;
                break;
            case 'u': {
                i64 v;
                if (!parsei64(val, &v) || v<0 || v>0x7fffffff) {
                    builtinerror(sh, scratch, name, concat(&scratch, val, ": invalid file descriptor specification"));
                    return 1;
                } else if (v>=FDMAX || !sh->fds->fd[v]) {
                    Str msg = concat(&scratch, fmti64(&scratch, v), ": invalid file descriptor: Bad file descriptor");
                    builtinerror(sh, scratch, name, msg);
                    return 1;
                }
                fd = (i32)v;
            } break;
            case 'C':
                callback = val;
                break;
            case 'c':
                if (!mfcount(sh, scratch, name, val, 1, "callback quantum", &quantum)) return 1;
                break;
            default:
                invalidoption(sh, scratch, name, span(a.data+k-1, a.data+k+1));
                return 2;
            }
        }
    }

    Str arrname = i<nargs ? args[i] : Str("MAPFILE");
    if (!arrname.len) {
        builtinerror(sh, scratch, name, "empty array variable name");
        return 2;
    } else if (!validvarname(arrname)) {
        notident(sh, scratch, name, arrname);
        return 1;
    }
    i32 aflags = 0;
    arrname = arraytarget(sh, scratch, arrname, &aflags, 1);  // namerefs
    if (!arrname.data) return 1;
    arrname = clone(&scratch, arrname);  // callbacks may change the nameref
    Value *val = assignslot(sh, scratch, arrname, aflags);
    if (!val) return 1;
    if (val->arr && val->arr->bucket) {
        builtinerror(sh, scratch, name, concat(&scratch, arrname, ": not an indexed array"));
        return 1;
    }
    toarray(sh, val);
    if (!isorigin) aclear(sh->store, val->arr);
    val->arr->isset = 1;

    MfIn in  = {};
    in.sh    = sh;
    in.f     = sh->fds->fd[fd];
    in.fd    = fd;
    in.delim = delim;
    in.cap   = (iz)1 << 16;
    b32 exact = count || callback.data;
    File *f = in.f;
    if (f && f->kind==F_MEM && (f->flags & OF_READ)) {
        in.mode = MF_MEM;
        in.buf  = f->mem.data;
        in.pos  = f->pos<f->mem.len ? f->pos : f->mem.len;
        in.len  = f->mem.len;
    } else if (f && f->kind==F_OS && os_peek(sh->os, f->h, 0, 0)<0 &&
               os_seek(sh->os, f->h, 0, 1)>=0) {
        in.mode = MF_SEEK;
    } else if (!exact) {
        in.mode = MF_BLOCK;
    } else if (f && f->kind==F_OS && os_peek(sh->os, f->h, 0, 0)>=0) {
        in.mode = MF_PEEK;
        in.cap  = 1 << 12;
    } else {
        in.mode = MF_BYTE;
    }
    if (in.mode != MF_MEM) {
        in.buf = allocbytes(&scratch, in.cap);
    }

    for (i64 n = 0; n < skip; n++) {
        Arena t = scratch;
        if (!mfline(&in, &t).data) break;
    }
    i64 idx = origin;
    // bash counts indices in an unsigned int, wrapping to 0
    for (i64 n = 1;; idx = (idx+1) & 0xffffffff, n++) {
        Arena t    = scratch;
        Str   line = mfline(&in, &t);
        if (!line.data) break;
        if (chop && line[line.len-1]==delim) {
            line.len--;
        }
        for (iz k = 0; k < line.len; k++) {
            if (!line[k]) {
                line.len = k;  // bash keeps C strings
                break;
            }
        }
        if (callback.data && n%quantum==0) {
            mfsync(&in);
            Buf  cmd(&t, callback.len + line.len + 40);
            print(&cmd, callback);
            putbyte(&cmd, ' ');
            print(&cmd, idx);
            putbyte(&cmd, ' ');
            print(&cmd, singlequote(&t, line));
            Str src = finish(&cmd);
            line = clone(&t, line);  // the callback may refill the buffer
            i32 status = sh->status;
            i32 lineno = sh->line;
            sh->evaldepth++;
            runsource(sh, src, "stdin", t, 0, lineno);
            sh->evaldepth--;
            sh->line   = lineno;
            sh->status = status;
            mfresync(&in);
        }
        setelem(sh, t, arrname, idx, {}, line, aflags|SV_NOREF);
        if (count && n>=count) break;
    }
    mfsync(&in);
    return 0;
}


// Commands

static b32 isreserved(Str s)
{
    for (iz i = 0; i < countof(reserved); i++) {
        if (reserved[i].word == s) return 1;
    }
    return 0;
}

// Describe a command for type and command -v/-V. Returns false if
// nothing was found.
static b32 describe(Shell *sh, Arena *a, Str name, u8 mode, b32 all, b32 nofuncs, b32 std = 0)
{
    b32 found = 0;
    // Without alias expansion, as in scripts, aliases are not commands
    Alias *al = sh->shopt[S_EXPAND_ALIASES] ? findalias(sh->aliases, name) : 0;
    if (al && mode!='p' && mode!='P') {
        found = 1;
        if (mode == 't') {
            bprint(sh, "alias\n");
        } else if (mode == 'v') {
            bprint(sh, "alias ");
            bprint(sh, name);
            bprint(sh, "=");
            bprint(sh, singlequote(a, al->value));
            bprint(sh, "\n");
        } else {
            bprint(sh, name);
            bprint(sh, " is aliased to `");
            bprint(sh, al->value);
            bprint(sh, "'\n");
        }
        if (!all) return 1;
    }
    if (isreserved(name) && mode!='p' && mode!='P') {
        found = 1;
        if (mode == 't') bprint(sh, "keyword\n");
        else if (mode == 'v') { bprint(sh, name); bprint(sh, "\n"); }
        else { bprint(sh, name); bprint(sh, " is a shell keyword\n"); }
        if (!all) return 1;
    }
    // POSIX mode finds special builtins before functions
    Builtin *b = findbuiltin(sh, name);
    Func    *f = nofuncs ? 0 : findfunc(sh, name);
    b32 specialfirst = b && (b->flags & B_SPECIAL) && sh->opt[O_POSIX];
    for (b32 pass = 0; pass < 2 && mode!='p' && mode!='P'; pass++) {
        if (pass == specialfirst) {
            if (!f) continue;
            found = 1;
            if (mode == 't') bprint(sh, "function\n");
            else if (mode == 'v') { bprint(sh, name); bprint(sh, "\n"); }
            else {
                bprint(sh, name);
                bprint(sh, " is a function\n");
                bprint(sh, printfunction(a, name, f->body, sh->opt[O_POSIX]));
                bprint(sh, "\n");
            }
        } else {
            if (!b) continue;
            found = 1;
            if (mode == 't') bprint(sh, "builtin\n");
            else if (mode == 'v') { bprint(sh, name); bprint(sh, "\n"); }
            else {
                bprint(sh, name);
                bprint(sh, specialfirst ? Str(" is a special shell builtin\n")
                                        : Str(" is a shell builtin\n"));
            }
        }
        if (!all) return 1;
    }
    if (mode=='p' && found && !all) return 1;
    b32 slash = haspath(sh, name);
    HashCmd *h = slash || std || (all && mode!='P') ? 0 : hashfind(sh, name);
    if (h) {
        hashhit(sh, h);
        if (mode == 't') bprint(sh, "file\n");
        else if (mode) { bprint(sh, h->path); bprint(sh, "\n"); }
        else {
            bprint(sh, name);
            bprint(sh, " is hashed (");
            bprint(sh, h->path);
            bprint(sh, ")\n");
        }
        return 1;
    }
    for (iz nth = 0;; nth++) {
        Str path = {};
        if (slash) {
            Stat st = pathstat(sh, *a, name, 1);
            if (!nth && canexec(sh, st)) path = name;
        } else {
            path = std ? pathsearch(sh, a, stdpath(sh), name, nth)
                       : pathlookup(sh, a, name, nth);
            if (path.data && !haspath(sh, path)) {
                // found with PATH unset: shown in full
                path = joinpath(a, sh->wd, path);
            }
        }
        if (!path.data) break;
        found = 1;
        if (mode == 't') bprint(sh, "file\n");
        else if (mode) { bprint(sh, path); bprint(sh, "\n"); }
        else {
            bprint(sh, name);
            bprint(sh, " is ");
            bprint(sh, path);
            bprint(sh, "\n");
        }
        if (!all) break;
    }
    return found;
}

static i32 b_type(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    u8  mode = 0;
    b32 all = 0, nofuncs = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        if (args[i] == "--") { i++; break; }
        for (iz k = 1; k < args[i].len; k++) {
            switch (args[i][k]) {
            case 't': mode = 't'; break;
            case 'p': mode = 'p'; break;
            case 'P': mode = 'P'; break;
            case 'a': all = 1; break;
            case 'f': nofuncs = 1; break;
            default:
                invalidoption(sh, scratch, "type", span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }
    i32 status = 0;
    for (; i < nargs; i++) {
        if (!describe(sh, &scratch, args[i], mode, all, nofuncs)) {
            if (mode != 't' && mode != 'p' && mode != 'P') {
                builtinerror(sh, scratch, "type", concat(&scratch, args[i], ": not found"));
            }
            status = 1;
        }
    }
    return status;
}

static i32 b_command(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    u8  mode = 0;
    b32 std  = 0;
    iz  i    = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        if (args[i] == "--") { i++; break; }
        for (iz k = 1; k < args[i].len; k++) {
            switch (args[i][k]) {
            case 'v': mode = 'v'; break;
            case 'V': mode = 'V'; break;
            case 'p': std = 1;    break;
            default:
                invalidoption(sh, scratch, "command", span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }
    if (i == nargs) return 0;
    if (std && sh->restricted) return restricted(sh, scratch, "command", "-p");
    if (mode) {
        i32 status = 0;
        for (; i < nargs; i++) {
            if (!describe(sh, &scratch, args[i], mode=='v' ? 'v' : 0, 0, 0, std)) {
                if (mode == 'V') {
                    builtinerror(sh, scratch, "command", concat(&scratch, args[i], ": not found"));
                }
                status = 1;
            }
        }
        return status;
    }
    Builtin *b = findbuiltin(sh, args[i]);
    if (b) {
        sh->viacommand = 1;
        i32 r = runbuiltin(sh, b, args+i, nargs-i, scratch);
        sh->viacommand = 0;
        return r;
    }
    // Flush pending builtin output before the program writes
    Slice<Str> env = makeenv(sh, &scratch);
    Str found = {};
    if (std && !haspath(sh, args[i])) {
        // -p: the standard utilities, whatever PATH holds
        found = pathsearch(sh, &scratch, stdpath(sh), args[i]);
        if (!found.data) {
            shellerror(sh, scratch, concat(&scratch, args[i], ": command not found"));
            return 127;
        }
    }
    return runexternal(sh, scratch, args+i, nargs-i, env, found);
}

static i32 b_builtin(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    u32 flags;
    iz  i = scanopts(sh, scratch, "builtin", args, nargs, "", &flags);
    if (i < 0) return 2;
    if (i == nargs) return 0;
    Builtin *b = findbuiltin(sh, args[i]);
    if (!b) {
        builtinerror(sh, scratch, "builtin", concat(&scratch, args[i], ": not a shell builtin"));
        return 1;
    }
    return runbuiltin(sh, b, args+i, nargs-i, scratch);
}

static i32 b_exec(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    Str argv0 = {};
    b32 clear = 0, login = 0;
    iz  i     = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") {
            i++;
            break;
        }
        for (iz k = 1; k < a.len; k++) {
            switch (a[k]) {
            case 'c': clear = 1; break;
            case 'l': login = 1; break;
            case 'a':
                if (k+1 < a.len) {
                    argv0 = cuthead(a, k+1);
                } else if (i+1 < nargs) {
                    argv0 = args[++i];
                } else {
                    builtinerror(sh, scratch, "exec", "-a: option requires an argument");
                    usageerror(sh, scratch, "exec");
                    return 2;
                }
                k = a.len;
                break;
            default:
                invalidoption(sh, scratch, "exec", span(a.data+k-1, a.data+k+1));
                return 2;
            }
        }
    }
    if (i == nargs) return 0;  // redirections were made permanent
    if (sh->restricted) return restricted(sh, scratch, "exec");
    Slice<Str> env = clear ? Slice<Str>{} : makeenv(sh, &scratch);
    flushbout(sh);  // pending builtin output first

    Str *argv = alloc<Str>(&scratch, nargs-i);
    for (iz k = i; k < nargs; k++) argv[k-i] = args[k];
    Str name  = argv[0];
    Str found = {};
    if (argv0.data) argv[0] = argv0;
    if (login) argv[0] = concat(&scratch, "-", argv[0]);

    i32 status = 127;
    b32 failed = 1;
    b32 tried  = 0;  // found: a failure ends the shell as a failed execve
    if (!haspath(sh, name) && !(found = pathlookup(sh, &scratch, name)).data) {
        Stat st = pathstat(sh, scratch, name, 1);
        if (!st.err && st.type==FT_DIR) {
            builtinerror(sh, scratch, "exec", concat(&scratch, name, ": cannot execute: Is a directory"));
            status = 126;
        } else {
            builtinerror(sh, scratch, "exec", concat(&scratch, name, ": not found"));
        }
    } else {
        if (!found.data) found = name;
        tried = 1;
        Str  full = absolute(sh, &scratch, found);
        Stat st   = pathstat(sh, scratch, found, 1);
        failed = !canexec(sh, st);
        if (!st.err && st.type==FT_DIR) {
            // bash names the directory in full here
            shellerror(sh, scratch, concat(&scratch, canonpath(&scratch, full), ": Is a directory"));
            status = 126;
        } else {
            // Replacing the process keeps $$ for the program, as bash;
            // runprogram decides whether nothing else stops it
            sh->execing = !sh->shopt[S_EXECFAIL] && !sh->interactive;
            status = runexternal(sh, scratch, argv, nargs-i, env, found);
            sh->execing = 0;
        }
    }
    if (failed && ((sh->shopt[S_EXECFAIL] && !sh->subshell) || sh->interactive)) {
        return status;  // a failed exec leaves the shell (not a subshell) running
    }
    sh->status  = status;
    sh->exiting = 1;
    sh->execed  = !failed || tried;  // replaced, or died trying: no EXIT trap
    if (sh->childsig) {
        // the program died by a signal: so does the replaced shell
        sh->killsig  = sh->childsig;
        sh->childsig = 0;
    }
    escape(sh, ESC_EXIT);
}

static i32 b_true(Shell *, Str *, iz, Arena)
{
    return 0;
}

// One alias as alias prints it: reusable as a command outside POSIX
// mode or with -p, else bare name=value
static void printalias(Shell *sh, Arena scratch, Alias *a, b32 reusable)
{
    if (reusable) {
        bprint(sh, "alias ");
        if (a->name.len && a->name[0]=='-') bprint(sh, "-- ");
    }
    bprint(sh, a->name);
    bprint(sh, "=");
    bprint(sh, singlequote(&scratch, a->value));
    bprint(sh, "\n");
}

static i32 b_alias(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    u32 flags;
    iz  i = scanopts(sh, scratch, "alias", args, nargs, "p", &flags);
    if (i < 0) return 2;
    b32 pflag    = flags & 1;
    b32 reusable = pflag || !sh->opt[O_POSIX];
    if (i==nargs || pflag) {
        // list, sorted
        Slice<Alias *> all = {};
        Alias *stack[256];
        i32 top = 0;
        if (sh->aliases) stack[top++] = sh->aliases;
        while (top) {
            Alias *a = stack[--top];
            if (a->value.data) all = push(&scratch, all, a);
            for (i32 k = 0; k < 4 && top < 252; k++) {
                if (a->child[k]) stack[top++] = a->child[k];
            }
        }
        for (iz k = 1; k < all.len; k++) {
            for (iz j = k; j>0 && compare(all[j]->name, all[j-1]->name)<0; j--) {
                Alias *t = all[j]; all[j] = all[j-1]; all[j-1] = t;
            }
        }
        for (iz k = 0; k < all.len; k++) {
            printalias(sh, scratch, all[k], reusable);
        }
        return 0;  // as bash, -p ignores any names
    }
    i32 status = 0;
    for (; i < nargs; i++) {
        Cut c = cut(args[i], '=');
        if (!c.ok) {
            Alias *a = findalias(sh->aliases, args[i]);
            if (a && a->value.data) {
                printalias(sh, scratch, a, reusable);
            } else {
                builtinerror(sh, scratch, "alias", concat(&scratch, args[i], ": not found"));
                status = 1;
            }
            continue;
        }
        if (!c.head.len || !validaliasname(c.head)) {
            Str name = c.head.len ? c.head : args[i];
            builtinerror(sh, scratch, "alias", concat3(&scratch, "`", name, "': invalid alias name"));
            status = 1;
            continue;
        }
        defalias(sh, c.head, c.tail);
    }
    return status;
}

static i32 b_unalias(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    u32 flags;
    iz  i = scanopts(sh, scratch, "unalias", args, nargs, "a", &flags);
    if (i < 0) return 2;
    if (flags & 1) {
        sh->aliases = 0;
        return 0;
    }
    if (i == nargs) {
        usageerror(sh, scratch, "unalias");
        return 2;
    }
    i32 status = 0;
    for (; i < nargs; i++) {
        Alias *a = findalias(sh->aliases, args[i]);
        if (!a || !a->value.data) {
            builtinerror(sh, scratch, "unalias", concat(&scratch, args[i], ": not found"));
            status = 1;
            continue;
        }
        undefalias(sh, a);
    }
    return status;
}

// Store getopts' result in its variable. As bash, an invalid name or a
// readonly variable (status 2) fails after the option is consumed.
static i32 optvar(Shell *sh, Arena scratch, Str var, Str value)
{
    if (!validvarname(var)) {
        notident(sh, scratch, "getopts", var);
        return 1;
    }
    return setvar(sh, scratch, var, value) ? 0 : 2;
}

static i32 getopts(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    if (nargs>1 && args[1]=="--") {
        args++;
        nargs--;
    } else if (nargs>1 && args[1].len>1 && args[1][0]=='-') {
        invalidoption(sh, scratch, "getopts", takehead(args[1], 2));
        return 2;
    }
    if (nargs < 3) {
        usageerror(sh, scratch, "getopts");
        return 2;
    }
    Str optstring = args[1];
    Str var       = args[2];
    Str *av = sh->args.v;
    iz   ac = sh->args.n;
    if (nargs > 3) {
        av = args + 3;
        ac = nargs - 3;
    }
    b32 silent = optstring.len && optstring[0]==':';
    if (silent) optstring = cuthead(optstring, 1);
    b32 quiet = silent;
    {
        // OPTERR=0 also quiets the messages: bash reads it as atoi
        Got e = getvar(sh, &scratch, "OPTERR");
        if (e.set) {
            Str s = trim(e.value);
            iz  i = s.len && (s[0]=='-' || s[0]=='+');
            b32 nonzero = 0;
            for (; i<s.len && s[i]>='0' && s[i]<='9'; i++) nonzero |= s[i]!='0';
            quiet = !nonzero;
        }
    }

    Got g = getvar(sh, &scratch, "OPTIND");
    i64 optind = 1;
    // bash keeps OPTIND in an int: larger values are truncated
    if (!parsei64(g.value, &optind)) optind = 1;
    optind = (i32)(u32)(u64)optind;
    if (optind < 1) optind = 1;
    if (sh->optchar<1 || (optind<=ac && sh->optchar>=av[optind-1].len)) {
        sh->optchar = 1;  // the words changed under it
    }

    if (optind > ac) {
        optvar(sh, scratch, var, "?");
        unsetvar(sh, "OPTARG", 1);
        setvar(sh, scratch, "OPTIND", fmti64(&scratch, ac+1));
        return 1;
    }
    Str arg = av[optind-1];
    if (sh->optchar == 1) {
        if (arg.len<2 || arg[0]!='-' || arg=="--") {
            if (arg == "--") {
                optind++;
                setvar(sh, scratch, "OPTIND", fmti64(&scratch, optind));
            }
            optvar(sh, scratch, var, "?");
            unsetvar(sh, "OPTARG", 1);
            return 1;
        }
    }
    u8 c = arg[sh->optchar];
    sh->optchar++;
    b32 lastchar = sh->optchar >= arg.len;
    iz  pos = -1;
    for (iz k = 0; k < optstring.len; k++) {
        if (optstring[k]==c && c!=':') { pos = k; break; }
    }
    Str cs = {&arg.data[sh->optchar-1], 1};
    i32 status = 0;
    if (pos < 0) {
        if (silent) {
            setvar(sh, scratch, "OPTARG", cs);
        } else {
            Buf b(&scratch, 64);
            print(&b, sh->name);
            print(&b, ": illegal option -- ");
            print(&b, cs);
            putbyte(&b, '\n');
            if (!quiet) fdwrite(sh, 2, finish(&b));
            unsetvar(sh, "OPTARG", 1);
        }
        status |= optvar(sh, scratch, var, "?");
    } else if (pos+1<optstring.len && optstring[pos+1]==':') {
        if (!lastchar) {
            setvar(sh, scratch, "OPTARG", cuthead(arg, sh->optchar));
            lastchar = 1;
            status |= optvar(sh, scratch, var, cs);
        } else if (optind < ac) {
            optind++;
            setvar(sh, scratch, "OPTARG", av[optind-1]);
            status |= optvar(sh, scratch, var, cs);
        } else {
            if (silent) {
                status |= optvar(sh, scratch, var, ":");
                setvar(sh, scratch, "OPTARG", cs);
            } else {
                Buf b(&scratch, 64);
                print(&b, sh->name);
                print(&b, ": option requires an argument -- ");
                print(&b, cs);
                putbyte(&b, '\n');
                if (!quiet) fdwrite(sh, 2, finish(&b));
                status |= optvar(sh, scratch, var, "?");
                unsetvar(sh, "OPTARG", 1);
            }
        }
    } else {
        status |= optvar(sh, scratch, var, cs);
        if (!unsetvar(sh, "OPTARG")) {
            // only here does bash respect a readonly OPTARG
            shellerror(sh, scratch, "OPTARG: readonly variable");
        }
    }
    if (lastchar) {
        optind++;
        sh->optchar = 1;
    }
    setvar(sh, scratch, "OPTIND", fmti64(&scratch, optind));
    return status;
}

static i32 b_getopts(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    sh->ingetopts = 1;
    i32 r = getopts(sh, args, nargs, scratch);
    sh->ingetopts = 0;
    return r;
}

// ulimit's resources, in its -a order, as bash names and scales them
static struct {
    u8  opt;
    i32 res;
    i64 scale;  // system units per unit shown
    Str desc;
    Str unit;
} const ulimits[] = {
    {'R', RL_RTTIME,     1,    "real-time non-blocking time", "microseconds"},
    {'c', RL_CORE,       512,  "core file size",       "blocks"},
    {'d', RL_DATA,       1024, "data seg size",        "kbytes"},
    {'e', RL_NICE,       1,    "scheduling priority",  ""},
    {'f', RL_FSIZE,      512,  "file size",            "blocks"},
    {'i', RL_SIGPENDING, 1,    "pending signals",      ""},
    {'l', RL_MEMLOCK,    1024, "max locked memory",    "kbytes"},
    {'m', RL_RSS,        1024, "max memory size",      "kbytes"},
    {'n', RL_NOFILE,     1,    "open files",           ""},
    {'p', RL_PIPE,       512,  "pipe size",            "512 bytes"},
    {'q', RL_MSGQUEUE,   1,    "POSIX message queues", "bytes"},
    {'r', RL_RTPRIO,     1,    "real-time priority",   ""},
    {'s', RL_STACK,      1024, "stack size",           "kbytes"},
    {'t', RL_CPU,        1,    "cpu time",             "seconds"},
    {'u', RL_NPROC,      1,    "max user processes",   ""},
    {'v', RL_AS,         1024, "virtual memory",       "kbytes"},
    {'x', RL_LOCKS,      1,    "file locks",           ""},
};

static Rlimit *rltable(Shell *sh)
{
    if (!sh->rlhave) {
        sh->rlhave = 1;
        for (i32 i = 0; i < RL_COUNT; i++) sh->rl[i] = os_getrlimit(sh->os, i);
    }
    return sh->rl;
}

static Rlimit nofilelimit(Shell *sh)
{
    return rltable(sh)[RL_NOFILE];
}

// Set a limit: in the shell's own process at the top level, so that
// the shell and everything it starts have it; in a subshell only in
// its table, which it passes to its programs, checked as a kernel
// checks an unprivileged process.
static i32 setrlimit(Shell *sh, i32 res, Rlimit r)
{
    Rlimit *t = rltable(sh) + res;
    if (!sh->subshell) {
        i32 err = os_setrlimit(sh->os, res, r);
        if (!err) *t = os_getrlimit(sh->os, res);
        return err;
    }
    if (res == RL_PIPE) return E_INVAL;
    if (r.hard!=RL_INFINITY && (r.soft==RL_INFINITY || r.soft>r.hard)) return E_INVAL;
    if (t->hard!=RL_INFINITY && (r.hard==RL_INFINITY || r.hard>t->hard)) return E_PERM;
    t->soft = r.soft;
    t->hard = r.hard;
    sh->rlmod = 1;
    return 0;
}

static void printulimit(Shell *sh, Arena a, i32 u, b32 hard, b32 named)
{
    Rlimit r = rltable(sh)[ulimits[u].res];
    i64    v = hard ? r.hard : r.soft;
    Buf b(&a, 80);
    if (named) {
        print(&b, ulimits[u].desc);
        for (iz n = ulimits[u].desc.len; n < 20; n++) putbyte(&b, ' ');
        putbyte(&b, ' ');
        Buf unit(&a, 32);
        print(&unit, "(");
        if (ulimits[u].unit.len) {
            print(&unit, ulimits[u].unit);
            print(&unit, ", ");
        }
        print(&unit, "-");
        putbyte(&unit, ulimits[u].opt);
        print(&unit, ") ");
        for (iz n = unit.len; n < 20; n++) putbyte(&b, ' ');
        print(&b, finish(&unit));
    }
    print(&b, v==RL_INFINITY ? Str("unlimited") : fmti64(&a, v/ulimits[u].scale));
    print(&b, "\n");
    bprint(sh, finish(&b));
}

// ulimit [-SHa...] [limit]: each resource option may take the next
// word as its value, unless it starts with '-', or the rest of its own
// word; a word after the options goes to the last resource named
// without one. With neither -H nor -S, a new value sets both.
static i32 b_ulimit(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    enum { HARD = 1, SOFT = 2 };
    struct { i32 u; Str arg; } cmds[64];
    i32 ncmds = 0;
    i32 mode  = 0;
    b32 all   = 0;
    iz  i     = 1;
    for (; i < nargs; i++) {
        Str a = args[i];
        if (a == "--") { i++; break; }
        if (a.len<2 || a[0]!='-') break;
        for (iz k = 1; k < a.len; k++) {
            u8 c = a[k];
            if (c == 'H') { mode |= HARD; continue; }
            if (c == 'S') { mode |= SOFT; continue; }
            if (c == 'a') { all = 1; continue; }
            i32 u = 0;
            for (; u<countof(ulimits) && ulimits[u].opt!=c; u++) {}
            if (u==countof(ulimits) || rltable(sh)[ulimits[u].res].err) {
                invalidoption(sh, scratch, "ulimit", span(a.data+k-1, a.data+k+1));
                return 2;
            }
            Str arg = {};
            if (k+1 < a.len) {
                arg = cuthead(a, k+1);
                k = a.len;
            } else if (i+1<nargs && !(args[i+1].len && args[i+1][0]=='-')) {
                arg = args[++i];
            }
            if (ncmds < countof(cmds)) {
                cmds[ncmds].u   = u;
                cmds[ncmds].arg = arg;
                ncmds++;
            }
        }
    }
    b32 hard = mode == HARD;
    if (all) {
        for (i32 u = 0; u < countof(ulimits); u++) {
            if (!rltable(sh)[ulimits[u].res].err) printulimit(sh, scratch, u, hard, 1);
        }
        return 0;
    }
    if (!ncmds) {
        cmds[0].u = 0;
        for (; ulimits[cmds[0].u].opt != 'f'; cmds[0].u++) {}
        cmds[0].arg = {};
        ncmds = 1;
    }
    if (i<nargs && !cmds[ncmds-1].arg.data) cmds[ncmds-1].arg = args[i];

    for (i32 k = 0; k < ncmds; k++) {
        i32 u   = cmds[k].u;
        i32 res = ulimits[u].res;
        Str arg = cmds[k].arg;
        if (!arg.data) {
            printulimit(sh, scratch, u, hard, ncmds>1);
            continue;
        }
        Rlimit cur = rltable(sh)[res];
        i64    v   = 0;
        if (arg == "unlimited") {
            v = RL_INFINITY;
        } else if (arg == "hard") {
            v = cur.hard;
        } else if (arg == "soft") {
            v = cur.soft;
        } else {
            b32 digits = arg.len > 0;
            for (iz j = 0; j < arg.len; j++) digits &= arg[j]>='0' && arg[j]<='9';
            if (!digits) {
                Str why = startswith(arg, "0x") ? Str(": invalid hex number") : ": invalid number";
                builtinerror(sh, scratch, "ulimit", concat(&scratch, arg, why));
                return 1;
            }
            for (iz j = 0; j < arg.len; j++) {
                i64 d = arg[j] - '0';
                v = v > (0x7fffffffffffffff-d)/10 ? 0x7fffffffffffffff : v*10 + d;
            }
            if (v > 0x7fffffffffffffff/ulimits[u].scale) {
                builtinerror(sh, scratch, "ulimit", concat(&scratch, arg, ": limit out of range"));
                return 1;
            }
            v *= ulimits[u].scale;
        }
        Rlimit r = cur;
        if (mode != SOFT) r.hard = v;
        if (mode != HARD) r.soft = v;
        i32 err = setrlimit(sh, res, r);
        if (err) {
            Str msg = concat3(&scratch, ulimits[u].desc, ": cannot modify limit: ", errtext(err));
            builtinerror(sh, scratch, "ulimit", msg);
            return 1;
        }
    }
    return 0;
}

// A chmod-style symbolic mode applied to the permission bits in mode
// (for umask, the complement of the mask), as bash applies it: clauses
// of who (ugoa, default all), then operators (+-=), each followed by
// permissions (rwxXst) or a class to copy from the original bits.
// -1 after an error message.
static i32 symbolicmode(Shell *sh, Arena scratch, Str s, i32 mode)
{
    i32 bits = mode;
    iz  i    = 0;
    for (;;) {
        i32 who = 0;
        for (; i<s.len && find(Str("agou"), Str{s.data+i, 1})>=0; i++) {
            switch (s[i]) {
            case 'u': who |= 0700; break;
            case 'g': who |= 0070; break;
            case 'o': who |= 0007; break;
            case 'a': who |= 0777; break;
            }
        }
        who = who ? who : 0777;
        for (;;) {
            u8 op = i<s.len ? s[i] : 0;
            i++;
            if (op!='+' && op!='-' && op!='=') {
                u8 c[1] = {op};
                Str msg = concat3(&scratch, "`", Str{c, 1}, "': invalid symbolic mode operator");
                builtinerror(sh, scratch, "umask", msg);
                return -1;
            }
            i32 perm = 0;
            for (; i<s.len && find(Str("rwxXstugo"), Str{s.data+i, 1})>=0; i++) {
                switch (s[i]) {
                case 'u': perm = (mode>>6 & 7) * 0111; break;
                case 'g': perm = (mode>>3 & 7) * 0111; break;
                case 'o': perm = (mode    & 7) * 0111; break;
                case 'r': perm |= 0444; break;
                case 'w': perm |= 0222; break;
                case 'X': if (mode & 0111) perm |= 0111; break;
                case 'x': perm |= 0111; break;
                case 's': case 't': break;  // no such bits in a mask
                }
            }
            perm &= who;
            switch (op) {
            case '+': bits |= perm; break;
            case '-': bits &= ~perm; break;
            case '=': bits = (bits & ~who) | perm; break;
            }
            if (i == s.len) return bits;
            if (s[i]=='+' || s[i]=='-' || s[i]=='=') continue;
            if (s[i] == ',') {
                i++;
                break;
            }
            u8 c[1] = {s[i]};
            Str msg = concat3(&scratch, "`", Str{c, 1}, "': invalid symbolic mode character");
            builtinerror(sh, scratch, "umask", msg);
            return -1;
        }
    }
}

static i32 b_umask(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 symbolic = 0, reuse = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        if (args[i] == "--") { i++; break; }
        for (iz k = 1; k < args[i].len; k++) {
            if (args[i][k] == 'S') symbolic = 1;
            else if (args[i][k] == 'p') reuse = 1;
            else {
                invalidoption(sh, scratch, "umask", span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }
    if (i == nargs) {
        i32 m = sh->umask;
        if (symbolic) {
            if (reuse) bprint(sh, "umask -S ");
            Str who[3] = {"u=", "g=", "o="};
            for (i32 w = 0; w < 3; w++) {
                if (w) bprint(sh, ",");
                bprint(sh, who[w]);
                i32 bits = ~m >> (6 - 3*w) & 7;
                if (bits & 4) bprint(sh, "r");
                if (bits & 2) bprint(sh, "w");
                if (bits & 1) bprint(sh, "x");
            }
            bprint(sh, "\n");
        } else {
            if (reuse) bprint(sh, "umask ");
            u8 d[4] = {'0', (u8)('0' + (m>>6&7)), (u8)('0' + (m>>3&7)), (u8)('0' + (m&7))};
            bprint(sh, Str{d, 4});
            bprint(sh, "\n");
        }
        return 0;
    }
    Str s = args[i];
    i32 m = 0;
    if (s.len && s[0]>='0' && s[0]<='9') {
        for (iz k = 0; k < s.len; k++) {
            if (s[k]>='0' && s[k]<='7') m = m*8 + (s[k] - '0');
            if (s[k]<'0' || s[k]>'7' || m>07777) {
                builtinerror(sh, scratch, "umask", concat(&scratch, s, ": octal number out of range"));
                return 1;
            }
        }
    } else {
        m = symbolicmode(sh, scratch, s, ~sh->umask & 0777);
        if (m < 0) return 1;
        m = ~m;
    }
    sh->umask = m & 0777;
    if (symbolic) {
        Str who[3] = {"u=", "g=", "o="};
        for (i32 w = 0; w < 3; w++) {
            if (w) bprint(sh, ",");
            bprint(sh, who[w]);
            i32 bits = ~sh->umask >> (6 - 3*w) & 7;
            if (bits & 4) bprint(sh, "r");
            if (bits & 2) bprint(sh, "w");
            if (bits & 1) bprint(sh, "x");
        }
        bprint(sh, "\n");
    }
    return 0;
}

static i32 b_hash(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 reset = 0, del = 0, show = 0, list = 0;
    Str path  = {};
    iz  i     = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") {
            i++;
            break;
        }
        for (iz k = 1; k < a.len; k++) {
            switch (a[k]) {
            case 'r': reset = 1; break;
            case 'd': del   = 1; break;
            case 't': show  = 1; break;
            case 'l': list  = 1; break;
            case 'p':
                if (k+1 < a.len) {
                    path = cuthead(a, k+1);
                } else if (i+1 < nargs) {
                    path = args[++i];
                } else {
                    builtinerror(sh, scratch, "hash", "-p: option requires an argument");
                    usageerror(sh, scratch, "hash");
                    return 2;
                }
                k = a.len;
                break;
            default:
                invalidoption(sh, scratch, "hash", span(a.data+k-1, a.data+k+1));
                return 2;
            }
        }
    }
    if (!sh->opt[O_HASHALL]) {
        builtinerror(sh, scratch, "hash", "hashing disabled");
        return 1;
    }
    if (reset) sh->cmdhash = 0;
    if (i == nargs) {
        if (del || show) {
            // as bash: the option's argument is a name
            Str msg = del ? Str("-d: option requires an argument") : "-t: option requires an argument";
            builtinerror(sh, scratch, "hash", msg);
            return 1;
        }
        if (path.data) {
            usageerror(sh, scratch, "hash");
            return 2;
        }
        if (reset) return 0;
        b32 any = 0;
        for (i32 b = 0; sh->cmdhash && b < NHASHCMD; b++) {
            for (HashCmd *h = sh->cmdhash[b]; h; h = h->next) {
                if (list) {
                    bprint(sh, "builtin hash -p ");
                    bprint(sh, h->path);
                    bprint(sh, " ");
                    bprint(sh, h->name);
                    bprint(sh, "\n");
                    any = 1;
                    continue;
                }
                if (!any) bprint(sh, "hits\tcommand\n");
                any = 1;
                Str n = fmti64(&scratch, (i64)h->hits);
                for (iz k = n.len; k < 4; k++) bprint(sh, " ");
                bprint(sh, n);
                bprint(sh, "\t");
                bprint(sh, h->path);
                bprint(sh, "\n");
            }
        }
        if (!any) bprint(sh, "hash: hash table empty\n");
        return 0;
    }

    if (path.data && !restrictedhash(sh, scratch, "hash", path)) return 1;
    i32 status = 0;
    b32 multi  = nargs-i > 1;
    for (; i < nargs; i++) {
        Str name = args[i];
        if (!show && find(name, "/")>=0) continue;  // as bash: never hashed
        if (path.data) {
            Stat st = pathstat(sh, scratch, path, 1);
            if (!st.err && st.type==FT_DIR) {
                Str msg = concat3(&scratch, path, ": ", errtext(E_ISDIR));
                builtinerror(sh, scratch, "hash", msg);
                status = 1;
                continue;
            }
            hashenter(sh, name, path, 0);
        } else if (del) {
            if (!hashremove(sh, name)) {
                builtinerror(sh, scratch, "hash", concat(&scratch, name, ": not found"));
                status = 1;
            }
        } else if (show || list) {
            HashCmd *h = hashfind(sh, name);
            if (!h) {
                builtinerror(sh, scratch, "hash", concat(&scratch, name, ": not found"));
                status = 1;
                continue;
            }
            hashhit(sh, h);
            if (list) {
                bprint(sh, "builtin hash -p ");
                bprint(sh, h->path);
                bprint(sh, " ");
                bprint(sh, name);
            } else {
                if (multi) {
                    bprint(sh, name);
                    bprint(sh, "\t");
                }
                bprint(sh, h->path);
            }
            bprint(sh, "\n");
        } else if (!haspath(sh, name) && !findbuiltin(sh, name) && !findfunc(sh, name)) {
            Str found = pathlookup(sh, &scratch, name);
            if (!found.data) {
                builtinerror(sh, scratch, "hash", concat(&scratch, name, ": not found"));
                status = 1;
            } else {
                hashenter(sh, name, found, 0);
            }
        }
    }
    return status;
}

static i32 b_times(Shell *sh, Str *, iz, Arena)
{
    bprint(sh, "0m0.000s 0m0.000s\n0m0.000s 0m0.000s\n");
    return 0;
}


// compgen, for the actions that list names. Completion proper, with
// files, commands, globs and -F/-C, arrives with the line editor (M8).

enum : u32 {
    CG_ALIAS = 1<<0, CG_BUILTIN = 1<<1, CG_EXPORT = 1<<2,
    CG_FUNCTION = 1<<3, CG_KEYWORD = 1<<4, CG_VARIABLE = 1<<5,
    CG_SETOPT = 1<<6, CG_SHOPT = 1<<7,
};

static void compaliases(Alias *t, Arena *a, Slice<Str> *out)
{
    if (!t) return;
    *out = push(a, *out, t->name);
    for (i32 i = 0; i < 4; i++) compaliases(t->child[i], a, out);
}

static void compvars(Var *v, b32 exported, Arena *a, Slice<Str> *out)
{
    if (!v) return;
    Value *t = v->top;
    if (t && (t->str.data || t->arr || t->dyn) && (!exported || (t->attrs & V_EXPORT))) {
        *out = push(a, *out, v->name);
    }
    for (i32 i = 0; i < 4; i++) compvars(v->child[i], exported, a, out);
}

static void compfuncs(Func *f, Arena *a, Slice<Str> *out)
{
    if (!f) return;
    if (f->body) *out = push(a, *out, f->name);
    for (i32 i = 0; i < 4; i++) compfuncs(f->child[i], a, out);
}

static void compbuiltins(Shell *, Arena *, Slice<Str> *);

static i32 b_compgen(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    static struct { Str name; u32 bit; } actions[] = {
        {"alias", CG_ALIAS}, {"builtin", CG_BUILTIN}, {"enabled", CG_BUILTIN},
        {"export", CG_EXPORT}, {"function", CG_FUNCTION},
        {"keyword", CG_KEYWORD}, {"variable", CG_VARIABLE},
        {"setopt", CG_SETOPT}, {"shopt", CG_SHOPT},
    };
    Arena *a = &scratch;
    u32 want = 0;
    Str words = {}, prefix = {}, suffix = {}, filter = {};
    b32 haswords = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str arg = args[i];
        if (arg == "--") {
            i++;
            break;
        }
        for (iz k = 1; k < arg.len; k++) {
            u8   c   = arg[k];
            Str *val = 0;
            switch (c) {
            case 'a': want |= CG_ALIAS;    continue;
            case 'b': want |= CG_BUILTIN;  continue;
            case 'e': want |= CG_EXPORT;   continue;
            case 'k': want |= CG_KEYWORD;  continue;
            case 'v': want |= CG_VARIABLE; continue;
            case 'A': case 'W': case 'P': case 'S': case 'X': break;
            default:
                invalidoption(sh, scratch, "compgen", span(arg.data+k-1, arg.data+k+1));
                return 2;
            }
            Str opt;
            if (k+1 < arg.len) {
                opt = cuthead(arg, k+1);
            } else if (i+1 < nargs) {
                opt = args[++i];
            } else {
                builtinerror(sh, scratch, "compgen", concat3(a, "-", Str{&arg.data[k], 1}, ": option requires an argument"));
                usageerror(sh, scratch, "compgen");
                return 2;
            }
            k = arg.len;
            switch (c) {
            case 'A': {
                u32 bit = 0;
                for (iz j = 0; j < countof(actions); j++) {
                    if (actions[j].name == opt) bit = actions[j].bit;
                }
                if (!bit) {
                    builtinerror(sh, scratch, "compgen", concat(a, opt, ": invalid action name"));
                    return 2;
                }
                want |= bit;
            } continue;
            case 'W': val = &words;  haswords = 1; break;
            case 'P': val = &prefix; break;
            case 'S': val = &suffix; break;
            case 'X': val = &filter; break;
            }
            *val = opt;
        }
    }
    Str word = i<nargs ? args[i] : Str("");

    Slice<Str> names = {};
    // Each action's names in turn, as bash: tables in their own order,
    // the others sorted
    for (u32 bit = 1; bit <= CG_SHOPT; bit <<= 1) {
        iz start = names.len;
        switch (want & bit) {
        case CG_ALIAS:    compaliases(sh->aliases, a, &names); break;
        case CG_BUILTIN:  compbuiltins(sh, a, &names);         break;
        case CG_EXPORT:   compvars(sh->vars, 1, a, &names);    break;
        case CG_FUNCTION: compfuncs(sh->funcs, a, &names);     break;
        case CG_VARIABLE: compvars(sh->vars, 0, a, &names);    break;
        case CG_KEYWORD:
            for (iz j = 0; j < countof(reserved); j++) {
                names = push(a, names, reserved[j].word);
            }
            break;
        case CG_SETOPT:
            for (i32 k = 0; k < O_COUNT; k++) {
                if (hasoption(sh, k)) names = push(a, names, setoptions[k].name);
            }
            break;
        case CG_SHOPT:
            for (i32 k = 0; k < S_COUNT; k++) {
                if (hasshopt(sh, k)) names = push(a, names, shoptnames[k]);
            }
            break;
        }
        if (bit!=CG_BUILTIN && bit!=CG_KEYWORD) {
            sort(names.data+start, names.len-start, strless, *a);
        }
    }
    if (haswords) {
        // Split on IFS, then expand each word
        Got ifs = getvar(sh, a, "IFS");
        Str sep = ifs.set ? ifs.value : Str(" \t\n");
        for (iz j = 0; j < words.len;) {
            iz beg = j;
            while (j<words.len && !ifsmember(sep, words[j])) j++;
            if (j > beg) {
                Str w = span(words.data+beg, words.data+j);
                names = push(a, names, expandstring(sh, a, w));
            }
            j += j < words.len;
        }
    }

    b32 negate = filter.len && filter[0]=='!';
    if (negate) filter = cuthead(filter, 1);
    i32 flags = sh->shopt[S_EXTGLOB] ? PM_EXTGLOB : 0;
    b32 any = 0;
    for (iz j = 0; j < names.len; j++) {
        Str n = names[j];
        if (!startswith(n, word)) continue;
        if (filter.data && patmatch(filter, n, flags, *a) != negate) continue;
        bprint(sh, prefix);
        bprint(sh, n);
        bprint(sh, suffix);
        bprint(sh, "\n");
        any = 1;
    }
    return !any;
}

// The table

static i32 b_trap(Shell *, Str *, iz, Arena);  // trap.cpp
static i32 b_history(Shell *, Str *, iz, Arena);  // history.cpp
static i32 b_fc(Shell *, Str *, iz, Arena);       // history.cpp
static i32 b_kill(Shell *, Str *, iz, Arena);
static i32 b_jobs(Shell *, Str *, iz, Arena);  // jobs.cpp
static i32 b_wait(Shell *, Str *, iz, Arena);
static i32 b_disown(Shell *, Str *, iz, Arena);
static i32 b_fg(Shell *, Str *, iz, Arena);
static i32 b_bg(Shell *, Str *, iz, Arena);
static i32 b_suspend(Shell *, Str *, iz, Arena);

static i32 b_enable(Shell *, Str *, iz, Arena);
static i32 b_help(Shell *, Str *, iz, Arena);         // help.cpp
static i32 builtinhelp(Shell *, Str name, Arena);     // help.cpp

static Builtin builtins[] = {
    {".",        b_source,   B_SPECIAL},
    {":",        b_colon,    B_SPECIAL|B_NOHELP},
    {"[",        b_test,     B_NOHELP},
    {"alias",    b_alias,    0},
    {"bg",       b_bg,       0},
    {"break",    b_break,    B_SPECIAL},
    {"builtin",  b_builtin,  0},
    {"caller",   b_caller,   0},
    {"cd",       b_cd,       0},
    {"command",  b_command,  0},
    {"compgen",  b_compgen,  0},
    {"continue", b_continue, B_SPECIAL},
    {"cygpath",  b_cygpath,  B_MSYS|B_NOHELP},
    {"declare",  b_declare,  B_DECL},
    {"dirs",     b_dirs,     0},
    {"disown",   b_disown,   0},
    {"echo",     b_echo,     B_NOHELP},
    {"enable",   b_enable,   0},
    {"eval",     b_eval,     B_SPECIAL},
    {"exec",     b_exec,     B_SPECIAL},
    {"exit",     b_exit,     B_SPECIAL},
    {"export",   b_export,   B_SPECIAL|B_DECL},
    {"false",    b_false,    B_NOHELP},
    {"fc",       b_fc,       0},
    {"fg",       b_fg,       0},
    {"getopts",  b_getopts,  0},
    {"hash",     b_hash,     0},
    {"help",     b_help,     0},
    {"history",  b_history,  0},
    {"jobs",     b_jobs,     0},
    {"kill",     b_kill,     0},
    {"let",      b_let,      0},
    {"local",    b_local,    B_DECL},
    {"logout",   b_logout,   0},
    {"mapfile",  b_mapfile,  0},
    {"popd",     b_popd,     0},
    {"printf",   b_printf,   0},
    {"pushd",    b_pushd,    0},
    {"pwd",      b_pwd,      0},
    {"read",     b_read,     0},
    {"readarray", b_mapfile, 0},
    {"readonly", b_readonly, B_SPECIAL|B_DECL},
    {"return",   b_return,   B_SPECIAL},
    {"set",      b_set,      B_SPECIAL},
    {"shift",    b_shift,    B_SPECIAL},
    {"shopt",    b_shopt,    0},
    {"source",   b_source,   B_SPECIAL},
    {"suspend",  b_suspend,  0},
    {"test",     b_test,     B_NOHELP},
    {"times",    b_times,    B_SPECIAL},
    {"trap",     b_trap,     B_SPECIAL},
    {"true",     b_true,     B_NOHELP},
    {"type",     b_type,     0},
    {"typeset",  b_declare,  B_DECL},
    {"ulimit",   b_ulimit,   0},
    {"umask",    b_umask,    0},
    {"unalias",  b_unalias,  0},
    {"unset",    b_unset,    B_SPECIAL},
    {"wait",     b_wait,     0},
};

// Builtins that exist in this mode: cygpath only in msys mode.
static b32 hasbuiltin(Shell *sh, Builtin *b)
{
    return !(b->flags & B_MSYS) || sh->pathmode==PM_MSYS;
}

static_assert(countof(builtins) <= 128, "Shell.disabled holds 128");

static b32 isdisabled(Shell *sh, Builtin *b)
{
    iz i = b - builtins;
    return (sh->disabled[i>>6] >> (i&63)) & 1;
}

static void compbuiltins(Shell *sh, Arena *a, Slice<Str> *out)
{
    for (iz i = 0; i < countof(builtins); i++) {
        if (hasbuiltin(sh, builtins+i)) *out = push(a, *out, builtins[i].name);
    }
}

// Run a builtin: NAME --help describes it instead, as bash's do
static i32 runbuiltin(Shell *sh, Builtin *b, Str *args, iz nargs, Arena scratch)
{
    if (nargs>1 && args[1]=="--help" && !(b->flags & B_NOHELP)) {
        return builtinhelp(sh, args[0], scratch);
    }
    return b->fn(sh, args, nargs, scratch);
}

// A builtin by name, enabled or not (enable's own lookup)
static Builtin *anybuiltin(Shell *sh, Str name)
{
    iz lo = 0, hi = countof(builtins);
    while (lo < hi) {
        iz  mid = lo + (hi - lo)/2;
        i32 c   = compare(builtins[mid].name, name);
        if (!c) return hasbuiltin(sh, builtins+mid) ? builtins+mid : 0;
        if (c < 0) lo = mid + 1; else hi = mid;
    }
    return 0;
}

static Builtin *findbuiltin(Shell *sh, Str name)
{
    Builtin *b = anybuiltin(sh, name);
    return b && !isdisabled(sh, b) ? b : 0;
}

// enable [-a] [-dnps] [-f file] [name ...]: there are no loadable
// builtins, so -f is refused as bash built without dlopen refuses it,
// and -d finds nothing it loaded. Disabling is per subshell, like the
// rest of the Shell.
static i32 b_enable(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    enum { A = 1, D = 2, N = 4, P = 8, S = 16 };
    i32 flags = 0;
    iz  i     = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        if (args[i] == "--") { i++; break; }
        for (iz k = 1; k < args[i].len; k++) {
            switch (args[i][k]) {
            case 'a': flags |= A; break;
            case 'd': flags |= D; break;
            case 'n': flags |= N; break;
            case 'p': flags |= P; break;
            case 's': flags |= S; break;
            case 'f':
                if (k+1==args[i].len && i+1==nargs) {
                    builtinerror(sh, scratch, "enable", "-f: option requires an argument");
                    usageerror(sh, scratch, "enable");
                    return 2;
                }
                builtinerror(sh, scratch, "enable", "dynamic loading not available");
                return 2;
            default:
                invalidoption(sh, scratch, "enable", span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }

    if (sh->restricted && (flags & D)) return restricted(sh, scratch, "enable");
    if (i==nargs || (flags & P)) {
        Buf b(&scratch, 1024);
        for (iz k = 0; k < countof(builtins); k++) {
            Builtin *bi = builtins + k;
            if (!hasbuiltin(sh, bi)) continue;
            if ((flags&S) && !(bi->flags&B_SPECIAL)) continue;
            b32 off = isdisabled(sh, bi);
            b32 on  = flags&A || !(flags&N);
            b32 dis = flags&A || flags&N;
            if (!off && on) {
                print(&b, "enable ");
            } else if (off && dis) {
                print(&b, "enable -n ");
            } else {
                continue;
            }
            print(&b, bi->name);
            putbyte(&b, '\n');
        }
        bprint(sh, finish(&b));
        return 0;
    }

    i32 status = 0;
    for (; i < nargs; i++) {
        Builtin *b = anybuiltin(sh, args[i]);
        if (!b) {
            builtinerror(sh, scratch, "enable", concat(&scratch, args[i], ": not a shell builtin"));
            status = 1;
        } else if (flags & D) {
            builtinerror(sh, scratch, "enable", concat(&scratch, args[i], ": not dynamically loaded"));
            status = 1;
        } else {
            iz  k   = b - builtins;
            u64 bit = (u64)1 << (k&63);
            if (flags & N) {
                sh->disabled[k>>6] |= bit;
            } else if (sh->restricted && (sh->disabled[k>>6] & bit)) {
                status = restricted(sh, scratch, "enable");
            } else {
                sh->disabled[k>>6] &= ~bit;
            }
        }
    }
    return status;
}
