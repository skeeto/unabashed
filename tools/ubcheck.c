// ubhost --check [--quick]: the library's host contract, checked
//
// Each case runs a few times after a warm-up run, between two snapshots
// of everything a run could leave changed in its host: mappings,
// descriptors, threads, signal dispositions and the thread's mask,
// process group and session, directory, umask, limits, environment,
// and the programs it started (each must be gone, with its group).
// A difference fails the case, as does a status, output or time other
// than the case expects. Included by ubhost.c.
// This is free and unencumbered software released into the public domain.
#include <dirent.h>
#include <poll.h>
#include <stdarg.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#if __linux__
#include <sys/prctl.h>
#endif
#if __APPLE__
#include <mach/mach.h>
#include <mach/mach_vm.h>
#endif

// Built with AddressSanitizer: its quarantine holds freed blocks back,
// so the allocator maps more with each case whatever allocates, and
// snapshots would differ. This host keeps none (ASAN_OPTIONS overrides).
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    define UBCHECK_ASAN 1
#  endif
#endif
#if __SANITIZE_ADDRESS__ || UBCHECK_ASAN
const char *__asan_default_options(void);
const char *__asan_default_options(void) { return "quarantine_size_mb=0"; }
#  define FORKSCALE 4  // macOS forks an ASan process in ~60 ms
#else
#  define FORKSCALE 1
#endif

typedef struct {
    char  *buf;
    size_t len, cap;
} Snap;

static void snapf(Snap *s, char const *fmt, ...)
{
    va_list ap;
    for (;;) {
        va_start(ap, fmt);
        int n = vsnprintf(s->buf+s->len, s->cap-s->len, fmt, ap);
        va_end(ap);
        if (n>=0 && (size_t)n < s->cap-s->len) {
            s->len += (size_t)n;
            return;
        }
        s->cap = s->cap ? s->cap*2 : 1<<16;
        s->buf = realloc(s->buf, s->cap);
    }
}

// The host's own spawn lock, shared with every run (ub_config's
// spawnlock): its own programs start under it.
static pthread_mutex_t hostlock = PTHREAD_MUTEX_INITIALIZER;

// A host pipe, close-on-exec before any program of the host's or of a
// run's could inherit it, as the contract asks
static int hostpipe(int p[2])
{
    #if __linux__
    return pipe2(p, O_CLOEXEC);
    #else
    pthread_mutex_lock(&hostlock);
    int r = pipe(p);
    if (!r) {
        fcntl(p[0], F_SETFD, FD_CLOEXEC);
        fcntl(p[1], F_SETFD, FD_CLOEXEC);
    }
    pthread_mutex_unlock(&hostlock);
    return r;
    #endif
}

static void spawnlock(void *user, int lock)
{
    (void)user;
    if (lock) pthread_mutex_lock(&hostlock);
    else pthread_mutex_unlock(&hostlock);
}

static int  nkids;
static long kids[1<<14];

static void onchild(void *user, int64_t pid, int live)
{
    (void)user;
    if (live && nkids < (int)(sizeof(kids)/sizeof(*kids))) kids[nkids++] = (long)pid;
}

static void snapshot(Snap *s)
{
    s->len = 0;
    #if __linux__
    FILE *f = fopen("/proc/self/maps", "r");
    char line[512];
    while (f && fgets(line, sizeof(line), f)) {
        if (!strstr(line, "[stack]")) snapf(s, "map %s", line);
    }
    if (f) fclose(f);
    DIR *d = opendir("/proc/self/task");
    int nthreads = 0;
    for (struct dirent *e; d && (e = readdir(d));) nthreads += e->d_name[0] != '.';
    if (d) closedir(d);
    snapf(s, "threads %d\n", nthreads);
    #elif __APPLE__
    // malloc carves its regions from untagged no-access reservations of
    // its own, growing once per size class whoever allocates (libSystem
    // inside posix_spawn): those are left out, and the total of every
    // region kept, which any mapping left behind changes. Under ASan its
    // allocator and shadow (VM_MEMORY_SANITIZER) grow the same way, and
    // are left out of both.
    mach_vm_address_t addr = 0;
    unsigned long long total = 0;
    for (;;) {
        mach_vm_size_t size  = 0;
        natural_t      depth = 0;
        vm_region_submap_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
        if (mach_vm_region_recurse(mach_task_self(), &addr, &size, &depth,
                                   (vm_region_recurse_info_t)&info, &count)) break;
        if (info.user_tag == VM_MEMORY_SANITIZER) {
            addr += size;
            continue;
        }
        total += size;
        int mallocs = info.user_tag>=VM_MEMORY_MALLOC &&
                      info.user_tag<=VM_MEMORY_MALLOC_PROB_GUARD;
        if (!mallocs && (info.user_tag || info.protection)) {
            snapf(s, "map %llx %llx %d %u\n", (unsigned long long)addr,
                  (unsigned long long)size, info.protection, info.user_tag);
        }
        addr += size;
    }
    snapf(s, "mapped %llx\n", total);
    thread_act_array_t threads;
    mach_msg_type_number_t nthreads = 0;
    if (!task_threads(mach_task_self(), &threads, &nthreads)) {
        for (mach_msg_type_number_t i = 0; i < nthreads; i++) {
            mach_port_deallocate(mach_task_self(), threads[i]);
        }
        vm_deallocate(mach_task_self(), (vm_address_t)threads,
                      nthreads*sizeof(*threads));
    }
    snapf(s, "threads %d\n", (int)nthreads);
    #endif
    for (int fd = 0; fd < 1024; fd++) {
        struct stat st;
        if (fstat(fd, &st)) continue;
        snapf(s, "fd %d %lld %lld %d %d\n", fd, (long long)st.st_dev,
              (long long)st.st_ino, fcntl(fd, F_GETFD), fcntl(fd, F_GETFL));
    }
    for (int sig = 1; sig < 32; sig++) {
        struct sigaction sa;
        if (sigaction(sig, 0, &sa)) continue;
        snapf(s, "sig %d %p %x\n", sig, (void *)sa.sa_handler, sa.sa_flags);
    }
    sigset_t mask;
    pthread_sigmask(SIG_SETMASK, 0, &mask);
    for (int sig = 1; sig < 32; sig++) {
        if (sigismember(&mask, sig)) snapf(s, "blocked %d\n", sig);
    }
    snapf(s, "pgrp %ld sid %ld\n", (long)getpgrp(), (long)getsid(0));
    char cwd[4096];
    snapf(s, "cwd %s\n", getcwd(cwd, sizeof(cwd)) ? cwd : "?");
    mode_t um = umask(022);
    umask(um);
    snapf(s, "umask %o\n", (int)um);
    for (int r = 0; r < RLIM_NLIMITS; r++) {
        struct rlimit rl;
        if (!getrlimit(r, &rl)) {
            snapf(s, "rlimit %d %llu %llu\n", r, (unsigned long long)rl.rlim_cur,
                  (unsigned long long)rl.rlim_max);
        }
    }
    extern char **environ;
    unsigned long long h = 1469598103934665603ull;
    for (char **e = environ; *e; e++) {
        for (char *c = *e; *c; c++) h = (h ^ (unsigned char)*c) * 1099511628211ull;
    }
    snapf(s, "environ %llx\n", h);
}

