// Unit tests for paths.cpp against Git Bash recordings, plus property
// loops on random input and native-mode expectations
// $ g++ -std=c++20 -g3 -Wall -Wextra -Wconversion -Wno-sign-conversion
//       -fsanitize=undefined -fsanitize-trap -o paths_test.exe
//       t/unit/paths_test.cpp
// $ ./paths_test.exe [t/unit/paths.golden [SEED]]
//
// The golden file is recorded from t/unit/paths.cases by
// t/unit/paths_record.sh, which documents its format. Its header holds
// the recording machine's context (install root, %TEMP%, working
// directory, drives, fstab), which the test rebuilds, so it runs on any
// host. Run from the repository root, or pass the golden file's path.
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"  // base.cpp's spares
#include "../../base.cpp"
#pragma GCC diagnostic pop
#include "../../paths.cpp"

[[noreturn]] static void os_oom()
{
    fputs("paths_test: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}


// Reporting

static i32 ntests;
static i32 nfailed;

static void show(char const *label, Str s)
{
    fprintf(stderr, "    %-6s \"", label);
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c == '\\') {
            fputs("\\\\", stderr);
        } else if (c>=0x20 && c<0x7f) {
            fputc(c, stderr);
        } else {
            fprintf(stderr, "\\x%02x", c);
        }
    }
    fputs(s.data ? "\"\n" : "\" (null)\n", stderr);
}

static b32 check(b32 ok, iz line, char const *what)
{
    ntests++;
    if (!ok) {
        nfailed++;
        fprintf(stderr, "FAIL line %td: %s\n", line, what);
    }
    return ok;
}

static b32 expect(Str got, Str want, iz line, char const *what, Str input)
{
    b32 ok = got==want && !got.data==!want.data;
    if (!check(ok, line, what)) {
        show("input", input);
        show("want", want);
        show("got", got);
    }
    return ok;
}


// Golden file

static i32 hexval(u8 c)
{
    return c<='9' ? c-'0' : (c|0x20) - 'a' + 10;
}

static Str decode(Arena *a, Str s)
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        if (s[i]=='\\' && i+1<s.len && s[i+1]=='\\') {
            putbyte(&b, '\\');
            i++;
        } else if (s[i]=='\\' && i+3<s.len && s[i+1]=='x') {
            putbyte(&b, (u8)(hexval(s[i+2])<<4 | hexval(s[i+3])));
            i += 3;
        } else {
            putbyte(&b, s[i]);
        }
    }
    Str r = finish(&b);
    r.data = r.data ? r.data : (u8 *)"";
    return r;
}

struct Record {
    Str        kind;
    Str        excl;
    Slice<Str> in;
    Slice<Str> out;
    iz         line;
};

struct Golden {
    Str           root;
    Str           temp;
    Str           cwd;
    Str           drives;
    Str           fstab;
    Slice<Record> records;
};

static Golden parse(Arena *a, Str src)
{
    Golden g   = {};
    Record cur = {};
    iz     n   = 0;
    Buf    fstab(a, 1<<10);
    for (Cut c = {.head={}, .tail=src, .ok=1}; c.ok;) {
        c = cut(c.tail, '\n');
        n++;
        Str line = c.head;
        if (line.len && line[line.len-1]=='\r') line.len--;
        if (!line.len) continue;
        Cut kv = cut(line, ' ');
        Str key = kv.head;
        Str val = decode(a, kv.tail);
        if (key == "@root") {
            g.root = val;
        } else if (key == "@temp") {
            g.temp = val;
        } else if (key == "@cwd") {
            g.cwd = val;
        } else if (key == "@drives") {
            g.drives = val;
        } else if (key == "@fstab") {
            print(&fstab, val);
            putbyte(&fstab, '\n');
        } else if (key == "T") {
            cur      = {};
            cur.kind = val;
            cur.line = n;
        } else if (key == "X") {
            cur.excl = val;
        } else if (key == "I") {
            cur.in = push(a, cur.in, val);
        } else if (key == "O") {
            cur.out = push(a, cur.out, val);
        } else if (key == ".") {
            g.records = push(a, g.records, cur);
        }
    }
    g.fstab = finish(&fstab);
    return g;
}

// /proc/N/ becomes /proc/PID/, as the recorder writes it
static Str procpid(Arena *a, Str s)
{
    iz i = find(s, "/proc/");
    if (i < 0) return s;
    iz j = i + 6;
    iz k = j;
    for (; k<s.len && digit(s[k]); k++) {}
    if (k==j || k==s.len || s[k]!='/') return s;
    Buf b(a, s.len+3);
    print(&b, takehead(s, j));
    print(&b, "PID");
    print(&b, cuthead(s, k));
    return finish(&b);
}

static PathCtx goldenctx(Arena *a, Golden *g)
{
    PathCtx ctx = {};
    ctx.mode = PM_MSYS;
    Str root = g->root;
    if (root.len>3 && root[root.len-1]=='/') root.len--;
    ctx.temp = g->temp;
    ctx.cwd  = g->cwd;
    ctx.pid  = 1234;
    for (iz i = 0; i < g->drives.len; i++) {
        ctx.drives |= 1u << (upcase(g->drives[i]) - 'A');
    }
    initmounts(&ctx, a, root, g->fstab);
    return ctx;
}

