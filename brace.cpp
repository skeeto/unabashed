// Brace expansion
//
// Brace expansion runs first, on a word's text as the parser stored it,
// with quotes and backslashes still in place. A brace expression is an
// unquoted "{" with a matching "}" and, between them at the same nesting
// level, either a comma (a list) or ".." (a sequence). Text inside
// quotes, after a backslash, inside ${...} and inside $(...), $((...)),
// <(...) and >(...) never forms one. A word expands left to right: the
// first brace expression found is expanded, prefixed by the text before
// it, and multiplied by the expansion of the text after it.
//
// Quirks of bash 5.3 kept here on purpose:
// - "}" closes only once a separator has been seen, so {a}b,c} is a
//   list of "a}b" and "c".
// - "{" is not special when it follows the start or a blank and is
//   followed by a blank, the end, or "}" (so {},a} stays as written).
// - A sequence candidate is first checked loosely; failing that check,
//   the "{" is passed over and the search goes on inside it. One that
//   passes but then does not parse stays as written, inner braces too.
// - ${ counts as an opening brace, but only outside quotes.
// - Inside a command substitution, ${...} ends at its first unquoted
//   "}" that closes no inner ${ (so ${x:-{} is complete), while
//   ${ cmd; } ends only at a reserved "}": one starting a word where a
//   command may start, outside a case pattern.
//
// Limits: brace expressions nested more than BRACE_MAXDEPTH deep are left
// as written, and constructs nested that deep count as unterminated, so
// hostile input cannot exhaust the stack. A sequence too long for the
// arena is left as written, where bash prints an allocation error.
//
// This is free and unencumbered software released into the public domain.


enum : i32 {  // what bracescan looks for
    BRACE_OPEN,
    BRACE_CLOSE,
    BRACE_COMMA,
};

enum : i32 {  // kinds of brace expression
    BRACE_NONE,
    BRACE_LIST,
    BRACE_SEQ,
};

enum : i32 {  // kinds of nested construct for braceskip
    BRACE_ARITH,   // $(( ... )): parentheses only
    BRACE_PARAM,   // ${ ... }: the first unquoted, unnested } closes
    BRACE_CMD,     // $( ... ), <( ... ), >( ... ), ( ... ) inside them
    BRACE_FUNSUB,  // ${ cmd; } and ${| cmd; }: closed by a reserved }
};

// Deeper nesting than this, of brace expressions or of constructs inside
// words, is not followed, which bounds the recursion on hostile input.
// Brace expressions nested deeper are left as written; a construct nested
// deeper counts as unterminated.
enum : i32 { BRACE_MAXDEPTH = 500 };

struct BraceScan {
    iz  pos;   // index of the delimiter found
    i32 kind;  // BRACE_CLOSE: BRACE_LIST or BRACE_SEQ
    b32 ok;
};

struct BraceInt {
    i64 value;
    iz  end;       // index just past the digits, or the start without any
    b32 ok;        // at least one digit and no overflow
};

static u8 braceat(Str s, iz i)  // zero past either end
{
    return i>=0 && i<s.len ? s[i] : 0;
}

static b32 braceblank(u8 c)  // a zero byte (past the end) counts
{
    return !c || c==' ' || c=='\t' || c=='\n';
}

static b32 bracefunsub(u8 c)
{
    return c==' ' || c=='\t' || c=='\n' || c=='|';
}

static i32 bracedolparen(u8 c)  // construct after "$(" followed by c
{
    return c=='(' ? BRACE_ARITH : BRACE_CMD;
}

static i32 bracedolbrace(u8 c)  // construct after "${" followed by c
{
    return bracefunsub(c) ? BRACE_FUNSUB : BRACE_PARAM;
}

static b32 bracebreak(u8 c)  // ends a word in a command
{
    switch (c) {
    case ' ': case '\t': case '\n': case ';': case '&': case '|':
    case '(': case ')': case '<': case '>':
        return 1;
    }
    return 0;
}

static iz braceskip(Str s, iz i, i32 kind, i32 depth);

// Skip a backquoted command from just past its opening quote, returning
// the index past the closing quote, or -1 if it is unterminated.
static iz braceskipbq(Str s, iz i)
{
    for (; i < s.len; i++) {
        if (s[i] == '\\') {
            i++;
        } else if (s[i] == '`') {
            return i + 1;
        }
    }
    return -1;
}

