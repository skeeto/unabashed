// Command history: the list, lines read into it, the history file,
// and the history and fc builtins (notes/research/history.md)
// This is free and unencumbered software released into the public domain.

// The list lives in the Shell by value (Hist in shell.cpp) and its
// entries in the store. An entry never changes once made: a change
// replaces it. The array of entries is copied before a synchronous
// subshell changes one its parent owns, so rolling back the Shell
// restores the parent's list.

// The number the next entry gets (bash's history_number), or 1 while
// history is off.
static i64 histnumber(Shell *sh)
{
    Hist *h = &sh->hist;
    return sh->opt[O_HISTORY] ? h->base+h->len : 1;
}

// Make the entry array this level's to change, with room for one more.
static void histown(Shell *sh, iz need)
{
    Hist  *h  = &sh->hist;
    Store *st = sh->store;
    b32 parents = h->ents && (byte *)h->ents<st->mark;
    if (!parents && h->len+need<=h->cap) return;
    iz cap = h->cap;
    if (h->len+need > cap) cap = cap ? cap*2 : 64;
    if (cap < h->len+need) cap = h->len + need;
    HistEnt **ents = (HistEnt **)salloc(st, cap*(iz)sizeof(HistEnt *));
    if (h->len) copybytes(ents, h->ents, h->len*(iz)sizeof(HistEnt *));
    sfree(st, h->ents, h->cap*(iz)sizeof(HistEnt *));
    h->ents = ents;
    h->cap  = cap;
}

static HistEnt *newhistent(Shell *sh, Str line, Str stamp)
{
    HistEnt *e = (HistEnt *)salloc(sh->store, sizeof(HistEnt));
    e->line  = sclone(sh->store, line);
    e->stamp = sclone(sh->store, stamp);
    return e;
}

static void freehistent(Shell *sh, HistEnt *e)
{
    if (!e) return;
    sfreestr(sh->store, e->line);
    sfreestr(sh->store, e->stamp);
    sfreestr(sh->store, e->orig);
    sfree(sh->store, e, sizeof(HistEnt));
}

// Remove entries [i, i+n).
static void histremove(Shell *sh, iz i, iz n)
{
    Hist *h = &sh->hist;
    histown(sh, 0);
    for (iz k = 0; k < n; k++) freehistent(sh, h->ents[i+k]);
    for (iz k = i; k+n < h->len; k++) h->ents[k] = h->ents[k+n];
    h->len -= n;
}

static void histreplace(Shell *sh, iz i, Str line)
{
    Hist *h = &sh->hist;
    histown(sh, 0);
    HistEnt *old = h->ents[i];
    HistEnt *e   = newhistent(sh, line, old->stamp);
    e->edited = old->edited;
    h->ents[i] = e;
    freehistent(sh, old);
}

// The editor's change to entry i: its line now and the undo list back
// to the line before the first change, kept with it. No undo list ends
// the change: the entry is plain again.
static void histedit(Shell *sh, iz i, Str line, EdUndo *undo)
{
    Hist *h = &sh->hist;
    histown(sh, 0);
    HistEnt *old = h->ents[i];
    HistEnt *e   = newhistent(sh, line, old->stamp);
    if (undo) {
        e->orig   = sclone(sh->store, old->undo ? old->orig : old->line);
        e->undo   = undo;
        e->edited = 1;
    }
    h->ents[i] = e;
    freehistent(sh, old);
}

// The time stamp a new entry gets: the comment character and the epoch
// seconds. As readline's, a NUL comment character is a NUL byte, which
// leaves the stamp empty to C (never written to a file) but still
// readable as a time while the comment character stays NUL.
static Str histstamp(Shell *sh, Arena *a, u8 com)
{
    Buf b(a, 24);
    putbyte(&b, com);
    print(&b, dynnow(sh) / 1000000000);
    return finish(&b);
}

// Readline's add_history: a new entry, the oldest dropped when the list
// is full. Nothing at all while the limit is 0.
static void histpush(Shell *sh, Str line, Str stamp)
{
    Hist *h = &sh->hist;
    if (!h->max) return;
    if (h->max>0 && h->len>=h->max) {
        histremove(sh, 0, (iz)(h->len-h->max+1));
        h->base++;
    }
    histown(sh, 1);
    h->ents[h->len++] = newhistent(sh, line, stamp);
}

static void reallyadd(Shell *sh, Str line)
{
    Hist *h = &sh->hist;
    byte  mem[32];
    Arena a = {mem, mem+sizeof(mem)};
    h->lastadded  = 1;
    h->lastpushed = 0;
    histpush(sh, line, histstamp(sh, &a, h->comchar));
    h->session++;
}

// Apply a limit: -1 for none. Shrinking drops the oldest entries and
// numbers the rest from the count dropped, as readline does.
static void histstifle(Shell *sh, i64 max)
{
    Hist *h = &sh->hist;
    if (max>=0 && h->len>max) {
        iz drop = h->len - (iz)max;
        histremove(sh, 0, drop);
        h->base = drop;
    }
    h->max = max;
}

// A number as bash's valid_number takes it: blanks around an optional
// sign and decimal digits, the value cut to an int.
static b32 histnum(Str s, i64 *out)
{
    i64 v = 0;
    if (!parsei64(s, &v)) return 0;
    *out = (i64)(i32)v;
    return 1;
}

static void histfilesize(Shell *sh);
static i32  histread(Shell *, Str file, i64 from, Arena);

// Variables that steer the history, as bash's sv_* hooks.
static void histvarchanged(Shell *sh, Str name)
{
    Hist *h = &sh->hist;
    Arena none = {};
    if (!h->ready) return;  // not from the environment
    if (name == "HISTSIZE") {
        Got g = getvar(sh, &none, name);
        i64 n = 0;
        if (!g.set || !g.value.len) {
            histstifle(sh, -1);
        } else if (histnum(g.value, &n)) {
            if (n < 0) {
                histstifle(sh, -1);
            } else {
                histstifle(sh, n);
                if (h->session > h->len) h->session = h->len;
            }
        }
    } else if (name == "HISTCONTROL") {
        Str v = getvar(sh, &none, name).value;
        h->control = 0;
        while (v.len) {
            Cut c = cut(v, ':');
            if (c.head == "ignorespace") h->control |= HC_IGNSPACE;
            if (c.head == "ignoredups")  h->control |= HC_IGNDUPS;
            if (c.head == "ignoreboth")  h->control |= HC_IGNSPACE|HC_IGNDUPS;
            if (c.head == "erasedups")   h->control |= HC_ERASEDUPS;
            v = c.tail;
        }
    } else if (name == "histchars") {
        Got g = getvar(sh, &none, name);
        if (!g.set) {
            h->expchar = '!';
            h->subchar = '^';
            h->comchar = '#';
        } else {
            h->expchar = g.value.len>0 ? g.value[0] : 0;
            if (g.value.len > 1) h->subchar = g.value[1];
            if (g.value.len > 2) h->comchar = g.value[2];
        }
    } else if (name == "HISTFILESIZE") {
        histfilesize(sh);
    } else if (name == "HISTTIMEFORMAT") {
        b32 set = getvar(sh, &none, name).set;
        if (set && !h->comchar) h->comchar = '#';
        h->writets = set;
    }
}

// The history list's defaults for a fresh shell.
static void histinit(Shell *sh)
{
    Hist *h = &sh->hist;
    h->base    = 1;
    h->max     = -1;
    h->expchar = '!';
    h->subchar = '^';
    h->expinhibit = !sh->interactive;
    h->ready   = 1;
}


// Adding lines as the parser reads them

// Bash's shell_comment: 1 if the line is a comment, 2 if a comment
// follows something, else 0. A # anywhere outside quotes and
// substitutions counts, as bash's skip_to_delim finds it.
static i32 shellcomment(Lexer *L, Str line)
{
    if (L->ddepth || (L->pst & PST_HEREDOC)) return 0;
    iz i = 0;
    for (; i<line.len && (line[i]==' ' || line[i]=='\t'); i++) {}
    if (i<line.len && line[i]=='#') return 1;
    for (; i < line.len; i++) {
        u8 c = line[i];
        if (c == '\\') {
            i++;
        } else if (c == '`') {
            for (i++; i<line.len && line[i]!='`'; i++) {
                if (line[i] == '\\') i++;
            }
        } else if (c == '#') {
            return 2;
        } else if (c == '\'') {
            for (i++; i<line.len && line[i]!='\''; i++) {}
        } else if (c == '"') {
            for (i++; i<line.len && line[i]!='"'; i++) {
                if (line[i] == '\\') i++;
            }
        } else if ((c=='$' || c=='<' || c=='>') && i+1<line.len &&
                   (line[i+1]=='(' || (c=='$' && line[i+1]=='{'))) {
            u8  open  = line[i+1];
            u8  close = open=='(' ? ')' : '}';
            i32 depth = 1;
            for (i += 2; i<line.len && depth; i++) {
                if (line[i] == '\\') {
                    i++;
                } else if (line[i] == open) {
                    depth++;
                } else if (line[i] == close) {
                    depth--;
                }
            }
            i--;
        }
    }
    return 0;
}

