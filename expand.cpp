// Word expansion
//
// Expansion works on the parser's word text, as bash does. One pass
// handles tilde, parameter, command and arithmetic expansion and quote
// removal, building each field as bytes plus a parallel flag per byte:
// XF_QUOTED bytes are not special to splitting or globbing, and
// XF_SPLIT bytes came from an unquoted expansion so IFS may split them.
// Field splitting and pathname expansion then work from the flags.
//
// This is free and unencumbered software released into the public domain.

// XF_NULL marks a placeholder byte for a quoted null ("" or '') in a
// word that will be split, so that it survives as an empty field next
// to a delimiter, as in ""$v. It is removed after splitting.
// XF_KEEP bytes are never split: unquoted $* or $@ elements expanded
// while IFS was null. XF_UNSET bytes were expanded while IFS was unset.
enum : u8 {
    XF_QUOTED = 1<<0, XF_SPLIT = 1<<1, XF_NULL = 1<<2, XF_KEEP = 1<<3,
    XF_UNSET  = 1<<4,
};

enum : i32 {  // expansion contexts
    XC_DQUOTE  = 1<<0,  // inside double quotes
    XC_HEREDOC = 1<<1,  // here-document body: quotes are literal
    XC_PARAM   = 1<<2,  // the word inside ${...}
    XC_ASSIGN  = 1<<3,  // assignment value: tildes after = and :
    XC_ARITH   = 1<<4,  // $((...)) text: double quotes are removed
    XC_PATTERN = 1<<5,  // a pattern: backslash-quote the result
    XC_ASSIGNW = 1<<6,  // looks like an assignment: XC_ASSIGN after =
    XC_NOPSUB  = 1<<7,  // <(...) and >(...) are plain text
    XC_SUBQ    = 1<<8,  // array subscripts expand once, quoted (as in arithmetic)
};

enum : i32 {  // expandwords flags
    XW_NOSPLIT = 1<<0,
    XW_NOGLOB  = 1<<1,
    XW_NOBRACE = 1<<2,
    XW_ASSIGN  = 1<<3,
    XW_SUBQ    = 1<<4,  // subscripts expand once, quoted (unset name[sub])
};

struct XField {
    Str data;
    Str flags;
    b32 quoted;  // had quotes: kept even when empty
};

struct X {
    Shell        *sh;
    Arena        *a;
    Buf           data;
    Buf           flags;
    b32           quoted;
    b32           atempty;  // "$@" with no parameters in this word
    Slice<XField> done;
    b32           nosplit;  // $@ and $* join instead of breaking fields
    b32           nullmark; // mark quoted nulls with XF_NULL bytes
    b32           splitspace;  // unquoted $* with IFS unset: see putlist
    Str           text;     // the text being expanded, for bad substitutions
};

// The runner side: defined with the executor.
static Str  runcapture(Shell *, Arena *, Str src, b32 backquote);
static Str  procsubst(Shell *, Arena *, Str body, b32 output);
static void expansionerror(Shell *, Arena, Str msg);
static void fatalerror(Shell *, Arena, Str msg);
static Arith evalarith(Shell *, Arena *, Str);
static Arith evalinteger(Shell *, Arena *, Str);
static Arith arithraw(Shell *, Arena *, Str);

static X *newx(Shell *sh, Arena *a)
{
    X *x = alloc<X>(a);
    x->sh    = sh;
    x->a     = a;
    x->data  = Buf(a, 64);
    x->flags = Buf(a, 64);
    return x;
}

static void xput(X *x, Str s, u8 flag)
{
    print(&x->data, s);
    reserve(&x->flags, s.len);
    __builtin_memset(x->flags.data+x->flags.len, flag, (uz)s.len);
    x->flags.len += s.len;
}

static void xputc(X *x, u8 c, u8 flag)
{
    putbyte(&x->data, c);
    putbyte(&x->flags, flag);
}

// End the current field (for "$@") and start another.
// A quoted null: marked when the word will be split.
static void xnull(X *x)
{
    x->quoted = 1;
    if (x->nullmark) {
        xputc(x, 0, XF_QUOTED|XF_NULL);
    }
}

static void xbreak(X *x)
{
    XField f = {};
    f.data   = clone(x->a, finish(&x->data));
    f.flags  = clone(x->a, finish(&x->flags));
    f.quoted = x->quoted;
    x->done  = push(x->a, x->done, f);
    x->data.len  = 0;
    x->flags.len = 0;
    x->quoted    = 0;
}

static XField xfinish(X *x)
{
    XField f = {};
    f.data   = finish(&x->data);
    f.flags  = finish(&x->flags);
    f.quoted = x->quoted;
    return f;
}


// Scanning helpers

// Index just past a ${...} or $(...) starting at s[i] (the $).
// An unterminated $( in a here-document, found as it is expanded: the
// command is abandoned. The line counts on through the rest of the text.
// Elsewhere (a $( in a quoted ${...} word, for instance), the parse of
// its text hits the end in a quote, or one line further on, at the
// missing parenthesis.
static void comsubeof(Shell *sh, Arena scratch, Str rest, b32 hd = 1)
{
    i32 line = sh->line + 1;
    for (iz i = 0; i < rest.len; i++) line += rest[i]=='\n';
    u8 quote = 0;
    for (iz i = 2; !hd && i < rest.len; i++) {
        u8 c = rest[i];
        if (quote == '\'') {
            quote = c=='\'' ? 0 : quote;
        } else if (c == '\\') {
            i++;
        } else if (quote) {
            quote = c=='"' ? 0 : quote;
        } else if (c=='\'' || c=='"') {
            quote = c;
        }
    }
    line += !hd && !quote;
    flushbout(sh);
    Buf b(&scratch, 128);
    print(&b, errname(sh));
    print(&b, ": command substitution: line ");
    print(&b, (i64)line);
    print(&b, ": unexpected EOF while looking for matching `");
    putbyte(&b, quote ? quote : ')');
    print(&b, "'\n");
    fdwrite(sh, 2, finish(&b));
    sh->status = 1;
    escape(sh, ESC_DISCARD);
}

static iz skipdollar(Shell *sh, Arena *a, Str s, iz i, b32 dquote = 0, b32 *ended = 0)
{
    Lexer *L = newlexer(a, {});
    L->extglob = sh->shopt[S_EXTGLOB];
    L->posix   = sh->opt[O_POSIX];
    if (i+2<s.len && s[i+1]=='(' && s[i+2]=='(') {
        // $((...)): the matching )) at depth zero
        i32 depth = 0;
        for (iz j = i+1; j < s.len; j++) {
            u8 c = s[j];
            if (c == '\\') {
                j++;
            } else if (c == '\'') {
                for (j++; j<s.len && s[j]!='\''; j++) {}
            } else if (c == '"') {
                for (j++; j<s.len && s[j]!='"'; j++) {
                    if (s[j] == '\\') j++;
                }
            } else if (c == '(') {
                depth++;
            } else if (c == ')') {
                if (!--depth) return j + 1;
            }
        }
        return s.len;
    }
    return skipexpansion(L, s, i, dquote, ended);
}

// Is the $((...)) from s[i] to s[e] arithmetic? Otherwise it is a
// command substitution starting with a subshell, as in $((a) 2>&1),
// where the parentheses inside do not balance before the final )).
static b32 isarith(Str s, iz i, iz e)
{
    if (i+4>=e || s[i+2]!='(' || s[e-1]!=')' || s[e-2]!=')') return 0;
    i32 depth = 0;
    for (iz j = i+3; j < e-2; j++) {
        u8 c = s[j];
        if (c == '\\') {
            j++;
        } else if (c == '\'') {
            for (j++; j<e-2 && s[j]!='\''; j++) {}
        } else if (c == '"') {
            for (j++; j<e-2 && s[j]!='"'; j++) {
                if (s[j] == '\\') j++;
            }
        } else if (c == '(') {
            depth++;
        } else if (c==')' && --depth<0) {
            return 0;
        }
    }
    return 1;
}

// Index just past the closing backquote for one at s[i], or -1.
static iz skipbackquote(Str s, iz i)
{
    for (iz j = i+1; j < s.len; j++) {
        if (s[j] == '\\') {
            j++;
        } else if (s[j] == '`') {
            return j + 1;
        }
    }
    return -1;
}

// The body of a backquoted command: backslash removed before $ ` and
// \, and also before " inside double quotes.
static Str backquotebody(Arena *a, Str s, b32 dquote)
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        if (s[i]=='\\' && i+1<s.len) {
            u8 n = s[i+1];
            if (n=='$' || n=='`' || n=='\\' || (dquote && n=='"')) {
                i++;
            }
        }
        putbyte(&b, s[i]);
    }
    return finish(&b);
}


// Parameters

static Str ifsvalue(Shell *sh, b32 *set)
{
    Arena none = {};
    Got g = getvar(sh, &none, "IFS");
    *set = g.set;
    return g.set ? g.value : Str(" \t\n");
}

static Str joinparams(Arena *a, Args args, Str sep)
{
    Buf b(a, 64);
    for (iz i = 0; i < args.n; i++) {
        if (i) print(&b, sep);
        print(&b, args.v[i]);
    }
    return finish(&b);
}

// The separator for "$*": the first character of IFS.
static Str starsep(Shell *sh)
{
    b32 set;
    Str ifs = ifsvalue(sh, &set);
    if (!set) return " ";
    if (!ifs.len) return "";
    iz  i = 0;
    if (sh->utf8) {
        utf8decode(ifs, &i);
    } else {
        i = 1;
    }
    return takehead(ifs, i);
}

static Str flagstring(Arena *a, Shell *sh)
{
    Buf b(a, 16);
    for (i32 i = 0; i < O_COUNT; i++) {
        if (sh->opt[i] && setoptions[i].letter && setoptions[i].letter!='B'
                && setoptions[i].letter!='h' && setoptions[i].letter!='H') {
            // the listing order of $- is bash's flag table order below
        }
    }
    // bash orders $- by its flag table: a b e f h k m n p t u v x B C H
    static u8 order[] = "abefhkmnptuvxBCEHPT";
    for (iz k = 0; order[k]; k++) {
        for (i32 i = 0; i < O_COUNT; i++) {
            if (setoptions[i].letter==order[k] && sh->opt[i]) {
                putbyte(&b, order[k]);
            }
        }
    }
    if (sh->interactive) putbyte(&b, 'i');
    if (sh->cflag) putbyte(&b, 'c');
    if (sh->sflag) putbyte(&b, 's');
    return finish(&b);
}

struct Param {
    Str  value;
    b32  set;
    b32  list;      // $@, $*, ${a[@]}, ${a[*]}: a list of values
    b32  star;      // * rather than @
    b32  array;     // the list is an array's elements
    b32  bad;       // a bad subscript, already reported
    Str *lv;        // the list
    i64 *li;        // array: element indices (positions if associative)
    iz   ln;
};

static Param getelems(X *, Str name, Str sub, b32 length = 0);

static Param getparam(X *x, Str name)
{
    Shell *sh = x->sh;
    Param  r  = {};
    if (name.len == 1) {
        switch (name[0]) {
        case '*': r.star = 1;  // fallthrough
        case '@':
            r.list = 1;
            r.set  = sh->args.n>0;
            r.lv   = sh->args.v;
            r.ln   = sh->args.n;
            return r;
        case '#': r.value = fmti64(x->a, sh->args.n); r.set = 1; return r;
        case '?': r.value = fmti64(x->a, sh->status); r.set = 1; return r;
        case '$': r.value = fmti64(x->a, sh->pid); r.set = 1; return r;
        case '!':
            if (sh->lastbg) {
                r.value = fmti64(x->a, sh->lastbg);
                r.set   = 1;
            }
            return r;
        case '-': r.value = flagstring(x->a, sh); r.set = 1; return r;
        case '0': r.value = dollar0(sh); r.set = 1; return r;
        }
    }
    if (digit(name[0])) {
        i64 n = 0;
        for (iz i = 0; i < name.len && n < 1<<30; i++) {
            n = n*10 + (name[i] - '0');
        }
        if (n>=1 && n<=sh->args.n) {
            r.value = sh->args.v[n-1];
            r.set   = 1;
        }
        return r;
    }
    Value *v = rawlookup(sh, name);
    if (v && (v->attrs & V_NAMEREF)) {
        // A nameref to arr[@] expands like "${arr[@]}"
        Ref ref = resolveref(sh, name);
        if (ref.hassub) return getelems(x, ref.name, ref.sub);
        v = !ref.name.data ? 0
          : ref.global     ? globalvalue(sh, ref.name)
          :                  rawlookup(sh, ref.name);
    }
    Got g = getvalue(sh, x->a, v);
    r.value = g.value;
    r.set   = g.set;
    return r;
}

static void unbound(X *x, Str name)
{
    Shell *sh = x->sh;
    Arena  a  = *x->a;
    fatalerror(sh, a, concat(&a, name, ": unbound variable"));
}

