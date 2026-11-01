// Unit tests for fakes.cpp on the virtual OS
// $ g++ -std=c++20 -g3 -Wall -Wextra -Wconversion -Wno-sign-conversion
//       -fsanitize=undefined -fsanitize-trap -o fakes_test t/unit/fakes_test.cpp
// $ ./fakes_test [t/unit]
//
// Runs each case of fakes.golden (recorded from fakes.cases by
// fakes_record.sh in the oracle image) on a fresh vos holding the files
// of fakes.fixture, and compares stdout, stderr and the exit status. The
// command lines use a small bash subset that this harness interprets:
// words, quotes, pipelines, && || ; and simple redirections, with the
// environment bash gives its children. Then it runs some hostile inputs
// too large for the golden file (deep nesting, pathological patterns).
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../base.cpp"
#include "../../api.cpp"
#include "../../fakes.cpp"
#include "../../vos.cpp"

struct Os {
    Vos vos;
};

static Vos *osvos(Os *os)
{
    return &os->vos;
}

[[noreturn]] static void os_oom()
{
    fprintf(stderr, "fakes_test: out of memory\n");
    fflush(stderr);
    __builtin_trap();
}

static b32 os_commit(Os *, byte *, iz)
{
    return 1;
}

// A fixed heap block as an arena that never commits.
static Arena newheap(Mem *m, iz size)
{
    byte *p = (byte *)malloc((uz)size);
    if (!p) os_oom();
    m->os     = 0;
    m->commit = p + size;
    m->limit  = p + size;
    Arena a = {p, p+size, m};
    return a;
}

static Str readfile(Arena *a, char const *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return {};
    Buf b(a, 1<<16);
    for (;;) {
        reserve(&b, 1<<16);
        size_t n = fread(b.data+b.len, 1, (size_t)(b.cap-b.len), f);
        if (!n) break;
        b.len += (iz)n;
    }
    fclose(f);
    return finish(&b);
}


// Fixture

static i32 hexval(u8 c)
{
    if (digit(c)) return c - '0';
    c = lowercase(c);
    return c>='a' && c<='f' ? c-'a'+10 : -1;
}

// printf %b escapes
static Str unescape(Arena *a, Str s)
{
    Buf b(a, s.len+1);
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c!='\\' || i+1==s.len) {
            putbyte(&b, c);
            continue;
        }
        c = s[++i];
        switch (c) {
        case 'n':  putbyte(&b, '\n'); break;
        case 't':  putbyte(&b, '\t'); break;
        case '\\': putbyte(&b, '\\'); break;
        case 'a':  putbyte(&b, '\a'); break;
        case 'b':  putbyte(&b, '\b'); break;
        case 'f':  putbyte(&b, '\f'); break;
        case 'r':  putbyte(&b, '\r'); break;
        case 'v':  putbyte(&b, '\v'); break;
        case 'x': {
            i32 v = 0, n = 0;
            for (; n<2 && i+1<s.len && hexval(s[i+1])>=0; n++) v = v*16 + hexval(s[++i]);
            if (n) {
                putbyte(&b, (u8)v);
            } else {
                print(&b, "\\x");
            }
        } break;
        default:
            putbyte(&b, '\\');
            putbyte(&b, c);
        }
    }
    return finish(&b);
}

static Str nextfield(Str *s)  // whitespace-separated, as read splits
{
    *s = trimleft(*s);
    iz i = 0;
    for (; i<s->len && !whitespace((*s)[i]); i++) {}
    Str r = takehead(*s, i);
    *s = cuthead(*s, i);
    return r;
}

static i32 octal(Str s)
{
    i32 v = 0;
    for (iz i = 0; i < s.len; i++) v = v*8 + s[i] - '0';
    return v;
}

static void writevos(Os *os, Arena a, Str path, Str data, i32 mode)
{
    Opened f = os_open(os, a, path, OF_WRITE|OF_CREATE|OF_TRUNC, 0666);
    assert(!f.err);
    os_write(os, f.h, data.data, data.len);
    os_close(os, f.h);
    os_chmod(os, a, path, mode);
}

