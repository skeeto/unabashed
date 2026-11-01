// Signals and traps: the trap and kill builtins, trap actions, and the
// delivery of signals to virtual processes
//
// Signal numbers and names are Linux x86-64's on every platform. Every
// virtual process (the shell, a synchronous subshell, a fiber vproc)
// has a Sigproc holding its pending signals, registered in its Sched
// by BASHPID so that kill finds it. Signals for the process itself
// (kill $$, or real ones from os_sigpending) go to the outermost
// level, which takes them only once it runs again, as a parent bash
// takes a signal once its foreground child is done. Pending signals
// are acted on at safe points (checksigs): after each simple command,
// between commands, after subshells and pipelines. A signal for a
// level blocked in a read or wait builtin interrupts it; one that
// will kill a level interrupts any pipe or OS wait.
//
// The trap table lives in the store, NTRAP entries: an action with
// null data is the default, an empty one ignores the signal. A
// subshell gets a table of its own with the caught signals reset and
// the ignored ones kept, while trap without arguments goes on listing
// its parent's table (Shell.shown) until the subshell changes one.
//
// This is free and unencumbered software released into the public domain.

// The names of signals 1-31; 32 and 33 have none, 34-64 are RTMIN+n
// and RTMAX-n.
static Str signames[32] = {
    "EXIT", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "BUS", "FPE",
    "KILL", "USR1", "SEGV", "USR2", "PIPE", "ALRM", "TERM", "STKFLT",
    "CHLD", "CONT", "STOP", "TSTP", "TTIN", "TTOU", "URG", "XCPU", "XFSZ",
    "VTALRM", "PROF", "WINCH", "IO", "PWR", "SYS",
};

// glibc's strsignal text, for reports of deaths by signal.
static Str sigdescs[32] = {
    "Unknown signal 0", "Hangup", "Interrupt", "Quit",
    "Illegal instruction", "Trace/breakpoint trap", "Aborted",
    "Bus error", "Floating point exception", "Killed",
    "User defined signal 1", "Segmentation fault", "User defined signal 2",
    "Broken pipe", "Alarm clock", "Terminated", "Stack fault",
    "Child exited", "Continued", "Stopped (signal)", "Stopped",
    "Stopped (tty input)", "Stopped (tty output)", "Urgent I/O condition",
    "CPU time limit exceeded", "File size limit exceeded",
    "Virtual timer expired", "Profiling timer expired", "Window changed",
    "I/O possible", "Power failure", "Bad system call",
};

static u64 sigbit(i32 sig)
{
    return (u64)1 << (sig-1);
}

// Signals whose default action does nothing here: ignored by default,
// and the stop signals, since stopping is not modelled.
static b32 sigharmless(i32 sig)
{
    switch (sig) {
    case 17: case 18: case 19: case 20: case 21: case 22: case 23: case 28:
        return 1;
    }
    return 0;
}

// A trap table index's name as trap -p prints it: SIGHUP, SIGRTMIN+1,
// EXIT, DEBUG. Empty for 32 and 33.
static Str trapname(Arena *a, i32 idx, b32 prefix)
{
    if (idx == TRAP_EXIT)   return "EXIT";
    if (idx == TRAP_DEBUG)  return "DEBUG";
    if (idx == TRAP_ERR)    return "ERR";
    if (idx == TRAP_RETURN) return "RETURN";
    Str name = {};
    if (idx < 32) {
        name = signames[idx];
    } else if (idx == 34) {
        name = "RTMIN";
    } else if (idx == 64) {
        name = "RTMAX";
    } else if (idx>34 && idx<50) {
        name = concat(a, "RTMIN+", fmti64(a, idx-34));
    } else if (idx>=50 && idx<64) {
        name = concat(a, "RTMAX-", fmti64(a, 64-idx));
    } else {
        return {};
    }
    return prefix ? concat(a, "SIG", name) : name;
}

static b32 strcaseeq(Str a, Str b)
{
    if (a.len != b.len) return 0;
    for (iz i = 0; i < a.len; i++) {
        if (lowercase(a[i]) != lowercase(b[i])) return 0;
    }
    return 1;
}

