// unabashed: a faithful clone of GNU bash 5.3
//
// Unity build: a root (main_linux.cpp, main_darwin.cpp, main_win32.cpp,
// main_test.cpp, main_fuzz.cpp, libunabashed_*.cpp) includes this file
// and implements the platform API (api.cpp) as a table, OsVtable, which
// its osvtable names. Design notes are in notes/, starting with notes/plan.md.
//
// This is free and unencumbered software released into the public domain.

#include "base.cpp"


#include "api.cpp"
#include "dispatch.cpp"


// Memory

// A new arena on its own reservation, committed as it grows. With
// maybe, a null arena where the address space has no room for it.
static Arena newarena(Os *os, iz reserve, b32 maybe = 0)
{
    byte *base = os_reserve(os, reserve);
    iz    head = (iz)1<<16;
    if (!base && maybe) {
        return {};
    }
    if (!base || !os_commit(os, base, head)) {
        os_oom();
    }
    Mem *m = (Mem *)base;
    m->os     = os;
    m->commit = base + head;
    m->limit  = base + reserve;
    Arena a = {};
    a.beg = base + sizeof(Mem);
    a.end = m->commit;
    a.mem = m;
    return a;
}

// Give back the pages well past an arena's front, as after a command
// that needed far more than usual. Copies of the arena older than
// floor's commit may still think the memory below it committed, so
// the pages under floor stay.
static void arenatrim(Arena *a, byte *floor)
{
    Mem *m = a->mem;
    if (!m) return;
    iz   keep = (iz)1<<23;
    byte *cut = a->beg>floor ? a->beg : floor;
    if (m->commit-cut <= 4*keep) return;
    cut += keep;
    cut += -(uz)cut & 0xffff;
    os_decommit(m->os, cut, m->commit-cut);
    m->commit = cut;
    a->end    = a->end<cut ? a->end : cut;
}


// Output: buffered with a sticky error flag

struct Output {
    Os    *os;
    Handle h;
    u8    *buf;
    iz     len;
    iz     cap;
    b32    err;
};

static Output *newoutput(Os *os, Arena *a, Handle h)
{
    Output *o = alloc<Output>(a);
    o->os  = os;
    o->h   = h;
    o->cap = 1<<12;
    o->buf = allocbytes(a, o->cap);
    return o;
}

static void flush(Output *o)
{
    if (!o->err && o->len) {
        o->err = !os_write(o->os, o->h, o->buf, o->len);
    }
    o->len = 0;
}

static void print(Output *o, Str s)
{
    for (iz off = 0; off < s.len;) {
        if (o->len == o->cap) {
            flush(o);
        }
        iz avail = o->cap - o->len;
        iz count = avail<s.len-off ? avail : s.len-off;
        copybytes(o->buf+o->len, s.data+off, count);
        o->len += count;
        off    += count;
    }
}

static void print(Output *o, i64 v)
{
    u8  tmp[32];
    u8 *end = tmp + countof(tmp);
    u8 *beg = end;
    i64 t   = v<0 ? v : -v;
    do {
        *--beg = (u8)('0' - t%10);
    } while (t /= 10);
    if (v < 0) {
        *--beg = '-';
    }
    print(o, span(beg, end));
}


// Self test: exercises platform primitives on a real host.

struct PingPong {
    Fiber *main;
    Fiber *a;
    Fiber *b;
    u8    *trace;
    iz     len;
    i32    rounds;
};

static void pingpong(Os *os, PingPong *p, u8 mark, Fiber *next)
{
    for (i32 i = 0; i < p->rounds; i++) {
        p->trace[p->len++] = mark;
        os_switch(os, next);
    }
    os_switch(os, p->main);
    assert(0);
}

static void ping(Os *os, void *arg)
{
    PingPong *p = (PingPong *)arg;
    pingpong(os, p, 'a', p->b);
}

static void pong(Os *os, void *arg)
{
    PingPong *p = (PingPong *)arg;
    pingpong(os, p, 'b', p->a);
}

// Run a fiber ping-pong and report the result. Returns the exit status.
static i32 selftest(Os *os, Arena *perm, Output *out)
{
    PingPong *p = alloc<PingPong>(perm);
    p->rounds = 1000;
    p->trace  = allocbytes(perm, 2*p->rounds);
    p->main   = os_self(os);
    p->a      = os_fiber(os, perm, (iz)1<<20, ping, p);
    p->b      = os_fiber(os, perm, (iz)1<<20, pong, p);
    os_switch(os, p->a);
    os_fiberfree(os, p->a);
    os_fiberfree(os, p->b);

    b32 ok = p->len == 2*p->rounds;
    for (iz i = 0; ok && i < p->len; i++) {
        ok = p->trace[i] == "ab"[i&1];
    }
    print(out, ok ? Str("fibers: ok\n") : Str("fibers: FAIL\n"));
    return !ok;
}




#include "unicode.cpp"
#include "wide.cpp"
#include "vtcon.cpp"
#include "parse.cpp"
#include "print.cpp"
#include "arith.cpp"
#include "pattern.cpp"
#include "regex.cpp"
#include "brace.cpp"
#include "b_test.cpp"
#include "float.cpp"
#include "b_printf.cpp"
#include "paths.cpp"
#include "sched.cpp"
#include "shell.cpp"
#include "array.cpp"
#include "vars.cpp"
#include "expand.cpp"
#include "glob.cpp"
#include "spawn.cpp"
#include "exec.cpp"
#include "proc.cpp"
#include "builtins.cpp"
#include "help.cpp"
#include "version.cpp"
#include "trap.cpp"
#include "jobs.cpp"
#include "history.cpp"
#include "edit.cpp"
#include "complete.cpp"
#include "pcomplete.cpp"
#include "inputrc.cpp"


// Startup

// Native mode turns PATH's colons into semicolons as busybox-w32 does
// when it is assigned: all but one right after a letter that starts an
// element, a drive's. In place, in a buffer this level owns.
static void nativepath(Shell *sh)
{
    Value *v = lookup(sh, "PATH");
    if (!v || !v->str.len || (byte *)v->str.data<sh->store->mark) return;
    Str s     = v->str;
    b32 start = 1;
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (start && ((c|0x20)>='a' && (c|0x20)<='z') && i+1<s.len && s[i+1]==':') {
            i++;
            start = 0;
            continue;
        }
        if (c == ':') s.data[i] = c = ';';
        start = c == ';';
    }
}

// Could setlocale take this locale? Only C, POSIX and UTF-8 ones are
// supported, so any other (zh_TW.big5, ru_RU.CP1251, a bare en_US) is
// as a locale that is not installed.
static b32 localeok(Str v)
{
    if (!v.len || v=="C" || v=="POSIX") return 1;
    Cut c = cut(v, '.');
    if (!c.ok) return 0;
    Str cs = cut(c.tail, '@').head;
    return equalfold(cs, "UTF-8") || equalfold(cs, "utf8");
}

// Assigning a locale variable that names no usable locale warns, as
// bash's sv_locale does when setlocale fails (LANG alone is quiet).
// Linux only: Cygwin takes nearly any name, busybox-w32 none.
static void localewarn(Shell *sh, Str name)
{
    if (sh->pathmode!=PM_POSIX || sh->nolocwarn) return;
    if (name!="LC_ALL" && sh->lcall) return;  // setlocale not called
    Arena a = *scratchat(sh, sh->capdepth+1);
    Got   g = getplain(sh, &a, name);
    if (!g.set || localeok(g.value)) return;
    Buf b(&a, 300);
    print(&b, "warning: setlocale: ");
    print(&b, name);
    print(&b, ": cannot change locale (");
    print(&b, g.value);
    print(&b, "): No such file or directory");
    shellerror(sh, a, finish(&b));
}

// The decimal point of a locale's language and territory, as glibc's
// LC_NUMERIC has it: a comma across most of Europe and beyond.
static u8 localeradix(Str v)
{
    static char const comma[][3] = {
        "af","az","be","bg","bs","ca","cs","da","de","el","eo","es","et",
        "eu","fi","fo","fr","gl","hr","hu","hy","id","is","it","ka","kk",
        "ky","lt","lv","mk","mn","nb","nl","nn","no","oc","pl","pt","ro",
        "ru","sk","sl","sq","sr","sv","tg","tk","tr","tt","uk","uz","vi",
    };
    static char const point[][6] = {  // territories that differ
        "de_CH","de_LI","it_CH","es_MX","es_US","es_DO","es_GT","es_HN",
        "es_NI","es_PA","es_PR","es_SV",
    };
    Str lang = cut(cut(cut(v, '.').head, '@').head, '_').head;
    Str terr = cut(cut(v, '.').head, '@').head;
    for (iz i = 0; i < countof(point); i++) {
        if (terr == Str(point[i])) return 0;
    }
    for (iz i = 0; i < countof(comma); i++) {
        if (lang == Str(comma[i])) return ',';
    }
    return 0;
}

// LC_ALL, else LC_NUMERIC, else LANG chose another numeric locale. One
// that cannot be set leaves the current one, as a failed setlocale.
static void numericchanged(Shell *sh)
{
    Arena a = *scratchat(sh, sh->capdepth+1);  // for a dynamic value
    Str v = getplain(sh, &a, "LC_ALL").value;
    if (!v.len) v = getplain(sh, &a, "LC_NUMERIC").value;
    if (!v.len) v = getplain(sh, &a, "LANG").value;
    if (sh->pathmode == PM_NATIVE) {
        sh->radix = 0;  // busybox-w32 has no locales
    } else if (localeok(v)) {
        sh->radix = localeradix(v);
    }
}

static void varchanged(Shell *sh, Str name)
{
    // Every assignment comes here: most names leave at the first byte
    switch (name.len ? name[0] : 0) {
    case '_':
        if (name.len == 1) sh->lastargdirty = 1;  // no longer the dynamic $_
        return;
    case 'B': case 'G': case 'H': case 'I': case 'L': case 'O': case 'P':
    case 'h': break;
    default: return;
    }
    if (name=="HISTSIZE" || name=="HISTCONTROL" || name=="histchars" ||
            name=="HISTTIMEFORMAT" || name=="HISTFILESIZE") {
        histvarchanged(sh, name);
    } else if (name=="LC_TIME" || name=="LC_COLLATE" || name=="LC_MESSAGES") {
        localewarn(sh, name);
    } else if (name == "LC_NUMERIC") {
        localewarn(sh, name);
        numericchanged(sh);
    } else if (name == "LC_ALL" || name == "LC_CTYPE" || name == "LANG") {
        {
            // As bash's lc_all (LANG at startup brings it in)
            Arena a = *scratchat(sh, sh->capdepth+1);
            sh->lcall = getplain(sh, &a, "LC_ALL").value.len > 0;
        }
        if (name != "LANG") localewarn(sh, name);
        numericchanged(sh);
        Arena a = *scratchat(sh, sh->capdepth+1);
        Str v = getplain(sh, &a, "LC_ALL").value;
        if (!v.len) v = getplain(sh, &a, "LC_CTYPE").value;
        if (!v.len) v = getplain(sh, &a, "LANG").value;
        b32 utf8 = 0;
        for (iz i = 0; i+4 < v.len+1; i++) {
            Str t = cuthead(v, i);
            if (startswith(t, "UTF-8") || startswith(t, "utf8") ||
                    startswith(t, "UTF8") || startswith(t, "utf-8")) {
                utf8 = 1;
            }
        }
        if (sh->lexer) sh->lexer->utf8 = utf8;
        sh->utf8 = utf8;
    } else if (name == "BASH_XTRACEFD") {
        // Traces go to an open descriptor it names, else to stderr
        Arena a = *scratchat(sh, sh->capdepth+1);
        Got g = getplain(sh, &a, name);
        i64 fd = 0;
        sh->tracefd = 2;
        if (g.value.len && parsei64(g.value, &fd) && fd>=0 && fd<FDMAX && sh->fds->fd[fd]) {
            sh->tracefd = (i32)fd;
        } else if (g.value.len) {
            Str msg = concat3(&a, "BASH_XTRACEFD: ", g.value,
                                  Str(": invalid value for trace file descriptor"));
            shellerror(sh, a, msg);
        }
    } else if (name == "BASH_COMPAT") {
        // A value it cannot take warns, as bash's sv_shcompat, which
        // runs for a program's temporary binding no more than setlocale
        Arena a = *scratchat(sh, sh->capdepth+1);
        Got g = getplain(sh, &a, name);
        if (!sh->nolocwarn && g.value.len && !compatvalue(g.value)) {
            Str msg = concat3(&a, "BASH_COMPAT: ", g.value,
                                  Str(": compatibility value out of range"));
            shellerror(sh, a, msg);
        }
    } else if (name == "IGNOREEOF") {
        Value *v = lookup(sh, name);  // set or not, as bash's var_isset
        sh->opt[O_IGNOREEOF] = v && (v->str.data || v->arr);
    } else if (name == "PIPESTATUS") {
        sh->pstatdirty = 1;
    } else if (name == "HOSTFILE") {
        // Unset, completion forgets its host names; set, it reads the
        // file again, adding to them (as bash's sv_hostfile)
        Arena a = *scratchat(sh, sh->capdepth+1);
        if (!getplain(sh, &a, name).set) {
            sfreestr(sh->store, sh->hosts);
            sh->hosts = {};
        }
        sh->hostsread = 0;
    } else if (name == "PATH") {
        sh->cmdhash = 0;  // bash forgets remembered paths
        if (sh->pathmode == PM_NATIVE) nativepath(sh);
    } else if (name == "OPTIND") {
        // Assigning or unsetting OPTIND restarts getopts at a word start
        if (!sh->ingetopts) sh->optchar = 1;
    } else if (name == "POSIXLY_CORRECT") {
        Value *v = lookup(sh, name);
        b32 on = v && (v->str.data || v->arr);  // as bash's var_isset
        if (on && (v->attrs&V_TEMP) && !sh->posixsaved) {
            // as bash, one level of saving for POSIXLY_CORRECT=y cmd
            for (i32 i = 0; i < countof(posixshopts); i++) {
                sh->posixshopts[i] = sh->shopt[posixshopts[i]];
            }
            sh->posixsaved = 1;
        }
        sh->opt[O_POSIX] = on;
        posixinit(sh, on);
    } else if (name == "GLOBIGNORE") {
        // A non-empty value turns dotglob on, and unsetting turns it off
        Arena a = *scratchat(sh, sh->capdepth+1);
        Got g = getplain(sh, &a, name);
        if (g.value.len) {
            sh->shopt[S_DOTGLOB] = 1;
        } else if (!g.set) {
            sh->shopt[S_DOTGLOB] = 0;
        }
    }
}

