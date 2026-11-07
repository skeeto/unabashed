// Regular expressions for [[ =~ ]]: POSIX EREs as glibc does them
//
// bash compiles the right side of =~ with glibc's regcomp (REG_EXTENDED,
// plus REG_ICASE under nocasematch) and fills BASH_REMATCH from regexec.
// This reproduces both: glibc's syntax and GNU extensions (\w \W \s \S
// \b \B \< \> \` \' and back-references \1-\9), its error codes and
// messages, which match it finds and which submatches it reports.
//
// The parser builds glibc's parse tree node for node: intervals expand
// into the same copies, groups repeated optionally carry the same
// "optional" mark, a group that is the whole of another merges into it,
// and nodes are numbered in the same postorder. The numbering matters:
// where glibc may go two ways it takes the lower-numbered node, which is
// why (|a) prefers the a, like (a|).
//
// Matching takes the leftmost start, then the longest end from there,
// with a Pike VM over node sets, linear in the subject. Submatches need
// a path, which glibc's set_regs picks: the nodes from which the end is
// still reachable at each position are found backward ("sifting"), then
// a walk from the start takes the lowest-numbered viable node at each
// fork, except that a fork already passed at the same position takes
// its other way (an empty loop runs at most once). Registers follow
// glibc's update rules, including its undoing of empty optional groups.
//
// glibc's automaton copies the nodes after an anchor so that they carry
// its constraints, and the copies have their own numbers and identities,
// which decide the walk's loops and the end node it must reach. These
// copies are made here in glibc's order. glibc's quirks follow from its
// automaton and are reproduced: see rxprev, rxnext, rxstall, rxsift and
// rxlongest.
//
// Back-references make the language irregular. With them, the longest
// end for each start is found by exhaustive search, and the walk
// backtracks as glibc's does with its fail stack.
//
// In a UTF-8 locale, . and brackets consume whole characters, and
// invalid bytes and the inside of characters match only themselves,
// where matches may also start when glibc runs the pattern byte by
// byte (see rxstart). A bracket's classes take multibyte characters as
// iswctype does ([[:alpha:]] matches é), as do \w and \s. The word
// anchors ask iswalnum of the characters around them, an invalid byte
// counting as the code point of its value (\xff as ÿ). Under REG_ICASE
// glibc raises the subject and the pattern a character at a time
// (towupper): sets compare a multibyte character raised, and a
// multibyte character of the pattern that has case becomes a set of
// itself raised.
//
// This is free and unencumbered software released into the public domain.

enum : i32 {
    REX_ICASE = 1<<0,  // REG_ICASE
    REX_UTF8  = 1<<1,  // a UTF-8 locale
};

// glibc's error codes, in order
enum : i32 {
    RXE_OK, RXE_NOMATCH, RXE_BADPAT, RXE_ECOLLATE, RXE_ECTYPE, RXE_EESCAPE,
    RXE_ESUBREG, RXE_EBRACK, RXE_EPAREN, RXE_EBRACE, RXE_BADBR, RXE_ERANGE,
    RXE_ESPACE, RXE_BADRPT, RXE_EEND, RXE_ESIZE, RXE_ERPAREN,
};

// What glibc's regerror says for each
static Str rxerrors[] = {
    "Success",
    "No match",
    "Invalid regular expression",
    "Invalid collation character",
    "Invalid character class name",
    "Trailing backslash",
    "Invalid back reference",
    "Unmatched [, [^, [:, [., or [=",
    "Unmatched ( or \\(",
    "Unmatched \\{",
    "Invalid content of \\{\\}",
    "Invalid range end",
    "Memory exhausted",
    "Invalid preceding regular expression",
    "Premature end of regular expression",
    "Regular expression too big",
    "Unmatched ) or \\)",
};

enum : i32 {
    RX_DUPMAX  = 0x7fff,  // RE_DUP_MAX
    RX_MAXTREE = 1<<20,   // parse tree nodes before "Memory exhausted"
    RX_MAXNEST = 500,     // group nesting, for the stack
};

// Anchors are glibc's constraints on the characters either side of a
// position, which accumulate along a run of epsilon moves
enum : i32 {
    RC_PREVWORD    = 1<<0,
    RC_PREVNOTWORD = 1<<1,
    RC_NEXTWORD    = 1<<2,
    RC_NEXTNOTWORD = 1<<3,
    RC_PREVNEWLINE = 1<<4,
    RC_NEXTNEWLINE = 1<<5,
    RC_PREVBEGBUF  = 1<<6,
    RC_NEXTENDBUF  = 1<<7,
};

enum : i32 {
    RA_LINEFIRST    = RC_PREVNEWLINE,               // ^
    RA_LINELAST     = RC_NEXTNEWLINE,               // $
    RA_BUFFIRST     = RC_PREVBEGBUF,                // \`
    RA_BUFLAST      = RC_NEXTENDBUF,                // \'
    RA_WORDFIRST    = RC_PREVNOTWORD|RC_NEXTWORD,   // \<
    RA_WORDLAST     = RC_PREVWORD|RC_NEXTNOTWORD,   // \>
    RA_INWORD       = RC_PREVWORD|RC_NEXTWORD,      // \B, in a word
    RA_INNOTWORD    = RC_PREVNOTWORD|RC_NEXTNOTWORD, // \B, outside words
    RA_WORDDELIM    = 1<<8,  // \b: parsed into WORDFIRST or WORDLAST
    RA_NOTWORDDELIM = 1<<9,  // \B: INWORD or INNOTWORD
};

// Parse tree nodes; the first ten are also automaton nodes
enum : i32 {
    RT_CHAR, RT_SET, RT_ANY, RT_BACKREF,       // consuming
    RT_ANCHOR, RT_OPEN, RT_CLOSE, RT_ALT, RT_STAR,  // epsilon
    RT_END,
    RT_CONCAT, RT_SUBEXP,
};

struct RxRange {
    i32 lo;
    i32 hi;
};

// A bracket expression or class escape. Single bytes are in bits (only
// ASCII in a UTF-8 locale, as in glibc); the multibyte characters of a
// UTF-8 locale match by code point against ranges, inverted by neg.
struct RxSet {
    u32            bits[8];
    Slice<RxRange> ranges;
    u32            classes;  // UC_* classes of multibyte characters
    b32            neg;
    b32            fold;     // REG_ICASE: compare characters raised
};

struct Rt {
    Rt    *left;
    Rt    *right;
    Rt    *first;  // the node that starts this subtree
    Rt    *next;   // the node after it
    Rt    *copy;
    RxSet *set;
    i32    type;
    i32    arg;    // byte, anchor or group
    i32    idx;
    b32    opt;    // a group inside an optional repetition
    b32    dup;    // made by an interval's copying
};

// An automaton node. A consuming node goes on to next, an epsilon node
// to e1 or e2 (glibc's edests, the lower-numbered first, e2 -1 when
// there is one), a back-reference to next when it consumes and to e1
// when it matches empty. cons holds glibc's constraints: an anchor's
// own, and those a copy made after anchors carries. dup marks such a
// copy, and, as in glibc, the nodes of a copy an interval made, so that
// the anchors there get no copies: their constraints then filter only
// themselves, as states hold whole closures (so (^a)+ fails on "aa").
struct RxNode {
    RxSet *set;
    i32    type;
    i32    arg;
    i32    next;
    i32    e1;
    i32    e2;
    i32    cons;
    i32    org;   // the node a copy was made from
    b32    opt;
    b32    dup;
};

struct Regex {
    RxNode *nodes;
    i32    *preds;     // epsilon predecessors of node i: preds[predbeg[i]..predbeg[i+1])
    i32    *predbeg;
    i32    *consume;   // the consuming nodes
    i32    *map;       // group i reports group map[i] (merged groups)
    i32    *refs;      // the groups back-references refer to
    u32    *init;      // glibc's initial state, ignoring constraints
    b32    *initclose; // group i closes in init
    i32     nnodes;
    i32     nconsume;
    i32     start;
    i32     nsub;
    i32     nrefs;
    i32     flags;
    b32     backrefs;
    b32     plural;    // any fork
    b32     initcons;  // any constrained node in init
    b32     copies;    // a group with back-references has several closes
    b32     midchar;   // matches may start inside a UTF-8 character
};

struct RxComp {
    Regex *re;
    i32    err;  // RXE_*, with re null
};

// A register: a group's span, -1 when unset
struct RxMatch {
    iz so;
    iz eo;
};


// Parsing

enum : i32 {
    TK_CHAR, TK_END, TK_BACKSLASH, TK_ALT, TK_STAR, TK_PLUS, TK_QUEST,
    TK_OPENDUP, TK_CLOSEDUP, TK_OPEN, TK_CLOSE, TK_BRACKET, TK_PERIOD,
    TK_ANCHOR, TK_BACKREF, TK_WORD, TK_NOTWORD, TK_SPACE, TK_NOTSPACE,
    // inside brackets
    TK_CLOSEBRACKET, TK_NONMATCH, TK_RANGE, TK_COLL, TK_EQUIV, TK_CLASS,
};

struct RxTok {
    i32 type;
    i32 c;    // the character, an anchor or a group
    iz  len;
};

struct RxParse {
    Str    pat;    // upper case under REX_ICASE
    Str    raw;    // as written: escapes and class names keep their case
    u8    *lead;   // UTF-8: 0 at bytes inside a character, else null
    iz     pos;
    RxTok  tok;    // the current token, pos just after it
    Arena *a;
    i32    flags;
    i32    err;
    i32    nsub;
    i32    ntree;
    i32    nbackref;
    u32    done;   // groups closed so far, for back-references
    b32    toobig; // stopped copying: "Memory exhausted" unless a
                   // syntax error comes later
    b32    wide;   // a word anchor, or a bracket glibc makes a
                   // COMPLEX_BRACKET in UTF-8 (see Regex.midchar)
};

static u8 rxupper(u8 c)
{
    return c>='a' && c<='z' ? c-32 : c;
}

static RxTok rxpeek(RxParse *p)
{
    RxTok t = {TK_CHAR, 0, 1};
    if (p->pos >= p->pat.len) {
        t.type = TK_END;
        t.len  = 0;
        return t;
    }
    t.c = p->pat[p->pos];
    if (p->lead && !p->lead[p->pos]) return t;  // inside a character
    switch (t.c) {
    case '\\':
        if (p->pos+1 >= p->pat.len) {
            t.type = TK_BACKSLASH;
            return t;
        }
        t.c   = p->raw[p->pos+1];
        t.len = 2;
        switch (t.c) {
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            t.type = TK_BACKREF;
            t.c   -= '1';
            break;
        case '<':  t.type = TK_ANCHOR; t.c = RA_WORDFIRST;    break;
        case '>':  t.type = TK_ANCHOR; t.c = RA_WORDLAST;     break;
        case 'b':  t.type = TK_ANCHOR; t.c = RA_WORDDELIM;    break;
        case 'B':  t.type = TK_ANCHOR; t.c = RA_NOTWORDDELIM; break;
        case '`':  t.type = TK_ANCHOR; t.c = RA_BUFFIRST;     break;
        case '\'': t.type = TK_ANCHOR; t.c = RA_BUFLAST;      break;
        case 'w':  t.type = TK_WORD;     break;
        case 'W':  t.type = TK_NOTWORD;  break;
        case 's':  t.type = TK_SPACE;    break;
        case 'S':  t.type = TK_NOTSPACE; break;
        }
        return t;
    case '|': t.type = TK_ALT;      break;
    case '*': t.type = TK_STAR;     break;
    case '+': t.type = TK_PLUS;     break;
    case '?': t.type = TK_QUEST;    break;
    case '{': t.type = TK_OPENDUP;  break;
    case '}': t.type = TK_CLOSEDUP; break;
    case '(': t.type = TK_OPEN;     break;
    case ')': t.type = TK_CLOSE;    break;
    case '[': t.type = TK_BRACKET;  break;
    case '.': t.type = TK_PERIOD;   break;
    case '^': t.type = TK_ANCHOR; t.c = RA_LINEFIRST; break;
    case '$': t.type = TK_ANCHOR; t.c = RA_LINELAST;  break;
    }
    return t;
}

static void rxfetch(RxParse *p)
{
    p->tok  = rxpeek(p);
    p->pos += p->tok.len;
}

static Rt *newrt(RxParse *p, i32 type, Rt *left = 0, Rt *right = 0, i32 arg = 0)
{
    p->ntree++;
    Rt *t = alloc<Rt>(p->a);
    t->type  = type;
    t->left  = left;
    t->right = right;
    t->arg   = arg;
    return t;
}