// Parse a signal spec: a number, or a name in any case with or without
// SIG. With pseudo, also EXIT, DEBUG, ERR and RETURN; EXIT and 0 are
// always accepted. Returns the trap table index, or -1.
static i32 parsesig(Str s, b32 pseudo)
{
    if (s.len && digit(s[0])) {
        i32 n = 0;
        for (iz i = 0; i < s.len; i++) {
            if (!digit(s[i]) || n>NSIGNAL) return -1;
            n = n*10 + (s[i] - '0');
        }
        return n<NSIGNAL ? n : -1;
    }
    if (strcaseeq(s, "EXIT")) return TRAP_EXIT;
    if (pseudo) {
        if (strcaseeq(s, "DEBUG"))  return TRAP_DEBUG;
        if (strcaseeq(s, "ERR"))    return TRAP_ERR;
        if (strcaseeq(s, "RETURN")) return TRAP_RETURN;
    }
    if (s.len>3 && strcaseeq(takehead(s, 3), "SIG")) s = cuthead(s, 3);
    for (i32 i = 1; i < 32; i++) {
        if (strcaseeq(s, signames[i])) return i;
    }
    if (s.len>=5 && (strcaseeq(takehead(s, 5), "RTMIN") || strcaseeq(takehead(s, 5), "RTMAX"))) {
        b32 max  = lowercase(s[4]) == 'x';
        Str rest = cuthead(s, 5);
        if (!rest.len) return max ? 64 : 34;
        if (rest[0] != (max ? '-' : '+') || rest.len==1) return -1;
        i32 n = 0;
        for (iz i = 1; i < rest.len; i++) {
            if (!digit(rest[i]) || n>NSIGNAL) return -1;
            n = n*10 + (rest[i] - '0');
        }
        i32 sig = max ? 64-n : 34+n;
        return sig>=34 && sig<=64 ? sig : -1;
    }
    return -1;
}


// The trap table

static b32 trapset(Shell *sh, i32 idx)  // an action, not ignored or default
{
    return sh->traps && sh->traps[idx].len;
}

static b32 trapignored(Shell *sh, i32 sig)
{
    if (sig>0 && sig<NSIGNAL && (sh->hardign & sigbit(sig))) return 1;
    if (sh->traps && sh->traps[sig].data) return !sh->traps[sig].len;
    return sig>0 && sig<NSIGNAL && (sh->asyncign & sigbit(sig));
}

// How the process should handle a real signal, per the main shell's
// table: caught when trapped, or when an EXIT trap should run first.
static void updatesigmode(Shell *sh, i32 sig)
{
    if (sh->subshell || sig<=0 || sig>=NSIGNAL) return;
    i32 mode = SIGM_DEFAULT;
    if (trapignored(sh, sig)) {
        mode = SIGM_IGNORE;
    } else if (trapset(sh, sig)) {
        mode = SIGM_CATCH;
    } else if (trapset(sh, TRAP_EXIT)) {
        switch (sig) {
        case 1: case 2: case 3: case 10: case 12: case 14: case 15:
        case 24: case 25: case 26: case 27:
            mode = SIGM_CATCH;  // run the EXIT trap, then die
        }
    }
    os_sigmode(sh->os, sig, mode);
}

// Set (cmd.data), ignore (empty cmd) or reset (null cmd) a trap.
static void settrap(Shell *sh, i32 idx, Str cmd)
{
    if (idx>0 && idx<NSIGNAL && (sh->hardign & sigbit(idx))) {
        return;  // ignored on entry: stays ignored
    }
    sh->shown = 0;  // a subshell lists its own table from now on
    if (!sh->traps) {
        if (!cmd.data) return;
        sh->traps = (Str *)salloc(sh->store, NTRAP*(iz)sizeof(Str));
    }
    sfreestr(sh->store, sh->traps[idx]);
    sh->traps[idx] = {};
    if (cmd.data) {
        sh->traps[idx] = sclone(sh->store, cmd);  // non-null even if empty
    }
    if (idx == TRAP_EXIT) {
        for (i32 sig = 1; sig < NSIGNAL; sig++) updatesigmode(sh, sig);
    } else {
        updatesigmode(sh, idx);
    }
}

