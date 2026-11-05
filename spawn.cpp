// Programs on Windows: PATH search and the execute bit by path mode, a
// cache of what files hold, and the command line, environment and image
// each kind of target gets (notes/m5a.md, step 7). The platform runs
// Spawn.path with Spawn.cmdline verbatim.
// This is free and unencumbered software released into the public domain.


// What files hold
//
// The Windows modes decide the execute bit and the kind of a target
// from a file's first bytes. What a file holds is remembered by its
// native path (ASCII case folded), modification time and size, in a
// table outside the journaled store, so that subshells and pipeline
// stages share it. When its memory runs out the table starts over.

enum : i32 {
    FK_TEXT,    // none of the below, or empty
    FK_SCRIPT,  // starts with #!
    FK_COLON,   // starts with ":\n", Cygwin's script marker
    FK_PE,      // a PE image, which Sniff.pe describes
    FK_MZ,      // MZ, but no PE image
};

struct Sniff {
    i32    kind;
    PeInfo pe;
    Str    line;  // FK_SCRIPT: what follows #!, up to CR or LF
    b32    ape;   // starts with MZqFpD (an Actually Portable Executable)
};

struct SniffEntry {
    SniffEntry *child[4];
    Str         key;
    i64         mtime;
    i64         size;
    Sniff       s;
};

struct SniffCache {
    Arena       a;
    byte       *base;
    SniffEntry *root;
};

enum : iz {
    SNIFFCACHE = (iz)1<<18,  // bytes of the table
    SNIFFHEAD  = 256,        // bytes read for #! lines, as Linux
};

static SniffCache *newsniffcache(Arena *perm)
{
    SniffCache *c = alloc<SniffCache>(perm);
    c->base  = (byte *)allocbytes(perm, SNIFFCACHE);
    c->a.beg = c->base;
    c->a.end = c->base + SNIFFCACHE;
    return c;
}

struct SniffFile {
    Os    *os;
    Handle h;
};

static iz sniffread(void *ctx, i64 off, u8 *buf, iz len)
{
    SniffFile *f = (SniffFile *)ctx;
    if (os_seek(f->os, f->h, off, 0) != off) return 0;
    iz n = 0;
    while (n < len) {
        iz got = os_read(f->os, f->h, buf+n, len-n);
        if (got <= 0) break;
        n += got;
    }
    return n;
}

// The first DLL an image imports that the loader would find in none of
// its places (the image's directory, System32 and SysWOW64, the Windows
// directory, PATH), as Cygwin names it for STATUS_DLL_NOT_FOUND. Null
// if all are there (a DLL's own import was missing).
static Str missingdll(Shell *sh, Arena *a, Str native, Slice<Str> env)
{
    Opened o = os_open(sh->os, *a, native, OF_READ, 0);
    if (o.err) return {};
    SniffFile f = {sh->os, o.h};
    PeInfo pe = pesniff(sniffread, &f, *a, a);
    os_close(sh->os, o.h);

    Slice<Str> dirs = {};
    dirs = push(a, dirs, takehead(native, native.len-lastpart(native).len));
    Str windir = sh->paths->windir;
    if (windir.len) {
        dirs = push(a, dirs, concat(a, windir, "/System32/"));
        dirs = push(a, dirs, concat(a, windir, "/SysWOW64/"));
        dirs = push(a, dirs, concat(a, windir, "/"));
    }
    Str path = envlookup(env, "PATH", 1);
    for (Cut c = {{}, path, path.len>0}; c.ok;) {
        c = cut(c.tail, sh->pathmode==PM_MSYS ? ':' : ';');
        if (!c.head.len) continue;
        PathRes r = resolve(sh, a, c.head);
        if (r.ok && r.kind==PK_FILE) dirs = push(a, dirs, concat(a, r.native, "/"));
    }

    for (iz i = 0; i < pe.dlls.len; i++) {
        Str dll = pe.dlls[i];
        if (startswith(dll, "api-ms-") || startswith(dll, "ext-ms-")) continue;
        b32 found = 0;
        for (iz d = 0; !found && d < dirs.len; d++) {
            Arena t = *a;
            Stat  s = os_stat(sh->os, t, concat(&t, dirs[d], dll), 1);
            found = !s.err && s.type==FT_FILE;
        }
        if (!found) return dll;
    }
    return {};
}

