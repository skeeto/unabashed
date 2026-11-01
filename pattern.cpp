// Shell pattern matching: globs, bracket expressions and extglob
//
// Patterns are in glob form: a backslash quotes the next character, and
// the core turns quoted characters into \c before matching. Matching
// follows bash 5.3, including globasciiranges and its handling of
// unterminated brackets and groups.
//
// Plain globs run on an iterative two-pointer matcher with no
// exponential backtracking. Patterns with extglob groups, and pathname
// matching, run on a set matcher: it carries the set of string positions
// reachable after each pattern node as a bitset, so that it is
// polynomial where bash backtracks. The set matcher reproduces bash's
// handling of a `*` followed by an extglob group (see evalrun), which
// differs from the plain regular semantics in a few corner cases.
// Where bash would backtrack, results are memoized (memoseq), so nested
// groups and runs of optional groups stay polynomial. Recursion is
// bounded (PAT_MAXNEST, PAT_MAXLEVEL), and patterns nested deeper than
// that never match, where bash would still try.
//
// With PM_UTF8, ? and brackets consume whole characters when both the
// pattern and the string are valid UTF-8 (glibc's definition: up to six
// bytes, no overlong forms, no surrogates); otherwise matching is
// byte-wise, as in bash. Character classes and case folding are
// ASCII-only, a known gap for non-ASCII letters.
//
// This is free and unencumbered software released into the public domain.

enum : i32 {
    PM_EXTGLOB  = 1<<0,  // ?(..) *(..) +(..) @(..) !(..)
    PM_NOCASE   = 1<<1,  // nocasematch, nocaseglob
    PM_PATHNAME = 1<<2,  // a slash is only matched by a literal slash
    PM_PERIOD   = 1<<3,  // a leading dot must be matched explicitly
    PM_UTF8     = 1<<4,  // UTF-8 locale
    PM_DOTDOT   = 1<<5,  // wildcards never match a leading . or .. (dotglob)
    PM_SKIPDOTS = 1<<6,  // globmatch: never match . or .. (globskipdots)
};

enum : i32 {
    PF_SHORTPREFIX,  // ${x#p}
    PF_LONGPREFIX,   // ${x##p}
    PF_SHORTSUFFIX,  // ${x%p}
    PF_LONGSUFFIX,   // ${x%%p}
    PF_FIRST,        // ${x/p/r}: leftmost start, longest match there
    PF_ANCHORSTART,  // ${x/#p/r}
    PF_ANCHOREND,    // ${x/%p/r}
};

struct PatSpan {
    iz  beg;
    iz  end;
    b32 ok;
};


// Characters

struct PatUnit {
    i32 c;
    iz  len;
};

// Length of the valid UTF-8 sequence at s[i], or 0 if it is invalid.
static iz glibclen(Str s, iz i)
{
    u8  c  = s[i];
    iz  n  = 0;
    u8  lo = 0x80;
    u8  hi = 0xbf;
    if (c < 0x80) {
        return 1;
    } else if (c < 0xc2) {
        return 0;
    } else if (c < 0xe0) {
        n = 1;
    } else if (c < 0xf0) {
        n  = 2;
        lo = c==0xe0 ? 0xa0 : lo;
        hi = c==0xed ? 0x9f : hi;
    } else if (c < 0xf8) {
        n  = 3;
        lo = c==0xf0 ? 0x90 : lo;
    } else if (c < 0xfc) {
        n  = 4;
        lo = c==0xf8 ? 0x88 : lo;
    } else if (c < 0xfe) {
        n  = 5;
        lo = c==0xfc ? 0x84 : lo;
    } else {
        return 0;
    }
    if (n > s.len-i-1 || s[i+1]<lo || s[i+1]>hi) {
        return 0;
    }
    for (iz k = 2; k <= n; k++) {
        if ((s[i+k]&0xc0) != 0x80) {
            return 0;
        }
    }
    return n + 1;
}

static b32 glibcvalid(Str s)
{
    for (iz i = 0, n; i < s.len; i += n) {
        n = glibclen(s, i);
        if (!n) return 0;
    }
    return 1;
}

static b32 allascii(Str s)
{
    for (iz i = 0; i < s.len; i++) {
        if (s[i] >= 0x80) return 0;
    }
    return 1;
}

// Decode the unit at s[i]: a byte, or in wide mode a whole character
// (the string is valid UTF-8 in wide mode).
static PatUnit unitat(Str s, iz i, b32 wide)
{
    PatUnit u = {s[i], 1};
    if (wide && u.c>=0x80) {
        u.len = glibclen(s, i);
        u.c  &= 0x7f >> u.len;
        for (iz k = 1; k < u.len; k++) {
            u.c = u.c<<6 | (s[i+k]&0x3f);
        }
    }
    return u;
}

static i32 patfold(i32 c, i32 flags)
{
    return (flags&PM_NOCASE) && c>='A' && c<='Z' ? c+32 : c;
}

enum : i32 {
    PCC_NONE, PCC_ALNUM, PCC_ALPHA, PCC_ASCII, PCC_BLANK, PCC_CNTRL, PCC_DIGIT,
    PCC_GRAPH, PCC_LOWER, PCC_PRINT, PCC_PUNCT, PCC_SPACE, PCC_UPPER, PCC_WORD,
    PCC_XDIGIT,
};

static Str ctypenames[] = {
    "", "alnum", "alpha", "ascii", "blank", "cntrl", "digit", "graph",
    "lower", "print", "punct", "space", "upper", "word", "xdigit",
};

// Class membership, ASCII only. bash in a UTF-8 locale classifies other
// characters with iswctype (so é is alpha and € is punct); that belongs
// here once the core has Unicode tables, along with case folding.
static b32 inctype(i32 c, i32 cc)
{
    b32 lower = c>='a' && c<='z';
    b32 upper = c>='A' && c<='Z';
    b32 dig   = c>='0' && c<='9';
    b32 graph = c>0x20 && c<0x7f;
    switch (cc) {
    case PCC_ALNUM:  return lower || upper || dig;
    case PCC_ALPHA:  return lower || upper;
    case PCC_ASCII:  return c>=0 && c<0x80;
    case PCC_BLANK:  return c==' ' || c=='\t';
    case PCC_CNTRL:  return (c>=0 && c<0x20) || c==0x7f;
    case PCC_DIGIT:  return dig;
    case PCC_GRAPH:  return graph;
    case PCC_LOWER:  return lower;
    case PCC_PRINT:  return graph || c==' ';
    case PCC_PUNCT:  return graph && !lower && !upper && !dig;
    case PCC_SPACE:  return c==' ' || (c>='\t' && c<='\r');
    case PCC_UPPER:  return upper;
    case PCC_WORD:   return lower || upper || dig || c=='_';
    case PCC_XDIGIT: return dig || (c>='a' && c<='f') || (c>='A' && c<='F');
    }
    return 0;
}

struct PatSym {
    Str name;
    i32 c;
};

// POSIX names for collating symbols, [.name.]
static PatSym collnames[] = {
    {"NUL",                   0}, {"SOH",                   1},
    {"STX",                   2}, {"ETX",                   3},
    {"EOT",                   4}, {"ENQ",                   5},
    {"ACK",                   6}, {"alert",                 7},
    {"BS",                    8}, {"backspace",             8},
    {"HT",                    9}, {"tab",                   9},
    {"LF",                   10}, {"newline",              10},
    {"VT",                   11}, {"vertical-tab",         11},
    {"FF",                   12}, {"form-feed",            12},
    {"CR",                   13}, {"carriage-return",      13},
    {"SO",                   14}, {"SI",                   15},
    {"DLE",                  16}, {"DC1",                  17},
    {"DC2",                  18}, {"DC3",                  19},
    {"DC4",                  20}, {"NAK",                  21},
    {"SYN",                  22}, {"ETB",                  23},
    {"CAN",                  24}, {"EM",                   25},
    {"SUB",                  26}, {"ESC",                  27},
    {"IS4",                  28}, {"FS",                   28},
    {"IS3",                  29}, {"GS",                   29},
    {"IS2",                  30}, {"RS",                   30},
    {"IS1",                  31}, {"US",                   31},
    {"space",               ' '}, {"exclamation-mark",    '!'},
    {"quotation-mark",      '"'}, {"number-sign",         '#'},
    {"dollar-sign",         '$'}, {"percent-sign",        '%'},
    {"ampersand",           '&'}, {"apostrophe",         '\''},
    {"left-parenthesis",    '('}, {"right-parenthesis",   ')'},
    {"asterisk",            '*'}, {"plus-sign",           '+'},
    {"comma",               ','}, {"hyphen",              '-'},
    {"hyphen-minus",        '-'}, {"minus",               '-'},
    {"dash",                '-'}, {"period",              '.'},
    {"full-stop",           '.'}, {"slash",               '/'},
    {"solidus",             '/'}, {"zero",                '0'},
    {"one",                 '1'}, {"two",                 '2'},
    {"three",               '3'}, {"four",                '4'},
    {"five",                '5'}, {"six",                 '6'},
    {"seven",               '7'}, {"eight",               '8'},
    {"nine",                '9'}, {"colon",               ':'},
    {"semicolon",           ';'}, {"less-than-sign",      '<'},
    {"equals-sign",         '='}, {"greater-than-sign",   '>'},
    {"question-mark",       '?'}, {"commercial-at",       '@'},
    {"left-square-bracket", '['}, {"backslash",          '\\'},
    {"reverse-solidus",    '\\'}, {"right-square-bracket",']'},
    {"circumflex",          '^'}, {"circumflex-accent",   '^'},
    {"underscore",          '_'}, {"grave-accent",        '`'},
    {"left-brace",          '{'}, {"left-curly-bracket",  '{'},
    {"vertical-line",       '|'}, {"right-brace",         '}'},
    {"right-curly-bracket", '}'}, {"tilde",               '~'},
    {"DEL",                127},
};


