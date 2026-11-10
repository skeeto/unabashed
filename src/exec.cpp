// Executor: commands, redirections, subshells and programs
//
// Subshells and command substitutions run on the caller's stack,
// isolated by the store's journal and the redirection undo list.
// Pipeline stages run concurrently (proc.cpp, sched.cpp).
//
// This is free and unencumbered software released into the public domain.

struct Builtin {
    Str name;
    i32 (*fn)(Shell *, Str *, iz, Arena);
    i32 flags;
};

enum : i32 {
    B_SPECIAL = 1<<0,  // POSIX special builtin
    B_DECL    = 1<<1,  // declaration builtin: assignment arguments
    B_MSYS    = 1<<2,  // msys mode only (cygpath)
    B_NOHELP  = 1<<3,  // --help is an ordinary argument
};

static Builtin *findbuiltin(Shell *, Str name);
static Builtin *tablebuiltin(Str name);
static Builtin *usablebuiltin(Shell *, Builtin *);
static Rlimit   nofilelimit(Shell *);  // builtins.cpp
static i32      runbuiltin(Shell *, Builtin *, Str *, iz, Arena);
static b32      parsei64(Str, i64 *);
static void     execute(Shell *, Node *, Arena);
static void     showps0(Shell *);  // unabashed.cpp
static Str      histline(void *, Lexer *, Str, b32);  // history.cpp
static i32      runsource(Shell *, Str src, Str iname, Arena, b32 reader = 0, i32 line = 1,
                          b32 (*fill)(void *, Lexer *) = 0, void *fillctx = 0,
                          b32 comsub = 0, b32 hist = 0);
static Str      runfunsub(Shell *, Arena *, Str src, Capture *, Arena);
static i32      compatlevel(Shell *);  // builtins.cpp
static void     joinpsubs(Shell *);
static void     closeall(Shell *);

// Jobs (jobs.cpp)
static b32      fgjobctl(Shell *);
static i32      fgstopped(Shell *, Arena, Handle proc, i64 pid, i32 sig);
static Handle   spawnstage(Shell *, Node *, File *in, File *out, i64 grp,
                           i64 *pid, Arena);
static b32      runfgimage(Shell *, Node *, Arena);
static void     runasync(Shell *, Node *, Arena);
static void     runcoproc(Shell *, Node *, Arena);
static void     notifyjobs(Shell *, Arena);
static void     notifyprompt(Shell *, Arena, Job *only);
static void     polljobs(Shell *);
static void     cleanupjobs(Shell *);
static void     orphanjobs(Shell *);
static void     reaporphans(Sched *);
static void     coprocfdclosed(Shell *, i32 fd);
static void     hidecoproc(Shell *, Arena *);

// Signals and traps (trap.cpp)
static void     checksigs(Shell *, Arena);
static void     runerrtrap(Shell *);
static b32      rundebugtrap(Shell *, Arena);
static b32      trapset(Shell *, i32 idx);
static void     runreturntrap(Shell *, Arena);
static void     subshelltraps(Shell *);
static void     chldreaped(Shell *, i32);
static void     sigenter(Shell *, Sigproc *, i64 pid, Task *);
static i64      newvpid(Shell *);
static void     sigleave(Shell *, Sigproc *);
static void     reportsig(Shell *, Arena, i32 sig, i64 pid, Str text);
static void     intrnewline(Shell *);

// Under job control the terminal's INT reached only the foreground job,
// which died of it. As bash, the shell acts as if it had it too in a
// loop, or untrapped interactively; an interactive loop is abandoned
// even if INT is trapped (the trap waits for an INT of its own).
// Otherwise a trapped INT starts a new line after the echoed ^C.
static void intrjob(Shell *sh)
{
    b32 trapped = trapset(sh, 2);
    b32 abandon = sh->interactive && sh->loopdepth;
    if (sh->loopdepth || (!trapped && sh->interactive)) {
        sh->sig->waitsig |= sigbit(2);
        if (trapped && abandon) sh->sig->bydefault |= sigbit(2);
    }
    if (trapped && !abandon) intrnewline(sh);
}
static b32      trapignored(Shell *, i32 sig);

// Signals the current level ignores, for the programs it starts.
static u64 ignoredsigs(Shell *sh)
{
    u64 m = sh->hardign | sh->entryign;
    for (i32 sig = 1; (sh->traps || sh->asyncign) && sig < NSIGNAL; sig++) {
        if (trapignored(sh, sig)) m |= (u64)1 << (sig-1);
    }
    return m;
}

// Between the commands of a `;` list, an interactive shell with job
// control reports the jobs that changed, as bash (not in POSIX mode).
static void listnotify(Shell *sh, Arena scratch)
{
    if (sh->jobs && sh->jobctl && sh->interactive && !sh->subshell && !sh->opt[O_POSIX]) {
        notifyprompt(sh, scratch, 0);
    }
}

// The command a subshell running n would exec as its last instead of
// waiting for it, as bash does: the last of a list, a simple command.
static Node *tailcommand(Node *n)
{
    while (n && !n->redirs && !(n->flags & (CMD_INVERT|CMD_TIME))) {
        if (n->kind==N_CONNECT && (n->connector==';' || n->connector=='\n')) {
            n = n->b;
        } else if (n->kind == N_GROUP) {
            n = n->a;
        } else {
            return n->kind==N_SIMPLE ? n : 0;
        }
    }
    return 0;
}

// Whether bash would still exec the tail command once a list reaches
// it: not while an EXIT or ERR trap, a RETURN trap in a function, or a
// caught signal's trap is set, as those must run in the subshell.
static b32 tailexecs(Shell *sh)
{
    Str *t = sh->traps;
    if (!t) return 1;
    if (t[TRAP_EXIT].data || t[TRAP_ERR].data) return 0;
    if (sh->funcdepth && t[TRAP_RETURN].data) return 0;
    for (i32 i = 1; i < NSIGNAL; i++) {
        if (t[i].len) return 0;
    }
    return 1;
}

// A list is about to run n: if it is the tail and bash would not exec
// it now, it runs as any command, its death its own to report.
static void reachtail(Shell *sh, Node *n)
{
    if (n==sh->tailnode && sh->taillevel==sh->subshell+1 && !tailexecs(sh)) {
        sh->taillevel = 0;
    }
}

// Report the last foreground child's death by a signal, if it was one.
static void reportchild(Shell *sh, Arena scratch, Node *n)
{
    i32 sig = sh->childsig;
    if (!sig || (n==sh->tailnode && sh->taillevel==sh->subshell+1)) return;  // the parent reports
    sh->childsig = 0;
    if (sh->noreport) return;
    Node c = *n;
    c.flags &= ~(CMD_INVERT|CMD_TIME|CMD_TIMEPOSIX);
    Printer *p = newprinter(&scratch);
    printcmd(p, &c);
    reportsig(sh, scratch, sig, sh->childpid, finish(&p->b));
}

// Builtin output, written to fd 1 when the builtin returns, or when
// the buffer fills: growing it would overrun the builtin's scratch.
static void bprint(Shell *sh, Str s)
{
    BOut *o = sh->bout;
    if (o->b.cap-o->b.len < s.len) {
        if (o->b.len) {
            o->failed |= !fdwrite(sh, 1, finish(&o->b));
            o->b.len = 0;
        }
        if (o->b.cap < s.len) {
            o->failed |= !fdwrite(sh, 1, s);
            return;
        }
    }
    print(&o->b, s);
}

static void bprinti(Shell *sh, i64 v)
{
    u8  tmp[32];
    Buf b = {};
    b.data = tmp;
    b.cap  = countof(tmp);
    print(&b, v);
    bprint(sh, finish(&b));
}

// The scratch arena for a nesting depth, reserved on first use.
static Arena *scratchat(Shell *sh, i32 depth)
{
    if (depth >= NPOOL) {
        os_oom();
    }
    if (!sh->pool[depth]) {
        // 32-bit address space holds only a few full-size reservations:
        // the first level (most comsubs) gets half, deeper ones what a
        // vproc gets
        iz    cap = sizeof(void *)==8 ? sh->reserve
                  : depth<=1          ? sh->reserve/2
                  :                     sh->sched->reserve;
        Arena a = newarena(sh->os, cap);
        sh->pool[depth] = alloc<Arena>(&a);
        *sh->pool[depth] = a;
    }
    return sh->pool[depth];
}

// checkwinsize: after a foreground program, LINES and COLUMNS take the
// size of the terminal on the shell's own standard error (not the
// program's, which a redirection may have changed), as bash does
// outside subshells, interactive or not.
static void checkwinsize(Shell *sh)
{
    if (!sh->shopt[S_CHECKWINSIZE] || sh->subshell) return;
    Handle h = sh->sched->err;
    Tty    t = os_tty(sh->os, h, h, TTY_QUERY);
    if (t.err || t.rows<1 || t.cols<1) return;
    Arena a = *scratchat(sh, sh->capdepth+1);
    Str num = fmti64(&a, t.rows);
    setvar(sh, a, "LINES",   num);
    Str num2 = fmti64(&a, t.cols);
    setvar(sh, a, "COLUMNS", num2);
}

// Before memory holding the BASH_COMMAND node is reused, keep its text
// in the store instead.
static void keepcmdtext(Shell *sh, byte *beg, byte *end)
{
    byte *p = (byte *)sh->cmdnode;
    if (!p || p<beg || p>=end) return;
    Str s = {};
    if (sh->capdepth+1 < NPOOL) {
        Arena scratch = *scratchat(sh, sh->capdepth+1);
        s = cmdtext(sh, &scratch);
    }
    Store *st  = sh->store;
    Str    old = sh->cmdtext;
    sh->cmdtext = sclone(st, s);
    sh->cmdnode = 0;
    if ((byte *)old.data >= st->mark) {
        sfreestr(st, old);
    }
}


// Arithmetic

struct ShellArith {
    Shell *sh;
    Arena *a;
    Str    who;       // the command, for messages: "((", "let", or none
    b32    rawsub;    // let under array_expand_once: subscripts as is
    b32    readonly;  // an assignment hit a readonly variable
};

// An indexed subscript as arithmetic expanded it (arithsubscript): its
// double quotes are backslash-quoted, but as in bash they are still
// removed when it is evaluated.
static Str arithindex(Shell *sh, Arena *a, Str name, Str sub)
{
    Value *v = lookup(sh, name);
    if ((v && v->arr && v->arr->bucket) || find(sub, "\\\"")<0) return sub;
    Buf b(a, sub.len);
    for (iz i = 0; i < sub.len; i++) {
        i += sub[i]==0x5c && i+1<sub.len && sub[i+1]==0x22;
        putbyte(&b, sub[i]);
    }
    return finish(&b);
}

// Whether a subscript's shell quoting is left open (let "a[$b]" with
// b="80's"): bash cannot expand such a key.
static b32 openquote(Str s)
{
    for (iz i = 0; i < s.len; i++) {
        if (s[i] == 0x5c) {
            i++;
        } else if (s[i]==0x27 || s[i]==0x22) {
            u8 q = s[i];
            for (i++; i<s.len && s[i]!=q; i++) {
                i += q==0x22 && s[i]==0x5c;
            }
            if (i >= s.len) return 1;
        }
    }
    return 0;
}

// Evaluate an indexed subscript in arithmetic: expanded first, unless
// let runs under array_expand_once, which takes it as it is.
static i64 arithsubindex(ShellArith *c, X *x, Arena *a, Str sub)
{
    return c->rawsub ? evalinteger(c->sh, a, sub).value : subindex(x, sub);
}

static Str arithget(void *ctx, ArithRef *ref, b32 *isset, Arena *a)
{
    ShellArith *c = (ShellArith *)ctx;
    if (ref->hassub) {
        X     *x = newx(c->sh, a);
        Value *v = lookup(c->sh, ref->name);
        b32    assoc = v && v->arr && v->arr->bucket;
        ref->sub = arithindex(c->sh, a, ref->name, ref->sub);
        if (!assoc && !ref->sub.len) {
            // As bash: reported (twice) and read as unset, but the
            // expression goes on
            Str msg = concat(a, ref->name, "[]: bad array subscript");
            shellerror(c->sh, *a, msg);
            shellerror(c->sh, *a, msg);
            *isset = 0;
            return {};
        }
        if (!assoc && !allsub(ref->sub)) {
            // Evaluate an indexed subscript once: the assignment that
            // may follow (a[RANDOM%6]++) reuses this index
            ref->sub = fmti64(a, arithsubindex(c, x, a, ref->sub));
        }
        if (assoc && !c->rawsub && openquote(ref->sub)) {
            // As bash: reported (twice) and read as unset
            Str msg = concat3(a, ref->name, "[", concat(a, ref->sub, "]: bad array subscript"));
            shellerror(c->sh, *a, msg);
            shellerror(c->sh, *a, msg);
            *isset = 0;
            return {};
        }
        if (assoc && c->rawsub && !allsub(ref->sub)) {
            // let under array_expand_once: the key as it is
            HItem *t = hfind(v->arr, ref->sub);
            *isset = !!t;
            return t ? t->val : Str{};
        }
        Param p = getelems(x, ref->name, ref->sub);
        if (p.list) {
            p.value = joinparams(a, Args{p.lv, p.ln}, starsep(c->sh));
        }
        *isset = p.set || varvisible(c->sh, ref->name);
        return p.value;
    }
    Got g = getvar(c->sh, a, ref->name);
    *isset = g.set;
    return g.value;
}

static Str arithset(void *ctx, ArithRef *ref, i64 value, Arena *a)
{
    ShellArith *c = (ShellArith *)ctx;
    Str v = fmti64(a, value);
    if (ref->hassub) {
        Value *var = lookup(c->sh, ref->name);
        b32    assoc = var && var->arr && var->arr->bucket;
        ref->sub = arithindex(c->sh, a, ref->name, ref->sub);
        if ((!assoc && !ref->sub.len) || (assoc && !c->rawsub && openquote(ref->sub))) {
            // As bash: reported, and the expression goes on without it
            Str msg = concat3(a, "`", ref->name, concat3(a, "[", ref->sub, "]': not a valid identifier"));
            if (c->who.len) msg = concat3(a, c->who, ": ", msg);
            shellerror(c->sh, *a, msg);
            return {};
        }
        if (!assoc && c->rawsub && !allsub(ref->sub)) {
            ref->sub = fmti64(a, arithsubindex(c, 0, a, ref->sub));
        }
        Sub s = evalsub(c->sh, a, ref->name, ref->sub, {}, assoc && c->rawsub);
        if (!s.ok) {
            return concat3(a, ref->name, "[", concat(a, ref->sub, "]: bad array subscript"));
        }
        if (!setelem(c->sh, *a, ref->name, s.idx, s.key, v, SV_QUIET)) {
            c->readonly = 1;
            return concat(a, ref->name, ": readonly variable");
        }
        return {};
    }
    if (!setvar(c->sh, *a, ref->name, v, SV_QUIET)) {
        Value *t = lookup(c->sh, ref->name);
        if (!t || !(t->attrs & V_READONLY)) return {};  // reported
        c->readonly = 1;
        return concat(a, ref->name, ": readonly variable");
    }
    return {};
}

// let's text is already expanded: as in bash (compat 52 and later, or
// under array_expand_once), an associative subscript is not skipped by
// shell quoting
static b32 arithliteralsub(void *ctx, Str name)
{
    ShellArith *c = (ShellArith *)ctx;
    if (compatlevel(c->sh)<=51 && !c->sh->shopt[S_ARRAY_EXPAND_ONCE]) return 0;
    Value *v = lookup(c->sh, name);
    return v && v->arr && v->arr->bucket;
}

static Arith arithrun(Shell *sh, Arena *a, Str expr, b32 *readonly, Str who = {})
{
    ShellArith c = {};
    c.sh     = sh;
    c.a      = a;
    c.who    = who;
    c.rawsub = who=="let" && sh->shopt[S_ARRAY_EXPAND_ONCE];
    ArithEnv env = {};
    env.ctx     = &c;
    env.get     = arithget;
    env.set     = arithset;
    env.nounset = sh->opt[O_NOUNSET];
    if (who == "let") env.literalsub = arithliteralsub;
    Arith r = arith(&env, a, expr);
    if (!r.ok && r.unbound) {
        fatalerror(sh, *a, r.err);  // set -u: fatal, with no prefix
    }
    *readonly = c.readonly;
    return r;
}

static Arith arithraw(Shell *sh, Arena *a, Str expr)
{
    b32 readonly = 0;
    return arithrun(sh, a, expr, &readonly);
}

// Evaluate an arithmetic expansion: an error abandons the command.
static Arith evalarith(Shell *sh, Arena *a, Str expr)
{
    Arith r = arithraw(sh, a, expr);
    if (!r.ok && sh->opt[O_POSIX] && !sh->interactive) {
        // as bash: fatal in a POSIX non-interactive shell, status 127
        // (1 in a subshell that catches it)
        shellerror(sh, *a, r.err);
        sh->status = sh->catchfatal ? 1 : 127;
        escape(sh, ESC_FATAL);
    } else if (!r.ok) {
        expansionerror(sh, *a, r.err);
    }
    return r;
}

// Evaluate the value assigned to an integer variable. As in bash, an
// error abandons not only the command but any -c, eval or source text.
static Arith evalinteger(Shell *sh, Arena *a, Str expr)
{
    Arith r = arithraw(sh, a, expr);
    if (!r.ok) {
        shellerror(sh, *a, r.err);
        sh->status = 1;
        escape(sh, ESC_ABORT);
    }
    return r;
}

// Evaluate arithmetic for a command such as (( )) or let, where an
// error is reported with a prefix and the command just fails.
static Arith evalarithcmd(Shell *sh, Arena *a, Str expr, Str who)
{
    b32 readonly = 0;
    Str savedname = sh->bltname;
    sh->bltname = who;
    Arith r = arithrun(sh, a, expr, &readonly, who);
    sh->bltname = savedname;
    if (!r.ok && readonly) {
        shellerror(sh, *a, r.err);  // bash reports these with no prefix
    } else if (!r.ok) {
        // a shell error, not a builtin's: no line below 1, as bash
        Str msg = concat3(a, who, ": ", r.err);
        shellerror(sh, *a, msg);
    }
    return r;
}

// Errors that end a non-interactive shell in POSIX mode, as bash.
static b32 posixfatal(Shell *sh)
{
    return sh->opt[O_POSIX] && !sh->interactive;
}

// A for or select variable that is not a name ends a POSIX shell.
static void posixbadvar(Shell *sh)
{
    if (posixfatal(sh)) {
        sh->status  = 2;
        sh->exiting = 1;
        escape(sh, ESC_EXIT);
    }
}

// An expansion error abandons the current complete command, and so
// ends a subshell.
static void expansionerror(Shell *sh, Arena a, Str msg)
{
    shellerror(sh, a, msg);
    sh->status = 1;
    escape(sh, posixfatal(sh) ? ESC_FATAL : ESC_DISCARD);  // as bash
}

// A fatal expansion error, such as an unbound variable, ends the shell.
static void fatalerror(Shell *sh, Arena a, Str msg)
{
    shellerror(sh, a, msg);
    sh->status = 1;
    escape(sh, ESC_FATAL);
}


// Subshells

enum : i32 { SUB_NODE, SUB_SOURCE, SUB_FUNSUB, SUB_CALL };

struct SubJob {
    i32   kind;
    Node *node;
    Str   src;
    b32   newloops;  // break and continue cannot see enclosing loops
    b32   comsub;    // SUB_SOURCE from $(...): see runsource
    b32   nojobs;    // a ( list ): it starts with no jobs or coprocesses
    Func *fn;        // SUB_CALL: a function and its arguments
    Str  *argv;
    iz    argc;
};

static i32 callfunction(Shell *, Func *, Str *, iz, Arena);

static void runjob(Shell *sh, SubJob *j, Arena scratch)
{
    switch (j->kind) {
    case SUB_NODE:
        execute(sh, j->node, scratch);
        break;
    case SUB_SOURCE:
        // Parsed again at run time, so its syntax errors are its own
        sh->tracedepth++;  // restored with the subshell
        runsource(sh, j->src, "command substitution", scratch, 0, sh->line,
                  0, 0, j->comsub);
        break;
    case SUB_FUNSUB:
        runsource(sh, j->src, sh->iname, scratch, 0, sh->line, 0, 0, 1);
        break;
    case SUB_CALL:
        sh->status = callfunction(sh, j->fn, j->argv, j->argc, scratch);
        break;
    }
}

