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
    b32     anysig;    // in the editor: any signal at all does (a resize),
                       // so its reads never block outside the scheduler
    b32     timedout;  // its wait was failed by its deadline
    b32     waitany;   // woken when any task finishes (wait -n)
    i32     change;    // a waiter's: its program's last stop (WSTOP|sig)
                       // or continue (WCONT) the jobs have not seen
    i64     deadline;  // os_now time when its pipe or OS wait fails, or 0
    Task   *joiner;    // a task waiting for this one to finish
    Task   *sib;       // its creator's list of procsubs to join later
    i64     pid;       // a procsub's BASHPID, for $! and wait
    u64     tag;       // identity: its creator's tag and spawn count
    u64     nspawn;    // tasks it has started
    u64     group;     // tracing: its nearest loose task's tag, or 0
    void  (*fn)(Task *);
    void   *arg;
    Io      io;        // its pending OS operation
};

// Chaos tracing: the writer group that last wrote a pipe or capture,
// whose reader sees the bytes of all writers merged (see tracewrite).
struct Wtrace {
    u64 group;
    b32 set;
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
    Wtrace wtrace;     // chaos: its last writer
};

enum : iz {
    PIPECAP    = (iz)1<<16,  // as a Linux pipe
    BIGSTACK   = (iz)8<<20,  // vprocs: shell code recurses
    SMALLSTACK = (iz)1<<18,  // pumps
};

struct Vproc;
struct Pump;
struct Shell;

// ChaCha8 as the stream behind /dev/random and /dev/urandom: one per
// process, keyed on first use with 256 bits of os_entropy (nothing else
// is drawn from it), then the keystream of blocks 0, 1, 2, ... with a
// zero nonce. Zero is unkeyed: a forked copy resets it to rekey.
struct Chacha {
    u32 key[8];
    u64 block;      // the next block's counter
    u8  out[64];    // the current block's keystream
    i32 used;       // bytes of out already given
    b32 keyed;
};

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
    u64      bydefault;  // pending ones to act on by default, trap or
                         // not (an interactive loop's job died of INT)
    i64      execpid;  // the program in its place, which bash would have
                       // exec'd (its tail command): signals go to it
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
    u64     entropy;     // the process's random base for RANDOM and
                         // SRANDOM (os_entropy once, on first use), or 0
    Chacha  chacha;      // /dev/random and /dev/urandom
    iz      ntasks;      // live tasks, including the main task
    iz      npending;    // tasks blocked on an Io
    i32     budget;      // safe points until the next yield check
    Sigproc *sigprocs;   // every live virtual process
    Sigproc *top;        // the outermost shell's, for the process's signals
    i64      pid;        // this process's real pid
    i64      nvpid;      // virtual pids handed out (newvpid)
    Task   *orphans;     // jobs whose level ended, to join (jobs.cpp)
    iz      nbg;         // job vprocs not yet joined, orphans included
    u64     chaos;       // random state for chaos mode, 0 if off
    Handle  err;         // where to report a deadlock
    Handle  stopproc;    // a grouped pipeline's last program stopped
    i64     stoppid;     // (runprogram for runpipeline, across the
    i32     stopsig;     // stage's subshell)
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
    s->stopproc = BADHANDLE;
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