// A subshell's table: the caught signals reset to their defaults and
// the ignored ones kept. ERR stays with errtrace, DEBUG and RETURN with
// functrace. Until it changes a trap, trap lists the parent's table.
static void subshelltraps(Shell *sh)
{
    Str *old = sh->traps;
    if (!old) return;
    Str *t = (Str *)salloc(sh->store, NTRAP*(iz)sizeof(Str));
    for (i32 i = 1; i < NSIGNAL; i++) {
        if (old[i].data && !old[i].len) t[i] = old[i];
    }
    if (sh->opt[O_ERRTRACE]) t[TRAP_ERR] = old[TRAP_ERR];
    if (sh->opt[O_FUNCTRACE]) {
        t[TRAP_DEBUG]  = old[TRAP_DEBUG];
        t[TRAP_RETURN] = old[TRAP_RETURN];
    }
    if (!sh->shown) sh->shown = old;
    sh->traps = t;
}

// The same for a fiber vproc, copying everything into its own store.
static void copytraps(Shell *c, Shell *sh)
{
    Store *st = c->store;
    c->traps = c->shown = 0;
    Str *list = sh->shown ? sh->shown : sh->traps;
    if (list) {
        c->shown = (Str *)salloc(st, NTRAP*(iz)sizeof(Str));
        for (i32 i = 0; i < NTRAP; i++) {
            c->shown[i] = sclone(st, list[i]);  // an empty one stays non-null
        }
    }
    if (sh->traps) {
        Str *saved = sh->traps;
        Str *t = (Str *)salloc(st, NTRAP*(iz)sizeof(Str));
        for (i32 i = 0; i < NTRAP; i++) {
            t[i] = sclone(st, saved[i]);
        }
        c->traps = t;
        Str *shown = c->shown;
        c->shown = 0;
        subshelltraps(c);
        c->shown = shown;
    }
}


// Running trap actions

// Run a trap's action as eval would, keeping $? (unless it returns
// from a function), the current line and the command being run.
// Returns the action's own status.
static i32 runtrap(Shell *sh, i32 idx)
{
    if (sh->intrap>=10 || sh->capdepth+2>=NPOOL) {
        // Actions nested this deep (a trap sending its own signal):
        // bash recurses until it crashes. Each level holds an arena.
        sh->status  = 139;
        sh->exiting = 1;
        escape(sh, ESC_EXIT);
    }
    i32   depth = ++sh->capdepth;
    Arena a     = *scratchat(sh, depth);
    Str   cmd   = clone(&a, sh->traps[idx]);  // it may change itself
    i32   status   = sh->status;
    i32   saved    = sh->trapstatus;
    i32   line     = sh->line;
    Node *current  = sh->current;
    Node *cmdnode  = sh->cmdnode;
    // BASH_COMMAND once its node was freed: the store copy may be
    // replaced while the action runs (keepcmdtext)
    Str   cmdtext_ = cmdnode ? Str{} : clone(&a, sh->cmdtext);
    i32   funcs    = sh->trapfuncs;
    sh->trapstatus = status;
    sh->trapfuncs  = sh->funcdepth;
    sh->intrap++;
    i32   busy  = sh->trapbusy;
    if (idx >= TRAP_DEBUG) sh->trapbusy |= 1 << (idx-TRAP_DEBUG);
    // BASH_TRAPSIG: the trap's number, while it runs
    pushtemp(sh, a, "BASH_TRAPSIG", fmti64(&a, idx));
    Value *v = lookup(sh, "BASH_TRAPSIG");
    if (v) v->attrs &= ~V_EXPORT;
    sh->evaldepth++;
    runsource(sh, cmd, sh->iname, a, 0, sh->line);
    i32 own = sh->status;
    sh->evaldepth--;
    poptemp(sh, "BASH_TRAPSIG");
    sh->capdepth = depth - 1;
    sh->trapbusy = busy;
    sh->intrap--;
    sh->trapstatus = saved;
    sh->trapfuncs  = funcs;
    sh->line    = line;
    sh->current = current;
    sh->cmdnode = cmdnode;
    if (!cmdnode && cmdtext_.data) {
        Store *st = sh->store;
        if ((byte *)sh->cmdtext.data >= st->mark) sfreestr(st, sh->cmdtext);
        sh->cmdtext = sclone(st, cmdtext_);
    }
    if (sh->flow != FLOW_RETURN) {
        sh->status = status;
    }
    return own;
}

