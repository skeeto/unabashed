// The init file: readline's inputrc reader, with its variables, key
// bindings, macros and conditionals (notes/research/inputrc.md, cited
// as §N). The bind builtin is M8.
// This is free and unencumbered software released into the public domain.


// Variables (§8)

enum : i32 { RV_BOOL, RV_INT, RV_STR, RV_SEQ, RV_BELL, RV_MODE, RV_KEYMAP };

struct RlVar {
    Str   name;
    i32   type;
    void *p;     // b32, i32 or Str
};

enum : i32 { KM_EMACS, KM_META, KM_CTLX, KM_VIMOVE, KM_VIINS };

static Str rlkeymapnames[][2] = {
    {"emacs", "emacs-standard"}, {"emacs-meta", ""}, {"emacs-ctlx", ""},
    {"vi", "vi-move"}, {"vi-insert", ""},
};

// The table, with pointers into this editor
static iz rlvars(Editor *ed, RlVar *t)
{
    EdVars *v = &ed->vars;
    RlVar tab[] = {
        {"bind-tty-special-chars",          RV_BOOL, &v->ttyspecial},
        {"blink-matching-paren",            RV_BOOL, &v->blink},
        {"byte-oriented",                   RV_BOOL, &v->byteoriented},
        {"colored-completion-prefix",       RV_BOOL, &v->colprefix},
        {"colored-stats",                   RV_BOOL, &v->colstats},
        {"completion-ignore-case",          RV_BOOL, &v->ignorecase},
        {"completion-map-case",             RV_BOOL, &v->mapcase},
        {"convert-meta",                    RV_BOOL, &v->convertmeta},
        {"disable-completion",              RV_BOOL, &v->nocompletion},
        {"echo-control-characters",         RV_BOOL, &v->echoctl},
        {"enable-active-region",            RV_BOOL, &v->activeregion},
        {"enable-bracketed-paste",          RV_BOOL, &v->bracketpaste},
        {"enable-keypad",                   RV_BOOL, &v->keypad},
        {"enable-meta-key",                 RV_BOOL, &v->metakey},
        {"expand-tilde",                    RV_BOOL, &v->expandtilde},
        {"force-meta-prefix",               RV_BOOL, &v->forcemeta},
        {"history-preserve-point",          RV_BOOL, &v->histpoint},
        {"horizontal-scroll-mode",          RV_BOOL, &v->hscrollmode},
        {"input-meta",                      RV_BOOL, &v->inputmeta},
        {"mark-directories",                RV_BOOL, &v->markdirs},
        {"mark-modified-lines",             RV_BOOL, &v->markmodified},
        {"mark-symlinked-directories",      RV_BOOL, &v->marksymdirs},
        {"match-hidden-files",              RV_BOOL, &v->matchhidden},
        {"menu-complete-display-prefix",    RV_BOOL, &v->menuprefix},
        {"meta-flag",                       RV_BOOL, &v->inputmeta},
        {"output-meta",                     RV_BOOL, &v->outputmeta},
        {"page-completions",                RV_BOOL, &v->pagecompl},
        {"prefer-visible-bell",             RV_BOOL, &v->visiblebell},
        {"print-completions-horizontally",  RV_BOOL, &v->horizontal},
        {"revert-all-at-newline",           RV_BOOL, &v->revertall},
        {"search-ignore-case",              RV_BOOL, &v->searchcase},
        {"show-all-if-ambiguous",           RV_BOOL, &v->showall},
        {"show-all-if-unmodified",          RV_BOOL, &v->showunmod},
        {"show-mode-in-prompt",             RV_BOOL, &v->showmode},
        {"skip-completed-text",             RV_BOOL, &v->skipcompleted},
        {"visible-stats",                   RV_BOOL, &v->visiblestats},
        {"active-region-end-color",         RV_SEQ,  &v->regionend},
        {"active-region-start-color",       RV_SEQ,  &v->regionstart},
        {"bell-style",                      RV_BELL, &v->bellstyle},
        {"comment-begin",                   RV_STR,  &v->comment},
        {"completion-display-width",        RV_INT,  &v->displaywidth},
        {"completion-prefix-display-length",RV_INT,  &v->prefixdisplay},
        {"completion-query-items",          RV_INT,  &v->queryitems},
        {"editing-mode",                    RV_MODE, &v->vimode},
        {"emacs-mode-string",               RV_SEQ,  &v->emacsstr},
        {"history-size",                    RV_INT,  &v->histsize},
        {"isearch-terminators",             RV_SEQ,  &v->isterm},
        {"keymap",                          RV_KEYMAP, &v->keymap},
        {"keyseq-timeout",                  RV_INT,  &ed->keytimeout},
        {"vi-cmd-mode-string",              RV_SEQ,  &v->vicmdstr},
        {"vi-ins-mode-string",              RV_SEQ,  &v->viinsstr},
    };
    for (iz i = 0; i < countof(tab); i++) t[i] = tab[i];
    return countof(tab);
}

