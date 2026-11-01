// Executor: commands, redirections, subshells and programs
//
// Subshells and command substitutions run on the caller's stack,
// isolated by the store's journal and the redirection undo list.
// Pipeline stages run concurrently (proc.cpp, sched.cpp).
//
// This is free and unencumbered software released into the public domain.

struct Builtin {
    Str name;
    i32 (*fn)(Shell *, Str *, iz, Arena);
    i32 flags;
};

enum : i32 {
    B_SPECIAL = 1<<0,  // POSIX special builtin
    B_DECL    = 1<<1,  // declaration builtin: assignment arguments
};

static Builtin *findbuiltin(Str name);
static b32      parsei64(Str, i64 *);
static void     execute(Shell *, Node *, Arena);
static i32      runsource(Shell *, Str src, Str iname, Arena, b32 reader = 0, i32 line = 1,
                          b32 (*fill)(void *, Lexer *) = 0, void *fillctx = 0);
static Str      runfunsub(Shell *, Arena *, Str src, b32 valsub, Arena);

// Builtin output, written to fd 1 when the builtin returns, or when
// the buffer fills: growing it would overrun the builtin's scratch.
static void bprint(Shell *sh, Str s)
{
    BOut *o = sh->bout;
    if (o->b.cap-o->b.len < s.len) {
        if (o->b.len) {
            o->failed |= !fdwrite(sh, 1, finish(&o->b));
            o->b.len = 0;
        }
        if (o->b.cap < s.len) {
            o->failed |= !fdwrite(sh, 1, s);
            return;
        }
    }
    print(&o->b, s);
}

static void bprinti(Shell *sh, i64 v)
{
    u8  tmp[32];
    Buf b = {};
    b.data = tmp;
    b.cap  = countof(tmp);
    print(&b, v);
    bprint(sh, finish(&b));
}

// The scratch arena for a nesting depth, reserved on first use.
static Arena *scratchat(Shell *sh, i32 depth)
{
    if (depth >= NPOOL) {
        os_oom();
    }
    if (!sh->pool[depth]) {
        Arena a = newarena(sh->os, sh->reserve);
        sh->pool[depth] = alloc<Arena>(&a);
        *sh->pool[depth] = a;
    }
    return sh->pool[depth];
}

// Before memory holding the BASH_COMMAND node is reused, keep its text
// in the store instead.
static void keepcmdtext(Shell *sh, byte *beg, byte *end)
{
    byte *p = (byte *)sh->cmdnode;
    if (!p || p<beg || p>=end) return;
    Str s = {};
    if (sh->capdepth+1 < NPOOL) {
        Arena scratch = *scratchat(sh, sh->capdepth+1);
        s = cmdtext(sh, &scratch);
    }
    Store *st  = sh->store;
    Str    old = sh->cmdtext;
    sh->cmdtext = sclone(st, s);
    sh->cmdnode = 0;
    if ((byte *)old.data >= st->mark) {
        sfreestr(st, old);
    }
}


// Arithmetic

struct ShellArith {
    Shell *sh;
    Arena *a;
    b32    readonly;  // an assignment hit a readonly variable
};

static Str arithget(void *ctx, ArithRef *ref, b32 *isset, Arena *a)
{
    ShellArith *c = (ShellArith *)ctx;
    if (ref->hassub) {
        X    *x = newx(c->sh, a);
        Param p = getelems(x, ref->name, ref->sub);
        if (p.list) {
            p.value = joinparams(a, Args{p.lv, p.ln}, starsep(c->sh));
        }
        *isset = p.set || varvisible(c->sh, ref->name);
        return p.value;
    }
    Got g = getvar(c->sh, a, ref->name);
    *isset = g.set;
    return g.value;
}

static Str arithset(void *ctx, ArithRef *ref, i64 value, Arena *a)
{
    ShellArith *c = (ShellArith *)ctx;
    Str v = fmti64(a, value);
    if (ref->hassub) {
        Sub s = evalsub(c->sh, a, ref->name, ref->sub, {});
        if (!s.ok) {
            return concat3(a, ref->name, "[", concat(a, ref->sub, "]: bad array subscript"));
        }
        if (!setelem(c->sh, *a, ref->name, s.idx, s.key, v, SV_QUIET)) {
            c->readonly = 1;
            return concat(a, ref->name, ": readonly variable");
        }
        return {};
    }
    if (!setvar(c->sh, *a, ref->name, v, SV_QUIET)) {
        c->readonly = 1;
        return concat(a, ref->name, ": readonly variable");
    }
    return {};
}

static Arith arithrun(Shell *sh, Arena *a, Str expr, b32 *readonly)
{
    ShellArith c = {sh, a, 0};
    ArithEnv env = {};
    env.ctx     = &c;
    env.get     = arithget;
    env.set     = arithset;
    env.nounset = sh->opt[O_NOUNSET];
    Arith r = arith(&env, a, expr);
    if (!r.ok && r.unbound) {
        fatalerror(sh, *a, r.err);  // set -u: fatal, with no prefix
    }
    *readonly = c.readonly;
    return r;
}

static Arith arithraw(Shell *sh, Arena *a, Str expr)
{
    b32 readonly = 0;
    return arithrun(sh, a, expr, &readonly);
}

// Evaluate an arithmetic expansion: an error abandons the command.
static Arith evalarith(Shell *sh, Arena *a, Str expr)
{
    Arith r = arithraw(sh, a, expr);
    if (!r.ok) {
        expansionerror(sh, *a, r.err);
    }
    return r;
}

// Evaluate the value assigned to an integer variable. As in bash, an
// error abandons not only the command but any -c, eval or source text.
static Arith evalinteger(Shell *sh, Arena *a, Str expr)
{
    Arith r = arithraw(sh, a, expr);
    if (!r.ok) {
        shellerror(sh, *a, r.err);
        sh->status = 1;
        escape(sh, ESC_ABORT);
    }
    return r;
}

// Evaluate arithmetic for a command such as (( )) or let, where an
// error is reported with a prefix and the command just fails.
static Arith evalarithcmd(Shell *sh, Arena *a, Str expr, Str who)
{
    b32 readonly = 0;
    Arith r = arithrun(sh, a, expr, &readonly);
    if (!r.ok && readonly) {
        shellerror(sh, *a, r.err);  // bash reports these with no prefix
    } else if (!r.ok) {
        builtinerror(sh, *a, who, r.err);
    }
    return r;
}

// An expansion error abandons the current complete command, and so
// ends a subshell.
static void expansionerror(Shell *sh, Arena a, Str msg)
{
    shellerror(sh, a, msg);
    sh->status = 1;
    escape(sh, ESC_DISCARD);
}

// A fatal expansion error, such as an unbound variable, ends the shell.
static void fatalerror(Shell *sh, Arena a, Str msg)
{
    shellerror(sh, a, msg);
    sh->status = 1;
    escape(sh, ESC_FATAL);
}


// Subshells

enum : i32 { SUB_NODE, SUB_SOURCE, SUB_FUNSUB, SUB_CALL };

struct SubJob {
    i32   kind;
    Node *node;
    Str   src;
    b32   newloops;  // break and continue cannot see enclosing loops
    Func *fn;        // SUB_CALL: a function and its arguments
    Str  *argv;
    iz    argc;
};

static i32 callfunction(Shell *, Func *, Str *, iz, Arena);

static void runjob(Shell *sh, SubJob *j, Arena scratch)
{
    switch (j->kind) {
    case SUB_NODE:
        execute(sh, j->node, scratch);
        break;
    case SUB_SOURCE:
        // Parsed again at run time, so its syntax errors are its own
        sh->tracedepth++;  // restored with the subshell
        runsource(sh, j->src, "command substitution", scratch, 0, sh->line);
        break;
    case SUB_FUNSUB:
        runsource(sh, j->src, sh->iname, scratch, 0, sh->line);
        break;
    case SUB_CALL:
        sh->status = callfunction(sh, j->fn, j->argv, j->argc, scratch);
        break;
    }
}

// Run a job in a synchronous subshell and return its exit status.
// Everything it changes is rolled back. With cap, its standard output
// is collected there.
[[gnu::noinline]]
static i32 subshell(Shell *sh, SubJob *j, Arena scratch, Buf *cap)
{
    Shell     saved = *sh;
    Watermark w     = storeenter(sh->store);
    Escape    e     = {};
    e.up    = sh->esc;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    sh->subshell++;
    sh->bashpid = ++sh->lastpid;
    // The frame is not in the store, so local's changes to its list of
    // locals are not journaled: restore it by hand
    Frame frame = sh->frame ? *sh->frame : Frame{};
    if (j->newloops) sh->loopdepth = 0;
    if (cap) {
        File *out = newfile(sh, F_CAPTURE, OF_WRITE);
        out->cap = cap;
        fdset(sh, 1, out, 0, &scratch);
    }
    if (!__builtin_setjmp(e.jmp)) {
        runjob(sh, j, scratch);
    }
    i32 status = sh->status;
    unwind(sh, saved.undo);
    storeleave(sh->store, &w);
    // Keep the scratch arenas the subshell reserved for deeper levels:
    // they are not store memory, and dropping them leaks reservations
    for (i32 i = 0; i < NPOOL; i++) saved.pool[i] = sh->pool[i];
    saved.lastpid = sh->lastpid;  // pids are not reused
    *sh = saved;
    if (sh->frame) *sh->frame = frame;
    sh->status = status;
    return status;
}

static Str trimnewlines(Str s)
{
    while (s.len && s[s.len-1]=='\n') s.len--;
    return s;
}

// The result of a substitution from its output: as bash, drop NUL
// bytes with a warning, then trailing newlines.
static Str capresult(Shell *sh, Arena scratch, Str s)
{
    iz n = 0;
    for (iz i = 0; i < s.len; i++) {
        s[n] = s[i];
        n += s[i] != 0;
    }
    if (n < s.len) {
        shellerror(sh, scratch, "warning: command substitution: ignored null byte in input");
    }
    s.len = n;
    return trimnewlines(s);
}

static Str readwholefile(Shell *, Arena *, Str, i32 *);

// The file named by $(< file), or a null string if src is not that form.
static Str fileread(Shell *sh, Arena *a, Str src)
{
    Str t = trim(src);
    if (!t.len || t[0]!='<') return {};
    Arena tmp = *a;
    Lexer *L = newlexer(&tmp, t);
    L->stackmax = (iz)1<<20;
    Parsed p = parseunit(L);
    Node *n = p.cmd;
    if (p.status!=PARSE_OK || !n || n->kind!=N_SIMPLE || n->words || !n->redirs ||
            n->redirs->next || n->redirs->op!=R_INPUT || n->redirs->src!=0 ||
            parseunit(L).status!=PARSE_EOF) {
        return {};
    }
    return n->redirs->word->text;
}