// Run a job in a synchronous subshell and return its exit status.
// Everything it changes is rolled back. With cap, its standard output
// is collected there.
[[gnu::noinline]]
static i32 subshell(Shell *sh, SubJob *j, Arena scratch, Buf *cap)
{
    Shell    *saved = alloc<Shell>(&scratch);  // large: not on the stack
    *saved = *sh;
    Watermark w     = storeenter(sh->store);
    Escape    e     = {};
    e.up    = sh->esc;
    e.bout  = sh->bout;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    sh->subshell++;
    sh->bashpid = newvpid(sh);
    sh->psubs   = 0;
    sh->killsig   = 0;
    sh->errexited = 0;
    sh->jobctl   &= j->comsub;  // as bash, which keeps it in a comsub
    sh->incomsub |= j->comsub;
    // a simple command (a stage, a job) is transparent, as bash's child
    // for it goes on with the shell's own point of return
    sh->catchfatal |= j->kind==SUB_SOURCE || j->nojobs ||
                      (j->kind==SUB_NODE && j->node->kind!=N_SIMPLE);
    subshelltraps(sh);
    Sigproc *up = sh->sig;
    up->comsub = j->comsub;
    Sigproc sig;
    sigenter(sh, &sig, sh->bashpid, sh->sched->cur);
    // The frame is not in the store, so local's changes to its list of
    // locals are not journaled: restore it by hand
    Frame frame = sh->frame ? *sh->frame : Frame{};
    if (j->newloops) sh->loopdepth = 0;
    if (j->nojobs) {
        sh->jobs   = 0;
        sh->curjob = sh->prevjob = 0;
        sh->bgstats = 0;
        if (sh->coprocs) hidecoproc(sh, &scratch);
    }
    Fdsave fds = savefds(sh, &scratch);
    if (cap) {
        File *out = newfile(sh, F_CAPTURE, OF_WRITE);
        out->cap = cap;
        fdset(sh, 1, out, 0, &scratch);
    }
    if (!jmpset(e.jmp)) {
        runjob(sh, j, scratch);
    } else if (e.why==ESC_ABORT && (j->kind==SUB_SOURCE || sh->cflag)) {
        // As bash, whose comsub (and -c) text is abandoned with status
        // 1, while a script's reader keeps the command's
        sh->status = 1;
    }
    if (!sh->execed && !sh->killsig && sh->traps) {
        if (sh->errexited) unwind(sh, saved->undo);
        runexittrap(sh, scratch);
    }
    i32 status = sh->status;
    i32 killed = sh->killsig;
    i64 pid    = sh->bashpid;
    if (!killed && sh->childsig && sh->taillevel==sh->subshell+1 &&
        (status & 255)==128+sh->childsig) {
        killed = sh->childsig;  // the program it ran last, as bash would exec it
        pid    = sh->childpid;
    }
    sigleave(sh, &sig);
    up->comsub = 0;
    if (sh->jobs) orphanjobs(sh);  // its jobs run on
    unwind(sh, saved->undo);
    restorefds(sh, fds);
    if (sh->psubs) {
        // Its procsubs end with it, once their descriptors are closed
        joinpsubs(sh);
    }
    storeleave(sh->store, &w);
    // Keep the scratch arenas the subshell reserved for deeper levels:
    // they are not store memory, and dropping them leaks reservations
    for (i32 i = 0; i < NPOOL; i++) saved->pool[i] = sh->pool[i];
    *sh = *saved;
    if (sh->frame) *sh->frame = frame;
    sh->status   = status;
    sh->childsig = killed;
    sh->childpid = pid;
    if (sh->sig->waitsig && !sh->sig->fgwait) {
        u64 died = killed ? sigbit(killed) : 0;
        if (fgjobctl(sh)) {
            // Under job control bash's subshell is a job of its own, so
            // the terminal's signals reached only it: as for a program
            // (runprogram), only an INT death counts, in a loop or
            // untrapped interactively
            u64 got = sh->sig->waitsig;
            sh->sig->waitsig = 0;
            if (killed==2 && (got & sigbit(2))) intrjob(sh);
            died &= sigbit(2);
        }
        waitdone(sh, died, 0);
    }
    return status;
}

// Drop trailing newlines, and with crlf the CR before each of them, as
// Git Bash and busybox-w32 do: "a\r\r\n" keeps one CR.
static Str trimnewlines(Str s, b32 crlf)
{
    while (s.len && s[s.len-1]=='\n') {
        s.len--;
        if (crlf && s.len && s[s.len-1]=='\r') s.len--;
    }
    return s;
}

// The result of a substitution from its output: as bash, drop NUL
// bytes with a warning (and with igncr every CR), then trailing
// newlines.
static Str capresult(Shell *sh, Arena scratch, Str s)
{
    b32 nocr = igncr(sh);
    iz  n    = 0;
    iz  nuls = 0;
    for (iz i = 0; i < s.len; i++) {
        s[n]  = s[i];
        nuls += !s[i];
        n    += s[i] && !(nocr && s[i]=='\r');
    }
    if (nuls) {
        shellerror(sh, scratch, "warning: command substitution: ignored null byte in input");
    }
    s.len = n;
    return trimnewlines(s, sh->pathmode!=PM_POSIX);
}

static Str readwholefile(Shell *, Arena *, Str, i32 *);

static Str redirtarget(Shell *, Arena *, Word *, b32 *);

// The file word of $(< file), or null if src is not that form.
static Word *fileread(Shell *sh, Arena *a, Str src)
{
    Str t = trim(src);
    if (!t.len || t[0]!='<') return {};
    Arena tmp = *a;
    Lexer *L = newlexer(&tmp, t);
    L->stackmax = (iz)1<<20;
    Parsed p = parseunit(L);
    Node *n = p.cmd;
    if (p.status!=PARSE_OK || !n || n->kind!=N_SIMPLE || n->words || !n->redirs ||
            n->redirs->next || n->redirs->op!=R_INPUT || n->redirs->src!=0 ||
            parseunit(L).status!=PARSE_EOF) {
        return 0;
    }
    *a = tmp;  // keep the word: the caller allocates past it
    return n->redirs->word;
}

// Is the procsub or job with this BASHPID still running?
static b32 pidrunning(Shell *sh, i64 pid)
{
    for (Task *t = sh->psubs; t; t = t->sib) {
        if (t->pid == pid) return t->state != TS_DONE;
    }
    for (Job *j = sh->jobs; j; j = j->next) {
        if (j->pid == pid) return j->task && j->task->state!=TS_DONE;
    }
    return 0;
}

// Finish a capture once its command is done: wait until every pump
// has seen EOF, and point the descriptors still on it at /dev/null.
// Those are what exec left in ${ ...; } (exec 2>&1), which in bash
// write to the substitution's file after no one reads it. So do the
// procsubs and jobs it started that still run: their pumps discard.
static void endcapture(Shell *sh, Capture *c, Arena scratch)
{
    Fdtab *fds = sh->fds;
    for (i32 fd = 0; fd < fds->top; fd++) {
        File *f = fds->fd[fd];
        if (!f || f->kind!=F_CAPTURE || f->cap!=&c->b) continue;
        Opened o = os_open(sh->os, scratch, "/dev/null", OF_WRITE, 0);
        f->kind = F_OS;  // in place: descriptors sharing it follow
        f->cap  = 0;
        f->h    = o.err ? BADHANDLE : o.h;
        if (o.err) f->flags = 0;  // writes fail
    }
    Sched *s = sh->sched;
    for (i32 pass = 0; pass < 2; pass++) {
        // Owned pumps last, giving their writers the longest time
        for (Task **pt = &c->pumps; *pt;) {
            Task *t = *pt;
            Pump *p = (Pump *)t->arg;
            if (!pass && p->owner) {
                pt = &t->sib;
                continue;
            }
            *pt = t->sib;
            if (p->owner && pidrunning(sh, p->owner)) {
                p->buf = 0;
                t->sib = s->orphans;
                s->orphans = t;
            } else {
                joinpump(s, t);
            }
        }
    }
}

// Run a command substitution and return its output, without trailing
// newlines. kind: 0 $(...), 1 `...`, 2 ${ ...; }.
static Str runcapture(Shell *sh, Arena *a, Str src, b32 kind)
{
    sh->nsubst++;
    if (kind != 2) {
        Word *word = fileread(sh, a, src);
        if (word) {
            // $(< file): read the file in the current shell, its name
            // expanded as a redirection's
            b32 ok;
            Str name = redirtarget(sh, a, word, &ok);
            if (!ok) {
                sh->status = 1;
                return {};
            }
            i32 err;
            Str data = readwholefile(sh, a, name, &err);
            if (err) {
                Str msg = concat3(a, name, Str(": "), errtext(err));
                shellerror(sh, *a, msg);
                sh->status = 1;
                return {};
            }
            sh->status = 0;
            return capresult(sh, *a, data);
        }
    }
    Capture *capture = alloc<Capture>(a);
    capture->b = Buf(a, 256);
    Buf *cap = &capture->b;

    i32   depth = ++sh->capdepth;
    Arena child = *scratchat(sh, depth);
    SubJob j = {};
    j.kind   = SUB_SOURCE;
    j.src    = src;
    j.comsub = kind == 0;
    if (kind==2 && src.len && src[0]=='|') {
        // ${| cmd; }: the current shell, expanding to a local REPLY
        Str val = runfunsub(sh, a, cuthead(src, 1), 0, child);
        sh->capdepth = depth - 1;
        sh->current = 0;
        return val;
    } else if (kind == 2) {
        // ${ cmd; }: the current shell, with only fd 1 redirected
        runfunsub(sh, a, src, capture, child);
    } else {
        b32 errexit = sh->opt[O_ERREXIT];
        b32 verbose = sh->opt[O_VERBOSE];
        if (!sh->shopt[S_INHERIT_ERREXIT]) {
            sh->opt[O_ERREXIT] = 0;
        }
        sh->opt[O_VERBOSE] = 0;  // as bash, which turns off -v there
        b32 noreport = sh->noreport;
        sh->noreport = 1;        // nor does it report signal deaths there
        i32 childsig = sh->childsig;
        i64 childpid = sh->childpid;
        subshell(sh, &j, child, cap);
        chldreaped(sh, 1);
        // Its death is not a program's: were it left as one, the
        // command expanding it would be reported killed by it
        sh->childsig = childsig;
        sh->childpid = childpid;
        sh->noreport = noreport;
        sh->opt[O_ERREXIT] = errexit;
        sh->opt[O_VERBOSE] = verbose;
        endcapture(sh, capture, child);
    }
    sh->capdepth = depth - 1;
    sh->current = 0;
    return capresult(sh, *a, finish(cap));
}

// An OS handle writing into a capture, for a writer the shell does not
// wait for in place: a pump appends what it delivers, and the capture
// waits for the pump, or with owner, the procsub or job writing it, for
// as long as that runs. The caller owns the handle.
static Handle capturewriter(Shell *sh, File *f, i64 owner)
{
    OsPipe p = os_pipe(sh->os);
    if (p.err) return BADHANDLE;
    Sched   *s  = sh->sched;
    Pump    *pp = newpump(s);
    Capture *c  = (Capture *)f->cap;
    pp->kind   = PUMP_TOBUF;
    pp->h      = p.r;
    pp->buf    = f->cap;
    pp->owner  = owner;
    pp->wtrace = &c->wtrace;
    Task    *t  = spawnpump(s, pp);
    if (owner) t->group = t->tag;  // tracing: the loose writer's group
    t->sib   = c->pumps;
    c->pumps = t;
    return p.w;
}


// Redirections

// A redirection's word: one field, not split in POSIX mode, nor globbed
// there unless interactive.
static Str redirtarget(Shell *sh, Arena *a, Word *w, b32 *ok)
{
    *ok = 0;
    b32 posix = sh->opt[O_POSIX];
    i32 flags = (posix ? XW_NOSPLIT : 0) | (posix && !sh->interactive ? XW_NOGLOB : 0);
    Slice<Str> r = expandwords(sh, a, w, flags);
    if (r.len != 1) {
        Str msg = concat(a, w->text, ": ambiguous redirect");
        shellerror(sh, *a, msg);
        return {};
    }
    *ok = 1;
    return r[0];
}

// The descriptor named by bash's own /dev/fd/N, /dev/stdin and
// friends, which the shell emulates, or -1 (FDMAX if out of range).
static i32 devfdnum(Str name)
{
    if (name == "/dev/stdin")  return 0;
    if (name == "/dev/stdout") return 1;
    if (name == "/dev/stderr") return 2;
    if (startswith(name, "/dev/fd/") && name.len>8) {
        i64 n = 0;
        b32 num = 1;
        for (iz i = 8; num && i<name.len; i++) {
            num &= digit(name[i]);
            n = n*10 + (name[i] - '0');
            if (n > FDMAX) num = 0;
        }
        if (num) return n<FDMAX ? (i32)n : FDMAX;
    }
    return -1;
}

static Str  procfile(Shell *, Arena *, PathRes);

// Does path name /dev/clipboard, the Windows clipboard's text?
static b32 isclipboard(Shell *sh, Arena scratch, Str path)
{
    if (sh->pathmode == PM_POSIX) return 0;
    PathRes r = resolve(sh, &scratch, path.len ? path : Str("."));
    return r.ok && r.kind==PK_DEV && r.sub=="clipboard";
}

// /dev/zero and /dev/full (zeros), /dev/random and /dev/urandom (the
// process's ChaCha8 stream) opened for reading: an OS pipe fed by an
// endless pump, so that builtins and programs read it alike. The pump
// ends once every read end is closed, reaped by the next one, or is
// abandoned as the shell exits (joinorphans).
static File *genfile(Shell *sh, i32 gen, i32 flags, i32 *err)
{
    Sched *s = sh->sched;
    reaporphans(s);
    OsPipe p = os_pipe(sh->os);
    if (p.err) {
        *err = p.err;
        return 0;
    }
    Pump *pp = newpump(s);
    pp->kind = PUMP_GEN;
    pp->gen  = gen;
    pp->h    = p.w;
    Task *t  = spawnpump(s, pp);
    t->sib     = s->orphans;
    s->orphans = t;
    File *f = newfile(sh, F_OS, flags);
    f->h = p.r;
    return f;
}

// openfile in the Windows modes, where the shell resolves the virtual
// files: /dev/fd/N and /proc/self/fd/N are its own descriptors, /dev/tty
// the console, and the /proc files it knows are text.
static File *openwin(Shell *sh, Arena *a, Str name, i32 flags)
{
    PathRes r    = resolve(sh, a, name);
    Str     path = r.native;
    i32     err  = 0;
    if (!r.ok) {
        err = r.kind==PK_DEV ? E_NOTDIR : E_NOENT;
    } else if (r.kind == PK_FD) {
        File *f = r.num<FDMAX ? sh->fds->fd[r.num] : 0;
        if (!f) {
            Str msg = concat(a, name, ": Bad file descriptor");
            shellerror(sh, *a, msg);
            return 0;
        }
        f->refs++;
        return f;
    } else if (r.kind == PK_TTY) {
        path = flags==OF_READ ? Str("//./CONIN$") : Str("//./CONOUT$");
    } else if (r.kind==PK_DEV && r.sub=="clipboard" && !(flags&OF_WRITE)) {
        // A snapshot of its text as it is now, CRLF kept, empty (not an
        // error) when it holds none
        clipsettle(sh->sched);
        ClipText c = os_clipget(sh->os, a);
        err = c.err;
        if (!err) {
            File *f = newfile(sh, F_MEM, OF_READ);
            f->mem    = sclone(sh->store, c.text);
            f->ownmem = 1;
            return f;
        }
    } else if (r.kind==PK_DEV && r.sub=="clipboard") {
        // Written: what goes through this open accumulates (Clip)
        File *f = newfile(sh, F_CLIP, flags);
        f->clip = newclip(sh->sched);
        return f;
    } else if (r.kind == PK_DEV) {
        // zero, full, random and urandom read from a pump; written, all
        // are sinks but full
        b32 dir  = !r.sub.len || r.sub=="fd";
        b32 full = r.sub == "full";
        i32 gen  = r.sub=="zero" || full ? GEN_ZERO
                 : r.sub=="random" || r.sub=="urandom" ? GEN_RANDOM : -1;
        err  = dir ? E_ISDIR : flags&OF_WRITE || gen>=0 ? 0 : E_NOENT;
        path = "//./NUL";
        if (!err && (flags & OF_READ) && gen>=0) {
            File *f = genfile(sh, gen, flags, &err);
            if (f) {
                f->full = full;
                return f;
            }
        } else if (!err && full) {
            // Writes fail: the shell's with ENOSPC, a program's on a
            // handle opened only for reading
            Opened o = os_open(sh->os, *a, path, OF_READ, 0);
            err = o.err;
            if (!err) {
                File *f = newfile(sh, F_OS, flags);
                f->h    = o.h;
                f->full = 1;
                return f;
            }
        }
    } else if (r.kind == PK_PROC) {
        Str text = procfile(sh, a, r);
        if (text.data && !(flags & OF_WRITE)) {
            File *f = newfile(sh, F_MEM, OF_READ);
            f->mem    = sclone(sh->store, text);
            f->ownmem = 1;
            return f;
        }
        Stat st = pathstat(sh, *a, name, 1);
        err = st.err ? st.err : st.type==FT_DIR ? E_ISDIR : E_ACCES;
    } else if (r.kind == PK_CYGDRIVE) {
        err = E_ISDIR;
    }
    if (err) {
        Str msg = concat3(a, name, Str(": "), errtext(err));
        shellerror(sh, *a, msg);
        return 0;
    }
    Opened o = os_open(sh->os, *a, path, flags, 0666);
    if (o.err) {
        Str msg = concat3(a, name, Str(": "), errtext(o.err));
        shellerror(sh, *a, msg);
        return 0;
    }
    File *f = newfile(sh, F_OS, flags);
    f->h = o.h;
    return f;
}

static b32 restrictedwrite(Shell *, Arena *, Str);

static File *openfile(Shell *sh, Arena *a, Str name, i32 flags)
{
    if ((flags & OF_WRITE) && restrictedwrite(sh, a, name)) return 0;
    if (!name.len) {
        // absolute() would turn it into the working directory
        Str msg = concat(a, name, ": No such file or directory");
        shellerror(sh, *a, msg);
        return 0;
    }
    if (sh->pathmode != PM_POSIX) return openwin(sh, a, name, flags);
    Str path = absolute(sh, a, name);
    i32 dupfd = devfdnum(name);
    if (dupfd >= 0) {
        File *f = dupfd<FDMAX ? sh->fds->fd[dupfd] : 0;
        if (!f) {
            Str msg = concat(a, name, ": Bad file descriptor");
            shellerror(sh, *a, msg);
            return 0;
        }
        f->refs++;
        return f;
    }
    Opened o = os_open(sh->os, *a, path, flags, 0666);
    if (o.err) {
        Str msg = concat3(a, name, Str(": "), errtext(o.err));
        shellerror(sh, *a, msg);
        return 0;
    }
    File *f = newfile(sh, F_OS, flags);
    f->h = o.h;
    return f;
}

static i32 parsefd(Str s)
{
    if (!s.len || s.len>9) return -1;
    i32 n = 0;
    for (iz i = 0; i < s.len; i++) {
        if (!digit(s[i])) return -1;
        n = n*10 + (s[i] - '0');
    }
    return n;
}

// A restricted shell writes no files by redirection, as bash: a write
// redirection to a file is refused, reported, true.
static b32 restrictedwrite(Shell *sh, Arena *a, Str name)
{
    if (!sh->restricted) return 0;
    Str msg = concat(a, name, ": restricted: cannot redirect output");
    shellerror(sh, *a, msg);
    return 1;
}

// Under set -C, refuse to truncate an existing regular file.
static b32 clobbers(Shell *sh, Arena *a, Str name)
{
    if (restrictedwrite(sh, a, name)) return 1;
    if (!sh->opt[O_NOCLOBBER]) return 0;
    Stat st = pathstat(sh, *a, name, 1);
    if (!st.err && st.type==FT_FILE) {
        Str msg = concat(a, name, ": cannot overwrite existing file");
        shellerror(sh, *a, msg);
        return 1;
    }
    return 0;
}

static b32 applyredirs(Shell *, Redir *, Arena *, b32);

// Apply a command's redirections. Returns false after reporting an
// error (the redirections done so far are left for the caller to undo),
// with the command's status in sh->redirfail. As bash, an expansion
// error in a redirection, even a fatal one such as ${x?}, only fails
// the redirection, keeping the error's status.
static b32 doredirs(Shell *sh, Redir *list, Arena *a, b32 keep)
{
    Escape e = {};
    e.up        = sh->esc;
    e.bout      = sh->bout;
    e.undo      = sh->undo;
    e.frame     = sh->frame;
    e.loopdepth = sh->loopdepth;
    sh->esc = &e;
    b32 ok = 0;
    sh->redirfail = 1;
    if (!jmpset(e.jmp)) {
        ok = applyredirs(sh, list, a, keep);
        sh->esc = e.up;
        return ok;
    }
    sh->esc = e.up;
    if (e.why!=ESC_DISCARD && e.why!=ESC_FATAL && e.why!=ESC_ABORT) {
        escape(sh, e.why);
    }
    sh->bout      = e.bout;
    sh->loopdepth = e.loopdepth;
    // as the shell, or the subshell that catches it, would exit
    sh->redirfail = e.why!=ESC_FATAL ? 1 : sh->catchfatal ? 1 : 127;
    return 0;
}

