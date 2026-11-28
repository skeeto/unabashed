// ubhost: a test host for libunabashed (unabashed.h), POSIX
//
// As a drop-in, "ubhost [bash args]" behaves as the unabashed
// executable does: the run takes its arguments, environment, directory,
// descriptors and pid, a signal thread forwards the signals it gets,
// and it exits as the shell did, dying of the same signal.
//
// When ubhost leads its terminal's session with its group in front, as a
// terminal tab's shell does (std 2's terminal, else std 0's), the run is
// interactive on it with UB_TERMINAL, and the signals are forwarded
// plainly, as Prospero does. UBHOST_TERM=1 asks for UB_TERMINAL
// regardless (with no terminal it does nothing, as in the library), and
// UBHOST_TERM=0 never; the run does not see the variable. A run the
// library refuses terminal mode, as it does unless ubhost leads the
// session (or, detected, by a race), is reported and exits 126, never
// retried without it. On macOS SIGWINCH, SIGURG and SIGIO are caught
// (unless ignored on entry), in every drop-in run, so that they reach
// the forwarder (notes/library.md, Terminal mode).
//
// "ubhost --check [--quick]" checks the host contract (ubcheck.c);
// "ubhost --term-case N [--quick]" is its child for a terminal case, and
// "--tcgrab" and "--pgsleep" are programs that cases run.
//
// "ubhost --wrap PATH ARGV0 [ARG...]" is a spawn hook's wrapper, as the
// contract has it (notes/library.md): it says "wrapped PATH" on stderr,
// then becomes the program.
//
// $ cc -O2 -o ubhost tools/ubhost.c libunabashed.o -lpthread
// On Windows, see ubhostw.c.
// This is free and unencumbered software released into the public domain.
#if _WIN32
#include "../unabashed.h"
#include "ubhostw.c"
#else
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../unabashed.h"
#include "ubcheck.c"

static ub_os *runos;
static int    terminal;  // the run is in terminal mode (UB_TERMINAL)

// The signals the forwarder waits for: every one but KILL, STOP and
// CHLD, the real-time signals too on Linux, as Prospero blocks them. On
// Linux a fault of the host's own kills it even while blocked, so the
// faults another process sends with kill are forwarded too, as the
// executable takes them. SIGPIPE and SIGXFSZ the run raises on itself
// stay pending on its own thread, unseen here; on macOS the host
// ignores them.
static void waitset(sigset_t *set)
{
    sigemptyset(set);
    #if __linux__
    int nsig = SIGRTMAX;  // libc's own, below SIGRTMIN, are refused
    #else
    int nsig = 31;
    #endif
    for (int sig = 1; sig <= nsig; sig++) {
        switch (sig) {
        case SIGKILL: case SIGSTOP: case SIGCHLD:
            continue;
        #if !__linux__
        case SIGILL: case SIGTRAP: case SIGBUS: case SIGFPE: case SIGSEGV:
        case SIGSYS:
            continue;
        #endif
        }
        sigaddset(set, sig);
    }
}

// Every signal the process gets, forwarded to the run. Outside terminal
// mode, the terminal's INT, QUIT and TSTP go to the whole foreground
// group, so to every level of the run (UB_GROUP), as the executable
// takes them. In terminal mode every signal is forwarded plainly: the
// run takes the terminal's as the terminal's already.
static void *forwarder(void *arg)
{
    (void)arg;
    sigset_t set;
    waitset(&set);
    for (;;) {
        int group = 0;
        #if __linux__
        siginfo_t si;
        int sig = sigwaitinfo(&set, &si);
        if (sig < 0) continue;
        group = !terminal &&
                (sig==SIGINT || sig==SIGQUIT || sig==SIGTSTP) &&
                si.si_code==SI_KERNEL;
        #else
        int sig;
        if (sigwait(&set, &sig)) continue;
        #endif
        ub_signal(runos, sig | (group ? UB_GROUP : 0));
    }
    return 0;
}

// Whether this process leads its terminal's session, its group in front:
// the terminal UB_TERMINAL would take, std 2's, else std 0's
static int leadsterminal(void)
{
    int fd = isatty(2) ? 2 : isatty(0) ? 0 : -1;
    return fd>=0 && getsid(0)==getpid() &&
           tcgetsid(fd)==getpid() && tcgetpgrp(fd)==getpgrp();
}

static void exitwith(int status)
{
    if (status < 0) {
        if (status==-UB_EINVAL && terminal) {
            fprintf(stderr, "ubhost: terminal mode refused: ubhost must "
                            "lead its terminal's session, its group in front\n");
        } else if (status != -UB_ENOMEM) {
            fprintf(stderr, "ubhost: run failed: error %d\n", -status);
        }
        _exit(status==-UB_ENOMEM ? 2 : 126);
    }
    if (status & UB_SIGNALED) {
        int sig = (status & 255) - 128;
        signal(sig, SIG_DFL);
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, sig);
        pthread_sigmask(SIG_UNBLOCK, &set, 0);
        kill(getpid(), sig);
    }
    _exit(status & 255);
}

