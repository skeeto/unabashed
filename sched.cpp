// Scheduler: tasks, in-memory pipes, and blocking I/O
//
// One thread runs every task of a shell process, cooperatively. A task
// is a fiber: a vproc running shell code (proc.cpp), or a pump copying
// bytes between an in-memory Pipe and an OS handle. A task blocks on a
// Pipe, on an OS operation (Io), or on another task's completion; when
// nothing is runnable the scheduler waits on the platform (os_wait).
// Shell code passes safe points (simple commands, loop iterations,
// function calls) where it may yield.
//
// With a single task, I/O is a direct blocking call, so plain scripts
// never touch the run queue. Fibers are pooled: a finished task's fiber
// waits on a free list until a new task reuses it. See notes/m3a.md.
//
// This is free and unencumbered software released into the public domain.

struct Sched;

enum : i32 { TS_READY = 1, TS_RUNNING, TS_BLOCKED, TS_DONE };
enum : i32 { W_NONE, W_PIPE, W_IO, W_JOIN };

struct Task {
    Task   *next;      // run queue, a wait list, or a free list
    Task   *all;       // every task of the scheduler
    Task  **waitlist;  // the wait list it is on, if blocked on a Pipe
    Sched  *s;
    Fiber  *fiber;
    iz      stack;     // usable stack bytes
    i32     state;     // T_*
    i32     wait;      // W_*: why it is blocked
    i32     status;    // the task's result, once done
    b32     failed;    // woken by deadlock recovery
    b32     cancel;    // asked to stop at its next wait (pumps)
    b32     intr;      // its wait was failed by a signal (taskinterrupt)
    b32     introk;    // in a read or wait builtin: any signal with an
                       // action interrupts its wait
    b32     timedout;  // its wait was failed by its deadline
    b32     waitany;   // woken when any task finishes (wait -n)
    i64     deadline;  // os_now time when its pipe or OS wait fails, or 0
    Task   *joiner;    // a task waiting for this one to finish
    Task   *sib;       // its creator's list of procsubs to join later
    i64     pid;       // a procsub's BASHPID, for $! and wait
    u64     tag;       // identity: its creator's tag and spawn count
    u64     nspawn;    // tasks it has started
    void  (*fn)(Task *);
    void   *arg;
    Io      io;        // its pending OS operation
};

// An in-memory pipe between tasks: a ring buffer with counts of open
// ends. Reads block while empty and writers remain; writes block while
// full and readers remain, and fail with E_PIPE once none do.
struct Pipe {
    Pipe  *next;       // free list
    u8    *buf;
    iz     head;       // first unread byte
    iz     len;        // bytes buffered
    i32    readers;
    i32    writers;
    Task  *rwait;      // tasks waiting to read
    Task  *wwait;      // tasks waiting to write
    Handle osr;        // its OS form once a program reads it (pipeosread)
    Task  *pump;       // feeding osr until the last reader closes
    b32    pumping;    // the pump still holds a reader reference
    u64    rtag;       // the tag of the last task that read it
    b32    read;       // rtag is set
};

enum : iz {
    PIPECAP    = (iz)1<<16,  // as a Linux pipe
    BIGSTACK   = (iz)8<<20,  // vprocs: shell code recurses
    SMALLSTACK = (iz)1<<18,  // pumps
};

struct Vproc;
struct Pump;
struct Shell;

// A virtual process that signals can be sent to: the shell, each
// synchronous subshell level, each fiber vproc (see trap.cpp).
struct Sigproc {
    Sigproc *next;     // the Sched's list
    i64      pid;      // its BASHPID
    u64      pending;  // bit n-1 for signal n
    Task    *task;     // the task running it, or null if not started
    Shell   *sh;       // the Shell that runs it
    u64      waitsig;  // terminal signals that came while it waited
                       // for a foreground child (see waitdone)
    b32      fgwait;   // waiting for a foreground program or stages
};

