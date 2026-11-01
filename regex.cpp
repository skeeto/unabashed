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
// Back-references make the language irregular. With them, the longest
// end for each start is found by exhaustive search, and the walk
// backtracks as glibc's does with its fail stack.
//
// In a UTF-8 locale, . and brackets consume whole characters, invalid
// bytes match only themselves, and matches start only at character
// boundaries, as in glibc. Classes, word characters and case folding
// are ASCII-only there, a known gap: glibc asks iswctype and towupper,
// so [[:alpha:]] and \w match é in glibc and not here.
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

// Anchors, by what the characters on either side must be
enum : i32 {
    RA_BEGBUF,       // ^ and \`
    RA_ENDBUF,       // $ and \'
    RA_WORDFIRST,    // \<
    RA_WORDLAST,     // \>
    RA_INWORD,       // \B, between two word characters
    RA_INNOTWORD,    // \B, between two others
    RA_WORDDELIM,    // \b: parsed into WORDFIRST or WORDLAST
    RA_NOTWORDDELIM, // \B: INWORD or INNOTWORD
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
    b32            neg;
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
};

// An automaton node: consuming nodes go on to next; anchors, group
// marks and back-references (when empty) too; forks to e1 or e2, the
// lower-numbered first (e2 is -1 when both are the same node).
struct RxNode {
    RxSet *set;
    i32    type;
    i32    arg;
    i32    next;
    i32    e1;
    i32    e2;
    b32    opt;
};