// Run a command substitution and return its output, without trailing
// newlines. kind: 0 $(...), 1 `...`, 2 ${ ...; }.
static Str runcapture(Shell *sh, Arena *a, Str src, b32 kind)
{
    sh->nsubst++;
    if (kind != 2) {
        Str word = fileread(sh, a, src);
        if (word.data) {
            // $(< file): read the file in the current shell
            Str name = expandstring(sh, a, word);
            i32 err;
            Str data = readwholefile(sh, a, name, &err);
            if (err) {
                shellerror(sh, *a, concat3(a, name, Str(": "), errtext(err)));
                sh->status = 1;
                return {};
            }
            sh->status = 0;
            return capresult(sh, *a, data);
        }
    }
    Buf *cap = alloc<Buf>(a);
    *cap = Buf(a, 256);

    i32   depth = ++sh->capdepth;
    Arena child = *scratchat(sh, depth);
    SubJob j = {};
    j.kind = SUB_SOURCE;
    j.src  = src;
    if (kind==2 && src.len && src[0]=='|') {
        // ${| cmd; }: the current shell, expanding to a local REPLY
        Str val = runfunsub(sh, a, cuthead(src, 1), 1, child);
        sh->capdepth = depth - 1;
        sh->current = 0;
        return val;
    } else if (kind == 2) {
        // ${ cmd; }: the current shell, with only fd 1 redirected
        Undo *mark = sh->undo;
        File *f = newfile(sh, F_CAPTURE, OF_WRITE);
        f->cap = cap;
        fdset(sh, 1, f, 0, &child);
        runfunsub(sh, a, src, 0, child);
        unwind(sh, mark);
    } else {
        b32 errexit = sh->opt[O_ERREXIT];
        b32 verbose = sh->opt[O_VERBOSE];
        if (!sh->shopt[S_INHERIT_ERREXIT] && !sh->opt[O_POSIX]) {
            sh->opt[O_ERREXIT] = 0;
        }
        sh->opt[O_VERBOSE] = 0;  // as bash, which turns off -v there
        subshell(sh, &j, child, cap);
        sh->opt[O_ERREXIT] = errexit;
        sh->opt[O_VERBOSE] = verbose;
    }
    sh->capdepth = depth - 1;
    sh->current = 0;
    return capresult(sh, *a, finish(cap));
}


// Redirections

static Str redirtarget(Shell *sh, Arena *a, Word *w, b32 *ok)
{
    *ok = 0;
    Slice<Str> r = expandwords(sh, a, w, sh->interactive ? 0 : XW_NOSPLIT*0);
    if (r.len != 1) {
        shellerror(sh, *a, concat(a, w->text, ": ambiguous redirect"));
        return {};
    }
    *ok = 1;
    return r[0];
}

static File *openfile(Shell *sh, Arena *a, Str name, i32 flags)
{
    if (!name.len) {
        // absolute() would turn it into the working directory
        shellerror(sh, *a, concat(a, name, ": No such file or directory"));
        return 0;
    }
    Str path = absolute(sh, a, name);
    // bash's own /dev/fd, /dev/stdin and friends
    i32 dupfd = -1;
    if (name == "/dev/stdin")  dupfd = 0;
    if (name == "/dev/stdout") dupfd = 1;
    if (name == "/dev/stderr") dupfd = 2;
    if (startswith(name, "/dev/fd/") && name.len>8) {
        i64 n = 0;
        b32 num = 1;
        for (iz i = 8; i < name.len; i++) {
            num &= digit(name[i]);
            n = n*10 + (name[i] - '0');
            if (n > FDMAX) num = 0;
        }
        if (num) dupfd = (i32)n;
    }
    if (dupfd >= 0) {
        File *f = dupfd<FDMAX ? sh->fds->fd[dupfd] : 0;
        if (!f) {
            shellerror(sh, *a, concat(a, name, ": Bad file descriptor"));
            return 0;
        }
        f->refs++;
        return f;
    }
    Opened o = os_open(sh->os, *a, path, flags, 0666);
    if (o.err) {
        shellerror(sh, *a, concat3(a, name, Str(": "), errtext(o.err)));
        return 0;
    }
    File *f = newfile(sh, F_OS, flags);
    f->h = o.h;
    return f;
}

static i32 parsefd(Str s)
{
    if (!s.len || s.len>9) return -1;
    i32 n = 0;
    for (iz i = 0; i < s.len; i++) {
        if (!digit(s[i])) return -1;
        n = n*10 + (s[i] - '0');
    }
    return n;
}

// Under set -C, refuse to truncate an existing regular file.
static b32 clobbers(Shell *sh, Arena *a, Str name)
{
    if (!sh->opt[O_NOCLOBBER]) return 0;
    Stat st = os_stat(sh->os, *a, absolute(sh, a, name), 1);
    if (!st.err && st.type==FT_FILE) {
        shellerror(sh, *a, concat(a, name, ": cannot overwrite existing file"));
        return 1;
    }
    return 0;
}

// Apply a command's redirections. Returns false after reporting an
// error (the redirections done so far are left for the caller to undo).
static b32 doredirs(Shell *sh, Redir *list, Arena *a, b32 keep)
{
    Fdtab *t = sh->fds;
    for (Redir *r = list; r; r = r->next) {
        i32 fd   = r->src;
        b32 kept = keep;
        if (r->srcvar) {
            // {var}>: allocate a descriptor from 10 up
            fd = FDUSER;
            while (fd<FDMAX && t->fd[fd]) fd++;
            if (fd == FDMAX) {
                shellerror(sh, *a, "redirection error: cannot duplicate fd: Too many open files");
                return 0;
            }
            if (r->op==R_CLOSE || r->op==R_MOVEIN || r->op==R_MOVEOUT) {
                Got g = getvar(sh, a, r->srcvar->text);
                fd = parsefd(g.value);
                if (fd < 0) {
                    shellerror(sh, *a, concat(a, r->srcvar->text, ": ambiguous redirect"));
                    return 0;
                }
            } else {
                setvar(sh, *a, r->srcvar->text, fmti64(a, fd));
                // The new descriptor outlives the command
                kept |= !sh->shopt[S_VARREDIR_CLOSE];
            }
        }
        if (fd<0 || fd>=FDMAX) {
            shellerror(sh, *a, concat(a, fmti64(a, fd), ": Bad file descriptor"));
            return 0;
        }

        File *f = 0;
        b32   ok;
        switch (r->op) {
        case R_OUTPUT:
        case R_CLOBBER: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            if (r->op==R_OUTPUT && clobbers(sh, a, name)) return 0;
            f = openfile(sh, a, name, OF_WRITE|OF_CREATE|OF_TRUNC);
            if (!f) return 0;
            fdset(sh, fd, f, kept, a);
        } break;
        case R_APPEND: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            f = openfile(sh, a, name, OF_WRITE|OF_CREATE|OF_APPEND);
            if (!f) return 0;
            fdset(sh, fd, f, kept, a);
        } break;
        case R_INPUT: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            f = openfile(sh, a, name, OF_READ);
            if (!f) return 0;
            fdset(sh, fd, f, kept, a);
        } break;
        case R_INOUT: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            f = openfile(sh, a, name, OF_READ|OF_WRITE|OF_CREATE);
            if (!f) return 0;
            fdset(sh, fd, f, kept, a);
        } break;
        case R_ERRANDOUT:
        case R_APPENDERRANDOUT: {
            Str name = redirtarget(sh, a, r->word, &ok);
            if (!ok) return 0;
            if (r->op==R_ERRANDOUT && clobbers(sh, a, name)) return 0;
            i32 flags = OF_WRITE|OF_CREATE;
            flags |= r->op==R_ERRANDOUT ? OF_TRUNC : OF_APPEND;
            f = openfile(sh, a, name, flags);
            if (!f) return 0;
            fdset(sh, 1, f, kept, a);
            f->refs++;
            fdset(sh, 2, f, kept, a);
        } break;
        case R_DUPIN:
        case R_DUPOUT:
        case R_MOVEIN:
        case R_MOVEOUT:
        case R_DUPINWORD:
        case R_DUPOUTWORD:
        case R_MOVEINWORD:
        case R_MOVEOUTWORD: {
            i32 src = r->dst;
            b32 move = r->op==R_MOVEIN || r->op==R_MOVEOUT ||
                       r->op==R_MOVEINWORD || r->op==R_MOVEOUTWORD;
            b32 word = r->op==R_DUPINWORD || r->op==R_DUPOUTWORD ||
                       r->op==R_MOVEINWORD || r->op==R_MOVEOUTWORD;
            // A bad source is named by its number, but a word dup onto
            // the default descriptor is named by its text, and a word dup
            // onto another by the redirector, as bash reports them
            Str bad = {};
            if (word) {
                // A move expands with its dash, as bash: <&$unset- closes
                Word dashed = *r->word;
                if (move) dashed.text = concat(a, r->word->text, "-");
                Str w = redirtarget(sh, a, &dashed, &ok);
                if (!ok) return 0;
                if (move && w.len>1) {
                    w = cuttail(w, 1);
                }
                if (w == "-") {
                    fdset(sh, fd, 0, kept, a);
                    break;
                }
                src = parsefd(w);
                if (src < 0) {
                    if (r->op == R_DUPOUTWORD && fd == 1) {
                        // >&file: both stdout and stderr
                        if (clobbers(sh, a, w)) return 0;
                        f = openfile(sh, a, w, OF_WRITE|OF_CREATE|OF_TRUNC);
                        if (!f) return 0;
                        fdset(sh, 1, f, kept, a);
                        f->refs++;
                        fdset(sh, 2, f, kept, a);
                        break;
                    }
                    shellerror(sh, *a, concat(a, w, ": ambiguous redirect"));
                    return 0;
                }
                b32 dflt = fd == (r->op==R_DUPOUTWORD || r->op==R_MOVEOUTWORD);
                bad = dflt && !move ? r->word->text : fmti64(a, fd);
            } else {
                bad = fmti64(a, src);
            }
            if (src == fd) break;  // even when closed
            if (src<0 || src>=FDMAX || !t->fd[src]) {
                if (word && move) {
                    Buf b(a, 64);
                    print(&b, sh->name);
                    print(&b, ": redirection error: cannot duplicate fd: Bad file descriptor\n");
                    flushbout(sh);
                    fdwrite(sh, 2, finish(&b));
                }
                shellerror(sh, *a, concat(a, bad, ": Bad file descriptor"));
                return 0;
            }
            b32 wasopen = !!t->fd[fd];
            f = t->fd[src];
            f->refs++;
            fdset(sh, fd, f, kept, a);
            if (move && (kept || wasopen)) {
                fdset(sh, src, 0, kept, a);
            } else if (move) {
                // bash restores the source only when the target was open
                fileunref(sh, t->fd[src]);
                t->fd[src] = 0;
            }
        } break;
        case R_CLOSE:
            if (t->fd[fd]) fdset(sh, fd, 0, kept, a);
            break;
        case R_HEREDOC:
        case R_HEREDOCSTRIP:
        case R_HERESTRING: {
            Str text;
            if (r->op == R_HERESTRING) {
                text = expandstring(sh, a, r->word->text);
                text = concat(a, text, "\n");
            } else if (r->word->flags & W_QUOTED) {
                text = r->body;
            } else {
                text = expandheredoc(sh, a, r->body);
            }
            f = newfile(sh, F_MEM, OF_READ);
            f->mem    = sclone(sh->store, text);
            f->ownmem = 1;
            fdset(sh, fd, f, kept, a);
        } break;
        }
    }
    return 1;
}