static void populate(Os *os, Arena a, Str fixture)
{
    Str home = "/home/oracle/";
    while (fixture.len) {
        Cut c = cut(fixture, '\n');
        fixture = c.tail;
        Str line = c.head;
        Str kind = nextfield(&line);
        if (!kind.len || kind[0]=='#') continue;
        Str name = concat(&a, clone(&a, home), nextfield(&line));
        Str mode = nextfield(&line);
        Str rest = trim(line);
        if (kind == "d") {
            os_mkdir(os, a, name, 0777);
            os_chmod(os, a, name, octal(mode));
        } else if (kind == "f") {
            writevos(os, a, name, unescape(&a, rest), octal(mode));
        } else if (kind == "l") {
            os_symlink(os, a, mode, name);
        }
    }
    // The container's account database, for id and ls -l
    writevos(os, a, "/etc/passwd",
             "root:x:0:0:root:/root:/bin/bash\n"
             "oracle:x:1000:1000::/home/oracle:/usr/local/bin/bash\n", 0644);
    writevos(os, a, "/etc/group", "root:x:0:\noracle:x:1000:\n", 0644);

    // The image links every fake into /opt/fakes/bin, including those
    // that bash's own helpers shadow in /usr/local/bin
    Vos *v = osvos(os);
    for (iz i = 0; i < v->nfakes; i++) {
        Str opt = concat(&a, Str("/opt/fakes/bin/"), v->fakes[i].name);
        if (os_stat(os, a, opt, 0).err) {
            vinstall(v, "opt/fakes/bin", v->fakes[i].name, (i32)i);
        }
    }
}


// A bash subset: parsing

enum : u8 { RD_IN, RD_OUT, RD_APPEND, RD_DUP, RD_HERE };

struct Redir {
    i32 fd;
    u8  op;
    Str word;
    i32 dupfd;
};

struct Cmd {
    Slice<Str>   assigns;
    Slice<Str>   argv;
    Slice<Redir> redirs;
};

struct Pipeline {
    Slice<Cmd> cmds;
    u8         conn;   // connector before it: ';', '&' (&&), '|' (||)
};

struct Parser {
    Arena *a;
    Str    s;
    iz     i;
    Str    err;
};

static b32 opchar(u8 c)
{
    return c=='|' || c=='&' || c==';' || c=='<' || c=='>';
}

static void skipblanks(Parser *p)
{
    for (; p->i<p->s.len && (p->s[p->i]==' ' || p->s[p->i]=='\t'); p->i++) {}
}

// Read one word, or null data at an operator or the end.
static Str parseword(Parser *p)
{
    Str s = p->s;
    skipblanks(p);
    if (p->i==s.len || opchar(s[p->i])) return {};
    Buf b(p->a, 64);
    b32 any = 0;
    while (p->i<s.len && s[p->i]!=' ' && s[p->i]!='\t' && !opchar(s[p->i])) {
        u8 c = s[p->i++];
        any = 1;
        if (c == '\'') {
            for (;;) {
                if (p->i == s.len) {
                    p->err = "unterminated single quote";
                    return {};
                }
                c = s[p->i++];
                if (c == '\'') break;
                putbyte(&b, c);
            }
        } else if (c=='$' && p->i<s.len && s[p->i]=='\'') {
            p->i++;
            for (;;) {
                if (p->i == s.len) {
                    p->err = "unterminated $'";
                    return {};
                }
                c = s[p->i++];
                if (c == '\'') break;
                if (c!='\\' || p->i==s.len) {
                    putbyte(&b, c);
                    continue;
                }
                c = s[p->i++];
                switch (c) {
                case 'a': putbyte(&b, '\a'); break;
                case 'b': putbyte(&b, '\b'); break;
                case 'e':
                case 'E': putbyte(&b, 0x1b); break;
                case 'f': putbyte(&b, '\f'); break;
                case 'n': putbyte(&b, '\n'); break;
                case 'r': putbyte(&b, '\r'); break;
                case 't': putbyte(&b, '\t'); break;
                case 'v': putbyte(&b, '\v'); break;
                case '\\': case '\'': case '"': case '?': putbyte(&b, c); break;
                case 'x': {
                    i32 v = 0, n = 0;
                    for (; n<2 && p->i<s.len && hexval(s[p->i])>=0; n++) {
                        v = v*16 + hexval(s[p->i++]);
                    }
                    putbyte(&b, (u8)v);
                } break;
                default:
                    if (c>='0' && c<='7') {
                        i32 v = c - '0';
                        for (i32 n = 1; n<3 && p->i<s.len && s[p->i]>='0' && s[p->i]<='7'; n++) {
                            v = v*8 + s[p->i++] - '0';
                        }
                        putbyte(&b, (u8)v);
                    } else {
                        putbyte(&b, '\\');
                        putbyte(&b, c);
                    }
                }
            }
        } else if (c == '"') {
            for (;;) {
                if (p->i == s.len) {
                    p->err = "unterminated double quote";
                    return {};
                }
                c = s[p->i++];
                if (c == '"') break;
                if (c=='$' || c=='`') {
                    p->err = "expansion in double quotes";
                    return {};
                }
                if (c=='\\' && p->i<s.len && cut("$`\"\\", s[p->i]).ok) {
                    c = s[p->i++];
                }
                putbyte(&b, c);
            }
        } else if (c == '\\') {
            if (p->i == s.len) {
                p->err = "trailing backslash";
                return {};
            }
            putbyte(&b, s[p->i++]);
        } else if (cut("$`*?[]{}~!", c).ok || (c=='#' && b.len==0)) {
            p->err = "unquoted special character";
            return {};
        } else {
            putbyte(&b, c);
        }
    }
    return any ? finish(&b) : Str{};
}