// What the file at a canonical native path holds, given its status.
static Sniff sniff(Shell *sh, Arena scratch, Str native, Stat st)
{
    SniffCache *c = sh->sniffs;
    Str key = clone(&scratch, native);
    for (iz i = 0; i < key.len; i++) key[i] = lowercase(key[i]);

    SniffEntry **slot = c ? &c->root : 0;
    for (u64 h = hash(key); slot && *slot; h <<= 2) {
        SniffEntry *e = *slot;
        if (e->key == key) {
            if (e->mtime==st.mtime && e->size==st.size) return e->s;
            break;  // changed since: read it again
        }
        slot = &e->child[h>>62];
    }

    Sniff  s = {};
    Opened o = os_open(sh->os, scratch, native, OF_READ, 0);
    if (o.err) return s;  // not remembered: perhaps locked for now
    u8 head[SNIFFHEAD];
    SniffFile f = {sh->os, o.h};
    iz n = sniffread(&f, 0, head, countof(head));
    Str h = {head, n};
    if (startswith(h, "#!")) {
        iz end = 2;
        for (; end<n && head[end]!='\r' && head[end]!='\n'; end++) {}
        s.kind = FK_SCRIPT;
        s.line = span(head+2, head+end);
    } else if (startswith(h, ":\n")) {
        s.kind = FK_COLON;
    } else if (startswith(h, "MZ")) {
        s.pe   = pesniff(sniffread, &f, scratch);
        s.kind = s.pe.ok ? FK_PE : FK_MZ;
        s.ape  = startswith(h, "MZqFpD");
    }
    os_close(sh->os, o.h);

    // Remember it, starting the table over when it is full
    iz need = (iz)sizeof(SniffEntry) + key.len + s.line.len + 64;
    if (!c || need>SNIFFCACHE/4) {
        s.line = clone(&scratch, s.line);
        return s;
    }
    if (c->a.end-c->a.beg < need) {
        c->a.beg = c->base;
        c->root  = 0;
        slot     = &c->root;
    }
    SniffEntry *e = *slot;
    if (!e) {
        e = alloc<SniffEntry>(&c->a);
        e->key = clone(&c->a, key);
        *slot  = e;
    }
    s.line   = clone(&c->a, s.line);
    e->mtime = st.mtime;
    e->size  = st.size;
    e->s     = s;
    return s;
}


// The execute bit

static b32 hasext(Str path, Str ext)  // ASCII case-insensitive suffix
{
    return path.len>=ext.len && equalfold(taketail(path, ext.len), ext);
}

// busybox-w32's executable extensions, in the order it tries them.
static Str winexts[] = {".com", ".exe", ".sh", ".bat", ".cmd"};

static b32 knownext(Str name)
{
    for (iz i = 0; i < countof(winexts); i++) {
        if (hasext(name, winexts[i])) return 1;
    }
    return 0;
}

static b32 isbatch(Str path)
{
    return hasext(path, ".bat") || hasext(path, ".cmd");
}

// The permission bits of a file in the Windows modes. The platform's
// execute bits say only what the content is (#!, MZ or ":\n"); the
// mode adds its rules. msys, Cygwin's noacl rule as recorded (t/msys
// 042): an .exe name, or that content. native, busybox-w32's: one of
// its extensions, #!, a PE program (not a DLL; a native, GUI, console
// or POSIX subsystem), or an APE header.
static i32 winmode(Shell *sh, Arena scratch, Str native, Stat st)
{
    i32 mode = st.mode & ~0111;
    b32 x    = 0;
    if (st.type != FT_FILE) {
        return st.mode;
    } else if (sh->pathmode == PM_MSYS) {
        x = hasext(native, ".exe") || (st.mode & 0111);
    } else if (knownext(native)) {
        x = 1;
    } else if (st.mode & 0111) {
        Sniff s = sniff(sh, scratch, native, st);
        i32 sub = s.pe.subsystem;
        b32 prog = s.kind==FK_PE && !s.pe.dll && (sub==1 || sub==2 || sub==3 || sub==7);
        x = s.kind==FK_SCRIPT || s.ape || prog;
    }
    return mode | (x ? 0111 : 0);
}