// Append a parameter's value: split-able when unquoted.
static void putvalue(X *x, Str v, i32 ctx)
{
    u8 flag = XF_QUOTED;
    if (!(ctx & (XC_DQUOTE|XC_HEREDOC))) {
        b32 set;
        ifsvalue(x->sh, &set);
        flag = set ? XF_SPLIT : XF_SPLIT|XF_UNSET;
    }
    xput(x, v, flag);
}

// Append $@ or $*, or a list of values derived from them.
static void putlist(X *x, Str *v, iz n, b32 star, i32 ctx)
{
    Shell *sh = x->sh;
    b32 dq = ctx & (XC_DQUOTE|XC_HEREDOC);
    if (dq && star) {
        Args a = {v, n};
        xput(x, joinparams(x->a, a, starsep(sh)), XF_QUOTED);
        return;
    }
    if (x->nosplit) {
        Args a = {v, n};
        Str  sep = star ? starsep(sh) : Str(" ");
        xput(x, joinparams(x->a, a, sep), dq ? XF_QUOTED : XF_SPLIT);
        return;
    }
    if (dq && !n) {
        // "$@" vanishes, though not inside an operator's word
        x->atempty = !(ctx & XC_PARAM);
        return;
    }
    // Unquoted with a non-empty IFS, the parameters are joined by its
    // first character and split again, which keeps empty ones when
    // that character is not whitespace. With a null IFS they are
    // separate fields, never split, and the word splits only on the
    // spaces of what was expanded while IFS was unset, whatever IFS is
    // by then: unset IFS; $v${IFS=}$*.
    b32 set;
    Str ifs = ifsvalue(sh, &set);
    Str sep = !dq && (!set || ifs.len) ? starsep(sh) : Str{};
    u8  vflag = dq ? XF_QUOTED : !set ? XF_SPLIT|XF_UNSET
              : !ifs.len ? XF_SPLIT|XF_KEEP : XF_SPLIT;
    x->splitspace |= !dq && set && !ifs.len;
    for (iz i = 0; i < n; i++) {
        if (i && sep.len) {
            xput(x, sep, set ? XF_SPLIT : XF_SPLIT|XF_UNSET);
        } else if (i) {
            xbreak(x);
        }
        xput(x, v[i], vflag);
        if (dq && !v[i].len) {
            xnull(x);
        } else if (dq) {
            x->quoted = 1;
        }
    }
}


// ${...}

static void expandinto(X *x, Str s, i32 ctx);

// Expand an operator's word into one field (no splitting).
static XField expandfield(X *x, Str word, i32 ctx)
{
    X *y = newx(x->sh, x->a);
    y->nosplit = 1;
    expandinto(y, word, ctx);
    return xfinish(y);
}

// Expand an operator's word into a plain string (no splitting).
static Str expandword(X *x, Str word, i32 ctx)
{
    return expandfield(x, word, ctx).data;
}

// Expand a pattern: quoted characters are backslash-escaped.
static Str expandpat(X *x, Str word, i32 ctx)
{
    X *y = newx(x->sh, x->a);
    y->nosplit = 1;
    expandinto(y, word, ctx | XC_PATTERN);
    XField f = xfinish(y);
    Buf b(x->a, f.data.len*2);
    for (iz i = 0; i < f.data.len; i++) {
        u8 c = f.data[i];
        if (f.flags[i] & XF_QUOTED) {
            if (c=='*' || c=='?' || c=='[' || c==']' || c=='\\' || c=='(' ||
                    c==')' || c=='|' || c=='+' || c=='@' || c=='!' || c=='^' ||
                    c=='-') {
                putbyte(&b, '\\');
            }
        }
        putbyte(&b, c);
    }
    return finish(&b);
}

static i32 patflags(Shell *sh)
{
    i32 f = 0;
    if (sh->shopt[S_EXTGLOB]) f |= PM_EXTGLOB;
    if (sh->utf8) f |= PM_UTF8;
    return f;
}

// Like bash, report the whole text being expanded at this level: the
// word, a double-quoted part, or an operator's word.
static void badsubst(X *x, Str whole)
{
    Arena a = *x->a;
    Str   s = x->text.data ? x->text : concat3(&a, "${", whole, "}");
    expansionerror(x->sh, a, concat(&a, s, ": bad substitution"));
}

// Index of the first unquoted c in s at the outer level, or -1. For
// the colon of ${x:off:len} (arith), skip parentheses and let each ?
// claim the next colon.
static iz findunquoted(Str s, u8 c, b32 arith = 0)
{
    i32 parens  = 0;
    i32 pending = 0;
    for (iz i = 0; i < s.len; i++) {
        u8 b = s[i];
        if (arith && b=='(') {
            parens++;
        } else if (arith && b==')') {
            parens -= parens > 0;
        } else if (arith && parens) {
            // inside parentheses
        } else if (arith && b=='?') {
            pending++;
        } else if (arith && b==':' && pending) {
            pending--;
        } else if (b == '\\') {
            i++;
        } else if (b == '\'') {
            for (i++; i<s.len && s[i]!='\''; i++) {}
        } else if (b == '"') {
            for (i++; i<s.len && s[i]!='"'; i++) {
                if (s[i] == '\\') i++;
            }
        } else if (b=='$' && i+1<s.len && (s[i+1]=='{' || s[i+1]=='(')) {
            i32 depth = 0;
            u8  open  = s[i+1];
            u8  close = open=='{' ? '}' : ')';
            for (i++; i < s.len; i++) {
                if (s[i] == '\\') { i++; continue; }
                if (s[i] == open) depth++;
                if (s[i]==close && !--depth) break;
            }
        } else if (b == '`') {
            for (i++; i<s.len && s[i]!='`'; i++) {
                if (s[i] == '\\') i++;
            }
        } else if (b == c) {
            return i;
        }
    }
    return -1;
}

// ^ uppercases, , lowercases, and ~ toggles; doubled, every character.
static Str casemod(Arena *a, Str v, Str op, Str pat, i32 pflags, b32 utf8)
{
    b32 upper = op[0]=='^' || op[0]=='~';
    b32 lower = op[0]==',' || op[0]=='~';
    b32 all   = op.len == 2;
    Str r = clone(a, v);
    for (iz i = 0; i < r.len; i++) {
        u8  c = r[i];
        u8  t = upper && c>='a' && c<='z' ? (u8)(c-32)
              : lower && c>='A' && c<='Z' ? (u8)(c+32)
              : c;
        if (t != c) {
            Str one = {&c, 1};
            if (!pat.len || patmatch(pat, one, pflags, *a)) {
                r[i] = t;
            }
        }
        if (!all) break;
    }
    (void)utf8;
    return r;
}

// Single-quote a string for reuse as input, as ${x@Q} does.
static Str quoteforreuse(Arena *a, Str s)
{
    b32 plain = 1;
    for (iz i = 0; i < s.len; i++) {
        if (s[i]<0x20 || s[i]>0x7e) plain = 0;
    }
    if (!plain) {
        return ansicquote(a, s);
    }
    return singlequote(a, s);
}

static b32 transformop(u8 c)
{
    switch (c) {
    case 'Q': case 'E': case 'P': case 'A': case 'K': case 'a': case 'k':
    case 'U': case 'u': case 'L':
        return 1;
    }
    return 0;
}

static Str basename(Str path)
{
    iz i = path.len;
    while (i>0 && path[i-1]!='/') i--;
    return cuthead(path, i);
}

// Prompt text from outside the prompt (names, directories): control
// characters made visible as ^X, then, when the prompt will be expanded,
// backslash-quoted for double quotes, as bash does.
static void promptput(Buf *b, Str s, b32 vis, b32 quote)
{
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (vis && (c<0x20 || c==0x7f)) {
            putbyte(b, '^');
            c = c==0x7f ? '?' : (u8)(c | 0x40);
        }
        if (quote && (c=='$' || c=='`' || c=='"' || c=='\\')) {
            putbyte(b, '\\');
        }
        putbyte(b, c);
    }
}

// Decode the backslash escapes of a prompt string, as ${x@P} does,
// then expand it as if in double quotes when promptvars is on.
static Str promptdecode(X *x, Str s)
{
    Shell *sh = x->sh;
    Arena *a  = x->a;
    b32 expand  = sh->shopt[S_PROMPTVARS] || sh->opt[O_POSIX];
    b32 editing = sh->opt[O_EMACS] || sh->opt[O_VI];
    Buf b(a, s.len+16);
    PfZone *zone = 0;
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c=='!' && sh->opt[O_POSIX]) {
            // the history number, or ! for !!; no history here
            if (i+1<s.len && s[i+1]=='!') {
                putbyte(&b, '!');
                i++;
            } else {
                putbyte(&b, '1');
            }
            continue;
        }
        if (c!='\\' || i+1==s.len) {
            putbyte(&b, c);
            continue;
        }
        c = s[++i];
        switch (c) {
        case 'a':  putbyte(&b, 7);  break;
        case 'e':  putbyte(&b, 27); break;
        case 'r':  putbyte(&b, '\r'); break;
        case '\\': putbyte(&b, '\\'); break;
        case 'n':  print(&b, editing ? Str("\r\n") : Str("\n")); break;
        case '[':  if (editing) putbyte(&b, 1); break;
        case ']':  if (editing) putbyte(&b, 2); break;
        case '$':
            if (expand && sh->euid) putbyte(&b, '\\');
            putbyte(&b, sh->euid ? '$' : '#');
            break;
        case '!':  putbyte(&b, '1'); break;  // no history
        case '#':  putbyte(&b, '1'); break;
        case 's':  promptput(&b, basename(sh->shellname), 1, expand); break;
        case 'j': {
            i64 n = 0;
            for (Job *j = sh->jobs; j; j = j->next) n++;
            print(&b, n);
        } break;
        case 'l':  print(&b, "tty"); break;
        case 'v':  print(&b, "5.3"); break;
        case 'V':  print(&b, "5.3.0"); break;
        case 'H':
            promptput(&b, getvar(sh, a, "HOSTNAME").value, 0, expand);
            break;
        case 'h':
            promptput(&b, cut(getvar(sh, a, "HOSTNAME").value, '.').head, 0, expand);
            break;
        case 'u':  promptput(&b, getvar(sh, a, "USER").value, 0, expand); break;
        case 'w':
        case 'W': {
            Str pwd  = getvar(sh, a, "PWD").value;
            Got home = getvar(sh, a, "HOME");
            b32 athome = home.set && home.value==pwd;
            if (c=='W' && !athome) {
                if (pwd!="/" && pwd!="//") pwd = basename(pwd);
            } else if (home.set && home.value.len && startswith(pwd, home.value) &&
                       (pwd.len==home.value.len || pwd[home.value.len]=='/')) {
                pwd = concat(a, "~", cuthead(pwd, home.value.len));
            }
            promptput(&b, pwd, 1, expand);
        } break;
        case 'd': case 't': case 'T': case '@': case 'A': case 'D': {
            Str f = {};
            switch (c) {
            case 'd': f = "%a %b %d";  break;
            case 't': f = "%H:%M:%S";  break;
            case 'T': f = "%I:%M:%S";  break;
            case '@': f = "%I:%M %p";  break;
            case 'A': f = "%H:%M";     break;
            }
            if (c == 'D') {
                if (i+1>=s.len || s[i+1]!='{') {
                    putbyte(&b, '\\');
                    putbyte(&b, c);
                    break;
                }
                iz k = i + 2;
                for (; k<s.len && s[k]!='}'; k++) {}
                f = span(s.data+i+2, s.data+k);
                f = f.len ? f : Str("%X");
                i = k<s.len ? k : s.len-1;
            }
            if (!zone) {
                Str tz = "UTC";
                Value *v = lookup(sh, "TZ");
                if (v && (v->attrs & V_EXPORT)) {
                    Got g = getvar(sh, a, "TZ");
                    tz = g.set ? g.value : tz;
                }
                zone  = alloc<PfZone>(a);
                *zone = parsezone(tz);
            }
            PfTm tm = localtm(os_now(sh->os)/1000000000, zone);
            PfTout t = {};
            fmttime(&t, f, &tm);
            Str r = t.full ? Str{} : Str{t.data, t.len};
            promptput(&b, r, 0, c=='D' && expand);
        } break;
        default:
            if (c>='0' && c<='7') {
                i32 v = 0;
                iz  k = i;
                for (; k<s.len && k<i+3 && s[k]>='0' && s[k]<='7'; k++) {
                    v = v*8 + (s[k] - '0');
                }
                putbyte(&b, (u8)v);
                i = k - 1;
            } else {
                putbyte(&b, '\\');
                putbyte(&b, c);
            }
        }
    }
    Str r = finish(&b);
    return expand ? expandword(x, r, XC_DQUOTE) : r;
}