struct Sched {
    Os     *os;
    Arena   a;           // tasks, fibers and pipes; pooled, never reset
    Task    main;        // the thread's own fiber
    Task   *cur;
    Task   *head;        // run queue
    Task   *tail;
    Task   *all;
    Task   *freebig;     // finished tasks whose fibers can be reused
    Task   *freesmall;
    Pipe   *freepipes;
    Pump   *freepumps;
    Vproc  *freevprocs;  // pooled vproc memory (proc.cpp)
    iz      reserve;     // address space for each arena of a vproc
    iz      ntasks;      // live tasks, including the main task
    iz      npending;    // tasks blocked on an Io
    i32     budget;      // safe points until the next yield check
    Sigproc *sigprocs;   // every live virtual process
    Sigproc *top;        // the outermost shell's, for the process's signals
    Task   *orphans;     // jobs whose level ended, to join (jobs.cpp)
    iz      nbg;         // job vprocs not yet joined, orphans included
    u64     chaos;       // random state for chaos mode, 0 if off
    Handle  err;         // where to report a deadlock
};

static Sched *newsched(Os *os, Arena *perm, iz reserve, u64 chaos, Handle err)
{
    Sched *s = alloc<Sched>(perm);
    // 32-bit address space holds only a few full-size reservations
    reserve = sizeof(void *)==4 ? reserve/16 : reserve;
    s->os      = os;
    s->a       = newarena(os, reserve);
    s->reserve = reserve;
    s->chaos   = chaos;
    s->err     = err;
    s->budget  = 1024;
    s->main.s     = s;
    s->main.fiber = os_self(os);
    s->main.state = TS_RUNNING;
    s->main.all   = 0;
    s->all     = &s->main;
    s->cur     = &s->main;
    s->ntasks  = 1;
    return s;
}

static u32 chaosrand(Sched *s)
{
    // xorshift64*
    u64 x = s->chaos;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    s->chaos = x;
    return (u32)((x * 0x2545f4914f6cdd1dull) >> 32);
}

static void enqueue(Sched *s, Task *t)
{
    t->next = 0;
    if (s->tail) {
        s->tail->next = t;
    } else {
        s->head = t;
    }
    s->tail = t;
}

static Task *dequeue(Sched *s)
{
    Task **p = &s->head;
    if (!*p) return 0;
    if (s->chaos) {
        // Chaos: any runnable task may go next
        iz n = 0;
        for (Task *t = s->head; t; t = t->next) n++;
        for (iz i = chaosrand(s) % (u32)n; i; i--) p = &(*p)->next;
    }
    Task *t = *p;
    *p = t->next;
    if (s->tail == t) {
        s->tail = 0;
        for (Task *q = s->head; q; q = q->next) s->tail = q;
    }
    t->next = 0;
    return t;
}

static void wake(Sched *s, Task *t)
{
    if (t->state != TS_BLOCKED && t->state) return;
    t->state = TS_READY;
    t->wait  = W_NONE;
    enqueue(s, t);
}

static void deadlock(Sched *s);
static b32  ossignals(Sched *s);
static void taskinterrupt(Sched *s, Task *t, b32 all);
static b32  taskfail(Sched *s, Task *t, i32 err);

// The earliest deadline of a blocked task, or 0 if none.
static i64 nextdeadline(Sched *s)
{
    i64 when = 0;
    for (Task *t = s->all; t; t = t->all) {
        if (t->state==TS_BLOCKED && t->deadline && (!when || t->deadline<when)) {
            when = t->deadline;
        }
    }
    return when;
}

// Fail the waits of the blocked tasks whose deadlines have passed.
static void expiretimers(Sched *s)
{
    i64 now = os_now(s->os);
    for (Task *t = s->all; t; t = t->all) {
        if (t->state!=TS_BLOCKED || !t->deadline || t->deadline>now) continue;
        if (taskfail(s, t, E_AGAIN)) {
            t->timedout = 1;
        } else {
            t->deadline = 0;  // the platform owns its operation: wait it out
        }
    }
}