// May this shell's user execute the file? As the kernel: the owner's
// bits for the owner, then the group's, then everyone else's.
static b32 canexec(Shell *sh, Stat st)
{
    if (st.err || st.type!=FT_FILE) return 0;
    i32 bits = st.uid==sh->euid ? st.mode>>6 : st.gid==sh->egid ? st.mode>>3 : st.mode;
    return (bits & 1) || (!sh->euid && (st.mode & 0111));
}


// PATH search

// Search a path list for the nth executable match of a name. As bash's
// find_user_command, when nothing executable matches, the first other
// file found is the answer for nth 0 (it then fails to run, as bash
// reports it). msys mode finds foo.exe as foo (pathstat); native mode
// tries busybox-w32's extensions after the bare name, unless the name
// has one or ends in a dot. The result is relative when the PATH entry
// is, as bash shows it.
static Str pathsearch(Shell *sh, Arena *a, Str path, Str name, iz nth = 0)
{
    b32 exts  = sh->pathmode==PM_NATIVE && !knownext(name)
                && !(name.len && name[name.len-1]=='.');
    u8  sep   = listsep(sh);
    b32 found = 0;
    Str lose  = {};
    for (Str rest = path;;) {
        Cut c = cut(rest, sep);
        Str dir  = c.head.len ? c.head : Str(".");
        Str base = joinpath(a, dir, name);
        for (iz e = -1; e < (exts ? countof(winexts) : 0); e++) {
            Str  cand = e<0 ? base : concat(a, base, winexts[e]);
            Stat st   = pathstat(sh, *a, cand, 1);
            if (st.err || st.type==FT_DIR) continue;
            if (!canexec(sh, st)) {
                lose = lose.data ? lose : cand;
            } else if (found = 1, !nth--) {
                return cand;
            }
        }
        if (!c.ok) break;
        rest = c.tail;
    }
    return !found && !nth ? lose : Str{};
}

// Search PATH for the nth match of an executable name.
static Str pathlookup(Shell *sh, Arena *a, Str name, iz nth = 0)
{
    Got g = getvar(sh, a, "PATH");
    if (!g.set) {
        // As bash: only the working directory, giving the bare name
        Stat st = pathstat(sh, *a, name, 1);
        return !nth && !st.err && st.type!=FT_DIR ? name : Str{};
    }
    return pathsearch(sh, a, g.value, name, nth);
}


// Command lines and environments

// The UTF-16 length of a WTF-8 string.
static iz widelen(Str s)
{
    iz n = 0;
    for (iz i = 0; i < s.len; i++) {
        n += (s[i]&0xc0) != 0x80;
        n += s[i] >= 0xf0;
    }
    return n;
}

// Append an argument for a native program as an MSYS2 program quotes it
// (Cygwin's linebuf::fromargv): in quotes only when empty or holding a
// blank, tab, CR, LF or quote, a quote escaped and the backslashes
// before it or the closing quote doubled. Every MSVCRT-family parser
// (msvcrt.dll, UCRT, CommandLineToArgvW) reads it back alike, since it
// never has "" inside quotes, so the target's CRT kind changes nothing.
static void quotemsys(Buf *b, Str arg)
{
    b32 quote = !arg.len;
    for (iz i = 0; i < arg.len; i++) {
        switch (arg[i]) {
        case ' ': case '\t': case '\r': case '\n': case '"':
            quote = 1;
        }
    }
    if (!quote) {
        print(b, arg);
        return;
    }
    putbyte(b, '"');
    iz slashes = 0;
    for (iz i = 0; i < arg.len; i++) {
        if (arg[i] == '\\') {
            slashes++;
        } else {
            if (arg[i] == '"') {
                for (iz k = 0; k <= slashes; k++) putbyte(b, '\\');
            }
            slashes = 0;
        }
        putbyte(b, arg[i]);
    }
    for (; slashes; slashes--) putbyte(b, '\\');
    putbyte(b, '"');
}