// The nodes of a tree in preorder, through an explicit stack: trees are
// as deep as patterns are long.
static Slice<Rt *> rtnodes(Rt *t, Arena *a)
{
    Slice<Rt *> r     = {};
    Slice<Rt *> stack = {};
    stack = push(a, stack, t);
    while (stack.len) {
        Rt *n = stack.data[--stack.len];
        r = push(a, r, n);
        if (n->right) stack = push(a, stack, n->right);
        if (n->left)  stack = push(a, stack, n->left);
    }
    return r;
}

static Rt *rtdup(RxParse *p, Rt *t)
{
    if (p->toobig) return t;
    Slice<Rt *> nodes = rtnodes(t, p->a);
    if (nodes.len > RX_MAXTREE-p->ntree) {
        p->toobig = 1;
        return t;
    }
    p->ntree += (i32)nodes.len;
    for (iz i = 0; i < nodes.len; i++) {
        Rt *c = alloc<Rt>(p->a);
        *c = *nodes[i];
        c->opt = 0;  // as glibc's duplicate_tree
        c->dup = 1;
        nodes[i]->copy = c;
    }
    for (iz i = 0; i < nodes.len; i++) {
        Rt *n = nodes[i];
        n->copy->left  = n->left  ? n->left->copy  : 0;
        n->copy->right = n->right ? n->right->copy : 0;
    }
    return t->copy;
}

static void rtmarkopt(RxParse *p, Rt *t, i32 group)
{
    Slice<Rt *> nodes = rtnodes(t, p->a);
    for (iz i = 0; i < nodes.len; i++) {
        if (nodes[i]->type==RT_SUBEXP && nodes[i]->arg==group) {
            nodes[i]->opt = 1;
        }
    }
}

static i32 rxclassid(Str name, i32 flags)
{
    static Str names[] = {
        "alnum", "cntrl", "lower", "space", "alpha", "digit", "print",
        "upper", "blank", "graph", "punct", "xdigit",
    };
    static i32 ids[] = {
        PCC_ALNUM, PCC_CNTRL, PCC_LOWER, PCC_SPACE, PCC_ALPHA, PCC_DIGIT,
        PCC_PRINT, PCC_UPPER, PCC_BLANK, PCC_GRAPH, PCC_PUNCT, PCC_XDIGIT,
    };
    if ((flags & REX_ICASE) && (name=="upper" || name=="lower")) {
        name = "alpha";
    }
    for (i32 i = 0; i < countof(names); i++) {
        if (name == names[i]) return ids[i];
    }
    return PCC_NONE;
}

static void rxsetbit(RxSet *s, i32 c)
{
    s->bits[c>>5] |= 1u << (c&31);
}

// A class's UC_* bit, for multibyte characters
static u32 rxucclass(i32 cc)
{
    switch (cc) {
    case PCC_ALNUM:  return UC_ALNUM;
    case PCC_ALPHA:  return UC_ALPHA;
    case PCC_BLANK:  return UC_BLANK;
    case PCC_CNTRL:  return UC_CNTRL;
    case PCC_DIGIT:  return UC_DIGIT;
    case PCC_GRAPH:  return UC_GRAPH;
    case PCC_LOWER:  return UC_LOWER;
    case PCC_PRINT:  return UC_PRINT;
    case PCC_PUNCT:  return UC_PUNCT;
    case PCC_SPACE:  return UC_SPACE;
    case PCC_UPPER:  return UC_UPPER;
    case PCC_XDIGIT: return UC_XDIGIT;
    }
    return 0;
}

static void rxaddclass(RxSet *s, i32 cc)
{
    for (i32 c = 0; c < 256; c++) {
        if (inctype(c, cc)) rxsetbit(s, c);
    }
}

// Finish a set: inverted, and single bytes only ASCII in UTF-8.
static void rxsetdone(RxParse *p, RxSet *s, b32 neg)
{
    if (neg) {
        for (i32 i = 0; i < 8; i++) s->bits[i] = ~s->bits[i];
        s->neg = 1;
    }
    p->wide |= neg;
    if (p->flags & REX_UTF8) {
        for (i32 i = 4; i < 8; i++) s->bits[i] = 0;
    }
}

// \w \W \s \S
static Rt *rxclassop(RxParse *p, i32 type)
{
    RxSet *s = alloc<RxSet>(p->a);
    p->wide = 1;
    if (type==TK_WORD || type==TK_NOTWORD) {
        rxaddclass(s, PCC_ALNUM);
        rxsetbit(s, '_');
        s->classes = UC_ALNUM;
    } else {
        rxaddclass(s, PCC_SPACE);
        s->classes = UC_SPACE;
    }
    rxsetdone(p, s, type==TK_NOTWORD || type==TK_NOTSPACE);
    Rt *t = newrt(p, RT_SET);
    t->set = s;
    return t;
}

enum : i32 { EL_CHAR, EL_MB, EL_COLL, EL_EQUIV, EL_CLASS };

struct RxElem {
    i32 type;
    i32 c;     // byte or code point
    Str name;  // of a collating symbol, equivalence class or class
};

static RxTok rxpeekbracket(RxParse *p)
{
    RxTok t = {TK_CHAR, 0, 1};
    if (p->pos >= p->pat.len) {
        t.type = TK_END;
        t.len  = 0;
        return t;
    }
    t.c = p->pat[p->pos];
    if (p->lead && !p->lead[p->pos]) return t;
    if (t.c == '[') {
        i32 c2 = p->pos+1<p->pat.len ? p->pat[p->pos+1] : 0;
        i32 type = c2=='.' ? TK_COLL : c2=='=' ? TK_EQUIV : c2==':' ? TK_CLASS : TK_CHAR;
        if (type != TK_CHAR) {
            t.type = type;
            t.c    = c2;
            t.len  = 2;
        }
        return t;
    }
    switch (t.c) {
    case '-': t.type = TK_RANGE;        break;
    case ']': t.type = TK_CLOSEBRACKET; break;
    case '^': t.type = TK_NONMATCH;     break;
    }
    return t;
}

// The name in [.name.], [=name=] or [:name:], from just after the
// opening token. Class names keep their case.
static RxElem rxsymbol(RxParse *p, RxTok t)
{
    RxElem e   = {};
    Str    buf = t.type==TK_CLASS ? p->raw : p->pat;
    if (p->pos >= p->pat.len) {
        p->err = RXE_EBRACK;
        return e;
    }
    iz beg = p->pos;
    for (i32 i = 0;; i++) {
        if (i >= 32) {
            p->err = RXE_EBRACK;
            return e;
        }
        u8 ch = buf[p->pos++];
        if (p->pos >= p->pat.len) {
            p->err = RXE_EBRACK;
            return e;
        }
        if (ch==t.c && p->pat[p->pos]==']') break;
    }
    e.name = span(buf.data+beg, buf.data+p->pos-1);
    e.type = t.type==TK_COLL ? EL_COLL : t.type==TK_EQUIV ? EL_EQUIV : EL_CLASS;
    p->pos++;
    return e;
}

static iz rxcharlen(RxParse *p, iz i)
{
    if (!p->lead || i>=p->pat.len || !p->lead[i]) return 1;
    iz n = glibclen(p->raw, i);
    return n ? n : 1;
}

static RxElem rxelem(RxParse *p, RxTok t, b32 hyphen)
{
    RxElem e = {};
    iz     n = rxcharlen(p, p->pos);
    if (n > 1) {
        e.type = EL_MB;
        e.c    = unitat(p->raw, p->pos, 1).c;
        p->pos += n;
        return e;
    }
    p->pos += t.len;
    if (t.type==TK_COLL || t.type==TK_EQUIV || t.type==TK_CLASS) {
        return rxsymbol(p, t);
    }
    if (t.type==TK_RANGE && !hyphen) {
        // A - that does not start a range may only come last
        if (rxpeekbracket(p).type != TK_CLOSEBRACKET) {
            p->err = RXE_ERANGE;
            return e;
        }
    }
    e.type = EL_CHAR;
    e.c    = t.c;
    return e;
}

static void rxaddelem(RxParse *p, RxSet *s, RxElem e)
{
    switch (e.type) {
    case EL_CHAR:
        rxsetbit(s, e.c);
        break;
    case EL_MB: {
        i32     c = p->flags&REX_ICASE ? cpupper(e.c) : e.c;
        RxRange r = {c, c};
        s->ranges = push(p->a, s->ranges, r);
        if (c < 0x80) rxsetbit(s, c);  // raised to ASCII (ı is I)
        p->wide   = 1;
    } break;
    case EL_COLL:
    case EL_EQUIV:
        // Without collation rules, only single bytes name themselves
        if (e.name.len != 1) {
            p->err = RXE_ECOLLATE;
            return;
        }
        rxsetbit(s, e.name[0]);
        break;
    case EL_CLASS: {
        i32 cc = rxclassid(e.name, p->flags);
        if (cc == PCC_NONE) {
            p->err = RXE_ECTYPE;
            return;
        }
        rxaddclass(s, cc);
        s->classes |= rxucclass(cc);
        p->wide = 1;
    } break;
    }
}

// A range bound's collation value, glibc's without collation rules:
// a byte or a one-byte collating symbol is its value, anything else
// has none.
static i32 rxbound(RxElem e)
{
    if (e.type == EL_CHAR) return e.c;
    if (e.type==EL_COLL && e.name.len==1) return e.name[0];
    return -1;
}

static void rxaddrange(RxParse *p, RxSet *s, RxElem lo, RxElem hi)
{
    p->wide = 1;
    if (lo.type==EL_EQUIV || lo.type==EL_CLASS ||
            hi.type==EL_EQUIV || hi.type==EL_CLASS) {
        p->err = RXE_ERANGE;
        return;
    }
    i32 a = rxbound(lo);
    i32 b = rxbound(hi);
    if (a<0 || b<0) {
        p->err = RXE_ECOLLATE;
        return;
    }
    if (a > b) {
        p->err = RXE_ERANGE;
        return;
    }
    for (i32 c = a; c <= b; c++) rxsetbit(s, c);
    if ((p->flags & REX_UTF8) && b>=0x80) {
        // compared against the code points of multibyte characters
        RxRange r = {a, b};
        s->ranges = push(p->a, s->ranges, r);
    }
}

// A bracket expression, from just after its [.
static Rt *rxbracket(RxParse *p)
{
    RxSet *s   = alloc<RxSet>(p->a);
    s->fold    = !!(p->flags & REX_ICASE);
    b32    neg = 0;
    RxTok  t   = rxpeekbracket(p);
    if (t.type == TK_END) {
        p->err = RXE_BADPAT;
        return 0;
    }
    if (t.type == TK_NONMATCH) {
        neg = 1;
        p->pos += t.len;
        t = rxpeekbracket(p);
        if (t.type == TK_END) {
            p->err = RXE_BADPAT;
            return 0;
        }
    }
    if (t.type == TK_CLOSEBRACKET) t.type = TK_CHAR;  // a leading ]
    for (b32 first = 1;; first = 0) {
        RxElem lo = rxelem(p, t, first);
        if (p->err) return 0;
        t = rxpeekbracket(p);
        b32   range = 0;
        RxTok t2    = {};
        if (lo.type!=EL_CLASS && lo.type!=EL_EQUIV) {
            if (t.type == TK_END) {
                p->err = RXE_EBRACK;
                return 0;
            }
            if (t.type == TK_RANGE) {
                p->pos += t.len;
                t2 = rxpeekbracket(p);
                if (t2.type == TK_END) {
                    p->err = RXE_EBRACK;
                    return 0;
                }
                if (t2.type == TK_CLOSEBRACKET) {
                    p->pos -= t.len;  // a - just before the ]
                    t.type  = TK_CHAR;
                } else {
                    range = 1;
                }
            }
        }
        if (range) {
            RxElem hi = rxelem(p, t2, 1);
            if (p->err) return 0;
            t = rxpeekbracket(p);
            rxaddrange(p, s, lo, hi);
        } else {
            rxaddelem(p, s, lo);
        }
        if (p->err) return 0;
        if (t.type == TK_END) {
            p->err = RXE_EBRACK;
            return 0;
        }
        if (t.type == TK_CLOSEBRACKET) break;
    }
    p->pos += t.len;
    rxsetdone(p, s, neg);
    Rt *r = newrt(p, RT_SET);
    r->set = s;
    return r;
}

