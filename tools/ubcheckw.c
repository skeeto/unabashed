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
W32(int,       TerminateProcess(uintptr_t, unsigned));
W32(uintptr_t, OpenThread(int, int, int));
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
W32(int,       ResetEvent(uintptr_t));
W32(int,       CreateDirectoryW(uint16_t const *, void *));
W32(int,       RemoveDirectoryW(uint16_t const *));
W32(int,       SetEnvironmentVariableW(uint16_t const *, uint16_t const *));

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
    char tmp[3*600 + 1];
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

// Whether an address lies in a module's image (0: the executable's)
static int inmodule(uint16_t const *name, uintptr_t a)
{
    unsigned char *base = (unsigned char *)GetModuleHandleW(name);
    if (!base) return 0;
    int pe;
    unsigned size;
    memcpy(&pe, base+0x3c, 4);
    memcpy(&size, base+pe+24+56, 4);  // OptionalHeader.SizeOfImage
    return a-(uintptr_t)base < size;
}

static int inntdll(uintptr_t a)
{
    return inmodule(u"ntdll.dll", a);
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
    uintptr_t starts[32];  // the counted threads' start addresses
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
    // Images and views by identity: each allocation whole, since its
    // pages split as copy-on-write data is first written (the check's
    // own statics, too)
    MemInfo m;
    uintptr_t mbase = 0, mlen = 0, mtype = 0;
    for (uintptr_t a = 0;; ) {
        int ok = VirtualQuery((void *)a, &m, sizeof(m)) == sizeof(m);
        int mapped = ok && m.state!=0x10000 && m.type!=0x20000;  // not free or private
        if (mlen && (!mapped || m.allocbase!=mbase)) {
            sput(s, "map ");
            sputx(s, mbase);
            sput(s, " ");
            sputx(s, mlen);
            sput(s, " ");
            sputx(s, mtype);
            sput(s, "\n");
            mlen = 0;
        }
        if (!ok) break;
        if (m.state!=0x10000 && m.type==0x20000) {  // heaps, stacks, the run's
            st->private_ += m.size;
        } else if (mapped) {
            if (!mlen) {
                mbase = m.allocbase;
                mtype = m.type;
            }
            mlen = m.base + m.size - mbase;
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
    typedef int (__stdcall *Qt)(uintptr_t, int, void *, int, int *);
    static Qt qt;
    if (!qt) qt = (Qt)GetProcAddress(GetModuleHandleW(u"ntdll.dll"),
                                     "NtQueryInformationThread");
    for (int ok = Thread32First(snap, &te); ok; ok = Thread32Next(snap, &te)) {
        if ((int)te.pid != self) continue;
        // The library's start in the executable. Others come and go
        // with the load, sometimes seconds later: the thread pool's
        // (ntdll) and, under emulation, the translator's (xtajit)
        uintptr_t t = OpenThread(0x40, 0, (int)te.tid);  // QUERY_INFORMATION
        uintptr_t start = 0;
        if (t && qt && !qt(t, 9, &start, sizeof(start), 0) && !inmodule(0, start)) {
            CloseHandle(t);
            continue;
        }
        if (t) CloseHandle(t);
        if (st->threads < 32) st->starts[st->threads] = start;
        st->threads++;
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
static int poolhandle(char const *p, ptrdiff_t n, int gone)
{
    // The library makes events, but none of the others itself: a wait it
    // leaked would add packets, which the whole check counts (packets)
    static char const *const types[] = {
        " Thread ntdll", " WaitCompletionPacket", " IRTimer",
        " TpWorkerFactory", " IoCompletion", " Event",
    };
    int ntypes = (int)(sizeof(types)/sizeof(*types)) - !gone;
    if (n<7 || memcmp(p, "handle ", 7)) return 0;
    for (int i = 0; i < ntypes; i++) {
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
        if (!hasline(in, p, e-p) && !poolhandle(p, e-p, gone) && !lockevent(p, e-p) && !protected_(p, e-p)) {
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
    // More threads started in the executable: a run left one
    if (b->threads > a->threads) {
        sput(why, "threads ");
        sputd(why, a->threads);
        sput(why, " -> ");
        sputd(why, b->threads);
        // Where each new one started: its module and offset
        for (int i = 0; i<b->threads && i<32; i++) {
            int old = 0;
            for (int j = 0; j<a->threads && j<32; j++) old |= a->starts[j]==b->starts[i];
            if (old) continue;
            MemInfo mi = {0};
            VirtualQuery((void *)b->starts[i], &mi, sizeof(mi));
            static uint16_t mod[512];
            int len = mi.allocbase ? GetModuleFileNameW(mi.allocbase, mod, 512) : 0;
            int k = len;
            while (k>0 && mod[k-1]!='\\') k--;
            sput(why, " (");
            sputw(why, mod+k, len-k);
            sput(why, "+");
            sputx(why, b->starts[i]-mi.allocbase);
            sput(why, ")");
        }
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


// The thread pool's wait packets: it keeps a few more as load peaks.
static int packets(State *st)
{
    int n = 0;
    for (char *p = st->text.buf; (p = strstr_(p, " WaitCompletionPacket")); p++) n++;
    return n;
}

// Programs: every descendant of the host's, started after it, is gone

static uint64_t hostborn;

// A process by pid, to wait on and time: XP lacks the limited right.
static uintptr_t openproc(int pid)
{
    uintptr_t h = OpenProcess(0x101000, 0, pid);  // SYNCHRONIZE|QUERY_LIMITED
    return h ? h : OpenProcess(0x100400, 0, pid);  // SYNCHRONIZE|QUERY
}

static uint64_t bornat(int pid)
{
    uint64_t c = 0, e, k, u;
    uintptr_t h = openproc(pid);
    if (!h) return 0;
    if (!GetProcessTimes(h, &c, &e, &k, &u)) c = 0;
    CloseHandle(h);
    return c;
}

// Every descendant seen while a case runs, sampled by a watcher thread:
// a leftover whose parent exited is out of the tree kidsgone walks.
typedef struct {
    int      pid;
    uint64_t born;
} Seen;

static Seen      seen[4096];
static int       nseen;
static uintptr_t watchstop;  // an event: the case has run

static int seenat(int pid)
{
    for (int i = 0; i < nseen; i++) {
        if (seen[i].pid == pid) return i;
    }
    return -1;
}

static void sample(void)
{
    static ProcEntry all[8192];
    int n = 0;
    uintptr_t snap = CreateToolhelp32Snapshot(2, 0);  // TH32CS_SNAPPROCESS
    all[0].size = sizeof(*all);
    for (int ok = Process32FirstW(snap, all); ok && n<8191;) {
        n++;
        all[n].size = sizeof(*all);
        ok = Process32NextW(snap, all+n);
    }
    CloseHandle(snap);
    int self = GetCurrentProcessId();
    for (int grew = 1; grew && nseen<4096;) {
        grew = 0;
        for (int i = 0; i<n && nseen<4096; i++) {
            int pid = (int)all[i].pid, ppid = (int)all[i].ppid;
            if (pid==self || seenat(pid)>=0) continue;
            int parent = seenat(ppid);
            if (ppid!=self && parent<0) continue;
            uint64_t born = bornat(pid);
            if (!born || born<hostborn) continue;  // gone, or the pid's earlier owner
            if (parent>=0 && born<seen[parent].born) continue;  // not that one's child
            seen[nseen].pid  = pid;
            seen[nseen].born = born;
            nseen++;
            grew = 1;
        }
    }
}

static unsigned __stdcall watcher(void *arg)
{
    (void)arg;
    do sample(); while (WaitForSingleObject(watchstop, 2));
    sample();
    return 0;
}

static uintptr_t watch(void)
{
    nseen = 0;
    if (!watchstop) watchstop = CreateEventW(0, 1, 0, 0);
    ResetEvent(watchstop);
    return CreateThread(0, 1<<16, watcher, 0, 0, 0);
}

// Stop watching, then every program seen must have ended (within a
// second, as teardown waits for them).
static int seenended(uintptr_t w, Snap *why)
{
    SetEvent(watchstop);
    WaitForSingleObject(w, -1);
    CloseHandle(w);
    for (int i = 0; i < nseen; i++) {
        uintptr_t h = openproc(seen[i].pid);
        if (!h) continue;  // gone
        uint64_t c = 0, e, k, u;
        int same = GetProcessTimes(h, &c, &e, &k, &u) && c==seen[i].born;
        int live = same && WaitForSingleObject(h, 1000);
        CloseHandle(h);
        if (live) {
            sput(why, "a program outlived the run: pid ");
            sputd(why, seen[i].pid);
            return 0;
        }
    }
    return 1;
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

// The hooks a case installs (Case.hooks), each logging its calls to
// static storage, since snapshots compare the process's memory
enum { H_EXIT = 1<<0, H_ACCESS = 1<<1, H_SPAWN = 1<<2, H_TRACE = 1<<3,
       H_ALL = H_EXIT|H_ACCESS|H_SPAWN|H_TRACE };

static char hooklogbuf[1<<16];
static Snap hooklog = {hooklogbuf, 0, sizeof(hooklogbuf)};
static int  nexits;      // the exit hook's calls in a case's run
static int  exitstatus;  // and the status it was last given

static int hasprefix(char const *s, char const *p)
{
    for (; *p; s++, p++) {
        if (*s != *p) return 0;
    }
    return 1;
}

// How many times the hooks' log holds s
static int logcount(char const *s)
{
    int n = 0;
    for (char const *p = hooklog.buf; (p = strstr_(p, s)); p++) n++;
    return n;
}

// The state the exit hook was last given, to start a second run from
static char        exitcwd[4096];
static char        exitenvbuf[1<<16];
static char const *exitenv[1024];

static void onexit(void *user, ub_exit const *x)
{
    (void)user;
    nexits++;
    exitstatus = x->status;
    sput(&hooklog, "EXIT ");
    sputd(&hooklog, x->status);
    sput(&hooklog, " pwd=");
    sput(&hooklog, x->pwd ? x->pwd : "-");
    sput(&hooklog, " cwd=");
    sput(&hooklog, x->cwd ? x->cwd : "-");
    sput(&hooklog, "\n");
    size_t cwdlen = x->cwd ? strlen(x->cwd) : 0;
    cwdlen = cwdlen<sizeof(exitcwd) ? cwdlen : 0;
    memcpy(exitcwd, x->cwd ? x->cwd : "", cwdlen);
    exitcwd[cwdlen] = 0;
    size_t len = 0;
    int    n   = 0;
    for (char const *const *e = x->env; *e; e++) {
        if (hasprefix(*e, "UBX_") || hasprefix(*e, "PWD=") || hasprefix(*e, "BASH_FUNC_")) {
            sput(&hooklog, "env ");
            sput(&hooklog, *e);
            sput(&hooklog, "\n");
        }
        size_t elen = strlen(*e) + 1;
        if (n<(int)(sizeof(exitenv)/sizeof(*exitenv))-1 && elen<=sizeof(exitenvbuf)-len) {
            exitenv[n++] = memcpy(exitenvbuf+len, *e, elen);
            len += elen;
        } else {
            sput(&hooklog, "env: truncated\n");
        }
    }
    exitenv[n] = 0;
    if (!x->delta) sput(&hooklog, "delta unknown\n");
    for (char const *const *e = x->delta; e && *e; e++) {
        sput(&hooklog, "delta ");
        sput(&hooklog, *e);
        sput(&hooklog, "\n");
    }
}

// The cases' directory, $T in their environment, and the temporary
// directory, with forward slashes, as the access hook's log shows them
static char     tdir[1024];
static char     tmpdir[1024];
static uint16_t twide[300];

// $T/name, for the system
static uint16_t *tfile(uint16_t *buf, char const *name)
{
    int n = 0;
    for (; twide[n]; n++) buf[n] = twide[n];
    buf[n++] = '/';
    for (; *name; name++) buf[n++] = (uint16_t)*name;
    buf[n] = 0;
    return buf;
}

static struct Case const *curcase;  // whose access hook rules apply
static int  ndenied;                // the refusals in its run
static char reasonbuf[1024];

typedef struct Case {
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
    char const *opt;        // an option before -c, or null
    char const *notout;     // a substring the output must not hold, or null
    int         hooks;      // H_*
    char const *log;        // a substring the hooks' log must hold, or null
    char const *log2;       // and another, or null
    int         logn;       // >0: the first, so many times
    char const *nolog;      // a substring the log must not hold, or null
    char const *rerun;      // then this script, from the state the exit
                            // hook was given: its status and output count
    char const *deny;       // a hook refuses an access or a program whose
                            // log line holds this ("OPEN wct $T/w"), or null
    int         denyret;    // returning this; 0: -UB_EACCES
    char const *reason;     // with this reason (%d: the refusal's count)
    int         wrap;       // the spawn hook wraps each program in
                            // "ubhost --wrap" (2: by a relative path)
    int         linger;     // ub_config.linger, with softsig logging
                            // "SIG pid sig" and ending the program
} Case;

// A signal to a program, as a host would deliver it: logged, and the
// program ended with the signal's status
static int onsoftsig(void *user, int64_t pid, int sig)
{
    (void)user;
    sput(&hooklog, "SIG ");
    sputd(&hooklog, sig);
    sput(&hooklog, "\n");
    uintptr_t h = OpenProcess(1, 0, (int)pid);  // PROCESS_TERMINATE
    if (!h) return -1;
    TerminateProcess(h, 128u + (unsigned)sig);
    CloseHandle(h);
    return 0;
}

// An access as a log line, "OP flags path", T and the temporary
// directory shown as $T and $TMP; refused if the case's rule says so
static int onaccess(void *user, ub_access const *a, char const **reason)
{
    (void)user;
    static char const *const ops[] = {"?", "OPEN", "STAT", "READLINK", "LIST", "TEMPFILE"};
    static char linebuf[8192];  // not on the run's stack
    Snap line = {linebuf, 0, sizeof(linebuf)};
    sput(&line, a->op>0 && a->op<6 ? ops[a->op] : "?");
    char fl[9], *p = fl;
    *p++ = ' ';
    for (int i = 0; i < 7; i++) {
        if (a->flags & 1<<i) *p++ = "rwctaxn"[i];
    }
    if (p == fl+1) *p++ = '-';
    *p++ = ' ';
    sputn(&line, fl, p-fl);
    if (hasprefix(a->path, tdir)) {
        sput(&line, "$T");
        sput(&line, a->path+strlen(tdir));
    } else if (hasprefix(a->path, tmpdir)) {
        sput(&line, "$TMP");
        sput(&line, a->path+strlen(tmpdir));
    } else {
        sput(&line, a->path);
    }
    sput(&line, "\n");
    sput(&hooklog, linebuf);
    Case const *c = curcase;
    if (!c->deny || !strstr_(linebuf, c->deny)) return 0;
    ndenied++;
    if (c->reason) {
        Snap r = {reasonbuf, 0, sizeof(reasonbuf)};
        char const *d = strstr_(c->reason, "%d");
        if (d) {
            sputn(&r, c->reason, d-c->reason);
            sputd(&r, ndenied);
            sput(&r, d+2);
        } else {
            sput(&r, c->reason);
        }
        *reason = reasonbuf;
    }
    return c->denyret ? c->denyret : -UB_EACCES;
}

// A simple command as log lines: "S<id> b|f|p argv" as it starts, with
// "+" after the id if in a subshell (a level other than $$, TRACEPID),
// T shown as $T; "E<id> status" as it ends, with " p" if a program's
enum { TRACEPID = 4242 };

static void ontrace(void *user, ub_trace const *t)
{
    (void)user;
    static char linebuf[8192];  // not on the run's stack
    Snap line = {linebuf, 0, sizeof(linebuf)};
    if (t->phase == UB_TRACE_END) {
        sput(&line, "E");
        sputd(&line, t->id);
        sput(&line, " ");
        sputd(&line, t->status);
        sput(&line, t->pid ? " p\n" : "\n");
        sput(&hooklog, linebuf);
        return;
    }
    static char const *const kinds[] = {" ?", " b", " f", " p"};
    sput(&line, "S");
    sputd(&line, t->id);
    sput(&line, t->level==TRACEPID ? "" : "+");
    sput(&line, kinds[t->kind>0 && t->kind<4 ? t->kind : 0]);
    for (int i = 0; i < t->argc; i++) {
        char const *a = t->argv[i];
        sput(&line, " ");
        if (tdir[0] && hasprefix(a, tdir)) {
            sput(&line, "$T");
            for (char const *p = tdir; *p; p++) a++;
        }
        sput(&line, a);
    }
    sput(&line, "\n");
    sput(&hooklog, linebuf);
}

static char selfutf8[3*32768];  // this program (check)

// A program as a log line, "P<trace> name"; refused or wrapped if the
// case's rule says so
static int onspawn(void *user, ub_spawn *s)
{
    (void)user;
    char const *name = s->path;
    for (char const *p = s->path; *p; p++) {
        if (*p=='/' || *p=='\\') name = p + 1;
    }
    static char linebuf[4200];  // not on the run's stack
    Snap line = {linebuf, 0, sizeof(linebuf)};
    sput(&line, "P");
    sputd(&line, s->trace);
    sput(&line, " ");
    sput(&line, name);
    sput(&line, "\n");
    sput(&hooklog, linebuf);
    Case const *c = curcase;
    if (c->deny && strstr_(linebuf, c->deny)) {
        ndenied++;
        if (c->reason) s->reason = c->reason;
        return c->denyret ? c->denyret : -UB_EACCES;
    }
    static char const *wrapabs[] = {selfutf8, "--wrap", 0};
    static char const *wraprel[] = {"ubhost.exe", "--wrap", 0};
    if (c->wrap) s->wrap = c->wrap==2 ? wraprel : wrapabs;
    return 0;
}

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

// One run of a case: its status, output in out, and in left why a
// program it started outlived it (before ub_free, which ends the job).
static int runcase(Case const *c, char *out, int outlen, double *secs, Snap *left)
{
    ub_config cfg = {0};
    cfg.spawnlock = hostlock;
    cfg.memlimit  = c->memlimit;
    cfg.fds       = c->fds;
    cfg.linger    = c->linger;
    if (c->linger) cfg.softsig = onsoftsig;
    if (c->hooks & H_EXIT) cfg.exit = onexit;
    if (c->hooks & H_ACCESS) cfg.access = onaccess;
    if (c->hooks & H_SPAWN) cfg.spawn = onspawn;
    if (c->hooks & H_TRACE) cfg.trace = ontrace;
    curcase = c;
    ndenied = 0;
    hooklog.len    = 0;
    hooklog.buf[0] = 0;
    nexits         = 0;
    ub_os *os = ub_new(&cfg);
    if (!os) return -9999;
    // GENERIC_READ|WRITE, share all, CREATE_ALWAYS, not inheritable
    uintptr_t f = CreateFileW(outpath, 0xc0000000u, 7, 0, 2, 0, 0);
    char const *argv[] = {"bash", "-c", c->script, 0, 0};
    if (c->opt) {
        argv[1] = c->opt;
        argv[2] = "-c";
        argv[3] = c->script;
    }
    ub_cmd cmd = {0};
    cmd.argc   = c->opt ? 4 : 3;
    cmd.argv   = argv;
    uintptr_t pr = 0, pw = 0;
    if (c->hostpipe) CreatePipe(&pr, &pw, 0, 0);
    cmd.std[0] = c->hostpipe ? (intptr_t)pr : -1;
    cmd.std[1] = (intptr_t)f;
    cmd.std[2] = (intptr_t)f;
    cmd.pid    = c->pid ? c->pid : c->hooks & H_TRACE ? TRACEPID : 0;
    Stopper s = {os, c->stopms, c->stopsig, 0};
    uintptr_t t = 0;
    if (c->stopms) s.done = CreateEventW(0, 1, 0, 0);
    if (c->stopms) t = CreateThread(0, 1<<16, stopper, &s, 0, 0);
    void *fiber = c->fiber ? ConvertThreadToFiber((void *)0x5eed) : 0;
    uintptr_t w = watch();
    double t0 = nowsec();
    int r = ub_run(os, &cmd);
    if (c->rerun && r>=0 && nexits==1) {
        // A second run, as a host starts the next from the first's end
        char const *argv2[] = {"bash", "-c", c->rerun, 0};
        cmd.argc = 3;
        cmd.argv = argv2;
        cmd.cwd  = exitcwd[0] ? exitcwd : 0;
        cmd.env  = exitenv;
        r = ub_run(os, &cmd);
    }
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
    seenended(w, left);
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
static int   packets0;
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
    runcase(c, out, sizeof(out), &secs, &why);  // warm-up, which counts too
    int ok = !why.len;
    if (ok) {
        for (double until = nowsec()+2; !kidsgone(&why) && nowsec()<until;) Sleep(1);
        why.len = 0;
    }
    snapshot(&before);
    for (int i = 0; i<reps && ok; i++) {
        int r = runcase(c, out, sizeof(out), &secs, &why);
        if (why.len) {
            ok = 0;  // a program outlived the run
        } else if (c->status!=ANY && r!=c->status) {
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
        } else if (c->notout && strstr_(out, c->notout)) {
            sput(&why, "output holds \"");
            sput(&why, c->notout);
            sput(&why, "\"");
            ok = 0;
        } else if ((c->hooks & H_EXIT) && nexits!=(r<0 ? 0 : c->rerun ? 2 : 1)) {
            sput(&why, "exit hook called ");
            sputd(&why, nexits);
            sput(&why, " times");
            ok = 0;
        } else if ((c->hooks & H_EXIT) && r>=0 && exitstatus!=r) {
            sput(&why, "exit hook given ");
            sputd(&why, exitstatus);
            ok = 0;
        } else if (c->log && (c->logn ? logcount(c->log)!=c->logn : !logcount(c->log))) {
            sput(&why, "log holds \"");
            sput(&why, c->log);
            sput(&why, "\" ");
            sputd(&why, logcount(c->log));
            sput(&why, " times");
            ok = 0;
        } else if (c->log2 && !logcount(c->log2)) {
            sput(&why, "log lacks \"");
            sput(&why, c->log2);
            sput(&why, "\"");
            ok = 0;
        } else if (c->nolog && logcount(c->nolog)) {
            sput(&why, "log holds \"");
            sput(&why, c->nolog);
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
    // The system's thread pool retires its workers and their events on
    // its own schedule, late under load: a difference must last 2 s
    double settled = 0;
    if (ok) {
        double t0 = nowsec();
        for (;;) {
            snapshot(&after);
            why.len    = 0;
            why.buf[0] = 0;
            ok = samestate(&before, &after, &why);
            if (ok || nowsec()-t0>=2) break;
            Sleep(10);
        }
        settled = ok ? nowsec() - t0 : 0;
    }
    sput(&report, ok ? "PASS " : "FAIL ");
    sput(&report, c->name);
    sput(&report, " ");
    sputd(&report, (int64_t)(secs*1000));
    sput(&report, " ms");
    if (settled >= 0.05) {
        sput(&report, " (settled in ");
        sputd(&report, (int64_t)(settled*1000));
        sput(&report, " ms)");
    }
    if (!ok) {
        sput(&report, ": ");
        sput(&report, why.buf);
        if (out[0]) {
            sput(&report, "\n     output: ");
            sputn(&report, out, strlen(out)<300 ? (ptrdiff_t)strlen(out) : 300);
        }
        if (hooklog.len) {
            sput(&report, "\n     log: ");
            sputn(&report, hooklog.buf, hooklog.len<600 ? hooklog.len : 600);
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
    concround(threads, 1);  // its failures count too
    for (double until = nowsec()+2; !kidsgone(&why) && nowsec()<until;) Sleep(1);
    why.len = 0;
    snapshot(&before);
    uintptr_t w = watch();
    double t0 = nowsec();
    concround(threads, reps);
    double secs = nowsec() - t0;
    int ended = seenended(w, &why);
    int ok = !concfails && ended && kidsgone(&why);
    for (double t1 = nowsec(); ok;) {  // settled, as in checkcase
        snapshot(&after);
        Snap diff = why;
        if (samestate(&before, &after, &diff)) break;
        if (nowsec()-t1 >= 2) {
            why = diff;
            ok  = 0;
        }
        Sleep(10);
    }
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
        ptrdiff_t n = 0;
        for (; selfpath[n]; n++) {}
        wtf8(selfutf8, selfpath, n);
    }
    {
        uint64_t e, k, u;
        GetProcessTimes(GetCurrentProcess(), &hostborn, &e, &k, &u);
    }
    int n = GetTempPathW(260, outpath);
    static uint16_t const name[] = u"ubcheck.out";
    for (int i = 0; i < (int)(sizeof(name)/2); i++) outpath[n+i] = name[i];

    // The cases' directory, $T, with a file to read, a script to source
    // and a directory, all with forward slashes
    for (int i = 0; i < n; i++) twide[i] = outpath[i]=='\\' ? '/' : outpath[i];
    wtf8(tmpdir, twide, n-1);  // less its trailing slash
    static uint16_t const tname[] = u"ubcheck.t";
    for (int i = 0; i < (int)(sizeof(tname)/2); i++) twide[n+i] = tname[i];
    wtf8(tdir, twide, n+(int)(sizeof(tname)/2)-1);
    CreateDirectoryW(twide, 0);
    uint16_t tbuf[300];
    CreateDirectoryW(tfile(tbuf, "d"), 0);
    static char const *const tfiles[][2] = {
        {"f", "data\n"}, {"s", "echo sourced\n"}, {"sh1", "#!/bin/sh\necho sh1\n"},
        {"b.bat", "@echo bat\r\n"},
    };
    for (int i = 0; i < 4; i++) {
        // GENERIC_WRITE, CREATE_ALWAYS
        uintptr_t h = CreateFileW(tfile(tbuf, tfiles[i][0]), 0x40000000u, 0, 0, 2, 0, 0);
        int w = 0;
        WriteFile(h, tfiles[i][1], (int)strlen(tfiles[i][1]), &w, 0);
        CloseHandle(h);
    }
    SetEnvironmentVariableW(u"T", twide);

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
        {"-n: syntax error", "echo ok; if", 2,
         "syntax error: unexpected end of file", .opt = "-n", .notout = "ok\n"},
        {"-n: unexpected token", "case x in esac; echo )", 2,
         "syntax error near unexpected token `)'", .opt = "-n"},
        {"-n: no errors", "echo hi; exit 3", 0, .opt = "-n", .notout = "hi"},
        {"-n: open here-document", "cat <<E\nx", 0,
         "here-document at line 1 delimited by end-of-file", .opt = "-n", .notout = "x\n"},
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
        // past WaitForMultipleObjects' 64, so with room for all 65 jobs'
        // descriptors at once (each holds about 6)
        {"65 reads at once",
         "for i in $(seq 65); do { sleep 1; echo $i; } | { read x; echo r$x; } & done;"
         " wait", 0, "r65", 20, .fds = 512},

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
        {"soft: trapped subshell",  // the trap's marker, not in "Terminated"
         "( trap 'echo T$((6*7))' TERM; sleep 100 ); sleep 100",
         UB_SIGNALED|(128+15), "T42", 1.5, 100, 15|UB_GROUP},
        {"soft: comsub keeps output",
         "trap 'echo trapped' TERM; x=$(sleep 0.5; echo hi); echo x=$x",
         0, "trapped\nx=hi", 2, 100, 15},

        {"out of memory", "x=a; while :; do x=$x$x; done", -UB_ENOMEM, "out of memory", 0, 0,
         0, 32<<20},
        {"descriptor budget",
         "for i in $(seq 40); do exec {fd}</dev/null || break; done;"
         " ((i<24)) && echo capped", 0, "capped", 0, 0, 0, 0, 24},

        // Paths: no root, so / is the working directory's drive
        {"exit hook", "cd /; export UBX_A=1; UBX_B=2; exit 3", 3, .hooks = H_EXIT,
         .log = "EXIT 3 pwd=/ cwd=", .log2 = "env UBX_A=1\n", .nolog = "UBX_B"},
        {"exit hook: function", "f() { cd /Windows; exit 4; }; f; echo no", 4,
         .notout = "no", .hooks = H_EXIT, .log = "EXIT 4 pwd=/Windows cwd=",
         .log2 = ":/Windows\n"},
        {"exit hook: set -e", "set -e; false; echo no", 1, .notout = "no", .hooks = H_EXIT,
         .log = "EXIT 1 "},
        {"exit hook: exit 300", "exit 300", 44, .hooks = H_EXIT, .log = "EXIT 44 "},
        {"exit hook: exec", "cd /; exec true; echo no", 0, .notout = "no", .hooks = H_EXIT,
         .log = "EXIT 0 pwd=/ "},
        {"exit hook: signal", "kill -TERM $$", UB_SIGNALED|(128+15), .hooks = H_EXIT},
        {"exit hook: EXIT trap", "trap 'cd /Windows; export UBX_T=t' EXIT; cd /", 0,
         .hooks = H_EXIT, .log = "EXIT 0 pwd=/Windows cwd=", .log2 = "env UBX_T=t\n"},
        // its value converted, as a native program gets it
        {"exit hook: PWD reassigned", "cd /Windows; PWD=/nowhere", 0, .hooks = H_EXIT,
         .log = "EXIT 0 pwd=/Windows cwd=", .log2 = ":/nowhere\n"},
        {"exit hook: export -f", "f() { echo; }; export -f f", 0, .hooks = H_EXIT,
         .log = "env BASH_FUNC_f%%=() {  echo\n}\n"},
        {"exit hook: background", "cd /; sleep 100 & exit 5", 5, 0, 1, .hooks = H_EXIT,
         .log = "EXIT 5 pwd=/ "},
        {"exit hook: cancel", "cd /; while :; do :; done", -UB_ECANCELED, 0, 0.5, 100,
         .hooks = H_EXIT},
        {"exit hook: out of memory", "x=a; while :; do x=$x$x; done", -UB_ENOMEM,
         "out of memory", 0, 0, 0, 32<<20, .hooks = H_EXIT},
        // The delta: none for HOME, PATH or the like, which the library
        // rewrites before the command starts, even from a run's own end
        {"exit hook: delta", "export UBX_N=n; f() { :; }; export -f f; unset T", 0,
         .hooks = H_EXIT, .log = "delta BASH_FUNC_f%%=() {  :\n}\ndelta UBX_N=n\ndelta T\n"},
        {"exit hook: no delta", ":", 0, .hooks = H_EXIT, .nolog = "delta ", .rerun = ":"},
        // linger: SIGTERM through softsig, then time until the kill;
        // the wait ends once the programs have
        {"linger: softsig first", "sleep 100 & sleep 0.2", 0, 0, 2, .linger = 5000,
         .log = "SIG 15\n"},
        {"exit hook: next run",

 "cd /Windows; export UBX_RT=rt; UBX_LOCAL=x", 0,
         "/Windows\nrt\nunset\n", .hooks = H_EXIT,
         .rerun = "pwd; echo $UBX_RT; echo ${UBX_LOCAL-unset}"},

        // Paths: native, with forward slashes, devices as //./NUL
        {"access: >", "echo a > $T/w", 0, .hooks = H_ACCESS, .log = "OPEN wct $T/w\n",
         .logn = 1},
        {"access: >>", "echo a >> $T/w", 0, .hooks = H_ACCESS, .log = "OPEN wca $T/w\n"},
        {"access: <", "read x < $T/f; echo $x", 0, "data", .hooks = H_ACCESS,
         .log = "OPEN r $T/f\n"},
        {"access: /dev/null", "echo a > /dev/null", 0, .hooks = H_ACCESS,
         .log = "OPEN wct //./NUL\n"},
        {"access: test", "test -e $T/f; true", 0, .hooks = H_ACCESS,
         .log = "STAT - $T/f\n"},
        {"access: cd", "cd $T/d && echo ok", 0, "ok", .hooks = H_ACCESS,
         .log = "STAT - $T/d\n"},
        {"access: source", "cd $T; source ./s", 0, "sourced", .hooks = H_ACCESS,
         .log = "OPEN r $T/s\n"},
        {"access: glob", "cd $T; echo *", 0, "d f s", .hooks = H_ACCESS,
         .log = "LIST r $T"},
        {"access: here-document file", "x=$(printf %070000d 0); cat <<E | wc -c\n$x\nE", 0,
         "70001", .hooks = H_ACCESS, .log = "TEMPFILE rwcx $TMP"},
        {"access: cd /proc", "cd /proc && echo ok", 0, "ok", .hooks = H_ACCESS,
         .nolog = "proc"},
        {"access: not a program's head", "echo hi | cat", 0, "hi", .hooks = H_ACCESS,
         .nolog = "OPEN r "},
        {"access: not /dev/null", "sleep 0 & wait", 0, .hooks = H_ACCESS,
         .nolog = "NUL"},
        {"deny: redirection", "echo a > $T/w; echo s=$?", 0, "w: no way\ns=1",
         .hooks = H_ACCESS, .deny = "OPEN wct $T/w", .reason = "no way"},
        {"deny: ENOENT", "echo a > $T/w", 1, "w: No such file or directory",
         .hooks = H_ACCESS, .deny = "OPEN wct $T/w", .denyret = -UB_ENOENT},
        {"deny: cd", "cd $T/d; echo s=$?", 0, "d: no\ns=1", .hooks = H_ACCESS,
         .deny = "STAT - $T/d", .reason = "no"},
        {"deny: test", "test -e $T/f && echo yes || echo no", 0, "no", .notout = "yes",
         .hooks = H_ACCESS, .deny = "STAT - $T/f", .reason = "no"},
        {"deny: glob", "cd $T; echo *", 0, "*\n", .hooks = H_ACCESS, .deny = "LIST",
         .reason = "no"},
        {"deny: PATH search", "ls; echo s=$?", 0, "ls: command not found\ns=127",
         .hooks = H_ACCESS, .deny = "/ls", .reason = "no"},
        {"deny: here-document file",
         "x=$(printf %070000d 0); cat <<E | wc -c\n$x\nE", ANY,
         "cannot create temp file for here-document: no", .hooks = H_ACCESS,
         .deny = "TEMPFILE", .reason = "no"},
        {"deny: 40 reasons", "for i in {1..40}; do echo > $T/w$i; done", 1,
         "w40: denied 40\n", .hooks = H_ACCESS, .deny = "OPEN wct $T/w",
         .reason = "denied %d"},
        // msys mode: //c, not /c, which becomes C:/
        {"spawn: wrapped", "cmd //c echo hi", 0, "wrapped ", .hooks = H_SPAWN,
         .log = "P0 cmd", .wrap = 1},
        {"spawn: wrapped output", "cmd //c echo hi", 0, "hi", .hooks = H_SPAWN, .wrap = 1},
        {"spawn: wrapped status", "cmd //c exit 7; echo s=$?", 0, "s=7", .hooks = H_SPAWN,
         .wrap = 1},
        {"spawn: wrapped stubs", "echo a | tr a b | cat", 0, "b", .hooks = H_SPAWN,
         .log = "P0 tr.exe\n", .wrap = 1},
        // native mode runs a busybox alias stub as busybox.exe
        {"spawn: wrapped stub, native", "shopt -u msyspath; echo a | tr a b", 0, "b",
         .hooks = H_SPAWN, .log = "P0 busybox.exe\n", .wrap = 1},
        {"spawn: wrapped &", "sleep 0 & wait $!; echo s=$?", 0, "s=0", .hooks = H_SPAWN,
         .wrap = 1},
        {"spawn: wrapped batch file", "$T/b.bat", 0, "bat", .hooks = H_SPAWN,
         .log = "P0 cmd", .wrap = 1},
        {"spawn: #! script in-process", "$T/sh1", 0, "sh1", .notout = "wrapped",
         .hooks = H_SPAWN, .nolog = "P0", .wrap = 1},
        {"spawn: relative wrapper", "cmd //c echo hi; echo s=$?", 0,
         "Invalid argument\ns=126", .hooks = H_SPAWN, .wrap = 2},
        {"spawn refused: reason", "cmd //c echo hi; echo s=$?", 0, ": not allowed\ns=126",
         .hooks = H_SPAWN, .deny = "P0 cmd", .reason = "not allowed"},
        {"spawn refused: ENOENT", "cmd //c echo hi; echo s=$?", 0,
         ": No such file or directory\ns=127", .hooks = H_SPAWN, .deny = "P0 cmd",
         .denyret = -UB_ENOENT},
        {"spawn refused: PIPESTATUS", "cmd //c echo hi | cat; echo ${PIPESTATUS[@]}", 0,
         "126 0", .hooks = H_SPAWN, .deny = "P0 cmd", .reason = "no"},
        {"spawn refused: exec", "exec cmd //c echo hi; echo after", 126, ": no",
         .notout = "after", .hooks = H_SPAWN, .deny = "P0 cmd", .reason = "no"},
        {"trace: builtin", "echo hi", 0, "hi", .hooks = H_TRACE, .log = "S1 b echo hi\nE1 0\n"},
        {"trace: function", "f() { true; false; }; f", 1, .hooks = H_TRACE,
         .log = "S1 f f\nS2 b true\nE2 0\nS3 b false\nE3 1\nE1 1\n"},
        {"trace: program", "cmd //c exit 3", 3, .hooks = H_TRACE,
         .log = "S1 p cmd //c exit 3\nE1 3 p\n"},
        {"trace: pipeline", "echo a | tr a b | cat", 0, "b", .hooks = H_TRACE,
         .log = "+ p tr a b\n", .log2 = "+ p cat\n"},
        {"trace: exit in a function", "f() { exit 3; }; f", 3, .hooks = H_TRACE,
         .log = "S1 f f\nS2 b exit 3\n", .nolog = "E"},
        {"trace: killed at the end", "sleep 100 & exit 4", 4, .hooks = H_TRACE|H_EXIT,
         .log = "S1+ p sleep 100\n", .log2 = "E1 393 p\n"},
        {"trace: spawn's id", "cmd //c echo hi", 0, "hi", .hooks = H_TRACE|H_SPAWN,
         .log = "S1 p cmd //c echo hi\nP1 cmd"},
        {"trace: refused stage", "cmd //c echo hi | cat", 0, .hooks = H_TRACE|H_SPAWN,
         .deny = "P1 cmd", .reason = "no", .log = "S1+ p cmd //c echo hi\nP1 cmd", .log2 = "E1 126\n"},
        {"trace: #! script in-process", "$T/sh1", 0, "sh1", .hooks = H_TRACE,
         .log = "S1 p $T/sh1\n", .log2 = " b echo sh1\n"},
    };
    int fails = 0;
    for (int i = 0; i < (int)(sizeof(cases)/sizeof(*cases)); i++) {
        Case const *c = cases + i;
        fails += !checkcase(c, c->stopms || c->maxsec>=1 ? 1 : reps);
        if (!packets0) packets0 = packets(&after);  // once the pool exists
    }

    // An out-of-memory sweep: aborts land everywhere, and the host is
    // as it was
    Case sweep = {"memlimit sweep",
        "for i in 1 2 3; do x=$(seq 2000 | sort -n | tail -1); y=$(cat <<E\n$x$x\nE\n);"
        " : $(echo $y) | cat > >(cat >/dev/null); z=$z$y$x; done; w=a;"
        " while :; do w=$w$w; done",
        -UB_ENOMEM};
    for (int h = 0; h < 2; h++) {
        // and again with every hook, which aborts never call
        sweep.hooks = h ? H_ALL : 0;
        for (ptrdiff_t m = 2<<20; m <= 24<<20; m += quick ? 4<<20 : 1<<20) {
            sweep.memlimit = m;
            static char name[64];
            Snap s = {name, 0, sizeof(name)};
            sput(&s, "memlimit ");
            sputd(&s, m>>10);
            sput(&s, h ? " KiB, hooks" : " KiB");
            sweep.name = name;
            fails += !checkcase(&sweep, 1);
        }
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
    // Hundreds of runs: a leaked wait would leave a packet each
    snapshot(&after);
    if (packets(&after) > packets0+8) {
        sput(&report, "FAIL the pool's wait packets grew from ");
        sputd(&report, packets0);
        sput(&report, " to ");
        sputd(&report, packets(&after));
        sput(&report, "\n");
        line(&report);
        fails++;
    }
    DeleteFileW(outpath);
    static char const *const tleft[] = {"f", "s", "w", "sh1", "b.bat"};
    for (int i = 0; i < 5; i++) DeleteFileW(tfile(tbuf, tleft[i]));
    for (int i = 1; i <= 40; i++) {
        char name[8] = {'w', (char)(i<10 ? '0'+i : '0'+i/10), (char)(i<10 ? 0 : '0'+i%10), 0};
        DeleteFileW(tfile(tbuf, name));
    }
    RemoveDirectoryW(tfile(tbuf, "d"));
    RemoveDirectoryW(twide);
    sputd(&report, fails);
    sput(&report, " failed\n");
    line(&report);
    return fails != 0;
}
