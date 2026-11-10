// Pathname expansion, as bash's glob_filename and glob_vector do it
//
// A pattern is split at its last slash. The directory part, if it holds
// glob characters, is expanded first in the same way, and then the last
// component is matched in each directory found; a component without
// glob characters is looked up with lstat instead of listed. Expanding
// from the right like bash is what gives its results: ** lists every
// name below a directory and enters directories but not links to them,
// a run of ** components counts once, patterns such as **/a/** list
// some names twice, and the order before sorting is bash's (each
// directory's matches in reverse listing order).
//
// One expansion lists each directory at most once, and an entry's type
// comes from its listing when the platform reports it there, so names
// are stat'ed only where bash's answer depends on more than the name: a
// link, a literal component, a trailing slash.
//
// The results then pass GLOBIGNORE and are sorted by GLOBSORT.
//
// This is free and unencumbered software released into the public domain.

enum : i32 {
    GX_ALLDIRS = 1<<0,  // the component is **: every name below the directory
    GX_ADDCUR  = 1<<1,  // with GX_ALLDIRS, the directory itself too
    GX_NULLDIR = 1<<2,  // no directory part precedes the component
    GX_STAR    = 1<<3,  // globstar
    GX_RECURSE = 1<<4,  // expanding a directory part
};

struct GlobName {
    Str name;
    i32 type;  // as lstat would report it, or FT_NONE if not known
};

struct GlobRes {
    Slice<GlobName> v;
    b32             err;  // bash's glob_error_return: nothing usable here
};

struct GlobDir {  // a listed directory, keyed by its path as written
    GlobDir *child[4];
    Str      key;
    Dirent  *list;
    i32      err;
};

struct Glob {
    Shell   *sh;
    Arena   *a;
    i32      pf;    // PM_* flags for names: extglob, case, locale, dots
    GlobDir *dirs;
};

// bash's glob_pattern_p: does a pattern (a whole directory part
// included) hold glob characters? Brackets may span slashes here, and
// the extglob operators count even with extglob off.
static b32 globpatternp(Str pat)
{
    b32 open = 0;
    for (iz i = 0; i < pat.len; i++) {
        switch (pat[i]) {
        case '?':
        case '*':  return 1;
        case '[':  open = 1; break;
        case ']':  if (open) return 1; break;
        case '+':
        case '@':
        case '!':  if (i+1<pat.len && pat[i+1]=='(') return 1; break;
        case '\\': if (++i == pat.len) return 0; break;
        }
    }
    return 0;
}

static b32 isstarstar(Str s)
{
    return s.len>=2 && s[0]=='*' && s[1]=='*' && (s.len==2 || s[2]=='/');
}

// With extglob, the last slash outside extglob groups (bash's
// glob_dirscan), else the last slash; -1 if there is none.
static iz globslash(Str pat, b32 extglob)
{
    iz d = -1;
    if (!extglob) {
        for (iz i = 0; i < pat.len; i++) d = pat[i]=='/' ? i : d;
        return d;
    }
    PatComp k = {};
    k.pat = pat;
    iz se = pat.len - 1;  // bash scans each group up to the last byte
    for (iz p = 0; p < pat.len; p++) {
        if (p+1<pat.len && isextop(pat[p]) && pat[p+1]=='(') {
            if (p+2 >= se) continue;
            iz pe = patscan(&k, p+2, se, 0);
            pe = pe<0 ? se : pe;
            if (pe >= pat.len) break;
            p = pe - 1;
            continue;
        }
        d = pat[p]=='/' ? p : d;
    }
    return d;
}

// bash's sh_makepath as glob_vector uses it: dir, a slash unless it ends
// with one, then name; under GX_NULLDIR a directory "." is left out.
static Str makepath(Arena *a, Str dir, Str name, i32 flags)
{
    if ((flags&GX_NULLDIR) && (dir=="." || dir=="./")) {
        return name;
    }
    return joinpath(a, dir, name);
}