// Die by a signal: the EXIT trap runs first (except for SIGKILL), then
// the level exits with 128+sig, reported as a death by that signal.
[[noreturn]] static void sigdie(Shell *sh, i32 sig, Arena scratch)
{
    if (sig != 9) runexittrap(sh, scratch);
    sh->killsig = sig;
    sh->status  = 128 + sig;
    sh->exiting = 1;
    escape(sh, ESC_EXIT);
}

// Act on this level's pending signals, lowest first.
static void runsigs(Shell *sh, Arena scratch)
{
    Sigproc *p = sh->sig;
    while (p->pending) {
        i32 sig = __builtin_ctzll(p->pending) + 1;
        p->pending &= ~sigbit(sig);
        if (trapignored(sh, sig)) continue;
        if (sh->traps && sh->traps[sig].data) {
            runtrap(sh, sig);
            continue;
        }
        if (!sigharmless(sig)) sigdie(sh, sig, scratch);
    }
}

// Take the signals that arrived from outside the process: they belong
// to the outermost level.
static void takeossigs(Shell *sh)
{
    u64 m = os_sigpending(sh->os);
    if (m) sh->sched->top->pending |= m;
}

// A safe point for signals.
static void checksigs(Shell *sh, Arena scratch)
{
    takeossigs(sh);
    if (sh->sig->pending) runsigs(sh, scratch);
}

// After a wait that failed, perhaps because a signal interrupted it:
// a signal that kills this level does so now. True if a signal was
// the cause.
static b32 sigtaken(Shell *sh)
{
    Task *t = sh->sched->cur;
    b32 intr = t->intr;
    t->intr = 0;
    u64 m = os_sigpending(sh->os);
    if (m) {
        sh->sched->top->pending |= m;
        intr = 1;
    }
    if (!intr) return 0;
    u64 pend = sh->sig->pending;
    for (u64 m2 = pend; m2; m2 &= m2-1) {
        i32 sig = __builtin_ctzll(m2) + 1;
        if (!trapignored(sh, sig) && !trapset(sh, sig) && !sigharmless(sig)) {
            sh->sig->pending &= ~sigbit(sig);
            Arena a = *scratchat(sh, sh->capdepth+1);
            sigdie(sh, sig, a);
        }
    }
    return 1;
}

// Send a signal to a virtual process of this shell process.
static void sigsend(Shell *sh, Sigproc *p, i32 sig)
{
    if (!sig) return;
    p->pending |= sigbit(sig);
    Shell *target = p->sh;
    if (target->sig != p) return;  // an outer level, busy with its child
    if (trapignored(target, sig) || (!target->traps || !target->traps[sig].data)) {
        if (trapignored(target, sig) || sigharmless(sig)) {
            p->pending &= ~sigbit(sig);
            return;
        }
        // It will die: fail whatever it waits for, except a task
        if (p->task && p->task!=sh->sched->cur) taskinterrupt(sh->sched, p->task, 1);
        return;
    }
    if (p->task && p->task!=sh->sched->cur) {
        taskinterrupt(sh->sched, p->task, 0);  // a read or wait builtin
    }
}

static Sigproc *findsigproc(Sched *s, i64 pid)
{
    for (Sigproc *p = s->sigprocs; p; p = p->next) {
        if (p->pid == pid) return p;
    }
    return 0;
}