enum { RLVARMAX = 64 };

static b32 rlequal(Str a, Str b)  // case-insensitive
{
    if (a.len != b.len) return 0;
    for (iz i = 0; i < a.len; i++) {
        if (complower(a[i]) != complower(b[i])) return 0;
    }
    return 1;
}

static RlVar rlfindvar(Editor *ed, Str name)
{
    RlVar t[RLVARMAX];
    iz    n = rlvars(ed, t);
    for (iz i = 0; i < n; i++) {
        if (rlequal(t[i].name, name)) return t[i];
    }
    return {};
}

// The value as bind -v prints it
static Str rlvarvalue(Editor *ed, Arena *a, RlVar v)
{
    switch (v.type) {
    case RV_BOOL: return *(b32 *)v.p ? Str("on") : Str("off");
    case RV_INT: {
        Buf b(a, 24);
        print(&b, (i64)*(i32 *)v.p);
        return finish(&b);
    }
    case RV_STR:
    case RV_SEQ:  return *(Str *)v.p;
    case RV_BELL: {
        i32 s = *(i32 *)v.p;
        return s==0 ? Str("none") : s==1 ? Str("visible") : Str("audible");
    }
    case RV_MODE: return *(i32 *)v.p ? Str("vi") : Str("emacs");
    case RV_KEYMAP: return rlkeymapnames[ed->vars.keymap][0];
    }
    return {};
}

static i32 rlatoi(Str s)
{
    iz  i = 0;
    while (i<s.len && (s[i]==' ' || s[i]=='\t')) i++;
    b32 neg = i<s.len && s[i]=='-';
    i += i<s.len && (s[i]=='-' || s[i]=='+');
    i64 v = 0;
    for (; i<s.len && s[i]>='0' && s[i]<='9'; i++) {
        v = v*10 + s[i] - '0';
        if (v > 0x7fffffff) v = 0x7fffffff;
    }
    return (i32)(neg ? -v : v);
}


// Key sequences (§4.3)

static i32 rlhex(u8 c)
{
    if (c>='0' && c<='9') return c - '0';
    if (c>='a' && c<='f') return c - 'a' + 10;
    if (c>='A' && c<='F') return c - 'A' + 10;
    return -1;
}

// Translate one key at s[*i]: prefixes \C- and \M-, then a character or
// an escape.
static u8 rlkey1(Str s, iz *ip)
{
    iz  i    = *ip;
    b32 ctl  = 0, meta = 0;
    for (;;) {
        if (i+2<s.len && s[i]=='\\' && s[i+1]=='C' && s[i+2]=='-') {
            ctl = 1;
            i += 3;
        } else if (i+2<s.len && s[i]=='\\' && s[i+1]=='M' && s[i+2]=='-') {
            meta = 1;
            i += 3;
        } else {
            break;
        }
    }
    u8 c = 0;
    if (i >= s.len) {
        c = 0;  // a prefix at the end: NUL
    } else if (s[i]!='\\' || i+1==s.len) {
        c = s[i++];
    } else {
        u8 e = s[i+1];
        i += 2;
        switch (e) {
        case 'a': c = 0x07; break;
        case 'b': c = 0x08; break;
        case 'd': c = 0x7f; break;
        case 'e': c = 0x1b; break;
        case 'f': c = 0x0c; break;
        case 'n': c = 0x0a; break;
        case 'r': c = 0x0d; break;
        case 't': c = 0x09; break;
        case 'v': c = 0x0b; break;
        case 'x': {
            i32 v = 0, n = 0;
            for (; n<2 && i<s.len && rlhex(s[i])>=0; n++, i++) v = v*16 + rlhex(s[i]);
            c = n ? (u8)v : 'x';
        } break;
        default:
            if (e>='0' && e<='7') {
                i32 v = e - '0';
                for (i32 n = 0; n<2 && i<s.len && s[i]>='0' && s[i]<='7'; n++, i++) {
                    v = v*8 + s[i] - '0';
                }
                c = (u8)v;
            } else {
                c = e;
            }
        }
    }
    if (ctl) {
        c = c=='?' ? 0x7f : (u8)((c>='a' && c<='z' ? c-32 : c) & 0x1f);
    }
    if (meta) c |= 0x80;
    *ip = i;
    return c;
}