enum : i32 {  // how a target splits its command line
    SPLIT_MSVCRT,  // the MSVCRT family: quotearg (native mode)
    SPLIT_MSYS,    // the same parsers as an MSYS2 parent quotes them
    SPLIT_CYGWIN,  // Cygwin's own startup (MSYS2 programs, our msys shell)
};

static Str cmdline(Arena *a, Slice<Str> argv, i32 split)
{
    Buf b(a, 256);
    for (iz i = 0; i < argv.len; i++) {
        if (i) putbyte(&b, ' ');
        if (split == SPLIT_MSYS) {
            quotemsys(&b, argv[i]);
        } else if (!i) {
            quoteprogram(&b, argv[i]);
        } else if (split == SPLIT_CYGWIN) {
            quotecygwin(&b, argv[i]);
        } else {
            quotearg(&b, argv[i]);
        }
    }
    return finish(&b);
}

static Str envvalue(Slice<Str> env, Str name)  // ignoring case, or null
{
    return envlookup(env, name, 1);
}

// The single paths among the variables the MSYS2 runtime converts for
// a child, already in Windows form: like its path conversion, add .exe
// to a name that is missing while its .exe is a file (recorded:
// SHELL=/usr/bin/bash reaches a child as ...\bash.exe, arguments and
// other variables do not).
static Slice<Str> envexes(Shell *sh, Arena *a, Slice<Str> env)
{
    static Str paths[] = {"HOME", "SHELL", "TMPDIR", "TMP", "TEMP"};
    for (iz i = 0; i < env.len; i++) {
        Cut n = cut(env[i], '=');
        if (!n.ok || !n.tail.len) continue;
        for (iz k = 0; k < countof(paths); k++) {
            if (n.head != paths[k]) continue;
            Arena t = *a;
            Str   p = slashify(&t, n.tail, '/');
            if (!hasdrive(p) || os_stat(sh->os, t, p, 1).err!=E_NOENT) break;
            Stat x = os_stat(sh->os, t, concat(&t, p, ".exe"), 1);
            if (!x.err && x.type==FT_FILE) env[i] = concat(a, env[i], ".exe");
            break;
        }
    }
    return env;
}

// The environment for an MSYS2 program (or our own shell) started by an
// msys shell: the variables its runtime converts at startup (environ.cc
// conv_envvars) in Windows form, so that it imports them back as they
// were, and everything else as is.
static Slice<Str> msyschildenv(Shell *sh, Arena *a, Slice<Str> env)
{
    static Str lists[] = {"PATH", "ORIGINAL_PATH", "LD_LIBRARY_PATH"};
    static Str paths[] = {"HOME", "SHELL", "TMPDIR", "TMP", "TEMP"};
    if (sh->pathmode != PM_MSYS) return env;
    PathCtx    c = pathctx(sh, a, {});
    Slice<Str> r = {};
    for (iz i = 0; i < env.len; i++) {
        Str e = env[i];
        Cut n = cut(e, '=');
        if (n.ok && n.head.len && n.tail.len) {
            for (iz k = 0; k < countof(lists); k++) {
                if (n.head == lists[k]) {
                    e = concat(a, concat(a, n.head, "="), towinlist(&c, a, n.tail, 1));
                }
            }
            for (iz k = 0; k < countof(paths); k++) {
                if (n.head == paths[k]) {
                    e = concat(a, concat(a, n.head, "="), towinabs(&c, a, n.tail, 1));
                }
            }
        }
        r = push(a, r, e);
    }
    return envexes(sh, a, r);
}

