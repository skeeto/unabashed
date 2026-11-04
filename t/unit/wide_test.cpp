// Unit tests for wide.cpp: command line codecs, CRT descriptors, PE
//   $ g++ -std=c++20 -g3 -Wall -Wextra -Wconversion -Wno-sign-conversion
//         -fsanitize=undefined -fsanitize-trap -o wide_test.exe
//         t/unit/wide_test.cpp
//   $ ./wide_test.exe [t/unit/wide.golden]
//   $ ./wide_test.exe --real [UNABASHED_DIR]
//
// The default mode replays wide.golden, recorded by wide_record.sh from
// CommandLineToArgvW, msvcrt.dll, ucrtbase.dll and Git's msys printf,
// through splitcmdline and splitcygwin (with a simulated glob and
// response files for the latter), then checks quote/split round trips
// on random byte strings, the lpReserved2 codec, and pesniff on
// synthetic images, intact, truncated and mutated.
//
// --real sniffs real binaries, read-only, on Windows: cmd.exe, Git's
// usr/bin/ls.exe ($GITBASH's directory), w64devkit's busybox.exe and
// echo.exe alias stub (found on PATH), and unabashed.exe and
// unabashed32.exe in UNABASHED_DIR (default ".").
//
// Golden escapes: ^t ^n ^r ^v ^f ^^, ^xHH for a byte, ^lN; for N a's.
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"  // unused foundations
#include "../../base.cpp"
#include "../../wide.cpp"
#pragma GCC diagnostic pop

[[noreturn]] static void os_oom()
{
    fputs("wide_test: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}

static i32 ntests;
static i32 nfailed;

static void showstr(Str s)
{
    fwrite(s.data, 1, (uz)s.len, stderr);
}

static void check(b32 ok, Str what, Str got, Str want)
{
    ntests++;
    if (!ok) {
        nfailed++;
        fputs("FAIL: ", stderr);
        showstr(what);
        fputs("\n  got:  ", stderr);
        showstr(got);
        fputs("\n  want: ", stderr);
        showstr(want);
        fputs("\n", stderr);
    }
}

static void expect(Str what, Str got, Str want)
{
    check(got==want, what, got, want);
}

static void expectint(Str what, i64 got, i64 want)
{
    ntests++;
    if (got != want) {
        nfailed++;
        fputs("FAIL: ", stderr);
        showstr(what);
        fprintf(stderr, "\n  got:  %lld (%#llx)\n  want: %lld (%#llx)\n",
                (long long)got, (long long)got, (long long)want, (long long)want);
    }
}

static Str readfile(Arena *a, char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return {};
    Str r = {};
    r.data = allocbytes(a, 0);
    for (;;) {
        iz cap = a->end - a->beg;
        iz n   = (iz)fread(a->beg, 1, (uz)cap, f);
        allocbytes(a, n);
        r.len += n;
        if (n < cap) break;
    }
    fclose(f);
    return r;
}

static u64 rng(u64 *s)
{
    *s = *s*0x3243f6a8885a308d + 1;
    u64 r = *s >> 32;
    return r ^ r>>16;
}


// Golden escapes

static Str unescape(Arena *a, Str s)
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        if (s[i]!='^' || i+1==s.len) {
            putbyte(&b, s[i]);
            continue;
        }
        switch (s[++i]) {
        case 't': putbyte(&b, '\t'); break;
        case 'n': putbyte(&b, '\n'); break;
        case 'r': putbyte(&b, '\r'); break;
        case 'v': putbyte(&b, '\v'); break;
        case 'f': putbyte(&b, '\f'); break;
        case 'x': {
            u8 v = 0;
            for (i32 k = 0; k<2 && i+1<s.len; k++) {
                u8 c = s[++i];
                v = (u8)(v<<4 | (digit(c) ? c-'0' : (c|0x20)-'a'+10));
            }
            putbyte(&b, v);
        } break;
        case 'l': {
            iz n = 0;
            for (i++; i<s.len && digit(s[i]); i++) n = n*10 + s[i] - '0';
            for (; n; n--) putbyte(&b, 'a');
        } break;
        default: putbyte(&b, s[i]);
        }
    }
    return finish(&b);
}

