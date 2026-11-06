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

// The time stamp a new entry gets: the comment character and the
// epoch seconds, or nothing while there is no comment character.
static Str histstamp(Shell *sh, Arena *a)
{
    Hist *h = &sh->hist;
    if (!h->comchar) return {};
    Buf b(a, 24);
    putbyte(&b, h->comchar);
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
        histremove(sh, 0, h->len-h->max+1);
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
    histpush(sh, line, histstamp(sh, &a));
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

// Variables that steer the history, as bash's sv_* hooks.
static void histvarchanged(Shell *sh, Str name)
{
    Hist *h = &sh->hist;
    Arena none = {};
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
    } else if (name == "HISTTIMEFORMAT") {
        if (getvar(sh, &none, name).set && !h->comchar) h->comchar = '#';
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

// set -o history, set -H and their shopt -o forms.
static void histoption(Shell *sh, i32 opt, b32 on)
{
    Hist *h = &sh->hist;
    if (opt == O_HISTORY) {
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
                sh->status = 2;
                escape(sh, ESC_DISCARD);  // the rest of the line too
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


// Time stamps in listings: HISTTIMEFORMAT's strftime format, ?? for an
// entry without a stamp.
static Str histtime(Shell *sh, Arena *a, Str stamp, Str fmt)
{
    if (!stamp.len) return "??";
    i64 t = 0;
    b32 ok = stamp[0] == sh->hist.comchar;
    iz  i  = 1;
    for (; ok && i<stamp.len && digit(stamp[i]) && t<(i64)1<<50; i++) {
        t = t*10 + stamp[i] - '0';
    }
    if (!ok || !t) return concat(a, cuthead(stamp, 1), ": invalid timestamp");
    PfTm   tm  = localtm(t, shellzone(sh, a));
    PfTout out = {};
    fmttime(&out, fmt, &tm);
    return out.full ? Str{} : clone(a, Str{out.data, out.len});
}


// Stubs until the file and expansion parts land
static Str histexpandline(Shell *, Lexer *, Str line, b32 *) { return line; }
static Str histexpandarg(Shell *, Arena *, Str s, i32 *r) { *r = 0; return s; }
static i32 histfileop(Shell *, i32, Str *, iz, iz, Arena) { return 0; }