// The environment for a native program started by an msys shell:
// msysenv's conversions, the drives' directories (=X:) as they are,
// and SYSTEMROOT and WINDIR when missing, as the MSYS2 runtime adds
// them (recorded: MSYSTEM is not added).
static Slice<Str> nativechildenv(Shell *sh, Arena *a, Slice<Str> env)
{
    if (sh->pathmode != PM_MSYS) return env;
    Slice<Str> rest   = {};
    Slice<Str> drives = {};
    for (iz i = 0; i < env.len; i++) {
        if (env[i].len && env[i][0]=='=') {
            drives = push(a, drives, env[i]);
        } else {
            rest = push(a, rest, env[i]);
        }
    }
    PathCtx    c = pathctx(sh, a, {});
    Slice<Str> r = envexes(sh, a, msysenv(&c, a, rest));
    for (iz i = 0; i < drives.len; i++) r = push(a, r, drives[i]);
    Str windir = sh->paths->windir;
    if (windir.len) {
        windir = slashify(a, windir, '\\');
        if (!envvalue(r, "SYSTEMROOT").data) r = push(a, r, concat(a, "SYSTEMROOT=", windir));
        if (!envvalue(r, "WINDIR").data)     r = push(a, r, concat(a, "WINDIR=", windir));
    }
    return r;
}


// Targets

// runprogram's error for a #! interpreter that failed, already
// reported: bash's status 126.
enum : i32 { E_INTERP = 1000 };

// Report "NAME: MSG" with no line number, as bash's sys_error.
static void syserror(Shell *sh, Arena scratch, Str msg)
{
    flushbout(sh);
    skipmsg(&scratch, msg);
    Buf b(&scratch, msg.len+64);
    print(&b, errname(sh));
    print(&b, ": ");
    print(&b, msg);
    putbyte(&b, '\n');
    fdwrite(sh, 2, finish(&b));
}

// What a program needs to start: the image, its command line and
// environment, or why it cannot start. E_NOEXEC means a file to run as
// shell code (no #! line), as bash does after execve fails so.
struct Target {
    Str        path;     // canonical native path of the image
    Str        cmdline;
    Slice<Str> env;
    i32        err;
    Str        interp;   // with err: the #! interpreter that failed
};

// A file to run, by the name the shell has for it: its native path and
// status, with msys mode's .exe suffix rule.
struct Located {
    Str  native;
    Stat st;
    i32  err;
};

static Located locate(Shell *sh, Arena *a, Str path)
{
    Located l = {};
    PathRes r = resolve(sh, a, path.len ? path : Str("."));
    if (!r.ok || r.kind!=PK_FILE) {
        l.err = r.kind==PK_FILE || !r.ok ? E_NOENT : E_ACCES;
        return l;
    }
    l.native = r.native;
    l.st     = os_stat(sh->os, *a, l.native, 1);
    if (l.st.err==E_NOENT && sh->pathmode==PM_MSYS && !hasext(l.native, ".exe")) {
        Str  exe = concat(a, l.native, ".exe");
        Stat x   = os_stat(sh->os, *a, exe, 1);
        if (!x.err && x.type==FT_FILE) {
            l.native = exe;
            l.st     = x;
        }
    }
    if (l.st.err==E_NOENT && sh->pathmode==PM_NATIVE && !knownext(l.native)) {
        // busybox-w32's add_win32_extension, also for explicit paths
        for (iz e = 0; e < countof(winexts); e++) {
            Str  cand = concat(a, l.native, winexts[e]);
            Stat x    = os_stat(sh->os, *a, cand, 1);
            if (!x.err && x.type==FT_FILE) {
                l.native = cand;
                l.st     = x;
                break;
            }
        }
    }
    l.err = l.st.err ? l.st.err : l.st.type==FT_DIR ? E_ACCES : 0;
    return l;
}

// The install root of an image, as pathsetup finds the shell's: its
// directory less usr/bin or bin.
static Str installroot(Str image)
{
    Str root = takehead(image, image.len-lastpart(image).len);
    for (; root.len>3 && root[root.len-1]=='/'; root.len--) {}
    if (root.len>8 && equalfold(taketail(root, 8), "/usr/bin")) {
        root.len -= 8;
    } else if (root.len>4 && equalfold(taketail(root, 4), "/bin")) {
        root.len -= 4;
    }
    return root;
}