static b32 isname(Str s)
{
    if (!s.len || digit(s[0])) return 0;
    for (iz i = 0; i < s.len; i++) {
        if (!fkwordc(s[i])) return 0;
    }
    return 1;
}

static Slice<Pipeline> parseline(Parser *p)
{
    Slice<Pipeline> r = {};
    Pipeline pl = {};
    pl.conn = ';';
    Cmd cmd = {};
    Str s = p->s;
    for (;;) {
        skipblanks(p);
        // A redirection, possibly with a leading descriptor number
        iz  save = p->i;
        i32 fd = -1;
        if (p->i<s.len && digit(s[p->i])) {
            iz j = p->i;
            i32 n = 0;
            for (; j<s.len && digit(s[j]); j++) n = n<1000 ? n*10 + s[j] - '0' : n;
            if (j<s.len && (s[j]=='<' || s[j]=='>')) {
                fd = n;
                p->i = j;
            }
        }
        if (p->i<s.len && (s[p->i]=='<' || s[p->i]=='>')) {
            Redir rd = {};
            if (startswith(cuthead(s, p->i), "<<<")) {
                rd.op = RD_HERE;
                rd.fd = 0;
                p->i += 3;
            } else if (startswith(cuthead(s, p->i), ">>")) {
                rd.op = RD_APPEND;
                rd.fd = 1;
                p->i += 2;
            } else if (startswith(cuthead(s, p->i), ">&")) {
                rd.op = RD_DUP;
                rd.fd = 1;
                p->i += 2;
            } else if (s[p->i] == '<') {
                rd.op = RD_IN;
                rd.fd = 0;
                p->i++;
            } else {
                rd.op = RD_OUT;
                rd.fd = 1;
                p->i++;
            }
            if (fd >= 0) rd.fd = fd;
            Str w = parseword(p);
            if (!w.data) {
                if (!p->err.data) p->err = "missing redirection target";
                return r;
            }
            if (rd.op == RD_DUP) {
                FkNum n = fkparseint(w);
                if (!n.ok) {
                    p->err = "bad descriptor";
                    return r;
                }
                rd.dupfd = (i32)n.v;
            }
            rd.word = w;
            cmd.redirs = push(p->a, cmd.redirs, rd);
            continue;
        }
        p->i = save;

        Str w = parseword(p);
        if (p->err.data) return r;
        if (w.data) {
            Cut eq = cut(w, '=');
            if (!cmd.argv.len && eq.ok && isname(eq.head)) {
                cmd.assigns = push(p->a, cmd.assigns, w);
            } else {
                cmd.argv = push(p->a, cmd.argv, w);
            }
            continue;
        }

        // An operator or the end
        skipblanks(p);
        b32 end = p->i == s.len;
        if (!cmd.argv.len) {
            p->err = "empty command";
            return r;
        }
        pl.cmds = push(p->a, pl.cmds, cmd);
        cmd = {};
        if (end) {
            r = push(p->a, r, pl);
            return r;
        }
        Str rest = cuthead(s, p->i);
        if (startswith(rest, "||") || startswith(rest, "&&") || rest[0]==';') {
            r = push(p->a, r, pl);
            pl = {};
            pl.conn = rest[0]=='|' ? '|' : rest[0];
            p->i += rest[0]==';' ? 1 : 2;
        } else if (rest[0] == '|') {
            p->i++;
        } else {
            p->err = "unsupported operator";
            return r;
        }
    }
}