static void escape(Buf *b, Str s)
{
    for (iz i = 0; i < s.len; i++) {
        iz run = 0;
        for (; i+run<s.len && s[i+run]=='a'; run++) {}
        if (run >= 16) {
            print(b, "^l");
            print(b, run);
            putbyte(b, ';');
            i += run - 1;
            continue;
        }
        u8 c = s[i];
        switch (c) {
        case '\t': print(b, "^t"); break;
        case '\n': print(b, "^n"); break;
        case '\r': print(b, "^r"); break;
        case '\v': print(b, "^v"); break;
        case '\f': print(b, "^f"); break;
        case '^':  print(b, "^^"); break;
        default:
            if (c<0x20 || c==0x7f) {
                print(b, "^x");
                printhex(b, c, 2);
            } else {
                putbyte(b, c);
            }
        }
    }
}


// A simulated Cygwin child: glob(3) over a few files in its directory,
// and response files

struct File {
    Str name;
    Str content;
};

struct Dir {
    File files[16];
    i32  nfiles;
};

// Match name against a glob pattern with \ quoting.
static b32 gmatch(Str p, iz i, Str s, iz j)
{
    for (; i < p.len; i++, j++) {
        u8 c = p[i];
        if (c == '*') {
            for (iz k = j; k <= s.len; k++) {
                if (gmatch(p, i+1, s, k)) return 1;
            }
            return 0;
        }
        if (j == s.len) return 0;
        if (c == '?') continue;
        if (c == '[') {
            iz  k   = i + 1;
            b32 neg = k<p.len && (p[k]=='!' || p[k]=='^');
            k += neg;
            b32 hit = 0;
            iz  beg = k;
            for (; k<p.len && (p[k]!=']' || k==beg); k++) {
                if (k+2<p.len && p[k+1]=='-' && p[k+2]!=']') {
                    hit |= s[j]>=p[k] && s[j]<=p[k+2];
                    k += 2;
                } else {
                    hit |= s[j] == p[k];
                }
            }
            if (k < p.len) {
                if (hit == neg) return 0;
                i = k;
                continue;
            }
        }
        if (c=='\\' && i+1<p.len) c = p[++i];
        if (c != s[j]) return 0;
    }
    return j == s.len;
}

static Str unquote(Arena *a, Str p)
{
    Buf b(a, p.len);
    for (iz i = 0; i < p.len; i++) {
        putbyte(&b, p[i]=='\\' && i+1<p.len ? p[++i] : p[i]);
    }
    return finish(&b);
}

static Slice<Str> braces(Arena *a, Slice<Str> out, Str p)
{
    iz open = -1, close = -1, depth = 0;
    for (iz i = 0; i < p.len; i++) {
        if (p[i] == '\\') {
            i++;
        } else if (p[i]=='{' && !(i+1<p.len && p[i+1]=='}')) {
            if (!depth++ && open<0) open = i;
        } else if (p[i]=='}' && depth && !--depth && close<0) {
            close = i;
        }
    }
    if (open<0 || close<0) {
        return push(a, out, p);
    }
    Str head = takehead(p, open);
    Str tail = cuthead(p, close+1);
    iz  beg  = open + 1;
    depth = 0;
    for (iz i = beg; i <= close; i++) {
        if (p[i] == '\\') {
            i++;
        } else if (p[i] == '{') {
            depth++;
        } else if (i==close || (p[i]==',' && !depth)) {
            Str alt = span(p.data+beg, p.data+i);
            out = braces(a, out, concat(a, concat(a, head, alt), tail));
            beg = i + 1;
        } else if (p[i] == '}') {
            depth--;
        }
    }
    return out;
}

static Slice<Str> simglob(Arena *a, Slice<Str> out, Dir *d, Str pat)
{
    Slice<Str> alts = braces(a, {}, pat);
    for (iz k = 0; k < alts.len; k++) {
        Str p      = alts[k];
        Str prefix = startswith(p, "./") ? Str("./") : Str("");
        Str rest   = cuthead(p, prefix.len);
        iz  before = out.len;
        if (rest.len && rest[0]!='~' && find(rest, "/")<0) {
            for (i32 i = 0; i < d->nfiles; i++) {  // files are sorted
                if (gmatch(rest, 0, d->files[i].name, 0)) {
                    out = push(a, out, concat(a, prefix, d->files[i].name));
                }
            }
        }
        if (out.len == before) {
            out = push(a, out, unquote(a, p));  // GLOB_NOCHECK
        }
    }
    return out;
}

static File *findfile(Dir *d, Str name)
{
    if (startswith(name, "./")) name = cuthead(name, 2);
    for (i32 i = 0; i < d->nfiles; i++) {
        if (d->files[i].name == name) return d->files + i;
    }
    return 0;
}