static Str rlkeyseq(Arena *a, Str s)
{
    Buf b(a, s.len+1);
    for (iz i = 0; i < s.len;) putbyte(&b, rlkey1(s, &i));
    return finish(&b);
}


// Binding

// The node a keymap binds into
static KeyNode *rlkeymap(Editor *ed, i32 km)
{
    switch (km) {
    case KM_META:   return keyget(&ed->perm, &ed->root, 0x1b);
    case KM_CTLX:   return keyget(&ed->perm, &ed->root, 0x18);
    case KM_VIMOVE: return &ed->vimove;
    case KM_VIINS:  return &ed->viins;
    }
    return &ed->root;
}

// Bind seq in the current keymap, high bytes as ESC sequences when the
// meta settings say so (§4.4).
static void rlbind(Editor *ed, Str seq, i32 fn, Str macro)
{
    if (!seq.len) return;
    Arena *a = &ed->perm;
    b32 conv = ed->vars.convertmeta || ed->vars.forcemeta;
    Buf b(a, seq.len*2);
    for (iz i = 0; i < seq.len; i++) {
        u8 c = seq[i];
        if (conv && c>=0x80) {
            putbyte(&b, 0x1b);
            putbyte(&b, c & 0x7f);
        } else {
            putbyte(&b, c);
        }
    }
    Str s = finish(&b);
    KeyNode *n = rlkeymap(ed, ed->vars.keymap);
    for (iz i = 0; i < s.len; i++) {
        KeyNode *k = keychild(n, s[i]);
        if (!k) {
            // A binding of the prefix stays as its shadow, except
            // do-lowercase-version
            if (n->fn == EF_DO_LOWERCASE_VERSION) n->fn = EF_NONE;
            k = keyget(a, n, s[i]);
        }
        n = k;
    }
    n->fn    = fn;
    n->macro = macro.len ? clone(a, macro) : Str{};
}

static i32 rlfunction(Str name)
{
    if (!name.len) return EF_NONE;
    if (rlequal(name, "insert-last-argument")) return EF_YANK_LAST_ARG;
    for (i32 i = 1; i < EF_COUNT; i++) {
        if (rlequal(edfuncs[i], name)) return i;
    }
    return EF_NONE;
}


// The reader

struct RlRead {
    Str  file;    // as given
    iz   line;
    i32  level;
};

static void rlerror(Editor *ed, Shell *sh, RlRead *r, Str msg, Arena *a)
{
    Buf b(a, msg.len+64);
    print(&b, "readline: ");
    if (ed->rlreading) {
        print(&b, r->file);
        print(&b, ": line ");
        print(&b, (i64)r->line);
        print(&b, ": ");
    }
    print(&b, msg);
    print(&b, "\n");
    fdwrite(sh, 2, finish(&b));
}

static b32 rlblank(u8 c)
{
    return c==' ' || c=='\t';
}

static Str rlskip(Str s)
{
    iz i = 0;
    while (i<s.len && rlblank(s[i])) i++;
    return cuthead(s, i);
}

static Str rlword(Str s)
{
    iz i = 0;
    while (i<s.len && !rlblank(s[i])) i++;
    return takehead(s, i);
}

static b32 rlreadfile(Editor *, Shell *, Str name, i32 level, Arena);