// Compiled patterns

enum : i32 { PE_CHAR, PE_RANGE, PE_CLASS, PE_EQUIV };

struct PatElem {
    i32 kind;
    i32 lo;     // character, range start or class
    i32 hi;     // range end
    b32 out;    // a character matching here first is not in the set
    iz  after;  // pattern index just after the element
};

// PN_FORK: alternatives that each hold the rest of the sequence, for a
// bracket whose extent depends on which element matched.
enum : i32 { PN_CHAR, PN_ANY, PN_STAR, PN_SET, PN_GROUP, PN_REST, PN_FORK };

enum : i32 {
    PN_ESC   = 1<<0,  // PN_CHAR: written with a backslash
    PN_TRAIL = 1<<1,  // PN_CHAR: a backslash ending the pattern
    PN_ENDNO = 1<<2,  // !(..) at the top: see endrule
};

struct PatSeq;

struct PatNode {
    i32      kind;
    i32      c;       // PN_CHAR: folded character; PN_GROUP, PN_REST: operator
    i32      flag;
    PatElem *elems;   // PN_SET
    iz       nelems;
    b32      neg;
    PatSeq  *alts;    // PN_GROUP, PN_FORK
    iz       nalts;
    Str      text;    // PN_REST: raw text compared literally
};

struct PatSeq {
    PatNode *nodes;
    iz       len;
};

// Limits that keep recursion within a small stack. bash itself handles
// deeper patterns, but nothing written by hand comes near them.
enum : i32 {
    PAT_MAXNEST  = 32,   // nesting of extglob groups
    PAT_MAXLEVEL = 128,  // nesting of evaluations at match time
};

enum : i32 { PM_NOENDRULE = 1<<30 };  // internal: see endrule

struct PatComp {
    Str  pat;
    i32  flags;
    b32  wide;
    b32  quirk;      // has a site where bash departs from regular semantics
    i32  nest;       // current group nesting
    b32  deep;       // nesting exceeded PAT_MAXNEST: never matches
    b32  memo;       // has both groups and stars: memoize evalseq
    iz   lastclose;  // index of the last ], or -1
    i32  forks;      // PN_FORK nodes made so far
};

static i32 peekc(PatComp *k, iz i, iz end)  // -1 at the end
{
    return i<end ? unitat(k->pat, i, k->wide).c : -1;
}

static iz nextu(PatComp *k, iz i, iz end)
{
    return i<end ? i+unitat(k->pat, i, k->wide).len : i+1;
}

// Find the end of a [.x.], [=x=] or [:x:] starting at the type
// character at i, returning the index of its closing type character.
static iz subbracket(PatComp *k, iz i, iz end)
{
    i32 type = peekc(k, i, end);
    for (iz j = nextu(k, i, end); j < end; j = nextu(k, j, end)) {
        i32 c = peekc(k, j, end);
        if ((c=='/' && (k->flags&PM_PATHNAME)) || (type!='.' && c==']')) {
            return -1;
        }
        if (c==type && peekc(k, j+1, end)==']') {
            return j;
        }
    }
    return -1;
}

static i32 collsym(PatComp *k, iz beg, iz end)
{
    Str name = span(k->pat.data+beg, k->pat.data+end);
    for (iz i = 0; i < countof(collnames); i++) {
        if (collnames[i].name == name) {
            return collnames[i].c;
        }
    }
    if (name.len && unitat(name, 0, k->wide).len==name.len) {
        return unitat(name, 0, k->wide).c;
    }
    return k->wide ? -1 : 0xff;  // bash's invalid symbol, as a byte
}

static i32 classid(PatComp *k, iz beg, iz end)
{
    u8 buf[16];
    iz len = 0;
    for (iz i = beg; i < end; i++) {
        if (k->pat[i] == '\\') {
            if (++i == end) break;
        }
        if (len == countof(buf)) return PCC_NONE;
        buf[len++] = k->pat[i];
    }
    Str name = {buf, len};
    for (i32 i = 1; i < countof(ctypenames); i++) {
        if (ctypenames[i] == name) {
            return i;
        }
    }
    return PCC_NONE;
}

struct PatBracket {
    Slice<PatElem> elems;
    iz             end;      // index after the closing bracket
    b32            neg;
    b32            ok;       // terminated
    b32            diverge;  // the extent depends on the matching element
};

// Parse a bracket expression from just after its opening bracket. An
// unterminated bracket (or one holding a slash under PM_PATHNAME) is
// a literal [ instead. Like bash, this scans to the end of the whole
// pattern, past the end of an extglob alternative if need be.
//
// Once an element matches, bash finds the end of the bracket with a
// simpler scan (skipscan) that disagrees with this parse in two places:
// a range ending in [ before [:x:] or [=x=], which the parse takes as a
// plain [, and a range ending in \[ before .x.], which the parse takes
// as a collating symbol. Such a bracket is noted as divergent.
static PatBracket parsebracket(PatComp *k, iz i, iz end, Arena *a)
{
    PatBracket r    = {};
    b32        path = k->flags & PM_PATHNAME;
    i32        c    = peekc(k, i, end);
    if (c=='!' || c=='^') {
        r.neg = 1;
        i++;
    }
    c = peekc(k, i, end);
    i = nextu(k, i, end);
    for (;;) {
        if (c < 0) return r;

        i32 t = peekc(k, i, end);
        if (c=='[' && (t=='=' || t==':')) {
            iz close = subbracket(k, i, end);
            if (close >= 0) {
                PatElem e = {};
                if (t == '=') {
                    e.kind = PE_EQUIV;
                    e.lo   = patfold(collsym(k, i+1, close), k->flags);
                } else {
                    e.kind = PE_CLASS;
                    e.lo   = classid(k, i+1, close);
                }
                i       = close + 2;
                e.after = i;
                r.elems = push(a, r.elems, e);
                c = peekc(k, i, end);
                i = nextu(k, i, end);
                if (c<0 || (c=='/' && path)) return r;
                if (c == ']') break;
                continue;
            }
        }

        i32 lo = c;
        if (c=='[' && t=='.') {
            iz close = subbracket(k, i, end);
            if (close >= 0) {
                lo = collsym(k, i+1, close);
                i  = close + 2;
            }
        } else if (c == '\\') {
            if (t<0 || (t=='/' && path)) return r;
            lo = t;
            i  = nextu(k, i, end);
        } else if (c=='/' && path) {
            return r;
        }

        iz after = i;
        c = peekc(k, i, end);
        i = nextu(k, i, end);
        if (c<0 || (c=='/' && path)) return r;
        if (c=='-' && peekc(k, i, end)!=']') {
            i32 hi  = peekc(k, i, end);
            b32 esc = hi == '\\';
            i = nextu(k, i, end);
            if (esc) {
                hi = peekc(k, i, end);
                i  = nextu(k, i, end);
            }
            if (hi<0 || (hi=='/' && path)) return r;
            i32 t2 = peekc(k, i, end);
            if (hi=='[' && t2=='.') {
                iz close = subbracket(k, i, end);
                if (close >= 0) {
                    hi = collsym(k, i+1, close);
                    i  = close + 2;
                    r.diverge |= esc;
                }
            } else if (hi=='[' && !esc && (t2==':' || t2=='=')) {
                r.diverge |= subbracket(k, i, end) >= 0;
            }
            after = i;
            c  = peekc(k, i, end);
            i  = nextu(k, i, end);
            lo = patfold(lo, k->flags);
            hi = patfold(hi, k->flags);
            if (lo <= hi) {  // an invalid symbol (-1) sorts first
                PatElem e = {PE_RANGE, lo, hi, 0, after};
                r.elems = push(a, r.elems, e);
            }
        } else {
            PatElem e = {PE_CHAR, patfold(lo, k->flags), 0, 0, after};
            r.elems = push(a, r.elems, e);
        }
        if (c == ']') break;
    }
    r.end = i;
    r.ok  = 1;
    return r;
}

static b32 elemmatch(PatElem *e, i32 c, i32 flags)
{
    i32 t = patfold(c, flags);
    switch (e->kind) {
    case PE_CHAR:
    case PE_EQUIV: return t == e->lo;
    case PE_RANGE: return t>=e->lo && t<=e->hi;
    case PE_CLASS: return inctype(c, e->lo);
    }
    return 0;
}

// The first element that matches decides, as in bash.
static b32 inbracket(PatNode *nd, i32 c, i32 flags)
{
    for (iz i = 0; i < nd->nelems; i++) {
        PatElem *e = nd->elems + i;
        if (elemmatch(e, c, flags)) return e->out == nd->neg;
    }
    return nd->neg;
}

// Where bash's scan for the end of a bracket after a match, starting at
// p, finds the closing bracket: the index after it, or -1 if the bracket
// is unterminated that way.
static iz skipscan(PatComp *k, iz p, iz end)
{
    b32 path = k->flags & PM_PATHNAME;
    for (;;) {
        i32 c = peekc(k, p, end);
        if (c<0 || (c=='/' && path)) return -1;
        p = nextu(k, p, end);
        i32 t = peekc(k, p, end);
        if (c=='[' && (t=='=' || t==':' || t=='.')) {
            iz close = subbracket(k, p, end);
            p = close<0 ? p : close+2;
        } else if (c == ']') {
            return p;
        } else if (c == '\\') {
            if (t<0 || (t=='/' && path)) return -1;
            p = nextu(k, p, end);
        }
    }
}

static b32 isextop(i32 c)
{
    return c=='?' || c=='*' || c=='+' || c=='@' || c=='!';
}

