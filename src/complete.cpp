// Completion: readline's engine with bash's word classifier, generators
// and quoting (notes/research/completion.md, cited as §N). Programmable
// completion is M8.
// This is free and unencumbered software released into the public domain.


enum : u8 { CQ_BACKSLASH = 1, CQ_SINGLE = 2, CQ_DOUBLE = 4 };

// readline's completion modes (§2.2)
enum : u8 { CM_TAB = '\t', CM_LIST = '?', CM_ALL = '*', CM_AMBIG = '!', CM_UNMOD = '@' };
enum : u8 { CM_BRACE = '{' };  // bash's complete-into-braces

// What to drop from file name matches (§6.1)
enum : i32 { IG_FIGNORE, IG_DIRS, IG_ALL, IG_NONE };

// Generators
enum : i32 {
    CX_SHELL,     // bash's classifier (§4)
    CX_FILE,      // M-/
    CX_COMMAND,   // M-!
    CX_VARIABLE,  // M-$
    CX_USER,      // M-~
    CX_HOST,      // M-@
    CX_HISTORY,   // M-TAB
    CX_GLOB,      // M-g: the word with * appended
    CX_GLOBX,     // C-x * and C-x g: as typed, * appended by an argument
    CX_DABBREV,   // dabbrev-expand: history words, newest first
};

// One attempt
struct Comp {
    Arena     *a;
    Str        line;      // the line up to point
    iz         start;     // the word: line[start, end)
    iz         end;
    Str        word;
    u8         quote;     // the open quote the word is in, or 0
    u8         found;     // CQ_*: quoting anywhere before point
    b32        files;     // the matches are file names
    b32        cmdstat;   // a name without a slash is found through PATH
    b32        noappend;
    b32        noquote;   // no closing quote appended
    b32        noqfn;     // no quoting of the matches
    u8         append;    // ' ', '/' or 0
    i32        ignore;    // IG_*
    b32        dirhook;   // expand $ and ` in a directory to open it
    Str        udir;      // the directory as the user wrote it
    Str        odir;      // and as opened
    Slice<Str> m;         // the matches, sorted and distinct
    Str        lcd;       // what replaces the word
    b32        dotinpath;
    Str        breaks;    // the word breaks used
    u8         mode;      // CM_*
    b32        partial;   // not bash's full quoting
    b32        nosort;    // complete -o nosort: generation order
    b32        fullquote; // -o fullquote: quoted as file names are
    b32        nofiles;   // no file name completion as the last resort
};

static Str compdequote(Arena *a, Str s, u8 quote)
{
    Buf b(a, s.len);
    u8 q = quote;
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c=='\\' && q!='\'') {
            if (i+1 == s.len) break;
            u8 n = s[i+1];
            if (q=='"' && n!='$' && n!='`' && n!='"' && n!='\\' && n!='\n') {
                putbyte(&b, c);
            }
            putbyte(&b, n);
            i++;
        } else if (q && c==q) {
            q = 0;
        } else if (!q && (c=='\'' || c=='"')) {
            q = c;
        } else {
            putbyte(&b, c);
        }
    }
    return finish(&b);
}

static b32 compin(Str set, u8 c)
{
    for (iz i = 0; i < set.len; i++) {
        if (set[i] == c) return 1;
    }
    return 0;
}

static u8 complower(u8 c)
{
    return c>='A' && c<='Z' ? c+32 : c;
}

// Do the first len bytes of a and b match, with the case and map-case
// rules (§5.2)?
static b32 compeq(Editor *ed, Str a, Str b, iz len)
{
    if (a.len<len || b.len<len) return 0;
    for (iz i = 0; i < len; i++) {
        u8 x = a[i], y = b[i];
        if (x == y) continue;
        if (!ed->vars.ignorecase) return 0;
        x = complower(x);
        y = complower(y);
        if (ed->vars.mapcase) {
            x = x=='_' ? '-' : x;
            y = y=='_' ? '-' : y;
        }
        if (x != y) return 0;
    }
    return 1;
}

static Str compbreaks(Shell *sh, Arena *a)
{
    Got g = getvar(sh, a, "COMP_WORDBREAKS");
    if (g.set) return g.value;
    return sh->shopt[S_HOSTCOMPLETE] ? Str(" \t\n\"'@><=;|&(:")
                                     : Str(" \t\n\"'><=;|&(:");
}

// Find the word before point (§3)
static void compword(Comp *c, Str breaks, b32 atprefix)
{
    Str line = c->line;
    iz  end  = line.len;
    u8  q    = 0;
    iz  qpos = 0;
    for (iz i = 0; i < end; i++) {
        u8 ch = line[i];
        if (ch=='\\' && q!='\'') {
            c->found |= CQ_BACKSLASH;
            i++;
        } else if (q) {
            if (ch == q) q = 0;
        } else if (ch=='\'' || ch=='"') {
            q    = ch;
            qpos = i;
            c->found |= ch=='\'' ? CQ_SINGLE : CQ_DOUBLE;
        }
    }
    if (q) {
        c->start = qpos + 1;
        c->quote = q;
    } else {
        iz i = end;
        for (; i > 0; i--) {
            u8 ch = line[i-1];
            if (compin(breaks, ch) && !(c->found && edquoted(line, i-1))) break;
        }
        c->start = i;
        if (i>0 && atprefix && line[i-1]=='@') c->start = i - 1;
    }
    c->end  = end;
    c->word = cuthead(line, c->start);
}

// The word with its quoting removed, when there is any
static Str compbare(Comp *c)
{
    return c->found||c->quote ? compdequote(c->a, c->word, c->quote) : c->word;
}


// Bash's classifier (§4)

static b32 compsep(u8 c)
{
    return c==';' || c=='|' || c=='&' || c=='{' || c=='(' || c=='`';
}

static b32 compassign(Str w)
{
    Cut e = cut(w, '=');
    return e.ok && e.head.len && validvarname(e.head);
}

// Is the word in command position? Sets *invalid for a ( that cannot
// start one (§4.1, §4.2).
static b32 compcmdpos(Shell *sh, Comp *c, b32 ps1, b32 *invalid)
{
    Str line = c->line;
    iz  ti   = c->start - 1;
    while (ti>=0 && (line[ti]==' ' || line[ti]=='\t')) ti--;
    if (ti>=0 && (line[ti]=='"' || line[ti]=='\'')) {
        ti--;
        while (ti>=0 && (line[ti]==' ' || line[ti]=='\t')) ti--;
    }
    if (ti < 0) return ps1;
    u8   ch     = line[ti];
    u8   prev   = ti>0 ? line[ti-1] : 0;
    b32  quoted = edquoted(line, ti);
    if (!quoted && compsep(ch)) {
        if (ch=='(' && sh->shopt[S_EXTGLOB] && prev && compin("?*+@!", prev)) {
            return 0;
        }
        if (ch=='&' && (prev=='<' || prev=='>')) return 0;
        if (ch=='|' && prev=='>') return 0;
        if (ch=='{' && prev=='$') return 0;
        if (ch=='(' && prev && !compin(" \t;|&(`{$<>", prev)) {
            *invalid = 1;
            return 0;
        }
        if (ch == '`') {
            iz n = 0;
            for (iz i = 0; i < line.len; i++) {
                n += line[i]=='`' && !edquoted(line, i);
            }
            if (!(n & 1)) return 0;
        }
        return 1;
    }
    if (!ps1) return 0;

    // Assignments before the command word
    if (c->start==0 || (line[c->start-1]!=' ' && line[c->start-1]!='\t')) {
        return 0;
    }
    iz s = 0;
    for (iz i = 0; i < c->start; i++) {
        u8 x = line[i];
        if (compsep(x) && !edquoted(line, i) && !(x=='|' && i>0 && line[i-1]=='>')) {
            s = i + 1;
        }
    }
    iz nwords = 0;
    for (iz i = s; i < c->start;) {
        while (i<c->start && (line[i]==' ' || line[i]=='\t')) i++;
        if (i == c->start) break;
        iz b = i;
        while (i<c->start && line[i]!=' ' && line[i]!='\t') {
            i = edshellstep(line, i);
        }
        if (!compassign(span(line.data+b, line.data+(i<c->start ? i : c->start)))) {
            return 0;
        }
        nwords++;
    }
    return nwords > 0;
}


