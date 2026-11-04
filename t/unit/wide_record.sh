#!/bin/sh
# Record t/unit/wide.golden from the real command line parsers
#
#   sh t/unit/wide_record.sh SCRATCHDIR
#
# Windows only, run from w64devkit. Builds two throwaway programs in
# SCRATCHDIR: a native argv printer, which splits its own command line
# with CommandLineToArgvW, msvcrt.dll's __wgetmainargs and ucrtbase.dll's
# _configure_wide_argv, and a driver, which starts the printer and Git's
# msys printf.exe ($GITBASH's directory, printing '<%s>\n' for each
# argument) through CreateProcess with each command line of wide.cases
# verbatim. A shell would re-quote these lines, hence the driver.
#
# The children run in SCRATCHDIR/wd, emptied first, with a minimal
# environment (HOME=/home/u), so that unquoted wildcards do not match
# unless a "file" line of wide.cases created a file there.
#
# wide.cases lines (escapes as described in wide_test.cpp):
#   tail ARGS           both parsers: "p ARGS", and "printf <%s>\n X ARGS"
#   full CMDLINE        native parsers only, the whole command line
#   cyg ARGS            msys printf only
#   file NAME CONTENT   create SCRATCHDIR/wd/NAME
# The golden repeats each line, followed by one line per parser.
set -eu

dir=$(cd "$(dirname "$0")" && pwd)
work=${1:?usage: wide_record.sh SCRATCHDIR}
gitbash=${GITBASH:-C:/Users/wellons/git/usr/bin/bash.exe}
printf=$(dirname "$gitbash")/printf.exe

mkdir -p "$work"
rm -rf "$work/wd"
mkdir "$work/wd"

cat >"$work/argvp.c" <<'EOF'
// Print this process's command line as split by three native parsers
#include <windows.h>
#include <shellapi.h>

static char buf[1<<16];
static int  len;

static void putc_(int c)
{
    if (len < (int)sizeof(buf)) buf[len++] = (char)c;
}

static void puts_(char *s)
{
    for (; *s; s++) putc_(*s);
}

static void escaped(char *s, int n)
{
    for (int i = 0; i < n; i++) {
        unsigned char c = s[i];
        int run = 0;
        for (; i+run<n && s[i+run]=='a'; run++) {}
        if (run >= 16) {
            char tmp[16];
            wsprintfA(tmp, "^l%d;", run);
            puts_(tmp);
            i += run - 1;
            continue;
        }
        switch (c) {
        case '\t': puts_("^t"); break;
        case '\n': puts_("^n"); break;
        case '\r': puts_("^r"); break;
        case '\v': puts_("^v"); break;
        case '\f': puts_("^f"); break;
        case '^':  puts_("^^"); break;
        default:
            if (c<0x20 || c==0x7f) {
                char tmp[8];
                wsprintfA(tmp, "^x%02x", c);
                puts_(tmp);
            } else {
                putc_(c);
            }
        }
    }
}

static void emit(char *name, int argc, wchar_t **argv)
{
    static char u8[1<<16];
    puts_(name);
    for (int i = 0; i < argc; i++) {
        int n = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, u8, sizeof(u8), 0, 0);
        puts_(" <");
        escaped(u8, n>0 ? n-1 : 0);
        putc_('>');
    }
    putc_('\n');
}

int main(void)
{
    int       argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    emit("shell32", argc, argv);

    HMODULE m = LoadLibraryA("msvcrt.dll");
    int (*wgetmainargs)(int *, wchar_t ***, wchar_t ***, int, int *) =
        (void *)GetProcAddress(m, "__wgetmainargs");
    wchar_t **env;
    int       si = 0;
    wgetmainargs(&argc, &argv, &env, 0, &si);
    emit("msvcrt", argc, argv);

    HMODULE u = LoadLibraryA("ucrtbase.dll");
    int (*configure)(int) = (void *)GetProcAddress(u, "_configure_wide_argv");
    int *(*pargc)(void) = (void *)GetProcAddress(u, "__p___argc");
    wchar_t ***(*pwargv)(void) = (void *)GetProcAddress(u, "__p___wargv");
    configure(1);  // _crt_argv_unexpanded_arguments
    emit("ucrt", *pargc(), *pwargv());

    DWORD w;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, &w, 0);
    return 0;
}
EOF

cat >"$work/drive.c" <<'EOF'
// drive CASES PRINTER PRINTF WORKDIR: run each case, print the golden
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static char *printer, *printf_, *workdir;

static int unescape(char *dst, char *s)
{
    int n = 0;
    for (; *s; s++) {
        if (*s != '^') {
            dst[n++] = *s;
            continue;
        }
        switch (*++s) {
        case 't': dst[n++] = '\t'; break;
        case 'n': dst[n++] = '\n'; break;
        case 'r': dst[n++] = '\r'; break;
        case 'v': dst[n++] = '\v'; break;
        case 'f': dst[n++] = '\f'; break;
        case '^': dst[n++] = '^';  break;
        case 'x': {
            char hex[3] = {s[1], s[2], 0};
            dst[n++] = (char)strtol(hex, 0, 16);
            s += 2;
        } break;
        case 'l': {
            int count = (int)strtol(s+1, &s, 10);
            for (int i = 0; i < count; i++) dst[n++] = 'a';
        } break;
        default: fprintf(stderr, "bad escape ^%c\n", *s); exit(1);
        }
    }
    dst[n] = 0;
    return n;
}

