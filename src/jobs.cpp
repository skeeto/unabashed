// Background jobs, wait, and coprocesses
//
// Interim M3a jobs, until M5b makes them real processes: "list &" runs
// list on a fiber vproc, a copy of the shell with its own memory (as
// a pipeline stage does), and an external command started directly is
// watched by a small waiter task. Each goes in the job table of the
// level that started it, for $!, wait, jobs, disown and kill %N.
//
// The table follows bash's non-interactive rules: a job that ends
// stays listed, unreported, until jobs lists it or wait collects its
// status; one killed by a signal bash reports (not INT, TERM, PIPE, nor
// a trapped one) is reported before the next top-level command, then
// dropped. Dropped jobs leave their status behind for a later wait.
// A job's end is noticed only at those points (polljobs), so what a
// script sees is deterministic on the virtual OS.
//
// Subshells see their parent's jobs (command substitutions and simple
// pipeline stages list them, as in bash) but cannot wait for them.
// Jobs still running when their level ends are orphaned: the Sched
// joins them later, and the shell waits for every orphan before it
// exits, since a fiber cannot outlive its process.
//
// This is free and unencumbered software released into the public domain.

// Live background vprocs (the fallback) before & waits. Every
// background level of a library's run is one.
enum { MAXBG = !LIBMODE ? 4 : sizeof(void *)==8 ? 64 : 16 };

static b32 ownjob(Shell *sh, Job *j)
{
    return j->owner == sh->bashpid;
}

// The job with that pid ended: so did its coprocess, if it was one.
static void coprocended(Shell *sh, i64 pid)
{
    for (Coproc *c = sh->coprocs; c && c->owner==sh->bashpid; c = c->next) {
        if (c->pid == pid) c->dead = 1;
    }
}

static Job *jobbyid(Shell *sh, i32 id)
{
    for (Job *j = sh->jobs; j; j = j->next) {
        if (j->id == id) return j;
    }
    return 0;
}

// A waiter's program stopped or continued (its IO_PROCSTOP reported a
// change): note it for polljobs, and wait again. True if so.
static b32 waiterchange(Sched *s, Task *t)
{
    b32 r = 0;
    while (t->io.done && !t->io.err && ((i32)t->io.n & (WSTOP|WCONT))) {
        t->change  = (i32)t->io.n;
        t->io.done = 0;
        t->io.n    = 0;
        os_submit(s->os, &t->io);
        if (!t->io.done) s->npending++;
        r = 1;
    }
    return r;
}

// A job whose program a waiter watches, rather than a vproc. Its wait
// is submitted at once (spawnwaiter), so that the program's end is
// noticed even before the waiter first runs. Under job control it
// also sees stops, which wake a task waiting for the job (fg, wait).
static void waitermain(Task *t)
{
    Sched *s = t->s;
    for (;;) {
        while (!t->io.done) {
            block(s, W_IO);
        }
        if (!waiterchange(s, t)) break;
        wakejoiners(s, t);
    }
    t->status = t->io.err ? 127 : (i32)t->io.n & ~(WMISSED|WNOLIB);
}

static Task *spawnwaiter(Sched *s, Handle proc, b32 stops = 0)
{
    Task *t = taskspawn(s, waitermain, (void *)proc, SMALLSTACK);
    t->io.op  = stops ? IO_PROCSTOP : IO_PROC;
    t->io.h   = proc;
    t->io.ctx = t;
    os_submit(s->os, &t->io);
    if (!t->io.done) s->npending++;
    return t;
}

static b32 iswaiter(Task *t)
{
    return t->fn == waitermain;
}


// Orphans: jobs whose level ended

static void jointask(Sched *s, Task *t, i32 *status)
{
    if (t->fn == pumpmain) {
        // a program's output pump, orphaned by runprogram
        joinpump(s, t);
        return;
    }
    s->nbg -= !iswaiter(t);
    i32 st = taskjoin(s, t);
    if (status) *status = st;
}

// Join the orphans that have finished.
static void reaporphans(Sched *s)
{
    for (Task **p = &s->orphans; *p;) {
        Task *t = *p;
        if (t->state == TS_DONE) {
            *p = t->sib;
            jointask(s, t, 0);
        } else {
            p = &t->sib;
        }
    }
}

// Hand the level's running jobs to the Sched as its level ends.
static void orphanjobs(Shell *sh)
{
    Sched *s = sh->sched;
    for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
        if (j->task) {
            j->task->sib = s->orphans;
            s->orphans   = j->task;
            j->task      = 0;
        }
    }
}

// As the shell exits: wait for the orphaned vprocs, which cannot
// outlive the process; programs run on (their waiters are dropped).
static void joinorphans(Sched *s)
{
    while (s->orphans) {
        Task *t = s->orphans;
        s->orphans = t->sib;
        // Waiters, and /dev/zero's pumps (genfile) that a program
        // still reads, never run again
        b32 gen = t->fn==pumpmain && ((Pump *)t->arg)->kind==PUMP_GEN;
        if ((iswaiter(t) || gen) && t->state!=TS_DONE) {
            if (!t->io.done && os_cancel(s->os, &t->io)) {
                s->npending--;
            }
            continue;  // never runs again
        }
        jointask(s, t, 0);
    }
}

// Too many live background vprocs: wait for one to finish first, as a
// busy system would make a fork wait.
static void throttlejobs(Shell *sh)
{
    Sched *s = sh->sched;
    while (s->nbg >= MAXBG) {
        reaporphans(s);
        b32 done = 0;
        for (Job *j = sh->jobs; j; j = j->next) {
            done |= j->task && j->task->state==TS_DONE;
        }
        if (done || s->nbg<MAXBG) break;
        if (!taskwaitany(s)) {
            s->cur->intr = 0;
            break;
        }
    }
}


// Job control's process groups and terminal

// Does job control make process groups here? At the outermost level of
// a shell under set -m, on a platform with groups.
static b32 fgjobctl(Shell *sh)
{
    return sh->jobctl && sh->shellpgid && !sh->subshell;
}

// set -m turned job control on: jobs get process groups of their own
// from now on. The terminal (fd 2, else fd 0) goes to foreground jobs
// when the shell's group holds it. An interactive shell leads a group
// of its own and takes the terminal; a script stays in its group, as
// bash.
static void jobctlon(Shell *sh)
{
    if (sh->subshell || !sh->startpgid || sh->shellpgid) return;
    sh->shellpgid = sh->startpgid;
    sh->jctty     = BADHANDLE;
    Handle tty = BADHANDLE;
    for (i32 fd = 2; fd >= 0; fd -= 2) {
        File *f = fd<sh->fds->top ? sh->fds->fd[fd] : 0;
        if (f && f->kind==F_OS && os_isatty(sh->os, f->h)) {
            tty = f->h;
            break;
        }
    }
    if (tty == BADHANDLE) return;
    if (sh->interactive && sh->startpgid!=sh->pid && !os_setpgid(sh->os, 0, 0)) {
        sh->shellpgid = sh->pid;
        os_tcpgrp(sh->os, tty, sh->shellpgid);
    }
    if (os_tcpgrp(sh->os, tty, 0) != sh->shellpgid) return;
    sh->jctty = os_dup(sh->os, tty);
    if (interactivetop(sh)) {
        for (i32 sig = 20; sig <= 22; sig++) updatesigmode(sh, sig);
    }
}

// As the shell exits: an interactive shell's stopped jobs are ended
// (TERM, then CONT to act on it), and the group and terminal it started
// with restored.
static void jobctloff(Shell *sh)
{
    if (sh->subshell || !sh->shellpgid) return;
    for (Job *j = sh->jobs; sh->interactive && j && ownjob(sh, j); j = j->next) {
        if (j->stopped && !j->done && j->pgid) {
            os_kill(sh->os, -j->pgid, 15);
            os_kill(sh->os, -j->pgid, 18);
        }
    }
    if (sh->shellpgid != sh->startpgid) {
        if (sh->jctty != BADHANDLE) os_tcpgrp(sh->os, sh->jctty, sh->startpgid);
        os_setpgid(sh->os, 0, sh->startpgid);
    }
    if (sh->jctty != BADHANDLE) os_close(sh->os, sh->jctty);
    sh->jctty     = BADHANDLE;
    sh->shellpgid = 0;
}

// The table

static void setcurrent(Shell *, i32 id);

// Apply the stop or continue a job's waiter saw: a job that stops
// becomes the current one.
static void jobchange(Shell *sh, Job *j)
{
    Task *t = j->task;
    i32   c = t->change;
    t->change = 0;
    if (!c) return;
    if (c & WSTOP) {
        if (j->stopped) return;
        j->stopped  = 1;
        j->stopsig  = c & 255;
        j->notified = 0;
        setcurrent(sh, j->id);
    } else if (j->stopped) {
        j->stopped  = 0;
        j->notified = 0;
    }
}

