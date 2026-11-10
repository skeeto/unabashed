// ubhost: a test host for libunabashed (unabashed.h), POSIX
//
// As a drop-in, "ubhost [bash args]" behaves as the unabashed
// executable does: the run takes its arguments, environment, directory,
// descriptors and pid, a signal thread forwards the signals it gets,
// and it exits as the shell did, dying of the same signal.
//
// $ cc -O2 -o ubhost tools/ubhost.c libunabashed.o -lpthread
// This is free and unencumbered software released into the public domain.
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

static ub_os *runos;

// A synchronous fault in this thread is the host's own, never waited
// for: blocking it would hang or kill without a core.
static int faultsig(int sig)
{
    switch (sig) {
    case SIGILL: case SIGTRAP: case SIGBUS: case SIGFPE: case SIGSEGV:
    case SIGSYS: case SIGKILL: case SIGSTOP: case SIGCHLD:
        return 1;
    }
    return 0;
}

static void waitset(sigset_t *set)
{
    sigemptyset(set);
    for (int sig = 1; sig < 32; sig++) {
        if (!faultsig(sig)) sigaddset(set, sig);
    }
}

// Every signal the process gets, forwarded to the run. The terminal's
// INT, QUIT and TSTP go to the whole foreground group, so to every
// level of the run (UB_GROUP), as the executable takes them.
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
        group = (sig==SIGINT || sig==SIGQUIT || sig==SIGTSTP) &&
                si.si_code==SI_KERNEL;
        #else
        int sig;
        if (sigwait(&set, &sig)) continue;
        #endif
        if (sig==SIGPIPE || sig==SIGXFSZ) continue;  // the run's own
        ub_signal(runos, sig | (group ? UB_GROUP : 0));
    }
    return 0;
}

static void exitwith(int status)
{
    if (status < 0) {
        if (status != -UB_ENOMEM) {
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

int main(int argc, char **argv)
{
    ub_cmd cmd = {0};
    cmd.argc = argc;
    cmd.argv = (char const *const *)argv;
    for (int i = 0; i < 3; i++) cmd.std[i] = i;
    cmd.pid  = getpid();
    cmd.ppid = getppid();

    // Entry dispositions: ignored signals stay so in the run
    for (int sig = 1; sig < 64; sig++) {
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
    #endif

    runos = ub_new(0);
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