// A bash subset: execution

struct Run {
    Os  *os;
    Str  err;      // harness failure
};

static Str builtins[] = {
    "echo", "printf", "true", "false", "pwd", "test", "[", "cd", "kill",
    "read", "type", "command", "exit", "set", "export", "unset", "eval",
    "exec", "source", ".", ":", "umask", "wait", "alias", "times", "let",
    "local", "declare", "hash", "help", "history", "jobs", "shift",
};

static void bashdiag(Os *os, Str a, Str b)
{
    Arena scratch = osvos(os)->a;
    Buf m(&scratch, 128);
    print(&m, "bash: line 1: ");
    print(&m, a);
    print(&m, ": ");
    print(&m, b);
    putbyte(&m, '\n');
    os_write(os, 2, m.data, m.len);
}

// Run one pipeline, returning its status.
static i32 runpipeline(Run *run, Pipeline *pl, b32 lastsimple, Arena scratch)
{
    Os *os = run->os;
    Str cwd = "/home/oracle";
    Handle in = 0;  // the outermost standard input, empty
    i32 status = 0;
    for (iz c = 0; c < pl->cmds.len; c++) {
        Cmd   *cmd = pl->cmds.data + c;
        b32    last = c == pl->cmds.len-1;
        Handle fds[10];
        Slice<Handle> opened = {};
        for (i32 i = 0; i < 10; i++) fds[i] = BADHANDLE;
        fds[0] = in;
        fds[1] = 1;
        fds[2] = 2;
        OsPipe pipe = {};
        if (!last) {
            pipe = os_pipe(os);
            fds[1] = pipe.w;
        }

        b32 failed = 0;
        for (iz r = 0; r < cmd->redirs.len && !failed; r++) {
            Redir *rd = cmd->redirs.data + r;
            if (rd->fd > 9) {
                run->err = "descriptor out of range";
                return 2;
            }
            Str path = rd->word;
            if (path.len && path[0]!='/') path = fkjoin(&scratch, cwd, path);
            Opened f = {};
            switch (rd->op) {
            case RD_IN:
                f = os_open(os, scratch, path, OF_READ, 0);
                break;
            case RD_OUT:
                f = os_open(os, scratch, path, OF_WRITE|OF_CREATE|OF_TRUNC, 0666);
                break;
            case RD_APPEND:
                f = os_open(os, scratch, path, OF_WRITE|OF_CREATE|OF_APPEND, 0666);
                break;
            case RD_DUP:
                fds[rd->fd] = fds[rd->dupfd];
                continue;
            case RD_HERE: {
                OsPipe h = os_pipe(os);
                os_write(os, h.w, rd->word.data, rd->word.len);
                os_write(os, h.w, (u8 *)"\n", 1);
                os_close(os, h.w);
                f.h = h.r;
            } break;
            }
            if (f.err) {
                bashdiag(os, rd->word, fkerrtext(f.err));
                failed = 1;
                break;
            }
            fds[rd->fd] = f.h;
            opened = push(&scratch, opened, f.h);
        }

        Str name = cmd->argv[0];
        for (iz i = 0; i < countof(builtins); i++) {
            if (name == builtins[i]) {
                run->err = "bash builtin used as a command";
                return 2;
            }
        }
        Str path = {};
        if (!failed && cut(name, '/').ok) {
            path = name[0]=='/' ? name : fkjoin(&scratch, cwd, name);
        } else if (!failed) {
            Str dirs[] = {"/usr/local/bin", "/opt/fakes/bin"};
            for (iz i = 0; i<countof(dirs) && !path.data; i++) {
                Str cand = fkjoin(&scratch, dirs[i], name);
                if (!os_stat(os, scratch, cand, 1).err) path = cand;
            }
            if (!path.data) {
                bashdiag(os, name, "command not found");
                status = 127;
                failed = 1;
            }
        }

        if (!failed) {
            // The environment bash gives its children, with temporary
            // assignments first, last one first
            Slice<Str> env = {};
            for (iz i = cmd->assigns.len-1; i >= 0; i--) {
                env = push(&scratch, env, cmd->assigns[i]);
            }
            Str base[] = {"PWD=/home/oracle", "TZ=UTC", "HOME=/home/oracle", "LANG=C",
                          lastsimple ? Str("SHLVL=0") : Str("SHLVL=1"),
                          "PATH=/usr/local/bin:/opt/fakes/bin"};
            for (iz i = 0; i < countof(base); i++) {
                Str key = cut(base[i], '=').head;
                b32 dup = 0;
                for (iz k = 0; k < cmd->assigns.len; k++) {
                    dup |= cut(cmd->assigns[k], '=').head == key;
                }
                if (!dup) env = push(&scratch, env, base[i]);
            }
            env = push(&scratch, env, concat(&scratch, Str("_="), path));

            Spawn s = {};
            s.path = path;
            s.argv = cmd->argv.data;
            s.argc = cmd->argv.len;
            s.env  = env.data;
            s.nenv = env.len;
            s.cwd  = cwd;
            SpawnFd sfds[10];
            for (i32 i = 0; i < 10; i++) {
                if (fds[i] != BADHANDLE) {
                    sfds[s.nfds].fd  = i;
                    sfds[s.nfds++].h = fds[i];
                }
            }
            s.fds = sfds;
            Spawned sp = os_spawn(os, scratch, &s);
            if (sp.err) {
                bashdiag(os, name, fkerrtext(sp.err));
                status = sp.err==E_NOENT ? 127 : 126;
            } else {
                status = os_waitproc(os, sp.proc);
            }
        } else if (!status) {
            status = 1;
        }

        for (iz i = 0; i < opened.len; i++) os_close(os, opened[i]);
        if (in != 0) os_close(os, in);
        if (!last) {
            os_close(os, pipe.w);
            in = pipe.r;
        }
    }
    return status;
}