// Evaluate an $if test (§5.1)
static b32 rliftest(Editor *ed, Shell *sh, RlRead *r, Str args, Arena a)
{
    Str w = rlword(args);
    if (w.len>=5 && rlequal(takehead(w, 5), "term=")) {
        Got t = getvar(sh, &a, "TERM");
        if (!t.set) return 0;
        Str name = cuthead(w, 5);
        Str full = t.value;
        return rlequal(name, full) || rlequal(name, cut(full, '-').head);
    }
    if (w.len>=5 && rlequal(takehead(w, 5), "mode=")) {
        Str m = cuthead(w, 5);
        return ed->vars.vimode ? rlequal(m, "vi") : rlequal(m, "emacs");
    }
    if (w.len>=7 && rlequal(takehead(w, 7), "version")) {
        Str s = rlskip(cuthead(args, 7));
        iz  i = 0;
        i32 op = 0;  // 1 = 2 != 3 < 4 <= 5 > 6 >=
        if (i<s.len && s[i]=='=') {
            op = 1;
            i += 1 + (i+1<s.len && s[i+1]=='=');
        } else if (i+1<s.len && s[i]=='!' && s[i+1]=='=') {
            op = 2, i += 2;
        } else if (i<s.len && s[i]=='<') {
            op = i+1<s.len && s[i+1]=='=' ? 4 : 3;
            i += op==4 ? 2 : 1;
        } else if (i<s.len && s[i]=='>') {
            op = i+1<s.len && s[i+1]=='=' ? 6 : 5;
            i += op==6 ? 2 : 1;
        }
        if (!op) {
            Str f = s.len ? s : Str("end-of-line");
            rlerror(ed, sh, r, concat3(&a, "comparison operator expected, found `", f, "'"), &a);
            return 0;
        }
        s = rlskip(cuthead(s, i));
        if (!s.len || s[0]<'0' || s[0]>'9') {
            Str f = s.len ? s : Str("end-of-line");
            rlerror(ed, sh, r, concat3(&a, "numeric argument expected, found `", f, "'"), &a);
            return 0;
        }
        i = 0;
        i64 major = 0, minor = 0;
        for (; i<s.len && s[i]>='0' && s[i]<='9'; i++) major = major*10 + s[i] - '0';
        if (i<s.len && s[i]=='.') {
            i++;
            if (i<s.len && (s[i]<'0' || s[i]>'9') && !rlblank(s[i]) && s[i]!='#') {
                rlerror(ed, sh, r, concat3(&a, "numeric argument expected, found `", cuthead(s, i), "'"), &a);
                return 0;
            }
            for (; i<s.len && s[i]>='0' && s[i]<='9'; i++) minor = minor*10 + s[i] - '0';
        }
        Str rest = cuthead(s, i);
        Str tr   = rlskip(rest);
        if (tr.len && tr[0]!='#') {
            rlerror(ed, sh, r, concat3(&a, "trailing garbage on line: `", rest, "'"), &a);
            return 0;
        }
        i64 v = major*10 + minor, rv = 83;
        switch (op) {
        case 1: return rv == v;
        case 2: return rv != v;
        case 3: return rv <  v;
        case 4: return rv <= v;
        case 5: return rv >  v;
        case 6: return rv >= v;
        }
        return 0;
    }
    if (rlequal(w, "bash")) return 1;
    RlVar v = rlfindvar(ed, w);
    if (v.name.len && w.len) {
        Str s = rlskip(cuthead(args, w.len));
        Str o = rlword(s);
        b32 ne;
        if (o=="=" || o=="==") {
            ne = 0;
        } else if (o == "!=") {
            ne = 1;
        } else {
            Str f = s.len ? s : Str("end-of-line");
            rlerror(ed, sh, r, concat3(&a, "equality comparison operator expected, found `", f, "'"), &a);
            return 0;
        }
        Str val = rlword(rlskip(cuthead(s, o.len)));
        if (!val.len) {
            rlerror(ed, sh, r, "argument expected, found `'", &a);
            return 0;
        }
        ed->vars.histsize = (i32)sh->hist.max;  // HISTSIZE may have set it
        Str cur = rlvarvalue(ed, &a, v);
        if (!cur.data) return 0;
        return rlequal(cur, val) != ne;
    }
    return 0;
}