struct Regex {
    RxNode *nodes;
    i32    *preds;     // epsilon predecessors of node i: preds[predbeg[i]..predbeg[i+1])
    i32    *predbeg;
    i32    *consume;   // the consuming nodes
    i32    *map;       // group i reports group map[i] (merged groups)
    i32     nnodes;
    i32     nconsume;
    i32     start;
    i32     end;
    i32     nsub;
    i32     flags;
    b32     backrefs;
    b32     plural;    // any fork
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
        case '`':  t.type = TK_ANCHOR; t.c = RA_BEGBUF;       break;
        case '\'': t.type = TK_ANCHOR; t.c = RA_ENDBUF;       break;
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
    case '^': t.type = TK_ANCHOR; t.c = RA_BEGBUF; break;
    case '$': t.type = TK_ANCHOR; t.c = RA_ENDBUF; break;
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
    Slice<Rt *> nodes = rtnodes(t, p->a);
    if (nodes.len > RX_MAXTREE-p->ntree) {
        p->err = RXE_ESPACE;
        return t;
    }
    p->ntree += (i32)nodes.len;
    for (iz i = 0; i < nodes.len; i++) {
        Rt *c = alloc<Rt>(p->a);
        *c = *nodes[i];
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
    if (p->flags & REX_UTF8) {
        for (i32 i = 4; i < 8; i++) s->bits[i] = 0;
    }
}

// \w \W \s \S
static Rt *rxclassop(RxParse *p, i32 type)
{
    RxSet *s = alloc<RxSet>(p->a);
    if (type==TK_WORD || type==TK_NOTWORD) {
        rxaddclass(s, PCC_ALNUM);
        rxsetbit(s, '_');
    } else {
        rxaddclass(s, PCC_SPACE);
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
        RxRange r = {e.c, e.c};
        s->ranges = push(p->a, s->ranges, r);
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
    if (p->ntree > RX_MAXTREE) p->err = RXE_ESPACE;
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

    Regex *re = alloc<Regex>(a);
    re->nodes    = alloc<RxNode>(a, count);
    re->nnodes   = count;
    re->start    = root->first->idx;
    re->end      = end->idx;
    re->nsub     = p.nsub;
    re->map      = map;
    re->flags    = flags;
    re->backrefs = p.nbackref > 0;
    re->predbeg  = alloc<i32>(a, count+1);
    re->consume  = alloc<i32>(a, count);
    for (iz i = 0; i < nodes.len; i++) {
        Rt *n = nodes[i];
        if (n->type == RT_CONCAT) continue;
        RxNode *x = re->nodes + n->idx;
        x->type = n->type;
        x->arg  = n->arg;
        x->set  = n->set;
        x->opt  = n->opt;
        x->next = n->next ? n->next->idx : -1;
        x->e1   = x->e2 = -1;
        if (n->type==RT_ALT || n->type==RT_STAR) {
            i32 l = n->left  ? n->left->first->idx  : n->next->idx;
            i32 r = n->right ? n->right->first->idx : n->next->idx;
            x->e1 = l<r ? l : r;
            x->e2 = l==r ? -1 : l<r ? r : l;
            re->plural = 1;
        }
    }

    // Epsilon predecessors, for sifting
    for (i32 i = 0; i < count; i++) {
        RxNode *x = re->nodes + i;
        if (x->type==RT_ALT || x->type==RT_STAR) {
            re->predbeg[x->e1]++;
            if (x->e2 >= 0) re->predbeg[x->e2]++;
        } else if (rxeps(x->type) || x->type==RT_BACKREF) {
            re->predbeg[x->next]++;
        }
        if (x->type <= RT_BACKREF) re->consume[re->nconsume++] = i;
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
        RxNode *x = re->nodes + i;
        i32 d[2] = {-1, -1};
        if (x->type==RT_ALT || x->type==RT_STAR) {
            d[0] = x->e1;
            d[1] = x->e2;
        } else if (rxeps(x->type) || x->type==RT_BACKREF) {
            d[0] = x->next;
        }
        for (i32 k = 0; k < 2; k++) {
            if (d[k] >= 0) re->preds[re->predbeg[d[k]] + fill[d[k]]++] = i;
        }
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
};

static u8 rxtr(RxCtx *c, u8 b)
{
    return (c->re->flags & REX_ICASE) ? rxupper(b) : b;
}

static b32 rxwordbyte(u8 b)
{
    return (b>='0' && b<='9') || (b>='a' && b<='z') || (b>='A' && b<='Z') || b=='_';
}

static b32 rxanchor(RxCtx *c, i32 kind, iz p)
{
    iz  n  = c->s.len;
    b32 pw = p>0 && rxwordbyte(c->s[p-1]);
    b32 nw = p<n && rxwordbyte(c->s[p]);
    switch (kind) {
    case RA_BEGBUF:    return p == 0;
    case RA_ENDBUF:    return p == n;
    case RA_WORDFIRST: return !pw && nw;
    case RA_WORDLAST:  return pw && !nw;
    case RA_INWORD:    return pw && nw;
    case RA_INNOTWORD: return !pw && !nw;
    }
    return 0;
}

// How many bytes a consuming node takes at p, or 0 if it cannot.
static iz rxaccept(RxCtx *c, RxNode *x, iz p)
{
    if (p >= c->s.len) return 0;
    u8 b = c->s[p];
    u8 n = c->ulen ? c->ulen[p] : 1;
    switch (x->type) {
    case RT_CHAR:
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
        b32 in = 0;
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
    iz  *start;
    i32  len;
};

static b32 rxhas(RxList *l, i32 x)
{
    i32 i = l->sparse[x];
    return i<l->len && l->dense[i]==x;
}

// Add node x at p, reached from start st, with its epsilon closure. A
// node already there from a later start takes this one.
static void rxthread(RxCtx *c, RxList *l, i32 x, iz st, iz p, i32 *stack)
{
    RxNode *nodes = c->re->nodes;
    i32     top   = 0;
    stack[top++] = x;
    while (top) {
        x = stack[--top];
        if (rxhas(l, x)) {
            if (l->start[x] <= st) continue;
        } else {
            l->sparse[x] = l->len;
            l->dense[l->len++] = x;
        }
        l->start[x] = st;
        RxNode *n = nodes + x;
        switch (n->type) {
        case RT_ANCHOR:
            if (rxanchor(c, n->arg, p)) stack[top++] = n->next;
            break;
        case RT_OPEN:
        case RT_CLOSE:
            stack[top++] = n->next;
            break;
        case RT_ALT:
        case RT_STAR:
            stack[top++] = n->e1;
            if (n->e2 >= 0) stack[top++] = n->e2;
            break;
        }
    }
}

static b32 rxboundary(RxCtx *c, iz p)
{
    return !c->ulen || p==c->s.len || c->ulen[p];
}

// The leftmost-longest match, without back-references.
static RxMatch rxpike(RxCtx *c, Arena scratch)
{
    Regex  *re = c->re;
    i32     nn = re->nnodes;
    i32     nl = c->ulen ? 8 : 2;  // positions ahead: up to 6 bytes
    RxList  lists[8] = {};
    for (i32 i = 0; i < nl; i++) {
        lists[i].dense  = alloc<i32>(&scratch, nn);
        lists[i].sparse = alloc<i32>(&scratch, nn);
        lists[i].start  = alloc<iz>(&scratch, nn);
    }
    i32    *stack = alloc<i32>(&scratch, 2*nn+2);
    RxMatch best  = {-1, -1};
    iz      n     = c->s.len;
    for (iz p = 0; p <= n; p++) {
        RxList *l = lists + p%nl;
        if (best.so<0 && rxboundary(c, p)) {
            rxthread(c, l, re->start, p, p, stack);
        }
        if (rxhas(l, re->end)) {
            iz st = l->start[re->end];
            if (best.so<0 || st<=best.so) {
                best.so = st;
                best.eo = p;
            }
        }
        if (p < n) {
            for (i32 i = 0; i < l->len; i++) {
                i32     x  = l->dense[i];
                iz      st = l->start[x];
                RxNode *nd = re->nodes + x;
                if (nd->type>RT_ANY || (best.so>=0 && st>best.so)) continue;
                iz k = rxaccept(c, nd, p);
                if (k) rxthread(c, lists+(p+k)%nl, nd->next, st, p+k, stack);
            }
        }
        l->len = 0;
        if (best.so >= 0) {
            b32 live = 0;
            for (i32 i = 0; i < nl; i++) live |= lists[i].len > 0;
            if (!live) break;
        }
    }
    return best;
}

// For each position in [s, e], the nodes from which the match can still
// end at e: glibc's sifted states. A back-reference is taken to match
// any length, which is enough: the walk checks it.
static u32 *rxsift(RxCtx *c, iz s, iz e, Arena *a)
{
    Regex *re   = c->re;
    i32    w    = c->words;
    u32   *v    = alloc<u32>(a, (e-s+1)*w);
    u32   *any  = re->backrefs ? alloc<u32>(a, w) : 0;  // union of later sets
    i32   *work = alloc<i32>(a, re->nnodes);
    for (iz p = e; p >= s; p--) {
        u32 *cur = v + (p-s)*w;
        i32  top = 0;
        if (p == e) {
            rxset(cur, re->end);
            work[top++] = re->end;
        }
        for (i32 i = 0; i < re->nconsume; i++) {
            i32     x  = re->consume[i];
            RxNode *nd = re->nodes + x;
            b32     ok = 0;
            if (nd->type == RT_BACKREF) {
                ok = any && rxbit(any, nd->next);
            } else {
                iz k = rxaccept(c, nd, p);
                ok = k && p+k<=e && rxbit(v+(p+k-s)*w, nd->next);
            }
            if (ok && !rxbit(cur, x)) {
                rxset(cur, x);
                work[top++] = x;
            }
        }
        while (top) {
            i32 m = work[--top];
            for (i32 j = re->predbeg[m]; j < re->predbeg[m+1]; j++) {
                i32     q  = re->preds[j];
                RxNode *nq = re->nodes + q;
                if (rxbit(cur, q)) continue;
                if (nq->type==RT_ANCHOR && !rxanchor(c, nq->arg, p)) continue;
                rxset(cur, q);
                work[top++] = q;
            }
        }
        if (any) {
            for (i32 i = 0; i < w; i++) any[i] |= cur[i];
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
    b32      fs;     // backtracking
    Arena   *a;
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
        f = alloc<RxFail>(w->a);
        f->regs = alloc<RxMatch>(w->a, 2*ng);
        f->eps  = alloc<u32>(w->a, nw);
    }
    f->idx  = w->idx;
    f->node = node;
    copybytes(f->regs, w->regs, ng*(iz)sizeof(RxMatch));
    copybytes(f->regs+ng, w->prev, ng*(iz)sizeof(RxMatch));
    copybytes(f->eps, w->eps, nw*(iz)sizeof(u32));
    f->next  = w->fails;
    w->fails = f;
}

static i32 rxpop(RxWalk *w)
{
    RxFail *f = w->fails;
    if (!f) return -1;
    i32 ng = w->c->re->nsub + 1;
    w->fails = f->next;
    w->idx   = f->idx;
    copybytes(w->regs, f->regs, ng*(iz)sizeof(RxMatch));
    copybytes(w->prev, f->regs+ng, ng*(iz)sizeof(RxMatch));
    copybytes(w->eps, f->eps, w->c->words*(iz)sizeof(u32));
    f->next = w->free;
    w->free = f;
    return f->node;
}

static void rxclear(RxWalk *w)
{
    for (i32 i = 0; i < w->c->words; i++) w->eps[i] = 0;
}

// glibc's proceed_next_node: the node after x, or -1.
static i32 rxproceed(RxWalk *w, i32 x)
{
    RxCtx  *c  = w->c;
    Regex  *re = c->re;
    RxNode *n  = re->nodes + x;
    if (rxeps(n->type)) {
        rxset(w->eps, x);
        i32 d[2] = {n->next, -1};
        if (n->type==RT_ALT || n->type==RT_STAR) {
            d[0] = n->e1;
            d[1] = n->e2;
        }
        i32 dest = -1;
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
        if (!k) {
            rxset(w->eps, x);
            if (rxviable(w, w->idx, n->next)) return n->next;
            return -1;
        }
    } else {
        k = rxaccept(c, n, w->idx);
        if (!k) return -1;
    }
    iz idx = w->idx + k;
    if (w->fs && (idx>w->e || !rxviable(w, idx, n->next))) return -1;
    rxclear(w);
    w->idx = idx;
    return n->next;
}

// glibc's set_regs: fill the registers for the match [s, e].
static b32 rxwalk(RxCtx *c, u32 *v, iz s, iz e, RxMatch *regs, Arena scratch)
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
    w.a    = &scratch;
    for (i32 i = 0; i < ng; i++) regs[i].so = regs[i].eo = -1;
    regs[0].so = s;
    regs[0].eo = e;
    for (i32 i = 0; i < ng; i++) w.prev[i] = regs[i];

    i32 cur = re->start;
    while (w.idx <= e) {
        if (!rxspend(c)) return 0;
        rxupdate(re, regs, w.prev, cur, w.idx);
        if ((w.idx==e && cur==re->end) || (w.fs && rxbit(w.eps, cur))) {
            i32 next = -1;
            if (w.fs) {
                for (i32 i = 0; i < ng; i++) {
                    if (regs[i].so>-1 && regs[i].eo==-1) {
                        next = rxpop(&w);
                        break;
                    }
                }
            }
            if (next < 0) return 1;
            cur = next;
        }
        cur = rxproceed(&w, cur);
        if (cur < 0) {
            cur = rxpop(&w);
            if (cur < 0) return 0;
        }
    }
    return 1;
}

struct RxFrame {
    RxFrame *next;
    RxMatch *regs;  // registers, then the previous registers
    u32     *eps;
    iz       idx;
    i32      node;
};

// With back-references: the longest end of a match from s, or -1,
// searching every path, back-references compared against the registers
// set along it.
static iz rxlongest(RxCtx *c, iz s, Arena scratch)
{
    Regex   *re    = c->re;
    i32      ng    = re->nsub + 1;
    i32      nw    = c->words;
    iz       n     = c->s.len;
    iz       best  = -1;
    RxFrame *stack = 0;
    RxFrame *free  = 0;

    RxFrame *f = alloc<RxFrame>(&scratch);
    f->regs = alloc<RxMatch>(&scratch, 2*ng);
    f->eps  = alloc<u32>(&scratch, nw);
    for (i32 i = 0; i < 2*ng; i++) f->regs[i].so = f->regs[i].eo = -1;
    f->idx  = s;
    f->node = re->start;
    stack   = f;

    while (stack && best<n) {
        f = stack;
        stack = f->next;
        RxMatch *regs = f->regs;
        RxMatch *prev = f->regs + ng;
        u32     *eps  = f->eps;
        iz       idx  = f->idx;
        i32      cur  = f->node;
        for (;;) {
            if (!rxspend(c)) return best;
            rxupdate(re, regs, prev, cur, idx);
            if (cur == re->end) {
                best = idx>best ? idx : best;
                break;
            }
            RxNode *x = re->nodes + cur;
            if (rxeps(x->type)) {
                if (rxbit(eps, cur)) break;  // an empty loop
                rxset(eps, cur);
                if (x->type==RT_ANCHOR && !rxanchor(c, x->arg, idx)) break;
                if (x->type==RT_ALT || x->type==RT_STAR) {
                    if (x->e2 >= 0) {
                        RxFrame *g = free;
                        if (g) {
                            free = g->next;
                        } else {
                            g = alloc<RxFrame>(&scratch);
                            g->regs = alloc<RxMatch>(&scratch, 2*ng);
                            g->eps  = alloc<u32>(&scratch, nw);
                        }
                        copybytes(g->regs, regs, 2*ng*(iz)sizeof(RxMatch));
                        copybytes(g->eps, eps, nw*(iz)sizeof(u32));
                        g->idx  = idx;
                        g->node = x->e2;
                        g->next = stack;
                        stack   = g;
                    }
                    cur = x->e1;
                } else {
                    cur = x->next;
                }
                continue;
            }
            iz k = 0;
            if (x->type == RT_BACKREF) {
                RxMatch r = regs[x->arg+1];
                if (r.so<0 || r.eo<0) break;
                k = r.eo - r.so;
                if (!k) {
                    if (rxbit(eps, cur)) break;
                    rxset(eps, cur);
                    cur = x->next;
                    continue;
                }
                if (!rxsame(c, r.so, idx, k)) break;
            } else {
                k = rxaccept(c, x, idx);
                if (!k) break;
            }
            idx += k;
            for (i32 i = 0; i < nw; i++) eps[i] = 0;
            cur = x->next;
        }
        f->next = free;
        free    = f;
    }
    return best;
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
    if ((re->flags & REX_UTF8) && !allascii(s)) {
        c.ulen = alloc<u8>(&scratch, s.len);
        for (iz i = 0; i < s.len;) {
            iz n = glibclen(s, i);
            c.ulen[i] = n ? (u8)n : 0xff;
            i += n ? n : 1;
        }
    }

    iz b = -1;
    iz e = -1;
    if (!re->backrefs) {
        RxMatch r = rxpike(&c, scratch);
        b = r.so;
        e = r.eo;
    } else {
        for (iz p = 0; p <= s.len && b<0; p++) {
            if (!rxboundary(&c, p)) continue;
            e = rxlongest(&c, p, scratch);
            if (c.gaveup) return -1;
            if (e >= 0) b = p;
        }
    }
    if (b < 0) return 0;

    for (i32 i = 0; i <= re->nsub; i++) m[i].so = m[i].eo = -1;
    m[0].so = b;
    m[0].eo = e;
    if (re->nsub) {
        u32 *v = 0;
        if (re->plural || re->backrefs) v = rxsift(&c, b, e, &scratch);
        b32 ok = rxwalk(&c, v, b, e, m, scratch);
        if (c.gaveup) return -1;
        if (!ok) return 0;
        for (i32 i = 1; i <= re->nsub; i++) {
            // glibc offsets an unclosed group's -1 by the match start
            if (m[i].so!=-1 && m[i].eo==-1) m[i].eo = b - 1;
        }
        for (i32 i = 0; i < re->nsub; i++) {
            if (re->map[i] != i) m[i+1] = m[re->map[i]+1];
        }
    }
    return 1;
}