static b32 blankline(Str s)
{
    for (iz i = 0; i < s.len; i++) {
        if (s[i]!=' ' && s[i]!='\t') return 0;
    }
    return 1;
}

// What joins a line to the command's entry when cmdhist joins the
// lines of one command (bash's history_delimiting_chars), from the
// tokens the parser has read: the newline just read is L->last.
static Str histdelim(Shell *sh, Lexer *L, Str line)
{
    Hist *h = &sh->hist;
    if (!(L->pst & PST_HEREDOC)) h->lastheredoc = 0;
    if (L->ddepth) {
        i32 d = currentdelim(L);
        if (d=='\'' || d=='"' || d=='`') return "\n";
        if (d == '(') {
            Str cur = h->len ? h->ents[h->len-1]->line : Str{};
            return cur.len && cur[cur.len-1]=='\n' ? Str("") : "\n";
        }
        return "\n";
    }
    if (L->pst & PST_HEREDOC) {
        if (h->lastheredoc) {
            h->lastheredoc = 0;
            return "\n";
        }
        return L->heredocfirst ? Str("\n") : "";
    }
    if (L->pst & PST_COMPASSIGN) return " ";
    iz i = 0;
    for (; i<line.len && (line[i]==' ' || line[i]=='\t'); i++) {}
    u8 first = i<line.len ? line[i] : 0;
    if (L->before == ')') {
        if (L->twoago == '(') return " ";
        if (L->pst & PST_CASESTMT) return " ";
        if (first=='&' || first=='|' || first==';') return "\n";
        return "; ";
    } else if (L->before==T_WORD && L->twoago==T_FUNCTION) {
        return " ";
    } else if (L->cmdlines>1 && L->last=='\n' && find(line, "<<")>=0) {
        h->lastheredoc = 1;
        return "\n";
    } else if (L->cmdlines>1 && L->nheredoc>0) {
        return "\n";
    } else if (L->before==T_WORD && L->twoago==T_FOR) {
        b32 in = i+1<line.len && line[i]=='i' && line[i+1]=='n';
        return in ? Str(" ") : ";";
    } else if (L->twoago==T_CASE && L->before==T_WORD && (L->pst & PST_CASESTMT)) {
        return " ";
    }
    static i32 nosemi[] = {
        '\n', '{', '(', ')', ';', '&', '|', T_CASE, T_DO, T_ELSE, T_IF,
        T_DSEMI, T_SEMIAND, T_DSEMIAND, T_THEN, T_UNTIL, T_WHILE,
        T_ANDAND, T_OROR, T_IN, T_DOLPAREN, T_DOLBRACE,
    };
    for (i32 k = 0; k < countof(nosemi); k++) {
        if (L->before == nosemi[k]) return " ";
    }
    if (blankline(line)) {
        b32 semi = L->cmdlines>1 && L->last=='\n' && L->before!='\n';
        return semi ? Str("; ") : "";
    }
    return "; ";
}

// Does HISTIGNORE drop this line? A & in a pattern stands for the
// previous entry, quoted to match itself.
static b32 histignored(Shell *sh, Str line, Arena scratch)
{
    Str ign = getvar(sh, &scratch, "HISTIGNORE").value;
    if (!ign.len) return 0;
    Hist *h    = &sh->hist;
    Str   prev = h->len ? h->ents[h->len-1]->line : Str{};
    b32   extglob = sh->shopt[S_EXTGLOB];
    i32   pf   = patflags(sh);
    for (iz i = 0; i < ign.len;) {
        iz  end = ignoreend(ign, i, extglob);
        Str pat = span(ign.data+i, ign.data+end);
        i = end + 1;
        if (!pat.len) continue;
        if (h->len && find(pat, "&")>=0) {
            Buf b(&scratch, pat.len+prev.len);
            for (iz k = 0; k < pat.len; k++) {
                if (pat[k]=='\\' && k+1<pat.len && pat[k+1]=='&') {
                    putbyte(&b, '&');
                    k++;
                } else if (pat[k] == '&') {
                    for (iz j = 0; j < prev.len; j++) {
                        u8 c = prev[j];
                        if (c=='*' || c=='?' || c=='[' || c==']' || c=='\\' ||
                            (extglob && (c=='+' || c=='@' || c=='!' ||
                                         c=='(' || c==')' || c=='|'))) {
                            putbyte(&b, '\\');
                        }
                        putbyte(&b, c);
                    }
                } else {
                    putbyte(&b, pat[k]);
                }
            }
            pat = finish(&b);
        }
        if (patmatch(pat, line, pf, scratch)) return 1;
    }
    return 0;
}

static b32 histcontrolok(Shell *sh, Str line)
{
    Hist *h = &sh->hist;
    if ((h->control & HC_IGNSPACE) && line.len && line[0]==' ') return 0;
    if ((h->control & HC_IGNDUPS) && h->len && h->ents[h->len-1]->line==line) {
        return 0;
    }
    return 1;
}

static void bashadd(Shell *sh, Lexer *L, Str line);

// Bash's check_add_history: add a line that passes HISTCONTROL and
// HISTIGNORE, erasing its duplicates first under erasedups. With force,
// always a new entry. True if it was added.
static b32 checkadd(Shell *sh, Lexer *L, Str line, b32 force, Arena scratch)
{
    Hist *h = &sh->hist;
    if (!histcontrolok(sh, line) || histignored(sh, line, scratch)) return 0;
    if (h->control & HC_ERASEDUPS) {
        for (iz i = h->len-1; i >= 0; i--) {
            if (h->ents[i]->line == line) histremove(sh, i, 1);
        }
    }
    if (force) {
        reallyadd(sh, line);
    } else {
        bashadd(sh, L, line);
    }
    return 1;
}

// Bash's bash_add_history: a new entry, or with cmdhist the line joined
// to the entry of the command it continues.
static void bashadd(Shell *sh, Lexer *L, Str line)
{
    Hist *h = &sh->hist;
    if (sh->shopt[S_CMDHIST] && L && L->cmdlines>1) {
        i32 comment = shellcomment(L, line);
        Str sep = {};
        if ((L->pst & PST_HEREDOC) && !L->heredocfirst && line.len && line[line.len-1]=='\n') {
            sep = "";
        } else if (L->cmdlines == h->commentline+1) {
            sep = "\n";
        } else if (sh->shopt[S_LITHIST]) {
            sep = "\n";
        } else {
            sep = histdelim(sh, L, line);
        }
        h->commentline = comment ? L->cmdlines : -2;
        if (h->len) {
            Str cur = h->ents[h->len-1]->line;
            if (!L->ddepth && cur.len && cur[cur.len-1]=='\\' &&
                    (cur.len<2 || cur[cur.len-2]!='\\')) {
                cur.len--;  // a backslash-newline joins the lines as is
                sep = "";
            }
            if (!L->ddepth && cur.len && cur[cur.len-1]=='\n' && sep.len && sep[0]==';') {
                sep = cuthead(sep, 1);
            }
            Arena a = *scratchat(sh, sh->capdepth+1);
            histreplace(sh, h->len-1, concat3(&a, cur, sep, line));
            return;
        }
    }
    if (h->max==0 && !h->len) return;
    reallyadd(sh, line);
}

// Bash's maybe_add_history: the first line of a command decides whether
// the rest of it is kept.
static void maybeadd(Shell *sh, Lexer *L, Str line)
{
    Hist *h = &sh->hist;
    h->lastadded = 0;
    i32 comment = shellcomment(L, line);
    if (L->cmdlines > 1) {
        if (h->firstsaved && ((L->pst & PST_HEREDOC) || sh->shopt[S_LITHIST] ||
                              L->ddepth || comment!=1)) {
            bashadd(sh, L, line);
        }
        h->commentline = comment ? L->cmdlines : -2;
        return;
    }
    h->commentline = comment ? L->cmdlines : -2;
    h->firstsaved  = checkadd(sh, L, line, 0, *scratchat(sh, sh->capdepth+1));
}

static Str histexpandline(Shell *sh, Lexer *L, Str line, b32 *drop);

// The parser's hook for each physical line the reader reads: here
// history expands it and adds it. A null result drops the line, as a
// failed expansion does; the reader then reads another.
static Str histline(void *ctx, Lexer *L, Str line, b32 heredoc)
{
    Shell *sh = (Shell *)ctx;
    sh->hist.cmdlines = L->cmdlines;
    if (!sh->opt[O_HISTORY]) return line;
    if (heredoc) {
        if (L->pst & PST_HEREDOC) maybeadd(sh, L, line);
        return line;
    }
    if (line.len) {
        b32 drop = 0;
        Str r = histexpandline(sh, L, line, &drop);
        if (drop) return {};
        if (r.len) maybeadd(sh, L, r);
        return r;
    } else if (L->cmdlines > 1) {
        if (L->ddepth) {
            maybeadd(sh, L, line);
        } else {
            Str sep = histdelim(sh, L, line);
            if (sep.len && sep[0]==';') maybeadd(sh, L, line);
        }
    }
    return line;
}


// Turning history on and off

static void histload(Shell *sh);

