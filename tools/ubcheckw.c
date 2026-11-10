// ubhost --check [--quick] on Windows: the library's host contract
//
// As ubcheck.c on POSIX: each case runs a few times after a warm-up
// run, between two snapshots of what a run could leave changed in its
// host: its address space (images and views by identity, private
// memory by total), handle table (by value, with each handle's flags),
// threads, programs (every descendant must be gone), console modes and
// title, the PEB's Ctrl-C flag, directory and environment. A
// difference fails the case, as does a status, output or time other
// than the case expects. Included by ubhostw.c, with no C runtime.
// This is free and unencumbered software released into the public domain.

typedef struct {
    uintptr_t base, allocbase;
    unsigned  allocprotect;
    uintptr_t size;
    unsigned  state, protect, type;
} MemInfo;  // MEMORY_BASIC_INFORMATION

typedef struct {
    unsigned  size, usage, pid;
    uintptr_t heap;
    unsigned  module, threads, ppid;
    int       pri;
    unsigned  flags;
    uint16_t  exe[260];
} ProcEntry;  // PROCESSENTRY32W

typedef struct {
    unsigned size, usage, tid, pid;
    int      base, delta;
    unsigned flags;
} ThreadEntry;  // THREADENTRY32

typedef struct {
    int        cb;
    void      *reserved, *desktop, *title;
    int        x, y, xsize, ysize, xchars, ychars, fill, flags;
    short      show, cbreserved2;
    void      *reserved2;
    uintptr_t  input, output, error;
} StartupInfoW;

typedef struct {
    uintptr_t process, thread;
    int       pid, tid;
} ProcessInfo;

W32(size_t,    VirtualQuery(void const *, MemInfo *, size_t));
W32(void *,    VirtualAlloc(void *, size_t, int, int));
W32(int,       GetHandleInformation(uintptr_t, unsigned *));
W32(uintptr_t, CreateToolhelp32Snapshot(int, int));
W32(int,       Process32FirstW(uintptr_t, ProcEntry *));
W32(int,       Process32NextW(uintptr_t, ProcEntry *));
W32(int,       Thread32First(uintptr_t, ThreadEntry *));
W32(int,       Thread32Next(uintptr_t, ThreadEntry *));
W32(int,       CloseHandle(uintptr_t));
W32(uintptr_t, OpenProcess(int, int, int));
W32(int,       GetProcessTimes(uintptr_t, uint64_t *, uint64_t *, uint64_t *, uint64_t *));
W32(int,       GetConsoleMode(uintptr_t, unsigned *));
W32(int,       GetConsoleTitleW(uint16_t *, int));
W32(int,       GetCurrentDirectoryW(int, uint16_t *));
W32(uint16_t *, GetEnvironmentStringsW(void));
W32(int,       FreeEnvironmentStringsW(uint16_t *));
W32(int,       QueryPerformanceCounter(int64_t *));
W32(int,       QueryPerformanceFrequency(int64_t *));
W32(int,       GetTempPathW(int, uint16_t *));
W32(uintptr_t, CreateFileW(uint16_t const *, unsigned, int, void *, int, int, uintptr_t));
W32(int,       ReadFile(uintptr_t, void *, int, int *, void *));
W32(int,       SetFilePointerEx(uintptr_t, int64_t, int64_t *, int));
W32(int,       SetEndOfFile(uintptr_t));
W32(int,       DeleteFileW(uint16_t const *));
W32(int,       CreatePipe(uintptr_t *, uintptr_t *, void *, int));
W32(int,       CreateProcessW(uint16_t const *, uint16_t *, void *, void *, int, int,
                              void *, void *, StartupInfoW *, ProcessInfo *));
W32(int,       GetExitCodeProcess(uintptr_t, unsigned *));
W32(void *,    ConvertThreadToFiber(void *));
W32(int,       ConvertFiberToThread(void));
W32(int,       GetProcessHandleCount(uintptr_t, unsigned *));
W32(int,       PeekNamedPipe(uintptr_t, void *, int, int *, int *, int *));
W32(int,       ReadProcessMemory(uintptr_t, void const *, void *, size_t, size_t *));
W32(uintptr_t, CreateEventW(void *, int, int, void *));
W32(int,       SetEvent(uintptr_t));

static int strcmp(char const *a, char const *b)
{
    for (; *a && *a==*b; a++, b++) {}
    return (unsigned char)*a - (unsigned char)*b;
}

static uintptr_t outh;  // the check's report: standard output

static void putout(char const *s, ptrdiff_t n)
{
    int w = 0;
    WriteFile(outh, s, (int)n, &w, 0);
}


// Text, without a runtime

typedef struct {
    char     *buf;
    ptrdiff_t len, cap;
} Snap;