static void testrecord(PathCtx *ctx, Arena scratch, Record *r)
{
    Arena *a = &scratch;
    Str    k = r->kind;
    if (k=="u" || k=="w" || k=="m" || k=="up" || k=="wp" || k=="mp") {
        Str in  = r->in[0];
        Str got = {};
        if      (k == "u")  got = toposix(ctx, a, in);
        else if (k == "w")  got = towin(ctx, a, in, 1);
        else if (k == "m")  got = towin(ctx, a, in, 0);
        else if (k == "up") got = toposixlist(ctx, a, in);
        else if (k == "wp") got = towinlist(ctx, a, in, 1);
        else if (k == "mp") got = towinlist(ctx, a, in, 0);
        Str want = r->out.len ? r->out[0] : Str("");
        if (!got.data) got = "";
        char what[32];
        snprintf(what, sizeof(what), "cygpath -%.*s", (int)k.len, k.cdata);
        expect(got, want, r->line, what, in);

        // The same through the cygpath builtin's argument parsing
        Str     args[] = {"cygpath", concat(a, "-", k), "--", in};
        CygOpts o      = cygpathopts(a, args, countof(args));
        CygRun  run    = cygpathrun(ctx, a, &o, args, countof(args), {}, {});
        Str     line   = concat(a, want, "\n");
        b32     ok     = o.ok && ((run.out==line && !run.status) ||
                                  (!want.len && run.out=="" && run.status==1));
        snprintf(what, sizeof(what), "builtin cygpath -%.*s", (int)k.len, k.cdata);
        if (!check(ok, r->line, what)) {
            show("input", in);
            show("want", line);
            show("got", run.out);
        }

    } else if (k == "arg") {
        Str in  = r->in[0];
        Str got = procpid(a, msysarg(ctx, a, in, {}));
        expect(got, r->out[0], r->line, "arg", in);

    } else if (k == "excl") {
        Slice<Str> env = {};
        env = push(a, env, concat(a, Str("MSYS2_ARG_CONV_EXCL="), r->excl));
        Slice<Str> got = msysargv(ctx, a, r->in, env);
        b32 ok = got.len == r->out.len;
        for (iz i = 0; ok && i < got.len; i++) {
            ok = got[i] == r->out[i];
        }
        if (!check(ok, r->line, "excl")) {
            show("excl", r->excl);
            for (iz i = 0; i < got.len; i++) show("got", got[i]);
        }

    } else if (k == "env") {
        Slice<Str> got = msysenv(ctx, a, r->in);
        b32 ok = got.len == r->out.len;
        for (iz i = 0; ok && i < got.len; i++) {
            ok = got[i] == r->out[i];
        }
        if (!check(ok, r->line, "env")) {
            for (iz i = 0; i < r->in.len; i++)  show("in", r->in[i]);
            for (iz i = 0; i < r->out.len; i++) show("want", r->out[i]);
            for (iz i = 0; i < got.len; i++)    show("got", got[i]);
        }

    } else {
        check(0, r->line, "unknown record kind");
    }
}


// Expectations that cannot be recorded

static PathRes resolve(PathCtx *ctx, Arena scratch, Str path)
{
    return topath(ctx, &scratch, path);
}

static void want(PathCtx *ctx, Arena scratch, Str path, i32 kind, Str native)
{
    PathRes r = topath(ctx, &scratch, path);
    b32 ok = r.ok && r.kind==kind && r.native==native;
    if (!check(ok, 0, "topath")) {
        show("input", path);
        show("want", native);
        show("got", r.native);
        fprintf(stderr, "    kind   want %d got %d, ok %d\n", kind, r.kind, r.ok);
    }
}

static void wantfd(PathCtx *ctx, Arena scratch, Str path, i32 kind, i64 num, Str sub)
{
    PathRes r = topath(ctx, &scratch, path);
    b32 ok = r.ok && r.kind==kind && r.num==num && r.sub==sub;
    if (!check(ok, 0, "topath virtual")) {
        show("input", path);
        fprintf(stderr, "    kind %d num %lld ok %d\n", r.kind, (long long)r.num, r.ok);
        show("sub", r.sub);
    }
}

static void wantmissing(PathCtx *ctx, Arena scratch, Str path)
{
    PathRes r = topath(ctx, &scratch, path);
    if (!check(!r.ok, 0, "topath missing")) {
        show("input", path);
        show("got", r.native);
    }
}