// Chaos tracing: the running task writes where w last wrote. A loose
// task is not waited for in place by the one that starts it: a procsub,
// a pipeline stage before the last (which runs alongside the last), a
// program started directly as such a stage, or a pump for a procsub.
// Its writes, and those of the tasks and programs it starts, form a
// writer group (Task.group, its tag) whose order is its own; the
// creator's other writes are a different group. When two groups write
// the same pipe or capture, the reader may see their bytes in either
// order, unlike writers in one group, which take turns.
static void tracewrite(Sched *s, Wtrace *w)
{
    if (!s->chaos) return;
    Task *t = s->cur;
    if (w->set && w->group!=t->group) os_racy(s->os);
    w->group = t->group;
    w->set   = 1;
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
                os_tasktag(s->os, t->tag, s->ntasks, t->group);
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

// Wake the tasks waiting for t: its joiner, and any in wait -n.
static void wakejoiners(Sched *s, Task *t)
{
    if (t->joiner) {
        wake(s, t->joiner);
        t->joiner = 0;
    }
    for (Task *w = s->all; w; w = w->all) {
        if (w->waitany && w->state==TS_BLOCKED && w->wait==W_JOIN) wake(s, w);
    }
}

static void taskentry(Os *, void *arg)
{
    Task  *t = (Task *)arg;
    Sched *s = t->s;
    for (;;) {
        t->fn(t);
        t->state = TS_DONE;
        s->ntasks--;
        wakejoiners(s, t);
        schedule(s);  // returns when the task is reused
    }
}

// A new identity for something the running task starts. It follows
// from the task's own, never from the schedule.
static u64 nexttag(Sched *s)
{
    u64 h = (s->cur->tag ^ ++s->cur->nspawn) * 0x9e3779b97f4a7c15u;
    return h ^ h>>31;
}

// Chaos tracing: put the running task in another writer group (see
// tracewrite), returning its old one. What it starts meanwhile is in
// that group too.
static u64 setgroup(Sched *s, u64 group)
{
    u64 old = s->cur->group;
    s->cur->group = group;
    os_tasktag(s->os, s->cur->tag, s->ntasks, group);
    return old;
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
    t->tag    = nexttag(s);
    t->nspawn = 0;
    t->group  = s->cur->group;
    s->ntasks++;
    os_tasktag(s->os, s->cur->tag, s->ntasks, s->cur->group);
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
    // A signal that failed this join meanwhile stays pending for the
    // level, not to fail some later interruptible wait
    s->cur->intr = 0;
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
    if (s->ntasks==1 && !s->cur->deadline && !s->cur->anysig) {
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

// Wait for a named pipe's client (os_namedpipe): 0 or an E_* error.
static i32 taskconnect(Sched *s, Handle h)
{
    Io *io = &s->cur->io;
    *io = {};
    io->op = IO_CONNECT;
    io->h  = h;
    taskio(s, io);
    s->cur->failed = 0;
    return io->err;
}

// Always asynchronously, even as the only task: a signal that comes
// meanwhile must interrupt it (os_wait returns early, os_waitproc not).
static i32 taskwaitproc(Sched *s, Handle proc)
{
    Io *io = &s->cur->io;
    *io = {};
    io->op = IO_PROC;
    io->h  = proc;
    taskio(s, io);
    s->cur->failed = 0;
    return io->err ? 127 : (i32)io->n;
}

// As taskwaitproc, for a program in a process group of its own under
// job control: it also ends when the program stops (WSTOP|sig).
static i32 taskwaitstop(Sched *s, Handle proc)
{
    for (;;) {
        Io *io = &s->cur->io;
        *io = {};
        io->op = IO_PROCSTOP;
        io->h  = proc;
        taskio(s, io);
        s->cur->failed = 0;
        if (io->err) return 127;
        if (!((i32)io->n & WCONT)) return (i32)io->n;
    }
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
        tracewrite(s, &p->wtrace);
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
// handle, for externals given a shell object (materialization), and
// between a named pipe and a descriptor (bridges, exec.cpp).

enum : i32 { NSPARE = 2 };  // a bridge's instances listening ahead
enum : i32 { PUMPBUF = 1<<12, GENBUF = 1<<16 };

// One ChaCha block (DJB's layout: a 64-bit counter, then a 64-bit
// nonce) with the given number of rounds, as little-endian bytes.
static void chachablock(u8 *out, u32 *key, u64 counter, u64 nonce, i32 rounds)
{
    u32 s[16] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
        key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7],
        (u32)counter, (u32)(counter>>32), (u32)nonce, (u32)(nonce>>32),
    };
    u32 x[16];
    for (i32 i = 0; i < 16; i++) x[i] = s[i];
    #define QR(a, b, c, d)         x[a] += x[b]; x[d] ^= x[a]; x[d] = x[d]<<16 | x[d]>>16;         x[c] += x[d]; x[b] ^= x[c]; x[b] = x[b]<<12 | x[b]>>20;         x[a] += x[b]; x[d] ^= x[a]; x[d] = x[d]<< 8 | x[d]>>24;         x[c] += x[d]; x[b] ^= x[c]; x[b] = x[b]<< 7 | x[b]>>25
    for (i32 r = 0; r < rounds; r += 2) {
        QR(0, 4,  8, 12); QR(1, 5,  9, 13); QR(2, 6, 10, 14); QR(3, 7, 11, 15);
        QR(0, 5, 10, 15); QR(1, 6, 11, 12); QR(2, 7,  8, 13); QR(3, 4,  9, 14);
    }
    #undef QR
    for (i32 i = 0; i < 16; i++) {
        u32 w = x[i] + s[i];
        out[4*i+0] = (u8)(w >>  0);
        out[4*i+1] = (u8)(w >>  8);
        out[4*i+2] = (u8)(w >> 16);
        out[4*i+3] = (u8)(w >> 24);
    }
}

// Fill buf from the process's ChaCha8 stream, keying it first if new.
static void chachafill(Sched *s, u8 *buf, iz len)
{
    Chacha *c = &s->chacha;
    if (!c->keyed) {
        for (i32 i = 0; i < 4; i++) {
            u64 e = os_entropy(s->os);
            c->key[2*i+0] = (u32)e;
            c->key[2*i+1] = (u32)(e >> 32);
        }
        c->block = 0;
        c->used  = 64;
        c->keyed = 1;
    }
    for (iz i = 0; i < len;) {
        if (c->used == 64) {
            chachablock(c->out, c->key, c->block++, 0, 8);
            c->used = 0;
        }
        iz n = 64 - c->used;
        n = n<len-i ? n : len-i;
        copybytes(buf+i, c->out+c->used, n);
        __builtin_memset(c->out+c->used, 0, (uz)n);  // given once only
        c->used += (i32)n;
        i += n;
    }
}

enum : i32 { GEN_ZERO, GEN_RANDOM };  // PUMP_GEN: what it writes

enum : i32 {
    PUMP_TOHANDLE, PUMP_FROMHANDLE, PUMP_TEXT, PUMP_TOBUF,
    PUMP_SERVE,  // a named pipe's client reads: peer into h
    PUMP_TAKE,   // its client writes: h into peer, or else into pipe
    PUMP_GEN,    // endless zeros or random bytes into h (/dev/zero ...)
};

struct Pump {
    Pump   *next;    // free list
    i32     kind;
    Handle  h;       // owned: closed when the pump ends
    Pipe   *pipe;    // owned end: the read end for PUMP_TOHANDLE
    Str     text;    // PUMP_TEXT: bytes to write
    Buf    *buf;     // PUMP_TOBUF: where to append what h delivers, or discard
    i64     owner;   // PUMP_TOBUF: BASHPID of the vproc writing it, or 0
    Wtrace *wtrace;  // PUMP_TOBUF: the capture's, for tracewrite
    Handle  peer;    // PUMP_SERVE, PUMP_TAKE: owned, or BADHANDLE
    // PUMP_SERVE, PUMP_TAKE: h is the oldest instance of the named pipe
    // still listening, spare the next ones (owned, or BADHANDLE)
    Handle  spare[NSPARE];
    u8      name[64];   // the pipe's name, len bytes
    iz      namelen;
    i32     how;        // its instances' NP_* flags
    b32     waiting;    // for a client
    b32     connected;  // a client is being served
    b32     ending;     // the program exited: no client is to come
    u8     *data;       // PUMPBUF bytes in transit, kept across reuse
    i32     gen;        // PUMP_GEN: GEN_*
    u8     *gendata;    // PUMP_GEN: GENBUF bytes, once used so, kept
};

// A Pump that outlives the arena of whoever starts it, pooled.
static Pump *newpump(Sched *s)
{
    Pump *p = s->freepumps;
    if (p) {
        s->freepumps = p->next;
        u8 *data = p->data;
        u8 *gen  = p->gendata;
        *p = {};
        p->data    = data;
        p->gendata = gen;
    } else {
        p = alloc<Pump>(&s->a);
        p->data = allocbytes(&s->a, PUMPBUF);
    }
    return p;
}

// A bridge's next client (PUMP_SERVE, PUMP_TAKE), or BADHANDLE when it
// ends first. Before it is served, a new instance listens behind the
// spares: a client opening the name again soon finds one waiting (as
// busybox does, looking first; an MSYS2 program opens it three times).
static Handle nextclient(Sched *s, Pump *p)
{
    // Once ending, only a client that came already
    p->waiting = 1;
    s->cur->deadline = p->ending ? os_now(s->os) : 0;
    i32 err = taskconnect(s, p->h);  // else served, even if cancelled
    s->cur->deadline = 0;
    p->waiting = 0;
    if (err) return BADHANDLE;
    Handle c = p->h;
    p->connected = 1;
    p->h = p->spare[0];
    for (i32 i = 0; i < NSPARE-1; i++) {
        p->spare[i] = p->spare[i+1];
    }
    p->spare[NSPARE-1] = BADHANDLE;
    if (p->h != BADHANDLE) {
        Arena     none = {};
        Str       name = {p->name, p->namelen};
        NamedPipe np   = os_namedpipe(s->os, &none, name, p->how);
        p->spare[NSPARE-1] = np.err ? BADHANDLE : np.h;
    }
    return c;
}

static void pumpmain(Task *t)
{
    Pump  *p = (Pump *)t->arg;
    Sched *s = t->s;
    u8    *buf = p->data;
    switch (p->kind) {
    case PUMP_GEN: {
        // Until the readers go, or the shell exits (joinorphans), in
        // writes as large as the pipe for speed
        if (!p->gendata) p->gendata = allocbytes(&s->a, GENBUF);
        u8 *g = p->gendata;
        if (p->gen == GEN_ZERO) __builtin_memset(g, 0, GENBUF);
        for (;;) {
            if (p->gen == GEN_RANDOM) chachafill(s, g, GENBUF);
            if (t->cancel || taskwrite(s, p->h, g, GENBUF)) break;
        }
        if (p->gen == GEN_RANDOM) __builtin_memset(g, 0, GENBUF);
    } break;
    case PUMP_TOHANDLE:
        for (;;) {
            iz n = pipereadraw(s, p->pipe, buf, PUMPBUF);
            if (n<=0 || taskwrite(s, p->h, buf, n)) break;
        }
        os_close(s->os, p->h);
        p->h = BADHANDLE;  // EOF for the OS form at once
        p->pipe->pumping = 0;
        pipeclose(s, p->pipe, 0);
        break;
    case PUMP_FROMHANDLE:
        for (;;) {
            iz n = taskread(s, p->h, buf, PUMPBUF);
            if (n<=0 || pipewrite(s, p->pipe, buf, n)) break;
        }
        pipeclose(s, p->pipe, 1);
        break;
    case PUMP_TEXT:
        taskwrite(s, p->h, p->text.data, p->text.len);
        break;
    case PUMP_TOBUF:
        for (;;) {
            iz n = taskread(s, p->h, buf, PUMPBUF);
            if (n <= 0) break;
            if (p->buf) {
                tracewrite(s, p->wtrace);
                print(p->buf, {buf, n});
            }
        }
        break;
    case PUMP_SERVE: {
        // Clients in turn, each served what the last left: one leaving
        // before it read (perhaps one that only looked, as a stat
        // through a handle does) takes nothing, the write failing at
        // once on the unbuffered pipe (NP_NOBUF).
        iz  len = 0;  // bytes in buf not yet delivered
        b32 eof = 0;
        while (!t->cancel && !p->ending && p->h!=BADHANDLE) {
            Handle c = nextclient(s, p);
            if (c == BADHANDLE) break;
            for (b32 fresh = 1; !t->cancel; fresh = 0) {
                if (!len && !eof) {
                    len = taskread(s, p->peer, buf, PUMPBUF);
                    eof = len <= 0;
                    len = eof ? 0 : len;
                }
                if (!len || t->cancel) break;
                if (taskwrite(s, c, buf, len)) {
                    len = fresh ? len : 0;  // else partly read: gone
                    break;
                }
                len = 0;
            }
            os_close(s->os, c);
            p->connected = 0;
        }
        break;
    }
    case PUMP_TAKE:
        // Clients in turn until the program exits, and those that came
        // by then: one may only look
        while (!t->cancel && p->h!=BADHANDLE) {
            Handle c   = nextclient(s, p);
            i32    err = c==BADHANDLE;
            for (; !err;) {
                iz n = taskread(s, c, buf, PUMPBUF);
                if (n <= 0) break;
                err = p->pipe ? pipewrite(s, p->pipe, buf, n)
                              : taskwrite(s, p->peer, buf, n);
            }
            if (c != BADHANDLE) os_close(s->os, c);
            p->connected = 0;
            if (err) break;
        }
        if (p->pipe) pipeclose(s, p->pipe, 1);
        break;
    }
    if (p->h != BADHANDLE) os_close(s->os, p->h);
    if (p->kind==PUMP_SERVE || p->kind==PUMP_TAKE) {
        for (i32 i = 0; i < NSPARE; i++) {
            if (p->spare[i] != BADHANDLE) os_close(s->os, p->spare[i]);
        }
        if (p->peer  != BADHANDLE) os_close(s->os, p->peer);
    }
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

// Stop a bridge pump (exec.cpp) at its next wait. Its spares go
// first, so that while it waits for a client only the instance it waits
// on listens (see os_cancel).
static void cancelbridge(Sched *s, Task *t)
{
    Pump *p = (Pump *)t->arg;
    for (i32 i = 0; i < NSPARE; i++) {
        if (p->spare[i] != BADHANDLE) os_close(s->os, p->spare[i]);
        p->spare[i] = BADHANDLE;
    }
    t->cancel = 1;
    if (t->state==TS_BLOCKED && t->wait==W_IO) {
        taskfail(s, t, E_PIPE);  // else it completes by itself
    } else if (t->state==TS_BLOCKED && t->wait==W_PIPE) {
        taskcancel(s, t);
    }
}

// End a bridge pump once its program has exited: a client yet to come
// will not, and a reading client is gone; what writing clients wrote
// is copied to the end, theirs that connected unnoticed too.
static void endbridge(Sched *s, Task *t)
{
    Pump *p = (Pump *)t->arg;
    p->ending = 1;
    if (p->kind == PUMP_SERVE) {
        cancelbridge(s, t);
    } else {
        for (i32 i = 0; i < NSPARE; i++) {
            if (p->spare[i] != BADHANDLE) os_close(s->os, p->spare[i]);
            p->spare[i] = BADHANDLE;
        }
        if (p->waiting) t->deadline = os_now(s->os);
    }
    joinpump(s, t);
}

// Stop a pump from newpump, once its handle's other end is closed,
// and return its Pump to the pool.
static void endpump(Sched *s, Task *t)
{
    taskcancel(s, t);
    joinpump(s, t);
}