// Notice the jobs that have ended: finished tasks are joined.
static void polljobs(Shell *sh)
{
    Sched *s = sh->sched;
    reaporphans(s);
    b32 any = 0;
    for (Job *j = sh->jobs; j; j = j->next) {
        any |= j->task && ownjob(sh, j);
    }
    if (!any) return;
    // Programs that exited: let their waiters see it now
    while (s->npending) {
        Io *io = os_wait(s->os, WAIT_POLL);
        if (!io) break;
        s->npending--;
        wake(s, (Task *)io->ctx);
    }
    for (Job *j = sh->jobs; j; j = j->next) {
        Task *t = j->task;
        if (!t || !ownjob(sh, j)) continue;
        if (iswaiter(t)) {
            waiterchange(s, t);
            jobchange(sh, j);
        }
        b32 proc   = t->io.op==IO_PROC || t->io.op==IO_PROCSTOP;
        b32 exited = iswaiter(t) && t->state==TS_READY && proc && t->io.done;
        if (t->state==TS_DONE || exited) {
            jointask(s, t, &j->status);
            j->task = 0;
            j->done = 1;
            j->notified = 0;  // a change jobs has not shown
            coprocended(sh, j->pid);
            chldreaped(sh, 1);
        }
    }
}

// Is a job running, as far as the shell knows?
static b32 jobrunning(Job *j)
{
    return !j->done && !j->stopped;
}

// The newest running job older than job id (or of all, if 0).
static i32 lastrunning(Shell *sh, i32 id)
{
    for (Job *j = sh->jobs; j; j = j->next) {
        if ((!id || j->id<id) && jobrunning(j)) return j->id;
    }
    return 0;
}

// The newest stopped job but job skip, or 0.
static i32 laststopped(Shell *sh, i32 skip)
{
    for (Job *j = sh->jobs; j; j = j->next) {
        if (j->id!=skip && j->stopped && !j->done) return j->id;
    }
    return 0;
}

// Bash's current (+) and previous (-) jobs: the newest running job,
// and the next newest, though stopped jobs come first.
static void setcurrent(Shell *sh, i32 id)
{
    if (sh->curjob != id) {
        sh->prevjob = sh->curjob;
        sh->curjob  = id;
    }
    Job *cur  = jobbyid(sh, id);
    i32  cand = laststopped(sh, id);
    if (!cand) cand = cur && jobrunning(cur) ? lastrunning(sh, id) : lastrunning(sh, 0);
    sh->prevjob = cand ? cand : sh->curjob;
}

static void resetcurrent(Shell *sh)
{
    i32 cand = laststopped(sh, 0);
    if (!cand) cand = lastrunning(sh, 0);
    if (cand) {
        setcurrent(sh, cand);
    } else {
        sh->curjob = sh->prevjob = 0;
    }
}

static void addbgstat(Shell *sh, i64 pid, i32 status)
{
    Bgstat *b = (Bgstat *)salloc(sh->store, sizeof(Bgstat));
    b->pid    = pid;
    b->status = status;
    b->next   = sh->bgstats;
    sh->bgstats = b;
}

static Bgstat *findbgstat(Shell *sh, i64 pid)
{
    for (Bgstat *b = sh->bgstats; b; b = b->next) {
        if (b->pid == pid) return b;
    }
    return 0;
}

static void delbgstat(Shell *sh, i64 pid)
{
    for (Bgstat **p = &sh->bgstats; *p; p = &(*p)->next) {
        if ((*p)->pid == pid) {
            Bgstat *b = *p;
            *p = b->next;
            sfree(sh->store, b, sizeof(Bgstat));
            return;
        }
    }
}

// Drop a job from the table, keeping its status if it ended.
static void deletejob(Shell *sh, Job *j)
{
    for (Job **p = &sh->jobs; *p; p = &(*p)->next) {
        if (*p == j) {
            *p = j->next;
            break;
        }
    }
    if (j->task) {
        // still running (disown): it carries on as an orphan
        Sched *s = sh->sched;
        j->task->sib = s->orphans;
        s->orphans   = j->task;
    } else {
        addbgstat(sh, j->pid, j->status & 255);
    }
    i32 id = j->id;
    sfreestr(sh->store, j->text);
    sfreestr(sh->store, j->wd);
    sfree(sh->store, j, sizeof(Job));
    if (id==sh->curjob || id==sh->prevjob) resetcurrent(sh);
}

static void disposecoproc(Shell *, Coproc *);

// Drop the jobs whose end was reported, and coprocesses that ended.
static void cleanupjobs(Shell *sh)
{
    for (Job *j = sh->jobs; j && ownjob(sh, j);) {
        Job *next = j->next;
        if (j->done && j->notified) deletejob(sh, j);
        j = next;
    }
    for (Coproc *c = sh->coprocs; c && c->owner==sh->bashpid;) {
        Coproc *next = c->next;
        if (c->dead) disposecoproc(sh, c);
        c = next;
    }
}

// Start a job table entry for a command started in the background.
static Job *newjob(Shell *sh, Arena scratch, Node *n, Task *t, i64 pid, b32 coproc)
{
    i32 id = 1;
    for (Job *j = sh->jobs; j; j = j->next) {
        id = j->id>=id ? j->id+1 : id;
    }
    Job *j = (Job *)salloc(sh->store, sizeof(Job));
    j->owner = sh->bashpid;
    j->pid   = pid;
    j->task  = t;
    j->id    = id;
    j->jc    = sh->jobctl;
    Node c = *n;
    c.flags &= ~(CMD_TIME|CMD_TIMEPOSIX);
    Printer *p = newprinter(&scratch);
    if (coproc) put(p, "coproc ");
    printcmd(p, &c);
    j->text  = sclone(sh->store, finish(&p->b));
    j->wd    = sclone(sh->store, sh->wd);
    j->next  = sh->jobs;
    sh->jobs = j;
    resetcurrent(sh);
    return j;
}

// A copy of the parent's table for a vproc to list: other levels' jobs
// cannot be waited for, so only what they were when copied.
static Job *copyjobs(Store *st, Job *j)
{
    Job  *head = 0;
    Job **tail = &head;
    for (; j; j = j->next) {
        Job *c = (Job *)salloc(st, sizeof(Job));
        *c = *j;
        c->task = 0;
        c->next = 0;
        c->text = sclone(st, j->text);
        c->wd   = sclone(st, j->wd);
        *tail = c;
        tail  = &c->next;
    }
    return head;
}

// How a job ended, or that it runs, as jobs describes it.
static Str jobstate(Shell *sh, Arena *a, Job *j, i32 form = 0)
{
    if (j->stopped && !j->done) {
        return form=='l' && j->stopsig>0 && j->stopsig<32 ? sigdescs[j->stopsig] : Str("Stopped");
    }
    if (jobrunning(j)) return "Running";
    i32 st = j->status;
    if (st & WSIGNALED) {
        i32 sig = (st & 255) - 128;
        return sig>0 && sig<32 ? sigdescs[sig]
                               : concat(a, "Real-time signal ", fmti64(a, sig-34));
    }
    st &= 255;
    if (!st) return "Done";
    if (sh->opt[O_POSIX]) return concat3(a, "Done(", fmti64(a, st), ")");
    return concat(a, "Exit ", fmti64(a, st));
}

static void padpid(Buf *b, i64 pid)
{
    u8  tmp[24];
    Buf t = {};
    t.data = tmp;
    t.cap  = countof(tmp);
    print(&t, pid);
    for (iz i = t.len; i < 5; i++) putbyte(b, ' ');
    print(b, finish(&t));
}

// An interactive shell names each job it starts in the background,
// "[N] PID" on standard error, as bash.
static void announcejob(Shell *sh, Arena scratch, Job *j)
{
    if (!sh->interactive || sh->subshell) return;
    Buf b(&scratch, 32);
    print(&b, "[");
    print(&b, (i64)j->id);
    print(&b, "] ");
    print(&b, j->pid);
    putbyte(&b, '\n');
    flushbout(sh);
    fdwrite(sh, 2, finish(&b));
}

// One job as jobs lists it: form 0, or 'l' with its pid.
static Str jobline(Shell *sh, Arena *perm, Job *j, i32 form)
{
    Buf b(perm, 128);
    print(&b, "[");
    print(&b, (i64)j->id);
    print(&b, "]");
    putbyte(&b, j->id==sh->curjob ? '+' : j->id==sh->prevjob ? '-' : ' ');
    putbyte(&b, ' ');
    if (form == 'l') {
        padpid(&b, j->pid);
    }
    putbyte(&b, ' ');
    Str st = jobstate(sh, perm, j, form);
    print(&b, st);
    for (iz i = st.len; i < 27; i++) putbyte(&b, ' ');
    print(&b, j->text);
    if (jobrunning(j)) print(&b, " &");
    if (!(j->wd == sh->wd)) {
        // As bash's polite form: HOME as ~
        Str wd   = j->wd;
        Got home = getvar(sh, perm, "HOME");
        print(&b, "  (wd: ");
        iz  n    = homeprefix(sh, wd, home.value);
        if (n >= 0) {
            print(&b, "~");
            wd = cuthead(wd, n);
        }
        print(&b, wd);
        print(&b, ")");
    }
    putbyte(&b, '\n');
    return finish(&b);
}

// List one job: form 0 as jobs, 'l' with its pid, 'p' its pid alone.
static void printjob(Shell *sh, Arena a, Job *j, i32 form)
{
    if (form == 'p') {
        bprinti(sh, j->pid);
        bprint(sh, "\n");
        return;
    }
    bprint(sh, jobline(sh, &a, j, form));
    if (ownjob(sh, j)) j->notified = 1;
}

