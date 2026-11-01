// Vprocs and pipelines
//
// A fiber vproc runs shell code concurrently with its parent: every
// shell stage of a pipeline but the last. It owns a deep copy of the
// parent's state, made by vproccopy: the Shell, a store of its own
// holding the variables, arrays, function table, aliases, remembered
// commands and positional parameters, the call frames, the command it
// runs, and its open files. No store memory is shared between live
// vprocs. Function bodies (Code) are shared by reference count.
//
// A pipeline creates every stage first: in-memory Pipes connect shell
// stages, OS pipes connect stages next to a likely external command.
// Stages but the last start as fiber vprocs; the last runs on the
// current stack, in a synchronous subshell unless lastpipe applies.
// The pipeline then joins every stage. See notes/m3a.md.
//
// This is free and unencumbered software released into the public domain.

// Memory for a fiber vproc, pooled in the Sched and reused.
struct Vproc {
    Vproc *next;          // free list
    Arena  perm;          // the Shell, its frames and node: reset per use
    byte  *permbase;
    Store  store;         // the vproc's store
    byte  *storebase;
    Arena *pool[NPOOL];   // scratch arenas by depth, kept across uses
    Shell *sh;
    Node  *node;          // what it runs, or else
    Str    src;           // source text to parse and run (procsubs)
    Sigproc sig;          // its pending signals, from creation to exit
};

static void copytraps(Shell *c, Shell *sh);
static void reportpipeline(Shell *, Arena, Node **stages, i32 n, i32 *statuses,
                           i64 *pids, Node *job);

static Vproc *newvproc(Sched *s)
{
    Vproc *m = s->freevprocs;
    if (m) {
        s->freevprocs = m->next;
    } else {
        Arena a = newarena(s->os, s->reserve);
        m = alloc<Vproc>(&a);
        m->perm      = a;
        m->permbase  = a.beg;
        m->store.a   = newarena(s->os, s->reserve);
        m->storebase = m->store.a.beg;
    }
    m->next     = 0;
    m->perm.beg = m->permbase;
    Arena sa = m->store.a;
    sa.beg   = m->storebase;
    m->store = {};
    m->store.a    = sa;
    m->store.base = sa.beg;
    return m;
}


// The copier

static Value *copyvalues(Store *st, Value *v)
{
    Value  *head = 0;
    Value **tail = &head;
    for (; v; v = v->below) {
        Value *c = (Value *)salloc(st, sizeof(Value));
        *c = *v;
        c->below = 0;
        c->str   = sclone(st, v->str);
        c->arr   = v->arr ? acopy(st, v->arr, 0) : 0;
        *tail = c;
        tail  = &c->below;
    }
    return head;
}

static Var *copyvars(Store *st, Var *v)
{
    if (!v) return 0;
    Var *c = (Var *)salloc(st, sizeof(Var));
    c->name = sclone(st, v->name);
    c->top  = copyvalues(st, v->top);
    for (i32 i = 0; i < 4; i++) {
        c->child[i] = copyvars(st, v->child[i]);
    }
    return c;
}

// Function bodies are shared: each copy holds a reference.
static Func *copyfuncs(Store *st, Func *f)
{
    if (!f) return 0;
    Func *c = (Func *)salloc(st, sizeof(Func));
    c->name  = sclone(st, f->name);
    c->body  = f->body;
    c->code  = f->code;
    c->attrs = f->attrs;
    if (c->code) c->code->refs++;
    for (i32 i = 0; i < 4; i++) {
        c->child[i] = copyfuncs(st, f->child[i]);
    }
    return c;
}

// Drop the references a vproc holds on other vprocs' function bodies.
static void releasefuncs(Shell *sh, Func *f)
{
    if (!f) return;
    Code *c = f->code;
    if (c && c->store!=sh->store && !--c->refs && c->dead) {
        freecode(sh, c);
    }
    for (i32 i = 0; i < 4; i++) {
        releasefuncs(sh, f->child[i]);
    }
}

