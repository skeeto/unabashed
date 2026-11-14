// Programmable completion: compspecs, the complete, compgen and compopt
// builtins, and the generation pipeline (notes/research/progcomp.md,
// cited as P§N). The editor's side is in complete.cpp.
// This is free and unencumbered software released into the public domain.

// Actions, in the table order complete -p prints them (P§2.2)
enum : i32 {
    PA_ALIAS, PA_ARRAYVAR, PA_BINDING, PA_BUILTIN, PA_COMMAND, PA_DIRECTORY,
    PA_DISABLED, PA_ENABLED, PA_EXPORT, PA_FILE, PA_FUNCTION, PA_HELPTOPIC,
    PA_HOSTNAME, PA_GROUP, PA_JOB, PA_KEYWORD, PA_RUNNING, PA_SERVICE,
    PA_SETOPT, PA_SHOPT, PA_SIGNAL, PA_STOPPED, PA_USER, PA_VARIABLE,
    PA_COUNT
};

static struct { Str name; u8 letter; } const pactions[PA_COUNT] = {
    {"alias", 'a'},    {"arrayvar", 0},  {"binding", 0},  {"builtin", 'b'},
    {"command", 'c'},  {"directory", 'd'}, {"disabled", 0}, {"enabled", 0},
    {"export", 'e'},   {"file", 'f'},    {"function", 0}, {"helptopic", 0},
    {"hostname", 0},   {"group", 'g'},   {"job", 'j'},    {"keyword", 'k'},
    {"running", 0},    {"service", 's'}, {"setopt", 0},   {"shopt", 0},
    {"signal", 0},     {"stopped", 0},   {"user", 'u'},   {"variable", 'v'},
};

// The order the actions generate in (P§5.1)
static i32 const pgenorder[PA_COUNT] = {
    PA_ALIAS, PA_ARRAYVAR, PA_BINDING, PA_BUILTIN, PA_DISABLED, PA_ENABLED,
    PA_EXPORT, PA_FUNCTION, PA_HELPTOPIC, PA_HOSTNAME, PA_JOB, PA_KEYWORD,
    PA_RUNNING, PA_SETOPT, PA_SHOPT, PA_SIGNAL, PA_STOPPED, PA_VARIABLE,
    PA_COMMAND, PA_FILE, PA_USER, PA_GROUP, PA_SERVICE, PA_DIRECTORY,
};

// -o options, in table order (P§1.1)
enum : i32 {
    PO_BASHDEFAULT, PO_DEFAULT, PO_DIRNAMES, PO_FILENAMES, PO_FULLQUOTE,
    PO_NOQUOTE, PO_NOSORT, PO_NOSPACE, PO_PLUSDIRS, PO_COUNT
};

static Str const poptions[PO_COUNT] = {
    "bashdefault", "default", "dirnames", "filenames", "fullquote",
    "noquote", "nosort", "nospace", "plusdirs",
};

// The string arguments, in the order complete -p prints them
enum : i32 { PS_GLOB, PS_WORDS, PS_PREFIX, PS_SUFFIX, PS_FILTER, PS_CMD,
             PS_FUNC, PS_COUNT };
static u8 const pstrletters[PS_COUNT] = {'G', 'W', 'P', 'S', 'X', 'C', 'F'};

struct CompSpec {
    uz  actions;  // 1<<PA_*
    uz  options;  // 1<<PO_*, pointer-sized for jwrite
    u32 has;      // 1<<PS_*: the strings given, even empty
    Str str[PS_COUNT];
};

struct CompEnt {
    CompEnt  *next;
    Str       key;
    u32       hash;
    CompSpec *cs;
};

// bash's hashlib table, so complete -p lists in its order (P§1.2)
struct CompTable {
    CompEnt **bucket;
    iz        nbuckets;
    iz        n;
};

// The special entries' keys
static Str const pckeys[3] = {"_DefaultCmD_", "_EmptycmD_", "_InitialWorD_"};

static CompSpec *pccopyspec(Store *st, CompSpec *cs)
{
    CompSpec *c = (CompSpec *)salloc(st, sizeof(CompSpec));
    *c = *cs;
    for (i32 i = 0; i < PS_COUNT; i++) c->str[i] = sclone(st, cs->str[i]);
    return c;
}

// Deep-copy a table into st. Compspecs shared by several names (one
// complete command) stay shared in the copy.
static CompTable *compscopy(Store *st, CompTable *t)
{
    if (!t) return 0;
    CompTable *c = (CompTable *)salloc(st, sizeof(CompTable));
    c->nbuckets = t->nbuckets;
    c->n        = t->n;
    c->bucket   = (CompEnt **)salloc(st, t->nbuckets*(iz)sizeof(CompEnt *));
    // Old and new compspecs, to keep the sharing
    iz         nmap = 0;
    CompSpec **from = (CompSpec **)salloc(st, (t->n+1)*(iz)sizeof(CompSpec *));
    CompSpec **to   = (CompSpec **)salloc(st, (t->n+1)*(iz)sizeof(CompSpec *));
    for (iz b = 0; b < t->nbuckets; b++) {
        CompEnt **tail = &c->bucket[b];
        for (CompEnt *e = t->bucket[b]; e; e = e->next) {
            CompEnt *n = (CompEnt *)salloc(st, sizeof(CompEnt));
            n->key  = sclone(st, e->key);
            n->hash = e->hash;
            iz k = 0;
            for (; k<nmap && from[k]!=e->cs; k++) {}
            if (k == nmap) {
                from[nmap] = e->cs;
                to[nmap++] = pccopyspec(st, e->cs);
            }
            n->cs = to[k];
            *tail = n;
            tail  = &n->next;
        }
    }
    sfree(st, from, (t->n+1)*(iz)sizeof(CompSpec *));
    sfree(st, to,   (t->n+1)*(iz)sizeof(CompSpec *));
    return c;
}

// The table, for writing: a subshell gets its own copy first
static CompTable *compsown(Shell *sh)
{
    Store     *st = sh->store;
    CompTable *t  = sh->comps;
    if (!t) {
        t = (CompTable *)salloc(st, sizeof(CompTable));
        t->nbuckets = 512;
        t->bucket   = (CompEnt **)salloc(st, t->nbuckets*(iz)sizeof(CompEnt *));
    } else if ((byte *)t < st->mark) {
        t = compscopy(st, t);
    }
    sh->comps = t;
    return t;
}

static CompEnt *pcfind(CompTable *t, Str key)
{
    if (!t) return 0;
    u32 h = hashkey(key);
    for (CompEnt *e = t->bucket[h & (t->nbuckets-1)]; e; e = e->next) {
        if (e->hash==h && e->key==key) return e;
    }
    return 0;
}