// Report a finished background job killed by a signal, as bash does
// for any but INT, TERM and PIPE and those this shell traps, once.
static void reportkilled(Shell *sh, Arena scratch, Job *j)
{
    if (!j->done || j->notified || !(j->status & WSIGNALED)) return;
    i32 sig = (j->status & 255) - 128;
    if (sig==2 || sig==13 || sig==15 || !longreport(sh, sig) || sh->noreport) return;
    flushbout(sh);
    Buf b(&scratch, 128);
    msgprefix(sh, &b);
    padpid(&b, j->pid);
    putbyte(&b, ' ');
    Str st = jobstate(sh, &scratch, j);
    print(&b, st);
    for (iz i = st.len; i < 27; i++) putbyte(&b, ' ');
    print(&b, j->text);
    putbyte(&b, '\n');
    fdwrite(sh, 2, finish(&b));
    j->notified = 1;
}

// Before each top-level command: report background jobs killed by a
// signal, then drop the reported jobs.
static void notifyjobs(Shell *sh, Arena scratch)
{
    polljobs(sh);
    if (sh->interactive) return;  // at the prompt instead
    for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
        reportkilled(sh, scratch, j);
    }
    cleanupjobs(sh);
}

// An interactive shell's notification, as bash's notify_and_cleanup:
// each of this level's jobs (or only that one) that ended or stopped
// since last shown, as jobs lists it, then drop the ended ones. Without
// job control the ended ones are dropped unreported. Before each
// primary prompt, after wait collects a job, and (with job control,
// not in POSIX mode) after the first command of a `;` list.
static void notifyprompt(Shell *sh, Arena scratch, Job *only)
{
    polljobs(sh);
    for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
        if ((only && j!=only) || j->notified || !(j->done || j->stopped)) continue;
        if (!sh->jobctl) {
            j->notified = j->done;
            continue;
        }
        Arena a = scratch;
        flushbout(sh);
        fdwrite(sh, 2, jobline(sh, &a, j, 0));
        j->notified = 1;
    }
    cleanupjobs(sh);
}

// wait collected a job: an interactive shell reports it now (outside
// POSIX mode, else at the prompt), a script only if a signal killed it,
// and it is dropped.
static void collected(Shell *sh, Arena scratch, Job *j)
{
    if (sh->interactive && !sh->subshell) {
        if (!sh->opt[O_POSIX]) notifyprompt(sh, scratch, j);
        return;
    }
    reportkilled(sh, scratch, j);
    j->notified = 1;
}

// Find the job a job spec names, as bash's %N %+ %- %% %name %?text,
// with or without the %. Reports an ambiguous name; *ambiguous says so.
static Job *findjob(Shell *sh, Arena scratch, Str who, Str spec, b32 *ambiguous)
{
    *ambiguous = 0;
    if (!spec.len) return 0;
    Str w = spec;
    if (w[0] == '%') {
        w = cuthead(w, 1);
    } else {
        Str msg = concat3(&scratch, "warning: ", spec, ": job specification requires leading `%'");
        builtinerror(sh, scratch, who, msg);
    }
    b32 num = w.len > 0;
    for (iz i = 0; i < w.len; i++) num &= digit(w[i]);
    if (num) {
        i64 id;
        return parsei64(w, &id) && id>0 && id<=(i64)1<<30 ? jobbyid(sh, (i32)id) : 0;
    }
    if (!w.len || w=="%" || w=="+" || startswith(w, "%") || startswith(w, "+")) {
        return sh->curjob ? jobbyid(sh, sh->curjob) : 0;
    }
    if (w[0] == '-') {
        return sh->prevjob ? jobbyid(sh, sh->prevjob) : 0;
    }
    b32 sub = w[0] == '?';
    if (sub) w = cuthead(w, 1);
    Job *found = 0;
    for (Job *j = sh->jobs; j; j = j->next) {
        b32 match;
        if (sub) {
            match = 0;
            for (iz i = 0; !match && i+w.len <= j->text.len; i++) {
                b32 eq = 1;
                for (iz k = 0; eq && k < w.len; k++) {
                    u8 x = j->text[i+k], y = w[k];
                    x = x>='A' && x<='Z' ? x+32 : x;
                    y = y>='A' && y<='Z' ? y+32 : y;
                    eq = x == y;
                }
                match = eq;
            }
        } else {
            match = startswith(j->text, w);
        }
        if (!match) continue;
        if (found) {
            Str msg = concat(&scratch, w, ": ambiguous job spec");
            builtinerror(sh, scratch, who, msg);
            *ambiguous = 1;
            return 0;
        }
        found = j;
    }
    return found;
}

// Wait until a job's waiter finishes, or its program stops (unless
// force, as wait -f). False if a signal interrupted the wait (intr: as
// the wait builtin).
static b32 joinorstop(Shell *sh, Job *j, b32 intr, b32 force = 0)
{
    Sched *s    = sh->sched;
    Task  *self = s->cur;
    Task  *t    = j->task;
    self->introk = intr;
    for (;;) {
        waiterchange(s, t);
        jobchange(sh, j);
        if (t->state==TS_DONE || (j->stopped && !force)) break;
        t->joiner = self;
        block(s, W_JOIN);
        if (intr && self->intr) {
            t->joiner    = 0;
            self->introk = 0;
            return 0;
        }
    }
    self->introk = 0;
    return 1;
}

// A job whose waiter finished: it is done, with the waiter's status.
static void joindone(Shell *sh, Job *j)
{
    j->status   = taskjoin(sh->sched, j->task);
    j->task     = 0;
    j->done     = 1;
    j->stopped  = 0;
    j->notified = 0;
    coprocended(sh, j->pid);
    chldreaped(sh, 1);
}

static Job *jobbypid(Shell *sh, i64 pid)
{
    for (Job *j = sh->jobs; j; j = j->next) {
        if (j->pid == pid) return j;
    }
    return 0;
}

// Wait for one of this level's jobs, until it ends or (unless force)
// stops. False if a trapped signal interrupted the wait, with *status
// 128 plus its number.
static b32 waitjob(Shell *sh, Job *j, i32 *status, b32 force = 0)
{
    if (j->task && iswaiter(j->task)) {
        // It may stop meanwhile, under job control
        if (!joinorstop(sh, j, 1, force)) {
            sh->sched->cur->intr = 0;
            u64 pend = sh->sig->pending;
            *status = pend ? 128 + __builtin_ctzll(pend) + 1 : 128;
            return 0;
        }
        if (j->stopped && j->task->state!=TS_DONE) {
            *status = 128 + j->stopsig;
            return 1;
        }
        joindone(sh, j);  // perhaps ended before its continue was seen
    } else if (j->task) {
        i32 st;
        if (!taskjoinintr(sh->sched, j->task, &st)) {
            sh->sched->cur->intr = 0;
            u64 pend = sh->sig->pending;
            *status = pend ? 128 + __builtin_ctzll(pend) + 1 : 128;
            return 0;
        }
        sh->sched->nbg--;
        j->task   = 0;
        j->done   = 1;
        chldreaped(sh, 1);
        j->notified = 0;
        j->status = st;
        coprocended(sh, j->pid);
    }
    *status = j->status & 255;
    return 1;
}


// Starting jobs

// Close the coprocesses' descriptors in a subshell, as bash does in
// every child but a command substitution.
static void closecoproc(Shell *c)
{
    Fdtab *t = c->fds;
    for (Coproc *cp = c->coprocs; cp; cp = cp->next) {
        i32 fds[2] = {cp->rfd, cp->wfd};
        for (i32 i = 0; i < 2; i++) {
            i32 fd = fds[i];
            if (fd>=0 && fd<t->top && t->fd[fd]) {
                fileunref(c, t->fd[fd]);
                t->fd[fd] = 0;
            }
        }
    }
    while (t->top && !t->fd[t->top-1]) t->top--;
    c->coprocs = 0;
}

// Give a background vproc its own handles into the captures it would
// write: a capture Buf is not shared beyond the command that made it.
static void detachcaptures(Shell *sh, Shell *c)
{
    Fdtab *t = c->fds;
    for (i32 fd = 0; fd < t->top; fd++) {
        File *f = t->fd[fd];
        if (!f || f->kind!=F_CAPTURE) continue;
        Handle h = capturewriter(sh, f, c->bashpid);
        f->kind = F_OS;  // in place: descriptors sharing it follow
        f->cap  = 0;
        f->h    = h;
        if (h == BADHANDLE) f->flags = 0;  // writes fail
    }
}

static File *devnull(Shell *sh, Arena scratch)
{
    Opened o = os_open(sh->os, scratch, "/dev/null", OF_READ, 0);
    if (o.err) return 0;
    File *f = newfile(sh, F_OS, OF_READ);
    f->h = o.h;
    return f;
}

// Make background subshell c, a copy of its parent's state, ready to
// run a copy of n in arena a, which it returns. keepjobs: a pipeline's
// simple command, which lists its parent's jobs, as bash's does.
static Node *bgnode(Shell *c, Arena *a, Node *n, b32 keepjobs)
{
    c->loopdepth = 0;
    if (!keepjobs) {
        c->jobs = 0;  // as a forked subshell, it starts with no jobs
        c->curjob = c->prevjob = 0;
        c->bgstats = 0;
    }
    Node *r = copynode(a, n);
    c->tailnode = n->kind==N_SIMPLE ? r : 0;
    if (r->kind == N_SUBSHELL) {
        r->kind = N_GROUP;
        c->tailnode = tailcommand(r->a);
    }
    c->taillevel  = c->subshell + 1;
    c->keepredirs = r;
    c->asyncign = (u64)1<<1 | (u64)1<<2;  // INT, QUIT
    return r;
}