static b32 applyredirs(Shell *sh, Redir *list, Arena *a, b32 keep)
{
    Fdtab *t = sh->fds;
    for (Redir *r = list; r; r = r->next) {
        i32 fd   = r->src;
        b32 kept = keep;
        b32 setfdvar = 0;
        if (r->srcvar) {
            // {var}>: allocate a descriptor from 10 up
            fd = FDUSER;
            while (fd<FDMAX && t->fd[fd]) fd++;
            if (fd == FDMAX) {
                shellerror(sh, *a, "redirection error: cannot duplicate fd: Too many open files");
                return 0;
            }
            Rlimit nofile = nofilelimit(sh);
            if (!nofile.err && nofile.soft!=RL_INFINITY && fd>=nofile.soft
                    && r->op!=R_CLOSE && r->op!=R_MOVEIN && r->op!=R_MOVEOUT) {
                // ulimit -n below 10: bash's move up to 10 fails, then
                // the redirection with it
                Buf b(a, 64);
                print(&b, sh->name);
                print(&b, ": redirection error: cannot duplicate fd: ");
                print(&b, errtext(E_INVAL));
                putbyte(&b, '\n');
                flushbout(sh);
                fdwrite(sh, 2, finish(&b));
                Str what = r->word ? r->word->text : fmti64(a, r->dst);
                Str msg = concat3(a, what, ": ", errtext(E_INVAL));
                shellerror(sh, *a, msg);
                return 0;
            }
            Str var = r->srcvar->text;
            b32 elem = find(var, "[") >= 0;  // {arr[i]}
            if (r->op==R_CLOSE || r->op==R_MOVEIN || r->op==R_MOVEOUT) {
                Str val = elem ? expandstring(sh, a, concat3(a, "${", var, "}"))
                               : getvar(sh, a, var).value;
                fd = parsefd(val);
                if (fd < 0) {
                    Str msg = concat(a, r->srcvar->text, ": ambiguous redirect");
                    shellerror(sh, *a, msg);
                    return 0;
                }
            } else {
                setfdvar = 1;  // once the descriptor is open, as bash
                // The new descriptor outlives the command
                kept |= !sh->shopt[S_VARREDIR_CLOSE];
            }
        }
        if (fd<0 || fd>=FDMAX) {
            Str msg = concat(a, fmti64(a, fd), ": Bad file descriptor");
            shellerror(sh, *a, msg);
            return 0;
        }

        File *f = 0;
        b32   ok;
        switch (r->op) {
        case R_OUTPUT:
        case R_CLOBBER: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            if (r->op==R_OUTPUT && clobbers(sh, a, name)) return 0;
            f = openfile(sh, a, name, OF_WRITE|OF_CREATE|OF_TRUNC);
            if (!f) return 0;
            fdset(sh, fd, f, kept, a);
        } break;
        case R_APPEND: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            f = openfile(sh, a, name, OF_WRITE|OF_CREATE|OF_APPEND);
            if (!f) return 0;
            fdset(sh, fd, f, kept, a);
        } break;
        case R_INPUT: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            f = openfile(sh, a, name, OF_READ);
            if (!f) return 0;
            fdset(sh, fd, f, kept, a);
        } break;
        case R_INOUT: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            f = openfile(sh, a, name, OF_READ|OF_WRITE|OF_CREATE);
            if (!f) return 0;
            fdset(sh, fd, f, kept, a);
        } break;
        case R_ERRANDOUT:
        case R_APPENDERRANDOUT: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            if (r->op==R_ERRANDOUT && clobbers(sh, a, name)) return 0;
            i32 flags = OF_WRITE|OF_CREATE;
            flags |= r->op==R_ERRANDOUT ? OF_TRUNC : OF_APPEND;
            f = openfile(sh, a, name, flags);
            if (!f) return 0;
            fdset(sh, 1, f, kept, a);
            f->refs++;
            fdset(sh, 2, f, kept, a);
        } break;
        case R_DUPIN:
        case R_DUPOUT:
        case R_MOVEIN:
        case R_MOVEOUT:
        case R_DUPINWORD:
        case R_DUPOUTWORD:
        case R_MOVEINWORD:
        case R_MOVEOUTWORD: {
            i32 src = r->dst;
            b32 move = r->op==R_MOVEIN || r->op==R_MOVEOUT ||
                       r->op==R_MOVEINWORD || r->op==R_MOVEOUTWORD;
            b32 word = r->op==R_DUPINWORD || r->op==R_DUPOUTWORD ||
                       r->op==R_MOVEINWORD || r->op==R_MOVEOUTWORD;
            // A bad source is named by its number, but a word dup onto
            // the default descriptor is named by its text, and a word dup
            // onto another by the redirector, as bash reports them
            Str bad = {};
            if (word) {
                // A move expands with its dash, as bash: <&$unset- closes
                Word dashed = *r->word;
                if (move) dashed.text = concat(a, r->word->text, "-");
                Str w = redirtarget(sh, a, &dashed, &ok);
                if (!ok) return 0;
                if (move && w.len>1) {
                    w = cuttail(w, 1);
                }
                if (w == "-") {
                    fdset(sh, fd, 0, kept, a);
                    if (sh->coprocs) coprocfdclosed(sh, fd);
                    break;
                }
                src = parsefd(w);
                if (src < 0) {
                    if (r->op == R_DUPOUTWORD && fd == 1) {
                        // >&file: both stdout and stderr
                        if (clobbers(sh, a, w)) return 0;
                        f = openfile(sh, a, w, OF_WRITE|OF_CREATE|OF_TRUNC);
                        if (!f) return 0;
                        fdset(sh, 1, f, kept, a);
                        f->refs++;
                        fdset(sh, 2, f, kept, a);
                        break;
                    }
                    Str msg = concat(a, w, ": ambiguous redirect");
                    shellerror(sh, *a, msg);
                    return 0;
                }
                b32 dflt = fd == (r->op==R_DUPOUTWORD || r->op==R_MOVEOUTWORD);
                bad = dflt && !move ? r->word->text : fmti64(a, fd);
            } else {
                bad = fmti64(a, src);
            }
            if (src == fd) break;  // even when closed
            if (src<0 || src>=FDMAX || !t->fd[src]) {
                if (word && move) {
                    Buf b(a, 64);
                    print(&b, sh->name);
                    print(&b, ": redirection error: cannot duplicate fd: Bad file descriptor\n");
                    flushbout(sh);
                    fdwrite(sh, 2, finish(&b));
                }
                Str msg = concat(a, bad, ": Bad file descriptor");
                shellerror(sh, *a, msg);
                return 0;
            }
            b32 wasopen = !!t->fd[fd];
            f = t->fd[src];
            f->refs++;
            fdset(sh, fd, f, kept, a);
            if (move && (kept || wasopen)) {
                fdset(sh, src, 0, kept, a);
            } else if (move) {
                // bash restores the source only when the target was open
                fileunref(sh, t->fd[src]);
                t->fd[src] = 0;
            }
            if (move && sh->coprocs) coprocfdclosed(sh, src);
        } break;
        case R_CLOSE:
            if (t->fd[fd]) fdset(sh, fd, 0, kept, a);
            if (sh->coprocs) coprocfdclosed(sh, fd);
            break;
        case R_HEREDOC:
        case R_HEREDOCSTRIP:
        case R_HERESTRING: {
            Str text;
            if (r->op == R_HERESTRING) {
                text = expandstring(sh, a, r->word->text);
                text = concat(a, text, "\n");
            } else if (r->word->flags & W_QUOTED) {
                text = r->body;
            } else {
                text = expandheredoc(sh, a, r->body);
            }
            f = newfile(sh, F_MEM, OF_READ);
            f->mem    = sclone(sh->store, text);
            f->ownmem = 1;
            fdset(sh, fd, f, kept, a);
        } break;
        }
        if (setfdvar) {
            Str var = r->srcvar->text;
            Str num = fmti64(a, fd);
            b32 ok  = find(var, "[")>=0 ? assignto(sh, *a, var, num)
                                        : setvar(sh, *a, var, num);
            if (!ok) {
                Str msg = concat(a, var, ": cannot assign fd to variable");
                shellerror(sh, *a, msg);
                fdset(sh, fd, 0, kept, a);
                return 0;
            }
        }
    }
    return 1;
}


// xtrace

// Does a traced word need single quotes? These are bash's shell
// metacharacters, plus ~ and # where they would be special.
static b32 tracemetas(Str s)
{
    for (iz i = 0; i < s.len; i++) {
        switch (s[i]) {
        case ' ':  case '\t': case '\n': case '\'': case '"':  case '\\':
        case '|':  case '&':  case ';':  case '(':  case ')':  case '<':
        case '>':  case '!':  case '{':  case '}':  case '*':  case '[':
        case '?':  case ']':  case '^':  case '$':  case '`':
            return 1;
        case '~':
            if (!i || s[i-1]=='=' || s[i-1]==':') return 1;
            break;
        case '#':
            if (!i) return 1;
            break;
        }
    }
    return 0;
}

// Quote a word as xtrace shows it: $'...' when anything is not
// printable in the locale, else single quotes around metacharacters.
static Str tracequote(Shell *sh, Arena *a, Str s)
{
    if (!s.len) {
        return "''";
    } else if (shouldquote(s, sh->utf8)) {
        Buf b(a, 4*s.len+3);
        ansicquote(&b, s, sh->utf8);
        return finish(&b);
    } else if (tracemetas(s)) {
        return singlequote(a, s);
    }
    return s;
}

static Str traceprefix(Shell *sh, Arena *a)
{
    // Unset, there is no prefix at all
    Got g = getvar(sh, a, "PS4");
    if (!g.set) return {};

    // PS4 is a prompt string: decoded, then expanded as if in double
    // quotes, so <(...) and >(...) are plain text. xtrace is off while
    // it expands, so commands in it are not traced. An expansion error,
    // even a fatal one, is reported and the raw text used instead.
    Str ps4 = g.value;
    b32 xtrace = sh->opt[O_XTRACE];
    i32 status = sh->status;
    Escape e = {};
    e.up    = sh->esc;
    e.bout  = sh->bout;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    sh->opt[O_XTRACE] = 0;
    i32 kind = sh->promptkind;
    sh->promptkind = 4;  // a prompt for \! and \#
    if (!jmpset(e.jmp)) {
        ps4 = expandstring(sh, a, "${PS4@P}");
    } else {
        unwind(sh, e.undo);
    }
    sh->promptkind = kind;
    sh->esc = e.up;
    sh->opt[O_XTRACE] = xtrace;
    sh->status = status;
    if (!ps4.len) return ps4;

    // Each level of substitution, eval and source repeats its first byte
    i32 depth = sh->tracedepth + sh->evaldepth + sh->sourcedepth;
    Buf b(a, ps4.len + depth + 2);
    for (i32 i = 0; i < depth; i++) putbyte(&b, ps4[0]);
    print(&b, ps4);
    return finish(&b);
}

static void tracewords(Shell *sh, Arena a, Str *words, iz nwords)
{
    Buf b(&a, 256);
    print(&b, traceprefix(sh, &a));
    for (iz i = 0; i < nwords; i++) {
        if (i) putbyte(&b, ' ');
        print(&b, tracequote(sh, &a, words[i]));
    }
    putbyte(&b, '\n');
    fdwrite(sh, sh->tracefd ? sh->tracefd : 2, finish(&b));
}

// Trace one assignment, on its own line, before it is performed. An
// empty value is shown bare.
static void traceassign(Shell *sh, Arena a, Str name, b32 append, Str value)
{
    Buf b(&a, 256);
    print(&b, traceprefix(sh, &a));
    print(&b, name);
    print(&b, append ? Str("+=") : Str("="));
    if (value.len) print(&b, tracequote(sh, &a, value));
    putbyte(&b, '\n');
    fdwrite(sh, sh->tracefd ? sh->tracefd : 2, finish(&b));
}

static void traceline(Shell *sh, Arena a, Str text)
{
    Buf b(&a, 256);
    print(&b, traceprefix(sh, &a));
    print(&b, text);
    putbyte(&b, '\n');
    fdwrite(sh, sh->tracefd ? sh->tracefd : 2, finish(&b));
}


// Programs

// The path that command -p searches, which finds the standard utilities.
static Str stdpath(Shell *sh)
{
    switch (sh->pathmode) {
    case PM_POSIX:  break;
    case PM_MSYS:   return "/usr/bin:/bin";
    case PM_NATIVE: {
        // the shell's own directory, where w64devkit keeps its tools
        Str dir = takehead(sh->exe, sh->exe.len-lastpart(sh->exe).len);
        if constexpr (LIBMODE) dir = selfdir(sh);
        return dir.len>3 ? cuttail(dir, 1) : dir;
    }
    }
    return "/bin:/usr/bin";
}

// Remembered command paths

static u32 hashcmdbucket(Str name)
{
    u32 h = 2166136261u;  // FNV-1, as bash hashes strings
    for (iz i = 0; i < name.len; i++) {
        h *= 16777619u;
        h ^= name[i];
    }
    return h & (NHASHCMD - 1);
}

static i32 setoptionbyname(Shell *, Str name, b32 on);  // builtins.cpp
static void setposix(Shell *, b32 on);                 // builtins.cpp

static HashCmd *hashfind(Shell *sh, Str name)
{
    if (!sh->cmdhash) return 0;
    HashCmd *h = sh->cmdhash[hashcmdbucket(name)];
    for (; h; h = h->next) {
        if (h->name == name) return h;
    }
    return 0;
}

// Count a use. Entries a subshell's parent owns are left alone, since
// the count would be rolled back anyway, so loops do not grow the
// journal.
static void hashhit(Shell *sh, HashCmd *h)
{
    if ((byte *)h >= sh->store->mark) h->hits++;
}

// Remember a path for name, replacing any earlier one in place.
static void hashenter(Shell *sh, Str name, Str path, uz hits)
{
    Store *st = sh->store;
    HashCmd *h = hashfind(sh, name);
    if (h) {
        Str old = h->path;
        Str dup = sclone(st, path);
        jwrite(st, &h->path.data, (uz)dup.data);
        jwrite(st, &h->path.len, (uz)dup.len);
        jwrite(st, &h->hits, hits);
        if ((byte *)h >= st->mark) sfreestr(st, old);
        return;
    }
    if (!sh->cmdhash) {
        sh->cmdhash = (HashCmd **)salloc(st, NHASHCMD*(iz)sizeof(HashCmd *));
    }
    HashCmd **slot = sh->cmdhash + hashcmdbucket(name);
    h = (HashCmd *)salloc(st, sizeof(HashCmd));
    h->name = sclone(st, name);
    h->path = sclone(st, path);
    h->hits = hits;
    h->next = *slot;
    jset(st, slot, h);
}

static b32 hashremove(Shell *sh, Str name)
{
    if (!sh->cmdhash) return 0;
    HashCmd **slot = sh->cmdhash + hashcmdbucket(name);
    for (; *slot; slot = &(*slot)->next) {
        if ((*slot)->name == name) {
            jset(sh->store, slot, (*slot)->next);
            return 1;
        }
    }
    return 0;
}

// The path a command name runs: remembered, or found on PATH and then
// remembered when enter is set.
static Str commandpath(Shell *sh, Arena *a, Str name, b32 enter)
{
    HashCmd *h = hashfind(sh, name);
    if (h && sh->shopt[S_CHECKHASH]) {
        Stat st = pathstat(sh, *a, h->path, 1);
        if (!canexec(sh, st)) {
            hashremove(sh, name);
            h = 0;
        }
    }
    if (h) {
        hashhit(sh, h);
        return h->path;
    }
    Str path = pathlookup(sh, a, name);
    if (path.data && enter) {
        hashenter(sh, name, path, 1);
    }
    return path;
}

// What a here-document or here-string is to programs, as in bash 5.3:
// an empty one is /dev/null, one that fits a pipe (64 KiB, and the
// platform's pipe capacity) a pipe filled before the command runs, and
// a larger one a deleted temporary file, readable by its owner.
enum : iz { HEREDOCPIPE = (iz)1<<16 };

static i32 memfiletype(Shell *sh, File *f)
{
    iz cap = os_pipecap(sh->os);
    cap = cap<HEREDOCPIPE ? cap : HEREDOCPIPE;
    return !f->mem.len ? FT_CHAR : f->mem.len<=cap ? FT_PIPE : FT_FILE;
}

// The status of one of the shell's descriptors, as /dev/fd/N shows it.
static Stat fdstat(Shell *sh, i64 fd)
{
    Stat  s = {};
    File *f = fd>=0 && fd<FDMAX ? sh->fds->fd[fd] : 0;
    if (!f) {
        s.err = E_NOENT;  // bash: EBADF from fstat, reported false
    } else if (f->kind == F_OS) {
        s = os_fstat(sh->os, f->h);
    } else {
        // What a program would find there (materialize)
        s.type  = f->kind==F_MEM  ? memfiletype(sh, f)
                : f->kind==F_CLIP ? FT_CHAR
                :                   FT_PIPE;
        s.mode  = s.type==FT_FILE ? 0400 : s.type==FT_CHAR ? 0666 : 0600;
        s.size  = s.type==FT_FILE ? f->mem.len : 0;
        s.nlink = 1;
        s.uid   = sh->euid;
        s.gid   = sh->egid;
    }
    return s;
}

// The contents of a file under /proc/self, or null if there is none:
// the few Cygwin files an agent reads (no comm in msys2-runtime 3.6).
static Str procfile(Shell *sh, Arena *a, PathRes r)
{
    if (r.kind!=PK_PROC || !procself(sh, r)) return {};
    PathCtx c = pathctx(sh, a, {});
    if (r.sub == "exename") {
        Str p = toposix(&c, a, sh->exe);
        if (endswith(p, ".exe")) p = cuttail(p, 4);
        return concat(a, p, "\n");
    } else if (r.sub == "winexename") {
        return concat(a, slashify(a, sh->exe, '\\'), "\n");
    }
    return {};
}

static Stat dirstat(Shell *sh)
{
    Stat s = {};
    s.type  = FT_DIR;
    s.mode  = 0555;
    s.nlink = 1;
    s.uid   = sh->euid;
    s.gid   = sh->egid;
    return s;
}

// os_stat for a path the shell was given. The Windows modes resolve it
// through the mount table, answer for the virtual files themselves and
// decide a file's execute bits (winmode). In msys mode, as Cygwin's
// stat, "foo" finds "foo.exe".
static Stat pathstat(Shell *sh, Arena a, Str path, b32 follow)
{
    if (sh->pathmode == PM_POSIX) {
        Str full = absolute(sh, &a, path);
        return os_stat(sh->os, a, full, follow);
    }
    Stat    s = {};
    PathRes r = resolve(sh, &a, path.len ? path : Str("."));
    if (!r.ok) {
        s.err = r.kind==PK_DEV ? E_NOTDIR : E_NOENT;
        return s;
    }
    switch (r.kind) {
    case PK_FILE: {
        Str native = r.native;
        s = os_stat(sh->os, a, native, follow);
        if (s.err==E_NOENT && sh->pathmode==PM_MSYS && !hasext(native, ".exe")) {
            Str  exe = concat(&a, native, ".exe");
            Stat x   = os_stat(sh->os, a, exe, follow);
            if (!x.err && x.type==FT_FILE) {
                s      = x;
                native = exe;
            }
        }
        if (!s.err) s.mode = winmode(sh, a, native, s);
        return s;
    }
    case PK_NULL:
    case PK_TTY:
    case PK_DEVICE:
        s.type = FT_CHAR;
        s.mode = 0666;
        break;
    case PK_FD:
        return fdstat(sh, r.num);
    case PK_DEV:
        if (!r.sub.len || r.sub=="fd") return dirstat(sh);
        s.type = FT_CHAR;
        s.mode = 0666;
        break;
    case PK_PROC:
        if (!r.num && !r.sub.len) return dirstat(sh);
        if (!r.num && proctype(r.sub)==FT_DIR) return dirstat(sh);
        if (!r.num && proctype(r.sub)==FT_FILE) {
            s.type = FT_FILE;  // without contents
            s.mode = 0444;
            break;
        }
        if (!r.num || !procself(sh, r)) {
            s.err = E_NOENT;
            return s;
        }
        if (!r.sub.len || r.sub=="fd") return dirstat(sh);
        if (!procfile(sh, &a, r).data) {
            s.err = E_NOENT;
            return s;
        }
        s.type = FT_FILE;
        s.mode = 0444;
        break;
    case PK_CYGDRIVE:
        return dirstat(sh);
    }
    s.nlink = 1;
    s.uid   = sh->euid;
    s.gid   = sh->egid;
    return s;
}

// Turn a here-document an external is given into its OS form, in
// place: the File then reads through the handle, so that what the
// program leaves unread stays for whoever reads next, as with bash's
// pipe or file. (An in-memory pipe gets an OS form shared by all its
// Files instead: pipeosread.) False on failure.
static b32 materialize(Shell *sh, Arena a, File *f)
{
    Handle h = BADHANDLE;
    if (f->kind == F_MEM) {
        Str rest = cuthead(f->mem, f->pos<f->mem.len ? f->pos : f->mem.len);
        switch (memfiletype(sh, f)) {
        case FT_CHAR: {
            Opened o = os_open(sh->os, a, "/dev/null", OF_READ, 0);
            if (o.err) return 0;
            h = o.h;
        } break;
        case FT_PIPE: {
            OsPipe p = os_pipe(sh->os);
            if (p.err) return 0;
            os_write(sh->os, p.w, rest.data, rest.len);  // it fits
            os_close(sh->os, p.w);
            h = p.r;
        } break;
        case FT_FILE: {
            Opened o = os_tempfile(sh->os, a);
            if (o.err) return 0;
            if (!os_write(sh->os, o.h, f->mem.data, f->mem.len) ||
                    os_seek(sh->os, o.h, f->pos, 0) < 0) {
                os_close(sh->os, o.h);
                return 0;
            }
            h = o.h;
        } break;
        }
        // The text stays until the File goes: the store may not free
        // memory the parent of a subshell owns
    } else {
        return 0;
    }
    f->kind = F_OS;
    f->h    = h;
    return 1;
}

// Bridges
//
// A native Windows program cannot open the shell's /dev/fd/N. An
// argument naming one of its descriptors 3 and up that way (the whole
// argument, or what follows its last "=", as in --file=/dev/fd/63)
// becomes the path of a named pipe, \.\pipe\NAME, whose server ends a
// pump bridges to the descriptor: what the descriptor reads goes to a
// program reading the pipe, what a program writes into it goes to the
// descriptor. Its clients come in turn, since a program may open the
// path once only to look (a stat) before it opens it to use; the pump
// ends with the program (endbridge). Our own shell needs none: it opens
// /dev/fd/N itself, the descriptor passed in lpReserved2.
//
// An MSYS2 program gets a temporary file instead (files): it takes
// \.\pipe\NAME (not //./pipe/NAME) but opens it three times, finds an
// unbuffered pipe empty, cannot open one to write, and at times hangs
// (notes/m5a.md, step 8). A file it reads holds everything the
// descriptor reads, to its end, before the program starts; one it
// writes starts empty and goes to the descriptor once the program has
// exited (endtemp). Either is deleted when the shell closes it, after
// the program.

enum { MAXBRIDGES = 16, MAXTEMPFILE = 1<<30 };

struct Bridges {
    Task  *t[MAXBRIDGES];   // a pipe's pump, or null for a file
    Handle h[MAXBRIDGES];   // a temporary file
    i32    fd[MAXBRIDGES];  // the descriptor each bridges
    b32    files;           // temporary files, for an MSYS2 program
    i32    n;
};

// A temporary file in place of descriptor fd: its native path, or a
// null string. Filled from the descriptor if the program reads it, up
// to MAXTEMPFILE (an endless procsub, such as yes, gives up there).
static Str starttemp(Shell *sh, Arena *a, i32 fd, Bridges *br)
{
    File *f = sh->fds->fd[fd];
    if (!sh->paths || !sh->paths->temp.len) return {};
    NamedTemp t = os_namedtemp(sh->os, a, sh->paths->temp);
    if (t.err) return {};
    if (f->flags & OF_READ) {
        Arena tmp   = *a;
        iz    cap   = 1<<16;
        u8   *buf   = allocbytes(&tmp, cap);
        iz    total = 0;
        for (;;) {
            iz n = fileread(sh, f, buf, cap);
            if (n <= 0) break;
            total += n;
            if (total>MAXTEMPFILE || !os_write(sh->os, t.h, buf, n)) {
                os_close(sh->os, t.h);
                return {};
            }
        }
        os_seek(sh->os, t.h, 0, 0);
    }
    br->t[br->n]    = 0;
    br->h[br->n]    = t.h;
    br->fd[br->n++] = fd;
    return t.path;
}

