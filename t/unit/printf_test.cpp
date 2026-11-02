// Unit tests for b_printf.cpp against behavior recorded from bash 5.3
// $ g++ -std=c++20 -g3 -Wall -Wextra -Wconversion -Wno-sign-conversion
//       -fsanitize=undefined -fsanitize-trap -o printf_test.exe
//       t/unit/printf_test.cpp
// $ ./printf_test.exe [t/unit/printf.golden]
//
// The golden file is recorded from t/unit/printf.cases by
// t/unit/printf_record.sh, which documents its format. Run from the
// repository root, or pass the golden file's path.
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"  // base.cpp's spares
#include "../../base.cpp"
#pragma GCC diagnostic pop
#include "../../float.cpp"
#include "../../b_printf.cpp"

[[noreturn]] static void os_oom()
{
    fputs("printf_test: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}


// Golden file

struct Record {
    u8         kind;    // 'T' printf, 'X' echo -e
    b32        utf8;
    Str        fmt;
    Slice<Str> args;
    Str        out;
    Str        err;
    i32        status;
    Str        var;     // name=value after %n, or null
    iz         line;
};

static Str decode(Arena *a, Str s)
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        if (s[i]=='\\' && i+1<s.len && s[i+1]=='\\') {
            putbyte(&b, '\\');
            i++;
        } else if (s[i]=='\\' && i+3<s.len && s[i+1]=='x') {
            putbyte(&b, (u8)(hexdigit(s[i+2])<<4 | hexdigit(s[i+3])));
            i += 3;
        } else {
            putbyte(&b, s[i]);
        }
    }
    Str r = finish(&b);
    r.data = r.data ? r.data : (u8 *)"";
    return r;
}

static Slice<Record> parse(Arena *a, Str src)
{
    Slice<Record> r   = {};
    Record        cur = {};
    iz            n   = 0;
    for (Cut c = {.head={}, .tail=src, .ok=1}; c.ok;) {
        c = cut(c.tail, '\n');
        n++;
        Str line = c.head;
        if (line.len && line[line.len-1]=='\r') {
            line.len--;
        }
        if (!line.len || line[0]=='#') {
            continue;
        }
        Str val = line.len>2 ? cuthead(line, 2) : Str{};
        switch (line[0]) {
        case 'T':
        case 'X':
            cur      = {};
            cur.kind = line[0];
            cur.utf8 = val == "U";
            cur.line = n;
            break;
        case 'F': cur.fmt  = decode(a, val); break;
        case 'A': cur.args = push(a, cur.args, decode(a, val)); break;
        case 'O': cur.out  = decode(a, val); break;
        case 'E': cur.err  = decode(a, val); break;
        case 'V': cur.var  = decode(a, val); break;
        case 'S':
            cur.status = 0;
            for (iz i = 0; i < val.len; i++) {
                cur.status = cur.status*10 + val[i] - '0';
            }
            break;
        case '.':
            r = push(a, r, cur);
            break;
        }
    }
    return r;
}


// Test environment

struct Ctx {
    Buf *err;
    Buf *var;
    i64  now;
};

static void onerror(void *ctx, Str msg)
{
    Ctx *c = (Ctx *)ctx;
    print(c->err, msg);
    putbyte(c->err, '\n');
}

static void onsetint(void *ctx, Str name, i64 v)
{
    Ctx *c = (Ctx *)ctx;
    c->var->len = 0;
    print(c->var, name);
    putbyte(c->var, '=');
    print(c->var, v);
}

static i64 onnow(void *ctx)
{
    return ((Ctx *)ctx)->now;
}

struct Result {
    Str out;
    Str err;
    Str var;
    i32 status;
};

static Result run(Arena *a, b32 utf8, Str fmt, Str *args, iz nargs)
{
    Result r   = {};
    Buf   *out = alloc<Buf>(a);
    Buf   *err = alloc<Buf>(a);
    Buf   *var = alloc<Buf>(a);
    *out = Buf(a, 1<<10);
    *err = Buf(a, 1<<10);
    *var = Buf(a, 1<<6);

    Ctx ctx = {};
    ctx.err = err;
    ctx.var = var;
    ctx.now = 1700000000;

    PrintfEnv env  = {};
    env.ctx        = &ctx;
    env.utf8       = utf8;
    env.now        = onnow;
    env.shellstart = 1600000000;
    env.error      = onerror;
    env.setint     = onsetint;

    r.status = printfmt(&env, out, fmt, args, nargs);
    r.out = finish(out);
    r.err = finish(err);
    r.var = finish(var);
    return r;
}


// Reporting

static i32 ntests;
static i32 nfailed;

static void show(Str label, Str s)
{
    fprintf(stderr, "    %-8.*s \"", (int)label.len, label.cdata);
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
    fputs("\"\n", stderr);
}

static void check(b32 ok, iz line, Str what)
{
    ntests++;
    if (!ok) {
        nfailed++;
        fprintf(stderr, "FAIL line %td: %.*s\n", line, (int)what.len, what.cdata);
    }
}