static i32 rxnumber(RxParse *p)
{
    i32 num = -1;
    for (;;) {
        rxfetch(p);
        RxTok t = p->tok;
        if (t.type == TK_END) return -2;
        if (t.type==TK_CLOSEDUP || t.c==',') break;
        b32 digit = t.type==TK_CHAR && t.c>='0' && t.c<='9';
        if (!digit || num==-2) {
            num = -2;
        } else if (num == -1) {
            num = t.c - '0';
        } else {
            num = num*10 + t.c - '0';
            num = num<RX_DUPMAX+1 ? num : RX_DUPMAX+1;
        }
    }
    return num;
}

// A repetition of elem: * + ? or an interval, the current token.
static Rt *rxdup(RxParse *p, Rt *elem)
{
    i32 start = 0;
    i32 end   = 0;
    if (p->tok.type == TK_OPENDUP) {
        start = rxnumber(p);
        if (start == -1) {
            if (p->tok.type!=TK_CHAR || p->tok.c!=',') {
                p->err = RXE_BADBR;  // {}
                return 0;
            }
            start = 0;  // {,m}
        }
        if (start != -2) {
            if (p->tok.type == TK_CLOSEDUP) {
                end = start;
            } else if (p->tok.type==TK_CHAR && p->tok.c==',') {
                end = rxnumber(p);
            } else {
                end = -2;
            }
        }
        if (start==-2 || end==-2) {
            p->err = p->tok.type==TK_END ? RXE_EBRACE : RXE_BADBR;
            return 0;
        }
        if ((end!=-1 && start>end) || p->tok.type!=TK_CLOSEDUP) {
            p->err = RXE_BADBR;
            return 0;
        }
        if ((end==-1 ? start : end) > RX_DUPMAX) {
            p->err = RXE_ESIZE;
            return 0;
        }
    } else {
        start = p->tok.type == TK_PLUS;
        end   = p->tok.type==TK_QUEST ? 1 : -1;
    }
    rxfetch(p);
    if (!elem || (!start && !end)) return 0;

    // e{n,m} is n copies of e, then ((e?e)?e)? for the rest
    Rt *old = 0;
    Rt *t   = 0;
    if (start > 0) {
        t = elem;
        for (i32 i = 2; i <= start; i++) {
            elem = rtdup(p, elem);
            if (p->err) return 0;
            t = newrt(p, RT_CONCAT, t, elem);
        }
        if (start == end) return t;
        elem = rtdup(p, elem);
        if (p->err) return 0;
        old = t;
    }
    if (elem->type == RT_SUBEXP) rtmarkopt(p, elem, elem->arg);
    t = newrt(p, end==-1 ? RT_STAR : RT_ALT, elem);
    for (i32 i = start+2; i <= end; i++) {
        elem = rtdup(p, elem);
        if (p->err) return 0;
        t = newrt(p, RT_CONCAT, t, elem);
        t = newrt(p, RT_ALT, t);
    }
    if (old) t = newrt(p, RT_CONCAT, old, t);
    return t;
}

static Rt *rxregexp(RxParse *p, i32 nest);

static Rt *rxgroup(RxParse *p, i32 nest)
{
    i32 g = p->nsub++;
    rxfetch(p);
    Rt *body = 0;
    if (p->tok.type != TK_CLOSE) {
        if (nest >= RX_MAXNEST) {
            p->err = RXE_ESPACE;
            return 0;
        }
        body = rxregexp(p, nest+1);
        if (p->err) return 0;
        if (p->tok.type != TK_CLOSE) {
            p->err = RXE_EPAREN;
            return 0;
        }
    }
    if (g <= 8) p->done |= 1u << g;
    return newrt(p, RT_SUBEXP, body, 0, g);
}

static Rt *rxexpr(RxParse *p, i32 nest)
{
    Rt   *t   = 0;
    RxTok tok = p->tok;
    switch (tok.type) {
    case TK_CHAR:
        if ((p->flags & REX_ICASE) && p->lead && p->pos<p->pat.len && !p->lead[p->pos]) {
            // a multibyte character with case, compared raised
            iz  at = p->pos - 1;
            for (; at>0 && !p->lead[at]; at--) {}
            i32 c  = unitat(p->raw, at, 1).c;
            if (cpupper(c)!=c || cplower(c)!=c) {
                while (p->pos<p->pat.len && !p->lead[p->pos]) rxfetch(p);
                RxSet  *s = alloc<RxSet>(p->a);
                RxRange r = {cpupper(c), cpupper(c)};
                s->ranges = push(p->a, s->ranges, r);
                s->fold   = 1;
                if (r.lo < 0x80) rxsetbit(s, r.lo);  // raised to ASCII
                t = newrt(p, RT_SET);
                t->set = s;
                break;
            }
        }
        t = newrt(p, RT_CHAR, 0, 0, tok.c);
        while (p->lead && p->pos<p->pat.len && !p->lead[p->pos]) {
            rxfetch(p);  // the rest of a multibyte character
            t = newrt(p, RT_CONCAT, t, newrt(p, RT_CHAR, 0, 0, p->tok.c));
        }
        break;
    case TK_OPEN:
        t = rxgroup(p, nest);
        if (p->err) return 0;
        break;
    case TK_BRACKET:
        t = rxbracket(p);
        if (p->err) return 0;
        break;
    case TK_BACKREF:
        if (!(p->done & (1u << tok.c))) {
            p->err = RXE_ESUBREG;
            return 0;
        }
        p->nbackref++;
        t = newrt(p, RT_BACKREF, 0, 0, tok.c);
        break;
    case TK_OPENDUP:
    case TK_STAR:
    case TK_PLUS:
    case TK_QUEST:
        p->err = RXE_BADRPT;
        return 0;
    case TK_CLOSE:     // only outside groups
    case TK_CLOSEDUP:
        t = newrt(p, RT_CHAR, 0, 0, tok.c);
        break;
    case TK_ANCHOR:
        p->wide |= tok.c==RA_WORDFIRST || tok.c==RA_WORDLAST ||
                   tok.c==RA_WORDDELIM || tok.c==RA_NOTWORDDELIM;
        if (tok.c == RA_WORDDELIM) {
            Rt *a = newrt(p, RT_ANCHOR, 0, 0, RA_WORDFIRST);
            Rt *b = newrt(p, RT_ANCHOR, 0, 0, RA_WORDLAST);
            t = newrt(p, RT_ALT, a, b);
        } else if (tok.c == RA_NOTWORDDELIM) {
            Rt *a = newrt(p, RT_ANCHOR, 0, 0, RA_INWORD);
            Rt *b = newrt(p, RT_ANCHOR, 0, 0, RA_INNOTWORD);
            t = newrt(p, RT_ALT, a, b);
        } else {
            t = newrt(p, RT_ANCHOR, 0, 0, tok.c);
        }
        rxfetch(p);
        return t;  // anchors take no repetition: ^* is an error
    case TK_PERIOD:
        t = newrt(p, RT_ANY);
        break;
    case TK_WORD:
    case TK_NOTWORD:
    case TK_SPACE:
    case TK_NOTSPACE:
        t = rxclassop(p, tok.type);
        break;
    case TK_ALT:
    case TK_END:
        return 0;  // an empty expression
    case TK_BACKSLASH:
        p->err = RXE_EESCAPE;
        return 0;
    }
    rxfetch(p);
    while (p->tok.type==TK_STAR || p->tok.type==TK_PLUS ||
           p->tok.type==TK_QUEST || p->tok.type==TK_OPENDUP) {
        t = rxdup(p, t);
        if (p->err) return 0;
    }
    p->toobig |= p->ntree > RX_MAXTREE;
    return t;
}

static b32 rxbranchend(RxParse *p, i32 nest)
{
    i32 t = p->tok.type;
    return t==TK_ALT || t==TK_END || (nest && t==TK_CLOSE);
}

static Rt *rxbranch(RxParse *p, i32 nest)
{
    Rt *t = rxexpr(p, nest);
    if (p->err) return 0;
    while (!rxbranchend(p, nest)) {
        Rt *e = rxexpr(p, nest);
        if (p->err) return 0;
        if (t && e) {
            t = newrt(p, RT_CONCAT, t, e);
        } else if (!t) {
            t = e;
        }
    }
    return t;
}

// Alternatives. A back-reference may not name a group of another
// alternative of the same list.
static Rt *rxregexp(RxParse *p, i32 nest)
{
    u32 initial = p->done;
    Rt *t = rxbranch(p, nest);
    if (p->err) return 0;
    while (p->tok.type == TK_ALT) {
        rxfetch(p);
        Rt *b = 0;
        if (!rxbranchend(p, nest)) {
            u32 seen = p->done;
            p->done = initial;
            b = rxbranch(p, nest);
            if (p->err) return 0;
            p->done |= seen;
        }
        t = newrt(p, RT_ALT, t, b);
    }
    return t;
}

static b32 rxeps(i32 type)
{
    return type>=RT_ANCHOR && type<=RT_STAR;
}

// The epsilon moves out of a node, its edests: none for a
// back-reference, whose move when it matches empty is no epsilon move
// to glibc (no closure goes through it). Returns how many.
static i32 rxedests(RxNode *x, i32 *d)
{
    if (!rxeps(x->type)) return 0;
    d[0] = x->e1;
    d[1] = x->e2;
    return x->e2<0 ? 1 : 2;
}

struct RxBuild {
    Slice<RxNode> nodes;
    Arena        *a;
    i32           err;
};

// glibc's duplicate_node: a copy of node org under constraints cons
static i32 rxdupnode(RxBuild *b, i32 org, i32 cons)
{
    if (b->nodes.len >= RX_MAXTREE) {
        b->err = RXE_ESPACE;
        return org;
    }
    RxNode n = b->nodes.data[org];
    n.cons |= cons;
    n.dup   = 1;
    n.org   = org;
    n.next  = n.e1 = n.e2 = -1;
    b->nodes = push(b->a, b->nodes, n);
    return (i32)b->nodes.len - 1;
}

// glibc's search_duplicated_node: among the copies made last
static i32 rxfinddup(RxBuild *b, i32 org, i32 cons)
{
    for (i32 i = (i32)b->nodes.len-1; i>0 && b->nodes.data[i].dup; i--) {
        RxNode *n = b->nodes.data + i;
        if (n->org==org && n->cons==cons) return i;
    }
    return -1;
}

static void rxaddedest(RxNode *x, i32 d)
{
    if (x->e1 < 0) {
        x->e1 = d;
    } else if (d != x->e1) {
        x->e2 = x->e1<d ? d : x->e1;
        x->e1 = x->e1<d ? x->e1 : d;
    }
}

struct RxDupTodo {
    i32 org;
    i32 clone;
    i32 cons;
};

// glibc's duplicate_node_closure: copy the epsilon closure after an
// anchor so that its nodes carry the anchor's constraints, in glibc's
// order, so that copies are numbered alike. Its recursion into a fork's
// first way is a stack of the second ways left to copy.
static void rxdupclosure(RxBuild *b, i32 root, i32 cons)
{
    Slice<RxDupTodo> todo = {};
    i32 org   = root;
    i32 clone = root;
    for (;;) {
        RxNode *on = b->nodes.data + org;
        b32     second = 0;
        if (on->type == RT_BACKREF) {
            i32 od = on->next;
            i32 cd = rxdupnode(b, od, cons);
            b->nodes.data[clone].next = b->nodes.data[org].next;
            b->nodes.data[clone].e1   = cd;
            b->nodes.data[clone].e2   = -1;
            org   = od;
            clone = cd;
        } else if (!rxeps(on->type)) {
            b->nodes.data[clone].next = on->next;
            if (!todo.len) break;
            RxDupTodo t = todo.data[--todo.len];
            org    = t.org;
            clone  = t.clone;
            cons   = t.cons;
            second = 1;
        } else if (on->e2 < 0) {
            i32 od = on->e1;
            b->nodes.data[clone].e1 = b->nodes.data[clone].e2 = -1;
            if (org==root && clone!=org) {
                // a loop back to the anchor: tie it to the anchor's copy
                b->nodes.data[clone].e1 = od;
                if (!todo.len) break;
                RxDupTodo t = todo.data[--todo.len];
                org    = t.org;
                clone  = t.clone;
                cons   = t.cons;
                second = 1;
            } else {
                cons |= on->cons;
                i32 cd = rxdupnode(b, od, cons);
                b->nodes.data[clone].e1 = cd;
                org   = od;
                clone = cd;
            }
        } else {
            i32 od = on->e1;
            b->nodes.data[clone].e1 = b->nodes.data[clone].e2 = -1;
            i32 cd = rxfinddup(b, od, cons);
            if (cd < 0) {
                cd = rxdupnode(b, od, cons);
                rxaddedest(b->nodes.data+clone, cd);
                RxDupTodo t = {org, clone, cons};
                todo  = push(b->a, todo, t);
                org   = od;
                clone = cd;
            } else {
                rxaddedest(b->nodes.data+clone, cd);
                second = 1;
            }
        }
        if (b->err) return;
        if (second) {
            i32 od = b->nodes.data[org].e2;
            i32 cd = rxdupnode(b, od, cons);
            rxaddedest(b->nodes.data+clone, cd);
            org   = od;
            clone = cd;
        }
        if (b->err) return;
    }
}