// Our own shell as a target: it splits its command line as the persona
// its argv[0] and install give it, Cygwin's way for msys (unabashed.cpp's
// msysargv) and CommandLineToArgvW's otherwise. In msys mode the
// environment goes as to an MSYS2 program, which its import undoes.
static Target selftarget(Shell *sh, Arena *a, Str image, Slice<Str> argv, Slice<Str> env)
{
    Target t = {};
    Stat   f = os_stat(sh->os, *a, concat(a, installroot(image), "/etc/fstab"), 1);
    b32    msys = personaof(argv[0], !f.err) == PM_MSYS;
    t.path    = image;
    t.cmdline = cmdline(a, argv, msys ? SPLIT_CYGWIN : SPLIT_MSVCRT);
    t.env     = msyschildenv(sh, a, env);
    return t;
}

// Is a PE image one of w64devkit's busybox alias stubs: a small native
// program with no CRT beside a busybox.exe? Then the busybox path.
static Str stubbusybox(Shell *sh, Arena *a, Str native, Stat st, Sniff s)
{
    if (s.pe.crt || s.pe.posix || s.pe.ours || s.pe.dll || st.size>((iz)1<<16)) {
        return {};
    }
    Str  bb = concat(a, takehead(native, native.len-lastpart(native).len), "busybox.exe");
    Stat b  = os_stat(sh->os, *a, bb, 1);
    return !b.err && b.type==FT_FILE ? bb : Str{};
}

// Does BB_OVERRIDE_APPLETS ask for an applet to be an external program
// (its name, or "-" for all)? Entries are separated by blanks, commas
// or semicolons, perhaps marked with a leading "+".
static b32 overridden(Slice<Str> env, Str applet)
{
    Str list = envlookup(env, "BB_OVERRIDE_APPLETS", 0);
    for (iz i = 0; i < list.len;) {
        for (; i<list.len && (list[i]==' ' || list[i]==',' || list[i]==';'); i++) {}
        iz beg = i;
        for (; i<list.len && list[i]!=' ' && list[i]!=',' && list[i]!=';'; i++) {}
        Str e = span(list.data+beg, list.data+i);
        if (e.len && e[0]=='+') e = cuthead(e, 1);
        if (e=="-" || (e.len && equalfold(e, applet))) return 1;
    }
    return 0;
}

// A PE image as a target.
static Target imagetarget(Shell *sh, Arena *a, Str native, Stat st, Sniff s,
                          Slice<Str> argv, Slice<Str> env)
{
    Target t = {};
    t.path = native;
    if (s.pe.ours) {
        return selftarget(sh, a, native, argv, env);
    } else if (s.pe.posix) {
        // MSYS2 and Cygwin programs: no conversion, nothing globbed
        t.cmdline = cmdline(a, argv, SPLIT_CYGWIN);
        t.env     = msyschildenv(sh, a, env);
        return t;
    }

    if (sh->pathmode == PM_NATIVE) {
        // A busybox alias stub would run busybox.exe as the applet it
        // is named after: skip it, unless the applet is overridden
        Str applet = lastpart(native);
        applet = hasext(applet, ".exe") ? cuttail(applet, 4) : applet;
        Str bb = stubbusybox(sh, a, native, st, s);
        if (bb.data && !overridden(env, applet)) {
            Slice<Str> v = {};
            v = push(a, v, applet);
            for (iz i = 1; i < argv.len; i++) v = push(a, v, argv[i]);
            t.path    = bb;
            t.cmdline = cmdline(a, v, SPLIT_MSVCRT);
            t.env     = env;
            return t;
        }
        // busybox-w32's libtool hack (w64devkit issue 50)
        if (argv.len>=3 && argv[0]=="cmd" && argv[1]=="//c" && argv[2]=="echo") {
            Slice<Str> v = {};
            for (iz i = 0; i < argv.len; i++) v = push(a, v, i==1 ? Str("/c") : argv[i]);
            argv = v;
        }
        // spawnveq: argv[0] with backslashes (recorded: ./x runs as .\x)
        Slice<Str> v = {};
        for (iz i = 0; i < argv.len; i++) {
            v = push(a, v, i ? argv[i] : slashify(a, argv[i], '\\'));
        }
        t.cmdline = cmdline(a, v, SPLIT_MSVCRT);
        t.env     = env;
        return t;
    }

    // An msys shell's native child, as Git Bash starts it: argv[0] is
    // the image's Windows path, the arguments and the environment go
    // through the path conversion
    Slice<Str> cenv = nativechildenv(sh, a, env);
    Slice<Str> v    = {};
    v = push(a, v, slashify(a, native, '\\'));
    PathCtx    c    = pathctx(sh, a, {});
    Slice<Str> rest = {argv.data+1, argv.len-1, argv.len-1};
    rest = msysargv(&c, a, rest, env);
    for (iz i = 0; i < rest.len; i++) v = push(a, v, rest[i]);
    t.cmdline = cmdline(a, v, SPLIT_MSYS);
    t.env     = cenv;
    return t;
}