// After the program: what it wrote into a temporary file for a
// descriptor goes to that descriptor (if feed), and the file goes.
static void endtemp(Shell *sh, Arena a, Bridges *br, i32 i, b32 feed)
{
    Sched *s = sh->sched;
    File  *f = sh->fds->fd[br->fd[i]];
    if (feed && f && !(f->flags&OF_READ) && os_seek(sh->os, br->h[i], 0, 0)==0) {
        iz  cap   = 1<<16;
        u8 *buf   = allocbytes(&a, cap);
        b32 wrote = 0;
        i32 err   = 0;
        while (!err) {
            iz n = os_read(sh->os, br->h[i], buf, cap);
            if (n <= 0) break;
            wrote = 1;
            switch (f->kind) {
            case F_OS:      err = taskwrite(s, f->h, buf, n);    break;
            case F_PIPE:    err = pipewrite(s, f->pipe, buf, n); break;
            case F_CAPTURE: print(f->cap, {buf, n});             break;
            case F_CLIP:
                err = clipappend(s, f->clip, {buf, n}) ? 0 : E_NOMEM;
                break;
            default:        err = E_BADF;
            }
        }
        if (f->kind==F_CLIP && wrote && !err) clipsync(s, f->clip);
    }
    os_close(sh->os, br->h[i]);
}

// Bridge descriptor fd: the path for the program, or a null string if
// it cannot be bridged.
static Str startbridge(Shell *sh, Arena *a, i32 fd, Bridges *br)
{
    Sched *s = sh->sched;
    File  *f = fd<FDMAX ? sh->fds->fd[fd] : 0;
    if (!f || br->n==MAXBRIDGES) return {};
    if (br->files) return starttemp(sh, a, fd, br);
    b32    out  = f->flags & OF_READ;  // the program reads
    Handle peer = BADHANDLE;
    Pipe  *pipe = 0;
    if (out && f->kind==F_MEM && !materialize(sh, *a, f)) return {};
    if (f->kind == F_OS) {
        peer = os_dup(sh->os, f->h);
        if (peer == BADHANDLE) return {};
    } else if (f->kind==F_PIPE && out) {
        Handle h = pipeosread(s, f->pipe);
        peer = h==BADHANDLE ? BADHANDLE : os_dup(sh->os, h);
        if (peer == BADHANDLE) return {};
    } else if (f->kind == F_PIPE) {
        pipe = f->pipe;
    } else {
        return {};  // a capture
    }
    // Instances listen ahead from the start (see nextclient)
    i32       how = out ? NP_OUT|NP_NOBUF : 0;
    NamedPipe np  = os_namedpipe(sh->os, a, {}, how);
    Handle    spare[NSPARE];
    i32       nspare = 0;
    i32       err    = np.err ? np.err : 0;
    if (!err && np.name.len>countof(((Pump *)0)->name)) err = E_NAMETOOLONG;
    for (; !err && nspare<NSPARE; nspare++) {
        NamedPipe more = os_namedpipe(sh->os, a, np.name, how);
        err = more.err;
        spare[nspare] = more.h;
    }
    if (err) {
        if (!np.err) os_close(sh->os, np.h);
        for (i32 i = 0; i < nspare-1; i++) os_close(sh->os, spare[i]);
        if (peer != BADHANDLE) os_close(sh->os, peer);
        return {};
    }
    Pump *p = newpump(s);
    p->kind    = out ? PUMP_SERVE : PUMP_TAKE;
    p->h       = np.h;
    p->peer    = peer;
    p->pipe    = pipe;
    for (i32 i = 0; i < NSPARE; i++) {
        p->spare[i] = spare[i];
    }
    p->how     = how;
    p->namelen = np.name.len;
    copybytes(p->name, np.name.data, np.name.len);
    br->fd[br->n]  = fd;
    br->t[br->n++] = spawnpump(s, p);
    return slashify(a, np.name, '\\');
}

// The /dev/fd/N an argument names (whole, or after its last "="), or -1.
static i32 argdevfd(Str arg)
{
    iz at = arg.len;
    for (; at>0 && arg[at-1]!='='; at--) {}
    Str tail = cuthead(arg, at);
    return startswith(tail, "/dev/fd/") ? devfdnum(tail) : -1;
}

// Would any argument be bridged?
static b32 needsbridge(Shell *sh, Str *argv, iz argc)
{
    for (iz i = 1; i < argc; i++) {
        i32 fd = argdevfd(argv[i]);
        if (fd>=3 && fd<FDMAX && sh->fds->fd[fd]) return 1;
    }
    return 0;
}

// argv with its /dev/fd/N arguments bridged, or argv itself if none.
static Str *bridgeargs(Shell *sh, Arena *a, Str *argv, iz argc, Bridges *br)
{
    Str *r = argv;
    i32  fds[MAXBRIDGES];
    Str  paths[MAXBRIDGES];
    i32  n = 0;
    for (iz i = 1; i < argc; i++) {
        Str arg = argv[i];
        iz  at  = arg.len;
        for (; at>0 && arg[at-1]!='='; at--) {}
        i32 fd = argdevfd(arg);
        if (fd<3 || fd>=FDMAX) continue;
        Str path = {};
        for (i32 k = 0; k < n; k++) {
            path = fds[k]==fd ? paths[k] : path;
        }
        if (!path.data && n<MAXBRIDGES) {
            path = startbridge(sh, a, fd, br);
            fds[n]     = fd;
            paths[n++] = path;
        }
        if (!path.data) continue;
        if (r == argv) {
            r = alloc<Str>(a, argc);
            for (iz k = 0; k < argc; k++) r[k] = argv[k];
        }
        r[i] = concat(a, takehead(arg, at), path);
    }
    return r;
}

// Run a program and wait for it. Returns its status, or the negated
// error that kept it from starting. Shell objects on its descriptors
// become OS pipes: text and in-memory pipes are fed by pump tasks, and
// a capture is read here until end of file.
// A program's temporary environment goes once it is spawned, as bash's
// parent drops it right after the fork: its setlocale warnings come
// before the program's output (see runsimple1).
static void undotemps(Shell *sh, Str *argv)
{
    if (sh->nundo && sh->undoargv==argv) {
        for (iz i = sh->nundo-1; i >= 0; i--) poptemp(sh, sh->undonames[i]);
        sh->nundo = 0;
    }
}

static i32 runprogram(Shell *sh, Arena a, Str path, Str *argv, iz argc,
                      Slice<Str> env)
{
    Sched *s = sh->sched;
    Fdtab *t = sh->fds;
    b32 execing = sh->execing;  // for this program only
    sh->execing = 0;
    Target  tg = {};
    Bridges br = {};
    if (sh->win32) {
        // What starts, and how: decided here, before descriptors move
        tg = wintarget(sh, &a, path, argv, argc, env);
        if (!tg.err && !tg.self && sh->spawnto && needsbridge(sh, argv, argc)) {
            // A direct pipeline stage's descriptors are not the
            // shell's to bridge: it runs as a stage of shell code
            sh->spawnto->retry = 1;
            return 0;
        }
        if (!tg.err && !tg.self && !sh->spawnto) {
            br.files = tg.cygwin;
            argv = bridgeargs(sh, &a, argv, argc, &br);
            if (br.n) tg = wintarget(sh, &a, path, argv, argc, env);
        }
        if (tg.err) {
            for (i32 i = 0; i < br.n; i++) {
                if (br.t[i]) {
                    endbridge(s, br.t[i]);
                } else {
                    endtemp(sh, a, &br, i, 0);
                }
            }
        }
        if (tg.interp.data) {
            Buf b(&a, 256);
            print(&b, path);
            print(&b, ": ");
            print(&b, tg.interp);
            print(&b, ": bad interpreter: ");
            print(&b, errtext(tg.err));
            syserror(sh, a, finish(&b));
            return -E_INTERP;
        } else if (tg.err) {
            return -tg.err;
        }
    }
    SpawnFd *fds = alloc<SpawnFd>(&a, t->top ? t->top : 1);
    iz nfds = 0;
    Handle *closeafter = alloc<Handle>(&a, t->top ? t->top : 1);
    i32     nclose = 0;
    Task   *pumps[16];
    i32     npumps = 0;
    File   *capfile = 0;
    Handle  capread = BADHANDLE;
    for (i32 fd = 0; fd < t->top; fd++) {
        File *f = t->fd[fd];
        if (!f) continue;
        b32 cloexec = 0;
        for (Coproc *c = sh->coprocs; c; c = c->next) {
            cloexec |= fd==c->rfd || fd==c->wfd;
        }
        if (cloexec) continue;  // a coprocess's: bash makes them close-on-exec
        // On Windows a descriptor above 2 reaches only a CRT program,
        // and not one it reaches through a bridge, whose pump reads it
        b32 unused = sh->win32 && fd>2 && !tg.crtslot;
        for (i32 i = 0; i < br.n; i++) {
            unused |= br.fd[i] == fd;
        }
        if (unused) continue;
        SpawnFd sf = {fd, BADHANDLE};
        // A File already given to an earlier descriptor shares its handle
        b32 shared = 0;
        for (iz k = 0; k<nfds && f->kind!=F_OS; k++) {
            if (t->fd[fds[k].fd] == f) {
                sf.h   = fds[k].h;
                shared = 1;
            }
        }
        if (shared) {
            fds[nfds++] = sf;
            continue;
        }
        switch (f->kind) {
        case F_OS:
            sf.h = f->h;
            break;
        case F_MEM:
            if (!materialize(sh, a, f)) continue;
            sf.h = f->h;
            break;
        case F_CAPTURE: {
            if (sh->spawnto) {
                // Not waited for here: a pump fills the capture
                sf.h = capturewriter(sh, f, 0);
                if (sf.h == BADHANDLE) continue;
                closeafter[nclose++] = sf.h;
                break;
            }
            OsPipe p = os_pipe(sh->os);
            if (p.err) continue;
            capfile = f;
            capread = p.r;
            sf.h = p.w;
            closeafter[nclose++] = p.w;
        } break;
        case F_PIPE: {
            if (f->flags & OF_READ) {
                sf.h = pipeosread(s, f->pipe);
                if (sf.h == BADHANDLE) continue;
                break;
            }
            // Output is pumped while the program runs, and joined after
            if (npumps == countof(pumps)) continue;
            OsPipe p = os_pipe(sh->os);
            if (p.err) continue;
            Pump *pp = newpump(s);  // may outlive this call
            pp->pipe = f->pipe;
            pp->kind = PUMP_FROMHANDLE;
            pp->h    = p.r;
            sf.h     = p.w;
            closeafter[nclose++] = sf.h;
            pumps[npumps++] = spawnpump(s, pp);
        } break;
        case F_CLIP: {
            // Appended by a pump, which sets the clipboard at the end,
            // joined after
            if (npumps == countof(pumps)) continue;
            ClipWriter w = clipwriter(s, f->clip);
            if (w.h == BADHANDLE) continue;
            sf.h = w.h;
            closeafter[nclose++] = w.h;
            pumps[npumps++] = w.t;
        } break;
        }
        fds[nfds++] = sf;
    }

    Spawn sp = {};
    sp.rlimits = sh->rlmod ? sh->rl : 0;
    sp.path  = absolute(sh, &a, path);
    sp.argv  = argv;
    sp.argc  = argc;
    sp.env   = env.data;
    sp.nenv  = env.len;
    sp.cwd   = spawncwd(sh, &a);
    sp.fds   = fds;
    sp.nfds  = nfds;
    sp.sigign = ignoredsigs(sh);
    if (sh->win32) {
        sp.path    = tg.path;
        sp.cmdline = tg.cmdline;
        sp.env     = tg.env.data;
        sp.nenv    = tg.env.len;
        sp.crtslot = tg.crtslot;
    }
    // Under job control a background job's program joins the job's
    // group (spawngrp), and a foreground program leads a group of its
    // own that holds the terminal while it runs
    b32 group = 0;
    b32 stops = 0;  // a grouped pipeline's last: its stop is reported
    if (sh->spawngrp) {
        sp.pgid = sh->spawngrp;
        sp.fg   = sh->spawnfg && sh->jctty!=BADHANDLE;
        sp.tty  = sh->jctty;
        stops   = sh->pipetail && !sh->spawnto;
        sh->pipetail = 0;
    } else if (!execing && fgjobctl(sh) && !sh->spawnto && !capfile && !npumps && !br.n) {
        group   = 1;
        sp.pgid = -1;
        sp.fg   = sh->jctty != BADHANDLE;
        sp.tty  = sh->jctty;
    }
    // The last command of a background job in a process of its own,
    // which bash would exec in the job's place
    b32 jobtail = !execing && sh->attail && sh->subshell==sh->procbase &&
                  sh->procbase && sh->taillevel==sh->procbase+1 && s->cur==&s->main;
    if ((execing || jobtail) && sh->subshell==sh->procbase && !sh->capdepth &&
            !capfile && !sh->spawnto && !br.n) {
        // exec at the process's outermost level: become the program (it
        // keeps the shell's pid). While other tasks run, a copy of the
        // process forked first keeps them, as bash's procsubs and jobs
        // are processes of their own already. Else (no exec, no fork,
        // or a job's tail that could not) start it as a child.
        b32 alone = s->ntasks==1 && !npumps;
        i32 err   = alone ? os_exec(sh->os, a, &sp) : os_forkexec(sh->os, a, &sp);
        if (err!=E_NOSYS && !(err && jobtail)) {
            for (i32 i = 0; i < nclose; i++) os_close(sh->os, closeafter[i]);
            if (!err) {
                // The copy: its tasks finish, joined as the shell exits,
                // and it rekeys its /dev/urandom stream on next use
                s->chacha = {};
                // A pump feeding the program's pipe runs to its end
                // rather than with the shell's references (pipeclose),
                // and only the program holds that pipe's read end.
                Fdtab *t = sh->fds;
                for (i32 fd = 0; fd < t->top; fd++) {
                    File *f = t->fd[fd];
                    if (f && f->kind==F_PIPE && f->pipe->pump) {
                        Pipe *p = f->pipe;
                        p->pump->sib = s->orphans;
                        s->orphans   = p->pump;
                        p->pump = 0;
                        os_close(sh->os, p->osr);
                        p->osr = BADHANDLE;
                    }
                }
                for (i32 i = 0; i < npumps; i++) {
                    pumps[i]->sib = s->orphans;
                    s->orphans    = pumps[i];
                }
                sh->execed = 1;  // no EXIT trap
                return 0;
            }
            return -err;
        }
    }
    // Killed while the program runs (or any other escape): it runs on,
    // so its output pumps do too, orphaned for the Sched to join
    Escape e = {};
    if (npumps || br.n) {
        e.up    = sh->esc;
        e.undo  = sh->undo;
        e.frame = sh->frame;
        e.bout  = sh->bout;
        sh->esc = &e;
        if (jmpset(e.jmp)) {
            sh->esc = e.up;
            for (i32 i = 0; i < npumps; i++) {
                pumps[i]->sib = s->orphans;
                s->orphans    = pumps[i];
            }
            for (i32 i = 0; i < br.n; i++) {
                if (!br.t[i]) {
                    endtemp(sh, a, &br, i, 0);  // what the program has stays
                    continue;
                }
                // The program runs on, but one that has not opened its
                // pipe yet is not waited for
                if (!((Pump *)br.t[i]->arg)->connected) cancelbridge(s, br.t[i]);
                br.t[i]->sib = s->orphans;
                s->orphans   = br.t[i];
            }
            escape(sh, e.why);
        }
    }
    clipsettle(s);
    Spawned r = os_spawn(sh->os, a, &sp);
    if (stops) closegate(sh);  // the group is whole: its stages may run
    for (i32 i = 0; i < nclose; i++) {
        os_close(sh->os, closeafter[i]);
    }
    if (!sh->spawnto) undotemps(sh, argv);
    if (sh->spawnto) {
        // A pipeline stage started directly: its waiter is runpipeline
        assert(!npumps && !capfile);
        sh->spawnto->proc = r.err ? BADHANDLE : r.proc;
        sh->spawnto->pid  = r.pid;
        return r.err ? -r.err : 0;
    }
    if (capfile) {
        iz  cap = (iz)1<<12;
        u8 *buf = allocbytes(&a, cap);
        for (;;) {
            iz n = taskread(s, capread, buf, cap);
            if (n <= 0) break;
            // tracing: for the program, which is in this task's group
            tracewrite(s, &((Capture *)capfile->cap)->wtrace);
            print(capfile->cap, {buf, n});
        }
        os_close(sh->os, capread);
    }
    takeossigs(sh);  // those from before the wait are this level's own
    sh->sig->fgwait  = !r.err;
    sh->sig->execpid = sh->attail && !r.err ? r.pid : 0;
    i32 status = r.err ? -r.err : group||stops ? taskwaitstop(s, r.proc) : taskwaitproc(s, r.proc);
    if (stops && !r.err && status>=0 && (status & WSTOP)) {
        s->stopproc = r.proc;
        s->stoppid  = r.pid;
        s->stopsig  = status & 255;
        status = 128 + (status & 255);
    }
    sh->sig->execpid = 0;
    sh->childsig = 0;
    if (group && !r.err) {
        if (sp.fg) {
            os_tcpgrp(sh->os, sh->jctty, sh->shellpgid);
            if ((status & WSIGNALED) && (status & 255)==128+2) {
                // The terminal's INT reached only the program: in a
                // loop (or untrapped, interactively) the shell acts as
                // if it had it too, as bash
                intrjob(sh);
            }
        }
        if (status>=0 && (status & WSTOP)) {
            status = fgstopped(sh, a, r.proc, r.pid, status & 255);
        }
    }
    if (!r.err) {
        // A signal the program missed is the level's own, as its
        // forked bash child would have had it
        b32 missed = status & WMISSED;
        status &= ~WMISSED;
        b32 died = status & WSIGNALED;
        u64 sigs = missed ? ~(u64)0 : died ? sigbit((status & 255) - 128) : 0;
        waitdone(sh, sigs, status!=0);
    }
    if (s->cur->intr) {
        sigtaken(sh);  // killed while waiting: the program runs on
    }
    if (!r.err) checkwinsize(sh);
    if (status>=0 && (status & WNOLIB)) {
        // As Cygwin reports a native program that could not load a DLL
        // (recorded from Git Bash: the image's POSIX path, .exe kept)
        status &= ~WNOLIB;
        Str image = tg.path;
        if (sh->pathmode == PM_MSYS) {
            PathCtx c = pathctx(sh, &a, {});
            Str     p = toposix(&c, &a, image);
            image = p.data ? p : image;
        }
        Str dll = missingdll(sh, &a, tg.path, env);
        Buf b(&a, 256);
        print(&b, image.len ? image : path);
        print(&b, ": error while loading shared libraries: ");
        print(&b, dll.data ? dll : Str("?"));
        print(&b, ": cannot open shared object file: No such file or directory\n");
        flushbout(sh);
        fdwrite(sh, 2, finish(&b));
    }
    if (status>=0 && (status & WSIGNALED)) {
        sh->childsig = (status & 255) - 128;
        sh->childpid = r.pid;
        status &= 255;
    }
    // What the program wrote is passed on before anything the shell
    // writes next
    if (npumps || br.n) sh->esc = e.up;
    for (i32 i = 0; i < br.n; i++) {
        if (br.t[i]) {
            endbridge(s, br.t[i]);
        } else {
            endtemp(sh, a, &br, i, 1);
        }
    }
    for (i32 i = 0; i < npumps; i++) {
        joinpump(s, pumps[i]);
    }
    for (i32 fd = 0; npumps && fd<sh->fds->top; fd++) {
        File *f = sh->fds->fd[fd];
        if (f && f->kind==F_CLIP && f->clip->err) {
            // Where the program's own write would have failed
            Str msg = concat(&a, "/dev/clipboard: ", errtext(f->clip->err));
            f->clip->err = 0;
            shellerror(sh, a, msg);
        }
    }
    if (sh->jobs || sh->coprocs) {
        // as bash after each foreground job: drop the jobs already
        // reported, and a coprocess that ended
        polljobs(sh);
        cleanupjobs(sh);
    }
    return status;
}

// The library with no self runs a #! script for sh in-process: the
// options on its #! line apply as they would to "sh -e script".
static void shebangoptions(Shell *sh, Str src)
{
    if (src.len<2 || src[0]!='#' || src[1]!='!') return;
    iz end = 2;
    for (; end<src.len && src[end]!='\n' && src[end]!='\r'; end++) {}
    Str o = parseshebang(span(src.data+2, src.data+end)).arg;
    if (o.len<2 || (o[0]!='-' && o[0]!='+')) return;
    for (iz i = 1; i < o.len; i++) {
        for (i32 k = 0; k < O_COUNT; k++) {
            if (setoptions[k].letter == o[i]) sh->opt[k] = o[0]=='-';
        }
    }
}

static b32 isshellscript(Shell *sh, Arena a, Str path)
{
    Str full = absolute(sh, &a, path);
    Opened o = os_open(sh->os, a, full, OF_READ, 0);
    if (o.err) return 0;
    u8 buf[80];
    iz n = os_read(sh->os, o.h, buf, countof(buf));
    os_close(sh->os, o.h);
    for (iz i = 0; i < n; i++) {
        if (!buf[i]) return 0;  // binary
    }
    return 1;
}

// The interpreter a file's #! line names, as bash reports it when the
// kernel refuses the file: from a 128-byte sample less its last byte,
// with a trailing CR shown as ^M. Null without a #! line.
static Str scriptinterp(Shell *sh, Arena *a, Str path)
{
    Str full = absolute(sh, a, path);
    Opened o = os_open(sh->os, *a, full, OF_READ, 0);
    if (o.err) return {};
    u8 buf[128];
    iz n = os_read(sh->os, o.h, buf, countof(buf));
    os_close(sh->os, o.h);
    if (n<3 || buf[0]!='#' || buf[1]!='!') return {};
    n--;
    iz i = 2;
    for (; i<n && (buf[i]==' ' || buf[i]=='\t'); i++) {}
    iz beg = i;
    for (; i<n && buf[i] && buf[i]!=' ' && buf[i]!='\t' && buf[i]!='\n'; i++) {}
    Str r = clone(a, span(buf+beg, buf+i));
    if (r.len && r[r.len-1]=='\r') r = concat(a, takehead(r, r.len-1), "^M");
    return r;
}