static void snapinit(Snap *s, ptrdiff_t cap)
{
    s->buf = VirtualAlloc(0, (size_t)cap, 0x3000, 4);
    s->cap = s->buf ? cap : 0;
    s->len = 0;
}

static void sputn(Snap *s, char const *p, ptrdiff_t n)
{
    for (ptrdiff_t i = 0; i<n && s->len<s->cap-1; i++) s->buf[s->len++] = p[i];
    if (s->cap) s->buf[s->len] = 0;
}

static void sput(Snap *s, char const *p)
{
    sputn(s, p, (ptrdiff_t)strlen(p));
}

static void sputd(Snap *s, int64_t v)
{
    char tmp[24], *e = tmp+sizeof(tmp), *p = e;
    uint64_t u = v<0 ? -(uint64_t)v : (uint64_t)v;
    do *--p = (char)('0' + u%10); while (u /= 10);
    if (v < 0) *--p = '-';
    sputn(s, p, e-p);
}

static void sputx(Snap *s, uint64_t v)
{
    char tmp[16], *e = tmp+sizeof(tmp), *p = e;
    do *--p = "0123456789abcdef"[v&15]; while (v >>= 4);
    sputn(s, p, e-p);
}

static void sputw(Snap *s, uint16_t const *w, ptrdiff_t n)
{
    char tmp[3*600];
    n = n<600 ? n : 600;
    sput(s, wtf8(tmp, w, n));
}

static char *strstr_(char const *h, char const *n)
{
    ptrdiff_t ln = (ptrdiff_t)strlen(n);
    for (; *h; h++) {
        if (!memcmp(h, n, (size_t)ln)) return (char *)h;
    }
    return ln ? 0 : (char *)h;
}


// Snapshots

// Whether an address lies in ntdll's image
static int inntdll(uintptr_t a)
{
    unsigned char *base = (unsigned char *)GetModuleHandleW(u"ntdll.dll");
    if (!base) return 0;
    int pe;
    unsigned size;
    memcpy(&pe, base+0x3c, 4);
    memcpy(&size, base+pe+24+56, 4);  // OptionalHeader.SizeOfImage
    return a-(uintptr_t)base < size;
}

// A handle's object type, by NtQueryObject (ObjectTypeInformation)
static void sputtype(Snap *s, uintptr_t h)
{
    typedef int (__stdcall *Query)(uintptr_t, int, void *, int, int *);
    static Query q;
    if (!q) q = (Query)GetProcAddress(GetModuleHandleW(u"ntdll.dll"), "NtQueryObject");
    static uintptr_t buf[256];  // UNICODE_STRING first
    if (!q || q(h, 2, buf, sizeof(buf), 0)) {
        sput(s, "?");
        return;
    }
    uint16_t len;
    memcpy(&len, buf, 2);
    uint16_t *name;
    memcpy(&name, (char *)buf + sizeof(void *), sizeof(name));
    sputw(s, name, len/2);
    if (len==12 && !memcmp(name, u"Thread", 12)) {
        // The thread pool's own come and go with the load
        typedef int (__stdcall *Qt)(uintptr_t, int, void *, int, int *);
        static Qt qt;
        if (!qt) qt = (Qt)GetProcAddress(GetModuleHandleW(u"ntdll.dll"),
                                         "NtQueryInformationThread");
        uintptr_t start = 0;
        if (qt && !qt(h, 9, &start, sizeof(start), 0) && inntdll(start)) {
            sput(s, " ntdll");
        }
    }
}

typedef struct {
    Snap     text;      // compared exactly
    uint64_t private_;  // private memory reserved, in all
    int      threads;
} State;

static int64_t freq;

static double nowsec(void)
{
    int64_t t = 0;
    QueryPerformanceCounter(&t);
    return (double)t / (double)freq;
}

static CriticalSection hostcs;  // the host's spawn lock, shared with runs

// Critical sections' lock events, which XP creates the first time each
// is contended: through the process's list of them (RTL_CRITICAL_SECTION
// and its DEBUG), from the host's own, read so a bad pointer is safe.
static int locksems(uintptr_t *sems, int cap)
{
    enum { P = sizeof(void *) };
    uintptr_t self = GetCurrentProcess(), dbg = (uintptr_t)hostcs.a, start, cur, n = 0;
    if (!dbg || dbg==(uintptr_t)-1) return 0;  // none kept (Windows 8)
    start = cur = dbg + 2*P;  // its ProcessLocksList
    for (int i = 0; i<100000 && n<(uintptr_t)cap; i++) {
        uintptr_t next = 0, cs = 0, back = 0, sem = 0;
        uintptr_t d = cur - 2*P;  // the list head (in ntdll) fails back==d
        if (ReadProcessMemory(self, (void *)(d+P), &cs, P, 0) &&
            ReadProcessMemory(self, (void *)cs, &back, P, 0) && back==d &&
            ReadProcessMemory(self, (void *)(cs+2*P+8), &sem, P, 0) && sem) {
            sems[n++] = sem;
        }
        if (!ReadProcessMemory(self, (void *)cur, &next, P, 0) || next==start) break;
        cur = next;
    }
    return (int)n;
}

