// ptyrun: run an editor test script (t/edit/*.ed) against a shell on a
// pseudo-terminal, printing the screen snapshots the script asks for
// $ g++ -std=c++20 -static -O2 -o ptyrun tools/ptyrun.cpp
//
// usage: ptyrun SHELL SCRIPT.ed   (- for standard input)
//
// Runs SHELL --noprofile --rcfile $HOME/.edrc -i on a pty of the
// script's size, its environment the vos's for editor tests
// (main_test.cpp, edenv) plus the script's env lines, with rc lines
// written to $HOME/.edrc. Keys are written to the pty as the script
// says; output is drawn on a vterm (vterm.cpp), the same screen model
// the vos uses, and a snapshot taken once the shell has been quiet for
// a while. The last snapshot is at the end of the script or when the
// shell exits, whichever comes first, as in the vos. Output on stdout
// is a .screens file. Meant for the oracle container (tools/edrecord).
//
// This is free and unencumbered software released into the public domain.
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "../base.cpp"
#include "../unicode.cpp"
#include "../vterm.cpp"

[[noreturn]] static void os_oom()
{
    fprintf(stderr, "ptyrun: out of memory\n");
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}

enum { QUIET_MS = 300 };  // no output for this long: the shell waits

static i64 nowms()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (i64)ts.tv_sec*1000 + ts.tv_nsec/1000000;
}

static i32   master = -1;
static b32   exited;
static VTerm *term;

// Draw output until ms pass (quiet: until none arrives for ms), or the
// shell's side closes.
static void pump(i64 ms, b32 quiet)
{
    i64 deadline = nowms() + ms;
    while (!exited) {
        i64 left = deadline - nowms();
        if (left <= 0) return;
        pollfd p = {master, POLLIN, 0};
        i32 r = poll(&p, 1, (int)left);
        if (r < 0) {
            if (errno == EINTR) continue;
            return;
        }
        if (!r) return;
        u8 buf[4096];
        iz n = read(master, buf, sizeof(buf));
        if (n <= 0) {
            exited = 1;  // EIO: every slave descriptor closed
            return;
        }
        if (getenv("PTYRUN_RAW")) fwrite(buf, 1, (uz)n, stderr);
        vtfeed(term, buf, n);
        if (quiet) deadline = nowms() + ms;
    }
}

static i32 nsnaps;

static void snap(Arena scratch)
{
    Str s = vtdump(term, &scratch);
    printf("== snap %d\n%.*s", ++nsnaps, (int)s.len, (char *)s.data);
}

static void writeall(i32 fd, Str s)
{
    for (iz off = 0; off < s.len;) {
        iz n = write(fd, s.data+off, (uz)(s.len-off));
        if (n < 0) {
            if (errno == EINTR) continue;
            return;
        }
        off += n;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: ptyrun SHELL SCRIPT.ed\n");
        return 2;
    }
    iz    cap = (iz)1<<26;
    byte *mem = (byte *)malloc((uz)cap);
    Arena a   = {mem, mem+cap};

    FILE *f = strcmp(argv[2], "-") ? fopen(argv[2], "rb") : stdin;
    if (!f) {
        fprintf(stderr, "ptyrun: cannot open %s\n", argv[2]);
        return 2;
    }
    u8 *src = allocbytes(&a, 1<<20);
    iz  len = (iz)fread(src, 1, 1<<20, f);
    if (f != stdin) fclose(f);
    EdScript ed = edparse(&a, {src, len});
    if (ed.err.data) {
        fprintf(stderr, "ptyrun: %s: %.*s\n", argv[2], (int)ed.err.len, (char *)ed.err.data);
        return 2;
    }

    char *home = getenv("HOME");
    if (!home) home = (char *)"/tmp";
    static char rcpath[4096];
    snprintf(rcpath, sizeof(rcpath), "%s/.edrc", home);
    FILE *rc = fopen(rcpath, "wb");
    if (!rc) {
        fprintf(stderr, "ptyrun: cannot write %s\n", rcpath);
        return 2;
    }
    fwrite(ed.rc.data, 1, (uz)ed.rc.len, rc);
    fclose(rc);

    // The environment: as edenv in main_test.cpp, HOME the real one
    static char homevar[4200];
    snprintf(homevar, sizeof(homevar), "HOME=%s", home);
    char *env[64] = {
        (char *)"LANG=C.UTF-8", (char *)"TZ=UTC", homevar, (char *)"TERM=xterm",
        (char *)"PATH=/usr/local/bin:/opt/fakes/bin",
    };
    i32 nenv = 5;
    for (iz i = 0; i<ed.nenv && nenv<62; i++) {
        char *e = (char *)allocbytes(&a, ed.env[i].len+1);
        memcpy(e, ed.env[i].data, (uz)ed.env[i].len);
        e[ed.env[i].len] = 0;
        env[nenv++] = e;
    }

    master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master<0 || grantpt(master) || unlockpt(master)) {
        fprintf(stderr, "ptyrun: no pty\n");
        return 2;
    }
    winsize ws = {};
    ws.ws_row = (unsigned short)ed.rows;
    ws.ws_col = (unsigned short)ed.cols;
    ioctl(master, TIOCSWINSZ, &ws);
    char *slavename = ptsname(master);
    term = vtnew(&a, ed.rows, ed.cols, !ed.nowrap);

    pid_t pid = fork();
    if (!pid) {
        setsid();
        i32 s = open(slavename, O_RDWR);
        if (s < 0) _exit(126);
        ioctl(s, TIOCSCTTY, 0);
        dup2(s, 0);
        dup2(s, 1);
        dup2(s, 2);
        if (s > 2) close(s);
        close(master);
        char *args[] = {argv[1], (char *)"--noprofile", (char *)"--rcfile", rcpath,
                        (char *)"-i", 0};
        execve(argv[1], args, env);
        _exit(127);
    }

    pump(QUIET_MS, 1);
    for (iz i = 0; i<ed.nkeys && !exited; i++) {
        VKey *k = ed.keys + i;
        switch (k->kind) {
        case VKEY_TEXT:
            writeall(master, k->text);
            pump(20, 0);
            break;
        case VKEY_PAUSE:
            pump(k->ms, 0);
            break;
        case VKEY_SNAP:
            pump(QUIET_MS, 1);
            if (!exited) snap(a);
            break;
        case VKEY_RESIZE:
            pump(QUIET_MS, 1);
            ws.ws_row = (unsigned short)k->rows;
            ws.ws_col = (unsigned short)k->cols;
            vtresize(term, &a, k->rows, k->cols);
            ioctl(master, TIOCSWINSZ, &ws);  // SIGWINCH to the foreground
            break;
        }
    }
    pump(QUIET_MS, 1);
    snap(a);
    kill(pid, SIGKILL);
    waitpid(pid, 0, 0);
    return 0;
}