static void testnative(Arena scratch)
{
    PathCtx ctx = {};
    ctx.mode = PM_NATIVE;
    ctx.cwd  = "C:/work/dir";
    ctx.drivecwd['D'-'A'] = "D:/d/cwd";
    Arena a = scratch;

    want(&ctx, a, "/x",              PK_FILE, "C:/x");
    want(&ctx, a, "/x/",             PK_FILE, "C:/x/");
    want(&ctx, a, "\\x\\y",          PK_FILE, "C:/x/y");
    want(&ctx, a, "x",               PK_FILE, "C:/work/dir/x");
    want(&ctx, a, "../x",            PK_FILE, "C:/work/x");
    want(&ctx, a, "../../../..",     PK_FILE, "C:/");
    want(&ctx, a, ".",               PK_FILE, "C:/work/dir/");
    want(&ctx, a, "D:y",             PK_FILE, "D:/d/cwd/y");
    want(&ctx, a, "d:",              PK_FILE, "D:/d/cwd");
    want(&ctx, a, "E:",              PK_FILE, "E:/");
    want(&ctx, a, "C:",              PK_FILE, "C:/work/dir");
    want(&ctx, a, "c:\\A\\.\\b\\",   PK_FILE, "C:/A/b/");
    want(&ctx, a, "C:/a//b/../c",    PK_FILE, "C:/a/c");
    want(&ctx, a, "/c/x",            PK_FILE, "C:/c/x");
    want(&ctx, a, "/tmp",            PK_FILE, "C:/tmp");
    want(&ctx, a, "///x",            PK_FILE, "C:/x");
    want(&ctx, a, "//srv/share/a/../b", PK_FILE, "//srv/share/b");
    want(&ctx, a, "\\\\srv\\share",  PK_FILE, "//srv/share");
    want(&ctx, a, "//srv/share/..",  PK_FILE, "//srv/share/");
    want(&ctx, a, "\\\\?\\C:\\x",    PK_FILE, "C:/x");
    want(&ctx, a, "\\\\?\\UNC\\srv\\share\\x", PK_FILE, "//srv/share/x");
    want(&ctx, a, "//./pipe/x",      PK_FILE, "//./pipe/x");
    want(&ctx, a, "\\\\.\\C:\\x",    PK_FILE, "C:/x");
    want(&ctx, a, "/dev/null",       PK_NULL, "//./NUL");
    want(&ctx, a, "/dev//null",      PK_NULL, "//./NUL");
    want(&ctx, a, "/dev/tty",        PK_TTY,  "//./CON");
    want(&ctx, a, "nul",             PK_NULL, "//./NUL");
    want(&ctx, a, "NUL.txt",         PK_NULL, "//./NUL");
    want(&ctx, a, "nul .txt",        PK_NULL, "//./NUL");
    want(&ctx, a, "nul. . ",         PK_NULL, "//./NUL");
    want(&ctx, a, "x/Nul",           PK_NULL, "//./NUL");
    want(&ctx, a, "C:/a/nul.tar.gz", PK_NULL, "//./NUL");
    want(&ctx, a, "con",             PK_TTY,  "//./CON");
    want(&ctx, a, "com1",            PK_DEVICE, "//./COM1");
    want(&ctx, a, "lpt9.log",        PK_DEVICE, "//./LPT9");
    want(&ctx, a, "x/aux.c",         PK_DEVICE, "//./AUX");
    want(&ctx, a, "conout$",         PK_DEVICE, "//./CONOUT$");
    want(&ctx, a, "//./NUL",         PK_NULL, "//./NUL");
    want(&ctx, a, "\\\\.\\COM2",     PK_DEVICE, "//./COM2");
    want(&ctx, a, "nully",           PK_FILE, "C:/work/dir/nully");
    want(&ctx, a, "com0",            PK_FILE, "C:/work/dir/com0");
    want(&ctx, a, "com10",           PK_FILE, "C:/work/dir/com10");
    want(&ctx, a, "nul/x",           PK_FILE, "C:/work/dir/nul/x");
    want(&ctx, a, "/dev/random",     PK_FILE, "C:/dev/random");
    want(&ctx, a, "/proc/self",      PK_FILE, "C:/proc/self");
    wantfd(&ctx, a, "/dev/fd/3",     PK_FD, 3, {});
    wantfd(&ctx, a, "/dev/stderr",   PK_FD, 2, {});
    wantfd(&ctx, a, "/dev/zero",     PK_DEV, 0, "zero");
    wantfd(&ctx, a, "/dev/urandom",  PK_DEV, 0, "urandom");
    wantmissing(&ctx, a, "");
    wantmissing(&ctx, a, "/dev/null/x");
    wantmissing(&ctx, a, "/dev/fd/x");

    // A literal flag is msys-only
    PathRes r = resolve(&ctx, a, "x.");
    check(r.ok && !r.literal && r.native=="C:/work/dir/x.", 0, "native x.");

    // On a share
    ctx.cwd = "//srv/share/d";
    want(&ctx, a, "/x",              PK_FILE, "//srv/share/x");
    want(&ctx, a, "..\\..\\..",      PK_FILE, "//srv/share/");
    want(&ctx, a, "y",               PK_FILE, "//srv/share/d/y");
    want(&ctx, a, "C:y",             PK_FILE, "C:/y");
}