static void snapshot(State *st)
{
    static uintptr_t sems[4096];
    int nsems = locksems(sems, 4096);
    Snap *s = &st->text;
    s->len = 0;
    st->private_ = 0;
    MemInfo m;
    for (uintptr_t a = 0; VirtualQuery((void *)a, &m, sizeof(m)) == sizeof(m);) {
        if (m.state != 0x10000) {  // MEM_FREE
            if (m.type == 0x20000) {  // MEM_PRIVATE: heaps, stacks, the run's
                st->private_ += m.size;
            } else {  // images and views, by identity
                sput(s, "map ");
                sputx(s, m.base);
                sput(s, " ");
                sputx(s, m.size);
                sput(s, " ");
                sputx(s, m.type);
                sput(s, " ");
                sputx(s, m.state);
                sput(s, "\n");
            }
        }
        uintptr_t next = m.base + m.size;
        if (next <= a) break;
        a = next;
    }

    // Handles by value, with their flags (inherit, protect)
    for (uintptr_t h = 4; h < 1<<16; h += 4) {
        unsigned flags = 0;
        if (GetHandleInformation(h, &flags)) {
            sput(s, "handle ");
            sputx(s, h);
            sput(s, " ");
            sputd(s, flags);
            sput(s, " ");
            sputtype(s, h);
            for (int i = 0; i < nsems; i++) {
                if (sems[i] == h) sput(s, " lock");
            }
            sput(s, "\n");
        }
    }

    st->threads = 0;
    uintptr_t snap = CreateToolhelp32Snapshot(4, 0);  // TH32CS_SNAPTHREAD
    ThreadEntry te = {0};
    te.size = sizeof(te);
    int self = GetCurrentProcessId();
    for (int ok = Thread32First(snap, &te); ok; ok = Thread32Next(snap, &te)) {
        st->threads += (int)te.pid == self;
    }
    CloseHandle(snap);

    for (int i = 0; i < 3; i++) {
        unsigned mode = 0;
        if (GetConsoleMode(GetStdHandle(-10 - i), &mode)) {
            sput(s, "conmode ");
            sputd(s, i);
            sput(s, " ");
            sputx(s, mode);
            sput(s, "\n");
        }
    }
    uint16_t title[512];
    int n = GetConsoleTitleW(title, 512);
    sput(s, "title ");
    sputw(s, title, n>0 ? n : 0);
    sput(s, "\n");
    sput(s, "ctrlc ");
    sputd(s, ctrlcignored());
    sput(s, "\n");
    uint16_t cwd[1024];
    n = GetCurrentDirectoryW(1024, cwd);
    sput(s, "cwd ");
    sputw(s, cwd, n>0 && n<1024 ? n : 0);
    sput(s, "\n");
    uint16_t *env = GetEnvironmentStringsW();
    uint64_t h = 1469598103934665603ull;
    for (uint16_t *p = env; p && (p[0] || p[1]); p++) {
        h = (h ^ *p) * 1099511628211ull;
    }
    FreeEnvironmentStringsW(env);
    sput(s, "environ ");
    sputx(s, h);
    sput(s, "\n");
}

// The first line where two snapshots differ, into why.
static int hasline(Snap *s, char const *l, ptrdiff_t n)
{
    for (char *p = s->buf, *e; *p; p = e+1) {
        for (e = p; *e!='\n'; e++) {}
        if (e-p==n && !memcmp(p, l, (size_t)n)) return 1;
    }
    return 0;
}

// Every line of one snapshot missing from the other, up to a few.
// A handle of the thread pool's, which may close by itself: the host's
// own are checked as sentinels instead.
static int poolhandle(char const *p, ptrdiff_t n)
{
    static char const *const types[] = {
        " Event", " Thread ntdll", " WaitCompletionPacket", " IRTimer",
        " TpWorkerFactory", " IoCompletion",
    };
    if (n<7 || memcmp(p, "handle ", 7)) return 0;
    for (int i = 0; i < (int)(sizeof(types)/sizeof(*types)); i++) {
        ptrdiff_t len = (ptrdiff_t)strlen(types[i]);
        if (n>len && !memcmp(p+n-len, types[i], (size_t)len)) return 1;
    }
    return 0;
}