// Make a dynamic variable. As bash, one the environment set keeps its
// value if an array (init_dynamic_array_var), so stays an ordinary
// variable; a scalar drops it, though SECONDS counts on from it
// (init_seconds_var).
static void dynamic(Shell *sh, Str name, i32 dyn, u32 attrs = 0)
{
    Value *had = lookup(sh, name);
    if (had && (attrs & (V_ARRAY|V_ASSOC))) return;
    Value *v = setattrs(sh, name, attrs & ~(V_ARRAY|V_ASSOC), 0);
    v->dyn    = dyn;
    v->attrs |= attrs;
    if (had && dyn==DYN_SECONDS && v->str.data) {
        i64 n = 0;
        if (!parsei64(trim(v->str), &n)) n = 0;
        v->base = n;
        setdyncell(sh, dyn, v->str);
    } else if (had && dyncellular(dyn)) {
        sfreestr(sh->store, v->str);
        v->str = {};
    }
    sh->dynexported |= had && dyncellular(dyn) && (v->attrs & V_EXPORT);
}

static void addbadenv(Shell *, Str entry);

// Define a function a parent exported as BASH_FUNC_name%%="() {...}".
// As in bash since Shellshock, the value must be a function definition
// and nothing more. One that is not is reported, as bash's parse of it
// named for the function, makes $? 1, and is passed on to programs.
static void importfunc(Shell *sh, Arena scratch, Str var, Str value)
{
    Str name = cuttail(cuthead(var, 10), 2);
    if (!name.len || !startswith(value, "() {")) return;
    if (sh->rshell) return;  // restricted: no functions from outside
    if (sh->importing & IMPORT_NONE) {
        addbadenv(sh, concat3(&scratch, var, "=", value));  // -n, -p: passed on only
        return;
    }
    b32 posix = sh->opt[O_POSIX] || (sh->importing & IMPORT_POSIX);
    b32 ok    = exportablefunc(name) && (!posix || validvarname(name));
    Node *n = 0;
    if (ok) {
        Str text = concat3(&scratch, name, " ", value);
        if (sh->importing & IMPORT_VERBOSE) {
            // -v: the text as it is read, as bash's parse of it echoes
            fdwrite(sh, 2, text);
            if (!endswith(text, "\n")) fdwrite(sh, 2, "\n");
        }
        Lexer *L = newlexer(&scratch, text);
        L->dstart = sh->dstart;
        L->lineno = -1;  // its first line is 0, as bash's line_number then
        parselimit(sh, L);
        Parsed p = parseunit(L);
        n = p.cmd;
        if (L->msgs) {
            Str iname = sh->iname;
            sh->iname = name;
            reportmessages(sh, L);
            sh->iname = iname;
        }
        ok = p.status==PARSE_OK && n;
        if (ok && (n->kind!=N_FUNCDEF || !(n->name->text == name) ||
                   parseunit(L).status!=PARSE_EOF)) {
            Str msg = concat3(&scratch, "warning: ", name, Str(": ignoring function definition attempt"));
            shellerror(sh, scratch, msg);
            ok = 0;
        }
    }
    if (!ok) {
        Str msg = concat3(&scratch, "error importing function definition for `", name, "'");
        shellerror(sh, scratch, msg);
        addbadenv(sh, concat3(&scratch, var, "=", value));
        sh->status = 1;
        return;
    }
    Node *body = n->a;
    if (n->redirs) {
        Node *g = alloc<Node>(&scratch);
        *g = *body;
        g->redirs = n->redirs;
        body = g;
    }
    sh->defsrc = "environment";
    definefunc(sh, scratch, name, body, n->endline);
    sh->defsrc = {};
    Func *f = findfunc(sh, name);
    if (f) f->attrs |= V_EXPORT;
}

// Startup files

struct Startup {
    Str rcfile;     // --rcfile, --init-file
    b32 restricted; // -r, --restricted
    b32 noediting;  // --noediting
    b32 login;      // -l, --login
    b32 noprofile;  // --noprofile
    b32 norc;       // --norc
    b32 posix;      // in POSIX mode before the files (--posix, ...)
    b32 busybox;    // busybox-w32's rules, not bash's (native mode)
    Str dir;        // busybox-w32's -d: the directory after the profile
};

// Run one startup file, as bash's maybe_execute_file does: a leading ~
// from HOME, a missing file skipped, other failures reported without a
// line number ("bash: FILE: is a directory"). False if the file is not
// there, so that the caller may try the next. Under busybox's rules
// every failure is quiet and a return's status stays.
static b32 startupfile(Shell *sh, Arena scratch, Str name, Startup *s)
{
    Str file = s->busybox ? name : tildefilename(sh, &scratch, name);
    if (!file.len) return 0;
    i32  err;
    Str  src = readwholefile<OF_OWN>(sh, &scratch, file, &err);
    Stat st  = pathstat(sh, scratch, file, 1|ST_OWN);
    if (err==E_NOENT && st.err) return 0;
    Str bad = err ? errtext(err) : Str{};
    if (!st.err && st.type==FT_DIR) bad = "is a directory";
    if (bad.data && s->busybox) {
        return 0;
    } else if (bad.data) {
        Buf b(&scratch, 64);
        print(&b, errname(sh));
        print(&b, ": ");
        print(&b, file);
        print(&b, ": ");
        print(&b, bad);
        putbyte(&b, '\n');
        fdwrite(sh, 2, finish(&b));
        return 1;
    }
    Str *args = alloc<Str>(&scratch);
    args[0] = file;
    sh->status = sourcetext(sh, args, 1, src, scratch, !s->busybox);
    return 1;
}

// A file named by a variable, ENV or BASH_ENV, after expanding the
// value (bash then expands a leading ~, busybox does not).
static void envfile(Shell *sh, Arena scratch, Str var, Startup *s)
{
    Got g = getplain(sh, &scratch, var);
    if (!g.value.len) return;
    // A copy: the file may assign the variable whose value this may be
    Str name = clone(&scratch, expandfilename(sh, &scratch, g.value));
    if (name.len) startupfile(sh, scratch, name, s);
}

// bash's login files: /etc/profile, then the first of ~/.bash_profile,
// ~/.bash_login and ~/.profile (only ~/.profile when named sh).
static void loginfiles(Shell *sh, Arena scratch, Startup *s, b32 actsh)
{
    if (s->noprofile) return;
    startupfile(sh, scratch, "/etc/profile", s);
    if (actsh) {
        startupfile(sh, scratch, "~/.profile", s);
    } else if (!startupfile(sh, scratch, "~/.bash_profile", s) &&
               !startupfile(sh, scratch, "~/.bash_login", s)) {
        startupfile(sh, scratch, "~/.profile", s);
    }
}

// Read the startup files for how the shell was started, with $0 and the
// positional parameters already set. bash's rules (POSIX and msys
// modes): -l or --login reads the login files unless in POSIX mode; a
// $0 starting with - makes a login shell that reads them only when
// interactive. A non-interactive shell then reads $BASH_ENV (not when
// named sh or in POSIX mode). An interactive one reads ~/.bashrc, or
// --rcfile's file, unless --norc or a login shell (an msys shell, as
// Git Bash, reads /etc/bash.bashrc first); named sh or in POSIX mode
// it reads $ENV instead. Named sh, the shell enters POSIX mode after
// the files. busybox-w32's rules (native mode, w64devkit's build): a
// login shell (-l, --login, a $0 starting with - or l, as lash) reads
// <system drive>/etc/profile, <exe>/../etc/profile and $HOME/.profile;
// then -d's directory becomes the working directory, and an interactive
// shell reads $ENV. --noprofile works in both.
static void startup(Shell *sh, Arena scratch, Str argv0, Startup *s)
{
    b32 dash = startswith(argv0, "-");
    Str base = progbase(dash ? cuthead(argv0, 1) : argv0);
    if (s->busybox) {
        b32 login = s->login || dash || startswith(base, "l") || startswith(base, "L");
        sh->login = sh->shopt[S_LOGIN_SHELL] = login;
        if (login && !s->noprofile) {
            Str windir = sh->paths->windir;
            Str drive  = windir.len>1 && windir[1]==':' ? takehead(windir, 2) : Str("C:");
            Str path = concat(&scratch, drive, "/etc/profile");
            startupfile(sh, scratch, path, s);
            Str dir = takehead(sh->exe, sh->exe.len - lastpart(sh->exe).len);
            if constexpr (LIBMODE) dir = selfdir(sh);
            if (!LIBMODE || dir.len) {  // the library with no root: none
                Str path2 = concat(&scratch, dir, "../etc/profile");
                startupfile(sh, scratch, path2, s);
            }
            Got home = getplain(sh, &scratch, "HOME");
            if (home.set) {
                Str path = concat(&scratch, home.value, "/.profile");
                startupfile(sh, scratch, path, s);
            }
        }
        if (s->dir.data) {
            // As busybox, quietly not when it is no directory, and
            // neither CDPATH nor a message for a relative one
            Str  dir = absolute(sh, &scratch, s->dir);
            Stat st  = pathstat(sh, scratch, dir, 1);
            if (!st.err && st.type==FT_DIR) cdto(sh, "cd", dir, scratch);
        }
        if (sh->interactive) envfile(sh, scratch, "ENV", s);
        return;
    }

    b32 actsh = sh->win32 ? equalfold(base, "sh") : base=="sh";
    b32 login = s->login || dash;
    sh->login = sh->shopt[S_LOGIN_SHELL] = login;
    b32 read  = 0;  // the login files
    if (s->login && !s->posix) {
        loginfiles(sh, scratch, s, actsh);
        read = 1;
    }
    if (!sh->interactive) {
        if (!s->posix && !actsh) envfile(sh, scratch, "BASH_ENV", s);
    } else if (s->posix) {
        envfile(sh, scratch, "ENV", s);
    } else {
        if (login && !read) loginfiles(sh, scratch, s, actsh);
        if (actsh) {
            envfile(sh, scratch, "ENV", s);
        } else if (!s->norc && !login) {
            if (sh->pathmode == PM_MSYS) {
                startupfile(sh, scratch, "/etc/bash.bashrc", s);
            }
            startupfile(sh, scratch, s->rcfile.data ? s->rcfile : Str("~/.bashrc"), s);
        }
    }
    if (actsh && !sh->opt[O_POSIX]) setposix(sh, 1);
}

// A login shell ending by the exit builtin (not in a subshell), or an
// interactive one ending at all, reads ~/.bash_logout, once, and Git
// Bash /etc/bash.bash_logout after it. The exit status stays unless
// the file runs exit itself.
static void logoutfiles(Shell *sh, Arena scratch)
{
    if (!sh->login || sh->loggedout || sh->subshell || sh->pathmode==PM_NATIVE) {
        return;
    }
    sh->loggedout = 1;
    Startup s  = {};
    i32 status = sh->status;
    startupfile(sh, scratch, "~/.bash_logout", &s);
    if (sh->pathmode == PM_MSYS) {
        startupfile(sh, scratch, "/etc/bash.bash_logout", &s);
    }
    sh->status = status;
}