static void sigenter(Shell *sh, Sigproc *p, i64 pid, Task *task)
{
    Sched *s = sh->sched;
    p->pid     = pid;
    p->pending = 0;
    p->task    = task;
    p->sh      = sh;
    p->next    = s->sigprocs;
    s->sigprocs = p;
    sh->sig    = p;
}

static void sigleave(Shell *sh, Sigproc *p)
{
    Sched *s = sh->sched;
    for (Sigproc **q = &s->sigprocs; *q; q = &(*q)->next) {
        if (*q == p) {
            *q = p->next;
            break;
        }
    }
}

// Run the EXIT trap, once, as the shell or a subshell that set one
// exits. Its own exit chooses the status; otherwise the status is the
// one it started with.
static void runexittrap(Shell *sh, Arena)
{
    if (!trapset(sh, TRAP_EXIT)) return;
    // A fresh arena: the caller's may hold the commands just run, to
    // which BASH_COMMAND may still point (so keep its text first)
    keepcmdtext(sh, 0, (byte *)-1);
    Arena scratch = *scratchat(sh, sh->capdepth+1);
    Str cmd = clone(&scratch, sh->traps[TRAP_EXIT]);
    sfreestr(sh->store, sh->traps[TRAP_EXIT]);
    sh->traps[TRAP_EXIT] = {};
    i32 status = sh->status;
    Escape e = {};
    e.up    = sh->esc;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    sh->exiting = 0;
    sh->flow    = FLOW_NONE;
    sh->trapstatus = status;
    sh->trapfuncs  = sh->funcdepth;
    sh->intrap++;
    pushtemp(sh, scratch, "BASH_TRAPSIG", "0");
    Value *v = lookup(sh, "BASH_TRAPSIG");
    if (v) v->attrs &= ~V_EXPORT;
    if (!__builtin_setjmp(e.jmp)) {
        runsource(sh, cmd, sh->iname, scratch, 0, sh->line);
        sh->status = status;
    }
    poptemp(sh, "BASH_TRAPSIG");
    sh->intrap--;
    sh->esc     = e.up;
    sh->exiting = 1;
}

// Should DEBUG, ERR or RETURN run now? Each runs once at a time, and in
// a function only if the function set it, or with functrace (errtrace
// for ERR): a caller's is not inherited.
static b32 pseudotrap(Shell *sh, i32 idx, b32 opt)
{
    i32 k = idx - TRAP_DEBUG;
    if (!trapset(sh, idx) || (sh->trapbusy & 1<<k)) return 0;
    return !sh->funcdepth || opt || !sh->frame || sh->traps[idx].data!=sh->frame->trapat[k];
}

// ERR: after a command fails where set -e would act. Subshells reset
// it without errtrace.
static void runerrtrap(Shell *sh)
{
    if (pseudotrap(sh, TRAP_ERR, sh->opt[O_ERRTRACE])) runtrap(sh, TRAP_ERR);
}

// DEBUG: before a simple command, (( )), [[ ]] or a for iteration.
// Under extdebug, a non-zero status from the action skips the command,
// and 2 in a function or sourced file also returns from it. Returns
// whether to skip.
static b32 rundebugtrap(Shell *sh, Arena)
{
    if (!pseudotrap(sh, TRAP_DEBUG, sh->opt[O_FUNCTRACE])) return 0;
    i32 own = runtrap(sh, TRAP_DEBUG);
    if (!sh->shopt[S_EXTDEBUG] || !own || sh->flow) return 0;
    if (own==2 && (sh->funcdepth || sh->sourcedepth)) {
        sh->flow   = FLOW_RETURN;
        sh->status = 2;
    }
    return 1;
}

// RETURN: as a function or a sourced file returns.
static void runreturntrap(Shell *sh, Arena)
{
    if (!pseudotrap(sh, TRAP_RETURN, sh->opt[O_FUNCTRACE])) return;
    i32 flow = sh->flow;
    sh->flow = FLOW_NONE;
    runtrap(sh, TRAP_RETURN);
    if (!sh->flow) sh->flow = flow;
}