// One value transformed by ${name@t}, except for @A and @a.
static Str transform(X *x, Str name, Str v, u8 t)
{
    (void)name;
    switch (t) {
    case 'Q':
    case 'K':
    case 'k': return quoteforreuse(x->a, v);
    case 'E': return ansiexpand(x->a, v, x->sh->utf8);
    case 'P': return promptdecode(x, v);
    case 'U': return casemod(x->a, v, "^^", {}, 0, 0);
    case 'L': return casemod(x->a, v, ",,", {}, 0, 0);
    case 'u': return casemod(x->a, v, "^", {}, 0, 0);
    }
    return v;
}

// ${name@A}: an assignment, as a declare command when attributes are set.
static Str assignform(X *x, Str name, Str v)
{
    name = tempname(x->sh, name);  // what a nameref names
    Value *val = lookup(x->sh, name);
    u32    at  = val ? val->attrs : 0;
    Str    let = attrletters(x->a, at & ~(V_LOCAL|V_TEMP));
    Buf b(x->a, v.len+name.len+16);
    if (let.len) {
        print(&b, "declare -");
        print(&b, let);
        putbyte(&b, ' ');
    } else if (at & V_LOCAL) {
        print(&b, "declare ");
    }
    print(&b, name);
    putbyte(&b, '=');
    print(&b, quoteforreuse(x->a, v));
    return finish(&b);
}

// The replacement word for ${x/p/r} under patsub_replacement: quoted
// backslashes and ampersands are backslash-escaped, so that unescaped
// & stands for the match.
static Str ampquote(Arena *a, XField f)
{
    Buf b(a, f.data.len*2);
    for (iz i = 0; i < f.data.len; i++) {
        u8 c = f.data[i];
        if ((f.flags[i] & XF_QUOTED) && (c=='\\' || c=='&')) {
            putbyte(&b, '\\');
        }
        putbyte(&b, c);
    }
    return finish(&b);
}

// Append rep with & replaced by the match, \& a literal &, and \\ one
// backslash. With amp off, rep is used as is.
static void putrep(Buf *b, Str rep, b32 amp, Str m)
{
    if (!amp) {
        print(b, rep);
        return;
    }
    for (iz i = 0; i < rep.len; i++) {
        u8 c = rep[i];
        if (c=='\\' && i+1<rep.len && (rep[i+1]=='&' || rep[i+1]=='\\')) {
            putbyte(b, rep[++i]);
        } else if (c == '&') {
            print(b, m);
        } else {
            putbyte(b, c);
        }
    }
}

static Str patreplace(X *x, Str v, Str pat, Str rep, b32 amp, i32 mode, b32 all, i32 pflags)
{
    Arena    *a  = x->a;
    PatSearch ps = {};
    if (all && mode==PF_FIRST) {
        ps = patsearch(pat, v, pflags, a);
    }
    Buf b(a, v.len + rep.len);
    iz from = 0;
    for (;;) {
        PatSpan m = ps.s.data ? searchnext(&ps, from, *a)
                  :             patfind(pat, v, from, mode, pflags, *a);
        if (!m.ok) break;
        print(&b, span(v.data+from, v.data+m.beg));
        putrep(&b, rep, amp, span(v.data+m.beg, v.data+m.end));
        if (m.end == m.beg) {
            if (m.end == v.len) { from = v.len; break; }
            // Copy one character, as bash does: a whole valid one in a
            // UTF-8 locale, else one byte
            iz clen = pflags&PM_UTF8 ? glibclen(v, m.end) : 1;
            clen = clen ? clen : 1;
            print(&b, span(v.data+m.end, v.data+m.end+clen));
            from = m.end + clen;
        } else {
            from = m.end;
        }
        if (!all || mode!=PF_FIRST || from>=v.len) break;  // while text remains
    }
    print(&b, cuthead(v, from<v.len ? from : v.len));
    return finish(&b);
}

// Evaluate the offset or length of ${name:off:len}. An error names
// the parameter.
static Arith substrarith(X *x, Str name, Str expr)
{
    Arith r = arithraw(x->sh, x->a, expandword(x, expr, XC_ARITH));
    if (!r.ok) {
        Arena a = *x->a;
        expansionerror(x->sh, a, concat3(&a, name, ": ", r.err));
    }
    return r;
}

// Skip n characters (n < 0: all) from byte offset k, ASCII 8 bytes a
// step: where that ends, and how many there were.
struct Skip {
    iz  at;
    i64 n;
};

static Skip utf8skip(Str v, iz k, i64 n)
{
    Skip r = {k, 0};
    while (r.at<v.len && r.n!=n) {
        if ((n<0 || n-r.n>=8) && v.len-r.at>=8) {
            u64 w;
            __builtin_memcpy(&w, v.data+r.at, 8);
            if (!(w & 0x8080808080808080ull)) {
                r.at += 8;
                r.n  += 8;
                continue;
            }
        }
        utf8decode(v, &r.at);
        r.n++;
    }
    return r;
}

static void substrnegative(X *x, Str len)
{
    Arena a = *x->a;
    Str msg = concat(&a, len, ": substring expression < 0");
    expansionerror(x->sh, a, msg);
}

// ${v:off:len} in a UTF-8 locale, counting characters, and the whole
// value only when an offset or length counts from its end.
static Str utf8substring(X *x, Str name, Str v, i64 off, Str len, b32 haslen)
{
    i64  n = -1;  // characters, once counted
    i64  start;
    Skip b;
    if (off < 0) {
        n     = utf8skip(v, 0, -1).n;
        start = n + off;
        if (start < 0) return Str{v.data, 0};
        b = utf8skip(v, 0, start);
    } else {
        start = off;
        b     = utf8skip(v, 0, start);
        if (b.n < start) return Str{v.data, 0};
    }
    iz lim = v.len;
    if (haslen) {
        Arith l = substrarith(x, name, len);
        if (l.value < 0) {
            n = n<0 ? start+utf8skip(v, b.at, -1).n : n;
            i64 end = n + l.value;
            if (end < start) substrnegative(x, len);
            lim = utf8skip(v, b.at, end>start ? end-start : 0).at;
        } else {
            lim = utf8skip(v, b.at, l.value).at;
        }
    }
    return span(v.data+b.at, v.data+lim);
}

static Str applysubstring(X *x, Str name, Str v, Str off, Str len, b32 haslen)
{
    Arith o = substrarith(x, name, off);
    if (x->sh->utf8) {
        return utf8substring(x, name, v, o.value, len, haslen);
    }
    i64 n = v.len;
    i64 start = o.value<0 ? n + o.value : o.value;
    if (start<0 || start>n) {
        return Str{v.data, 0};
    }
    i64 end = n;
    if (haslen) {
        Arith l = substrarith(x, name, len);
        if (l.value < 0) {
            end = n + l.value;
            if (end < start) substrnegative(x, len);
        } else {
            end = start+l.value<n ? start+l.value : n;
        }
    }
    return span(v.data+start, v.data+end);
}

// Arrays

static b32 allsub(Str sub)
{
    return sub=="@" || sub=="*";
}

// An indexed subscript's value: the text expanded, then evaluated.
static i64 subindex(X *x, Str sub)
{
    // As bash, an error abandons any -c, eval or source text too
    return evalinteger(x->sh, x->a, expandword(x, sub, XC_ARITH|XC_DQUOTE)).value;
}

// An associative subscript: the text expanded, quotes removed.
static Str subkey(X *x, Str sub)
{
    return clone(x->a, expandword(x, sub, 0));
}

static Str *onevalue(Arena *a, Str v)
{
    Str *r = alloc<Str>(a);
    *r = v;
    return r;
}

// ${name[sub]}: one element, or for @ and * all of them. A scalar acts
// as an array of one element.
// A bad subscript: reported, and for ${#name[sub]} the command ends.
static void badelem(X *x, Str name, Str sub, b32 length)
{
    Arena a = *x->a;
    if (length) {
        expansionerror(x->sh, a, concat3(&a, "[", sub, "]: bad array subscript"));
    }
    shellerror(x->sh, a, concat(&a, name, ": bad array subscript"));
}

static Param getelems(X *x, Str name, Str sub, b32 length)
{
    Shell *sh = x->sh;
    Param  r  = {};
    Array *arr = varray(sh, x->a, lookup(sh, name));
    if (allsub(sub)) {
        r.list  = 1;
        r.star  = sub=="*";
        r.array = 1;
        if (arr) {
            AList l = alist(x->a, arr, 0);
            r.lv = l.vals;
            r.li = l.idx;
            r.ln = l.n;
        } else {
            Got g = getvar(sh, x->a, name);
            if (g.set) {
                r.lv = onevalue(x->a, g.value);
                r.li = alloc<i64>(x->a);
                r.ln = 1;
            }
        }
        r.set = r.ln > 0;
        return r;
    }
    if (arr && arr->bucket) {
        Str key = subkey(x, sub);
        if (!key.len) {
            badelem(x, name, sub, length);
            r.bad = 1;
            return r;
        }
        HItem *t = hfind(arr, key);
        if (t) {
            r.value = t->val;
            r.set   = 1;
        }
        return r;
    }
    i64 idx = subindex(x, sub);
    if (idx < 0) {
        // counting back from the end of an array; a scalar has none
        idx = (i64)((u64)idx + (arr ? (u64)amax(arr)+1 : 0));  // wraps, as bash
        if (idx < 0) {
            badelem(x, name, sub, length);
            r.bad = 1;
            return r;
        }
    }
    if (arr) {
        Str *e = aget(arr, idx);
        if (e) {
            r.value = *e;
            r.set   = 1;
        }
    } else if (!idx) {
        Got g = getvar(sh, x->a, name);
        r.value = g.value;
        r.set   = g.set;
    }
    return r;
}

// ${name[sub]} for one element, as a nameref to it reads.
static Got getelem(Shell *sh, Arena *a, Str name, Str sub)
{
    X    *x = newx(sh, a);
    Param p = getelems(x, name, sub);
    Got   r = {};
    if (p.list) {
        r.value = joinparams(a, Args{p.lv, p.ln}, p.star ? starsep(sh) : Str(" "));
        r.set   = p.ln > 0;
    } else {
        r.value = p.value;
        r.set   = p.set;
    }
    return r;
}

// Does the variable have a value, or for an array, has it been assigned?
static b32 valuevisible(Value *v)
{
    return v && (v->str.data || v->dyn || (v->arr && v->arr->isset));
}

static b32 varvisible(Shell *sh, Str name)
{
    return valuevisible(lookup(sh, name));
}

// Names of the set variables starting with prefix, sorted.
static void prefixnames(Var *v, Str prefix, Slice<Str> *out, Arena *a)
{
    if (!v) return;
    if (v->top && startswith(v->name, prefix)) {
        if (valuevisible(v->top)) {
            *out = push(a, *out, v->name);
        }
    }
    for (i32 i = 0; i < 4; i++) prefixnames(v->child[i], prefix, out, a);
}

static b32 strless(Str *, Str *);

// ${!name[@]}: the keys or indices.
static Param getkeys(X *x, Str name, b32 star)
{
    Shell *sh = x->sh;
    Param  r  = {};
    Array *arr = varray(sh, x->a, lookup(sh, name));
    r.list = 1;
    r.star = star;
    if (arr) {
        AList l = alist(x->a, arr, 1);
        r.lv = l.keys;
        r.ln = l.n;
    } else if (getvar(sh, x->a, name).set) {
        r.lv = onevalue(x->a, "0");
        r.ln = 1;
    }
    r.set = r.ln > 0;
    return r;
}

// Parse "name[sub]" (an indirect target); false if it is not one.
static b32 splitref(Str s, Str *name, Str *sub)
{
    Cut c = cut(s, '[');
    if (!c.ok || !validvarname(c.head) || !s.len || s[s.len-1]!=']') return 0;
    iz e = skipsubscript(s, c.head.len);
    if (e != s.len-1) return 0;
    *name = c.head;
    *sub  = span(s.data+c.head.len+1, s.data+e);
    return 1;
}

// splitref, and under assoc_expand_once, name[key] for an associative
// array whose key holds brackets: the key then runs to the last ].
static b32 splitrefassoc(Shell *sh, Str s, Str *name, Str *sub)
{
    if (!sh->shopt[S_ASSOC_EXPAND_ONCE] || !s.len) return 0;
    Cut c = cut(s, '[');
    if (!c.ok || !validvarname(c.head) || s[s.len-1]!=']') return 0;
    Value *v = lookup(sh, c.head);
    if (!v || !v->arr || !v->arr->bucket) return 0;
    *name = c.head;
    *sub  = span(s.data+c.head.len+1, s.data+s.len-1);
    return 1;
}

// As declare parses name[key] under assoc_expand_once: the key ends
// at the first ], brackets in it not nesting.
static b32 splitrefflat(Shell *sh, Str s, Str *name, Str *sub)
{
    Cut c = cut(s, '[');
    if (!sh->shopt[S_ASSOC_EXPAND_ONCE] || !c.ok || !validvarname(c.head)) return 0;
    if (cut(c.tail, ']').head.len != c.tail.len-1) return 0;
    *name = c.head;
    *sub  = cuttail(c.tail, 1);
    return 1;
}