// glibc's calc_eclosure, for its side effect: anchors get their copies
// in the order glibc makes them, the first time its depth-first walk
// reaches each. The walk's bookkeeping of unfinished closures decides
// which nodes it enters again, so it is kept.
static void rxeclosures(RxBuild *b)
{
    struct Frame {
        i32 node;
        i32 k;
        b32 root;
        b32 incomplete;
    };
    Slice<u8> status = {};  // 0 to do, 2 in progress, 1 done
    for (iz i = 0; i < b->nodes.len; i++) status = push(b->a, status, (u8)0);
    Slice<Frame> stack = {};
    for (i32 i = 0; i < b->nodes.len; i++) {
        if (status.data[i]) continue;
        Frame f = {i, 0, 1, 0};
        stack = push(b->a, stack, f);
        for (b32 enter = 1; stack.len;) {
            Frame *t = stack.data + stack.len - 1;
            if (enter) {
                enter = 0;
                status.data[t->node] = 2;
                RxNode *x = b->nodes.data + t->node;
                if (x->cons && x->e1>=0 && !b->nodes.data[x->e1].dup) {
                    rxdupclosure(b, t->node, x->cons);
                    if (b->err) return;
                    while (status.len < b->nodes.len) status = push(b->a, status, (u8)0);
                    t = stack.data + stack.len - 1;
                }
            }
            RxNode *x = b->nodes.data + t->node;
            if (rxeps(x->type) && t->k<2 && (t->k ? x->e2 : x->e1)>=0) {
                i32 d = t->k++ ? x->e2 : x->e1;
                if (status.data[d] == 2) {
                    t->incomplete = 1;
                } else if (!status.data[d]) {
                    Frame g = {d, 0, 0, 0};
                    stack = push(b->a, stack, g);
                    enter = 1;
                }
                continue;
            }
            status.data[t->node] = t->incomplete && !t->root ? 0 : 1;
            b32 undone = !status.data[t->node];
            stack.len--;
            if (stack.len && undone) stack.data[stack.len-1].incomplete = 1;
        }
    }
}

// All nodes reachable from x by epsilon moves, constraints ignored:
// glibc's static closures, without back-references
static void rxstatic(Regex *re, i32 x, u32 *set, i32 *stack)
{
    i32 top = 0;
    set[x>>5] |= 1u << (x&31);
    stack[top++] = x;
    while (top) {
        RxNode *n = re->nodes + stack[--top];
        if (!rxeps(n->type)) continue;
        i32 d[2] = {n->e1, n->e2};
        for (i32 k = 0; k < 2; k++) {
            if (d[k]<0 || set[d[k]>>5]>>(d[k]&31) & 1) continue;
            set[d[k]>>5] |= 1u << (d[k]&31);
            stack[top++] = d[k];
        }
    }
}

// Compile pat. The result lives in a.
static RxComp rxcompile(Str pat, i32 flags, Arena *a)
{
    RxComp  r = {};
    RxParse p = {};
    p.raw   = pat;
    p.pat   = pat;
    p.a     = a;
    p.flags = flags;
    if (flags & REX_ICASE) {
        p.pat = {allocbytes(a, pat.len), pat.len};
        for (iz i = 0; i < pat.len; i++) p.pat.data[i] = rxupper(pat[i]);
    }
    if ((flags & REX_UTF8) && !allascii(pat)) {
        p.lead = alloc<u8>(a, pat.len);
        for (iz i = 0; i < pat.len;) {
            iz n = glibclen(pat, i);
            p.lead[i] = 1;
            i += n ? n : 1;
        }
    }

    rxfetch(&p);
    Rt *tree = rxregexp(&p, 0);
    if (!p.err && p.toobig) p.err = RXE_ESPACE;
    if (p.err) {
        r.err = p.err;
        return r;
    }
    Rt *end  = newrt(&p, RT_END);
    Rt *root = tree ? newrt(&p, RT_CONCAT, tree, end) : end;

    // A group whose body is just another group: the inner one reports
    // the outer one's span, and back-references to it follow.
    i32 *map = alloc<i32>(a, p.nsub+1);
    for (i32 i = 0; i < p.nsub; i++) map[i] = i;
    Slice<Rt *> stack = {};
    stack = push(a, stack, root);
    while (stack.len) {
        Rt *n = stack.data[--stack.len];
        if (n->type==RT_SUBEXP && n->left && n->left->type==RT_SUBEXP) {
            map[n->left->arg] = map[n->arg];
            n->left = n->left->left;
        } else if (n->type == RT_BACKREF) {
            n->arg = map[n->arg];
        }
        if (n->right) stack = push(a, stack, n->right);
        if (n->left)  stack = push(a, stack, n->left);
    }

    // Groups into open and close marks around their bodies
    Slice<Rt *> nodes = rtnodes(root, a);
    for (iz i = 0; i < nodes.len; i++) {
        Rt *n = nodes[i];
        if (n->type != RT_SUBEXP) continue;
        Rt *op = newrt(&p, RT_OPEN,  0, 0, n->arg);
        Rt *cl = newrt(&p, RT_CLOSE, 0, 0, n->arg);
        op->opt = cl->opt = n->opt;
        n->type  = RT_CONCAT;
        n->right = n->left ? newrt(&p, RT_CONCAT, n->left, cl) : cl;
        n->left  = op;
    }

    // Number in postorder: the reverse of a root-right-left preorder
    nodes = {};
    stack = {};
    stack = push(a, stack, root);
    while (stack.len) {
        Rt *n = stack.data[--stack.len];
        nodes = push(a, nodes, n);
        if (n->left)  stack = push(a, stack, n->left);
        if (n->right) stack = push(a, stack, n->right);
    }
    i32 count = 0;
    for (iz i = nodes.len-1; i >= 0; i--) {
        Rt *n = nodes[i];
        if (n->type == RT_CONCAT) {
            n->first = n->left->first;
        } else {
            n->first = n;
            n->idx   = count++;
        }
    }

    // What follows each node, in preorder
    stack = {};
    stack = push(a, stack, root);
    while (stack.len) {
        Rt *n = stack.data[--stack.len];
        switch (n->type) {
        case RT_STAR:
            n->left->next = n;
            break;
        case RT_CONCAT:
            n->left->next  = n->right->first;
            n->right->next = n->next;
            break;
        default:
            if (n->left)  n->left->next  = n->next;
            if (n->right) n->right->next = n->next;
        }
        if (n->right) stack = push(a, stack, n->right);
        if (n->left)  stack = push(a, stack, n->left);
    }

    RxBuild b = {};
    b.a = a;
    b.nodes.data = alloc<RxNode>(a, count);
    b.nodes.len  = b.nodes.cap = count;
    for (iz i = 0; i < nodes.len; i++) {
        Rt *n = nodes[i];
        if (n->type == RT_CONCAT) continue;
        RxNode *x = b.nodes.data + n->idx;
        x->type = n->type;
        x->arg  = n->arg;
        x->set  = n->set;
        x->opt  = n->opt;
        x->org  = n->idx;
        x->dup  = n->dup;  // so an anchor in a copy gets no copies
        x->next = x->e1 = x->e2 = -1;
        switch (n->type) {
        case RT_ALT:
        case RT_STAR: {
            i32 l = n->left  ? n->left->first->idx  : n->next->idx;
            i32 r = n->right ? n->right->first->idx : n->next->idx;
            x->e1 = l<r ? l : r;
            x->e2 = l==r ? -1 : l<r ? r : l;
        } break;
        case RT_ANCHOR:
            x->cons = n->arg;
            x->e1   = n->next->idx;
            break;
        case RT_OPEN:
        case RT_CLOSE:
            x->e1 = n->next->idx;
            break;
        case RT_BACKREF:
            x->next = x->e1 = n->next->idx;
            break;
        case RT_END:
            break;
        default:
            x->next = n->next->idx;
        }
    }
    rxeclosures(&b);
    if (b.err) {
        r.err = b.err;
        return r;
    }

    Regex *re = alloc<Regex>(a);
    count        = (i32)b.nodes.len;
    re->nodes    = b.nodes.data;
    re->nnodes   = count;
    re->start    = root->first->idx;
    re->nsub     = p.nsub;
    re->map      = map;
    re->flags    = flags;
    re->backrefs = p.nbackref > 0;
    re->midchar  = !(flags & REX_ICASE) && !p.wide;
    re->predbeg  = alloc<i32>(a, count+1);
    re->consume  = alloc<i32>(a, count);
    for (i32 i = 0; i < count; i++) {
        RxNode *x = re->nodes + i;
        re->plural |= x->type==RT_ALT || x->type==RT_STAR;
        if (x->type <= RT_BACKREF) re->consume[re->nconsume++] = i;
    }

    // Epsilon predecessors, for sifting
    for (i32 i = 0; i < count; i++) {
        i32 d[2];
        for (i32 k = rxedests(re->nodes+i, d)-1; k >= 0; k--) re->predbeg[d[k]]++;
    }
    i32 sum = 0;
    for (i32 i = 0; i <= count; i++) {
        i32 n = i<count ? re->predbeg[i] : 0;
        re->predbeg[i] = sum;
        sum += n;
    }
    re->preds = alloc<i32>(a, sum);
    i32 *fill = alloc<i32>(a, count);
    for (i32 i = 0; i < count; i++) {
        i32 d[2];
        for (i32 k = rxedests(re->nodes+i, d)-1; k >= 0; k--) {
            re->preds[re->predbeg[d[k]] + fill[d[k]]++] = i;
        }
    }

    re->refs = alloc<i32>(a, p.nsub);
    for (i32 i = 0; i < count; i++) {
        RxNode *x = re->nodes + i;
        if (x->type != RT_BACKREF) continue;
        i32 k = 0;
        while (k<re->nrefs && re->refs[k]!=x->arg) k++;
        if (k == re->nrefs) re->refs[re->nrefs++] = x->arg;
        i32 closes = 0;
        for (i32 j = 0; j < count; j++) {
            closes += re->nodes[j].type==RT_CLOSE && re->nodes[j].arg==x->arg;
        }
        re->copies |= closes > 1;
    }

    // The initial state's nodes, as glibc's create_initial_state: the
    // start's closure, constraints ignored, and the closure after each
    // back-reference there whose group has a close node there, as if it
    // matched empty, whether or not its group was set on the way
    re->init      = alloc<u32>(a, (count+31)/32);
    re->initclose = alloc<b32>(a, p.nsub);
    u32 *init = re->init;
    i32 *work = alloc<i32>(a, count);
    rxstatic(re, re->start, init, work);
    for (b32 grew = 1; grew;) {
        grew = 0;
        for (i32 i = 0; i < count; i++) {
            RxNode *x = re->nodes + i;
            if (x->type==RT_CLOSE && init[i>>5]>>(i&31)&1) re->initclose[x->arg] = 1;
        }
        for (i32 i = 0; i < count; i++) {
            RxNode *x = re->nodes + i;
            i32     d = x->e1;
            if (x->type!=RT_BACKREF || !(init[i>>5]>>(i&31)&1)) continue;
            if (re->initclose[x->arg] && !(init[d>>5]>>(d&31)&1)) {
                rxstatic(re, d, init, work);
                grew = 1;
            }
        }
    }
    for (i32 i = 0; i < count; i++) {
        if (init[i>>5]>>(i&31) & 1) re->initcons |= re->nodes[i].cons != 0;
    }
    r.re = re;
    return r;
}


// Matching

struct RxCtx {
    Regex *re;
    Str    s;
    u8    *ulen;    // UTF-8 with non-ASCII: character length at each byte,
                    // 0 inside a character, 0xff for an invalid byte
    i64    budget;  // steps left for back-references, negative for no limit
    b32    gaveup;
    i32    words;   // u32 words in a node bitset
    iz     memomax; // bytes for each back-reference search's RxMemo
};