// Arguments as a Cygwin program sees them.
static Slice<Str> cygexpand(Arena *a, Slice<Str> out, Dir *d, Str cmd, b32 program, i32 *nfiles)
{
    Slice<CygArg> args = splitcygwin(a, cmd, program);
    for (iz i = 0; i < args.len; i++) {
        CygArg r = args[i];
        File  *f = r.flags&CYG_ATFILE ? findfile(d, r.file) : 0;
        if (f && ++*nfiles<=10) {
            Str more = concat(a, concat(a, f->content, " "), cuthead(cmd, r.end));
            return cygexpand(a, out, d, more, 0, nfiles);
        } else if (r.flags & CYG_GLOB) {
            out = simglob(a, out, d, r.pat);
        } else {
            out = push(a, out, r.arg);
        }
    }
    return out;
}


// Golden replay

static Str nativeline(Arena *a, Str name, Str cmd, i32 parser)
{
    cmd = clone(a, cmd);  // split in place
    Slice<Str> args = splitcmdline(a, cmd, parser);
    Buf b(a);
    print(&b, name);
    for (iz i = 0; i < args.len; i++) {
        print(&b, " <");
        escape(&b, args[i]);
        putbyte(&b, '>');
    }
    return finish(&b);
}

static Str cygwinline(Arena *a, Dir *d, Str tail)
{
    Str cmd = concat(a, "\"C:/git/usr/bin/printf.exe\" <%s>\\n X ", tail);
    i32 nfiles = 0;
    Slice<Str> args = cygexpand(a, {}, d, cmd, 1, &nfiles);
    Buf out(a);
    for (iz i = 3; i < args.len; i++) {
        putbyte(&out, '<');
        print(&out, args[i]);
        print(&out, ">\n");
    }
    Buf b(a);
    print(&b, "cygwin ");
    escape(&b, finish(&out));
    return finish(&b);
}

static void golden(Arena scratch, Str path)
{
    char cpath[1024];
    snprintf(cpath, sizeof(cpath), "%.*s", (int)path.len, path.data);
    Str src = readfile(&scratch, cpath);
    if (!src.data) {
        fprintf(stderr, "wide_test: cannot read %s\n", cpath);
        exit(2);
    }

    Dir *d      = alloc<Dir>(&scratch);
    Str  native = {};
    Str  tail   = {};
    Str  label  = {};
    i32  cases  = 0;
    for (Cut c = {{}, src, 1}; c.ok;) {
        c = cut(c.tail, '\n');
        Str line = c.head;
        if (!line.len || line[0]=='#') continue;
        Arena t = scratch;
        Cut   w = cut(line, ' ');
        Str   kind = w.head;
        Str   rest = w.tail;
        if (kind=="tail" || kind=="full" || kind=="cyg") {
            label  = line;
            cases++;
            tail   = unescape(&scratch, rest);
            native = kind=="tail" ? concat(&scratch, "p ", tail) : tail;
        } else if (kind == "file") {
            Cut nc = cut(rest, ' ');
            File *f = d->files + d->nfiles++;
            f->name    = nc.head;
            f->content = unescape(&scratch, nc.tail);
            for (i32 i = d->nfiles-1; i>0 && compare(d->files[i].name, d->files[i-1].name)<0; i--) {
                File tmp = d->files[i];
                d->files[i]   = d->files[i-1];
                d->files[i-1] = tmp;
            }
        } else if (kind == "shell32") {
            expect(label, nativeline(&t, kind, native, ARGV_SHELL32), line);
        } else if (kind == "msvcrt") {
            expect(label, nativeline(&t, kind, native, ARGV_MSVCRT), line);
        } else if (kind == "ucrt") {
            expect(label, nativeline(&t, kind, native, ARGV_UCRT), line);
        } else if (kind == "cygwin") {
            expect(label, cygwinline(&t, d, tail), line);
        } else {
            check(0, line, "", "a known line");
        }
    }
    fprintf(stderr, "wide_test: %d golden cases\n", cases);
}


// Round trips

// A random string, mostly from alphabet, the rest any byte but NUL
// unless only is set.
static Str randarg(Arena *a, u64 *s, Str alphabet, iz maxlen, b32 only)
{
    iz  len = (iz)(rng(s) % (u64)(maxlen+1));
    Str r   = {allocbytes(a, len), len};
    for (iz i = 0; i < len; i++) {
        u64 x = rng(s);
        if (only || x%4) {
            r[i] = alphabet[(iz)(x/4 % (u64)alphabet.len)];
        } else {
            r[i] = (u8)(1 + x/4%255);  // any byte but NUL
        }
    }
    return r;
}