// The descriptors a background shell process gets, as a program would,
// its 0 and 1 replaced by in and out when given: handles to pass and
// handles to close once it has started. An in-memory pipe it writes
// is pumped by an orphan task, which ends when the process closes it.
struct BgFds {
    SpawnFd *fds;
    iz       nfds;
    Handle  *close;
    iz       nclose;
    Task    *caps[8];  // capture pumps, owned by the job once started
    i32      ncaps;
};

static BgFds bgfds(Shell *sh, Arena *a, File *in, File *out)
{
    Sched *s = sh->sched;
    Fdtab *t = sh->fds;
    i32    top = t->top>2 ? t->top : 2;
    BgFds  r = {};
    r.fds   = alloc<SpawnFd>(a, top);
    r.close = alloc<Handle>(a, top);
    for (i32 fd = 0; fd < top; fd++) {
        File *f = fd==0 && in ? in : fd==1 && out ? out : fd<t->top ? t->fd[fd] : 0;
        if (!f) continue;
        b32 cloexec = 0;
        for (Coproc *c = sh->coprocs; c; c = c->next) {
            cloexec |= fd==c->rfd || fd==c->wfd;
        }
        if (cloexec) continue;  // closed in every subshell but $( )
        SpawnFd sf = {fd, BADHANDLE};
        for (iz k = 0; k < r.nfds; k++) {
            File *g = r.fds[k].fd==0 && in ? in : r.fds[k].fd==1 && out ? out : t->fd[r.fds[k].fd];
            if (g==f && f->kind!=F_OS) sf.h = r.fds[k].h;
        }
        if (sf.h != BADHANDLE) {
            r.fds[r.nfds++] = sf;
            continue;
        }
        switch (f->kind) {
        case F_OS:
            sf.h = f->h;
            break;
        case F_MEM:
            if (!materialize(sh, *a, f)) continue;
            sf.h = f->h;
            break;
        case F_CAPTURE: {
            if (r.ncaps == countof(r.caps)) continue;
            sf.h = capturewriter(sh, f, 0);
            if (sf.h == BADHANDLE) continue;
            r.close[r.nclose++] = sf.h;
            r.caps[r.ncaps++] = ((Capture *)f->cap)->pumps;
        } break;
        case F_PIPE: {
            if (f->flags & OF_READ) {
                sf.h = pipeosread(s, f->pipe);
                if (sf.h == BADHANDLE) continue;
                break;
            }
            OsPipe p = os_pipe(sh->os);
            if (p.err) continue;
            Pump *pp = newpump(s);
            pp->pipe = f->pipe;
            pp->kind = PUMP_FROMHANDLE;
            pp->h    = p.r;
            sf.h     = p.w;
            r.close[r.nclose++] = sf.h;
            Task *pt = spawnpump(s, pp);
            pt->sib    = s->orphans;
            s->orphans = pt;
        } break;
        case F_CLIP: {
            // Set as the process closes it, by an orphan pump
            ClipWriter w = clipwriter(s, f->clip);
            if (w.h == BADHANDLE) continue;
            sf.h = w.h;
            r.close[r.nclose++] = sf.h;
            w.t->sib   = s->orphans;
            s->orphans = w.t;
        } break;
        }
        r.fds[r.nfds++] = sf;
    }
    return r;
}

// Start node as a background shell process continuing from an image of
// this one (notes/m5b.md), its descriptors 0 and 1 replaced when in and
// out are given. Returns its process handle, with *pid its pid, or -1
// where the platform cannot (then it runs as a vproc). fg: a foreground
// job's process, in group sh->spawngrp, taking the terminal if it leads.
static i64 spawnimage(Shell *sh, Node *n, File *in, File *out, i64 *pid,
                      Arena scratch, b32 fg = 0)
{
    ImageRef im = os_imagenew(sh->os, sh->reserve);
    if (!im.base) return -1;
    BgFds fds = bgfds(sh, &scratch, in, out);
    i64   grp = sh->spawngrp;
    Image img = imagebuild(sh, n, im, fg && n->kind==N_SIMPLE);
    if (fg) img.head->sh->asyncign = 0;  // the terminal's INT reaches it

    Spawn sp = {};
    sp.rlimits = sh->rlmod ? sh->rl : 0;
    sp.image  = img.ref;
    sp.argv   = &sh->shellname;
    sp.argc   = 1;
    sp.cwd    = spawncwd(sh, &scratch);
    sp.fds    = fds.fds;
    sp.nfds   = fds.nfds;
    sp.sigign = ignoredsigs(sh);
    sp.pgid   = grp;
    sp.fg     = fg && grp==-1 && sh->jctty!=BADHANDLE;
    sp.tty    = sh->jctty;
    sp.gated  = fg && sh->gated;
    sp.gater  = sh->gate.r;
    sp.gatew  = sh->gate.w;
    if (!grp) sp.sigign |= (u64)1<<1 | (u64)1<<2;  // INT, QUIT
    Spawned r = os_spawn(sh->os, scratch, &sp);
    for (iz i = 0; i < fds.nclose; i++) {
        os_close(sh->os, fds.close[i]);
    }
    if (r.err) {
        os_imagefree(sh->os, img.ref);
        return -1;
    }
    for (i32 i = 0; i < fds.ncaps; i++) {
        // As capturewriter with an owner: a ${ ...; } it writes does not
        // wait for it
        Task *t = fds.caps[i];
        ((Pump *)t->arg)->owner = r.pid;
        t->group = t->tag;
    }
    *pid = r.pid;
    return (i64)r.proc;
}

// A grouped pipeline's shell stages wait to run until the parent has
// started all its processes, as bash's children read its pgrp pipe: a
// stage's `kill -STOP 0` then stops the whole job. Without a pipe they
// do not wait.
static void opengate(Shell *sh)
{
    OsPipe p = os_pipe(sh->os);
    if (p.err) return;
    sh->gate  = p;
    sh->gated = 1;
}

static void closegate(Shell *sh)
{
    if (!sh->gated) return;
    sh->gated = 0;
    os_close(sh->os, sh->gate.r);
    os_close(sh->os, sh->gate.w);
}

// A here-document a child inherits becomes an OS object first, so that
// both read one open file, as bash's forked child shares the temporary
// file or pipe: { read x; } & reads only its own line
static void materializefds(Shell *sh, b32 skip0)
{
    Fdtab *t = sh->fds;
    for (i32 fd = skip0 ? 1 : 0; fd < t->top; fd++) {
        File *f = t->fd[fd];
        if (f && f->kind==F_MEM) {
            byte  mem[256];
            Arena a = {mem, mem+sizeof(mem)};
            materialize(sh, a, f);
        }
    }
}

// Under job control, shell code in a foreground job (a pipeline stage
// or a ( ) subshell) runs in a process of its own, as bash forks it, so
// that it stops with the job: in group grp, or leading a new one that
// takes the terminal when grp is 0. Returns its process, with *pid its
// pid, or BADHANDLE where the platform cannot.
static Handle spawnstage(Shell *sh, Node *n, File *in, File *out, i64 grp,
                         i64 *pid, Arena scratch)
{
    materializefds(sh, in!=0);
    i64 save = sh->spawngrp;
    sh->spawngrp = grp ? grp : -1;
    i64 proc = spawnimage(sh, n, in, out, pid, scratch, 1);
    sh->spawngrp = save;
    return proc<0 ? BADHANDLE : (Handle)proc;
}

// Run a foreground ( ) under job control as a process of its own
// (spawnstage), waiting for it to end or stop: a stop makes it a
// stopped job. False where the platform cannot start one.
static b32 runfgimage(Shell *sh, Node *n, Arena scratch)
{
    Node m = *n;
    m.redirs = 0;  // applied already, its descriptors inherited
    i64    pid  = 0;
    Handle proc = spawnstage(sh, &m, 0, 0, 0, &pid, scratch);
    if (proc == BADHANDLE) return 0;
    Sched *s = sh->sched;
    takeossigs(sh);
    sh->sig->fgwait = 1;
    i32 st = taskwaitstop(s, proc);
    if (sh->jctty != BADHANDLE) os_tcpgrp(sh->os, sh->jctty, sh->shellpgid);
    sh->childsig = 0;
    if (st>=0 && (st & WSTOP)) {
        Job *j = newjob(sh, scratch, n, spawnwaiter(s, proc, 1), pid, 0);
        j->pgid    = pid;
        j->stopped = 1;
        j->stopsig = st & 255;
        waitdone(sh, 0, 0);
        sh->status = fgstop(sh, scratch, j);
    } else {
        b32 missed = st>=0 && (st & WMISSED);
        st = st<0 ? 127 : st & ~(WMISSED|WNOLIB);
        b32 died = st & WSIGNALED;
        i32 sig  = (st & 255) - 128;
        if (died && sig==2) intrjob(sh);
        if (died) {
            sh->childsig = sig;
            sh->childpid = pid;
        }
        u64 sigs = missed ? ~(u64)0 : died && sig>0 && sig<NSIGNAL ? sigbit(sig) : 0;
        waitdone(sh, sigs, (st & 255)!=0);
        sh->status = st & 255;
    }
    chldreaped(sh, 1);
    if (!n->redirs) reportchild(sh, scratch, n);  // else once they are undone
    setpstat1(sh);
    checkerrexit(sh);
    checksigs(sh, scratch);
    return 1;
}

