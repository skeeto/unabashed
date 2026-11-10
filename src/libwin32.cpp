// libunabashed on Windows: runs of the shell inside a host process
//
// As on POSIX (libposix.cpp): a run is the shell's whole life as a
// process, on a fiber of its own on the host's thread, which the run
// converts to a fiber for the run (unless it is one already). Setup and
// teardown run on the host's fiber; sh_main on the run's shell fiber.
// Every resource the run takes is recorded the moment it exists
// (reservations, handles, fibers, helpers, process waits, programs), so
// that teardown can end the run from anywhere without the core.
//
// What differs from the executable (win32.cpp, under LIBMODE):
// - no console control handler, signal channel, Ctrl-C judging,
//   ExitProcess, console mode or title, or image self-spawn;
// - the run's thread never blocks in a pipe or console read or write
//   of its own: helpers do, while it waits on its wake event, where a
//   signal or a cancel finds it (syncio);
// - programs start suspended in the handle's kill-on-close job, each in
//   a console process group of its own, inside the host's spawn lock,
//   inheriting only their own handles (Vista and later);
// - signal numbers are Linux's, as everywhere in the shell.
// This is free and unencumbered software released into the public domain.

enum : i32 { E_CANCELED = 125 };

enum : u32 { REQ_HARD = 1 };

// A reservation of the run's, after a header that links it.
struct LibRes {
    LibRes *next;
    LibRes *prev;
    iz      len;   // the whole reservation, header included
};
enum : iz { RESHEAD = (iz)1<<16 };  // the allocation granularity

struct LibKid {
    i64 pid;
    uz  proc;       // the core's handle, until reaped
    u64 sigign;     // signals it started with ignored
    b32 shared;     // shares the host's console (UB_SHARECONSOLE)
    b32 injob;      // in the run's job
    b32 announced;  // given to the child hook
    b32 reaped;
};

enum : i32 { MAXKIDS = 1024, HSET = 1<<12, MAXFIBS = 4096 };

using IsThreadAFiberFn = b32 (__stdcall *)();

struct Lib {
    Os         os;  // first: an Os* is a Lib*
    ub_config  cfg;
    i32        running;   // a run is active (atomic)
    u32        req;       // REQ_HARD (atomic)
    b32        inrun;     // on a run fiber, so it can be aborted
    i32        why;       // the abort's error, 0 for a normal end
    i32        status;    // sh_main's
    Fiber     *shell;
    ub_cmd const *cmd;
    i64        pid;       // $$
    Arena      arena;     // the run's own: OsInfo
    c16       *tmpdir;    // from the run's environment, or null
    uz         errh;      // the run's stderr, once it has one
    c16       *envblock;  // the host's environment, while copied
    Str        root;      // ub_config.root normalized, with its slash
    Str        self;      // ub_config.self, with forward slashes
    b32        converted; // the library made the thread a fiber
    IsThreadAFiberFn isfiber;
    b32  (__stdcall *cancelsync0)(uz);  // dynamics', before xpio
    i32        conorphans; // helpers left in a console read: never freed
    u64        entryign;  // signals ignored at entry

    // Memory
    LibRes    *res;
    iz         committed;
    iz         memlimit;  // -1 for none

    // Handles the run owns: an open-addressed set (counted ones are its
    // descriptors, within its budget)
    uz         hset[HSET];
    u8         hcount[HSET];
    i32        nset;
    i32        nfds;
    i32        maxfds;

    // Fibers the run made (DeleteFiber at its end)
    void      *fibs[MAXFIBS];
    i32        nfibs;

    // Programs
    LibKid     kids[MAXKIDS];
    i32        nkids;
};

// The run on this thread: the current fiber's (TEB FiberData). Not a
// fiber at all: below 64 KiB (0, or 0x1E00 on a thread never one).
static void *tebfiber()
{
    void *f = 0;
    #if defined(__x86_64__)
    __asm volatile ("mov %%gs:0x20, %0" : "=r"(f));
    #elif defined(__i386__)
    __asm volatile ("mov %%fs:0x10, %0" : "=r"(f));
    #elif defined(__aarch64__)
    byte *teb = 0;
    __asm volatile ("mov %0, x18" : "=r"(teb));
    __builtin_memcpy(&f, teb+0x20, sizeof(f));
    #endif
    return f;
}

// Only on a run's fiber: on the host's (perhaps a fiber of its own) the
// fiber's data is not a Fiber.
static Lib *runlib()
{
    void *f = tebfiber();
    if ((uz)f < 0x10000) return 0;
    Fiber *fb = *(Fiber **)f;
    return fb ? (Lib *)fb->os : 0;
}


// Hooks

static i32 libspin;

static void defaultlock(void *, int lock)
{
    if (!lock) {
        __atomic_store_n(&libspin, 0, __ATOMIC_RELEASE);
        return;
    }
    while (__atomic_exchange_n(&libspin, 1, __ATOMIC_ACQUIRE)) {
        Sleep(0);
    }
}

static void lib_spawnlock(Os *os, b32 lock)
{
    Lib *l = (Lib *)os;
    l->cfg.spawnlock(l->cfg.user, lock);
}


// Aborts and check points

[[noreturn]] static void lib_abort(Os *os, i32 err)
{
    Lib *l = (Lib *)os;
    if (!l->inrun || os->fibers.current==&os->fibers.main || os->inwindow) {
        __builtin_trap();  // never on the host's fiber, nor in the lock
    }
    l->why   = err;
    l->inrun = 0;
    os_switch(os, &os->fibers.main);
    __builtin_trap();  // never resumed
}