// SHELLOPTS or BASHOPTS inherited from the environment turns on the
// options it names, as in bash.
static void importopts(Shell *sh, Arena scratch, b32 shopt)
{
    if (sh->rshell) return;  // as bash, a restricted shell ignores them
    Str list = getplain(sh, &scratch, shopt ? Str("BASHOPTS") : Str("SHELLOPTS")).value;
    while (list.len) {
        Cut c = cut(list, ':');
        list = c.tail;
        if (!shopt) {
            setoptionbyname(sh, c.head, 1);
            continue;
        }
        for (i32 k = 0; k < S_COUNT; k++) {
            if (shoptnames[k]!=c.head || !hasshopt(sh, k)) continue;
            sh->shopt[k] = 1;
            if (k==S_ARRAY_EXPAND_ONCE || k==S_ASSOC_EXPAND_ONCE) {
                sh->shopt[S_ARRAY_EXPAND_ONCE] = sh->shopt[S_ASSOC_EXPAND_ONCE] = 1;
            }
        }
    }
}

static void addbadenv(Shell *sh, Str entry)
{
    Str *n = (Str *)salloc(sh->store, (sh->nbadenv+1)*(iz)sizeof(Str));
    for (iz k = 0; k < sh->nbadenv; k++) n[k] = sh->badenv[k];
    n[sh->nbadenv++] = sclone(sh->store, entry);
    sh->badenv = n;
}

// The path mode and mount table, before the environment is imported.
// On Windows the persona comes from argv[0] and whether the install
// root (the shell's directory, less usr/bin or bin) has an etc/fstab,
// which also gives the mounts (paths.cpp's personaof and initmounts).
// An open that found nothing
static Opened unopened()
{
    Opened o = {};
    o.err = E_NOENT;
    return o;
}

static void pathsetup(Shell *sh, OsInfo *info, Arena *perm, Arena scratch)
{
    PathCtx *c = alloc<PathCtx>(perm);
    sh->paths    = c;
    sh->win32    = info->windows;
    sh->pathmode = PM_POSIX;
    if (!info->windows) return;

    Slice<Str> env = {info->env, info->nenv};
    Str cwd = winnorm(perm, info->cwd, "C:/");
    Str exe = winnorm(perm, info->exe, cwd.data ? cwd : Str("C:/"));
    c->pid  = info->pid;
    c->cwd  = cwd.data ? cwd : Str("C:/");
    sh->exe = exe.data ? exe : Str{};
    c->windir  = info->windir.len ? winnorm(perm, info->windir, "C:/") : Str{};
    sh->sniffs = newsniffcache(perm);
    Str temp = envlookup(env, "TEMP", 1);
    temp = temp.data ? temp : envlookup(env, "TMP", 1);
    c->temp = temp.data ? winnorm(perm, temp, c->cwd) : Str{};

    Str root = takehead(sh->exe, sh->exe.len-lastpart(sh->exe).len);
    if constexpr (LIBMODE) root = selfdir(sh);
    b32 none = LIBMODE && !root.len;  // the library with no root or self
    for (; root.len>3 && root[root.len-1]=='/'; root.len--) {}
    Str bindir = {};  // the shell's directory when it is root/bin
    if (root.len>8 && equalfold(taketail(root, 8), "/usr/bin")) {
        root.len -= 8;
    } else if (root.len>4 && equalfold(taketail(root, 4), "/bin")) {
        bindir = root;
        root.len -= 4;
    }
    if (!root.len) {
        // The start directory's drive, or with UB_SYSROOT the system's
        b32 sys = 0;
        if constexpr (LIBMODE) sys = none && lib_sysroot(sh->os) && c->windir.len;
        root = nativeroot(sys ? c->windir : c->cwd);
    }
    root = clone(perm, root);

    // fstab, read whole if it is there
    Str    fstab = {};
    Str path = concat(&scratch, root, "/etc/fstab");
    Opened o     = !LIBMODE || !none ? os_open(sh->os, scratch, path, OF_READ|OF_OWN, 0) : unopened();
    if (!o.err) {
        Buf b(&scratch, 4096);
        for (;;) {
            reserve(&b, 4096);
            iz n = os_read(sh->os, o.h, b.data+b.len, b.cap-b.len);
            if (n <= 0) break;
            b.len += n;
        }
        os_close(sh->os, o.h);
        fstab = clone(perm, finish(&b));
    }
    initmounts(c, perm, root, fstab);
    if (!fstab.data) {
        c->cygdrive = {};  // no fstab: /c is C:, as in every MSYS2 install
        if (bindir.data) {
            // w64devkit's layout has no usr/bin: /bin is the shell's
            // own directory, so PATH survives a trip through msys form
            addmount(c, perm, bindir, "/bin", 0, 0);
        }
        // nor a tmp: /tmp is the user's temporary directory, as Git for
        // Windows' fstab mounts it (usertemp), so /tmp paths work
        if (c->temp.len) {
            addmount(c, perm, c->temp, "/tmp", MNT_NOCASE|MNT_IMMUTABLE, 0);
        }
    }
    Str argv0    = info->nargs ? info->args[0] : sh->exe;
    sh->pathmode = personaof(argv0.len ? argv0 : Str("bash"), fstab.data!=0);
}

// Import one Windows environment variable in msys mode, as the MSYS2
// runtime does before bash starts (environ.cc): its names in uppercase
// (renv_arr), the path variables in POSIX form. False to drop it.
static b32 importmsys(Shell *sh, Arena *a, Str *name, Str *value)
{
    static Str upper[] = {
        "COMMONPROGRAMFILES", "COMSPEC", "PATH", "PROGRAMFILES",
        "SYSTEMDRIVE", "SYSTEMROOT", "TEMP", "TMP", "WINDIR", "MSYSTEM",
    };
    for (iz i = 0; i < countof(upper); i++) {
        if (equalfold(*name, upper[i])) *name = upper[i];
    }
    if (*name=="TERM" && *value=="msys") return 0;
    PathCtx c = pathctx(sh, a, {});
    *value = envtoposix(&c, a, *name, *value);
    return 1;
}

// Import the environment in native mode, as busybox-w32's ash does when
// a Windows parent started it (a mixed-case Path, ComSpec or
// ProgramData): names in uppercase, an invalid name also as a variable
// with _ for its bad characters, and values with forward slashes except
// COMSPEC and SYSTEMROOT. Returns whether it did.
// An imported BASH_COMPAT is checked once $0 is final, as bash's
// sv_shcompat comes after set_argv0 (initvars).
static i32 importhook(Str name)
{
    return name=="BASH_COMPAT" ? SV_NOHOOK : 0;
}

static b32 importnative(Shell *sh, OsInfo *info, Arena scratch)
{
    b32 winenv = 0;
    for (iz i = 0; i < info->nenv; i++) {
        Str n = cut(info->env[i], '=').head;
        winenv |= n=="Path" || n=="ComSpec" || n=="ProgramData";
    }
    if (!winenv) return 0;
    for (iz i = 0; i < info->nenv; i++) {
        Cut c = cut(info->env[i], '=');
        if (!c.ok) {
            continue;
        } else if (!c.head.len) {
            addbadenv(sh, info->env[i]);  // =C:=C:\x and the like
            continue;
        } else if (startswith(c.head, "BASH_FUNC_") && endswith(c.head, "%%")) {
            importfunc(sh, scratch, c.head, c.tail);
            continue;
        }
        Str name = clone(&scratch, c.head);
        if (!validvarname(name)) {
            addbadenv(sh, info->env[i]);
            for (iz k = 0; k < name.len; k++) {
                if (!alnum(name[k]) && name[k]!='_') name[k] = '_';
            }
            if (digit(name[0])) continue;
        }
        for (iz k = 0; k < name.len; k++) name[k] = upcase(name[k]);
        Str value = c.tail;
        if (name!="COMSPEC" && name!="SYSTEMROOT") value = slashify(&scratch, value, '/');
        setvar(sh, scratch, name, value, SV_FORCE|importhook(name));
        setattrs(sh, name, V_EXPORT, 0);
    }
    return 1;
}

// Defaults the Windows parents leave out. msys: HOME from USERPROFILE
// and TERM as the MSYS2 runtime sets them, PATH its /usr/bin. Native:
// HOME, USER and LOGNAME as busybox-w32 sets them (not exported), PATH
// the shell's own directory, and BB_GLOBBING exported.
static void initwinvars(Shell *sh, Arena scratch)
{
    Arena *a    = &scratch;
    b32    msys = sh->pathmode == PM_MSYS;
    Got    home = getplain(sh, a, "HOME");
    Got    prof = getplain(sh, a, "USERPROFILE");
    if (!home.set && prof.value.len) {
        PathCtx c = pathctx(sh, a, {});
        Str     h = msys ? envtoposix(&c, a, "HOME", prof.value)
                         : slashify(a, prof.value, '/');
        setvar(sh, scratch, "HOME", h);
        if (msys) setattrs(sh, "HOME", V_EXPORT, 0);
    }
    if (msys) {
        if (!getplain(sh, a, "TERM").set) {
            setvar(sh, scratch, "TERM", "xterm-256color");
            setattrs(sh, "TERM", V_EXPORT, 0);
        }
        if (!getplain(sh, a, "PATH").set) {
            setvar(sh, scratch, "PATH", "/usr/bin");
            setattrs(sh, "PATH", V_EXPORT, 0);
        }
        return;
    }
    Got user = getplain(sh, a, "USERNAME");
    if (user.value.len && !getplain(sh, a, "USER").set) {
        setvar(sh, scratch, "USER", user.value);
    }
    if (user.value.len && !getplain(sh, a, "LOGNAME").set) {
        setvar(sh, scratch, "LOGNAME", user.value);
    }
    // busybox-w32's startup (win32/mingw.c's _setargv): BB_GLOBBING=0,
    // exported unless inherited, tells its programs that a shell already
    // expanded their arguments, and that they are in its environment
    if (!getplain(sh, a, "BB_GLOBBING").set) {
        setvar(sh, scratch, "BB_GLOBBING", "0");
        setattrs(sh, "BB_GLOBBING", V_EXPORT, 0);
    }
    if (!getplain(sh, a, "PATH").set) {
        Str dir = takehead(sh->exe, sh->exe.len-lastpart(sh->exe).len);
        if constexpr (LIBMODE) dir = selfdir(sh);
        if (!LIBMODE || dir.len) {  // the library with no root: none
            setvar(sh, scratch, "PATH", dir.len>3 ? cuttail(dir, 1) : dir);
        }
    }
}