static void pcinsert(Shell *sh, Str key, CompSpec *cs)
{
    Store     *st = sh->store;
    CompTable *t  = compsown(sh);
    CompEnt   *e  = pcfind(t, key);
    if (e) {
        e->cs = cs;
        return;
    }
    if (t->n >= t->nbuckets*2) {
        // Grow as hashlib does: chains rebuilt by prepending in walk order
        iz        nb = t->nbuckets * 4;
        CompEnt **nt = (CompEnt **)salloc(st, nb*(iz)sizeof(CompEnt *));
        for (iz b = 0; b < t->nbuckets; b++) {
            for (CompEnt *p = t->bucket[b], *next; p; p = next) {
                next = p->next;
                iz i = p->hash & (nb-1);
                p->next = nt[i];
                nt[i]   = p;
            }
        }
        sfree(st, t->bucket, t->nbuckets*(iz)sizeof(CompEnt *));
        t->bucket   = nt;
        t->nbuckets = nb;
    }
    e = (CompEnt *)salloc(st, sizeof(CompEnt));
    e->key  = sclone(st, key);
    e->hash = hashkey(key);
    e->cs   = cs;
    iz i = e->hash & (t->nbuckets-1);
    e->next     = t->bucket[i];
    t->bucket[i] = e;
    t->n++;
}

static b32 pcremove(Shell *sh, Str key)
{
    if (!pcfind(sh->comps, key)) return 0;
    CompTable *t = compsown(sh);
    u32 h = hashkey(key);
    for (CompEnt **p = &t->bucket[h & (t->nbuckets-1)]; *p; p = &(*p)->next) {
        if ((*p)->hash==h && (*p)->key==key) {
            *p = (*p)->next;
            t->n--;
            return 1;
        }
    }
    return 0;
}

static CompSpec *pclookup(Shell *sh, Str key)
{
    CompEnt *e = pcfind(sh->comps, key);
    return e ? e->cs : 0;
}


// Printing

// A name or -F argument as complete -p prints it
static Str pcquote(Arena *a, Str s)
{
    if (!s.len) return "''";
    return shellmetas(s) ? singlequote(a, s) : s;
}

static Str pcname(Arena *a, Str key)
{
    if (key == pckeys[0]) return "-D";
    if (key == pckeys[1]) return "-E";
    if (key == pckeys[2]) return "-I";
    return pcquote(a, key);
}

static void pcprintone(Shell *sh, Arena a, CompSpec *cs, Str key)
{
    Buf b(&a, 128);
    print(&b, "complete ");
    for (i32 i = 0; i < PO_COUNT; i++) {
        if (cs->options & ((uz)1<<i)) {
            print(&b, "-o ");
            print(&b, poptions[i]);
            putbyte(&b, ' ');
        }
    }
    for (i32 i = 0; i < PA_COUNT; i++) {
        if ((cs->actions & ((uz)1<<i)) && pactions[i].letter) {
            putbyte(&b, '-');
            putbyte(&b, pactions[i].letter);
            putbyte(&b, ' ');
        }
    }
    for (i32 i = 0; i < PA_COUNT; i++) {
        if ((cs->actions & ((uz)1<<i)) && !pactions[i].letter) {
            print(&b, "-A ");
            print(&b, pactions[i].name);
            putbyte(&b, ' ');
        }
    }
    for (i32 i = 0; i < PS_COUNT; i++) {
        if (!(cs->has & (1u<<i))) continue;
        putbyte(&b, '-');
        putbyte(&b, pstrletters[i]);
        putbyte(&b, ' ');
        print(&b, i==PS_FUNC ? pcquote(&a, cs->str[i]) : singlequote(&a, cs->str[i]));
        putbyte(&b, ' ');
    }
    print(&b, pcname(&a, key));
    putbyte(&b, '\n');
    bprint(sh, finish(&b));
}

static void pcprintall(Shell *sh, Arena a)
{
    CompTable *t = sh->comps;
    if (!t) return;
    for (iz i = 0; i < t->nbuckets; i++) {
        for (CompEnt *e = t->bucket[i]; e; e = e->next) {
            pcprintone(sh, a, e->cs, e->key);
        }
    }
}

// compopt's listing: every option, on or off
static void pcprintopts(Shell *sh, Arena a, uz options, Str key)
{
    Buf b(&a, 128);
    print(&b, "compopt ");
    for (i32 i = 0; i < PO_COUNT; i++) {
        print(&b, options & ((uz)1<<i) ? Str("-o ") : Str("+o "));
        print(&b, poptions[i]);
        putbyte(&b, ' ');
    }
    print(&b, pcname(&a, key));
    putbyte(&b, '\n');
    bprint(sh, finish(&b));
}


// Option parsing shared by complete and compgen (P§2.1)

enum : u32 {
    PF_P = 1<<0, PF_R = 1<<1, PF_D = 1<<2, PF_E = 1<<3, PF_I = 1<<4,
};

struct PcOpts {
    CompSpec cs;
    u32      flags;   // PF_*
    Str      var;     // -V
    b32      hasvar;
    b32      any;     // an option was given
    iz       next;    // the first operand
    i32      status;  // nonzero on an error, already reported
};

static PcOpts pcparse(Shell *sh, Arena *a, Str name, Str *args, iz nargs)
{
    PcOpts r = {};
    iz i = 1;
    for (; i < nargs; i++) {
        Str w = args[i];
        if (w.len<2 || w[0]!='-') break;
        if (w == "--") {
            i++;
            break;
        }
        for (iz k = 1; k < w.len; k++) {
            u8 c = w[k];
            Str opt = {};
            if (compin("oAGWPSXFCV", c)) {
                if (k+1 < w.len) {
                    opt = cuthead(w, k+1);
                } else if (i+1 < nargs) {
                    opt = args[++i];
                } else {
                    u8 o[2] = {'-', c};
                    Str msg = concat(a, clone(a, Str{o, 2}), ": option requires an argument");
                    builtinerror(sh, *a, name, msg);
                    usageerror(sh, *a, name);
                    r.status = 2;
                    return r;
                }
                k = w.len;
            }
            r.any = 1;
            i32 act = -1;
            switch (c) {
            case 'a': act = PA_ALIAS;    break;
            case 'b': act = PA_BUILTIN;  break;
            case 'c': act = PA_COMMAND;  break;
            case 'd': act = PA_DIRECTORY; break;
            case 'e': act = PA_EXPORT;   break;
            case 'f': act = PA_FILE;     break;
            case 'g': act = PA_GROUP;    break;
            case 'j': act = PA_JOB;      break;
            case 'k': act = PA_KEYWORD;  break;
            case 's': act = PA_SERVICE;  break;
            case 'u': act = PA_USER;     break;
            case 'v': act = PA_VARIABLE; break;
            case 'p': r.flags |= PF_P;   continue;
            case 'r': r.flags |= PF_R;   continue;
            case 'D': r.flags |= PF_D;   continue;
            case 'E': r.flags |= PF_E;   continue;
            case 'I': r.flags |= PF_I;   continue;
            case 'A':
                for (i32 j = 0; j < PA_COUNT; j++) {
                    if (pactions[j].name == opt) act = j;
                }
                if (act < 0) {
                    Str msg = concat(a, opt, ": invalid action name");
                    builtinerror(sh, *a, name, msg);
                    r.status = 2;
                    return r;
                }
                break;
            case 'o': {
                i32 o = -1;
                for (i32 j = 0; j < PO_COUNT; j++) {
                    if (poptions[j] == opt) o = j;
                }
                if (o < 0) {
                    Str msg = concat(a, opt, ": invalid option name");
                    builtinerror(sh, *a, name, msg);
                    r.status = 2;
                    return r;
                }
                r.cs.options |= (uz)1 << o;
            } continue;
            case 'V':
                r.var    = opt;
                r.hasvar = 1;
                continue;
            case 'G': case 'W': case 'P': case 'S': case 'X': case 'C': case 'F': {
                i32 s = 0;
                for (; pstrletters[s] != c; s++) {}
                if (c == 'F') {
                    for (iz j = 0; j < opt.len; j++) {
                        if (compin("()<>;&| \t\n", opt[j])) {
                            notident(sh, *a, name, opt);
                            r.status = 2;
                            return r;
                        }
                    }
                }
                r.cs.str[s] = opt;
                r.cs.has   |= 1u << s;
            } continue;
            default: {
                u8 o[2] = {'-', c};
                Str copy = clone(a, Str{o, 2});
                invalidoption(sh, *a, name, copy);
                r.status = 2;
                return r;
            }
            }
            r.cs.actions |= (uz)1 << act;
        }
    }
    r.next = i;
    return r;
}