static void escaped(char *s, int n)
{
    for (int i = 0; i < n; i++) {
        unsigned char c = s[i];
        int run = 0;
        for (; i+run<n && s[i+run]=='a'; run++) {}
        if (run >= 16) {
            printf("^l%d;", run);
            i += run - 1;
            continue;
        }
        switch (c) {
        case '\t': fputs("^t", stdout); break;
        case '\n': fputs("^n", stdout); break;
        case '\r': fputs("^r", stdout); break;
        case '\v': fputs("^v", stdout); break;
        case '\f': fputs("^f", stdout); break;
        case '^':  fputs("^^", stdout); break;
        default:
            if (c<0x20 || c==0x7f) {
                printf("^x%02x", c);
            } else {
                putchar(c);
            }
        }
    }
}

// Run app with the command line, returning its stdout and stderr.
static int run(char *app, char *cmd, char *out, int cap)
{
    static wchar_t wapp[1024], wcmd[1<<16], wdir[1024];
    MultiByteToWideChar(CP_UTF8, 0, app, -1, wapp, 1024);
    MultiByteToWideChar(CP_UTF8, 0, cmd, -1, wcmd, 1<<16);
    MultiByteToWideChar(CP_UTF8, 0, workdir, -1, wdir, 1024);

    static wchar_t env[] =
        L"HOME=/home/u\0"
        L"PATH=C:\\Windows\\system32\0"
        L"SYSTEMROOT=C:\\Windows\0";

    SECURITY_ATTRIBUTES sa = {sizeof(sa), 0, TRUE};
    HANDLE r, w;
    CreatePipe(&r, &w, &sa, 0);
    SetHandleInformation(r, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si = {sizeof(si)};
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = INVALID_HANDLE_VALUE;
    si.hStdOutput = w;
    si.hStdError  = w;
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(wapp, wcmd, 0, 0, TRUE, CREATE_UNICODE_ENVIRONMENT,
                        env, wdir, &si, &pi)) {
        fprintf(stderr, "CreateProcess failed: %lu\n", GetLastError());
        exit(1);
    }
    CloseHandle(w);
    int   len = 0;
    DWORD n;
    while (ReadFile(r, out+len, cap-len, &n, 0) && n) len += n;
    CloseHandle(r);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return len;
}

static void native(char *cmd)
{
    static char out[1<<17];
    int len = run(printer, cmd, out, sizeof(out));
    fwrite(out, 1, len, stdout);
}

static void cygwin(char *args)
{
    static char cmd[1<<17], out[1<<17];
    snprintf(cmd, sizeof(cmd), "\"%s\" <%%s>\\n X %s", printf_, args);
    int len = run(printf_, cmd, out, sizeof(out));
    char *p = out;
    if (len>=4 && !memcmp(out, "<X>\n", 4)) {
        p += 4;
        len -= 4;
    } else {
        fputs("(no sentinel) ", stdout);
    }
    fputs("cygwin ", stdout);
    escaped(p, len);
    putchar('\n');
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: drive CASES PRINTER PRINTF WORKDIR\n");
        return 1;
    }
    _setmode(1, _O_BINARY);
    printer = argv[2];
    printf_ = argv[3];
    workdir = argv[4];
    FILE *f = fopen(argv[1], "rb");
    static char line[1<<16], raw[1<<17], cmd[1<<17];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        fputs(line, stdout);
        putchar('\n');
        if (!line[0] || line[0]=='#') continue;
        char *sp   = strchr(line, ' ');
        char *rest = sp ? sp+1 : line+strlen(line);
        if (sp) *sp = 0;
        unescape(raw, rest);
        if (!strcmp(line, "tail")) {
            snprintf(cmd, sizeof(cmd), "p %s", raw);
            native(cmd);
            cygwin(raw);
        } else if (!strcmp(line, "full")) {
            native(raw);
        } else if (!strcmp(line, "cyg")) {
            cygwin(raw);
        } else if (!strcmp(line, "file")) {
            char *name = rest, *content = strchr(rest, ' ');
            *content++ = 0;
            int  n = unescape(raw, content);
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", workdir, name);
            FILE *o = fopen(path, "wb");
            fwrite(raw, 1, n, o);
            fclose(o);
        } else {
            fprintf(stderr, "bad line: %s\n", line);
            return 1;
        }
        fflush(stdout);
    }
    return 0;
}
EOF

gcc -O -o "$work/argvp.exe" "$work/argvp.c" -lshell32
gcc -O -o "$work/drive.exe" "$work/drive.c"
"$work/drive.exe" "$dir/wide.cases" "$work/argvp.exe" "$printf" "$work/wd" \
    >"$dir/wide.golden"