static void initvars(Shell *sh, OsInfo *info, Arena scratch)
{
    // The environment: exported variables
    b32 msys = sh->pathmode == PM_MSYS;
    b32 done = sh->pathmode==PM_NATIVE && importnative(sh, info, scratch);
    for (iz i = 0; !done && i < info->nenv; i++) {
        Cut c = cut(info->env[i], '=');
        if (c.ok && startswith(c.head, "BASH_FUNC_") && endswith(c.head, "%%")) {
            importfunc(sh, scratch, c.head, c.tail);
            continue;
        }
        if (c.ok && !validvarname(c.head)) {
            // not a variable, but passed on to programs, as bash does
            addbadenv(sh, info->env[i]);
            continue;
        }
        if (!c.ok) continue;
        if (msys && !importmsys(sh, &scratch, &c.head, &c.tail)) continue;
        setvar(sh, scratch, c.head, c.tail, SV_FORCE|importhook(c.head));
        setattrs(sh, c.head, V_EXPORT, 0);
    }
    Arena *a = &scratch;
    Got g;

    // Identity: POSIX as the build target (the platform layer's triple)
    // or else the oracle's, msys as Git Bash's, native as a MinGW
    // build's. HOSTTYPE and OSTYPE are the triple's cpu and os, as bash
    // takes them from its configure triple.
    Str host = sizeof(void *)==4 ? Str("i686") : Str("x86_64");
    Str mach = info->machtype.len ? info->machtype : Str("x86_64-pc-linux-gnu");
    sh->mach = sclone(sh->store, mach);  // for reinitvars
    Str bash = info->args && info->nargs ? Str("/usr/local/bin/bash") : Str("bash");
    Cut cpu  = cut(mach, '-');
    Str os   = cut(cpu.tail, '-').tail;
    Str posixhost = cpu.head;
    Str shell = "/usr/local/bin/bash";
    if (sh->pathmode == PM_MSYS) {
        PathCtx c = pathctx(sh, a, {});
        mach  = concat(a, host, "-pc-cygwin");
        os    = "cygwin";
        bash  = sh->exe.len ? toposix(&c, a, sh->exe) : Str("/usr/bin/bash");
        if (endswith(bash, ".exe")) bash = cuttail(bash, 4);
        shell = "/usr/bin/bash";  // the account's shell in MSYS2's passwd
    } else if (sh->pathmode == PM_NATIVE) {
        mach  = concat(a, host, "-w64-mingw32");
        os    = "win32";
        bash  = sh->exe.len ? sh->exe : Str("sh");
        shell = bash;
    }
    setvar(sh, scratch, "BASH", bash);
    setvar(sh, scratch, "BASH_VERSION", "5.3.0(1)-release");
    setvar(sh, scratch, "HOSTTYPE", sh->pathmode==PM_POSIX ? posixhost : host);
    setvar(sh, scratch, "MACHTYPE", mach);
    setvar(sh, scratch, "OSTYPE", os);
    setvar(sh, scratch, "OPTERR", "1");
    setvar(sh, scratch, "OPTIND", "1");
    setattrs(sh, "OPTIND", V_INTEGER, 0);
    setvar(sh, scratch, "IFS", " \t\n");
    // PS4 comes from the environment, except for root
    if (info->euid==0 || !getplain(sh, a, "PS4").set) {
        setvar(sh, scratch, "PS4", "+ ");
    }
    sh->host = sclone(sh->store, info->host.len ? info->host : Str("oracle"));
    setvar(sh, scratch, "HOSTNAME", sh->host);
    sh->user = sclone(sh->store, getplain(sh, a, sh->win32 ? Str("USERNAME") : Str("USER")).value);
    // Each number first: built in the call, setvar's copy of scratch
    // (taken first under clang) would allocate over it
    Str num = fmti64(a, info->ppid);
    setvar(sh, scratch, "PPID", num);
    setattrs(sh, "PPID", V_INTEGER|V_READONLY, 0);
    num = fmti64(a, info->uid);
    setvar(sh, scratch, "UID", num);
    setattrs(sh, "UID", V_INTEGER|V_READONLY, 0);
    num = fmti64(a, info->euid);
    setvar(sh, scratch, "EUID", num);
    setattrs(sh, "EUID", V_INTEGER|V_READONLY, 0);
    if (sh->pathmode != PM_POSIX) {
        initwinvars(sh, scratch);
    }
    g = getplain(sh, a, "TERM");
    // busybox-w32 leaves TERM unset, which a console needs no more of
    if (!g.set && sh->pathmode!=PM_NATIVE) setvar(sh, scratch, "TERM", "dumb");
    g = getplain(sh, a, "SHELL");
    if (!g.set) setvar(sh, scratch, "SHELL", shell);
    g = getplain(sh, a, "PATH");
    if (!g.set) {
        setvar(sh, scratch, "PATH",
               "/usr/local/bin:/usr/local/sbin:/usr/bin:/usr/sbin:/bin:/sbin:.");
    }

    // PWD: inherited if it names the working directory
    g = getplain(sh, a, "PWD");
    if (sh->pathmode==PM_POSIX && info->cwderr) {
        // No usable name: the process's own directory is the shell's
        // (an empty cwd). With no name at all bash reports it, keeps
        // PWD as inherited, and its pwd fails; one too long to resolve
        // is still the logical directory.
        sh->cwd = {};
        if (info->cwd.len) {
            sh->wd = sclone(sh->store, info->cwd);
            setvar(sh, scratch, "PWD", info->cwd);
            setattrs(sh, "PWD", V_EXPORT, 0);
        } else {
            Str msg = concat3(&scratch,
                "shell-init: error retrieving current directory: getcwd: "
                "cannot access parent directories: ", errtext(info->cwderr), "\n");
            fdwrite(sh, 2, msg);
            sh->cwderr = info->cwderr;
            b32 named  = g.set && isabsolute(sh, g.value);
            sh->wd = named ? sclone(sh->store, g.value) : Str{};
        }
    } else {
        Str pwd = info->cwd.len ? info->cwd : Str("/");
        Str cwd = pwd;
        if (sh->pathmode != PM_POSIX) {
            cwd = sh->paths->cwd;
            pwd = cwd;
            if (sh->pathmode == PM_MSYS) {
                PathCtx c = pathctx(sh, a, {});
                pwd = toposix(&c, a, cwd);
            }
        }
        if (g.set && isabsolute(sh, g.value) && sh->pathmode!=PM_NATIVE) {
            Str  abs = absolute(sh, a, g.value);  // as for the numbers
            Stat x   = os_stat(sh->os, scratch, abs, 1|ST_OWN);
            Stat y = os_stat(sh->os, scratch, cwd, 1|ST_OWN);
            if (!x.err && !y.err && x.ino==y.ino && x.dev==y.dev) {
                pwd = clone(a, g.value);  // setvar frees PWD's old buffer
            }
        }
        setvar(sh, scratch, "PWD", pwd);
        setattrs(sh, "PWD", V_EXPORT, 0);
        sh->cwd = sclone(sh->store, cwd);
        sh->wd  = pwd==cwd ? sh->cwd : sclone(sh->store, pwd);
        if (sh->cwd.len) os_cwd(sh->os, scratch, sh->cwd);
    }
    g = getplain(sh, a, "OLDPWD");
    if (!g.set) setattrs(sh, "OLDPWD", V_EXPORT, 0);

    // SHLVL counts nested shells
    g = getplain(sh, a, "SHLVL");
    i64 lvl = 0;
    if (g.set && !parsei64(g.value, &lvl)) lvl = 0;
    lvl = lvl<0 ? 0 : lvl+1;
    num = fmti64(a, lvl);
    setvar(sh, scratch, "SHLVL", num);
    setattrs(sh, "SHLVL", V_EXPORT, 0);

    sh->envgen = ~(u64)0;  // programs get the environment made anew
    dynamic(sh, "LINENO", DYN_LINENO);
    dynamic(sh, "RANDOM", DYN_RANDOM, V_INTEGER);
    dynamic(sh, "SRANDOM", DYN_SRANDOM, V_INTEGER);
    dynamic(sh, "SECONDS", DYN_SECONDS, V_INTEGER);
    dynamic(sh, "EPOCHSECONDS", DYN_EPOCHSECONDS);
    dynamic(sh, "EPOCHREALTIME", DYN_EPOCHREALTIME);
    dynamic(sh, "BASHPID", DYN_BASHPID, V_INTEGER);
    dynamic(sh, "BASH_SUBSHELL", DYN_BASH_SUBSHELL);
    dynamic(sh, "BASH_MONOSECONDS", DYN_BASH_MONOSECONDS);
    dynamic(sh, "BASH_COMMAND", DYN_BASH_COMMAND);
    dynamic(sh, "FUNCNAME", DYN_FUNCNAME, V_ARRAY);
    dynamic(sh, "BASH_LINENO", DYN_BASH_LINENO, V_ARRAY);
    dynamic(sh, "BASH_SOURCE", DYN_BASH_SOURCE, V_ARRAY);
    dynamic(sh, "BASH_ARGV", DYN_BASH_ARGV, V_ARRAY);
    dynamic(sh, "BASH_ARGC", DYN_BASH_ARGC, V_ARRAY);
    sh->pstatdirty = 1;  // PIPESTATUS exists once a command has run
    setvar(sh, scratch, "BASH_LOADABLES_PATH", "/usr/local/lib/bash:/usr/lib/bash:/opt/local/lib/bash:/usr/pkg/lib/bash:/opt/pkg/lib/bash:.");
    dynamic(sh, "COMP_WORDBREAKS", DYN_COMP_WORDBREAKS);
    dynamic(sh, "DIRSTACK", DYN_DIRSTACK, V_ARRAY);
    importopts(sh, scratch, 0);
    importopts(sh, scratch, 1);
    dynamic(sh, "SHELLOPTS", DYN_SHELLOPTS, V_READONLY);
    dynamic(sh, "BASHOPTS", DYN_BASHOPTS, V_READONLY);
    dynamic(sh, "BASH_ALIASES", DYN_BASH_ALIASES, V_ASSOC);
    dynamic(sh, "BASH_CMDS", DYN_BASH_CMDS, V_ASSOC);
    g = getplain(sh, a, "_");  // else the shell's argv[0]
    startlastarg(sh, scratch, g.set ? g.value : sh->shellname, g.set);
    Str versinfo[] = {"5", "3", "0", "1", "release", mach};
    for (iz i = 0; i < countof(versinfo); i++) {
        setelem(sh, scratch, "BASH_VERSINFO", i, {}, versinfo[i]);
    }
    setattrs(sh, "BASH_VERSINFO", V_READONLY, 0);
    dynamic(sh, "GROUPS", DYN_GROUPS, V_ARRAY|V_NOASSIGN);
    dynamic(sh, "BASH_ARGV0", DYN_BASH_ARGV0);
    for (iz i = 0; i < info->nenv; i++) {
        Cut c = cut(info->env[i], '=');
        if (c.ok && c.head=="BASH_ARGV0") {
            setvar(sh, scratch, "BASH_ARGV0", c.tail);  // and so $0
        }
    }
    // As bash's sv_shcompat, after set_argv0: a warning names that $0
    Str name = sh->name;
    if (sh->argv0.data) sh->name = sh->argv0;
    varchanged(sh, "BASH_COMPAT");  // importhook
    sh->name = name;
    dynamic(sh, "HISTCMD", DYN_HISTCMD, V_INTEGER);
    varchanged(sh, "LANG");
}

// A script without #! run in-process starts as bash's child does once
// shell_reinitialize has deleted every variable and function: from the
// environment it was given, as a new shell would.
static void reinitvars(Shell *sh, Arena scratch, Slice<Str> env)
{
    OsInfo info = {};
    info.args     = &sh->shellname;
    info.nargs    = 1;
    info.env      = env.data;
    info.nenv     = env.len;
    info.cwd      = sh->cwd;
    info.cwderr   = sh->cwderr;
    info.ppid     = sh->ppid;
    info.uid      = sh->uid;
    info.euid     = sh->euid;
    info.host     = sh->host;
    info.machtype = sh->mach;
    sh->vars    = 0;
    sh->ifs     = 0;
    sh->funcs   = 0;
    sh->badenv  = 0;
    sh->nbadenv = 0;
    initvars(sh, &info, scratch);
}

static File *stdfile(Shell *sh, Handle h, i32 flags)
{
    File *f = newfile(sh, F_OS, flags);
    f->h = h;
    return f;
}

// Commands from standard input, read as the parser asks for them, a
// line at a time, so that the commands it runs can read what follows
// (bash's buffered input). A seekable input is read in blocks and then
// rewound past the line; anything else a byte at a time. As in bash,
// it is whatever fd 0 is now, which exec may have changed.
struct StdinSource {
    Shell *sh;
    Buf    buf;
    i32    eofs;   // ends of input in a row ignored for IGNOREEOF
    i64    tmout;  // os_now time when reading the command gives up, or 0
};

// A number as C's atoi reads it, for IGNOREEOF and TMOUT: blanks, a
// sign, then digits as far as they go (held within a billion).
static i64 atoilike(Str v)
{
    iz i = 0;
    for (; i<v.len && (v[i]==' ' || (v[i]>='\t' && v[i]<='\r')); i++) {}
    b32 neg = i<v.len && v[i]=='-';
    i += i<v.len && (v[i]=='-' || v[i]=='+');
    i64 n = 0;
    for (; i<v.len && v[i]>='0' && v[i]<='9' && n<1000000000; i++) {
        n = n*10 + v[i] - '0';
    }
    return neg ? -n : n;
}

// The deadline for reading an interactive command, from its first
// prompt on: TMOUT seconds as atoi reads them, as bash's alarm, or 0.
static i64 tmoutdeadline(Shell *sh)
{
    Arena a    = *scratchat(sh, sh->capdepth+1);
    i64   secs = atoilike(getplain(sh, &a, "TMOUT").value);
    return secs>0 ? os_now(sh->os) + secs*1000000000 : 0;
}

// TMOUT passed with the command unread: bash's message, on standard
// output, then an exit as the exit builtin's, without its checks. The
// logout files run in the next depth's arena, and their command
// substitutions at the one after: claim it.
[[noreturn]] static void tmoutexit(Shell *sh)
{
    fdwrite(sh, 1, "\atimed out waiting for input: auto-logout\n");
    i32 status = sh->status;
    i32 depth  = ++sh->capdepth;
    logoutfiles(sh, *scratchat(sh, depth));
    sh->capdepth = depth - 1;
    sh->status   = status;
    sh->exiting = 1;
    escape(sh, ESC_EXIT);
}

static Str psname(b32 first)
{
    return first ? Str("PS1") : "PS2";
}

// The prompt in variable name (PS1 and the like) as ${name@P} expands
// it, \1 and \2 around the invisible parts while editing. Unset, there
// is none, even under set -u, as in bash.
static Str promptstr(Shell *sh, Arena *a, Str name)
{
    Got v = getplain(sh, a, name);
    if (!v.set) return {};
    // Native mode shows PS1 and PS2 as busybox-w32's shell does
    b32 native = sh->pathmode==PM_NATIVE && (name=="PS1" || name=="PS2");
    Str expr   = concat3(a, "${", name, "@P}");
    Escape e;
    escenter(sh, &e);
    i32 status = sh->status;
    // \! and \# differ by prompt, and HISTCMD while a prompt shows
    sh->promptkind = name=="PS1" ? 1 : name=="PS2" ? 2 : 3;
    // The caller's arena is the next depth's scratch, and a command
    // substitution in the prompt runs there: claim it
    sh->capdepth++;
    Str ps = {};
    if (!jmpset(e.jmp)) {
        ps = native ? nativeprompt(sh, a, v.value) : expandstring(sh, a, expr);
    }
    escresume(sh, &e);  // the depth and promptkind too
    if (e.why == ESC_EXIT) {
        // As set -e with an unbound variable or ${x?} (fatalerror)
        escape(sh, e.why);
    }
    sh->status = status;
    return ps;
}