static void testmsys(Arena scratch)
{
    PathCtx ctx = {};
    ctx.mode = PM_MSYS;
    ctx.temp = "C:/Users/u/AppData/Local/Temp";
    ctx.cwd  = "C:/Users/u/AppData/Local/Temp/work";
    ctx.pid  = 77;
    Arena a = scratch;
    Str fstab =
        "# comment\r\n"
        "none / cygdrive binary,posix=0,noacl,user 0 0\r\n"
        "none /tmp usertemp binary,posix=0,noacl 0 0\n"
        "  D:/Data\\040Files   /data   ntfs binary,posix=0 0 0\n"
        "e:/three /three ntfs\n"
        "e:/bad /bad ntfs binary,bogus 0 0\n"
        "e:/two /two\n"
        "c:/other / ntfs binary 0 0\n"
        "C:/x/usr/share /usr/share ntfs binary,posix=1 0 0\n";
    initmounts(&ctx, &a, "C:/x", fstab);

    i32 nmounts = 0;
    for (iz i = 0; i < ctx.mounts.len; i++) {
        Mount *m = &ctx.mounts[i];
        if (m->posix=="/" && m->native=="C:/x") nmounts++;
        if (m->posix=="/tmp" && m->native=="C:/Users/u/AppData/Local/Temp"
                && m->flags==(MNT_NOCASE|MNT_IMMUTABLE)) nmounts++;
        if (m->posix=="/data" && m->native=="D:/Data Files"
                && m->flags==MNT_NOCASE) nmounts++;
        if (m->posix=="/two" && m->native=="E:/two" && m->flags==MNT_NOCASE) nmounts++;
        if (m->posix=="/usr/share" && !m->flags) nmounts++;
        if (m->posix=="/bin" && m->native=="C:/x/usr/bin" && !m->flags) nmounts++;
    }
    check(nmounts==6 && ctx.mounts.len==6, 0, "fstab mounts");
    check(ctx.cygdrive == "/", 0, "fstab cygdrive");

    want(&ctx, a, "/c/x",            PK_FILE, "C:/x");
    want(&ctx, a, "/c",              PK_FILE, "C:/");
    want(&ctx, a, "/",               PK_FILE, "C:/x/");
    want(&ctx, a, "/usr/bin/ls",     PK_FILE, "C:/x/usr/bin/ls");
    want(&ctx, a, "/bin/ls",         PK_FILE, "C:/x/usr/bin/ls");
    want(&ctx, a, "/BIN/ls",         PK_FILE, "C:/x/BIN/ls");
    want(&ctx, a, "/TMP/a",          PK_FILE, "C:/Users/u/AppData/Local/Temp/a");
    want(&ctx, a, "/DATA/a",         PK_FILE, "D:/Data Files/a");
    want(&ctx, a, "/usr/share/x",    PK_FILE, "C:/x/usr/share/x");
    want(&ctx, a, "x",               PK_FILE, "C:/Users/u/AppData/Local/Temp/work/x");
    want(&ctx, a, "..",              PK_FILE, "C:/Users/u/AppData/Local/Temp/");
    want(&ctx, a, "../..",           PK_FILE, "C:/x/");
    want(&ctx, a, "../../c/y",       PK_FILE, "C:/y");
    want(&ctx, a, "C:",              PK_FILE, "C:/");
    want(&ctx, a, "c:y",             PK_FILE, "C:/y");
    want(&ctx, a, "C:\\a\\..\\b",    PK_FILE, "C:/b");
    want(&ctx, a, "\\a",             PK_FILE, "C:/a");
    want(&ctx, a, "//srv/share/x",   PK_FILE, "//srv/share/x");
    want(&ctx, a, "/dev/null",       PK_NULL, "//./NUL");
    want(&ctx, a, "/dev/tty",        PK_TTY,  "//./CON");
    want(&ctx, a, "/dev/console",    PK_TTY,  "//./CON");
    want(&ctx, a, "nul",             PK_NULL, "//./NUL");
    want(&ctx, a, "/c/t/NUL",        PK_NULL, "//./NUL");
    want(&ctx, a, "/Dev/null",       PK_FILE, "C:/x/Dev/null");
    want(&ctx, a, "/dev/xyz",        PK_FILE, "C:/x/dev/xyz");
    want(&ctx, a, "/dev",            PK_FILE, "C:/x/dev");
    want(&ctx, a, "/proc/cygdrive/d/x", PK_FILE, "D:/x");
    want(&ctx, a, "/proc/cygdrive",  PK_FILE, "C:/x/");
    want(&ctx, a, "/PROC",           PK_FILE, "C:/x/PROC");
    wantfd(&ctx, a, "/dev/fd/5",     PK_FD, 5, {});
    wantfd(&ctx, a, "/dev/stdin",    PK_FD, 0, {});
    wantfd(&ctx, a, "/proc/self/fd/2", PK_FD, 2, {});
    wantfd(&ctx, a, "/proc/77/fd/4", PK_FD, 4, {});
    wantfd(&ctx, a, "/proc/12/fd/4", PK_PROC, 12, "fd/4");
    wantfd(&ctx, a, "/proc/123/comm", PK_PROC, 123, "comm");
    wantfd(&ctx, a, "/proc/self/comm", PK_PROC, -1, "comm");
    wantfd(&ctx, a, "/proc/self",    PK_PROC, -1, {});
    wantfd(&ctx, a, "/proc",         PK_PROC, 0, {});
    wantfd(&ctx, a, "/proc/",        PK_PROC, 0, {});
    wantfd(&ctx, a, "/proc/cpuinfo", PK_PROC, 0, "cpuinfo");
    wantfd(&ctx, a, "/dev/clipboard", PK_DEV, 0, "clipboard");
    wantfd(&ctx, a, "/dev/fd",       PK_DEV, 0, "fd");
    wantmissing(&ctx, a, "/dev/null/");
    wantmissing(&ctx, a, "/dev/fd/x");

    // Names Win32 would alter are opened literally
    PathRes r = resolve(&ctx, a, "nul.txt");
    check(r.ok && r.kind==PK_FILE && r.literal, 0, "msys nul.txt");
    r = resolve(&ctx, a, "/c/a./b");
    check(r.ok && r.literal && r.native=="C:/a./b", 0, "msys a.");
    r = resolve(&ctx, a, "/c/a /b");
    check(r.ok && r.literal, 0, "msys trailing space");
    r = resolve(&ctx, a, "/c/a.b/c");
    check(r.ok && !r.literal, 0, "msys a.b");

    // A cygdrive prefix other than /
    ctx.cygdrive = "/cygdrive/";
    want(&ctx, a, "/cygdrive/c/x",   PK_FILE, "C:/x");
    want(&ctx, a, "/c/x",            PK_FILE, "C:/x/c/x");
    wantfd(&ctx, a, "/cygdrive",     PK_CYGDRIVE, 0, {});
    wantfd(&ctx, a, "/proc/cygdrive", PK_CYGDRIVE, 0, {});
    wantmissing(&ctx, a, "/cygdrive/xy");
    Str u = toposix(&ctx, &a, "C:\\y");
    expect(u, "/cygdrive/c/y", 0, "cygdrive prefix", "C:\\y");
    ctx.cygdrive = "/";

    // Startup import of Windows-form variables
    Str cases[][3] = {
        {"PATH", "C:\\a;\"C:\\b c\";;C:\\d\\\\;C:\\x\\usr\\bin", "/c/a:/c/b c:/c/d:/usr/bin"},
        {"ORIGINAL_PATH", "rel;C:\\Users\\u\\AppData\\Local\\Temp", "rel:/tmp"},
        {"LD_LIBRARY_PATH", "\"C:\\a\";;D:\\b", "\"C:/a\":/d/b"},
        {"HOME", "C:\\Users\\u", "/c/Users/u"},
        {"HOME", "/already", "/already"},
        {"TMP", "sub", "/tmp/work/sub"},
        {"SHELL", "", ""},
        {"FOO", "C:\\x", "C:\\x"},
    };
    for (iz i = 0; i < countof(cases); i++) {
        Str got = envtoposix(&ctx, &a, cases[i][0], cases[i][1]);
        expect(got, cases[i][2], 0, "envtoposix", cases[i][1]);
    }

    // Environment details not worth a recording
    Slice<Str> env = {};
    env = push(&a, env, Str("!C:=C:\\x"));
    env = push(&a, env, Str("=D:=D:\\y"));
    env = push(&a, env, Str("PATH=/usr/bin"));
    Slice<Str> got = msysenv(&ctx, &a, env);
    check(got.len==2 && got[0]=="=C:=C:\\x" && got[1]=="PATH=C:\\x\\usr\\bin",
          0, "msysenv =C:");

    // A bind mount and an override of the root
    PathCtx b = {};
    b.mode = PM_MSYS;
    initmounts(&b, &a, "C:/r", "C:/s / ntfs binary 0 0\n"
                               "/usr/lib /lib ntfs bind 0 0\n");
    want(&b, a, "/x",   PK_FILE, "C:/r/x");
    want(&b, a, "/lib/a", PK_FILE, "C:/r/usr/lib/a");
    want(&b, a, "/cygdrive/e", PK_FILE, "E:/");
    initmounts(&b, &a, "C:/r", "C:/s / ntfs binary,override 0 0\n");
    want(&b, a, "/x",   PK_FILE, "C:/s/x");
    want(&b, a, "/bin", PK_FILE, "C:/r/usr/bin");
}