static void rldirective(Editor *ed, Shell *sh, RlRead *r, Str s, Arena a)
{
    s = rlskip(s);
    Str d    = rlword(s);
    Str args = rlskip(cuthead(s, d.len));
    if (rlequal(d, "if")) {
        ed->ifstack = push(&ed->perm, ed->ifstack, ed->ifoff);
        if (!ed->ifoff) ed->ifoff = !rliftest(ed, sh, r, args, a);
    } else if (rlequal(d, "else")) {
        if (!ed->ifstack.len) {
            rlerror(ed, sh, r, "$else found without matching $if", &a);
            return;
        }
        for (iz i = 0; i < ed->ifstack.len; i++) {
            if (ed->ifstack[i]) return;
        }
        ed->ifoff = !ed->ifoff;
    } else if (rlequal(d, "endif")) {
        if (!ed->ifstack.len) {
            rlerror(ed, sh, r, "$endif without matching $if", &a);
            return;
        }
        ed->ifoff = ed->ifstack[ed->ifstack.len-1];
        ed->ifstack.len--;
    } else if (rlequal(d, "include")) {
        if (ed->ifoff) return;
        if (r->level < 32) {
            rlreadfile(ed, sh, s.len>d.len ? rlskip(cuthead(s, d.len)) : Str(""), r->level+1, a);
            ed->rlreading = 0;  // as readline: the includer's later
                                // messages lose their location
        }
    } else {
        rlerror(ed, sh, r, concat(&a, d, ": unknown parser directive"), &a);
    }
}

// set NAME VALUE (§3)
static void rlset(Editor *ed, Shell *sh, RlRead *r, Str s, Arena a)
{
    s = rlskip(s);
    Str name  = rlword(s);
    Str value = rlskip(cuthead(s, name.len));
    RlVar v = rlfindvar(ed, name);
    if (!v.name.len) {
        rlerror(ed, sh, r, concat(&a, name, ": unknown variable name"), &a);
        return;
    }
    if (v.type == RV_BOOL) {
        Str w  = rlword(value);
        b32 on = !w.len || rlequal(w, "on") || w=="1";
        *(b32 *)v.p = on;
        if (v.p == &ed->vars.visiblebell) ed->vars.bellstyle = on ? 1 : 2;
        if (v.p == &ed->vars.bracketpaste) ed->vars.activeregion = on;
        return;
    }
    if (value.len && value[0]=='"') {
        iz i = 1;
        for (; i<value.len && value[i]!='"'; i++) {
            if (value[i]=='\\' && i+1<value.len) i++;
        }
        value = span(value.data+1, value.data+i);
    } else {
        while (value.len && rlblank(value[value.len-1])) value.len--;
    }
    b32 ok = 1;
    switch (v.type) {
    case RV_INT: {
        i32 n = rlatoi(value);
        i32 *p = (i32 *)v.p;
        if (p == &ed->vars.displaywidth) {
            *p = value.len ? n : -1;
        } else if (p == &ed->vars.queryitems) {
            *p = value.len ? (n<0 ? 0 : n) : 100;
        } else if (p == &ed->vars.prefixdisplay) {
            *p = n<0 ? 0 : n;
        } else if (p == &ed->vars.histsize) {
            // Readline's list is the shell's, apart from HISTSIZE
            *p = value.len ? n : 500;
            Hist *h = &sh->hist;
            histstifle(sh, *p<0 ? -1 : *p);
            if (h->session > h->len) h->session = h->len;
        } else {
            *p = n<0 ? 0 : n;  // keyseq-timeout
        }
    } break;
    case RV_STR:
        if (!value.len) {
            ok = 0;
        } else {
            *(Str *)v.p = clone(&ed->perm, value);
        }
        break;
    case RV_SEQ:
        if (v.p == &ed->vars.isterm) {
            Str t = value;
            if (t.len && (t[0]=='"' || t[0]=='\'')) {
                iz i = 1;
                for (; i<t.len && t[i]!=t[0]; i++) {
                    if (t[i]=='\\' && i+1<t.len) i++;
                }
                t = span(t.data+1, t.data+i);
            } else {
                t = rlword(t);
            }
            value = t;
        }
        *(Str *)v.p = rlkeyseq(&ed->perm, value);
        break;
    case RV_BELL:
        if (!value.len || rlequal(value, "audible") || rlequal(value, "on")) {
            ed->vars.bellstyle = 2;
        } else if (rlequal(value, "none") || rlequal(value, "off")) {
            ed->vars.bellstyle = 0;
        } else if (rlequal(value, "visible")) {
            ed->vars.bellstyle = 1;
        } else {
            ok = 0;
        }
        break;
    case RV_MODE:
        if (value.len>=2 && rlequal(takehead(value, 2), "vi")) {
            ed->vars.vimode = 1;
            ed->vars.keymap = KM_VIINS;
        } else if (value.len>=5 && rlequal(takehead(value, 5), "emacs")) {
            ed->vars.vimode = 0;
            ed->vars.keymap = KM_EMACS;
        } else {
            ok = 0;
        }
        break;
    case RV_KEYMAP: {
        static struct { Str name; i32 km; } maps[] = {
            {"emacs", KM_EMACS}, {"emacs-standard", KM_EMACS},
            {"emacs-meta", KM_META}, {"emacs-ctlx", KM_CTLX},
            {"vi", KM_VIMOVE}, {"vi-move", KM_VIMOVE},
            {"vi-command", KM_VIMOVE}, {"vi-insert", KM_VIINS},
        };
        ok = 0;
        for (iz i = 0; i < countof(maps); i++) {
            if (rlequal(maps[i].name, value)) {
                ed->vars.keymap = maps[i].km;
                ok = 1;
            }
        }
    } break;
    }
    if (!ok) {
        rlerror(ed, sh, r, concat(&a, concat3(&a, name, ": could not set value to `", value), "'"), &a);
    }
}