// A batch file runs through %ComSpec% /d /e:on /v:off /s /c "...", its
// arguments quoted for cmd.exe as Rust's fix for BatBadBut does, since
// CreateProcess would otherwise start cmd.exe with them unescaped.
static Target batchtarget(Shell *sh, Arena *a, Str native, Slice<Str> argv, Slice<Str> env)
{
    Target t = {};
    Slice<Str> cenv = nativechildenv(sh, a, env);
    if (sh->pathmode == PM_MSYS) {
        PathCtx    c    = pathctx(sh, a, {});
        Slice<Str> rest = {argv.data+1, argv.len-1, argv.len-1};
        rest = msysargv(&c, a, rest, env);
        Slice<Str> v = {};
        v = push(a, v, argv[0]);
        for (iz i = 0; i < rest.len; i++) v = push(a, v, rest[i]);
        argv = v;
    }
    Str comspec = envvalue(cenv, "COMSPEC");
    if (!comspec.len) {
        Str windir = sh->paths->windir.len ? sh->paths->windir : Str("C:/Windows");
        comspec = concat(a, slashify(a, windir, '\\'), "\\System32\\cmd.exe");
    }
    Buf b(a, 256);
    quoteprogram(&b, comspec);
    print(&b, " /d /e:on /v:off /s /c \"");
    quotebatarg(&b, slashify(a, native, '\\'), 1);
    for (iz i = 1; i < argv.len; i++) {
        putbyte(&b, ' ');
        if (!quotebatarg(&b, argv[i], 0)) {
            t.err = E_INVAL;  // cmd.exe cannot carry CR or LF
            return t;
        }
    }
    putbyte(&b, '"');
    t.path    = comspec;
    t.cmdline = finish(&b);
    t.env     = cenv;
    return t;
}

// The two halves of a #! line: the interpreter and the rest of the line
// as one argument, blanks around them dropped.
struct Shebang {
    Str interp;
    Str arg;
};

static Shebang parseshebang(Str line)
{
    Shebang r = {};
    iz i = 0;
    for (; i<line.len && (line[i]==' ' || line[i]=='\t'); i++) {}
    iz beg = i;
    for (; i<line.len && line[i]!=' ' && line[i]!='\t'; i++) {}
    r.interp = span(line.data+beg, line.data+i);
    for (; i<line.len && (line[i]==' ' || line[i]=='\t'); i++) {}
    iz end = line.len;
    for (; end>i && (line[end-1]==' ' || line[end-1]=='\t'); end--) {}
    r.arg = span(line.data+i, line.data+end);
    return r;
}

// Is a #! line one for a POSIX shell, which this shell runs itself?
static b32 selfinterp(Shebang b)
{
    Str i = b.interp;
    if (i=="/bin/sh" || i=="/bin/bash" || i=="/usr/bin/sh" || i=="/usr/bin/bash") {
        return 1;
    }
    return i=="/usr/bin/env" && (b.arg=="sh" || b.arg=="bash");
}