// Every program a run started is gone, and its group with it.
static int kidsgone(char *why, size_t len)
{
    #if __linux__
    // Orphans of programs, reparented here (a subreaper). Killed ones
    // are zombies, whose groups still answer kill(-pgid, 0): reap them.
    // A living one is an escape.
    DIR *d = opendir("/proc");
    for (struct dirent *e; d && (e = readdir(d));) {
        char path[300], buf[512];
        snprintf(path, sizeof(path), "/proc/%s/stat", e->d_name);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        size_t n = fread(buf, 1, sizeof(buf)-1, f);
        fclose(f);
        buf[n] = 0;
        char *p = strrchr(buf, ')');
        long ppid = 0;
        char state;
        if (p && sscanf(p+1, " %c %ld", &state, &ppid)==2 && ppid==getpid()) {
            if (state == 'Z') {
                waitpid(atoi(e->d_name), 0, 0);
                continue;
            }
            snprintf(why, len, "escaped: pid %.20s (%c)", e->d_name, state);
            closedir(d);
            return 0;
        }
    }
    if (d) closedir(d);
    #endif
    for (int i = 0; i < nkids; i++) {
        int alive = !kill((pid_t)kids[i], 0) || errno!=ESRCH;
        int group = !kill(-(pid_t)kids[i], 0) || errno!=ESRCH;
        if (alive || group) {
            snprintf(why, len, "program %ld %s", kids[i], alive ? "alive" : "group alive");
            return 0;
        }
    }
    nkids = 0;
    return 1;
}

static double nowsec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

enum { ANY = 1<<30 };

// The hooks a case installs (Case.hooks), each logging its calls to
// static storage, since snapshots compare the heap's mappings
// H_HELD, with H_TRACE and H_EXIT: the exit hook logs "held N", how
// many of the programs traced to their END are still zombies, kept so
// that their groups' ids cannot be reused before the run ends
enum { H_EXIT = 1<<0, H_ACCESS = 1<<1, H_SPAWN = 1<<2, H_TRACE = 1<<3,
       H_ALL = H_EXIT|H_ACCESS|H_SPAWN|H_TRACE, H_HELD = 1<<4 };

static long long endpids[256];  // H_HELD: the programs traced to an END
static int       nendpids;

static char   hooklog[1<<16];
static size_t hooklen;
static int    nexits;      // the exit hook's calls in a case's run
static int    exitstatus;  // and the status it was last given

static void logf_(char const *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(hooklog+hooklen, sizeof(hooklog)-hooklen, fmt, ap);
    va_end(ap);
    if (n > 0) hooklen += (size_t)n<sizeof(hooklog)-hooklen ? (size_t)n : sizeof(hooklog)-1-hooklen;
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
    logf_("EXIT %d pwd=%s cwd=%s\n", x->status, x->pwd ? x->pwd : "-", x->cwd ? x->cwd : "-");
    if (nendpids) {
        int held = 0;
        for (int i = 0; i < nendpids; i++) {
            siginfo_t si = {0};
            held += !waitid(P_PID, (id_t)endpids[i], &si, WEXITED|WNOHANG|WNOWAIT) &&
                    si.si_pid==(pid_t)endpids[i];
        }
        logf_("held %d\n", held);
    }
    snprintf(exitcwd, sizeof(exitcwd), "%s", x->cwd ? x->cwd : "");
    size_t len = 0;
    int    n   = 0;
    for (char const *const *e = x->env; *e; e++) {
        if (!strncmp(*e, "UBX_", 4) || !strncmp(*e, "PWD=", 4) ||
                !strncmp(*e, "BASH_FUNC_", 10)) {
            logf_("env %s\n", *e);
        }
        size_t elen = strlen(*e) + 1;
        if (n<(int)(sizeof(exitenv)/sizeof(*exitenv))-1 && elen<=sizeof(exitenvbuf)-len) {
            exitenv[n++] = memcpy(exitenvbuf+len, *e, elen);
            len += elen;
        } else {
            logf_("env: truncated\n");
        }
    }
    exitenv[n] = 0;
    if (!x->delta) logf_("delta unknown\n");
    for (char const *const *e = x->delta; e && *e; e++) logf_("delta %s\n", *e);
}

// The cases' directory, $T in their environment, and the run's TMPDIR,
// as the access hook's log shows them
static char tdir[256];
static char const *tmpdir;

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
    int         ignchld;    // SIGCHLD set to SIG_IGN for the run
    int         hostfd;     // pass a host pipe as std[1], read end closed
    char const *opt;        // an option before -c, or null
    char const *notout;     // a substring the output must not hold, or null
    int         hooks;      // H_*
    char const *log;        // a substring the hooks' log must hold, or null
    char const *log2;       // and another, or null
    int         logn;       // >0: and so many times
    char const *nolog;      // a substring the log must not hold, or null
    char const *rerun;      // then this script, from the state the exit
                            // hook was given: its status and output count
    char const *deny;       // a hook refuses an access or a program whose
                            // log line holds this ("OPEN wct $T/w"), or null
    int         denyret;    // returning this; 0: -UB_EACCES
    char const *reason;     // with this reason (%d: the refusal's count)
    int         wrap;       // the spawn hook wraps each program in
                            // "ubhost --wrap" (2: by a relative path)
    int         baseenv;    // run with only PATH, UBX_KEEP=k, UBX_CHG=c,
                            // UBX_GONE=g, UBX_UNX=u, BASH_ENV=$T/benv
    char const *envx;       // and this entry too, or null
    int         linger;     // ub_config.linger
} Case;

// An access as a log line, "OP flags path", T and TMPDIR shown as $T and
// $TMP; refused if the case's rule says so
static int onaccess(void *user, ub_access const *a, char const **reason)
{
    (void)user;
    static char const *const ops[] = {"?", "OPEN", "STAT", "READLINK", "LIST", "TEMPFILE"};
    char const *op = a->op>0 && a->op<6 ? ops[a->op] : "?";
    char fl[8], *p = fl;
    for (int i = 0; i < 7; i++) {
        if (a->flags & 1<<i) *p++ = "rwctaxn"[i];
    }
    if (p == fl) *p++ = '-';
    *p = 0;
    static char line[8192];  // not on the run's stack
    size_t tlen = strlen(tdir), mlen = strlen(tmpdir);
    if (!strncmp(a->path, tdir, tlen)) {
        snprintf(line, sizeof(line), "%s %s $T%s\n", op, fl, a->path+tlen);
    } else if (!strncmp(a->path, tmpdir, mlen)) {
        snprintf(line, sizeof(line), "%s %s $TMP%s\n", op, fl, a->path+mlen);
    } else {
        snprintf(line, sizeof(line), "%s %s %s\n", op, fl, a->path);
    }
    logf_("%s", line);
    Case const *c = curcase;
    if (!c->deny || !strstr(line, c->deny)) return 0;
    ndenied++;
    if (c->reason) {
        snprintf(reasonbuf, sizeof(reasonbuf), c->reason, ndenied);
        *reason = reasonbuf;
    }
    return c->denyret ? c->denyret : -UB_EACCES;
}