// Is s an unquoted name[sub] argument of the running builtin, which
// assoc_expand_once takes as it is?
static b32 onewordarg(Shell *sh, Str s)
{
    if (!sh->shopt[S_ASSOC_EXPAND_ONCE]) return 0;
    for (iz i = 0; i < sh->nbltargs; i++) {
        if (sh->bltargs[i].data==s.data && i<sh->compargs.len && sh->compargs[i]==3) {
            return 1;
        }
    }
    return 0;
}

// Is s an argument of the running builtin, whose subscript then is not
// expanded again under array_expand_once?
static b32 builtinarg(Shell *sh, Str s)
{
    if (!sh->shopt[S_ARRAY_EXPAND_ONCE]) return 0;
    for (iz i = 0; i < sh->nbltargs; i++) {
        if (sh->bltargs[i].data == s.data) return 1;
    }
    return 0;
}

static b32 splitrefonce(Shell *sh, Str s, Str *name, Str *sub)
{
    if (splitref(s, name, sub)) return 1;
    return onewordarg(sh, s) && splitrefassoc(sh, s, name, sub);
}

// The list for ${name[@]@A}: declare, its options, and the assignment.
static iz assignwords(X *x, Str name, Str *out)
{
    Shell *sh  = x->sh;
    name = tempname(sh, name);  // what a nameref names
    Value *val = lookup(sh, name);
    if (!val) return 0;
    Str let = attrletters(x->a, val->attrs & ~(V_LOCAL|V_TEMP));
    iz  n   = 0;
    out[n++] = "declare";
    out[n++] = concat(x->a, "-", let.len ? let : Str("-"));
    Array *arr = varray(sh, x->a, val);
    if (arr && !(arr->len || arr->n) && !arr->isset) {
        out[n++] = name;  // declared, never assigned
    } else if (arr) {
        out[n++] = concat3(x->a, name, "=", arrayassign(x->a, arr));
    } else {
        Got g = getvar(sh, x->a, name);
        out[n++] = concat3(x->a, name, "=", quoteforreuse(x->a, g.value));
    }
    return n;
}

static b32 assignelem(Shell *, Arena, Str name, Str sub, Str lhs, Str value, i32 flags);

static void expandbraced(X *x, Str whole, i32 ctx)
{
    Shell *sh = x->sh;
    Str    s  = whole;
    iz     i  = 0;
    b32    length   = 0;
    b32    indirect = 0;

    if (s.len>1 && s[0]=='#') {
        // ${#name}, unless # is the name followed by an operator. With
        // one character after #, it is always a length: ${#-} ${#/}.
        u8 n = s[1];
        b32 opnext = n==':' || n=='-' || n=='=' || n=='?' || n=='+' ||
                     n=='%' || n=='/' || n=='^' || n==',' || n=='#' ||
                     n=='@';
        if (s.len==2 || !opnext) {
            length = 1;
            i = 1;
        }
    } else if (s.len>1 && s[0]=='!') {
        // ${!name}, unless ! is the name: ${!-word} ${!:0}
        u8 n = s[1];
        if (identstart(n) || digit(n) || n=='#' || n=='?' || n=='@' || n=='*') {
            indirect = 1;
            i = 1;
        }
    }

    iz nb = i;
    if (i<s.len && identstart(s[i])) {
        for (i++; i<s.len && identchar(s[i]); i++) {}
    } else if (i<s.len && digit(s[i])) {
        for (i++; i<s.len && digit(s[i]); i++) {}
    } else if (i<s.len && (s[i]=='@' || s[i]=='*' || s[i]=='#' || s[i]=='?' ||
                            s[i]=='-' || s[i]=='$' || s[i]=='!')) {
        i++;
    } else {
        badsubst(x, whole);
    }
    Str name = span(s.data+nb, s.data+i);

    Str sub = {};
    b32 hassub = 0;
    if (i<s.len && s[i]=='[' && identstart(name[0])) {
        iz e = skipsubscript(s, i);
        if (e == s.len) badsubst(x, whole);
        sub    = span(s.data+i+1, s.data+e);
        hassub = 1;
        i = e + 1;
    }
    Str rest  = cuthead(s, i);
    Str uname = hassub ? span(name.data, s.data+i) : name;  // for errors

    Param p = {};
    Param q = {};  // the indirection's own value
    b32 got = 0;
    if (indirect && rest.len==1 && (rest[0]=='*' || rest[0]=='@') &&
            !hassub && identstart(name[0])) {
        // ${!prefix*} and ${!prefix@}: variable names
        Slice<Str> names = {};
        prefixnames(sh->vars, name, &names, x->a);
        sort(names.data, names.len, strless, *x->a);
        putlist(x, names.data, names.len, rest[0]=='*', ctx);
        return;
    } else if (indirect && hassub && allsub(sub) && !rest.len) {
        // ${!name[@]}: the keys (with an operator, an indirection)
        p = getkeys(x, name, sub=="*");
        got = 1;
        indirect = 0;
    } else if (indirect && !hassub && identstart(name[0]) &&
               (rawlookup(sh, name) ? rawlookup(sh, name)->attrs & V_NAMEREF : 0)) {
        // ${!ref} names the variable a nameref refers to
        Ref ref = resolveref(sh, name);
        p.value = ref.last ? ref.last->str : Str{};
        p.set   = !!p.value.data;
        got = 1;
        indirect = 0;
    } else if (indirect) {
        if (rest.len && find(":-=?+#%/^,~@", takehead(rest, 1))<0) {
            badsubst(x, whole);  // checked before the indirection
        }
        q = hassub ? getelems(x, name, sub) : getparam(x, name);
        if (q.bad) return;
        if (!q.set && !q.list) {
            // As bash: only an unset variable, or an element of a
            // missing array, is an error; an unset positional, special
            // or element names nothing, and expands as unset
            b32 ident = identstart(name[0]);
            if ((ident && !hassub) || (hassub && !lookup(sh, name))) {
                Arena a = *x->a;
                expansionerror(sh, a, concat(&a, uname, ": invalid indirect expansion"));
            }
            uname    = concat(x->a, "!", uname);
            got      = 1;
            indirect = 0;
            hassub   = 0;
        }
    }
    if (indirect) {
        if (q.list && !q.ln) {
            return;  // nothing to name a variable
        }
        if (q.list) {
            q.value = joinparams(x->a, Args{q.lv, q.ln}, " ");
        }
        // The value must name a variable, an array element, a
        // positional or a special
        Str  v  = q.value;
        Str  rn = {}, rs = {};
        b32  ok = validvarname(v) || (v.len==1 && find("@*#?-$!0", v)>=0);
        for (iz k = 0; !ok && k < v.len && digit(v[k]); k++) {
            ok = k == v.len-1;
        }
        if (!ok && splitref(v, &rn, &rs)) {
            ok     = 1;
            v      = rn;
            sub    = rs;
            hassub = 1;
        } else {
            hassub = 0;
        }
        if (!ok) {
            Arena a = *x->a;
            expansionerror(sh, a, concat(&a, v, ": invalid variable name"));
        }
        uname = concat(x->a, "!", uname);
        name  = v;
    }

    if (!got) {
        p = hassub ? getelems(x, name, sub, length) : getparam(x, name);
    }
    b32 elem = hassub && !allsub(sub);  // a single array element

    if (length) {
        if (rest.len) badsubst(x, whole);
        i64 n = 0;
        if (hassub && !p.set && sh->opt[O_NOUNSET] && !varvisible(sh, name)) {
            // not fatal, unlike other unbound variables
            Arena a = *x->a;
            expansionerror(sh, a, concat(&a, allsub(sub) ? name : uname, ": unbound variable"));
        }
        if (p.list) {
            n = p.ln;
        } else {
            if (!hassub && !p.set && sh->opt[O_NOUNSET] && !(name=="@" || name=="*")) {
                unbound(x, uname);
            }
            if (sh->utf8) {
                for (iz k = 0; k < p.value.len;) {
                    utf8decode(p.value, &k);
                    n++;
                }
            } else {
                n = p.value.len;
            }
        }
        putvalue(x, fmti64(x->a, n), ctx);
        return;
    }

    if (!rest.len) {
        if (!p.set && !p.list && sh->opt[O_NOUNSET]) {
            unbound(x, uname);
        }
        if (p.list) {
            putlist(x, p.lv, p.ln, p.star, ctx);
        } else {
            putvalue(x, p.value, ctx);
        }
        return;
    }

    u8 op = rest[0];
    b32 colon = op==':' && rest.len>1 &&
                (rest[1]=='-' || rest[1]=='=' || rest[1]=='?' || rest[1]=='+');
    if (colon) {
        rest = cuthead(rest, 1);
        op = rest[0];
    }
    i32 wctx = (ctx & (XC_DQUOTE|XC_HEREDOC)) | XC_PARAM;
    i32 pctx = XC_PARAM;  // patterns and replacements: as if unquoted

    if (op=='-' || op=='=' || op=='?' || op=='+') {
        Str word = cuthead(rest, 1);
        b32 isset = p.list ? p.ln>0 : p.set;
        // A list is null when its values joined are empty
        Str sep   = p.star ? starsep(sh) : Str(" ");
        b32 null  = p.list ? !joinparams(x->a, Args{p.lv, p.ln}, sep).len : !p.value.len;
        b32 use   = colon ? isset && !null : isset;
        if (op == '+') {
            if (use) {
                expandinto(x, word, wctx);
            } else if (p.list && !p.star && !isset && (ctx & XC_DQUOTE)) {
                x->atempty = 1;  // "${@+word}" vanishes like "$@"
            }
            return;
        }
        if (use) {
            if (p.list) {
                putlist(x, p.lv, p.ln, p.star, ctx);
            } else {
                putvalue(x, p.value, ctx);
            }
            return;
        }
        if (op == '-') {
            expandinto(x, word, wctx);
        } else if (op == '=') {
            // an associative array takes @ and * as keys
            Value *av   = hassub && !elem ? lookup(sh, name) : 0;
            b32    akey = av && av->arr && av->arr->bucket;
            if (!validvarname(name) || (hassub && !elem && !akey)) {
                Arena a = *x->a;
                Str   m = concat3(&a, "$", hassub ? uname : name, ": cannot assign in this way");
                expansionerror(sh, a, m);
            }
            Str v = expandword(x, word, wctx);
            Value *self = elem ? 0 : rawlookup(sh, name);
            b32 refself = self && (self->attrs & V_NAMEREF) && !self->str.len;
            b32 ok = akey ? setelem(sh, *x->a, name, 0, sub, v)
                   : elem ? assignelem(sh, *x->a, name, sub, uname, v, 0)
                   :        setvar(sh, *x->a, name, v);
            if (!ok) {
                // A readonly variable: status 2, or fatal in POSIX mode
                if (sh->opt[O_POSIX] && !sh->interactive) {
                    sh->status = 1;
                    escape(sh, ESC_FATAL);
                }
                sh->status = 2;
                escape(sh, ESC_DISCARD);
            }
            if (refself) {
                putvalue(x, v, ctx);  // it gave the nameref its value
            } else {
                Param q = elem || akey ? getelems(x, name, sub) : getparam(x, name);
                if (q.list) {
                    putlist(x, q.lv, q.ln, q.star, ctx);
                } else {
                    putvalue(x, q.value, ctx);
                }
            }
        } else {
            Str msg = word.len ? expandword(x, word, wctx)
                    : colon    ? Str("parameter null or not set")
                    :            Str("parameter not set");
            Arena a = *x->a;
            fatalerror(sh, a, concat3(&a, uname, ": ", msg));
        }
        return;
    }

    if (!p.set && !p.list && sh->opt[O_NOUNSET]) {
        // As bash: a non-empty array's @a and @A are not unbound for
        // want of element 0
        Value *val  = op=='@' && identstart(name[0]) ? lookup(sh, name) : 0;
        Array *arr  = varray(sh, x->a, val);
        b32 exempt  = arr && acount(arr) && (rest=="@a" || rest=="@A");
        if (!exempt) unbound(x, uname);
    }

    if (op=='#' || op=='%') {
        b32 longest = rest.len>1 && rest[1]==op;
        Str patword = cuthead(rest, longest ? 2 : 1);
        Str pat     = expandpat(x, patword, pctx);
        i32 mode    = op=='#' ? (longest ? PF_LONGPREFIX : PF_SHORTPREFIX)
                              : (longest ? PF_LONGSUFFIX : PF_SHORTSUFFIX);
        i32 pf = patflags(sh);
        if (p.list) {
            Str *v = alloc<Str>(x->a, p.ln);
            for (iz k = 0; k < p.ln; k++) {
                Str a = p.lv[k];
                PatSpan m = patfind(pat, a, 0, mode, pf, *x->a);
                v[k] = !m.ok ? a : op=='#' ? cuthead(a, m.end) : takehead(a, m.beg);
            }
            putlist(x, v, p.ln, p.star, ctx);
        } else {
            Str a = p.value;
            PatSpan m = patfind(pat, a, 0, mode, pf, *x->a);
            putvalue(x, !m.ok ? a : op=='#' ? cuthead(a, m.end) : takehead(a, m.beg), ctx);
        }
        return;
    }

    if (op == '/') {
        Str body = cuthead(rest, 1);
        i32 mode = PF_FIRST;
        b32 all  = 0;
        if (body.len && body[0]=='/') {
            all = 1;
            body = cuthead(body, 1);
        } else if (body.len && body[0]=='#') {
            mode = PF_ANCHORSTART;
            body = cuthead(body, 1);
        } else if (body.len && body[0]=='%') {
            mode = PF_ANCHOREND;
            body = cuthead(body, 1);
        }
        // As bash, a pattern may start with the slash after ${x//
        iz  off   = all && body.len && body[0]=='/';
        iz  slash = findunquoted(cuthead(body, off), '/');
        slash += slash<0 ? 0 : off;
        Str patword = slash<0 ? body : takehead(body, slash);
        Str repword = slash<0 ? Str{} : cuthead(body, slash+1);
        Str pat = expandpat(x, patword, pctx);
        b32 amp = sh->shopt[S_PATSUB_REPLACEMENT];
        XField rf = expandfield(x, repword, pctx);
        Str rep = amp ? ampquote(x->a, rf) : rf.data;
        i32 pf  = patflags(sh);
        if (sh->shopt[S_NOCASEMATCH]) pf |= PM_NOCASE;  // unlike # and %
        if (!pat.len && mode==PF_FIRST) {
            // An empty pattern replaces nothing
            if (p.list) putlist(x, p.lv, p.ln, p.star, ctx);
            else putvalue(x, p.value, ctx);
            return;
        }
        if (p.list) {
            Str *v = alloc<Str>(x->a, p.ln);
            for (iz k = 0; k < p.ln; k++) {
                v[k] = patreplace(x, p.lv[k], pat, rep, amp, mode, all, pf);
            }
            putlist(x, v, p.ln, p.star, ctx);
        } else {
            putvalue(x, patreplace(x, p.value, pat, rep, amp, mode, all, pf), ctx);
        }
        return;
    }

    if (op=='^' || op==',' || op=='~') {
        b32 all = rest.len>1 && rest[1]==op;
        Str opstr = takehead(rest, all ? 2 : 1);
        Str pat = expandpat(x, cuthead(rest, opstr.len), pctx);
        b32 utf8 = sh->utf8;
        i32 pf = patflags(sh);
        if (p.list) {
            Str *v = alloc<Str>(x->a, p.ln);
            for (iz k = 0; k < p.ln; k++) {
                v[k] = casemod(x->a, p.lv[k], opstr, pat, pf, utf8);
            }
            putlist(x, v, p.ln, p.star, ctx);
        } else {
            putvalue(x, casemod(x->a, p.value, opstr, pat, pf, utf8), ctx);
        }
        return;
    }

    if (op == ':') {
        Str body = cuthead(rest, 1);
        iz  c2   = findunquoted(body, ':', 1);
        if (!body.len) badsubst(x, whole);
        Str off  = c2<0 ? body : takehead(body, c2);
        Str len  = c2<0 ? Str{} : cuthead(body, c2+1);
        Value *val = p.array ? lookup(sh, name) : 0;
        Array *arr = varray(sh, x->a, val);  // dynamic arrays too
        if (p.list && p.array && !arr) {
            // a scalar's ${s[@]:off:len} is a substring
            Str v = p.ln ? p.lv[0] : Str{};
            putvalue(x, applysubstring(x, uname, v, off, len, c2>=0), ctx);
            return;
        }
        if (p.list && p.array) {
            // ${a[@]:off:len} selects elements from index off on
            b32 assoc = arr && arr->bucket;
            Arith o = substrarith(x, uname, off);
            i64 top = assoc ? p.ln-1 : p.ln ? p.li[p.ln-1] : -1;
            // bash's count of indices wraps for the largest top index,
            // and then a negative offset selects nothing
            b32 wraps = top == 0x7fffffffffffffff;
            i64 from  = o.value>=0 ? o.value : wraps ? -1 : top + 1 + o.value;
            iz start = p.ln;
            if (from >= 0) {
                for (start = 0; start<p.ln && p.li[start]<from; start++) {}
            }
            iz end = p.ln;
            if (c2 >= 0) {
                Arith l = substrarith(x, uname, len);
                if (l.value < 0) {
                    Arena a = *x->a;
                    expansionerror(sh, a, concat(&a, len, ": substring expression < 0"));
                }
                end = l.value < p.ln-start ? start+(iz)l.value : p.ln;
            }
            putlist(x, p.lv+start, end-start, p.star, ctx);
            return;
        }
        if (p.list) {
            // ${@:off:len} selects parameters; offset 0 is $0
            Arith o = substrarith(x, name, off);
            iz n = sh->args.n + 1;
            Str *all = alloc<Str>(x->a, n);
            all[0] = dollar0(sh);
            for (iz k = 0; k < sh->args.n; k++) all[k+1] = sh->args.v[k];
            i64 start = o.value<0 ? n + o.value : o.value;
            if (start < 0) start = n;  // out of range: nothing
            if (start > n) start = n;
            i64 end = n;
            if (c2 >= 0) {
                Arith l = substrarith(x, name, len);
                if (l.value < 0) {
                    Arena a = *x->a;
                    expansionerror(sh, a, concat(&a, len, ": substring expression < 0"));
                }
                end = start+l.value<n ? start+l.value : n;
            }
            putlist(x, all+start, (iz)(end-start), p.star, ctx);
            return;
        }
        putvalue(x, applysubstring(x, uname, p.value, off, len, c2>=0), ctx);
        return;
    }

    if (op=='@' && rest=="@A" && p.list && p.array) {
        // An array variable's declaration even when it has no elements;
        // a scalar's assignment, as ${name@A}
        Value *val = lookup(sh, name);
        if (val && (val->arr || (val->attrs & (V_ARRAY|V_ASSOC)))) {
            Str v[3];
            iz  n = assignwords(x, name, v);
            putlist(x, v, n, p.star, ctx);
        } else if (p.ln) {
            putvalue(x, assignform(x, name, p.lv[0]), ctx);
        } else if (val) {
            Str let = attrletters(x->a, val->attrs & ~(V_LOCAL|V_TEMP));
            if (let.len) {
                putvalue(x, concat3(x->a, "declare -", let, concat(x->a, " ", name)), ctx);
            }
        }
        return;
    }
    if (op=='@' && p.list && !p.ln) {
        // No values: nothing to transform, nor to check, except that a
        // variable with attributes still reports them
        Value *val = identstart(name[0]) ? lookup(sh, name) : 0;
        Str    let = val ? attrletters(x->a, val->attrs & ~(V_LOCAL|V_TEMP)) : Str{};
        if (let.len && rest=="@a") {
            putvalue(x, let, ctx);
        } else if (let.len && rest=="@A") {
            putvalue(x, concat3(x->a, "declare -", let, concat(x->a, " ", name)), ctx);
        }
        return;
    }
    if (op=='@' && rest.len==2 && transformop(rest[1])) {
        u8 t = rest[1];
        Value *val = lookup(sh, name);
        Array *arr = varray(sh, x->a, val);
        if (p.list && p.array && t=='A') {
            Str v[3];
            iz  n = assignwords(x, name, v);
            putlist(x, v, n, p.star, ctx);
        } else if (p.list && t=='A') {
            // "set --" then each parameter quoted, as separate words
            iz   n = p.ln;
            Str *v = alloc<Str>(x->a, n+2);
            v[0] = "set";
            v[1] = "--";
            for (iz k = 0; k < n; k++) {
                v[k+2] = quoteforreuse(x->a, p.lv[k]);
            }
            putlist(x, v, n ? n+2 : 0, p.star, ctx);
        } else if (p.list && p.array && t=='K' && arr) {
            putvalue(x, arraykv(x->a, arr), ctx);
        } else if (p.list && p.array && t=='k' && arr) {
            AList l = alist(x->a, arr, 1);
            Str *v = alloc<Str>(x->a, 2*l.n);
            for (iz k = 0; k < l.n; k++) {
                v[2*k]   = l.keys[k];
                v[2*k+1] = l.vals[k];
            }
            putlist(x, v, 2*l.n, p.star, ctx);
        } else if (p.list) {
            u32 at = val ? val->attrs & ~(V_LOCAL|V_TEMP) : 0;
            Str *v = alloc<Str>(x->a, p.ln);
            for (iz k = 0; k < p.ln; k++) {
                v[k] = t=='a' ? (p.array ? attrletters(x->a, at) : Str{})
                     : transform(x, name, p.lv[k], t);
            }
            putlist(x, v, p.ln, p.star, ctx);
        } else if (t == 'a') {
            u32 at = val ? val->attrs & ~(V_LOCAL|V_TEMP) : 0;
            putvalue(x, attrletters(x->a, at), ctx);
        } else if (t == 'A') {
            if (p.set && identstart(name[0])) {
                putvalue(x, assignform(x, name, p.value), ctx);
            } else if (val && identstart(name[0])) {
                // an array without the element, or a variable with
                // attributes but no value: declared alone
                Str let = attrletters(x->a, val->attrs & ~(V_LOCAL|V_TEMP));
                if (let.len) {
                    putvalue(x, concat3(x->a, "declare -", let, concat(x->a, " ", name)), ctx);
                }
            }
        } else if (p.set) {
            putvalue(x, transform(x, name, p.value, t), ctx);
        }
        return;
    }
    badsubst(x, whole);
}