// The generators (P§7)

// The word with its quoting removed, as bash's filename dequoter
static Str pcdequote(Arena *a, Str w)
{
    return compdequote(a, w, 0);
}

static void pcsorted(Arena *a, Slice<Str> *m, iz from)
{
    sort(m->data+from, m->len-from, strless, *a);
}

// The signal names in trap's table order
static Slice<Str> pcsignals(Arena *a)
{
    Slice<Str> m = {};
    for (i32 i = 0; i < NSIGNAL; i++) {
        if (!i) {
            m = push(a, m, Str("EXIT"));
        } else if (i==32 || i==33) {
            m = push(a, m, concat3(a, "SIGJUNK(", fmti64(a, i), ")"));
        } else {
            m = push(a, m, trapname(a, i, 1));
        }
    }
    m = push(a, m, Str("DEBUG"));
    m = push(a, m, Str("ERR"));
    m = push(a, m, Str("RETURN"));
    return m;
}

// The first fields of a colon-separated database, as getgrent
static Slice<Str> pcdbnames(Shell *sh, Arena *a, Str file)
{
    Slice<Str> m = {};
    if (sh->win32) return m;
    i32 err = 0;
    Str db  = readwholefile(sh, a, file, &err);
    for (Cut l = {{}, db, 1}; l.ok;) {
        l = cut(l.tail, '\n');
        Str name = cut(l.head, ':').head;
        if (name.len) m = push(a, m, name);
    }
    return m;
}

// Service names and their aliases, as getservent: each a primary name
// followed by aliases, the alias given when the word matches only it
static Slice<Str> pcservices(Shell *sh, Arena *a, Str w)
{
    Slice<Str> m = {};
    if (sh->win32) return m;
    i32 err = 0;
    Str db  = readwholefile(sh, a, "/etc/services", &err);
    for (Cut l = {{}, db, 1}; l.ok;) {
        l = cut(l.tail, '\n');
        Str line = cut(l.head, '#').head;
        Slice<Str> f = {};
        for (iz i = 0; i < line.len;) {
            for (; i<line.len && (line[i]==' ' || line[i]=='\t'); i++) {}
            iz beg = i;
            for (; i<line.len && line[i]!=' ' && line[i]!='\t'; i++) {}
            if (i > beg) f = push(a, f, span(line.data+beg, line.data+i));
        }
        if (f.len < 2) continue;
        if (startswith(f[0], w)) {
            m = push(a, m, f[0]);
            continue;
        }
        for (iz i = 2; i < f.len; i++) {
            if (startswith(f[i], w)) {
                m = push(a, m, f[i]);
                break;
            }
        }
    }
    return m;
}

static Slice<Str> pcjobs(Shell *sh, Arena *a, i32 which)
{
    Slice<Str> m = {};
    for (Job *j = sh->jobs; j; j = j->next) {
        if (which==PA_RUNNING && !jobrunning(j)) continue;
        if (which==PA_STOPPED && !(j->stopped && !j->done)) continue;
        Str t = j->text;
        iz  e = 0;
        for (; e<t.len && t[e]!=' ' && t[e]!='\t'; e++) {}
        m = push(a, m, takehead(t, e));
    }
    return m;
}

static Slice<Str> pcarrayvars(Var *v, Arena *a, Slice<Str> m)
{
    if (!v) return m;
    Value *t = v->top;
    if (t && t->arr && (t->arr->len || t->arr->n || t->arr->isset)) {
        m = push(a, m, v->name);
    }
    for (i32 i = 0; i < 4; i++) m = pcarrayvars(v->child[i], a, m);
    return m;
}

// Is this one of the functions bash adds to readline's?
static b32 pcbashfn(Str name)
{
    static Str const names[] = {
        "alias-expand-line", "bash-vi-complete", "complete-command",
        "complete-filename", "complete-hostname", "complete-into-braces",
        "complete-username", "complete-variable", "dabbrev-expand",
        "display-shell-version", "dynamic-complete-history",
        "edit-and-execute-command", "glob-complete-word",
        "glob-expand-word", "glob-list-expansions",
        "history-and-alias-expand-line", "history-expand-line",
        "insert-last-argument", "magic-space",
        "possible-command-completions", "possible-filename-completions",
        "possible-hostname-completions", "possible-username-completions",
        "possible-variable-completions", "shell-backward-kill-word",
        "shell-backward-word", "shell-expand-line", "shell-forward-word",
        "shell-kill-word", "shell-transpose-words", "spell-correct-word",
        "vi-edit-and-execute-command",
    };
    for (iz i = 0; i < countof(names); i++) {
        if (names[i] == name) return 1;
    }
    return 0;
}

// A list action's names before filtering by the word
static Slice<Str> pclist(Shell *sh, Arena *a, i32 act)
{
    Slice<Str> m = {};
    switch (act) {
    case PA_ALIAS:    compaliases(sh->aliases, a, &m); pcsorted(a, &m, 0); break;
    case PA_ARRAYVAR: m = pcarrayvars(sh->vars, a, m); pcsorted(a, &m, 0); break;
    case PA_BINDING:
        for (iz i = 0; i < countof(rlfunmap); i++) {
            // Until line editing starts, readline has only its own
            if (!(sh->editor && sh->editor->inited) && pcbashfn(rlfunmap[i])) continue;
            m = push(a, m, rlfunmap[i]);
        }
        pcsorted(a, &m, 0);
        break;
    case PA_BUILTIN:
    case PA_DISABLED:
    case PA_ENABLED:
        for (iz i = 0; i < countof(builtins); i++) {
            Builtin *b = builtins + i;
            if (!hasbuiltin(sh, b)) continue;
            if (act==PA_DISABLED && !isdisabled(sh, b)) continue;
            if (act==PA_ENABLED && isdisabled(sh, b)) continue;
            m = push(a, m, b->name);
        }
        break;
    case PA_EXPORT:   compvars(sh->vars, 1, a, &m); pcsorted(a, &m, 0); break;
    case PA_FUNCTION: compfuncs(sh->funcs, a, &m);  pcsorted(a, &m, 0); break;
    case PA_HELPTOPIC:
        for (iz i = 0; i < countof(helptopics); i++) m = push(a, m, helptopics[i].name);
        break;
    case PA_HOSTNAME: {
        Comp c = {};
        c.a = a;
        Slice<Str> h = comphosts(sh, &c, "");
        for (iz i = 0; i < h.len; i++) m = push(a, m, cuthead(h[i], 1));
    } break;
    case PA_JOB:
    case PA_RUNNING:
    case PA_STOPPED:
        m = pcjobs(sh, a, act);
        break;
    case PA_KEYWORD:
        for (iz i = 0; i < countof(reserved); i++) m = push(a, m, reserved[i].word);
        break;
    case PA_SETOPT:
        for (i32 k = 0; k < O_COUNT; k++) {
            if (hasoption(sh, k)) m = push(a, m, setoptions[k].name);
        }
        pcsorted(a, &m, 0);  // alphabetical, with msys's own placed
        break;
    case PA_SHOPT:
        for (i32 k = 0; k < S_COUNT; k++) {
            if (hasshopt(sh, k)) m = push(a, m, shoptnames[k]);
        }
        pcsorted(a, &m, 0);
        break;
    case PA_SIGNAL:   m = pcsignals(a); break;
    case PA_VARIABLE: compvars(sh->vars, 0, a, &m); pcsorted(a, &m, 0); break;
    }
    return m;
}