// Show a prompt on standard error, without the editor's markers.
static void showprompt(Shell *sh, Str name)
{
    Arena a  = *scratchat(sh, sh->capdepth+1);
    Str   ps = promptstr(sh, &a, name);
    iz    n  = 0;
    for (iz i = 0; i < ps.len; i++) {
        if (ps[i]!=1 && ps[i]!=2) ps.data[n++] = ps[i];
    }
    fdwrite(sh, 2, takehead(ps, n));
}

// Read the next bytes of the command input. At an interactive shell's
// prompt, Ctrl-C discards the line: -2.
static iz readstdin(Shell *sh, File *f, u8 *buf, iz len)
{
    if (!sh->interactive) return fileread(sh, f, buf, len);
    Escape e;
    escenter(sh, &e);
    if (jmpset(e.jmp)) {
        if (e.why != ESC_INTR) {
            escleave(sh, &e);
            escape(sh, e.why);
        }
        escresume(sh, &e);
        return -2;
    }
    iz n = fileread(sh, f, buf, len);
    escleave(sh, &e);
    return n;
}

// PS0, shown after a command is read and before it runs.
static void showps0(Shell *sh)
{
    showprompt(sh, "PS0");
}

// PROMPT_COMMAND's commands. Nothing outside catches what escapes them,
// so what bash's jump to the top level would abandon, they are, here:
// the reason (ESC_INTR, ESC_DISCARD, ESC_ABORT), or 0.
static i32 promptrun(Shell *sh, Str *cmds, iz n, Arena a)
{
    Escape e;
    escenter(sh, &e);
    if (jmpset(e.jmp)) {
        if (e.why==ESC_EXIT || e.why==ESC_FATAL) {
            escleave(sh, &e);
            escape(sh, e.why);
        }
        escresume(sh, &e);
        sh->flow = FLOW_NONE;
        return e.why;
    }
    for (iz i = 0; i<n && !sh->exiting; i++) {
        if (cmds[i].len) runsource(sh, cmds[i], "PROMPT_COMMAND", a, 0, 1);
    }
    escleave(sh, &e);
    return 0;
}

// PROMPT_COMMAND, run before each primary prompt: the string, or each
// element of an indexed array, as eval would run it. $_ survives it.
// An interrupt runs it again from the start, as bash's new prompt
// after its jump to the top level does; an error gives it up, status 1.
static void promptcommand(Shell *sh)
{
    Value *v = lookup(sh, "PROMPT_COMMAND");
    if (!v || (v->attrs&V_ASSOC)) return;
    i32   depth  = ++sh->capdepth;
    Arena a      = *scratchat(sh, depth);
    Str   last   = clone(&a, getplain(sh, &a, "_").value);
    i32   status = sh->status;  // $? too, unlike eval
    i32   line   = sh->line;
    b32   lexeof = sh->lexeof;
    AList cmds   = {};
    if (v->attrs & V_ARRAY) {
        cmds = alist(&a, v->arr, 0);
    } else if (v->str.len) {
        cmds.vals = &v->str;
        cmds.n    = 1;
    }
    Str *copies = alloc<Str>(&a, cmds.n);  // the commands may change it
    for (iz i = 0; i < cmds.n; i++) copies[i] = clone(&a, cmds.vals[i]);
    sh->evaldepth++;
    i32 why;
    while ((why = promptrun(sh, copies, cmds.n, a)) == ESC_INTR) {
        status = sh->status = 128 + 2;
    }
    if (why) status = 1;
    sh->evaldepth--;
    sh->capdepth = depth - 1;
    sh->status   = status;
    sh->line     = line;
    sh->lexeof   = lexeof;
    bindlastarg(sh, a, last);
}

// A script read from standard input that cannot be read (not open for
// reading, a directory) ends the shell with 128, its partial line not
// run, as bash's FATAL_READERROR. A closed descriptor is the end of
// input, as bash, which drops its buffered stream then.
[[noreturn]] static void readfailed(Shell *sh, Lexer *L, File *f)
{
    i32 err = readerror(sh, f, E_IO);
    Arena scratch = *scratchat(sh, sh->capdepth+1);
    sh->line = L->lineno;
    Str msg = concat(&scratch, "script file read error: ", errtext(err));
    shellerror(sh, scratch, msg);
    // As bash's exit_shell, whose EXIT trap returns $? instead
    if (!trapset(sh, TRAP_EXIT)) sh->status = 128;
    sh->exiting = 1;
    escape(sh, ESC_EXIT);
}

static b32 fillline(void *ctx, Lexer *L)
{
    StdinSource *in = (StdinSource *)ctx;
    Shell *sh    = in->sh;
    File  *f     = sh->fds->fd[0];
    Buf   *b     = &in->buf;
    iz     start = b->len;
    b32 seekable = f && f->kind==F_OS && os_seek(sh->os, f->h, 0, 1)>=0;
    // As bash, which prints them as it parses: before the next prompt
    if (sh->interactive) reportmessages(sh, L);
    if (sh->interactive && L->newunit) {
        in->tmout = tmoutdeadline(sh);
        if (sh->jobs) notifyprompt(sh, *scratchat(sh, sh->capdepth+1), 0);
        promptcommand(sh);
    }
    if (sh->interactive && (sh->opt[O_EMACS] || sh->opt[O_VI])) {
        Arena a  = *scratchat(sh, sh->capdepth+1);
        Str   ps = promptstr(sh, &a, psname(L->newunit));
        EdRead opts = {};
        opts.delim = -1;
        opts.shell = 1;
        // A command word may start the line at PS1, and at PS2 where
        // the parser's last token allows one, as bash asks it
        opts.ps1   = L->newunit || cmdposition(L, L->last);
        b32 ps2    = !L->newunit;
        opts.deadline = in->tmout;
        opts.init  = clone(&a, sh->hist.preload);
        sfreestr(sh->store, sh->hist.preload);
        sh->hist.preload = {};
        i32   r  = editline(sh, ps, b, &opts);
        if (r == ED_TIMEOUT) tmoutexit(sh);
        if (r != ED_NOTTY) {
            L->newunit = 0;
            if (r == ED_LINE) {
                putbyte(b, '\n');
            } else if (r == ED_INTR) {
                // Ctrl-C at the prompt: an empty line instead, status
                // 130, and at PS2 the command given up. A trap for INT
                // runs now, as bash's, over the row of the echoed ^C,
                // then a new line
                sh->status = 128 + 2;
                b->len = start;
                putbyte(b, '\n');
                L->intr = ps2;
                if (trapset(sh, 2) && interactivetop(sh)) {
                    runtrap(sh, 2);
                    sh->status = 128 + 2;
                    fdwrite(sh, 2, "\n");
                }
            } else if (r == ED_ABORT) {
                // An expansion error while editing: the same, status 1
                sh->status = 1;
                b->len = start;
                putbyte(b, '\n');
            }
            L->src = {b->data, b->len};
            return b->len > start;
        }
    }
    if (sh->interactive) {
        showprompt(sh, psname(L->newunit));
        L->newunit = 0;
    }
    for (b32 done = 0; !done;) {
        iz n = 0;
        if (seekable) {
            reserve(b, 4096);
            n = readstdin(sh, f, b->data+b->len, 4096);
        } else {
            u8    c;
            Task *self = sh->sched->cur;
            self->deadline = in->tmout;
            n = readstdin(sh, f, &c, 1);
            self->deadline = 0;
            if (self->timedout) {
                self->timedout = 0;
                tmoutexit(sh);
            }
            if (n == 1) {
                putbyte(b, c);
                done = c == '\n';
                continue;
            }
        }
        if (n == -2) {
            // Ctrl-C at the prompt (sigdefault wrote the newline): an
            // empty line instead, status 130
            sh->status = 128 + 2;
            b->len = start;
            putbyte(b, '\n');
            break;
        }
        if (n<0 && f && !sh->interactive) readfailed(sh, L, f);
        if (!seekable || n<=0) break;  // end of input, or an error
        iz keep = n;
        for (iz i = 0; i < n; i++) {
            if (b->data[b->len+i] == '\n') {
                keep = i + 1;
                done = 1;
                break;
            }
        }
        if (keep < n) os_seek(sh->os, f->h, keep-n, 1);
        b->len += keep;
    }
    Str got = {b->data+start, b->len-start};
    b->len  = start + squeezecr(got.data, got, crmode(sh));
    L->src  = {b->data, b->len};
    return b->len > start;
}

// The interactive shell's input, where an end of input at the start of
// a command prints exit, or asks for exit as many times as IGNOREEOF
// says (its value as atoi reads it, 10 if empty).
static b32 fillstdin(void *ctx, Lexer *L)
{
    StdinSource *in = (StdinSource *)ctx;
    Shell *sh = in->sh;
    for (;;) {
        b32 first = L->newunit;
        if (fillline(ctx, L)) {
            in->eofs = 0;
            return 1;
        }
        if (!sh->interactive || !first) return 0;
        Str   word = sh->login ? Str("logout") : "exit";
        byte  mem[64];
        Arena a = {mem, mem+sizeof(mem)};
        if (sh->opt[O_IGNOREEOF]) {
            Arena scratch = *scratchat(sh, sh->capdepth+1);
            Str v = getplain(sh, &scratch, "IGNOREEOF").value;
            i64 limit = v.len ? atoilike(v) : 10;
            if (in->eofs < limit) {
                in->eofs++;
                fdwrite(sh, 2, concat3(&a, "Use \"", word, "\" to leave the shell.\n"));
                L->newunit = 1;
                continue;
            }
        }
        fdwrite(sh, 2, concat(&a, word, "\n"));
        if (exitwarning(sh)) {
            L->newunit = 1;
            continue;
        }
        return 0;
    }
}

// Cygwin's glob for a CYG_GLOB argument: braces, a leading ~ when HOME
// names a directory, then the shell's own globbing, and the unquoted
// pattern for an alternative nothing matches (GLOB_NOCHECK).
static Slice<Str> cygexpand(Shell *sh, Arena *a, Slice<Str> out, Str pat)
{
    Slice<Str> alts = cygbraces(a, {}, pat, (iz)1<<12);
    Str home = getplain(sh, a, "HOME").value;
    Stat st  = home.len ? pathstat(sh, *a, home, 1|ST_OWN) : Stat{};
    b32 dir  = home.len && !st.err && st.type==FT_DIR;
    for (iz k = 0; k < alts.len; k++) {
        Str p = alts[k];
        if (dir && p.len && p[0]=='~' && (p.len==1 || p[1]=='/')) {
            Buf b(a);
            for (iz i = 0; i < home.len; i++) {
                if (find("\\*?[{}", {home.data+i, 1}) >= 0) putbyte(&b, '\\');
                putbyte(&b, home[i]);
            }
            print(&b, cuthead(p, 1));
            p = finish(&b);
        }
        Slice<Str> m = glob(sh, a, p);
        if (!m.len) {
            out = push(a, out, cygunquote(a, p));
        }
        for (iz i = 0; i < m.len; i++) {
            out = push(a, out, m[i]);
        }
    }
    return out;
}