// Skip a double-quoted string from just past its opening quote.
static iz braceskipdq(Str s, iz i, i32 depth)
{
    while (i < s.len) {
        u8 c = s[i];
        u8 n = braceat(s, i+1);
        u8 k = braceat(s, i+2);
        if (c == '\\') {
            i += 2;
        } else if (c == '"') {
            return i + 1;
        } else if (c == '`') {
            i = braceskipbq(s, i+1);
        } else if (c=='$' && n=='(') {
            i = braceskip(s, i+2, bracedolparen(k), depth);
        } else if (c=='$' && n=='{') {
            i = braceskip(s, i+2, bracedolbrace(k), depth);
        } else {
            i++;
        }
        if (i < 0) return -1;
    }
    return -1;
}

// Reserved words after which a command may start.
static b32 bracecmdword(Str w)
{
    static Str words[] = {
        "if", "then", "else", "elif", "do", "while", "until",
        "!", "{", "time", "fi", "done", "}", "coproc",
    };
    for (iz i = 0; i < countof(words); i++) {
        if (w == words[i]) return 1;
    }
    return 0;
}

struct BraceDoc {
    Str eof;    // delimiter as written, quotes included
    b32 strip;  // <<- strips leading tabs
};

// End of a here-document delimiter word starting at i.
static iz bracedocend(Str s, iz i)
{
    while (i < s.len) {
        u8 c = s[i];
        if (c == '\\') {
            i += 2;
        } else if (c=='\'' || c=='"') {
            for (i++; i<s.len && s[i]!=c; i++) {
                i += c=='"' && s[i]=='\\';
            }
            i++;
        } else if (bracebreak(c)) {
            break;
        } else {
            i++;
        }
    }
    return i<s.len ? i : s.len;
}

// Does a body line end the here-document? Compares the line with the
// delimiter after quote removal.
static b32 bracedocmatch(BraceDoc d, Str line)
{
    iz j = 0;
    for (; d.strip && j<line.len && line[j]=='\t'; j++) {}
    u8 q = 0;
    for (iz i = 0; i < d.eof.len; i++) {
        u8 c = d.eof[i];
        if (q!='\'' && c=='\\' && i+1<d.eof.len) {
            u8 n = d.eof[i+1];
            if (!q || n=='\\' || n=='"' || n=='$' || n=='`') {
                c = n;
                i++;
            }
        } else if (!q && (c=='\'' || c=='"')) {
            q = c;
            continue;
        } else if (q && c==q) {
            q = 0;
            continue;
        }
        if (j==line.len || line[j++]!=c) {
            return 0;
        }
    }
    return j == line.len;
}

// Skip the bodies of pending here-documents, which start at i, returning
// the index past the last delimiter line, or -1 if one is unterminated.
static iz braceskipdocs(Str s, iz i, BraceDoc *docs, i32 ndocs)
{
    for (i32 k = 0; k < ndocs; k++) {
        for (b32 done = 0; !done;) {
            if (i >= s.len) {
                return -1;
            }
            Cut c = cut(cuthead(s, i), '\n');
            done = bracedocmatch(docs[k], c.head);
            i += c.head.len + c.ok;
        }
    }
    return i;
}