// A generator run as readline's match driver does: matches folded and
// sorted under completion-ignore-case (P§5.1)
static Slice<Str> pcdriver(Editor *ed, Arena *a, Slice<Str> m)
{
    if (ed->vars.ignorecase) pcsorted(a, &m, 0);
    return m;
}

struct PcRes {
    Slice<Str> m;
    b32        files;   // readline's "results are filenames"
    b32        retry;   // a function returned 124
    b32        notfound;  // 127
};

static Slice<Str> pcdirs(Editor *ed, Shell *sh, Arena *a, Str word, PcRes *r)
{
    Comp c = {};
    c.a = a;
    Str w = pcdequote(a, word);
    Slice<Str> f = compfiles(ed, sh, &c, w, 1);
    Slice<Str> m = {};
    for (iz i = 0; i < f.len; i++) {
        if (compstat(sh, &c, f[i], 1).type == FT_DIR) m = push(a, m, f[i]);
    }
    r->files = 1;
    return pcdriver(ed, a, m);
}

static Slice<Str> pcaction(Editor *ed, Shell *sh, Arena *a, i32 act, Str word, PcRes *r)
{
    Comp c = {};
    c.a = a;
    Str w = pcdequote(a, word);
    switch (act) {
    case PA_COMMAND:
        return pcdriver(ed, a, compcommands(ed, sh, &c, w));
    case PA_FILE:
        r->files = 1;
        return pcdriver(ed, a, compfiles(ed, sh, &c, w, 1));
    case PA_USER: {
        Slice<Str> u = compusers(sh, &c, w);
        r->files |= w.len && w[0]=='~';
        return pcdriver(ed, a, u);
    }
    case PA_GROUP: {
        Slice<Str> g = pcdbnames(sh, a, "/etc/group");
        Slice<Str> m = {};
        for (iz i = 0; i < g.len; i++) {
            if (startswith(g[i], w)) m = push(a, m, g[i]);
        }
        return pcdriver(ed, a, m);
    }
    case PA_SERVICE:
        return pcdriver(ed, a, pcservices(sh, a, w));
    case PA_DIRECTORY:
        return pcdirs(ed, sh, a, word, r);
    }
    Slice<Str> all = pclist(sh, a, act);
    Slice<Str> m   = {};
    for (iz i = 0; i < all.len; i++) {
        if (startswith(all[i], w)) m = push(a, m, all[i]);
    }
    return m;
}


// Splitting text into words (bash's split_at_delims, P§5.3, P§5.4)

// Past a quoted string or an expansion starting at s[i]
static iz pcskip(Str s, iz i)
{
    iz  n = s.len;
    u8  c = s[i];
    if (c == '\\') return i+2<n ? i+2 : n;
    if (c == '\'') {
        iz j = i + 1;
        for (; j<n && s[j]!='\''; j++) {}
        return j<n ? j+1 : n;
    }
    if (c=='"' || c=='`') {
        iz j = i + 1;
        while (j<n && s[j]!=c) {
            if (s[j] == '\\') {
                j += 2;
            } else if (c=='"' && (s[j]=='$' || s[j]=='`')) {
                j = pcskip(s, j);
            } else {
                j++;
            }
        }
        return j<n ? j+1 : n;
    }
    if (i+1 < n) {
        u8 d = s[i+1];
        if (c=='$' && d=='\'') {
            iz j = i + 2;
            for (; j<n && s[j]!='\''; j += 1 + (s[j]=='\\')) {}
            return j<n ? j+1 : n;
        }
        if (((c=='$' || c=='<' || c=='>') && d=='(') || (c=='$' && d=='{')) {
            u8 open = d, close = d=='(' ? ')' : '}';
            i32 depth = 1;
            iz  j = i + 2;
            while (j < n) {
                u8 e = s[j];
                if (e=='\\' || e=='\'' || e=='"' || e=='`' || (e=='$' && j+1<n)) {
                    iz k = pcskip(s, j);
                    if (k > j+1 || e!='$') {
                        j = k;
                        continue;
                    }
                }
                if (e == open) depth++;
                if (e==close && !--depth) return j+1;
                j++;
            }
            return n;
        }
    }
    return i + 1;
}

static b32 pcspace(u8 c)
{
    return c==' ' || c=='\t' || c=='\n';
}

// -W's words: split at IFS, quotes and expansions kept whole
static Slice<Str> pcifssplit(Arena *a, Str s, Str ifs)
{
    Slice<Str> m = {};
    iz i = 0, n = s.len;
    for (; i<n && compin(ifs, s[i]) && pcspace(s[i]); i++) {}
    while (i < n) {
        iz beg = i;
        while (i<n && !compin(ifs, s[i])) i = pcskip(s, i);
        if (i > beg) m = push(a, m, span(s.data+beg, s.data+i));
        if (i >= n) break;
        if (!pcspace(s[i])) {
            i++;
            for (; i<n && compin(ifs, s[i]) && pcspace(s[i]); i++) {}
        } else {
            for (; i<n && compin(ifs, s[i]) && pcspace(s[i]); i++) {}
            if (i<n && compin(ifs, s[i]) && !pcspace(s[i])) {
                i++;
                for (; i<n && compin(ifs, s[i]) && pcspace(s[i]); i++) {}
            }
        }
    }
    return m;
}

struct PcWords {
    Slice<Str> w;
    iz         cword;  // -1 if none
};

// COMP_WORDS: the command's text split at the word break characters,
// and the word holding point
static PcWords pcbreaksplit(Arena *a, Str s, Str breaks, iz point)
{
    PcWords r = {};
    r.cword = -1;
    Buf delims(a, breaks.len);
    for (iz i = 0; i < breaks.len; i++) {
        if (breaks[i]!='\'' && breaks[i]!='"') putbyte(&delims, breaks[i]);
    }
    Str d = finish(&delims);
    iz  i = 0, n = s.len;
    iz  lastend = -1;
    for (; i<n && compin(d, s[i]) && pcspace(s[i]); i++) {}
    while (i < n) {
        iz beg = i;
        if (!pcspace(s[i]) && compin(d, s[i])) {
            for (; i<n && compin(d, s[i]) && !pcspace(s[i]); i++) {}
        } else {
            while (i<n && !compin(d, s[i])) i = pcskip(s, i);
        }
        if (r.cword<0 && point<beg && point>lastend) {
            // Point in the blanks before this word: an empty word there
            r.cword = r.w.len;
            r.w     = push(a, r.w, Str(""));
        }
        if (r.cword<0 && point>=beg && point<=i) r.cword = r.w.len;
        r.w     = push(a, r.w, span(s.data+beg, s.data+i));
        lastend = i;
        for (; i<n && compin(d, s[i]) && pcspace(s[i]); i++) {}
    }
    if (r.cword < 0) {
        if (point>0 && point<=n && pcspace(s[point-1])) {
            r.cword = r.w.len;
            r.w     = push(a, r.w, Str(""));
        } else if (r.w.len) {
            r.cword = r.w.len - 1;
        }
    }
    return r;
}