// bash's glob_testdir: 0 for a directory, -2 for a link (only without
// following), -1 otherwise. A known type from a listing saves the stat.
static i32 testdir(Glob *g, Str path, i32 type, b32 nofollow)
{
    if (!path.len) return -1;  // no such file
    if (type == FT_DIR) return 0;
    if (type==FT_LINK && nofollow) return -2;
    if (type!=FT_NONE && type!=FT_LINK) return -1;
    Arena scratch = *g->a;
    Stat  st = pathstat(g->sh, scratch, path, !nofollow);
    if (st.err) return -1;
    return st.type==FT_DIR ? 0 : st.type==FT_LINK ? -2 : -1;
}

static Dirent *addentry(Arena *a, Dirent *list, Str name, i32 type)
{
    for (Dirent *e = list; e; e = e->next) {
        if (equalfold(e->name, name)) return list;
    }
    Dirent *e = alloc<Dirent>(a);
    e->next = list;
    e->name = name;
    e->type = type;
    return e;
}

// The listing of a /proc directory, as Git Bash's (msys2-runtime
// 3.6.9): /proc's fixed entries (procnames) and the processes, of which
// the shell knows only itself; /proc/self, the files the shell serves.
static Dirent *proclist(Shell *sh, Arena *a, PathRes r)
{
    Dirent *list = 0;
    if (!r.num && !r.sub.len) {
        for (iz i = 0; i < countof(procnames); i++) {
            Str n = procnames[i].name;
            list = addentry(a, list, n, proctype(n));
        }
        i64 pids[] = {sh->pid, sh->bashpid};
        for (iz i = 0; i < countof(pids); i++) {
            Buf b(a, 24);
            print(&b, pids[i]);
            list = addentry(a, list, finish(&b), FT_DIR);
        }
    } else if (!r.sub.len && procself(sh, r)) {
        list = addentry(a, list, "exename", FT_FILE);
        list = addentry(a, list, "fd", FT_DIR);
        list = addentry(a, list, "winexename", FT_FILE);
    }
    return list;
}

// os_list for a directory as the shell names it. In msys mode, as
// Cygwin's readdir, a directory also lists the mount points in it, the
// root also dev and proc, /dev holds the devices the shell emulates and
// /proc what proclist has.
static Dirent *pathlist(Shell *sh, Arena *a, Str dir, i32 *err)
{
    if constexpr (LIBMODE) lib_step();  // a long walk ends
    if (sh->pathmode != PM_MSYS) {
        return os_list(sh->os, a, absolute(sh, a, dir), err);
    }
    // Not a scratch copy of *a: the entries go there while these paths
    // are still in use
    PathRes r = resolve(sh, a, dir);
    Str     p = dir.len && dir[0]=='/' ? dir : joinposix(a, sh->wd, dir);
    p = posixnorm(a, slashify(a, p, '/'));
    Dirent *list = 0;
    *err = E_NOENT;
    if (r.ok && r.kind==PK_FILE) {
        list = os_list(sh->os, a, r.native, err);
    } else if (r.ok && r.kind==PK_DEV && !r.sub.len) {
        static Str devs[] = {
            "clipboard", "console", "fd", "full", "null", "random",
            "stderr", "stdin", "stdout", "tty", "urandom", "zero",
        };
        *err = 0;
        for (iz i = 0; i < countof(devs); i++) {
            list = addentry(a, list, devs[i], devs[i]=="fd" ? FT_DIR : FT_CHAR);
        }
        return list;
    } else if (r.ok && r.kind==PK_PROC) {
        *err = 0;
        return proclist(sh, a, r);
    } else if (r.ok && r.kind==PK_CYGDRIVE) {
        *err = 0;
        return 0;
    }
    if (*err) return 0;

    if (p.len>1 && p[p.len-1]=='/') p.len--;
    PathCtx *c = sh->paths;
    for (iz i = 0; i < c->mounts.len; i++) {
        Str m = c->mounts[i].posix;
        if (m.len<2 || m.len<=p.len) continue;
        Str name = lastpart(m);
        Str up   = takehead(m, m.len-name.len);
        if (up.len > 1) up.len--;
        if (up == p) list = addentry(a, list, clone(a, name), FT_DIR);
    }
    if (p == "/") {
        list = addentry(a, list, "dev", FT_DIR);
        list = addentry(a, list, "proc", FT_DIR);
    }
    return list;
}

static void takeossigs(Shell *);
static void sigfatal(Shell *);