static Str showargs(Arena *a, Str *args, iz n)
{
    Buf b(a);
    for (iz i = 0; i < n; i++) {
        print(&b, i ? Str(" <") : Str("<"));
        escape(&b, args[i]);
        putbyte(&b, '>');
    }
    return finish(&b);
}

static void roundtrips(Arena scratch)
{
    Str special = "\"\\ \t\n\r\v'*?[]{}()~@C:ab";
    Str program = "abc/\\:. _-'";
    u64 seed    = 1;
    i32 n       = 20000;

    for (i32 iter = 0; iter < n; iter++) {
        Arena t     = scratch;
        iz    nargs = 1 + (iz)(rng(&seed) % 6);
        Str  *args  = alloc<Str>(&t, nargs);
        args[0] = randarg(&t, &seed, program, 8, 1);
        for (iz i = 1; i < nargs; i++) {
            args[i] = randarg(&t, &seed, special, 12, 0);
        }
        Str want = showargs(&t, args, nargs);

        Buf b(&t);
        quoteprogram(&b, args[0]);
        for (iz i = 1; i < nargs; i++) {
            putbyte(&b, ' ');
            quotearg(&b, args[i]);
        }
        Str cmd = finish(&b);
        for (i32 p = ARGV_SHELL32; p <= ARGV_UCRT; p++) {
            Slice<Str> got = splitcmdline(&t, clone(&t, cmd), p);
            Str what = concat(&t, "msvcrt round trip: ", cmd);
            expect(what, showargs(&t, got.data, got.len), want);
        }

        b = Buf(&t);
        quoteprogram(&b, args[0]);
        for (iz i = 1; i < nargs; i++) {
            putbyte(&b, ' ');
            quotecygwin(&b, args[i]);
        }
        cmd = finish(&b);
        Slice<CygArg> got = splitcygwin(&t, cmd, 1);
        Str *plain = alloc<Str>(&t, got.len);
        i32  flags = 0;
        for (iz i = 0; i < got.len; i++) {
            plain[i] = got[i].arg;
            flags   |= got[i].flags;
        }
        Str what = concat(&t, "cygwin round trip: ", cmd);
        expect(what, showargs(&t, plain, got.len), want);
        expectint(what, flags, 0);
    }

    // Random command lines: no crash, sane offsets
    for (i32 iter = 0; iter < n; iter++) {
        Arena t   = scratch;
        Str   cmd = randarg(&t, &seed, special, 40, 0);
        Slice<CygArg> got = splitcygwin(&t, cmd, (b32)(iter&1));
        iz last = 0;
        b32 ok = 1;
        for (iz i = 0; i < got.len; i++) {
            ok &= got[i].end>last && got[i].end<=cmd.len;
            last = got[i].end;
        }
        if (!ok) check(0, concat(&t, "cygwin ends: ", cmd), "", "");
        for (i32 p = ARGV_SHELL32; p <= ARGV_UCRT; p++) {
            splitcmdline(&t, clone(&t, cmd), p);
        }
    }
    fprintf(stderr, "wide_test: %d round trips each\n", n);
}


// Hand-picked splits

static void splits(Arena scratch)
{
    struct {
        Str cmd;
        Str want;
    } cases[] = {
        {"p",               "<p>"},
        {"p a\\\"b",        "<p> <a\\b>"},
        {"p 'C:'\\x(",      "<p> <C:\\x(>"},
        {"p \"C:\"\\a\\b(", "<p> <C:\\a\\b(>"},
        {"p @'a b'",        "<p> <@a b>"},
        {"'p q' x",         "<p q> <x>"},
        {"\"p\"'q' r",      "<pq> <r>"},
        {"p\0x y",          "<p>"},  // a C string to Cygwin
    };
    for (iz i = 0; i < countof(cases); i++) {
        Arena t   = scratch;
        Str   cmd = cases[i].cmd;
        Slice<CygArg> got = splitcygwin(&t, cmd, 1);
        Str *a = alloc<Str>(&t, got.len);
        for (iz k = 0; k < got.len; k++) a[k] = got[k].arg;
        expect(concat(&t, "splitcygwin ", cmd), showargs(&t, a, got.len), cases[i].want);
    }

    Arena t = scratch;
    Slice<CygArg> g = splitcygwin(&t, "p ~/x* @r\"s t\" @ a{b,c} \"*\" x", 1);
    expectint("cygwin flags count", g.len, 7);
    if (g.len == 7) {
        expectint("~/x* flags", g[1].flags, CYG_GLOB);
        expect("~/x* pat", g[1].pat, "~/x*");
        expectint("@file flags", g[2].flags, CYG_ATFILE);
        expect("@file name", g[2].file, "rs t");
        expect("@file arg", g[2].arg, "@rs t");
        expectint("@ alone", g[3].flags, 0);
        expect("brace pat", g[4].pat, "a{b,c}");
        expectint("brace flags", g[4].flags, CYG_GLOB);
        expectint("quoted * flags", g[5].flags, 0);
        expect("quoted * pat", g[5].pat, "\\*");
        expect("quoted * arg", g[5].arg, "*");
        expectint("end", g[5].end, 28);
        expectint("last end", g[6].end, 29);
    }
}