// set -o history, set -H and their shopt -o forms. (Also set -n, which
// an interactive shell ignores, though not as its own -n argument, and
// set -m, which turns job control on and off.)
static void histoption(Shell *sh, i32 opt, b32 on)
{
    Hist *h = &sh->hist;
    if (opt == O_MONITOR) {
        sh->jobctl = on;
        if (on) jobctlon(sh);
    } else if (opt==O_NOEXEC && sh->interactive) {
        sh->opt[O_NOEXEC] = 0;
    } else if (opt == O_HISTORY) {
        h->expinhibit = !on;
        if (on) {
            histvarchanged(sh, "HISTCONTROL");
            if (!h->session) histload(sh);
        }
    } else if (opt==O_HISTEXPAND && on && !h->comchar) {
        h->comchar = '#';
    }
}

// Startup and set -o history: HISTSIZE defaults to 500 and HISTFILESIZE
// to it, then the file is read.
static void histload(Shell *sh)
{
    Arena none = {};
    Arena a    = *scratchat(sh, sh->capdepth+1);
    if (!getvar(sh, &none, "HISTSIZE").set) setvar(sh, a, "HISTSIZE", "500");
    histvarchanged(sh, "HISTSIZE");
    Got fs = getvar(sh, &none, "HISTFILESIZE");
    if (!fs.set) {
        Str n = clone(&a, getvar(sh, &none, "HISTSIZE").value);
        setvar(sh, a, "HISTFILESIZE", n);
    }
    histfilesize(sh);
    Str file = getvar(sh, &a, "HISTFILE").value;
    if (file.len && !pathstat(sh, a, file, 1).err) {
        if (!histread(sh, file, 0, a)) sh->hist.infile = sh->hist.readlines;
    }
}


// The history builtin

static Str histtime(Shell *, Arena *, Str stamp, Str fmt);

static void histlist(Shell *sh, iz from, Arena scratch)
{
    Hist *h = &sh->hist;
    Str fmt = getvar(sh, &scratch, "HISTTIMEFORMAT").value;
    for (iz i = from; i < h->len; i++) {
        Arena    a = scratch;
        HistEnt *e = h->ents[i];
        Buf b(&a, e->line.len+32);
        Str n = fmti64(&a, h->base+i);
        for (iz k = n.len; k < 5; k++) putbyte(&b, ' ');
        print(&b, n);
        putbyte(&b, e->edited ? '*' : ' ');
        putbyte(&b, ' ');
        if (fmt.len) print(&b, histtime(sh, &a, e->stamp, fmt));
        print(&b, e->line);
        putbyte(&b, '\n');
        bprint(sh, finish(&b));
    }
}

// Delete the newest entry (bash's bash_delete_last_history): false if
// there is none.
static b32 histdroplast(Shell *sh)
{
    Hist *h = &sh->hist;
    if (!h->len) return 0;
    histremove(sh, h->len-1, 1);
    h->session--;
    return 1;
}

static i32 histfileop(Shell *sh, i32 op, Str *args, iz nargs, iz i, Arena scratch);
static Str histexpandarg(Shell *sh, Arena *a, Str s, i32 *result);

static i32 b_history(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    Hist *h = &sh->hist;
    b32 clear = 0, push = 0, print_ = 0;
    i32 fileop = 0, nfileops = 0;
    Str del = {};
    b32 hasdel = 0;
    iz  i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str a = args[i];
        if (a == "--") {
            i++;
            break;
        }
        if (digit(a[1])) break;
        for (iz k = 1; k < a.len; k++) {
            switch (a[k]) {
            case 'c': clear  = 1; break;
            case 's': push   = 1; break;
            case 'p': print_ = 1; break;
            case 'a': case 'n': case 'r': case 'w':
                if (fileop != a[k]) nfileops++;
                fileop = a[k];
                break;
            case 'd':
                if (k+1 < a.len) {
                    del = cuthead(a, k+1);
                } else if (i+1 < nargs) {
                    del = args[++i];
                } else {
                    builtinerror(sh, scratch, "history", "-d: option requires an argument");
                    usageerror(sh, scratch, "history");
                    return 2;
                }
                hasdel = 1;
                k = a.len;
                break;
            default:
                invalidoption(sh, scratch, "history", span(a.data+k-1, a.data+k+1));
                return 2;
            }
        }
    }
    if (nfileops > 1) {
        builtinerror(sh, scratch, "history", "cannot use more than one of -anrw");
        return 1;
    }
    if (clear) {
        for (iz k = 0; k < h->len; k++) freehistent(sh, h->ents[k]);
        h->len     = 0;
        h->base    = 1;
        h->session = 0;
        if (i == nargs) return 0;
    }
    if (push) {
        if (i < nargs) {
            Buf b(&scratch, 64);
            for (iz k = i; k < nargs; k++) {
                if (k > i) putbyte(&b, ' ');
                print(&b, args[k]);
            }
            Str line = finish(&b);
            // The entry of this command goes first, the whole compound
            // command's under cmdhist
            if (sh->opt[O_HISTORY] && !h->lastpushed &&
                    (h->lastadded || (h->cmdlines>0 && h->firstsaved && sh->shopt[S_CMDHIST])) &&
                    !histdroplast(sh)) {
                return 0;
            }
            checkadd(sh, 0, line, 1, scratch);
            h->lastpushed = 1;
        }
        return 0;
    }
    if (print_) {
        if (i == nargs) return 0;
        if (!h->lastpushed && h->lastadded && !histdroplast(sh)) return 1;
        i32 status = 0;
        for (; i < nargs; i++) {
            Arena a = scratch;
            i32   r = 0;
            Str   x = histexpandarg(sh, &a, args[i], &r);
            if (r < 0) {
                builtinerror(sh, a, "history", concat(&a, args[i], ": history expansion failed"));
                status = 1;
                continue;
            }
            bprint(sh, x);
            bprint(sh, "\n");
        }
        return status;
    }
    if (hasdel) {
        iz dash = -1;
        for (iz k = 1; k < del.len; k++) {
            if (del[k] == '-') {
                dash = k;
                break;
            }
        }
        if (dash > 0) {
            Str s = takehead(del, dash), e = cuthead(del, dash+1);
            i64 from = 0, to = 0;
            if (!histnum(s, &from) || !histnum(e, &to)) {
                builtinerror(sh, scratch, "history", concat(&scratch, del, ": history position out of range"));
                return 1;
            }
            Str which[2] = {s, e};
            i64 *vals[2] = {&from, &to};
            for (i32 k = 0; k < 2; k++) {
                i64 v = *vals[k];
                if (which[k].len && which[k][0]=='-' && v<0) {
                    v += h->len;
                } else if (v > 0) {
                    v -= h->base;
                }
                if (v<0 || v>=h->len) {
                    builtinerror(sh, scratch, "history", concat(&scratch, which[k], ": history position out of range"));
                    return 1;
                }
                *vals[k] = v;
            }
            if (from > to) return 1;
            histremove(sh, (iz)from, (iz)(to-from+1));
            h->session -= to - from + 1;
            return 0;
        }
        i64 v = 0;
        if (!histnum(del, &v)) {
            Str what = ": invalid number";
            if (del.len>1 && del[0]=='0' && digit(del[1])) what = ": invalid octal number";
            if (del.len>1 && del[0]=='0' && (del[1]=='x' || del[1]=='X')) what = ": invalid hex number";
            builtinerror(sh, scratch, "history", concat(&scratch, del, what));
            return 1;
        }
        i64 idx = 0;
        if (del.len && del[0]=='-') {
            idx = h->len + v;
            if (idx < 0) {
                builtinerror(sh, scratch, "history", concat(&scratch, del, ": history position out of range"));
                return 1;
            }
        } else {
            if (v<h->base || v>=h->base+h->len) {
                builtinerror(sh, scratch, "history", concat(&scratch, del, ": history position out of range"));
                return 1;
            }
            idx = v - h->base;
        }
        histremove(sh, (iz)idx, 1);
        h->session--;
        return 0;
    }
    if (!fileop && !clear) {
        iz from = 0;
        if (i < nargs) {
            if (i+1 < nargs) {
                builtinerror(sh, scratch, "history", "too many arguments");
                sh->status = 1;
                escape(sh, ESC_ABORT);  // as bash's jump to top level
            }
            i64 n = 0;
            if (!parsei64(args[i], &n)) {
                builtinerror(sh, scratch, "history", concat(&scratch, args[i], ": numeric argument required"));
                return 2;
            }
            if (n < 0) n = -n;
            if (n < h->len) from = h->len - (iz)n;
        }
        histlist(sh, from, scratch);
        return 0;
    }
    return histfileop(sh, fileop, args, nargs, i, scratch);
}


// Time stamps in listings: HISTTIMEFORMAT's strftime format for a stamp
// that starts with the comment character, else the stamp as invalid, or
// ?? for one empty to C.
static Str histtime(Shell *sh, Arena *a, Str stamp, Str fmt)
{
    i64 t = 0;
    if (stamp.len && stamp[0]==sh->hist.comchar) {
        for (iz i = 1; i<stamp.len && digit(stamp[i]) && t<(i64)1<<50; i++) {
            t = t*10 + stamp[i] - '0';
        }
    }
    if (!t && (!stamp.len || !stamp[0])) return "??";
    if (!t) {
        Str s = stamp[0]=='#' ? cuthead(stamp, 1) : stamp;
        Cut c = cut(s, 0);  // as a C string
        return concat(a, c.head, ": invalid timestamp");
    }
    PfTm   tm  = localtm(t, shellzone(sh, a));
    PfTout out = {};
    fmttime(&out, fmt, &tm);
    return out.full ? Str{} : clone(a, Str{out.data, out.len});
}