// Does a script look like a binary? As bash: a NUL in the first line,
// within its first 80 bytes.
static b32 binaryfile(Str s)
{
    for (iz i = 0; i<s.len && i<80; i++) {
        if (s[i] == '\n') return 0;
        if (s[i] == 0) return 1;
    }
    return 0;
}

static Str readwholefile(Shell *sh, Arena *a, Str path, i32 *err)
{
    i32   fd = devfdnum(path);
    File *f  = fd>=0 && fd<FDMAX ? sh->fds->fd[fd] : 0;
    if (f) {
        // One of the shell's descriptors, such as a procsub's, read to
        // its end. Others are the platform's (a nested shell's script).
        *err = f->flags & OF_READ ? 0 : E_BADF;
        if (*err) return {};
        Buf b(a, 4096);
        for (;;) {
            reserve(&b, 4096);
            iz n = fileread(sh, f, b.data+b.len, b.cap-b.len);
            if (n <= 0) break;
            b.len += n;
        }
        return finish(&b);
    }
    if (isclipboard(sh, *a, path)) {
        clipsettle(sh->sched);
        ClipText c = os_clipget(sh->os, a);
        *err = c.err;
        return c.text;
    }
    Str full = absolute(sh, a, path);
    Opened o = os_open(sh->os, *a, full, OF_READ, 0);
    *err = o.err;
    if (o.err) return {};
    Buf b(a, 4096);
    for (;;) {
        reserve(&b, 4096);
        iz n = os_read(sh->os, o.h, b.data+b.len, b.cap-b.len);
        if (n <= 0) break;
        b.len += n;
    }
    os_close(sh->os, o.h);
    return finish(&b);
}

// An exported variable's value, null when it is not in the environment.
static Str envvalue(Shell *sh, Arena *a, Str name)
{
    Value *v = lookup(sh, name);
    if (!v || !(v->attrs & V_EXPORT)) return {};
    Got g = getvar(sh, a, name);
    return !g.set ? Str{} : g.value.data ? g.value : Str("");
}

// The zone local time is in, as the C library bash uses finds it: TZ
// when it is in the environment, else the system's (os_localtz). Like
// glibc, a TZ naming a zoneinfo file (absolute, or under TZDIR or
// /usr/share/zoneinfo) uses the file, otherwise it is a POSIX TZ
// string. An msys shell reads zoneinfo through its mounts and, like
// Cygwin, takes a TZ that does not parse as GMT; a native one, like
// busybox-w32, has no zoneinfo.
static PfZone *shellzone(Shell *sh, Arena *a)
{
    Str tz     = envvalue(sh, a, "TZ");
    b32 system = !tz.data;
    if (system) tz = os_localtz(sh->os, a);
    PfZone *z = alloc<PfZone>(a);
    Str name = startswith(tz, ":") ? cuthead(tz, 1) : tz;
    if (name.len && sh->pathmode!=PM_NATIVE) {
        Str path = name;
        if (name[0] != '/') {
            Str dir = sh->pathmode==PM_POSIX ? envvalue(sh, a, "TZDIR") : Str{};
            dir  = dir.len ? dir : Str("/usr/share/zoneinfo");
            path = concat3(a, dir, "/", name);
        }
        Stat st = pathstat(sh, *a, path, 1);
        if (!st.err && st.type==FT_FILE && st.size<=((iz)1<<20)) {
            Arena tmp = *a;
            i32   err = 0;
            Str   s   = readwholefile(sh, &tmp, path, &err);
            PfZone f  = err ? PfZone{} : parsetzif(&tmp, s);
            if (f.ok) {
                *a = tmp;  // keep the file's tables
                *z = f;
                return z;
            }
        }
    }
    *z = parsezone(tz);
    if (system && !z->ok) {
        *z = parsezone("UTC0");  // an unreadable /etc/localtime
    } else if (sh->pathmode==PM_MSYS && !z->ok) {
        *z = parsezone("GMT0");
    }
    return z;
}

// Execute an external command, by path or by searching PATH, unless
// the caller has already found it.
static i32 runexternal(Shell *sh, Arena a, Str *argv, iz argc, Slice<Str> env, Str found = {})
{
    Spawnto *direct = sh->spawnto;  // for runprogram only
    sh->spawnto = 0;
    Str name = argv[0];
    Str path = found;
    b32 slash = haspath(sh, name);
    if (sh->restricted && slash) {
        Str msg = concat3(&a, name, Str(": restricted: "), "cannot specify `/' in command names");
        shellerror(sh, a, msg);
        return 1;
    }
    if (found.data) {
        slash = haspath(sh, found);
    } else if (slash) {
        path = name;
    } else {
        path = commandpath(sh, &a, name, 1);
        Func *handler = path.data ? 0 : findfunc(sh, "command_not_found_handle");
        if (handler && !sh->inhandler) {
            // As bash, in a subshell, given the command and arguments
            SubJob j = {};
            j.kind = SUB_CALL;
            j.fn   = handler;
            j.argc = argc + 1;
            j.argv = alloc<Str>(&a, argc+1);
            j.argv[0] = "command_not_found_handle";
            for (iz i = 0; i < argc; i++) j.argv[i+1] = argv[i];
            sh->inhandler = 1;
            i32 status = subshell(sh, &j, a, 0);
            sh->inhandler = 0;
            chldreaped(sh, 1);
            return status;
        }
        if (!path.data && !getvar(sh, &a, "PATH").set) {
            // bash tries the bare name, in the working directory
            undotemps(sh, argv);
            Str msg = concat(&a, name, ": No such file or directory");
            shellerror(sh, a, msg);
            return 127;
        } else if (!path.data) {
            Str shown = name;
            if (shouldquote(name, sh->utf8)) {
                Buf b(&a, 4*name.len+3);
                ansicquote(&b, name, sh->utf8);
                shown = finish(&b);
            }
            undotemps(sh, argv);
            Str msg = concat(&a, shown, ": command not found");
            shellerror(sh, a, msg);
            return 127;
        }
    }
    sh->spawnto = direct;
    i32 st = runprogram(sh, a, path, argv, argc, env);
    sh->spawnto = 0;
    if (st >= 0) return st;
    i32 err = -st;
    if (err == E_INTERP) {
        return 126;  // reported
    }
    if (err==E_NOEXEC && sh->pathmode==PM_NATIVE &&
        !canexec(sh, pathstat(sh, a, path, 1))) {
        // Windows ran what native mode does not count as executable (a
        // text file without #! or extension): as busybox-w32, refused
        // rather than run as shell code (msys mode runs it, as Git Bash)
        err = E_ACCES;
    }
    if (err==E_NOEXEC && direct && isshellscript(sh, a, path)) {
        direct->retry = 1;  // run as shell code, concurrently
        return 0;
    }
    if (err == E_NOEXEC && isshellscript(sh, a, path)) {
        // No #! line: bash runs it as a shell script in a subshell
        Str src = readwholefile(sh, &a, path, &err);
        Shell *saved = alloc<Shell>(&a);  // large: not on the stack
        *saved = *sh;
        Fdsave fds = savefds(sh, &a);
        Watermark w = storeenter(sh->store);
        Escape e = {};
        e.up = sh->esc;
        e.bout = sh->bout;
        e.undo = sh->undo;
        sh->esc = &e;
        sh->args.v = argv + 1;
        sh->args.n = argc - 1;
        sh->name   = name;
        sh->argv0  = {};
        sh->frame  = 0;
        sh->funcdepth = 0;
        sh->funcs  = 0;
        sh->srcname     = {};
        sh->evaldepth   = 0;
        sh->sourcedepth = 0;
        sh->psubs       = 0;
        sh->aliases     = 0;
        sh->cflag       = 0;
        sh->restricted  = 0;  // as bash, the script runs unrestricted
        sh->noerrexit   = 0;  // a new shell: no condition is pending
        defaultoptions(sh, 1);
        if constexpr (LIBMODE) shebangoptions(sh, src);
        sh->capdepth++;
        // As a new shell: what was ignored is ignored on entry, caught
        // signals are reset, and only its own traps are listed
        sh->hardign  |= ignoredsigs(sh);
        sh->killsig   = 0;
        sh->errexited = 0;
        sh->catchfatal = 1;
        subshelltraps(sh);
        sh->shown   = 0;
        sh->bashpid = newvpid(sh);
        Sigproc sig;
        sigenter(sh, &sig, sh->bashpid, sh->sched->cur);
        if (!jmpset(e.jmp)) {
            runsource(sh, src, name, *scratchat(sh, sh->capdepth), 1);
        }
        if (!sh->execed && !sh->killsig && sh->traps) {
            if (sh->errexited) unwind(sh, saved->undo);
            runexittrap(sh, *scratchat(sh, sh->capdepth));
        }
        i32 status = sh->status;
        i32 killed = sh->killsig;
        i64 pid    = sh->bashpid;
        sigleave(sh, &sig);
        unwind(sh, saved->undo);
        restorefds(sh, fds);
        joinpsubs(sh);
        storeleave(sh->store, &w);
        for (i32 i = 0; i < NPOOL; i++) saved->pool[i] = sh->pool[i];
        *sh = *saved;
        if (killed) {
            sh->childsig = killed;
            sh->childpid = pid;
        }
        if (sh->sig->waitsig) waitdone(sh, killed ? sigbit(killed) : 0, 0);
        return status;
    }
    Stat s = pathstat(sh, a, path, 1);
    if (!s.err && s.type==FT_DIR) {
        err = E_ISDIR;
    } else if (canexec(sh, s) && err!=E_2BIG && err!=E_NOMEM) {
        // The kernel refused an executable file: say why, as bash
        Str interp = scriptinterp(sh, &a, path);
        if (interp.data) {
            Buf b(&a, 256);
            print(&b, path);
            print(&b, ": ");
            print(&b, interp);
            print(&b, ": bad interpreter: ");
            print(&b, errtext(err));
            syserror(sh, a, finish(&b));
            return 126;
        } else if (err == E_NOENT) {
            Str msg = concat(&a, path, ": cannot execute: required file not found");
            shellerror(sh, a, msg);
            return 127;
        }
    }
    if (err == E_NOENT) {
        // a stale remembered path is reported as such
        Str msg = concat(&a, path, ": No such file or directory");
        shellerror(sh, a, msg);
        return 127;
    }
    Str msg = concat3(&a, path, Str(": "), errtext(err));
    shellerror(sh, a, msg);
    return err==E_NOENT ? 127 : 126;
}


// Simple commands

static void checkerrexit(Shell *sh)
{
    if (sh->status && !sh->noerrexit && !sh->exiting && sh->traps) {
        runerrtrap(sh);
    }
    if (sh->status && sh->opt[O_ERREXIT] && !sh->noerrexit && !sh->exiting) {
        sh->exiting   = 1;
        sh->errexited = 1;
        escape(sh, ESC_EXIT);
    }
}

// Recursion deep enough to exhaust the stack: bash would crash with
// SIGSEGV, so this shell (or subshell) dies by it, no EXIT trap run,
// and its parent reports "Segmentation fault".
static void checkstack(Shell *sh)
{
    byte here = 0;
    if (sh->stackmax && sh->stacktop-&here > sh->stackmax) {
        sh->status  = 139;
        sh->killsig = 11;
        sh->exiting = 1;
        escape(sh, ESC_EXIT);
    }
}

// Leave a call's frame: pop its locals and restore the options that
// "local -" saved.
static void leaveframe(Shell *sh, Frame *f)
{
    poplocals(sh, f);
    if (f->opts) {
        b32 eof = sh->opt[O_IGNOREEOF];
        b32 posix = sh->opt[O_POSIX];
        for (i32 i = 0; i < O_COUNT; i++) sh->opt[i] = f->opts[i];
        if (posix != sh->opt[O_POSIX]) {
            sh->opt[O_POSIX] = posix;
            setposix(sh, !posix);  // and POSIXLY_CORRECT
        }
        if (eof != sh->opt[O_IGNOREEOF]) {
            setoptionbyname(sh, "ignoreeof", sh->opt[O_IGNOREEOF]);  // and IGNOREEOF
        }
        sfree(sh->store, f->opts, O_COUNT*(iz)sizeof(b32));
    }
    sh->funcdepth--;
    sh->frame = f->up;
}

[[gnu::noinline]]
static void checkfuncnest(Shell *sh, Str name, Arena scratch)
{
    Got nest = getvar(sh, &scratch, "FUNCNEST");
    i64 limit = 0;
    if (nest.set && parsei64(nest.value, &limit) && limit>0 && sh->funcdepth>=limit) {
        Str msg = concat3(&scratch, name,
                   Str(": maximum function nesting level exceeded ("),
                   concat(&scratch, nest.value, ")"));
        shellerror(sh, scratch, msg);
        sh->status = 1;
        escape(sh, ESC_ABORT);
    }
}

static i32 callfunction(Shell *sh, Func *fn, Str *argv, iz argc, Arena scratch)
{
    safepoint(sh->sched);
    checkstack(sh);
    checkfuncnest(sh, argv[0], scratch);
    Frame f = {};
    f.up   = sh->frame;
    f.name = argv[0];
    f.line = sh->line;
    f.file = fn->code->file;
    f.src  = fn->code->src;
    f.args.v    = argv + 1;
    f.args.n    = argc - 1;
    f.argpushed = sh->shopt[S_EXTDEBUG];
    if (sh->traps) {
        for (i32 k = 0; k < 3; k++) f.trapat[k] = sh->traps[TRAP_DEBUG+k].data;
    }
    Args saved = sh->args;
    sh->args.v = argv + 1;
    sh->args.n = argc - 1;
    sh->frame  = &f;
    sh->funcdepth++;
    i32 loopdepth = sh->loopdepth;
    sh->loopdepth = 0;

    Code *code = fn->code;
    code->refs++;
    Str srcname = sh->srcname;
    sh->srcname = code->file;
    i32 ctlline = sh->ctlline;

    // Pop this call on the way out of an escape too, so that a shell
    // that resumes after an expansion error has no stale frames
    Escape e = {};
    e.up    = sh->esc;
    e.bout  = sh->bout;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    b32 escaped = jmpset(e.jmp);
    if (!escaped) {
        // As bash, DEBUG runs as the function starts, at its body's
        // line, where it applies (a traced function, set -T, extdebug)
        b32 skip = 0;
        if (sh->traps) {
            sh->line = code->body->line;
            skip = rundebugtrap(sh, scratch) || sh->flow;
        }
        if (code->body->redirs) sh->line = code->body->line;
        sh->ctlline = code->body->line;
        if (!skip) execute(sh, code->body, scratch);
        if (sh->traps) {
            sh->line = code->body->line;  // RETURN sees it too
            runreturntrap(sh, scratch);
        }
    }
    sh->esc = e.up;
    if (escaped && e.why==ESC_EXIT && sh->exitcalled) {
        // As bash, exit runs the EXIT trap still inside the function:
        // its FUNCNAME, locals and positional parameters
        sh->exitcalled = 0;
        if (!sh->execed && !sh->killsig) runexittrap(sh, scratch);
    }

    if (!--code->refs && code->dead) {
        freecode(sh, code);
    }
    if (sh->flow == FLOW_RETURN) {
        sh->flow = FLOW_NONE;
    }
    sh->loopdepth = loopdepth;
    sh->srcname   = srcname;
    sh->ctlline   = ctlline;
    leaveframe(sh, &f);
    sh->args = saved;
    if (escaped) {
        escape(sh, e.why);
    }
    return sh->status;
}

// Run a ${ cmd; } body in the current shell as bash does: like a
// function call for local and return, but with the caller's FUNCNAME
// and positional parameters, and its standard output collected in cap.
// Without cap, ${| cmd; }, REPLY is local and its final value, copied
// to a, is the result. The capture ends here even when the body
// escapes, since its memory goes with the caller's frame.
static Str runfunsub(Shell *sh, Arena *a, Str src, Capture *cap, Arena scratch)
{
    checkstack(sh);
    b32   valsub = !cap;
    Undo *mark   = sh->undo;
    if (cap) {
        File *out = newfile(sh, F_CAPTURE, OF_WRITE);
        out->cap = &cap->b;
        fdset(sh, 1, out, 0, &scratch);
    }
    Frame f = {};
    if (sh->frame) {
        f.name = sh->frame->name;
        f.def  = sh->frame->def;
        f.line = sh->frame->line;
        f.file = sh->frame->file;
        f.src  = sh->frame->src;
    }
    f.up = sh->frame;
    sh->frame = &f;
    sh->funcdepth++;
    i32 loopdepth = sh->loopdepth;
    sh->loopdepth = 0;

    Escape e = {};
    e.up    = sh->esc;
    e.bout  = sh->bout;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    Str val = {};
    i32 tracedepth = sh->tracedepth++;
    // As in $(...): no -v, and -e only with inherit_errexit, both
    // restored afterwards
    b32 errexit = sh->opt[O_ERREXIT];
    b32 verbose = sh->opt[O_VERBOSE];
    if (!sh->shopt[S_INHERIT_ERREXIT]) sh->opt[O_ERREXIT] = 0;
    sh->opt[O_VERBOSE] = 0;
    b32 escaped = jmpset(e.jmp);
    if (!escaped) {
        if (valsub) makelocal(sh, &scratch, "REPLY");
        runsource(sh, src, sh->iname, scratch, 0, sh->line, 0, 0, 1);
        if (valsub) val = clone(a, getvar(sh, &scratch, "REPLY").value);
    }
    sh->esc = e.up;
    sh->tracedepth = tracedepth;
    sh->opt[O_ERREXIT] = errexit;
    sh->opt[O_VERBOSE] = verbose;

    if (sh->flow == FLOW_RETURN) {
        sh->flow = FLOW_NONE;
    }
    sh->loopdepth = loopdepth;
    leaveframe(sh, &f);
    if (cap) {
        // As bash, the exit builtin, and anything that ends a subshell,
        // runs the EXIT trap with the redirections still in place, fd 1
        // included, so the trap writes nowhere. Their records stay for
        // the handler, in this depth's scratch, which stays reserved.
        b32 ends = e.why==ESC_EXIT || e.why==ESC_FATAL || e.why==ESC_DISCARD;
        b32 keep = escaped && ((e.why==ESC_EXIT && sh->exitcalled) ||
                               (ends && sh->subshell));
        if (!keep) unwind(sh, mark);
        endcapture(sh, cap, scratch);
    }
    if (escaped) {
        escape(sh, e.why);
    }
    return val;
}

// A leading word of this form assigns, even one read outside command
// position (see resumeaftereof), which the parser leaves splittable.
static b32 isassignword(Word *w)
{
    return w->flags & W_ASSIGNMENT;
}

struct Assign {
    Str name;
    Str value;
    b32 append;
};

static Assign splitassign(Shell *sh, Arena *a, Str text)
{
    Assign r = {};
    iz eq = assignment(text, 0);
    r.name = takehead(text, eq);
    if (r.name.len && r.name[r.name.len-1]=='+') {
        r.append = 1;
        r.name   = cuttail(r.name, 1);
    }
    r.value = expandstring(sh, a, cuthead(text, eq+1), XC_ASSIGN);
    return r;
}

// The value of a temporary binding (NAME=value before a command): for
// NAME+=value, appended to the current value, or added under -i.
static Str tempvalue(Shell *sh, Arena *a, Assign as)
{
    if (!as.append) return as.value;
    Value *v     = lookup(sh, as.name);
    u32    attrs = v ? v->attrs : 0;
    Str    old   = getvar(sh, a, as.name).value;
    return assignedvalue(sh, a, attrs, old, as.value, SV_APPEND);
}

// Perform the assignments of a command with no command word, each
// traced just before it happens. Returns false if one failed.
static b32 doassignments(Shell *sh, Arena *a, Word *w, iz n)
{
    for (iz i = 0; i < n; i++, w = w->next) {
        if (w->flags & W_COMPASSIGN) {
            // name=(...), traced as written
            iz  eq     = assignment(w->text, 0);
            Str name   = takehead(w->text, eq);
            b32 append = name.len && name[name.len-1]=='+';
            if (append) name = cuttail(name, 1);
            Str body = cuttail(cuthead(w->text, eq+2), 1);
            if (sh->opt[O_XTRACE]) {
                traceline(sh, *a, w->text);
            }
            Str base, sub;
            if (splitref(name, &base, &sub)) {
                Arena t = *a;
                Str msg = concat(&t, name, ": cannot assign list to array member");
                shellerror(sh, t, msg);
                return 0;
            }
            if (!assigncompound(sh, *a, name, body, append ? SV_APPEND : 0)) {
                return 0;
            }
            continue;
        }
        Assign as = splitassign(sh, a, w->text);
        if (sh->opt[O_XTRACE]) {
            traceassign(sh, *a, as.name, as.append, as.value);
        }
        if (!assignto(sh, *a, as.name, as.value, as.append ? SV_APPEND : 0)) {
            return 0;
        }
    }
    return 1;
}

[[gnu::noinline]]
static void runsimple1(Shell *sh, Node *n, Arena scratch, Str *last);

// $_: after each simple command, its last word (empty for assignments
// alone), never exported. The dynamic variable _ reads it from a buffer,
// so binding is a copy; only after a script assigned, unset or declared
// _ is the variable itself made dynamic again, as bash rebinds it.
static void bindlastarg(Shell *sh, Arena scratch, Str last)
{
    // A buffer below the store's mark belongs to the level around a
    // synchronous subshell, which gets it back as it was: replace it
    Store *st  = sh->store;
    b32    own = sh->lastarg.data && (byte *)sh->lastarg.data>=st->mark;
    if (!own || last.len>sh->lastargcap) {
        if (own) sfree(st, sh->lastarg.data, sh->lastargcap);
        // Doubling grows this level's own buffer: a fresh one in a
        // nested level is sized by need, or it doubles with depth
        iz grow = own ? 2*sh->lastargcap : 0;
        iz cap  = last.len>grow ? last.len : grow;
        cap = cap<32 ? 32 : cap;
        sh->lastarg.data = (u8 *)salloc(st, cap);
        sh->lastargcap   = cap;
    }
    copybytes(sh->lastarg.data, last.data, last.len);
    sh->lastarg.len = last.len;
    if (sh->lastargdirty) {
        sh->lastargdirty = 0;
        Value *v = lookup(sh, "_");
        if (v && (v->attrs & V_READONLY)) return;
        if (!v || v->arr) {
            unsetvar(sh, "_");
            v = setattrs(sh, "_", 0, 0);
        }
        Str old = v->str;
        v->str    = {};
        v->dyn    = DYN_UNDERSCORE;
        v->attrs &= ~V_EXPORT;
        if (old.data && (byte *)old.data >= st->mark) sfreestr(st, old);
        sh->lastargdirty = 0;
    }
}