static i32 runline(Run *run, Str line, Arena scratch)
{
    Parser p = {};
    p.a = &scratch;
    p.s = line;
    Slice<Pipeline> pls = parseline(&p);
    if (p.err.data) {
        run->err = p.err;
        return 2;
    }
    i32 status = 0;
    for (iz i = 0; i < pls.len; i++) {
        Pipeline *pl = pls.data + i;
        if (pl->conn=='&' && status)  continue;
        if (pl->conn=='|' && !status) continue;
        // bash -c execs a final simple command, which lowers SHLVL
        b32 lastsimple = i==pls.len-1 && pl->cmds.len==1;
        status = runpipeline(run, pl, lastsimple, scratch);
        if (run->err.data) return status;
    }
    return status;
}


// Golden file

struct Case {
    Str line;
    Str out;
    Str err;
    i32 status;
    b32 ok;
};

// Parse "== NAME LEN\n" plus LEN bytes and a newline.
static Str block(Str *s, Str name, b32 *ok)
{
    Cut h = cut(*s, '\n');
    Str prefix = "== ";
    if (!startswith(h.head, prefix)) {
        *ok = 0;
        return {};
    }
    Str rest = cuthead(h.head, prefix.len);
    Cut nm   = cut(rest, ' ');
    FkNum n  = fkparseint(nm.tail);
    if (nm.head!=name || !n.ok || n.v>h.tail.len) {
        *ok = 0;
        return {};
    }
    Str r = takehead(h.tail, (iz)n.v);
    *s = cuthead(h.tail, (iz)n.v);
    if (s->len && (*s)[0]=='\n') *s = cuthead(*s, 1);
    return r;
}

static void printesc(FILE *f, Str s)
{
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c == '\n') {
            fputs("\\n\n    ", f);
        } else if (c<0x20 || c>=0x7f) {
            fprintf(f, "\\x%02x", c);
        } else {
            fputc(c, f);
        }
    }
    fputc('\n', f);
}