static void testrecord(Arena perm, Record *t)
{
    Arena *a = &perm;
    if (t->kind == 'X') {
        b32 stop = 0;
        Str got  = echoescape(a, t->args[0], 0, t->utf8, &stop);
        if (!stop) {
            got = concat(a, got, " X\n");
        }
        b32 ok = got == t->out;
        check(ok, t->line, "echo -e");
        if (!ok) {
            show("arg", t->args[0]);
            show("want", t->out);
            show("got", got);
        }
        return;
    }

    Result r  = run(a, t->utf8, t->fmt, t->args.data, t->args.len);
    b32    ok = r.out==t->out && r.err==t->err && r.status==t->status;
    if (t->var.data) {
        if (!r.var.len) {
            r.var = concat(a, cut(t->var, '=').head, "=");  // never assigned
        }
        ok &= r.var == t->var;
    }
    check(ok, t->line, "printf");
    if (!ok) {
        show("format", t->fmt);
        for (iz i = 0; i < t->args.len; i++) {
            show("arg", t->args[i]);
        }
        show("want out", t->out);
        show("got out",  r.out);
        show("want err", t->err);
        show("got err",  r.err);
        fprintf(stderr, "    status   want %d got %d\n", t->status, r.status);
        if (t->var.data) {
            show("want var", t->var);
            show("got var",  r.var);
        }
    }

    // %b and %q of one argument also exercise the standalone functions
    if (t->args.len==1 && t->err.len==0 && t->fmt=="%b") {
        b32 stop = 0;
        Str got  = echoescape(a, t->args[0], 1, t->utf8, &stop);
        check(got == t->out, t->line, "echoescape %b");
    }
    if (t->args.len==1 && t->fmt=="%q") {
        Str got = shellquote(a, t->args[0], t->utf8);
        check(got == t->out, t->line, "shellquote");
    }
}


// Cases that cannot be recorded: the clock, and deferred floats

static void expect(Arena perm, Str fmt, Str arg, Str want)
{
    Str    args[] = {arg};
    Result r      = run(&perm, 0, fmt, args, arg.data ? 1 : 0);
    b32    ok     = r.out==want && !r.err.len && !r.status;
    check(ok, 0, fmt);
    if (!ok) {
        show("format", fmt);
        show("want", want);
        show("got", r.out);
        show("err", r.err);
    }
}

static void testunclosed(Arena perm, Str unit, Str want)
{
    iz  n = 100000;
    Buf f(&perm, n*unit.len);
    for (iz i = 0; i < n; i++) {
        print(&f, unit);
    }
    Result r  = run(&perm, 0, finish(&f), 0, 0);
    b32    ok = !r.status && r.out.len==n*want.len;
    for (iz i = 0; ok && i < n; i++) {
        ok = takehead(cuthead(r.out, i*want.len), want.len) == want;
    }
    check(ok, 0, unit);
}

static void testinline(Arena perm)
{
    expect(perm, "%(%s)T",   {},     "1700000000");
    expect(perm, "%(%s)T",   "-1",   "1700000000");
    expect(perm, "%(%s)T",   "-2",   "1600000000");
    expect(perm, "%(%s)T",   "-3",   "-3");
    expect(perm, "%()T",     {},     "22:13:20");
    expect(perm, "%f|%s",    "1.5",  "1.500000|");
    expect(perm, "%9.2e|",   "1",    " 1.00e+00|");
    expect(perm, "%a%A%E%F%g%G", {}, "0x0p+00X0P+00.000000E+000.00000000");

    // Standalone quoting entry points
    Arena *a = &perm;
    check(shellquote(a, "", 0) == "''", 0, "shellquote empty");
    check(shellquote(a, "a b", 1) == "a\\ b", 0, "shellquote blank");
    check(shellquote(a, "~x#", 0) == "\\~x#", 0, "shellquote tilde");
    b32 stop = 0;
    check(echoescape(a, "a\\cb", 0, 0, &stop)=="a" && stop, 0, "echo \\c");
    check(echoescape(a, "a\\xg", 1, 0, &stop)=="a\\xg" && !stop, 0, "%b \\x");

    // Unclosed %( must not rescan the rest of the format each time,
    // including after bash's in-place rewrite of "%l(" to "%((".
    testunclosed(perm, "%(",  "%(");
    testunclosed(perm, "%l(", "%((");
}


int main(int argc, char **argv)
{
    char const *path = argc>1 ? argv[1] : "t/unit/printf.golden";

    iz    cap = (iz)1<<28;
    byte *mem = (byte *)malloc((size_t)cap);
    Arena perm = {mem, mem+cap, 0};

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "printf_test: cannot open %s\n", path);
        return 2;
    }
    Str src = {};
    src.data = (u8 *)perm.beg;
    src.len  = (iz)fread(src.data, 1, (size_t)(cap/4), f);
    fclose(f);
    perm.beg += src.len;

    Slice<Record> records = parse(&perm, src);
    for (iz i = 0; i < records.len; i++) {
        testrecord(perm, &records[i]);
    }
    testinline(perm);

    printf("%d tests, %d failed\n", ntests, nfailed);
    return nfailed != 0;
}