// Skip a nested construct from just inside its opening delimiter to just
// past its closing one, or return -1 if the text ends first. This only
// has to find the end, so commands are understood just well enough for
// quotes, comments, here-documents, the unbalanced ")" of case patterns
// and the reserved words "{" and "}".
static iz braceskip(Str s, iz i, i32 kind, i32 depth)
{
    b32      cmd     = kind==BRACE_CMD || kind==BRACE_FUNSUB;
    i32      parens  = 0;   // unmatched ( outside commands
    i32      braces  = 0;   // unmatched reserved { in a ${ cmd; }
    i32      cases   = 0;   // open case commands
    i32      casew   = 0;   // words seen since "case", until "in"
    b32      cmdpos  = 1;   // a command may start here
    b32      inpat   = 0;   // at a case pattern: only "esac" is reserved
    b32      fname   = 0;   // the next word names a function
    iz       word    = -1;  // start of the current word in a command
    BraceDoc docs[8] = {};  // here-documents whose bodies follow the line
    i32      ndocs   = 0;

    if (depth >= BRACE_MAXDEPTH) {
        return -1;
    }
    depth++;

    while (i < s.len) {
        u8 c = s[i];
        u8 n = braceat(s, i+1);
        u8 k = braceat(s, i+2);

        if (cmd) {
            if (bracebreak(c) && word>=0) {
                Str w = span(s.data+word, s.data+i);
                word = -1;
                if (casew) {
                    casew++;
                    if (casew>2 && w=="in") {
                        casew  = 0;
                        cmdpos = 1;
                        inpat  = 1;
                    }
                } else if (fname) {
                    fname  = 0;
                    cmdpos = 1;  // the body may be a { group
                } else if (cmdpos && !inpat && w=="function") {
                    fname  = 1;
                    cmdpos = 0;
                } else if (cmdpos && !inpat && w=="case") {
                    cases++;
                    casew  = 1;
                    cmdpos = 0;
                } else if (cmdpos && cases && w=="esac") {
                    cases--;
                    cmdpos = 0;
                    inpat  = 0;
                } else {
                    cmdpos = cmdpos && !inpat && bracecmdword(w);
                }
            } else if (!bracebreak(c) && word<0) {
                if (c == '#') {
                    for (; i<s.len && s[i]!='\n'; i++) {}
                    continue;
                }
                word = i;
            }
        }
        b32 reserved = cmd && word==i && cmdpos && !inpat;  // could be one
        b32 alone    = bracebreak(n) || i+1==s.len;         // ends the word

        switch (c) {
        case '\\':
            i += 2;
            continue;
        case '\'':
            for (i++; i<s.len && s[i]!='\''; i++) {}
            if (i == s.len) return -1;
            i++;
            continue;
        case '"':
            i = braceskipdq(s, i+1, depth);
            if (i < 0) return -1;
            continue;
        case '`':
            i = braceskipbq(s, i+1);
            if (i < 0) return -1;
            continue;
        case '$':
            if (n == '(') {
                i = braceskip(s, i+2, bracedolparen(k), depth);
                if (i < 0) return -1;
                continue;
            } else if (n == '{') {
                i = braceskip(s, i+2, bracedolbrace(k), depth);
                if (i < 0) return -1;
                continue;
            }
            break;
        case '<':
        case '>':
            if (n == '(') {
                i = braceskip(s, i+2, BRACE_CMD, depth);
                if (i < 0) return -1;
                continue;
            } else if (cmd && c=='<' && n=='<' && k=='<') {
                i += 3;  // here-string
                continue;
            } else if (cmd && c=='<' && n=='<') {
                BraceDoc d = {};
                i += 2;
                d.strip = braceat(s, i) == '-';
                i += d.strip;
                for (; i<s.len && (s[i]==' ' || s[i]=='\t'); i++) {}
                iz end = bracedocend(s, i);
                d.eof = span(s.data+i, s.data+end);
                if (ndocs < countof(docs)) {
                    docs[ndocs++] = d;
                }
                i = end;
                continue;
            }
            break;
        case '(':
            if (cmd && n=='(') {
                // An arithmetic command
                i = braceskip(s, i+2, BRACE_ARITH, depth);
                if (i < 0) return -1;
                i += braceat(s, i) == ')';
                cmdpos = 0;
                continue;
            } else if (cmd) {
                // A subshell or a case pattern gets its own state.
                i = braceskip(s, i+1, BRACE_CMD, depth);
                if (i < 0) return -1;
                cmdpos = 1;
                inpat  = 0;
                continue;
            }
            parens++;
            break;
        case ')':
            if (parens) {
                parens--;
            } else if (cmd && cases) {
                cmdpos = 1;  // end of a case pattern
                inpat  = 0;
            } else if (kind==BRACE_ARITH || kind==BRACE_CMD) {
                return i + 1;
            }
            break;
        case '{':
            // Only a whole word "{" opens a group. In ${...} only another
            // ${ nests, as handled above.
            braces += reserved && alone && kind==BRACE_FUNSUB;
            break;
        case '}':
            if (kind == BRACE_PARAM) {
                return i + 1;
            } else if (reserved && kind==BRACE_FUNSUB) {
                // A reserved "}" is a word by itself, even before "x".
                if (!braces) return i + 1;
                braces--;
                word = -1;
            }
            break;
        case '\n':
            cmdpos = 1;
            if (ndocs) {
                i = braceskipdocs(s, i+1, docs, ndocs);
                if (i < 0) return -1;
                ndocs = 0;
                continue;
            }
            break;
        case ';':
            cmdpos = 1;
            if (cases && (n==';' || n=='&')) {
                inpat = 1;  // ;; ;& or ;;& ends a case item
                i += 2;
                continue;
            }
            break;
        case '&': case '|':
            cmdpos = 1;
            break;
        }
        i++;
    }
    return -1;
}

