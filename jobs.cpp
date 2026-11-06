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

enum { MAXBG = 4 };   // live background vprocs before & waits for one

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

// A job whose program a waiter watches, rather than a vproc. Its wait
// is submitted at once (spawnwaiter), so that the program's end is
// noticed even before the waiter first runs.
static void waitermain(Task *t)
{
    while (!t->io.done) {
        block(t->s, W_IO);
    }
    t->status = t->io.err ? 127 : (i32)t->io.n & ~(WMISSED|WNOLIB);
}

static Task *spawnwaiter(Sched *s, Handle proc)
{
    Task *t = taskspawn(s, waitermain, (void *)proc, SMALLSTACK);
    t->io.op  = IO_PROC;
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
        if (iswaiter(t) && t->state!=TS_DONE) {
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


// The table

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
        b32 exited = iswaiter(t) && t->state==TS_READY && t->io.op==IO_PROC && t->io.done;
        if (t->state==TS_DONE || exited) {
            jointask(s, t, &j->status);
            j->task = 0;
            j->done = 1;
            j->notified = 0;  // a change jobs has not shown
            coprocended(sh, j->pid);
        }
    }
}

// Is a job running, as far as the shell knows?
static b32 jobrunning(Job *j)
{
    return !j->done;
}

// The newest running job older than job id (or of all, if 0).
static i32 lastrunning(Shell *sh, i32 id)
{
    for (Job *j = sh->jobs; j; j = j->next) {
        if ((!id || j->id<id) && jobrunning(j)) return j->id;
    }
    return 0;
}

// Bash's current (+) and previous (-) jobs: the newest running job,
// and the next newest.
static void setcurrent(Shell *sh, i32 id)
{
    if (sh->curjob != id) {
        sh->prevjob = sh->curjob;
        sh->curjob  = id;
    }
    Job *cur = jobbyid(sh, id);
    i32  cand = cur && jobrunning(cur) ? lastrunning(sh, id) : lastrunning(sh, 0);
    sh->prevjob = cand ? cand : sh->curjob;
}