// Inherited CRT descriptors

static Str hexstr(Arena *a, Str s)
{
    Buf b(a);
    for (iz i = 0; i < s.len; i++) printhex(&b, s[i], 2);
    return finish(&b);
}

static void crtfds(Arena scratch)
{
    Arena t = scratch;
    CrtFd fds[5] = {
        {0x10,               CRTFD_OPEN|CRTFD_DEV},
        {0x7fffffff12345678, CRTFD_OPEN|CRTFD_PIPE},
        {0x99,               0},  // closed: flags 0, handle -1
        {-2,                 CRTFD_OPEN},
        {0x44,               0},  // trailing closed: dropped
    };
    Str b8 = encodecrtfds(&t, fds, 5, 8);
    expect("crtfds width 8", hexstr(&t, b8),
           "04000000" "41090001"
           "1000000000000000" "78563412ffffff7f"
           "ffffffffffffffff" "feffffffffffffff");
    Slice<CrtFd> d8 = decodecrtfds(&t, b8, 8);
    expectint("crtfds decode 8 count", d8.len, 4);
    if (d8.len == 4) {
        expectint("crtfds decode 8 h1", d8[1].handle, 0x7fffffff12345678);
        expectint("crtfds decode 8 f1", d8[1].flags, CRTFD_OPEN|CRTFD_PIPE);
        expectint("crtfds decode 8 h2", d8[2].handle, -1);
        expectint("crtfds decode 8 f2", d8[2].flags, 0);
        expectint("crtfds decode 8 h3", d8[3].handle, -2);
    }

    fds[1].handle = 0x12345678;
    Str b4 = encodecrtfds(&t, fds, 4, 4);
    expect("crtfds width 4", hexstr(&t, b4),
           "04000000" "41090001" "10000000" "78563412" "ffffffff" "feffffff");
    Slice<CrtFd> d4 = decodecrtfds(&t, b4, 4);
    expectint("crtfds decode 4 count", d4.len, 4);
    if (d4.len == 4) {
        expectint("crtfds decode 4 h0", d4[0].handle, 0x10);
        expectint("crtfds decode 4 sign", d4[3].handle, -2);
    }

    CrtFd none[3] = {};
    expectint("crtfds none open", encodecrtfds(&t, none, 3, 8).len, 0);
    expectint("crtfds empty", encodecrtfds(&t, none, 0, 4).len, 0);

    // The 65535-byte cap
    iz     many = 20000;
    CrtFd *big  = alloc<CrtFd>(&t, many);
    for (iz i = 0; i < many; i++) {
        big[i] = {i, CRTFD_OPEN};
    }
    Str c8 = encodecrtfds(&t, big, many, 8);
    Str c4 = encodecrtfds(&t, big, many, 4);
    expectint("crtfds cap 8", c8.len, 4 + 7281*9);
    expectint("crtfds cap 4", c4.len, 4 + 13106*5);
    expectint("crtfds cap 8 count", decodecrtfds(&t, c8, 8).len, 7281);
    Slice<CrtFd> d = decodecrtfds(&t, c4, 4);
    expectint("crtfds cap 4 count", d.len, 13106);
    expectint("crtfds cap 4 last", d.len ? d[d.len-1].handle : 0, 13105);

    // Malformed blobs
    expectint("crtfds short", decodecrtfds(&t, {b8.data, 3}, 8).len, 0);
    expectint("crtfds cut", decodecrtfds(&t, {b8.data, 4+4+2*8+3}, 8).len, 2);
    u8 cyg[16] = {};  // Cygwin's child_info starts with a zero count
    expectint("crtfds cygwin", decodecrtfds(&t, {cyg, 16}, 8).len, 0);
    u8 neg[9] = {0xff, 0xff, 0xff, 0xff};
    expectint("crtfds negative", decodecrtfds(&t, {neg, 9}, 4).len, 0);
    expectint("crtfds wrong width", decodecrtfds(&t, b4, 8).len, 2);
}