// Does a report of a death by sig name the job's pid and line? Not for
// SIGTERM, nor for a signal this shell traps or ignores itself.
static b32 longreport(Shell *sh, i32 sig)
{
    return sig!=15 && !trapignored(sh, sig) && !(sh->traps && sh->traps[sig].data);
}

// Report a foreground job killed by a signal, as bash does for any but
// SIGINT and SIGPIPE: the signal's description and the command, after
// the line and the pid unless longreport says otherwise.
static void reportsig(Shell *sh, Arena scratch, i32 sig, i64 pid, Str text)
{
    if (sig==2 || sig==13 || sig<=0 || sig>=NSIGNAL || sh->noreport) return;
    Str desc = sig<32 ? sigdescs[sig]
             : sig<34 ? concat(&scratch, "Unknown signal ", fmti64(&scratch, sig))
             : concat(&scratch, "Real-time signal ", fmti64(&scratch, sig-34));
    flushbout(sh);
    Buf b(&scratch, 128 + text.len);
    if (longreport(sh, sig)) {
        msgprefix(sh, &b);
        Str p = fmti64(&scratch, pid);
        for (iz i = p.len; i < 5; i++) putbyte(&b, ' ');
        print(&b, p);
        putbyte(&b, ' ');
    }
    print(&b, desc);
    for (iz i = desc.len; i < 27; i++) putbyte(&b, ' ');
    print(&b, text);
    putbyte(&b, '\n');
    fdwrite(sh, 2, finish(&b));
}


// Builtins

// "trap -- 'cmd' SIGNAME", with the action quoted as bash does.
static void printtrap(Shell *sh, Arena scratch, Str cmd, i32 idx)
{
    Buf b(&scratch, cmd.len + 32);
    print(&b, "trap -- '");
    for (iz i = 0; i < cmd.len; i++) {
        if (cmd[i] == '\'') {
            print(&b, "'\\''");
        } else {
            putbyte(&b, cmd[i]);
        }
    }
    print(&b, "' ");
    Str name = trapname(&scratch, idx, 1);
    print(&b, name.len ? name : fmti64(&scratch, idx));
    putbyte(&b, '\n');
    bprint(sh, finish(&b));
}

// The table trap lists: a subshell's parent's, until it changes one.
static Str *listedtraps(Shell *sh)
{
    return sh->shown ? sh->shown : sh->traps;
}

// kill -l and trap -l: the numbered table, five to a row.
static void siglisting(Shell *sh, Arena scratch)
{
    i32 col = 0;
    for (i32 sig = 1; sig < NSIGNAL; sig++) {
        if (sig==32 || sig==33) continue;
        Buf b(&scratch, 32);
        Str n = fmti64(&scratch, sig);
        if (n.len < 2) putbyte(&b, ' ');
        print(&b, n);
        print(&b, ") ");
        print(&b, trapname(&scratch, sig, 1));
        print(&b, ++col%5 ? "\t" : "\n");
        bprint(sh, finish(&b));
    }
    if (col % 5) bprint(sh, "\n");
}