static GlobDir *listdir(Glob *g, Str dir)
{
    // A safe point before each listing: an untrapped Ctrl-C ends a long
    // walk (**), as bash's glob checks for interrupts
    takeossigs(g->sh);
    sigfatal(g->sh);
    Str key = dir;
    for (; key.len>1 && key[key.len-1]=='/'; key.len--) {}
    GlobDir **m = &g->dirs;
    for (u64 h = hash(key); *m; h <<= 2) {
        if ((*m)->key == key) return *m;
        m = &(*m)->child[h>>62];
    }
    GlobDir *d = alloc<GlobDir>(g->a);
    d->key  = key;
    d->err  = E_NOENT;
    if (dir.len) {
        d->list = pathlist(g->sh, g->a, dir, &d->err);
    }
    *m = d;
    return d;
}

static Slice<GlobName> reversed(Slice<GlobName> v)
{
    for (iz i = 0, j = v.len-1; i < j; i++, j--) {
        GlobName t = v[i];
        v[i] = v[j];
        v[j] = t;
    }
    return v;
}

// bash's glob_vector: the names in dir that pat selects. Under
// GX_ALLDIRS (pat is **), every name below dir as a path, directories
// entered recursively, and with GX_ADDCUR dir itself ("" under
// GX_NULLDIR). dtype is dir's type when known.
static GlobRes globvector(Glob *g, Str pat, Str dir, i32 dtype, i32 flags)
{
    GlobRes r = {};
    Arena  *a = g->a;
    if (!pat.len) {
        if (testdir(g, dir, dtype, 0) < 0) {
            r.err = 1;
            return r;
        }
        r.v = push(a, r.v, GlobName{"", FT_DIR});
        return r;
    }

    if (!globpatternp(pat)) {
        if (testdir(g, dir, dtype, 0) < 0) {
            r.err = 1;
            return r;
        }
        Str   name = unquoteglob(a, pat);
        Arena scratch = *a;
        Str   path = concat3(&scratch, dir, "/", name);
        Stat  st   = pathstat(g->sh, scratch, path, 0);
        if (!st.err) {
            r.v = push(a, r.v, GlobName{name, st.type});
        }
        return r;
    }

    GlobDir *d = listdir(g, dir);
    if (d->err) {
        r.err = 1;
        return r;
    }

    // bash prepends each name to a list; appending and reversing at the
    // end gives the same order, and a subdirectory's own results, which
    // bash splices in reversed, are appended as they come.
    b32     all  = flags & GX_ALLDIRS;
    PatProg prog = {};
    if (!all) {
        prog = globprog(pat, g->pf, a);
    }
    Dirent dots[2] = {};
    dots[0].name = ".";
    dots[0].type = FT_DIR;
    dots[0].next = dots + 1;
    dots[1].name = "..";
    dots[1].type = FT_DIR;
    dots[1].next = d->list;
    iz n = 0;
    for (Dirent *e = dots; e; e = e->next) {
        if (n++>=2 && dotordotdot(e->name)) continue;  // listed above
        if (skipname(pat, e->name, g->pf, 0)) continue;
        if (all) {
            Str sub = makepath(a, dir, e->name, flags);
            if (!testdir(g, sub, e->type, 1)) {
                GlobRes s = globvector(g, pat, sub, FT_DIR, flags&~GX_ADDCUR);
                if (!s.err) {
                    for (iz i = 0; i < s.v.len; i++) r.v = push(a, r.v, s.v[i]);
                }
            }
            r.v = push(a, r.v, GlobName{sub, e->type});
        } else if (progmatch(&prog, e->name, *a)) {
            r.v = push(a, r.v, GlobName{e->name, e->type});
        }
    }
    if ((flags&GX_ALLDIRS) && (flags&GX_ADDCUR)) {
        Str self = flags&GX_NULLDIR ? Str("") : dir;
        r.v = push(a, r.v, GlobName{self, FT_DIR});
    }
    r.v = reversed(r.v);
    return r;
}

// bash's glob_dir_to_array: prefix each name with dir and a slash.
static Slice<GlobName> dirtoarray(Arena *a, Str dir, Slice<GlobName> v)
{
    if (!dir.len) return v;
    for (iz i = 0; i < v.len; i++) {
        v[i].name = joinpath(a, dir, v[i].name);
    }
    return v;
}