// PE images

struct PeSpec {
    i32 machine;
    b32 pe64;
    i32 subsystem;
    b32 dll;
    b32 mark;
    Str imports[4];
};

static void put(Str img, iz off, u64 v, i32 n)
{
    for (i32 i = 0; i < n; i++) {
        img[off+i] = (u8)(v >> (8*i));
    }
}

// A minimal image: headers, .text, .idata at RVA 0x2000 (file 0x600),
// and optionally .unabash.
static Str mkpe(Arena *a, PeSpec s)
{
    Str img = {alloc<u8>(a, 0xc00), 0xc00};
    img[0] = 'M';
    img[1] = 'Z';
    iz pe = 0x80;
    put(img, 0x3c, pe, 4);
    put(img, pe, 0x4550, 4);
    i32 nsect   = s.mark ? 3 : 2;
    i32 optsize = s.pe64 ? 240 : 224;
    put(img, pe+4,  s.machine, 2);
    put(img, pe+6,  nsect, 2);
    put(img, pe+20, optsize, 2);
    put(img, pe+22, 0x0102 | (s.dll ? 0x2000 : 0), 2);
    iz opt  = pe + 24;
    iz dirs = opt + (s.pe64 ? 112 : 96);
    put(img, opt, s.pe64 ? 0x20b : 0x10b, 2);
    put(img, opt+60, 0x400, 4);
    put(img, opt+68, s.subsystem, 2);
    put(img, dirs-4, 16, 4);

    iz sect = opt + optsize;
    struct { Str name; u32 va, vsize, rawsize, rawptr; } sects[] = {
        {".text",    0x1000, 0x200, 0x200, 0x400},
        {".idata",   0x2000, 0x300, 0x400, 0x600},
        {".unabash", 0x3000, 0x10,  0x200, 0xa00},
    };
    for (i32 i = 0; i < nsect; i++) {
        iz o = sect + 40*i;
        copybytes(img.data+o, sects[i].name.data, sects[i].name.len);
        put(img, o+ 8, sects[i].vsize, 4);
        put(img, o+12, sects[i].va, 4);
        put(img, o+16, sects[i].rawsize, 4);
        put(img, o+20, sects[i].rawptr, 4);
    }

    i32 n = 0;
    for (; n<countof(s.imports) && s.imports[n].len; n++) {}
    if (n) {
        put(img, dirs+8,  0x2000, 4);
        put(img, dirs+12, 20*(n+1), 4);
    }
    iz name = 0x2000 + 20*(n+1);
    for (i32 i = 0; i < n; i++) {
        put(img, 0x600 + 20*i + 12, (u64)name, 4);
        Str dll = s.imports[i];
        copybytes(img.data + 0x600 + (name-0x2000), dll.data, dll.len);
        name += dll.len + 1;
    }
    return img;
}

static iz memread(void *ctx, i64 off, u8 *buf, iz len)
{
    Str *s = (Str *)ctx;
    if (off<0 || off>=s->len) return 0;
    iz n = s->len-off < len ? s->len-(iz)off : len;
    copybytes(buf, s->data+off, n);
    return n;
}

static Str showpe(Arena *a, PeInfo p)
{
    Buf b(a);
    if (!p.ok) {
        print(&b, "not PE");
        return finish(&b);
    }
    print(&b, "machine=");
    printhex(&b, (u64)p.machine, 4);
    print(&b, " subsystem=");
    print(&b, p.subsystem);
    print(&b, " width=");
    print(&b, p.width);
    Str crt[] = {"none", "msvcrt", "ucrt"};
    print(&b, " crt=");
    print(&b, crt[p.crt]);
    Str posix[] = {"-", "cygwin", "msys"};
    print(&b, " posix=");
    print(&b, posix[p.posix]);
    print(&b, p.dll ? Str(" dll") : Str(""));
    print(&b, p.ours ? Str(" ours") : Str(""));
    return finish(&b);
}