// Run other tasks until the current one is runnable again. The caller
// has already queued it (yield), blocked it, or finished it.
static void schedule(Sched *s)
{
    Task *self = s->cur;
    for (;;) {
        Task *t = dequeue(s);
        if (t) {
            t->state = TS_RUNNING;
            if (t != self) {
                s->cur = t;
                os_tasktag(s->os, t->tag, s->ntasks);
                os_switch(s->os, t->fiber);
                // resumed by whichever task switched back here
            }
            return;
        }
        i64 deadline = nextdeadline(s);
        if (s->npending || deadline) {
            Io *io = os_wait(s->os, deadline ? deadline : WAIT_FOREVER);
            if (io) {
                s->npending--;
                wake(s, (Task *)io->ctx);
                continue;
            }
            if (ossignals(s)) continue;  // interrupting waits it fails
            if (deadline) {
                expiretimers(s);
                continue;
            }
        }
        deadlock(s);
    }
}

// Every task is blocked and no operation can complete: a bug, or the
// virtual OS saying a script would hang. Report it, then fail the
// blocked operations so that the shell carries on.
static void deadlock(Sched *s)
{
    Str msg = "unabashed: deadlock: every task is blocked\n";
    os_write(s->os, s->err, msg.data, msg.len);
    b32 woke = 0;
    for (Task *t = s->all; t; t = t->all) {
        if (t->state != TS_BLOCKED) continue;
        switch (t->wait) {
        case W_PIPE:
            for (Task **p = t->waitlist; *p; p = &(*p)->next) {
                if (*p == t) {
                    *p = t->next;
                    break;
                }
            }
            t->waitlist = 0;
            break;
        case W_IO:
            if (!os_cancel(s->os, &t->io)) continue;
            s->npending--;
            t->io.err  = E_DEADLK;
            t->io.done = 1;
            break;
        case W_JOIN:
            continue;  // its target is blocked too, and will be woken
        }
        t->failed = 1;
        wake(s, t);
        woke = 1;
    }
    if (!woke) {
        os_oom();  // nothing left to try
    }
}

// Fail the wait of a blocked task, as a signal interrupts a system
// call: its pipe or OS wait returns an error with intr set. With all
// set, for a signal that will end the task, any pipe or OS wait; else
// only the waits of a read or wait builtin (introk).
static void taskinterrupt(Sched *s, Task *t, b32 all)
{
    if (t->state != TS_BLOCKED || (!all && !t->introk)) return;
    if (taskfail(s, t, E_INTR)) t->intr = 1;
}

// Fail the wait of a blocked task, waking it: its pipe wait fails, its
// OS operation is withdrawn with err, a join returns early. False if
// the operation cannot be withdrawn.
static b32 taskfail(Sched *s, Task *t, i32 err)
{
    switch (t->wait) {
    case W_PIPE:
        for (Task **p = t->waitlist; *p; p = &(*p)->next) {
            if (*p == t) {
                *p = t->next;
                break;
            }
        }
        t->waitlist = 0;
        t->failed   = 1;
        break;
    case W_IO:
        if (!os_cancel(s->os, &t->io)) return 0;
        s->npending--;
        t->io.err  = err;
        t->io.done = 1;
        break;
    case W_JOIN:
        break;
    default:
        return 0;
    }
    wake(s, t);
    return 1;
}

static void block(Sched *s, i32 why)
{
    s->cur->state = TS_BLOCKED;
    s->cur->wait  = why;
    schedule(s);
}

// Let other runnable tasks go first.
static void yield(Sched *s)
{
    if (!s->head) return;
    s->cur->state = TS_READY;
    enqueue(s, s->cur);
    schedule(s);
}