static b32 lib_hard(Os *os)
{
    Lib *l = (Lib *)os;
    return l->inrun && (__atomic_load_n(&l->req, __ATOMIC_ACQUIRE) & REQ_HARD);
}

static void checkhard(Lib *l)
{
    if (lib_hard(&l->os)) lib_abort(&l->os, E_CANCELED);
}

static void lib_step()
{
    Lib *l = runlib();
    if (l) checkhard(l);
}

static Str lib_root(Os *os)
{
    return ((Lib *)os)->root;
}

static c16 *lib_tmpdir(Os *os)
{
    return ((Lib *)os)->tmpdir;
}


// Memory: every reservation is listed, so teardown releases them all

static byte *libreserve(Os *os, iz len)
{
    Lib *l = (Lib *)os;
    if (len<0 || len>(iz)(~(uz)0>>2) - RESHEAD) return 0;
    byte *p = VirtualAlloc(0, len+RESHEAD, MEM_RESERVE, PAGE_NOACCESS);
    if (!p) return 0;
    if (!VirtualAlloc(p, sizeof(LibRes), MEM_COMMIT, PAGE_READWRITE)) {
        VirtualFree(p, 0, MEM_RELEASE);
        return 0;
    }
    LibRes *r = (LibRes *)p;
    r->len  = len + RESHEAD;
    r->prev = 0;
    r->next = l->res;
    if (l->res) l->res->prev = r;
    l->res = r;
    return p + RESHEAD;
}

static void libunreserve(Lib *l, byte *p)
{
    LibRes *r = (LibRes *)(p - RESHEAD);
    if (r->prev) r->prev->next = r->next;
    else l->res = r->next;
    if (r->next) r->next->prev = r->prev;
    VirtualFree((byte *)r, 0, MEM_RELEASE);
}

static b32 libcommit(Os *os, byte *p, iz len)
{
    Lib *l = (Lib *)os;
    checkhard(l);
    if (l->memlimit>=0 && len>l->memlimit-l->committed) return 0;
    if (!plt_commit(os, p, len)) return 0;
    l->committed += len;
    return 1;
}

static void libdecommit(Os *os, byte *p, iz len)
{
    Lib *l = (Lib *)os;
    plt_decommit(os, p, len);
    l->committed -= len<l->committed ? len : l->committed;
}

// A read-ahead buffer (reserveahead), counted against the limit.
static u8 *lib_ahead(Os *os, iz cap)
{
    byte *p = libreserve(os, cap);
    if (!p || !libcommit(os, p, cap)) os_oom();
    return (u8 *)p;
}

static void lib_unahead(Os *os, u8 *buf, iz cap)
{
    Lib *l = (Lib *)os;
    l->committed -= cap<l->committed ? cap : l->committed;
    libunreserve(l, (byte *)buf);
}

// An arena as newarena makes one, but failing rather than aborting.
static b32 tryarena(Lib *l, Arena *a, iz reserve)
{
    byte *base = libreserve(&l->os, reserve);
    iz    head = (iz)1<<16;
    if (!base || !libcommit(&l->os, base, head)) return 0;
    Mem *m = (Mem *)base;
    m->os     = &l->os;
    m->commit = base + head;
    m->limit  = base + reserve;
    a->beg = base + sizeof(Mem);
    a->end = m->commit;
    a->mem = m;
    return 1;
}

// Fibers: each the run makes is deleted at its end.
static void lib_fiber(Os *os, void *handle)
{
    Lib *l = (Lib *)os;
    if (l->nfibs == MAXFIBS) {
        DeleteFiber(handle);
        os_oom();
    }
    l->fibs[l->nfibs++] = handle;
}

static void lib_unfiber(Os *os, void *handle)
{
    Lib *l = (Lib *)os;
    for (i32 i = 0; i < l->nfibs; i++) {
        if (l->fibs[i] == handle) {
            l->fibs[i] = l->fibs[--l->nfibs];
            return;
        }
    }
}


// Handles: the run's own, within its budget

static i32 hhash(uz h)
{
    return (i32)(((u64)(h>>2) * 0x9e3779b97f4a7c15u) >> (64 - 12));
}

static i32 hfind(Lib *l, uz h)
{
    if (!h || h==INVALID_HANDLE_VALUE) return -1;
    for (i32 i = hhash(h);; i = (i+1) & (HSET-1)) {
        if (l->hset[i] == h) return i;
        if (!l->hset[i]) return -1;
    }
}

static b32 lib_handle(Os *os, uz h, b32 counted)
{
    Lib *l = (Lib *)os;
    if (!h || h==INVALID_HANDLE_VALUE) return 1;
    i32 i = hfind(l, h);
    if (i >= 0) {  // a value recycled under a stale record: retake it
        l->nfds -= l->hcount[i];
        l->hcount[i] = 0;
    } else {
        if (l->nset >= HSET/4*3) return 0;
        for (i = hhash(h); l->hset[i]; i = (i+1) & (HSET-1)) {}
        l->hset[i] = h;
        l->nset++;
    }
    if (counted && l->nfds>=l->maxfds) {
        l->hcount[i] = 0;
        lib_unhandle(os, h);
        return 0;
    }
    l->hcount[i] = (u8)!!counted;
    l->nfds += !!counted;
    return 1;
}