static void runsimple(Shell *sh, Node *n, Arena scratch)
{
    Str last = {};
    runsimple1(sh, n, scratch, &last);
    sh->cmdnamelen = 0;
    if (last.data) bindlastarg(sh, scratch, last);
    checksigs(sh, scratch);
}

static void runsimple1(Shell *sh, Node *n, Arena scratch, Str *last)
{
    // A direct pipeline stage's program starts without waiting, but not
    // the programs its words or a trap run first: they finish before it
    Spawnto *direct = sh->spawnto;
    sh->spawnto = 0;
    b32 debugdone = sh->debugdone;  // a pipeline stage's, by its parent
    sh->debugdone = 0;
    b32 forked = sh->forked;
    b32 async  = sh->asyncpct;
    sh->forked   = 0;
    sh->asyncpct = 0;
    safepoint(sh->sched);
    sh->line    = n->line;
    sh->current = n;
    if (!sh->intrap) sh->cmdnode = n;  // in trap actions, what ran them
    if (sh->traps && !debugdone) {
        b32 skip = rundebugtrap(sh, scratch);
        sh->line    = n->line;
        sh->current = n;
        if (skip) return;
    }
    Arena *a = &scratch;

    // Leading assignments, then the rest
    Word *assigns = n->words;
    iz    nassign = 0;
    Word *rest    = n->words;
    while (rest && isassignword(rest)) {
        nassign++;
        rest = rest->next;
    }
    if (rest && sh->opt[O_KEYWORD]) {
        // set -k: assignments anywhere count, gathered after the others
        Word  *head = 0, **tail = &head;
        Word  *words = 0, **wtail = &words;
        for (Word *w = n->words; w; w = w->next) {
            Word *c = alloc<Word>(a);
            *c = *w;
            c->next = 0;
            b32 asg = isassignword(w) || (assignment(w->text, 0) && !(w->flags & W_COMPASSIGN));
            if (asg) {
                c->flags |= W_ASSIGNMENT|W_NOSPLIT;
                *tail = c;
                tail  = &c->next;
            } else {
                *wtail = c;
                wtail  = &c->next;
            }
        }
        nassign = 0;
        for (Word *w = head; w; w = w->next) nassign++;
        *tail   = words;
        assigns = head ? head : words;
        rest    = words;
    }

    // Procsubs in the words close with the command's redirections
    Undo *mark = sh->undo;

    // Is the command a declaration builtin? Then its assignment-looking
    // arguments expand as assignments.
    Slice<Str> argv = {};
    Slice<u8>  compargs = {};
    b32 decl = 0;
    // The registry entry for argv[0], found once for the declaration
    // check and for running it, when the first word expands to argv[0]
    Builtin *entry = 0;
    b32      named = 0;
    if (rest) {
        Slice<Str> first = {};
        Word one = *rest;
        one.next = 0;
        first = expandwords(sh, a, &one, 0);
        named = first.len > 0;
        entry = named ? tablebuiltin(first[0]) : 0;
        Builtin *b = usablebuiltin(sh, entry);
        decl = b && (b->flags & B_DECL);
        argv = first;
        for (Word *w = rest->next; w; w = w->next) {
            Word one2 = *w;
            one2.next = 0;
            Slice<Str> more;
            if (decl && (w->flags & W_COMPASSIGN)) {
                // name=(...) goes to the builtin as written, for it to
                // expand as an array assignment
                while (compargs.len < argv.len) compargs = push(a, compargs, (u8)0);
                compargs = push(a, compargs, (u8)1);
                argv = push(a, argv, w->text);
                continue;
            } else if (decl && (w->flags & W_ASSIGNMENT)) {
                // Brace expanded, but not split or globbed
                more = expandwords(sh, a, &one2, XW_NOSPLIT|XW_NOGLOB|XW_ASSIGN);
            } else if (argv.len && argv[0]=="unset" && arrayrefword(sh, *a, w->text)) {
                // compargs 2: its subscript was expanded and quoted
                more = expandwords(sh, a, &one2, XW_SUBQ);
                while (compargs.len < argv.len) compargs = push(a, compargs, (u8)0);
                for (iz i = 0; i < more.len; i++) compargs = push(a, compargs, (u8)2);
            } else {
                more = expandwords(sh, a, &one2, 0);
                if (more.len==1 && b && !decl && arrayrefword(sh, *a, w->text)) {
                    // compargs 3: an unquoted name[sub] word, which
                    // assoc_expand_once takes whole
                    while (compargs.len < argv.len) compargs = push(a, compargs, (u8)0);
                    compargs = push(a, compargs, (u8)3);
                }
            }
            for (iz i = 0; i < more.len; i++) argv = push(a, argv, more[i]);
        }
    }

    if (!argv.len) {
        *last = "";  // $_ becomes empty
        // Assignments only, with redirections in a temporary context.
        // The status is the last command substitution's, or zero.
        u32  nsubst = sh->nsubst;
        b32  ok     = doassignments(sh, a, assigns, nassign);
        i32  laststatus = sh->nsubst==nsubst ? 0 : sh->status;
        if (n->redirs && !doredirs(sh, n->redirs, a, 0)) {
            unwind(sh, mark);
            sh->status = sh->redirfail;
            checkerrexit(sh);
            return;
        }
        unwind(sh, mark);
        sh->status = ok ? laststatus : 1;
        if (!ok && posixfatal(sh)) {
            escape(sh, ESC_FATAL);
        } else if (!ok) {
            escape(sh, ESC_DISCARD);  // as bash abandons the command
        }
        checkerrexit(sh);
        return;
    }

    Str name = argv[0];
    *last = argv[argv.len-1];

    // Expand assignment values now (bash does it after the words)
    Assign *as = alloc<Assign>(a, nassign);
    {
        // Each sees those before it, bound for the time being, as in
        // K=x A=${K#?} cmd
        Word *w      = assigns;
        b32  *pushed = alloc<b32>(a, nassign);
        sh->nolocwarn = 1;  // not yet the command's binding
        for (iz i = 0; i < nassign; i++, w = w->next) {
            as[i] = splitassign(sh, a, w->text);
            b32 later = 0;
            for (Word *v = w->next; v && !later; v = v->next) {
                later = find(v->text, "$")>=0 || find(v->text, "`")>=0;
            }
            Value *cur = lookup(sh, as[i].name);
            if (later && !(cur && (cur->attrs & V_READONLY)) && validvarname(as[i].name)) {
                Str value = tempvalue(sh, a, as[i]);
                pushed[i] = pushtemp(sh, *a, as[i].name, value);
            }
        }
        for (iz i = nassign-1; i >= 0; i--) {
            if (pushed[i]) poptemp(sh, as[i].name);
        }
        sh->nolocwarn = 0;
    }
    {
        // An array element cannot be bound for one command: reported,
        // and the command runs without it, as in bash
        iz kept = 0;
        for (iz i = 0; i < nassign; i++) {
            if (validvarname(as[i].name)) {
                as[kept++] = as[i];
            } else {
                Str msg = concat3(&scratch, "`", as[i].name, "': not a valid identifier");
                shellerror(sh, scratch, msg);
            }
        }
        nassign = kept;
    }

    if (sh->opt[O_XTRACE]) {
        for (iz i = 0; i < nassign; i++) {
            traceassign(sh, scratch, as[i].name, as[i].append, as[i].value);
        }
        tracewords(sh, scratch, argv.data, argv.len);
    }

    // Job control shorthand: "%N" is "fg %N", and "%N &" is "bg %N",
    // the builtin even if a function has its name, and as in bash with
    // no redirections. Not where bash forks before expanding the words:
    // a stage, or a job's command.
    b32 jobspec = name.len && name[0]=='%' && !forked;
    if (jobspec) {
        Slice<Str> v = {};
        v = push(a, v, async ? Str("bg") : Str("fg"));
        for (iz i = 0; i < argv.len; i++) v = push(a, v, argv[i]);
        argv     = v;
        name     = argv[0];
        named    = 0;
        compargs = {};
    }

    // exec without a command makes its redirections permanent
    // (outside a subshell only once all of them succeed: a failure
    // undoes the rest, as in bash), also run through command
    iz ex = 0;
    while (ex<argv.len && argv[ex]=="command" && !findfunc(sh, "command")) {
        for (ex++; ex<argv.len && argv[ex]=="-p"; ex++) {}
        ex += ex<argv.len && argv[ex]=="--";
    }
    b32 execonly = ex<argv.len && argv[ex]=="exec" &&
                   (argv.len==ex+1 || (argv.len==ex+2 && argv[ex+1]=="--"));
    b32 commit   = execonly && !sh->subshell;
    i64 lastbg   = sh->lastbg;
    Str savedname = sh->bltname;
    if (n->redirs && findbuiltin(sh, name)) sh->bltname = name;  // for messages
    b32 keep       = (execonly && !commit) || n==sh->keepredirs;
    b32 redirected = jobspec || doredirs(sh, n->redirs, a, keep);
    sh->bltname = savedname;
    if (!redirected) {
        unwind(sh, mark);
        sh->status = sh->redirfail;
        Builtin *sb = findbuiltin(sh, name);
        if (sb && (sb->flags & B_SPECIAL) && posixfatal(sh)) {
            sh->exiting = 1;  // as bash: a special builtin's, fatal
            escape(sh, ESC_EXIT);
        }
        checkerrexit(sh);
        return;
    }
    if (execonly) {
        closepsubs(sh, mark);  // exec 3< <(f) keeps 3, not 63
    }
    if (commit) {
        commitredirs(sh, mark);
    }
    if (execonly) {
        mark = sh->undo;
    }

    // Checked again: an expansion since may have run enable or shopt
    Func    *fn = 0;
    Builtin *bi = named ? usablebuiltin(sh, entry) : findbuiltin(sh, name);
    b32 special = bi && (bi->flags & B_SPECIAL);
    if (!(special && sh->opt[O_POSIX]) && !jobspec) {
        // as bash, a name with a slash too, outside POSIX mode
        if (!sh->opt[O_POSIX] || !haspath(sh, name)) fn = findfunc(sh, name);
    }

    if (posixfatal(sh)) {
        // As bash, an assignment that fails ends the shell before a
        // special builtin, or abandons the command before another
        for (iz i = 0; i < nassign; i++) {
            Value *cur = lookup(sh, as[i].name);
            if (cur && (cur->attrs & V_READONLY)) {
                Str msg = concat(&scratch, as[i].name, ": readonly variable");
                shellerror(sh, scratch, msg);
                unwind(sh, mark);
                sh->status = 1;
                escape(sh, special&&!fn ? ESC_FATAL : ESC_DISCARD);
            }
        }
    }

    if (fn || bi) {
        // Temporary bindings for builtins and functions
        b32 persist = special && sh->opt[O_POSIX];
        for (iz i = 0; i < nassign; i++) {
            if (persist) {
                i32 flags = as[i].append ? SV_APPEND : 0;
                setvar(sh, scratch, as[i].name, as[i].value, flags);
                Value *v = lookup(sh, as[i].name);
                if (v && (v->attrs & V_TEMP)) {
                    // as bash, also past a function call's temporary
                    // binding, so the value outlives the call
                    setvar(sh, scratch, as[i].name, as[i].value, flags|SV_UNDERTEMP);
                }
                // and exported, as the temporary binding bash makes and
                // then keeps is
                for (i32 k = 0; k < 2; k++) {
                    i32    f = SV_QUIET|SV_FORCE|(k ? SV_UNDERTEMP : 0);
                    Value *t = assignslot(sh, scratch, as[i].name, f);
                    if (t && !(t->attrs & V_EXPORT)) {
                        t->attrs |= V_EXPORT;
                        sh->exportgen++;
                    }
                }
            } else {
                Str value = tempvalue(sh, &scratch, as[i]);
                pushtemp(sh, scratch, as[i].name, value);
            }
        }
        if (fn) {
            // Until its first simple command ends, a function's name
            // prefixes errors from declaration arguments name=(...)
            i32 len = (i32)(name.len<(iz)sizeof(sh->cmdname) ? name.len : 0);
            copybytes(sh->cmdname, name.data, len);
            sh->cmdnamelen = len;
            callfunction(sh, fn, argv.data, argv.len, scratch);
            sh->line = n->line;
        } else {
            BOut  b     = {};
            BOut *saved = sh->bout;
            b.b  = Buf(a, (iz)1<<12);
            sh->bout = &b;
            Slice<u8> savedcomp = sh->compargs;
            sh->compargs = compargs;
            Str  savedname  = sh->bltname;
            Str *savedargs  = sh->bltargs;
            iz   savednargs = sh->nbltargs;
            sh->bltname  = name;
            sh->bltargs  = argv.data;
            sh->nbltargs = argv.len;
            sh->nowriteerr = 0;
            sh->bifatal    = 0;
            sh->status = runbuiltin(sh, bi, argv.data, argv.len, scratch);
            b32 fatal = special && sh->bifatal && posixfatal(sh) && !sh->noerrexit;
            sh->bifatal = 0;
            if (fatal) {
                // As bash, a special builtin's error (a usage error, an
                // invalid identifier, ...) ends a POSIX shell
                sh->bltname  = savedname;
                sh->bltargs  = savedargs;
                sh->nbltargs = savednargs;
                sh->compargs = savedcomp;
                sh->bout     = saved;
                if (b.b.len) fdwrite(sh, 1, finish(&b.b));
                sh->exiting = 1;
                escape(sh, ESC_EXIT);
            }
            sh->bltname  = savedname;
            sh->bltargs  = savedargs;
            sh->nbltargs = savednargs;
            sh->compargs = savedcomp;
            sh->bout = saved;
            b32 failed = (b.b.len && !fdwrite(sh, 1, finish(&b.b))) || b.failed;
            if (failed && !sh->nowriteerr) {
                i32 e   = sh->writeerr;
                b32 keep = e==E_PIPE || e==E_NOSPC || e==E_ACCES || e==E_NOMEM;
                Str why  = errtext(keep ? e : E_BADF);
                Str msg = concat(&scratch, "write error: ", why);
                builtinerror(sh, scratch, name, msg);
                sh->status = 1;
            }
        }
        if (!persist) {
            for (iz i = nassign-1; i >= 0; i--) poptemp(sh, as[i].name);
        }
    } else {
        // bash redirects a program in its forked child, so a procsub
        // there does not set $! here
        sh->lastbg = lastbg;
        Slice<Str> env = makeenv(sh, a);
        for (iz i = 0; i < nassign; i++) {
            Str tname = tempname(sh, as[i].name);  // what a nameref names
            Str entry = concat3(a, tname, "=", tempvalue(sh, a, as[i]));
            b32 replaced = 0;
            for (iz k = 0; k < env.len; k++) {
                Cut c = cut(env[k], '=');
                if (c.head == tname) {
                    env[k]   = entry;
                    replaced = 1;
                }
            }
            if (!replaced) env = push(a, env, entry);
        }
        // Also bound in the shell while the program is looked up, as
        // bash's temporary environment: a PATH there is searched, and
        // command_not_found_handle sees them all
        // (no setlocale warning: bash sets the locale only in the child)
        sh->nolocwarn = 1;
        for (iz i = 0; i < nassign; i++) {
            Str value = tempvalue(sh, &scratch, as[i]);
            pushtemp(sh, scratch, as[i].name, value);
        }
        sh->nolocwarn = 0;
        // As bash, a program finds its own path in $_
        Str path = find(name, "/")>=0 ? name : pathlookup(sh, a, name);
        if (path.len) {
            Str  entry = concat(a, "_=", path);
            b32  done  = 0;
            for (iz k = 0; k<env.len && !done; k++) {
                if (startswith(env[k], "_=")) {
                    env[k] = entry;
                    done   = 1;
                }
            }
            if (!done) env = push(a, env, entry);
        }
        sh->childsig = 0;
        sh->spawnto = direct;
        sh->attail  = n==sh->tailnode && sh->taillevel==sh->subshell+1;
        Str *names = alloc<Str>(&scratch, nassign);
        for (iz i = 0; i < nassign; i++) names[i] = as[i].name;
        sh->undonames = names;
        sh->nundo     = nassign;
        sh->undoargv  = argv.data;
        sh->status = runexternal(sh, scratch, argv.data, argv.len, env);
        sh->attail  = 0;
        if (!direct) chldreaped(sh, 1);
        if (sh->nundo) {
            // Not spawned here: a stage that ends in the program, as
            // bash's child that execs, never restores (and so is quiet)
            sh->nolocwarn = !!direct;
            for (iz i = nassign-1; i >= 0; i--) poptemp(sh, as[i].name);
            sh->nolocwarn = 0;
            sh->nundo     = 0;
        }
        reportchild(sh, scratch, n);
    }
    unwind(sh, mark);
    if (sh->flow != FLOW_RETURN) {
        checkerrexit(sh);
    }
}


// Compound commands

static b32 casematch(Shell *sh, Arena *a, Str word, Word *patterns)
{
    i32 pf = patflags(sh);
    if (sh->shopt[S_NOCASEMATCH]) pf |= PM_NOCASE;
    for (Word *p = patterns; p; p = p->next) {
        Str pat = expandpattern(sh, a, p->text, wordctx(sh, p));
        if (patmatch(pat, word, pf, *a)) return 1;
    }
    return 0;
}

static i32 condeval(Shell *sh, Arena *a, Cond *c);

struct TestCtx {
    Shell *sh;
    Arena *a;
    b32    cond;  // [[ ]]: -v subscripts were already expanded, quoted
};

static Stat envstat(void *ctx, Str path, b32 follow)
{
    TestCtx *t  = (TestCtx *)ctx;
    Shell   *sh = t->sh;
    Arena    a  = *t->a;

    // As bash, /dev/fd/N and /dev/std* name the shell's own descriptors
    // (the Windows modes resolve them with the other virtual files)
    b32 posix = sh->pathmode == PM_POSIX;
    i64 fd    = -1;
    if (posix && path=="/dev/stdin") {
        fd = 0;
    } else if (posix && path=="/dev/stdout") {
        fd = 1;
    } else if (posix && path=="/dev/stderr") {
        fd = 2;
    } else if (posix && startswith(path, "/dev/fd/")) {
        TestNum n = testparseint(cuthead(path, 8));
        fd = n.ok && n.value>=0 ? n.value : (i64)FDMAX;
    }
    if (fd >= 0) return fdstat(sh, fd);
    return pathstat(sh, a, path, follow);
}

static b32 envaccess(void *ctx, Str path, i32 mode)
{
    Stat s = envstat(ctx, path, 1);
    if (s.err) return 0;
    if (!mode) return 1;
    Shell *sh = ((TestCtx *)ctx)->sh;
    i32 bits = s.uid==sh->euid ? s.mode>>6 : s.gid==sh->egid ? s.mode>>3 : s.mode;
    if ((mode & 1) && s.type==FT_DIR) return (bits & 1) != 0;
    return (bits & mode) == mode;
}

static b32 envisatty(void *ctx, i64 fd)
{
    Shell *sh = ((TestCtx *)ctx)->sh;
    File  *f  = fd>=0 && fd<FDMAX ? sh->fds->fd[fd] : 0;
    return f && f->kind==F_OS && os_isatty(sh->os, f->h);
}

static i32 envoption(void *ctx, Str name)
{
    Shell *sh = ((TestCtx *)ctx)->sh;
    for (i32 i = 0; i < O_COUNT; i++) {
        if (setoptions[i].name==name && hasoption(sh, i)) return sh->opt[i];
    }
    return -1;
}

static b32 envvarset(void *ctx, Str name)
{
    Shell  *sh = ((TestCtx *)ctx)->sh;
    TestNum n  = testparseint(name);
    if (n.ok) {
        // As bash, a number names a positional parameter, and 0 is set
        return n.value>=0 && n.value<=sh->args.n;
    }
    Str rn, rs;
    if (splitref(name, &rn, &rs)) {
        // In [[ ]], as bash 5.3, the subscript is not expanded again: it
        // was expanded once, its result backslash-quoted
        Arena a = *((TestCtx *)ctx)->a;
        X    *x = newx(sh, &a);
        if (!((TestCtx *)ctx)->cond) {
            Value *v = lookup(sh, rn);
            if (v && v->arr && v->arr->bucket && (allsub(rs) || sh->shopt[S_ARRAY_EXPAND_ONCE])) {
                return !!hfind(v->arr, rs);  // a key as it is (array_expand_once)
            }
            if (sh->shopt[S_ARRAY_EXPAND_ONCE] && !allsub(rs) && !(v && v->arr && v->arr->bucket)) {
                rs = fmti64(&a, evalarith(sh, &a, rs).value);  // an index as it is
            }
            return getelems(x, rn, rs).set;
        }
        Buf   b(&a, rs.len);
        for (iz i = 0; i < rs.len; i++) {
            if (rs[i]=='\\' && i+1<rs.len) i++;
            putbyte(&b, rs[i]);
        }
        Str    key = finish(&b);
        Value *v   = lookup(sh, rn);
        if (v && v->arr && v->arr->bucket) {
            return !!hfind(v->arr, key);
        }
        if (!allsub(rs)) {
            rs = fmti64(&a, evalarith(sh, &a, key).value);
        }
        return getelems(x, rn, rs).set;
    }
    Arena none = {};
    return getvar(sh, &none, name).set;
}

static b32 envnameref(void *ctx, Str name)
{
    Shell *sh = ((TestCtx *)ctx)->sh;
    Value *v = rawlookup(sh, name);
    return v && (v->attrs & V_NAMEREF);
}