static void pes(Arena scratch)
{
    struct {
        PeSpec spec;
        Str    want;
    } cases[] = {
        {{0x8664, 1, 3, 0, 0, {"KERNEL32.dll", "msvcrt.dll"}},
         "machine=8664 subsystem=3 width=8 crt=msvcrt posix=-"},
        {{0x14c, 0, 2, 0, 0, {"KERNEL32.dll", "api-ms-win-crt-runtime-l1-1-0.dll"}},
         "machine=014c subsystem=2 width=4 crt=ucrt posix=-"},
        {{0x8664, 1, 3, 0, 0, {"msys-2.0.dll", "KERNEL32.dll"}},
         "machine=8664 subsystem=3 width=8 crt=none posix=msys"},
        {{0x14c, 0, 3, 0, 0, {"cygwin1.dll"}},
         "machine=014c subsystem=3 width=4 crt=none posix=cygwin"},
        {{0x14c, 0, 3, 0, 0, {"MSVCR80.dll"}},
         "machine=014c subsystem=3 width=4 crt=msvcrt posix=-"},
        {{0x14c, 0, 3, 0, 0, {"msvcr120.dll"}},
         "machine=014c subsystem=3 width=4 crt=ucrt posix=-"},
        {{0x8664, 1, 3, 0, 0, {"UCRTBASE.DLL", "msvcrt.dll"}},
         "machine=8664 subsystem=3 width=8 crt=ucrt posix=-"},
        {{0xaa64, 1, 3, 1, 0, {"msvcrt20.dll"}},
         "machine=aa64 subsystem=3 width=8 crt=msvcrt posix=- dll"},
        {{0x8664, 1, 3, 0, 1, {"KERNEL32.dll"}},
         "machine=8664 subsystem=3 width=8 crt=none posix=- ours"},
        {{0x14c, 0, 3, 0, 1, {}},
         "machine=014c subsystem=3 width=4 crt=none posix=- ours"},
        {{0x14c, 0, 3, 0, 0, {"msvcrxx.dll", "msvcrt.dl"}},
         "machine=014c subsystem=3 width=4 crt=msvcrt posix=-"},
    };
    for (iz i = 0; i < countof(cases); i++) {
        Arena  t   = scratch;
        Str    img = mkpe(&t, cases[i].spec);
        PeInfo p   = pesniff(memread, &img, t);
        Buf    w(&t);
        print(&w, "pesniff case ");
        print(&w, i);
        expect(finish(&w), showpe(&t, p), cases[i].want);
    }

    // Damaged images
    Arena  t   = scratch;
    PeSpec ok  = {0x8664, 1, 3, 0, 1, {"msvcrt.dll"}};
    Str    img = mkpe(&t, ok);
    Str    full = showpe(&t, pesniff(memread, &img, t));

    Str bad = clone(&t, img);
    bad[1] = 'X';
    expect("pesniff no MZ", showpe(&t, pesniff(memread, &bad, t)), "not PE");
    bad = clone(&t, img);
    bad[0x81] = 'X';
    expect("pesniff no PE", showpe(&t, pesniff(memread, &bad, t)), "not PE");
    bad = clone(&t, img);
    put(bad, 0x3c, 0xfffffff0, 4);
    expect("pesniff far e_lfanew", showpe(&t, pesniff(memread, &bad, t)), "not PE");
    bad = clone(&t, img);
    put(bad, 0x80+24, 0x107, 2);
    expect("pesniff ROM magic", showpe(&t, pesniff(memread, &bad, t)), "not PE");
    bad = clone(&t, img);
    put(bad, 0x80+24+112+8, 0x9000, 4);
    expect("pesniff imports outside", showpe(&t, pesniff(memread, &bad, t)),
           "machine=8664 subsystem=3 width=8 crt=none posix=- ours");
    bad = clone(&t, img);
    put(bad, 0x80+24+112-4, 1, 4);
    expect("pesniff one directory", showpe(&t, pesniff(memread, &bad, t)),
           "machine=8664 subsystem=3 width=8 crt=none posix=- ours");
    bad = clone(&t, img);
    put(bad, 0x80+6, 0xffff, 2);
    expect("pesniff many sections", showpe(&t, pesniff(memread, &bad, t)), full);
    bad = clone(&t, img);
    put(bad, 0x80+24+240+40+16, 0x10, 4);  // .idata mostly not in the file
    expect("pesniff short raw data", showpe(&t, pesniff(memread, &bad, t)),
           "machine=8664 subsystem=3 width=8 crt=none posix=- ours");
    bad = clone(&t, img);
    for (iz i = 0x600; i < 0xa00; i++) bad[i] = 0x41;  // no terminator
    put(bad, 0x600+12, 0x2010, 4);
    expect("pesniff unterminated", showpe(&t, pesniff(memread, &bad, t)),
           "machine=8664 subsystem=3 width=8 crt=none posix=- ours");

    // Every truncation, and random damage: no crash
    for (iz len = 0; len <= img.len; len++) {
        Str    part = {img.data, len};
        PeInfo p    = pesniff(memread, &part, t);
        if (len == img.len) expect("pesniff whole", showpe(&t, p), full);
    }
    u64 seed = 7;
    for (i32 iter = 0; iter < 20000; iter++) {
        Arena u = t;
        Str   m = clone(&u, img);
        for (i32 k = 1 + (i32)(rng(&seed)%8); k; k--) {
            u64 x = rng(&seed);
            iz  o = (iz)(x % 0x400);  // headers and imports
            o = o<0x200 ? o : 0x600+o-0x200;
            m[o] = (u8)(x >> 32);
        }
        pesniff(memread, &m, u);
    }
    fputs("wide_test: pesniff damage ok\n", stderr);
}