// Keyname: function (§4.2): the one key it names, or -1
static i32 rlkeyname(Editor *ed, Shell *sh, RlRead *r, Str k, Arena a)
{
    iz dash = -1;
    for (iz i = 0; i < k.len; i++) {
        if (k[i] == '-') dash = i;
    }
    Str base = cuthead(k, dash+1);
    static struct { Str name; u8 key; } names[] = {
        {"DEL", 0x7f}, {"Rubout", 0x7f}, {"ESC", 0x1b}, {"Escape", 0x1b},
        {"LFD", 0x0a}, {"Newline", 0x0a}, {"RET", 0x0d}, {"Return", 0x0d},
        {"SPC", 0x20}, {"Space", 0x20}, {"Tab", 0x09},
    };
    i32 key = base.len ? base[0] : 0;
    for (iz i = 0; i < countof(names); i++) {
        if (rlequal(names[i].name, base)) key = names[i].key;
    }
    // Modifiers: a case-insensitive search of the whole key part
    b32 ctl = 0, meta = 0;
    Str lk = clone(&a, k);
    for (iz i = 0; i < lk.len; i++) lk.data[i] = complower(lk[i]);
    for (iz i = 0; i < lk.len; i++) {
        Str t = cuthead(lk, i);
        ctl  |= startswith(t, "control-") || startswith(t, "c-") || startswith(t, "ctrl-");
        meta |= startswith(t, "meta") || startswith(t, "m-");
    }
    if (dash>=0 && !ctl && !meta) {
        rlerror(ed, sh, r, concat(&a, k, ": unknown key modifier"), &a);
        return -1;
    }
    if (ctl) key = (key>='a' && key<='z' ? key-32 : key) & 0x1f;
    if (meta) key |= 0x80;
    return key;
}

static void rlline(Editor *ed, Shell *sh, RlRead *r, Str line, Arena a)
{
    line = rlskip(line);
    if (!line.len || line[0]=='#') return;
    if (line[0] == '$') {
        rldirective(ed, sh, r, cuthead(line, 1), a);
        return;
    }
    if (ed->ifoff) return;

    iz  i      = 0;
    b32 quoted = line[0] == '"';
    iz  qend   = 0;
    if (quoted) {
        i = 1;
        for (; i<line.len && line[i]!='"'; i++) {
            if (line[i]=='\\' && i+1<line.len) i++;
        }
        if (i == line.len) {
            rlerror(ed, sh, r, concat(&a, line, ": no closing `\"' in key binding"), &a);
            return;
        }
        qend = i;
        i++;
    }
    for (; i<line.len && line[i]!=':' && !rlblank(line[i]); i++) {}
    Str key = takehead(line, i);
    if (!key.len) {
        rlerror(ed, sh, r, concat3(&a, "`", line, "': invalid key binding: missing key sequence"), &a);
        return;
    }
    if (i+1<line.len && line[i]==':' && line[i+1]=='=') return;  // ignored
    if (rlequal(key, "set")) {
        rlset(ed, sh, r, i<line.len ? cuthead(line, i+1) : Str(""), a);
        return;
    }
    if (i == line.len) {
        rlerror(ed, sh, r, concat(&a, line, ": no key sequence terminator"), &a);
        return;
    }
    Str rhs = rlskip(cuthead(line, i+1));

    i32 fn    = EF_NONE;
    Str macro = {};
    if (rhs.len && (rhs[0]=='"' || rhs[0]=='\'')) {
        u8 q = rhs[0];
        iz j = 1;
        for (; j<rhs.len && rhs[j]!=q; j++) {
            if (rhs[j]=='\\' && j+1<rhs.len) j++;
        }
        if (j == rhs.len) {
            rlerror(ed, sh, r, concat3(&a, "`", rhs, "': missing closing quote for macro"), &a);
            return;
        }
        for (; j<rhs.len && !rlblank(rhs[j]); j++) {}
        Str body = span(rhs.data+1, rhs.data+j);
        if (body.len && body[body.len-1]==q) body.len--;
        macro = rlkeyseq(&a, body);
        fn    = EF_MACRO;
        if (!macro.len) fn = EF_NONE;
    } else {
        fn = rlfunction(rlword(rhs));
    }

    if (quoted) {
        rlbind(ed, rlkeyseq(&a, span(line.data+1, line.data+qend)), fn, macro);
    } else {
        i32 k = rlkeyname(ed, sh, r, key, a);
        if (k < 0) return;
        u8 b = (u8)k;
        rlbind(ed, {&b, 1}, fn, macro);
    }
}