// Tilde expansion

static Str tildeprefix(X *x, Str prefix, b32 *ok)
{
    Shell *sh = x->sh;
    *ok = 1;
    if (!prefix.len) {
        Got g = getvar(sh, x->a, "HOME");
        if (g.set) return g.value;
        return "/home/oracle";
    }
    if (prefix == "+") {
        Got g = getvar(sh, x->a, "PWD");
        if (g.set) return g.value;
    } else if (prefix == "-") {
        Got g = getvar(sh, x->a, "OLDPWD");
        if (g.set) return g.value;
    } else {
        // ~N, ~+N, ~-N: the directory stack, which holds only the
        // current directory until pushd exists
        Str n = prefix;
        if (n[0]=='+' || n[0]=='-') n = cuthead(n, 1);
        b32 zero = n.len > 0;
        for (iz i = 0; i < n.len; i++) zero &= n[i] == '0';
        if (zero) {
            Got g = getvar(sh, x->a, "PWD");
            return g.set ? g.value : sh->cwd;
        }
    }
    *ok = 0;
    return {};
}


// The scanner

// Append the expansion of s (word text) in context ctx.
// The ']' closing the subscript opened at s[i], skipping quotes and
// expansions, or -1.
static iz skiprawsub(Shell *sh, Arena *a, Str s, iz i)
{
    i32 depth = 0;
    for (iz e = i; e < s.len; e++) {
        u8 c = s[e];
        if (c == '\\') {
            e++;
        } else if (c == '\'') {
            for (e++; e<s.len && s[e]!='\''; e++) {}
        } else if (c == '"') {
            for (e++; e<s.len && s[e]!='"'; e++) {
                if (s[e] == '\\') {
                    e++;
                } else if (s[e]=='$' && e+1<s.len && (s[e+1]=='(' || s[e+1]=='{')) {
                    e = skipdollar(sh, a, s, e, 1) - 1;
                }
            }
        } else if (c=='$' && e+1<s.len && (s[e+1]=='(' || s[e+1]=='{')) {
            e = skipdollar(sh, a, s, e) - 1;
        } else if (c == '`') {
            iz end = skipbackquote(s, e);
            if (end < 0) return -1;
            e = end - 1;
        } else if (c == '[') {
            depth++;
        } else if (c==']' && !--depth) {
            return e;
        }
    }
    return -1;
}

// Is the word as written an array reference, name[sub]? For unset, as
// bash, its subscript expands once and is not expanded again.
static b32 arrayrefword(Shell *sh, Arena a, Str t)
{
    if (!t.len || !identstart(t[0])) return 0;
    iz i = 1;
    for (; i<t.len && identchar(t[i]); i++) {}
    return i<t.len && t[i]=='[' && skiprawsub(sh, &a, t, i)==t.len-1;
}