static Alias *copyaliases(Store *st, Alias *a)
{
    if (!a) return 0;
    Alias *c = (Alias *)salloc(st, sizeof(Alias));
    c->name  = sclone(st, a->name);
    c->value = sclone(st, a->value);
    for (i32 i = 0; i < 4; i++) {
        c->child[i] = copyaliases(st, a->child[i]);
    }
    return c;
}

static HashCmd **copyhash(Store *st, HashCmd **h)
{
    if (!h) return 0;
    HashCmd **r = (HashCmd **)salloc(st, NHASHCMD*(iz)sizeof(HashCmd *));
    for (i32 b = 0; b < NHASHCMD; b++) {
        HashCmd **tail = r + b;
        for (HashCmd *e = h[b]; e; e = e->next) {
            HashCmd *c = (HashCmd *)salloc(st, sizeof(HashCmd));
            c->name = sclone(st, e->name);
            c->path = sclone(st, e->path);
            c->hits = e->hits;
            *tail = c;
            tail  = &c->next;
        }
    }
    return r;
}

// Copy the call frames, pointing their locals at the copied variables.
static Frame *copyframes(Shell *c, Arena *a, Frame *f)
{
    Store  *st   = c->store;
    Frame  *head = 0;
    Frame **tail = &head;
    for (; f; f = f->up) {
        Frame *r = alloc<Frame>(a);
        *r = *f;
        r->up   = 0;
        r->name = clone(a, f->name);
        r->file = clone(a, f->file);
        if (f->caplocals) {
            r->locals = (Var **)salloc(st, f->caplocals*(iz)sizeof(Var *));
            for (iz i = 0; i < f->nlocals; i++) {
                r->locals[i] = findvar(c, f->locals[i]->name);
            }
        }
        if (f->opts) {
            r->opts = (b32 *)salloc(st, O_COUNT*(iz)sizeof(b32));
            for (i32 i = 0; i < O_COUNT; i++) r->opts[i] = f->opts[i];
        }
        *tail = r;
        tail  = &r->up;
    }
    return head;
}

// A File of vproc c for the same object as f: a duplicate handle, the
// same capture buffer, a copy of the text, or another end of the pipe.
static File *copyfile(Shell *c, File *f)
{
    File *r = newfile(c, f->kind, f->flags);
    switch (f->kind) {
    case F_OS:
        r->h = os_dup(c->os, f->h);
        break;
    case F_CAPTURE:
        r->cap = f->cap;
        break;
    case F_MEM:
        r->mem    = sclone(c->store, f->mem);
        r->ownmem = 1;
        r->pos    = f->pos;
        break;
    case F_PIPE:
        r->pipe = f->pipe;
        if (f->flags & OF_READ) {
            f->pipe->readers++;
        } else {
            f->pipe->writers++;
        }
        break;
    }
    return r;
}

static Fdtab *copyfdtab(Shell *c, Arena *a, Fdtab *t)
{
    Fdtab *r = alloc<Fdtab>(a);
    File  *from[64];
    File  *to[64];
    i32    n = 0;
    for (i32 fd = 0; fd < t->top; fd++) {
        File *f = t->fd[fd];
        if (!f) continue;
        // Descriptors sharing a File share its copy too
        File *same = 0;
        for (i32 i = 0; i < n; i++) {
            if (from[i] == f) same = to[i];
        }
        if (same) {
            same->refs++;
            r->fd[fd] = same;
            continue;
        }
        r->fd[fd] = copyfile(c, f);
        if (n < countof(from)) {
            from[n] = f;
            to[n++] = r->fd[fd];
        }
    }
    r->top = t->top;
    return r;
}