// Read an init file; false if it could not be read (silently)
static b32 rlreadfile(Editor *ed, Shell *sh, Str name, i32 level, Arena a)
{
    Str path = tildefilename(sh, &a, name);
    i32 err  = 0;
    Str text = readwholefile(sh, &a, path, &err);
    if (err) return 0;
    RlRead r = {};
    r.file  = name;
    r.level = level;
    ed->rlreading = 1;
    for (Str rest = text; rest.len;) {
        Cut c = cut(rest, '\n');
        rest = c.tail;
        Str l = c.head;
        if (sh->pathmode==PM_MSYS && l.len && l[l.len-1]=='\r') l.len--;
        for (iz i = 0; i < l.len; i++) {
            if (!l[i]) l.len = i;  // a NUL ends the line
        }
        r.line++;
        rlline(ed, sh, &r, l, a);
    }
    ed->rlreading = 0;
    return 1;
}

// The init file: the last one read, else INPUTRC, else ~/.inputrc or
// /etc/inputrc (§1.1)
static void rlinit(Editor *ed, Shell *sh)
{
    Arena a = *scratchat(sh, sh->capdepth+1);
    if (ed->rllast.len) {
        rlreadfile(ed, sh, ed->rllast, 0, a);
    } else {
        Str rc = getvar(sh, &a, "INPUTRC").value;
        if (rc.len) {
            if (rlreadfile(ed, sh, rc, 0, a)) ed->rllast = clone(&ed->perm, rc);
        } else if (rlreadfile(ed, sh, "~/.inputrc", 0, a)) {
            ed->rllast = "~/.inputrc";
        } else if (rlreadfile(ed, sh, "/etc/inputrc", 0, a)) {
            ed->rllast = "/etc/inputrc";
        }
    }
    ed->vars.keymap = ed->vars.vimode ? KM_VIINS : KM_EMACS;
}

// The editor's first use
static void rlstart(Editor *ed, Shell *sh)
{
    rlinit(ed, sh);
}

// The terminal's special characters, bound on each line (§4.8)
static void rlttychars(Editor *ed)
{
    if (!ed->vars.ttyspecial) return;
    struct { u8 c; i32 fn; } m[] = {
        {ed->tty.erase,  EF_BACKWARD_DELETE_CHAR},
        {ed->tty.kill,   EF_UNIX_LINE_DISCARD},
        {ed->tty.lnext,  EF_QUOTED_INSERT},
        {ed->tty.werase, EF_UNIX_WORD_RUBOUT},
    };
    // The last line's characters become self-insert, then this line's
    // are bound, touching only keys bound to functions
    for (i32 pass = 0; pass < 2; pass++) {
        for (iz i = 0; i < countof(m); i++) {
            u8 c = pass ? m[i].c : ed->ttysaved[i];
            if (!c) continue;
            KeyNode *k = keychild(&ed->root, c);
            if (k && (k->kids || k->fn==EF_MACRO)) continue;
            k = keyget(&ed->perm, &ed->root, c);
            k->fn = pass ? m[i].fn : EF_SELF_INSERT;
        }
    }
    for (iz i = 0; i < countof(m); i++) ed->ttysaved[i] = m[i].c;
}