// xtrace

// Does a traced word need single quotes? These are bash's shell
// metacharacters, plus ~ and # where they would be special.
static b32 tracemetas(Str s)
{
    for (iz i = 0; i < s.len; i++) {
        switch (s[i]) {
        case ' ':  case '\t': case '\n': case '\'': case '"':  case '\\':
        case '|':  case '&':  case ';':  case '(':  case ')':  case '<':
        case '>':  case '!':  case '{':  case '}':  case '*':  case '[':
        case '?':  case ']':  case '^':  case '$':  case '`':
            return 1;
        case '~':
            if (!i || s[i-1]=='=' || s[i-1]==':') return 1;
            break;
        case '#':
            if (!i) return 1;
            break;
        }
    }
    return 0;
}

// Quote a word as xtrace shows it: $'...' when anything is not
// printable in the locale, else single quotes around metacharacters.
static Str tracequote(Shell *sh, Arena *a, Str s)
{
    if (!s.len) {
        return "''";
    } else if (shouldquote(s, sh->utf8)) {
        Buf b(a, 4*s.len+3);
        ansicquote(&b, s, sh->utf8);
        return finish(&b);
    } else if (tracemetas(s)) {
        return singlequote(a, s);
    }
    return s;
}

static Str traceprefix(Shell *sh, Arena *a)
{
    // Unset, there is no prefix at all
    Got g = getvar(sh, a, "PS4");
    if (!g.set) return {};

    // xtrace is off while PS4 expands, so commands in it are not
    // traced. An expansion error, even a fatal one, is reported and
    // the raw text used instead.
    Str ps4 = g.value;
    b32 xtrace = sh->opt[O_XTRACE];
    i32 status = sh->status;
    Escape e = {};
    e.up    = sh->esc;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    sh->opt[O_XTRACE] = 0;
    if (!__builtin_setjmp(e.jmp)) {
        ps4 = expandstring(sh, a, g.value);
    } else {
        unwind(sh, e.undo);
    }
    sh->esc = e.up;
    sh->opt[O_XTRACE] = xtrace;
    sh->status = status;
    if (!ps4.len) return ps4;

    // Each level of substitution, eval and source repeats its first byte
    i32 depth = sh->tracedepth + sh->evaldepth + sh->sourcedepth;
    Buf b(a, ps4.len + depth + 2);
    for (i32 i = 0; i < depth; i++) putbyte(&b, ps4[0]);
    print(&b, ps4);
    return finish(&b);
}

static void tracewords(Shell *sh, Arena a, Str *words, iz nwords)
{
    Buf b(&a, 256);
    print(&b, traceprefix(sh, &a));
    for (iz i = 0; i < nwords; i++) {
        if (i) putbyte(&b, ' ');
        print(&b, tracequote(sh, &a, words[i]));
    }
    putbyte(&b, '\n');
    fdwrite(sh, 2, finish(&b));
}

// Trace one assignment, on its own line, before it is performed. An
// empty value is shown bare.
static void traceassign(Shell *sh, Arena a, Str name, b32 append, Str value)
{
    Buf b(&a, 256);
    print(&b, traceprefix(sh, &a));
    print(&b, name);
    print(&b, append ? Str("+=") : Str("="));
    if (value.len) print(&b, tracequote(sh, &a, value));
    putbyte(&b, '\n');
    fdwrite(sh, 2, finish(&b));
}

static void traceline(Shell *sh, Arena a, Str text)
{
    Buf b(&a, 256);
    print(&b, traceprefix(sh, &a));
    print(&b, text);
    putbyte(&b, '\n');
    fdwrite(sh, 2, finish(&b));
}


// Programs

// May this shell's user execute the file? As the kernel: the owner's
// bits for the owner, then the group's, then everyone else's.
static b32 canexec(Shell *sh, Stat st)
{
    if (st.err || st.type!=FT_FILE) return 0;
    i32 bits = st.uid==sh->euid ? st.mode>>6 : st.gid==sh->egid ? st.mode>>3 : st.mode;
    return (bits & 1) || (!sh->euid && (st.mode & 0111));
}

// Search a path list for the nth match of an executable name.
static Str pathsearch(Shell *sh, Arena *a, Str path, Str name, iz nth = 0)
{
    static Str exts[] = {"", ".exe", ".com", ".bat", ".cmd"};
    u8  sep  = sh->windows ? ';' : ':';
    iz  next = sh->windows ? countof(exts) : 1;
    for (Str rest = path;;) {
        Cut c = cut(rest, sep);
        Str dir = c.head.len ? c.head : Str(".");
        for (iz e = 0; e < next; e++) {
            Str cand = concat(a, joinpath(a, dir, name), exts[e]);
            Str full = absolute(sh, a, cand);
            Stat st = os_stat(sh->os, *a, full, 1);
            if (canexec(sh, st) && !nth--) {
                return cand;  // relative when the PATH entry is, as bash shows
            }
        }
        if (!c.ok) break;
        rest = c.tail;
    }
    return {};
}

// Search PATH for the nth match of an executable name.
static Str pathlookup(Shell *sh, Arena *a, Str name, iz nth = 0)
{
    Got g = getvar(sh, a, "PATH");
    if (!g.set) {
        // As bash: only the working directory, giving the bare name
        Stat st = os_stat(sh->os, *a, absolute(sh, a, name), 1);
        return !nth && canexec(sh, st) ? name : Str{};
    }
    return pathsearch(sh, a, g.value, name, nth);
}

// The path that command -p searches, which finds the standard utilities.
static Str stdpath(Shell *sh)
{
    return sh->windows ? Str("/usr/bin;/bin") : Str("/bin:/usr/bin");
}

// Remembered command paths

static u32 hashcmdbucket(Str name)
{
    u32 h = 2166136261u;  // FNV-1, as bash hashes strings
    for (iz i = 0; i < name.len; i++) {
        h *= 16777619u;
        h ^= name[i];
    }
    return h & (NHASHCMD - 1);
}

static HashCmd *hashfind(Shell *sh, Str name)
{
    if (!sh->cmdhash) return 0;
    HashCmd *h = sh->cmdhash[hashcmdbucket(name)];
    for (; h; h = h->next) {
        if (h->name == name) return h;
    }
    return 0;
}

// Count a use. Entries a subshell's parent owns are left alone, since
// the count would be rolled back anyway, so loops do not grow the
// journal.
static void hashhit(Shell *sh, HashCmd *h)
{
    if ((byte *)h >= sh->store->mark) h->hits++;
}

// Remember a path for name, replacing any earlier one in place.
static void hashenter(Shell *sh, Str name, Str path, uz hits)
{
    Store *st = sh->store;
    HashCmd *h = hashfind(sh, name);
    if (h) {
        Str old = h->path;
        Str dup = sclone(st, path);
        jwrite(st, &h->path.data, (uz)dup.data);
        jwrite(st, &h->path.len, (uz)dup.len);
        jwrite(st, &h->hits, hits);
        if ((byte *)h >= st->mark) sfreestr(st, old);
        return;
    }
    if (!sh->cmdhash) {
        sh->cmdhash = (HashCmd **)salloc(st, NHASHCMD*(iz)sizeof(HashCmd *));
    }
    HashCmd **slot = sh->cmdhash + hashcmdbucket(name);
    h = (HashCmd *)salloc(st, sizeof(HashCmd));
    h->name = sclone(st, name);
    h->path = sclone(st, path);
    h->hits = hits;
    h->next = *slot;
    jset(st, slot, h);
}

static b32 hashremove(Shell *sh, Str name)
{
    if (!sh->cmdhash) return 0;
    HashCmd **slot = sh->cmdhash + hashcmdbucket(name);
    for (; *slot; slot = &(*slot)->next) {
        if ((*slot)->name == name) {
            jset(sh->store, slot, (*slot)->next);
            return 1;
        }
    }
    return 0;
}

// The path a command name runs: remembered, or found on PATH and then
// remembered when enter is set.
static Str commandpath(Shell *sh, Arena *a, Str name, b32 enter)
{
    HashCmd *h = hashfind(sh, name);
    if (h && sh->shopt[S_CHECKHASH]) {
        Stat st = os_stat(sh->os, *a, absolute(sh, a, h->path), 1);
        if (!canexec(sh, st)) {
            hashremove(sh, name);
            h = 0;
        }
    }
    if (h) {
        hashhit(sh, h);
        return h->path;
    }
    Str path = pathlookup(sh, a, name);
    if (path.data && enter) {
        hashenter(sh, name, path, 1);
    }
    return path;
}