// Generators

static b32 compexists(Shell *sh, Arena a, Str path)
{
    return !pathstat(sh, a, path, 0).err;
}

// Expand a directory as bash's directory hook does: $ and ` words that
// do not exist literally, as a prompt string would be (no command
// substitution); null if expansion fails.
static Str compexpdir(Shell *sh, Arena *a, Str dir)
{
    // Parameters only: never a command substitution, nor ${ cmd; } and
    // ${| cmd; }, which run in the shell itself
    if (!compin(dir, '$') || compin(dir, '`')) return dir;
    for (iz i = 0; i+1 < dir.len; i++) {
        if (dir[i] != '$') continue;
        if (dir[i+1] == '(') return dir;
        if (dir[i+1]=='{' && i+2<dir.len && compin(" \t\n|", dir[i+2])) return dir;
    }
    if (compexists(sh, *a, dir)) return dir;
    Escape e = {};
    e.up    = sh->esc;
    e.bout  = sh->bout;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    i32 status = sh->status;
    Str r = {};
    if (!jmpset(e.jmp)) {
        r = expandstring(sh, a, dir, XC_NOPSUB);
    } else {
        unwind(sh, e.undo);
        r = dir;
    }
    sh->esc    = e.up;
    sh->status = status;
    return r;
}

static Slice<Str> compadd(Comp *c, Slice<Str> m, Str s)
{
    return push(c->a, m, s);
}

// File names: the entries of the word's directory that start with its
// last part (§5). keep: 1 for every match, 2 for executables and
// directories only (a command with a slash).
// User names from the password file, as readline's: a ~name for each
// when the word has its ~ (M-~ takes a bare word too). Their homes,
// being files, are marked and appended to as files are.
static Slice<Str> compusers(Shell *sh, Comp *c, Str w)
{
    Arena *a = c->a;
    c->files = 1;
    c->udir  = c->odir = {};
    Str pre = w.len && w[0]=='~' ? Str("~") : Str("");
    Str nw  = cuthead(w, pre.len);
    Slice<Str> m = {};
    if (sh->win32) return m;  // no password file
    i32 err = 0;
    Str pw  = readwholefile(sh, a, "/etc/passwd", &err);
    for (Cut l = {{}, pw, 1}; l.ok;) {
        l = cut(l.tail, '\n');
        Str name = cut(l.head, ':').head;
        if (name.len && startswith(name, nw)) m = compadd(c, m, concat(a, pre, name));
    }
    return m;
}

static Slice<Str> compfiles(Editor *ed, Shell *sh, Comp *c, Str w, i32 keep)
{
    Arena *a = c->a;
    Slice<Str> m = {};
    iz slash = -1;
    for (iz i = 0; i < w.len; i++) {
        if (w[i] == '/') slash = i;
    }
    if (slash<0 && w.len && w[0]=='~') {
        return compusers(sh, c, w);  // ~name: a user's home
    }
    Str udir = takehead(w, slash+1);
    Str part = cuthead(w, slash+1);
    Str odir = udir.len ? udir : Str(".");
    if (odir[0] == '~') {
        odir = tildefilename(sh, a, odir);
        if (ed->vars.expandtilde) {
            // expand-tilde: the matches carry the expanded directory
            b32 slash = odir.len && odir[odir.len-1]=='/';
            udir = slash ? odir : concat(a, odir, "/");
        }
    }
    if (c->dirhook) {
        odir = compexpdir(sh, a, odir);
        if (sh->shopt[S_DIREXPAND] && udir.len) udir = odir;
    }
    if (c->dirhook && !sh->opt[O_PHYSICAL] && !isabsolute(sh, odir)) {
        // As bash's hook, a relative directory is read as cd would take
        // it, against $PWD: ../ under a symbolic link is its parent
        Got pwd = getvar(sh, a, "PWD");
        Str abs = joinpath(a, pwd.set && pwd.value.len ? pwd.value : sh->wd, odir);
        Str can = cdcanon(sh, a, abs);
        Stat st = pathstat(sh, *a, can, 1);
        if (!st.err && st.type==FT_DIR) odir = can;
    }
    if (c->dirhook && sh->shopt[S_DIREXPAND] && sh->shopt[S_DIRSPELL] &&
            udir.len && part.len && !compexists(sh, *a, odir)) {
        // As bash's directory hook, a misspelled directory is corrected
        // as cd's cdspell does, then made absolute and canonical
        Got pwd = getvar(sh, a, "PWD");
        Str abs = isabsolute(sh, odir) ? odir
                : joinpath(a, pwd.set ? pwd.value : sh->wd, odir);
        Str fix = spellpath(sh, a, abs);
        if (fix.data) {
            fix  = cdcanon(sh, a, fix);
            fix  = concat(a, fix, fix.len && fix[fix.len-1]=='/' ? Str("") : Str("/"));
            odir = udir = fix;
        }
    }
    c->udir  = udir;
    c->odir  = odir;
    c->files = 1;

    i32     err  = 0;
    Dirent *list = pathlist(sh, a, odir, &err);
    b32 dot = 0, dotdot = 0;
    for (Dirent *d = list; d; d = d->next) {
        dot    |= d->name == ".";
        dotdot |= d->name == "..";
    }
    if (!err && part.len) {
        // As readdir, . and .. are entries
        if (!dotdot) list = addentry(a, list, "..", FT_DIR);
        if (!dot)    list = addentry(a, list, ".",  FT_DIR);
    }
    for (Dirent *d = list; d; d = d->next) {
        Str n = d->name;
        if (!part.len) {
            if (n=="." || n=="..") continue;
            if (n.len && n[0]=='.' && !ed->vars.matchhidden) continue;
        } else if (!compeq(ed, n, part, part.len)) {
            continue;
        }
        Str r = concat(a, udir, n);
        if (keep == 2) {
            Str  p  = joinpath(a, odir, n);
            Stat st = pathstat(sh, *a, p, 1);
            if (st.err || (st.type!=FT_DIR && !canexec(sh, st))) continue;
        }
        m = compadd(c, m, r);
    }
    return m;
}

// completion_strip_exe, as Cygwin's bash: a command match's .exe goes,
// unless a file of the name without it exists beside it. path is the
// match as the file system knows it.
static Str compexe(Shell *sh, Arena a, Str path, Str n)
{
    if (!sh->shopt[S_COMPLETION_STRIP_EXE] || n.len<=4 || path.len<=4) return n;
    Str t = cuthead(n, n.len-4);
    if (t[0]!='.' || complower(t[1])!='e' || complower(t[2])!='x' || complower(t[3])!='e') {
        return n;
    }
    Str bare = takehead(path, path.len-4);
    // The name exactly, without msys's implied .exe
    Str full = absolute(sh, &a, bare);
    if (os_stat(sh->os, a, full, 0).err) n.len -= 4;
    return n;
}