static TestEnv testenv(TestCtx *t)
{
    Shell  *sh = t->sh;
    TestEnv e  = {};
    e.ctx     = t;
    e.stat    = envstat;
    e.access  = envaccess;
    e.isatty  = envisatty;
    e.option  = envoption;
    e.varset  = envvarset;
    e.nameref = envnameref;
    e.euid    = sh->euid;
    e.egid    = sh->egid;
    return e;
}

// With subq, for -v and the arithmetic operators, array subscripts
// expand once, as in arithmetic.
static Str condword(Shell *sh, Arena *a, Word *w, b32 subq = 0)
{
    return expandstring(sh, a, w->text, wordctx(sh, w) | (subq ? XC_SUBQ : 0));
}

// Trace one [[ ]] primary as it is evaluated, operands unquoted. The
// right operand is null for a unary primary.
static void tracecond(Shell *sh, Arena a, b32 invert, Str op, Str lhs, Str *rhs)
{
    if (!sh->opt[O_XTRACE]) return;
    Buf b(&a, 128);
    print(&b, "[[ ");
    if (invert) print(&b, "! ");
    if (rhs) {
        print(&b, lhs.len ? lhs : Str("''"));
        putbyte(&b, ' ');
        print(&b, op);
        putbyte(&b, ' ');
        print(&b, rhs->len ? *rhs : Str("''"));
    } else {
        print(&b, op);
        putbyte(&b, ' ');
        print(&b, lhs.len ? lhs : Str("''"));
    }
    print(&b, " ]]");
    traceline(sh, a, finish(&b));
}

// Whether a signal that would end this level (an untrapped Ctrl-C) is
// pending, to stop a regex search. Unlike glibc's, ours stops: some
// patterns take minutes, or longer.
static b32 signoop(Shell *, i32);

static b32 rxsigstop(void *ctx)
{
    Shell *sh = (Shell *)ctx;
    takeossigs(sh);
    for (u64 m = sh->sig->pending; m; m &= m-1) {
        i32 sig = __builtin_ctzll(m) + 1;
        if (!trapignored(sh, sig) && !trapset(sh, sig) && !signoop(sh, sig)) return 1;
    }
    return 0;
}

// [[ s =~ re ]]: 0 for a match, 1 for none, 2 for an invalid regular
// expression. BASH_REMATCH is emptied, then gets the match and its
// groups, an unset group as an empty string, as bash's sh_regmatch.
static i32 condregex(Shell *sh, Arena a, Str s, Str re)
{
    i32 flags = 0;
    if (sh->shopt[S_NOCASEMATCH]) flags |= REX_ICASE;
    if (sh->utf8) flags |= REX_UTF8;
    RxComp rc = rxcompile(re, flags, &a);
    if (!rc.re) {
        Str msg = concat3(&a, "invalid regular expression `", re, "': ");
        Str msg2 = concat(&a, msg, rxerrors[rc.err]);
        builtinerror(sh, a, "[[", msg2);
        return 2;
    }
    i32      ng = rc.re->nsub + 1;
    RxMatch *m  = alloc<RxMatch>(&a, ng);
    i32      r  = rxexec(rc.re, s, m, a, -1, rxsigstop, sh);
    if (r == -1) sigfatal(sh);  // stopped by a signal that ends this level
    // -2: out of room, as glibc's REG_ESPACE, which bash takes for no match
    b32      ok = r == 1;

    i32 aflags = SV_FORCE;
    Str name   = arraytarget(sh, a, "BASH_REMATCH", &aflags, 1);
    if (!name.data) return !ok;
    Value *val = assignslot(sh, a, name, aflags & ~SV_FORCE);
    if (!val || (val->attrs & V_NOASSIGN)) return !ok;
    if (val->arr && val->arr->bucket) {
        Str msg = concat(&a, name, ": not an indexed array");
        builtinerror(sh, a, "[[", msg);
        return !ok;
    }
    toarray(sh, val);
    aclear(sh->store, val->arr);
    val->arr->isset = 1;
    for (i32 i = 0; ok && i < ng; i++) {
        // bash copies eo-so bytes from so: an unclosed group (eo < so)
        // copies the rest of the string
        Str v = {};
        if (m[i].so >= 0) {
            v = cuthead(s, m[i].so);
            if (m[i].eo >= m[i].so) v = takehead(v, m[i].eo-m[i].so);
        }
        setelem(sh, a, name, i, {}, v, aflags);
    }
    return !ok;
}

// The status of a [[ ]] expression: 0 true, 1 false, 2 for an error
// (the -t operand's or a regular expression's), which || and ! treat as
// false, as bash does.
static i32 condeval(Shell *sh, Arena *a, Cond *c)
{
    i32 r = 1;
    switch (c->kind) {
    case C_AND:
        r = condeval(sh, a, c->left);
        if (!r) r = condeval(sh, a, c->right);
        break;
    case C_OR:
        r = condeval(sh, a, c->left);
        if (r) r = condeval(sh, a, c->right);
        break;
    case C_EXPR:
        r = condeval(sh, a, c->left);
        break;
    case C_TERM: {
        Str arg = condword(sh, a, c->op);
        tracecond(sh, *a, c->invert, "-n", arg, 0);
        r = arg.len == 0;
    } break;
    case C_UNARY: {
        TestCtx tc = {sh, a, 1};
        TestEnv e = testenv(&tc);
        Str arg = condword(sh, a, c->left->op, c->op->text=="-v");
        tracecond(sh, *a, c->invert, c->op->text, arg, 0);
        TestResult t = testunary(&e, a, c->op->text, arg);
        if (t.status == 2) {
            builtinerror(sh, *a, "[[", t.err);
        }
        r = t.status;
    } break;
    case C_BINARY: {
        Str op = c->op->text;
        b32 arith = op=="-eq" || op=="-ne" || op=="-lt" || op=="-le" ||
                    op=="-gt" || op=="-ge";
        Str lhs = condword(sh, a, c->left->op, arith);
        if (op=="==" || op=="=" || op=="!=") {
            Str pat = expandpattern(sh, a, c->right->op->text, wordctx(sh, c->right->op));
            tracecond(sh, *a, c->invert, op, lhs, &pat);
            i32 pf  = patflags(sh) | PM_EXTGLOB;  // always, as in bash
            if (sh->shopt[S_NOCASEMATCH]) pf |= PM_NOCASE;
            b32 m = patmatch(pat, lhs, pf, *a);
            r = (op=="!=") == m;
        } else if (op == "=~") {
            Str rhs = expandregex(sh, a, c->right->op->text, wordctx(sh, c->right->op));
            tracecond(sh, *a, c->invert, op, lhs, &rhs);
            r = condregex(sh, *a, lhs, rhs);
        } else if (arith) {
            Str rhs = condword(sh, a, c->right->op, 1);
            tracecond(sh, *a, c->invert, op, lhs, &rhs);
            // An arithmetic error makes just this primary false
            Arith x = evalarithcmd(sh, a, lhs, "[[");
            Arith y = x.ok ? evalarithcmd(sh, a, rhs, "[[") : x;
            r = !(x.ok && y.ok && testintcmp(op, x.value, y.value));
        } else {
            Str rhs = condword(sh, a, c->right->op);
            tracecond(sh, *a, c->invert, op, lhs, &rhs);
            TestCtx tc = {sh, a, 1};
            TestEnv e = testenv(&tc);
            TestResult t = testbinary(&e, a, op, lhs, rhs);
            if (t.status == 2) builtinerror(sh, *a, "[[", t.err);
            r = t.status;
        }
    } break;
    }
    return c->invert ? r==0 : r;
}

static b32 loopflow(Shell *sh)
{
    // Handle break and continue at a loop's end of iteration. Returns
    // true if the loop should stop.
    if (sh->flow == FLOW_BREAK) {
        if (--sh->flowcount <= 0) sh->flow = FLOW_NONE;
        return 1;
    }
    if (sh->flow == FLOW_CONTINUE) {
        if (--sh->flowcount <= 0) {
            sh->flow = FLOW_NONE;
            return 0;
        }
        return 1;
    }
    return sh->flow == FLOW_RETURN || sh->exiting;
}

// A loop's safe point. Now and then it also takes signals, which a
// loop of only arithmetic and compound commands would otherwise never
// see. True if a trap's break, return or exit ends the loop.
static b32 loopsafepoint(Shell *sh, Arena scratch)
{
    if (!safepoint(sh->sched)) return 0;
    checksigs(sh, scratch);
    return (sh->flow && loopflow(sh)) || sh->exiting;
}

static void runpipeline(Shell *sh, Node *n, Arena scratch);
static void runselect(Shell *sh, Node *n, Arena scratch);

// Evaluate one part of an arithmetic for, traced as bash does with
// leading space trimmed. An empty part is true. Returns false on error.
static b32 arithpart(Shell *sh, Arena scratch, Word *w, Arith *r = 0)
{
    Str e = {};
    if (w) e = trimleft(expandstring(sh, &scratch, w->text, XC_ARITH|XC_DQUOTE));
    if (!e.len) e = "1";
    if (sh->opt[O_XTRACE]) {
        Str msg = concat3(&scratch, "(( ", e, " ))");
        traceline(sh, scratch, msg);
    }
    Arith t = evalarithcmd(sh, &scratch, e, "((");
    if (r) *r = t;
    return t.ok;
}

// A duration in ns as the time keyword prints it: seconds with prec
// decimals (truncated), or minutes and seconds when long.
static void printduration(Buf *b, i64 ns, i32 prec, b32 islong, u8 radix)
{
    if (ns < 0) ns = 0;
    i64 sec = ns / 1000000000;
    i64 frac = ns % 1000000000;
    if (islong) {
        print(b, sec/60);
        putbyte(b, 'm');
        sec %= 60;
    }
    print(b, sec);
    if (prec) {
        putbyte(b, radix ? radix : '.');
        for (i32 i = 0; i < prec; i++) {
            frac *= 10;
            putbyte(b, (u8)('0' + frac/1000000000));
            frac %= 1000000000;
        }
    }
    if (islong) putbyte(b, 's');
}

// Report a timed pipeline per TIMEFORMAT (or the POSIX format for -p)
// on standard error. CPU times are not tracked yet and show as zero.
static void timereport(Shell *sh, Arena scratch, i64 real, b32 posix)
{
    Str fmt = "\nreal\t%3lR\nuser\t%3lU\nsys\t%3lS";
    if (posix) {
        fmt = "real %2R\nuser %2U\nsys %2S";
    } else {
        Got g = getvar(sh, &scratch, "TIMEFORMAT");
        if (g.set) fmt = g.value;
    }
    if (!fmt.len) return;
    Buf b(&scratch, 64);
    for (iz i = 0; i < fmt.len; i++) {
        u8 c = fmt[i];
        if (c!='%' || i+1==fmt.len) {
            putbyte(&b, c);
            continue;
        }
        iz j = i + 1;
        if (fmt[j] == '%') {
            putbyte(&b, '%');
            i = j;
            continue;
        }
        i32 prec = 3;
        b32 islong = 0;
        if (j<fmt.len && fmt[j]>='0' && fmt[j]<='9') {
            prec = fmt[j++] - '0';
            if (prec > 3) prec = 3;
        }
        if (j<fmt.len && fmt[j]=='l') {
            islong = 1;
            j++;
        }
        if (j == fmt.len) {
            putbyte(&b, c);
            continue;
        }
        switch (fmt[j]) {
        case 'R': printduration(&b, real, prec, islong, sh->radix); break;
        case 'U':
        case 'S': printduration(&b, 0, prec, islong, sh->radix);    break;
        case 'P': print(&b, "0.00");                     break;
        default: {
            // As bash: an error instead of the whole report
            Str bad = {&fmt.data[j], 1};
            Str msg = concat3(&scratch, "TIMEFORMAT: `", bad, "': invalid format character");
            shellerror(sh, scratch, msg);
            return;
        }
        }
        i = j;
    }
    putbyte(&b, '\n');
    flushbout(sh);
    fdwrite(sh, 2, finish(&b));
}

static void executenode(Shell *, Node *, Arena);
static void executeone(Shell *, Node *, Arena);

static void execute(Shell *sh, Node *n, Arena scratch)
{
    if (n && (n->flags & CMD_TIME) && !sh->exiting && !sh->flow) {
        i64 start = os_now(sh->os);
        executeone(sh, n, scratch);
        timereport(sh, scratch, os_now(sh->os)-start, n->flags & CMD_TIMEPOSIX);
        return;
    }
    executenode(sh, n, scratch);
}

// Compound command cases, kept out of executenode so that its frame
// stays small: every level of shell recursion passes through it.

[[gnu::noinline]]
static void execfuncdef(Shell *sh, Node *n, Arena scratch)
{
    // Errors cite, as bash does, the definition's line in a
    // function, and elsewhere where the parser has got to
    Str name = n->name->text;
    sh->line = sh->funcdepth || !sh->lexer ? n->line : sh->lexer->lineno;
    if (n->name->flags & (W_HASDOLLAR|W_QUOTED)) {
        Str msg = concat3(&scratch, "`", name, "': not a valid identifier");
        shellerror(sh, scratch, msg);
        sh->status = 1;
        return;
    }
    Builtin *special = sh->opt[O_POSIX] ? findbuiltin(sh, name) : 0;
    if (special && (special->flags & B_SPECIAL)) {
        Str msg = concat3(&scratch, "`", name, "': is a special builtin");
        shellerror(sh, scratch, msg);
        sh->status = 2;
        if (!sh->interactive) {
            sh->exiting = 1;
            escape(sh, ESC_EXIT);
        }
        return;
    }
    i32 errline = sh->line;
    sh->line = n->line;
    Node *body = n->a;
    if (n->redirs) {
        // f() { ...; } >file: the redirections belong to the body,
        // and their errors cite the definition's line
        Node *g = alloc<Node>(&scratch);
        *g = *body;
        g->redirs = n->redirs;
        g->line   = n->line;
        body = g;
    }
    Func *old = findfunc(sh, name);
    if (old && (old->attrs & V_READONLY)) {
        sh->line = errline;  // as bash: where the parser got to
        Str msg = concat(&scratch, name, ": readonly function");
        shellerror(sh, scratch, msg);
        sh->status = 1;
        return;
    }
    definefunc(sh, scratch, name, body);
    sh->status = 0;
}

[[gnu::noinline]]
static void execsubshell(Shell *sh, Node *n, Arena scratch)
{
    if (fgjobctl(sh) && runfgimage(sh, n, scratch)) return;
    SubJob j = {};
    j.kind = SUB_NODE;
    j.node = n->a;
    j.newloops = 1;
    j.nojobs   = 1;
    sh->childsig  = 0;
    sh->tailnode  = tailcommand(n->a);
    sh->taillevel = sh->subshell + 2;
    sh->status = subshell(sh, &j, scratch, 0);
    chldreaped(sh, 1);
    sh->tailnode  = 0;
    sh->taillevel = 0;
    if (!n->redirs) reportchild(sh, scratch, n);  // else once they are undone
    setpstat1(sh);
    checkerrexit(sh);
    checksigs(sh, scratch);
}

[[gnu::noinline]]
static void execfor(Shell *sh, Node *n, Arena scratch)
{
    sh->line    = n->line;
    sh->ctlline = n->line;
    Slice<Str> list = {};
    if (n->words) {
        list = expandwords(sh, &scratch, n->words, 0);
    }
    // As bash, the header is traced as source text, every iteration
    Str head = {};
    b32 debug = n->kind==N_FOR && sh->traps && sh->traps[TRAP_DEBUG].len;
    if ((sh->opt[O_XTRACE] || debug) && n->kind==N_FOR) {
        Buf b(&scratch, 64);
        print(&b, "for ");
        print(&b, n->name->text);
        print(&b, " in");
        for (Word *w = n->words; w; w = w->next) {
            putbyte(&b, ' ');
            print(&b, w->text);
        }
        head = finish(&b);
    }
    Str var = n->name->text;
    if (!validvarname(var)) {
        Str msg = concat3(&scratch, "`", var, "': not a valid identifier");
        shellerror(sh, scratch, msg);
        sh->status = 1;
        posixbadvar(sh);
        return;
    }
    // $? carries into the body, perhaps from the list's expansion
    if (!list.len) sh->status = 0;
    sh->loopdepth++;
    for (iz i = 0; i < list.len; i++) {
        Arena it = scratch;
        if (loopsafepoint(sh, it)) break;
        if (head.data && sh->opt[O_XTRACE]) traceline(sh, it, head);
        if (debug) {
            // DEBUG before each iteration, BASH_COMMAND the header
            if (!sh->intrap) {
                Store *st = sh->store;
                if ((byte *)sh->cmdtext.data >= st->mark) sfreestr(st, sh->cmdtext);
                sh->cmdtext = sclone(st, head);
                sh->cmdnode = 0;
            }
            sh->line = n->line;
            rundebugtrap(sh, it);
            sh->line = n->line;
        }
        Value *ref = rawlookup(sh, var);
        if (ref && (ref->attrs & V_NAMEREF)) {
            // A nameref loop variable refers to each word in turn
            Ref r = resolveref(sh, var);
            if (!validnameref(list[i])) {
                Str msg = concat3(&it, "`", list[i], "': not a valid identifier");
                shellerror(sh, it, msg);
                sh->status = 1;
                break;
            }
            if (!setvar(sh, it, r.last ? r.lastname : var, list[i], SV_NOREF)) {
                sh->status = 1;
                break;
            }
        } else if (!setvar(sh, it, var, list[i])) {
            sh->status = 1;
            if (posixfatal(sh)) escape(sh, ESC_FATAL);  // as bash
            break;
        }
        execute(sh, n->b, it);
        if (sh->flow && loopflow(sh)) break;
        if (sh->exiting) break;
    }
    sh->loopdepth--;
}

[[gnu::noinline]]
// DEBUG before one of an arithmetic for's expressions, BASH_COMMAND
// naming it as (( expr )).
static void debugarithfor(Shell *sh, Node *n, Word *part, Arena scratch)
{
    if (!sh->intrap) {
        Store *st = sh->store;
        if ((byte *)sh->cmdtext.data >= st->mark) sfreestr(st, sh->cmdtext);
        Str t = part ? part->text : Str{};
        sh->cmdtext = sclone(st, concat3(&scratch, "((", t, "))"));
        sh->cmdnode = 0;
    }
    sh->line = n->line;
    rundebugtrap(sh, scratch);
    sh->line = n->line;
}

static void execarithfor(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    Word *init = n->arith;
    Word *test = init ? init->next : 0;
    Word *step = test ? test->next : 0;
    // As with (( )), an arithmetic error fails the loop with status 1.
    // DEBUG runs before each of the three expressions, as in bash.
    if (sh->traps) debugarithfor(sh, n, init, scratch);
    if (!arithpart(sh, scratch, init)) {
        sh->status = 1;
        return;
    }
    i32 status = 0;
    sh->loopdepth++;
    for (;;) {
        Arena it = scratch;
        if (loopsafepoint(sh, it)) break;
        Arith r = {};
        if (sh->traps) debugarithfor(sh, n, test, it);
        if (!arithpart(sh, it, test, &r)) status = 1;
        if (!r.ok || !r.value) break;
        execute(sh, n->b, it);
        status = sh->status;
        if (sh->flow && loopflow(sh)) break;
        if (sh->exiting) break;
        if (sh->traps) debugarithfor(sh, n, step, it);
        if (!arithpart(sh, it, step)) {
            status = 1;
            break;
        }
    }
    sh->loopdepth--;
    sh->status = status;
}

[[gnu::noinline]]
// DEBUG before a compound command that BASH_COMMAND names by its
// header, such as "case WORD in ". Returns whether to skip it.
static b32 debugheader(Shell *sh, Str head, i32 line)
{
    if (!sh->traps || !sh->traps[TRAP_DEBUG].len) return 0;
    if (!sh->intrap) {
        Store *st = sh->store;
        if ((byte *)sh->cmdtext.data >= st->mark) sfreestr(st, sh->cmdtext);
        sh->cmdtext = sclone(st, head);
        sh->cmdnode = 0;
    }
    sh->line = line;
    Arena scratch = *scratchat(sh, sh->capdepth+1);
    b32 skip = rundebugtrap(sh, scratch);
    sh->line = line;
    return skip;
}

[[gnu::noinline]]
static void execcase(Shell *sh, Node *n, Arena scratch)
{
    sh->line    = n->line;
    sh->ctlline = n->line;
    if (sh->traps && debugheader(sh, concat3(&scratch, "case ", n->words->text, " in "), n->line)) {
        return;
    }
    if (sh->opt[O_XTRACE]) {
        Str msg = concat3(&scratch, "case ", n->words->text, " in");
        traceline(sh, scratch, msg);
    }
    Str word = expandstring(sh, &scratch, n->words->text, wordctx(sh, n->words));
    b32 ran  = 0;
    for (Case *c = n->cases; c; c = c->next) {
        if (!casematch(sh, &scratch, word, c->patterns)) continue;
        ran = 1;
        for (;;) {
            if (!c->body) sh->status = 0;  // an empty body succeeds
            execute(sh, c->body, scratch);
            if (c->term==CASE_FALLTHROUGH && c->next && !sh->flow) {
                c = c->next;
                continue;
            }
            break;
        }
        if (c->term != CASE_TESTNEXT || sh->flow) break;
    }
    if (!ran) sh->status = 0;
}

// The DEBUG trap before (( )) and [[ ]], which BASH_COMMAND names.
static b32 debugcompound(Shell *sh, Node *n, Arena scratch)
{
    if (!sh->intrap) sh->cmdnode = n;
    b32 skip = rundebugtrap(sh, scratch);
    sh->line = n->line;
    return skip;
}

[[gnu::noinline]]
static void execarith(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    if (sh->traps && debugcompound(sh, n, scratch)) return;
    Str text = expandstring(sh, &scratch, n->arith->text, XC_ARITH|XC_DQUOTE);
    if (sh->opt[O_XTRACE]) {
        Str msg = concat3(&scratch, "(( ", text, " ))");
        traceline(sh, scratch, msg);
    }
    Arith r = evalarithcmd(sh, &scratch, text, "((");
    sh->status = r.ok ? !r.value : 1;
    setpstat1(sh);
    checkerrexit(sh);
}