// Deep-copy a shell's state into vproc memory m, as a subshell of it.
static Shell *vproccopy(Shell *sh, Vproc *m)
{
    Arena *a  = &m->perm;
    Store *st = &m->store;
    Shell *c  = alloc<Shell>(a);
    *c = *sh;
    c->store   = st;
    c->reserve = sh->sched->reserve;
    for (i32 i = 0; i < NPOOL; i++) c->pool[i] = m->pool[i];

    c->vars    = copyvars(st, sh->vars);
    c->funcs   = copyfuncs(st, sh->funcs);
    c->aliases = copyaliases(st, sh->aliases);
    c->cmdhash = copyhash(st, sh->cmdhash);
    c->args.v  = (Str *)salloc(st, (sh->args.n ? sh->args.n : 1)*(iz)sizeof(Str));
    for (iz i = 0; i < sh->args.n; i++) {
        c->args.v[i] = sclone(st, sh->args.v[i]);
    }
    c->name       = sclone(st, sh->name);
    c->iname      = sclone(st, sh->iname);
    c->srcname    = sclone(st, sh->srcname);
    c->argv0      = sclone(st, sh->argv0);
    c->cwd        = sclone(st, sh->cwd);
    c->underscore = sclone(st, sh->underscore);
    c->host       = sclone(st, sh->host);
    Arena tmp = *a;
    c->cmdtext    = sclone(st, cmdtext(sh, &tmp));
    c->cmdnode    = 0;
    c->frame      = copyframes(c, a, sh->frame);
    c->fds        = copyfdtab(c, a, sh->fds);

    c->undo     = 0;
    c->esc      = 0;
    c->bout     = 0;
    c->lexer    = 0;
    c->current  = 0;
    c->compargs = {};
    c->capdepth = 0;
    c->psubs    = 0;
    c->flow     = FLOW_NONE;
    c->exiting  = 0;
    c->subshell++;
    c->bashpid  = ++sh->lastpid;
    c->lastpid  = sh->lastpid;
    c->killsig  = 0;
    c->childsig = 0;
    c->errexited = 0;
    c->intrap   = 0;
    c->trapbusy = 0;
    c->intrread = 0;
    copytraps(c, sh);
    sigenter(c, &m->sig, c->bashpid, 0);
    return c;
}

// Fiber vproc entry: run the node, then release everything.
static void vprocmain(Task *t)
{
    Vproc *m  = (Vproc *)t->arg;
    Shell *sh = m->sh;
    Escape top = {};
    sh->esc = &top;
    byte stackmark = 0;
    sh->stacktop = &stackmark;
    iz room = t->stack - ((iz)1<<16);  // the parent's limit, if smaller
    sh->stackmax = sh->stackmax<room ? sh->stackmax : room;
    m->sig.task = t;
    if (__builtin_setjmp(top.jmp)) {
        // escaped
    } else if (m->node) {
        execute(sh, m->node, *scratchat(sh, 0));
    } else {
        // A procsub: as a command substitution, parsed only now
        sh->tracedepth++;
        sh->opt[O_VERBOSE] = 0;
        runsource(sh, m->src, "process substitution", *scratchat(sh, 0), 0, sh->line);
    }
    if (!sh->execed && !sh->killsig) {
        // Its own EXIT trap, set in the vproc
        sh->esc = &top;
        if (sh->errexited) unwindtemp(sh, 0);
        if (!__builtin_setjmp(top.jmp)) {
            runexittrap(sh, *scratchat(sh, 0));
        }
    }
    b32 tail = sh->childsig && sh->taillevel==sh->subshell+1;  // its last program's
    t->status = (sh->status & 255) | (sh->killsig || tail ? WSIGNALED : 0);
    sigleave(sh, &m->sig);

    unwind(sh, 0);
    closeall(sh);
    joinpsubs(sh);
    releasefuncs(sh, sh->funcs);
    for (i32 i = 0; i < NPOOL; i++) m->pool[i] = sh->pool[i];
    Sched *s = sh->sched;
    m->next = s->freevprocs;
    s->freevprocs = m;
}