// Command words (§4.8)
static Slice<Str> compcommands(Editor *ed, Shell *sh, Comp *c, Str w)
{
    Arena *a = c->a;
    if (compin(w, '/')) {
        if (w.len && w[0]=='~') {
            // The user's spelling is kept: complete the expanded name and
            // put the prefix back
            Str t = tildefilename(sh, a, w);
            Slice<Str> m = compfiles(ed, sh, c, t, 2);
            Str pre  = cut(w, '/').head;
            Str tpre = takehead(t, t.len - (w.len - pre.len));
            for (iz i = 0; i < m.len; i++) {
                m[i] = concat(a, pre, cuthead(m[i], tpre.len));
            }
            c->udir = concat(a, pre, cuthead(c->udir, tpre.len));
            for (iz i = 0; i < m.len; i++) {
                Str path = joinpath(a, c->odir, cuthead(m[i], c->udir.len));
                m[i] = compexe(sh, *a, path, m[i]);
            }
            return m;
        }
        Slice<Str> m = compfiles(ed, sh, c, w, 2);
        for (iz i = 0; i < m.len; i++) {
            Str path = joinpath(a, c->odir, cuthead(m[i], c->udir.len));
            m[i] = compexe(sh, *a, path, m[i]);
        }
        return m;
    }
    c->files   = 1;
    c->cmdstat = 1;
    Slice<Str> m = {};
    Slice<Str> t = {};
    compaliases(sh->aliases, a, &t);
    for (iz i = 0; i < t.len; i++) {
        if (compeq(ed, t[i], w, w.len)) m = compadd(c, m, t[i]);
    }
    for (iz i = 0; i < countof(reserved); i++) {
        if (startswith(reserved[i].word, w)) m = compadd(c, m, reserved[i].word);
    }
    t = {};
    compfuncs(sh->funcs, a, &t);
    for (iz i = 0; i < t.len; i++) {
        if (compeq(ed, t[i], w, w.len)) m = compadd(c, m, t[i]);
    }
    t = {};
    compbuiltins(sh, a, &t);
    for (iz i = 0; i < t.len; i++) {
        if (startswith(t[i], w)) m = compadd(c, m, t[i]);
    }
    if (w.len && w[0]!='~') {
        Stat st = pathstat(sh, *a, w, 1);
        if (!st.err && st.type==FT_DIR) m = compadd(c, m, w);
    }
    Str path = getvar(sh, a, "PATH").value;
    u8  sep  = listsep(sh);
    for (Str rest = path; rest.data;) {
        Cut d   = cut(rest, sep);
        Str dir = d.head.len ? d.head : Str(".");
        rest = d.ok ? d.tail : Str{};
        if (dir == ".") c->dotinpath = 1;
        if (dir[0] == '~') dir = tildefilename(sh, a, dir);
        i32 err = 0;
        for (Dirent *e = pathlist(sh, a, dir, &err); e; e = e->next) {
            if (e->type == FT_DIR) continue;
            if (!compeq(ed, e->name, w, w.len)) continue;
            Str path = joinpath(a, dir, e->name);
            Stat st = pathstat(sh, *a, path, 1);
            if (st.err || st.type==FT_DIR || !canexec(sh, st)) continue;
            Str path2 = joinpath(a, dir, e->name);
            m = compadd(c, m, compexe(sh, *a, path2, e->name));
        }
    }
    return m;
}

// Variables: $NAME or ${NAME} (§4.5)
static Slice<Str> compvariables(Shell *sh, Comp *c, Str w)
{
    Arena *a = c->a;
    Str pre = w.len && w[0]=='$' ? Str("$") : Str("");
    Str suf = {};
    Str nw  = cuthead(w, pre.len);
    if (nw.len && nw[0]=='{') {
        pre = concat(a, pre, "{");
        suf = "}";
        nw  = cuthead(nw, 1);
    }
    Slice<Str> t = {}, m = {};
    compvars(sh->vars, 0, a, &t);
    for (iz i = 0; i < t.len; i++) {
        if (startswith(t[i], nw)) m = compadd(c, m, concat3(a, pre, t[i], suf));
    }
    if (m.len == 1) {
        Str  v  = compexpdir(sh, a, m[0]);
        Stat st = pathstat(sh, *a, v, 1);
        if (!st.err && st.type==FT_DIR) c->append = '/';
    }
    return m;
}

static b32 hostblank(u8 c)
{
    return c==' ' || c=='\t' || c=='\r';
}

// Host names from one hosts file, as bash reads it: in pieces of at
// most 254 bytes (its fgets buffer), each a line of its own; # starts a
// comment only where a word would; a leading address (a word starting
// with a digit) is skipped; "$include FILE" reads FILE in its place.
static void hostfile(Shell *sh, Arena *a, Buf *names, Str file, i32 depth)
{
    i32    err  = 0;
    Str    text = readwholefile(sh, a, file, &err);
    for (Str rest = text; rest.len;) {
        Cut l = cut(rest, '\n');
        Str s = l.head;
        if (s.len > 254) {
            s    = takehead(rest, 254);
            rest = cuthead(rest, 254);
        } else {
            rest = l.tail;
        }
        iz i = 0;
        while (i<s.len && hostblank(s[i])) i++;
        if (i==s.len || s[i]=='#') continue;
        if (startswith(cuthead(s, i), "$include ")) {
            iz b = i + 9;
            while (b<s.len && (s[b]==' ' || s[b]=='\t')) b++;
            iz e = b;
            while (e<s.len && !hostblank(s[e])) e++;
            if (depth < 32) {
                Str inc = span(s.data+b, s.data+e);
                hostfile(sh, a, names, inc, depth+1);
            }
            continue;
        }
        b32 addr = s[i]>='0' && s[i]<='9';
        while (i < s.len) {
            while (i<s.len && hostblank(s[i])) i++;
            if (i==s.len || s[i]=='#') break;
            iz b = i;
            while (i<s.len && !hostblank(s[i])) i++;
            Str name = span(s.data+b, s.data+i);
            if (addr) {
                addr = 0;
                continue;
            }
            print(names, name);
            putbyte(names, '\n');
        }
    }
}

// Host names from HOSTFILE (else hostname_completion_file, else
// /etc/hosts), read once there are any, and again, adding to them,
// after HOSTFILE is assigned (§4.7)
static Slice<Str> comphosts(Shell *sh, Comp *c, Str w)
{
    Arena *a = c->a;
    if (!sh->hostsread) {
        Got g = getvar(sh, a, "HOSTFILE");
        if (!g.set) g = getvar(sh, a, "hostname_completion_file");
        Str file = g.set ? g.value : Str("/etc/hosts");
        Buf names(a, 256);
        print(&names, sh->hosts);
        hostfile(sh, a, &names, file, 0);
        Str all = finish(&names);
        if (all.len > sh->hosts.len) {
            sfreestr(sh->store, sh->hosts);
            sh->hosts = sclone(sh->store, all);
        }
        sh->hostsread = sh->hosts.len > 0;
    }
    Str nw = cuthead(w, w.len && w[0]=='@');
    Slice<Str> m = {};
    for (Str rest = sh->hosts; rest.len;) {
        Cut l = cut(rest, '\n');
        rest = l.tail;
        if (startswith(l.head, nw)) m = compadd(c, m, concat(a, "@", l.head));
    }
    return m;
}