// Start node as a background vproc, its descriptors 0 and 1 replaced
// when in and out are given. Returns the task, with *pid its BASHPID.
static Task *spawnbg(Shell *sh, Node *n, File *in, File *out, i64 *pid,
                     Arena scratch)
{
    materializefds(sh, in!=0);
    i64   proc = spawnimage(sh, n, in, out, pid, scratch);
    if (proc >= 0) return spawnwaiter(sh->sched, (Handle)proc, sh->spawngrp!=0);
    Vproc *m = vprocwith(sh, in, out);
    Shell *c = m->sh;
    c->catchfatal |= n->kind != N_SIMPLE;  // as imagebuild
    closecoproc(c);
    detachcaptures(sh, c);
    m->node = bgnode(c, &m->perm, n);
    *pid = c->bashpid;
    sh->sched->nbg++;
    return taskspawn(sh->sched, vprocmain, m, BIGSTACK);
}

// May a background command start directly as a program? As a direct
// pipeline stage, with its output not an in-memory pipe.
static b32 directbg(Shell *sh, Arena scratch, Node *n, b32 nullin)
{
    File *out = sh->fds->top>1 ? sh->fds->fd[1] : 0;
    if (out && out->kind!=F_OS && out->kind!=F_CAPTURE) return 0;
    return directstage(sh, scratch, n, !nullin);
}

static void runasync1(Shell *, Node *, Arena);

// "node &": start it in the background as a job, and set $!. As in
// bash, "a & b &" is two jobs, but "a && b &" one.
static void runasync(Shell *sh, Node *n, Arena scratch)
{
    if (n->kind==N_CONNECT && n->connector=='&' && !n->redirs && !n->flags) {
        runasync(sh, n->a, scratch);
        if (n->b) runasync(sh, n->b, scratch);
        return;
    }
    runasync1(sh, n, scratch);
}

static void runasync1(Shell *sh, Node *n, Arena scratch)
{
    if (n->kind==N_SIMPLE && n->words && startswith(n->words->text, "%")) {
        // "%N &" is "bg %N", in this shell: bash does not fork for it
        sh->asyncpct = 1;
        runsimple(sh, n, scratch);
        return;
    }
    Sched *s = sh->sched;
    polljobs(sh);
    cleanupjobs(sh);
    throttlejobs(sh);
    // DEBUG for a simple command runs here, as bash runs it before it
    // forks, and not again in the job (debugdone, under functrace)
    if (stagedebug(sh, n, sh->status, scratch)) return;
    b32 debugged = n->kind==N_SIMPLE && sh->traps && sh->traps[TRAP_DEBUG].len;
    if (!sh->opt[O_FUNCTRACE]) {
        // and each simple stage of a pipeline, which the job (with no
        // DEBUG then) runs without it, as runpipeline flattens it
        i32 before = sh->status;
        for (Node *c = n; c->kind==N_CONNECT && c->connector=='|';) {
            if (stagedebug(sh, c->a, before, scratch)) return;
            c = c->b;
            if (c->flags || c->redirs) break;
            if (c->kind!=N_CONNECT || c->connector!='|') {
                if (stagedebug(sh, c, before, scratch)) return;
            }
        }
    }
    // Without job control, or in a subshell, its input is /dev/null
    // unless redirected
    b32   nullin = !sh->stdinredir && (sh->subshell || !sh->jobctl);
    File *in     = nullin ? devnull(sh, scratch) : 0;
    Task *t      = 0;
    i64   pid    = 0;
    b32   retry  = 1;
    i32   status = 0;
    // Under job control it is a process group of its own, which the
    // terminal's INT and QUIT do not reach (so not ignored), and it
    // may stop
    b32 grouped = fgjobctl(sh);
    sh->spawngrp = grouped ? -1 : 0;
    if (directbg(sh, scratch, n, nullin)) {
        u64 ign = sh->asyncign;
        if (!grouped) sh->asyncign |= (u64)1<<1 | (u64)1<<2;  // INT, QUIT
        Handle proc = spawndirect(sh, n, in, 0, scratch, &status, &retry, 0);
        sh->asyncign = ign;
        pid = sh->childpid;
        if (proc != BADHANDLE) {
            t = spawnwaiter(s, proc, grouped);
        } else if (!retry) {
            pid = newvpid(sh);  // it failed as its own process would
        }
    }
    if (retry) {
        sh->debugdone = debugged;
        sh->forked    = n->kind == N_SIMPLE;
        t = spawnbg(sh, n, in, 0, &pid, scratch);
        sh->debugdone = 0;
        sh->forked    = 0;
    }
    sh->spawngrp = 0;
    fileunref(sh, in);
    Job *j = newjob(sh, scratch, n, t, pid, 0);
    if (grouped && t && iswaiter(t)) j->pgid = pid;
    announcejob(sh, scratch, j);
    if (!t) {
        j->done   = 1;
        j->status = status & 255;
    }
    sh->lastbg = pid;
    sh->status = 0;
    if (retry) {
        // It gets going at once, until it blocks, as a forked child
        // runs alongside its parent (programs already run)
        yield(s);
    }
}


// Coprocesses

// The highest free descriptor below 64 and not taken, as bash's
// move_to_high_fd, or else the lowest such.
static i32 highfd(Shell *sh, i32 *taken, i32 ntaken)
{
    Fdtab *t = sh->fds;
    for (i32 pass = 0; pass < 2; pass++) {
        for (i32 i = 0; i < FDMAX; i++) {
            i32 fd = pass ? i : 63-i;
            if (!pass && fd<=3) break;
            b32 used = fd<t->top && t->fd[fd];
            for (i32 k = 0; k < ntaken; k++) used |= taken[k] == fd;
            if (!used) return fd;
        }
    }
    return -1;
}

// In a ( list ): the parent's coprocesses are not there.
static void hidecoproc(Shell *sh, Arena *scratch)
{
    Fdtab *t = sh->fds;
    for (Coproc *c = sh->coprocs; c; c = c->next) {
        i32 fds[2] = {c->rfd, c->wfd};
        for (i32 i = 0; i < 2; i++) {
            if (fds[i]>=0 && fds[i]<t->top && t->fd[fds[i]]) fdset(sh, fds[i], 0, 0, scratch);
        }
    }
    sh->coprocs = 0;
}

// kill %spec: to the job's process group if it has one, continuing a
// stopped job to take TERM or HUP, as bash; else to its pid. Returns -1
// after reporting no such job, else 0 or an E_* error.
static i32 jobkill(Shell *sh, Arena scratch, Str spec, i32 sig)
{
    polljobs(sh);
    b32  amb;
    Job *j = findjob(sh, scratch, "kill", spec, &amb);
    if (!j) {
        if (!amb) {
            Str msg = concat(&scratch, spec, ": no such job");
            builtinerror(sh, scratch, "kill", msg);
        }
        return -1;
    }
    if (!j->pgid) return killpid(sh, j->pid, sig);
    i32 err = os_kill(sh->os, -j->pgid, sig);
    if (!err && j->stopped && (sig==15 || sig==1)) os_kill(sh->os, -j->pgid, 18);
    return err;
}

static void setcoprocvars(Shell *sh, Arena scratch, Coproc *c)
{
    i32 flags = 0;
    Str name  = arraytarget(sh, scratch, c->name, &flags, 1);
    if (!name.data) return;
    Value *v = assignslot(sh, scratch, name, flags);
    if (!v) return;
    toarray(sh, v);
    Str num = fmti64(&scratch, c->rfd);
    setelem(sh, scratch, name, 0, {}, num, flags);
    Str num2 = fmti64(&scratch, c->wfd);
    setelem(sh, scratch, name, 1, {}, num2, flags);
    Str name2 = concat(&scratch, c->name, "_PID");
    Str num3 = fmti64(&scratch, c->pid);
    setvar(sh, scratch, name2, num3);
}

// A coprocess ended and the shell noticed: drop its variables and
// close its descriptors.
static void disposecoproc(Shell *sh, Coproc *c)
{
    for (Coproc **p = &sh->coprocs; *p; p = &(*p)->next) {
        if (*p == c) {
            *p = c->next;
            break;
        }
    }
    Arena scratch = *scratchat(sh, sh->capdepth+1);
    unsetvar(sh, concat(&scratch, c->name, "_PID"), 1);  // even readonly, as bash
    Str    name = tempname(sh, c->name);  // what a nameref names
    Value *v    = lookup(sh, name);
    if (v && (v->attrs & V_READONLY)) {
        Str msg = concat(&scratch, c->name, ": cannot unset: readonly variable");
        shellerror(sh, scratch, msg);
    } else {
        unsetvar(sh, name);
    }
    Fdtab *t = sh->fds;
    i32 fds[2] = {c->rfd, c->wfd};
    for (i32 i = 0; i < 2; i++) {
        i32 fd = fds[i];
        if (fd>=0 && fd<t->top && t->fd[fd]) {
            fileunref(sh, t->fd[fd]);
            t->fd[fd] = 0;
        }
    }
    while (t->top && !t->fd[t->top-1]) t->top--;
    sfreestr(sh->store, c->name);
    sfree(sh->store, c, sizeof(Coproc));
}