// Scan for the end of an extglob group body (delim 0) or alternative
// (delim '|'), tracking nested groups and brackets the way bash does.
// Returns the index after the delimiter, or -1.
static iz patscan(PatComp *k, iz i, iz end, i32 delim)
{
    i32 pnest  = 0;
    i32 bnest  = 0;
    iz  bfirst = -1;
    b32 skip   = 0;
    if (i == end) {
        return -1;
    }
    for (iz j = i; j < end; j++) {
        u8 c = k->pat[j];
        if (skip) {
            skip = 0;
            continue;
        }
        switch (c) {
        case '\\':
            skip = 1;
            break;
        case '[':
            if (!bnest) {
                bfirst = j + 1;
                if (bfirst<end && (k->pat[bfirst]=='!' || k->pat[bfirst]=='^')) {
                    bfirst++;
                }
                bnest++;
            } else if (j+1<end && (k->pat[j+1]==':' || k->pat[j+1]=='.' ||
                                   k->pat[j+1]=='=')) {
                iz close = subbracket(k, j+1, k->pat.len);
                if (close >= 0) {
                    j = close + 1;
                }
            }
            break;
        case ']':
            if (bnest && j!=bfirst) {
                bnest--;
                bfirst = -1;
            }
            break;
        case '(':
            if (!bnest) pnest++;
            break;
        case ')':
            if (!bnest && pnest--<=0) return j + 1;
            break;
        case '|':
            if (!bnest && !pnest && delim=='|') return j + 1;
            break;
        }
    }
    return -1;
}

static b32 seqnullable(PatSeq q, iz j)
{
    for (; j < q.len; j++) {
        PatNode *nd = q.nodes + j;
        switch (nd->kind) {
        case PN_STAR:
            break;
        case PN_GROUP:
            if (nd->c=='@' || nd->c=='+') {
                b32 any = 0;
                for (iz i = 0; i < nd->nalts; i++) {
                    any |= seqnullable(nd->alts[i], 0);
                }
                if (!any) return 0;
            }
            break;
        default:
            return 0;
        }
    }
    return 1;
}

static b32 runelem(PatNode *nd)
{
    switch (nd->kind) {
    case PN_STAR:
    case PN_ANY:   return 1;
    case PN_GROUP:
    case PN_REST:  return nd->c=='?' || nd->c=='*';
    }
    return 0;
}

// Note where bash's handling of a `*` run departs from regular
// semantics: a run followed by a group that could match the empty
// string, or by a negation, or by a trailing backslash, or a run
// holding an unclosed group.
static void findquirks(PatComp *k, PatSeq q)
{
    for (iz i = 0; i < q.len; i++) {
        if (q.nodes[i].kind != PN_STAR) continue;
        iz j = i + 1;
        for (; j<q.len && runelem(q.nodes+j); j++) {
            if (q.nodes[j].kind == PN_REST) k->quirk = 1;
        }
        if (j == q.len) continue;
        PatNode *r1 = q.nodes + j;
        if (r1->kind==PN_CHAR && (r1->flag&PN_TRAIL)) {
            k->quirk = 1;
        } else if (r1->kind == PN_REST) {
            k->quirk |= r1->c == '!';
        } else if (r1->kind == PN_GROUP) {
            k->quirk |= r1->c=='!' || seqnullable(q, j);
        }
        i = j - 1;
    }
}

static PatSeq compileseq(PatComp *, iz, iz, Arena *);
static b32    patmatch(Str, Str, i32, Arena);

// Does a star run that reaches the end of the string, just before the
// top-level negation at bang, match? bash rescans the group from the !
// instead of the (, so the group seems to extend to the first unbalanced
// ) after it. With none, the run matches. Otherwise it matches unless no
// alternative of the rescanned group matches the empty string and the
// rest after it does. The checks themselves leave this rule out.
static b32 endrule(PatComp *k, iz bang, Arena scratch)
{
    iz rest = patscan(k, bang, k->pat.len, 0);
    if (rest<0 || (k->flags&PM_NOENDRULE)) {
        return 1;
    }
    i32 flags = k->flags | PM_NOENDRULE;
    for (iz sub = bang+1;;) {
        iz next = patscan(k, sub, k->pat.len, '|');
        next = next<0 ? rest : next;
        Str alt = span(k->pat.data+sub, k->pat.data+next-1);
        if (patmatch(alt, "", flags, scratch)) return 1;
        if (next >= rest) break;
        sub = next;
    }
    return !patmatch(cuthead(k->pat, rest), "", flags, scratch);
}

// PN_ENDNO for a top-level negation at i ending a star run, per endrule.
static i32 endflag(PatComp *k, Slice<PatNode> q, i32 op, iz i, Arena scratch)
{
    if (op!='!' || k->nest) {
        return 0;
    }
    iz j = q.len - 1;
    for (; j>=0 && runelem(q.data+j) && q.data[j].kind!=PN_STAR; j--) {}
    return j>=0 && q.data[j].kind==PN_STAR && !endrule(k, i, scratch) ? PN_ENDNO : 0;
}

// A sequence of the node nd followed by the compiled pattern from i to
// end. Past the end of an alternative, bash's matcher stops: nothing
// follows.
static PatSeq prefixseq(PatComp *k, PatNode nd, iz i, iz end, Arena *a)
{
    PatSeq rest = i<=end ? compileseq(k, i, end, a) : PatSeq{};
    PatSeq r    = {alloc<PatNode>(a, rest.len+1), rest.len+1};
    r.nodes[0] = nd;
    for (iz j = 0; j < rest.len; j++) r.nodes[j+1] = rest.nodes[j];
    return r;
}

// A divergent bracket starting at i becomes a fork with an alternative
// for each place it may end, each a bracket matching the characters
// whose first matching element ends there, then the rest of the
// sequence. Where the scan finds no end, only [ matches, literally.
static PatNode forkbracket(PatComp *k, iz i, iz end, PatBracket b, Arena *a)
{
    k->forks++;
    k->quirk = 1;  // a pattern with a fork is never reversed
    iz  n    = b.elems.len;
    iz *ends = alloc<iz>(a, n);
    for (iz e = 0; e < n; e++) {
        ends[e] = skipscan(k, b.elems.data[e].after, k->pat.len);
    }

    Slice<PatSeq> alts = {};
    for (iz e = 0; e < n; e++) {
        iz   v    = ends[e];
        b32  seen = v < 0;
        iz   last = e;
        for (iz j = 0; j < e; j++) seen |= ends[j] == v;
        if (seen) continue;
        for (iz j = e; j < n; j++) last = ends[j]==v ? j : last;
        PatNode set = {};
        set.kind   = PN_SET;
        set.elems  = alloc<PatElem>(a, last+1);
        set.nelems = last + 1;
        for (iz j = 0; j <= last; j++) {
            set.elems[j]     = b.elems.data[j];
            set.elems[j].out = ends[j] != v;
        }
        alts = push(a, alts, prefixseq(k, set, v, end, a));
    }

    b32 literal = !b.ok;
    for (iz e = 0; e < n; e++) {
        if (elemmatch(b.elems.data+e, '[', k->flags)) {
            literal = ends[e] < 0;
            break;
        }
    }
    if (literal) {
        PatNode lb = {};
        lb.kind = PN_CHAR;
        lb.c    = '[';
        alts = push(a, alts, prefixseq(k, lb, i+1, end, a));
    }

    PatNode r = {};
    r.kind  = PN_FORK;
    r.c     = '@';
    r.alts  = alts.data;
    r.nalts = alts.len;
    return r;
}

static PatSeq compileseq(PatComp *k, iz i, iz end, Arena *a)
{
    Slice<PatNode> q = {};
    while (i < end) {
        PatNode nd = {};
        PatUnit u  = unitat(k->pat, i, k->wide);
        if ((k->flags&PM_EXTGLOB) && isextop(u.c) && i+1<end && k->pat[i+1]=='(') {
            iz close = patscan(k, i+2, end, 0);
            if (close < 0) {
                // Unclosed: bash compares the rest of the pattern literally
                nd.kind = PN_REST;
                nd.c    = u.c;
                nd.flag = endflag(k, q, u.c, i, *a);
                nd.text = span(k->pat.data+i, k->pat.data+end);
                q = push(a, q, nd);
                break;
            }
            if (++k->nest > PAT_MAXNEST) {
                k->deep = 1;
                return {};
            }
            Slice<PatSeq> alts = {};
            for (iz j = i+2;;) {
                iz next = patscan(k, j, end, '|');
                assert(next>j && next<=close);  // scans agree on nesting
                alts = push(a, alts, compileseq(k, j, next-1, a));
                if (k->deep) return {};
                if (next == close) break;
                j = next;
            }
            k->nest--;
            nd.kind  = PN_GROUP;
            nd.c     = u.c;
            nd.flag  = endflag(k, q, u.c, i, *a);
            nd.alts  = alts.data;
            nd.nalts = alts.len;
            i = close;

        } else if (u.c == '*') {
            nd.kind = PN_STAR;
            i++;

        } else if (u.c == '?') {
            nd.kind = PN_ANY;
            i++;

        } else if (u.c == '[') {
            // The first character is never the closing bracket, so a
            // bracket starting after the last ] needs no scan, and the
            // elements of an unterminated bracket are discarded.
            Arena      t = *a;
            PatBracket b = {};
            if (k->lastclose >= i+2) {
                b = parsebracket(k, i+1, k->pat.len, &t);
            }
            if (b.diverge && !b.neg && k->forks<4) {
                *a = t;
                q  = push(a, q, forkbracket(k, i, end, b, a));
                break;
            } else if (b.ok) {
                *a = t;
                nd.kind   = PN_SET;
                nd.elems  = b.elems.data;
                nd.nelems = b.elems.len;
                nd.neg    = b.neg;
                i = b.end<end ? b.end : end;
            } else {
                nd.kind = PN_CHAR;
                nd.c    = '[';
                i++;
            }

        } else if (u.c == '\\') {
            nd.kind = PN_CHAR;
            if (i+1 == end) {
                nd.c    = '\\';
                nd.flag = PN_TRAIL;
                i++;
            } else {
                PatUnit e = unitat(k->pat, i+1, k->wide);
                nd.c    = patfold(e.c, k->flags);
                nd.flag = PN_ESC;
                i += 1 + e.len;
            }

        } else {
            nd.kind = PN_CHAR;
            nd.c    = patfold(u.c, k->flags);
            i += u.len;
        }
        q = push(a, q, nd);
    }
    PatSeq r = {q.data, q.len};
    findquirks(k, r);
    return r;
}