// History expansion (readline's histexpand.c, with bash's hooks)

// Replace a string the history keeps.
static Str hset(Shell *sh, Str old, Str s)
{
    sfreestr(sh->store, old);
    return sclone(sh->store, s);
}

static b32 hblank(u8 c)
{
    return c==' ' || c=='\t' || c=='\n';
}

static b32 hdelim(u8 c)  // history_word_delimiters
{
    return hblank(c) || c==';' || c=='&' || c=='(' || c==')' ||
           c=='|' || c=='<' || c=='>';
}

// The end of a parenthesized group opening at s[i]=='(': past its ')'.
static iz hparens(Str s, iz i)
{
    i32 depth = 0;
    for (; i < s.len; i++) {
        u8 c = s[i];
        if (c=='\\' && i+1<s.len) {
            i++;
        } else if (c=='\'' || c=='"') {
            for (i++; i<s.len && s[i]!=c; i++) {
                if (c=='"' && s[i]=='\\') i++;
            }
        } else if (c == '(') {
            depth++;
        } else if (c==')' && !--depth) {
            return i + 1;
        }
    }
    return s.len;
}

// The end of the word starting at s[i], as history_tokenize sees it.
static iz hwordend(Str s, iz i, u8 com)
{
    if (i >= s.len) return i;
    iz start = i;
    // A file descriptor before a redirection belongs to it
    iz d = i;
    for (; d<s.len && digit(s[d]); d++) {}
    if (d>i && d<s.len && (s[d]=='<' || s[d]=='>')) i = d;
    u8 c = s[i];
    u8 n = i+1<s.len ? s[i+1] : 0;
    if ((c=='<' || c=='>') && n=='(') {
        // a process substitution is a word
    } else if (c=='(' || c==')') {
        return i + 1;
    } else if (c==';' || c=='&' || c=='|' || c=='<' || c=='>') {
        if (n == c) {
            i += 2;
            if (c=='<' && i<s.len && (s[i]=='-' || s[i]=='<')) i++;
            return i;
        }
        if ((c=='<' || c=='>') && n=='&') {
            for (i += 2; i<s.len && digit(s[i]); i++) {}
            if (i<s.len && s[i]=='-') i++;
            return i;
        }
        if ((c=='&' && n=='>') || (c=='>' && n=='|')) return i + 2;
        for (; i<s.len && hdelim(s[i]) && !hblank(s[i]); i++) {}
        return i;
    } else if (i > start) {
        return i;  // digits then a lone operator handled above
    }
    for (; i < s.len; i++) {
        c = s[i];
        n = i+1<s.len ? s[i+1] : 0;
        if (c == '\\') {
            i += i+1 < s.len;
        } else if (c=='\'' || c=='"' || c=='`') {
            for (i++; i<s.len && s[i]!=c; i++) {
                if (c!='\'' && s[i]=='\\') i++;
            }
        } else if (n=='(' && (c=='$' || c=='<' || c=='>' || c=='@' ||
                              c=='!' || c=='?' || c=='+' || c=='*')) {
            i = hparens(s, i+1) - 1;
        } else if (hdelim(c)) {
            break;
        }
    }
    (void)com;
    return i;
}

struct HWords {
    Str *word;
    iz   len;
    iz   at;  // the word holding a position, or -1
};

// Split a line into words; also find which holds position pos.
static HWords histwords(Arena *a, Str s, u8 com, iz pos)
{
    HWords r = {};
    r.at = -1;
    iz cap = 8;
    r.word = alloc<Str>(a, cap);
    for (iz i = 0;;) {
        for (; i<s.len && hblank(s[i]); i++) {}
        if (i>=s.len || (com && s[i]==com)) break;
        iz e = hwordend(s, i, com);
        if (e == i) e++;
        if (r.len == cap) {
            Str *w = alloc<Str>(a, cap*2);
            copybytes(w, r.word, cap*(iz)sizeof(Str));
            r.word = w;
            cap *= 2;
        }
        if (pos>=i && pos<e) r.at = r.len;
        r.word[r.len++] = span(s.data+i, s.data+e);
        i = e;
    }
    return r;
}

// Past a quoted string opening before s[i], or the end.
static iz hskipquoted(Str s, iz i, u8 q)
{
    for (; i < s.len; i++) {
        if (q=='"' && s[i]=='\\') {
            i++;
        } else if (s[i] == q) {
            return i + 1;
        }
    }
    return s.len;
}

// The next ! at or after start that bash's scanner finds unquoted, or
// the end of the line. Each scan starts outside any quotes.
static iz hnextbang(Shell *sh, Str s, iz start, u8 bang)
{
    b32 posix  = sh->opt[O_POSIX];
    b32 pass   = 0;
    b32 backq  = 0;
    b32 dq     = 0;
    b32 olddq  = 0;
    i32 comsub = 0;
    iz  i      = start;
    while (i < s.len) {
        u8 c  = s[i];
        u8 n  = i+1<s.len ? s[i+1] : 0;
        u8 n2 = i+2<s.len ? s[i+2] : 0;
        if (pass) {
            pass = 0;
            i++;
        } else if (c == '\\') {
            pass = 1;
            i++;
        } else if (backq && c=='`') {
            backq = 0;
            dq    = olddq;
            i++;
        } else if (c == '`') {
            backq = 1;
            olddq = dq;  // one level only, as bash
            dq    = 0;
            i++;
        } else if (dq && c==bang && n=='"') {
            i++;
        } else if (c == bang) {
            return i;
        } else if (dq && c=='\'') {
            i++;
        } else if (c == '\'') {
            i = hskipquoted(s, i+1, '\'');
        } else if (!posix && c=='"') {
            dq = !dq;
            i++;
        } else if (c == '"') {
            i = hskipquoted(s, i+1, '"');
        } else if ((c=='$' || c=='<' || c=='>') && n=='(' && n2!='(') {
            if (i+2 >= s.len) return i + 2;
            i += 2;
            comsub++;
            olddq = dq;
            dq    = 0;
        } else if (comsub && c==')') {
            comsub--;
            dq = olddq;
            i++;
        } else {
            i++;
        }
    }
    return i;
}

// Bash's veto by quoting: does its scanner, hopping from one unquoted !
// to the next, step over the one at s[i]? quote is the parser's open
// quote the line continues.
static b32 hquoted(Shell *sh, Str s, iz i, u8 quote)
{
    u8 bang = sh->hist.expchar;
    iz si   = 0;
    if (quote == '\'') {
        for (; si<s.len && s[si]!='\''; si++) {}
        if (si>=s.len || si>=i) return 1;
        si++;
    }
    iz t = hnextbang(sh, s, si, bang);
    if (t <= 0) return 0;
    while (t < i) {
        t = hnextbang(sh, s, t+1, bang);
        if (t <= 0) return 0;
    }
    return t > i;
}

static b32 hhas(Str s, iz from, u8 c)
{
    for (iz i = from; i < s.len; i++) {
        if (s[i] == c) return 1;
    }
    return 0;
}

// Does bash keep the ! at s[i] from expanding?
static b32 hveto(Shell *sh, Str s, iz i, u8 quote)
{
    if (i>0 && s[i-1]=='[' && hhas(s, i+1, ']')) return 1;
    if (i>1 && s[i-1]=='{' && s[i-2]=='$' && hhas(s, i+1, '}')) return 1;
    if (i>1 && s[i-1]=='$') return 1;
    if (sh->shopt[S_EXTGLOB] && i>1 && i+1<s.len && s[i+1]=='(' &&
            hhas(s, i+2, ')')) return 1;
    return hquoted(sh, s, i, quote);
}

struct HExp {
    Buf  out;
    Str  err;      // the message on failure
    Str  line;     // the line expanding
    iz   hlen;     // the entries it may refer to
    i32  quote;    // :q 1, :x 2
    b32  print;    // :p
};

static Str herror(Arena *a, Str s, iz beg, iz end, Str msg)
{
    return concat3(a, span(s.data+beg, s.data+end), ": ", msg);
}

static Str hentry(Shell *sh, iz i)
{
    return sh->hist.ents[i]->line;
}

// The rightmost occurrence of pat in s, or -1.
static iz hrfind(Str s, Str pat)
{
    for (iz i = s.len-pat.len; i >= 0; i--) {
        if (!pat.len || takehead(cuthead(s, i), pat.len) == pat) return i;
    }
    return -1;
}

// One delimited part of :s/old/new/, ending past its delimiter (or at the
// end of the line). In new, & and \& stay for later.
static Str hsubpart(Arena *a, Str s, iz *j, u8 delim, b32 rhs)
{
    Buf b(a, 16);
    iz  i = *j;
    for (; i<s.len && s[i]!=delim; i++) {
        if (s[i]=='\\' && i+1<s.len && s[i+1]==delim) {
            i++;
        } else if (rhs && s[i]=='\\' && i+1<s.len && s[i+1]=='&') {
            putbyte(&b, s[i++]);
        }
        putbyte(&b, s[i]);
    }
    *j = i + (i < s.len);
    return finish(&b);
}