// A simple command as log lines: "S<id> b|f|p argv" as it starts, with
// "+" after the id if in a subshell (a level other than $$, TRACEPID),
// T shown as $T; "E<id> status" as it ends, with " p" if a
// program's
enum { TRACEPID = 4242 };

static void ontrace(void *user, ub_trace const *t)
{
    (void)user;
    if (t->phase == UB_TRACE_END) {
        logf_("E%lld %d%s\n", (long long)t->id, t->status, t->pid ? " p" : "");
        if (t->pid && (curcase->hooks & H_HELD) && nendpids<256) endpids[nendpids++] = t->pid;
        return;
    }
    static char const kinds[] = "?bfp";
    logf_("S%lld%s %c", (long long)t->id, t->level==TRACEPID ? "" : "+",
          kinds[t->kind>0 && t->kind<4 ? t->kind : 0]);
    size_t tlen = strlen(tdir);
    for (int i = 0; i < t->argc; i++) {
        char const *a = t->argv[i];
        if (tlen && !strncmp(a, tdir, tlen)) {
            logf_(" $T%s", a+tlen);
        } else {
            logf_(" %s", a);
        }
    }
    logf_("\n");
}

static char self[4096];  // this program (check)

// A program as a log line, "P<trace> name"; refused or wrapped if the
// case's rule says so
static int onspawn(void *user, ub_spawn *s)
{
    (void)user;
    char const *name = strrchr(s->path, '/');
    static char line[4200];  // not on the run's stack
    snprintf(line, sizeof(line), "P%lld %s\n", (long long)s->trace, name ? name+1 : s->path);
    logf_("%s", line);
    Case const *c = curcase;
    if (c->deny && strstr(line, c->deny)) {
        ndenied++;
        if (c->reason) {
            snprintf(reasonbuf, sizeof(reasonbuf), c->reason, ndenied);
            s->reason = reasonbuf;
        }
        return c->denyret ? c->denyret : -UB_EACCES;
    }
    static char const *wrapabs[] = {self, "--wrap", 0};
    static char const *wraprel[] = {"ubhost", "--wrap", 0};
    if (c->wrap) s->wrap = c->wrap==2 ? wraprel : wrapabs;
    return 0;
}

typedef struct {
    ub_os *os;
    int    ms;
    int    sig;
    int    done;
} Stopper;

static void *stopper(void *arg)
{
    Stopper *s = arg;
    for (int i = 0; i < s->ms && !__atomic_load_n(&s->done, __ATOMIC_ACQUIRE); i++) {
        usleep(1000);
    }
    if (!__atomic_load_n(&s->done, __ATOMIC_ACQUIRE)) {
        if (s->sig) ub_signal(s->os, s->sig);
        else ub_cancel(s->os);
    }
    return 0;
}

static char outpath[256];

// One run of a case: its status, output in out.
static int runcase(Case const *c, char *out, size_t outlen, double *secs)
{
    ub_config cfg = {0};
    cfg.spawnlock = spawnlock;
    cfg.child    = onchild;
    cfg.memlimit = c->memlimit;
    cfg.fds      = c->fds;
    cfg.linger   = c->linger;
    if (c->hooks & H_EXIT) cfg.exit = onexit;
    if (c->hooks & H_ACCESS) cfg.access = onaccess;
    if (c->hooks & H_SPAWN) cfg.spawn = onspawn;
    if (c->hooks & H_TRACE) cfg.trace = ontrace;
    curcase = c;
    ndenied = 0;
    hooklen    = 0;
    hooklog[0] = 0;
    nexits     = 0;
    nendpids   = 0;
    ub_os *os = ub_new(&cfg);
    if (!os) return -9999;
    int fd = open(outpath, O_RDWR|O_CREAT|O_TRUNC|O_CLOEXEC, 0600);
    char const *argv[] = {"bash", "-c", c->script, 0, 0};
    if (c->opt) {
        argv[1] = c->opt;
        argv[2] = "-c";
        argv[3] = c->script;
    }
    ub_cmd cmd = {0};
    cmd.argc   = c->opt ? 4 : 3;
    cmd.argv   = argv;
    cmd.std[0] = -1;
    cmd.std[1] = fd;
    cmd.std[2] = fd;
    cmd.pid    = c->pid ? c->pid : c->hooks & H_TRACE ? TRACEPID : 0;
    static char envpath[4200], envbash[300];
    static char const *baseenv[8];
    if (c->baseenv) {
        snprintf(envpath, sizeof(envpath), "PATH=%s", getenv("PATH") ? getenv("PATH") : "");
        snprintf(envbash, sizeof(envbash), "BASH_ENV=%s/benv", tdir);
        char const *v[] = {envpath, "UBX_KEEP=k", "UBX_CHG=c", "UBX_GONE=g", "UBX_UNX=u",
                           envbash, c->envx, 0};
        memcpy(baseenv, v, sizeof(v));
        cmd.env = baseenv;
    }
    int p[2] = {-1, -1};
    if (c->hostfd && !hostpipe(p)) {
        close(p[0]);
        cmd.std[1] = p[1];
    }
    struct sigaction old, ign = {0};
    ign.sa_handler = SIG_IGN;
    if (c->ignchld) sigaction(SIGCHLD, &ign, &old);
    Stopper s = {os, c->stopms, c->stopsig, 0};
    pthread_t t;
    if (c->stopms) pthread_create(&t, 0, stopper, &s);
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
    if (c->stopms) {
        __atomic_store_n(&s.done, 1, __ATOMIC_RELEASE);
        pthread_join(t, 0);
    }
    if (c->ignchld) sigaction(SIGCHLD, &old, 0);
    if (p[1] >= 0) close(p[1]);
    ub_free(os);
    ssize_t n = pread(fd, out, outlen-1, 0);
    out[n<0 ? 0 : n] = 0;
    close(fd);
    return r;
}

static Snap before, after;

// How many times the hooks' log holds s
static int logcount(char const *s)
{
    int n = 0;
    for (char const *p = hooklog; (p = strstr(p, s)); p++) n++;
    return n;
}