[[gnu::noinline]]
static void execcond(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    if (sh->traps && debugcompound(sh, n, scratch)) return;
    sh->status = condeval(sh, &scratch, n->cond);
    setpstat1(sh);
    checkerrexit(sh);
}

// Groups, if branches and the right sides of lists are followed in a
// loop rather than by recursion, so that each level of shell function
// calls costs as little stack as possible.
static void executenode(Shell *sh, Node *n, Arena scratch)
{
    for (;;) {
        if (!n || sh->exiting || sh->flow) {
            return;
        }
        if (n->flags & CMD_TIME) {
            execute(sh, n, scratch);
            return;
        }
        if (n->redirs || (n->flags & CMD_INVERT)) {
            break;
        }
        switch (n->kind) {
        case N_SIMPLE:
            checkstack(sh);
            runsimple(sh, n, scratch);
            setpstat1(sh);
            return;
        case N_GROUP:
            n = n->a;
            continue;
        case N_IF:
            sh->noerrexit++;
            execute(sh, n->a, scratch);
            sh->noerrexit--;
            if (sh->flow || sh->exiting) return;
            if (!sh->status) {
                n = n->b;
            } else if (n->c) {
                n = n->c;
            } else {
                sh->status = 0;
                return;
            }
            continue;
        case N_CONNECT:
            switch (n->connector) {
            case '|':
            case '&':
                break;
            case T_ANDAND:
            case T_OROR:
                sh->noerrexit++;
                execute(sh, n->a, scratch);
                sh->noerrexit--;
                if (sh->flow || sh->exiting) return;
                if ((n->connector==T_ANDAND) != (sh->status==0)) return;
                n = n->b;
                continue;
            default:
                execute(sh, n->a, scratch);
                listnotify(sh, scratch);
                reachtail(sh, n->b);
                n = n->b;
                continue;
            }
            break;
        }
        break;
    }
    executeone(sh, n, scratch);
}

static void executeone(Shell *sh, Node *n, Arena scratch)
{
    if (!n || sh->exiting || sh->flow) {
        return;
    }
    checkstack(sh);
    Undo *mark = sh->undo;
    b32   invert = n->flags & CMD_INVERT;
    if (invert) sh->noerrexit++;

    b32 stdinredir = sh->stdinredir;
    if (n->kind!=N_SIMPLE && n->kind!=N_CONNECT && n->kind!=N_FUNCDEF && n->redirs) {
        for (Redir *r = n->redirs; r; r = r->next) {
            sh->stdinredir |= r->src==0 && !r->srcvar;  // & reads it then
        }
        // As bash, a compound command's redirections are at its line
        // as only some commands set it: that of the enclosing for,
        // case, select or call, else where the parser finished the
        // top-level command
        sh->line = sh->ctlline;
        if (!doredirs(sh, n->redirs, &scratch, n==sh->keepredirs)) {
            sh->stdinredir = stdinredir;
            unwind(sh, mark);
            sh->status = sh->redirfail;
            if (invert) sh->noerrexit--;
            else checkerrexit(sh);
            return;
        }
    }

    switch (n->kind) {
    case N_SIMPLE:
        runsimple(sh, n, scratch);
        setpstat1(sh);
        break;

    case N_CONNECT:
        switch (n->connector) {
        case T_ANDAND:
        case T_OROR:
            sh->noerrexit++;
            execute(sh, n->a, scratch);
            sh->noerrexit--;
            if (sh->flow || sh->exiting) break;
            if ((n->connector==T_ANDAND) == (sh->status==0)) {
                execute(sh, n->b, scratch);
            }
            break;
        case '|':
            runpipeline(sh, n, scratch);
            break;
        case '&':
            runasync(sh, n->a, scratch);
            execute(sh, n->b, scratch);
            break;
        default:
            execute(sh, n->a, scratch);
            listnotify(sh, scratch);
            reachtail(sh, n->b);
            execute(sh, n->b, scratch);
        }
        break;

    case N_FUNCDEF:
        execfuncdef(sh, n, scratch);
        break;

    case N_GROUP:
        execute(sh, n->a, scratch);
        break;

    case N_SUBSHELL:
        execsubshell(sh, n, scratch);
        break;

    case N_IF:
        sh->noerrexit++;
        execute(sh, n->a, scratch);
        sh->noerrexit--;
        if (sh->flow || sh->exiting) break;
        if (!sh->status) {
            execute(sh, n->b, scratch);
        } else if (n->c) {
            execute(sh, n->c, scratch);
        } else {
            sh->status = 0;
        }
        break;

    case N_WHILE:
    case N_UNTIL: {
        i32 status = 0;
        sh->loopdepth++;
        for (;;) {
            Arena it = scratch;
            if (loopsafepoint(sh, it)) break;
            sh->noerrexit++;
            execute(sh, n->a, it);
            sh->noerrexit--;
            if (sh->flow) {
                if (loopflow(sh)) break;
                continue;
            }
            if (sh->exiting) break;
            if ((sh->status==0) != (n->kind==N_WHILE)) break;
            execute(sh, n->b, it);
            status = sh->status;
            if (sh->flow && loopflow(sh)) break;
            if (sh->exiting) break;
        }
        sh->loopdepth--;
        sh->status = status;
    } break;

    case N_SELECT: {
        i32 ctlline = sh->ctlline;  // theirs only while they run
        runselect(sh, n, scratch);
        sh->ctlline = ctlline;
    } break;

    case N_FOR: {
        i32 ctlline = sh->ctlline;
        execfor(sh, n, scratch);
        sh->ctlline = ctlline;
    } break;

    case N_ARITHFOR:
        execarithfor(sh, n, scratch);
        break;

    case N_CASE: {
        i32 ctlline = sh->ctlline;
        execcase(sh, n, scratch);
        sh->ctlline = ctlline;
    } break;

    case N_ARITH:
        execarith(sh, n, scratch);
        break;

    case N_COND:
        execcond(sh, n, scratch);
        break;

    case N_COPROC:
        runcoproc(sh, n, scratch);
        break;
    }

    if (n->kind!=N_SIMPLE && n->kind!=N_CONNECT && n->kind!=N_COPROC) {
        unwind(sh, mark);  // a coproc's descriptors stay open
    }
    sh->stdinredir = stdinredir;
    if (n->kind==N_SUBSHELL && n->redirs) {
        reportchild(sh, scratch, n);
    }
    if (invert) {
        sh->noerrexit--;
        if (sh->flow != FLOW_RETURN) {  // return leaves before inverting
            sh->status = !sh->status;
        }
    }
}

// Write select's numbered menu to standard error, as bash lays it out:
// columns when a single row would not do, padded relative to each
// column's own start.
static void selectmenu(Shell *sh, Arena scratch, Slice<Str> list)
{
    iz maxlen = 0;
    for (iz i = 0; i < list.len; i++) {
        maxlen = list[i].len>maxlen ? list[i].len : maxlen;
    }
    i32 width = 1;
    for (iz v = list.len; v >= 10; v /= 10) width++;
    iz colwidth = maxlen + width + 4;  // ") " and two spaces

    i64 columns = 80;
    Got g = getvar(sh, &scratch, "COLUMNS");
    if (!g.set || !parsei64(g.value, &columns) || columns<=0) columns = 80;
    iz cols = (iz)(columns / colwidth);
    cols = cols ? cols : 1;
    iz rows = list.len/cols + (list.len%cols != 0);
    cols = list.len/rows + (list.len%rows != 0);
    b32 single = rows == 1;
    if (single) {
        rows = cols;
        cols = 1;
    }

    Buf b(&scratch, 256);
    for (iz row = 0; row < rows; row++) {
        for (iz i = row, col = 0; i < list.len; i += rows, col++) {
            iz start = b.len;
            Str num = fmti64(&scratch, i+1);
            // bash leaves the first of several columns unpadded
            for (iz k = num.len; k<width && (cols==1 || col); k++) putbyte(&b, ' ');
            print(&b, num);
            print(&b, ") ");
            print(&b, list[i]);
            if (i+rows >= list.len) break;
            for (iz pos = b.len-start; pos < colwidth;) {
                if (colwidth/8 > pos/8) {
                    putbyte(&b, '\t');
                    pos += 8 - pos%8;
                } else {
                    putbyte(&b, ' ');
                    pos++;
                }
            }
        }
        putbyte(&b, '\n');
    }
    fdwrite(sh, 2, finish(&b));
}

// select: show the menu and prompt on standard error, read a line into
// REPLY, and run the body with the chosen word, until end of input.
static void runselect(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    if (sh->traps) {
        Buf b(&scratch, 64);
        print(&b, "select ");
        print(&b, n->name->text);
        print(&b, " in");
        if (!n->words && !n->inlist) print(&b, " \"$@\"");
        for (Word *w = n->words; w; w = w->next) {
            putbyte(&b, ' ');
            print(&b, w->text);
        }
        if (debugheader(sh, finish(&b), n->line)) return;
    }
    Slice<Str> list = {};
    if (n->words) {
        list = expandwords(sh, &scratch, n->words, 0);
    } else if (!n->inlist) {
        list.data = sh->args.v;
        list.len  = sh->args.n;
    }
    Str var = n->name->text;
    if (!validvarname(var)) {
        // As bash, which checks before it sets its line: the error cites
        // the line of the enclosing for, case, call or top-level command
        sh->line = sh->ctlline;
        Str msg = concat3(&scratch, "`", var, "': not a valid identifier");
        shellerror(sh, scratch, msg);
        sh->line = n->line;
        sh->status = 1;
        posixbadvar(sh);
        return;
    }
    sh->ctlline = n->line;
    if (!list.len) {
        sh->status = 0;
        return;
    }
    sh->status = 0;
    sh->loopdepth++;
    b32 menu = 1;
    for (;;) {
        Arena it = scratch;
        if (loopsafepoint(sh, it)) break;
        if (menu) selectmenu(sh, it, list);
        Got ps3 = getvar(sh, &it, "PS3");
        fdwrite(sh, 2, ps3.set ? ps3.value : Str("#? "));
        Buf line(&it, 64);
        u8  c   = 0;
        b32 eof = 0;
        while (!(eof = fdread(sh, 0, &c, 1) != 1) && c != '\n') putbyte(&line, c);
        if (eof && !line.len) {
            fdwrite(sh, 1, "\n");  // as bash, on standard output
            sh->status = 1;
            break;
        }
        Str reply = finish(&line);
        setvar(sh, it, "REPLY", reply);
        menu = !trim(reply).len;
        if (menu) continue;
        i64 choice = 0;
        b32 ok = parsei64(trim(reply), &choice) && choice>=1 && choice<=list.len;
        if (!setvar(sh, it, var, ok ? list[(iz)choice-1] : Str(""))) {
            sh->status = 1;
            if (posixfatal(sh)) escape(sh, ESC_FATAL);  // as bash
            break;
        }
        execute(sh, n->b, it);
        if (sh->flow && loopflow(sh)) break;
        if (sh->exiting) break;
    }
    sh->loopdepth--;
}

// Running source text

// Run one complete command from source text. An expansion error
// abandons it, and execution resumes with the next command unless this
// is a subshell, which it ends. Reading a script file (reader) also
// resumes after an error that abandons all -c, eval and source text.
static void runcommand(Shell *sh, Node *cmd, Arena scratch, b32 reader)
{
    Escape e = {};
    e.up        = sh->esc;
    e.bout      = sh->bout;
    e.undo      = sh->undo;
    e.frame     = sh->frame;
    e.loopdepth = sh->loopdepth;
    BOut  *bout      = sh->bout;
    Lexer *lexer     = sh->lexer;
    Str    iname     = sh->iname;
    Str    srcname   = sh->srcname;
    i32    noerrexit = sh->noerrexit;
    i32    evaldepth = sh->evaldepth;
    i32    srcdepth  = sh->sourcedepth;
    i32    capdepth  = sh->capdepth;
    sh->esc = &e;
    if (!jmpset(e.jmp)) {
        execute(sh, cmd, scratch);
        sh->esc = e.up;
        return;
    }
    sh->esc = e.up;
    b32 resume = e.why==ESC_DISCARD || (e.why==ESC_ABORT && reader) ||
                 (e.why==ESC_INTR && reader);
    if (!resume || sh->subshell) {
        if (e.why==ESC_DISCARD && !reader) sh->status = 1;
        escape(sh, e.why);
    }
    unwind(sh, e.undo);
    sh->status      = reader && sh->status ? sh->status : 1;
    sh->flow        = FLOW_NONE;
    sh->loopdepth   = e.loopdepth;
    sh->bout        = bout;
    sh->lexer       = lexer;
    sh->iname       = iname;
    sh->srcname     = srcname;
    sh->noerrexit   = noerrexit;
    sh->evaldepth   = evaldepth;
    sh->sourcedepth = srcdepth;
    sh->capdepth    = capdepth;
    sh->current     = 0;
}

// A $"..." string the parser found under -D or --dump-po-strings,
// printed as bash does: as written, or as a po file entry citing the
// input and line.
static void dumpstring(Shell *sh, Arena scratch, Str s, i32 line)
{
    Buf b(&scratch, 2*s.len+64);
    if (!(sh->dumpstrings & 2)) {
        putbyte(&b, '"');
        print(&b, s);
        print(&b, "\"\n");
        fdwrite(sh, 1, finish(&b));
        return;
    }
    print(&b, "#: ");
    print(&b, sh->iname);
    putbyte(&b, ':');
    print(&b, (i64)line);
    print(&b, "\nmsgid ");
    b32 nl = 0;
    for (iz i = 0; i < s.len; i++) nl |= s[i] == '\n';
    if (nl) print(&b, "\"\"\n");
    putbyte(&b, '"');
    for (iz i = 0; i < s.len; i++) {
        if (s[i] == '\n') {
            print(&b, "\\n\"\n\"");
            continue;
        }
        if (s[i]=='"' || s[i]=='\\') putbyte(&b, '\\');
        putbyte(&b, s[i]);
    }
    print(&b, "\"\nmsgstr \"\"\n");
    fdwrite(sh, 1, finish(&b));
}

// Parse and run src one command at a time, as -c, eval, source and
// command substitution do, numbering its lines from line. Returns the
// final status. With comsub, src is the printed text of a $(...) or
// ${ ...; } body, whose aliases the parser expanded only in POSIX mode:
// as in bash, it is parsed again with aliases only outside POSIX mode,
// until a command changes expand_aliases.
static i32 runsource(Shell *sh, Str src, Str iname, Arena scratch, b32 reader, i32 line,
                     b32 (*fill)(void *, Lexer *), void *fillctx, b32 comsub, b32 hist)
{
    byte *beg = scratch.beg;
    byte *end = scratch.mem ? scratch.mem->limit : scratch.end;
    byte *floor = scratch.mem ? scratch.mem->commit : scratch.end;
    src = dropcr(&scratch, src, crmode(sh));
    Lexer *L = newlexer(&scratch, src);
    L->fill          = fill;
    L->fillctx       = fillctx;
    L->newunit       = 1;
    L->lineno        = line - 1;
    L->aliases       = &sh->aliases;
    L->stackmax      = (iz)1<<20;
    L->expandaliases = L->aliasopt = sh->shopt[S_EXPAND_ALIASES];
    L->extglob       = L->extglobopt = sh->shopt[S_EXTGLOB];
    L->posix         = sh->opt[O_POSIX];
    L->extquote      = sh->shopt[S_EXTQUOTE];
    L->utf8          = sh->utf8;
    if ((reader || hist) && !comsub) {
        // The lines it reads go into the history: the reader's, and -c's
        // once it turns history on
        L->hist    = histline;
        L->histctx = sh;
    }
    if (comsub && L->posix) {
        L->expandaliases = 0;
    }
    Lexer *outer = sh->lexer;
    Str    oname = sh->iname;
    sh->lexer  = L;
    sh->iname  = iname;
    sh->lexeof = 0;
    L->dump    = sh->dumpstrings;
    b32 prettynl = 0;
    for (;;) {
        Arena unit = *L->a;  // reset after each complete line
        iz     pos  = L->srcpos;
        if (sh->lexeof) {
            resumeaftereof(L);  // a nested parse ended this way
        }
        L->cmdlines = 0;
        if (L->hist) sh->hist.cmdlines = 0;
        // With history, set -v echoes the lines as expanded
        L->echoing = L->hist && sh->opt[O_VERBOSE];
        if (L->echoing) L->echo = Buf(L->a, 128);
        Parsed r = parseunit(L);
        if (r.status != PARSE_EOF) {
            sh->lexeof = L->current == T_EOF;
        }
        if (sh->opt[O_VERBOSE] && L->srcpos>pos) {
            // set -v: echo the input lines as they are read, each
            // with a newline as the lexer sees it
            Str text = span(L->src.data+pos, L->src.data+L->srcpos);
            if (L->echoing) text = finish(&L->echo);
            if (text.len) fdwrite(sh, 2, text);
            if (text.len && text[text.len-1]!='\n') fdwrite(sh, 2, "\n");
        }
        // Report parser messages. As bash's parser_error, a shell reading
        // interactively (at the prompt or in eval, not a sourced file or
        // PROMPT_COMMAND) names no line, nor quotes the line in error.
        Arena msgarena = *L->a;
        b32   noline   = sh->interactive && !sh->sourcedepth && !(iname=="PROMPT_COMMAND");
        for (Message *m = L->msgs; m; m = m->next) {
            if (noline && m->kind==MSG_LINE) continue;
            if (m->kind == MSG_DUMP) {
                dumpstring(sh, msgarena, m->text, m->line);
                continue;
            }
            Buf b(&msgarena, 128);
            print(&b, errname(sh));
            if (noline) {
                print(&b, ": ");
                if (m->kind == MSG_WARNING) print(&b, "warning: ");
            } else if (m->kind != MSG_WARNING) {
                if (!(errname(sh) == sh->iname)) {
                    print(&b, ": ");
                    print(&b, sh->iname);
                }
                print(&b, ": line ");
                print(&b, (i64)m->line);
                print(&b, ": ");
            } else {
                print(&b, ": line ");
                print(&b, (i64)m->line);
                print(&b, ": warning: ");
            }
            print(&b, m->text);
            putbyte(&b, '\n');
            fdwrite(sh, 2, finish(&b));
        }
        L->msgs    = 0;
        L->msgtail = &L->msgs;
        if (r.status==PARSE_EOF && sh->prettyprint && !outer && !prettynl) {
            fdwrite(sh, 1, "\n");
        }
        if (r.status == PARSE_EOF) break;
        if (r.status == PARSE_ERROR) {
            if (L->overflow) {
                sh->status = 139;
            } else if (L->comsuberr && (sh->evaldepth || sh->sourcedepth)) {
                // Like ${x?}, abandons the shell, or ends a subshell
                sh->lexer  = outer;
                sh->iname  = oname;
                sh->status = 1;
                escape(sh, ESC_FATAL);
            } else if (L->comsuberr && sh->iname=="-c") {
                sh->status = 127;
            } else if (L->reported || !sh->status) {
                sh->status = 2;
            }
            if (sh->prettyprint && !outer) sh->status = 1;  // as bash's
            if (sh->evaldepth || sh->sourcedepth) {
                sh->bifatal = 1;  // eval's or source's: fatal in POSIX mode
            }
            if (!sh->interactive && !sh->evaldepth && !sh->sourcedepth) {
                sh->exiting = 1;
            } else if (reader && fill && sh->interactive && !L->eofreached) {
                // At the prompt: the rest of the line goes, and the next
                // is read
                L->line    = {};
                L->idx     = 0;
                L->pushed  = 0;
                L->newunit = 1;
                *L->a      = unit;
                continue;
            } else if (reader && fill && sh->interactive) {
                // The input ended within a command: as at its start
                fdwrite(sh, 2, sh->login ? Str("logout\n") : "exit\n");
            }
            break;
        }
        if (r.status == PARSE_DISCARD) {
            if (L->intr) {
                // Ctrl-C at PS2: the status the interrupt set, and a
                // new command
                L->intr    = 0;
                L->line    = {};
                L->idx     = 0;
                L->newunit = 1;
                *L->a      = unit;
            } else {
                sh->status = 1;
            }
            continue;
        }
        if (sh->prettyprint && !outer) {
            // --pretty-print: each command as bash prints it, and a
            // blank line for a run of empty ones
            if (r.cmd) {
                Arena t = *L->a;
                Printer *p = newprinter(&t);
                p->posix    = 1;
                p->external = 1;
                printcmd(p, r.cmd);
                putbyte(&p->b, '\n');
                fdwrite(sh, 1, finish(&p->b));
                prettynl = 0;
            } else if (!prettynl) {
                fdwrite(sh, 1, "\n");
                prettynl = 1;
            }
        } else if (r.cmd && !sh->opt[O_NOEXEC]) {
            // As in bash, LINENO is where the parser stopped until a
            // command sets it: a compound command's redirections cite
            // its last line
            sh->line    = L->lineno;
            sh->ctlline = L->lineno;
            if (sh->jobs && !outer) notifyjobs(sh, *L->a);
            if (fill && sh->interactive) showps0(sh);
            if (reader) sh->cmdnumber++;
            // Traps for signals that came while the line was read, as
            // before every command in bash
            if (fill && sh->interactive) checksigs(sh, *L->a);
            runcommand(sh, r.cmd, *L->a, reader);
        }
        // Syncing options the command may have changed
        if (!comsub || L->aliasopt!=sh->shopt[S_EXPAND_ALIASES]) {
            L->expandaliases = L->aliasopt = sh->shopt[S_EXPAND_ALIASES];
            comsub = 0;
        }
        L->extglob       = L->extglobopt = sh->shopt[S_EXTGLOB];
        L->posix         = sh->opt[O_POSIX];
        L->extquote      = sh->shopt[S_EXTQUOTE];
        if (sh->exiting || sh->flow) break;
        if (!L->pushed && L->idx>=L->line.len && !L->nheredoc) {
            // Nothing refers to the unit's memory now: function
            // bodies were copied into the store
            keepcmdtext(sh, unit.beg, end);
            L->line = {};
            L->idx  = 0;
            L->newunit = 1;
            *L->a   = unit;
            // The outermost reader gives back what a big command took
            if (!outer) arenatrim(L->a, floor);
        }
    }
    keepcmdtext(sh, beg, end);
    sh->lexer = outer;
    sh->iname = oname;
    return sh->status;
}