// A critical section's, which XP makes and keeps by itself
static int lockevent(char const *p, ptrdiff_t n)
{
    return n>11 && !memcmp(p+n-11, " Event lock", 11);
}

// Protected from closing (flag 2): the system's, as XP's, which fill a
// cache once under load. The library never sets the flag.
static int protected_(char const *p, ptrdiff_t n)
{
    if (n<7 || memcmp(p, "handle ", 7)) return 0;
    char const *f = p + 7;
    while (f<p+n && *f!=' ') f++;
    return f+1<p+n && (f[1]=='2' || f[1]=='3') && f+2<p+n && f[2]==' ';
}

static int linesgone(Snap *from, Snap *in, char const *mark, Snap *why, int shown)
{
    int gone = *mark == 'g';
    for (char *p = from->buf, *e; *p; p = e+1) {
        for (e = p; *e!='\n'; e++) {}
        if (!hasline(in, p, e-p) && !(gone && poolhandle(p, e-p)) && !lockevent(p, e-p) && !protected_(p, e-p)) {
            if (shown++ < 8) {
                sput(why, shown>1 ? "; " : "host changed: ");
                sput(why, mark);
                sputn(why, p, e-p);
            }
        }
    }
    return shown;
}

static int samestate(State *a, State *b, Snap *why)
{
    if (a->text.len!=b->text.len || memcmp(a->text.buf, b->text.buf, (size_t)a->text.len)) {
        int n = linesgone(&a->text, &b->text, "gone ", why, 0);
        n = linesgone(&b->text, &a->text, "new ", why, n);
        if (n) return 0;  // else only reordered
    }
    // Pool threads come and go by themselves; a run's would stay
    if (b->threads > a->threads) {
        sput(why, "threads ");
        sputd(why, a->threads);
        sput(why, " -> ");
        sputd(why, b->threads);
        return 0;
    }
    if (b->private_ > a->private_ + (4<<20)) {
        sput(why, "private memory grew ");
        sputd(why, (int64_t)((b->private_ - a->private_) >> 10));
        sput(why, " KiB");
        return 0;
    }
    return 1;
}


// Programs: every descendant of the host's, started after it, is gone

static uint64_t hostborn;

static uint64_t bornat(int pid)
{
    uint64_t c = 0, e, k, u;
    uintptr_t h = OpenProcess(0x1000, 0, pid);  // QUERY_LIMITED_INFORMATION
    if (!h) return 0;
    if (!GetProcessTimes(h, &c, &e, &k, &u)) c = 0;
    CloseHandle(h);
    return c;
}

static int kidsgone(Snap *why)
{
    int set[512], n = 0;
    set[n++] = GetCurrentProcessId();
    for (int grew = 1; grew;) {
        grew = 0;
        uintptr_t snap = CreateToolhelp32Snapshot(2, 0);  // TH32CS_SNAPPROCESS
        ProcEntry pe = {0};
        pe.size = sizeof(pe);
        for (int ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
            int known = 0, parent = 0;
            for (int i = 0; i < n; i++) {
                known  |= set[i] == (int)pe.pid;
                parent |= set[i] == (int)pe.ppid;
            }
            if (known || !parent || n==512) continue;
            uint64_t born = bornat((int)pe.pid);
            if (born && born<hostborn) continue;  // the pid's earlier owner
            set[n++] = (int)pe.pid;
            grew = 1;
        }
        CloseHandle(snap);
    }
    if (n > 1) {
        sput(why, "programs left: pid ");
        sputd(why, set[1]);
        return 0;
    }
    return 1;
}


// Cases


static void hostlock(void *user, int lock)
{
    (void)user;
    if (lock) EnterCriticalSection(&hostcs);
    else LeaveCriticalSection(&hostcs);
}

enum { ANY = 1<<30 };

typedef struct {
    char const *name;
    char const *script;
    int         status;     // or ANY
    char const *out;        // a substring the output must hold, or null
    double      maxsec;     // 0: no limit
    int         stopms;     // >0: stop it after so many ms
    int         stopsig;    // 0: ub_cancel, else ub_signal's sig
    ptrdiff_t   memlimit;
    int         fds;
    int64_t     pid;
    int         fiber;      // the host's thread is a fiber already
    int         hostpipe;   // stdin: a host pipe, written once the run ends
} Case;

typedef struct {
    ub_os *os;
    int    ms;
    int    sig;
    uintptr_t done;  // an event: the run ended
} Stopper;

static unsigned __stdcall stopper(void *arg)
{
    Stopper *s = arg;
    if (WaitForSingleObject(s->done, s->ms)) {  // timed out: still running
        if (s->sig) ub_signal(s->os, s->sig);
        else ub_cancel(s->os);
    }
    return 0;
}