// An array subscript in arithmetic text (bash 5.2 and later): expanded
// now as a word of its own, with its quotes removed, and the result's
// special characters backslash-quoted so that evaluating the reference
// expands it to itself rather than a second time. Returns the index
// past the ']', or 0 when the subscript is malformed or empty.
static iz arithsubscript(X *x, Str s, iz i)
{
    iz e = skiprawsub(x->sh, x->a, s, i);
    if (e<0 || e==i+1) return 0;
    // Each byte keeps its flags, so that the result splits as it would
    // have (for unset)
    XField v = expandfield(x, span(s.data+i+1, s.data+e), XC_NOPSUB);
    xputc(x, '[', 0);
    for (iz k = 0; k < v.data.len; k++) {
        u8 c = v.data[k];
        u8 f = v.flags[k] & ~XF_NULL;
        // brackets written as they are stay: $(( a[a[0]] ))
        b32 bracket = (c=='[' || c==']') && f;
        if (bracket || c=='$' || c=='`' || c=='~' || c=='\\' || c=='\'' || c=='"') {
            xputc(x, '\\', f);
        }
        xputc(x, c, f);
    }
    xputc(x, ']', 0);
    return e + 1;
}

static void expandinto(X *x, Str s, i32 ctx)
{
    Shell *sh = x->sh;
    if (ctx & XC_ASSIGNW) {
        ctx &= ~XC_ASSIGNW;
        iz eq = assignment(s, 0);
        if (eq) {
            expandinto(x, takehead(s, eq+1), ctx);
            expandinto(x, cuthead(s, eq+1), ctx|XC_ASSIGN);
            return;
        }
    }
    b32 dq   = ctx & XC_DQUOTE;
    b32 hd   = ctx & XC_HEREDOC;
    u8  lit  = dq||hd ? XF_QUOTED : ctx&XC_PARAM ? XF_SPLIT : 0;
    if (lit == XF_SPLIT) {
        b32 set;
        ifsvalue(sh, &set);
        lit |= set ? 0 : XF_UNSET;
    }
    b32 tildeok = !(ctx & (XC_DQUOTE|XC_HEREDOC|XC_ARITH));
    Str outer   = x->text;
    x->text = s;
    b32 inner   = 0;  // between toggling quotes inside ${...} in quotes

    for (iz i = 0; i < s.len;) {
        u8 c = s[i];

        if (tildeok && c=='~') {
            // A tilde prefix: unquoted characters up to / (or : in an
            // assignment value)
            iz e = i + 1;
            b32 plain = 1;
            for (; e < s.len; e++) {
                u8 d = s[e];
                if (d=='/' || ((ctx & XC_ASSIGN) && d==':')) break;
                if (d=='\\' || d=='\'' || d=='"' || d=='$' || d=='`') {
                    plain = 0;
                    break;
                }
            }
            if (plain) {
                b32 ok;
                Str home = tildeprefix(x, span(s.data+i+1, s.data+e), &ok);
                if (ok) {
                    xput(x, home, XF_QUOTED);
                    x->quoted = 1;  // an empty result is still a field
                    i = e;
                    tildeok = 0;
                    continue;
                }
            }
        }
        tildeok = 0;

        if (c=='[' && (ctx & (XC_ARITH|XC_SUBQ))) {
            iz e = arithsubscript(x, s, i);
            if (e) {
                i = e;
                continue;
            }
        }

        if (c=='\'' && !dq && !hd) {
            iz e = i + 1;
            for (; e<s.len && s[e]!='\''; e++) {}
            xput(x, span(s.data+i+1, s.data+e), XF_QUOTED);
            x->quoted = 1;
            if (e == i+1) xnull(x);
            i = e<s.len ? e+1 : e;
            continue;
        }

        if (c=='"' && (!hd || (ctx & XC_PARAM))) {
            if (dq || hd) {
                // Inside ${...} in double quotes: quotes toggle, and
                // between them a backslash quotes any character
                inner = !inner;
                i++;
                continue;
            }
            if (ctx & XC_ARITH) {
                i++;
                continue;
            }
            // Find the closing quote, skipping nested expansions
            iz e = i + 1;
            for (; e < s.len && s[e] != '"'; e++) {
                if (s[e] == '\\') {
                    e++;
                } else if (s[e]=='$' && e+1<s.len && (s[e+1]=='(' || s[e+1]=='{')) {
                    e = skipdollar(sh, x->a, s, e, 1) - 1;
                } else if (s[e] == '`') {
                    iz end = skipbackquote(s, e);
                    e = end<0 ? s.len-1 : end-1;
                }
            }
            Buf  savedata  = x->data;
            iz   before    = x->data.len;
            b32  wasquoted = x->quoted;
            b32  wasempty  = x->atempty;
            x->atempty = 0;
            expandinto(x, span(s.data+i+1, s.data+(e<s.len?e:s.len)), (ctx & ~XC_PARAM) | XC_DQUOTE);
            if (x->atempty && x->data.len==before && x->done.len==0) {
                x->quoted = wasquoted;  // "$@" with no parameters vanishes
            } else if (x->data.len == before) {
                xnull(x);
            } else {
                x->quoted = 1;
            }
            x->atempty = wasempty || x->atempty;
            (void)savedata;
            i = e<s.len ? e+1 : e;
            continue;
        }

        if (c == '\\') {
            if (i+1 == s.len) {
                // Only brace expansion leaves one here. Unquoted it
                // becomes a quoted null; in double quotes it stays.
                if (dq || hd) {
                    xputc(x, '\\', lit|XF_QUOTED);
                } else {
                    x->quoted = 1;
                }
                i++;
                continue;
            }
            u8 n = s[i+1];
            if ((hd || dq) && !inner) {
                b32 special = n=='$' || n=='`' || n=='\\' || n=='\n' ||
                              (n=='"' && (!hd || (ctx & XC_PARAM))) || ((ctx & XC_PARAM) && n=='}');
                if (!special) {
                    xputc(x, '\\', XF_QUOTED);
                    i++;
                    continue;
                }
                if (n != '\n') xputc(x, n, XF_QUOTED);
                i += 2;
                continue;
            }
            xputc(x, n, XF_QUOTED);
            x->quoted = 1;
            i += 2;
            continue;
        }

        if (c == '`') {
            iz e = skipbackquote(s, i);
            if (e<0 && i+1==s.len) {
                xputc(x, c, lit);  // a lone backquote ending the word
                i++;
                continue;
            } else if (e < 0) {
                Arena scratch = *x->a;
                Str   msg = concat(&scratch, "bad substitution: no closing \"`\" in ", cuthead(s, i));
                expansionerror(sh, scratch, msg);
            }
            Str body = span(s.data+i+1, s.data+e-1);
            Str out = runcapture(sh, x->a, backquotebody(x->a, body, dq), 1);
            putvalue(x, out, ctx);
            i = e;
            continue;
        }

        if (c=='$' && i+1<s.len && s[i+1]=='\'' && !dq && !hd) {
            // $'...' the parser left alone: a pattern in a here-document
            iz e = i + 2;
            for (; e<s.len && s[e]!='\''; e++) {
                if (s[e]=='\\' && e+1<s.len) e++;
            }
            Str t = ansiexpand(x->a, span(s.data+i+2, s.data+e), sh->utf8);
            xput(x, t, XF_QUOTED);
            x->quoted = 1;
            i = e<s.len ? e+1 : e;
            continue;
        }

        if (c=='$' && i+1<s.len && s[i+1]=='"' && !dq && !hd) {
            i++;  // $"..." the parser left alone, as $'...' above
            continue;
        }

        if (c=='$' && i+1<s.len) {
            u8 n = s[i+1];
            if (n == '(') {
                b32 ended = 1;
                iz  e = skipdollar(sh, x->a, s, i, dq||hd, &ended);
                if (!ended && !(i+2<s.len && s[i+2]=='(')) {
                    // Parsed only now, as in bash
                    comsubeof(sh, *x->a, cuthead(s, i), hd);
                }
                if (isarith(s, i, e)) {
                    Str body = span(s.data+i+3, s.data+(e>=i+5 ? e-2 : e));
                    Str text = expandword(x, body, XC_ARITH|XC_DQUOTE);
                    Arith r  = evalarith(sh, x->a, text);
                    if (r.ok) putvalue(x, fmti64(x->a, r.value), ctx);
                } else {
                    Str body = span(s.data+i+2, s.data+(e>i+2 ? e-1 : e));
                    Str out  = runcapture(sh, x->a, body, 0);
                    putvalue(x, out, ctx);
                }
                i = e;
                continue;
            }
            if (n == '{') {
                iz e = skipdollar(sh, x->a, s, i, dq||hd);
                Str inner = span(s.data+i+2, s.data+(e>i+2 ? e-1 : e));
                if (inner.len && funsubchar(inner[0])) {
                    // ${ cmd; } and ${| cmd; }: run in the current shell
                    Str out = runcapture(sh, x->a, inner, 2);
                    putvalue(x, out, ctx);
                } else {
                    expandbraced(x, inner, ctx);
                }
                i = e;
                continue;
            }
            if (n == '[') {
                // $[expr], the old arithmetic form
                iz e = skipsubscript(s, i+1);
                Str body = span(s.data+i+2, s.data+(e<s.len ? e : s.len));
                Str text = expandword(x, body, XC_ARITH|XC_DQUOTE);
                Arith r  = evalarith(sh, x->a, text);
                if (r.ok) putvalue(x, fmti64(x->a, r.value), ctx);
                i = e<s.len ? e+1 : e;
                continue;
            }
            if (identstart(n)) {
                iz e = i + 1;
                for (; e<s.len && identchar(s[e]); e++) {}
                Str name = span(s.data+i+1, s.data+e);
                Param p = getparam(x, name);
                if (!p.set && !p.list && sh->opt[O_NOUNSET]) unbound(x, name);
                if (p.list) {
                    putlist(x, p.lv, p.ln, p.star, ctx);
                } else {
                    putvalue(x, p.value, ctx);
                }
                i = e;
                continue;
            }
            if (digit(n) || n=='@' || n=='*' || n=='#' || n=='?' || n=='-' ||
                    n=='$' || n=='!') {
                Str name = span(s.data+i+1, s.data+i+2);
                Param p = getparam(x, name);
                if (p.list) {
                    putlist(x, sh->args.v, sh->args.n, p.star, ctx);
                } else {
                    if (!p.set && sh->opt[O_NOUNSET] && n!='@' && n!='*') {
                        unbound(x, concat(x->a, "$", name));
                    }
                    putvalue(x, p.value, ctx);
                }
                i += 2;
                continue;
            }
        }

        if ((c=='<' || c=='>') && i+1<s.len && s[i+1]=='(' && !dq && !hd &&
                !(ctx & (XC_ARITH|XC_NOPSUB))) {
            // Process substitution: the parser kept <(...) only where
            // it is one
            iz  e    = skipdollar(sh, x->a, s, i);
            Str body = span(s.data+i+2, s.data+(e>i+2 ? e-1 : e));
            xput(x, procsubst(sh, x->a, body, c=='>'), XF_QUOTED);
            x->quoted = 1;
            i = e;
            continue;
        }

        if ((ctx & XC_ASSIGN) && !dq && c==':' && i+1<s.len && s[i+1]=='~') {
            xputc(x, c, lit);
            i++;
            tildeok = 1;
            continue;
        }

        xputc(x, c, lit);
        i++;
    }
    x->text = outer;
}


// Field splitting

// IFS whitespace: any space-class character, as in POSIX 2024
static b32 ifswhite(u8 c)
{
    return c==' ' || (c>='\t' && c<='\r');
}

static b32 isdelim(XField *f, Str ifs, iz i)
{
    if ((f->flags[i] & (XF_SPLIT|XF_QUOTED|XF_KEEP)) != XF_SPLIT) return 0;
    for (iz k = 0; k < ifs.len; k++) {
        if (ifs[k] == f->data[i]) return 1;
    }
    return 0;
}

static b32 iswhitedelim(XField *f, Str ifs, iz i)
{
    return isdelim(f, ifs, i) && ifswhite(f->data[i]);
}

static void splitfield(X *x, XField f, Str ifs, Slice<XField> *out)
{
    Arena *a = x->a;
    iz n      = f.data.len;
    iz before = out->len;
    iz i      = 0;
    while (i<n && iswhitedelim(&f, ifs, i)) i++;
    while (i < n) {
        iz start = i;
        while (i<n && !isdelim(&f, ifs, i)) i++;
        XField g = {};
        g.data   = span(f.data.data+start, f.data.data+i);
        g.flags  = span(f.flags.data+start, f.flags.data+i);
        g.quoted = 1;
        *out = push(a, *out, g);
        if (i < n) {
            if (iswhitedelim(&f, ifs, i)) {
                while (i<n && iswhitedelim(&f, ifs, i)) i++;
                if (i<n && isdelim(&f, ifs, i)) i++;
            } else {
                i++;
            }
            while (i<n && iswhitedelim(&f, ifs, i)) i++;
        }
    }
    if (out->len==before && f.quoted) {
        XField g = {};
        g.data.data  = f.data.data;
        g.flags.data = f.flags.data;
        g.quoted     = 1;
        *out = push(a, *out, g);
    }
}


// Pathname expansion

static b32 globchar(u8 c, b32 extglob)
{
    return c=='*' || c=='?' || c=='[' || (extglob && (c=='+' || c=='@' || c=='!'));
}