// Run a program and wait for it. Returns its status, or the negated
// error that kept it from starting. Shell objects on its descriptors
// become OS pipes: text and in-memory pipes are fed by pump tasks, and
// a capture is read here until end of file.
static i32 runprogram(Shell *sh, Arena a, Str path, Str *argv, iz argc,
                      Slice<Str> env)
{
    Sched *s = sh->sched;
    Fdtab *t = sh->fds;
    SpawnFd *fds = alloc<SpawnFd>(&a, t->top ? t->top : 1);
    iz nfds = 0;
    Handle *closeafter = alloc<Handle>(&a, t->top ? t->top : 1);
    i32     nclose = 0;
    Task   *pumps[16];
    b32     input[16];
    i32     npumps = 0;
    File   *capfile = 0;
    Handle  capread = BADHANDLE;
    for (i32 fd = 0; fd < t->top; fd++) {
        File *f = t->fd[fd];
        if (!f) continue;
        SpawnFd sf = {fd, BADHANDLE};
        // A File already given to an earlier descriptor shares its handle
        b32 shared = 0;
        for (iz k = 0; k<nfds && f->kind!=F_OS; k++) {
            if (t->fd[fds[k].fd] == f) {
                sf.h   = fds[k].h;
                shared = 1;
            }
        }
        if (shared) {
            fds[nfds++] = sf;
            continue;
        }
        switch (f->kind) {
        case F_OS:
            sf.h = f->h;
            break;
        case F_MEM: {
            OsPipe p = os_pipe(sh->os);
            if (p.err) continue;
            Str rest = cuthead(f->mem, f->pos<f->mem.len ? f->pos : f->mem.len);
            iz  now  = rest.len<os_pipecap(sh->os) ? rest.len : os_pipecap(sh->os);
            os_write(sh->os, p.w, rest.data, now);
            if (now<rest.len && npumps<countof(pumps)) {
                Pump *pp = alloc<Pump>(&a);
                pp->kind = PUMP_TEXT;
                pp->h    = p.w;
                pp->text = cuthead(rest, now);
                input[npumps] = 1;
                pumps[npumps++] = spawnpump(s, pp);
            } else {
                os_close(sh->os, p.w);
            }
            sf.h = p.r;
            closeafter[nclose++] = p.r;
        } break;
        case F_CAPTURE: {
            OsPipe p = os_pipe(sh->os);
            if (p.err) continue;
            capfile = f;
            capread = p.r;
            sf.h = p.w;
            closeafter[nclose++] = p.w;
        } break;
        case F_PIPE: {
            if (npumps == countof(pumps)) continue;
            OsPipe p = os_pipe(sh->os);
            if (p.err) continue;
            Pump *pp = alloc<Pump>(&a);
            pp->pipe = f->pipe;
            if (f->flags & OF_READ) {
                pp->kind = PUMP_TOHANDLE;
                pp->h    = p.w;
                sf.h     = p.r;
            } else {
                pp->kind = PUMP_FROMHANDLE;
                pp->h    = p.r;
                sf.h     = p.w;
            }
            closeafter[nclose++] = sf.h;
            input[npumps] = pp->kind == PUMP_TOHANDLE;
            pumps[npumps++] = spawnpump(s, pp);
        } break;
        }
        fds[nfds++] = sf;
    }

    Spawn sp = {};
    sp.path  = absolute(sh, &a, path);
    sp.argv  = argv;
    sp.argc  = argc;
    sp.env   = env.data;
    sp.nenv  = env.len;
    sp.cwd   = sh->cwd;
    sp.fds   = fds;
    sp.nfds  = nfds;
    Spawned r = os_spawn(sh->os, a, &sp);
    for (i32 i = 0; i < nclose; i++) {
        os_close(sh->os, closeafter[i]);
    }
    if (sh->spawnto) {
        // A pipeline stage started directly: its waiter is runpipeline
        assert(!npumps && !capfile);
        sh->spawnto->proc = r.err ? BADHANDLE : r.proc;
        return r.err ? -r.err : 0;
    }
    if (capfile) {
        u8 buf[4096];
        for (;;) {
            iz n = taskread(s, capread, buf, countof(buf));
            if (n <= 0) break;
            print(capfile->cap, {buf, n});
        }
        os_close(sh->os, capread);
    }
    i32 status = r.err ? -r.err : taskwaitproc(s, r.proc);
    // What the program did not read stays unread; what it wrote is
    // passed on before anything the shell writes next
    for (i32 i = 0; i < npumps; i++) {
        if (input[i]) taskcancel(s, pumps[i]);
    }
    for (i32 i = 0; i < npumps; i++) {
        taskjoin(s, pumps[i]);
    }
    return status;
}

static b32 isshellscript(Shell *sh, Arena a, Str path)
{
    Opened o = os_open(sh->os, a, absolute(sh, &a, path), OF_READ, 0);
    if (o.err) return 0;
    u8 buf[80];
    iz n = os_read(sh->os, o.h, buf, countof(buf));
    os_close(sh->os, o.h);
    for (iz i = 0; i < n; i++) {
        if (!buf[i]) return 0;  // binary
    }
    return 1;
}

// Does a script look like a binary? As bash: a NUL in the first line,
// within its first 80 bytes.
static b32 binaryfile(Str s)
{
    for (iz i = 0; i<s.len && i<80; i++) {
        if (s[i] == '\n') return 0;
        if (s[i] == 0) return 1;
    }
    return 0;
}

static Str readwholefile(Shell *sh, Arena *a, Str path, i32 *err)
{
    Opened o = os_open(sh->os, *a, absolute(sh, a, path), OF_READ, 0);
    *err = o.err;
    if (o.err) return {};
    Buf b(a, 4096);
    for (;;) {
        reserve(&b, 4096);
        iz n = os_read(sh->os, o.h, b.data+b.len, b.cap-b.len);
        if (n <= 0) break;
        b.len += n;
    }
    os_close(sh->os, o.h);
    return finish(&b);
}

// Execute an external command, by path or by searching PATH, unless
// the caller has already found it.
static i32 runexternal(Shell *sh, Arena a, Str *argv, iz argc, Slice<Str> env, Str found = {})
{
    Str name = argv[0];
    Str path = found;
    b32 slash = haspath(sh, name);
    if (found.data) {
        slash = haspath(sh, found);
    } else if (slash) {
        path = name;
    } else {
        path = commandpath(sh, &a, name, 1);
        Func *handler = path.data ? 0 : findfunc(sh, "command_not_found_handle");
        if (handler && !sh->inhandler) {
            // As bash, in a subshell, given the command and arguments
            SubJob j = {};
            j.kind = SUB_CALL;
            j.fn   = handler;
            j.argc = argc + 1;
            j.argv = alloc<Str>(&a, argc+1);
            j.argv[0] = "command_not_found_handle";
            for (iz i = 0; i < argc; i++) j.argv[i+1] = argv[i];
            sh->inhandler = 1;
            i32 status = subshell(sh, &j, a, 0);
            sh->inhandler = 0;
            return status;
        }
        if (!path.data && !getvar(sh, &a, "PATH").set) {
            // bash tries the bare name, in the working directory
            shellerror(sh, a, concat(&a, name, ": No such file or directory"));
            return 127;
        } else if (!path.data) {
            Str shown = name;
            if (shouldquote(name, sh->utf8)) {
                Buf b(&a, 4*name.len+3);
                ansicquote(&b, name, sh->utf8);
                shown = finish(&b);
            }
            shellerror(sh, a, concat(&a, shown, ": command not found"));
            return 127;
        }
    }
    i32 st = runprogram(sh, a, path, argv, argc, env);
    Spawnto *direct = sh->spawnto;
    sh->spawnto = 0;
    if (st >= 0) return st;
    i32 err = -st;
    if (err==E_NOEXEC && direct && isshellscript(sh, a, path)) {
        direct->retry = 1;  // run as shell code, concurrently
        return 0;
    }
    if (err == E_NOEXEC && isshellscript(sh, a, path)) {
        // No #! line: bash runs it as a shell script in a subshell
        Str src = readwholefile(sh, &a, path, &err);
        Shell saved = *sh;
        Watermark w = storeenter(sh->store);
        Escape e = {};
        e.up = sh->esc;
        e.undo = sh->undo;
        sh->esc = &e;
        sh->args.v = argv + 1;
        sh->args.n = argc - 1;
        sh->name   = name;
        sh->argv0  = {};
        sh->frame  = 0;
        sh->funcdepth = 0;
        sh->funcs  = 0;
        sh->srcname     = {};
        sh->evaldepth   = 0;
        sh->sourcedepth = 0;
        sh->capdepth++;
        if (!__builtin_setjmp(e.jmp)) {
            runsource(sh, src, name, *scratchat(sh, sh->capdepth), 1);
        }
        i32 status = sh->status;
        unwind(sh, saved.undo);
        storeleave(sh->store, &w);
        for (i32 i = 0; i < NPOOL; i++) saved.pool[i] = sh->pool[i];
        *sh = saved;
        return status;
    }
    Stat s = os_stat(sh->os, a, absolute(sh, &a, path), 1);
    if (!s.err && s.type==FT_DIR) {
        err = E_ISDIR;
    }
    if (err == E_NOENT) {
        // a stale remembered path is reported as such
        shellerror(sh, a, concat(&a, path, ": No such file or directory"));
        return 127;
    }
    shellerror(sh, a, concat3(&a, name, Str(": "), errtext(err)));
    return err==E_NOENT ? 127 : 126;
}


// Simple commands

static void checkerrexit(Shell *sh)
{
    if (sh->status && sh->opt[O_ERREXIT] && !sh->noerrexit && !sh->exiting) {
        sh->exiting = 1;
        escape(sh, ESC_EXIT);
    }
}

// Recursion deep enough to exhaust the stack: bash would crash with
// SIGSEGV, so this shell (or subshell) dies with that status.
static void checkstack(Shell *sh)
{
    byte here = 0;
    if (sh->stackmax && sh->stacktop-&here > sh->stackmax) {
        sh->status  = 139;
        sh->exiting = 1;
        escape(sh, ESC_EXIT);
    }
}

// Leave a call's frame: pop its locals and restore the options that
// "local -" saved.
static void leaveframe(Shell *sh, Frame *f)
{
    poplocals(sh, f);
    if (f->opts) {
        for (i32 i = 0; i < O_COUNT; i++) sh->opt[i] = f->opts[i];
        sfree(sh->store, f->opts, O_COUNT*(iz)sizeof(b32));
    }
    sh->funcdepth--;
    sh->frame = f->up;
}

[[gnu::noinline]]
static void checkfuncnest(Shell *sh, Str name, Arena scratch)
{
    Got nest = getvar(sh, &scratch, "FUNCNEST");
    i64 limit = 0;
    if (nest.set && parsei64(nest.value, &limit) && limit>0 && sh->funcdepth>=limit) {
        shellerror(sh, scratch, concat3(&scratch, name,
                   Str(": maximum function nesting level exceeded ("),
                   concat(&scratch, nest.value, ")")));
        sh->status = 1;
        escape(sh, ESC_ABORT);
    }
}

static i32 callfunction(Shell *sh, Func *fn, Str *argv, iz argc, Arena scratch)
{
    safepoint(sh->sched);
    checkstack(sh);
    checkfuncnest(sh, argv[0], scratch);
    Frame f = {};
    f.up   = sh->frame;
    f.name = argv[0];
    f.line = sh->line;
    f.file = fn->code->file;
    Args saved = sh->args;
    sh->args.v = argv + 1;
    sh->args.n = argc - 1;
    sh->frame  = &f;
    sh->funcdepth++;
    i32 loopdepth = sh->loopdepth;
    sh->loopdepth = 0;

    Code *code = fn->code;
    code->refs++;
    Str srcname = sh->srcname;
    sh->srcname = code->file;

    // Pop this call on the way out of an escape too, so that a shell
    // that resumes after an expansion error has no stale frames
    Escape e = {};
    e.up    = sh->esc;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    b32 escaped = __builtin_setjmp(e.jmp);
    if (!escaped) {
        if (code->body->redirs) sh->line = code->body->line;
        execute(sh, code->body, scratch);
    }
    sh->esc = e.up;

    if (!--code->refs && code->dead) {
        freecode(sh, code);
    }
    if (sh->flow == FLOW_RETURN) {
        sh->flow = FLOW_NONE;
    }
    sh->loopdepth = loopdepth;
    sh->srcname   = srcname;
    leaveframe(sh, &f);
    sh->args = saved;
    if (escaped) {
        escape(sh, e.why);
    }
    return sh->status;
}