static void testposix(Arena scratch)
{
    PathCtx ctx = {};
    ctx.cwd = "/home/u";
    want(&ctx, scratch, "x",      PK_FILE, "/home/u/x");
    want(&ctx, scratch, "../x",   PK_FILE, "/home/u/../x");
    want(&ctx, scratch, "/a/../b", PK_FILE, "/a/../b");
    want(&ctx, scratch, "/dev/null", PK_FILE, "/dev/null");
    want(&ctx, scratch, "C:\\x",  PK_FILE, "/home/u/C:\\x");
    ctx.cwd = "/";
    want(&ctx, scratch, "x",      PK_FILE, "/x");
    wantmissing(&ctx, scratch, "");
}

static void testpersona()
{
    struct { Str argv0; b32 fstab; PathMode want; } cases[] = {
        {"bash", 0, PM_MSYS},
        {"C:\\git\\usr\\bin\\bash.exe", 1, PM_MSYS},
        {"sh", 0, PM_NATIVE},
        {"SH.EXE", 0, PM_NATIVE},
        {"C:/w64devkit/bin/ash.exe", 0, PM_NATIVE},
        {"lash", 0, PM_NATIVE},
        {"C:/git/usr/bin/sh.exe", 1, PM_MSYS},
        {"C:sh", 0, PM_NATIVE},
        {"unabashed.exe", 0, PM_MSYS},
        {"cygpath", 0, PM_MSYS},
        {"shx", 0, PM_MSYS},
        {"", 0, PM_MSYS},
    };
    for (iz i = 0; i < countof(cases); i++) {
        PathMode got = personaof(cases[i].argv0, cases[i].fstab);
        if (!check(got == cases[i].want, 0, "personaof")) {
            show("argv0", cases[i].argv0);
        }
    }
}


// Property loops on random input

static Str mountnative(PathCtx *ctx, Str posix)
{
    for (iz i = 0; i < ctx->mounts.len; i++) {
        if (ctx->mounts[i].posix == posix) return ctx->mounts[i].native;
    }
    return {};
}