// Does q hold a star (bit 0) or a group (bit 1) at any depth?
static i32 groupstar(PatSeq q)
{
    i32 r = 0;
    for (iz i = 0; i < q.len; i++) {
        PatNode *nd = q.nodes + i;
        r |= nd->kind==PN_STAR ? 1 : 0;
        if (nd->kind==PN_GROUP || nd->kind==PN_FORK) {
            r |= 2;
            for (iz j = 0; j < nd->nalts; j++) {
                r |= groupstar(nd->alts[j]);
            }
        }
    }
    return r;
}

static PatSeq patcompile(PatComp *k, Str pat, i32 flags, b32 wide, Arena *a)
{
    k->pat       = pat;
    k->flags     = flags;
    k->wide      = wide;
    k->quirk     = 0;
    k->nest      = 0;
    k->deep      = 0;
    k->lastclose = -1;
    k->forks     = 0;
    for (iz i = 0; i < pat.len; i++) {
        k->lastclose = pat[i]==']' ? i : k->lastclose;
    }
    PatSeq q = compileseq(k, 0, pat.len, a);
    k->memo = groupstar(q) == 3;
    return q;
}

// Does the pattern hold only the nodes of a plain glob?
static b32 isplain(PatSeq q)
{
    for (iz i = 0; i < q.len; i++) {
        i32 kind = q.nodes[i].kind;
        if (kind==PN_GROUP || kind==PN_REST || kind==PN_FORK) {
            return 0;
        }
    }
    return 1;
}


// Matching context

struct PosSet {
    u64 *w;
    iz   lo;  // first word
    iz   hi;  // one past the last word
};

struct PatMemo;

struct PatCtx {
    Str      s;
    iz       words;
    i32      flags;
    b32      wide;
    PosSet   bound;     // character boundaries in wide mode
    iz       fixedend;  // end of a whole match, or -1
    b32      clean;     // plain regular semantics for * runs
    b32      slashes;   // PM_PATHNAME and s holds a slash
    i32      level;     // evaluation nesting
    b32      overflow;  // level exceeded PAT_MAXLEVEL: the result is void
    PatMemo *memo;      // results of evalseq, when memoizing
    Arena    mem;       // fixed block holding the memo
};

static PatUnit cunit(PatCtx *x, iz pos)
{
    return unitat(x->s, pos, x->wide);
}

// Is pos a place where a wildcard may not match, because of a leading
// dot (PM_PERIOD) or a leading . or .. (PM_DOTDOT)?
static b32 dotblocked(PatCtx *x, iz pos)
{
    Str s = x->s;
    if (!(x->flags&(PM_PERIOD|PM_DOTDOT)) || pos>=s.len || s[pos]!='.') {
        return 0;
    }
    b32 path = x->flags & PM_PATHNAME;
    if (pos>0 && !(path && s[pos-1]=='/')) {
        return 0;
    }
    if (x->flags & PM_PERIOD) {
        return 1;
    }
    iz n = s.len - pos;
    if (pos == 0) {
        return n==1 || (n==2 && s[1]=='.');
    }
    return n==1 || s[pos+1]=='/' ||
           (s[pos+1]=='.' && (n==2 || s[pos+2]=='/'));
}

// Can a ? or bracket consume the unit at pos?
static b32 wildok(PatCtx *x, iz pos, PatUnit u)
{
    return !(u.c=='/' && (x->flags&PM_PATHNAME)) && !dotblocked(x, pos);
}

static b32 matchone(PatCtx *x, PatNode *nd, iz pos, PatUnit u)
{
    switch (nd->kind) {
    case PN_CHAR: return (nd->flag&PN_TRAIL ? u.c : patfold(u.c, x->flags)) == nd->c;
    case PN_ANY:  return wildok(x, pos, u);
    case PN_SET:  return wildok(x, pos, u) && inbracket(nd, u.c, x->flags);
    }
    return 0;
}

// Plain globs: two pointers, backing up only to the last star.
static b32 twoptr(PatCtx *x, PatSeq q)
{
    iz n = x->s.len;
    if (q.len && q.nodes[0].kind!=PN_CHAR && dotblocked(x, 0)) {
        return 0;
    }
    if (q.len && (q.nodes[q.len-1].flag&PN_TRAIL)) {
        // bash never matches a trailing backslash right after a * run
        for (iz i = q.len-2; i>=0 && q.nodes[i].kind!=PN_CHAR; i--) {
            if (q.nodes[i].kind==PN_STAR) return 0;
            if (q.nodes[i].kind!=PN_ANY) break;
        }
    }

    iz px     = 0;
    iz sx     = 0;
    iz nextpx = -1;
    iz nextsx = 0;
    while (px<q.len || sx<n) {
        if (px < q.len) {
            PatNode *nd = q.nodes + px;
            if (nd->kind == PN_STAR) {
                nextpx = px;
                nextsx = sx<n ? sx+cunit(x, sx).len : n+1;
                px++;
                continue;
            }
            if (sx < n) {
                PatUnit u = cunit(x, sx);
                if (matchone(x, nd, sx, u)) {
                    sx += u.len;
                    px++;
                    continue;
                }
            }
        }
        if (nextpx>=0 && nextsx<=n) {
            px = nextpx;
            sx = nextsx;
            continue;
        }
        return 0;
    }
    return 1;
}


// Position sets

static PosSet newset(Arena *a, iz lo, iz hi)
{
    PosSet r = {};
    r.w  = alloc<u64>(a, hi-lo);
    r.lo = lo;
    r.hi = hi;
    return r;
}

static PosSet setone(Arena *a, iz pos)
{
    PosSet r = newset(a, pos>>6, (pos>>6)+1);
    r.w[0] = (u64)1 << (pos&63);
    return r;
}

static b32 sethas(PosSet s, iz pos)
{
    iz i = pos >> 6;
    return i>=s.lo && i<s.hi && s.w[i-s.lo]>>(pos&63)&1;
}

static void setput(PosSet *s, iz pos)
{
    iz i = pos >> 6;
    assert(i>=s->lo && i<s->hi);
    s->w[i-s->lo] |= (u64)1 << (pos&63);
}

static u64 setword(PosSet s, iz i)
{
    return i>=s.lo && i<s.hi ? s.w[i-s.lo] : 0;
}

static PosSet settrim(PosSet s)
{
    for (; s.lo<s.hi && !s.w[0]; s.lo++, s.w++) {}
    for (; s.hi>s.lo && !s.w[s.hi-s.lo-1]; s.hi--) {}
    return s;
}

static b32 setempty(PosSet s)
{
    for (iz i = s.lo; i < s.hi; i++) {
        if (s.w[i-s.lo]) return 0;
    }
    return 1;
}

static iz setmin(PosSet s)  // -1 if empty
{
    for (iz i = s.lo; i < s.hi; i++) {
        u64 v = s.w[i-s.lo];
        if (v) return i<<6 | (iz)__builtin_ctzll(v);
    }
    return -1;
}

static iz setmax(PosSet s)  // -1 if empty
{
    for (iz i = s.hi-1; i >= s.lo; i--) {
        u64 v = s.w[i-s.lo];
        if (v) return i<<6 | (63 - (iz)__builtin_clzll(v));
    }
    return -1;
}

static PosSet setor(Arena *a, PosSet x, PosSet y)
{
    if (x.lo == x.hi) return y;
    if (y.lo == y.hi) return x;
    PosSet r = newset(a, x.lo<y.lo ? x.lo : y.lo, x.hi>y.hi ? x.hi : y.hi);
    for (iz i = x.lo; i < x.hi; i++) r.w[i-r.lo] |= x.w[i-x.lo];
    for (iz i = y.lo; i < y.hi; i++) r.w[i-r.lo] |= y.w[i-y.lo];
    return r;
}

// Merge y into x, whose range covers y's set bits.
static void orinto(PosSet *x, PosSet y)
{
    for (iz i = y.lo; i < y.hi; i++) {
        if (y.w[i-y.lo]) {
            assert(i>=x->lo && i<x->hi);
            x->w[i-x->lo] |= y.w[i-y.lo];
        }
    }
}

static PosSet setdrop(Arena *a, PosSet s, iz pos)
{
    if (!sethas(s, pos)) return s;
    PosSet r = newset(a, s.lo, s.hi);
    for (iz i = s.lo; i < s.hi; i++) r.w[i-r.lo] = s.w[i-s.lo];
    r.w[(pos>>6)-r.lo] &= ~((u64)1 << (pos&63));
    return settrim(r);
}

static u64 lastmask(PatCtx *x)
{
    iz bits = x->s.len&63;
    return bits==63 ? ~(u64)0 : ((u64)1<<(bits+1)) - 1;
}

// Bits of word i that are character boundaries.
static u64 boundword(PatCtx *x, iz i)
{
    u64 m = x->wide ? setword(x->bound, i) : ~(u64)0;
    return i==x->words-1 ? m&lastmask(x) : m;
}

static PosSet makebound(PatCtx *x, Arena *a)
{
    PosSet r = newset(a, 0, x->words);
    for (iz i = 0; i < x->s.len; i += cunit(x, i).len) {
        setput(&r, i);
    }
    setput(&r, x->s.len);
    return r;
}