static void lib_unhandle(Os *os, uz h)
{
    Lib *l = (Lib *)os;
    i32  i = hfind(l, h);
    if (i < 0) return;
    if (h == l->errh) l->errh = 0;  // closed: never the OOM message's
    l->nfds -= l->hcount[i];
    l->nset--;
    // Backward-shift deletion: no tombstones
    for (i32 j = i;;) {
        j = (j+1) & (HSET-1);
        if (!l->hset[j]) break;
        i32 k = hhash(l->hset[j]);
        if (i<=j ? i<k && k<=j : i<k || k<=j) continue;
        l->hset[i]   = l->hset[j];
        l->hcount[i] = l->hcount[j];
        i = j;
    }
    l->hset[i]   = 0;
    l->hcount[i] = 0;
}

// One of the run's descriptors (not a host's, nor a private one)
static b32 owned(Lib *l, uz h)
{
    i32 i = hfind(l, h);
    return i>=0 && l->hcount[i];
}

static void libclose(Os *os, Handle h)
{
    if (!owned((Lib *)os, (uz)h)) {
        flushw(os);
        return;  // never the host's
    }
    plt_close(os, h);
}

static Handle libdup(Os *os, Handle h)
{
    if (!owned((Lib *)os, (uz)h)) {
        flushw(os);
        return BADHANDLE;
    }
    return plt_dup(os, h);
}


// Programs

static LibKid *findkid(Lib *l, uz proc)
{
    for (i32 i = 0; i < l->nkids; i++) {
        if (!l->kids[i].reaped && l->kids[i].proc==proc) return l->kids + i;
    }
    return 0;
}

// Inside the spawn lock: no allocation, no check point.
static void lib_spawned(Os *os, uz proc, i64 pid, u64 sigign, b32 shared, b32 injob)
{
    Lib    *l = (Lib *)os;
    LibKid *k = l->kids + l->nkids++;  // room made by libspawn
    k->pid       = pid;
    k->proc      = proc;
    k->sigign    = sigign;
    k->shared    = shared;
    k->injob     = injob;
    k->announced = 0;
    k->reaped    = 0;
}

static void lib_reaped(Os *os, uz proc)
{
    Lib    *l = (Lib *)os;
    LibKid *k = findkid(l, proc);
    if (!k) return;
    k->reaped = 1;
    k->proc   = 0;
    if (k->announced && l->cfg.child) l->cfg.child(l->cfg.user, k->pid, 0);
}

// Room for one more: reaped programs are forgotten when the table fills.
static b32 kidroom(Lib *l)
{
    if (l->nkids < MAXKIDS) return 1;
    i32 n = 0;
    for (i32 i = 0; i < l->nkids; i++) {
        if (!l->kids[i].reaped) l->kids[n++] = l->kids[i];
    }
    l->nkids = n;
    return n < MAXKIDS;
}

static Spawned libspawn(Os *os, Arena scratch, Spawn *s)
{
    Lib *l = (Lib *)os;
    checkhard(l);
    if (!kidroom(l)) {
        flushw(os);
        Spawned r = {};
        r.proc = BADHANDLE;
        r.err  = E_AGAIN;
        return r;
    }
    i32 before = l->nkids;
    Spawned r = plt_spawn(os, scratch, s);
    if (l->nkids > before) {
        LibKid *k = l->kids + before;
        k->announced = 1;
        if (l->cfg.child) l->cfg.child(l->cfg.user, k->pid, 1);
    }
    return r;
}

static i32 libwaitproc(Os *os, Handle proc)
{
    checkhard((Lib *)os);
    return plt_waitproc(os, proc);
}

// A wait that a cancel ends (sigwaiting sees it): the run's end, not
// the scheduler's deadlock.
static Io *libwait(Os *os, i64 deadline)
{
    Lib *l = (Lib *)os;
    checkhard(l);
    Io *io = plt_wait(os, deadline);
    if (!io) checkhard(l);
    return io;
}

static void recordkill(Os *os, i64 pid, u32 code)
{
    i32     i = os->nkilled % (i32)countof(os->killed);
    Killed *k = os->killed + i;
    k->pid  = (i32)pid;
    k->code = code;
    os->nkilled = i + 1;
}

static i32 libkill(Os *os, i64 pid, i32 sig)
{
    Lib *l = (Lib *)os;
    flushw(os);
    if (!pid || pid==-l->pid) {
        // The run's own group: every level and program, at the next
        // check point
        if (sig) {
            if (sig<1 || sig>64) return E_INVAL;
            __atomic_fetch_or(&os->siggrp, (u64)1<<(sig-1), __ATOMIC_SEQ_CST);
            SetEvent(os->wake);
        }
        return 0;
    }
    i64 host = GetCurrentProcessId();
    if (pid==-1 || pid==l->pid || pid==-l->pid) return E_PERM;
    if (pid==host || pid==-host) return sig ? E_PERM : 0;
    return plt_kill(os, pid, sig);
}


// Signals: ub_signal sets the bits, as the executable's console handler
// does

static u64 libsigpending(Os *os)
{
    checkhard((Lib *)os);
    return plt_sigpending(os);
}

// A program's own background job, with whatever it started: only one
// that holds it, never the job of an earlier program of its pid.
static uz kidjob(Os *os, LibKid *k)
{
    uz  job = bgjobof(os, k->pid);
    b32 in  = 0;
    return job && IsProcessInJob(k->proc, job, &in) && in ? job : 0;
}

