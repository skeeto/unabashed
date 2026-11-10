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
    int         ignchld;    // SIGCHLD set to SIG_IGN for the run
    int         hostfd;     // pass a host pipe as std[1], read end closed
} Case;

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
    ub_os *os = ub_new(&cfg);
    if (!os) return -9999;
    int fd = open(outpath, O_RDWR|O_CREAT|O_TRUNC|O_CLOEXEC, 0600);
    char const *argv[] = {"bash", "-c", c->script, 0};
    ub_cmd cmd = {0};
    cmd.argc   = 3;
    cmd.argv   = argv;
    cmd.std[0] = -1;
    cmd.std[1] = fd;
    cmd.std[2] = fd;
    cmd.pid    = c->pid;
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

    // Sized once, so that no snapshot sees the host's heap grow
    snapshot(&before);
    snapshot(&after);
    printf("ubhost --check%s\n", quick ? " --quick" : "");
    fflush(stdout);
    char f[64], big[64];
    snprintf(f, sizeof(f), "/tmp/ubcheck.out.%ld", (long)getpid());
    snprintf(big, sizeof(big), "/tmp/ubcheck.big.%ld", (long)getpid());
    char procsub[600], tee[600], fsize[600];
    snprintf(procsub, sizeof(procsub), "seq 1000 > >(sort -rn > %s)", f);
    snprintf(tee, sizeof(tee), "exec > >(tee %s >/dev/null); echo hi", f);
    snprintf(fsize, sizeof(fsize),
             "ulimit -f 1; head -c 4096 /dev/zero > %s; echo s=$?; rm -f %s", big, big);

    Case cases[] = {
        {"exit status", "exit 3", 3},
        {"kill $$", "kill -TERM $$", UB_SIGNALED|(128+SIGTERM)},
        {"kill -STOP $$ refused", "kill -STOP $$ 2>/dev/null; echo s=$?", 0, "s=1"},
        {"trapped TSTP to $$", "trap 'echo t' TSTP; kill -TSTP $$", 0, "t"},
        {"kill 0", "kill 0; echo no", UB_SIGNALED|(128+SIGTERM)},
        {"kill -0 group and host", "kill -0 0 && kill -0 $PPID && echo ok", 0, "ok"},
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

        {"leftover sleep &", "sleep 100 &", 0, 0, 0.5},
        {"leftover sh -c &", "sh -c 'sleep 100 &'", 0, 0, 0.5},
        {"leftover (sleep &)", "(sleep 100 &)", 0, 0, 0.5},
        {"leftover busy level", "{ while :; do :; done; } &", 0, 0, 0.5},
        {"leftover procsub", ": > >(while :; do sleep 0.01; done)", 0, 0, 1.5},
        {"procsub finishes", procsub, 0, 0, 1.5},
        {"exec into procsub", tee, 0, 0, 1.5},
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
    for (ptrdiff_t m = 1<<20; m <= 24<<20; m += quick ? 4<<20 : 1<<20) {
        sweep.memlimit = m;
        char name[64];
        snprintf(name, sizeof(name), "memlimit %ld KiB", (long)(m>>10));
        sweep.name = name;
        fails += !checkcase(&sweep, 1);
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
    printf("%d failed\n", fails);
    return fails != 0;
}