// Where a #! interpreter is: the path, through the mount table in msys
// mode (and native mode's extensions), else its name on PATH. Null if
// nowhere.
static Str findinterp(Shell *sh, Arena *a, Str interp)
{
    Str  last = lastpart(interp);
    Stat st   = pathstat(sh, *a, interp, 1);
    if (!st.err && st.type==FT_FILE) return interp;
    if (sh->pathmode==PM_NATIVE && !knownext(interp)) {
        for (iz e = 0; e < countof(winexts); e++) {
            Str cand = concat(a, interp, winexts[e]);
            st = pathstat(sh, *a, cand, 1);
            if (!st.err && st.type==FT_FILE) return cand;
        }
    }
    Got g = getvar(sh, a, "PATH");
    return last.len && g.set ? pathsearch(sh, a, g.value, last) : Str{};
}

// How to start the program at path (as the shell names it, typed or
// found on PATH) with argv and the shell's exported environment: the
// kind of target decides the image, the command line's quoting and
// conversions, and the environment (notes/m5a.md, step 7). #! lines
// nest up to four deep.
static Target wintarget(Shell *sh, Arena *a, Str path, Str *argv, iz argc, Slice<Str> env)
{
    Target     t = {};
    Slice<Str> v = {};
    for (iz i = 0; i < argc; i++) v = push(a, v, argv[i]);
    if (!v.len) v = push(a, v, path);

    Str file = path;
    for (i32 depth = 0;; depth++) {
        Located l = locate(sh, a, file);
        if (l.err) {
            t.err    = l.err;
            t.interp = depth ? v[0] : Str{};
            return t;
        }
        if (isbatch(l.native)) {
            t = batchtarget(sh, a, l.native, v, env);
            break;
        }

        Sniff s = sniff(sh, *a, l.native, l.st);
        if (s.kind == FK_PE) {
            t = imagetarget(sh, a, l.native, l.st, s, v, env);
            break;
        }
        b32 sh_ = sh->pathmode==PM_NATIVE && hasext(l.native, ".sh");
        if (s.kind!=FK_SCRIPT && !sh_) {
            // No #! line: bash runs it, unless it is an image
            // CreateProcess may know better (MZ but no PE header)
            if (s.kind == FK_MZ) {
                t.path    = l.native;
                t.cmdline = cmdline(a, v, SPLIT_MSVCRT);
                t.env     = env;
                break;
            }
            t.err    = E_NOEXEC;
            t.interp = depth ? v[0] : Str{};
            return t;
        }

        Shebang b = {};
        if (s.kind == FK_SCRIPT) {
            b = parseshebang(s.line);
            if (!b.interp.len) {
                t.err = E_NOEXEC;  // "#!" alone: bash runs it
                return t;
            }
        }
        if (depth == 4) {
            t.err    = E_LOOP;
            t.interp = b.interp;
            return t;
        }

        // The script becomes an argument of its interpreter
        Slice<Str> next = {};
        if (sh_ || selfinterp(b)) {
            // A POSIX shell: this one, in its current path mode
            next = push(a, next, sh->exe);
            next = push(a, next, sh->pathmode==PM_MSYS ? Str("-O") : Str("+O"));
            next = push(a, next, Str("msyspath"));
            if (b.arg.len && b.interp!="/usr/bin/env") next = push(a, next, b.arg);
        } else {
            next = push(a, next, b.interp);
            if (b.arg.len) next = push(a, next, b.arg);
        }
        next = push(a, next, file);
        for (iz i = 1; i < v.len; i++) next = push(a, next, v[i]);
        v = next;

        if (sh_ || selfinterp(b)) {
            t = selftarget(sh, a, sh->exe, v, env);
            break;
        }
        file = findinterp(sh, a, b.interp);
        if (!file.data) {
            t.err    = E_NOENT;
            t.interp = b.interp;
            return t;
        }
    }
    if (!t.err && widelen(t.cmdline)>32766) {
        t.err = E_2BIG;
    }
    return t;
}