// Start node running concurrently in a subshell of sh, a fiber vproc,
// with its standard input and output replaced when in or out is given
// (the caller keeps its references to them). The caller must join the
// returned task, whose status is the subshell's.
static Vproc *vprocwith(Shell *sh, File *in, File *out)
{
    Vproc *m = newvproc(sh->sched);
    Shell *c = vproccopy(sh, m);
    Fdtab *t = c->fds;
    if (in) {
        fileunref(c, t->fd[0]);
        t->fd[0] = copyfile(c, in);
    }
    if (out) {
        fileunref(c, t->fd[1]);
        t->fd[1] = copyfile(c, out);
    }
    for (i32 fd = 0; fd < 2; fd++) {
        if (t->fd[fd] && fd>=t->top) t->top = fd + 1;
    }
    m->sh   = c;
    m->node = 0;
    m->src  = {};
    return m;
}

static Task *spawnvproc(Shell *sh, Node *node, File *in, File *out, b32 newloops)
{
    Vproc *m = vprocwith(sh, in, out);
    Shell *c = m->sh;
    if (newloops) c->loopdepth = 0;
    c->simplestages += node->kind == N_SIMPLE;
    m->node = copynode(&m->perm, node);
    // A simple command, or the last of a ( list ), is what bash would
    // exec in the forked stage: not a { list }'s
    c->tailnode  = node->kind==N_SIMPLE ? m->node : 0;
    if (m->node && m->node->kind==N_SUBSHELL) {
        // ( list ) runs in the vproc's own subshell, not a nested one
        m->node->kind = N_GROUP;
        c->tailnode = tailcommand(m->node->a);
    }
    c->taillevel = c->subshell + 1;
    return taskspawn(sh->sched, vprocmain, m, BIGSTACK);
}


// Process substitution
//
// <(list) and >(list) run list in a fiber vproc whose standard output
// (or input) is one end of a pipe. The other end opens in the shell at
// a free descriptor from 63 down, as bash, and the word becomes
// /dev/fd/N: the shell's redirections and builtins (openfile) and the
// platform's programs (vos, POSIX) open that name as a duplicate. The
// descriptor closes when the command ends (an Undo record), but the
// vproc runs on, unwaited as in bash, on its creator's list (psubs):
// finished ones are reaped when the next starts; wait joins one; the
// end of the creating subshell, vproc or shell joins the rest.

// Does a procsub body start with a likely external command? Then its
// pipe is an OS pipe, which the program writes directly. A wrong guess
// costs only speed.
static b32 likelyexternaltext(Shell *sh, Str body)
{
    iz i = 0;
    for (; i<body.len && (body[i]==' ' || body[i]=='\t'); i++) {}
    iz beg = i;
    for (; i<body.len && body[i]!=' ' && body[i]!='\t' && body[i]!='\n'; i++) {
        u8 c = body[i];
        if (!alnum(c) && c!='_' && c!='-' && c!='.' && c!='/' && c!='+') return 0;
    }
    Str name = span(body.data+beg, body.data+i);
    for (iz k = 0; k < countof(reserved); k++) {
        if (reserved[k].word == name) return 0;
    }
    return name.len && !findbuiltin(name) && !findfunc(sh, name);
}

// Join finished procsubs.
static void reappsubs(Shell *sh)
{
    for (Task **p = &sh->psubs; *p;) {
        Task *t = *p;
        if (t->state == TS_DONE) {
            *p = t->sib;
            sh->psubpid    = t->pid;
            sh->psubstatus = taskjoin(sh->sched, t) & 255;
        } else {
            p = &t->sib;
        }
    }
}

// Join every procsub started at this level, once its descriptors are
// closed.
static void joinpsubs(Shell *sh)
{
    while (sh->psubs) {
        Task *t = sh->psubs;
        sh->psubs = t->sib;
        taskjoin(sh->sched, t);
    }
}