// Whether a match may start at p. glibc's search skips the inside of a
// UTF-8 character, except when its compiler found the pattern simple
// enough to run byte by byte (optimize_utf8: no REG_ICASE, no word
// anchors, no bracket with a class, a range, negation or a multibyte
// character), when nothing tells it a byte is inside a character.
static b32 rxstart(RxCtx *c, iz p)
{
    return !c->ulen || p==c->s.len || c->ulen[p] || c->re->midchar;
}

static u8 rxtr(RxCtx *c, u8 b)
{
    return (c->re->flags & REX_ICASE) ? rxupper(b) : b;
}

static b32 rxwordbyte(u8 b)
{
    return (b>='0' && b<='9') || (b>='a' && b<='z') || (b>='A' && b<='Z') || b=='_';
}

// Whether the character holding byte i is a word character. In UTF-8
// glibc asks iswalnum, and takes an invalid byte as the code point of
// its value (ÿ as ÿ, a letter).
static b32 rxwordat(RxCtx *c, iz i)
{
    if (!c->ulen || c->s[i]<0x80) return rxwordbyte(c->s[i]);
    for (; i>0 && !c->ulen[i]; i--) {}
    i32 cp = c->ulen[i]==0xff ? c->s[i] : unitat(c->s, i, 1).c;
    return !!(cpclasses(cp) & UC_ALNUM);
}

// Whether the constraints k on what precedes p hold, in a match from
// st: the start of the subject or a character, but a newline counts as
// one only when the match itself consumed it, as glibc's automaton
// sees it without REG_NEWLINE.
static b32 rxprev(RxCtx *c, i32 k, iz p, iz st)
{
    Str s  = c->s;
    b32 pw = p>0 && rxwordat(c, p-1);
    if ((k & RC_PREVWORD)    && !pw) return 0;
    if ((k & RC_PREVNOTWORD) &&  pw) return 0;
    if ((k & RC_PREVNEWLINE) && p>0 && !(p>st && s[p-1]=='\n')) return 0;
    if ((k & RC_PREVBEGBUF)  && p>0) return 0;
    return 1;
}

// Whether the constraints k on what follows p hold: the end of the
// subject or a character. A newline counts as one only when the
// automaton consumes it (consume), not when the match ends before it
// or when sifting checks the node that consumes it.
static b32 rxnext(RxCtx *c, i32 k, iz p, b32 consume)
{
    Str s  = c->s;
    iz  n  = s.len;
    b32 w  = p<n && rxwordat(c, p);
    b32 nl = p==n || (consume && s[p]=='\n');
    if ((k & RC_NEXTWORD)    && !w)  return 0;
    if ((k & RC_NEXTNOTWORD) &&  w)  return 0;
    if ((k & RC_NEXTNEWLINE) && !nl) return 0;
    if ((k & RC_NEXTENDBUF)  && p<n) return 0;
    return 1;
}

// How many bytes a consuming node takes at p, or 0 if it cannot.
static iz rxaccept(RxCtx *c, RxNode *x, iz p)
{
    if (p >= c->s.len) return 0;
    u8 b = c->s[p];
    u8 n = c->ulen ? c->ulen[p] : 1;
    switch (x->type) {
    case RT_CHAR:
        if (n>1 && n!=0xff && (c->re->flags & REX_ICASE)) {
            // a character raised to ASCII, as ı to I
            return cpupper(unitat(c->s, p, 1).c)==x->arg ? n : 0;
        }
        return rxtr(c, b) == x->arg;
    case RT_ANY:
        return n==0xff ? 0 : n;
    case RT_SET: {
        RxSet *s = x->set;
        if (n == 1) {
            u8 t = rxtr(c, b);
            return s->bits[t>>5]>>(t&31) & 1;
        }
        if (!n || n==0xff) return 0;
        i32 cp = unitat(c->s, p, 1).c;
        cp = s->fold ? cpupper(cp) : cp;
        if (cp < 0x80) return (s->bits[cp>>5]>>(cp&31) & 1) ? n : 0;  // raised to ASCII
        b32 in = s->classes && (cpclasses(cp) & s->classes);
        for (iz i = 0; i < s->ranges.len; i++) {
            in |= s->ranges[i].lo<=cp && cp<=s->ranges[i].hi;
        }
        return in!=s->neg ? n : 0;
    }
    }
    return 0;
}

static b32 rxbit(u32 *set, i32 i)
{
    return set[i>>5]>>(i&31) & 1;
}

static void rxset(u32 *set, i32 i)
{
    set[i>>5] |= 1u << (i&31);
}

struct RxList {
    i32 *dense;
    i32 *sparse;
    iz  *start;  // the earliest start reaching each node
    u8  *ok;     // the node's constraints on what precedes hold
    i32  len;
};

// n rows of w words: a subject's positions times the node sets, which
// overflows a 32-bit size for subjects of a few megabytes
static u32 *rxtable(Arena *a, iz n, i32 w)
{
    if (w && n > (iz)((uz)-1 >> 1)/(iz)sizeof(u32)/w) os_oom();
    return alloc<u32>(a, n*w);
}

static RxList newlist(Arena *a, i32 nn)
{
    RxList l = {};
    l.dense  = alloc<i32>(a, nn);
    l.sparse = alloc<i32>(a, nn);
    l.start  = alloc<iz>(a, nn);
    l.ok     = alloc<u8>(a, nn);
    return l;
}

static b32 rxhas(RxList *l, i32 x)
{
    i32 i = l->sparse[x];
    return i<l->len && l->dense[i]==x;
}

// Add node x at p, reached from start st, with its epsilon closure, as
// glibc's states hold them: the whole closure, nodes whose constraints
// on the preceding character fail marked unusable. A node there from a
// later start takes this one.
static void rxthread(RxCtx *c, RxList *l, i32 x, iz st, iz p, i32 *stack)
{
    RxNode *nodes = c->re->nodes;
    i32     top   = 0;
    stack[top++] = x;
    while (top) {
        x = stack[--top];
        RxNode *n = nodes + x;
        if (!rxhas(l, x)) {
            l->sparse[x] = l->len;
            l->dense[l->len++] = x;
        } else if (l->start[x] <= st) {
            continue;
        }
        l->start[x] = st;
        l->ok[x]    = !n->cons || rxprev(c, n->cons, p, st);
        if (rxeps(n->type)) {
            stack[top++] = n->e1;
            if (n->e2 >= 0) stack[top++] = n->e2;
        }
    }
}

// Take the characters at p from the threads of list l into the lists
// ahead, those from starts after limit left out.
static void rxstep(RxCtx *c, RxList *lists, i32 nl, iz p, iz limit, iz last, i32 *stack)
{
    Regex  *re = c->re;
    RxList *l  = lists + p%nl;
    for (i32 i = 0; i < l->len; i++) {
        i32     x  = l->dense[i];
        iz      st = l->start[x];
        RxNode *nd = re->nodes + x;
        if (nd->type>RT_ANY || st>limit || !l->ok[x]) continue;
        iz k = rxaccept(c, nd, p);
        if (!k || p+k>last || !rxnext(c, nd->cons, p, 1)) continue;
        rxthread(c, lists+(p+k)%nl, nd->next, st, p+k, stack);
    }
}

// The leftmost-longest match starting at from or later, without
// back-references.
static RxMatch rxpike(RxCtx *c, iz from, Arena scratch)
{
    Regex  *re = c->re;
    i32     nn = re->nnodes;
    i32     nl = c->ulen ? 8 : 2;  // positions ahead: up to 6 bytes
    RxList  lists[8] = {};
    for (i32 i = 0; i < nl; i++) lists[i] = newlist(&scratch, nn);
    i32    *stack = alloc<i32>(&scratch, 2*nn+2);
    RxMatch best  = {-1, -1};
    iz      n     = c->s.len;
    for (iz p = from; p <= n; p++) {
        RxList *l = lists + p%nl;
        if (best.so<0 && rxstart(c, p)) {
            rxthread(c, l, re->start, p, p, stack);
        }
        for (i32 i = 0; i < l->len; i++) {
            i32     x  = l->dense[i];
            RxNode *nd = re->nodes + x;
            iz      st = l->start[x];
            if (nd->type!=RT_END || !l->ok[x] || !rxnext(c, nd->cons, p, 0)) continue;
            if (best.so<0 || st<=best.so) {
                best.so = st;
                best.eo = p;
            }
        }
        rxstep(c, lists, nl, p, best.so<0 ? n : best.so, n, stack);
        l->len = 0;
        if (best.so >= 0) {
            b32 live = 0;
            for (i32 i = 0; i < nl; i++) live |= lists[i].len > 0;
            if (!live) break;
        }
    }
    return best;
}

// glibc's last_node for the match [s, e]: the lowest-numbered end node
// (there are copies, after anchors) in its state at e that may end it.
// Sifting and the walk must arrive at that one.
static i32 rxlastnode(RxCtx *c, iz s, iz e, Arena scratch)
{
    Regex  *re = c->re;
    i32     nn = re->nnodes;
    i32     nl = c->ulen ? 8 : 2;
    RxList  lists[8] = {};
    for (i32 i = 0; i < nl; i++) lists[i] = newlist(&scratch, nn);
    i32    *stack = alloc<i32>(&scratch, 2*nn+2);
    rxthread(c, lists + s%nl, re->start, s, s, stack);
    for (iz p = s; p < e; p++) {
        rxstep(c, lists, nl, p, s, e, stack);
        lists[p%nl].len = 0;
    }
    RxList *l    = lists + e%nl;
    i32     last = -1;
    for (i32 i = 0; i < l->len; i++) {
        i32     x  = l->dense[i];
        RxNode *nd = re->nodes + x;
        if (nd->type==RT_END && l->ok[x] && rxnext(c, nd->cons, e, 0) && (last<0 || x<last)) {
            last = x;
        }
    }
    return last;
}

// glibc's search skips ahead while its automaton stays in its initial
// state, and an empty match at the start does not stop that: the match
// is then reported as many characters later (a*\B on "ca." reports
// (2,2) for the empty match at 1). The number of characters it skips
// after an empty match at p, the only one from p. With back-references
// glibc does not skip.
static iz rxstall(RxCtx *c, iz p, Arena scratch)
{
    Regex *re = c->re;
    if (re->backrefs) return 0;
    Str    s  = c->s;
    i32    nn = re->nnodes;
    i32    w  = c->words;
    i32    c0 = p==0 ? 3 : rxwordat(c, p-1) ? 1 : 0;  // the state's context
    i32   *stack = alloc<i32>(&scratch, nn);
    u32   *next  = alloc<u32>(&scratch, w);
    iz     k     = 0;
    for (; p+k < s.len; k++) {
        iz q  = p + k;
        u8 ch = s[q];
        if (ch >= 0x80) break;
        i32 c1 = rxwordbyte(ch) ? 1 : ch=='\n' ? 2 : 0;
        if (re->initcons && c1!=c0) break;
        for (i32 i = 0; i < w; i++) next[i] = 0;
        for (i32 x = 0; x < nn; x++) {
            RxNode *nd = re->nodes + x;
            if (!rxbit(re->init, x) || nd->type>RT_ANY) continue;
            if (nd->cons && !rxprev(c, nd->cons, p, p)) continue;
            if (!rxaccept(c, nd, q) || !rxnext(c, nd->cons, q, 1)) continue;
            rxstatic(re, nd->next, next, stack);
        }
        for (i32 i = 0; i < w; i++) {
            if (next[i] != re->init[i]) return k;
        }
    }
    return k;
}

// A span glibc's get_subexp found for a back-reference: node at idx may
// match len bytes there
struct RxEntry {
    iz  idx;
    i32 node;
    i32 len;
};

// The back-references' spans at each position in [s, e]: those at p
// are ents[order[at[p-s]]] to ents[order[at[p-s+1]-1]].
struct RxSpans {
    i32 *at;
    i32 *order;
};

static RxSpans rxspans(Slice<RxEntry> ents, iz s, iz e, Arena *a)
{
    RxSpans r = {};
    r.at    = alloc<i32>(a, e-s+2);
    r.order = alloc<i32>(a, ents.len);
    for (iz i = 0; i < ents.len; i++) {
        iz p = ents[i].idx;
        if (p>=s && p<=e && ents[i].len>=0) r.at[p-s+1]++;
    }
    for (iz i = 0; i <= e-s; i++) r.at[i+1] += r.at[i];
    i32 *fill = alloc<i32>(a, e-s+1);
    for (iz i = 0; i < ents.len; i++) {
        iz p = ents[i].idx;
        if (p>=s && p<=e && ents[i].len>=0) r.order[r.at[p-s] + fill[p-s]++] = (i32)i;
    }
    return r;
}