[[gnu::noinline]]
static void safepointslow(Sched *s)
{
    s->budget = s->chaos ? (i32)(chaosrand(s) % 8) : 1024;
    if (nextdeadline(s)) expiretimers(s);  // even while others keep busy
    while (s->npending) {
        Io *io = os_wait(s->os, WAIT_POLL);
        if (!io) break;
        s->npending--;
        wake(s, (Task *)io->ctx);
    }
    if (!s->chaos || chaosrand(s)%2) {
        yield(s);
    }
}

// A point where shell code may give way to other tasks: cheap unless
// its budget ran out.
static void safepoint(Sched *s)
{
    if (--s->budget < 0) {
        safepointslow(s);
    }
}


// Tasks

static void taskentry(Os *, void *arg)
{
    Task  *t = (Task *)arg;
    Sched *s = t->s;
    for (;;) {
        t->fn(t);
        t->state = TS_DONE;
        s->ntasks--;
        if (t->joiner) {
            wake(s, t->joiner);
            t->joiner = 0;
        }
        for (Task *w = s->all; w; w = w->all) {
            if (w->waitany && w->state==TS_BLOCKED && w->wait==W_JOIN) wake(s, w);
        }
        schedule(s);  // returns when the task is reused
    }
}

// Start a task running fn(t) with t->arg. It runs once the current
// task blocks or yields. Its creator must join it.
static Task *taskspawn(Sched *s, void (*fn)(Task *), void *arg, iz stack)
{
    Task **pool = stack>SMALLSTACK ? &s->freebig : &s->freesmall;
    Task  *t    = *pool;
    if (t) {
        *pool = t->next;
    } else {
        t = alloc<Task>(&s->a);
        t->s     = s;
        t->stack = stack;
        t->fiber = os_fiber(s->os, &s->a, stack, taskentry, t);
        t->all   = s->all;
        s->all   = t;
    }
    t->fn     = fn;
    t->arg    = arg;
    t->io     = {};
    t->status = 0;
    t->failed = 0;
    t->cancel = 0;
    t->intr   = 0;
    t->introk = 0;
    t->timedout = 0;
    t->waitany  = 0;
    t->deadline = 0;
    t->joiner = 0;
    t->sib    = 0;
    t->pid    = 0;
    t->state  = 0;
    // Its identity follows from its creator's, never from the schedule
    u64 h = (s->cur->tag ^ ++s->cur->nspawn) * 0x9e3779b97f4a7c15u;
    t->tag    = h ^ h>>31;
    t->nspawn = 0;
    s->ntasks++;
    os_tasktag(s->os, s->cur->tag, s->ntasks);
    wake(s, t);
    return t;
}

// Wait for a task to finish, return its status, and recycle it.
static i32 taskjoin(Sched *s, Task *t)
{
    while (t->state != TS_DONE) {
        t->joiner = s->cur;
        block(s, W_JOIN);
    }
    i32 status = t->status;
    Task **pool = t->stack>SMALLSTACK ? &s->freebig : &s->freesmall;
    t->state = 0;
    t->next  = *pool;
    *pool    = t;
    return status;
}

// As taskjoin, but in the wait builtin: false, with the task left to
// join later, if a signal interrupts the wait.
static b32 taskjoinintr(Sched *s, Task *t, i32 *status)
{
    Task *self = s->cur;
    self->introk = 1;
    while (t->state != TS_DONE) {
        t->joiner = self;
        block(s, W_JOIN);
        if (self->intr) {
            t->joiner    = 0;
            self->introk = 0;
            return 0;
        }
    }
    self->introk = 0;
    *status = taskjoin(s, t);
    return 1;
}

// Wait until some task finishes, as wait -n does: false if a signal
// interrupted the wait (as in taskjoinintr).
static b32 taskwaitany(Sched *s)
{
    Task *self = s->cur;
    self->introk  = 1;
    self->waitany = 1;
    block(s, W_JOIN);
    self->waitany = 0;
    self->introk  = 0;
    return !self->intr;
}