// A group signal the core takes goes to every program the run started,
// then to every level (ossignals): an unabashed shell through its
// signal channel, another program through the host's softsig hook, or
// it is ended as by that signal. Not those that ignore it, and with a
// shared console not INT and QUIT, which the console delivered itself.
static u64 libsiggroup(Os *os)
{
    Lib *l = (Lib *)os;
    u64 bits = plt_siggroup(os);
    for (i32 sig = 1; sig<=64 && bits>>(sig-1); sig++) {
        if (!(bits>>(sig-1) & 1)) continue;
        b32 kills = sig!=17 && sig!=18 && !(sig>=19 && sig<=23) && sig!=28;
        if (!kills) continue;  // ignored by default, or a stop
        for (i32 i = 0; i < l->nkids; i++) {
            LibKid *k = l->kids + i;
            if (k->reaped || (k->sigign>>(sig-1) & 1)) continue;
            if (os->sharecon && k->shared && (sig==2 || sig==3)) continue;
            if (sig != 9) {
                if (!chansend(k->pid, 0, sig)) continue;
                if (l->cfg.softsig && !l->cfg.softsig(l->cfg.user, k->pid, sig)) {
                    continue;
                }
            }
            u32 code = sig==2 ? 0xc000013au : SIGEXIT|(u32)(128+sig);
            uz  job  = kidjob(os, k);
            if ((job && TerminateJobObject(job, code)) ||
                TerminateProcess(k->proc, code)) {
                recordkill(os, k->pid, code);
            }
        }
    }
    return bits;
}


// Refusals: nothing that would change the host process

static ImageRef libimagenew(Os *os, iz)
{
    flushw(os);
    return {};  // levels are fibers
}

static void libimagefree(Os *, ImageRef)
{
}

static i32 libexec(Os *os, Arena, Spawn *)
{
    flushw(os);
    return E_NOSYS;  // the core spawns and waits instead
}

static i32 libforkexec(Os *os, Arena, Spawn *)
{
    flushw(os);
    return E_NOSYS;
}

static i32 libsiglevel(Os *os, i64, i64, i32)
{
    flushw(os);
    return E_NOSYS;
}

static LevelSig liblevelsig(Os *)
{
    return {};
}

static void libsigmode(Os *os, i32, i32)
{
    flushw(os);
}

static Tty libtty(Os *os, Handle, Handle, i32)
{
    flushw(os);
    Tty r = {};
    r.err = E_NOTTY;
    return r;
}

static b32 libcon(Os *os, Handle, ConOp *)
{
    flushw(os);
    return 0;  // the host's console
}

static ClipText libclipget(Os *os, Arena *)
{
    flushw(os);
    ClipText r = {};
    r.err = E_NOSYS;
    return r;
}

static i32 libclipset(Os *os, Arena, Str)
{
    flushw(os);
    return E_NOSYS;
}

static constexpr OsVtable libtable(OsVtable t)
{
    t.reserve    = libreserve;
    t.commit     = libcommit;
    t.decommit   = libdecommit;
    t.close      = libclose;
    t.dup        = libdup;
    t.tty        = libtty;
    t.con        = libcon;
    t.spawn      = libspawn;
    t.imagenew   = libimagenew;
    t.imagefree  = libimagefree;
    t.exec       = libexec;
    t.forkexec   = libforkexec;
    t.waitproc   = libwaitproc;
    t.wait       = libwait;
    t.kill       = libkill;
    t.siglevel   = libsiglevel;
    t.levelsig   = liblevelsig;
    t.sigpending = libsigpending;
    t.siggroup   = libsiggroup;
    t.sigmode    = libsigmode;
    t.clipget    = libclipget;
    t.clipset    = libclipset;
    return t;
}
static constexpr OsVtable libvtable = libtable(pltvtable);


// A run

static Str libclone(Arena *a, char const *s)
{
    Str r = {};
    if (!s) return r;
    for (; s[r.len]; r.len++) {}
    r.data = (u8 *)allocbytes(a, r.len);
    copybytes(r.data, (void *)s, r.len);
    return r;
}

// Native form: forward slashes, no \\?\ prefix.
static Str nativeform(Str s)
{
    if (startswith(s, "\\\\?\\UNC\\")) {
        s = cuthead(s, 6);
        s[0] = '\\';
    } else if (startswith(s, "\\\\?\\")) {
        s = cuthead(s, 4);
    }
    for (iz i = 0; i < s.len; i++) {
        s[i] = s[i]=='\\' ? '/' : s[i];
    }
    return s;
}

static b32 absolute(char const *p)
{
    b32 drive = ((p[0]|0x20)>='a' && (p[0]|0x20)<='z') && p[1]==':' &&
                (p[2]=='/' || p[2]=='\\');
    b32 unc   = (p[0]=='/' || p[0]=='\\') && (p[1]=='/' || p[1]=='\\');
    return drive || unc;
}

// $$ for a run without one: odd, never a real pid (those are multiples
// of 4 below 2^32), never another run's, below 2^47 (so that a level's
// pid, $$ shifted 16 bits, fits).
static i64 virtualpid()
{
    static u32 serial;
    u32 n = __atomic_fetch_add(&serial, 1u, __ATOMIC_RELAXED) % (1u<<15);
    i64 pid = GetCurrentProcessId();
    return ((i64)1<<46) + ((pid/4)<<16) + 2*(i64)n + 1;
}