// The wrapper: everything inherited, the program exec'd in its place,
// or exec's failure reported as bash does: 127 if it is not there, else
// 126 (an interpreter not found too)
static int wrap(int argc, char **argv)
{
    if (argc < 4) return 2;
    fprintf(stderr, "wrapped %s\n", argv[2]);
    execv(argv[2], argv+3);
    int err = errno;
    fprintf(stderr, "ubhost: %s: %s\n", argv[2], strerror(err));
    return err==ENOENT && access(argv[2], F_OK) ? 127 : 126;
}

int main(int argc, char **argv)
{
    if (argc>1 && !strcmp(argv[1], "--wrap")) {
        return wrap(argc, argv);
    }
    if (argc>1 && !strcmp(argv[1], "--check")) {
        #if __APPLE__
        signal(SIGPIPE, SIG_IGN);  // as every macOS host must
        signal(SIGXFSZ, SIG_IGN);
        #endif
        return check(argv[0], argc>2 && !strcmp(argv[2], "--quick"));
    }
    if (argc>2 && !strcmp(argv[1], "--term-case")) {
        return termchild(argv[0], atoi(argv[2]), argc>3 && !strcmp(argv[3], "--quick"));
    }
    if (argc>1 && !strcmp(argv[1], "--fds")) {
        return inheritedfds();
    }
    if (argc>1 && !strcmp(argv[1], "--tcgrab")) {
        return tcgrab();
    }
    if (argc>1 && !strcmp(argv[1], "--pgsleep")) {
        return pgsleep();
    }
    ub_cmd cmd = {0};
    cmd.argc = argc;
    cmd.argv = (char const *const *)argv;
    for (int i = 0; i < 3; i++) cmd.std[i] = i;
    cmd.pid  = getpid();
    cmd.ppid = getppid();

    // Entry dispositions: ignored signals stay so in the run
    for (int sig = 1; sig <= 64; sig++) {
        struct sigaction sa;
        if (!sigaction(sig, 0, &sa) && sa.sa_handler==SIG_IGN) {
            cmd.sigign |= (uint64_t)1 << (sig-1);
        }
    }

    // Inherited descriptors, now the host's own (close-on-exec)
    static ub_fd fds[1024];
    for (int fd = 3; fd < 1024; fd++) {
        int fl = fcntl(fd, F_GETFD);
        if (fl < 0) continue;
        fcntl(fd, F_SETFD, fl|FD_CLOEXEC);
        fds[cmd.nfds].fd  = fd;
        fds[cmd.nfds++].h = fd;
    }
    cmd.fds = fds;

    #if __APPLE__
    // macOS directs these at the process, so the host ignores them
    signal(SIGPIPE, SIG_IGN);
    signal(SIGXFSZ, SIG_IGN);
    // macOS discards a signal whose default is to ignore it, blocked or
    // not, unless it is caught: a resize's SIGWINCH, and URG and IO,
    // would never reach the forwarder. Those ignored on entry stay so.
    // Programs start with default dispositions regardless.
    int sigs[] = {SIGWINCH, SIGURG, SIGIO};
    for (int i = 0; i < (int)(sizeof(sigs)/sizeof(*sigs)); i++) {
        if (cmd.sigign>>(sigs[i]-1) & 1) continue;
        struct sigaction sa = {0};
        sa.sa_handler = caught;
        sigaction(sigs[i], &sa, 0);
    }
    #endif

    // Terminal mode: when it leads its terminal's session, or as asked.
    // The request is ubhost's alone, not the run's environment, so that
    // a nested ubhost decides for itself. With neither std 2 nor std 0 a
    // terminal the flag does nothing, as the library has it: the run is
    // isolated, and its programs want the terminal's signals by UB_GROUP.
    char const *term = getenv("UBHOST_TERM");
    int ask;
    if (term && !strcmp(term, "1")) {
        ask = 1;
    } else if (term && !strcmp(term, "0")) {
        ask = 0;
    } else {
        ask = leadsterminal();
    }
    unsetenv("UBHOST_TERM");
    terminal = ask && (isatty(2) || isatty(0));
    ub_config cfg = {0};
    cfg.flags = ask ? UB_TERMINAL : 0;
    runos = ub_new(&cfg);
    if (!runos) {
        fprintf(stderr, "ubhost: ub_new failed\n");
        return 126;
    }
    sigset_t set;
    waitset(&set);
    pthread_sigmask(SIG_BLOCK, &set, 0);
    pthread_t t;
    pthread_create(&t, 0, forwarder, 0);
    exitwith(ub_run(runos, &cmd));
}
#endif