// The field as a pattern, with quoted characters backslash-escaped.
// Returns a null string if it has no unquoted glob characters.
static Str globpattern(Arena *a, XField f, b32 extglob)
{
    // As bash decides: [ counts only with an unquoted ] after it (and
    // no / between), and an unquoted backslash, which can only come
    // from an expansion, protects the next character.
    b32 any  = 0;
    b32 open = 0;
    for (iz i = 0; i<f.data.len && !any; i++) {
        if (f.flags[i] & XF_QUOTED) continue;
        switch (f.data[i]) {
        case '*':
        case '?':  any = 1; break;
        case '[':  open = 1; break;
        case ']':  any = open; break;
        case '/':  open = 0; break;
        case '\\': i++; break;
        case '+':
        case '@':
        case '!':  any = extglob && i+1<f.data.len && f.data[i+1]=='('; break;
        }
    }
    if (!any) return {};
    Buf b(a, 2*f.data.len);
    for (iz i = 0; i < f.data.len; i++) {
        u8 c = f.data[i];
        if ((f.flags[i] & XF_QUOTED) && (globchar(c, 1) || c=='\\' || c==']' ||
                                         c=='(' || c==')' || c=='|' || c=='-' ||
                                         c=='!' || c=='^')) {
            putbyte(&b, '\\');
        } else if (!(f.flags[i] & XF_QUOTED) && c=='\\' && (f.flags[i] & XF_SPLIT)) {
            // a backslash from an unquoted expansion stays a backslash
        }
        putbyte(&b, c);
    }
    return finish(&b);
}

static Slice<Str> glob(Shell *, Arena *, Str pat);  // glob.cpp

static b32 strless(Str *a, Str *b)
{
    return compare(*a, *b) < 0;
}

static Str joinpath(Arena *a, Str dir, Str name)
{
    if (!dir.len) return name;
    Buf b(a, dir.len+name.len+1);
    print(&b, dir);
    if (dir[dir.len-1] != '/') putbyte(&b, '/');
    print(&b, name);
    return finish(&b);
}

static b32 isabsolute(Shell *sh, Str path)
{
    if (path.len && path[0]=='/') return 1;
    if (sh->windows) {
        if (path.len && path[0]=='\\') return 1;
        if (path.len>1 && letter(path[0]) && path[1]==':') return 1;
    }
    return 0;
}

// Does a command name contain a directory part, so PATH is not used?
static b32 haspath(Shell *sh, Str name)
{
    for (iz i = 0; i < name.len; i++) {
        if (name[i]=='/' || (sh->windows && name[i]=='\\')) return 1;
    }
    return sh->windows && name.len>1 && letter(name[0]) && name[1]==':';
}

static Str absolute(Shell *sh, Arena *a, Str path)
{
    if (isabsolute(sh, path)) return path;
    return joinpath(a, sh->cwd, path.len ? path : Str("."));
}


// Entry points

// After $* with a null IFS (see putlist): only XF_UNSET bytes split.
static b32 unsetonly(Arena *a, XField *f)
{
    b32 any = 0;
    for (iz i = 0; i < f->flags.len; i++) any |= f->flags[i] & XF_UNSET;
    if (!any) return 0;
    Str flags = {alloc<u8>(a, f->flags.len), f->flags.len};
    for (iz i = 0; i < flags.len; i++) {
        u8 c = f->flags[i];
        flags[i] = c&XF_UNSET ? c : c&~XF_SPLIT;
    }
    f->flags = flags;
    return 1;
}

static XField stripnulls(Arena *a, XField f)
{
    iz n = 0;
    for (iz i = 0; i < f.flags.len; i++) n += !(f.flags[i] & XF_NULL);
    if (n == f.flags.len) return f;
    XField r = f;
    r.data  = Str{alloc<u8>(a, n), n};
    r.flags = Str{alloc<u8>(a, n), n};
    for (iz i = 0, j = 0; i < f.flags.len; i++) {
        if (!(f.flags[i] & XF_NULL)) {
            r.data[j]  = f.data[i];
            r.flags[j] = f.flags[i];
            j++;
        }
    }
    return r;
}

static Str fieldstr(XField f)
{
    return f.data;
}

// A word that looks like an assignment gets tilde expansion after its
// first = and each :, outside posix mode.
static i32 wordctx(Shell *sh, Word *w)
{
    return w->flags&W_ASSIGNMENT && !sh->opt[O_POSIX] ? XC_ASSIGNW : 0;
}

// Expand a list of words into fields, with brace expansion, splitting
// and pathname expansion as the flags allow.
static Slice<Str> expandwords(Shell *sh, Arena *a, Word *words, i32 flags)
{
    Slice<Str> out = {};
    b32 braces = sh->opt[O_BRACEEXPAND] && !(flags & XW_NOBRACE);
    b32 split  = !(flags & XW_NOSPLIT);
    b32 doglob = !(flags & XW_NOGLOB) && !sh->opt[O_NOGLOB];
    b32 ifsset;
    b32 extglob = sh->shopt[S_EXTGLOB];

    // Per-text working memory is reused from one text to the next, so
    // that a long brace expansion such as {1..5000} costs little more
    // than its resulting fields: the fields' bytes stay in x->data.
    X            *x      = 0;
    Slice<XField> fields = {};
    Slice<XField> split2 = {};
    for (Word *w = words; w; w = w->next) {
        Slice<Str> texts = {};
        if (braces && !(w->flags & W_NOBRACE) && hasbrace(w->text)) {
            texts = braceexpand(a, w->text);
        } else {
            texts = push(a, texts, w->text);
        }
        for (iz t = 0; t < texts.len; t++) {
            if (!x) {
                x = newx(sh, a);
            } else {
                Buf flagbuf = x->flags;
                flagbuf.len = 0;
                *x = X{};
                x->sh    = sh;
                x->a     = a;
                x->data  = Buf(a, 16);
                x->flags = flagbuf;
            }
            x->nosplit = !split || (w->flags & W_NOSPLIT);
            x->nullmark = split && !(w->flags & W_NOSPLIT);
            // A declaration builtin's assignment argument gets tilde
            // expansion after = even in posix mode
            i32 ctx = flags&XW_ASSIGN && w->flags&W_ASSIGNMENT ? XC_ASSIGNW : wordctx(sh, w);
            ctx |= flags&XW_SUBQ ? XC_SUBQ : 0;
            expandinto(x, texts[t], ctx);
            fields.len = 0;
            for (iz k = 0; k < x->done.len; k++) {
                fields = push(a, fields, x->done[k]);
            }
            fields = push(a, fields, xfinish(x));

            // Split with IFS as it is now, after any ${IFS=...}
            Str ifs = ifsvalue(sh, &ifsset);
            split2.len = 0;
            for (iz k = 0; k < fields.len; k++) {
                XField f = fields[k];
                b32 dosplit = split && !(w->flags & W_NOSPLIT);
                Str sifs    = ifs;
                if (dosplit && x->splitspace) {
                    dosplit = unsetonly(a, &f);
                    sifs    = " ";
                } else if (!ifs.len) {
                    dosplit = 0;
                }
                if (dosplit) {
                    splitfield(x, f, sifs, &split2);
                } else if (f.data.len || f.quoted) {
                    split2 = push(a, split2, f);
                } else {
                    // unquoted empty: removed
                    b32 any = 0;
                    for (iz j = 0; j < f.flags.len; j++) any |= f.flags[j];
                    (void)any;
                }
            }
            for (iz k = 0; k < split2.len; k++) {
                XField f = x->nullmark ? stripnulls(a, split2[k]) : split2[k];
                Str pat = doglob && !(w->flags & W_NOGLOB) ? globpattern(a, f, extglob) : Str{};
                if (pat.data) {
                    Slice<Str> m = glob(sh, a, pat);
                    if (m.len) {
                        for (iz j = 0; j < m.len; j++) out = push(a, out, m[j]);
                        continue;
                    }
                    if (sh->shopt[S_FAILGLOB]) {
                        expansionerror(sh, *a, concat(a, "no match: ", fieldstr(f)));
                    }
                    if (sh->shopt[S_NULLGLOB]) continue;
                }
                out = push(a, out, fieldstr(f));
            }
        }
    }
    return out;
}

// Expand a word to a single string, without splitting or globbing.
static Str expandstring(Shell *sh, Arena *a, Str text, i32 ctx = 0)
{
    X *x = newx(sh, a);
    x->nosplit = 1;
    expandinto(x, text, ctx);
    return finish(&x->data);
}

// Expand a word as a pattern: quoted characters are escaped.
static Str expandpattern(Shell *sh, Arena *a, Str text, i32 ctx = 0)
{
    X *x = newx(sh, a);
    return expandpat(x, text, ctx);
}

// Is byte k of a field the unquoted character c?
static b32 unquotedat(XField f, iz k, u8 c)
{
    return k<f.data.len && f.data[k]==c && !(f.flags[k] & XF_QUOTED);
}

static b32 erespecial(u8 c)
{
    switch (c) {
    case '.': case '[': case '\\': case '(': case ')': case '*': case '+':
    case '?': case '{': case '|': case '^': case '$':
        return 1;
    }
    return 0;
}

// Expand the right side of =~ as bash's quote_string_for_globbing does
// for a regular expression: a quoted character special in an ERE gets
// a backslash, except inside a bracket expression, where quoted
// characters are left as they are. Its class, equivalence class and
// collating symbol states carry from one bracket to the next, as in
// bash, and a bracket without its ] is rescanned as plain text. A [
// just after a \001 byte opens no bracket (bash sees it quoted).
static Str expandregex(Shell *sh, Arena *a, Str text, i32 ctx = 0)
{
    X *x = newx(sh, a);
    x->nosplit = 1;
    expandinto(x, text, ctx | XC_PATTERN);
    XField f = xfinish(x);
    Str    d = f.data;
    Buf    b(a, d.len*2);
    b32    cclass = 0, equiv = 0, collsym = 0;
    for (iz i = 0; i < d.len; i++) {
        u8  c = d[i];
        b32 q = f.flags[i] & XF_QUOTED;
        if (q) {
            if (erespecial(c)) putbyte(&b, '\\');
            putbyte(&b, c);
            continue;
        }
        if (c!='[' || (i && d[i-1]==1)) {
            putbyte(&b, c);
            continue;
        }

        // Copy a bracket expression through its ], if it has one
        iz mark = b.len;
        iz j    = i + 1;
        putbyte(&b, '[');
        if (unquotedat(f, j, '^')) putbyte(&b, d[j++]);
        if (unquotedat(f, j, ']')) putbyte(&b, d[j++]);
        b32 closed = 0;
        for (; j < d.len; j++) {
            u8 e = d[j];
            if (f.flags[j] & XF_QUOTED) {
                putbyte(&b, e);
            } else if (e == ']') {
                closed = 1;
                break;
            } else if (e=='[' && unquotedat(f, j+1, ':')) {
                print(&b, "[:");
                j++;
                cclass = 1;
            } else if (cclass && e==':' && unquotedat(f, j+1, ']')) {
                print(&b, ":]");
                j++;
                cclass = 0;
            } else if (e=='[' && (unquotedat(f, j+1, '=') || unquotedat(f, j+1, '.'))) {
                putbyte(&b, e);
                putbyte(&b, d[++j]);
                if (d[j] == '=') equiv = 1; else collsym = 1;
                if (unquotedat(f, j+1, ']')) putbyte(&b, d[++j]);
            } else if ((equiv && e=='=' && unquotedat(f, j+1, ']')) ||
                       (collsym && e=='.' && unquotedat(f, j+1, ']'))) {
                putbyte(&b, e);
                putbyte(&b, d[++j]);
                if (e == '=') equiv = 0; else collsym = 0;
            } else {
                putbyte(&b, e);
            }
        }
        if (closed) {
            putbyte(&b, ']');
            i = j;
        } else {
            b.len = mark + 1;  // just the [, and on from there
        }
    }
    return finish(&b);
}

// Expand a here-document body.
static Str expandheredoc(Shell *sh, Arena *a, Str body)
{
    X *x = newx(sh, a);
    x->nosplit = 1;
    expandinto(x, body, XC_HEREDOC);
    return finish(&x->data);
}


// Array assignments

struct Sub {
    i64 idx;
    Str key;
    b32 ok;
};

static void badarraysub(Shell *sh, Arena a, Str word)
{
    shellerror(sh, a, concat(&a, word, ": bad array subscript"));
}

// Evaluate the subscript of name[sub] for an assignment: a key if the
// array is associative, else an index, negative ones counting back
// from the end. word is the text for error messages.
// Remove quotes and backslashes, expanding nothing.
static Str unquote(Arena *a, Str s)
{
    Buf b(a, s.len);
    u8  q = 0;
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c=='\\' && q!='\'' && i+1<s.len) {
            putbyte(&b, s[++i]);
        } else if ((c=='\'' || c=='"') && (!q || q==c)) {
            q = q ? 0 : c;
        } else {
            putbyte(&b, c);
        }
    }
    return finish(&b);
}