// The pipeline (P§5)

// What a completion knows besides the word, for -F and -C
struct PcCtx {
    Str        cmd;     // $1
    Str        word;    // $2
    Str        line;    // COMP_LINE
    iz         point;   // in line, bytes
    b32        live;    // completing a line, not compgen
};

static Str pcwordbreaks(Shell *sh, Arena *a)
{
    Got g = getvar(sh, a, "COMP_WORDBREAKS");
    if (g.set) return g.value;
    return wordbreaks(sh, sh->shopt[S_HOSTCOMPLETE]);
}

static i64 pccharcount(Str s)
{
    i64 n = 0;
    for (iz i = 0; i < s.len; i++) n += (s[i] & 0xc0) != 0x80;
    return n;
}

static Str const pcvarnames[6] = {
    "COMP_LINE", "COMP_POINT", "COMP_TYPE", "COMP_KEY", "COMP_WORDS",
    "COMP_CWORD",
};

static void pcbind(Shell *sh, Arena a, PcCtx *x, PcWords *w, b32 exported)
{
    setvar(sh, a, pcvarnames[0], x->line);
    Str num = fmti64(&a, pccharcount(takehead(x->line, x->point)));
    setvar(sh, a, pcvarnames[1], num);
    Str num2 = fmti64(&a, sh->comptype);
    setvar(sh, a, pcvarnames[2], num2);
    Str num3 = fmti64(&a, sh->compkey);
    setvar(sh, a, pcvarnames[3], num3);
    if (exported) {
        for (i32 i = 0; i < 4; i++) setattrs(sh, pcvarnames[i], V_EXPORT, 0);
        return;
    }
    Value *v = rawlookup(sh, pcvarnames[4]);
    if (v && (v->attrs & V_NAMEREF)) setattrs(sh, pcvarnames[4], 0, V_NAMEREF);
    v = assignslot(sh, a, pcvarnames[4], 0);
    if (v) {
        if (v->arr && v->arr->bucket) v = 0;  // an associative array stays
        else toarray(sh, v);
    }
    if (v) v->arr->isset = 1;
    for (iz i = 0; v && i < w->w.len; i++) {
        setelem(sh, a, pcvarnames[4], i, {}, w->w[i]);
    }
    Str num4 = fmti64(&a, w->cword);
    setvar(sh, a, pcvarnames[5], num4);
}

static void pcunbind(Shell *sh)
{
    for (i32 i = 0; i < 6; i++) unsetvar(sh, pcvarnames[i], 1);
}

static Slice<Str> pcfunction(Shell *sh, Arena *a, CompSpec *cs, PcCtx *x,
                             PcWords *w, PcRes *r)
{
    Str   name = cs->str[PS_FUNC];
    Func *fn   = findfunc(sh, name);
    if (!fn || !fn->body) {
        Str msg = concat3(a, "function `", name, "' not found");
        builtinerror(sh, *a, "completion", msg);
        if (sh->editor && x->live) {
            // The message is on the line: the prompt and line again below
            edbell(sh->editor);
            compredraw(sh->editor, sh);
        }
        return {};
    }
    Str prev = {};
    if (w->cword > 0) {
        prev = w->w[w->cword-1];
    } else if (!w->cword) {
        prev = w->w[0];
    }
    pcbind(sh, *a, x, w, 0);
    Str *argv = alloc<Str>(a, 4);
    argv[0] = name;
    argv[1] = clone(a, x->cmd);
    argv[2] = clone(a, x->word);
    argv[3] = clone(a, prev.data ? prev : Str(""));
    i32 status = sh->status;
    i32 line   = sh->line;
    i32 got    = callfunction(sh, fn, argv, 4, *a);
    sh->status = status;
    sh->line   = line;
    pcunbind(sh);

    Slice<Str> m = {};
    Value *v = lookup(sh, "COMPREPLY");
    if (got == 124) {
        r->retry = 1;
    } else if (got == 127) {
        r->notfound = 1;
    } else if (v && v->arr && !v->arr->bucket) {
        Array *arr = v->arr;
        for (iz i = 0; i < arr->len; i++) m = push(a, m, clone(a, arr->e[i].val));
    } else if (v && !v->arr && v->str.data) {
        m = push(a, m, clone(a, v->str));
    }
    if (v) unsetvar(sh, "COMPREPLY", 1);
    return m;
}

static Slice<Str> pccommand(Shell *sh, Arena *a, CompSpec *cs, PcCtx *x, PcWords *w)
{
    Str prev = {};
    if (w->cword > 0) {
        prev = w->w[w->cword-1];
    } else if (!w->cword) {
        prev = w->w[0];
    }
    Buf b(a, 64);
    print(&b, cs->str[PS_CMD]);
    putbyte(&b, ' ');
    print(&b, singlequote(a, x->cmd));
    putbyte(&b, ' ');
    print(&b, singlequote(a, x->word));
    putbyte(&b, ' ');
    print(&b, singlequote(a, prev));
    Str src = finish(&b);
    pcbind(sh, *a, x, w, 1);
    i32 status = sh->status;
    Str out    = runcapture(sh, a, src, 0);
    sh->status = status;
    for (i32 i = 0; i < 4; i++) unsetvar(sh, pcvarnames[i], 1);

    // Lines, a backslash keeping the newline after it
    Slice<Str> m = {};
    iz beg = 0;
    for (iz i = 0; i <= out.len; i++) {
        if (i<out.len && out[i]=='\\' && i+1<out.len) {
            i++;
            continue;
        }
        if (i<out.len && out[i]!='\n') continue;
        if (i > beg) m = push(a, m, span(out.data+beg, out.data+i));
        beg = i + 1;
    }
    return m;
}

// -X: the pattern with & standing for the word, and its leading !
static Slice<Str> pcfilter(Shell *sh, Arena *a, Str pat, Str word, Slice<Str> m)
{
    Buf b(a, pat.len+word.len);
    for (iz i = 0; i < pat.len; i++) {
        u8 c = pat[i];
        if (c=='\\' && i+1<pat.len && pat[i+1]=='&') {
            putbyte(&b, '&');
            i++;
        } else if (c=='\\' && i+1<pat.len) {
            putbyte(&b, c);
            putbyte(&b, pat[++i]);
        } else if (c == '&') {
            for (iz k = 0; k < word.len; k++) {
                if (compin("*?[]\\+@!()|", word[k])) putbyte(&b, '\\');
                putbyte(&b, word[k]);
            }
        } else {
            putbyte(&b, c);
        }
    }
    Str p      = finish(&b);
    b32 ext    = sh->shopt[S_EXTGLOB];
    b32 negate = p.len && p[0]=='!' && !(ext && p.len>1 && p[1]=='(');
    if (negate) p = cuthead(p, 1);
    i32 flags = (ext ? PM_EXTGLOB : 0) | (sh->shopt[S_NOCASEMATCH] ? PM_NOCASE : 0);
    Slice<Str> keep = {};
    for (iz i = 0; i < m.len; i++) {
        b32 match = patmatch(p, m[i], flags, *a);
        if (match == negate) keep = push(a, keep, m[i]);
    }
    return keep;
}