// In msys mode, split the raw command line again as the MSYS2 runtime
// does for a native parent (dcrt0.cc's build_argv and globify), which
// is what Git Bash's scripts and agents' wrappers count on: Cygwin's
// quotes, then response files (@file, at most 10) and globbing, which
// splitcygwin leaves to its caller. argv[0] with a ':' or '\' is made
// POSIX, and an empty command line is the program's own path. False,
// after a message, for an 11th response file.
static b32 msysargv(Shell *sh, OsInfo *info, Arena *perm, Arena scratch)
{
    Str cmd = info->cmdline;
    if (!sh->win32 || (!cmd.len && info->nargs)) return 1;
    PathCtx c = pathctx(sh, &scratch, {});
    Slice<Str> out = {};
    if (!cmd.len) {
        Str self = toposix(&c, perm, sh->exe);
        out = push(perm, out, self.data ? self : Str("bash"));
    }
    i32 nfiles  = 0;
    b32 program = 1;
    while (cmd.len) {
        Slice<CygArg> args = splitcygwin(&scratch, cmd, program);
        Str more = {};
        for (iz i = 0; !more.data && i < args.len; i++) {
            CygArg r = args[i];
            if (program && !i) {
                Str arg0 = r.arg;
                if (find(arg0, ":")>=0 || find(arg0, "\\")>=0) {
                    // as POSIX, and in msys mode without .exe (recorded:
                    // $0 is /usr/bin/bash when started as ...\bash.exe)
                    Str p = toposix(&c, &scratch, arg0);
                    arg0 = p.data ? p : arg0;
                    if (p.data && sh->pathmode==PM_MSYS && hasext(arg0, ".exe")) {
                        arg0.len -= 4;
                    }
                }
                out = push(perm, out, clone(perm, arg0));
                continue;
            }
            if (r.flags & CYG_ATFILE) {
                Stat st = pathstat(sh, scratch, r.file, 1|ST_OWN);
                i32  err = st.err ? st.err : st.type!=FT_FILE ? E_ISDIR : 0;
                Str  text = err ? Str{} : readwholefile<OF_OWN>(sh, &scratch, r.file, &err);
                if (!err) {
                    if (++nfiles > 10) {
                        Buf b(&scratch);
                        print(&b, out.len ? out[0] : Str("bash"));
                        print(&b, ": @");
                        print(&b, r.file);
                        print(&b, ": too many levels of response files\n");
                        fdwrite(sh, 2, finish(&b));
                        return 0;
                    }
                    Buf b(&scratch, text.len + cmd.len);
                    print(&b, text);
                    putbyte(&b, ' ');
                    print(&b, cuthead(cmd, r.end));
                    more = finish(&b);
                    continue;
                }
            }
            if (r.flags & CYG_GLOB) {
                Slice<Str> m = cygexpand(sh, &scratch, {}, r.pat);
                for (iz k = 0; k < m.len; k++) out = push(perm, out, clone(perm, m[k]));
            } else {
                out = push(perm, out, clone(perm, r.arg));
            }
        }
        cmd     = more;
        program = 0;
    }
    if (program && !out.len) return 1;  // a blank line: keep the platform's
    info->args  = out.data;
    info->nargs = out.len;
    sh->name      = out.len ? out[0] : Str("bash");
    sh->shellname = sh->name;
    sh->iname     = sh->name;
    return 1;
}

// The descriptors the process started with, each usable as it is open,
// as bash's are: a terminal on fd 0 takes writes, as on 2 reads. One
// the process lacks (BADHANDLE) is closed, as a POSIX parent's `<&-`
// leaves it, not an open File that fails each use.
static void startfds(Shell *sh, OsInfo *info)
{
    for (i32 i = 0; i < 3; i++) {
        if (info->std[i] == BADHANDLE) continue;
        i32 flags = info->stdflags[i];
        flags = flags ? flags : i ? OF_WRITE : OF_READ;
        File *f = stdfile(sh, info->std[i], flags);
        sh->fds->fd[i] = f;
    }
    sh->fds->top = 3;
    for (iz i = 0; i < info->ninherit; i++) {
        InheritFd in = info->inherit[i];
        if (in.fd<3 || in.fd>=FDMAX || sh->fds->fd[in.fd]) {
            os_close(sh->os, in.h);
            continue;
        }
        sh->fds->fd[in.fd] = stdfile(sh, in.h, in.flags);
        sh->fds->top = in.fd>=sh->fds->top ? in.fd+1 : sh->fds->top;
    }
}

// The -o and -O options (or +o, +O) at startup, as set -o and shopt -s
// do it. Without a name they list the options, in the reusable form
// for +; an unknown name ends the shell with status 2, as bash.
static void startoption(Shell *sh, Arena scratch, u8 c, b32 on, Str *name)
{
    BOut out = {};
    out.b    = Buf(&scratch, (iz)1<<12);
    sh->bout = &out;
    b32 ok = 1;
    if (!name && c=='o') {
        listsetoptions(sh, !on);
    } else if (!name) {
        Str args[] = {"shopt", "-p"};
        b_shopt(sh, args, on ? 1 : 2, scratch);
    } else if (c == 'o') {
        i32 r = setoptionbyname(sh, *name, on);
        ok    = r;
        if constexpr (LIBMODE) {
            if (r < 0) sh->opt[O_MONITOR] = 1;  // refused after the setup
        }
    } else {
        ok = setshoptbyname(sh, scratch, *name, on);
    }
    flushbout(sh);
    sh->bout = 0;
    if (!ok) {
        Str msg = c=='o' ? Str(": invalid option name") : Str(": invalid shell option name");
        // bash names itself, as the builtin, for -o only
        Str cmd = c=='o' ? sh->name : Str{};
        Str msg2 = concat(&scratch, *name, msg);
        builtinerror(sh, scratch, cmd, msg2);
        sh->status = 2;
        escape(sh, ESC_EXIT);
    }
}

// The long options, in bash's order, which its usage lists
enum {
    LO_DEBUG, LO_DEBUGGER, LO_DUMPPO, LO_DUMP, LO_HELP, LO_INITFILE,
    LO_LOGIN, LO_NOEDITING, LO_NOPROFILE, LO_NORC, LO_POSIX, LO_PRETTY,
    LO_RCFILE, LO_RESTRICTED, LO_VERBOSE, LO_VERSION,
};
static Str const longopts[] = {
    "debug", "debugger", "dump-po-strings", "dump-strings", "help",
    "init-file", "login", "noediting", "noprofile", "norc", "posix",
    "pretty-print", "rcfile", "restricted", "verbose", "version",
};

// Will this shell be restricted: named rbash, or given -r or
// --restricted? Known before the environment is imported, which a
// restricted shell does without functions, SHELLOPTS and BASHOPTS.
static b32 startsrestricted(OsInfo *info)
{
    if (!info->nargs) return 0;
    Str prog = info->args[0];
    prog = progbase(startswith(prog, "-") ? cuthead(prog, 1) : prog);
    if (equalfold(prog, "rbash")) return 1;
    iz i = 1;
    for (; i<info->nargs && startswith(info->args[i], "-"); i++) {
        Str a    = info->args[i];
        b32 lng  = a.len>2 && a[1]=='-';
        Str name = cuthead(a, lng ? 2 : 1);
        if (name == "restricted") return 1;
        if (name=="rcfile" || name=="init-file") {
            i++;
            continue;
        }
        b32 known = 0;
        for (iz k = 0; k < countof(longopts); k++) known |= name==longopts[k];
        if (!known) break;
    }
    for (; i < info->nargs; i++) {
        Str a = info->args[i];
        if (a.len<1 || (a[0]!='-' && a[0]!='+') || a=="-" || a=="--") break;
        iz next = i + 1;
        for (iz k = 1; k < a.len; k++) {
            if (a[k]=='r' && a[0]=='-') return 1;
            next += a[k]=='o' || a[k]=='O';
        }
        i = next - 1;
    }
    return 0;
}

// The options bash has parsed when it imports the environment's
// functions, before this shell's own parse (IMPORT_*): -v echoes each
// one's text, POSIX mode (also from POSIXLY_CORRECT or POSIX_PEDANTIC)
// refuses a name that is not an identifier, and -n or -p imports none.
// A bad option ends bash first: nothing is echoed then.
static u32 startimport(Shell *sh, OsInfo *info)
{
    u32 flags = 0;
    for (iz i = 0; i < info->nenv; i++) {
        Str name = cut(info->env[i], '=').head;
        if (name=="POSIXLY_CORRECT" || name=="POSIX_PEDANTIC") flags |= IMPORT_POSIX;
    }
    b32 verbose = 0, noexec = 0, priv = 0, bad = 0;
    iz  i = 1;
    for (; i<info->nargs && startswith(info->args[i], "-"); i++) {
        Str a    = info->args[i];
        b32 lng  = a.len>2 && a[1]=='-';
        Str name = cuthead(a, lng ? 2 : 1);
        b32 known = 0;
        for (iz k = 0; k < countof(longopts); k++) known |= name==longopts[k];
        if (!known) {
            bad = lng;
            break;
        }
        if (name == "posix")   flags |= IMPORT_POSIX;
        if (name == "verbose") verbose = 1;
        i += name=="rcfile" || name=="init-file";
    }
    for (; !bad && i<info->nargs; i++) {
        Str a = info->args[i];
        if (a.len<1 || (a[0]!='-' && a[0]!='+') || a=="-" || a=="--") break;
        b32 on   = a[0] == '-';
        iz  next = i + 1;
        for (iz k = 1; !bad && k < a.len; k++) {
            u8  c    = a[k];
            Str name = {};
            if (c == 'o') {
                if (next >= info->nargs) continue;  // a listing
                name = info->args[next++];
                bad  = 1;
                for (i32 o = 0; o < O_COUNT; o++) {
                    if (setoptions[o].name==name && hasoption(sh, o)) bad = 0;
                }
            } else if (c == 'O') {
                next++;
            } else if (c!='c' && c!='s' && c!='i' && c!='l' && c!='r' && c!='D') {
                bad = 1;
                for (i32 o = 0; o < O_COUNT; o++) {
                    if (setoptions[o].letter == c) bad = 0;
                }
            }
            if (c=='v' || name=="verbose") verbose = on;
            if (c=='n' || name=="noexec")     noexec = on;
            if (c=='p' || name=="privileged") priv   = on;
            if (name=="posix") flags = on ? flags|IMPORT_POSIX : flags & ~IMPORT_POSIX;
        }
        i = next - 1;
    }
    if (verbose && !bad) flags |= IMPORT_VERBOSE;
    if (noexec || priv) flags |= IMPORT_NONE;
    return flags;
}

// The options summary, as bash prints it for --help or a bad option
static void shellusage(Shell *sh, Buf *b)
{
    print(b, "Usage:\t");
    print(b, sh->name);
    print(b, " [GNU long option] [option] ...\n\t");
    print(b, sh->name);
    print(b, " [GNU long option] [option] script-file ...\n");
    print(b, "GNU long options:\n");
    for (iz i = 0; i < countof(longopts); i++) {
        print(b, "\t--");
        print(b, longopts[i]);
        putbyte(b, '\n');
    }
    print(b, "Shell options:\n");
    print(b, "\t-ilrsD or -c command or -O shopt_option\t\t(invocation only)\n");
    print(b, "\t-abefhkmnptuvxBCEHPT or -o option\n");
}

// A bad startup option, OPT or its sign and the letter C: complain,
// show the usage and exit with status 2, as bash.
static void badoption(Shell *sh, Arena scratch, Str opt, u8 c = 0)
{
    Buf b(&scratch, 1024);
    print(&b, sh->name);
    print(&b, ": ");
    print(&b, opt);
    if (c) putbyte(&b, c);
    print(&b, ": invalid option\n");
    shellusage(sh, &b);
    fdwrite(sh, 2, finish(&b));
    sh->status = 2;
    escape(sh, ESC_EXIT);
}

// --debugger: source bashdb's start file with extdebug on, as bash
// does, or say why not and carry on without it.
static void startdebugger(Shell *sh, Arena scratch)
{
    Str  path = "/usr/local/share/bashdb/bashdb-main.inc";
    Stat st   = pathstat(sh, scratch, path, 1|ST_OWN);
    if (st.err) {
        Buf b(&scratch, 256);
        print(&b, errname(sh));
        print(&b, ": ");
        print(&b, path);
        print(&b, ": ");
        print(&b, errtext(st.err));
        putbyte(&b, '\n');
        print(&b, errname(sh));
        print(&b, ": warning: cannot start debugger; debugging mode disabled\n");
        fdwrite(sh, 2, finish(&b));
        return;
    }
    setshoptbyname(sh, scratch, "extdebug", 1);
    Str args[] = {"source", path};
    sh->status = b_source(sh, args, 2, scratch);
}

// Stack that shell recursion may use of the OsInfo stack: all but 64K
// (unoptimized, more) for the frames between checks, or half of a small
// one (a low ulimit -s)
static iz stackroom(iz stack)
{
    iz margin = ((iz)1<<16)*STACKSCALE;
    return !stack ? (iz)1<<19 : stack>2*margin ? stack-margin : stack/2;
}