// Ask a task to stop at its next wait on a Pipe, waking it if blocked
// on one now.
static void taskcancel(Sched *s, Task *t)
{
    t->cancel = 1;
    if (t->state==TS_BLOCKED && t->wait==W_PIPE) {
        for (Task **p = t->waitlist; *p; p = &(*p)->next) {
            if (*p == t) {
                *p = t->next;
                break;
            }
        }
        t->waitlist = 0;
        wake(s, t);
    }
}

// Release the fibers of finished tasks, when the shell exits.
static void freesched(Sched *s)
{
    for (Task *t = s->freebig; t; t = t->next) os_fiberfree(s->os, t->fiber);
    for (Task *t = s->freesmall; t; t = t->next) os_fiberfree(s->os, t->fiber);
    s->freebig = s->freesmall = 0;
}


// Blocking I/O on OS handles: direct calls while alone, else submitted
// so that other tasks run meanwhile.

static void taskio(Sched *s, Io *io)
{
    io->ctx = s->cur;
    os_submit(s->os, io);
    if (!io->done) {
        s->npending++;
        block(s, W_IO);
    }
}

// Read up to len bytes: 0 at end of file, -1 on error.
static iz taskread(Sched *s, Handle h, u8 *buf, iz len)
{
    if (s->ntasks==1 && !s->cur->deadline) {
        return os_read(s->os, h, buf, len);
    }
    Io *io = &s->cur->io;
    *io = {};
    io->op  = IO_READ;
    io->h   = h;
    io->buf = buf;
    io->len = len;
    taskio(s, io);
    s->cur->failed = 0;
    return io->err ? -1 : io->n;
}

// Write all bytes. Returns 0 or an E_* error, E_PIPE without readers.
static i32 taskwrite(Sched *s, Handle h, u8 *buf, iz len)
{
    if (s->ntasks == 1) {
        if (os_write(s->os, h, buf, len)) return 0;
        Stat st = os_fstat(s->os, h);
        return st.type==FT_PIPE || st.type==FT_FIFO ? E_PIPE : E_IO;
    }
    while (len > 0) {
        Io *io = &s->cur->io;
        *io = {};
        io->op  = IO_WRITE;
        io->h   = h;
        io->buf = buf;
        io->len = len;
        taskio(s, io);
        s->cur->failed = 0;
        if (io->err) return io->err;
        buf += io->n;
        len -= io->n;
    }
    return 0;
}

// Could a read of h complete now, with data or at end of file? A
// zero-length read submitted and withdrawn if it does not complete at
// once (read -t 0).
static b32 taskreadable(Sched *s, Handle h)
{
    Io io = {};
    io.op  = IO_READ;
    io.h   = h;
    io.buf = (u8 *)&io;
    io.len = 0;
    io.ctx = s->cur;
    os_submit(s->os, &io);
    while (!io.done) {
        // It may be ready and only deferred (chaos): poll once more
        Io *r = os_wait(s->os, WAIT_POLL);
        if (!r) break;
        if (r != &io) {
            s->npending--;
            wake(s, (Task *)r->ctx);
        }
    }
    if (io.done) return !io.err;
    if (os_cancel(s->os, &io)) return 0;
    // The platform owns it now: wait for its verdict
    while (!io.done) {
        Io *r = os_wait(s->os, WAIT_FOREVER);
        if (!r) return 0;
        if (r != &io) {
            s->npending--;
            wake(s, (Task *)r->ctx);
        }
    }
    return !io.err;
}

static i32 taskwaitproc(Sched *s, Handle proc)
{
    if (s->ntasks == 1) {
        return os_waitproc(s->os, proc);
    }
    Io *io = &s->cur->io;
    *io = {};
    io->op = IO_PROC;
    io->h  = proc;
    taskio(s, io);
    s->cur->failed = 0;
    return io->err ? 127 : (i32)io->n;
}


// In-memory pipes

static Pipe *newpipe(Sched *s)
{
    Pipe *p = s->freepipes;
    if (p) {
        s->freepipes = p->next;
        u8 *buf = p->buf;
        *p = {};
        p->buf = buf;
    } else {
        p = alloc<Pipe>(&s->a);
        p->buf = allocbytes(&s->a, PIPECAP);
    }
    p->readers = 1;
    p->writers = 1;
    p->osr     = BADHANDLE;
    return p;
}