static void resetcurrent(Shell *sh)
{
    i32 cand = lastrunning(sh, 0);
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
static Str jobstate(Shell *sh, Arena *a, Job *j)
{
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

// List one job: form 0 as jobs, 'l' with its pid, 'p' its pid alone.
static void printjob(Shell *sh, Arena a, Job *j, i32 form)
{
    if (form == 'p') {
        bprinti(sh, j->pid);
        bprint(sh, "\n");
        return;
    }
    Buf b(&a, 128);
    print(&b, "[");
    print(&b, (i64)j->id);
    print(&b, "]");
    putbyte(&b, j->id==sh->curjob ? '+' : j->id==sh->prevjob ? '-' : ' ');
    putbyte(&b, ' ');
    if (form == 'l') {
        padpid(&b, j->pid);
    }
    putbyte(&b, ' ');
    Str st = jobstate(sh, &a, j);
    print(&b, st);
    for (iz i = st.len; i < 27; i++) putbyte(&b, ' ');
    print(&b, j->text);
    if (jobrunning(j)) print(&b, " &");
    if (!(j->wd == sh->wd)) {
        // As bash's polite form: HOME as ~
        Str wd   = j->wd;
        Got home = getvar(sh, &a, "HOME");
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
    bprint(sh, finish(&b));
    if (ownjob(sh, j)) j->notified = 1;
}

// Before each top-level command: report background jobs killed by a
// signal, as bash does for any but INT, TERM and PIPE and those this
// shell traps, then drop the reported jobs.
static void notifyjobs(Shell *sh, Arena scratch)
{
    polljobs(sh);
    for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
        if (!j->done || j->notified || !(j->status & WSIGNALED)) continue;
        i32 sig = (j->status & 255) - 128;
        if (sig==2 || sig==13 || sig==15 || !longreport(sh, sig) || sh->noreport) continue;
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
    cleanupjobs(sh);
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
            builtinerror(sh, scratch, who, concat(&scratch, w, ": ambiguous job spec"));
            *ambiguous = 1;
            return 0;
        }
        found = j;
    }
    return found;
}

static Job *jobbypid(Shell *sh, i64 pid)
{
    for (Job *j = sh->jobs; j; j = j->next) {
        if (j->pid == pid) return j;
    }
    return 0;
}

// Wait for one of this level's jobs. False if a trapped signal
// interrupted the wait, with *status 128 plus its number.
static b32 waitjob(Shell *sh, Job *j, i32 *status)
{
    if (j->task) {
        i32 st;
        b32 waiter = iswaiter(j->task);
        if (!taskjoinintr(sh->sched, j->task, &st)) {
            sh->sched->cur->intr = 0;
            u64 pend = sh->sig->pending;
            *status = pend ? 128 + __builtin_ctzll(pend) + 1 : 128;
            return 0;
        }
        sh->sched->nbg -= !waiter;
        j->task   = 0;
        j->done   = 1;
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
// run a copy of n in arena a, which it returns.
static Node *bgnode(Shell *c, Arena *a, Node *n)
{
    c->loopdepth = 0;
    c->jobs = 0;  // as a forked subshell, it starts with no jobs
    c->curjob = c->prevjob = 0;
    c->bgstats = 0;
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
        }
        r.fds[r.nfds++] = sf;
    }
    return r;
}

// Start node as a background shell process continuing from an image of
// this one (notes/m5b.md), its descriptors 0 and 1 replaced when in and
// out are given. Returns its process handle, with *pid its pid, or -1
// where the platform cannot (then it runs as a vproc).
static i64 spawnimage(Shell *sh, Node *n, File *in, File *out, i64 *pid,
                      Arena scratch)
{
    ImageRef im = os_imagenew(sh->os, sh->reserve);
    if (!im.base) return -1;
    BgFds fds = bgfds(sh, &scratch, in, out);
    Image img = imagebuild(sh, n, im);

    Spawn sp = {};
    sp.image  = img.ref;
    sp.argv   = &sh->shellname;
    sp.argc   = 1;
    sp.cwd    = spawncwd(sh, &scratch);
    sp.fds    = fds.fds;
    sp.nfds   = fds.nfds;
    sp.sigign = ignoredsigs(sh) | (u64)1<<1 | (u64)1<<2;  // INT, QUIT
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

// Start node as a background vproc, its descriptors 0 and 1 replaced
// when in and out are given. Returns the task, with *pid its BASHPID.
static Task *spawnbg(Shell *sh, Node *n, File *in, File *out, i64 *pid,
                     Arena scratch)
{
    // A here-document it inherits becomes an OS object first, so that
    // both read one open file, as bash's forked child shares the
    // temporary file or pipe: { read x; } & reads only its own line
    Fdtab *t = sh->fds;
    for (i32 fd = in ? 1 : 0; fd < t->top; fd++) {
        File *f = t->fd[fd];
        if (f && f->kind==F_MEM) {
            byte  mem[256];
            Arena a = {mem, mem+sizeof(mem)};
            materialize(sh, a, f);
        }
    }
    i64   proc = spawnimage(sh, n, in, out, pid, scratch);
    if (proc >= 0) return spawnwaiter(sh->sched, (Handle)proc);
    Vproc *m = vprocwith(sh, in, out);
    Shell *c = m->sh;
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
    Sched *s = sh->sched;
    polljobs(sh);
    cleanupjobs(sh);
    throttlejobs(sh);
    // Without job control, its input is /dev/null unless redirected
    b32   nullin = !sh->stdinredir && (!sh->interactive || !sh->opt[O_MONITOR]);
    File *in     = nullin ? devnull(sh, scratch) : 0;
    Task *t      = 0;
    i64   pid    = 0;
    b32   retry  = 1;
    i32   status = 0;
    if (directbg(sh, scratch, n, nullin)) {
        u64 ign = sh->asyncign;
        sh->asyncign |= (u64)1<<1 | (u64)1<<2;  // INT, QUIT
        Handle proc = spawndirect(sh, n, in, 0, scratch, &status, &retry, 0);
        sh->asyncign = ign;
        pid = sh->childpid;
        if (proc != BADHANDLE) {
            t = spawnwaiter(s, proc);
        } else if (!retry) {
            pid = ++sh->lastpid;  // it failed as its own process would
        }
    }
    if (retry) {
        t = spawnbg(sh, n, in, 0, &pid, scratch);
    }
    fileunref(sh, in);
    Job *j = newjob(sh, scratch, n, t, pid, 0);
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

// The pid a kill job spec names, or -1 after reporting why not.
static i64 jobspecpid(Shell *sh, Arena scratch, Str who, Str spec)
{
    polljobs(sh);
    b32  amb;
    Job *j = findjob(sh, scratch, who, spec, &amb);
    if (!j) {
        if (!amb) builtinerror(sh, scratch, who, concat(&scratch, spec, ": no such job"));
        return -1;
    }
    return j->pid;
}

static void setcoprocvars(Shell *sh, Arena scratch, Coproc *c)
{
    i32 flags = 0;
    Str name  = arraytarget(sh, scratch, c->name, &flags, 1);
    if (!name.data) return;
    Value *v = assignslot(sh, scratch, name, flags);
    if (!v) return;
    toarray(sh, v);
    setelem(sh, scratch, name, 0, {}, fmti64(&scratch, c->rfd), flags);
    setelem(sh, scratch, name, 1, {}, fmti64(&scratch, c->wfd), flags);
    setvar(sh, scratch, concat(&scratch, c->name, "_PID"), fmti64(&scratch, c->pid));
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
        shellerror(sh, scratch, concat(&scratch, c->name, ": cannot unset: readonly variable"));
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
        shellerror(sh, scratch, concat3(&scratch, "`", name, "': not a valid identifier"));
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
        shellerror(sh, scratch, concat(&scratch, "cannot make pipe for coprocess: ", errtext(p2.err)));
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
            pid = ++sh->lastpid;
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
            if (stopped || (running && !jobrunning(j)) || (changed && j->notified)) continue;
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
            builtinerror(sh, scratch, "jobs", concat(&scratch, args[i], ": no such job"));
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
    if (var.len) assignto(sh, scratch, var, fmti64(&scratch, pid));
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
                if (!amb) builtinerror(sh, scratch, "wait", concat(&scratch, ids[i], ": no such job"));
            } else {
                builtinerror(sh, scratch, "wait", concat3(&scratch, "`", ids[i], "': not a pid or valid job spec"));
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
            if (j->task) live = j;
        }
        if (found) {
            i32 st  = found->status & 255;
            i64 pid = found->pid;
            deletejob(sh, found);
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

static i32 b_wait(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 any = 0;
    Str var = {};
    iz  i   = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") { i++; break; }
        for (iz k = 1; k < a.len; k++) {
            u8 c = a[k];
            if (c == 'n') {
                any = 1;
            } else if (c == 'f') {
                // without job control, as plain wait
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
            builtinerror(sh, scratch, "wait", concat(&scratch, var, ": cannot unset: readonly variable"));
            return 1;
        }
    }
    if (any) {
        i32 st = waitany(sh, scratch, args+i, nargs-i, var);
        return st;
    }

    if (i == nargs) {
        // Every running job, then the last procsub if $! names it
        for (;;) {
            polljobs(sh);
            Job *live = 0;
            for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
                if (j->task) live = j;  // the oldest
            }
            if (!live) break;
            i32 st;
            if (!waitjob(sh, live, &st)) return st;
            live->notified = 1;  // waited for: dropped, even $!'s
        }
        b32 found;
        waitpsub(sh, sh->lastbg, &found);
        // Statuses are forgotten, but for the job $! names
        for (Job *j = sh->jobs; j && ownjob(sh, j); j = j->next) {
            if (j->done && j->pid!=sh->lastbg) j->notified = 1;
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
                builtinerror(sh, scratch, "wait", concat3(&scratch, "`", a, "': not a pid or valid job spec"));
                return 1;
            }
            polljobs(sh);
            Job *j = jobbypid(sh, pid);
            if (j && !ownjob(sh, j)) j = 0;
            if (j) {
                if (!waitjob(sh, j, &status)) return status;
                j->notified = 1;
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
            builtinerror(sh, scratch, "wait", concat3(&scratch, "pid ", a, " is not a child of this shell"));
            status  = 127;
            lastpid = -1;
        } else if (a.len && a[0]=='%') {
            b32  amb;
            polljobs(sh);
            Job *j = findjob(sh, scratch, "wait", a, &amb);
            if (j && !ownjob(sh, j)) j = 0;
            if (!j) {
                if (!amb) builtinerror(sh, scratch, "wait", concat(&scratch, a, ": no such job"));
                status  = 127;
                lastpid = -1;
                continue;
            }
            if (!waitjob(sh, j, &status)) return status;
            j->notified = 1;
            lastpid = j->pid;
            if (sh->opt[O_POSIX]) cleanupjobs(sh);
        } else {
            builtinerror(sh, scratch, "wait", concat3(&scratch, "`", a, "': not a pid or valid job spec"));
            status  = 1;
            lastpid = -1;
        }
    }
    if (var.data && lastpid>=0) waitvar(sh, scratch, var, lastpid);
    return status;
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
                    deletejob(sh, j);
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
            builtinerror(sh, scratch, "disown", concat(&scratch, shown, ": no such job"));
            status = 1;
        } else if (nohup) {
            j->nohup = 1;
        } else {
            deletejob(sh, j);
        }
        i++;
    } while (i < nargs);
    return status;
}

static i32 b_fg(Shell *sh, Str *args, iz, Arena scratch)
{
    builtinerror(sh, scratch, args[0], "no job control");
    return 1;
}

static i32 b_suspend(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    for (iz i = 1; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") break;
        for (iz k = 1; k < a.len; k++) {
            if (a[k] != 'f') return jobsusage(sh, scratch, "suspend", span(a.data+k-1, a.data+k+1));
        }
    }
    builtinerror(sh, scratch, "suspend", "cannot suspend: no job control");
    return 1;
}