// Hostile inputs too large for the golden file
//
// Each runs one fake directly with a generated argument list and checks
// its stdout and status. Nesting up to 1000 deep works as in GNU's tools.
// Deeper nesting, which GNU's recursive parsers accept until their stack
// runs out, must fail cleanly instead of exhausting ours: the regex
// parser reports "Regular expression too big", and expr dies as GNU's
// does, with SIGSEGV's status. Worst-case patterns must finish quickly
// with the right answer, including some that GNU takes minutes over.

static Str repeat(Arena *a, Str s, iz n)
{
    Buf b(a, s.len*n);
    for (iz i = 0; i < n; i++) print(&b, s);
    return finish(&b);
}

static Str nest(Arena *a, Str open, Str mid, Str close, iz n)
{
    return concat(a, concat(a, repeat(a, open, n), mid), repeat(a, close, n));
}

struct Stress {
    Str        name;
    Slice<Str> argv;
    Str        input;
    Str        out;
    i32        status;
};

static Stress stress(Arena *a, Str name, Str input, Str out, i32 status,
                     Str a0, Str a1 = {}, Str a2 = {}, Str a3 = {})
{
    Stress t = {};
    t.name   = name;
    t.input  = input;
    t.out    = out;
    t.status = status;
    Str args[] = {a0, a1, a2, a3};
    for (iz i = 0; i < countof(args); i++) {
        if (args[i].data) t.argv = push(a, t.argv, args[i]);
    }
    return t;
}

static b32 runstress(Os *os, Stress *t, Arena scratch)
{
    Vos *v = osvos(os);
    v->out.len = v->err.len = v->merged.len = 0;
    OsPipe h = os_pipe(os);
    os_write(os, h.w, t->input.data, t->input.len);
    os_close(os, h.w);

    Str     path = concat(&scratch, Str("/opt/fakes/bin/"), t->argv[0]);
    Str     env[] = {"PATH=/usr/local/bin:/opt/fakes/bin", "LANG=C"};
    SpawnFd fds[] = {{0, h.r}, {1, 1}, {2, 2}};
    Spawn   s = {};
    s.path = path;
    s.argv = t->argv.data;
    s.argc = t->argv.len;
    s.env  = env;
    s.nenv = countof(env);
    s.cwd  = "/home/oracle";
    s.fds  = fds;
    s.nfds = countof(fds);
    Spawned sp = os_spawn(os, scratch, &s);
    os_close(os, h.r);
    i32 status = sp.err ? -1 : os_waitproc(os, sp.proc);
    b32 pass = status==t->status && v->out==t->out;
    if (!pass) {
        fprintf(stderr, "FAIL stress %.*s: status %d (want %d)\n",
                (int)t->name.len, t->name.data, status, t->status);
        fprintf(stderr, "  stdout got:  ");
        printesc(stderr, takehead(v->out, v->out.len<200 ? v->out.len : 200));
        fprintf(stderr, "  stderr got:  ");
        printesc(stderr, takehead(v->err, v->err.len<200 ? v->err.len : 200));
    }
    return pass;
}

