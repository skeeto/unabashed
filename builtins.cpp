// Builtin commands
// This is free and unencumbered software released into the public domain.

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
        {"getopts",  "getopts optstring name [arg ...]"},
        {"hash",     "hash [-lr] [-p pathname] [-dt] [name ...]"},
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
    return name;
}

// Bash prints the usage line without the "NAME: line N: " prefix.
static void usageerror(Shell *sh, Arena a, Str name)
{
    flushbout(sh);
    Str msg = concat3(&a, name, ": usage: ", usage(name));
    fdwrite(sh, 2, concat(&a, msg, "\n"));
}

static void invalidoption(Shell *sh, Arena a, Str name, Str opt)
{
    builtinerror(sh, a, name, concat(&a, opt, ": invalid option"));
    usageerror(sh, a, name);
}

static void notident(Shell *sh, Arena a, Str name, Str word)
{
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
    shellerror(c->sh, *c->a, msg);
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

static i32 b_exit(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    // In a trap action, the status from before the action (POSIX)
    i32 status = sh->intrap ? sh->trapstatus : sh->status;
    iz  i = skipdashdash(args, nargs, 1);
    if (i < nargs) {
        i64 v;
        if (!parsei64(args[i], &v)) {
            // bash 5.3 reports it and carries on without exiting
            builtinerror(sh, scratch, "exit", concat(&scratch, args[i], ": numeric argument required"));
            return 2;
        } else if (i+1 < nargs) {
            // As bash, abandon the -c, eval or source text
            builtinerror(sh, scratch, "exit", "too many arguments");
            sh->status = 1;
            escape(sh, ESC_ABORT);
        } else {
            status = (i32)(v & 255);
        }
    }
    sh->status  = status;
    sh->exiting = 1;
    escape(sh, ESC_EXIT);
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
    if (!sh->funcdepth && !sh->sourcedepth) {
        builtinerror(sh, scratch, "return", "can only `return' from a function or sourced script");
        return 2;
    }
    // In a trap action itself, the status from before the action
    b32 intrap = sh->intrap && sh->funcdepth==sh->trapfuncs;
    i32 status = intrap ? sh->trapstatus : sh->status;
    iz  i = skipdashdash(args, nargs, 1);
    if (i < nargs) {
        i64 v;
        if (!parsei64(args[i], &v)) {
            builtinerror(sh, scratch, "return", concat(&scratch, args[i], ": numeric argument required"));
            status = 2;
        } else if (i+1 < nargs) {
            builtinerror(sh, scratch, "return", "too many arguments");
            sh->status = 1;
            escape(sh, ESC_ABORT);
        } else {
            status = (i32)(v & 255);
        }
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
            sh->status = 1;
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

static i32 b_source(Shell *sh, Str *args, iz nargs, Arena scratch)
{
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
    Str file = args[i];
    Str path = file;
    b32 slash = 0;
    for (iz k = 0; k < file.len; k++) slash |= file[k]=='/';
    if (!slash && (only || sh->shopt[S_SOURCEPATH])) {
        if (!only) g = getvar(sh, &scratch, "PATH");
        b32 found = 0;
        for (Str rest = g.value; g.set;) {
            Cut c = cut(rest, ':');
            Str cand = joinpath(&scratch, c.head.len ? c.head : Str("."), file);
            Stat st = os_stat(sh->os, scratch, absolute(sh, &scratch, cand), 1);
            if (!st.err && st.type==FT_FILE) {
                path  = cand;
                found = 1;
                break;
            }
            if (!c.ok) break;
            rest = c.tail;
        }
        if (only && !found) {
            builtinerror(sh, scratch, name, concat(&scratch, file, ": file not found"));
            return 1;
        }
    }
    Stat st = os_stat(sh->os, scratch, absolute(sh, &scratch, path), 1);
    if (!st.err && st.type==FT_DIR) {
        builtinerror(sh, scratch, name, concat(&scratch, file, ": is a directory"));
        return 1;
    }
    i32 err;
    Str src = readwholefile(sh, &scratch, path, &err);
    if (err) {
        builtinerror(sh, scratch, "", concat3(&scratch, file, Str(": "), errtext(err)));
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
    Args saved = sh->args;
    if (i+1 < nargs) {
        sh->args.v = args + i + 1;
        sh->args.n = nargs - i - 1;
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
    if (i+1 < nargs) {
        sf.args.v    = args + i + 1;
        sf.args.n    = nargs - i - 1;
        sf.argpushed = sh->shopt[S_EXTDEBUG];
    } else {
        sf.args.v    = args + i;  // as bash: the name stands in
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
    }
    sh->line = line;
    sh->sourcedepth--;
    if (i+1 < nargs) sh->args = saved;
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
        i32 status = 0;
        for (; i < nargs; i++) {
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
            if (funcnames) {
                bprint(sh, args[i]);
                bprint(sh, "\n");
            } else if (!print && ((on|off) & fattrs)) {
                uz attrs = (f->attrs | (on & fattrs)) & ~(uz)(off & fattrs);
                jwrite(sh->store, &f->attrs, attrs);
            } else {
                printfunc(sh, &scratch, args[i], f);
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
            continue;
        }
        Str  val      = eq<a.len ? cuthead(a, eq+1) : Str{};
        b32  wordcomp = i<sh->compargs.len && sh->compargs[i];
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
                vname = r.name;
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
                Str msg = concat(&scratch, vname, ": readonly variable");
                // export -a and readonly -a run declare, which names itself
                if ((name=="export" || name=="readonly") && !(on & (V_ARRAY|V_ASSOC))) {
                    shellerror(sh, scratch, msg);
                } else {
                    builtinerror(sh, scratch, name, msg);
                }
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
                // names the stale command, a function's or none
                Str stale = {sh->cmdname, sh->cmdnamelen};
                builtinerror(sh, scratch, stale, msg);
                if (global || append) builtinerror(sh, scratch, name, msg);  // and again
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
    Var *v = findvar(sh, name);
    if (!v || !v->top) return 0;
    Value *cur = v->top;
    if (cur->attrs & V_READONLY) {
        builtinerror(sh, scratch, "unset", concat(&scratch, name, ": cannot unset: readonly variable"));
        return 1;
    }
    X *x = newx(sh, &scratch);
    if (cur->dyn==DYN_BASH_ALIASES || cur->dyn==DYN_BASH_CMDS) {
        return 0;  // as bash, the tables stay as they are
    }
    if (cur->arr && cur->arr->bucket) {
        Str key = raw ? sub : subkey(x, sub);
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
    if (!cur->arr) {
        if (idx && quiet) {
            return 0;
        } else if (idx) {
            builtinerror(sh, scratch, "unset", concat(&scratch, name, ": not an array variable"));
            return 1;
        }
        return unsetvar(sh, name) ? 0 : 1;
    }
    if (idx < 0) idx += amax(cur->arr) + 1;
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
        if (!unsetvar(sh, base)) {
            builtinerror(sh, scratch, "unset", concat(&scratch, base, ": cannot unset: readonly variable"));
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
            return 2;
        }
        if (i+1 < nargs) {
            // As bash, abandon the -c, eval or source text
            builtinerror(sh, scratch, "shift", "too many arguments");
            sh->status = 1;
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

static i32 setoptionbyname(Shell *sh, Str name, b32 on)
{
    for (i32 k = 0; k < O_COUNT; k++) {
        if (setoptions[k].name == name) {
            sh->opt[k] = on;
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
                    return 2;
                }
                continue;
            }
            b32 found = 0;
            for (i32 o = 0; o < O_COUNT; o++) {
                if (setoptions[o].letter == c) {
                    sh->opt[o] = on;
                    found = 1;
                }
            }
            if (!found) {
                invalidoption(sh, scratch, "set", span(a.data+k-1, a.data+k+1));
                return 2;
            }
        }
    }
    if (setargs) {
        iz  n = nargs - i;
        Str *v = (Str *)salloc(sh->store, (n ? n : 1)*(iz)sizeof(Str));
        for (iz k = 0; k < n; k++) v[k] = sclone(sh->store, args[i+k]);
        sh->args.v = v;
        sh->args.n = n;
    }
    return 0;
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
        i32 idx = -1;
        for (i32 k = 0; k < count; k++) {
            if ((oflag ? setoptions[k].name : shoptnames[k]) == args[i]) idx = k;
        }
        if (idx < 0) {
            builtinerror(sh, scratch, "shopt", concat(&scratch, args[i], oflag ? Str(": invalid option name") : Str(": invalid shell option name")));
            status = 1;
            continue;
        }
        b32 *slot = oflag ? &sh->opt[idx] : &sh->shopt[idx];
        if (set || unset) {
            *slot = set;
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

static i32 b_cd(Shell *sh, Str *args, iz nargs, Arena scratch)
{
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
                invalidoption(sh, scratch, "cd", span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }
    Str dir;
    b32 printdir = 0;
    if (i == nargs) {
        Got g = getvar(sh, &scratch, "HOME");
        if (!g.set) {
            builtinerror(sh, scratch, "cd", "HOME not set");
            return 1;
        }
        dir = g.value;
    } else if (i+1 < nargs) {
        builtinerror(sh, scratch, "cd", "too many arguments");
        return 2;
    } else if (args[i] == "-") {
        Got g = getvar(sh, &scratch, "OLDPWD");
        if (!g.set) {
            builtinerror(sh, scratch, "cd", "OLDPWD not set");
            return 1;
        }
        dir = g.value;
        printdir = 1;
    } else {
        dir = args[i];
    }
    if (!dir.len) {
        return 0;
    }
    Got pwd = getvar(sh, &scratch, "PWD");
    Str base = pwd.set && isabsolute(sh, pwd.value) ? pwd.value : sh->cwd;
    Str target = isabsolute(sh, dir) ? dir : joinpath(&scratch, base, dir);

    // CDPATH for relative names not starting with . or ..
    Got cdpath = getvar(sh, &scratch, "CDPATH");
    if (cdpath.set && !isabsolute(sh, dir) && dir[0]!='.') {
        for (Str rest = cdpath.value;;) {
            Cut c = cut(rest, ':');
            Str cand = c.head.len ? joinpath(&scratch, c.head, dir) : Str{};
            if (cand.len) {
                Str abs = isabsolute(sh, cand) ? cand : joinpath(&scratch, base, cand);
                Stat st = os_stat(sh->os, scratch, canonpath(&scratch, abs), 1);
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

    Str logical = canonpath(&scratch, target);
    Stat st = os_stat(sh->os, scratch, physical ? target : logical, 1);
    if (st.err || st.type!=FT_DIR) {
        i32 err = st.err ? st.err : E_NOTDIR;
        builtinerror(sh, scratch, "cd", concat3(&scratch, dir, Str(": "), errtext(err)));
        return 1;
    }
    Str newcwd = physical ? canonpath(&scratch, target) : logical;
    setvar(sh, scratch, "OLDPWD", base, SV_FORCE);
    setvar(sh, scratch, "PWD", newcwd, SV_FORCE);
    sh->cwd = sclone(sh->store, newcwd);
    if (printdir) {
        bprint(sh, newcwd);
        bprint(sh, "\n");
    }
    return 0;
}

static i32 b_pwd(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 physical = sh->opt[O_PHYSICAL];
    for (iz i = 1; i < nargs; i++) {
        if (args[i] == "-P") physical = 1;
        else if (args[i] == "-L") physical = 0;
        else if (args[i] == "--") break;
        else if (args[i].len>1 && args[i][0]=='-') {
            invalidoption(sh, scratch, "pwd", takehead(args[i], 2));
            return 2;
        }
    }
    Got g = getvar(sh, &scratch, "PWD");
    bprint(sh, !physical && g.set && g.value.len ? g.value : sh->cwd);
    bprint(sh, "\n");
    return 0;
}


// Input

// Read one byte from a descriptor, -1 at end of input, or -2 on error.
static i32 readbyte(Shell *sh, i32 fd)
{
    u8 c;
    iz n = fdread(sh, fd, &c, 1);
    return n==1 ? c : n<0 ? -2 : -1;
}

// Parse a read -t timeout, a non-negative decimal with an optional
// fraction of which microseconds count, into nanoseconds. False if
// invalid.
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
            if (sec > (i64)1<<31) return 0;
            sec = sec*10 + (s[i] - '0');
        } else if (scale) {
            usec += (s[i] - '0') * scale;
            scale /= 10;
        }
    }
    *ns = sec*1000000000 + usec*1000;
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
            case 'e': case 'E': break;
            case 'p': prompt = val; break;
            case 'd': delim = val.len ? val[0] : 0; break;
            case 'n':
            case 'N':
                if (!parsei64(val, &nchars) || nchars < 0) {
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
                if (!parsei64(val, &v) || v<0) {
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
            case 'i': break;
            default:
                invalidoption(sh, scratch, "read", span(a.data+k-1, a.data+k+1));
                return 2;
            }
        }
    }
    // The prompt and -s apply only to a terminal, which input never is
    (void)silent;
    (void)prompt;
    File *in = sh->fds->fd[fd];
    if (timed && !timeout) {
        return !inputready(sh, in);  // -t 0: is there input?
    }
    if (!timed) {
        Got tm = getvar(sh, &scratch, "TMOUT");  // the default timeout
        if (tm.set && !parsetimeout(tm.value, &timeout)) timeout = 0;
    }
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

    // Read the raw line, keeping track of which bytes were escaped
    Buf line(&scratch, 128);
    Buf esc(&scratch, 128);
    b32 eof = 0;
    b32 timedout = 0;
    i64 count = 0;
    Str s = {}, e = {};
    b32 done = 0;
    if (in && in->kind==F_MEM && (in->flags & OF_READ)) {
        // Text in memory (a here-document or here-string): scan it in
        // place, unless an escape needs the general loop
        Str rest = cuthead(in->mem, in->pos<in->mem.len ? in->pos : in->mem.len);
        iz  lim  = rest.len;
        b32 capped = nchars>=0 && nchars<=lim;  // stops at the count, not EOF
        lim = capped ? (iz)nchars : lim;
        iz  k = 0;
        b32 slash = 0;
        for (; k<lim && (exact || rest[k]!=delim); k++) slash |= rest[k]=='\\';
        if (raw || !slash) {
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
    b32 peekable = in && in->kind==F_OS && (in->flags & OF_READ);
    b32 seekable = 0;
    if (peekable && os_peek(sh->os, in->h, 0, 0)<0) {
        peekable = 0;
        seekable = os_seek(sh->os, in->h, 0, 1) >= 0;
    }
    u8 *ahead = 0;
    iz  alen = 0, aoff = 0;
    iz  look = 128;  // grows while lines are longer
    for (; !done;) {
        if (nchars>=0 && count>=nchars) break;
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
        if (aoff < alen) {
            c = ahead[aoff++];
        } else {
            sh->intrread = self->introk = 1;
            self->deadline = deadline;
            c = readbyte(sh, fd);
            self->deadline = 0;
            sh->intrread = self->introk = 0;
        }
        if (self->timedout) {
            self->timedout = 0;
            timedout = 1;
            break;
        }
        if (c == -2) {
            Str msg = concat(&scratch, fmti64(&scratch, fd), ": read error: Bad file descriptor");
            builtinerror(sh, scratch, "read", msg);
            return 1;
        } else if (c < 0) {
            eof = 1;
            break;
        }
        if (c==delim && !exact) break;
        if (c=='\\' && !raw) {
            i32 d;
            if (aoff < alen) {
                d = ahead[aoff++];
            } else {
                self->deadline = deadline;
                d = readbyte(sh, fd);
                self->deadline = 0;
            }
            if (self->timedout) {
                self->timedout = 0;
                timedout = 1;
                break;
            }
            if (d < 0) {
                eof = 1;
                break;
            }
            if (d == '\n') continue;  // line continuation
            putbyte(&line, (u8)d);
            putbyte(&esc, 1);
            count++;
            continue;
        }
        putbyte(&line, (u8)c);
        putbyte(&esc, 0);
        count++;
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
    Builtin *b = findbuiltin(name);
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
            Stat st = os_stat(sh->os, *a, absolute(sh, a, name), 1);
            if (!nth && canexec(sh, st)) path = name;
        } else {
            path = std ? pathsearch(sh, a, stdpath(sh), name, nth)
                       : pathlookup(sh, a, name, nth);
            if (path.data && !haspath(sh, path)) {
                // found with PATH unset: shown in full
                path = joinpath(a, sh->cwd, path);
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
    Builtin *b = findbuiltin(args[i]);
    if (b) {
        return b->fn(sh, args+i, nargs-i, scratch);
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
    if (nargs < 2) return 0;
    Builtin *b = findbuiltin(args[1]);
    if (!b) {
        builtinerror(sh, scratch, "builtin", concat(&scratch, args[1], ": not a shell builtin"));
        return 1;
    }
    return b->fn(sh, args+1, nargs-1, scratch);
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
        Stat st = os_stat(sh->os, scratch, absolute(sh, &scratch, name), 1);
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
        Stat st   = os_stat(sh->os, scratch, full, 1);
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

static i32 b_alias(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    iz i = 1;
    if (i<nargs && args[i]=="-p") i++;
    i = skipdashdash(args, nargs, i);
    if (i == nargs) {
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
            bprint(sh, "alias ");
            bprint(sh, all[k]->name);
            bprint(sh, "=");
            bprint(sh, singlequote(&scratch, all[k]->value));
            bprint(sh, "\n");
        }
        return 0;
    }
    i32 status = 0;
    for (; i < nargs; i++) {
        Cut c = cut(args[i], '=');
        if (!c.ok) {
            Alias *a = findalias(sh->aliases, args[i]);
            if (a && a->value.data) {
                bprint(sh, "alias ");
                bprint(sh, a->name);
                bprint(sh, "=");
                bprint(sh, singlequote(&scratch, a->value));
                bprint(sh, "\n");
            } else {
                builtinerror(sh, scratch, "alias", concat(&scratch, args[i], ": not found"));
                status = 1;
            }
            continue;
        }
        if (!c.head.len) {
            builtinerror(sh, scratch, "alias", concat3(&scratch, "`", args[i], "': invalid alias name"));
            status = 1;
            continue;
        }
        defalias(sh, c.head, c.tail);
    }
    return status;
}

static i32 b_unalias(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    iz i = 1;
    if (i<nargs && args[i]=="-a") {
        sh->aliases = 0;
        return 0;
    }
    i = skipdashdash(args, nargs, i);
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
    if (!parsei64(g.value, &optind) || optind < 1) optind = 1;
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
    for (iz k = 0; k < s.len; k++) {
        if (s[k]<'0' || s[k]>'7') {
            builtinerror(sh, scratch, "umask", concat(&scratch, s, ": octal number out of range"));
            return 1;
        }
        m = m*8 + (s[k] - '0');
    }
    sh->umask = m & 0777;
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
    if (reset) sh->cmdhash = 0;
    if (i == nargs) {
        if (path.data || del || show) {
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

    i32 status = 0;
    b32 multi  = nargs-i > 1;
    for (; i < nargs; i++) {
        Str name = args[i];
        if (path.data) {
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
        } else if (!haspath(sh, name) && !findbuiltin(name) && !findfunc(sh, name)) {
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

static void compbuiltins(Arena *, Slice<Str> *);

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
        case CG_BUILTIN:  compbuiltins(a, &names);             break;
        case CG_EXPORT:   compvars(sh->vars, 1, a, &names);    break;
        case CG_FUNCTION: compfuncs(sh->funcs, a, &names);     break;
        case CG_VARIABLE: compvars(sh->vars, 0, a, &names);    break;
        case CG_KEYWORD:
            for (iz j = 0; j < countof(reserved); j++) {
                names = push(a, names, reserved[j].word);
            }
            break;
        case CG_SETOPT:
            for (i32 k = 0; k < O_COUNT; k++) names = push(a, names, setoptions[k].name);
            break;
        case CG_SHOPT:
            for (i32 k = 0; k < S_COUNT; k++) names = push(a, names, shoptnames[k]);
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
static i32 b_kill(Shell *, Str *, iz, Arena);
static i32 b_jobs(Shell *, Str *, iz, Arena);  // jobs.cpp
static i32 b_wait(Shell *, Str *, iz, Arena);
static i32 b_disown(Shell *, Str *, iz, Arena);
static i32 b_fg(Shell *, Str *, iz, Arena);
static i32 b_suspend(Shell *, Str *, iz, Arena);

static Builtin builtins[] = {
    {".",        b_source,   B_SPECIAL},
    {":",        b_colon,    B_SPECIAL},
    {"[",        b_test,     0},
    {"alias",    b_alias,    0},
    {"bg",       b_fg,       0},
    {"break",    b_break,    B_SPECIAL},
    {"builtin",  b_builtin,  0},
    {"caller",   b_caller,   0},
    {"cd",       b_cd,       0},
    {"command",  b_command,  0},
    {"compgen",  b_compgen,  0},
    {"continue", b_continue, B_SPECIAL},
    {"declare",  b_declare,  B_DECL},
    {"disown",   b_disown,   0},
    {"echo",     b_echo,     0},
    {"eval",     b_eval,     B_SPECIAL},
    {"exec",     b_exec,     B_SPECIAL},
    {"exit",     b_exit,     B_SPECIAL},
    {"export",   b_export,   B_SPECIAL|B_DECL},
    {"false",    b_false,    0},
    {"fg",       b_fg,       0},
    {"getopts",  b_getopts,  0},
    {"hash",     b_hash,     0},
    {"jobs",     b_jobs,     0},
    {"kill",     b_kill,     0},
    {"let",      b_let,      0},
    {"local",    b_local,    B_DECL},
    {"printf",   b_printf,   0},
    {"pwd",      b_pwd,      0},
    {"read",     b_read,     0},
    {"readonly", b_readonly, B_SPECIAL|B_DECL},
    {"return",   b_return,   B_SPECIAL},
    {"set",      b_set,      B_SPECIAL},
    {"shift",    b_shift,    B_SPECIAL},
    {"shopt",    b_shopt,    0},
    {"source",   b_source,   0},
    {"suspend",  b_suspend,  0},
    {"test",     b_test,     0},
    {"times",    b_times,    B_SPECIAL},
    {"trap",     b_trap,     B_SPECIAL},
    {"true",     b_true,     0},
    {"type",     b_type,     0},
    {"typeset",  b_declare,  B_DECL},
    {"umask",    b_umask,    0},
    {"unalias",  b_unalias,  0},
    {"unset",    b_unset,    B_SPECIAL},
    {"wait",     b_wait,     0},
};

static void compbuiltins(Arena *a, Slice<Str> *out)
{
    for (iz i = 0; i < countof(builtins); i++) *out = push(a, *out, builtins[i].name);
}

static Builtin *findbuiltin(Str name)
{
    iz lo = 0, hi = countof(builtins);
    while (lo < hi) {
        iz  mid = lo + (hi - lo)/2;
        i32 c   = compare(builtins[mid].name, name);
        if (!c) return builtins + mid;
        if (c < 0) lo = mid + 1; else hi = mid;
    }
    return 0;
}