// Every word of every history line (M-TAB), oldest or newest line first
static Slice<Str> comphistory(Shell *sh, Comp *c, Str w, b32 newest)
{
    Slice<Str> m = {};
    for (iz n = 0; n < sh->hist.len; n++) {
        iz i = newest ? sh->hist.len-1-n : n;
        HWords hw = histwords(c->a, sh->hist.ents[i]->line, 0, -1);
        for (iz k = 0; k < hw.len; k++) {
            if (startswith(hw.word[k], w)) m = compadd(c, m, hw.word[k]);
        }
    }
    return m;
}

static b32 compglobchars(Str w)
{
    for (iz i = 0; i < w.len; i++) {
        u8 ch = w[i];
        if (ch == '\\') {
            i++;
        } else if (ch=='*' || ch=='?' || ch=='[') {
            return 1;
        }
    }
    return 0;
}

static Slice<Str> compglob(Shell *sh, Comp *c, Str pat)
{
    Slice<Str> g = glob(sh, c->a, pat);
    c->files = 1;
    Slice<Str> m = {};
    for (iz i = 0; i < g.len; i++) m = compadd(c, m, g[i]);
    return m;
}

static Slice<Str> pcomplete(Editor *, Shell *, Comp *, b32 *, i32 *);  // pcomplete.cpp

// Choose and run the generator (§4.3)
static Slice<Str> compgenerate(Editor *ed, Shell *sh, Comp *c, i32 gen)
{
    Str w = compbare(c);
    c->dirhook = 1;
    switch (gen) {
    case CX_FILE:
        c->dirhook = 0;
        return compfiles(ed, sh, c, w, 1);
    case CX_COMMAND:  return compcommands(ed, sh, c, w);
    case CX_VARIABLE: return compvariables(sh, c, w);
    case CX_USER:     return compusers(sh, c, w);
    case CX_HOST:     return comphosts(sh, c, w);
    case CX_HISTORY:  return comphistory(sh, c, w, 0);
    case CX_DABBREV:
        c->nosort   = 1;  // adjacent duplicates only are dropped
        c->noappend = 1;
        return comphistory(sh, c, w, 1);
    case CX_GLOB:     return compglob(sh, c, concat(c->a, c->word, "*"));
    case CX_GLOBX:
        return compglob(sh, c, ed->argset ? concat(c->a, c->word, "*") : c->word);
    }

    b32 invalid = 0;
    b32 cmdpos  = ed->shellline && compcmdpos(sh, c, ed->ps1, &invalid);
    if (invalid) return {};
    Str raw = c->word;
    if (raw.len && raw[0]=='`' && c->quote!='\'') {
        // A command substitution: the rest as a command word
        Str rest = cuthead(raw, 1);
        Comp t = *c;
        t.word  = rest;
        t.start = c->start + 1;
        Slice<Str> m = compcommands(ed, sh, &t, rest);
        if (m.len) {
            for (iz i = 0; i < m.len; i++) m[i] = concat(c->a, "`", m[i]);
            c->files    = 0;
            c->noquote  = 1;
            c->noappend = !(m.len==1 && t.files && pathstat(sh, *c->a, cuthead(m[0], 1), 1).type==FT_DIR);
            if (!c->noappend) c->append = '/';
            return m;
        }
    }
    if (ed->ps1) {
        // Programmable completion (pcomplete.cpp)
        i32 how = 0;
        Slice<Str> m = pcomplete(ed, sh, c, &cmdpos, &how);
        if (how == 1) return m;
    }
    // Variables after programmable completion, as bash's order
    if (raw.len && raw[0]=='$' && !(raw.len>1 && raw[1]=='(')) {
        Slice<Str> m = compvariables(sh, c, raw);
        if (m.len) return m;
    }
    if (raw.len && raw[0]=='~' && !compin(raw, '/')) {
        // No user database: nothing here, and the file names below
    }
    if (raw.len && raw[0]=='@' && sh->shopt[S_HOSTCOMPLETE]) {
        Slice<Str> m = comphosts(sh, c, raw);
        if (m.len) return m;
    }
    if (cmdpos) {
        if (sh->shopt[S_NO_EMPTY_CMD_COMPLETION] && !raw.len) {
            c->ignore = IG_ALL;
        } else {
            Slice<Str> m = compcommands(ed, sh, c, w);
            if (m.len == 1 && !compin(m[0], '/') && m[0][0]!='~' && !c->dotinpath) {
                Stat st = pathstat(sh, *c->a, m[0], 1);
                if (!st.err && st.type==FT_DIR) {
                    c->noappend = 1;
                    c->files    = 0;
                }
            }
            if (m.len) return m;
            c->ignore  = IG_DIRS;
            c->cmdstat = 0;
        }
    }
    if (compglobchars(raw)) {
        // In TAB mode several matches are dropped (and the pattern
        // matches no file name either)
        Slice<Str> m = compglob(sh, c, raw);
        if (m.len>1 && (c->mode==CM_AMBIG || c->mode==CM_UNMOD)) c->noappend = 1;
        if (m.len>1 && c->mode==CM_LIST && raw[raw.len-1]=='/') c->noappend = 1;
        if (m.len==1 || (m.len && c->mode!=CM_TAB)) return m;
    }
    if (c->nofiles) return {};
    return compfiles(ed, sh, c, w, 1);
}


// Post-processing (§6.1)

static b32 compless(Str *a, Str *b)
{
    return compare(*a, *b) < 0;
}

// A match's path as the file system knows it
static Str comppath(Shell *sh, Comp *c, Str m)
{
    Arena *a = c->a;
    if (c->cmdstat && !compin(m, '/')) {
        Str p = pathsearch(sh, a, getvar(sh, a, "PATH").value, m);
        if (p.len) return p;
    }
    if (c->udir.len && startswith(m, c->udir)) {
        return joinpath(a, c->odir, cuthead(m, c->udir.len));
    }
    if (m.len && m[0]=='~') return tildefilename(sh, a, m);
    return compexpdir(sh, a, m);
}

static Stat compstat(Shell *sh, Comp *c, Str m, b32 follow)
{
    return pathstat(sh, *c->a, comppath(sh, c, m), follow);
}

static void compfilter(Shell *sh, Comp *c)
{
    if (!c->files || c->ignore==IG_NONE) return;
    Arena *a = c->a;
    Slice<Str> keep = {};
    Str fig = getvar(sh, a, "FIGNORE").value;
    for (iz i = 0; i < c->m.len; i++) {
        Str  n    = c->m[i];
        b32  drop = 0;
        switch (c->ignore) {
        case IG_ALL:  drop = 1; break;
        case IG_DIRS: drop = compstat(sh, c, n, 1).type != FT_DIR; break;
        case IG_FIGNORE:
            for (Str rest = fig; rest.data && !drop;) {
                Cut s = cut(rest, ':');
                rest = s.ok ? s.tail : Str{};
                if (s.head.len && n.len>s.head.len && endswith(n, s.head)) drop = 1;
            }
            break;
        }
        if (!drop) keep = push(a, keep, n);
    }
    if (!keep.len && c->ignore==IG_FIGNORE && !sh->shopt[S_FORCE_FIGNORE]) return;
    c->m = keep;
}