// wait for a procsub by its pid. Sets *found if pid was one. A signal
// with a trap interrupts the wait: then 128 plus its number.
static i32 waitpsub(Shell *sh, i64 pid, b32 *found)
{
    *found = 1;
    for (Task **p = &sh->psubs; *p; p = &(*p)->sib) {
        Task *t = *p;
        if (t->pid == pid) {
            i32 st;
            if (!taskjoinintr(sh->sched, t, &st)) {
                sh->sched->cur->intr = 0;
                u64 pend = sh->sig->pending;
                return pend ? 128 + __builtin_ctzll(pend) + 1 : 128;
            }
            *p = t->sib;
            sh->psubpid    = pid;
            sh->psubstatus = st & 255;
            return sh->psubstatus;
        }
    }
    if (pid && pid==sh->psubpid) return sh->psubstatus;
    *found = 0;
    return 127;
}

// Close every descriptor, as a process does when it exits.
static void closeall(Shell *sh)
{
    Fdtab *fds = sh->fds;
    for (i32 fd = 0; fd < fds->top; fd++) {
        fileunref(sh, fds->fd[fd]);
        fds->fd[fd] = 0;
    }
    fds->top = 0;
}

static Str procsubst(Shell *sh, Arena *a, Str body, b32 output)
{
    if (sh->psubs) {
        // Earlier ones may finish now, as forked children would have
        yield(sh->sched);
        reappsubs(sh);
    }
    Fdtab *t  = sh->fds;
    i32    fd = 63;
    for (; fd>3 && t->fd[fd]; fd--) {}
    if (t->fd[fd]) {
        for (fd = 64; fd<FDMAX && t->fd[fd]; fd++) {}
    }
    File *mine   = 0;
    File *theirs = 0;
    if (fd < FDMAX && likelyexternaltext(sh, body)) {
        OsPipe p = os_pipe(sh->os);
        if (!p.err) {
            mine   = newfile(sh, F_OS, output ? OF_WRITE : OF_READ);
            mine->h   = output ? p.w : p.r;
            theirs = newfile(sh, F_OS, output ? OF_READ : OF_WRITE);
            theirs->h = output ? p.r : p.w;
        }
    } else if (fd < FDMAX) {
        Pipe *p = newpipe(sh->sched);
        mine   = newfile(sh, F_PIPE, output ? OF_WRITE : OF_READ);
        mine->pipe   = p;
        theirs = newfile(sh, F_PIPE, output ? OF_READ : OF_WRITE);
        theirs->pipe = p;
    }
    if (!mine) {
        shellerror(sh, *a, "cannot make pipe for process substitution");
        return {};
    }

    Vproc *m = vprocwith(sh, output ? theirs : 0, output ? 0 : theirs);
    fileunref(sh, theirs);
    m->src = clone(&m->perm, body);
    m->sh->noreport = 1;  // nothing it runs is reported, as in bash
    Task *task = taskspawn(sh->sched, vprocmain, m, BIGSTACK);
    task->pid  = m->sh->bashpid;
    task->sib  = sh->psubs;
    sh->psubs  = task;
    sh->lastbg = task->pid;  // $!, as bash

    fdsetpsub(sh, fd, mine);
    return concat(a, "/dev/fd/", fmti64(a, fd));
}


// Pipelines

// Is a pipeline stage probably an external program? Then the pipes
// next to it are OS pipes. A wrong guess costs only speed: a shell
// stage can use an OS pipe, and an external given an in-memory pipe
// gets an OS pipe and a pump (runprogram).
static b32 likelyexternal(Shell *sh, Node *n)
{
    if (n->kind != N_SIMPLE) return 0;
    Word *w = n->words;
    while (w && isassignword(w)) w = w->next;
    if (!w || (w->flags & (W_HASDOLLAR|W_QUOTED|W_ASSIGNMENT))) return 0;
    Str name = w->text;
    if (!name.len) return 0;
    for (iz i = 0; i < name.len; i++) {
        u8 c = name[i];
        if (!alnum(c) && c!='_' && c!='-' && c!='.' && c!='/' && c!='+') return 0;
    }
    return !findbuiltin(name) && !findfunc(sh, name);
}