// new with & as old and \& as &.
static Str hrhs(Arena *a, Str rhs, Str old)
{
    Buf b(a, rhs.len+old.len);
    for (iz i = 0; i < rhs.len; i++) {
        if (rhs[i]=='\\' && i+1<rhs.len && rhs[i+1]=='&') {
            putbyte(&b, '&');
            i++;
        } else if (rhs[i] == '&') {
            print(&b, old);
        } else {
            putbyte(&b, rhs[i]);
        }
    }
    return finish(&b);
}

// Replace old by new in text: the first occurrence, all (global), or
// the first in each blank-separated word (each). Null if none.
static Str hsubst(Arena *a, Str text, Str old, Str rep, b32 global, b32 each)
{
    Buf b(a, text.len+rep.len);
    b32 any = 0;
    b32 done = 0;
    iz  i = 0;
    while (i < text.len) {
        if (each && hblank(text[i])) {
            done = 0;
            putbyte(&b, text[i++]);
            continue;
        }
        b32 hit = !done && old.len<=text.len-i &&
                  takehead(cuthead(text, i), old.len)==old;
        if (hit) {
            print(&b, rep);
            i  += old.len;
            any = 1;
            done = !global;
            if (!old.len) {
                if (i < text.len) putbyte(&b, text[i]);
                i++;
            }
        } else {
            putbyte(&b, text[i++]);
        }
    }
    return any ? finish(&b) : Str{};
}

// :q and :x quoting.
static Str hquote(Arena *a, Str s, b32 split)
{
    Buf b(a, s.len+8);
    putbyte(&b, '\'');
    for (iz i = 0; i < s.len; i++) {
        if (s[i] == '\'') {
            print(&b, "'\\''");
        } else if (split && hblank(s[i])) {
            putbyte(&b, '\'');
            putbyte(&b, s[i]);
            putbyte(&b, '\'');
        } else {
            putbyte(&b, s[i]);
        }
    }
    putbyte(&b, '\'');
    return finish(&b);
}

// The word designator at s[*j], applied to the event's text. Null with
// x->err set on failure.
static Str hdesignate(Shell *sh, Arena *a, HExp *x, Str text, iz *pj, iz matchat)
{
    Hist *h = &sh->hist;
    Str   s = x->line;
    iz    j = *pj;
    iz    start = j;
    u8    c = j<s.len ? s[j] : 0;
    u8    n = j+1<s.len ? s[j+1] : 0;
    b32 colon = c==':' && (digit(n) || n=='^' || n=='$' || n=='*' ||
                           n=='-' || n=='%');
    if (colon) {
        j++;
        c = n;
    } else if (!(c=='^' || c=='$' || c=='*' || c=='-' || c=='%')) {
        return text;
    }
    HWords w = histwords(a, text, h->comchar, matchat);
    iz last  = w.len - 1;
    iz first = 0;
    iz end   = 0;
    if (c == '%') {
        j++;
        *pj = j;
        return w.at>=0 && matchat>=0 ? w.word[w.at] : Str("");
    }
    if (c == '*') {
        j++;
        *pj = j;
        if (w.len <= 1) return Str("");
        first = 1;
        end   = last;
    } else {
        if (c == '^') {
            first = 1;
            j++;
        } else if (c == '$') {
            first = last;
            j++;
        } else if (digit(c)) {
            first = 0;
            for (; j<s.len && digit(s[j]); j++) {
                first = first*10 + (s[j]-'0');
                if (first > 1<<20) first = 1<<20;
            }
        } else {
            first = 0;  // c == '-'
        }
        c = j<s.len ? s[j] : 0;
        if (c == '*') {
            j++;
            end = last;
        } else if (c == '-') {
            j++;
            c = j<s.len ? s[j] : 0;
            if (c == '$') {
                j++;
                end = last;
            } else if (c == '^') {
                j++;
                end = 1;
            } else if (digit(c)) {
                end = 0;
                for (; j<s.len && digit(s[j]); j++) {
                    end = end*10 + (s[j]-'0');
                    if (end > 1<<20) end = 1<<20;
                }
            } else {
                end = last - 1;
            }
        } else {
            end = first;
        }
    }
    *pj = j;
    if (first<0 || end<first || end>last) {
        x->err = herror(a, s, start, j, "bad word specifier");
        return {};
    }
    Buf b(a, text.len);
    for (iz k = first; k <= end; k++) {
        if (k > first) putbyte(&b, ' ');
        print(&b, w.word[k]);
    }
    return finish(&b);
}

// The modifiers at s[*j] applied to text. Null with x->err on failure.
static Str hmodify(Shell *sh, Arena *a, HExp *x, Str text, iz *pj)
{
    Hist *h = &sh->hist;
    Str   s = x->line;
    iz    j = *pj;
    iz    start = j;
    while (j+1<s.len && s[j]==':') {
        u8  c  = s[j+1];
        b32 global = 0;
        b32 each   = 0;
        j += 2;
        if ((c=='g' || c=='a' || c=='G') && j<s.len) {
            global = c != 'G';
            each   = c == 'G';
            c = s[j++];
        }
        switch (c) {
        case 'h': {
            iz k = text.len;
            while (k>0 && text[k-1]!='/') k--;
            if (k) text = takehead(text, k-1);
        } break;
        case 't': {
            iz k = text.len;
            while (k>0 && text[k-1]!='/') k--;
            if (k) text = cuthead(text, k);
        } break;
        case 'r': {
            iz k = text.len;
            while (k>0 && text[k-1]!='.') k--;
            if (k) text = takehead(text, k-1);
        } break;
        case 'e': {
            iz k = text.len;
            while (k>0 && text[k-1]!='.') k--;
            if (k) text = cuthead(text, k-1);
        } break;
        case 'p': x->print = 1; break;
        case 'q': x->quote = 1; break;
        case 'x': x->quote = 2; break;
        case 's':
        case '&': {
            if (c == 's') {
                if (j >= s.len) {
                    *pj = j;
                    return text;  // a bare s at the end does nothing
                }
                u8  delim = s[j++];
                Str old   = hsubpart(a, s, &j, delim, 0);
                Str rep   = hsubpart(a, s, &j, delim, 1);
                if (old.len) {
                    h->subold = hset(sh, h->subold, old);
                } else if (!h->subold.data && h->search.data) {
                    h->subold = hset(sh, h->subold, h->search);
                }
                h->subnew = hset(sh, h->subnew, rep);
                if (!h->subnew.data) h->subnew = sclone(sh->store, "");
            }
            if (!h->subold.data) {
                x->err = herror(a, s, start, j, "no previous substitution");
                return {};
            }
            Str rep = hrhs(a, h->subnew, h->subold);
            Str r   = h->subold.len>text.len ? Str{} :
                      hsubst(a, text, h->subold, rep, global, each);
            if (!r.data) {
                x->err = herror(a, s, start, j, "substitution failed");
                return {};
            }
            text = r;
        } break;
        default:
            x->err = herror(a, s, j-1, j, "unrecognized history modifier");
            return {};
        }
    }
    *pj = j;
    return text;
}

// The event at s[*pj] (just past the expansion character). Null with
// x->err set on failure; *matchat is where a ?string? search matched.
static Str hevent(Shell *sh, Arena *a, HExp *x, iz bang, iz *pj, b32 dq, iz *matchat)
{
    Hist *h = &sh->hist;
    Str   s = x->line;
    iz    j = *pj;
    u8    c = j<s.len ? s[j] : 0;
    *matchat = -1;
    if (c == h->expchar) {
        *pj = j + 1;
        if (!x->hlen) goto notfound;
        return hentry(sh, x->hlen-1);
    }
    if (c==':' || c=='$' || c=='*' || c=='%' || c=='^') {
        if (!x->hlen) goto notfound;
        return hentry(sh, x->hlen-1);
    }
    if (c == '#') {
        *pj = j + 1;
        return clone(a, Str{x->out.data, x->out.len});
    }
    if (digit(c) || (c=='-' && j+1<s.len && digit(s[j+1]))) {
        b32 neg = c == '-';
        j += neg;
        i64 v = 0;
        for (; j<s.len && digit(s[j]); j++) {
            v = v*10 + (s[j]-'0');
            if (v > (i64)1<<40) v = (i64)1<<40;
        }
        *pj = j;
        i64 idx = neg ? x->hlen-v : v-h->base;
        if (idx<0 || idx>=x->hlen) goto notfound;
        return hentry(sh, (iz)idx);
    }
    if (c == '?') {
        iz k = ++j;
        for (; k<s.len && s[k]!='?' && s[k]!='\n'; k++) {}
        Str pat = span(s.data+j, s.data+k);
        j = k + (k<s.len && s[k]=='?');
        *pj = j;
        if (!pat.len) {
            if (!h->search.data) goto notfound;
            pat = h->search;
        } else {
            h->search = hset(sh, h->search, pat);
        }
        for (iz e = x->hlen-1; e >= 0; e--) {
            iz at = hrfind(hentry(sh, e), pat);
            if (at >= 0) {
                *matchat = at;
                return hentry(sh, e);
            }
        }
        goto notfound;
    }
    {
        iz k = j;
        for (; k < s.len; k++) {
            u8 d = s[k];
            if (hblank(d) || d==':' || (d=='-' && k>j) || d=='^' || d=='$' ||
                d=='*' || d=='%' || d==';' || d=='&' || d=='(' || d==')' ||
                d=='|' || d=='<' || d=='>' || (dq && d=='"')) break;
        }
        Str pre = span(s.data+j, s.data+k);
        *pj = k;
        for (iz e = x->hlen-1; pre.len && e>=0; e--) {
            if (startswith(hentry(sh, e), pre)) return hentry(sh, e);
        }
    }
    notfound:
    x->err = herror(a, s, bang, *pj, "event not found");
    return {};
}