static uint16_t outpath[300];

// One run of a case: its status, output in out.
static int runcase(Case const *c, char *out, int outlen, double *secs)
{
    ub_config cfg = {0};
    cfg.spawnlock = hostlock;
    cfg.memlimit  = c->memlimit;
    cfg.fds       = c->fds;
    ub_os *os = ub_new(&cfg);
    if (!os) return -9999;
    // GENERIC_READ|WRITE, share all, CREATE_ALWAYS, not inheritable
    uintptr_t f = CreateFileW(outpath, 0xc0000000u, 7, 0, 2, 0, 0);
    char const *argv[] = {"bash", "-c", c->script, 0};
    ub_cmd cmd = {0};
    cmd.argc   = 3;
    cmd.argv   = argv;
    uintptr_t pr = 0, pw = 0;
    if (c->hostpipe) CreatePipe(&pr, &pw, 0, 0);
    cmd.std[0] = c->hostpipe ? (intptr_t)pr : -1;
    cmd.std[1] = (intptr_t)f;
    cmd.std[2] = (intptr_t)f;
    cmd.pid    = c->pid;
    Stopper s = {os, c->stopms, c->stopsig, 0};
    uintptr_t t = 0;
    if (c->stopms) s.done = CreateEventW(0, 1, 0, 0);
    if (c->stopms) t = CreateThread(0, 1<<16, stopper, &s, 0, 0);
    void *fiber = c->fiber ? ConvertThreadToFiber((void *)0x5eed) : 0;
    double t0 = nowsec();
    int r = ub_run(os, &cmd);
    *secs = nowsec() - t0;
    if (fiber) {
        void *data = 0;
        #if defined(__x86_64__)
        __asm volatile ("mov %%gs:0x20, %0" : "=r"(data));
        #elif defined(__i386__)
        __asm volatile ("mov %%fs:0x10, %0" : "=r"(data));
        #endif
        if (data!=fiber || *(void **)data!=(void *)0x5eed) r = -8888;  // altered
        ConvertFiberToThread();
    }
    if (t) {
        SetEvent(s.done);
        WaitForSingleObject(t, -1);
        CloseHandle(t);
        CloseHandle(s.done);
    }
    if (c->hostpipe) {
        // The peer's next operation: an XP helper orphaned in the read
        // takes it, then frees itself
        int w = 0;
        WriteFile(pw, "x\n", 2, &w, 0);
        CloseHandle(pw);
        CloseHandle(pr);
        Sleep(200);
    }
    ub_free(os);
    int n = 0;
    SetFilePointerEx(f, 0, 0, 0);
    ReadFile(f, out, outlen-1, &n, 0);
    out[n<0 ? 0 : n] = 0;
    CloseHandle(f);
    return r;
}

static State before, after;
static Snap  report;

static void line(Snap *s)
{
    putout(s->buf, s->len);
    s->len = 0;
}

static int checkcase(Case const *c, int reps)
{
    static char out[1<<14];
    Snap why;
    snapinit(&why, 1<<12);
    double secs;
    runcase(c, out, sizeof(out), &secs);  // warm-up
    for (double until = nowsec()+2; !kidsgone(&why) && nowsec()<until;) Sleep(1);
    why.len = 0;
    snapshot(&before);
    int ok = 1;
    for (int i = 0; i<reps && ok; i++) {
        int r = runcase(c, out, sizeof(out), &secs);
        if (c->status!=ANY && r!=c->status) {
            sput(&why, "status ");
            sputd(&why, r);
            sput(&why, ", not ");
            sputd(&why, c->status);
            ok = 0;
        } else if (c->out && !strstr_(out, c->out)) {
            sput(&why, "output lacks \"");
            sput(&why, c->out);
            sput(&why, "\"");
            ok = 0;
        } else if (c->maxsec && secs>c->maxsec) {
            sput(&why, "took ");
            sputd(&why, (int64_t)(secs*1000));
            sput(&why, " ms");
            ok = 0;
        }
        if (ok) {
            double until = nowsec() + 2;
            while (!(ok = kidsgone(&why)) && nowsec()<until) Sleep(1);
            if (ok) why.len = 0;
        }
    }
    if (ok) {
        snapshot(&after);
        ok = samestate(&before, &after, &why);
    }
    sput(&report, ok ? "PASS " : "FAIL ");
    sput(&report, c->name);
    sput(&report, " ");
    sputd(&report, (int64_t)(secs*1000));
    sput(&report, " ms");
    if (!ok) {
        sput(&report, ": ");
        sput(&report, why.buf);
        if (out[0]) {
            sput(&report, "\n     output: ");
            sputn(&report, out, strlen(out)<300 ? (ptrdiff_t)strlen(out) : 300);
        }
    }
    sput(&report, "\n");
    line(&report);
    return ok;
}