static Slice<Str> pcappend(Arena *a, Slice<Str> m, Slice<Str> more)
{
    for (iz i = 0; i < more.len; i++) m = push(a, m, more[i]);
    return m;
}

// Run a compspec on the word (P§5)
static PcRes pcgenerate(Shell *sh, Arena *a, CompSpec *cs, PcCtx *x)
{
    PcRes  r  = {};
    Editor *ed = editor(sh);
    Str    word = x->word;
    for (i32 k = 0; k < PA_COUNT; k++) {
        i32 act = pgenorder[k];
        if (!(cs->actions & ((uz)1<<act))) continue;
        r.m = pcappend(a, r.m, pcaction(ed, sh, a, act, word, &r));
    }
    if (cs->has & (1u<<PS_GLOB)) {
        r.files = 1;
        Slice<Str> g = glob(sh, a, cs->str[PS_GLOB]);
        r.m = pcappend(a, r.m, g);
    }
    if (cs->has & (1u<<PS_WORDS)) {
        Got ifs = getvar(sh, a, "IFS");
        Slice<Str> raw = pcifssplit(a, cs->str[PS_WORDS], ifs.set ? ifs.value : Str(" \t\n"));
        Word  *head = 0;
        Word **tail = &head;
        for (iz i = 0; i < raw.len; i++) {
            *tail = newword(a, raw[i], 0);
            tail  = &(*tail)->next;
        }
        Slice<Str> ws = expandwords(sh, a, head, XW_NOGLOB);
        Str w = pcdequote(a, word);
        for (iz i = 0; i < ws.len; i++) {
            if (startswith(ws[i], w)) r.m = push(a, r.m, ws[i]);
        }
    }
    if (cs->has & ((1u<<PS_FUNC) | (1u<<PS_CMD))) {
        PcWords w = {};
        w.cword = -1;
        if (x->live) {
            w = pcbreaksplit(a, x->line, pcwordbreaks(sh, a), x->point);
            if (!x->cmd.len && w.w.len && w.w[0].len) {
                Slice<Str> t = {};
                t = push(a, t, Str(""));
                w.w   = pcappend(a, t, w.w);
                w.cword++;
            }
        }
        if (cs->has & (1u<<PS_FUNC)) {
            r.m = pcappend(a, r.m, pcfunction(sh, a, cs, x, &w, &r));
        }
        if (cs->has & (1u<<PS_CMD)) {
            r.m = pcappend(a, r.m, pccommand(sh, a, cs, x, &w));
        }
    }
    if (r.retry) {
        r.m = {};
        return r;
    }
    if (cs->has & (1u<<PS_FILTER)) {
        r.m = pcfilter(sh, a, cs->str[PS_FILTER], word, r.m);
    }
    if (cs->has & ((1u<<PS_PREFIX) | (1u<<PS_SUFFIX))) {
        for (iz i = 0; i < r.m.len; i++) {
            r.m[i] = concat3(a, cs->str[PS_PREFIX], r.m[i], cs->str[PS_SUFFIX]);
        }
    }
    if ((cs->options & ((uz)1<<PO_DIRNAMES)) && !r.m.len) {
        r.m = pcdirs(ed, sh, a, word, &r);
    } else if (cs->options & ((uz)1<<PO_PLUSDIRS)) {
        r.m = pcappend(a, r.m, pcdirs(ed, sh, a, word, &r));
    }
    return r;
}


// The builtins

static i32 b_complete(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    Arena *a = &scratch;
    if (nargs == 1) {
        pcprintall(sh, scratch);
        return 0;
    }
    PcOpts o = pcparse(sh, a, "complete", args, nargs);
    if (o.status) return o.status;
    if (o.hasvar) {
        invalidoption(sh, scratch, "complete", "-V");
        return 2;
    }
    Str special = {};
    if (o.flags & PF_I) special = pckeys[2];
    if (o.flags & PF_E) special = pckeys[1];
    if (o.flags & PF_D) special = pckeys[0];
    Str *names  = args + o.next;
    iz   nnames = special.data ? 0 : nargs - o.next;
    i32  status = 0;

    if ((o.flags & PF_P) || (!o.any && !nnames)) {
        if (special.data) {
            names  = &special;
            nnames = 1;
        } else if (!nnames) {
            pcprintall(sh, scratch);
            return 0;
        }
        for (iz i = 0; i < nnames; i++) {
            CompSpec *cs = pclookup(sh, names[i]);
            if (!cs) {
                builtinerror(sh, scratch, "complete", concat(a, names[i], ": no completion specification"));
                status = 1;
            } else {
                pcprintone(sh, scratch, cs, names[i]);
            }
        }
        return status;
    }
    if (o.flags & PF_R) {
        if (special.data) {
            names  = &special;
            nnames = 1;
        } else if (!nnames) {
            if (sh->comps) compsown(sh);
            if (sh->comps) {
                for (iz i = 0; i < sh->comps->nbuckets; i++) sh->comps->bucket[i] = 0;
                sh->comps->n = 0;
            }
            return 0;
        }
        for (iz i = 0; i < nnames; i++) {
            if (!pcremove(sh, names[i])) {
                builtinerror(sh, scratch, "complete", concat(a, names[i], ": no completion specification"));
                status = 1;
            }
        }
        return status;
    }
    if (!special.data && !nnames) {
        usageerror(sh, scratch, "complete");
        return 2;
    }
    CompSpec *cs = pccopyspec(sh->store, &o.cs);
    if (special.data) {
        pcinsert(sh, special, cs);
    } else {
        for (iz i = 0; i < nnames; i++) pcinsert(sh, names[i], cs);
    }
    return 0;
}

// bash's default completion on a word not in command position, for
// compgen -o bashdefault: only the $, ~, @ and glob forms (P§3.2)
static Slice<Str> pcbashdefault(Shell *sh, Arena *a, Str word)
{
    Comp c = {};
    c.a    = a;
    c.word = word;
    c.mode = CM_TAB;
    Slice<Str> m = {};
    if (word.len && word[0]=='$' && !(word.len>1 && word[1]=='(')) {
        m = compvariables(sh, &c, word);
    }
    if (!m.len && word.len && word[0]=='~' && !compin(word, '/')) {
        m = compusers(sh, &c, word);
    }
    if (!m.len && word.len && word[0]=='@' && sh->shopt[S_HOSTCOMPLETE]) {
        m = comphosts(sh, &c, word);
    }
    if (!m.len && compglobchars(word)) {
        m = compglob(sh, &c, word);
    }
    return m;
}

