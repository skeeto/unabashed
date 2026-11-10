// ubtiny: the smallest Windows host of libunabashed, as an embedder's
//
// CRT-free, with no root, no self and no .unabash marker: the library
// must never take this image for a shell. It runs echo, a #!/bin/sh
// script (in-process, with no self), sh -c (busybox's sh by PATH) and
// ub_system, and through the child hook confirms that no program it
// starts is this image. Exits 0 when all is well.
//
// $ i686-w64-mingw32-gcc -O2 -nostartfiles -nostdlib -o ubtiny.exe
//       tools/ubtiny.c libunabashed32.obj -lkernel32 -lgcc
// This is free and unencumbered software released into the public domain.
#include <stddef.h>
#include <stdint.h>
#include "../unabashed.h"

#define W32(r, p) __declspec(dllimport) r __stdcall p
typedef struct {
    unsigned  size, usage, pid;
    uintptr_t heap;
    unsigned  module, threads, ppid;
    int       pri;
    unsigned  flags;
    uint16_t  exe[260];
} ProcEntry;  // PROCESSENTRY32W

W32(void,      ExitProcess(unsigned)) __attribute((noreturn));
W32(uintptr_t, GetStdHandle(int));
W32(int,       WriteFile(uintptr_t, void const *, int, int *, void *));
W32(int,       GetModuleFileNameW(uintptr_t, uint16_t *, int));
W32(int,       GetTempPathW(int, uint16_t *));
W32(uintptr_t, CreateToolhelp32Snapshot(int, int));
W32(int,       Process32FirstW(uintptr_t, ProcEntry *));
W32(int,       Process32NextW(uintptr_t, ProcEntry *));
W32(int,       CloseHandle(uintptr_t));

#pragma GCC optimize("no-tree-loop-distribute-patterns")

void *memcpy(void *restrict d, void const *restrict s, size_t n)
{
    unsigned char *dd = d;
    unsigned char const *ss = s;
    for (size_t i = 0; i < n; i++) dd[i] = ss[i];
    return d;
}

void *memmove(void *d, void const *s, size_t n)
{
    unsigned char *dd = d;
    unsigned char const *ss = s;
    if (dd < ss) {
        for (size_t i = 0; i < n; i++) dd[i] = ss[i];
    } else {
        for (size_t i = n; i; i--) dd[i-1] = ss[i-1];
    }
    return d;
}

void *memset(void *d, int c, size_t n)
{
    unsigned char *dd = d;
    for (size_t i = 0; i < n; i++) dd[i] = (unsigned char)c;
    return d;
}

int memcmp(void const *a, void const *b, size_t n)
{
    unsigned char const *x = a, *y = b;
    for (size_t i = 0; i < n; i++) {
        if (x[i] != y[i]) return x[i] - y[i];
    }
    return 0;
}

void *memchr(void const *s, int c, size_t n)
{
    unsigned char const *p = s;
    for (size_t i = 0; i < n; i++) {
        if (p[i] == (unsigned char)c) return (void *)(p + i);
    }
    return 0;
}

size_t strlen(char const *s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void say(char const *s)
{
    int n = 0;
    WriteFile(GetStdHandle(-11), s, (int)strlen(s), &n, 0);
}

static uint16_t selfname[260];  // this image's file name, lowercase
static int      selfprograms;   // programs started with it

static uint16_t lower(uint16_t c)
{
    return c>='A' && c<='Z' ? (uint16_t)(c + 32) : c;
}

// Each program the run starts: its image is never this one.
static void child(void *user, int64_t pid, int live)
{
    (void)user;
    if (!live) return;
    uintptr_t snap = CreateToolhelp32Snapshot(2, 0);  // TH32CS_SNAPPROCESS
    ProcEntry pe = {0};
    pe.size = sizeof(pe);
    for (int ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        if (pe.pid != (unsigned)pid) continue;
        int i = 0;
        for (; selfname[i] && lower(pe.exe[i])==selfname[i]; i++) {}
        selfprograms += !selfname[i] && !pe.exe[i];
    }
    CloseHandle(snap);
}

// UTF-16 to ASCII, enough for a temporary directory in this test.
static char *narrow(char *buf, uint16_t const *s, int n)
{
    for (int i = 0; i < n; i++) buf[i] = s[i]<0x80 ? (char)s[i] : '?';
    buf[n] = 0;
    return buf;
}

static char const script[] =
    "echo echo ok\n"
    "printf '#!/bin/sh\\necho script ok $#\\n' >ubtiny.sh\n"
    "chmod +x ubtiny.sh\n"
    "./ubtiny.sh a b || exit 1\n"
    "sh -c 'echo sh ok' || exit 2\n"
    "rm ubtiny.sh\n"
    "printf '#!/bin/sh -e\\nfalse\\necho no\\n' >ubtiny2.sh\n"
    "chmod +x ubtiny2.sh\n"
    "./ubtiny2.sh >ubtiny2.out && exit 4\n"
    "[ -s ubtiny2.out ] && exit 5\n"
    "echo script -e ok\n"
    "printf 'echo via $#\\n' >ubtiny3.sh\n"
    "chmod +x ubtiny3.sh\n"
    "[ \"$(./ubtiny3.sh a)\" = 'via 1' ] || exit 6\n"
    "case $PWD in */) exit 7 ;; esac\n"
    "echo plain script ok\n"
    "rm ubtiny2.sh ubtiny2.out ubtiny3.sh\n"
    "case $BASH in *[Uu][Bb][Tt][Ii][Nn][Yy].[Ee][Xx][Ee]) exit 3 ;; esac\n"
    "exit 0\n";

__attribute((force_align_arg_pointer))
void mainCRTStartup(void)
{
    uint16_t path[260];
    int len = GetModuleFileNameW(0, path, 260);
    int base = len;
    for (; base && path[base-1]!='\\' && path[base-1]!='/'; base--) {}
    for (int i = base; i < len; i++) selfname[i-base] = lower(path[i]);

    uint16_t tmp[260];
    char cwd[260];
    int ntmp = GetTempPathW(260, tmp);
    if (ntmp>0 && ntmp<260) narrow(cwd, tmp, ntmp);

    ub_config cfg = {0};
    cfg.child = child;
    ub_os *os = ub_new(&cfg);
    if (!os) {
        say("ubtiny: ub_new failed\n");
        ExitProcess(2);
    }
    char const *argv[] = {"bash", "-c", script, 0};
    ub_cmd cmd = {0};
    cmd.argc   = 3;
    cmd.argv   = argv;
    cmd.cwd    = ntmp>0 && ntmp<260 ? cwd : 0;
    cmd.std[0] = -1;
    cmd.std[1] = (intptr_t)GetStdHandle(-11);
    cmd.std[2] = (intptr_t)GetStdHandle(-12);
    int status = ub_run(os, &cmd);
    ub_free(os);
    int sys = ub_system("echo system ok");

    int fails = 0;
    if (status) {
        char num[16], *e = num+sizeof(num), *q = e;
        *--q = 0;
        unsigned u = status<0 ? -(unsigned)status : (unsigned)status;
        do *--q = (char)('0' + u%10); while (u /= 10);
        if (status < 0) *--q = '-';
        say("ubtiny: FAIL the run's status is ");
        say(q);
        say(", not 0\n");
        fails++;
    }
    if (sys) {
        say("ubtiny: FAIL ub_system's status is not 0\n");
        fails++;
    }
    if (selfprograms) {
        say("ubtiny: FAIL a program started as this image\n");
        fails++;
    }
    say(fails ? "ubtiny: failed\n" : "ubtiny: ok\n");
    ExitProcess(fails ? 1 : 0);
}