// Concurrent runs, outputs compared byte for byte, while a host thread
// starts programs of its own, inheriting all it may, under the lock

static char const concurrent[] =
    "x=$(seq 50 | sort -rn | head -3); echo $x; (echo sub) | cat\n"
    "cat <<E\nhere\nE\n"
    "f=ubcheck.f.$$; echo f > $f; read y < $f; rm $f; echo $y\n"
    "{ echo bg; } & wait; for i in 1 2 3; do echo $i; done | tr '\\n' ' '; echo\n";
static char const concurrentout[] = "50 49 48\nsub\nhere\nf\nbg\n1 2 3 \n";
static int concfails;

static unsigned __stdcall concworker(void *arg)
{
    int reps = *(int *)arg;
    ub_config cfg = {0};
    cfg.spawnlock = hostlock;
    ub_os *os = ub_new(&cfg);
    for (int i = 0; i < reps; i++) {
        uintptr_t r = 0, w = 0;
        if (!CreatePipe(&r, &w, 0, 1<<16)) continue;
        char const *argv[] = {"bash", "-c", concurrent, 0};
        ub_cmd c = {0};
        c.argc   = 3;
        c.argv   = argv;
        c.std[0] = -1;
        c.std[1] = (intptr_t)w;
        c.std[2] = (intptr_t)w;
        int st = ub_run(os, &c);
        CloseHandle(w);  // the run's own copy is gone: EOF now
        char buf[4096];
        int n = 0;
        for (int k; n<(int)sizeof(buf)-1 && ReadFile(r, buf+n, (int)sizeof(buf)-1-n, &k, 0) && k;) {
            n += k;
        }
        buf[n] = 0;
        CloseHandle(r);
        if (st || strcmp(buf, concurrentout)) {
            __atomic_add_fetch(&concfails, 1, __ATOMIC_RELAXED);
            Snap e;
            snapinit(&e, 1<<13);
            sput(&e, "concurrent: status ");
            sputd(&e, st);
            sput(&e, ", output:\n");
            sput(&e, buf);
            line(&e);
        }
    }
    ub_free(os);
    return 0;
}

static uint16_t selfpath[32768];

// A program the host starts, inheriting all it may, under the lock: its
// ubhost --fds status.
static unsigned hostspawn(void)
{
    static uint16_t cmdline[] = u"ubhost --fds";
    StartupInfoW si = {0};
    si.cb = sizeof(si);
    ProcessInfo pi = {0};
    EnterCriticalSection(&hostcs);
    int ok = CreateProcessW(selfpath, cmdline, 0, 0, 1, 0x08000000, 0, 0, &si, &pi);
    LeaveCriticalSection(&hostcs);
    if (!ok) return ~0u;
    WaitForSingleObject(pi.process, -1);
    unsigned code = 0;
    GetExitCodeProcess(pi.process, &code);
    CloseHandle(pi.process);
    CloseHandle(pi.thread);
    return code;
}

// Its new console's handles, which it always has
static unsigned basefds;

static unsigned __stdcall hostspawner(void *arg)
{
    int *stop = arg;
    while (!__atomic_load_n(stop, __ATOMIC_ACQUIRE)) {
        unsigned code = hostspawn();
        if (code!=~0u && code!=basefds) {
            Snap e;
            snapinit(&e, 1<<10);
            sput(&e, "concurrent: a host program inherited ");
            sputd(&e, (int)code - 100);
            sput(&e, " handles, not ");
            sputd(&e, (int)basefds - 100);
            sput(&e, "\n");
            line(&e);
            __atomic_add_fetch(&concfails, 1, __ATOMIC_RELAXED);
        }
    }
    return 0;
}

// ubhost --fds: exit with 100 plus the number of inheritable handles it
// inherited (beyond none), else 0.
static int inheritedfds(void)
{
    int n = 0;
    for (uintptr_t h = 4; h < 1<<16; h += 4) {
        unsigned flags = 0;
        n += GetHandleInformation(h, &flags) && (flags & 1);
    }
    return n ? 100+n : 0;
}

// One round: threads of reps runs each, beside the host spawner.
static void concround(int threads, int reps)
{
    int stop = 0;
    uintptr_t sp = CreateThread(0, 1<<16, hostspawner, &stop, 0, 0), t[64];
    for (int i = 0; i < threads; i++) t[i] = CreateThread(0, 1<<16, concworker, &reps, 0, 0);
    for (int i = 0; i < threads; i++) {
        WaitForSingleObject(t[i], -1);
        CloseHandle(t[i]);
    }
    __atomic_store_n(&stop, 1, __ATOMIC_RELEASE);
    WaitForSingleObject(sp, -1);
    CloseHandle(sp);
}