static void complcd(Editor *ed, Comp *c, Str typed)
{
    Slice<Str> m = c->m;
    if (!m.len) return;
    if (m.len == 1) {
        c->lcd = m[0];
        return;
    }
    iz n = m[0].len;
    for (iz i = 1; i < m.len; i++) {
        iz k = 0;
        Str x = m[0], y = m[i];
        for (; k<n && k<y.len; k++) {
            u8 p = x[k], q = y[k];
            if (ed->vars.ignorecase) p = complower(p), q = complower(q);
            if (p != q) break;
        }
        n = k;
    }
    c->lcd = takehead(m[0], n);
    if (ed->vars.ignorecase) {
        iz len = typed.len<n ? typed.len : n;
        for (iz i = 0; i < m.len; i++) {
            if (takehead(m[i], len) == takehead(typed, len)) {
                c->lcd = takehead(m[i], n);
                break;
            }
        }
    }
    if (!n && typed.len) c->lcd = typed;
}

static void compprocess(Editor *ed, Shell *sh, Comp *c)
{
    Slice<Str> m = c->m;
    if (!c->nosort) sort(m.data, m.len, compless, *c->a);
    iz n = 0;
    for (iz i = 0; i < m.len; i++) {
        if (!n || m[i]!=m[n-1]) m[n++] = m[i];
    }
    m.len = n;
    c->m  = m;
    compfilter(sh, c);
    complcd(ed, c, compbare(c));
}


// Quoting (§6.3)

enum : i32 { QS_BACKSLASH, QS_SINGLE, QS_DOUBLE };

static Str compbackslash(Arena *a, Str s, Str breaks, Str noesc, b32 tilde)
{
    Buf b(a, s.len*2);
    for (iz i = 0; i < s.len; i++) {
        u8  ch  = s[i];
        b32 esc = compin(" \t\n'\"$=:@,][{}!&*\\();|<>?^`", ch)
                  || (i==0 && ch=='#') || (i==0 && ch=='~' && tilde)
                  || compin(breaks, ch);
        if (esc && !compin(noesc, ch)) putbyte(&b, '\\');
        putbyte(&b, ch);
    }
    return finish(&b);
}

// Quote a match for insertion; *style gets the style used. mult cuts the
// closing quote.
static Str compquote(Editor *, Shell *sh, Comp *c, Str s, b32 mult, i32 *style)
{
    Arena *a = c->a;
    Str  breaks = c->breaks;
    i32  qs     = c->quote=='\'' ? QS_SINGLE : c->quote=='"' ? QS_DOUBLE : QS_BACKSLASH;
    Str  noesc  = {};
    b32  plain  = 0;  // double quotes leaving $ and ` alone
    if (qs==QS_BACKSLASH && !sh->shopt[S_DIREXPAND]
        && (compin(s, '$') || compin(s, '`')) && !compexists(sh, *a, s)) {
        noesc = "${}()~`";
        b32 any = 0;
        for (iz i = 0; i < s.len; i++) {
            any |= !compin(noesc, s[i]) && compin(" \t\n\\\"'@<>=;|&#?*[!:", s[i]);
        }
        if (any) qs = QS_DOUBLE;
        plain = 1;
    } else if (qs==QS_BACKSLASH && compin(s, '\n')) {
        qs = QS_SINGLE;
    } else if (qs==QS_DOUBLE) {
        plain = 1;
        if (sh->opt[O_HISTEXPAND] && compin(s, '!')) qs = QS_BACKSLASH;
    }
    if (qs!=QS_BACKSLASH && s.len && s[0]=='~') s = tildefilename(sh, a, s);
    *style = qs;
    Buf b(a, s.len*2+2);
    switch (qs) {
    case QS_BACKSLASH: {
        b32 tilde = s.len && s[0]=='~' && compexists(sh, *a, s);
        // Without bash's full quoting (the specific commands), = and :
        // are escaped only as word breaks
        if (c->partial) noesc = concat(a, noesc, "=:");
        return compbackslash(a, s, breaks, noesc, tilde);
    }
    case QS_SINGLE:
        putbyte(&b, '\'');
        for (iz i = 0; i < s.len; i++) {
            if (s[i] == '\'') print(&b, "'\\''"); else putbyte(&b, s[i]);
        }
        if (!mult) putbyte(&b, '\'');
        break;
    case QS_DOUBLE:
        putbyte(&b, '"');
        for (iz i = 0; i < s.len; i++) {
            u8 ch = s[i];
            if (ch=='"' || ch=='\\' || (!plain && (ch=='$' || ch=='`'))) putbyte(&b, '\\');
            putbyte(&b, ch);
        }
        if (!mult) putbyte(&b, '"');
        break;
    }
    return finish(&b);
}

static b32 compshouldquote(Comp *c, Str s)
{
    if ((!c->files && !c->fullquote) || c->noqfn) return 0;
    for (iz i = 0; i < s.len; i++) {
        if (compin(" \t\n\\\"'@<>=;|&()#$`?*[!:{~", s[i])) return 1;
    }
    return !c->quote && c->found;
}

// Replace the word with s (§6.2), as one undo group
static void compreplace(Editor *ed, Shell *sh, Comp *c, Str s, b32 mult)
{
    iz  beg = c->start;
    Str t   = s;
    if (compshouldquote(c, s)) {
        i32 style;
        t = compquote(ed, sh, c, s, mult, &style);
        b32 changed = c->quote && style==QS_BACKSLASH;
        if (beg>0 && c->quote && ed->line.data[beg-1]==c->quote
            && ((t.len && t[0]==c->quote) || changed)) {
            beg--;
        }
        if (changed) c->noquote = 1;
    }
    if (ed->vars.skipcompleted) {
        // Skip what already matches, from the word's start on, past
        // point if it goes on matching
        iz j = 0;
        while (beg+j<ed->line.len && j<t.len && ed->line.data[beg+j]==t[j]) j++;
        Str rest = cuthead(t, j);
        if (beg+j<=c->end || rest.len) {
            edgroup(ed, U_BEGIN);
            if (beg+j < c->end) eddelete(ed, beg+j, c->end);
            ed->point = beg + j;
            edinsert(ed, rest);
            edgroup(ed, U_END);
        }
        ed->point = beg + j + rest.len;
        return;
    }
    Str old = {ed->line.data+beg, c->end-beg};
    if (old != t) {
        edgroup(ed, U_BEGIN);
        eddelete(ed, beg, c->end);
        ed->point = beg;
        edinsert(ed, t);
        edgroup(ed, U_END);
    } else {
        ed->point = c->end;
    }
}

// The character after a single match (§6.4)
static void compappend(Editor *ed, Shell *sh, Comp *c, Str match)
{
    u8  buf[2];
    iz  n = 0;
    iz  p = ed->point;
    if (c->quote && !c->noquote && !(p>0 && ed->line.data[p-1]==c->quote)) {
        buf[n++] = c->quote;
    }
    if (!c->noappend && c->append) buf[n++] = c->append;
    b32 atend = p == ed->line.len;
    if (c->files) {
        Str  bare  = compbare(c);
        b32  added = match.len > bare.len;
        b32  link  = 0;
        Stat st    = compstat(sh, c, match, !(added && !ed->vars.marksymdirs));
        if (!st.err && st.type==FT_LINK) {
            link = compstat(sh, c, match, 1).type == FT_DIR;
        }
        if (!st.err && st.type==FT_DIR) {
            if (!ed->vars.markdirs) return;
            if (p<ed->line.len && ed->line.data[p]=='/') return;
            if (atend && p>0 && ed->line.data[p-1]=='/') return;
            edinsert(ed, "/");
            return;
        }
        if (link) return;
    }
    if (n && atend) edinsert(ed, {buf, n});
}