// A descriptor of the shell closes or moves: if it was a coprocess's,
// its variable says -1 from then on.
static void coprocfdclosed(Shell *sh, i32 fd)
{
    for (Coproc *c = sh->coprocs; c && c->owner==sh->bashpid; c = c->next) {
        if (c->rfd!=fd && c->wfd!=fd) continue;
        if (c->rfd == fd) c->rfd = -1;
        if (c->wfd == fd) c->wfd = -1;
        Arena scratch = *scratchat(sh, sh->capdepth+1);
        setcoprocvars(sh, scratch, c);
    }
}

// coproc [NAME] command: run it in the background with its input and
// output piped to the shell, at NAME[1] and NAME[0].
static void runcoproc(Shell *sh, Node *n, Arena scratch)
{
    Sched *s = sh->sched;
    Str name = expandstring(sh, &scratch, n->name->text);
    if (!validvarname(name)) {
        Str msg = concat3(&scratch, "`", name, "': not a valid identifier");
        shellerror(sh, scratch, msg);
        sh->status = 1;
        return;
    }
    polljobs(sh);
    cleanupjobs(sh);
    throttlejobs(sh);
    // Two pipes, their ends moved high as bash does: the shell keeps
    // the read end of the first and the write end of the second
    OsPipe p1 = os_pipe(sh->os);
    OsPipe p2 = p1.err ? p1 : os_pipe(sh->os);
    if (p1.err || p2.err) {
        if (!p1.err) {
            os_close(sh->os, p1.r);
            os_close(sh->os, p1.w);
        }
        Str msg = concat(&scratch, "cannot make pipe for coprocess: ", errtext(p2.err));
        shellerror(sh, scratch, msg);
        sh->status = 1;
        return;
    }
    File *r1 = newfile(sh, F_OS, OF_READ);   r1->h = p1.r;
    File *w1 = newfile(sh, F_OS, OF_WRITE);  w1->h = p1.w;
    File *r2 = newfile(sh, F_OS, OF_READ);   r2->h = p2.r;
    File *w2 = newfile(sh, F_OS, OF_WRITE);  w2->h = p2.w;
    i32 fds[4];
    for (i32 i = 0; i < 4; i++) {
        fds[i] = highfd(sh, fds, i);
    }
    if (fds[0]<0 || fds[3]<0) {
        fileunref(sh, r1);
        fileunref(sh, w1);
        fileunref(sh, r2);
        fileunref(sh, w2);
        shellerror(sh, scratch, "cannot make pipe for coprocess: Too many open files");
        sh->status = 1;
        return;
    }
    fdset(sh, fds[0], r1, 1, &scratch);
    fdset(sh, fds[3], w2, 1, &scratch);
    Fdtab *t = sh->fds;

    Node *body = n->a;
    i64   pid  = 0;
    Task *task = 0;
    i32   status = 0;
    b32   retry  = 1;
    if (directbg(sh, scratch, body, 1)) {
        // closed in the program, as bash's are close-on-exec
        File *save[2] = {t->fd[fds[0]], t->fd[fds[3]]};
        t->fd[fds[0]] = t->fd[fds[3]] = 0;
        u64 ign = sh->asyncign;
        sh->asyncign |= (u64)1<<1 | (u64)1<<2;  // INT, QUIT
        Handle proc = spawndirect(sh, body, r2, w1, scratch, &status, &retry, 0);
        sh->asyncign = ign;
        t->fd[fds[0]] = save[0];
        t->fd[fds[3]] = save[1];
        pid = sh->childpid;
        if (proc != BADHANDLE) {
            task = spawnwaiter(s, proc);
        } else if (!retry) {
            pid = newvpid(sh);
        }
    }
    if (retry) {
        Coproc mine = {};
        mine.next = sh->coprocs;
        mine.rfd  = fds[0];
        mine.wfd  = fds[3];
        sh->coprocs = &mine;  // the vproc closes the shell's ends
        task = spawnbg(sh, body, r2, w1, &pid, scratch);
        sh->coprocs = mine.next;
    }
    fileunref(sh, r2);
    fileunref(sh, w1);

    Coproc *c = (Coproc *)salloc(sh->store, sizeof(Coproc));
    c->name  = sclone(sh->store, name);
    c->owner = sh->bashpid;
    c->pid   = pid;
    c->rfd   = fds[0];
    c->wfd   = fds[3];
    c->next  = sh->coprocs;
    sh->coprocs = c;
    Job *j = newjob(sh, scratch, n, task, pid, 0);
    announcejob(sh, scratch, j);
    if (!task) {
        j->done   = 1;
        j->status = status & 255;
        c->dead   = 1;
    }
    setcoprocvars(sh, scratch, c);
    sh->lastbg = pid;
    sh->status = 0;
}


// Builtins

static i32 jobsusage(Shell *sh, Arena scratch, Str name, Str opt)
{
    invalidoption(sh, scratch, name, opt);
    return 2;
}

// jobs -x: run the command with each job spec replaced by its pid.
static i32 jobsexec(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    if (!nargs) return 0;
    polljobs(sh);
    Buf b(&scratch, 128);
    for (iz i = 0; i < nargs; i++) {
        Str a = args[i];
        if (a.len && a[0]=='%') {
            b32  amb;
            Job *j = findjob(sh, scratch, "jobs", a, &amb);
            if (j) a = fmti64(&scratch, j->pid);  // else as it is
        }
        if (i) putbyte(&b, ' ');
        print(&b, singlequote(&scratch, a));
    }
    Str src = finish(&b);
    i32 depth = ++sh->capdepth;
    runsource(sh, src, sh->iname, *scratchat(sh, depth), 0, sh->line);
    sh->capdepth = depth - 1;
    return sh->status;
}

static i32 b_jobs(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    i32 form = 0;
    b32 changed = 0, running = 0, stopped = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") { i++; break; }
        for (iz k = 1; k < a.len; k++) {
            if (a[k] == 'x') {
                if (form || changed || running || stopped) {
                    builtinerror(sh, scratch, "jobs", "no other options allowed with `-x'");
                    return 1;
                }
                return jobsexec(sh, args+i+1, nargs-i-1, scratch);
            }
            switch (a[k]) {
            case 'l': form = 'l'; break;
            case 'p': form = 'p'; break;
            case 'n': changed = 1; break;
            case 'r': running = 1; break;
            case 's': stopped = 1; break;
            default:
                return jobsusage(sh, scratch, "jobs", span(a.data+k-1, a.data+k+1));
            }
        }
    }
    polljobs(sh);
    if (i == nargs) {
        cleanupjobs(sh);
        Job *list[256];
        i32  n = 0;
        for (Job *j = sh->jobs; j && n<countof(list); j = j->next) list[n++] = j;
        for (i32 k = n-1; k >= 0; k--) {
            Job *j = list[k];
            b32 skip = (running && !jobrunning(j)) || (stopped && !(j->stopped && !j->done));
            if (skip || (changed && j->notified)) continue;
            printjob(sh, scratch, j, form);
        }
        if (!running && !stopped) cleanupjobs(sh);
        return 0;
    }
    i32 status = 0;
    for (; i < nargs; i++) {
        b32  amb;
        Job *j = findjob(sh, scratch, "jobs", args[i], &amb);
        if (!j) {
            Str msg = concat(&scratch, args[i], ": no such job");
            builtinerror(sh, scratch, "jobs", msg);
            status = 1;
            continue;
        }
        printjob(sh, scratch, j, form);
        cleanupjobs(sh);
    }
    return status;
}

// Set wait -p's variable.
static void waitvar(Shell *sh, Arena scratch, Str var, i64 pid)
{
    if (var.len) {
        Str num = fmti64(&scratch, pid);
        assignto(sh, scratch, var, num);
    }
}

// wait -n: the first of the jobs (or of the given ones) to end.
static i32 waitany(Shell *sh, Arena scratch, Str *ids, iz nids, Str var)
{
    // Pids whose jobs are already gone first, then procsubs
    for (iz i = 0; i < nids; i++) {
        i64 pid;
        if (ids[i].len && ids[i][0]=='%') continue;
        if (!parsei64(ids[i], &pid) || pid<0) continue;
        Bgstat *b = findbgstat(sh, pid);
        if (b) {
            i32 st = b->status;
            if (sh->opt[O_POSIX]) delbgstat(sh, pid);
            waitvar(sh, scratch, var, pid);
            return st;
        }
    }
    Job *want[64];
    i32  nwant = 0;
    for (iz i = 0; i < nids; i++) {
        i64  pid;
        Job *j = 0;
        b32  amb = 0;
        b32  ispid = parsei64(ids[i], &pid);
        if (ispid) {
            j = jobbypid(sh, pid);
        } else {
            j = findjob(sh, scratch, "wait", ids[i], &amb);
        }
        if (j && !ownjob(sh, j)) j = 0;
        if (!j) {
            if (ids[i].len && ids[i][0]=='%') {
                if (!amb) {
                    Str msg = concat(&scratch, ids[i], ": no such job");
                    builtinerror(sh, scratch, "wait", msg);
                }
            } else {
                Str msg = concat3(&scratch, "`", ids[i], "': not a pid or valid job spec");
                builtinerror(sh, scratch, "wait", msg);
            }
            continue;
        }
        if (nwant < countof(want)) want[nwant++] = j;
    }
    if (nids && !nwant) return 127;
    for (;;) {
        polljobs(sh);
        Job *found = 0, *live = 0;
        for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
            b32 wanted = !nids;
            for (i32 k = 0; k < nwant; k++) wanted |= want[k] == j;
            if (!wanted) continue;
            if (j->done && !j->notified) found = j;  // the oldest
            if (j->task && !j->stopped) live = j;
        }
        if (found) {
            i32 st  = found->status & 255;
            i64 pid = found->pid;
            collected(sh, scratch, found);
            if (sh->opt[O_POSIX]) delbgstat(sh, pid);
            waitvar(sh, scratch, var, pid);
            cleanupjobs(sh);
            return st;
        }
        if (!live) return 127;
        if (!taskwaitany(sh->sched)) {
            sh->sched->cur->intr = 0;
            u64 pend = sh->sig->pending;
            return pend ? 128 + __builtin_ctzll(pend) + 1 : 128;
        }
    }
}