static void wakeall(Sched *s, Task **list)
{
    while (*list) {
        Task *t = *list;
        *list = t->next;
        t->waitlist = 0;
        wake(s, t);
    }
}

// Block on a pipe's wait list. False if woken to fail or cancel.
static b32 waiton(Sched *s, Task **list)
{
    Task *t = s->cur;
    if (t->cancel) return 0;
    t->next     = *list;
    *list       = t;
    t->waitlist = list;
    block(s, W_PIPE);
    b32 ok = !t->failed && !t->cancel;
    t->failed = 0;
    return ok;
}

// Read up to len bytes from the ring: 0 at end of file, -1 if the
// wait failed.
static iz pipereadraw(Sched *s, Pipe *p, u8 *buf, iz len)
{
    while (!p->len && p->writers) {
        if (!waiton(s, &p->rwait)) return -1;
    }
    iz n = len<p->len ? len : p->len;
    if (s->chaos) {
        if (n > 1) n = 1 + chaosrand(s)%(u32)n;
        // Readers sharing a pipe may split its bytes any way
        if (n && p->read && p->rtag!=s->cur->tag) os_racy(s->os);
        p->rtag = s->cur->tag;
        p->read = 1;
    }
    for (iz i = 0; i < n; i++) {
        buf[i] = p->buf[(p->head + i) & (PIPECAP - 1)];
    }
    p->head = (p->head + n) & (PIPECAP - 1);
    p->len -= n;
    wakeall(s, &p->wwait);
    return n;
}

// Read up to len bytes: 0 at end of file, -1 on failure. Once the
// pipe has an OS form, every reader reads that.
static iz piperead(Sched *s, Pipe *p, u8 *buf, iz len)
{
    for (;;) {
        if (p->osr != BADHANDLE) {
            return taskread(s, p->osr, buf, len);
        }
        if (p->len || !p->writers) {
            return pipereadraw(s, p, buf, len);
        }
        // A program may take it over meanwhile: then read its OS form,
        // or bytes would be read around the pump's
        if (!waiton(s, &p->rwait)) return -1;
    }
}

// Write all bytes. Returns 0 or an E_* error: E_PIPE without readers.
static i32 pipewrite(Sched *s, Pipe *p, u8 *buf, iz len)
{
    while (len > 0) {
        if (!p->readers) {
            os_racy(s->os);  // whether the reader was gone yet
            return E_PIPE;
        }
        iz space = PIPECAP - p->len;
        if (!space) {
            if (!waiton(s, &p->wwait)) return E_DEADLK;
            continue;
        }
        iz n = len<space ? len : space;
        if (s->chaos && n>1) n = 1 + chaosrand(s)%(u32)n;
        iz tail = (p->head + p->len) & (PIPECAP - 1);
        for (iz i = 0; i < n; i++) {
            p->buf[(tail + i) & (PIPECAP - 1)] = buf[i];
        }
        p->len += n;
        buf += n;
        len -= n;
        wakeall(s, &p->rwait);
        if (s->chaos && chaosrand(s)%4==0) yield(s);
    }
    return 0;
}

static void endpump(Sched *, Task *);

// Close one end of a pipe, waking whoever waits on the other side.
static void pipeclose(Sched *s, Pipe *p, b32 writer)
{
    if (writer) {
        p->writers--;
        wakeall(s, &p->rwait);
    } else {
        p->readers--;
        wakeall(s, &p->wwait);
        if (p->pump && p->readers==p->pumping) {
            // The last reader of its OS form: stop the pump, which then
            // drops its own reference
            b32   pumping = p->pumping;
            Task *t = p->pump;
            p->pump = 0;
            os_close(s->os, p->osr);
            p->osr = BADHANDLE;
            endpump(s, t);
            if (pumping) return;
        }
    }
    if (!p->readers && !p->writers) {
        p->next = s->freepipes;
        s->freepipes = p;
    }
}