// The run's own non-inheritable duplicate of a host handle, or of the
// null device for -1 and 0, registered as one of its descriptors.
static uz runhandle(Lib *l, uz host)
{
    Os *os  = &l->os;
    uz  cur = GetCurrentProcess();
    uz  h   = 0;
    if (!host || host==INVALID_HANDLE_VALUE) {
        h = CreateFileW((c16 *)u"NUL", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_ALL,
                        0, OPEN_EXISTING, 0, 0);
        h = h==INVALID_HANDLE_VALUE ? 0 : h;
    } else if (!DuplicateHandle(cur, host, cur, &h, 0, 0, DUPLICATE_SAME_ACCESS)) {
        h = 0;
    }
    if (!h || !regh(os, h, 1)) lib_abort(os, E_BADF);
    return h;
}

// The run's shell fiber: the process's main.
static void runmain(Os *os, void *arg)
{
    Lib          *l   = (Lib *)arg;
    ub_cmd const *cmd = l->cmd;
    Arena        *a   = &l->arena;
    l->inrun = 1;
    checkhard(l);

    OsInfo info = {};
    if (cmd->cmdline) {
        info.cmdline = libclone(a, cmd->cmdline);
        Slice<Str> args = splitcmdline(a, clone(a, info.cmdline));
        info.args  = args.data;
        info.nargs = args.len;
    } else {
        info.nargs = cmd->argc;
        info.args  = alloc<Str>(a, cmd->argc);
        for (i32 i = 0; i < cmd->argc; i++) {
            info.args[i] = libclone(a, cmd->argv[i]);
        }
    }

    if (cmd->env) {
        for (; cmd->env[info.nenv]; info.nenv++) {}
        info.env = alloc<Str>(a, info.nenv);
        for (iz i = 0; i < info.nenv; i++) {
            info.env[i] = libclone(a, cmd->env[i]);
        }
    } else {
        // Freed at the run's end if copying it runs out of memory
        Slice<Str> env = {};
        l->envblock = GetEnvironmentStringsW();
        for (c16 *p = l->envblock; p && *p;) {
            iz n = 0;
            for (; p[n]; n++) {}
            env = push(a, env, fromwide(a, p, n));
            p += n + 1;
        }
        if (l->envblock) FreeEnvironmentStringsW(l->envblock);
        l->envblock = 0;
        info.env  = env.data;
        info.nenv = env.len;
    }
    os->cancelsync = l->cancelsync0;
    xpio(os, info.env, info.nenv);

    // The temporary directory as GetTempPathW finds it, but in the
    // run's environment
    Str tmp = envget(info.env, info.nenv, "TMP");
    tmp = tmp.len ? tmp : envget(info.env, info.nenv, "TEMP");
    tmp = tmp.len ? tmp : envget(info.env, info.nenv, "USERPROFILE");
    if (tmp.len) {
        Str16 w = towide(a, tmp);
        c16  *d = alloc<c16>(a, w.len+2);
        copybytes(d, w.data, w.len*(iz)sizeof(c16));
        iz n = w.len;
        if (d[n-1]!='\\' && d[n-1]!='/') d[n++] = '\\';
        d[n] = 0;
        l->tmpdir = d;
    }

    if (cmd->cwd) {
        info.cwd = nativeform(libclone(a, cmd->cwd));
        for (; info.cwd.len>3 && info.cwd[info.cwd.len-1]=='/'; info.cwd.len--) {}
    } else {
        for (i32 cap = 260;; cap *= 2) {
            c16 *buf = alloc<c16>(a, cap);
            i32  len = GetCurrentDirectoryW(cap, buf);
            if (len>0 && len<cap) {
                info.cwd = nativeform(fromwide(a, buf, len));
                break;
            } else if (len<=0 || cap>=1<<16) {
                info.cwd = "C:/";
                break;
            }
        }
    }

    info.windows = 1;
    info.exe     = l->self.len ? clone(a, l->self) : Str{};
    hostinfo(a, &info);
    info.ppid    = cmd->ppid ? cmd->ppid : GetCurrentProcessId();
    info.pid     = l->pid;
    info.reserve = l->cfg.reserve;
    info.stack   = l->cfg.stack;
    info.sigign  = l->entryign;

    // The run's own copies of the host's handles, which it closes: one
    // object for equal values, as the executable has for its own
    uz *hostv = alloc<uz>(a, 3 + cmd->nfds);
    for (i32 i = 0; i < 3; i++) {
        uz host = (uz)cmd->std[i];
        uz h    = runhandle(l, host);
        Wobj *w = 0;
        for (i32 j = 0; j<i && !w && host; j++) {
            w = hostv[j]==host ? lookup(os, (uz)info.std[j]) : 0;
        }
        if (w) {
            reg(os, h, w);
        } else {
            track(os, h);
        }
        hostv[i]    = host==INVALID_HANDLE_VALUE ? 0 : host;
        info.std[i] = (Handle)h;
    }
    l->errh = (uz)info.std[2];
    info.inherit = alloc<InheritFd>(a, cmd->nfds);
    for (i32 i = 0; i < cmd->nfds; i++) {
        uz host = (uz)cmd->fds[i].h;
        if (!host || host==INVALID_HANDLE_VALUE) lib_abort(os, E_BADF);
        uz h = runhandle(l, host);
        Wobj *w = 0;
        for (i32 j = 0; j<3+i && !w; j++) {
            Handle hj = j<3 ? info.std[j] : info.inherit[j-3].h;
            w = hostv[j]==host ? lookup(os, (uz)hj) : 0;
        }
        if (w) {
            reg(os, h, w);
        } else {
            track(os, h);
        }
        hostv[3+i] = host;
        InheritFd *e = info.inherit + info.ninherit++;
        e->fd    = cmd->fds[i].fd;
        e->h     = (Handle)h;
        e->flags = OF_READ | OF_WRITE;
    }

    // The null device for an empty handle list (libstartproc)
    os->nulh = CreateFileW((c16 *)u"NUL", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_ALL,
                           0, OPEN_EXISTING, 0, 0);
    os->nulh = os->nulh==INVALID_HANDLE_VALUE ? 0 : os->nulh;
    if (os->nulh && !regh(os, os->nulh, 0)) os->nulh = 0;

    l->status = sh_main(os, &info);
    l->inrun  = 0;
    os_switch(os, &os->fibers.main);
    __builtin_trap();  // never resumed
}