static i32 b_compgen(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    Arena *a = &scratch;
    PcOpts o = pcparse(sh, a, "compgen", args, nargs);
    if (o.status) return o.status;
    u32 bad = o.flags & (PF_P|PF_R|PF_D|PF_E|PF_I);
    if (bad) {
        // The first in getopt's letter order
        Str letter = bad&PF_P ? Str("-p") : bad&PF_R ? Str("-r")
                   : bad&PF_D ? Str("-D") : bad&PF_E ? Str("-E") : Str("-I");
        invalidoption(sh, scratch, "compgen", letter);
        return 2;
    }
    if (o.hasvar && !validvarname(o.var)) {
        notident(sh, scratch, "compgen", o.var);
        return 2;
    }
    if (!o.any) return 0;
    Str word = o.next<nargs ? args[o.next] : Str("");
    if (o.cs.has & (1u<<PS_FUNC)) {
        builtinerror(sh, scratch, "compgen", "warning: -F option may not work as you expect");
    }
    if (o.cs.has & (1u<<PS_CMD)) {
        builtinerror(sh, scratch, "compgen", "warning: -C option may not work as you expect");
    }

    PcCtx x = {};
    x.cmd  = "compgen";
    x.word = word;
    x.line = "";
    PcRes r = pcgenerate(sh, a, &o.cs, &x);
    Slice<Str> m = r.m;
    if (!m.len && (o.cs.options & ((uz)1<<PO_BASHDEFAULT))) {
        m = pcbashdefault(sh, a, word);
    }
    if (!m.len && (o.cs.options & ((uz)1<<PO_DEFAULT))) {
        Comp c = {};
        c.a = a;
        m = compfiles(editor(sh), sh, &c, word, 1);
    }

    if (o.hasvar) {
        Value *v = assignslot(sh, scratch, o.var, 0);
        if (!v) return 1;
        if (v->arr && v->arr->bucket) {
            builtinerror(sh, scratch, "compgen", concat(a, o.var, ": not an indexed array"));
            return 1;
        }
        toarray(sh, v);
        aclear(sh->store, v->arr);
        v->arr->isset = 1;
        for (iz i = 0; i < m.len; i++) setelem(sh, scratch, o.var, i, {}, m[i]);
        return !m.len;
    }
    for (iz i = 0; i < m.len; i++) {
        bprint(sh, m[i]);
        bprint(sh, "\n");
    }
    return !m.len;
}

static i32 b_compopt(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    Arena *a = &scratch;
    uz  on = 0, off = 0;
    u32 flags = 0;
    iz  i = 1;
    for (; i < nargs; i++) {
        Str w = args[i];
        if (w.len<2 || (w[0]!='-' && w[0]!='+')) break;
        if (w == "--") {
            i++;
            break;
        }
        b32 plus = w[0] == '+';
        for (iz k = 1; k < w.len; k++) {
            u8 c = w[k];
            switch (c) {
            case 'D': flags |= PF_D; continue;
            case 'E': flags |= PF_E; continue;
            case 'I': flags |= PF_I; continue;
            case 'o': break;
            default: {
                u8 opt[2] = {w[0], c};
                invalidoption(sh, scratch, "compopt", clone(a, Str{opt, 2}));
                return 2;
            }
            }
            Str opt;
            if (k+1 < w.len) {
                opt = cuthead(w, k+1);
            } else if (i+1 < nargs) {
                opt = args[++i];
            } else {
                u8 o[2] = {w[0], c};
                builtinerror(sh, scratch, "compopt", concat(a, clone(a, Str{o, 2}), ": option requires an argument"));
                usageerror(sh, scratch, "compopt");
                return 2;
            }
            k = w.len;
            i32 o = -1;
            for (i32 j = 0; j < PO_COUNT; j++) {
                if (poptions[j] == opt) o = j;
            }
            if (o < 0) {
                builtinerror(sh, scratch, "compopt", concat(a, opt, ": invalid option name"));
                return 2;
            }
            if (plus) off |= (uz)1 << o; else on |= (uz)1 << o;
        }
    }
    Str special = {};
    if (flags & PF_I) special = pckeys[2];
    if (flags & PF_E) special = pckeys[1];
    if (flags & PF_D) special = pckeys[0];
    Str *names  = args + i;
    iz   nnames = nargs - i;
    if (special.data) {
        names  = &special;
        nnames = 1;
    }
    if (!nnames) {
        CompSpec *cs = sh->compcur;
        if (!cs) {
            builtinerror(sh, scratch, "compopt", "not currently executing completion function");
            return 1;
        }
        if (!on && !off) {
            pcprintopts(sh, scratch, cs->options, sh->compcurkey);
        } else {
            cs->options = (cs->options | on) & ~off;
        }
        return 0;
    }
    i32 status = 0;
    for (iz k = 0; k < nnames; k++) {
        if (!pclookup(sh, names[k])) {
            builtinerror(sh, scratch, "compopt", concat(a, names[k], ": no completion specification"));
            status = 1;
            continue;
        }
        if (!on && !off) {
            pcprintopts(sh, scratch, pclookup(sh, names[k])->options, names[k]);
            continue;
        }
        compsown(sh);
        CompSpec *cs = pclookup(sh, names[k]);
        cs->options = (cs->options | on) & ~off;
    }
    return status;
}


// Completing a line (P§8)

// Is s[i] a separator ending a simple command, as bash's command
// finder sees it: ; | & ( ` and a { where a reserved word may be
static b32 pcsep(Str s, iz i, iz cmdstart)
{
    u8 c = s[i];
    if (c=='|' && i>0 && s[i-1]=='>') return 0;  // >|
    if (c==';' || c=='|' || c=='&' || c=='(' || c=='`') return 1;
    if (c != '{') return 0;
    if (i+1<s.len && !compin(" \t\n;|&()<>", s[i+1])) return 0;
    for (iz j = cmdstart; j < i; j++) {
        if (!pcspace(s[j])) return 0;
    }
    return 1;
}

// Past what a scan for separators takes whole: quotes, $(...), ${...}
static iz pcscan(Str s, iz i)
{
    u8 c = s[i];
    if (c=='\\' || c=='\'' || c=='"') return pcskip(s, i);
    if (c=='$' && i+1<s.len && compin("({'", s[i+1])) return pcskip(s, i);
    return i + 1;
}

static iz pccmdstart(Str s, iz upto)
{
    iz start = 0;
    for (iz i = 0; i < upto;) {
        if (pcsep(s, i, start)) {
            start = ++i;
            continue;
        }
        i = pcscan(s, i);
    }
    return start;
}

static iz pccmdend(Str s, iz from, iz start)
{
    for (iz i = start; i < s.len;) {
        if (i>=from && pcsep(s, i, start)) return i;
        if (pcsep(s, i, start)) start = i + 1;
        i = pcscan(s, i);
    }
    return s.len;
}

struct PcLive {
    b32        found;
    Slice<Str> m;
    b32        files;
    uz         options;
};