static i32 b_trap(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 list = 0, print_ = 0, action = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") {
            i++;
            break;
        }
        for (iz k = 1; k < a.len; k++) {
            switch (a[k]) {
            case 'l': list   = 1; break;
            case 'p': print_ = 1; break;
            case 'P': action = 1; break;
            default:
                invalidoption(sh, scratch, "trap", span(a.data+k-1, a.data+k+1));
                return 2;
            }
        }
    }
    if (list) {
        siglisting(sh, scratch);
        return 0;
    }
    if (action && print_) {
        builtinerror(sh, scratch, "trap", "cannot specify both -p and -P");
        return 2;
    }
    if (action && i==nargs) {
        builtinerror(sh, scratch, "trap", "-P requires at least one signal name");
        return 2;
    }
    i32 status = 0;
    if (print_ || action || i==nargs) {
        Str *t = listedtraps(sh);
        if (i == nargs) {
            for (i32 idx = 0; idx < NTRAP; idx++) {
                if (t && t[idx].data) printtrap(sh, scratch, t[idx], idx);
            }
            return 0;
        }
        for (; i < nargs; i++) {
            i32 idx = parsesig(args[i], 1);
            if (idx < 0) {
                builtinerror(sh, scratch, "trap", concat(&scratch, args[i], ": invalid signal specification"));
                status = 1;
            } else if (t && t[idx].data) {
                if (action) {
                    bprint(sh, t[idx]);
                    bprint(sh, "\n");
                } else {
                    printtrap(sh, scratch, t[idx], idx);
                }
            }
        }
        return status;
    }

    // An action and signals, or one signal to reset
    Str cmd    = args[i];
    b32 number = cmd.len > 0;
    for (iz k = 0; k < cmd.len; k++) number &= digit(cmd[k]);
    b32 reset  = 1;
    if (i+1 == nargs) {
        // One operand: a signal to reset
        if (parsesig(cmd, 1) < 0) {
            usageerror(sh, scratch, "trap");
            return 2;
        }
    } else if (cmd == "-") {
        i++;
    } else if (!number) {
        reset = 0;
        i++;
    }  // else POSIX: a leading unsigned number is a signal too
    for (; i < nargs; i++) {
        i32 idx = parsesig(args[i], 1);
        if (idx < 0) {
            builtinerror(sh, scratch, "trap", concat(&scratch, args[i], ": invalid signal specification"));
            status = 1;
            continue;
        }
        Str c = {};
        if (!reset) {
            c = cmd;
            if (!c.data) c.data = (u8 *)args;  // empty: ignore
        }
        settrap(sh, idx, c);
    }
    return status;
}

// kill -l with arguments: names for numbers (an exit status above 128
// names its signal), numbers for names.
static i32 killlist(Shell *sh, Arena scratch, Str *args, iz nargs)
{
    i32 status = 0;
    for (iz i = 0; i < nargs; i++) {
        Str a = args[i];
        if (a.len && digit(a[0])) {
            i64 n = 0;
            b32 ok = 1;
            for (iz k = 0; k < a.len; k++) {
                ok &= digit(a[k]);
                if (n < 100000) n = n*10 + (a[k] - '0');
            }
            if (ok && n>128) n -= 128;
            if (!ok || n>=NSIGNAL) {
                builtinerror(sh, scratch, "kill", concat(&scratch, a, ": invalid signal specification"));
                status = 1;
                continue;
            }
            Str name = trapname(&scratch, (i32)n, 0);
            if (name.len) {
                bprint(sh, name);
                bprint(sh, "\n");
            }
            continue;
        }
        i32 sig = parsesig(a, 0);
        if (sig < 0) {
            builtinerror(sh, scratch, "kill", concat(&scratch, a, ": invalid signal specification"));
            status = 1;
            continue;
        }
        bprinti(sh, sig);
        bprint(sh, "\n");
    }
    return status;
}

// Send a signal to a pid: this process's own virtual processes first
// ($$ is the outermost level), else the platform's.
static i32 killpid(Shell *sh, i64 pid, i32 sig)
{
    Sched *s = sh->sched;
    Sigproc *p = pid==sh->pid ? s->top : findsigproc(s, pid);
    if (p) {
        sigsend(sh, p, sig);
        return 0;
    }
    return os_kill(sh->os, pid, sig);
}

static i64 jobspecpid(Shell *, Arena, Str who, Str spec);  // jobs.cpp

