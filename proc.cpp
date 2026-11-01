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
    Node  *node;          // what it runs
};

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
    c->flow     = FLOW_NONE;
    c->exiting  = 0;
    c->subshell++;
    c->bashpid  = ++sh->lastpid;
    c->lastpid  = sh->lastpid;
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
    if (!__builtin_setjmp(top.jmp)) {
        execute(sh, m->node, *scratchat(sh, 0));
    }
    t->status = sh->status;

    unwind(sh, 0);
    Fdtab *fds = sh->fds;
    for (i32 fd = 0; fd < fds->top; fd++) {
        fileunref(sh, fds->fd[fd]);
        fds->fd[fd] = 0;
    }
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
static Task *spawnvproc(Shell *sh, Node *node, File *in, File *out, b32 newloops)
{
    Sched *s = sh->sched;
    Vproc *m = newvproc(s);
    Shell *c = vproccopy(sh, m);
    if (newloops) c->loopdepth = 0;
    c->simplestages += node->kind == N_SIMPLE;
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
    m->node = copynode(&m->perm, node);
    if (m->node && m->node->kind==N_SUBSHELL) {
        // ( list ) runs in the vproc's own subshell, not a nested one
        m->node->kind = N_GROUP;
    }
    return taskspawn(s, vprocmain, m, BIGSTACK);
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

static void runpipeline(Shell *sh, Node *n, Arena scratch)
{
    // Flatten the left-nested chain, left to right
    Node *rev[256];
    i32   nstages = 0;
    Node *cur = n;
    while (cur->kind==N_CONNECT && cur->connector=='|' && nstages<255) {
        rev[nstages++] = cur->b;
        cur = cur->a;
    }
    rev[nstages++] = cur;
    Node *stages[256];
    for (i32 k = 0; k < nstages; k++) stages[k] = rev[nstages-1-k];

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

    // Every stage but the last on a fiber vproc
    Task *tasks[256];
    for (i32 k = 0; k+1 < nstages; k++) {
        sh->status = before;
        b32 newloops = stages[k]->kind != N_SIMPLE;  // as bash: break | x is silent
        tasks[k] = spawnvproc(sh, stages[k], in[k], out[k], newloops);
    }
    for (i32 k = 0; k+1 < nstages; k++) {
        fileunref(sh, in[k]);
        fileunref(sh, out[k]);
    }
    // As forked children do, the earlier stages get going first
    yield(s);

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
        status = subshell(sh, &j, child, 0);
        sh->simplestages -= stage->kind == N_SIMPLE;
    }
    sh->capdepth = depth - 1;
    unwind(sh, mark);

    i32 failed = status;
    for (i32 k = last-1; k >= 0; k--) {
        i32 st = taskjoin(s, tasks[k]);
        if (!failed) failed = st;
    }
    sh->status = status;
    if (escaped) {
        escape(sh, why);
    }
    // With pipefail, the rightmost nonzero status
    if (sh->opt[O_PIPEFAIL] && !status) {
        sh->status = failed;
    }
    checkerrexit(sh);
}