// For each position in [s, e], the nodes in glibc's automaton state
// with back-references there, which moves along the spans found (ents)
// whatever path led to each: closures of what arrives at a position,
// holding the nodes whose context allows them. Where a back-reference
// arrives, empty or not, glibc builds the state again from the context
// as it is, so that a newline before it is no line start, for every
// node there. Also, its initial state grows through back-references
// whose groups close in it (see re->init), before its context filters
// it.
static u32 *rxreach(RxCtx *c, iz s, iz e, Slice<RxEntry> ents, Arena *a)
{
    Regex  *re    = c->re;
    i32     w     = c->words;
    u32    *in    = rxtable(a, e-s+1, w);
    u32    *seed  = rxtable(a, e-s+1, w);  // what arrives at each position
    u8     *ref   = alloc<u8>(a, e-s+1);   // a back-reference arrives
    u32    *seen  = alloc<u32>(a, w);
    i32    *work  = alloc<i32>(a, re->nnodes);
    RxSpans sp    = rxspans(ents, s, e, a);
    rxset(seed, re->start);

    for (iz p = s; p <= e; p++) {
        u32 *row  = in + (p-s)*w;
        i32 *pos  = sp.order + sp.at[p-s];  // the spans here
        i32  npos = sp.at[p-s+1] - sp.at[p-s];

        // The closure, again from the context as it is when an empty
        // back-reference turns out to arrive
        for (b32 again = 1; again;) {
            again = 0;
            i32 top = 0;
            for (i32 i = 0; i < w; i++) {
                seen[i] = seed[(p-s)*w+i];
                row[i]  = 0;
            }
            for (i32 x = 0; x < re->nnodes; x++) {
                if (rxbit(seen, x)) work[top++] = x;
            }
            while (top) {
                i32     x    = work[--top];
                RxNode *nd   = re->nodes + x;
                iz      from = ref[p-s] ? p : s;
                b32     g    = !nd->cons || rxprev(c, nd->cons, p, from);
                i32     d[2] = {-1, -1};
                if (g) rxset(row, x);
                if (rxeps(nd->type)) {
                    d[0] = nd->e1;
                    d[1] = nd->e2;
                } else if (nd->type == RT_BACKREF) {
                    b32 empty = 0;
                    for (i32 j = 0; g && j<npos; j++) {
                        RxEntry *en = ents.data + pos[j];
                        empty |= en->node==x && !en->len;
                    }
                    if (empty && !ref[p-s]) {
                        ref[p-s] = 1;
                        again    = 1;
                    }
                    if (empty || (p==s && re->initclose[nd->arg])) d[0] = nd->e1;
                }
                for (i32 k = 0; k < 2; k++) {
                    if (d[k]>=0 && !rxbit(seen, d[k])) {
                        rxset(seen, d[k]);
                        work[top++] = d[k];
                    }
                }
            }
        }

        // Moves on from the nodes in the state
        for (i32 x = 0; x < re->nnodes; x++) {
            RxNode *nd = re->nodes + x;
            if (!rxbit(row, x)) continue;
            if (nd->type == RT_BACKREF) {
                for (i32 j = 0; j < npos; j++) {
                    RxEntry *en = ents.data + pos[j];
                    if (en->node!=x || !en->len || p+en->len>e) continue;
                    rxset(seed + (p-s+en->len)*w, nd->next);
                    ref[p-s+en->len] = 1;
                }
            } else if (nd->type <= RT_ANY) {
                iz k = rxaccept(c, nd, p);
                if (k && p+k<=e && rxnext(c, nd->cons, p, 1)) {
                    rxset(seed + (p-s+k)*w, nd->next);
                }
            }
        }
    }
    return in;
}

// For each position in [s, e], the nodes from which the match can still
// arrive at the end node last at e: glibc's sifted states. Like glibc,
// it keeps only nodes in the automaton's state there (states, from s,
// see rxreach; null for every node), which rules out the original of a
// node an anchor copied, as in $.()\1| on "\n". A node that consumes is
// checked as glibc's check_node_accept does, where a newline does not
// satisfy a $ before it. A back-reference is in a set when one of its
// spans (ents) leads into the set where it ends, as glibc's
// sift_states_bkref, and only that way: to glibc, its move when it
// matches empty is no epsilon move.
static u32 *rxsift(RxCtx *c, i32 last, iz s, iz e, Slice<RxEntry> ents, u32 *states, Arena *a)
{
    Regex  *re    = c->re;
    i32     w     = c->words;
    u32    *v     = rxtable(a, e-s+1, w);
    i32    *work  = alloc<i32>(a, re->nnodes);
    u32    *seen  = alloc<u32>(a, w);  // reached, viable or not
    RxSpans sp    = rxspans(ents, s, e, a);
    i32    *at    = sp.at;
    i32    *order = sp.order;

    for (iz p = e; p >= s; p--) {
        u32 *cur = v + (p-s)*w;
        u32 *in  = states ? states+(p-s)*w : 0;
        i32  top = 0;
        for (i32 i = 0; i < w; i++) seen[i] = 0;
        if (p == e) {
            rxset(cur, last);
            rxset(seen, last);
            work[top++] = last;
        }
        for (i32 i = 0; i<re->nconsume && p<e; i++) {
            i32     x  = re->consume[i];
            RxNode *nd = re->nodes + x;
            if (nd->type==RT_BACKREF || (in && !rxbit(in, x))) continue;
            if (nd->cons && !rxprev(c, nd->cons, p, s)) continue;
            iz k = rxaccept(c, nd, p);
            if (k && p+k<=e && rxnext(c, nd->cons, p, 0) &&
                    rxbit(v+(p+k-s)*w, nd->next) && !rxbit(seen, x)) {
                rxset(cur, x);
                rxset(seen, x);
                work[top++] = x;
            }
        }
        for (;;) {
            // Epsilon nodes whose static closures reach those, as glibc's
            // inverse closures, through nodes the context rules out
            while (top) {
                i32 m = work[--top];
                for (i32 j = re->predbeg[m]; j < re->predbeg[m+1]; j++) {
                    i32     q  = re->preds[j];
                    RxNode *nq = re->nodes + q;
                    if (rxbit(seen, q)) continue;
                    rxset(seen, q);
                    work[top++] = q;
                    if (in && !rxbit(in, q)) continue;
                    if (!nq->cons || rxprev(c, nq->cons, p, s)) rxset(cur, q);
                }
            }
            // Back-references here whose spans lead on, the empty ones
            // into this set, and what reaches them
            for (i32 j = at[p-s]; j < at[p-s+1]; j++) {
                RxEntry *en = ents.data + order[j];
                RxNode  *nd = re->nodes + en->node;
                i32      d  = en->len ? nd->next : nd->e1;
                if (rxbit(seen, en->node) || (in && !rxbit(in, en->node))) continue;
                if (p+en->len > e) continue;
                if (!rxbit(v+(p+en->len-s)*w, d)) continue;
                rxset(cur, en->node);
                rxset(seen, en->node);
                work[top++] = en->node;
            }
            if (!top) break;
        }
    }
    return v;
}

// glibc's update_regs: group marks set registers. A group closing
// empty inside an optional repetition, after it matched before, undoes
// everything since the last group that closed non-empty.
static void rxupdate(Regex *re, RxMatch *regs, RxMatch *prev, i32 x, iz idx)
{
    RxNode *n  = re->nodes + x;
    i32     ng = re->nsub + 1;
    if (n->type == RT_OPEN) {
        regs[n->arg+1].so = idx;
        regs[n->arg+1].eo = -1;
    } else if (n->type == RT_CLOSE) {
        RxMatch *r = regs + n->arg + 1;
        if (r->so < idx) {
            r->eo = idx;
            for (i32 i = 0; i < ng; i++) prev[i] = regs[i];
        } else if (n->opt && prev[n->arg+1].so!=-1) {
            for (i32 i = 0; i < ng; i++) regs[i] = prev[i];
        } else {
            r->eo = idx;
        }
    }
}

static b32 rxsame(RxCtx *c, iz a, iz b, iz len)
{
    if (b+len > c->s.len) return 0;
    for (iz i = 0; i < len; i++) {
        if (rxtr(c, c->s[a+i]) != rxtr(c, c->s[b+i])) return 0;
    }
    return 1;
}

static b32 rxspend(RxCtx *c)
{
    if (c->budget < 0) return 1;
    if (!c->budget) {
        c->gaveup = 1;
        return 0;
    }
    c->budget--;
    return 1;
}

// The states a back-reference search has visited after a character,
// keyed by everything its future depends on, in a 4-ary hash trie.
// Coming back to one adds nothing new, which makes the searches
// polynomial where the paths are exponential, as with ((a|aa)*)\1\1c.
// A state is only recorded once its node and position were reached
// before, so searches where nothing repeats, like (.*)x\1, pay for a
// bit each, and those bits wait until the search has consumed as many
// characters as they take words. Records come from chunks of the
// search's own arena, up to memomax bytes, after which states are not
// recorded; when the search itself is short of room, the memo is
// dropped and its chunks hold the search's frames instead. Positions
// in keys are 32-bit, so longer subjects get none.
enum : i32 { RS_OPEN, RS_DEAD, RS_UNCLOSED };

struct RxSeen {
    RxSeen  *child[4];
    RxSeen  *up;     // the walk's unfinished states, latest first
    RxMatch *regs;   // RS_UNCLOSED: the registers there
    i32     *key;
    i32      depth;  // the walk's fail stack when it got here
    i32      how;    // how the walk's search from here ended
};

struct RxChunk {
    RxChunk *next;
    byte    *end;
};

struct RxMemo {
    RxSeen  *root;
    Arena   *a;
    Arena    blk;     // the rest of the latest chunk
    RxChunk *chunks;  // latest first
    i32     *key;     // the state to look up
    u32     *once;    // node-major bits: (node, position) reached
    iz       base;    // the position of once's first bit
    iz       row;     // words for each node in once
    iz       left;    // bytes it may still take
    iz       wait;    // characters before it starts
    i32      nnodes;
    i32      len;     // values in a key
    b32      off;
};

static iz rxavail(Arena *a)
{
    return (a->mem ? a->mem->limit : a->end) - a->beg;
}

// Zeroed bytes from the memo's chunks, or null when it has no room.
static void *rxkeep(RxMemo *m, iz size)
{
    if (m->off) return 0;
    size = (size + 15) & -(iz)16;
    if (m->blk.end-m->blk.beg < size) {
        // 64 KiB, or less to leave half the arena to the search
        iz need = (iz)sizeof(RxChunk) + 32 + size;
        iz want = need>((iz)1<<16) ? need : (iz)1<<16;
        want = want<rxavail(m->a)/2 ? want : rxavail(m->a)/2;
        want = want<m->left ? want : m->left;
        if (want < need) return 0;
        m->left -= want;
        byte    *p = (byte *)allocbytes(m->a, want);
        RxChunk *k = (RxChunk *)(p + (-(uz)p & 15));
        k->next    = m->chunks;
        k->end     = p + want;
        m->chunks  = k;
        m->blk.beg = (byte *)k + ((sizeof(RxChunk) + 15) & -(uz)16);
        m->blk.end = k->end;
    }
    byte *r = m->blk.beg;
    m->blk.beg += size;
    __builtin_memset(r, 0, (uz)size);
    return r;
}

// A memo for keys of len values whose positions lie in [base, base+span].
static RxMemo newmemo(RxCtx *c, Arena *a, i32 len, iz base, iz span)
{
    RxMemo m = {};
    m.a      = a;
    m.left   = c->memomax;
    m.len    = len;
    m.base   = base;
    m.row    = span/32 + 1;
    m.nnodes = c->re->nnodes;
    m.off    = m.row > m.left/4/m.nnodes;
    m.wait   = m.row * m.nnodes;
    return m;
}

// The arena for size bytes of the search's own, from the memo's chunks
// when the search's arena is short, which ends the memo.
static Arena *rxframes(RxMemo *m, iz size)
{
    if (rxavail(m->a) >= size) return m->a;
    if (!m->off) {
        m->off  = 1;
        m->root = 0;
        m->blk  = {};
    }
    while (m->blk.end-m->blk.beg<size && m->chunks) {
        RxChunk *k = m->chunks;
        m->chunks  = k->next;
        m->blk.beg = (byte *)k;
        m->blk.end = k->end;
    }
    return m->blk.end-m->blk.beg>=size ? &m->blk : m->a;
}