static Sub evalsub(Shell *sh, Arena *a, Str name, Str sub, Str word, b32 raw = 0)
{
    Sub    r = {};
    Value *v = lookup(sh, name);
    X     *x = newx(sh, a);
    if (v && ((v->arr && v->arr->bucket) || (v->attrs & V_ASSOC))) {
        r.key = raw ? sub : subkey(x, sub);
        if (!r.key.len) {
            if (word.data) badarraysub(sh, *a, word);
            return r;
        }
        r.ok = 1;
        return r;
    }
    if (!sub.len || allsub(sub)) {
        if (word.data) badarraysub(sh, *a, word);
        return r;
    }
    if (raw) {
        // array_expand_once in a builtin: quotes removed but nothing
        // expanded, then evaluated; an error abandons the command and
        // any -c text, as in bash
        r.idx = evalinteger(sh, a, unquote(a, sub)).value;
    } else {
        r.idx = subindex(x, sub);
    }
    if (r.idx < 0) {
        i64 top = v && v->arr ? amax(v->arr) : v && v->str.data ? 0 : -1;
        r.idx = (i64)((u64)r.idx + (u64)top + 1);  // wraps, as bash
        if (r.idx < 0) {
            if (word.data) badarraysub(sh, *a, word);
            return r;
        }
    }
    r.ok = 1;
    return r;
}

static b32 assignelem(Shell *sh, Arena scratch, Str name, Str sub, Str word, Str value, i32 flags)
{
    if (!(flags & SV_NOREF)) name = arraytarget(sh, scratch, name, &flags);
    if (!name.data) return 0;
    Sub s = evalsub(sh, &scratch, name, sub, word, flags & SV_RAWKEY);
    if (!s.ok) return 0;
    return setelem(sh, scratch, name, s.idx, s.key, value, flags & ~SV_RAWKEY);
}

// Assign to NAME or NAME[SUB], the left side of an assignment.
static b32 assignto(Shell *sh, Arena scratch, Str lhs, Str value, i32 flags = 0)
{
    Str name, sub;
    if (splitrefonce(sh, lhs, &name, &sub)) {
        flags |= builtinarg(sh, lhs) ? SV_RAWKEY : 0;
        return assignelem(sh, scratch, name, sub, lhs, value, flags);
    }
    return setvar(sh, scratch, lhs, value, flags);
}

// Split the list of a compound assignment into its words, as written.
static Slice<Str> compoundwords(Shell *sh, Arena *a, Str s)
{
    Slice<Str> r = {};
    iz i = 0;
    for (;;) {
        for (; i<s.len && (s[i]==' ' || s[i]=='\t' || s[i]=='\n'); i++) {}
        if (i == s.len) break;
        if (s[i] == '#') {
            for (; i<s.len && s[i]!='\n'; i++) {}
            continue;
        }
        iz start = i;
        if (s[i] == '[') {
            // A subscript may hold blanks
            iz e = skipsubscript(s, i);
            if (e+1<s.len && (s[e+1]=='=' || (s[e+1]=='+' && e+2<s.len && s[e+2]=='='))) {
                i = e + 1;
            }
        }
        while (i<s.len && s[i]!=' ' && s[i]!='\t' && s[i]!='\n') {
            u8 c = s[i];
            if (c == '\\') {
                i += 2;
            } else if (c == '\'') {
                for (i++; i<s.len && s[i]!='\''; i++) {}
                i++;
            } else if (c=='$' && i+1<s.len && s[i+1]=='\'') {
                for (i += 2; i<s.len && s[i]!='\''; i++) {
                    if (s[i] == '\\') i++;
                }
                i++;
            } else if (c == '"') {
                for (i++; i<s.len && s[i]!='"'; i++) {
                    if (s[i] == '\\') {
                        i++;
                    } else if (s[i]=='$' && i+1<s.len && (s[i+1]=='(' || s[i+1]=='{')) {
                        i = skipdollar(sh, a, s, i, 1) - 1;
                    } else if (s[i] == '`') {
                        iz e = skipbackquote(s, i);
                        i = e<0 ? s.len : e-1;
                    }
                }
                i++;
            } else if (c=='$' && i+1<s.len && (s[i+1]=='(' || s[i+1]=='{')) {
                i = skipdollar(sh, a, s, i);
            } else if (c == '`') {
                iz e = skipbackquote(s, i);
                i = e<0 ? s.len : e;
            } else if ((c=='<' || c=='>') && i+1<s.len && s[i+1]=='(') {
                i32 depth = 0;
                for (i++; i < s.len; i++) {
                    if (s[i] == '(') depth++;
                    if (s[i]==')' && !--depth) break;
                }
                i++;
            } else {
                i++;
            }
        }
        i = i<s.len ? i : s.len;
        r = push(a, r, span(s.data+start, s.data+i));
    }
    return r;
}

struct CItem {      // an element of an indexed compound assignment
    Str word;       // as written, for messages
    Str sub;
    Str val;
    b32 hassub;
    b32 append;
};

// While the list for a new local expands, the binding it hides shows:
// local -a arr=("${arr[@]}") copies the caller's array. (An escape
// during the expansion leaves the local hidden, which only drops it.)
struct Hidden {
    Var   *var;
    Value *top;
};

static Hidden hidelocal(Shell *sh, Str name, i32 flags)
{
    Hidden r = {};
    if (!(flags & SV_NEWLOCAL)) return r;
    r.var = findvar(sh, name);
    r.top = r.var ? r.var->top : 0;
    if (!r.top) return {};
    r.var->top = r.top->below;
    return r;
}

static void unhide(Hidden h)
{
    if (h.var) h.var->top = h.top;
}

// Discard the rest of the command after a failed compound assignment.
[[noreturn]] static void compoundfail(Shell *sh)
{
    sh->status = 1;
    escape(sh, ESC_DISCARD);
}

// Assign name=(body). From declare, words in messages appear quoted, as
// bash shows them after requoting.
static b32 assigncompound(Shell *sh, Arena scratch, Str name, Str body, i32 flags, b32 decl = 0)
{
    Arena *a  = &scratch;
    Store *st = sh->store;
    Slice<Str> words = compoundwords(sh, a, body);
    if (!(flags & SV_NOREF)) name = arraytarget(sh, scratch, name, &flags, 1);
    if (!name.data) compoundfail(sh);

    Var   *var = findvar(sh, name);
    Value *cur = var ? var->top : 0;
    if (flags & SV_GLOBAL) {
        while (cur && cur->depth) cur = cur->below;
    }
    if (cur && (cur->attrs & V_READONLY) && !(flags & SV_FORCE)) {
        shellerror(sh, scratch, concat(a, name, ": readonly variable"));
        return 0;
    }
    if (cur && (cur->attrs & V_NOASSIGN)) return 1;
    b32 append = flags & SV_APPEND;
    i32 eflags = (flags & ~SV_APPEND) | SV_FORCE;

    if (!(cur && cur->arr && cur->arr->bucket)) {
        // Indexed: expand every word first
        Hidden hid = hidelocal(sh, name, flags);
        Slice<CItem> items = {};
        for (iz k = 0; k < words.len; k++) {
            Str w  = words[k];
            iz  eq = assignment(w, 1);
            if (eq) {
                CItem it = {};
                Str lhs = takehead(w, eq);
                it.append = lhs.len && lhs[lhs.len-1]=='+';
                if (it.append) lhs = cuttail(lhs, 1);
                it.word   = w;
                it.sub    = span(lhs.data+1, lhs.data+lhs.len-1);
                it.hassub = 1;
                it.val    = expandstring(sh, a, cuthead(w, eq+1), XC_ASSIGN);
                items = push(a, items, it);
                continue;
            }
            Word one = {};
            one.text = w;
            Slice<Str> f = expandwords(sh, a, &one, 0);
            for (iz j = 0; j < f.len; j++) {
                CItem it = {};
                it.word = w;
                it.val  = f[j];
                items = push(a, items, it);
            }
        }
        unhide(hid);
        Value *val = assignslot(sh, scratch, name, flags|SV_FORCE);
        toarray(sh, val);
        if (!append) aclear(st, val->arr);
        val->arr->isset = 1;
        // Indices wrap past the largest, as bash's, to a bad subscript
        i64 ind = append ? (i64)((u64)amax(val->arr) + 1) : 0;
        for (iz k = 0; k < items.len; k++) {
            CItem it = items[k];
            if (it.hassub) {
                if (allsub(it.sub)) {
                    shellerror(sh, scratch, concat(a, it.word, ": cannot assign to non-numeric index"));
                    compoundfail(sh);
                }
                if (!it.sub.len) {
                    badarraysub(sh, scratch, it.word);
                    compoundfail(sh);
                }
                // Unlike a[sub]=v, quotes in the subscript are removed.
                // bash then removes them once more, which drops each
                //  (its internal escape) before the next byte.
                X  *x   = newx(sh, a);
                Str sub = expandword(x, it.sub, 0);
                if (find(sub, "") >= 0) {
                    Buf b(a, sub.len);
                    for (iz i = 0; i < sub.len; i++) {
                        if (sub[i]==1 && i+1<sub.len) i++;
                        putbyte(&b, sub[i]);
                    }
                    sub = finish(&b);
                }
                ind = evalarith(sh, a, sub).value;
                if (ind < 0) {
                    Value *v = lookup(sh, name);
                    ind = (i64)((u64)ind + (u64)(v && v->arr ? amax(v->arr) : -1) + 1);
                }
                if (ind < 0) {
                    badarraysub(sh, scratch, it.word);
                    compoundfail(sh);
                }
            } else if (ind < 0) {
                Str sub = concat3(a, "[", fmti64(a, ind), "]");
                badarraysub(sh, scratch, concat(a, name, sub));
                compoundfail(sh);
            }
            setelem(sh, scratch, name, ind, {}, it.val, eflags|(it.append ? SV_APPEND : 0));
            ind = (i64)((u64)ind + 1);
        }
        assigned(sh, assignslot(sh, scratch, name, flags|SV_FORCE), name);
        return 1;
    }

    // Associative: build the new table while the old stays visible
    Value *val = assignslot(sh, scratch, name, flags|SV_FORCE);
    Array *arr = val->arr;
    Array *dst = append ? arr : newassoc(st, arr->nbuckets);
    u32    at  = val->attrs;
    b32    failed = 0;
    Hidden hid = hidelocal(sh, name, flags);
    if (words.len && !assignment(words[0], 1)) {
        // key value key value ..., each word expanded as an assignment
        // value is: no splitting, no globbing
        Slice<Str> f = {};
        for (iz k = 0; k < words.len; k++) {
            f = push(a, f, expandstring(sh, a, words[k], XC_ASSIGN));
        }
        for (iz k = 0; k < f.len; k += 2) {
            Str key = f[k];
            Str v   = k+1<f.len ? f[k+1] : Str("");
            if (!key.len) {
                // declare shows the word requoted, and goes on
                badarraysub(sh, scratch, decl ? singlequote(a, key) : words[k]);
                if (decl) continue;
                failed = 1;
                break;
            }
            v = assignedvalue(sh, a, at, {}, v, 0);
            hput(st, dst, key, sclone(st, v));
        }
    } else {
        for (iz k = 0; k < words.len; k++) {
            Str w  = words[k];
            iz  eq = assignment(w, 1);
            if (!eq) {
                Str shown = decl ? singlequote(a, w) : w;
                Str msg = concat(a, concat3(a, name, ": ", shown),
                                 ": must use subscript when assigning associative array");
                shellerror(sh, scratch, msg);
                failed = 1;
                break;
            }
            Str lhs = takehead(w, eq);
            b32 app = lhs.len && lhs[lhs.len-1]=='+';
            if (app) lhs = cuttail(lhs, 1);
            X  *x   = newx(sh, a);
            Str key = subkey(x, span(lhs.data+1, lhs.data+lhs.len-1));
            Str v   = expandstring(sh, a, cuthead(w, eq+1), XC_ASSIGN);
            if (!key.len) {
                Str shown = w;
                if (decl) {
                    shown = concat3(a, "[", singlequote(a, key), "]=");
                    shown = concat(a, shown, singlequote(a, v));
                }
                badarraysub(sh, scratch, shown);
                failed = 1;
                break;
            }
            HItem *t = hfind(dst, key);
            v = assignedvalue(sh, a, at, t ? t->val : Str{}, v, app ? SV_APPEND : 0);
            hput(st, dst, key, sclone(st, v));
        }
    }
    unhide(hid);
    val = assignslot(sh, scratch, name, flags|SV_FORCE);
    if (dst != val->arr) {
        afree(st, val->arr);
        val->arr = dst;
    }
    dst->isset = 1;
    assigned(sh, val, name);
    if (failed) compoundfail(sh);
    return 1;
}