static GlobRes globfile(Glob *g, Str path, i32 flags)
{
    GlobRes r = {};
    Arena  *a = g->a;
    iz slash = globslash(path, g->pf & PM_EXTGLOB);
    Str dir  = takehead(path, slash+1);
    Str file = cuthead(path, slash+1);
    b32 star = flags & GX_STAR;
    b32 filestar = star && file=="**";

    if (dir.len && globpatternp(dir)) {
        // A leading run of ** components is one **, and so is a run
        // ending the directory part
        Str d      = dir;
        i32 dflags = flags;
        b32 allstar = 0;
        if (star && isstarstar(d)) {
            Str p = d;
            while (isstarstar(d) && d.len>2) {
                p = d;
                d = cuthead(d, 3);
                for (; d.len && d[0]=='/'; d = cuthead(d, 1)) {}
                if (!d.len) break;
            }
            allstar = !d.len;
            d = p;
            dflags |= GX_ALLDIRS | GX_ADDCUR;
        }
        iz dlen = d.len;
        b32 laststar = 0;
        if (star && !allstar) {
            iz dl = dlen, prev = dlen;
            while (dl>=4 && d[dl-1]=='/' && d[dl-2]=='*' && d[dl-3]=='*' && d[dl-4]=='/') {
                prev = dl;
                dl  -= 3;
            }
            laststar = dl != dlen;
            dlen = prev;
        }
        if (laststar && dlen>4 && filestar) {
            dlen -= 3;
        }
        d = takehead(d, dlen);
        // A run of slashes as one, else a recursion for each
        while (d.len && d[d.len-1]=='/') d.len--;

        GlobRes dirs = globfile(g, d, dflags|GX_RECURSE);
        if (dirs.err || !dirs.v.len) {
            r.err = 1;
            return r;
        }
        if (allstar && filestar) {
            // **/** is **
            dir = "";
            goto onlyfile;
        }
        for (iz i = 0; i < dirs.v.len; i++) {
            Str dname = dirs.v[i].name;
            i32 dtype = dirs.v[i].type;
            i32 df = flags & ~(GX_ALLDIRS|GX_ADDCUR);
            if (filestar) df |= GX_ALLDIRS | GX_ADDCUR;
            if (!dname.len && file.len) {
                df   |= GX_NULLDIR;
                dname = ".";
                dtype = FT_DIR;
            }
            GlobRes t = {};
            if (allstar && !(df&GX_NULLDIR) && testdir(g, dname, dtype, 1)==-2 &&
                !testdir(g, dname, FT_NONE, 0)) {
                // A link to a directory found by ** is listed, not entered
                if (file.len) continue;
                t.v = push(a, t.v, GlobName{"", FT_LINK});
            } else {
                t = globvector(g, file, dname, dtype, df);
            }
            if (t.err) continue;

            b32 stop = 0;
            Slice<GlobName> v = t.v;
            if ((df&GX_ALLDIRS) && isstarstar(file)) {
                // The paths are complete already. From a null directory
                // part, ** has listed everything: drop the placeholders
                // and stop, so nothing is listed twice.
                if ((df&GX_NULLDIR) && !(flags&GX_NULLDIR) && v.len && !v[0].name.len) {
                    iz n = 0;
                    for (; n<v.len && !v[n].name.len; n++) {}
                    v = {v.data+n, v.len-n, v.len-n};
                    stop = 1;
                }
            } else {
                v = dirtoarray(a, dirs.v[i].name, v);
            }
            for (iz j = 0; j < v.len; j++) r.v = push(a, r.v, v[j]);
            if (stop) break;
        }
        return r;
    }

    onlyfile:
    if (!file.len) {
        r.v = push(a, r.v, GlobName{dir, FT_NONE});
        return r;
    }
    dir = unquoteglob(a, dir);
    i32 df = flags;
    if (!dir.len) df |= GX_NULLDIR;
    if (filestar) {
        df |= GX_ALLDIRS | GX_ADDCUR;
        if (!dir.len && !(flags&GX_ALLDIRS)) df &= ~GX_ADDCUR;
    }
    GlobRes t = globvector(g, file, dir.len ? dir : Str("."), dir.len ? FT_NONE : FT_DIR, df);
    if (t.err) return t;
    t.v = dirtoarray(a, df&GX_ALLDIRS ? Str("") : dir, t.v);
    return t;
}