static PatCtx makectx(Str s, i32 flags, b32 wide, Arena *a)
{
    PatCtx x = {};
    x.s        = s;
    x.words    = (s.len>>6) + 1;
    x.flags    = flags;
    x.wide     = wide;
    x.fixedend = -1;
    if (wide) {
        x.bound = makebound(&x, a);
    }
    for (iz i = 0; i<s.len && (flags&PM_PATHNAME); i++) {
        x.slashes |= s[i] == '/';
    }
    return x;
}

// Give the context a memo block for evalseq, sized for a few dozen
// full position sets and capped at a fraction of what remains.
static void addmemo(PatCtx *x, Arena *a)
{
    iz avail = (a->mem ? a->mem->limit : a->end) - a->beg;
    iz size  = ((iz)1<<18) + 8*64*x->words;
    size = size<((iz)1<<24) ? size : (iz)1<<24;
    size = size<avail/4 ? size : avail/4;
    x->mem.beg = (byte *)allocbytes(a, size);
    x->mem.end = x->mem.beg + size;
}

// Every boundary from the smallest position in s through the end.
static PosSet fillall(PatCtx *x, PosSet s, Arena *a)
{
    iz     first = setmin(s);
    PosSet r     = newset(a, first>>6, x->words);
    for (iz i = r.lo; i < r.hi; i++) {
        u64 m = boundword(x, i);
        if (i == r.lo) m &= ~(u64)0 << (first&63);
        r.w[i-r.lo] = m;
    }
    return r;
}

// Positions a star may stop at under PM_PATHNAME, starting from each
// position in s: through the next slash (incl), or only before it.
static PosSet fillpath(PatCtx *x, PosSet s, b32 incl, Arena *a)
{
    iz     first = setmin(s);
    PosSet r     = newset(a, first>>6, x->words);
    b32    carry = 0;
    for (iz p = first;;) {
        carry |= sethas(s, p);
        b32 slash = p<x->s.len && x->s[p]=='/';
        if (carry && (incl || !slash)) {
            setput(&r, p);
        }
        if (p == x->s.len) break;
        carry &= !slash;
        p += cunit(x, p).len;
    }
    return settrim(r);
}

// Advance each position by one unit: ? inside a * run.
static PosSet stepany(PatCtx *x, PosSet s, Arena *a)
{
    PosSet r = newset(a, s.lo, s.hi+1<x->words ? s.hi+1 : x->words);
    for (iz i = s.lo; i < s.hi; i++) {
        for (u64 v = s.w[i-s.lo]; v; v &= v-1) {
            iz pos = i<<6 | (iz)__builtin_ctzll(v);
            if (pos < x->s.len) setput(&r, pos+cunit(x, pos).len);
        }
    }
    return settrim(r);
}

static PosSet dropslashes(PatCtx *x, PosSet s, Arena *a)
{
    PosSet r = newset(a, s.lo, s.hi);
    for (iz i = s.lo; i < s.hi; i++) {
        for (u64 v = s.w[i-s.lo]; v; v &= v-1) {
            iz pos = i<<6 | (iz)__builtin_ctzll(v);
            if (pos==x->s.len || x->s[pos]!='/') setput(&r, pos);
        }
    }
    return settrim(r);
}

static PosSet dropblocked(PatCtx *x, PosSet s, Arena *a)
{
    if (!(x->flags&(PM_PERIOD|PM_DOTDOT))) return s;
    PosSet r = newset(a, s.lo, s.hi);
    for (iz i = s.lo; i < s.hi; i++) {
        for (u64 v = s.w[i-s.lo]; v; v &= v-1) {
            iz pos = i<<6 | (iz)__builtin_ctzll(v);
            if (!dotblocked(x, pos)) setput(&r, pos);
        }
    }
    return settrim(r);
}


// Set matcher

static PosSet evalseq(PatCtx *, PatSeq, iz, PosSet, i32, Arena *);
static PosSet memoseq(PatCtx *, PatSeq, iz, PosSet, i32, Arena *);

// Positions after one node that consumes a unit, or a literal tail.
static PosSet stepnode(PatCtx *x, PatNode *nd, PosSet s, Arena *a)
{
    iz     n  = x->s.len;
    iz     hi = nd->kind==PN_REST ? x->words : s.hi+1<x->words ? s.hi+1 : x->words;
    PosSet r  = newset(a, s.lo, hi);
    for (iz i = s.lo; i < s.hi; i++) {
        for (u64 v = s.w[i-s.lo]; v; v &= v-1) {
            iz pos = i<<6 | (iz)__builtin_ctzll(v);
            if (pos == n) continue;
            if (nd->kind == PN_REST) {
                Str tail = cuthead(x->s, pos);
                if (startswith(tail, nd->text)) setput(&r, pos+nd->text.len);
            } else {
                PatUnit u = cunit(x, pos);
                if (matchone(x, nd, pos, u)) setput(&r, pos+u.len);
            }
        }
    }
    return settrim(r);
}

static PosSet groupstep(PatCtx *x, PatNode *nd, PosSet s, i32 depth, Arena *a)
{
    PosSet r = {};
    switch (nd->c) {
    case '?':
    case '@':
        r = nd->c=='?' ? s : r;
        for (iz i = 0; i < nd->nalts; i++) {
            r = setor(a, r, evalseq(x, nd->alts[i], 0, s, depth, a));
        }
        return r;

    case '*':
    case '+': {
        iz     lo     = s.lo;
        PosSet acc    = newset(a, lo, x->words);
        PosSet buf[2] = {newset(a, lo, x->words), newset(a, lo, x->words)};
        if (nd->c == '*') orinto(&acc, s);
        PosSet f = s;
        for (i32 which = 0;; which ^= 1) {
            Arena  t    = *a;
            PosSet next = {};
            for (iz i = 0; i < nd->nalts; i++) {
                next = setor(&t, next, evalseq(x, nd->alts[i], 0, f, depth, &t));
            }
            next = settrim(next);
            if (next.lo == next.hi) break;
            PosSet b = buf[which];
            for (iz i = next.lo; i < next.hi; i++) {
                u64 v = next.w[i-next.lo] & ~acc.w[i-lo];
                b.w[i-lo]    = v;
                acc.w[i-lo] |= v;
            }
            f.w  = b.w + (next.lo - lo);
            f.lo = next.lo;
            f.hi = next.hi;
            f    = settrim(f);
            if (f.lo == f.hi) break;
        }
        return settrim(acc);
    }

    case '!': {
        PosSet acc = newset(a, s.lo, x->words);
        for (iz i = s.lo; i < s.hi; i++) {
            for (u64 v = s.w[i-s.lo]; v; v &= v-1) {
                iz pos = i<<6 | (iz)__builtin_ctzll(v);
                if (dotblocked(x, pos)) continue;
                Arena  t = *a;
                PosSet e = {};
                PosSet p = setone(&t, pos);
                for (iz j = 0; j < nd->nalts; j++) {
                    e = setor(&t, e, memoseq(x, nd->alts[j], 0, p, depth, &t));
                }
                for (iz w = pos>>6; w < x->words; w++) {
                    u64 m = boundword(x, w) & ~setword(e, w);
                    if (w == pos>>6) m &= ~(u64)0 << (pos&63);
                    acc.w[w-acc.lo] |= m;
                }
            }
        }
        return settrim(acc);
    }
    }
    return r;
}

// Where the plain nodes q[j..j2) first match, starting from positions
// in s in order: the position after them, or -1. Each node consumes one
// unit, so this is also the smallest end over all starts.
static iz leftmost(PatCtx *x, PatSeq q, iz j, iz j2, PosSet s)
{
    iz n = x->s.len;
    for (iz i = s.lo; i < s.hi; i++) {
        for (u64 v = s.w[i-s.lo]; v; v &= v-1) {
            iz pos = i<<6 | (iz)__builtin_ctzll(v);
            iz k   = j;
            for (; k<j2 && pos<n; k++) {
                PatUnit u = cunit(x, pos);
                if (!matchone(x, q.nodes+k, pos, u)) break;
                pos += u.len;
            }
            if (k == j2) return pos;
        }
    }
    return -1;
}

// The ends reached by matching q[j..] from each position in s, other
// than that position itself. Inside a group, where the end of the match
// varies, bash tries these only at positions before the end.
static PosSet consumeall(PatCtx *x, PatSeq q, iz j, PosSet s, i32 depth, Arena *a)
{
    PosSet acc = newset(a, s.lo, x->words);
    for (iz i = s.lo; i < s.hi; i++) {
        for (u64 v = s.w[i-s.lo]; v; v &= v-1) {
            iz     pos = i<<6 | (iz)__builtin_ctzll(v);
            Arena  t   = *a;
            PosSet e   = memoseq(x, q, j, setone(&t, pos), depth, &t);
            b32    had = sethas(acc, pos);  // later starts never reach pos
            orinto(&acc, e);
            if (!had) acc.w[(pos>>6)-acc.lo] &= ~((u64)1 << (pos&63));
        }
    }
    return settrim(acc);
}

// The result of evaluating a star run: the positions in res, plus those
// reached by matching q[k..] from the positions in s. Returning the rest
// of the sequence to evalseq, instead of evaluating it here, keeps the
// recursion depth independent of the number of stars.
struct PatCont {
    PosSet res;
    PosSet s;
    iz     k;
};

static PatCont patcont(PosSet res, iz k, PosSet s)
{
    PatCont r = {};
    r.res = res;
    r.s   = s;
    r.k   = k;
    return r;
}

// Is the whole match already known to succeed? Then nothing more needs
// evaluating at the top level.
static b32 topmatch(PatCtx *x, PosSet s, i32 depth)
{
    return depth==0 && x->fixedend>=0 && sethas(s, x->fixedend);
}