// May a stage start directly, with no vproc? A likely external with
// no redirections, whose words run no shell code when expanded, and
// no descriptor that would need a pump.
static b32 directstage(Shell *sh, Arena scratch, Node *n, b32 first)
{
    if (!likelyexternal(sh, n) || n->redirs) return 0;
    for (Word *w = n->words; w; w = w->next) {
        Str t = w->text;
        if (find(t, "$(")>=0 || find(t, "`")>=0 || find(t, "<(")>=0 || find(t, ">(")>=0) {
            return 0;
        }
    }
    Fdtab *t = sh->fds;
    for (i32 fd = first ? 0 : 2; fd < t->top; fd++) {
        File *f = t->fd[fd];
        if (fd!=1 && f && f->kind!=F_OS && f->kind!=F_CAPTURE) return 0;
    }
    Word *w = n->words;
    while (isassignword(w)) w = w->next;
    return haspath(sh, w->text) || commandpath(sh, &scratch, w->text, 0).data;
}

// Start an external stage: its words expand in a synchronous subshell
// with the stage's descriptors, then the program starts and the
// subshell is rolled back. Returns its process, or BADHANDLE with
// *status set if it did not start, or with *retry set if it turned out
// to be shell code after all.
static Handle spawndirect(Shell *sh, Node *stage, File *in, File *out,
                          Arena scratch, i32 *status, b32 *retry)
{
    Undo *mark = sh->undo;
    if (in) {
        in->refs++;
        fdset(sh, 0, in, 0, &scratch);
    }
    out->refs++;
    fdset(sh, 1, out, 0, &scratch);
    Spawnto sp = {BADHANDLE, 0, 0};
    SubJob  j  = {};
    j.kind = SUB_NODE;
    j.node = stage;
    i32   depth = ++sh->capdepth;
    Arena child = *scratchat(sh, depth);
    sh->spawnto = &sp;
    sh->simplestages++;
    *status = subshell(sh, &j, child, 0);
    sh->simplestages--;
    sh->spawnto  = 0;
    sh->capdepth = depth - 1;
    unwind(sh, mark);
    *retry = sp.retry;
    sh->childpid = sp.pid;  // for a report
    return sp.proc;
}