// GLOBIGNORE

// The end of the GLOBIGNORE pattern starting at i: the next colon not
// quoted, in a bracket expression, an extglob group, or what looks like
// an expansion or substitution (bash's skip_to_delim: the patterns
// are never expanded, but `$(x:y)` stays one).
static iz ignoreend(Str s, iz i, b32 extglob)
{
    while (i < s.len) {
        u8 c = s[i];
        if (c == '\\') {
            i += 2;
        } else if (c == ':') {
            return i;
        } else if (c == '`') {
            for (i++; i<s.len && s[i]!='`'; i++) {
                if (s[i] == '\\') i++;
            }
            i++;
        } else if ((c=='$' || c=='<' || c=='>') && i+1<s.len &&
                   (s[i+1]=='(' || (c=='$' && (s[i+1]=='{' || s[i+1]=='[')))) {
            // To the matching close, counting nested opens
            u8  open  = s[i+1];
            u8  close = open=='(' ? ')' : open=='{' ? '}' : ']';
            i32 depth = 1;
            for (i += 2; i<s.len && depth; i++) {
                if (s[i] == '\\') {
                    i++;
                } else if (s[i]=='\'' || s[i]=='"') {
                    u8 q = s[i];
                    for (i++; i<s.len && s[i]!=q; i++) {
                        if (q=='"' && s[i]=='\\') i++;
                    }
                } else if (s[i] == open) {
                    depth++;
                } else if (s[i] == close) {
                    depth--;
                }
            }
        } else if (c=='\'' || c=='"') {
            for (i++; i<s.len && s[i]!=c; i++) {
                if (c=='"' && s[i]=='\\') i++;
            }
            i++;
        } else if (c=='[' || (extglob && isextop(c) && i+1<s.len && s[i+1]=='(')) {
            // To the matching close, counting nested opens
            u8  open  = c=='[' ? '[' : '(';
            u8  close = c=='[' ? ']' : ')';
            i32 depth = 1;
            for (i += c=='[' ? 1 : 2; i<s.len && depth; i++) {
                if (s[i] == '\\') {
                    i++;
                } else if (s[i] == open) {
                    depth++;
                } else if (s[i] == close) {
                    depth--;
                }
            }
        } else {
            i++;
        }
    }
    return s.len;
}

// Does GLOBIGNORE (ign, non-empty) remove this match? Its patterns
// match the whole path, and . and .. as the last component always go.
static b32 globignored(Shell *sh, Str path, Str ign, Arena scratch)
{
    Str base = path;
    for (iz i = path.len-1; i >= 0; i--) {
        if (path[i] == '/') {
            if (i+1 < path.len) base = cuthead(path, i+1);
            break;
        }
    }
    if (dotordotdot(base)) return 1;
    i32 pf = PM_PATHNAME | patflags(sh);
    if (sh->shopt[S_NOCASEGLOB]) pf |= PM_NOCASE;
    b32 extglob = sh->shopt[S_EXTGLOB];
    for (iz i = 0; i < ign.len;) {
        iz  end = ignoreend(ign, i, extglob);
        end = end<ign.len ? end : ign.len;
        Str pat = span(ign.data+i, ign.data+end);
        if (pat.len && patmatch(pat, path, pf, scratch)) return 1;
        i = end + 1;
    }
    return 0;
}


// GLOBSORT

enum : i32 {
    GS_NONE, GS_NAME, GS_SIZE, GS_MTIME, GS_ATIME, GS_CTIME, GS_BLOCKS,
    GS_NUMERIC, GS_NOSORT,
    GS_REVERSE = 16,
};

static i32 globsort(Shell *sh)
{
    Arena none = {};
    Str   v    = getvar(sh, &none, "GLOBSORT").value;
    if (!v.len) return GS_NONE;
    for (; v.len && (v[0]==' ' || v[0]=='\t'); v = cuthead(v, 1)) {}
    i32 rev = 0;
    if (v.len && v[0]=='+') {
        v = cuthead(v, 1);
    } else if (v.len && v[0]=='-') {
        rev = GS_REVERSE;
        v   = cuthead(v, 1);
    }
    if (!v.len) return GS_NAME | rev;
    static Str names[] = {
        "", "name", "size", "mtime", "atime", "ctime", "blocks", "numeric", "nosort",
    };
    for (i32 i = 1; i < countof(names); i++) {
        if (v == names[i]) return i | rev;
    }
    return GS_NONE;  // any other value sorts by name, ascending
}