// A star and the run of *, ?, ?(..) and *(..) after it, then the rest of
// the sequence. This follows bash: each ?(..) in the run is also tried
// in place, a *(..) at each later position before the end, and the rest
// after the run is tried only at positions before the end of the string
// (or the next slash under PM_PATHNAME), so a following group cannot
// match the empty string there. A run that reaches the end of a whole
// match before a top-level !(..) matches regardless of what follows,
// unless endrule says otherwise.
static PatCont evalrun(PatCtx *x, PatSeq q, iz k, PosSet s, i32 depth, Arena *a)
{
    iz     n    = x->s.len;
    b32    path = x->slashes;  // otherwise PM_PATHNAME changes nothing here
    b32    top  = depth==0 && x->fixedend>=0;
    PosSet p    = settrim(dropblocked(x, s, a));
    PosSet res  = {};
    PosSet none = {};

    // bash looks for a leading dot within the string being matched, so a
    // star ending a group alternative still matches the empty string at
    // a leading dot.
    b32 last = x->flags & PM_PERIOD;
    for (iz j = k+1; j < q.len; j++) {
        last &= q.nodes[j].kind == PN_STAR;
    }
    if (last && depth>0) {
        PosSet b = newset(a, s.lo, s.hi);
        for (iz i = s.lo; i < s.hi; i++) {
            for (u64 v = s.w[i-s.lo]; v; v &= v-1) {
                iz pos = i<<6 | (iz)__builtin_ctzll(v);
                if (dotblocked(x, pos)) setput(&b, pos);
            }
        }
        res = settrim(b);
    }

    if (x->clean) {
        if (p.lo == p.hi) return patcont(res, q.len, none);
        PosSet f = path ? fillpath(x, p, 1, a) : fillall(x, p, a);
        return patcont(res, k+1, f);
    }

    iz j = k + 1;
    for (; j<q.len && runelem(q.nodes+j); j++) {
        PatNode *e = q.nodes + j;
        if (path) p = dropslashes(x, p, a);
        if (p.lo == p.hi) return patcont(res, q.len, none);
        if (e->kind == PN_REST) {
            return patcont(setor(a, res, fillall(x, p, a)), q.len, none);
        } else if (e->kind==PN_GROUP && e->c=='?') {
            res = setor(a, res, memoseq(x, q, j, p, depth, a));
        } else if (e->kind==PN_GROUP && top) {
            PosSet f = setdrop(a, fillall(x, p, a), n);
            if (f.lo < f.hi) {
                res = setor(a, res, memoseq(x, q, j, f, depth, a));
            }
        } else if (e->kind == PN_GROUP) {
            res = setor(a, res, consumeall(x, q, j, fillall(x, p, a), depth, a));
        } else if (e->kind == PN_ANY) {
            p = stepany(x, p, a);
        }
        if (topmatch(x, res, depth)) return patcont(res, q.len, none);
    }
    if (j == q.len) {
        res = p.lo<p.hi ? setor(a, res, fillall(x, p, a)) : res;
        return patcont(res, q.len, none);
    }
    if (p.lo == p.hi) {
        return patcont(res, q.len, none);
    }

    PatNode *r1 = q.nodes + j;
    if (r1->kind==PN_CHAR && (r1->flag&PN_TRAIL)) {
        return patcont(res, q.len, none);
    }
    b32 neg = (r1->kind==PN_GROUP || r1->kind==PN_REST) && r1->c=='!';
    if (neg && !(r1->flag&PN_ENDNO) && top && sethas(p, n)) {
        res = setor(a, res, setone(a, n));
    }
    if (path && r1->kind==PN_CHAR && r1->c=='/' && !(r1->flag&PN_ESC)) {
        return patcont(res, j, fillpath(x, p, 1, a));
    }

    PosSet f = path ? fillpath(x, p, 0, a) : fillall(x, p, a);
    if (f.lo == f.hi) {
        return patcont(res, q.len, none);
    }

    // Before another star, bash commits to the leftmost match of the
    // plain segment between them (a backtracking shortcut from glibc).
    iz j2 = j;
    for (; j2 < q.len; j2++) {
        i32 kind = q.nodes[j2].kind;
        if (kind!=PN_CHAR && kind!=PN_ANY && kind!=PN_SET) break;
    }
    if (j2>j && j2<q.len && q.nodes[j2].kind==PN_STAR) {
        iz end = leftmost(x, q, j, j2, f);
        if (end < 0) {
            return patcont(res, q.len, none);
        }
        return patcont(res, j2, setone(a, end));
    }

    if (r1->kind!=PN_GROUP || (!top && r1->c!='!' && !seqnullable(q, j))) {
        return patcont(res, j, f);
    }
    if (top) {
        return patcont(res, j, setdrop(a, f, n));
    }

    res = setor(a, res, consumeall(x, q, j, f, depth, a));
    return patcont(res, q.len, none);
}


// Memo

// Results of evalseq keyed by the node and the trimmed start set, in a
// 4-ary hash trie held in its own fixed block, so that entries survive
// the temporary arenas of the callers. When the block fills up, new
// results are simply not recorded.
struct PatMemo {
    PatMemo *child[4];
    PatNode *nodes;
    iz       k;
    PosSet   key;
    PosSet   val;
};

static u64 memohash(PatNode *nodes, iz k, PosSet s)
{
    u64 h = (u64)(uz)nodes ^ (u64)k<<48 ^ (u64)s.lo<<24 ^ (u64)s.hi;
    for (iz i = 0; i < s.hi-s.lo; i++) {
        h ^= s.w[i];
        h *= 1111111111111111111u;
    }
    return h ^ h>>31;
}

static b32 sameset(PosSet x, PosSet y)
{
    if (x.lo!=y.lo || x.hi!=y.hi) return 0;
    for (iz i = 0; i < x.hi-x.lo; i++) {
        if (x.w[i] != y.w[i]) return 0;
    }
    return 1;
}

static PosSet copyset(Arena *a, PosSet s)
{
    PosSet r = newset(a, s.lo, s.hi);
    for (iz i = 0; i < s.hi-s.lo; i++) r.w[i] = s.w[i];
    return r;
}

static PatMemo **memofind(PatCtx *x, PatNode *nodes, iz k, PosSet s)
{
    PatMemo **m = &x->memo;
    for (u64 h = memohash(nodes, k, s); *m; h <<= 2) {
        if ((*m)->nodes==nodes && (*m)->k==k && sameset((*m)->key, s)) break;
        m = &(*m)->child[h>>62];
    }
    return m;
}

// Evaluate q[k..] from s through the memo, at the call sites where
// bash backtracks: alternatives tried in place within a star run, rest
// of a sequence tried from each position, and negations.
static PosSet memoseq(PatCtx *x, PatSeq q, iz k, PosSet s, i32 depth, Arena *a)
{
    s = settrim(s);
    if (!x->mem.beg || k==q.len || s.lo==s.hi) {
        return evalseq(x, q, k, s, depth, a);
    }
    PatMemo **m = memofind(x, q.nodes, k, s);
    if (*m) {
        return (*m)->val;
    }

    PosSet r    = settrim(evalseq(x, q, k, s, depth, a));
    iz     need = 2*(iz)sizeof(PatMemo) + 8*(s.hi-s.lo + r.hi-r.lo + 8);
    if (!x->overflow && need<x->mem.end-x->mem.beg) {
        m = memofind(x, q.nodes, k, s);  // evaluation may have added entries
        PatMemo *e = alloc<PatMemo>(&x->mem);
        e->nodes = q.nodes;
        e->k     = k;
        e->key   = copyset(&x->mem, s);
        e->val   = copyset(&x->mem, r);
        *m = e;
    }
    return r;
}

// Copy s into a full-size buffer, returning the view of its range.
static PosSet keepset(PosSet buf, PosSet s)
{
    PosSet r = {buf.w+s.lo, s.lo, s.hi};
    for (iz i = 0; i < s.hi-s.lo; i++) r.w[i] = s.w[i];
    return r;
}

// Positions reachable after matching q[k..] from any position in s.
// Once a sequence runs long, each node is evaluated in a temporary
// arena and only its result is kept, in alternating buffers, so that
// memory does not grow with the length of the pattern.
static PosSet evalseq(PatCtx *x, PatSeq q, iz k, PosSet s, i32 depth, Arena *a)
{
    if (x->level == PAT_MAXLEVEL) {
        x->overflow = 1;
        return {};
    }
    x->level++;
    PosSet acc     = {};
    PosSet keep[2] = {};
    i32    which   = 0;
    for (iz steps = 0; k < q.len; k++, steps++) {
        s = settrim(s);
        if (s.lo == s.hi) break;
        if (steps==16 && !keep[0].w) {
            PosSet full = newset(a, 0, x->words);
            orinto(&full, acc);
            acc     = full;
            keep[0] = newset(a, 0, x->words);
            keep[1] = newset(a, 0, x->words);
        }
        Arena    t  = *a;
        Arena   *ta = keep[0].w ? &t : a;
        PatNode *nd = q.nodes + k;
        PatCont  c  = {};
        switch (nd->kind) {
        case PN_STAR:
            c = evalrun(x, q, k, s, depth, ta);
            if (keep[0].w) {
                orinto(&acc, c.res);
            } else {
                acc = setor(a, acc, c.res);
            }
            s = c.s;
            k = c.k - 1;
            if (topmatch(x, acc, depth)) {
                s = {};
                k = q.len;
            }
            break;
        case PN_GROUP: s = groupstep(x, nd, s, depth+1, ta); break;
        case PN_FORK:  s = groupstep(x, nd, s, depth, ta);   break;
        default:       s = stepnode(x, nd, s, ta);
        }
        if (keep[0].w) {
            s = keepset(keep[which], settrim(s));
            which ^= 1;
        }
    }
    x->level--;
    return setor(a, acc, s);
}


// Interface