// The listing (§7)

// The part of a match listed
static Str compprintable(Comp *c, Str m)
{
    if (!c->files) return m;
    iz e = m.len;
    if (e>1 && m[e-1]=='/') e--;
    iz i = e;
    while (i>0 && m[i-1]!='/') i--;
    return i==0 && e<m.len ? m : cuthead(m, i);
}

static i32 compwidth(Str s)
{
    i32 w = 0;
    for (iz i = 0; i < s.len;) {
        u8  ch = s[i];
        if (ch<0x20 || ch==0x7f) {
            w += 2;
            i++;
            continue;
        }
        i32 cp = utf8decode(s, &i);
        i32 cw = cpwidth(cp);
        w += cw<0 ? 1 : cw;
    }
    return w;
}

static void compputs(Buf *b, Str s)
{
    for (iz i = 0; i < s.len; i++) {
        u8 ch = s[i];
        if (ch<0x20 || ch==0x7f) {
            putbyte(b, '^');
            putbyte(b, ch==0x7f ? '?' : ch|0x40);
        } else {
            putbyte(b, ch);
        }
    }
}

static u8 compmark(Editor *ed, Shell *sh, Comp *c, Str m)
{
    if (!c->files) return 0;
    if (ed->vars.visiblestats) {
        Stat st = compstat(sh, c, m, 0);
        if (st.err) return 0;
        switch (st.type) {
        case FT_DIR:   return '/';
        case FT_CHAR:  return '%';
        case FT_BLOCK: return '#';
        case FT_LINK:  return '@';
        case FT_SOCK:  return '=';
        case FT_FIFO:  return '|';
        case FT_FILE:  return canexec(sh, st) ? '*' : 0;
        }
        return 0;
    }
    if (ed->vars.markdirs) {
        Stat st = compstat(sh, c, m, 1);
        if (!st.err && st.type==FT_DIR) return '/';
    }
    return 0;
}

// Read a key for a listing prompt
static i32 compkey(Editor *ed, Shell *sh)
{
    edflush(ed, sh);
    return edkey(ed, sh, -1);
}

// After a listing, the prompt and line drawn afresh below it
static void compredraw(Editor *ed, Shell *sh)
{
    for (iz i = 0; i < ed->phead.len; i++) {
        u8 ch = ed->phead[i];
        if (ch!=1 && ch!=2) putbyte(&ed->out_, ch);
    }
    edredraw(ed, sh);
}

static i32 complistwidth(Editor *ed, Shell *sh, Arena *a)
{
    i32 w = ed->width;
    if (ed->vars.displaywidth>=0 && ed->vars.displaywidth<=w) {
        return ed->vars.displaywidth;
    }
    Got g = getvar(sh, a, "COLUMNS");
    if (g.set && g.value.len) {
        i64 v = 0;
        b32 ok = 1;
        for (iz i = 0; i < g.value.len; i++) {
            u8 ch = g.value[i];
            if (ch<'0' || ch>'9' || v>1000000) { ok = 0; break; }
            v = v*10 + ch - '0';
        }
        if (ok && v <= w) return (i32)v;
    }
    return w;
}

static void complist(Editor *ed, Shell *sh, Comp *c)
{
    Arena *a = c->a;
    Slice<Str> m = c->m;
    Layout *l = &ed->shown;
    edmove(ed, l->nrows>0 ? l->nrows-1 : 0, 0);
    edput(ed, "\r\n");

    // The printable parts, with any common prefix shortened
    iz n = m.len;
    Str *items = alloc<Str>(a, n);
    u8  *marks = alloc<u8>(a, n);
    i32  max   = 0;
    for (iz i = 0; i < n; i++) {
        items[i] = compprintable(c, m[i]);
        marks[i] = compmark(ed, sh, c, m[i]);
    }
    iz plen = 0;
    if (ed->vars.prefixdisplay > 0 && n > 1) {
        plen = items[0].len;
        for (iz i = 1; i < n; i++) {
            iz k = 0;
            for (; k<plen && k<items[i].len && items[i][k]==items[0][k]; k++) {}
            plen = k;
        }
        if (plen<=ed->vars.prefixdisplay || plen<=3) plen = 0;
    }
    Str *shown = alloc<Str>(a, n);
    for (iz i = 0; i < n; i++) {
        Str s = items[i];
        if (plen && s.len>plen) {
            s = concat(a, s[plen]=='.' ? Str("___") : Str("..."), cuthead(s, plen));
        }
        shown[i] = s;
        i32 w = compwidth(s);
        max = w>max ? w : max;
    }

    if (n == 1) {
        Buf b(a, 64);
        compputs(&b, shown[0]);
        if (marks[0]) putbyte(&b, marks[0]);
        print(&b, "\r\n");
        edput(ed, finish(&b));
        compredraw(ed, sh);
        return;
    }

    i32 q = ed->vars.queryitems;
    if (q>0 && n>=q) {
        Buf b(a, 64);
        print(&b, "Display all ");
        print(&b, (i64)n);
        print(&b, " possibilities? (y or n)");
        edput(ed, finish(&b));
        for (;;) {
            i32 k = compkey(ed, sh);
            if (k<0 || k==0x07) {
                edbell(ed);
                edput(ed, "\r\n");
                compredraw(ed, sh);
                return;
            }
            if (k=='y' || k=='Y' || k==' ') break;
            if (k=='n' || k=='N' || k==0x7f) {
                edput(ed, "\r\n");
                compredraw(ed, sh);
                return;
            }
            edbell(ed);
        }
        edput(ed, "\r\n");
    }

    i32 width = complistwidth(ed, sh, a);
    i32 w     = max + 2;
    i32 cols  = width / w;
    if (cols!=1 && cols*w==width) cols--;
    if (cols < 1) cols = 1;
    iz  rows  = (n + cols - 1) / cols;
    b32 horiz = ed->vars.horizontal;
    i32 lines = 0;
    i32 page  = ed->tty.rows - 1;
    for (iz r = 0; r < rows; r++) {
        Buf b(a, 256);
        for (i32 k = 0; k < cols; k++) {
            iz i = horiz ? r*cols + k : r + k*rows;
            if (i >= n) break;
            compputs(&b, shown[i]);
            i32 pw = compwidth(shown[i]);
            if (marks[i]) {
                putbyte(&b, marks[i]);
                pw++;
            }
            iz next = horiz ? i+1 : i+rows;
            b32 last = k==cols-1 || next>=n || (horiz && (k+1)==cols);
            if (!last) {
                i32 pad = w - pw;
                if (pad < 1) pad = 1;
                for (i32 j = 0; j < pad; j++) putbyte(&b, ' ');
            }
        }
        print(&b, "\r\n");
        edput(ed, finish(&b));
        lines++;
        if (ed->vars.pagecompl && lines>=page && r+1<rows) {
            edput(ed, "--More--");
            i32 act = 0;
            for (;;) {
                i32 k = compkey(ed, sh);
                if (k<0 || k==0x07) { act = 3; edbell(ed); break; }
                if (k=='y' || k=='Y' || k==' ') { act = 0; break; }
                if (k=='\r' || k=='\n') { act = 1; break; }
                if (k=='q' || k=='Q' || k=='n' || k=='N' || k==0x7f) { act = 2; break; }
                edbell(ed);
            }
            edput(ed, "\r\x1b[K\r");
            if (act >= 2) {
                compredraw(ed, sh);
                return;
            }
            lines = act==1 ? page-1 : 0;
        }
    }
    compredraw(ed, sh);
}