// A self-spawned background shell: take over the parent's image
// (proc.cpp, imagebuild) and run its command, as vprocmain would.
static i32 imagemain(Os *os, OsInfo *info)
{
    ImageHead *h = (ImageHead *)info->image.base;
    if (h->magic!=IMAGEMAGIC || h->build!=imagebuild_()) {
        os_write(os, info->std[2], (u8 *)"bash: bad state image\n", 22);
        return 126;
    }
    Arena  perm = newarena(os, info->reserve);
    Shell *sh   = h->sh;
    sh->os      = os;
    sh->store->a.mem->os = os;
    sh->sched   = newsched(os, &perm, info->reserve, info->chaos, info->std[2]);
    sh->reserve = info->reserve;
    sh->bashpid = info->pid;
    sh->procbase = sh->subshell;  // its traps are the process's
    sh->sched->pid = info->pid;
    if (sh->win32) sh->sniffs = newsniffcache(&perm);
    if (sh->paths) sh->paths->pid = info->pid;

    Escape top;
    escenter(sh, &top);  // the outermost: the image has no escape point
    Sigproc *sig = alloc<Sigproc>(&perm);
    sigenter(sh, sig, sh->bashpid, sh->sched->cur);
    sh->sched->top = sig;
    byte stackmark = 0;
    sh->stacktop = STACKAT(stackmark);
    sh->stackmax = stackroom(info->stack);
    startfds(sh, info);

    Arena scratch = newarena(os, info->reserve);
    if (!jmpset(top.jmp)) {
        checksigs(sh, scratch);
        execute(sh, h->node, scratch);
    }
    storerehold(sh->store, top.hold);  // after an escape, from any holder
    cfatalstatus(sh, &top);
    if (!sh->execed && !sh->killsig) {
        // Its own EXIT trap, set in the background job
        sh->esc = &top;
        if (top.why == ESC_FATAL) fatalunwind(sh, 0);
        errexitundo(sh, 0);
        if (!jmpset(top.jmp)) {
            sh->exiting = 1;
            runexittrap(sh, scratch);
        }
    }
    exitflush(sh);
    // its last program's death, if that is still its status
    b32 tail = sh->childsig && sh->taillevel==sh->subshell+1 &&
               (sh->status & 255)==128+sh->childsig;
    i32 status = (sh->status & 255) | (sh->killsig || tail ? WSIGNALED : 0);
    if (sh->jobs) orphanjobs(sh);
    if (sh->psubs || sh->sched->orphans) {
        Fdtab *t = sh->fds;
        for (i32 fd = 0; fd < t->top; fd++) {
            File *f = t->fd[fd];
            b32 std = f && f->kind==F_OS &&
                      (f->h==info->std[0] || f->h==info->std[1] || f->h==info->std[2]);
            if (f && !std) {
                fileunref(sh, f);
                t->fd[fd] = 0;
            }
        }
        joinpsubs(sh);
        joinorphans(sh->sched);
    }
    if (info->ownstd) closeall(sh);
    freesched(sh->sched);
    return status;
}

// Commands from standard input, the shell's own.
static void readstdin(Shell *sh, Arena scratch)
{
    StdinSource in = {};
    in.sh  = sh;
    Arena inarena = newarena(sh->os, sh->reserve);
    in.buf = Buf(&inarena, 4096);
    runsource(sh, {}, sh->name, scratch, 1, 1, fillstdin, &in);
}

// A library's run ends: the environment for the host's exit hook, which
// the next run may start from, so as a native program gets it, as the
// host passes it back. An escape point of its own, as sh_main's top one
// is still armed: an escape in makeenv (checkstack) leaves it empty,
// and *ok false.
static Slice<Str> exitenv(Shell *sh, Arena *a, b32 *ok)
{
    Escape e;
    escenter(sh, &e);
    if (!jmpset(e.jmp)) {
        Slice<Str> env = nativechildenv(sh, a, makeenv(sh, a));
        escleave(sh, &e);
        *ok = 1;
        return env;
    }
    escleave(sh, &e);
    *ok = 0;
    return {};
}

// -m or -o monitor at startup refused in a POSIX library run, as bash
// finding no terminal for job control: the shell goes on without it.
// An interactive shell with no terminal said so already. Its own
// function, so that the executables' sh_main is unchanged.
static void startnojobctl(Shell *sh, Arena scratch, OsInfo *info, b32 busybox)
{
    if (!sh->opt[O_MONITOR] || !nojobctl(sh)) return;
    sh->opt[O_MONITOR] = 0;
    Os *os  = sh->os;
    b32 tty = os_isatty(os, info->std[0]) || os_isatty(os, info->std[2]);
    if (sh->interactive && !busybox && !tty) return;
    Buf b(&scratch, 64);
    print(&b, progbase(sh->shellname));
    print(&b, ": no job control in this shell\n");
    fdwrite(sh, 2, finish(&b));
}