// Whole-string match. pat uses backslash to quote the next byte. For
// case and [[ == ]] (which bash always matches with extglob), pass
// PM_EXTGLOB, PM_NOCASE and PM_UTF8 as appropriate; for pathname
// expansion, use globmatch instead.
static b32 patmatch(Str pat, Str s, i32 flags, Arena scratch)
{
    b32 wide = 0;
    if ((flags&PM_UTF8) && !(allascii(pat) && allascii(s))) {
        wide = glibcvalid(pat) && glibcvalid(s);
    }
    PatComp k = {};
    PatSeq  q = patcompile(&k, pat, flags, wide, &scratch);
    if (k.deep) {
        return 0;
    }
    b32 slash = 0;
    for (iz i = 0; i<s.len && (flags&PM_PATHNAME); i++) {
        slash |= s[i] == '/';
    }
    if (!slash && isplain(q)) {
        // Without a slash in s, PM_PATHNAME changes nothing here
        PatCtx x = {};
        x.s     = s;
        x.flags = flags;
        x.wide  = wide;
        return twoptr(&x, q);
    }
    PatCtx x = makectx(s, flags, wide, &scratch);
    x.fixedend = s.len;
    if (k.memo) {
        addmemo(&x, &scratch);
    }
    PosSet e = evalseq(&x, q, 0, setone(&scratch, 0), 0, &scratch);
    return !x.overflow && sethas(e, s.len);
}

// Does pat contain an unquoted glob operator, so that it undergoes
// pathname expansion? Like bash, any [ followed by a ] with no slash
// between counts, and so do extglob groups with PM_EXTGLOB.
static b32 hasglob(Str pat, i32 flags)
{
    b32 open = 0;
    for (iz i = 0; i < pat.len; i++) {
        switch (pat[i]) {
        case '?':
        case '*':
            return 1;
        case '[':
            open = 1;
            break;
        case ']':
            if (open) return 1;
            break;
        case '/':
            open = 0;
            break;
        case '+':
        case '@':
        case '!':
            if ((flags&PM_EXTGLOB) && i+1<pat.len && pat[i+1]=='(') return 1;
            break;
        case '\\':
            if (++i == pat.len) return 0;
            break;
        }
    }
    return 0;
}

// Remove the backslash quoting from a pattern (for a literal fallback).
// A trailing lone backslash is dropped.
static Str unquoteglob(Arena *a, Str pat)
{
    iz i = 0;
    for (; i<pat.len && pat[i]!='\\'; i++) {}
    if (i == pat.len) {
        return pat;
    }
    Str r = {allocbytes(a, pat.len), 0};
    for (i = 0; i < pat.len; i++) {
        if (pat[i]=='\\' && ++i==pat.len) break;
        r.data[r.len++] = pat[i];
    }
    return r;
}


// Pathname expansion

static b32 dotordotdot(Str s)
{
    return s=="." || s=="..";
}

static b32 leaddot(Str pat)
{
    return startswith(pat, ".") || startswith(pat, "\\.");
}

static b32 extstart(Str pat)
{
    return pat.len>1 && isextop(pat[0]) && pat[1]=='(';
}

static b32 skipname(Str pat, Str name, i32 flags, i32 depth);

struct PatSkip {
    Str rest;  // when more: the rest of the pattern decides
    b32 skip;
    b32 more;
};

// The leading-dot check for a pattern that starts with a group: a dot
// file is kept when some alternative (or, after a ?(..) or *(..), the
// rest of the pattern) starts with a dot.
static PatSkip extskip(Str pat, Str name, i32 flags, i32 depth)
{
    PatSkip r = {};
    PatComp k = {};
    k.pat = pat;
    b32 wild = pat[0]=='*' || pat[0]=='?';
    iz  pp   = 2;
    iz  pe   = patscan(&k, pp, pat.len, 0);
    if (pe < 0) {
        return r;
    }
    if (pe == pat.len) {
        Str body = span(pat.data+pp, pat.data+pe-1);
        b32 bar  = 0;
        for (iz i = 0; i < body.len; i++) bar |= body[i]=='|';
        if (!bar) {
            r.skip = skipname(body, name, flags, depth+1);
            return r;
        }
    }

    r.skip = 1;
    for (;;) {
        iz t = patscan(&k, pp, pe, '|');
        if (t < 0) break;
        Str sub = span(pat.data+pp, pat.data+t-1);
        if (pat[t-1]==')' && extstart(sub)) {
            sub = cuthead(pat, pp);
        }
        r.skip = skipname(sub, name, flags, depth+1);
        if (!r.skip) return r;
        pp = t;
        if (pp == pe) break;
    }
    if (pp!=pat.len && wild && pe<pat.len) {
        r.rest = cuthead(pat, pe);
        r.more = 1;
    }
    r.skip |= pp != pat.len;
    return r;
}

// Should the directory entry be skipped before matching, because of
// its leading dot? Nesting too deep to match at all skips it.
static b32 skipname(Str pat, Str name, i32 flags, i32 depth)
{
    if (depth > PAT_MAXNEST) {
        return 1;
    }
    while (extstart(pat)) {
        PatSkip r = extskip(pat, name, flags, depth);
        if (!r.more) return r.skip;
        pat = r.rest;
    }
    if ((flags&PM_SKIPDOTS) && dotordotdot(name)) {
        return 1;
    }
    if (flags & PM_PERIOD) {
        return startswith(name, ".") && !leaddot(pat);
    }
    return !leaddot(pat) && dotordotdot(name);
}

// Does pathname expansion of the component pattern pat (one without
// slashes, for which hasglob is true) select the directory entry name?
// Flags: PM_EXTGLOB, PM_NOCASE (nocaseglob), PM_UTF8, PM_PERIOD (dotglob
// off) and PM_SKIPDOTS (globskipdots). This includes bash's checks for
// leading dots before matching, also inside extglob groups. A component
// without glob characters is looked up directly instead, as in bash.
static b32 globmatch(Str pat, Str name, i32 flags, Arena scratch)
{
    if (skipname(pat, name, flags, 0)) {
        return 0;
    }
    i32 mf = flags & (PM_EXTGLOB|PM_NOCASE|PM_UTF8);
    mf |= PM_PATHNAME | (flags&PM_PERIOD ? PM_PERIOD : PM_DOTDOT);
    return patmatch(pat, name, mf, scratch);
}


// Parameter expansion

// bash's fixed match length for its substitution shortcut, in units, or
// -1 if the pattern may match strings of different lengths. It miscounts
// some brackets, which then never match there.
static iz bashmatchlen(Str pat, b32 wide)
{
    PatComp k = {};
    k.pat  = pat;
    k.wide = wide;
    iz end = pat.len;
    iz len = 0;
    for (iz i = 0; i < end;) {
        i32 c = peekc(&k, i, end);
        i = nextu(&k, i, end);
        switch (c) {
        default:
            len++;
            break;
        case '\\':
            len++;
            i = i<end ? nextu(&k, i, end) : i;
            break;
        case '*':
            return -1;
        case '?':
        case '+':
        case '!':
        case '@':
            if (peekc(&k, i, end) == '(') return -1;
            len++;
            break;
        case '[': {
            iz  n   = 1;
            b32 cls = 0;
            b32 sym = 0;
            b32 eqv = 0;
            c = peekc(&k, i, end);
            i = nextu(&k, i, end);
            for (;;) {
                i32 t = peekc(&k, i, end);
                if (c < 0) {
                    return len + n;
                } else if (c == '\\') {
                    n++;
                    if (i >= end) return len + n;
                    i = nextu(&k, i, end);
                    if (i >= end) return len + n;
                } else if (c=='[' && t==':') {
                    i++, n++, cls = 1;
                } else if (cls && c==':' && t==']') {
                    i++, n++, cls = 0;
                } else if (c=='[' && (t=='.' || t=='=')) {
                    i++, n++;
                    if (peekc(&k, i, end) == ']') i++, n++;
                    sym |= t=='.';
                    eqv |= t=='=';
                } else if (sym && c=='.' && t==']') {
                    i++, n++, sym = 0;
                } else if (eqv && c=='=' && t==']') {
                    i++, n++, eqv = 0;
                } else {
                    n++;
                }
                c = peekc(&k, i, end);
                i = nextu(&k, i, end);
                if (c == ']') break;
            }
            len++;
        } break;
        }
    }
    return len;
}

// The true fixed length of a compiled pattern in units, or -1.
static iz truelen(PatSeq q, b32 wide)
{
    iz len = 0;
    for (iz i = 0; i < q.len; i++) {
        PatNode *nd = q.nodes + i;
        switch (nd->kind) {
        case PN_STAR:
        case PN_GROUP:
        case PN_FORK:
            return -1;
        case PN_REST:
            for (iz j = 0; j < nd->text.len; j += unitat(nd->text, j, wide).len) {
                len++;
            }
            break;
        default:
            len++;
        }
    }
    return len;
}

// bash's quick test that a match could start at s[p].
static b32 firstchar(Str pat, Str s, iz p, i32 flags, b32 wide)
{
    if (p == s.len) {
        return pat[0] == '*';
    }
    i32 c    = unitat(pat, 0, wide).c;
    i32 next = pat.len>1 ? unitat(pat, 1, wide).c : 0;
    i32 sc   = patfold(unitat(s, p, wide).c, flags);
    switch (c) {
    case '*':
    case '[':
        return 1;
    case '?':
        return 1;
    case '\\':
        return sc == patfold(next, flags);
    case '+':
    case '!':
    case '@':
        if (next == '(') return 1;
    }
    return sc == patfold(c, flags);
}