// Plain wait passes over a stopped job, saying so.
static void waitstopped(Shell *sh, Arena scratch, Job *j)
{
    Buf b(&scratch, 64);
    print(&b, "warning: job ");
    print(&b, (i64)j->id);
    print(&b, "[");
    print(&b, j->pid);
    print(&b, "] stopped");
    builtinerror(sh, scratch, "wait", finish(&b));
}

static i32 b_wait(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 any   = 0;
    b32 force = 0;
    Str var   = {};
    iz  i     = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") { i++; break; }
        for (iz k = 1; k < a.len; k++) {
            u8 c = a[k];
            if (c == 'n') {
                any = 1;
            } else if (c == 'f') {
                force = sh->jobctl;  // through stops (without, as plain)
            } else if (c == 'p') {
                if (k+1 < a.len) {
                    var = cuthead(a, k+1);
                } else if (i+1 < nargs) {
                    var = args[++i];
                } else {
                    builtinerror(sh, scratch, "wait", "-p: option requires an argument");
                    usageerror(sh, scratch, "wait");
                    return 2;
                }
                break;
            } else {
                return jobsusage(sh, scratch, "wait", span(a.data+k-1, a.data+k+1));
            }
        }
    }
    if (var.data) {
        Str rn, rs;
        if (!validvarname(var) && !splitrefonce(sh, var, &rn, &rs)) {
            notident(sh, scratch, "wait", var);
            return 1;
        }
        if (validvarname(var) && !unsetvar(sh, var)) {
            Str msg = concat(&scratch, var, ": cannot unset: readonly variable");
            builtinerror(sh, scratch, "wait", msg);
            return 1;
        }
    }
    if (any) {
        i32 st = waitany(sh, scratch, args+i, nargs-i, var);
        return st;
    }

    if (i == nargs) {
        // Every running job, then the last procsub if $! names it.
        // Stopped ones are passed over, with a warning
        Job *list[256];
        i32  n = 0;
        for (Job *j = sh->jobs; j && ownjob(sh, j) && n<countof(list); j = j->next) {
            list[n++] = j;
        }
        polljobs(sh);
        for (i32 k = n-1; k >= 0; k--) {
            if (list[k]->stopped && !list[k]->done) waitstopped(sh, scratch, list[k]);
        }
        for (;;) {
            polljobs(sh);
            Job *live = 0;
            for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
                if (j->task && !j->stopped) live = j;  // the oldest
            }
            if (!live) break;
            i32 st;
            if (!waitjob(sh, live, &st)) return st;
            if (live->stopped) {
                waitstopped(sh, scratch, live);
                continue;
            }
            collected(sh, scratch, live);  // dropped, even $!'s
        }
        for (Job *j = sh->jobs; !sh->interactive && j && ownjob(sh, j); j = j->next) {
            reportkilled(sh, scratch, j);  // also those that ended earlier
        }
        b32 found;
        waitpsub(sh, sh->lastbg, &found);
        // Statuses are forgotten, but for the job $! names in a script
        for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
            if (j->done && (sh->interactive || j->pid!=sh->lastbg)) j->notified = 1;
        }
        cleanupjobs(sh);
        while (sh->bgstats) delbgstat(sh, sh->bgstats->pid);
        return 0;
    }

    i32 status = 0;
    i64 lastpid = -1;
    for (; i < nargs; i++) {
        Str a = args[i];
        if (a.len && digit(a[0])) {
            i64 pid;
            b32 num = 1;
            for (iz k = 0; k < a.len; k++) num &= digit(a[k]);
            if (!num || !parsei64(a, &pid)) {
                Str msg = concat3(&scratch, "`", a, "': not a pid or valid job spec");
                builtinerror(sh, scratch, "wait", msg);
                return 1;
            }
            polljobs(sh);
            Job *j = jobbypid(sh, pid);
            if (j && !ownjob(sh, j)) j = 0;
            if (j) {
                if (!waitjob(sh, j, &status, force)) return status;
                if (j->stopped) {
                    lastpid = pid;
                    continue;
                }
                collected(sh, scratch, j);
                if (sh->opt[O_POSIX]) {
                    cleanupjobs(sh);
                    delbgstat(sh, pid);
                }
                lastpid = pid;
                continue;
            }
            Bgstat *b = findbgstat(sh, pid);
            if (b) {
                status = b->status;
                if (sh->opt[O_POSIX]) delbgstat(sh, pid);
                lastpid = pid;
                continue;
            }
            b32 found;
            i32 st = waitpsub(sh, pid, &found);
            if (found) {
                status = st;
                lastpid = pid;
                if (sh->sched->cur->intr || (st>128 && sh->sig->pending)) return st;
                continue;
            }
            Str msg = concat3(&scratch, "pid ", a, " is not a child of this shell");
            builtinerror(sh, scratch, "wait", msg);
            status  = 127;
            lastpid = -1;
        } else if (a.len && a[0]=='%') {
            b32  amb;
            polljobs(sh);
            Job *j = findjob(sh, scratch, "wait", a, &amb);
            if (j && !ownjob(sh, j)) j = 0;
            if (!j) {
                if (!amb) {
                    Str msg = concat(&scratch, a, ": no such job");
                    builtinerror(sh, scratch, "wait", msg);
                }
                status  = 127;
                lastpid = -1;
                continue;
            }
            if (j->stopped) {
                Str msg = concat3(&scratch, "warning: wait_for_job: job ", fmti64(&scratch, j->id), " is stopped");
                shellerror(sh, scratch, msg);
            }
            if (!waitjob(sh, j, &status, force)) return status;
            lastpid = j->pid;
            if (j->stopped) continue;
            collected(sh, scratch, j);
            if (sh->opt[O_POSIX]) cleanupjobs(sh);
        } else {
            Str msg = concat3(&scratch, "`", a, "': not a pid or valid job spec");
            builtinerror(sh, scratch, "wait", msg);
            status  = 1;
            lastpid = -1;
        }
    }
    if (var.data && lastpid>=0) waitvar(sh, scratch, var, lastpid);
    return status;
}

// Drop a job from the table, warning of one left stopped, as bash.
static void disownjob(Shell *sh, Arena scratch, Job *j)
{
    if (j->stopped && !j->done) {
        Buf b(&scratch, 96);
        print(&b, "warning: deleting stopped job ");
        print(&b, (i64)j->id);
        print(&b, " with process group ");
        print(&b, j->pgid ? j->pgid : j->pid);
        shellerror(sh, scratch, finish(&b));
    }
    deletejob(sh, j);
}

static i32 b_disown(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 all = 0, running = 0, nohup = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") { i++; break; }
        for (iz k = 1; k < a.len; k++) {
            switch (a[k]) {
            case 'a': all = 1; break;
            case 'r': running = 1; break;
            case 'h': nohup = 1; break;
            default:
                return jobsusage(sh, scratch, "disown", span(a.data+k-1, a.data+k+1));
            }
        }
    }
    polljobs(sh);
    if (i==nargs && (all || running)) {
        for (Job *j = sh->jobs; j && ownjob(sh, j);) {
            Job *next = j->next;
            if (!running || jobrunning(j)) {
                if (nohup) {
                    j->nohup = 1;
                } else {
                    disownjob(sh, scratch, j);
                }
            }
            j = next;
        }
        return 0;
    }
    i32 status = 0;
    do {
        Job *j = 0;
        Str  a = i<nargs ? args[i] : Str{};
        i64  pid;
        b32  amb = 0;
        if (i == nargs) {
            j = sh->curjob ? jobbyid(sh, sh->curjob) : 0;
        } else if (parsei64(a, &pid)) {
            j = jobbypid(sh, pid);
        } else {
            j = findjob(sh, scratch, "disown", a, &amb);
        }
        if (j && !ownjob(sh, j)) j = 0;
        if (!j) {
            Str shown = i<nargs ? a : Str("current");
            Str msg = concat(&scratch, shown, ": no such job");
            builtinerror(sh, scratch, "disown", msg);
            status = 1;
        } else if (nohup) {
            j->nohup = 1;
        } else {
            disownjob(sh, scratch, j);
        }
        i++;
    } while (i < nargs);
    return status;
}

// The job fg or bg acts on: the one spec names, or the current job. Or
// null after saying why not.
static Job *fgbgjob(Shell *sh, Arena scratch, Str who, Str *spec)
{
    b32  amb = 0;
    Job *j   = spec ? findjob(sh, scratch, who, *spec, &amb)
             : sh->curjob ? jobbyid(sh, sh->curjob) : 0;
    if (!j) {
        if (!amb) {
            Str name = spec ? *spec : Str("current");
            Str msg = concat(&scratch, name, ": no such job");
            builtinerror(sh, scratch, who, msg);
        }
        return 0;
    }
    if (!j->jc) {
        Str msg = concat3(&scratch, "job ", fmti64(&scratch, j->id), " started without job control");
        builtinerror(sh, scratch, who, msg);
        return 0;
    }
    if (!ownjob(sh, j) || sh->incomsub) {
        builtinerror(sh, scratch, who, "no current jobs");
        return 0;
    }
    return j;
}