// Run the compspec chosen for cmd, as bash's lookup loop: an exact
// name, its basename, then -D, again after a function returns 124
static PcLive pcrun(Shell *sh, Arena *a, PcCtx *x)
{
    PcLive    out  = {};
    CompSpec *prev = 0;
    i32       pass = 0;
    Str       orig = x->cmd;
    for (; pass < 33; pass++) {
        Str       key = x->cmd;
        CompSpec *cs  = pclookup(sh, key);
        if (cs == prev) cs = 0;
        if (!cs) {
            iz slash = -1;
            for (iz i = 0; i < x->cmd.len; i++) {
                if (x->cmd[i] == '/') slash = i;
            }
            if (slash>=0 && slash+1<x->cmd.len) {
                key = cuthead(x->cmd, slash+1);
                cs  = pclookup(sh, key);
                if (cs == prev) cs = 0;
            }
        }
        if (!cs) {
            key = pckeys[0];
            cs  = pclookup(sh, key);
            if (cs == prev) cs = 0;
        }
        if (!cs) {
            // progcomp_alias: the alias's value in place of the name,
            // its first word the command, tried again (a value that
            // starts with the name itself loops until the warning)
            Alias *al = 0;
            if (sh->shopt[S_PROGCOMP_ALIAS] && x->live && startswith(x->line, x->cmd)) {
                al = findalias(sh->aliases, x->cmd);
            }
            if (!al || !al->value.len) break;
            Str v = al->value;
            iz  n = 0;
            while (n<v.len && !compin("()<>;&| \t\n", v[n])) n = pcscan(v, n);
            if (!n) break;
            x->line   = concat(a, v, cuthead(x->line, x->cmd.len));
            x->point += v.len - x->cmd.len;
            x->cmd    = takehead(v, n);
            continue;
        }
        CompSpec copy = *cs;
        CompSpec *was = sh->compcur;
        Str       wkey = sh->compcurkey;
        sh->compcur    = &copy;
        sh->compcurkey = key;
        PcRes r = pcgenerate(sh, a, &copy, x);
        sh->compcur    = was;
        sh->compcurkey = wkey;
        if (r.retry) {
            prev = cs;
            continue;
        }
        if (r.notfound && !copy.options) break;
        out.found   = 1;
        out.m       = r.m;
        out.files   = r.files;
        out.options = copy.options;
        return out;
    }
    if (pass == 33) {
        Str msg = concat3(a, "warning: programmable_completion: ", orig, ": possible retry loop");
        builtinerror(sh, *a, "", msg);
    }
    return out;
}

// Programmable completion of the word in c, if a compspec applies.
// *cmdpos is the classifier's command position, which this refines.
static PcLive pclive(Editor *ed, Shell *sh, Comp *c, b32 *cmdpos)
{
    PcLive out = {};
    if (!sh->shopt[S_PROGCOMP] || !sh->comps || !sh->comps->n) return out;
    b32 haveI = pclookup(sh, pckeys[2]) != 0;
    if (*cmdpos && c->word.len && !haveI) return out;

    Arena *a     = c->a;
    Str    line  = clone(a, edtext(ed));
    iz     ws    = c->start;
    iz     point = c->end;
    Str    word  = c->word;
    iz     s     = pccmdstart(line, ws);
    iz     e     = pccmdend(line, point, s);

    // Assignments before the command name are passed over
    iz  i = s, s1 = -1, e1 = -1;
    b32 assign = 0;
    for (;;) {
        for (; i<e && pcspace(line[i]); i++) {}
        iz b = i;
        while (i<e && !compin("()<>;&| \t\n", line[i])) i = pcscan(line, i);
        if (i == b) break;
        if (compassign(span(line.data+b, line.data+i))) {
            assign = 1;
            if (i >= point) break;
            continue;
        }
        s1 = b;
        e1 = i;
        break;
    }
    b32 hasname = s1 >= 0;
    Str name    = hasname ? span(line.data+s1, line.data+e1) : Str{};
    iz  cs      = hasname ? s1 : i;  // the command's start, past blanks

    PcCtx x = {};
    x.word = word;
    x.live = 1;
    Str key = {};
    if (!word.len && !ws && !point && hasname && !s1) {
        *cmdpos = 1;  // before the command name, at column 0
    } else if (!word.len && !point && (hasname ? s1>0 : !assign && !s)) {
        key = pckeys[1];  // an empty line, or blanks then a command
    } else if (!word.len && hasname && point<s1) {
        // in the blanks before the command name
    } else if (hasname && s1==ws && e1==point && e==line.len &&
               line.len && !pcspace(line[line.len-1])) {
        *cmdpos = name == word;  // the command name itself, at the end
    } else if (hasname && !(assign && s1>=point)) {
        key = name;
        *cmdpos = s1==ws && (haveI || name==word);
        if (*cmdpos && haveI) key = {};
    } else if (!hasname && !word.len && !assign) {
        *cmdpos = 1;  // after a separator
    }

    iz start = cs<point ? cs : point;
    iz end   = e<line.len ? e : line.len;
    if (key.data) {
        x.cmd   = key;
        x.line  = span(line.data+start, line.data+end);
        x.point = point - start;
        out = pcrun(sh, a, &x);
    }
    // Not after assignments alone (X=1 then TAB)
    if (!out.found && *cmdpos && haveI && !(assign && !hasname)) {
        x.cmd   = pckeys[2];
        x.line  = span(line.data+start, line.data+end);
        x.point = point - start;
        out = pcrun(sh, a, &x);
    }
    return out;
}

// pclive, guarded: an expansion error abandons the completion
static PcLive pclivesafe(Editor *ed, Shell *sh, Comp *c, b32 *cmdpos)
{
    Escape e = {};
    e.up    = sh->esc;
    e.bout  = sh->bout;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    i32 depth  = sh->capdepth;
    i32 status = sh->status;  // $? and PIPESTATUS kept, as bash's
    i32 npstat = sh->npstat;
    u8  pstat[countof(sh->pstat)];
    copybytes(pstat, sh->pstat, countof(pstat));
    PcLive r = {};
    if (!jmpset(e.jmp)) {
        r = pclive(ed, sh, c, cmdpos);
    } else {
        unwind(sh, e.undo);
        sh->esc      = e.up;
        sh->capdepth = depth;
        if (e.why==ESC_EXIT || e.why==ESC_INTR) escape(sh, e.why);
        r = {};
        r.found = 1;  // nothing, and no other completion
    }
    sh->esc    = e.up;
    sh->status = status;
    if (!sh->pstatdirty) {
        sh->npstat = npstat;
        copybytes(sh->pstat, pstat, countof(pstat));
    }
    return r;
}

// The editor's hook (complete.cpp): the matches when a compspec
// applies, its options set on c. *how: 0 no compspec, 1 these are the
// matches, 2 bash's default completion next (bashdefault, P§8.3).
static Slice<Str> pcomplete(Editor *ed, Shell *sh, Comp *c, b32 *cmdpos, i32 *how)
{
    *how = 0;
    PcLive p = pclivesafe(ed, sh, c, cmdpos);
    if (!p.found) return {};
    uz o = p.options;
    c->files     = p.files || ((o>>PO_FILENAMES) & 1);
    c->noqfn     = (o>>PO_NOQUOTE) & 1;
    c->noappend  = (o>>PO_NOSPACE) & 1;
    c->nosort    = (o>>PO_NOSORT) & 1;
    c->fullquote = (o>>PO_FULLQUOTE) & 1;
    *how = 1;
    if (p.m.len) return p.m;
    if ((o>>PO_BASHDEFAULT) & 1) {
        *how = 2;
        c->nofiles = !((o>>PO_DEFAULT) & 1);
        return {};
    }
    if ((o>>PO_DEFAULT) & 1) {
        c->files = 1;
        return compfiles(ed, sh, c, compbare(c), 1);
    }
    return {};
}