// Real binaries, read-only

static iz fileread(void *ctx, i64 off, u8 *buf, iz len)
{
    FILE *f = (FILE *)ctx;
    if (fseek(f, (long)off, SEEK_SET)) return 0;
    return (iz)fread(buf, 1, (uz)len, f);
}

static void sniffreal(Arena scratch, char *path, Str want)
{
    Arena t = scratch;
    FILE *f = fopen(path, "rb");
    Str   p = {(u8 *)path, (iz)strlen(path)};
    if (!f) {
        check(0, p, "(cannot open)", want);
        return;
    }
    Str got = showpe(&t, pesniff(fileread, f, t));
    fclose(f);
    fprintf(stderr, "%s: %.*s\n", path, (int)got.len, got.data);
    expect(p, got, want);
}

// Find name in a directory of PATH, into buf.
static b32 onpath(char *buf, iz cap, char *name)
{
    char *path = getenv("PATH");
    for (char *p = path; p && *p;) {
        char *end = p + strcspn(p, ";");
        snprintf(buf, (uz)cap, "%.*s/%s", (int)(end-p), p, name);
        FILE *f = fopen(buf, "rb");
        if (f) {
            fclose(f);
            return 1;
        }
        p = *end ? end+1 : end;
    }
    return 0;
}

static void real(Arena scratch, char *dir)
{
    char  buf[1024];
    char *gitbash = getenv("GITBASH");
    Str   g = gitbash ? Str{(u8 *)gitbash, (iz)strlen(gitbash)}
                      : Str("C:/Users/wellons/git/usr/bin/bash.exe");
    iz slash = g.len;
    for (; slash>0 && g[slash-1]!='/' && g[slash-1]!='\\'; slash--) {}

    sniffreal(scratch, (char *)"C:/Windows/System32/cmd.exe",
              "machine=8664 subsystem=3 width=8 crt=ucrt posix=-");
    snprintf(buf, sizeof(buf), "%.*sls.exe", (int)slash, g.data);
    sniffreal(scratch, buf,
              "machine=8664 subsystem=3 width=8 crt=none posix=msys");
    if (onpath(buf, countof(buf), (char *)"busybox.exe")) {
        sniffreal(scratch, buf,
                  "machine=8664 subsystem=3 width=8 crt=msvcrt posix=-");
        iz n = (iz)strlen(buf) - 11;
        snprintf(buf+n, sizeof(buf)-(uz)n, "echo.exe");
        sniffreal(scratch, buf,
                  "machine=8664 subsystem=3 width=8 crt=none posix=-");
    } else {
        check(0, "busybox.exe on PATH", "", "found");
    }
    snprintf(buf, sizeof(buf), "%s/unabashed.exe", dir);
    sniffreal(scratch, buf,
              "machine=8664 subsystem=3 width=8 crt=none posix=- ours");
    snprintf(buf, sizeof(buf), "%s/unabashed32.exe", dir);
    sniffreal(scratch, buf,
              "machine=014c subsystem=3 width=4 crt=none posix=- ours");
}


int main(int argc, char **argv)
{
    iz    cap = (iz)1<<28;
    byte *mem = (byte *)malloc((uz)cap);
    Arena a   = {mem, mem+cap};

    if (argc>1 && !strcmp(argv[1], "--real")) {
        real(a, argc>2 ? argv[2] : (char *)".");
    } else {
        char *path = argc>1 ? argv[1] : (char *)"t/unit/wide.golden";
        golden(a, {(u8 *)path, (iz)strlen(path)});
        splits(a);
        roundtrips(a);
        crtfds(a);
        pes(a);
    }
    fprintf(stderr, "%d tests, %d failed\n", ntests, nfailed);
    return !!nfailed;
}