static u64 rng = 1;

static u32 rand32()
{
    rng = rng*0x3243f6a8885a308d + 1;
    return (u32)(rng >> 32);
}

static Str randstr(Arena *a, Str alphabet, iz maxlen)
{
    iz  len = rand32() % (maxlen + 1);
    Str r   = {allocbytes(a, len), len};
    for (iz i = 0; i < len; i++) {
        r[i] = alphabet[rand32() % alphabet.len];
    }
    return r;
}

// A random path component without dots or spaces at its end
static Str randpart(Arena *a)
{
    Str set = "abcXYZ019_-";
    Str r   = randstr(a, set, 6);
    if (!r.len) r = "q";
    if (rand32()%4 == 0) {
        r = concat(a, r, rand32()%2 ? Str(".x") : Str(" y"));
    }
    return r;
}

// Is a native result canonical: forward slashes, no empty, "." or
// ".." components after the root?
static b32 canonical(Str s)
{
    if (find(s, "\\") >= 0) return 0;
    Str root = nativeroot(s);
    if (!root.len) return 0;
    if (hasdrive(s) && !(s[0]>='A' && s[0]<='Z')) return 0;
    for (Cut c = {.head={}, .tail=cuthead(s, root.len), .ok=1}; c.ok;) {
        c = cut(c.tail, '/');
        if (c.head=="." || c.head=="..") return 0;
        if (!c.head.len && c.ok) return 0;
    }
    return 1;
}

static void testproperties(PathCtx *msys, Arena scratch)
{
    PathCtx native = {};
    native.mode = PM_NATIVE;
    native.cwd  = "C:/work/dir";
    PathCtx *ctxs[] = {msys, &native};

    Str nasty = "/\\:;.,=-@~'\"`*?[] aXc1\t";
    i32 failures = 0;
    for (i32 n = 0; n < 200000 && failures < 10; n++) {
        Arena a = scratch;

        // msysarg: anything goes, and nothing without a slash changes
        Str s = randstr(&a, nasty, 14);
        Str r = msysarg(msys, &a, s, {});
        b32 need = 0;
        for (iz i = 0; i < s.len && !whitespace(s[i]); i++) {
            need |= pathsep(s[i]);
        }
        b32 ok = (need || r==s) && (r==s || find(r, ";;")<0);
        ok &= msysarg(msys, &a, s, {}) == r;
        if (!check(ok, 0, "msysarg property")) {
            show("input", s);
            show("got", r);
            failures++;
        }

        // topath: canonical results, mapping to themselves
        for (iz m = 0; m < countof(ctxs); m++) {
            PathCtx *ctx = ctxs[m];
            PathRes  p   = topath(ctx, &a, s);
            if (!p.ok || p.kind!=PK_FILE) continue;
            ok = canonical(p.native);
            PathRes q = topath(ctx, &a, p.native);
            ok &= q.ok && q.kind==PK_FILE && q.native==p.native;
            if (!check(ok, 0, "topath property")) {
                show("input", s);
                show("got", p.native);
                show("again", q.native);
                failures++;
            }
        }

        // cygpath forms: no crash, Windows forms are fixed points
        Str w = towin(msys, &a, s, 0);
        Str u = toposix(msys, &a, s);
        towinlist(msys, &a, s, 1);
        toposixlist(msys, &a, s);
        ok = !s.len || (w.data && u.data);
        if (w.data && hasdrive(w) && w.len>2 && w[2]=='/' && !endswith(w, "//")) {
            ok &= towin(msys, &a, w, 0) == w;
        }
        if (!check(ok, 0, "cygpath property")) {
            show("input", s);
            show("towin", w);
            failures++;
        }

        // the builtin: any arguments parse without a crash, and a name
        // converts as the engine does
        Str toks[] = {"-u", "-w", "-m", "-a", "-p", "-l", "-s", "-W", "-S",
                      "-f", "-h", "--", "-", "--file=", "--mixed", "-wpa",
                      "-mf", "--bogus", "-x", "", s};
        Str args[6] = {"cygpath"};
        iz  nargs   = 1 + rand32()%5;
        for (iz i = 1; i < nargs; i++) args[i] = toks[rand32()%countof(toks)];
        CygOpts o = cygpathopts(&a, args, nargs);
        ok = o.ok || o.err.len;
        if (o.ok && !o.help) {
            CygRun run = cygpathrun(msys, &a, &o, args, nargs, s, {});
            ok &= run.status==0 || run.err.data;
        }
        Str one[] = {"cygpath", "-m", "--", s};
        o = cygpathopts(&a, one, 4);
        CygRun run = cygpathrun(msys, &a, &o, one, 4, {}, {});
        ok &= s.len ? run.out==concat(&a, w, "\n") : run.status==1;
        if (!check(ok, 0, "cygpath builtin property")) {
            for (iz i = 0; i < nargs; i++) show("arg", args[i]);
            failures++;
        }
    }

    for (i32 n = 0; n < 50000 && failures < 10; n++) {
        Arena a = scratch;

        // Windows to POSIX and back, from bases that hit the mounts
        Str bases[] = {
            "C:", "D:", "c:", "z:", mountnative(msys, "/"),
            mountnative(msys, "/tmp"), mountnative(msys, "/bin"),
        };
        Buf b(&a, 64);
        print(&b, bases[rand32() % countof(bases)]);
        for (i32 k = rand32()%4; k >= 0; k--) {
            putbyte(&b, rand32()%2 ? '/' : '\\');
            print(&b, randpart(b.a));
        }
        Str win  = finish(&b);
        Str posx = toposix(msys, &a, win);
        Str back = towin(msys, &a, posx, 0);
        Str want = winnorm(&a, win, msys->cwd);
        // A one-letter directory under the root reads back as a drive,
        // as in Git Bash, since the cygdrive prefix is "/"
        Str root  = mountnative(msys, "/");
        b32 alias = pathprefix(root, want, 1) && posx.len>=2 && letter(posx[1])
                    && (posx.len==2 || posx[2]=='/');
        b32 ok = (alias || equalfold(back, want)) && posx.len && posx[0]=='/';
        if (!check(ok, 0, "Windows round trip")) {
            show("input", win);
            show("posix", posx);
            show("back", back);
            failures++;
        }

        // POSIX to Windows is stable through a POSIX round trip
        Buf p(&a, 64);
        for (i32 k = rand32()%4; k >= 0; k--) {
            putbyte(&p, '/');
            Str parts[] = {"c", "x", "tmp", "bin", "usr", "TMP", "..", "."};
            print(&p, rand32()%2 ? parts[rand32()%countof(parts)] : randpart(p.a));
        }
        Str px = finish(&p);
        Str w1 = towin(msys, &a, px, 0);
        Str u1 = toposix(msys, &a, w1);
        Str w2 = towin(msys, &a, u1, 0);
        // Except that "X:/" for a missing drive comes back "X://"
        ok = w1==w2 || (w1.len==3 && concat(&a, w1, "/")==w2);
        if (!check(ok, 0, "POSIX round trip")) {
            show("input", px);
            show("win", w1);
            show("posix", u1);
            show("again", w2);
            failures++;
        }

        // Environment blocks are sorted, PATH last when added
        Slice<Str> env = {};
        for (i32 k = rand32()%6; k > 0; k--) {
            Str names[] = {"PATH", "HOME", "TZ", "tz", "T_A", "MSYS2_ENV_CONV_EXCL", "Path"};
            Str e = concat(&a, clone(&a, names[rand32()%countof(names)]), "=");
            env = push(&a, env, concat(&a, e, randstr(&a, nasty, 10)));
        }
        Slice<Str> got = msysenv(msys, &a, env);
        ok = got.len > 0;
        iz sorted = got.len - (got.len && startswith(got[got.len-1], "PATH=")
                               && find(got[got.len-1], ";")<0);
        for (iz i = 1; ok && i < sorted; i++) {
            ok = compare(got[i-1], got[i]) <= 0;
        }
        if (!check(ok, 0, "msysenv property")) {
            for (iz i = 0; i < got.len; i++) show("got", got[i]);
            failures++;
        }
    }
}