// Scan for an unquoted, unnested delimiter from i: an opening brace, the
// closing brace of an expression whose body starts at i, or a comma
// separating list items. A closing brace counts only after a comma or
// "..", which also decides the kind of expression. An unterminated
// nested construct ends the search like the end of the text.
static BraceScan bracescan(Str s, iz i, i32 want)
{
    BraceScan r      = {};
    u8        target = want==BRACE_OPEN ? '{' : want==BRACE_CLOSE ? '}' : ',';
    i32       level  = 0;  // unmatched { and ${
    b32       armed  = want != BRACE_CLOSE;
    u8        quote  = 0;
    r.pos = s.len;

    while (i < s.len) {
        u8 c = s[i];
        u8 n = braceat(s, i+1);

        if (c=='\\' && quote!='\'') {
            i += 2;
            continue;
        }

        if (c=='$' && n=='{' && quote!='\'') {
            if (bracefunsub(braceat(s, i+2))) {
                i = braceskip(s, i+2, BRACE_FUNSUB, 0);
                if (i < 0) return r;
                continue;
            }
            level += !quote;
            i += 2;
            continue;
        }

        if (quote) {
            if (c == quote) {
                quote = 0;
            } else if (quote=='"' && c=='$' && n=='(') {
                i = braceskip(s, i+2, bracedolparen(braceat(s, i+2)), 0);
                if (i < 0) return r;
                continue;
            }
            i++;
            continue;
        }

        switch (c) {
        case '"': case '\'': case '`':
            quote = c;
            i++;
            continue;
        case '$': case '<': case '>':
            if (n == '(') {
                u8 k = braceat(s, i+2);
                i = braceskip(s, i+2, c=='$' ? bracedolparen(k) : BRACE_CMD, 0);
                if (i < 0) return r;
                continue;
            }
            break;
        }

        if (c==target && !level && armed) {
            if (c!='{' || !braceblank(braceat(s, i-1)) || !(braceblank(n) || n=='}')) {
                r.pos = i;
                r.ok  = 1;
                return r;
            }
        } else if (c == '{') {
            level++;
        } else if (c=='}' && level) {
            level--;
        } else if (want==BRACE_CLOSE && !level) {
            if (c == ',') {
                r.kind = BRACE_LIST;
                armed  = 1;
            } else if (c=='.' && n=='.' && braceat(s, i+2)!='}' && !r.kind) {
                r.kind = BRACE_SEQ;
                armed  = 1;
            }
        }
        i++;
    }
    return r;
}


// Sequences

enum : i32 {
    BRACE_BAD,
    BRACE_INT,
    BRACE_CHAR,
};

// Parse a decimal integer at i as strtoimax does: optional blanks, an
// optional sign, then digits.
static BraceInt braceint(Str s, iz i)
{
    BraceInt r = {};
    r.end = i;
    for (; i<s.len && whitespace(s[i]); i++) {}
    b32 neg = 0;
    if (i<s.len && (s[i]=='+' || s[i]=='-')) {
        neg = s[i++] == '-';
    }
    if (i==s.len || !digit(s[i])) {
        return r;
    }
    i64 min  = -0x7fffffffffffffff - 1;
    i64 v    = 0;  // accumulated negatively to reach the minimum
    b32 over = 0;
    for (; i<s.len && digit(s[i]); i++) {
        i32 d = s[i] - '0';
        if (v < (min + d)/10) {
            over = 1;
        } else {
            v = v*10 - d;
        }
    }
    if (!neg && !over) {
        over = v == min;
        v    = over ? 0 : -v;
    }
    r.value = over ? 0 : v;
    r.end   = i;
    r.ok    = !over;
    return r;
}

static b32 braceintstart(Str s)  // a digit, or a sign and a digit
{
    u8 c = braceat(s, 0);
    return digit(c) || ((c=='+' || c=='-') && digit(braceat(s, 1)));
}

// The loose check made while looking for an expression. The text after
// the body is always "}".
static b32 braceseqok(Str body)
{
    iz dots = find(body, "..");
    if (dots < 0) {
        return 0;
    }
    Str lhs = takehead(body, dots);
    Str rhs = cuthead(body, dots+2);
    if (!lhs.len || !rhs.len) {
        return 0;
    }

    u8  l1 = lhs.len>1 ? lhs[1] : '.';
    u8  r1 = rhs.len>1 ? rhs[1] : '}';
    i32 lt = braceintstart(lhs)        ? BRACE_INT  :
             letter(lhs[0]) && l1=='.' ? BRACE_CHAR :
                                         BRACE_BAD;
    i32 rt = braceintstart(rhs)                     ? BRACE_INT  :
             letter(rhs[0]) && (r1=='}' || r1=='.') ? BRACE_CHAR :
                                                      BRACE_BAD;
    return lt!=BRACE_BAD && lt==rt;
}