static i32 b_kill(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    i32 sig = 15;
    iz  i   = 1;
    if (i < nargs && (args[i]=="-l" || args[i]=="-L")) {
        i++;
        if (i<nargs && args[i]=="--") i++;
        if (i == nargs) {
            siglisting(sh, scratch);
            return 0;
        }
        return killlist(sh, scratch, args+i, nargs-i);
    }
    if (i < nargs && args[i].len>1 && args[i][0]=='-' && !(args[i]=="--")) {
        Str a = args[i];
        Str spec = {};
        if (a=="-s" || a=="-n") {
            if (i+1 == nargs) {
                builtinerror(sh, scratch, "kill", concat(&scratch, a, ": option requires an argument"));
                return 1;
            }
            spec = args[++i];
        } else if (a.len>2 && a[1]=='s') {
            spec = cuthead(a, 2);
        } else {
            spec = cuthead(a, 1);
        }
        sig = parsesig(spec, 0);
        if (sig < 0) {
            builtinerror(sh, scratch, "kill", concat(&scratch, spec, ": invalid signal specification"));
            return 1;
        }
        i++;
    }
    if (i<nargs && args[i]=="--") i++;
    if (i == nargs) {
        usageerror(sh, scratch, "kill");
        return 2;
    }
    i32 status = 0;
    for (; i < nargs; i++) {
        Str a = args[i];
        i64 pid;
        if (a.len && a[0]=='%') {
            pid = jobspecpid(sh, scratch, "kill", a);
            if (pid < 0) {
                status = 1;
                continue;
            }
            i32 err = killpid(sh, pid, sig);
            if (err) {
                builtinerror(sh, scratch, "kill", concat3(&scratch, a, Str(": "), errtext(err)));
                status = 1;
            }
            continue;
        }
        Str digits = a.len && a[0]=='-' ? cuthead(a, 1) : a;
        b32 ok = digits.len>0;
        for (iz k = 0; k < digits.len; k++) ok &= digit(digits[k]);
        if (!ok || !parsei64(a, &pid)) {
            builtinerror(sh, scratch, "kill", concat3(&scratch, "`", a, "': not a pid or valid job spec"));
            status = 1;
            continue;
        }
        i32 err = killpid(sh, pid, sig);
        if (err) {
            Buf b(&scratch, 64);
            print(&b, "(");
            print(&b, pid);
            print(&b, ") - ");
            print(&b, errtext(err));
            builtinerror(sh, scratch, "kill", finish(&b));
            status = 1;
        }
    }
    return status;
}

// Report a pipeline whose last stage died by a signal: one line for
// SIGTERM, as for a single command, else a line per stage with its
// pid and how it ended.
static void reportpipeline(Shell *sh, Arena scratch, Node **stages, i32 n, i32 *statuses,
                           i64 *pids, Node *job)
{
    i32 sig = (statuses[n-1] & 255) - 128;
    if (sig==2 || sig==13 || sh->noreport) return;
    if (!longreport(sh, sig)) {
        Node c = *job;
        c.flags &= ~(CMD_INVERT|CMD_TIME|CMD_TIMEPOSIX);
        Printer *p = newprinter(&scratch);
        printcmd(p, &c);
        reportsig(sh, scratch, sig, pids[n-1], finish(&p->b));
        return;
    }
    flushbout(sh);
    Buf b(&scratch, 256);
    for (i32 k = 0; k < n; k++) {
        i32 st = statuses[k];
        if (k) {
            print(&b, "     ");
        } else {
            msgprefix(sh, &b);
        }
        Str p = fmti64(&scratch, pids[k]);
        for (iz i = p.len; i < 5; i++) putbyte(&b, ' ');
        print(&b, p);
        putbyte(&b, ' ');
        Str desc = "Done";
        if (st & WSIGNALED) {
            i32 s = (st & 255) - 128;
            desc = s<32 ? sigdescs[s] : concat(&scratch, "Real-time signal ", fmti64(&scratch, s-34));
        } else if (st & 255) {
            desc = concat(&scratch, "Exit ", fmti64(&scratch, st & 255));
        }
        print(&b, desc);
        for (iz i = desc.len; i < 27; i++) putbyte(&b, ' ');
        if (k) print(&b, "| ");
        Printer *pr = newprinter(&scratch);
        printcmd(pr, stages[k]);
        print(&b, finish(&pr->b));
        putbyte(&b, '\n');
    }
    fdwrite(sh, 2, finish(&b));
}