// Run a ${ cmd; } body in the current shell as bash does: like a
// function call for local and return, but with the caller's FUNCNAME
// and positional parameters. With valsub, ${| cmd; }, REPLY is local
// and its final value, copied to a, is the result.
static Str runfunsub(Shell *sh, Arena *a, Str src, b32 valsub, Arena scratch)
{
    checkstack(sh);
    Frame f = {};
    if (sh->frame) {
        f.name = sh->frame->name;
        f.def  = sh->frame->def;
        f.line = sh->frame->line;
        f.file = sh->frame->file;
    }
    f.up = sh->frame;
    sh->frame = &f;
    sh->funcdepth++;
    i32 loopdepth = sh->loopdepth;
    sh->loopdepth = 0;

    Escape e = {};
    e.up    = sh->esc;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    Str val = {};
    i32 tracedepth = sh->tracedepth++;
    b32 escaped = __builtin_setjmp(e.jmp);
    if (!escaped) {
        if (valsub) makelocal(sh, &scratch, "REPLY");
        runsource(sh, src, sh->iname, scratch, 0, sh->line);
        if (valsub) val = clone(a, getvar(sh, &scratch, "REPLY").value);
    }
    sh->esc = e.up;
    sh->tracedepth = tracedepth;

    if (sh->flow == FLOW_RETURN) {
        sh->flow = FLOW_NONE;
    }
    sh->loopdepth = loopdepth;
    leaveframe(sh, &f);
    if (escaped) {
        escape(sh, e.why);
    }
    return val;
}

static b32 isassignword(Word *w)
{
    return (w->flags & (W_ASSIGNMENT|W_NOSPLIT)) == (W_ASSIGNMENT|W_NOSPLIT);
}

struct Assign {
    Str name;
    Str value;
    b32 append;
};

static Assign splitassign(Shell *sh, Arena *a, Str text)
{
    Assign r = {};
    iz eq = assignment(text, 0);
    r.name = takehead(text, eq);
    if (r.name.len && r.name[r.name.len-1]=='+') {
        r.append = 1;
        r.name   = cuttail(r.name, 1);
    }
    r.value = expandstring(sh, a, cuthead(text, eq+1), XC_ASSIGN);
    return r;
}

// Perform the assignments of a command with no command word, each
// traced just before it happens. Returns false if one failed.
static b32 doassignments(Shell *sh, Arena *a, Word *w, iz n)
{
    for (iz i = 0; i < n; i++, w = w->next) {
        if (w->flags & W_COMPASSIGN) {
            // name=(...), traced as written
            iz  eq     = assignment(w->text, 0);
            Str name   = takehead(w->text, eq);
            b32 append = name.len && name[name.len-1]=='+';
            if (append) name = cuttail(name, 1);
            Str body = cuttail(cuthead(w->text, eq+2), 1);
            if (sh->opt[O_XTRACE]) {
                traceline(sh, *a, w->text);
            }
            Str base, sub;
            if (splitref(name, &base, &sub)) {
                Arena t = *a;
                shellerror(sh, t, concat(&t, name, ": cannot assign list to array member"));
                return 0;
            }
            if (!assigncompound(sh, *a, name, body, append ? SV_APPEND : 0)) {
                return 0;
            }
            continue;
        }
        Assign as = splitassign(sh, a, w->text);
        if (sh->opt[O_XTRACE]) {
            traceassign(sh, *a, as.name, as.append, as.value);
        }
        if (!assignto(sh, *a, as.name, as.value, as.append ? SV_APPEND : 0)) {
            return 0;
        }
    }
    return 1;
}

[[gnu::noinline]]
static void runsimple(Shell *sh, Node *n, Arena scratch)
{
    safepoint(sh->sched);
    sh->line    = n->line;
    sh->current = n;
    sh->cmdnode = n;
    Arena *a = &scratch;

    // Leading assignments, then the rest
    Word *assigns = n->words;
    iz    nassign = 0;
    Word *rest    = n->words;
    while (rest && isassignword(rest)) {
        nassign++;
        rest = rest->next;
    }

    // Is the command a declaration builtin? Then its assignment-looking
    // arguments expand as assignments.
    Slice<Str> argv = {};
    Slice<u8>  compargs = {};
    b32 decl = 0;
    if (rest) {
        Slice<Str> first = {};
        Word one = *rest;
        one.next = 0;
        first = expandwords(sh, a, &one, 0);
        Builtin *b = first.len ? findbuiltin(first[0]) : 0;
        decl = b && (b->flags & B_DECL);
        argv = first;
        for (Word *w = rest->next; w; w = w->next) {
            Word one2 = *w;
            one2.next = 0;
            Slice<Str> more;
            if (decl && (w->flags & W_COMPASSIGN)) {
                // name=(...) goes to the builtin as written, for it to
                // expand as an array assignment
                while (compargs.len < argv.len) compargs = push(a, compargs, (u8)0);
                compargs = push(a, compargs, (u8)1);
                argv = push(a, argv, w->text);
                continue;
            } else if (decl && (w->flags & W_ASSIGNMENT)) {
                // Brace expanded, but not split or globbed
                more = expandwords(sh, a, &one2, XW_NOSPLIT|XW_NOGLOB|XW_ASSIGN);
            } else {
                more = expandwords(sh, a, &one2, 0);
            }
            for (iz i = 0; i < more.len; i++) argv = push(a, argv, more[i]);
        }
    }

    Undo *mark = sh->undo;

    if (!argv.len) {
        // Assignments only, with redirections in a temporary context.
        // The status is the last command substitution's, or zero.
        u32  nsubst = sh->nsubst;
        b32  ok     = doassignments(sh, a, assigns, nassign);
        i32  laststatus = sh->nsubst==nsubst ? 0 : sh->status;
        if (n->redirs) {
            if (!doredirs(sh, n->redirs, a, 0)) {
                unwind(sh, mark);
                sh->status = 1;
                checkerrexit(sh);
                return;
            }
            unwind(sh, mark);
        }
        sh->status = ok ? laststatus : 1;
        if (!ok && !sh->interactive && sh->opt[O_POSIX]) {
            sh->exiting = 1;
            escape(sh, ESC_EXIT);
        } else if (!ok) {
            escape(sh, ESC_DISCARD);  // as bash abandons the command
        }
        checkerrexit(sh);
        return;
    }

    Str name = argv[0];
    sh->underscore = argv[argv.len-1];

    // Expand assignment values now (bash does it after the words)
    Assign *as = alloc<Assign>(a, nassign);
    {
        Word *w = assigns;
        for (iz i = 0; i < nassign; i++, w = w->next) {
            as[i] = splitassign(sh, a, w->text);
        }
    }

    if (sh->opt[O_XTRACE]) {
        for (iz i = 0; i < nassign; i++) {
            traceassign(sh, scratch, as[i].name, as[i].append, as[i].value);
        }
        tracewords(sh, scratch, argv.data, argv.len);
    }

    // exec without a command makes its redirections permanent
    // (outside a subshell only once all of them succeed: a failure
    // undoes the rest, as in bash)
    b32 execonly = name=="exec" && (argv.len==1 || (argv.len==2 && argv[1]=="--"));
    b32 commit   = execonly && !sh->subshell;
    if (!doredirs(sh, n->redirs, a, execonly && !commit)) {
        unwind(sh, mark);
        sh->status = 1;
        checkerrexit(sh);
        return;
    }
    if (commit) {
        commitredirs(sh, mark);
    }
    if (execonly) {
        mark = sh->undo;
    }

    Func    *fn = 0;
    Builtin *bi = findbuiltin(name);
    b32 special = bi && (bi->flags & B_SPECIAL);
    if (!(special && sh->opt[O_POSIX])) {
        if (!haspath(sh, name)) fn = findfunc(sh, name);
    }

    if (fn || bi) {
        // Temporary bindings for builtins and functions
        b32 persist = special && sh->opt[O_POSIX];
        for (iz i = 0; i < nassign; i++) {
            if (persist) {
                i32 flags = as[i].append ? SV_APPEND : 0;
                setvar(sh, scratch, as[i].name, as[i].value, flags);
                Value *v = lookup(sh, as[i].name);
                if (v && (v->attrs & V_TEMP)) {
                    // as bash, also past a function call's temporary
                    // binding, so the value outlives the call
                    setvar(sh, scratch, as[i].name, as[i].value, flags|SV_UNDERTEMP);
                }
            } else {
                pushtemp(sh, scratch, as[i].name, as[i].value);
            }
        }
        if (fn) {
            callfunction(sh, fn, argv.data, argv.len, scratch);
        } else {
            BOut  b     = {};
            BOut *saved = sh->bout;
            b.b  = Buf(a, (iz)1<<12);
            sh->bout = &b;
            Slice<u8> savedcomp = sh->compargs;
            sh->compargs = compargs;
            sh->status = bi->fn(sh, argv.data, argv.len, scratch);
            sh->compargs = savedcomp;
            sh->bout = saved;
            if ((b.b.len && !fdwrite(sh, 1, finish(&b.b))) || b.failed) {
                builtinerror(sh, scratch, name, "write error: Bad file descriptor");
                sh->status = 1;
            }
        }
        if (!persist) {
            for (iz i = nassign-1; i >= 0; i--) poptemp(sh, as[i].name);
        }
    } else {
        Slice<Str> env = makeenv(sh, a);
        for (iz i = 0; i < nassign; i++) {
            Str entry = concat3(a, as[i].name, "=", as[i].value);
            b32 replaced = 0;
            for (iz k = 0; k < env.len; k++) {
                Cut c = cut(env[k], '=');
                if (c.head == as[i].name) {
                    env[k]   = entry;
                    replaced = 1;
                }
            }
            if (!replaced) env = push(a, env, entry);
        }
        sh->status = runexternal(sh, scratch, argv.data, argv.len, env);
    }
    unwind(sh, mark);
    if (sh->flow != FLOW_RETURN) {
        checkerrexit(sh);
    }
}


// Compound commands

static b32 casematch(Shell *sh, Arena *a, Str word, Word *patterns)
{
    i32 pf = patflags(sh);
    if (sh->shopt[S_NOCASEMATCH]) pf |= PM_NOCASE;
    for (Word *p = patterns; p; p = p->next) {
        Str pat = expandpattern(sh, a, p->text, wordctx(sh, p));
        if (patmatch(pat, word, pf, *a)) return 1;
    }
    return 0;
}

static b32 condeval(Shell *sh, Arena *a, Cond *c);

struct TestCtx {
    Shell *sh;
    Arena *a;
};