static int checkcase(Case const *c, int reps)
{
    char   out[1<<14], why[256] = "";
    double secs;
    runcase(c, out, sizeof(out), &secs);  // warm-up
    for (double until = nowsec()+1; !kidsgone(why, sizeof(why)) && nowsec()<until;) {
        usleep(1000);
    }
    why[0] = 0;
    snapshot(&before);
    int ok = 1;
    for (int i = 0; i<reps && ok; i++) {
        int r = runcase(c, out, sizeof(out), &secs);
        if (c->status!=ANY && r!=c->status) {
            snprintf(why, sizeof(why), "status %d, not %d", r, c->status);
            ok = 0;
        } else if (c->out && !strstr(out, c->out)) {
            snprintf(why, sizeof(why), "output lacks \"%s\"", c->out);
            ok = 0;
        } else if (c->notout && strstr(out, c->notout)) {
            snprintf(why, sizeof(why), "output holds \"%s\"", c->notout);
            ok = 0;
        } else if ((c->hooks & H_EXIT) && nexits!=(r<0 ? 0 : c->rerun ? 2 : 1)) {
            snprintf(why, sizeof(why), "exit hook called %d times", nexits);
            ok = 0;
        } else if ((c->hooks & H_EXIT) && r>=0 && exitstatus!=r) {
            snprintf(why, sizeof(why), "exit hook given %d", exitstatus);
            ok = 0;
        } else if (c->log && (c->logn ? logcount(c->log)!=c->logn : !logcount(c->log))) {
            snprintf(why, sizeof(why), "log holds \"%s\" %d times", c->log, logcount(c->log));
            ok = 0;
        } else if (c->log2 && !logcount(c->log2)) {
            snprintf(why, sizeof(why), "log lacks \"%s\"", c->log2);
            ok = 0;
        } else if (c->nolog && logcount(c->nolog)) {
            snprintf(why, sizeof(why), "log holds \"%s\"", c->nolog);
            ok = 0;
        } else if (c->maxsec && secs>c->maxsec) {
            snprintf(why, sizeof(why), "took %.3fs, over %.3fs", secs, c->maxsec);
            ok = 0;
        }
        if (ok) {
            // A killed program may take a moment to be reapable
            double until = nowsec() + 1;
            while (!(ok = kidsgone(why, sizeof(why))) && nowsec()<until) usleep(1000);
        }
    }
    if (ok) {
        snapshot(&after);
        if (before.len!=after.len || memcmp(before.buf, after.buf, before.len)) {
            ok = 0;
            // The first line that differs
            char *a = before.buf, *b = after.buf;
            while (*a && *a==*b) {
                char *na = strchr(a, '\n'), *nb = strchr(b, '\n');
                if (!na || !nb || na-a!=nb-b || memcmp(a, b, (size_t)(na-a))) break;
                a = na+1;
                b = nb+1;
            }
            char *e = strchr(a, '\n');
            snprintf(why, sizeof(why), "host changed at: %.*s", e ? (int)(e-a) : 40, a);
        }
    }
    printf("%s %-28s %.3fs%s%s\n", ok ? "PASS" : "FAIL", c->name, secs,
           ok ? "" : ": ", ok ? "" : why);
    if (!ok && out[0]) printf("     output: %.300s\n", out);
    if (!ok && hooklen) printf("     log: %.600s\n", hooklog);
    return ok;
}

// Runs on many threads at once, outputs compared byte for byte, while a
// host thread spawns programs of its own and closes each run's write
// end as soon as its run returns.
static char const concurrent[] =
    "x=$(seq 50 | sort -rn | head -3); echo $x; (echo sub) | cat\n"
    "cat <<E\nhere\nE\n"
    "f=/tmp/ubcheck.f.$$; echo f > $f; read y < $f; rm $f; echo $y\n"
    "{ echo bg; } & wait; for i in 1 2 3; do echo $i; done | tr '\\n' ' '; echo\n";
static char const concurrentout[] = "50 49 48\nsub\nhere\nf\nbg\n1 2 3 \n";
static int concfails;

static void *concworker(void *arg)
{
    int reps = *(int *)arg;
    ub_config cfg = {0};
    cfg.spawnlock = spawnlock;
    ub_os *os = ub_new(&cfg);
    for (int i = 0; i < reps; i++) {
        int p[2];
        if (hostpipe(p)) continue;
        char const *argv[] = {"bash", "-c", concurrent, 0};
        ub_cmd c = {0};
        c.argc   = 3;
        c.argv   = argv;
        c.std[0] = -1;
        c.std[1] = p[1];
        c.std[2] = p[1];
        int r = ub_run(os, &c);
        close(p[1]);  // the run's own copy is gone: EOF now
        char buf[4096];
        size_t n = 0;
        for (ssize_t k; (k = read(p[0], buf+n, sizeof(buf)-1-n)) > 0;) n += (size_t)k;
        buf[n] = 0;
        close(p[0]);
        if (r || strcmp(buf, concurrentout)) {
            __atomic_add_fetch(&concfails, 1, __ATOMIC_RELAXED);
            fprintf(stderr, "concurrent: status %d, output:\n%s", r, buf);
        }
    }
    ub_free(os);
    return 0;
}

static char self[4096];  // this program, for --fds

// ubhost --fds: exit with the lowest descriptor above 2 it inherited
// (less 100), or 0.
static int inheritedfds(void)
{
    for (int fd = 3; fd < 1024; fd++) {
        if (fcntl(fd, F_GETFD) >= 0) {
            #if __linux__
            char path[32], name[256];
            snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
            ssize_t n = readlink(path, name, sizeof(name)-1);
            name[n<0 ? 0 : n] = 0;
            fprintf(stderr, "inherited fd %d: %s\n", fd, name);
            #endif
            return 100 + (fd<100 ? fd : 99);
        }
    }
    return 0;
}

static void *hostspawner(void *arg)
{
    int *stop = arg;
    while (!__atomic_load_n(stop, __ATOMIC_ACQUIRE)) {
        pthread_mutex_lock(&hostlock);
        pid_t pid = fork();
        if (!pid) {
            // Every descriptor it inherits, which must be none of a run's
            execl(self, self, "--fds", (char *)0);
            _exit(99);
        }
        pthread_mutex_unlock(&hostlock);
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status)) {
            fprintf(stderr, "concurrent: a host program inherited fd %d\n",
                    WEXITSTATUS(status) - 100);
            __atomic_add_fetch(&concfails, 1, __ATOMIC_RELAXED);
        }
    }
    return 0;
}

static int checkconcurrent(int threads, int reps)
{
    char why[256] = "";
    concfails = 0;
    snapshot(&before);
    double t0 = nowsec();
    int stop = 0;
    pthread_t sp, t[64];
    pthread_create(&sp, 0, hostspawner, &stop);
    for (int i = 0; i < threads; i++) pthread_create(t+i, 0, concworker, &reps);
    for (int i = 0; i < threads; i++) pthread_join(t[i], 0);
    __atomic_store_n(&stop, 1, __ATOMIC_RELEASE);
    pthread_join(sp, 0);
    double secs = nowsec() - t0;
    snapshot(&after);
    int ok = !concfails && kidsgone(why, sizeof(why));
    // Thread stacks may stay cached: compare all but the mappings
    char *a = strstr(before.buf, "\nfd "), *b = strstr(after.buf, "\nfd ");
    if (ok && (!a || !b || strcmp(a, b))) {
        ok = 0;
        while (a && b && *a && *a==*b) {
            char *na = strchr(a+1, '\n'), *nb = strchr(b+1, '\n');
            if (!na || !nb || na-a!=nb-b || memcmp(a, b, (size_t)(na-a))) break;
            a = na;
            b = nb;
        }
        char *e = a ? strchr(a+1, '\n') : 0;
        char *f = b ? strchr(b+1, '\n') : 0;
        snprintf(why, sizeof(why), "host changed at:%.*s, now:%.*s", e ? (int)(e-a) : 40,
                 a ? a : "", f ? (int)(f-b) : 40, b ? b : "");
    }
    printf("%s %-28s %.3fs%s%s\n", ok ? "PASS" : "FAIL", "concurrent runs", secs,
           ok ? "" : ": ", ok ? "" : concfails ? "outputs differ" : why);
    return ok;
}