static b32 bracezeroled(Str s)  // asks for zero padding
{
    return (s.len>1 && s[0]=='0') || (s.len>2 && s[0]=='-' && s[1]=='0');
}

static Str braceformat(Arena *a, i64 v, iz width)
{
    u8  tmp[32];
    u8 *end = tmp + countof(tmp);
    u8 *beg = end;
    i64 t   = v<0 ? v : -v;
    do {
        *--beg = (u8)('0' - t%10);
    } while (t /= 10);

    iz  digits = end - beg;
    iz  sign   = v < 0;
    iz  zeros  = width>digits+sign ? width-digits-sign : 0;
    Str r      = {};
    r.len  = sign + zeros + digits;
    r.data = allocbytes(a, r.len);
    if (sign) r.data[0] = '-';
    for (iz i = 0; i < zeros; i++) {
        r.data[sign+i] = '0';
    }
    copybytes(r.data+sign+zeros, beg, digits);
    return r;
}

// Expand a sequence expression's body. Returns an empty slice when the
// body does not parse, its numbers overflow, or it is too long to fit.
static Slice<Str> braceseq(Arena *a, Str body)
{
    Slice<Str> r = {};

    iz dots = find(body, "..");
    if (dots < 0) {
        return r;
    }
    Str lhs = takehead(body, dots);
    Str rhs = cuthead(body, dots+2);
    if (!lhs.len || !rhs.len) {
        return r;
    }

    BraceInt lv = braceint(lhs, 0);
    iz       le = lv.end;
    for (; le<lhs.len && whitespace(lhs[le]); le++) {}
    i32 lt = lv.ok && le==lhs.len         ? BRACE_INT  :
             letter(lhs[0]) && lhs.len==1 ? BRACE_CHAR :
                                            BRACE_BAD;

    BraceInt rv   = {};
    i32      rt   = BRACE_BAD;
    iz       rlen = 0;  // length of the end term, without the increment
    if (braceintstart(rhs)) {
        rv   = braceint(rhs, 0);
        rlen = rv.end;
        b32 tail = rv.end==rhs.len || rhs[rv.end]=='.';
        rt = rv.ok && tail ? BRACE_INT : BRACE_BAD;
    } else if (letter(rhs[0]) && (rhs.len==1 || rhs[1]=='.')) {
        rlen = 1;
        rt   = BRACE_CHAR;
    }

    i64 incr = 1;
    if (rt != BRACE_BAD) {
        Str rest = cuthead(rhs, rlen);
        if (rest.len>2 && rest[0]=='.' && rest[1]=='.') {
            BraceInt iv = braceint(rest, 2);
            if (!iv.ok || iv.end!=rest.len) {
                rt = BRACE_BAD;
            }
            incr = iv.value;
        } else if (rest.len) {
            rt = BRACE_BAD;
        }
    }

    if (lt==BRACE_BAD || lt!=rt) {
        return r;
    }

    i64 lo    = lv.value;
    i64 hi    = rv.value;
    iz  width = 0;
    if (lt == BRACE_CHAR) {
        lo = lhs[0];
        hi = rhs[0];
    } else if (bracezeroled(lhs) || bracezeroled(takehead(rhs, rlen))) {
        width = lhs.len>rlen ? lhs.len : rlen;
    }

    // Normalize the step: its sign follows the direction of the range.
    if (incr == 0) {
        incr = 1;
    } else if (incr == -0x7fffffffffffffff - 1) {
        return r;
    }
    u64 step = (u64)(incr<0 ? -incr : incr);
    incr = lo<hi ? (i64)step : -(i64)step;
    u64 dist = lo<hi ? (u64)hi-(u64)lo : (u64)lo-(u64)hi;
    if (dist > (u64)0x7fffffffffffffff) {
        return r;
    }
    u64 count = dist/step + 1;

    // Like a failed allocation in bash, a sequence that cannot fit leaves
    // the expression as written instead of exhausting memory.
    Mem *m     = a->mem;
    iz   avail = (m ? m->limit : a->end) - a->beg;
    iz   each  = 2*(iz)sizeof(Str) + (width>21 ? width : 21);
    if (count > (u64)(avail/each)) {
        return r;
    }

    r.data = alloc<Str>(a, (iz)count);
    r.len  = r.cap = (iz)count;
    i64 v  = lo;
    for (iz i = 0; i < r.len; i++) {
        if (lt == BRACE_CHAR) {
            r.data[i] = {allocbytes(a, 1), 1};
            r.data[i].data[0] = (u8)v;
        } else {
            r.data[i] = braceformat(a, v, width);
        }
        if (i+1 < r.len) v += incr;
    }
    return r;
}