static Stat envstat(void *ctx, Str path, b32 follow)
{
    TestCtx *t  = (TestCtx *)ctx;
    Shell   *sh = t->sh;
    Arena    a  = *t->a;

    // As bash, /dev/fd/N and /dev/std* name the shell's own descriptors
    i64 fd = -1;
    if (path == "/dev/stdin") {
        fd = 0;
    } else if (path == "/dev/stdout") {
        fd = 1;
    } else if (path == "/dev/stderr") {
        fd = 2;
    } else if (startswith(path, "/dev/fd/")) {
        TestNum n = testparseint(cuthead(path, 8));
        fd = n.ok && n.value>=0 ? n.value : (i64)FDMAX;
    }
    if (fd >= 0) {
        Stat  s = {};
        File *f = fd<FDMAX ? sh->fds->fd[fd] : 0;
        if (!f) {
            s.err = E_NOENT;  // bash: EBADF from fstat, reported false
        } else if (f->kind == F_OS) {
            s = os_fstat(sh->os, f->h);
        } else {
            s.type  = FT_PIPE;
            s.mode  = 0600;
            s.nlink = 1;
            s.uid   = sh->euid;
            s.gid   = sh->egid;
        }
        return s;
    }

    return os_stat(sh->os, a, absolute(sh, &a, path), follow);
}

static b32 envaccess(void *ctx, Str path, i32 mode)
{
    Stat s = envstat(ctx, path, 1);
    if (s.err) return 0;
    if (!mode) return 1;
    Shell *sh = ((TestCtx *)ctx)->sh;
    i32 bits = s.uid==sh->euid ? s.mode>>6 : s.gid==sh->egid ? s.mode>>3 : s.mode;
    if ((mode & 1) && s.type==FT_DIR) return (bits & 1) != 0;
    return (bits & mode) == mode;
}

static b32 envisatty(void *, i64)
{
    return 0;
}

static i32 envoption(void *ctx, Str name)
{
    Shell *sh = ((TestCtx *)ctx)->sh;
    for (i32 i = 0; i < O_COUNT; i++) {
        if (setoptions[i].name == name) return sh->opt[i];
    }
    return -1;
}

static b32 envvarset(void *ctx, Str name)
{
    Shell  *sh = ((TestCtx *)ctx)->sh;
    TestNum n  = testparseint(name);
    if (n.ok) {
        // As bash, a number names a positional parameter, and 0 is set
        return n.value>=0 && n.value<=sh->args.n;
    }
    Str rn, rs;
    if (splitref(name, &rn, &rs)) {
        Arena a = *((TestCtx *)ctx)->a;
        X    *x = newx(sh, &a);
        Value *v = lookup(sh, rn);
        if (v && v->arr && v->arr->bucket && allsub(rs)) {
            return !!hfind(v->arr, rs);  // a key, as bash 5.3 has it
        }
        return getelems(x, rn, rs).set;
    }
    Arena none = {};
    return getvar(sh, &none, name).set;
}

static b32 envnameref(void *ctx, Str name)
{
    Shell *sh = ((TestCtx *)ctx)->sh;
    Value *v = lookup(sh, name);
    return v && (v->attrs & V_NAMEREF);
}

static TestEnv testenv(TestCtx *t)
{
    Shell  *sh = t->sh;
    TestEnv e  = {};
    e.ctx     = t;
    e.stat    = envstat;
    e.access  = envaccess;
    e.isatty  = envisatty;
    e.option  = envoption;
    e.varset  = envvarset;
    e.nameref = envnameref;
    e.euid    = sh->euid;
    e.egid    = sh->egid;
    return e;
}

static Str condword(Shell *sh, Arena *a, Word *w)
{
    return expandstring(sh, a, w->text, wordctx(sh, w));
}

// Trace one [[ ]] primary as it is evaluated, operands unquoted. The
// right operand is null for a unary primary.
static void tracecond(Shell *sh, Arena a, b32 invert, Str op, Str lhs, Str *rhs)
{
    if (!sh->opt[O_XTRACE]) return;
    Buf b(&a, 128);
    print(&b, "[[ ");
    if (invert) print(&b, "! ");
    if (rhs) {
        print(&b, lhs.len ? lhs : Str("''"));
        putbyte(&b, ' ');
        print(&b, op);
        putbyte(&b, ' ');
        print(&b, rhs->len ? *rhs : Str("''"));
    } else {
        print(&b, op);
        putbyte(&b, ' ');
        print(&b, lhs.len ? lhs : Str("''"));
    }
    print(&b, " ]]");
    traceline(sh, a, finish(&b));
}

static b32 condeval(Shell *sh, Arena *a, Cond *c)
{
    b32 r = 0;
    switch (c->kind) {
    case C_AND:
        r = condeval(sh, a, c->left) && condeval(sh, a, c->right);
        break;
    case C_OR:
        r = condeval(sh, a, c->left) || condeval(sh, a, c->right);
        break;
    case C_EXPR:
        r = condeval(sh, a, c->left);
        break;
    case C_TERM: {
        Str arg = condword(sh, a, c->op);
        tracecond(sh, *a, c->invert, "-n", arg, 0);
        r = arg.len > 0;
    } break;
    case C_UNARY: {
        TestCtx tc = {sh, a};
        TestEnv e = testenv(&tc);
        Str arg = condword(sh, a, c->left->op);
        tracecond(sh, *a, c->invert, c->op->text, arg, 0);
        TestResult t = testunary(&e, a, c->op->text, arg);
        if (t.status == 2) {
            shellerror(sh, *a, t.err);
        }
        r = t.status == 0;
    } break;
    case C_BINARY: {
        Str op = c->op->text;
        Str lhs = condword(sh, a, c->left->op);
        if (op=="==" || op=="=" || op=="!=") {
            Str pat = expandpattern(sh, a, c->right->op->text, wordctx(sh, c->right->op));
            tracecond(sh, *a, c->invert, op, lhs, &pat);
            i32 pf  = patflags(sh) | PM_EXTGLOB;  // always, as in bash
            if (sh->shopt[S_NOCASEMATCH]) pf |= PM_NOCASE;
            r = patmatch(pat, lhs, pf, *a);
            if (op == "!=") r = !r;
        } else if (op == "=~") {
            Str rhs = expandpattern(sh, a, c->right->op->text, wordctx(sh, c->right->op));
            tracecond(sh, *a, c->invert, op, lhs, &rhs);
            shellerror(sh, *a, "=~: regular expressions arrive in M4");
            r = 0;
        } else if (op=="-eq" || op=="-ne" || op=="-lt" || op=="-le" ||
                   op=="-gt" || op=="-ge") {
            Str rhs = condword(sh, a, c->right->op);
            tracecond(sh, *a, c->invert, op, lhs, &rhs);
            // An arithmetic error makes just this primary false
            Arith x = evalarithcmd(sh, a, lhs, "[[");
            Arith y = x.ok ? evalarithcmd(sh, a, rhs, "[[") : x;
            r = x.ok && y.ok && testintcmp(op, x.value, y.value);
        } else {
            Str rhs = condword(sh, a, c->right->op);
            tracecond(sh, *a, c->invert, op, lhs, &rhs);
            TestCtx tc = {sh, a};
            TestEnv e = testenv(&tc);
            TestResult t = testbinary(&e, a, op, lhs, rhs);
            if (t.status == 2) shellerror(sh, *a, t.err);
            r = t.status == 0;
        }
    } break;
    }
    return c->invert ? !r : r;
}

static b32 loopflow(Shell *sh)
{
    // Handle break and continue at a loop's end of iteration. Returns
    // true if the loop should stop.
    if (sh->flow == FLOW_BREAK) {
        if (--sh->flowcount <= 0) sh->flow = FLOW_NONE;
        return 1;
    }
    if (sh->flow == FLOW_CONTINUE) {
        if (--sh->flowcount <= 0) {
            sh->flow = FLOW_NONE;
            return 0;
        }
        return 1;
    }
    return sh->flow == FLOW_RETURN || sh->exiting;
}

static void runpipeline(Shell *sh, Node *n, Arena scratch);
static void runselect(Shell *sh, Node *n, Arena scratch);

// Evaluate one part of an arithmetic for, traced as bash does with
// leading space trimmed. An empty part is true. Returns false on error.
static b32 arithpart(Shell *sh, Arena scratch, Word *w, Arith *r = 0)
{
    Str e = {};
    if (w) e = trimleft(expandstring(sh, &scratch, w->text, XC_ARITH|XC_DQUOTE));
    if (!e.len) e = "1";
    if (sh->opt[O_XTRACE]) {
        traceline(sh, scratch, concat3(&scratch, "(( ", e, " ))"));
    }
    Arith t = evalarithcmd(sh, &scratch, e, "((");
    if (r) *r = t;
    return t.ok;
}

// A duration in ns as the time keyword prints it: seconds with prec
// decimals (truncated), or minutes and seconds when long.
static void printduration(Buf *b, i64 ns, i32 prec, b32 islong)
{
    if (ns < 0) ns = 0;
    i64 sec = ns / 1000000000;
    i64 frac = ns % 1000000000;
    if (islong) {
        print(b, sec/60);
        putbyte(b, 'm');
        sec %= 60;
    }
    print(b, sec);
    if (prec) {
        putbyte(b, '.');
        for (i32 i = 0; i < prec; i++) {
            frac *= 10;
            putbyte(b, (u8)('0' + frac/1000000000));
            frac %= 1000000000;
        }
    }
    if (islong) putbyte(b, 's');
}

// Report a timed pipeline per TIMEFORMAT (or the POSIX format for -p)
// on standard error. CPU times are not tracked yet and show as zero.
static void timereport(Shell *sh, Arena scratch, i64 real, b32 posix)
{
    Str fmt = "\nreal\t%3lR\nuser\t%3lU\nsys\t%3lS";
    if (posix) {
        fmt = "real %2R\nuser %2U\nsys %2S";
    } else {
        Got g = getvar(sh, &scratch, "TIMEFORMAT");
        if (g.set) fmt = g.value;
    }
    if (!fmt.len) return;
    Buf b(&scratch, 64);
    for (iz i = 0; i < fmt.len; i++) {
        u8 c = fmt[i];
        if (c!='%' || i+1==fmt.len) {
            putbyte(&b, c);
            continue;
        }
        iz j = i + 1;
        if (fmt[j] == '%') {
            putbyte(&b, '%');
            i = j;
            continue;
        }
        i32 prec = 3;
        b32 islong = 0;
        if (j<fmt.len && fmt[j]>='0' && fmt[j]<='9') {
            prec = fmt[j++] - '0';
            if (prec > 3) prec = 3;
        }
        if (j<fmt.len && fmt[j]=='l') {
            islong = 1;
            j++;
        }
        if (j == fmt.len) {
            putbyte(&b, c);
            continue;
        }
        switch (fmt[j]) {
        case 'R': printduration(&b, real, prec, islong); break;
        case 'U':
        case 'S': printduration(&b, 0, prec, islong);    break;
        case 'P': print(&b, "0.00");                     break;
        default: {
            // As bash: an error instead of the whole report
            Str bad = {&fmt.data[j], 1};
            shellerror(sh, scratch, concat3(&scratch, "TIMEFORMAT: `", bad, "': invalid format character"));
            return;
        }
        }
        i = j;
    }
    putbyte(&b, '\n');
    flushbout(sh);
    fdwrite(sh, 2, finish(&b));
}