// Whether the search reached node x at position idx before, marking it
// reached: only then can its state repeat.
static b32 rxagain(RxMemo *m, i32 x, iz idx)
{
    if (m->off) return 0;
    if (!m->once) {
        // Zeroing the bits waits until the search has done as much
        // work, so searches that end soon, as most do, pay nothing
        if (m->wait-- > 0) return 0;
        m->key  = (i32 *)rxkeep(m, m->len*(iz)sizeof(i32));
        m->once = (u32 *)rxkeep(m, m->row*m->nnodes*(iz)sizeof(u32));
        m->off  = !m->key || !m->once;
        if (m->off) return 0;
    }
    iz   p   = idx - m->base;
    u32 *w   = m->once + x*m->row + p/32;
    u32  bit = (u32)1 << (p%32);
    b32  r   = !!(*w & bit);
    *w |= bit;
    return r;
}

// The record of state m->key, null if it is not recorded. Sets *old when
// it was visited before.
static RxSeen *rxvisit(RxMemo *m, b32 *old)
{
    i32 *key = m->key;
    *old = 0;
    if (m->off) return 0;
    u64 h = 0x100;
    for (i32 i = 0; i < m->len; i++) {
        h ^= (u32)key[i];
        h *= 1111111111111111111u;
    }
    RxSeen **e = &m->root;
    for (; *e; h <<= 2) {
        i32 i = 0;
        while (i<m->len && (*e)->key[i]==key[i]) i++;
        if (i == m->len) {
            *old = 1;
            return *e;
        }
        e = &(*e)->child[h>>62];
    }
    iz    size = (iz)sizeof(RxSeen) + m->len*(iz)sizeof(i32);
    byte *r    = (byte *)rxkeep(m, size);
    if (r) {
        *e = (RxSeen *)r;
        (*e)->key = (i32 *)(r + sizeof(RxSeen));
        copybytes((*e)->key, key, m->len*(iz)sizeof(i32));
    }
    return *e;
}

struct RxFail {
    RxFail  *next;
    RxMatch *regs;  // registers, then the previous registers
    u32     *eps;
    iz       idx;
    i32      node;
};

struct RxWalk {
    RxCtx   *c;
    u32     *v;      // sifted sets from s, or null when all nodes are viable
    iz       s;
    iz       e;
    RxMatch *regs;
    RxMatch *prev;
    u32     *eps;    // epsilon nodes passed since the last character
    iz       idx;
    RxFail  *fails;
    RxFail  *free;
    i32      nfails;
    RxMemo   seen;
    RxSeen  *open;   // states whose searches have not ended
    b32      fs;     // backtracking
};

static b32 rxviable(RxWalk *w, iz idx, i32 x)
{
    return !w->v || rxbit(w->v + (idx-w->s)*w->c->words, x);
}

static void rxpush(RxWalk *w, i32 node)
{
    i32     ng = w->c->re->nsub + 1;
    i32     nw = w->c->words;
    RxFail *f  = w->free;
    if (f) {
        w->free = f->next;
    } else {
        iz     size = (iz)sizeof(RxFail) + 2*ng*(iz)sizeof(RxMatch) + nw*4 + 32;
        Arena *a    = rxframes(&w->seen, size);
        if (w->seen.off) w->open = 0;
        f = alloc<RxFail>(a);
        f->regs = alloc<RxMatch>(a, 2*ng);
        f->eps  = alloc<u32>(a, nw);
    }
    f->idx  = w->idx;
    f->node = node;
    copybytes(f->regs, w->regs, ng*(iz)sizeof(RxMatch));
    copybytes(f->regs+ng, w->prev, ng*(iz)sizeof(RxMatch));
    copybytes(f->eps, w->eps, nw*(iz)sizeof(u32));
    f->next  = w->fails;
    w->fails = f;
    w->nfails++;
}

static i32 rxpop(RxWalk *w)
{
    RxFail *f = w->fails;
    if (!f) return -1;
    i32 ng = w->c->re->nsub + 1;
    w->fails = f->next;
    w->nfails--;
    w->idx   = f->idx;
    copybytes(w->regs, f->regs, ng*(iz)sizeof(RxMatch));
    copybytes(w->prev, f->regs+ng, ng*(iz)sizeof(RxMatch));
    copybytes(w->eps, f->eps, w->c->words*(iz)sizeof(u32));
    f->next = w->free;
    w->free = f;
    return f->node;
}

// glibc's proceed_next_node: the node after x, or -1.
static i32 rxproceed(RxWalk *w, i32 x)
{
    RxCtx  *c  = w->c;
    Regex  *re = c->re;
    RxNode *n  = re->nodes + x;
    if (rxeps(n->type)) {
        rxset(w->eps, x);
        i32 d[2]  = {n->e1, n->e2};
        i32 dest  = -1;
        for (i32 k = 0; k < 2; k++) {
            i32 cand = d[k];
            if (cand<0 || !rxviable(w, w->idx, cand)) continue;
            if (dest < 0) {
                dest = cand;
            } else {
                // A fork passed before goes the other way: (a*)* ends
                if (rxbit(w->eps, dest)) return cand;
                if (w->fs) rxpush(w, cand);
                break;
            }
        }
        return dest;
    }

    iz k = 0;
    if (n->type == RT_BACKREF) {
        RxMatch r = w->regs[n->arg+1];
        k = r.eo - r.so;
        if (w->fs) {
            if (r.so==-1 || r.eo==-1) return -1;
            if (k && !rxsame(c, r.so, w->idx, k)) return -1;
        }
        if (k < 0) return -1;
        if (!k) {
            rxset(w->eps, x);
            if (rxviable(w, w->idx, n->e1)) return n->e1;
            return -1;
        }
    } else {
        k = rxaccept(c, n, w->idx);
        if (!k || !rxnext(c, n->cons, w->idx, 0)) return -1;
    }
    iz idx = w->idx + k;
    if (w->fs && (idx>w->e || !rxviable(w, idx, n->next))) return -1;
    for (i32 i = 0; i < c->words; i++) w->eps[i] = 0;
    w->idx = idx;
    return n->next;
}

// Backtrack from a dead end, or from an end with an unclosed group whose
// registers were end: the node to go on from, or -1. A state's search
// ended when the walk goes back past the fail stack it found, the last
// way it ended being this one.
static i32 rxfail(RxWalk *w, i32 how, RxMatch *end)
{
    i32      ng   = w->c->re->nsub + 1;
    iz       size = ng * (iz)sizeof(RxMatch);
    RxMatch *copy = end==w->regs ? 0 : end;
    for (; w->open && w->open->depth>=w->nfails && w->fails; w->open = w->open->up) {
        RxSeen *st = w->open;
        st->how = how;
        if (how==RS_UNCLOSED && !copy) {
            copy = (RxMatch *)rxkeep(&w->seen, size);
            if (copy) copybytes(copy, end, size);
        }
        st->regs = copy;
        if (how==RS_UNCLOSED && !copy) st->how = RS_OPEN;
    }
    return rxpop(w);
}

// The state the walk reached after a character, at node x: its record
// if a search from there already ended, else null.
static RxSeen *rxended(RxWalk *w, i32 x)
{
    RxMatch *regs = w->regs;
    RxMatch *prev = w->prev;
    i32      ng   = w->c->re->nsub + 1;
    i32     *key  = w->seen.key;
    if (!rxagain(&w->seen, x, w->idx)) return 0;
    key[0] = x;
    key[1] = (i32)w->idx;
    for (i32 i = 0; i < ng; i++) {
        key[2+4*i] = (i32)regs[i].so;
        key[3+4*i] = (i32)regs[i].eo;
        key[4+4*i] = (i32)prev[i].so;
        key[5+4*i] = (i32)prev[i].eo;
    }
    b32     old = 0;
    RxSeen *st  = rxvisit(&w->seen, &old);
    if (st && !old) {
        st->depth = w->nfails;
        st->up    = w->open;
        w->open   = st;
    }
    return old && st->how!=RS_OPEN ? st : 0;
}

// glibc's set_regs: fill the registers for the match [s, e], arriving
// at end node last.
//
// With backtracking, the walk's path from a state after a character
// depends on that state alone, and it cannot come back to it (it would
// loop). Reaching it again means its search ended before, so the walk
// backtracks as that search last did, from a dead end or from an end
// with an unclosed group, which succeeds when nothing is left to try.
static b32 rxwalk(RxCtx *c, u32 *v, i32 last, iz s, iz e, RxMatch *regs, Arena scratch)
{
    Regex *re = c->re;
    i32    ng = re->nsub + 1;
    RxWalk w  = {};
    w.c    = c;
    w.v    = v;
    w.s    = s;
    w.e    = e;
    w.regs = regs;
    w.prev = alloc<RxMatch>(&scratch, ng);
    w.eps  = alloc<u32>(&scratch, c->words);
    w.idx  = s;
    w.fs   = re->plural && re->backrefs;
    w.seen = newmemo(c, &scratch, 2 + 4*ng, s, e-s);
    for (i32 i = 0; i < ng; i++) regs[i].so = regs[i].eo = -1;
    regs[0].so = s;
    regs[0].eo = e;
    for (i32 i = 0; i < ng; i++) w.prev[i] = regs[i];

    i32 cur = re->start;
    while (w.idx <= e) {
        if (!rxspend(c)) return 0;
        rxupdate(re, regs, w.prev, cur, w.idx);
        if ((w.idx==e && cur==last) || (w.fs && rxbit(w.eps, cur))) {
            i32 next = -1;
            if (w.fs) {
                for (i32 i = 0; i < ng; i++) {
                    if (regs[i].so>-1 && regs[i].eo==-1) {
                        next = rxfail(&w, RS_UNCLOSED, regs);
                        break;
                    }
                }
            }
            if (next < 0) return 1;
            cur = next;
        }
        for (;;) {
            iz      at = w.idx;
            RxSeen *st = 0;
            cur = rxproceed(&w, cur);
            if (cur>=0 && w.fs && w.idx>at) st = rxended(&w, cur);
            if (cur<0 || (st && st->how==RS_DEAD)) {
                cur = rxfail(&w, RS_DEAD, 0);
                if (cur < 0) return 0;
            } else if (st) {
                cur = rxfail(&w, RS_UNCLOSED, st->regs);
                if (cur < 0) {
                    copybytes(regs, st->regs, ng*(iz)sizeof(RxMatch));
                    return 1;
                }
                continue;
            }
            break;
        }
    }
    return 1;
}

struct RxFrame {
    RxFrame *next;
    RxMatch *regs;
    u8      *usable;    // whether each group's span may be referred to
    u32     *eps;
    iz       idx;
    iz       quirk;     // the last position a newline quirk let through
    i32      node;
    i32      steps;     // epsilon moves since the last character
    b32      afterref;  // a back-reference arrived here
};

// Records kept once each, indexed by a 4-ary hash trie
struct RxEntSet {
    RxEntSet *child[4];
    i32       i;
};

struct RxLong {
    iz             best;  // the longest end, or -1
    i32            last;  // the lowest-numbered end node reaching it
    Slice<RxEntry> ents;  // the back-references' spans glibc finds, the
                          // end nodes reached (len -1), and those whose
                          // context fails (len -2)
    RxEntSet      *set;
};

static void rxrecord(RxLong *r, RxMemo *seen, Arena *a, RxEntry en)
{
    u64       h = (u64)en.idx<<32 ^ (u64)(u32)en.node<<12 ^ (u32)en.len;
    RxEntSet **e = &r->set;
    for (h *= 1111111111111111111u; *e; h <<= 2) {
        RxEntry *o = r->ents.data + (*e)->i;
        if (o->idx==en.idx && o->node==en.node && o->len==en.len) return;
        e = &(*e)->child[h>>62];
    }
    *e = alloc<RxEntSet>(rxframes(seen, (iz)sizeof(RxEntSet)+16));
    (*e)->i = (i32)r->ents.len;
    r->ents = push(a, r->ents, en);
}

// The lowest-numbered end node a search from rxlongest reached at e,
// or -1. glibc's retry of shorter ends takes a state for a final one
// when it holds an end node at all, even one whose context fails, and
// then sifts from node 0, its check_halt_state_context's answer for
// none: \>(^|a)\1$|(a|b)$\>. matches empty on "a\n  ".
static i32 rxendat(Slice<RxEntry> ents, iz e)
{
    i32 last = -1;
    b32 halt = 0;
    for (iz i = 0; i < ents.len; i++) {
        RxEntry en = ents[i];
        if (en.idx != e) continue;
        halt |= en.len == -2;
        if (en.len==-1 && (last<0 || en.node<last)) last = en.node;
    }
    return last<0 && halt ? 0 : last;
}