static Slice<Stress> stresses(Arena *a)
{
    iz  ok   = 1000;   // deepest accepted nesting
    iz  deep = 10000;
    Str as   = repeat(a, "a", deep);
    Str line = concat(a, as, Str("\n"));
    Slice<Stress> r = {};

    r = push(a, r, stress(a, "grep -E nested groups", "a\n", "a\n", 0,
                          "grep", "-E", nest(a, "(", "a", ")", ok)));
    r = push(a, r, stress(a, "grep nested groups", "a\n", "a\n", 0,
                          "grep", nest(a, "\\(", "a", "\\)", ok)));
    r = push(a, r, stress(a, "grep -E too deep", "a\n", "", 2,
                          "grep", "-E", nest(a, "(", "a", ")", deep)));
    r = push(a, r, stress(a, "grep too deep", "a\n", "", 2,
                          "grep", nest(a, "\\(", "a", "\\)", deep)));
    r = push(a, r, stress(a, "grep -E nested stars", line, "1\n", 0,
                          "grep", "-cE", nest(a, "(", "a", ")*", ok)));
    r = push(a, r, stress(a, "grep many stars", line, "1\n", 0,
                          "grep", "-c", concat(a, Str("a"), repeat(a, "*", deep))));
    r = push(a, r, stress(a, "grep -E mixed repeats", line, "1\n", 0,
                          "grep", "-cE", concat(a, Str("a"), repeat(a, "+?*", deep))));
    r = push(a, r, stress(a, "grep empty repeats", "b\n", "b\n", 0,
                          "grep", "-E", Str("^a{0}*b$")));
    r = push(a, r, stress(a, "grep long literal", line, "1\n", 0,
                          "grep", "-c", as));
    r = push(a, r, stress(a, "grep long alternation", line, "1\n", 0,
                          "grep", "-cE", concat(a, repeat(a, "a|", deep), Str("b"))));
    r = push(a, r, stress(a, "grep many stars, no match", line, "0\n", 1,
                          "grep", "-cE", concat(a, repeat(a, "a*", ok), Str("b"))));
    r = push(a, r, stress(a, "grep nested stars, no match", line, "0\n", 1,
                          "grep", "-cE", "(a*)*(a*)*(a*)*(a*)*(a*)*(a*)*b"));
    r = push(a, r, stress(a, "grep exponential alternation", line, "0\n", 1,
                          "grep", "-cE", "(a|aa)*c"));
    r = push(a, r, stress(a, "grep nested plus", line, "1\n", 0,
                          "grep", "-cE", "^(a+)+$"));
    r = push(a, r, stress(a, "grep back reference, no match", line, "0\n", 1,
                          "grep", "-c", "\\(a*\\)*\\1b"));
    r = push(a, r, stress(a, "sed nested groups", "a\n", "x\n", 0,
                          "sed", concat(a, concat(a, Str("s/"), nest(a, "\\(", "a", "\\)", ok)),
                                        Str("/x/"))));
    r = push(a, r, stress(a, "sed too deep", "a\n", "", 1,
                          "sed", concat(a, concat(a, Str("s/"), nest(a, "\\(", "a", "\\)", deep)),
                                        Str("/x/"))));
    r = push(a, r, stress(a, "sed nested blocks", "a\n", "a\n", 0,
                          "sed", "-n", nest(a, "{", "p", "}", deep)));
    r = push(a, r, stress(a, "sed many commands", "a\n", repeat(a, "a\n", deep), 0,
                          "sed", "-n", repeat(a, "p;", deep)));
    r = push(a, r, stress(a, "expr nested match groups", "", "a\n", 0,
                          "expr", "a", ":", nest(a, "\\(", "a", "\\)", ok)));
    r = push(a, r, stress(a, "expr too deep match groups", "", "", 2,
                          "expr", "a", ":", nest(a, "\\(", "a", "\\)", deep)));

    Stress t = stress(a, "expr nested parentheses", "", "1\n", 0, "expr");
    for (iz i = 0; i < ok-1; i++) t.argv = push(a, t.argv, Str("("));
    t.argv = push(a, t.argv, Str("1"));
    for (iz i = 0; i < ok-1; i++) t.argv = push(a, t.argv, Str(")"));
    r = push(a, r, t);

    t = stress(a, "expr too deep parentheses", "", "", 128+11, "expr");
    for (iz i = 0; i < deep; i++) t.argv = push(a, t.argv, Str("("));
    t.argv = push(a, t.argv, Str("1"));
    for (iz i = 0; i < deep; i++) t.argv = push(a, t.argv, Str(")"));
    r = push(a, r, t);

    t = stress(a, "expr nested keywords", "", "1\n", 0, "expr");
    for (iz i = 0; i < ok-1; i++) t.argv = push(a, t.argv, Str("length"));
    t.argv = push(a, t.argv, Str("abc"));
    r = push(a, r, t);

    t = stress(a, "expr too deep keywords", "", "", 128+11, "expr");
    for (iz i = 0; i < deep; i++) t.argv = push(a, t.argv, Str("length"));
    t.argv = push(a, t.argv, Str("abc"));
    r = push(a, r, t);

    t = stress(a, "expr long sum", "", "10001\n", 0, "expr", "1");
    for (iz i = 0; i < deep; i++) t.argv = push(a, push(a, t.argv, Str("+")), Str("1"));
    r = push(a, r, t);

    // Big numbers: (10^n - 1)^2 and (10^2n - 1) / (10^n - 1)
    iz  nd     = 5000;
    Str nines  = repeat(a, "9", nd);
    Str nines2 = repeat(a, "9", 2*nd);
    Str square = concat(a, concat(a, repeat(a, "9", nd-1), Str("8")),
                        concat(a, repeat(a, "0", nd-1), Str("1\n")));
    r = push(a, r, stress(a, "expr big product", "", square, 0,
                          "expr", nines, "*", nines));
    r = push(a, r, stress(a, "expr big quotient", "",
                          concat(a, concat(a, Str("1"), repeat(a, "0", nd-1)), Str("1\n")), 0,
                          "expr", nines2, "/", nines));
    r = push(a, r, stress(a, "expr big remainder", "", "0\n", 1,
                          "expr", nines2, "%", nines));

    return r;
}