static void executenode(Shell *, Node *, Arena);
static void executeone(Shell *, Node *, Arena);

static void execute(Shell *sh, Node *n, Arena scratch)
{
    if (n && (n->flags & CMD_TIME) && !sh->exiting && !sh->flow) {
        i64 start = os_now(sh->os);
        executeone(sh, n, scratch);
        timereport(sh, scratch, os_now(sh->os)-start, n->flags & CMD_TIMEPOSIX);
        return;
    }
    executenode(sh, n, scratch);
}

// Compound command cases, kept out of executenode so that its frame
// stays small: every level of shell recursion passes through it.

[[gnu::noinline]]
static void execfuncdef(Shell *sh, Node *n, Arena scratch)
{
    // Errors cite, as bash does, the definition's line in a
    // function, and elsewhere where the parser has got to
    Str name = n->name->text;
    sh->line = sh->funcdepth || !sh->lexer ? n->line : sh->lexer->lineno;
    if (n->name->flags & (W_HASDOLLAR|W_QUOTED)) {
        shellerror(sh, scratch, concat3(&scratch, "`", name, "': not a valid identifier"));
        sh->status = 1;
        return;
    }
    Builtin *special = sh->opt[O_POSIX] ? findbuiltin(name) : 0;
    if (special && (special->flags & B_SPECIAL)) {
        shellerror(sh, scratch, concat3(&scratch, "`", name, "': is a special builtin"));
        sh->status = 2;
        if (!sh->interactive) {
            sh->exiting = 1;
            escape(sh, ESC_EXIT);
        }
        return;
    }
    sh->line = n->line;
    Node *body = n->a;
    if (n->redirs) {
        // f() { ...; } >file: the redirections belong to the body,
        // and their errors cite the definition's line
        Node *g = alloc<Node>(&scratch);
        *g = *body;
        g->redirs = n->redirs;
        g->line   = n->line;
        body = g;
    }
    Func *old = findfunc(sh, name);
    if (old && (old->attrs & V_READONLY)) {
        shellerror(sh, scratch, concat(&scratch, name, ": readonly function"));
        sh->status = 1;
        return;
    }
    definefunc(sh, scratch, name, body);
    sh->status = 0;
}

[[gnu::noinline]]
static void execsubshell(Shell *sh, Node *n, Arena scratch)
{
    SubJob j = {};
    j.kind = SUB_NODE;
    j.node = n->a;
    j.newloops = 1;
    sh->status = subshell(sh, &j, scratch, 0);
    setpstat1(sh);
    checkerrexit(sh);
}

[[gnu::noinline]]
static void execfor(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    Slice<Str> list = {};
    if (n->words) {
        list = expandwords(sh, &scratch, n->words, 0);
    }
    // As bash, the header is traced as source text, every iteration
    Str head = {};
    if (sh->opt[O_XTRACE] && n->kind==N_FOR) {
        Buf b(&scratch, 64);
        print(&b, "for ");
        print(&b, n->name->text);
        print(&b, " in");
        for (Word *w = n->words; w; w = w->next) {
            putbyte(&b, ' ');
            print(&b, w->text);
        }
        head = finish(&b);
    }
    Str var = n->name->text;
    if (!validvarname(var)) {
        shellerror(sh, scratch, concat3(&scratch, "`", var, "': not a valid identifier"));
        sh->status = 1;
        return;
    }
    // $? carries into the body, perhaps from the list's expansion
    if (!list.len) sh->status = 0;
    sh->loopdepth++;
    for (iz i = 0; i < list.len; i++) {
        Arena it = scratch;
        safepoint(sh->sched);
        if (head.data && sh->opt[O_XTRACE]) traceline(sh, it, head);
        if (!setvar(sh, it, var, list[i])) {
            sh->status = 1;
            break;
        }
        execute(sh, n->b, it);
        if (sh->flow && loopflow(sh)) break;
        if (sh->exiting) break;
    }
    sh->loopdepth--;
}

[[gnu::noinline]]
static void execarithfor(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    Word *init = n->arith;
    Word *test = init ? init->next : 0;
    Word *step = test ? test->next : 0;
    // As with (( )), an arithmetic error fails the loop with status 1
    if (!arithpart(sh, scratch, init)) {
        sh->status = 1;
        return;
    }
    i32 status = 0;
    sh->loopdepth++;
    for (;;) {
        Arena it = scratch;
        safepoint(sh->sched);
        Arith r = {};
        if (!arithpart(sh, it, test, &r)) status = 1;
        if (!r.ok || !r.value) break;
        execute(sh, n->b, it);
        status = sh->status;
        if (sh->flow && loopflow(sh)) break;
        if (sh->exiting) break;
        if (!arithpart(sh, it, step)) {
            status = 1;
            break;
        }
    }
    sh->loopdepth--;
    sh->status = status;
}

[[gnu::noinline]]
static void execcase(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    if (sh->opt[O_XTRACE]) {
        traceline(sh, scratch, concat3(&scratch, "case ", n->words->text, " in"));
    }
    Str word = expandstring(sh, &scratch, n->words->text, wordctx(sh, n->words));
    b32 ran  = 0;
    for (Case *c = n->cases; c; c = c->next) {
        if (!casematch(sh, &scratch, word, c->patterns)) continue;
        ran = 1;
        for (;;) {
            if (!c->body) sh->status = 0;  // an empty body succeeds
            execute(sh, c->body, scratch);
            if (c->term==CASE_FALLTHROUGH && c->next && !sh->flow) {
                c = c->next;
                continue;
            }
            break;
        }
        if (c->term != CASE_TESTNEXT || sh->flow) break;
    }
    if (!ran) sh->status = 0;
}

[[gnu::noinline]]
static void execarith(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    Str text = expandstring(sh, &scratch, n->arith->text, XC_ARITH|XC_DQUOTE);
    if (sh->opt[O_XTRACE]) {
        traceline(sh, scratch, concat3(&scratch, "(( ", text, " ))"));
    }
    Arith r = evalarithcmd(sh, &scratch, text, "((");
    sh->status = r.ok ? !r.value : 1;
    setpstat1(sh);
    checkerrexit(sh);
}

[[gnu::noinline]]
static void execcond(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    sh->status = condeval(sh, &scratch, n->cond) ? 0 : 1;
    setpstat1(sh);
    checkerrexit(sh);
}

// Groups, if branches and the right sides of lists are followed in a
// loop rather than by recursion, so that each level of shell function
// calls costs as little stack as possible.
static void executenode(Shell *sh, Node *n, Arena scratch)
{
    for (;;) {
        if (!n || sh->exiting || sh->flow) {
            return;
        }
        if (n->flags & CMD_TIME) {
            execute(sh, n, scratch);
            return;
        }
        if (n->redirs || (n->flags & CMD_INVERT)) {
            break;
        }
        switch (n->kind) {
        case N_SIMPLE:
            checkstack(sh);
            runsimple(sh, n, scratch);
            setpstat1(sh);
            return;
        case N_GROUP:
            n = n->a;
            continue;
        case N_IF:
            sh->noerrexit++;
            execute(sh, n->a, scratch);
            sh->noerrexit--;
            if (sh->flow || sh->exiting) return;
            if (!sh->status) {
                n = n->b;
            } else if (n->c) {
                n = n->c;
            } else {
                sh->status = 0;
                return;
            }
            continue;
        case N_CONNECT:
            switch (n->connector) {
            case '|':
            case '&':
                break;
            case T_ANDAND:
            case T_OROR:
                sh->noerrexit++;
                execute(sh, n->a, scratch);
                sh->noerrexit--;
                if (sh->flow || sh->exiting) return;
                if ((n->connector==T_ANDAND) != (sh->status==0)) return;
                n = n->b;
                continue;
            default:
                execute(sh, n->a, scratch);
                n = n->b;
                continue;
            }
            break;
        }
        break;
    }
    executeone(sh, n, scratch);
}

static void executeone(Shell *sh, Node *n, Arena scratch)
{
    if (!n || sh->exiting || sh->flow) {
        return;
    }
    checkstack(sh);
    Undo *mark = sh->undo;
    b32   invert = n->flags & CMD_INVERT;
    if (invert) sh->noerrexit++;

    if (n->kind!=N_SIMPLE && n->kind!=N_CONNECT && n->kind!=N_FUNCDEF && n->redirs) {
        if (!doredirs(sh, n->redirs, &scratch, 0)) {
            unwind(sh, mark);
            sh->status = 1;
            if (invert) sh->noerrexit--;
            else checkerrexit(sh);
            return;
        }
    }

    switch (n->kind) {
    case N_SIMPLE:
        runsimple(sh, n, scratch);
        setpstat1(sh);
        break;

    case N_CONNECT:
        switch (n->connector) {
        case T_ANDAND:
        case T_OROR:
            sh->noerrexit++;
            execute(sh, n->a, scratch);
            sh->noerrexit--;
            if (sh->flow || sh->exiting) break;
            if ((n->connector==T_ANDAND) == (sh->status==0)) {
                execute(sh, n->b, scratch);
            }
            break;
        case '|':
            runpipeline(sh, n, scratch);
            break;
        case '&':
            // Background jobs arrive in M3a; run synchronously meanwhile,
            // but in a subshell as bash would
            {
                SubJob j = {};
                j.kind = SUB_NODE;
                j.node = n->a;
                j.newloops = 1;
                i32   depth = ++sh->capdepth;
                Arena child = *scratchat(sh, depth);
                subshell(sh, &j, child, 0);
                sh->capdepth = depth - 1;
                sh->lastbg   = sh->lastpid;  // $!: the subshell's BASHPID
            }
            sh->status = 0;
            execute(sh, n->b, scratch);
            break;
        default:
            execute(sh, n->a, scratch);
            execute(sh, n->b, scratch);
        }
        break;

    case N_FUNCDEF:
        execfuncdef(sh, n, scratch);
        break;

    case N_GROUP:
        execute(sh, n->a, scratch);
        break;

    case N_SUBSHELL:
        execsubshell(sh, n, scratch);
        break;

    case N_IF:
        sh->noerrexit++;
        execute(sh, n->a, scratch);
        sh->noerrexit--;
        if (sh->flow || sh->exiting) break;
        if (!sh->status) {
            execute(sh, n->b, scratch);
        } else if (n->c) {
            execute(sh, n->c, scratch);
        } else {
            sh->status = 0;
        }
        break;

    case N_WHILE:
    case N_UNTIL: {
        i32 status = 0;
        sh->loopdepth++;
        for (;;) {
            Arena it = scratch;
            safepoint(sh->sched);
            sh->noerrexit++;
            execute(sh, n->a, it);
            sh->noerrexit--;
            if (sh->flow) {
                if (loopflow(sh)) break;
                continue;
            }
            if (sh->exiting) break;
            if ((sh->status==0) != (n->kind==N_WHILE)) break;
            execute(sh, n->b, it);
            status = sh->status;
            if (sh->flow && loopflow(sh)) break;
            if (sh->exiting) break;
        }
        sh->loopdepth--;
        sh->status = status;
    } break;

    case N_SELECT:
        runselect(sh, n, scratch);
        break;

    case N_FOR:
        execfor(sh, n, scratch);
        break;

    case N_ARITHFOR:
        execarithfor(sh, n, scratch);
        break;

    case N_CASE:
        execcase(sh, n, scratch);
        break;

    case N_ARITH:
        execarith(sh, n, scratch);
        break;

    case N_COND:
        execcond(sh, n, scratch);
        break;

    case N_COPROC:
        shellerror(sh, scratch, "coproc: not supported until M3a");
        sh->status = 1;
        break;
    }

    if (n->kind!=N_SIMPLE && n->kind!=N_CONNECT) {
        unwind(sh, mark);
    }
    if (invert) {
        sh->noerrexit--;
        if (sh->flow != FLOW_RETURN) {  // return leaves before inverting
            sh->status = !sh->status;
        }
    }
}