// With back-references: the ends of a match from s, and the spans
// glibc's get_subexp finds for back-references on the way. Every path is
// searched, as glibc's automaton does: a back-reference matches what its
// group last spanned, with no undoing of empty optional groups, and a
// fork passed again at the same position goes its other way. The
// results live in perm.
//
// glibc's spans come from its own search between a group's open and
// close nodes and on to the back-reference, which differs from its
// automaton in two ways. It looks for the lowest-numbered close node of
// the group in a state, so it cannot see spans closed by a copy (from
// an interval or an anchor) at a position where a lower-numbered close
// node of the group is in the state: (a?){2}\1 never matches. With
// closes, a first search records the close nodes in the state at each
// position from s (record), and a second one refers only to spans
// glibc finds. And it takes the context of each position as it is, so
// a newline before a ^ or after a $ is no line boundary there: a span
// cannot be used when the way from its group's start to the
// back-reference went through such an anchor (.^(a)\1 on "\naa").
// Where a back-reference arrives, empty or not, glibc builds the state
// from that context too, so a newline the match consumed no longer
// counts as a line start for a ^ after it (.()\1^ fails on "\n").
//
// Its initial state also takes each back-reference whose group closes
// in it to match empty, set or not: in (|^())\2 the empty alternative
// reaches the end through \2 (see re->init).
static RxLong rxlongest(RxCtx *c, iz s, u32 *closes, b32 record, Arena *perm)
{
    Regex   *re    = c->re;
    i32      ng    = re->nsub + 1;
    i32      nw    = c->words;
    RxLong   r     = {};
    RxFrame *stack = 0;
    RxFrame *free  = 0;
    Arena    scratch = *perm;
    r.best = r.last = -1;

    RxFrame *f = alloc<RxFrame>(&scratch);
    f->regs   = alloc<RxMatch>(&scratch, ng);
    f->usable = alloc<u8>(&scratch, ng);
    f->eps    = alloc<u32>(&scratch, nw);
    for (i32 i = 0; i < ng; i++) f->regs[i].so = f->regs[i].eo = -1;
    f->idx   = s;
    f->quirk = -1;
    f->node  = re->start;
    stack    = f;

    RxMemo seen = newmemo(c, &scratch, 3 + 3*re->nrefs, s, c->s.len-s);
    iz     size = (iz)sizeof(RxFrame) + ng*(iz)(sizeof(RxMatch)+1) + nw*4 + 32;

    while (stack) {
        f = stack;
        stack = f->next;
        RxMatch *regs  = f->regs;
        u8      *ok    = f->usable;
        u32     *eps   = f->eps;
        iz       idx   = f->idx;
        iz       quirk = f->quirk;
        i32      cur   = f->node;
        i32      steps = f->steps;
        b32      ref   = f->afterref;
        for (;;) {
            if (!rxspend(c)) return r;
            RxNode *x = re->nodes + cur;
            b32     g = !x->cons || rxprev(c, x->cons, idx, ref ? idx : s);
            if (!g && !rxeps(x->type)) {  // not in the state
                if (x->type==RT_END && !record) {
                    rxrecord(&r, &seen, &scratch, {idx, cur, -2});
                }
                b32 grow = x->type==RT_BACKREF && idx==s && re->initclose[x->arg];
                if (!grow || rxbit(eps, cur)) break;
                // glibc grows its initial state before its context
                // filters it, for an end node there to make it final
                rxset(eps, cur);
                cur = x->e1;
                continue;
            }
            if (g && x->cons && !rxprev(c, x->cons, idx, idx)) quirk = idx;
            if (x->type == RT_OPEN) {
                regs[x->arg+1].so = idx;
                regs[x->arg+1].eo = -1;
            } else if (x->type == RT_CLOSE) {
                regs[x->arg+1].eo = idx;
                u32 *at = closes ? closes + (idx-s)*nw : 0;
                if (at && record) {
                    if (g) rxset(at, cur);
                } else if (at) {
                    i32 low = 0;
                    while (low<cur && (!rxbit(at, low) || re->nodes[low].type!=RT_CLOSE ||
                           re->nodes[low].arg!=x->arg)) low++;
                    ok[x->arg+1] = low == cur;
                }
            }
            if (x->type == RT_END) {
                if (rxnext(c, x->cons, idx, 0)) {
                    if (!record) rxrecord(&r, &seen, &scratch, {idx, cur, -1});
                    if (idx >= r.best) {
                        r.last = idx>r.best || cur<r.last ? cur : r.last;
                        r.best = idx;
                    }
                } else if (!record) {
                    rxrecord(&r, &seen, &scratch, {idx, cur, -2});
                }
                break;
            }
            if (rxeps(x->type)) {
                if (++steps > 4*re->nnodes) break;
                rxset(eps, cur);
                if (x->e2 < 0) {
                    cur = x->e1;
                } else if (rxbit(eps, x->e1)) {
                    cur = x->e2;
                } else {
                    RxFrame *h = free;
                    if (h) {
                        free = h->next;
                    } else {
                        Arena *a = rxframes(&seen, size);
                        h = alloc<RxFrame>(a);
                        h->regs   = alloc<RxMatch>(a, ng);
                        h->usable = alloc<u8>(a, ng);
                        h->eps    = alloc<u32>(a, nw);
                    }
                    copybytes(h->regs, regs, ng*(iz)sizeof(RxMatch));
                    copybytes(h->usable, ok, ng);
                    copybytes(h->eps, eps, nw*(iz)sizeof(u32));
                    h->idx      = idx;
                    h->quirk    = quirk;
                    h->node     = x->e2;
                    h->steps    = steps;
                    h->afterref = ref;
                    h->next     = stack;
                    stack       = h;
                    cur = x->e1;
                }
                continue;
            }
            iz k = 0;
            if (x->type == RT_BACKREF) {
                i32     gi  = x->arg + 1;
                RxMatch m   = regs[gi];
                b32     use = m.so>=0 && m.eo>=0 && quirk<m.so &&
                              (!closes || record || ok[gi]) &&
                              rxnext(c, x->cons, idx, 0) &&
                              rxsame(c, m.so, idx, m.eo-m.so);
                if (!use && !(idx==s && re->initclose[x->arg])) break;
                if (!use) {
                    // Without a span glibc's search for later ones cannot
                    // pass here, so no group so far can be referred to:
                    // ^|(\B|)(^)\2.(\B|)\1. matches empty on "ab"
                    for (i32 i = 1; i < ng; i++) regs[i].so = regs[i].eo = -1;
                }
                k = use ? m.eo-m.so : 0;
                if (use && !record) rxrecord(&r, &seen, &scratch, {idx, cur, (i32)k});
                if (!k) {
                    if (rxbit(eps, cur)) break;
                    rxset(eps, cur);
                    ref |= use;
                    cur = x->e1;
                    continue;
                }
                ref = 1;
            } else {
                k = rxaccept(c, x, idx);
                if (!k || !rxnext(c, x->cons, idx, 1)) break;
                if (x->cons && !rxnext(c, x->cons, idx, 0)) quirk = idx;
                ref = 0;
            }
            idx  += k;
            steps = 0;
            for (i32 i = 0; i < nw; i++) eps[i] = 0;
            cur = x->next;

            if (!rxagain(&seen, cur, idx)) continue;

            // Only the registers back-references read matter ahead,
            // and whether they may: whether a quirk came since each
            // group opened, or is right here
            i32 *key = seen.key;
            key[0] = cur;
            key[1] = (i32)idx;
            key[2] = ref | (quirk==idx)<<1;
            for (i32 i = 0; i < re->nrefs; i++) {
                i32 g = re->refs[i] + 1;
                key[3+3*i] = (i32)regs[g].so;
                key[4+3*i] = (i32)regs[g].eo;
                key[5+3*i] = (closes && !record && ok[g]) | (quirk>=regs[g].so)<<1;
            }
            b32 old = 0;
            rxvisit(&seen, &old);
            if (old) break;
        }
        f->next = free;
        free    = f;
    }

    // The records to the front of perm
    r.set = 0;
    iz    len = r.ents.len * (iz)sizeof(RxEntry);
    byte *dst = perm->beg + (-(uz)perm->beg & (alignof(RxEntry)-1));
    if (len) __builtin_memmove(dst, r.ents.data, (uz)len);
    r.ents.data = (RxEntry *)dst;
    r.ents.cap  = r.ents.len;
    perm->beg   = dst + len;
    return r;
}

// Match s against re: 1 with m[0..nsub] filled for a match (-1 for an
// unset group), 0 for none, -1 when budget steps (back-references and
// submatches) ran out; a negative budget has no limit.
static i32 rxexec(Regex *re, Str s, RxMatch *m, Arena scratch, i64 budget = -1)
{
    RxCtx c = {};
    c.re     = re;
    c.s      = s;
    c.budget = budget;
    c.words  = (re->nnodes + 31) / 32;
    if (re->backrefs && s.len<0x7fffffff) {
        // Up to 256 MiB, and half of what remains
        iz avail = (scratch.mem ? scratch.mem->limit : scratch.end) - scratch.beg;
        c.memomax = (iz)1 << 28;
        c.memomax = c.memomax<avail/2 ? c.memomax : avail/2;
    }
    if ((re->flags & REX_UTF8) && !allascii(s)) {
        c.ulen = alloc<u8>(&scratch, s.len);
        for (iz i = 0; i < s.len;) {
            iz n = glibclen(s, i);
            c.ulen[i] = n ? (u8)n : 0xff;
            i += n ? n : 1;
        }
    }

    for (iz from = 0; from <= s.len;) {
        Arena  base = scratch;  // to drop a rejected start's spans
        iz     b    = -1;
        iz     e    = -1;
        i32    last = -1;
        RxLong lg   = {};
        if (!re->backrefs) {
            RxMatch r = rxpike(&c, from, scratch);
            b = r.so;
            e = r.eo;
        } else {
            for (iz p = from; p <= s.len && b<0; p++) {
                if (!rxstart(&c, p)) continue;
                u32  *closes = 0;
                Arena t      = scratch;
                if (re->copies) {
                    closes = rxtable(&t, s.len-p+1, c.words);
                    Arena u = t;
                    rxlongest(&c, p, closes, 1, &u);
                }
                lg = rxlongest(&c, p, closes, 0, &t);
                if (c.gaveup) return -1;
                if (lg.best >= 0) {
                    b       = p;
                    e       = lg.best;
                    last    = lg.last;
                    scratch = t;  // keep the spans
                }
            }
        }
        if (b < 0) return 0;
        iz shift = e==b ? rxstall(&c, b, scratch) : 0;

        if (re->nsub) {
            // Like glibc, give up a start whose match sifting rejects,
            // and look for a later one; with back-references it first
            // tries the shorter ends its automaton reached. A sifted
            // state at the start without the start node fails the walk.
            // The automaton's states (in) are those of the longest end,
            // and hold for the shorter ones.
            Arena t = scratch;
            if (!re->backrefs) last = rxlastnode(&c, b, e, t);
            u32 *v  = 0;
            u32 *in = re->backrefs ? rxreach(&c, b, e, lg.ents, &t) : 0;
            if (re->plural || re->backrefs) {
                for (;;) {
                    Arena u = t;
                    v = rxsift(&c, last, b, e, lg.ents, in, &u);
                    b32 found = rxbit(v, re->start);
                    for (i32 i = 0; re->backrefs && i<c.words; i++) found |= v[i] != 0;
                    if (found) {
                        t = u;
                        break;
                    }
                    v = 0;
                    if (!re->backrefs) break;
                    do {
                        last = --e<b ? -1 : rxendat(lg.ents, e);
                    } while (e>=b && last<0);
                    if (e < b) break;
                }
                if (!v) {
                    scratch = base;
                    from = b + shift + 1;
                    continue;
                }
            }
            b32 ok = rxwalk(&c, v, last, b, e, m, t);
            if (c.gaveup) return -1;
            if (!ok) return 0;
            for (i32 i = 1; i <= re->nsub; i++) {
                // glibc offsets an unclosed group's -1 by the match start
                if (m[i].so!=-1 && m[i].eo==-1) m[i].eo = b - 1;
            }
            for (i32 i = 0; i < re->nsub; i++) {
                if (re->map[i] != i) m[i+1] = m[re->map[i]+1];
            }
        } else {
            m[0].so = b;
            m[0].eo = e;
        }
        for (i32 i = 0; i <= re->nsub; i++) {
            if (m[i].so != -1) {
                m[i].so += shift;
                m[i].eo += shift;
            }
        }
        return 1;
    }
    return 0;
}