static i32 sh_main(Os *os, OsInfo *osinfo)
{
    if (osvtable(os)->version != OSVTABLE_VERSION) {
        Str msg = "bash: bad platform table version\n";
        os_write(os, osinfo->std[2], msg.data, msg.len);
        return 126;
    }
    if (osinfo->image.base) return imagemain(os, osinfo);
    OsInfo copy = *osinfo;  // msys mode replaces its arguments
    OsInfo *info = &copy;
    Arena perm    = newarena(os, info->reserve);
    Arena scratch = newarena(os, info->reserve);

    Store *st = alloc<Store>(&perm);
    st->a    = newarena(os, info->reserve);
    st->base = st->a.beg;

    Shell *sh = alloc<Shell>(&perm);
    sh->os      = os;
    sh->sched   = newsched(os, &perm, info->reserve, info->chaos, info->std[2]);
    sh->store   = st;
    sh->fds     = alloc<Fdtab>(&perm);
    sh->reserve = info->reserve;
    sh->name    = info->nargs ? info->args[0] : Str("bash");
    sh->shellname = sh->name;
    sh->iname   = sh->name;
    sh->pid     = info->pid;
    sh->bashpid = info->pid;
    sh->sched->pid = info->pid;
    sh->ppid    = info->ppid;
    sh->parentpid = info->ppid;
    sh->dstart[1] = -1;  // as bash's save_dstart
    sh->startpgid = info->pgid;
    sh->jctty   = BADHANDLE;
    sh->uid     = info->uid;
    sh->euid    = info->euid;
    sh->gid     = info->gid;
    sh->egid    = info->egid;
    sh->start   = os_now(os);

    Escape top;
    escenter(sh, &top);
    Sigproc *sig = alloc<Sigproc>(&perm);
    sigenter(sh, sig, sh->pid, sh->sched->cur);
    sh->sched->top = sig;
    sh->hardign  = info->sigign;
    sh->entryign = info->sigign;
    byte stackmark = 0;
    sh->stacktop = STACKAT(stackmark);
    sh->stackmax = stackroom(info->stack);

    startfds(sh, info);

    defaultoptions(sh, 0);

    if (!jmpset(top.jmp)) {
        pathsetup(sh, info, &perm, scratch);
        nativeoptions(sh);
        sh->rshell = startsrestricted(info);
        sh->importing = startimport(sh, info);
        initvars(sh, info, scratch);
        sh->importing = 0;
        entrytraps(sh);
        if (sh->pathmode==PM_MSYS && !msysargv(sh, info, &perm, scratch)) {
            sh->status = 1;
            escape(sh, ESC_FATAL);
        }
        Str prog = progbase(info->nargs ? info->args[0] : Str{});
        if (sh->pathmode==PM_MSYS && equalfold(prog, "cygpath")) {
            // Started as cygpath (a copy of the shell by that name): be it
            BOut out = {};
            out.b    = Buf(&perm, (iz)1<<12);
            sh->bout = &out;
            sh->status = b_cygpath(sh, info->args, info->nargs, scratch);
            if (out.b.len) fdwrite(sh, 1, finish(&out.b));
            sh->bout = 0;
            escape(sh, ESC_EXIT);
        }

        // Options before the command string or script: bash's long
        // options first (-name too), then the short ones
        iz  i = 1;
        Str cmd = {};
        b32 havecmd = 0;
        b32 selftest_ = 0;
        b32 wanthelp = 0, wantversion = 0, debugger = 0;
        Startup st = {};
        st.busybox = sh->pathmode == PM_NATIVE;
        for (; i<info->nargs && startswith(info->args[i], "-"); i++) {
            Str a = info->args[i];
            if (a == "--unabashed-selftest") {
                selftest_ = 1;
                break;
            }
            b32 longarg = a.len>2 && a[1]=='-';
            Str name = cuthead(a, longarg ? 2 : 1);
            i32 opt = -1;
            for (i32 k = 0; k < countof(longopts); k++) {
                opt = name==longopts[k] ? k : opt;
            }
            if (opt<0 && longarg) {
                badoption(sh, scratch, a);
            } else if (opt < 0) {
                break;  // perhaps short options
            }
            switch (opt) {
            case LO_DEBUG:      break;
            case LO_DEBUGGER:   debugger = 1; break;
            case LO_DUMPPO:     sh->dumpstrings |= 3; break;
            case LO_DUMP:       sh->dumpstrings |= 1; break;
            case LO_HELP:       wanthelp = 1; break;
            case LO_LOGIN:      st.login = 1; break;
            case LO_NOEDITING:  st.noediting = 1; break;
            case LO_NOPROFILE:  st.noprofile = 1; break;
            case LO_NORC:       st.norc = 1; break;
            case LO_POSIX:      setposix(sh, 1); break;
            case LO_PRETTY:     sh->prettyprint = 1; break;
            case LO_RESTRICTED: st.restricted = 1; break;
            case LO_VERBOSE:    sh->opt[O_VERBOSE] = 1; break;
            case LO_VERSION:    wantversion = 1; break;
            case LO_INITFILE:
            case LO_RCFILE:
                if (i+1 == info->nargs) {
                    Buf b(&scratch, 64);
                    print(&b, sh->name);
                    print(&b, ": ");
                    print(&b, name);
                    print(&b, ": option requires an argument\n");
                    fdwrite(sh, 2, finish(&b));
                    sh->status = 2;
                    escape(sh, ESC_EXIT);
                }
                st.rcfile = info->args[++i];
                break;
            }
        }
        if (wanthelp || wantversion) {
            // As bash: before the short options are even looked at
            Buf b(&scratch, 2048);
            if (wanthelp) {
                print(&b, helpversion(sh, &scratch));
                putbyte(&b, '\n');
                shellusage(sh, &b);
                print(&b, "Type `");
                print(&b, sh->name);
                print(&b, " -c \"help set\"' for more information about shell options.\n");
                print(&b, "Type `");
                print(&b, sh->name);
                print(&b, " -c help' for more information about shell builtin commands.\n");
            } else {
                print(&b, helpversion(sh, &scratch));
                print(&b, "\nunabashed ");
                print(&b, unabashedversion(&scratch));
                putbyte(&b, '\n');
                print(&b, "This is free and unencumbered software released into the public domain.\n");
            }
            fdwrite(sh, 1, finish(&b));
            sh->status = 0;
            escape(sh, ESC_EXIT);
        }
        Slice<Str> shopts = {};  // -O and +O: applied after the setup
        Str title = {};          // native -t
        for (; !selftest_ && i<info->nargs; i++) {
            Str a = info->args[i];
            if (a.len<1 || (a[0]!='-' && a[0]!='+')) break;
            if (a=="-" || a=="--") {
                i++;
                break;
            }
            b32 on   = a[0] == '-';
            iz  next = i + 1;  // -o and -O take the arguments that follow
            for (iz k = 1; k < a.len; k++) {
                u8 c = a[k];
                if (c == 'c') {
                    sh->cflag = 1;
                } else if (c == 's') {
                    sh->sflag = 1;
                } else if (c == 'i') {
                    sh->interactive = on;  // the last of -i and +i
                } else if (c == 'o') {
                    Str *name = next<info->nargs ? &info->args[next++] : 0;
                    startoption(sh, scratch, c, on, name);
                } else if (c=='O' && next>=info->nargs) {
                    startoption(sh, scratch, c, on, 0);
                } else if (c == 'O') {
                    shopts = push(&scratch, shopts, Str(on ? "-" : "+"));
                    shopts = push(&scratch, shopts, info->args[next++]);
                } else if (c == 'l') {
                    st.login = 1;
                } else if (c=='r' && on) {
                    st.restricted = 1;
                } else if (c == 'D') {
                    sh->dumpstrings |= 1;
                } else if (st.busybox && on && (c=='d' || c=='t')) {
                    // busybox-w32's undocumented -d DIR, the working
                    // directory, and -t TITLE, the console's title
                    if (next >= info->nargs) {
                        Buf b(&scratch, 64);
                        print(&b, sh->name);
                        print(&b, ": -");
                        putbyte(&b, c);
                        print(&b, ": option requires an argument\n");
                        fdwrite(sh, 2, finish(&b));
                        sh->status = 2;
                        escape(sh, ESC_EXIT);
                    }
                    Str v = info->args[next++];
                    if (c == 'd') {
                        st.dir = v;
                    } else {
                        title = v;
                    }
                } else {
                    b32 found = 0;
                    for (i32 o = 0; o < O_COUNT; o++) {
                        if (setoptions[o].letter == c) {
                            sh->opt[o] = on;
                            found = 1;
                        }
                    }
                    if (!found) badoption(sh, scratch, span(a.data, a.data+1), c);
                }
            }
            i = next - 1;
        }
        if (sh->dumpstrings) sh->opt[O_NOEXEC] = 1;  // as bash: dump only

        // busybox-w32's -s with -c: the command, then standard input
        b32 thenstdin = st.busybox && sh->cflag && sh->sflag;
        if (!sh->interactive && (!sh->cflag || thenstdin) &&
                (sh->sflag || i>=info->nargs) &&
                os_isatty(os, info->std[0]) && os_isatty(os, info->std[2])) {
            sh->interactive = 1;  // as bash: commands from a terminal
        }
        if (title.data) {
            ConOp o = {};
            o.op   = CON_TITLE;
            o.text = title;
            os_con(os, info->std[2], &o);
        }
        histinit(sh);
        sh->cmdnumber = 1;
        if (sh->interactive) {
            // History on, and HISTFILE's default (read after the
            // startup files)
            sh->opt[O_HISTORY] = sh->opt[O_HISTEXPAND] = 1;
            sh->hist.comchar = '#';
            if (!getplain(sh, &scratch, "HISTFILE").set) {
                Str home = getplain(sh, &scratch, "HOME").value;
                Str name = st.busybox        ? Str("/.ash_history")
                         : sh->opt[O_POSIX] ? Str("/.sh_history")
                         :                    Str("/.bash_history");
                Str name2 = concat(&scratch, home, name);
                setvar(sh, scratch, "HISTFILE", name2);
            }
            histvarchanged(sh, "HISTCONTROL");
            histvarchanged(sh, "histchars");
            histvarchanged(sh, "HISTTIMEFORMAT");
            // As bash, it takes HUP and INT even if ignored on entry
            // (its programs still start with them ignored), and acts
            // on HUP, INT, QUIT and TERM itself
            sh->hardign &= ~(sigbit(1) | sigbit(2));
            if (sh->traps) {
                sfreestr(sh->store, sh->traps[1]);
                sfreestr(sh->store, sh->traps[2]);
                sh->traps[1] = sh->traps[2] = {};
            }
            updatesigmode(sh, 1);
            updatesigmode(sh, 2);
            updatesigmode(sh, 15);
            if (os_isatty(os, info->std[0]) || os_isatty(os, info->std[2])) {
                if (!LIBMODE || !nojobctl(sh)) sh->opt[O_MONITOR] = 1;  // job control, as bash
            }
            sh->shopt[S_EXPAND_ALIASES] = 1;
            if (!st.noediting && !sh->opt[O_VI]) sh->opt[O_EMACS] = 1;
            // Native mode takes busybox-w32 ash's prompt, as "~ $ "
            Str ps1 = st.busybox ? Str("\\w \\$ ") : Str("\\s-\\v\\$ ");
            if (!getplain(sh, &scratch, "PS1").set) setvar(sh, scratch, "PS1", ps1);
            if (!getplain(sh, &scratch, "PS2").set) setvar(sh, scratch, "PS2", "> ");
            b32 tty = os_isatty(os, info->std[0]) || os_isatty(os, info->std[2]);
            if (!st.busybox && !tty) {
                // As bash finding no terminal for job control (a
                // terminal on either stream stands in for /dev/tty):
                // the group it went back to, unless it led its own
                Str name = progbase(sh->shellname);
                i64 pgrp = info->pgid && info->pgid!=info->pid ? info->pgid : -1;
                Buf b(&scratch, 128);
                print(&b, name);
                print(&b, ": cannot set terminal process group (");
                print(&b, pgrp);
                print(&b, "): Inappropriate ioctl for device\n");
                print(&b, name);
                print(&b, ": no job control in this shell\n");
                fdwrite(sh, 2, finish(&b));
            }
        }
        // Every shell ignores QUIT itself, and takes INT itself, as bash
        updatesigmode(sh, 2);
        updatesigmode(sh, 3);

        for (iz k = 0; k < shopts.len; k += 2) {
            startoption(sh, scratch, 'O', shopts[k]=="-", &shopts[k+1]);
        }
        if constexpr (LIBMODE) startnojobctl(sh, scratch, info, st.busybox);
        if (sh->opt[O_MONITOR]) histoption(sh, O_MONITOR, 1);

        // $0 and the positional parameters, then the startup files
        Str file = {};
        if (selftest_) {
            // no startup files
        } else if (sh->cflag) {
            if (i >= info->nargs) {
                Buf b(&scratch, 64);
                print(&b, sh->name);
                print(&b, ": -c: option requires an argument\n");
                fdwrite(sh, 2, finish(&b));
                sh->status = 2;
                escape(sh, ESC_EXIT);
            }
            cmd = info->args[i++];
            havecmd = 1;
            setvar(sh, scratch, "BASH_EXECUTION_STRING", cmd);
            if (i < info->nargs) {
                sh->name  = sh->cname = info->args[i++];
                sh->argv0 = {};  // as bash, replacing BASH_ARGV0's
            }
            sh->args.v = info->args + i;
            sh->args.n = info->nargs - i;
            sh->iname  = "-c";
        } else if (i<info->nargs && !sh->sflag) {
            file = info->args[i++];
            sh->name   = file;
            sh->argv0  = {};  // as bash, replacing BASH_ARGV0's
            sh->iname  = file;
            sh->args.v = info->args + i;
            sh->args.n = info->nargs - i;
        } else {
            sh->sflag  = 1;
            sh->args.v = info->args + i;
            sh->args.n = info->nargs>i ? info->nargs-i : 0;
        }
        if (!selftest_) {
            st.posix = sh->opt[O_POSIX];
            startup(sh, scratch, info->nargs ? info->args[0] : Str("bash"), &st);
            // As bash: the restrictions start after the startup files
            sh->rshell |= st.restricted;
            sh->shopt[S_RESTRICTED_SHELL] = sh->rshell;
            if (sh->rshell || sh->restricted) makerestricted(sh);
            if constexpr (LIBMODE) {
                if (lib_exitwanted(os)) {
                    // What the exit hook's delta is against: the exports
                    // as the command starts, after the startup files
                    Arena      a  = scratch;
                    b32        ok = 0;
                    Slice<Str> env = exitenv(sh, &a, &ok);
                    lib_exitbase(os, &perm, env, ok);
                }
            }
        }
        if (sh->interactive && sh->opt[O_HISTORY] && !sh->hist.session) {
            histload(sh);  // unless the startup files added lines
        }
        if (sh->interactive) checkwinsize(sh);  // as bash, once at the start
        if (sh->prettyprint && sh->interactive) {
            Buf b(&scratch, 128);
            print(&b, errname(sh));
            print(&b, ": warning: pretty-printing mode ignored in interactive shells\n");
            fdwrite(sh, 2, finish(&b));
        }
        sh->prettyprint &= !havecmd && !sh->interactive;  // -c just runs
        if (debugger && !selftest_ && (file.data || !sh->interactive)) {
            startdebugger(sh, scratch);
        }

        if (selftest_) {
            Output *out = newoutput(os, &perm, info->std[1]);
            sh->status = selftest(os, &perm, out);
            flush(out);
        } else if (havecmd) {
            sh->hist.noexpand = 1;
            // Its lines go in the history once on, as bash's, but never
            // in native mode, as busybox's
            sh->oneshot = !thenstdin;
            runsource(sh, cmd, "-c", scratch, 0, 1, 0, 0, 0, !st.busybox);
            sh->hist.noexpand = 0;
            if (thenstdin && !sh->exiting) {
                sh->iname = sh->name;
                readstdin(sh, scratch);
            }
        } else if (file.data) {
            // A script file
            i32  err;
            Str  src = readwholefile(sh, &scratch, file, &err);
            if (err) {
                // Windows may refuse to open a directory (.) at all
                Stat d = pathstat(sh, scratch, file, 1);
                err = !d.err && d.type==FT_DIR ? 0 : err;
            }
            Got  path = getplain(sh, &scratch, "PATH");
            if (err==E_NOENT && path.set && !haspath(sh, file)) {
                // As bash: a script found along PATH, whose found name
                // the messages then use, though $0 keeps the given one
                for (Str rest = path.value;;) {
                    Cut c = cut(rest, listsep(sh));
                    Str cand = joinpath(&scratch, c.head.len ? c.head : Str("."), file);
                    Stat cs = pathstat(sh, scratch, cand, 1);
                    if (!cs.err && cs.type==FT_FILE) {
                        i32 e = 0;
                        Str s = readwholefile(sh, &scratch, cand, &e);
                        if (!e) {
                            file = cand;
                            src  = s;
                            err  = 0;
                        }
                        break;
                    }
                    if (!c.ok) break;
                    rest = c.tail;
                }
                sh->iname = file;
            }
            Stat st  = pathstat(sh, scratch, file, 1);
            Str  bad = {};
            if (!err && !st.err && st.type==FT_DIR) {
                bad = errtext(E_ISDIR);
            } else if (!err && binaryfile(src)) {
                bad = "cannot execute binary file";
            }
            if (bad.data) {
                // with $0 already the script, as bash
                Buf b(&scratch, 64);
                print(&b, file);
                print(&b, ": ");
                print(&b, file);
                print(&b, ": ");
                print(&b, bad);
                putbyte(&b, '\n');
                fdwrite(sh, 2, finish(&b));
                sh->status = 126;
            } else if (err) {
                Buf b(&scratch, 64);
                print(&b, info->nargs ? info->args[0] : Str("bash"));
                print(&b, ": ");
                print(&b, file);
                print(&b, ": ");
                print(&b, errtext(err));
                putbyte(&b, '\n');
                fdwrite(sh, 2, finish(&b));
                sh->status = err==E_NOENT ? 127 : 126;
            } else {
                runsource(sh, src, file, scratch, 1);
            }
        } else if (!sh->interactive && !sh->fds->fd[0]) {
            // As bash, which finds no input to buffer on a closed fd 0
            Str msg = "error creating buffered stream: ";
            shellerror(sh, scratch, concat(&scratch, msg, errtext(E_BADF)));
            sh->status = 126;
        } else {
            readstdin(sh, scratch);
        }
    } else if (top.why == ESC_FATAL) {
        cfatalstatus(sh, &top);
    } else if (top.why==ESC_ABORT && sh->cflag) {
        sh->status = 1;  // as bash's -c, its text abandoned
    }
    // An escape may leave holders' counts behind: nothing holds the
    // store here, so the EXIT trap's frees take effect as it goes
    storerehold(sh->store, top.hold);
    if (!sh->execed && !sh->killsig) {
        // A login shell's logout files, signals still pending, then the
        // EXIT trap
        if (sh->interactive && !jmpset(top.jmp)) {
            logoutfiles(sh, scratch);
        }
        storerehold(sh->store, top.hold);  // as above, after the files
        if (!jmpset(top.jmp)) {
            sh->exiting = 0;
            checksigs(sh, scratch);
            sh->exiting = 1;
            if (top.why == ESC_FATAL) fatalunwind(sh, 0);
            if (top.why == ESC_EXIT) exitunwind(sh, 0, sh->cflag);
            if (top.why==ESC_EXIT && sh->cflag && exitpops(sh)) {
                sh->errexitoff = 0;  // as runsource's, for -c's parse
            }
            errexitundo(sh, 0);
            runexittrap(sh, scratch);
        }
        histexit(sh);
        if (sh->interactive && sh->login && sh->shopt[S_HUPONEXIT]) {
            hangupjobs(sh);  // as bash's exit_shell
        }
    }
    jobctloff(sh);
    exitflush(sh);
    if constexpr (LIBMODE) {
        // A library's run: the non-standard descriptors close, so that
        // the procsubs reading them finish, and get a second to do so.
        // The host's teardown ends every program, level and task left.
        Fdtab *t = sh->fds;
        for (i32 fd = 0; fd < t->top; fd++) {
            File *f = t->fd[fd];
            b32 std = f && f->kind==F_OS &&
                      (f->h==info->std[0] || f->h==info->std[1] || f->h==info->std[2]);
            if (f && !std) {
                fileunref(sh, f);
                t->fd[fd] = 0;
            }
        }
        i64 until = os_now(os) + 1000000000;
        while (sh->psubs) {
            Task *p = sh->psubs;
            i32   st;
            if (!taskjoinby(sh->sched, p, until, &st)) break;
            sh->psubs = p->sib;
        }
        i32 status = sh->killsig ? WSIGNALED|(128 + sh->killsig) : sh->status&255;
        if (lib_exitwanted(os)) {
            // The state the next run may start from
            Arena      a   = scratch;
            b32        ok  = 0;
            Slice<Str> env = exitenv(sh, &a, &ok);
            Str        pwd = !sh->wdlost && isabsolute(sh, sh->wd) ? sh->wd : Str{};
            lib_exit(os, a, status, pwd, sh->cwd, env, ok);
        }
        return status;
    }
    if (sh->jobs) orphanjobs(sh);
    if (sh->psubs || sh->sched->orphans) {
        // Procsubs and background jobs still running end on EOF once
        // the shell's ends of their pipes close, as at a process exit,
        // and are waited for: a fiber cannot outlive its process. The
        // platform's own handles stay open.
        Fdtab *t = sh->fds;
        for (i32 fd = 0; fd < t->top; fd++) {
            File *f = t->fd[fd];
            b32 std = f && f->kind==F_OS &&
                      (f->h==info->std[0] || f->h==info->std[1] || f->h==info->std[2]);
            if (f && !std) {
                fileunref(sh, f);
                t->fd[fd] = 0;
            }
        }
        joinpsubs(sh);
        joinorphans(sh->sched);
    }
    if (info->ownstd) closeall(sh);
    freesched(sh->sched);
    if (sh->killsig) return WSIGNALED | (128 + sh->killsig);
    return sh->status & 255;
}