static int checkconcurrent(int threads, int reps)
{
    Snap why;
    snapinit(&why, 1<<12);
    // A warm-up round: on XP the host's first spawn loads a DLL, and
    // contended locks get their events
    basefds = hostspawn();
    concround(threads, 1);
    for (double until = nowsec()+2; !kidsgone(&why) && nowsec()<until;) Sleep(1);
    why.len = 0;
    concfails = 0;
    snapshot(&before);
    double t0 = nowsec();
    concround(threads, reps);
    double secs = nowsec() - t0;
    Sleep(100);
    snapshot(&after);
    int ok = !concfails && kidsgone(&why) && samestate(&before, &after, &why);
    sput(&report, ok ? "PASS " : "FAIL ");
    sput(&report, "concurrent runs ");
    sputd(&report, (int64_t)(secs*1000));
    sput(&report, " ms");
    if (!ok) {
        sput(&report, ": ");
        sput(&report, concfails ? "outputs differ" : why.buf);
    }
    sput(&report, "\n");
    line(&report);
    return ok;
}

static int check(int quick)
{
    outh = GetStdHandle(-11);
    // As the contract asks, the host keeps its own handles from programs
    // (those it inherited too): a program it starts inherits none
    for (uintptr_t h = 4; h < 1<<16; h += 4) {
        unsigned flags = 0;
        if (GetHandleInformation(h, &flags) && (flags & 1)) SetHandleInformation(h, 1, 0);
    }
    int reps = quick ? 1 : 3;
    QueryPerformanceFrequency(&freq);
    InitializeCriticalSection(&hostcs);
    GetModuleFileNameW(0, selfpath, 32768);
    {
        uint64_t e, k, u;
        GetProcessTimes(GetCurrentProcess(), &hostborn, &e, &k, &u);
    }
    int n = GetTempPathW(260, outpath);
    static uint16_t const name[] = u"ubcheck.out";
    for (int i = 0; i < (int)(sizeof(name)/2); i++) outpath[n+i] = name[i];

    // A host pipe that must stay open and silent, never inherited
    uintptr_t sentr = 0, sentw = 0;
    CreatePipe(&sentr, &sentw, 0, 0);
    // And an event, set: pool events vanishing are not failures, so the
    // host's own are checked here
    uintptr_t sentev = CreateEventW(0, 1, 1, 0);

    snapinit(&before.text, 1<<20);
    snapinit(&after.text, 1<<20);
    snapinit(&report, 1<<14);
    snapshot(&before);
    snapshot(&after);
    sput(&report, quick ? "ubhost --check --quick\n" : "ubhost --check\n");
    line(&report);

    Case cases[] = {
        {"exit status", "exit 3", 3},
        {"kill $$", "kill -TERM $$", UB_SIGNALED|(128+15)},
        {"kill -STOP $$ refused", "kill -STOP $$ 2>/dev/null; echo s=$?", 0, "s=1"},
        {"trapped TSTP to $$", "trap 'echo t' TSTP; kill -TSTP $$", 0, "t"},
        {"kill 0", "kill 0; echo no", UB_SIGNALED|(128+15)},
        {"kill -0 group and host", "kill -0 0 && kill -0 $PPID && echo ok", 0, "ok"},
        {"kill -1, host refused",
         "kill -TERM -1 2>/dev/null; echo a=$?; kill $PPID 2>/dev/null; echo b=$?", 0,
         "a=1\nb=1"},
        {"exec", "exec true; echo no", 0},
        {"exec >&-", "exec >&-; echo x 2>/dev/null; true", 0},
        {"ulimit -n", "ulimit -n 64; ulimit -n", 0, "64"},
        {"set -m", "set -m; sleep 0 & wait; echo ok", 0, "ok"},
        {"suspend", "suspend 2>/dev/null; echo s=$?", 0, "s=1"},
        {"title", "printf '\\033]0;x\\007'; echo ok", 0, "ok"},
        {"bad pid", "true", -UB_EINVAL, 0, 0, 0, 0, 0, 0, (int64_t)1<<47},
        {"already a fiber", "echo $((6*7)) | cat", 0, "42", 0, 0, 0, 0, 0, 0, 1},
        {"programs and pipes", "x=$(echo a | tr a b); echo $x; sh -c 'exit 4'; echo s=$?",
         0, "b\ns=4"},

        {"leftover sleep &", "sleep 100 &", 0, 0, 1.5},
        {"leftover sh -c &", "sh -c 'sleep 100 &'", 0, 0, 1.5},
        {"leftover (sleep &)", "(sleep 100 &)", 0, 0, 1.5},
        {"leftover busy level", "{ while :; do :; done; } &", 0, 0, 1},
        {"leftover procsub", ": > >(while :; do sleep 0.01; done)", 0, 0, 2.5},
        {"100 programs left", "for i in $(seq 100); do sleep 1000 & done; true; echo done",
         0, "done", 20},
        {"65 reads at once",
         "for i in $(seq 65); do { sleep 1; echo $i; } | { read x; echo r$x; } & done;"
         " wait", 0, "r65", 20},

        {"hard: busy loop", "while :; do :; done", -UB_ECANCELED, 0, 0.35, 100},
        {"hard: arith loop", "while ((1)); do ((1)); done", -UB_ECANCELED, 0, 0.35, 100},
        {"hard: sleep", "sleep 100", -UB_ECANCELED, 0, 0.6, 100},
        {"hard: read", "read x < <(sleep 100)", -UB_ECANCELED, 0, 0.6, 100},
        {"hard: pipeline", "yes | cat >/dev/null", -UB_ECANCELED, 0, 0.6, 100},
        {"hard: read a host pipe", "read x", -UB_ECANCELED, 0, 0.35, 100,
         0, 0, 0, 0, 0, 1},
        {"hard: trapped loop", "trap 'echo t' TERM; while :; do :; done",
         -UB_ECANCELED, 0, 0.35, 100},
        {"soft: busy loop", "while :; do :; done", UB_SIGNALED|(128+15), 0, 1, 100,
         15|UB_GROUP},
        {"soft: sleep", "sleep 100", UB_SIGNALED|(128+15), 0, 1, 100, 15|UB_GROUP},
        {"soft: pipeline", "sleep 100 | sleep 100 | cat", UB_SIGNALED|(128+15), 0, 1,
         100, 15|UB_GROUP},
        {"soft: trapped subshell", "( trap 'echo t' TERM; sleep 100 ); sleep 100",
         UB_SIGNALED|(128+15), "t", 1.5, 100, 15|UB_GROUP},
        {"soft: comsub keeps output",
         "trap 'echo trapped' TERM; x=$(sleep 0.5; echo hi); echo x=$x",
         0, "trapped\nx=hi", 2, 100, 15},

        {"out of memory", "x=a; while :; do x=$x$x; done", -UB_ENOMEM, "out of memory", 0, 0,
         0, 32<<20},
        {"descriptor budget",
         "for i in $(seq 40); do exec {fd}</dev/null || break; done; echo n=$i",
         0, "n=", 0, 0, 0, 0, 24},
    };
    int fails = 0;
    for (int i = 0; i < (int)(sizeof(cases)/sizeof(*cases)); i++) {
        Case const *c = cases + i;
        fails += !checkcase(c, c->stopms || c->maxsec>=1 ? 1 : reps);
    }

    // An out-of-memory sweep: aborts land everywhere, and the host is
    // as it was
    Case sweep = {"memlimit sweep",
        "for i in 1 2 3; do x=$(seq 2000 | sort -n | tail -1); y=$(cat <<E\n$x$x\nE\n);"
        " : $(echo $y) | cat > >(cat >/dev/null); z=$z$y$x; done; w=a;"
        " while :; do w=$w$w; done",
        -UB_ENOMEM};
    for (ptrdiff_t m = 2<<20; m <= 24<<20; m += quick ? 4<<20 : 1<<20) {
        sweep.memlimit = m;
        static char name[64];
        Snap s = {name, 0, sizeof(name)};
        sput(&s, "memlimit ");
        sputd(&s, m>>10);
        sput(&s, " KiB");
        sweep.name = name;
        fails += !checkcase(&sweep, 1);
    }
    Case again = {"a run after the sweep", "echo ok", 0, "ok"};
    fails += !checkcase(&again, 1);

    // 32-bit: four runs fit in 2 GiB of address space, beside the host's
    fails += !checkconcurrent(sizeof(void *)==8 ? 8 : 4, quick ? 5 : 20);

    int avail = 0;
    if (!PeekNamedPipe(sentr, 0, 0, 0, &avail, 0) || avail) {
        sput(&report, "FAIL host pipe was written or closed\n");
        line(&report);
        fails++;
    }
    unsigned evflags = 0;
    if (!GetHandleInformation(sentev, &evflags) || WaitForSingleObject(sentev, 0)) {
        sput(&report, "FAIL host event was closed or reset\n");
        line(&report);
        fails++;
    }
    DeleteFileW(outpath);
    sputd(&report, fails);
    sput(&report, " failed\n");
    line(&report);
    return fails != 0;
}