// Expand the line's history references. -1 on failure (with the message
// in err), 0 if there are none, 1 if expanded, 2 if only to be printed.
// quote is the parser's open quote the line continues, if any.
struct HResult {
    Str line;
    Str err;
    i32 r;
};

static HResult histexpand(Shell *sh, Arena *a, Str line, iz hlen, u8 quote)
{
    Hist   *h = &sh->hist;
    HResult r = {};
    r.line = line;
    u8 bang = h->expchar;
    if (!bang) return r;
    if (h->subchar && line.len && line[0]==h->subchar && quote!='\'') {
        Buf b(a, line.len+4);
        putbyte(&b, bang);
        putbyte(&b, bang);
        print(&b, ":s");
        print(&b, line);
        line = finish(&b);
    }
    HExp x = {};
    x.out  = Buf(a, line.len+64);
    x.line = line;
    x.hlen = hlen;
    b32 changed = 0;
    for (i32 pass = line.data!=r.line.data; pass < 2; pass++) {
    // The first pass only looks for a reference not vetoed by the line
    // as read; the second expands, and vetoes by the line made so far.
    b32 found = 0;
    b32 dq = quote == '"';
    iz  i  = 0;
    x.out.len = 0;
    if (quote == '\'') {
        for (; i<line.len && line[i]!='\''; i++) {}
        i += i < line.len;
        print(&x.out, takehead(line, i));
    }
    while (i < line.len) {
        u8 c = line[i];
        u8 n = i+1<line.len ? line[i+1] : 0;
        if (c == '\\') {
            putbyte(&x.out, c);
            if (i+1 < line.len) putbyte(&x.out, n);
            i += 2;
            continue;
        }
        if (c=='\'' && !dq) {
            iz k = i + 1;
            b32 dollar = i>0 && line[i-1]=='$';
            for (; k<line.len && line[k]!='\''; k++) {
                if (dollar && line[k]=='\\') k++;
            }
            k += k < line.len;
            if (k > line.len) k = line.len;
            print(&x.out, span(line.data+i, line.data+k));
            i = k;
            continue;
        }
        if (c == '"') {
            dq = !dq;
            putbyte(&x.out, c);
            i++;
            continue;
        }
        if (h->comchar && c==h->comchar && !dq &&
                (i==0 || hdelim(line[i-1]))) {
            print(&x.out, cuthead(line, i));
            break;
        }
        if (c != bang) {
            putbyte(&x.out, c);
            i++;
            continue;
        }
        b32 literal = !n || hblank(n) || n=='\r' || n=='=' || n==';' ||
                      n=='&' || n=='|' || n=='(' || n==')' || n=='<' ||
                      n=='>' || (dq && n=='"');
        b32 veto = 0;
        if (!literal && !pass) {
            veto = hveto(sh, line, i, quote);
        } else if (!literal) {
            Buf v(a, x.out.len+2);
            print(&v, Str{x.out.data, x.out.len});
            putbyte(&v, c);
            putbyte(&v, n);
            veto = hveto(sh, finish(&v), x.out.len, quote);
        }
        if (literal || veto) {
            putbyte(&x.out, c);
            i++;
            continue;
        }
        if (!pass) {
            found = 1;
            break;
        }
        iz  j       = i + 1;
        iz  matchat = -1;
        Str ev = hevent(sh, a, &x, i, &j, dq, &matchat);
        if (!ev.data) {
            r.r   = -1;
            r.err = clone(a, x.err);
            return r;
        }
        Str w = hdesignate(sh, a, &x, ev, &j, matchat);
        if (w.data) w = hmodify(sh, a, &x, w, &j);
        if (!w.data) {
            r.r   = -1;
            r.err = clone(a, x.err);
            return r;
        }
        if (x.quote) w = hquote(a, w, x.quote==2);
        x.quote = 0;
        print(&x.out, w);
        changed = 1;
        i = j;
    }
    if (!pass && !found) return r;
    }
    r.line = finish(&x.out);
    r.r    = x.print ? 2 : changed;
    return r;
}

// The reader's line: expanded when history expansion is on. *drop when
// it is not to run: a failure (reported), or a :p line (shown).
static Str histexpandline(Shell *sh, Lexer *L, Str line, b32 *drop)
{
    Hist *h = &sh->hist;
    if (!sh->opt[O_HISTEXPAND] || h->expinhibit || h->noexpand) return line;
    if (!h->expchar) return line;
    b32 any = 0;
    for (iz i = 0; i<line.len && !any; i++) {
        any = line[i]==h->expchar || (h->subchar && line[i]==h->subchar);
    }
    if (!any) return line;
    Arena a = *scratchat(sh, sh->capdepth+1);
    iz hlen = h->len;
    if (L->cmdlines>1 && sh->shopt[S_CMDHIST] && h->firstsaved && hlen) hlen--;
    i32 cd = L->ddepth ? currentdelim(L) : 0;
    HResult r = histexpand(sh, &a, line, hlen, cd=='\'' || cd=='"' ? (u8)cd : 0);
    if (r.r < 0) {
        // As bash, the line is not counted from here on
        i32 saved = sh->line;
        sh->line = (i32)L->lineno;
        L->lineno--;
        shellerror(sh, a, r.err);
        sh->line = saved;
        if (sh->shopt[S_HISTREEDIT] && sh->interactive) {
            h->preload = hset(sh, h->preload, line);
        }
        L->cmdlines--;
        h->cmdlines = L->cmdlines;
        *drop = 1;
        return {};
    }
    if (!r.r) return line;
    if (r.r==1 && sh->shopt[S_HISTVERIFY] && sh->interactive) {
        h->preload = hset(sh, h->preload, r.line);
        L->lineno--;
        L->cmdlines--;
        *drop = 1;
        return {};
    }
    flushbout(sh);
    fdwrite(sh, 2, concat(&a, r.line, "\n"));
    r.line = clone(L->a, r.line);  // maybeadd reuses the scratch arena
    if (r.r == 2) {
        if (r.line.len) maybeadd(sh, L, r.line);
        L->lineno--;
        L->cmdlines--;
        *drop = 1;
        return {};
    }
    return r.line;
}

// history -p: one argument expanded, the library's way.
static Str histexpandarg(Shell *sh, Arena *a, Str s, i32 *result)
{
    HResult r = histexpand(sh, a, s, sh->hist.len, 0);
    *result = r.r;
    return r.line;
}


// The history file (readline's histfile.c)

// HISTFILE, or readline's default when it is unset.
static Str histfile(Shell *sh, Arena *a)
{
    Got g = getvar(sh, a, "HISTFILE");
    if (g.set) return g.value;
    return concat(a, getvar(sh, a, "HOME").value, "/.history");
}

// A time stamp line: the comment character and a digit.
static b32 isstamp(Str s, iz i, u8 com)
{
    return com && i+1<s.len && s[i]==com && digit(s[i+1]);
}

static b32 writestamps(Shell *sh)
{
    return sh->hist.writets;
}

// Append the entries of a file to the list, skipping its first from
// lines (time stamp lines aside): readline's read_history_range. The
// lines it went through go in readlines, 0 on an error. An error, or 0.
static i32 histread(Shell *sh, Str file, i64 from, Arena scratch)
{
    Hist *h  = &sh->hist;
    Stat  st = pathstat(sh, scratch, file, 1);
    if (st.type!=FT_FILE && !st.err) return E_INVAL;  // count kept
    h->readlines = 0;
    if (st.err) return st.err;
    i32 err = 0;
    Str buf = readwholefile(sh, &scratch, file, &err);
    if (err) return err;

    u8 com = h->comchar;
    if (!com && buf.len>1 && buf[0]=='#' && digit(buf[1])) com = '#';
    if (isstamp(buf, 0, com) && writestamps(sh)) h->multiline++;
    b32 multi = h->multiline > 0;

    i64 line = 0;
    iz  pos  = 0;
    while (line<from && pos<buf.len) {
        Cut c = cut(cuthead(buf, pos), '\n');
        if (!c.ok) {
            pos = buf.len;  // an unterminated last line is not read
            break;
        }
        line += !isstamp(c.head, 0, com);
        pos  += c.head.len + 1;
    }

    Str  stamp   = {};  // a time stamp line waiting for its entry
    b32  afterts = 0;   // the last line was a time stamp
    while (pos < buf.len) {
        Cut c = cut(cuthead(buf, pos), '\n');
        if (!c.ok) break;
        pos += c.head.len + 1;
        Str text = c.head;
        if (text.len && text[text.len-1]=='\r') text.len--;
        if (isstamp(text, 0, com)) {
            stamp   = text;
            afterts = 1;
            continue;
        }
        line++;
        if (!text.len && (!multi || afterts)) continue;
        if (multi && !afterts && h->len) {
            Arena t   = scratch;
            Str   cur = h->ents[h->len-1]->line;
            histreplace(sh, h->len-1, concat3(&t, cur, "\n", text));
        } else {
            byte  mem[32];
            Arena t = {mem, mem+sizeof(mem)};
            histpush(sh, text, stamp.data ? stamp : histstamp(sh, &t, com));
        }
        stamp   = {};
        afterts = 0;
    }
    h->readlines = line;
    return 0;
}