int main(int argc, char **argv)
{
    char const *dir = argc>1 ? argv[1] : "t/unit";
    Mem   pm, vm, sm;
    Arena perm    = newheap(&pm, (iz)1<<26);
    Arena vosmem  = newheap(&vm, (iz)1<<27);
    Arena scratch = newheap(&sm, (iz)1<<28);

    char path[512];
    snprintf(path, sizeof(path), "%s/fakes.golden", dir);
    Str golden = readfile(&perm, path);
    snprintf(path, sizeof(path), "%s/fakes.fixture", dir);
    Str fixture = readfile(&perm, path);
    if (!golden.data || !fixture.data) {
        fprintf(stderr, "fakes_test: cannot read %s/fakes.golden or fakes.fixture\n", dir);
        return 1;
    }

    static Os os;
    Vos *v = &os.vos;
    v->outcap     = (iz)1 << 20;
    v->out.data   = (u8 *)malloc((uz)v->outcap);
    v->err.data   = (u8 *)malloc((uz)v->outcap);
    v->merged.data = (u8 *)malloc((uz)v->outcap);

    i32 ntests = 0, nfailed = 0;
    Str s = golden;
    while (s.len && s[0]=='#') s = cut(s, '\n').tail;
    while (s.len) {
        b32 ok = 1;
        Case c = {};
        c.line = block(&s, "case", &ok);
        c.out  = block(&s, "stdout", &ok);
        c.err  = block(&s, "stderr", &ok);
        Cut st = cut(s, '\n');
        s = st.tail;
        FkNum n = fkparseint(cuthead(st.head, st.head.len<10 ? st.head.len : 10));
        if (!ok || !startswith(st.head, "== status ") || !n.ok) {
            fprintf(stderr, "fakes_test: malformed golden file near case %d\n", ntests+1);
            return 1;
        }
        c.status = (i32)n.v;
        ntests++;

        vreset(v, vosmem, fakes, countof(fakes));
        populate(&os, scratch, fixture);
        v->out.len = v->err.len = v->merged.len = 0;
        v->in = {};
        v->inpos = 0;

        Run run = {};
        run.os = &os;
        i32 status = runline(&run, c.line, scratch);
        b32 pass = !run.err.data && v->out==c.out && v->err==c.err && status==c.status;
        if (!pass) {
            nfailed++;
            fprintf(stderr, "FAIL %d: %.*s\n", ntests, (int)c.line.len, c.line.data);
            if (run.err.data) {
                fprintf(stderr, "  harness: %.*s\n", (int)run.err.len, run.err.data);
            }
            if (!(v->out == c.out)) {
                fprintf(stderr, "  stdout want: ");
                printesc(stderr, c.out);
                fprintf(stderr, "  stdout got:  ");
                printesc(stderr, v->out);
            }
            if (!(v->err == c.err)) {
                fprintf(stderr, "  stderr want: ");
                printesc(stderr, c.err);
                fprintf(stderr, "  stderr got:  ");
                printesc(stderr, v->err);
            }
            if (status != c.status) {
                fprintf(stderr, "  status want %d, got %d\n", c.status, status);
            }
        }
    }

    Slice<Stress> st = stresses(&perm);
    for (iz i = 0; i < st.len; i++) {
        ntests++;
        vreset(v, vosmem, fakes, countof(fakes));
        populate(&os, scratch, fixture);
        nfailed += !runstress(&os, st.data+i, scratch);
    }

    printf("%d tests, %d failed\n", ntests, nfailed);
    return nfailed != 0;
}