static int check(char *argv0, int quick)
{
    int reps = quick ? 1 : 3;
    if (!realpath(argv0, self)) snprintf(self, sizeof(self), "%s", argv0);
    snprintf(outpath, sizeof(outpath), "/tmp/ubcheck.%ld", (long)getpid());
    #if __linux__
    prctl(PR_SET_CHILD_SUBREAPER, 1);
    #endif
    // A host file lock that every run must leave held (flock's belongs
    // to the open file, not the process)
    char lockpath[256];
    snprintf(lockpath, sizeof(lockpath), "/tmp/ubcheck.lock.%ld", (long)getpid());
    int lockfd = open(lockpath, O_RDWR|O_CREAT|O_CLOEXEC, 0600);
    flock(lockfd, LOCK_EX);
    // A host pipe that must stay open and silent
    int sentinel[2];
    if (hostpipe(sentinel)) return 1;

    // The cases' directory, with a file to read, a script to source, one
    // to run without #!, and a directory
    snprintf(tdir, sizeof(tdir), "/tmp/ubcheck.t.%ld", (long)getpid());
    tmpdir = getenv("TMPDIR") && *getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
    char tpath[300];
    mkdir(tdir, 0700);
    snprintf(tpath, sizeof(tpath), "%s/d", tdir);
    mkdir(tpath, 0700);
    static char const *const tfiles[][2] = {
        {"f", "data\n"}, {"s", "echo sourced\n"}, {"ns", "echo ns\n"},
        {"sh1", "#!/bin/sh\necho sh1\n"}, {"bi", "#!/nonexistent/interp\n"},
        // the startup file of baseenv's cases
        {"benv", "export UBX_PROF=p; UBX_CHG=prof; [ -z \"$UBX_QUIT\" ] || exit 7\n"},
    };
    for (int i = 0; i < (int)(sizeof(tfiles)/sizeof(*tfiles)); i++) {
        snprintf(tpath, sizeof(tpath), "%s/%s", tdir, tfiles[i][0]);
        int fd = open(tpath, O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, 0755);
        if (fd<0 || write(fd, tfiles[i][1], strlen(tfiles[i][1]))<0) return 1;
        close(fd);
    }
    setenv("T", tdir, 1);

    // Sized once, so that no snapshot sees the host's heap grow
    snapshot(&before);
    snapshot(&after);
    printf("ubhost --check%s\n", quick ? " --quick" : "");
    fflush(stdout);
    char f[64], big[64];
    snprintf(f, sizeof(f), "/tmp/ubcheck.out.%ld", (long)getpid());
    snprintf(big, sizeof(big), "/tmp/ubcheck.big.%ld", (long)getpid());
    char procsub[600], tee[600], fsize[600], fifo[600];
    snprintf(procsub, sizeof(procsub), "seq 1000 > >(sort -rn > %s)", f);
    snprintf(tee, sizeof(tee), "exec > >(tee %s >/dev/null); echo hi", f);
    snprintf(fsize, sizeof(fsize),
             "ulimit -f 1; head -c 4096 /dev/zero > %s; echo s=$?; rm -f %s", big, big);
    // A named FIFO's end, which macOS poll never reports (fifoends). The
    // writer opens it itself: an open with no reader would block the run.
    snprintf(fifo, sizeof(fifo),
             "f=/tmp/ubcheck.fifo.%ld; rm -f $f; mkfifo $f; sh -c \"echo a >$f\" & "
             "exec 3<$f; sleep 1 & while read -u3 l; do echo got $l; done; "
             "read -u3 l; echo again $?; cat <&3; read -u3 l; echo cat $?; "
             "exec 3<&-; rm -f $f; echo end", (long)getpid());

    // A reason too long for one record, cut before a character it splits
    char longreason[400], longout[300];
    memset(longreason, 'A', 254);
    snprintf(longreason+254, sizeof(longreason)-254, "\xc3\xa9%050d\nsecond", 0);
    memset(longout, 'A', 254);
    snprintf(longout+254, sizeof(longout)-254, "\n");

    Case cases[] = {
        {"exit status", "exit 3", 3},
        {"kill $$", "kill -TERM $$", UB_SIGNALED|(128+SIGTERM)},
        {"kill -STOP $$ refused", "kill -STOP $$ 2>/dev/null; echo s=$?", 0, "s=1"},
        {"trapped TSTP to $$", "trap 'echo t' TSTP; kill -TSTP $$", 0, "t"},
        {"kill 0", "kill 0; echo no", UB_SIGNALED|(128+SIGTERM)},
        {"kill -0 group and host", "kill -0 0 && kill -0 $PPID && echo ok", 0, "ok"},
        {"kill -64 0", "trap 'echo t' 64; kill -64 0; echo after", 0, "t\nafter"},
        #if __APPLE__
        // Linux's signal 64, which macOS lacks: the executable's status
        {"kill -64 0 untrapped", "kill -64 0; echo no", 128+64},
        #else
        {"kill -64 0 untrapped", "kill -64 0; echo no", UB_SIGNALED|(128+64)},
        #endif
        {"kill -1, host refused",
         "kill -TERM -1 2>/dev/null; echo a=$?; kill $PPID 2>/dev/null; echo b=$?", 0,
         "a=1\nb=1"},
        {"exec", "exec true; echo no", 0},
        {"exec >&-", "exec >&-; echo x 2>/dev/null; true", 0},
        {"ulimit -n", "ulimit -n 64; ulimit -n; sh -c 'ulimit -n'", 0, "64\n64"},
        {"ulimit -f", fsize, 0, "s=153"},
        {"set -m", "set -m; sleep 0 & wait; echo ok", 0, "ok"},
        {"suspend", "suspend 2>/dev/null; echo s=$?", 0, "s=1"},
        {"title and stty", "printf '\\033]0;x\\007'; stty 2>/dev/null; echo ok", 0, "ok"},
        {"bad pid", "true", -UB_EINVAL, 0, 0, 0, 0, 0, 0, (int64_t)1<<22},
        {"SIGCHLD ignored", "true", -UB_ECHILD, 0, 0, 0, 0, 0, 0, 0, 1},
        {"-n: syntax error", "echo ok; if", 2,
         "syntax error: unexpected end of file", .opt = "-n", .notout = "ok\n"},
        {"-n: unexpected token", "case x in esac; echo )", 2,
         "syntax error near unexpected token `)'", .opt = "-n"},
        {"-n: no errors", "echo hi; exit 3", 0, .opt = "-n", .notout = "hi"},
        {"-n: open here-document", "cat <<E\nx", 0,
         "here-document at line 1 delimited by end-of-file", .opt = "-n", .notout = "x\n"},

        {"leftover sleep &", "sleep 100 &", 0, 0, 0.5},
        {"leftover sh -c &", "sh -c 'sleep 100 &'", 0, 0, 0.5},
        {"leftover (sleep &)", "(sleep 100 &)", 0, 0, 0.5},
        {"leftover busy level", "{ while :; do :; done; } &", 0, 0, 0.5},
        {"leftover procsub", ": > >(while :; do sleep 0.01; done)", 0, 0, 1.5},
        {"procsub finishes", procsub, 0, 0, 1.5},
        {"exec into procsub", tee, 0, 0, 1.5},
        {"named FIFO ends", fifo, 0, "got a\nagain 1\ncat 1\nend", 2},
        {"300 programs left", "for i in $(seq 300); do sleep 1000 & done; true; echo done",
         0, "done", 10*FORKSCALE},  // QEMU: 300 emulated starts
        {"SIGPIPE in programs", "{ yes | head -1; echo ${PIPESTATUS[@]}; } & wait", 0,
         "141 0"},
        {"closed host pipe", "echo hi; echo no >&2", UB_SIGNALED|(128+SIGPIPE), 0, 0, 0,
         0, 0, 0, 0, 0, 1},

        {"hard: busy loop", "while :; do :; done", -UB_ECANCELED, 0, 0.35, 100},
        {"hard: arith loop", "while ((1)); do ((1)); done", -UB_ECANCELED, 0, 0.35, 100},
        {"hard: sleep", "sleep 100", -UB_ECANCELED, 0, 0.35, 100},
        {"hard: read", "read x < <(sleep 100)", -UB_ECANCELED, 0, 0.35, 100},
        {"hard: pipeline", "cat /dev/zero | cat >/dev/null", -UB_ECANCELED, 0, 0.35, 100},
        {"hard: trapped loop", "trap 'echo t' TERM; while :; do :; done",
         -UB_ECANCELED, 0, 0.35, 100},
        {"soft: busy loop", "while :; do :; done", UB_SIGNALED|(128+SIGTERM), 0, 1, 100,
         SIGTERM|UB_GROUP},
        {"soft: sleep", "sleep 100", UB_SIGNALED|(128+SIGTERM), 0, 1, 100, SIGTERM|UB_GROUP},
        {"soft: pipeline", "sleep 100 | sleep 100 | cat", UB_SIGNALED|(128+SIGTERM), 0, 1,
         100, SIGTERM|UB_GROUP},
        {"soft: trapped subshell", "( trap 'echo t' TERM; sleep 100 ); sleep 100",
         UB_SIGNALED|(128+SIGTERM), "t", 1, 100, SIGTERM|UB_GROUP},
        {"soft: comsub keeps output", "trap 'echo trapped' TERM; x=$(sleep 0.5; echo hi); echo x=$x",
         0, "trapped\nx=hi", 1.5, 100, SIGTERM},

        {"out of memory", "x=a; while :; do x=$x$x; done", -UB_ENOMEM, "out of memory", 0, 0,
         0, 32<<20},

        {"exit hook", "cd /; export UBX_A=1; UBX_B=2; exit 3", 3, .hooks = H_EXIT,
         .log = "EXIT 3 pwd=/ cwd=/\nenv PWD=/\nenv UBX_A=1\n", .nolog = "UBX_B"},
        {"exit hook: function", "f() { cd /usr; exit 4; }; f; echo no", 4, .notout = "no",
         .hooks = H_EXIT, .log = "EXIT 4 pwd=/usr cwd=/usr\n"},
        {"exit hook: set -e", "set -e; false; echo no", 1, .notout = "no", .hooks = H_EXIT,
         .log = "EXIT 1 "},
        {"exit hook: exit 300", "exit 300", 44, .hooks = H_EXIT, .log = "EXIT 44 "},
        {"exit hook: exec", "cd /; exec true; echo no", 0, .notout = "no", .hooks = H_EXIT,
         .log = "EXIT 0 pwd=/ "},
        {"exit hook: signal", "kill -TERM $$", UB_SIGNALED|(128+SIGTERM), .hooks = H_EXIT},
        {"exit hook: EXIT trap", "trap 'cd /usr; export UBX_T=t' EXIT; cd /", 0,
         .hooks = H_EXIT, .log = "EXIT 0 pwd=/usr cwd=/usr\nenv PWD=/usr\nenv UBX_T=t\n"},
        {"exit hook: PWD reassigned", "cd /usr; PWD=/nowhere", 0, .hooks = H_EXIT,
         .log = "EXIT 0 pwd=/usr cwd=/usr\nenv PWD=/nowhere\n"},
        {"exit hook: export -f", "f() { echo; }; export -f f", 0, .hooks = H_EXIT,
         .log = "env BASH_FUNC_f%%=() {  echo\n}\n"},
        {"exit hook: background", "cd /; sleep 100 & exit 5", 5, 0, 0.5, .hooks = H_EXIT,
         .log = "EXIT 5 pwd=/ "},
        {"exit hook: cancel", "cd /; while :; do :; done", -UB_ECANCELED, 0, 0.35, 100,
         .hooks = H_EXIT},
        {"exit hook: out of memory", "x=a; while :; do x=$x$x; done", -UB_ENOMEM,
         "out of memory", 0, 0, 0, 32<<20, .hooks = H_EXIT},
        // The delta: what the run changed of the exports after its
        // startup files, so not what the profile set (UBX_PROF)
        {"exit hook: delta",
         "UBX_CHG=new; unset UBX_GONE; export -n UBX_UNX; export UBX_NEW=n;"
         " f() { :; }; export -f f; echo $UBX_PROF", 0, "p\n", .hooks = H_EXIT, .baseenv = 1,
         .log = "delta BASH_FUNC_f%%=() {  :\n}\ndelta UBX_CHG=new\ndelta UBX_NEW=n\n"
                "delta UBX_GONE\ndelta UBX_UNX\n", .nolog = "delta UBX_PROF"},
        {"exit hook: no delta", ":", 0, .hooks = H_EXIT, .baseenv = 1, .nolog = "delta"},
        {"exit hook: exit in startup", ":", 7, .hooks = H_EXIT, .baseenv = 1,
         .envx = "UBX_QUIT=1", .log = "delta unknown\n"},
        // linger: a SIGTERM, and time to act on it, before the kill;
        // the wait ends once the programs have
        {"linger: SIGTERM first",
         "sh -c 'trap \"echo term >$T/lg; exit 0\" TERM; while :; do sleep 0.01; done' &"
         " sleep 0.2", 0, "term\n", 2, .hooks = H_EXIT, .linger = 5000,
         .rerun = "cat $T/lg; rm $T/lg"},
        {"linger: ends at most then", "sh -c 'trap \"\" TERM; sleep 100' & sleep 0.2", 0,
         0, 1.5, .linger = 300},
        {"exit hook: next run",

 "cd /usr; export UBX_RT=rt; UBX_LOCAL=x", 0,
         "/usr\nrt\nunset\n", .hooks = H_EXIT,
         .rerun = "pwd; echo $UBX_RT; echo ${UBX_LOCAL-unset}"},

        {"access: >", "echo a > $T/w", 0, .hooks = H_ACCESS, .log = "OPEN wct $T/w\n",
         .logn = 1},
        {"access: >>", "echo a >> $T/w", 0, .hooks = H_ACCESS, .log = "OPEN wca $T/w\n"},
        {"access: <", "read x < $T/f; echo $x", 0, "data", .hooks = H_ACCESS,
         .log = "OPEN r $T/f\n"},
        {"access: <>", "exec 3<> $T/w", 0, .hooks = H_ACCESS, .log = "OPEN rwc $T/w\n"},
        {"access: $(<f)", "x=$(<$T/f); echo $x", 0, "data", .hooks = H_ACCESS,
         .log = "OPEN r $T/f\n"},
        {"access: test", "test -e $T/f && test -h $T/f; true", 0, .hooks = H_ACCESS,
         .log = "STAT - $T/f\n", .log2 = "STAT n $T/f\n"},
        {"access: cd", "cd $T/d && echo ok", 0, "ok", .hooks = H_ACCESS,
         .log = "STAT - $T/d\n"},
        {"access: source", "cd $T; source ./s", 0, "sourced", .hooks = H_ACCESS,
         .log = "OPEN r $T/./s\n"},
        {"access: glob", "cd $T; echo *", 0, "d f ns s", .hooks = H_ACCESS,
         .log = "LIST r $T"},
        {"access: here-document file", "x=$(printf %070000d 0); cat <<E | wc -c\n$x\nE", 0,
         "70001", .hooks = H_ACCESS, .log = "TEMPFILE rwcx $TMP"},
        {"access: script without #!", "$T/ns", 0, "ns", .hooks = H_ACCESS,
         .log = "OPEN r $T/ns\n", .logn = 1},
        {"access: not a program's head", "/bin/echo hi", 0, "hi", .hooks = H_ACCESS,
         .nolog = "OPEN r /bin/echo"},
        {"access: not /dev/null", "sleep 0 & wait", 0, .hooks = H_ACCESS,
         .nolog = "/dev/null"},
        {"access: not the password file", "echo ~root", 0, .hooks = H_ACCESS,
         .nolog = "passwd"},
        {"access: not zoneinfo", "TZ=Europe/Paris printf '%(%H)T\\n' 0", 0,
         .hooks = H_ACCESS, .nolog = "zoneinfo"},
        {"deny: redirection", "echo a > $T/w; echo s=$?", 0, "w: no way\ns=1",
         .hooks = H_ACCESS, .deny = "OPEN wct $T/w", .reason = "no way"},
        {"deny: unknown value", "echo a > $T/w", 1, "w: Permission denied",
         .hooks = H_ACCESS, .deny = "OPEN wct $T/w", .denyret = 77},
        {"deny: ENOENT", "echo a > $T/w", 1, "w: No such file or directory",
         .hooks = H_ACCESS, .deny = "OPEN wct $T/w", .denyret = -UB_ENOENT},
        {"deny: long reason", "echo a > $T/w", 1, longout, .notout = "\xc3",
         .hooks = H_ACCESS, .deny = "OPEN wct $T/w", .reason = longreason},
        {"deny: $(<f)", "x=$(<$T/f); echo \"x=$x\"", 0, "f: no\nx=\n",
         .hooks = H_ACCESS, .deny = "OPEN r $T/f", .reason = "no"},
        {"deny: source", "source $T/s; echo s=$?", 0, "s: no\ns=1", .notout = "sourced",
         .hooks = H_ACCESS, .deny = "OPEN r $T/s", .reason = "no"},
        {"deny: cd", "cd $T/d; echo s=$?", 0, "d: no\ns=1", .hooks = H_ACCESS,
         .deny = "STAT - $T/d", .reason = "no"},
        {"deny: test", "test -e $T/f && echo yes || echo no", 0, "no", .notout = "yes",
         .hooks = H_ACCESS, .deny = "STAT - $T/f", .reason = "no"},
        {"deny: glob", "cd $T; echo *", 0, "*\n", .hooks = H_ACCESS, .deny = "LIST",
         .reason = "no"},
        {"deny: PATH search", "ls; echo s=$?", 0, "ls: command not found\ns=127",
         .hooks = H_ACCESS, .deny = "/ls\n", .reason = "no"},
        {"deny: script read", "$T/ns; echo s=$?", 0, "ns: no\ns=126", .notout = "ns\n",
         .hooks = H_ACCESS, .deny = "OPEN r $T/ns", .reason = "no"},
        {"deny: script read, ENOENT", "$T/ns; echo s=$?", 0,
         "ns: No such file or directory\ns=127", .hooks = H_ACCESS,
         .deny = "OPEN r $T/ns", .denyret = -UB_ENOENT},
        {"deny: here-document file",
         "x=$(printf %070000d 0); cat <<E | wc -c\n$x\nE\necho s=${PIPESTATUS[0]}", 0,
         "cannot create temp file for here-document: no", .hooks = H_ACCESS,
         .deny = "TEMPFILE", .reason = "no"},
        {"deny: 40 reasons", "for i in {1..40}; do echo > $T/w$i; done", 1,
         "w40: denied 40\n", .hooks = H_ACCESS, .deny = "OPEN wct $T/w",
         .reason = "denied %d"},
        {"spawn: wrapped", "/bin/echo hi", 0, "wrapped /bin/echo\nhi\n", .hooks = H_SPAWN,
         .log = "P0 echo\n", .wrap = 1},
        {"spawn: wrapped pipeline", "echo a | tr a b | cat", 0, "b\n", .hooks = H_SPAWN,
         .log = "P0 tr\n", .log2 = "P0 cat\n", .wrap = 1},
        {"spawn: wrapped &", "sleep 0 & wait $!; echo s=$?", 0, "s=0", .hooks = H_SPAWN,
         .log = "P0 sleep\n", .wrap = 1},
        {"spawn: wrapped exec", "exec /bin/echo hi; echo no", 0, "wrapped /bin/echo\nhi\n",
         .notout = "no", .hooks = H_SPAWN, .wrap = 1},
        {"spawn: script without #!", "$T/ns", 0, "ns\n", .notout = "wrapped",
         .hooks = H_SPAWN, .nolog = "P0 ns", .wrap = 1},
        {"spawn: stage without #!", "$T/ns | cat", 0, "ns\n", .hooks = H_SPAWN,
         .log = "P0 cat\n", .nolog = "P0 ns", .wrap = 1},
        {"spawn: #! script", "$T/sh1", 0, "sh1\n", .hooks = H_SPAWN, .log = "P0 sh1\n",
         .wrap = 1},
        {"spawn: bad interpreter", "$T/bi; echo s=$?", 0, "s=126", .hooks = H_SPAWN,
         .wrap = 1},
        {"spawn: relative wrapper", "/bin/echo hi; echo s=$?", 0,
         "/bin/echo: Invalid argument\ns=126", .hooks = H_SPAWN, .wrap = 2},
        {"spawn refused: reason", "/bin/echo hi; echo s=$?", 0,
         "/bin/echo: not allowed\ns=126", .notout = "hi\n", .hooks = H_SPAWN,
         .deny = "P0 echo", .reason = "not allowed"},
        {"spawn refused: errno", "/bin/echo hi; echo s=$?", 0,
         "/bin/echo: Operation not permitted\ns=126", .hooks = H_SPAWN, .deny = "P0 echo",
         .denyret = -UB_EPERM},
        {"spawn refused: ENOENT", "/bin/echo hi; echo s=$?", 0,
         "/bin/echo: No such file or directory\ns=127", .hooks = H_SPAWN, .deny = "P0 echo",
         .denyret = -UB_ENOENT},
        {"spawn refused: PIPESTATUS", "/bin/echo hi | cat; echo ${PIPESTATUS[@]}", 0,
         "/bin/echo: no\n126 0", .hooks = H_SPAWN, .deny = "P0 echo", .reason = "no"},
        {"spawn refused: &", "/bin/echo hi & wait $!; echo s=$?", 0, "s=126",
         .hooks = H_SPAWN, .deny = "P0 echo", .reason = "no"},
        {"spawn refused: exec", "exec /bin/echo hi; echo after", 126, "/bin/echo: no",
         .notout = "after", .hooks = H_SPAWN, .deny = "P0 echo", .reason = "no"},
        {"spawn refused: #! script", "$T/sh1; echo s=$?", 0, "sh1: no\ns=126",
         .notout = "interpreter", .hooks = H_SPAWN, .deny = "P0 sh1", .reason = "no"},
        {"trace: builtin", "echo hi", 0, "hi", .hooks = H_TRACE, .log = "S1 b echo hi\nE1 0\n"},
        {"trace: function", "f() { true; false; }; f", 1, .hooks = H_TRACE,
         .log = "S1 f f\nS2 b true\nE2 0\nS3 b false\nE3 1\nE1 1\n"},
        {"trace: program", "/bin/echo hi", 0, "hi", .hooks = H_TRACE,
         .log = "S1 p /bin/echo hi\nE1 0 p\n"},
        {"trace: pipeline", "seq 3 | tr 1 x | cat", 0, "x\n2\n3\n", .hooks = H_TRACE,
         .log = "S1+ p seq 3\n", .log2 = "S3+ p cat\n", .nolog = "S4"},
        {"trace: &", "sleep 0 & wait $!", 0, .hooks = H_TRACE,
         .log = "S1+ p sleep 0\n", .log2 = "S2 b wait ", .nolog = "S3"},
        {"trace: redirection failure", "echo hi > /nonexistent/x", 1, .hooks = H_TRACE,
         .log = "S1 b echo hi\nE1 1\n"},
        {"trace: exit in a function", "f() { exit 3; }; f", 3, .hooks = H_TRACE,
         .log = "S1 f f\nS2 b exit 3\n", .nolog = "E"},
        {"trace: set -e", "set -e; false; echo no", 1, .notout = "no", .hooks = H_TRACE,
         .log = "S1 b set -e\nE1 0\nS2 b false\nE2 1\n", .nolog = "S3"},
        {"trace: exec", "exec /bin/echo hi; echo no", 0, "hi", .notout = "no",
         .hooks = H_TRACE, .log = "S1 b exec /bin/echo hi\nE1 0 p\n"},
        {"trace: killed at the end", "sleep 100 & exit 4", 4, .hooks = H_TRACE|H_EXIT,
         .log = "S1+ p sleep 100\n", .log2 = "E1 393 p\n"},
        // An exited program stays a zombie until the run ends, so that
        // the id of its group, which may outlive it, stays the run's;
        // past 64, the oldest are reaped. macOS reaps at once one that
        // was alone in its group.
        {"held: group outlives its leader", "sh -c 'sleep 100 & exit 3'; echo s=$?", 0,
         "s=3", .hooks = H_TRACE|H_EXIT|H_HELD, .log = "held 1\n"},
        {"held: at most 64", "for i in $(seq 100); do /bin/sh -c :; done", 0,
         .hooks = H_TRACE|H_EXIT|H_HELD,
#if __APPLE__
         .log = "held 0\n"},
#else
         .log = "held 64\n"},
#endif
        {"trace: stage without #!", "$T/ns | cat", 0, "ns\n", .hooks = H_TRACE,
         .log = "$T/ns\n", .logn = 1, .log2 = "S2+ b echo ns\nE2 0\n"},
        {"trace: spawn's id", "/bin/echo hi", 0, "hi", .hooks = H_TRACE|H_SPAWN,
         .log = "S1 p /bin/echo hi\nP1 echo\nE1 0 p\n"},
        {"trace: refused stage", "/bin/echo hi | cat", 0, .hooks = H_TRACE|H_SPAWN,
         .deny = "echo", .reason = "no", .log = "S1+ p /bin/echo hi\nP1 echo\n", .log2 = "E1 126\n"},
        {"descriptor budget", "for i in $(seq 40); do exec {fd}</dev/null || break; done; echo n=$i",
         0, "n=", 0, 0, 0, 0, 24},
    };
    int fails = 0;
    for (size_t i = 0; i < sizeof(cases)/sizeof(*cases); i++) {
        Case const *c = cases + i;
        fails += !checkcase(c, c->stopms || c->maxsec>=1 ? 1 : reps);
        if (c->script==procsub || c->script==tee) {
            // what the procsub wrote must be there whole
            char buf[8192] = "";
            int fd = open(f, O_RDONLY);
            ssize_t n = fd<0 ? 0 : read(fd, buf, sizeof(buf)-1);
            if (fd >= 0) close(fd);
            buf[n<0 ? 0 : n] = 0;
            int lines = 0;
            for (char *p = buf; *p; p++) lines += *p == '\n';
            int good = c->script==procsub ? lines==1000 && !strncmp(buf, "1000\n", 5)
                                          : !strcmp(buf, "hi\n");
            if (!good) {
                printf("FAIL %-28s wrote %d lines\n", c->name, lines);
                fails++;
            }
            unlink(f);
        }
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
        for (ptrdiff_t m = 1<<20; m <= 24<<20; m += quick ? 4<<20 : 1<<20) {
            sweep.memlimit = m;
            char name[64];
            snprintf(name, sizeof(name), "memlimit %ld KiB%s", (long)(m>>10), h ? ", hooks" : "");
            sweep.name = name;
            fails += !checkcase(&sweep, 1);
        }
    }
    Case after = {"a run after the sweep", "echo ok", 0, "ok"};
    fails += !checkcase(&after, 1);

    fails += !checkconcurrent(8, quick ? 10 : 50);

    // The sentinels
    struct pollfd pf = {sentinel[0], POLLIN, 0};
    if (poll(&pf, 1, 0) != 0) {
        printf("FAIL host pipe was written or closed\n");
        fails++;
    }
    int other = open(lockpath, O_RDWR|O_CLOEXEC);
    if (!flock(other, LOCK_EX|LOCK_NB)) {
        printf("FAIL host flock was released\n");
        fails++;
    }
    close(other);
    unlink(lockpath);
    unlink(outpath);
    char rm[300];
    snprintf(rm, sizeof(rm), "rm -rf %s", tdir);
    if (system(rm)) printf("cannot remove %s\n", tdir);
    printf("%d failed\n", fails);
    return fails != 0;
}