// Commands


static void compinsertall(Editor *ed, Shell *sh, Comp *c)
{
    Arena *a = c->a;
    Buf b(a, 256);
    for (iz i = 0; i < c->m.len; i++) {
        Str s = c->m[i];
        if (compshouldquote(c, s)) {
            i32 style;
            s = compquote(ed, sh, c, s, 0, &style);
        }
        print(&b, s);
        putbyte(&b, ' ');
    }
    Str t = finish(&b);
    edgroup(ed, U_BEGIN);
    iz beg = c->start;
    if (c->quote && beg>0 && ed->line.data[beg-1]==c->quote) beg--;
    eddelete(ed, beg, c->end);
    ed->point = beg;
    edinsert(ed, t);
    edgroup(ed, U_END);
}

// A piece of a brace expansion, backslash-quoted
static void compbq(Buf *b, Str s)
{
    for (iz i = 0; i < s.len; i++) {
        if (compin(" \t\n'\"$=:@,][{}!&*\\();|<>?^`#~", s[i])) putbyte(b, '\\');
        putbyte(b, s[i]);
    }
}

// Sorted matches m[lo, hi), alike before off, as one brace expansion:
// those sharing their next character grouped under their common prefix,
// and so on within each group.
static void compbrace(Buf *b, Slice<Str> m, iz lo, iz hi, iz off)
{
    putbyte(b, '{');
    for (iz i = lo; i < hi;) {
        if (i > lo) putbyte(b, ',');
        iz j = i + 1;
        if (m[i].len > off) {
            for (; j<hi && m[j].len>off && m[j][off]==m[i][off]; j++) {}
        }
        if (j-i == 1) {
            compbq(b, cuthead(m[i], off));
        } else {
            iz l = off + 1;
            for (;; l++) {
                b32 same = l < m[i].len;
                for (iz k = i+1; same && k<j; k++) {
                    same = l<m[k].len && m[k][l]==m[i][l];
                }
                if (!same) break;
            }
            compbq(b, span(m[i].data+off, m[i].data+l));
            compbrace(b, m, i, j, l);
        }
        i = j;
    }
    putbyte(b, '}');
}

// complete-into-braces: the word replaced by its matches as a brace
// expansion, and a space
static void compbraces(Editor *ed, Shell *, Comp *c)
{
    Slice<Str> m = c->m;
    Buf b(c->a, 256);
    if (m.len == 1) {
        compbq(&b, m[0]);
    } else {
        iz p = 0;
        for (;; p++) {
            b32 same = p < m[0].len;
            for (iz k = 1; same && k<m.len; k++) same = p<m[k].len && m[k][p]==m[0][p];
            if (!same) break;
        }
        compbq(&b, takehead(m[0], p));
        compbrace(&b, m, 0, m.len, p);
    }
    putbyte(&b, ' ');
    Str t = finish(&b);
    edgroup(ed, U_BEGIN);
    iz beg = c->start;
    if (c->quote && beg>0 && ed->line.data[beg-1]==c->quote) beg--;
    eddelete(ed, beg, c->end);
    ed->point = beg;
    edinsert(ed, t);
    edgroup(ed, U_END);
}

// One completion attempt in a mode with a generator
static void edcomplete1(Editor *ed, Shell *sh, u8 mode, i32 gen)
{
    Arena a = *scratchat(sh, sh->capdepth);  // claimed by edcomplete
    Str before = clone(&a, edtext(ed));
    Comp c = {};
    c.a      = &a;
    c.line   = {ed->line.data, ed->point};
    c.append = ' ';
    Str breaks = compbreaks(sh, &a);
    if (gen == CX_FILE) breaks = " \t\n\"'";
    c.breaks  = breaks;
    c.mode    = mode;
    c.partial = gen != CX_SHELL;
    sh->comptype = mode;  // COMP_TYPE and COMP_KEY
    sh->compkey  = ed->curkey;
    compword(&c, breaks, gen==CX_SHELL && sh->shopt[S_HOSTCOMPLETE]);
    if (!ed->shellline && gen==CX_SHELL) gen = CX_FILE;
    c.m = compgenerate(ed, sh, &c, gen);
    compprocess(ed, sh, &c);

    if (!c.m.len) {
        edbell(ed);
        ed->cfailed  = 1;
        ed->cchanged = 0;
        return;
    }
    b32 failed = ed->cfailed;
    ed->cfailed = !c.lcd.len;
    Str bare = compbare(&c);
    switch (mode) {
    case CM_TAB:
        if (c.lcd.len) compreplace(ed, sh, &c, c.lcd, c.m.len>1);
        if (c.m.len == 1) {
            compappend(ed, sh, &c, c.m[0]);
        } else if (!c.m.len || !ed->vars.vimode) {
            edbell(ed);  // vi mode is quiet on more than one match
        }
        break;
    case CM_AMBIG:
    case CM_UNMOD:
        if (c.m.len == 1) {
            compreplace(ed, sh, &c, c.lcd, 0);
            compappend(ed, sh, &c, c.m[0]);
        } else if (mode==CM_UNMOD && c.lcd!=bare) {
            compreplace(ed, sh, &c, c.lcd, 1);
        } else {
            if (c.lcd.len >= bare.len) compreplace(ed, sh, &c, c.lcd, 1);
            complist(ed, sh, &c);
        }
        break;
    case CM_LIST:
        if (failed && c.m.len==1) {
            compreplace(ed, sh, &c, c.lcd, 0);
            compappend(ed, sh, &c, c.m[0]);
        } else {
            complist(ed, sh, &c);
        }
        break;
    case CM_ALL:
        compinsertall(ed, sh, &c);
        break;
    case CM_BRACE:
        compbraces(ed, sh, &c);
        break;
    }
    ed->cchanged = edtext(ed) != before;
}

// The mode of a command that completes (rl_completion_mode)
static u8 compmode(Editor *ed, i32 fn)
{
    if (ed->lastfn==fn && !ed->cchanged) return CM_LIST;
    if (ed->vars.showall)   return CM_AMBIG;
    if (ed->vars.showunmod) return CM_UNMOD;
    return CM_TAB;
}