static void runpipeline(Shell *sh, Node *n, Arena scratch)
{
    // Flatten the right-nested chain, left to right
    Node *stages[256];
    i32   nstages = 0;
    Node *cur = n;
    while (cur->kind==N_CONNECT && cur->connector=='|' && nstages<255 &&
           (cur==n || (!cur->flags && !cur->redirs))) {
        stages[nstages++] = cur->a;
        cur = cur->b;
    }
    stages[nstages++] = cur;

    Sched *s      = sh->sched;
    i32    before = sh->status;  // what $? is in every stage
    File  *in[256]  = {};        // each stage's input, after the first
    File  *out[256] = {};        // each stage's output, before the last
    b32    ext[256];
    for (i32 k = 0; k < nstages; k++) {
        ext[k] = likelyexternal(sh, stages[k]);
    }
    for (i32 k = 0; k+1 < nstages; k++) {
        if (ext[k] || ext[k+1]) {
            OsPipe p = os_pipe(sh->os);
            if (p.err) {
                shellerror(sh, scratch, concat(&scratch, "pipe error: ", errtext(p.err)));
                for (i32 j = 0; j < k; j++) {
                    fileunref(sh, in[j+1]);
                    fileunref(sh, out[j]);
                }
                sh->status = 1;
                return;
            }
            in[k+1] = newfile(sh, F_OS, OF_READ);
            in[k+1]->h = p.r;
            out[k] = newfile(sh, F_OS, OF_WRITE);
            out[k]->h = p.w;
        } else {
            Pipe *p = newpipe(s);
            in[k+1] = newfile(sh, F_PIPE, OF_READ);
            in[k+1]->pipe = p;
            out[k] = newfile(sh, F_PIPE, OF_WRITE);
            out[k]->pipe = p;
        }
    }

    // Every stage but the last: externals started directly, the rest
    // on fiber vprocs, each getting going at once, as forked children
    Task  *tasks[256] = {};
    Handle procs[256];
    i32    statuses[256];
    i64    pids[256] = {};
    for (i32 k = 0; k+1 < nstages; k++) {
        sh->status = before;
        procs[k] = BADHANDLE;
        b32 retry = 1;
        if (ext[k] && directstage(sh, scratch, stages[k], !k)) {
            procs[k] = spawndirect(sh, stages[k], in[k], out[k], scratch,
                                   statuses+k, &retry);
            pids[k]  = sh->childpid;
            sh->status = before;
        }
        if (retry) {
            b32 newloops = stages[k]->kind != N_SIMPLE;  // as bash: break | x is silent
            tasks[k] = spawnvproc(sh, stages[k], in[k], out[k], newloops);
            pids[k]  = ((Vproc *)tasks[k]->arg)->sh->bashpid;
            yield(s);
        }
    }
    for (i32 k = 0; k+1 < nstages; k++) {
        fileunref(sh, in[k]);
        fileunref(sh, out[k]);
    }

    // The last stage here, reading the previous stage's output
    i32   last  = nstages - 1;
    Node *stage = stages[last];
    Undo *mark  = sh->undo;
    sh->status = before;
    fdset(sh, 0, in[last], 0, &scratch);
    i32   depth = ++sh->capdepth;
    Arena child = *scratchat(sh, depth);
    i32   status;
    b32   escaped = 0;
    i32   why     = 0;
    if (sh->shopt[S_LASTPIPE] && !sh->opt[O_MONITOR]) {
        // In this shell: stages still running must be joined even if
        // it exits
        Escape e = {};
        e.up        = sh->esc;
        e.undo      = sh->undo;
        e.frame     = sh->frame;
        e.loopdepth = sh->loopdepth;
        sh->esc = &e;
        if (!__builtin_setjmp(e.jmp)) {
            execute(sh, stage, child);
        } else {
            escaped = 1;
            why     = e.why;
        }
        sh->esc = e.up;
        status = sh->status;
    } else {
        SubJob j = {};
        j.kind = SUB_NODE;
        j.node = stage;
        j.newloops = stage->kind != N_SIMPLE;
        Node group;
        if (stage->kind == N_SUBSHELL) {
            group = *stage;
            group.kind = N_GROUP;
            j.node = &group;
        }
        sh->simplestages += stage->kind == N_SIMPLE;
        // A program as the last stage is reported with the pipeline
        sh->tailnode  = stage->kind==N_SIMPLE ? stage
                      : stage->kind==N_SUBSHELL ? tailcommand(stage->a) : 0;
        sh->taillevel = sh->subshell + 2;
        sh->childsig  = 0;
        status = subshell(sh, &j, child, 0);
        sh->tailnode  = 0;
        sh->taillevel = 0;
        sh->simplestages -= stage->kind == N_SIMPLE;
    }
    sh->capdepth = depth - 1;
    unwind(sh, mark);
    i32 lastsig = escaped ? 0 : sh->childsig;
    pids[last]  = sh->childpid;
    sh->childsig = 0;

    i32 failed = status;
    statuses[last] = status | (lastsig ? WSIGNALED : 0);
    for (i32 k = last-1; k >= 0; k--) {
        i32 st = statuses[k];
        if (tasks[k]) {
            st = taskjoin(s, tasks[k]);
        } else if (procs[k] != BADHANDLE) {
            st = taskwaitproc(s, procs[k]);
        }
        statuses[k] = st;
        if (!failed) failed = st & 255;
    }
    if (lastsig) reportpipeline(sh, scratch, stages, nstages, statuses, pids, n);
    for (i32 k = 0; k < nstages; k++) statuses[k] &= 255;
    sh->status = status;
    setpstat(sh, statuses, nstages);
    if (escaped) {
        escape(sh, why);
    }
    // With pipefail, the rightmost nonzero status
    if (sh->opt[O_PIPEFAIL] && !status) {
        sh->status = failed;
    }
    checkerrexit(sh);
    checksigs(sh, scratch);
}