// Pumps: tasks that copy between an in-memory Pipe or text and an OS
// handle, for externals given a shell object (materialization).

enum : i32 { PUMP_TOHANDLE, PUMP_FROMHANDLE, PUMP_TEXT, PUMP_TOBUF };

struct Pump {
    Pump  *next;     // free list
    i32    kind;
    Handle h;        // owned: closed when the pump ends
    Pipe  *pipe;     // owned end: the read end for PUMP_TOHANDLE
    Str    text;     // PUMP_TEXT: bytes to write
    Buf   *buf;      // PUMP_TOBUF: where to append what h delivers
};

// A Pump that outlives the arena of whoever starts it, pooled.
static Pump *newpump(Sched *s)
{
    Pump *p = s->freepumps;
    if (p) {
        s->freepumps = p->next;
        *p = {};
    } else {
        p = alloc<Pump>(&s->a);
    }
    return p;
}

static void pumpmain(Task *t)
{
    Pump  *p = (Pump *)t->arg;
    Sched *s = t->s;
    u8     buf[1<<12];
    switch (p->kind) {
    case PUMP_TOHANDLE:
        for (;;) {
            iz n = pipereadraw(s, p->pipe, buf, countof(buf));
            if (n<=0 || taskwrite(s, p->h, buf, n)) break;
        }
        os_close(s->os, p->h);
        p->h = BADHANDLE;  // EOF for the OS form at once
        p->pipe->pumping = 0;
        pipeclose(s, p->pipe, 0);
        break;
    case PUMP_FROMHANDLE:
        for (;;) {
            iz n = taskread(s, p->h, buf, countof(buf));
            if (n<=0 || pipewrite(s, p->pipe, buf, n)) break;
        }
        pipeclose(s, p->pipe, 1);
        break;
    case PUMP_TEXT:
        taskwrite(s, p->h, p->text.data, p->text.len);
        break;
    case PUMP_TOBUF:
        for (;;) {
            iz n = taskread(s, p->h, buf, countof(buf));
            if (n <= 0) break;
            print(p->buf, {buf, n});
        }
        break;
    }
    if (p->h != BADHANDLE) os_close(s->os, p->h);
}

// Start a pump; the Pump must stay in place until it is joined.
static Task *spawnpump(Sched *s, Pump *p)
{
    if (p->pipe) {
        if (p->kind == PUMP_TOHANDLE) {
            p->pipe->readers++;
        } else {
            p->pipe->writers++;
        }
    }
    return taskspawn(s, pumpmain, p, SMALLSTACK);
}

// Give a pipe an OS form for a program to read: an OS pipe fed by a
// pump, which every reader reads from then on, so that what a program
// leaves unread stays for the next reader, as with a real pipe shared
// across processes. Returns its read handle, or BADHANDLE.
static Handle pipeosread(Sched *s, Pipe *p)
{
    if (p->osr != BADHANDLE) return p->osr;
    OsPipe op = os_pipe(s->os);
    if (op.err) return BADHANDLE;
    Pump *pp = newpump(s);
    pp->kind = PUMP_TOHANDLE;
    pp->h    = op.w;
    pp->pipe = p;
    p->osr     = op.r;
    p->pumping = 1;
    p->pump    = spawnpump(s, pp);
    return p->osr;
}

// Join a pump from newpump and return its Pump to the pool.
static i32 joinpump(Sched *s, Task *t)
{
    Pump *p = (Pump *)t->arg;
    i32   r = taskjoin(s, t);
    p->next = s->freepumps;
    s->freepumps = p;
    return r;
}

// Stop a pump from newpump, once its handle's other end is closed,
// and return its Pump to the pool.
static void endpump(Sched *s, Task *t)
{
    taskcancel(s, t);
    joinpump(s, t);
}