// The pattern bash uses to test for any match at all before searching:
// the pattern with a * added at the front and back as needed.
static Str anypat(Arena *a, Str pat, i32 mode, i32 flags, b32 wide)
{
    b32 lead = pat[0]=='*' && !((flags&PM_EXTGLOB) && pat.len>1 && pat[1]=='(');
    b32 tail = pat[pat.len-1] == '*';
    if (lead && tail) {
        return pat;
    }
    b32 pre  = !lead && (wide || mode!=PF_ANCHORSTART);
    b32 post = wide || mode!=PF_ANCHOREND;
    if (post && tail) {
        iz n = 0;
        for (iz i = pat.len-2; i>=0 && pat[i]=='\\'; i--) n++;
        post = n & 1;
    }
    Str r = {allocbytes(a, pat.len+2), 0};
    if (pre) r.data[r.len++] = '*';
    copybytes(r.data+r.len, pat.data, pat.len);
    r.len += pat.len;
    if (post) r.data[r.len++] = '*';
    return r;
}

// Reverse the units of s.
static Str revstr(Arena *a, Str s, b32 wide)
{
    Str r = {allocbytes(a, s.len), s.len};
    for (iz i = 0; i < s.len;) {
        iz len = unitat(s, i, wide).len;
        copybytes(r.data+s.len-i-len, s.data+i, len);
        i += len;
    }
    return r;
}

static PatSeq revseq(Arena *a, PatSeq q, b32 wide)
{
    PatSeq r = {alloc<PatNode>(a, q.len), q.len};
    for (iz i = 0; i < q.len; i++) {
        PatNode nd = q.nodes[q.len-1-i];
        if (nd.kind == PN_GROUP) {
            PatSeq *alts = alloc<PatSeq>(a, nd.nalts);
            for (iz j = 0; j < nd.nalts; j++) {
                alts[j] = revseq(a, nd.alts[j], wide);
            }
            nd.alts = alts;
        } else if (nd.kind == PN_REST) {
            nd.text = revstr(a, nd.text, wide);
        }
        r.nodes[i] = nd;
    }
    return r;
}

// Search by probing substrings with whole matches, as bash does. This
// is exact for the corner cases the fast search does not model.
static PatSpan probespan(Str pat, Str s, i32 mode, i32 flags, b32 wide, Arena scratch)
{
    PatSpan r   = {};
    iz     *pos = alloc<iz>(&scratch, s.len+1);
    iz      np  = 0;
    for (iz i = 0; i < s.len; i += wide ? glibclen(s, i) : 1) {
        pos[np++] = i;
    }
    pos[np++] = s.len;
    iz last = np - 1;

    switch (mode) {
    case PF_SHORTPREFIX:
    case PF_LONGPREFIX:
        for (iz i = 0; i < np; i++) {
            iz e = pos[mode==PF_SHORTPREFIX ? i : last-i];
            if (patmatch(pat, takehead(s, e), flags, scratch)) {
                r.end = e;
                r.ok  = 1;
                return r;
            }
        }
        return r;

    case PF_SHORTSUFFIX:
    case PF_LONGSUFFIX:
        for (iz i = 0; i < np; i++) {
            iz b = pos[mode==PF_LONGSUFFIX ? i : last-i];
            if (patmatch(pat, cuthead(s, b), flags, scratch)) {
                r.beg = b;
                r.end = s.len;
                r.ok  = 1;
                return r;
            }
        }
        return r;
    }

    if (!patmatch(anypat(&scratch, pat, mode, flags, wide), s, flags, scratch)) {
        return r;
    }
    iz mlen = bashmatchlen(pat, wide);
    if (mlen > last) {
        return r;
    }
    switch (mode) {
    case PF_FIRST:
        for (iz i = 0; i < np; i++) {
            if (!firstchar(pat, s, pos[i], flags, wide)) continue;
            iz j = mlen<0 ? last : i+mlen;
            if (j > last) break;
            for (; j >= i; j--) {
                Str sub = span(s.data+pos[i], s.data+pos[j]);
                if (patmatch(pat, sub, flags, scratch)) {
                    r.beg = pos[i];
                    r.end = pos[j];
                    r.ok  = 1;
                    return r;
                }
                if (mlen >= 0) break;
            }
        }
        break;
    case PF_ANCHORSTART:
        if (!firstchar(pat, s, 0, flags, wide)) break;
        for (iz j = mlen<0 ? last : mlen; j >= 0; j--) {
            if (patmatch(pat, takehead(s, pos[j]), flags, scratch)) {
                r.end = pos[j];
                r.ok  = 1;
                return r;
            }
            if (mlen >= 0) break;
        }
        break;
    case PF_ANCHOREND:
        for (iz i = mlen<0 ? 0 : last-mlen; i < np; i++) {
            if (patmatch(pat, cuthead(s, pos[i]), flags, scratch)) {
                r.beg = pos[i];
                r.end = s.len;
                r.ok  = 1;
                return r;
            }
            if (mlen >= 0) break;
        }
        break;
    }
    return r;
}

static PatSpan findspan(Str pat, Str s, i32 mode, i32 flags, Arena scratch)
{
    PatSpan r = {};
    if (!pat.len) {
        // bash: ${x/#/r} prefixes and ${x/%/r} appends
        r.beg = r.end = mode==PF_ANCHOREND ? s.len : 0;
        r.ok  = mode==PF_ANCHORSTART || mode==PF_ANCHOREND;
        return r;
    }

    b32     multi = (flags&PM_UTF8) && !(allascii(pat) && allascii(s));
    b32     wide  = multi && glibcvalid(pat) && glibcvalid(s);
    PatComp k     = {};
    PatSeq  q     = patcompile(&k, pat, flags, wide, &scratch);
    if (k.deep) {
        return r;
    } else if (multi && !wide) {
        // Invalid UTF-8: byte positions, but bash decides per substring
        // whether to match by character.
        return probespan(pat, s, mode, flags, 0, scratch);
    }

    if (k.quirk || !s.len || (flags&(PM_PATHNAME|PM_PERIOD|PM_DOTDOT))) {
        return probespan(pat, s, mode, flags, wide, scratch);
    }
    iz len = truelen(q, wide);
    if (mode==PF_FIRST || mode==PF_ANCHORSTART || mode==PF_ANCHOREND) {
        iz  mlen = bashmatchlen(pat, wide);
        b32 bs   = pat.len==1 && pat[0]=='\\';
        if ((mlen>=0 && mlen!=len) || bs) {
            return probespan(pat, s, mode, flags, wide, scratch);
        }
        // bash first tests for any match with a * added at either end.
        // That is redundant unless an end lacks its star (the pattern
        // ends with \*), an added star changes the pattern (after a
        // trailing backslash, or before a parenthesis), or the stars
        // create a quirk site.
        Str     any  = anypat(&scratch, pat, mode, flags, wide);
        PatComp ka   = {};
        PatSeq  qa   = patcompile(&ka, any, flags, wide, &scratch);
        b32     same = qa.len == q.len+any.len-pat.len;
        b32     lead = qa.len && qa.nodes[0].kind==PN_STAR;
        b32     tail = qa.len && qa.nodes[qa.len-1].kind==PN_STAR;
        lead |= mode == PF_ANCHORSTART;
        tail |= mode == PF_ANCHOREND;
        if ((ka.quirk || !same || !lead || !tail) && !patmatch(any, s, flags, scratch)) {
            return r;
        }
    }

    iz     n = s.len;
    PatCtx x = makectx(s, flags, wide, &scratch);
    x.clean = 1;
    if (mode==PF_SHORTPREFIX || mode==PF_LONGPREFIX || mode==PF_ANCHORSTART) {
        PosSet e = evalseq(&x, q, 0, setone(&scratch, 0), 0, &scratch);
        if (setempty(e)) return r;
        r.end = mode==PF_SHORTPREFIX ? setmin(e) : setmax(e);
        r.ok  = 1;
        return r;
    }

    if (mode==PF_FIRST && len>=0) {
        // Without stars or groups, try each start in turn, so that no
        // input past the match is examined.
        for (iz i = 0; i < n; i += cunit(&x, i).len) {
            Arena  t = scratch;
            PosSet e = evalseq(&x, q, 0, setone(&t, i), 0, &t);
            if (!setempty(e)) {
                r.beg = i;
                r.end = setmax(e);
                r.ok  = 1;
                return r;
            }
        }
        return r;
    }

    // Match the reversed pattern against the reversed string to find
    // where matches start.
    Str    rs = revstr(&scratch, s, wide);
    PatSeq rq = revseq(&scratch, q, wide);
    PatCtx rx = makectx(rs, flags, wide, &scratch);
    rx.clean = 1;
    PosSet from;
    if (mode == PF_FIRST) {
        from = newset(&scratch, 0, rx.words);
        for (iz i = 0; i < rx.words; i++) from.w[i] = boundword(&rx, i);
    } else {
        from = setone(&scratch, 0);
    }
    PosSet e = evalseq(&rx, rq, 0, from, 0, &scratch);
    if (setempty(e)) {
        return r;
    }
    if (mode != PF_FIRST) {
        r.beg = n - (mode==PF_SHORTSUFFIX ? setmin(e) : setmax(e));
        r.end = n;
        r.ok  = 1;
        return r;
    }
    r.beg = n - setmax(e);
    PosSet f = evalseq(&x, q, 0, setone(&scratch, r.beg), 0, &scratch);
    r.end = setmax(f);
    r.ok  = 1;
    return r;
}

// Find a match in s for x#p, x##p, x%p, x%%p, x/p/r (PF_FIRST from offset
// 'from': the leftmost start, longest match there), x/#p/r and x/%p/r.
// For x//p/r, bash searches again from the end of each match (one
// character past it after an empty match) while text remains. Note that
// bash applies nocasematch (PM_NOCASE) to x/p/r but not to x#p.
static PatSpan patfind(Str pat, Str s, iz from, i32 mode, i32 flags, Arena scratch)
{
    if (mode != PF_FIRST) {
        return findspan(pat, s, mode, flags, scratch);
    }
    PatSpan r = findspan(pat, cuthead(s, from), mode, flags, scratch);
    r.beg += from;
    r.end += from;
    return r;
}