// menu-complete and menu-complete-backward (§8), and bash's
// dabbrev-expand, which also continues a menu-complete
static void edmenu(Editor *ed, Shell *sh, i32 fn, i32 count)
{
    if (fn == EF_MENU_COMPLETE_BACKWARD) count = -count;
    i32 last = ed->lastfn;
    b32 cont = (last==EF_MENU_COMPLETE || last==EF_MENU_COMPLETE_BACKWARD
                || (last==EF_DABBREV_EXPAND && fn==EF_DABBREV_EXPAND))
               && !ed->menudone && ed->menu && ed->menu->m.len;
    i32 gen = fn==EF_DABBREV_EXPAND ? CX_DABBREV
            : ed->shellline         ? CX_SHELL
            :                         CX_FILE;
    if (!ed->menu) ed->menu = alloc<Comp>(&ed->perm);
    if (!cont) {
        ed->cmem = {(byte *)(ed->cmem.mem+1), ed->cmem.end, ed->cmem.mem};
        Comp *c = ed->menu;
        *c = {};
        c->a      = &ed->cmem;
        c->line   = {ed->line.data, ed->point};
        c->append = ' ';
        c->breaks = compbreaks(sh, c->a);
        c->mode   = CM_TAB;
        sh->comptype = '%';
        sh->compkey  = ed->curkey;
        compword(c, c->breaks, sh->shopt[S_HOSTCOMPLETE]);
        c->line = clone(c->a, c->line);
        c->word = cuthead(c->line, c->start);
        c->m = compgenerate(ed, sh, c, gen);
        compprocess(ed, sh, c);
        ed->menudone = 0;
        ed->menuidx  = 0;
        if (!c->m.len) {
            edbell(ed);
            return;
        }
        if (c->lcd.len) {
            compreplace(ed, sh, c, c->lcd, c->m.len>1);
            c->end = ed->point;
        }
        if (c->m.len == 1) {
            compappend(ed, sh, c, c->m[0]);
            ed->menudone = 1;
            return;
        }
        if (ed->vars.showall) {
            complist(ed, sh, c);
            i32 q = ed->vars.queryitems;
            if (q>0 && c->m.len>=q) {
                edbell(ed);
                ed->menudone = 1;
                return;
            }
            if (ed->vars.menuprefix) {
                edbell(ed);
                return;
            }
        } else if (ed->vars.menuprefix) {
            edbell(ed);
            return;
        }
    }
    Comp *c = ed->menu;
    i32 total = (i32)c->m.len + 1;
    ed->menuidx = ((ed->menuidx + count) % total + total) % total;
    Comp t = *c;
    t.end   = ed->point;
    t.quote = c->quote;
    if (ed->menuidx == 0) {
        edbell(ed);
        compreplace(ed, sh, &t, c->lcd, 1);
    } else {
        Str s = c->m[ed->menuidx-1];
        compreplace(ed, sh, &t, s, 0);
        compappend(ed, sh, &t, s);
    }
}

// export-completions: the matches written out for another program, and
// the line cleared by an argument above one. The display is left as it
// was, as readline does.
static void edexport(Editor *ed, Shell *sh, i32 count)
{
    Arena a = *scratchat(sh, sh->capdepth);  // claimed by edcomplete
    Comp c = {};
    c.a      = &a;
    c.line   = {ed->line.data, ed->point};
    c.append = ' ';
    c.breaks = compbreaks(sh, &a);
    c.mode   = '$';
    sh->comptype = '$';
    sh->compkey  = ed->curkey;
    compword(&c, c.breaks, sh->shopt[S_HOSTCOMPLETE]);
    c.m = compgenerate(ed, sh, &c, ed->shellline ? CX_SHELL : CX_FILE);
    compprocess(ed, sh, &c);

    Buf b(&a, 256);
    iz n = c.m.len>1 ? c.m.len+1 : c.m.len;
    print(&b, "\r\n");
    print(&b, (i64)n);
    print(&b, "\r\n");
    print(&b, c.word);
    print(&b, "\r\n");
    print(&b, (i64)c.start);
    print(&b, ":");
    print(&b, (i64)c.end);
    print(&b, "\r\n");
    for (iz i = 0; i < n; i++) {
        Str m = c.m.len>1 ? (i ? c.m[i-1] : c.lcd) : c.m[i];
        compputs(&b, m);
        u8 mark = compmark(ed, sh, &c, m);
        if (mark) putbyte(&b, mark);
        print(&b, "\r\n");
    }
    // Written past the editor's output, which stays as it was
    edflush(ed, sh);
    Str out = finish(&b);
    if (ed->tty.legacy) {
        vtconwrite(sh->os, ed->out, &ed->vtcon, out);
    } else {
        fdwrite(sh, 2, out);
    }

    if (count > 1) {
        edgroup(ed, U_BEGIN);
        eddelete(ed, 0, ed->line.len);
        edgroup(ed, U_END);
        ed->point = ed->mark = 0;
    }
}

static void edcomplete2(Editor *ed, Shell *sh, i32 fn, i32 count)
{
    switch (fn) {
    case EF_COMPLETE:
        if (ed->vars.nocompletion) {
            for (i32 i = 0; i < (count>0 ? count : 1); i++) edinsert(ed, "\t");
            return;
        }
        edcomplete1(ed, sh, compmode(ed, fn), CX_SHELL);
        return;
    case EF_POSSIBLE_COMPLETIONS:
        ed->cfailed = 0;
        edcomplete1(ed, sh, CM_LIST, CX_SHELL);
        return;
    case EF_DELETE_CHAR_OR_LIST:
        ed->cfailed = 0;
        edcomplete1(ed, sh, CM_LIST, CX_SHELL);
        return;
    case EF_INSERT_COMPLETIONS:
        edcomplete1(ed, sh, CM_ALL, CX_SHELL);
        return;
    case EF_COMPLETE_FILENAME:  edcomplete1(ed, sh, compmode(ed, fn), CX_FILE);     return;
    case EF_COMPLETE_COMMAND:   edcomplete1(ed, sh, compmode(ed, fn), CX_COMMAND);  return;
    case EF_COMPLETE_VARIABLE:  edcomplete1(ed, sh, compmode(ed, fn), CX_VARIABLE); return;
    case EF_COMPLETE_USERNAME:  edcomplete1(ed, sh, compmode(ed, fn), CX_USER);     return;
    case EF_COMPLETE_HOSTNAME:  edcomplete1(ed, sh, compmode(ed, fn), CX_HOST);     return;
    case EF_DYNAMIC_COMPLETE_HISTORY:
        edcomplete1(ed, sh, compmode(ed, fn), CX_HISTORY);
        return;
    case EF_GLOB_COMPLETE_WORD: edcomplete1(ed, sh, compmode(ed, fn), CX_GLOB);     return;
    case EF_GLOB_EXPAND_WORD:     edcomplete1(ed, sh, CM_ALL, CX_GLOBX);   return;
    case EF_GLOB_LIST_EXPANSIONS: edcomplete1(ed, sh, CM_LIST, CX_GLOBX);  return;
    case EF_COMPLETE_INTO_BRACES: edcomplete1(ed, sh, CM_BRACE, CX_FILE);  return;
    case EF_POSSIBLE_FILENAME_COMPLETIONS:
        ed->cfailed = 0;
        edcomplete1(ed, sh, CM_LIST, CX_FILE);
        return;
    case EF_POSSIBLE_COMMAND_COMPLETIONS:
        ed->cfailed = 0;
        edcomplete1(ed, sh, CM_LIST, CX_COMMAND);
        return;
    case EF_POSSIBLE_VARIABLE_COMPLETIONS:
        ed->cfailed = 0;
        edcomplete1(ed, sh, CM_LIST, CX_VARIABLE);
        return;
    case EF_POSSIBLE_USERNAME_COMPLETIONS:
        ed->cfailed = 0;
        edcomplete1(ed, sh, CM_LIST, CX_USER);
        return;
    case EF_POSSIBLE_HOSTNAME_COMPLETIONS:
        ed->cfailed = 0;
        edcomplete1(ed, sh, CM_LIST, CX_HOST);
        return;
    case EF_MENU_COMPLETE:
    case EF_MENU_COMPLETE_BACKWARD:
    case EF_DABBREV_EXPAND:
        edmenu(ed, sh, fn, count);
        return;
    case EF_EXPORT_COMPLETIONS:
        edexport(ed, sh, count);
        return;
    }
    edbell(ed);
}

// Run a completion command. It claims a scratch depth, since compspecs
// run shell code whose command substitutions take the next one.
static void edcomplete(Editor *ed, Shell *sh, i32 fn, i32 count)
{
    i32 depth = ++sh->capdepth;
    edcomplete2(ed, sh, fn, count);
    sh->capdepth = depth - 1;
}