// The cygpath builtin's argument handling: one run, its output and
// status. The input, if any, is -f's contents.
static void cygcase(PathCtx *ctx, Arena scratch, iz line, Slice<Str> args,
                    Str want, i32 status, Str in = {}, Str windir = {})
{
    Arena  *a = &scratch;
    CygOpts o = cygpathopts(a, args.data, args.len);
    CygRun  r = {};
    if (!o.ok) {
        r.status = 1;
    } else if (!o.help) {
        r = cygpathrun(ctx, a, &o, args.data, args.len, in, windir);
    }
    b32 ok = r.out==want && r.status==status && (!r.status || !o.ok || r.err.data);
    if (!check(ok, line, "cygpath builtin")) {
        for (iz i = 0; i < args.len; i++) show("arg", args[i]);
        if (!o.ok) show("error", o.err);
        show("want", want);
        show("got", r.out);
        fprintf(stderr, "    status %d, want %d\n", r.status, status);
    }
}

#define CYG(want, status, ...) \
    do { \
        Str argv_[] = {"cygpath", __VA_ARGS__}; \
        cygcase(ctx, scratch, __LINE__, {argv_, countof(argv_)}, want, status); \
    } while (0)

static void testcygpath(PathCtx *ctx, Arena scratch)
{
    Arena *a   = &scratch;
    Str    cwd = toposix(ctx, a, ctx->cwd);
    Str    dir = cwd;
    for (; dir.len && dir[dir.len-1]!='/'; dir.len--) {}
    dir.len--;  // the parent

    CYG(concat(a, cwd, "/sub\n"), 0, "-a", "--", "sub");
    CYG(concat(a, ctx->cwd, "/sub\n"), 0, "-am", "--", "sub");
    CYG(concat(a, cwd, "/sub/x\n"), 0, "-a", "sub\\x");
    CYG(concat(a, cwd, "\n"), 0, "--absolute", ".");
    CYG(concat(a, dir, "\n"), 0, "-ua", "..");
    CYG(concat(a, cwd, "/b\n"), 0, "-a", "./a/../b");
    CYG(concat(a, towin(ctx, a, dir, 1), "\n"), 0, "-wa", "..");
    CYG(concat(a, ctx->cwd, "\n"), 0, "--mixed", "--absolute", ".");
    CYG(concat(a, ctx->cwd, "/\n"), 0, "-ma", "./");
    CYG("-x\n", 0, "-m", "--", "-x");
    CYG("/c/Windows\n", 0, "-ua", "C:\\Windows");
    CYG("C:\\\n", 0, "-wa", "/c/Windows/..");
    CYG("/c/a:/d/b\n", 0, "-p", "C:\\a;D:\\b");
    CYG("/c/a:/d/b\n", 0, "--path", "--unix", "C:\\a;D:\\b");
    CYG("C:/a;/d/b\n", 0, "-mp", "/c/a;/d/b");
    CYG("C:\\a;.\n", 0, "-wp", "/c/a:");
    CYG("C:\\Windows\n", 0, "-l", "-s", "-w", "/c/Windows");
    CYG("C:\\Windows\n", 0, "--long-name", "--windows", "/c/Windows");
    CYG("C:/\n", 0, "-mm", "/c");
    CYG("C:\\a\nC:\\b\n", 0, "-w", "C:\\a", "/c/b");
    CYG("/c/a\n", 1, "-u", "C:\\a", "", "C:\\b");
    CYG("", 1, "");
    CYG("", 1, "-wp", "");
    CYG("", 0, "-h");
    CYG("", 0, "--help", "-w", "x");   // help wins over the names
    CYG("", 1);                        // no names
    CYG("", 1, "-w", "-u", "/c");
    CYG("", 1, "--unix", "-m", "/c");
    CYG("", 1, "-W", "x");
    CYG("", 1, "-S", "-f", "x");
    CYG("", 1, "-q", "/x");
    CYG("", 1, "--bogus", "/x");
    CYG("", 1, "--unix=x", "/x");
    CYG("", 1, "-f");
    CYG("", 1, "--file");

    // -W and -S from the Windows directory the caller found
    Str w[]  = {"cygpath", "-W"};
    Str ws[] = {"cygpath", "-wS"};
    Str mw[] = {"cygpath", "-mW"};
    Str us[] = {"cygpath", "-uS"};
    cygcase(ctx, scratch, __LINE__, {w, 2}, "/c/Windows\n", 0, {}, "C:\\Windows");
    cygcase(ctx, scratch, __LINE__, {ws, 2}, "C:\\Windows\\System32\n", 0, {}, "C:\\Windows");
    cygcase(ctx, scratch, __LINE__, {mw, 2}, "C:/Windows\n", 0);
    cygcase(ctx, scratch, __LINE__, {us, 2}, "/d/win/System32\n", 0, {}, "d:/win/");

    // -f: the caller reads the file; lines lose a final CR, an empty
    // line stops the run
    Str f1[] = {"cygpath", "-f", "-"};
    Str f2[] = {"cygpath", "-mfFILE"};
    Str f3[] = {"cygpath", "--file=x", "-w"};
    Str f4[] = {"cygpath", "-wp", "--file", "y"};
    cygcase(ctx, scratch, __LINE__, {f1, 3}, "/c/a\n/d/b\n", 0, "C:\\a\r\nD:\\b\r\n");
    cygcase(ctx, scratch, __LINE__, {f1, 3}, "/c/a\n/d/b\n", 0, "C:\\a\nD:\\b");
    cygcase(ctx, scratch, __LINE__, {f1, 3}, "", 0, "");
    cygcase(ctx, scratch, __LINE__, {f2, 2}, "C:/a\n", 1, "C:\\a\n\nD:\\b\n");
    cygcase(ctx, scratch, __LINE__, {f3, 3}, "C:\\a\n", 0, "/c/a\n");
    cygcase(ctx, scratch, __LINE__, {f4, 4}, "C:\\a;C:\\b\n", 0, "/c/a:/c/b\n");
    CygOpts o = cygpathopts(a, f2, 2);
    check(o.ok && o.file=="FILE" && o.form==CYG_MIXED, __LINE__, "cygpath -fFILE");
    o = cygpathopts(a, f3, 3);
    check(o.ok && o.file=="x" && o.form==CYG_WINDOWS, __LINE__, "cygpath --file=x");
    o = cygpathopts(a, f4, 4);
    check(o.ok && o.file=="y" && o.list && o.first==4, __LINE__, "cygpath --file y");
}

int main(int argc, char **argv)
{
    char const *path = argc>1 ? argv[1] : "t/unit/paths.golden";
    rng = argc>2 ? strtoull(argv[2], 0, 10) : 1;

    iz    cap = (iz)1<<28;
    byte *mem = (byte *)malloc((size_t)cap);
    Arena perm = {mem, mem+cap, 0};

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "paths_test: cannot open %s\n", path);
        return 2;
    }
    Str src = {};
    src.data = (u8 *)perm.beg;
    src.len  = (iz)fread(src.data, 1, (size_t)(cap/4), f);
    fclose(f);
    perm.beg += src.len;

    Golden  g   = parse(&perm, src);
    PathCtx ctx = goldenctx(&perm, &g);
    for (iz i = 0; i < g.records.len; i++) {
        testrecord(&ctx, perm, &g.records[i]);
    }
    i32 golden = ntests;
    testnative(perm);
    testmsys(perm);
    testposix(perm);
    testpersona();
    testcygpath(&ctx, perm);
    i32 inline_ = ntests - golden;
    testproperties(&ctx, perm);

    printf("%d tests (%d golden, %d inline, %d property), %d failed\n",
           ntests, golden, inline_, ntests-golden-inline_, nfailed);
    return nfailed != 0;
}