struct GlobKey {
    Str  name;
    i64  key;
    i32  how;    // GS_*
    b32  rev;
    b32  num;    // GS_NUMERIC: the name is all digits
};

static i32 cmpi64(i64 a, i64 b)
{
    return (a > b) - (a < b);
}

static i32 globcmp(GlobKey *a, GlobKey *b)
{
    GlobKey *x = a->rev ? b : a;
    GlobKey *y = a->rev ? a : b;
    i32 c = 0;
    switch (a->how) {
    case GS_NONE:
    case GS_NAME:
        break;
    case GS_NUMERIC:
        if (x->num && y->num) return cmpi64(x->key, y->key);
        if (x->num != y->num) return x->num ? -1 : 1;
        break;
    default:
        c = cmpi64(x->key, y->key);
    }
    return c ? c : compare(x->name, y->name);
}

static b32 globless(GlobKey *a, GlobKey *b)
{
    return globcmp(a, b) < 0;
}

// Sort the matches in place as GLOBSORT says: by name (the default),
// by an attribute that stat reports with the name breaking ties, by
// numeric value for names of all digits, or not at all.
static void sortglob(Shell *sh, Arena scratch, Slice<Str> m)
{
    i32 how = globsort(sh);
    b32 rev = how & GS_REVERSE;
    how &= ~GS_REVERSE;
    if (how==GS_NOSORT || m.len<2) return;
    GlobKey *k = alloc<GlobKey>(&scratch, m.len);
    for (iz i = 0; i < m.len; i++) {
        k[i].name = m[i];
        k[i].how  = how;
        k[i].rev  = rev;
        if (how == GS_NUMERIC) {
            b32 num = m[i].len > 0;
            i64 v   = 0;
            for (iz j = 0; j < m[i].len; j++) {
                i32 dg = m[i][j] - '0';
                if (dg<0 || dg>9) {
                    num = 0;
                    break;
                }
                v = v>(0x7fffffffffffffff-dg)/10 ? 0x7fffffffffffffff : v*10+dg;
            }
            k[i].num = num;
            k[i].key = v;
        } else if (how != GS_NAME && how != GS_NONE) {
            Arena t  = scratch;
            Stat  st = pathstat(sh, t, m[i], 1);
            i64   v  = -1;
            if (!st.err) {
                switch (how) {
                case GS_SIZE:   v = st.size;   break;
                case GS_MTIME:  v = st.mtime;  break;
                case GS_ATIME:  v = st.atime;  break;
                case GS_CTIME:  v = st.ctime;  break;
                case GS_BLOCKS: v = st.blocks; break;
                }
            }
            k[i].key = v;
        }
    }
    sort(k, m.len, globless, scratch);
    for (iz i = 0; i < m.len; i++) m[i] = k[i].name;
}


// Expand the pattern (with quoted characters backslash-escaped): the
// matches after GLOBIGNORE, sorted, or none.
static Slice<Str> glob(Shell *sh, Arena *a, Str pat)
{
    Glob g = {};
    g.sh = sh;
    g.a  = a;
    g.pf = patflags(sh);
    if (sh->shopt[S_NOCASEGLOB])   g.pf |= PM_NOCASE;
    if (!sh->shopt[S_DOTGLOB])     g.pf |= PM_PERIOD;
    if (sh->shopt[S_GLOBSKIPDOTS]) g.pf |= PM_SKIPDOTS;
    GlobRes r = globfile(&g, pat, sh->shopt[S_GLOBSTAR] ? GX_STAR : 0);

    Slice<Str> out = {};
    if (r.err) return out;
    Str ign = getvar(sh, a, "GLOBIGNORE").value;
    for (iz i = 0; i < r.v.len; i++) {
        if (!ign.len || !globignored(sh, r.v[i].name, ign, *a)) {
            out = push(a, out, r.v[i].name);
        }
    }
    sortglob(sh, *a, out);
    return out;
}