// Write select's numbered menu to standard error, as bash lays it out:
// columns when a single row would not do, padded relative to each
// column's own start.
static void selectmenu(Shell *sh, Arena scratch, Slice<Str> list)
{
    iz maxlen = 0;
    for (iz i = 0; i < list.len; i++) {
        maxlen = list[i].len>maxlen ? list[i].len : maxlen;
    }
    i32 width = 1;
    for (iz v = list.len; v >= 10; v /= 10) width++;
    iz colwidth = maxlen + width + 4;  // ") " and two spaces

    i64 columns = 80;
    Got g = getvar(sh, &scratch, "COLUMNS");
    if (!g.set || !parsei64(g.value, &columns) || columns<=0) columns = 80;
    iz cols = (iz)(columns / colwidth);
    cols = cols ? cols : 1;
    iz rows = list.len/cols + (list.len%cols != 0);
    cols = list.len/rows + (list.len%rows != 0);
    b32 single = rows == 1;
    if (single) {
        rows = cols;
        cols = 1;
    }

    Buf b(&scratch, 256);
    for (iz row = 0; row < rows; row++) {
        for (iz i = row, col = 0; i < list.len; i += rows, col++) {
            iz start = b.len;
            Str num = fmti64(&scratch, i+1);
            // bash leaves the first of several columns unpadded
            for (iz k = num.len; k<width && (cols==1 || col); k++) putbyte(&b, ' ');
            print(&b, num);
            print(&b, ") ");
            print(&b, list[i]);
            if (i+rows >= list.len) break;
            for (iz pos = b.len-start; pos < colwidth;) {
                if (colwidth/8 > pos/8) {
                    putbyte(&b, '\t');
                    pos += 8 - pos%8;
                } else {
                    putbyte(&b, ' ');
                    pos++;
                }
            }
        }
        putbyte(&b, '\n');
    }
    fdwrite(sh, 2, finish(&b));
}

// select: show the menu and prompt on standard error, read a line into
// REPLY, and run the body with the chosen word, until end of input.
static void runselect(Shell *sh, Node *n, Arena scratch)
{
    sh->line = n->line;
    Slice<Str> list = {};
    if (n->words) {
        list = expandwords(sh, &scratch, n->words, 0);
    } else if (!n->inlist) {
        list.data = sh->args.v;
        list.len  = sh->args.n;
    }
    Str var = n->name->text;
    if (!validvarname(var)) {
        shellerror(sh, scratch, concat3(&scratch, "`", var, "': not a valid identifier"));
        sh->status = 1;
        return;
    }
    if (!list.len) {
        sh->status = 0;
        return;
    }
    sh->status = 0;
    sh->loopdepth++;
    b32 menu = 1;
    for (;;) {
        Arena it = scratch;
        safepoint(sh->sched);
        if (menu) selectmenu(sh, it, list);
        Got ps3 = getvar(sh, &it, "PS3");
        fdwrite(sh, 2, ps3.set ? ps3.value : Str("#? "));
        Buf line(&it, 64);
        u8  c   = 0;
        b32 eof = 0;
        while (!(eof = fdread(sh, 0, &c, 1) != 1) && c != '\n') putbyte(&line, c);
        if (eof && !line.len) {
            fdwrite(sh, 2, "\n");
            sh->status = 1;
            break;
        }
        Str reply = finish(&line);
        setvar(sh, it, "REPLY", reply);
        menu = !trim(reply).len;
        if (menu) continue;
        i64 choice = 0;
        b32 ok = parsei64(trim(reply), &choice) && choice>=1 && choice<=list.len;
        if (!setvar(sh, it, var, ok ? list[(iz)choice-1] : Str(""))) {
            sh->status = 1;
            break;
        }
        execute(sh, n->b, it);
        if (sh->flow && loopflow(sh)) break;
        if (sh->exiting) break;
    }
    sh->loopdepth--;
}

// Running source text

// Run one complete command from source text. An expansion error
// abandons it, and execution resumes with the next command unless this
// is a subshell, which it ends. Reading a script file (reader) also
// resumes after an error that abandons all -c, eval and source text.
static void runcommand(Shell *sh, Node *cmd, Arena scratch, b32 reader)
{
    Escape e = {};
    e.up        = sh->esc;
    e.undo      = sh->undo;
    e.frame     = sh->frame;
    e.loopdepth = sh->loopdepth;
    BOut  *bout      = sh->bout;
    Lexer *lexer     = sh->lexer;
    Str    iname     = sh->iname;
    Str    srcname   = sh->srcname;
    i32    noerrexit = sh->noerrexit;
    i32    evaldepth = sh->evaldepth;
    i32    srcdepth  = sh->sourcedepth;
    i32    capdepth  = sh->capdepth;
    sh->esc = &e;
    if (!__builtin_setjmp(e.jmp)) {
        execute(sh, cmd, scratch);
        sh->esc = e.up;
        return;
    }
    sh->esc = e.up;
    b32 resume = e.why==ESC_DISCARD || (e.why==ESC_ABORT && reader);
    if (!resume || sh->subshell) {
        if (e.why==ESC_DISCARD && !reader) sh->status = 1;
        escape(sh, e.why);
    }
    unwind(sh, e.undo);
    sh->status      = reader && sh->status ? sh->status : 1;
    sh->flow        = FLOW_NONE;
    sh->loopdepth   = e.loopdepth;
    sh->bout        = bout;
    sh->lexer       = lexer;
    sh->iname       = iname;
    sh->srcname     = srcname;
    sh->noerrexit   = noerrexit;
    sh->evaldepth   = evaldepth;
    sh->sourcedepth = srcdepth;
    sh->capdepth    = capdepth;
    sh->current     = 0;
}

// Parse and run src one command at a time, as -c, eval, source and
// command substitution do, numbering its lines from line. Returns the
// final status.
static i32 runsource(Shell *sh, Str src, Str iname, Arena scratch, b32 reader, i32 line,
                     b32 (*fill)(void *, Lexer *), void *fillctx)
{
    byte *beg = scratch.beg;
    byte *end = scratch.mem ? scratch.mem->limit : scratch.end;
    Lexer *L = newlexer(&scratch, src);
    L->fill          = fill;
    L->fillctx       = fillctx;
    L->lineno        = line - 1;
    L->aliases       = &sh->aliases;
    L->stackmax      = (iz)1<<20;
    L->expandaliases = L->aliasopt = sh->shopt[S_EXPAND_ALIASES];
    L->extglob       = L->extglobopt = sh->shopt[S_EXTGLOB];
    L->posix         = sh->opt[O_POSIX];
    L->utf8          = sh->utf8;
    Lexer *outer = sh->lexer;
    Str    oname = sh->iname;
    sh->lexer = L;
    sh->iname = iname;
    for (;;) {
        Arena unit = *L->a;  // reset after each complete line
        iz     pos  = L->srcpos;
        Parsed r = parseunit(L);
        if (sh->opt[O_VERBOSE] && L->srcpos>pos) {
            // set -v: echo the input lines as they are read, each
            // with a newline as the lexer sees it
            Str text = span(L->src.data+pos, L->src.data+L->srcpos);
            fdwrite(sh, 2, text);
            if (text[text.len-1] != '\n') fdwrite(sh, 2, "\n");
        }
        // Report parser messages
        Arena msgarena = *L->a;
        for (Message *m = L->msgs; m; m = m->next) {
            Buf b(&msgarena, 128);
            print(&b, errname(sh));
            if (m->kind == MSG_ERROR) {
                if (!(errname(sh) == sh->iname)) {
                    print(&b, ": ");
                    print(&b, sh->iname);
                }
                print(&b, ": line ");
                print(&b, (i64)m->line);
                print(&b, ": ");
            } else {
                print(&b, ": line ");
                print(&b, (i64)m->line);
                print(&b, ": warning: ");
            }
            print(&b, m->text);
            putbyte(&b, '\n');
            fdwrite(sh, 2, finish(&b));
        }
        L->msgs    = 0;
        L->msgtail = &L->msgs;
        if (r.status == PARSE_EOF) break;
        if (r.status == PARSE_ERROR) {
            if (L->overflow) {
                sh->status = 139;
            } else if (L->comsuberr && (sh->evaldepth || sh->sourcedepth)) {
                // Like ${x?}, abandons the shell, or ends a subshell
                sh->lexer  = outer;
                sh->iname  = oname;
                sh->status = 1;
                escape(sh, ESC_FATAL);
            } else if (L->comsuberr && sh->iname=="-c") {
                sh->status = 127;
            } else if (L->reported || !sh->status) {
                sh->status = 2;
            }
            if (!sh->interactive && !sh->evaldepth && !sh->sourcedepth) {
                sh->exiting = 1;
            }
            break;
        }
        if (r.status == PARSE_DISCARD) {
            sh->status = 1;
            continue;
        }
        if (r.cmd && !(sh->opt[O_NOEXEC] && !sh->interactive)) {
            // As in bash, LINENO is where the parser stopped until a
            // command sets it: a compound command's redirections cite
            // its last line
            sh->line = L->lineno;
            runcommand(sh, r.cmd, *L->a, reader);
        }
        // Syncing options the command may have changed
        L->expandaliases = L->aliasopt = sh->shopt[S_EXPAND_ALIASES];
        L->extglob       = L->extglobopt = sh->shopt[S_EXTGLOB];
        L->posix         = sh->opt[O_POSIX];
        if (sh->exiting || sh->flow) break;
        if (!L->pushed && L->idx>=L->line.len && !L->nheredoc) {
            // Nothing refers to the unit's memory now: function
            // bodies were copied into the store
            keepcmdtext(sh, unit.beg, end);
            L->line = {};
            L->idx  = 0;
            *L->a   = unit;
        }
    }
    keepcmdtext(sh, beg, end);
    sh->lexer = outer;
    sh->iname = oname;
    return sh->status;
}