// Replace a history file's contents, as readline does: written to
// FILE-PID.tmp beside it, then renamed over it, so the file is new (mode
// 0600). A symbolic link, which readline follows to write beside its
// target, is written through in place (the core cannot read links). An
// error, or 0.
static i32 histreplace(Shell *sh, Str file, Str data, Arena scratch)
{
    Str  path = absolute(sh, &scratch, file);
    Stat ls   = os_stat(sh->os, scratch, path, 0);
    Str  tmp  = path;
    if (ls.err || ls.type!=FT_LINK) {
        Buf t(&scratch, path.len+24);
        print(&t, path);
        print(&t, "-");
        print(&t, (i64)sh->pid);
        print(&t, ".tmp");
        tmp = finish(&t);
    }
    Opened o = os_open(sh->os, scratch, tmp, OF_WRITE|OF_CREATE|OF_TRUNC, 0600);
    if (o.err) return o.err;
    b32 ok = !data.len || os_write(sh->os, o.h, data.data, data.len);
    os_close(sh->os, o.h);
    i32 err = ok ? 0 : E_IO;
    if (tmp.data != path.data) {
        if (!err) err = os_rename(sh->os, scratch, tmp, path);
        if (err) os_unlink(sh->os, scratch, tmp);
    }
    return err;
}

// Write entries [from, len) to a file, each after its time stamp line
// when HISTTIMEFORMAT is set: appended to a file that must exist, or
// replacing it. An error, or 0.
static i32 histwrite(Shell *sh, Str file, iz from, b32 append, Arena scratch)
{
    Hist *h  = &sh->hist;
    b32   ts = writestamps(sh);
    Buf b(&scratch, 4096);
    for (iz i = from; i < h->len; i++) {
        HistEnt *e = h->ents[i];
        if (ts && e->stamp.len && e->stamp[0]) {
            print(&b, e->stamp);
            putbyte(&b, '\n');
        }
        print(&b, e->line);
        putbyte(&b, '\n');
    }
    Str data = finish(&b);
    if (!append) return histreplace(sh, file, data, scratch);
    Str    path = absolute(sh, &scratch, file);
    Opened o    = os_open(sh->os, scratch, path, OF_WRITE|OF_APPEND, 0600);
    if (o.err) return o.err;
    b32 ok = !data.len || os_write(sh->os, o.h, data.data, data.len);
    os_close(sh->os, o.h);
    return ok ? 0 : E_IO;
}

// Cut a history file to its last n entries, each time stamp kept with
// its entry: readline's history_truncate_file.
static void histtruncate(Shell *sh, Str file, i64 n, Arena scratch)
{
    Stat st = pathstat(sh, scratch, file, 1);
    if (st.err || st.type!=FT_FILE) return;
    Str keep = {};
    if (n) {
        i32 err = 0;
        Str buf = readwholefile(sh, &scratch, file, &err);
        if (err || !buf.len) return;
        u8  com   = sh->hist.comchar;
        b32 ts    = writestamps(sh);
        i64 lines = n + ts;
        // Count newlines back from the end, those before a time stamp
        // line aside, until n lines are behind.
        iz i = buf.len - 1;
        for (; i>0 && lines>0; i--) {
            if (buf[i]=='\n' && !isstamp(buf, i+1, com)) lines--;
        }
        // Then back to the start of a line, or of a time stamp line
        for (; i > 0; i--) {
            if (buf[i]=='\n' && (!ts || isstamp(buf, i+1, com))) {
                i++;
                break;
            }
        }
        if (i <= 0) return;  // no more lines than that
        keep = cuthead(buf, i);
    }
    histreplace(sh, file, keep, scratch);
}

// HISTFILESIZE: a number of 0 or more cuts the file at once.
static void histfilesize(Shell *sh)
{
    Hist *h = &sh->hist;
    Arena a = *scratchat(sh, sh->capdepth+1);
    Got   g = getvar(sh, &a, "HISTFILESIZE");
    i64   n = 0;
    if (!g.value.len || !histnum(g.value, &n) || n<0) return;
    histtruncate(sh, histfile(sh, &a), n, a);
    if (n < h->infile) h->infile = n;
}

// Create a missing file, empty. An error, or 0.
static i32 histcreate(Shell *sh, Str file, Arena scratch)
{
    if (!pathstat(sh, scratch, file, 1).err) return 0;
    Str    path = absolute(sh, &scratch, file);
    Opened o    = os_open(sh->os, scratch, path, OF_WRITE|OF_CREATE, 0600);
    if (o.err) return o.err;
    os_close(sh->os, o.h);
    return 0;
}

// At exit with history on: this session's entries go to the end of the
// file, or the whole list replaces it when some of those are already
// gone from the list. Then HISTFILESIZE cuts it.
static void histexit(Shell *sh)
{
    Hist *h = &sh->hist;
    if (!sh->opt[O_HISTORY] || h->session<=0) return;
    Arena a    = *scratchat(sh, sh->capdepth+1);
    Str   file = getvar(sh, &a, "HISTFILE").value;
    if (!file.len) return;
    histcreate(sh, file, a);
    if (h->session<=h->len || sh->shopt[S_HISTAPPEND]) {
        iz n = h->session<h->len ? (iz)h->session : h->len;
        histwrite(sh, file, h->len-n, 1, a);
        h->infile += h->session;
    } else {
        histwrite(sh, file, 0, 0, a);
        h->infile = h->len;
    }
    h->session = 0;
    histfilesize(sh);
}

// history -a, -n, -r, -w, with the file args[i] or HISTFILE.
static i32 histfileop(Shell *sh, i32 op, Str *args, iz nargs, iz i, Arena scratch)
{
    Hist *h    = &sh->hist;
    Str   file = i<nargs ? args[i] : getvar(sh, &scratch, "HISTFILE").value;
    if (!file.len) {
        Str msg = i<nargs ? Str("empty filename") : "HISTFILE: parameter null or not set";
        builtinerror(sh, scratch, "history", msg);
        return !sh->interactive;
    }
    if (sh->restricted && find(file, "/")>=0) {
        builtinerror(sh, scratch, "history", concat(&scratch, file, ": restricted"));
        return 1;
    }
    i32 err = 0;
    switch (op) {
    case 'a':
        if (h->session > 0) {
            err = histcreate(sh, file, scratch);
            if (err) {
                Str msg = concat3(&scratch, file, ": cannot create: ", errtext(err));
                builtinerror(sh, scratch, "history", msg);
                return 1;
            }
            if (h->session > h->len) h->session = h->len;
            err = histwrite(sh, file, h->len-(iz)h->session, 1, scratch);
            h->infile += h->session;
        }
        h->session = 0;
        break;
    case 'w':
        err = histwrite(sh, file, 0, 0, scratch);
        break;
    case 'r':
        err = histread(sh, file, 0, scratch);
        h->infile = h->readlines;
        break;
    case 'n': {
        i64 old  = h->infile;
        i64 base = h->base;
        err = histread(sh, file, h->infile, scratch);
        h->infile = h->readlines;
        if (!sh->shopt[S_HISTAPPEND]) {
            h->session += h->infile - old + h->base - base;
        }
    } break;
    }
    return err ? 1 : 0;
}


// The fc builtin

enum : i32 { FC_NOTFOUND = -1, FC_INVALID = -2 };

// An fc operand as an entry index (bash's fc_gethnum): last is the
// newest entry it may name, real the newest of all.
static iz fcentry(Shell *sh, Str spec, iz last, iz real, b32 first, b32 listing)
{
    Hist *h = &sh->hist;
    if (last < 0) return FC_NOTFOUND;
    if (!spec.data) return last;
    b32 neg = spec.len && spec[0]=='-';
    Str num = cuthead(spec, neg);
    if (num.len && digit(num[0])) {
        i64 n = 0;
        for (iz k = 0; k<num.len && digit(num[k]); k++) {
            n = n*10 + (num[k]-'0');
            if (n > (i64)1<<40) n = (i64)1<<40;
        }
        if (neg) {
            if (!n) return listing ? real : (iz)FC_INVALID;
            i64 idx = last + 1 - n;
            return idx<0 ? 0 : (iz)idx;
        }
        if (!n) return last;
        i64 idx = n - h->base;
        if (idx<0 || idx>=last) return first ? 0 : last;
        return (iz)idx;
    }
    for (iz i = last; i >= 0; i--) {
        if (startswith(h->ents[i]->line, spec)) return i;
    }
    return FC_NOTFOUND;
}