// Every helper ended or orphaned: an idle one is told to quit and
// joined; a busy one is cancelled where the system can (Vista), its
// connect ended by becoming the client, and given until a deadline;
// one still busy then is orphaned, to free itself when its call ends.
static void endhelpers(Lib *l)
{
    Os *os = &l->os;
    i32 n  = os->nhelpers;
    for (i32 i = 0; i < n; i++) {
        Helper *hp = os->helpers[i];
        if (__atomic_load_n(&hp->live, __ATOMIC_SEQ_CST) != 1) continue;
        __atomic_store_n(&hp->stop, 1, __ATOMIC_SEQ_CST);
        if (hp->op == H_CONNECT) {
            Wobj *w = hp->obj;
            i32 access = w->pipeout ? GENERIC_READ : GENERIC_WRITE;
            uz  c = CreateFileW(w->pname, access, 0, 0, OPEN_EXISTING, 0, 0);
            if (c != INVALID_HANDLE_VALUE) CloseHandle(c);
        }
    }
    u32 start = GetTickCount();
    for (;;) {
        b32 busy = 0;
        for (i32 i = 0; i < n; i++) {
            Helper *hp = os->helpers[i];
            if (__atomic_load_n(&hp->live, __ATOMIC_SEQ_CST) != 1) continue;
            busy = 1;
            // Never a console read: the console would drop the next line
            if (os->cancelsync && hp->kind!=K_CONIN) os->cancelsync(hp->thread);
        }
        if (!busy || GetTickCount()-start >= 100) break;
        Sleep(1);
    }
    for (i32 i = 0; i < n; i++) {
        Helper *hp = os->helpers[i];
        uz  thread = hp->thread;
        uz  go     = hp->go;
        uz  own    = hp->own;
        b32 con    = hp->kind == K_CONIN;
        i32 one    = 1;
        if (__atomic_compare_exchange_n(&hp->live, &one, 2, 0,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            // Orphaned: it closes own and go, and frees its block
            lib_unhandle(os, own);
            CloseHandle(thread);
            l->conorphans += con;
            continue;
        }
        hp->op = H_QUIT;
        SetEvent(go);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        CloseHandle(go);
        VirtualFree((byte *)hp, 0, MEM_RELEASE);
    }
    os->nhelpers = 0;
    os->idle     = 0;
}

// Everything the run took, back: from any state setup or an abort left.
static void teardown(Lib *l)
{
    Os *os = &l->os;
    l->inrun = 0;
    if (os->inwindow) __builtin_trap();
    flushw(os);

    // Out of memory: said on the run's stderr, if that cannot block (a
    // file or a console, no helper writing it)
    if (l->why==E_NOMEM && l->errh) {
        Wobj *w = lookup(os, l->errh);
        if (w ? w->kind==K_CONOUT && !busy(w) : GetFileType(l->errh)==FILE_TYPE_DISK) {
            Str msg = "bash: out of memory\n";
            i32 n   = 0;
            WriteFile(l->errh, msg.data, (i32)msg.len, &n, 0);
        }
    }

    // Every program, with whatever it started (the job holds both)
    u32 code = SIGEXIT | (128+9);
    TerminateJobObject(os->runjob, code);
    for (i32 i = 0; i < l->nkids; i++) {
        LibKid *k = l->kids + i;
        if (!k->reaped) {
            uz job = kidjob(os, k);
            if (job) TerminateJobObject(job, code);
            if (!k->injob) TerminateProcess(k->proc, code);
        }
    }
    for (u32 start = GetTickCount(); GetTickCount()-start < 1000; Sleep(1)) {
        JobCounts c = {};
        b32 left = QueryInformationJobObject(os->runjob, JobObjectBasicAccountingInformation,
                                             &c, sizeof(c), 0) && c.active;
        for (i32 i = 0; i<l->nkids && !left; i++) {
            LibKid *k = l->kids + i;
            left = !k->reaped && !k->injob && WaitForSingleObject(k->proc, 0);
        }
        if (!left) break;
    }

    // Thread pool waits, before the handles they watch close
    for (Pwait *pw = os->pwaits; pw; pw = pw->next) {
        UnregisterWaitEx(pw->wait, INVALID_HANDLE_VALUE);
    }
    os->pwaits = 0;

    endhelpers(l);
    (void)__atomic_exchange_n(&os->wakeq, (Wnode *)0, __ATOMIC_ACQUIRE);

    if (os->findh) FindClose(os->findh);
    os->findh = 0;
    for (i32 i = 0; i < HSET; i++) {
        if (l->hset[i]) CloseHandle(l->hset[i]);
        l->hset[i]   = 0;
        l->hcount[i] = 0;
    }
    l->nset = l->nfds = 0;
    for (i32 i = 0; i < l->nkids; i++) {
        LibKid *k = l->kids + i;
        if (k->reaped) continue;
        CloseHandle(k->proc);
        if (k->announced && l->cfg.child) l->cfg.child(l->cfg.user, k->pid, 0);
    }
    l->nkids = 0;

    for (i32 i = 0; i < l->nfibs; i++) DeleteFiber(l->fibs[i]);
    l->nfibs = 0;
    if (l->converted) ConvertFiberToThread();
    l->converted = 0;
    os->fibers   = {};

    if (l->envblock) FreeEnvironmentStringsW(l->envblock);
    l->envblock = 0;
    for (LibRes *r = l->res, *next; r; r = next) {
        next = r->next;
        VirtualFree((byte *)r, 0, MEM_RELEASE);
    }
    l->res       = 0;
    l->committed = 0;
    os->perm     = {};
    os->wlen     = 0;
    // Nothing left pointing into what was released, nor this run's abort,
    // for a later run's early return (-UB_EINVAL) to find
    for (i32 i = 0; i < countof(os->htab); i++) os->htab[i] = 0;
    os->freehent  = 0;
    os->freeobj   = 0;
    os->lends     = 0;
    os->freelend  = 0;
    os->freepwait = 0;
    os->freeip    = 0;
    os->hungry    = 0;
    os->done      = 0;
    os->donetail  = &os->done;
    os->zero      = 0;
    l->why  = 0;
    l->errh = 0;
    __atomic_and_fetch(&l->req, ~REQ_HARD, __ATOMIC_RELEASE);
}

static i32 setup(Lib *l, ub_cmd const *cmd)
{
    Os *os = &l->os;
    if (!cmd || (cmd->argc<1 && !cmd->cmdline) || (cmd->argc>0 && !cmd->argv) ||
        cmd->nfds<0 || (cmd->nfds && !cmd->fds) || cmd->pid<0 ||
        cmd->pid>=(i64)1<<47 || (cmd->cwd && !absolute(cmd->cwd))) {
        return -E_INVAL;
    }
    for (i32 i = 0; !cmd->cmdline && i<cmd->argc; i++) {
        if (!cmd->argv[i]) return -E_INVAL;
    }
    for (i32 i = 0; i < cmd->nfds; i++) {
        if (cmd->fds[i].fd < 3) return -E_INVAL;
    }

    // The run's state, from scratch
    l->cmd    = cmd;
    l->why    = 0;
    l->status = 0;
    l->tmpdir = 0;
    l->errh   = 0;
    l->pid    = cmd->pid ? cmd->pid : virtualpid();
    os->fibers    = {};
    os->done      = 0;
    os->donetail  = &os->done;
    os->zero      = 0;
    os->nwait     = 0;
    os->idle      = 0;
    os->nhelpers  = 0;
    for (i32 i = 0; i < countof(os->htab); i++) os->htab[i] = 0;
    os->freehent  = 0;
    os->freeobj   = 0;
    os->lends     = 0;
    os->freelend  = 0;
    os->freepwait = 0;
    os->freeip    = 0;
    os->pwaits    = 0;
    os->hungry    = 0;
    os->findh     = 0;
    os->nulh      = 0;
    os->inwindow  = 0;
    os->wlen      = 0;
    os->wpend     = 0;
    os->werrh     = 0;
    os->ttyraw    = 0;
    os->ttykeys   = 0;
    os->nkilled   = 0;
    for (i32 i = 0; i < countof(os->killed); i++) os->killed[i] = {};
    for (i32 i = 0; i < countof(os->bgjobs); i++) os->bgjobs[i] = {};
    os->nbgjobs   = 0;
    os->libstack  = l->cfg.stack;
    os->rlinit    = 0;
    Rlimit *rl = rltable(os);
    rl[RL_NOFILE] = {l->maxfds, l->maxfds, 0};
    l->entryign   = cmd->sigign & ~((u64)1<<8 | (u64)1<<18);  // not KILL, STOP

    if (!tryarena(l, &os->perm, (iz)1 << (sizeof(void *)==8 ? 24 : 22)) ||
        !tryarena(l, &l->arena, l->cfg.reserve)) {
        return -E_NOMEM;
    }

    // The thread as a fiber: the host's own if it is one already
    b32 isfiber = l->isfiber ? l->isfiber() : (uz)tebfiber()>=0x10000;
    os->fibers.main.os = os;
    if (isfiber) {
        os->fibers.main.handle = tebfiber();  // GetCurrentFiber
    } else {
        os->fibers.main.handle = ConvertThreadToFiber(&os->fibers.main);
        if (!os->fibers.main.handle) return -E_NOMEM;
        l->converted = 1;
    }
    os->fibers.current = &os->fibers.main;

    Fiber *f = alloc<Fiber>(&l->arena);
    f->os  = os;
    f->fn  = runmain;
    f->arg = l;
    f->handle = CreateFiberEx(1<<16, (uz)l->cfg.stack, 0, fiberentry, f);
    if (!f->handle) return -E_NOMEM;
    l->fibs[l->nfibs++] = f->handle;
    l->shell = f;
    return 0;
}

static i32 librun(Lib *l, ub_cmd const *cmd)
{
    i32 idle = 0;
    if (!__atomic_compare_exchange_n(&l->running, &idle, 1, 0,
                                     __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        return -E_BUSY;
    }
    i32 r = setup(l, cmd);
    if (!r) {
        os_switch(&l->os, l->shell);
        if (l->why) {
            r = -l->why;
        } else if (l->status & WSIGNALED) {
            r = WSIGNALED | (l->status & 255);  // Linux numbering here
        } else {
            r = l->status & 255;
        }
    }
    teardown(l);
    __atomic_store_n(&l->running, 0, __ATOMIC_RELEASE);
    return r;
}

static iz cstrlen(char const *s)
{
    iz n = 0;
    for (; s && s[n]; n++) {}
    return n;
}

struct MemStatus {  // MEMORYSTATUSEX
    u32 len;
    u32 load;
    u64 totalphys, availphys, totalpage, availpage;
    u64 totalvirt, availvirt, availext;
};

static Lib *libnew(ub_config const *c)
{
    iz rootlen = cstrlen(c ? c->root : 0);
    iz selflen = cstrlen(c ? c->self : 0);
    iz size    = (iz)sizeof(Lib) + rootlen + 1 + selflen;
    Lib *l = (Lib *)VirtualAlloc(0, size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!l) return 0;
    if (c) l->cfg = *c;
    b32 wide = sizeof(void *) == 8;
    if (!l->cfg.spawnlock) l->cfg.spawnlock = defaultlock;
    if (l->cfg.reserve <= 0) l->cfg.reserve = (iz)1 << (wide ? 34 : 26);
    if (l->cfg.stack <= 0) l->cfg.stack = (iz)1 << (wide ? 24 : 23);
    l->cfg.stack = (l->cfg.stack + 0xffff) & -(iz)0x10000;
    l->memlimit = l->cfg.memlimit;
    if (!l->memlimit) {
        MemStatus m = {};
        m.len = sizeof(m);
        u64 quarter = GlobalMemoryStatusEx(&m) ? m.totalphys/4 : 0;  // in 64 bits
        u64 max     = wide ? (u64)1<<30 : (u64)1<<28;
        l->memlimit = (iz)(quarter && quarter<max ? quarter : max);
    } else if (l->memlimit < 0) {
        l->memlimit = -1;
    }
    l->maxfds = l->cfg.fds>0 ? l->cfg.fds : 256;

    // The paths, copied after the Lib: root as the install's shell
    // directory with its slash, self with forward slashes
    u8 *p = (u8 *)(l + 1);
    if (rootlen) {
        Str r = {p, rootlen};
        copybytes(r.data, (void *)c->root, rootlen);
        r = nativeform(r);
        if (r[r.len-1] != '/') r.data[r.len++] = '/';
        l->root = r;
        p = r.data + r.len;
    }
    if (selflen) {
        Str s = {p, selflen};
        copybytes(s.data, (void *)c->self, selflen);
        l->self = nativeform(s);
    }
    l->cfg.root = l->cfg.self = 0;  // the host's, not kept

    Os *os = &l->os;
    dynamics(os);
    os->lateattach = 0;  // no Ctrl-C judging
    os->jobflags   = 0;  // a program's own job lets nothing break away
    os->sharecon   = !!(l->cfg.flags & UB_SHARECONSOLE);
    l->cancelsync0 = os->cancelsync;
    l->isfiber     = (IsThreadAFiberFn)dynamic(u"kernel32.dll", "IsThreadAFiber");
    os->wake       = CreateEventW(0, 0, 0, 0);
    os->donetail   = &os->done;
    os->runjob     = CreateJobObjectW(0, 0);
    JobLimits jl = {};
    // Before Windows 8 jobs do not nest: a shell the run starts takes its
    // programs out of this job into its own, which then ends with it (as
    // the executable's jobs let its programs go)
    jl.flags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
               (os->nestedjobs ? 0 : JOB_OBJECT_LIMIT_BREAKAWAY_OK);
    if (!os->wake || !os->runjob ||
        !SetInformationJobObject(os->runjob, JobObjectExtendedLimitInformation,
                                 &jl, sizeof(jl))) {
        if (os->wake) CloseHandle(os->wake);
        if (os->runjob) CloseHandle(os->runjob);
        VirtualFree((byte *)l, 0, MEM_RELEASE);
        return 0;
    }
    return l;
}

static void libfree(Lib *l)
{
    if (!l) return;
    Os *os = &l->os;
    CloseHandle(os->runjob);  // ends any program left
    if (os->advapi) FreeLibrary(os->advapi);
    if (l->conorphans) return;  // its helper may yet signal the run
    CloseHandle(os->wake);
    VirtualFree((byte *)l, 0, MEM_RELEASE);
}

static i32 libsignal(Lib *l, i32 sig)
{
    i32 n = sig & 0xff;
    if (n<1 || n>64 || (sig & ~(0xff|UB_GROUP))) return -E_INVAL;
    u64 *bits = sig&UB_GROUP ? &l->os.siggrp : &l->os.sigown;
    __atomic_fetch_or(bits, (u64)1<<(n-1), __ATOMIC_SEQ_CST);
    SetEvent(l->os.wake);
    return 0;
}

static void libcancel(Lib *l)
{
    __atomic_fetch_or(&l->req, REQ_HARD, __ATOMIC_RELEASE);
    SetEvent(l->os.wake);
}