// Expansion

static Str bracejoin(Arena *a, Str x, Str y)
{
    Str r = {};
    r.len  = x.len + y.len;
    r.data = allocbytes(a, r.len);
    copybytes(r.data, x.data, x.len);
    copybytes(r.data+x.len, y.data, y.len);
    return r;
}

static Slice<Str> braceword(Arena *, Str, i32 depth);

// Expand the body of a list: each comma-separated item in turn.
static Slice<Str> bracelist(Arena *a, Str body, i32 depth)
{
    Slice<Str> r = {};
    for (iz i = 0;;) {
        BraceScan  sep  = bracescan(body, i, BRACE_COMMA);
        iz         end  = sep.ok ? sep.pos : body.len;
        Slice<Str> item = braceword(a, span(body.data+i, body.data+end), depth);
        for (iz j = 0; j < item.len; j++) {
            r = push(a, r, item[j]);
        }
        if (!sep.ok) {
            return r;
        }
        i = end + 1;
    }
}

static Slice<Str> braceword(Arena *a, Str s, i32 depth)
{
    Slice<Str> r = {};
    r = push(a, r, Str{});
    if (depth >= BRACE_MAXDEPTH) {
        r[0] = s;
        return r;
    }

    for (b32 first = 1;; first = 0) {
        // Find the first valid brace expression.
        BraceScan open  = {};
        BraceScan close = {};
        for (iz i = 0;; i = open.pos + 1) {
            open = bracescan(s, i, BRACE_OPEN);
            if (!open.ok) {
                break;
            }
            close = bracescan(s, open.pos+1, BRACE_CLOSE);
            if (close.ok) {
                Str body = span(s.data+open.pos+1, s.data+close.pos);
                if (close.kind!=BRACE_SEQ || braceseqok(body)) {
                    break;
                }
            }
        }

        if (!open.ok) {
            if (first) {
                r[0] = s;
                return r;
            }
            for (iz i = 0; s.len && i<r.len; i++) {
                r[i] = bracejoin(a, r[i], s);
            }
            return r;
        }

        Str        pre  = takehead(s, open.pos);
        Str        body = span(s.data+open.pos+1, s.data+close.pos);
        Slice<Str> mid  = {};
        if (close.kind == BRACE_SEQ) {
            mid = braceseq(a, body);
            if (!mid.len) {
                // Left as written, braces included
                Str whole = span(s.data+open.pos, s.data+close.pos+1);
                mid = push(a, mid, whole);
            }
        } else {
            mid = bracelist(a, body, depth+1);
        }

        if (r.len > (iz)(~(uz)0>>1)/mid.len) {
            os_oom();  // where bash would fail to allocate
        }
        Slice<Str> next = {};
        next.cap  = r.len * mid.len;
        next.data = alloc<Str>(a, next.cap);
        for (iz i = 0; i < r.len; i++) {
            Str head = pre.len ? bracejoin(a, r[i], pre) : r[i];
            for (iz j = 0; j < mid.len; j++) {
                next.data[next.len++] = bracejoin(a, head, mid[j]);
            }
        }
        r = next;
        s = cuthead(s, close.pos+1);
    }
}

// Expand braces in a word's text as the parser stores it: quotes and
// backslashes are still present, and $'...' has already been rewritten
// to '...'. Returns the words in order; a word without valid brace
// expressions returns itself as a single element.
static Slice<Str> braceexpand(Arena *a, Str word)
{
    return braceword(a, word, 0);
}

// Cheap test: could this word contain a brace expansion? It needs a "{"
// followed by a "}" and by a comma or "..".
static b32 hasbrace(Str word)
{
    iz i = 0;
    for (; i<word.len && word[i]!='{'; i++) {}
    b32 close = 0;
    b32 sep   = 0;
    for (i++; i < word.len; i++) {
        u8 c = word[i];
        close |= c == '}';
        sep   |= c==',' || (c=='.' && braceat(word, i+1)=='.');
    }
    return close && sep;
}