static b32 fcnumber(Str s)
{
    iz i = s.len && s[0]=='-';
    if (i >= s.len) return 0;
    for (; i < s.len; i++) {
        if (!digit(s[i])) return 0;
    }
    return 1;
}

// Every occurrence of pat replaced by rep; an empty pat replaces each
// character.
static Str fcreplace(Arena *a, Str s, Str pat, Str rep)
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len;) {
        if (!pat.len) {
            print(&b, rep);
            i++;
        } else if (pat.len<=s.len-i && takehead(cuthead(s, i), pat.len)==pat) {
            print(&b, rep);
            i += pat.len;
        } else {
            putbyte(&b, s[i++]);
        }
    }
    return finish(&b);
}

// Run a string with history off, as bash's parse_and_execute does.
static i32 fcrun(Shell *sh, Str src, Arena scratch)
{
    b32 saved = sh->opt[O_HISTORY];
    sh->opt[O_HISTORY] = 0;
    sh->evaldepth++;
    i32 line   = sh->line;
    i32 status = runsource(sh, src, "fc", scratch, 0, line);
    sh->line = line;
    sh->evaldepth--;
    sh->opt[O_HISTORY] = saved;
    return status;
}

// A new file in TMPDIR (or /tmp) for fc to edit, named as bash's
// mkstemp makes it (bash-fc.XXXXXX): its name, or an error.
static Opened fctemp(Shell *sh, Arena *a, Str *name)
{
    Str dir = getvar(sh, a, "TMPDIR").value;
    if (!dir.len || pathstat(sh, *a, dir, 1).type!=FT_DIR) dir = "/tmp";
    Opened o = {};
    for (i32 try_ = 0; try_ < 100; try_++) {
        u64 r = (u64)dynnow(sh) ^ (u64)sh->bashpid*0x9e3779b97f4a7c15u ^
                (u64)try_*0x2545f4914f6cdd1du;
        Buf b(a, dir.len+24);
        print(&b, dir);
        print(&b, "/bash-fc.");
        Str set = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        for (i32 k = 0; k < 6; k++) {
            putbyte(&b, set[(iz)(r%62)]);
            r /= 62;
        }
        *name = finish(&b);
        Arena t = *a;
        o = os_open(sh->os, t, absolute(sh, &t, *name), OF_WRITE|OF_CREATE|OF_EXCL, 0600);
        if (o.err != E_EXIST) break;
    }
    return o;
}

static i32 b_fc(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    Hist *h = &sh->hist;
    Str  ename   = {};
    b32  listing = 0;
    b32  nonum   = 0;
    b32  reverse = 0;
    b32  exec    = 0;
    iz   i       = 1;
    for (; i < nargs; i++) {
        Str a = args[i];
        if (a == "--") {
            i++;
            break;
        }
        if (a.len<2 || a[0]!='-' || fcnumber(a)) break;
        for (iz k = 1; k < a.len; k++) {
            switch (a[k]) {
            case 'l': listing = 1; break;
            case 'n': nonum   = 1; break;
            case 'r': reverse = 1; break;
            case 's': exec    = 1; break;
            case 'e':
                if (k+1 < a.len) {
                    ename = cuthead(a, k+1);
                } else if (i+1 < nargs) {
                    ename = args[++i];
                } else {
                    builtinerror(sh, scratch, "fc", "-e: option requires an argument");
                    usageerror(sh, scratch, "fc");
                    return 2;
                }
                k = a.len;
                break;
            default:
                invalidoption(sh, scratch, "fc", span(a.data+k-1, a.data+k+1));
                return 2;
            }
        }
    }
    if (ename == "-") exec = 1;

    iz real = h->len - 1;
    iz last = h->len - (sh->opt[O_HISTORY] ? 1 : 0) - (iz)h->lastadded;
    if (last < 0) last = 0;
    if (!sh->opt[O_HISTORY]) last = h->len - 1;

    if (exec) {
        Str *pats = alloc<Str>(&scratch, nargs);
        Str *reps = alloc<Str>(&scratch, nargs);
        iz   npairs = 0;
        for (; i < nargs; i++) {
            Cut c = cut(args[i], '=');
            if (!c.ok) break;
            pats[npairs]   = c.head;
            reps[npairs++] = c.tail;
        }
        Str spec = i<nargs ? args[i++] : Str{};
        iz  e    = h->len ? fcentry(sh, spec, last, real, 0, 0) : (iz)FC_NOTFOUND;
        if (e < 0) {
            builtinerror(sh, scratch, "fc", "no command found");
            return 1;
        }
        if (sh->opt[O_POSIX] && i<nargs) {
            builtinerror(sh, scratch, "fc", "too many arguments");
            return 1;
        }
        Str cmd = h->ents[e]->line;
        for (iz k = 0; k < npairs; k++) {
            cmd = fcreplace(&scratch, cmd, pats[k], reps[k]);
        }
        flushbout(sh);
        fdwrite(sh, 2, concat(&scratch, cmd, "\n"));
        if (sh->opt[O_HISTORY]) {
            Str add = cmd;
            if (add.len && add[add.len-1]=='\n') add.len--;
            if (h->len) {
                histremove(sh, h->len-1, 1);
                h->session -= h->session > 0;
            }
            Arena t = scratch;
            checkadd(sh, 0, clone(&t, add), 1, t);
        }
        return fcrun(sh, cmd, scratch);
    }

    if (!h->len) return 0;
    Str fspec = i<nargs ? args[i++] : Str{};
    Str lspec = i<nargs ? args[i++] : Str{};
    iz first = 0;
    iz end   = 0;
    if (fspec.data) {
        first = fcentry(sh, fspec, last, real, 1, listing);
        if (lspec.data) {
            end = fcentry(sh, lspec, last, real, 0, listing);
        } else {
            end = listing ? last : first;
            if (first == real) end = real;
        }
    } else if (listing) {
        first = last>15 ? last-15 : 0;
        end   = last;
    } else {
        first = end = last;
    }
    if (first==FC_INVALID || end==FC_INVALID) {
        builtinerror(sh, scratch, "fc", "history specification out of range");
        return 1;
    }
    if (first<0 || end<0) {
        builtinerror(sh, scratch, "fc", "no command found");
        return 1;
    }

    if (!listing && sh->opt[O_HISTORY] && h->lastadded) {
        // The fc line itself goes before editing
        histremove(sh, h->len-1, 1);
        h->session -= h->session > 0;
        h->lastadded = 0;
        iz top = h->len - 1;
        if (top < 0) return 0;
        if (first > top) first = top;
        if (end > top)   end   = top;
    }
    if (first > end) {
        iz t  = first;
        first = end;
        end   = t;
        reverse = 1;
    }

    if (listing) {
        for (iz k = 0; k <= end-first; k++) {
            iz n = reverse ? end-k : first+k;
            HistEnt *ent = h->ents[n];
            if (!nonum) bprinti(sh, h->base+n);
            bprint(sh, "\t");
            if (!sh->opt[O_POSIX]) bprint(sh, ent->edited ? Str("*") : " ");
            bprint(sh, ent->line);
            bprint(sh, "\n");
        }
        return 0;
    }

    // Edit the lines in a file, then run it
    Str    file = {};
    Opened o    = fctemp(sh, &scratch, &file);
    if (o.err) {
        Str msg = concat3(&scratch, file, ": cannot open temp file: ", errtext(o.err));
        builtinerror(sh, scratch, "fc", msg);
        return 1;
    }
    {
        Arena t = scratch;
        Buf   b(&t, 256);
        for (iz k = 0; k <= end-first; k++) {
            print(&b, h->ents[reverse ? end-k : first+k]->line);
            putbyte(&b, '\n');
        }
        if (b.len) os_write(sh->os, o.h, b.data, b.len);
        os_close(sh->os, o.h);
    }
    Str editor = ename;
    if (!editor.data) {
        editor = getvar(sh, &scratch, "FCEDIT").value;
        if (!editor.len) editor = getvar(sh, &scratch, "EDITOR").value;
        if (!editor.len) editor = sh->opt[O_POSIX] ? Str("ed") : "vi";
    }
    Str cmd    = concat3(&scratch, editor, " ", file);
    i32 status = fcrun(sh, cmd, scratch);
    if (!status) {
        i32 err = 0;
        Str src = readwholefile(sh, &scratch, file, &err);
        if (!err) {
            b32 hist    = sh->opt[O_HISTORY];
            b32 verbose = sh->opt[O_VERBOSE];
            b32 noexp   = h->noexpand;
            sh->opt[O_HISTORY] = 1;
            sh->opt[O_VERBOSE] = 1;
            h->noexpand = 1;
            sh->evaldepth++;
            i32 line = sh->line;
            status = runsource(sh, src, file, scratch, 0, 1, 0, 0, 0, 1);
            sh->line = line;
            sh->evaldepth--;
            h->noexpand = noexp;
            sh->opt[O_VERBOSE] = verbose;
            sh->opt[O_HISTORY] = hist;
        } else {
            status = 1;
        }
    } else {
        status = 1;
    }
    os_unlink(sh->os, scratch, absolute(sh, &scratch, file));
    return status;
}