// fg and bg take no options, but --. Returns the index of the first
// operand, or -1 after a usage error.
static iz fgbgoptions(Shell *sh, Arena scratch, Str *args, iz nargs)
{
    iz i = 1;
    if (i<nargs && args[i]=="--") return i + 1;
    if (i<nargs && args[i].len>1 && args[i][0]=='-') {
        jobsusage(sh, scratch, args[0], span(args[i].data, args[i].data+2));
        return -1;
    }
    return i;
}

// The command a job runs, as fg prints it: with where it started, if
// not here.
static Str jobtext(Shell *sh, Arena *a, Job *j, Str sep)
{
    if (j->wd == sh->wd) return j->text;
    Str wd   = j->wd;
    Got home = getvar(sh, a, "HOME");
    iz  n    = homeprefix(sh, wd, home.value);
    if (n >= 0) wd = concat(a, "~", cuthead(wd, n));
    return concat3(a, j->text, sep, concat3(a, "(wd: ", wd, ")"));
}

static i32 fgstop(Shell *, Arena, Job *);
static void fgforward(Shell *, Job *);

static i32 b_fg(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    if (!sh->jobctl) {
        builtinerror(sh, scratch, "fg", "no job control");
        return 1;
    }
    iz i = fgbgoptions(sh, scratch, args, nargs);
    if (i < 0) return 2;
    polljobs(sh);
    Job *j = fgbgjob(sh, scratch, "fg", i<nargs ? args+i : 0);  // the rest ignored
    if (!j) return 1;
    if (j->done) {
        builtinerror(sh, scratch, "fg", "job has terminated");
        return 1;
    }
    setcurrent(sh, j->id);
    bprint(sh, concat(&scratch, jobtext(sh, &scratch, j, "\t"), "\n"));
    flushbout(sh);
    // As a foreground command: trapped signals wait for its end
    b32 term = 0;  // the terminal's INT reaches only the job
    if (j->task && iswaiter(j->task) && j->pgid) {
        // Its group holds the terminal, continued if stopped, until it
        // ends or stops again
        b32 tty = sh->jctty != BADHANDLE;
        if (tty) os_tcpgrp(sh->os, sh->jctty, j->pgid);
        if (j->stopped) {
            os_kill(sh->os, -j->pgid, 18);
            j->stopped = 0;
        }
        joinorstop(sh, j, 0);
        if (tty) os_tcpgrp(sh->os, sh->jctty, sh->shellpgid);
        if (j->stopped) return fgstop(sh, scratch, j);
        joindone(sh, j);
        term = tty;
    } else if (j->task) {
        if (sh->win32 && iswaiter(j->task)) {
            fgforward(sh, j);
            term = 1;
        }
        jointask(sh->sched, j->task, &j->status);
        j->task = 0;
        j->done = 1;
        coprocended(sh, j->pid);
        chldreaped(sh, 1);
    }
    if (term && (j->status & WSIGNALED) && (j->status & 255)==128+2) {
        // As runprogram: the terminal's INT reached only the job
        if (sh->loopdepth || (!trapset(sh, 2) && sh->interactive)) {
            sh->sig->pending |= sigbit(2);
        }
    }
    i32 st  = j->status;
    i64 pid = j->pid;
    Str txt = clone(&scratch, jobtext(sh, &scratch, j, "  "));
    j->notified = 1;
    deletejob(sh, j);
    delbgstat(sh, pid);
    if (st & WSIGNALED) reportsig(sh, scratch, (st & 255) - 128, pid, txt);
    return st & 255;
}

static i32 b_bg(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    if (!sh->jobctl) {
        builtinerror(sh, scratch, "bg", "no job control");
        return 1;
    }
    iz i = fgbgoptions(sh, scratch, args, nargs);
    if (i < 0) return 2;
    polljobs(sh);
    i32 status = 0;
    do {
        Job *j = fgbgjob(sh, scratch, "bg", i<nargs ? args+i : 0);
        if (!j) {
            status = 1;
        } else if (j->done) {
            builtinerror(sh, scratch, "bg", "job has terminated");
            status = 1;
        } else if (j->stopped && j->pgid) {
            os_kill(sh->os, -j->pgid, 18);
            j->stopped = 0;
            sh->lastbg = j->pgid;
            Buf b(&scratch, 128);
            print(&b, "[");
            print(&b, (i64)j->id);
            print(&b, "]");
            putbyte(&b, j->id==sh->curjob ? '+' : j->id==sh->prevjob ? '-' : ' ');
            putbyte(&b, ' ');
            print(&b, j->text);
            print(&b, " &\n");
            bprint(sh, finish(&b));
        } else {
            sh->lastbg = j->pid;  // as bash, its process group
            Str msg = concat3(&scratch, "job ", fmti64(&scratch, j->id), " already in background");
            builtinerror(sh, scratch, "bg", msg);
        }
    } while (++i < nargs);
    return status;
}

static i32 b_suspend(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 force = 0;
    for (iz i = 1; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") break;
        for (iz k = 1; k < a.len; k++) {
            if (a[k] != 'f') return jobsusage(sh, scratch, "suspend", span(a.data+k-1, a.data+k+1));
            force = 1;
        }
    }
    if (!sh->jobctl || !sh->shellpgid) {
        builtinerror(sh, scratch, "suspend", "cannot suspend: no job control");
        return 1;
    }
    if (sh->login && !force) {
        builtinerror(sh, scratch, "suspend", "cannot suspend a login shell");
        return 1;
    }
    // As bash, the shell's whole group stops; continued, it takes the
    // terminal back
    flushbout(sh);
    os_kill(sh->os, -sh->shellpgid, 19);
    if (sh->jctty != BADHANDLE) os_tcpgrp(sh->os, sh->jctty, sh->shellpgid);
    return 0;
}

// Windows has no process groups: the console's Ctrl-C and Ctrl-Break
// reach this shell, not the job fg waits for, which was started
// ignoring them. Until it ends, pass them on to it (os_kill: a shell
// of this image takes the signal, another program ends as by Ctrl-C),
// as bash's terminal sends them to its foreground job, and the shell
// does not take them itself.
static void fgforward(Shell *sh, Job *j)
{
    Sched   *s    = sh->sched;
    Task    *self = s->cur;
    Task    *t    = j->task;
    Sigproc *p    = sh->sig;
    u64      ctl  = sigbit(2) | sigbit(3);
    self->introk = 1;
    while (t->state != TS_DONE) {
        t->joiner = self;
        block(s, W_JOIN);
        takeossigs(sh);
        u64 got = p->pending & ctl;
        p->pending &= ~got;
        self->intr  = 0;
        for (; got; got &= got-1) {
            // to its job where the platform has one (what it started)
            i32 sig = __builtin_ctzll(got) + 1;
            if (os_kill(sh->os, -j->pid, sig)) os_kill(sh->os, j->pid, sig);
        }
    }
    t->joiner    = 0;
    self->introk = 0;
}

// A foreground job stopped: it becomes the current job, and the
// command's status is 128 plus the signal. An interactive shell says
// so (a script does not), and a stop by TSTP breaks out of every loop
// running, as bash.
static i32 fgstop(Shell *sh, Arena scratch, Job *j)
{
    setcurrent(sh, j->id);
    if (sh->interactive) {
        flushbout(sh);
        fdwrite(sh, 2, concat(&scratch, "\n", jobline(sh, &scratch, j, 0)));
        j->notified = 1;
    }
    if (j->stopsig==20 && sh->loopdepth) {
        sh->flow      = FLOW_BREAK;
        sh->flowcount = sh->loopdepth;
    }
    return 128 + j->stopsig;
}

// The foreground program runprogram waited for stopped by sig: it
// becomes a stopped job, its text the command's.
static i32 fgstopped(Shell *sh, Arena scratch, Handle proc, i64 pid, i32 sig)
{
    Node  none = {};
    none.kind = N_SIMPLE;
    Task *t = spawnwaiter(sh->sched, proc, 1);
    Job  *j = newjob(sh, scratch, sh->current ? sh->current : &none, t, pid, 0);
    j->pgid    = pid;
    j->stopped = 1;
    j->stopsig = sig;
    return fgstop(sh, scratch, j);
}

// Asked to exit, an interactive shell with stopped jobs (or running
// ones, under checkjobs) says so and stays: true then. An exit right
// after goes ahead.
static b32 exitwarning(Shell *sh)
{
    if (!sh->interactive || sh->subshell || !sh->jobctl) return 0;
    if (sh->exitwarned && sh->cmdnumber-sh->exitwarned<=1) return 0;
    polljobs(sh);
    b32 stopped = 0, running = 0;
    for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
        stopped |= j->stopped && !j->done;
        running |= jobrunning(j);
    }
    Str msg = {};
    if (stopped) {
        msg = "There are stopped jobs.\n";
    } else if (running && sh->shopt[S_CHECKJOBS]) {
        msg = "There are running jobs.\n";
    }
    if (!msg.len) return 0;
    flushbout(sh);
    fdwrite(sh, 2, msg);
    sh->exitwarned = sh->cmdnumber;
    return 1;
}
