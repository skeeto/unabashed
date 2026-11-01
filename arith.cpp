// Arithmetic evaluation: $((...)), ((...)), let, subscripts, declare -i
//
// A direct recursive-descent evaluator over the expression text. The
// tokenizer follows bash's: identifiers are evaluated as they are read,
// unless the next token is "=", and reading one looks ahead past it, so
// errors surface in the same order and with the same error token. A
// variable's value is itself an expression, evaluated recursively up to
// bash's limit of 1024 levels. Errors are sticky: the first one is
// recorded, the token stream ends, and no further variable is read or
// written. Nothing is allocated except error messages.
//
// This is free and unencumbered software released into the public domain.

// A variable, or a[sub] with sub the raw text between the brackets. The
// get callback may rewrite sub to a canonical form (an evaluated index)
// which is then passed back to set, so that a subscript like a[i++] in
// a[i++]+=1 or a[i++]++ is evaluated once, as in bash.
struct ArithRef {
    Str name;
    Str sub;
    b32 hassub;
};

struct ArithExpr;

struct ArithEnv {
    void *ctx;
    // Current value as a string; set *isset=0 if the variable does not
    // exist (for a[sub], the array itself). The core handles arrays and
    // evaluates subscripts, through arith with this same env, so that
    // the recursion and nesting limits span them. Under set -u, bash
    // reports a missing array before evaluating its subscript.
    Str  (*get)(void *ctx, ArithRef *, b32 *isset, Arena *);
    // Assign a value; return an error message (no prefix) or a null Str.
    Str  (*set)(void *ctx, ArithRef *, i64 value, Arena *);
    b32  nounset;  // set -u: an unset variable is an error

    // Maintained by arith, so that calls nested in a callback continue
    // the recursion count; zero-initialize them.
    i32        depth;
    ArithExpr *cur;
};

struct Arith {
    i64 value;
    Str err;       // message after bash's "NAME: line N: " (and "let: ")
    b32 ok;
    b32 unbound;   // err is "NAME: unbound variable", fatal under set -u
};

enum : i32 {
    ARITH_MAXDEPTH = 1024,  // bash's MAX_EXPR_RECURSION_LEVEL
    ARITH_MAXNEST  = 1024,  // nesting, summed over an evaluation, to bound stack use
};

enum : i32 {  // tokens; single characters stand for themselves
    AR_END,
    AR_EQEQ,
    AR_NEQ,
    AR_LEQ,
    AR_GEQ,
    AR_STR,
    AR_NUM,
    AR_LAND,
    AR_LOR,
    AR_LSH,
    AR_RSH,
    AR_OPASSIGN,
    AR_COND,
    AR_POWER,
    AR_PREINC,
    AR_PREDEC,
    AR_POSTINC,
    AR_POSTDEC,
};

struct ArithCtx {  // shared by an evaluation and its nested expressions
    ArithEnv *env;
    Arena    *a;
    Str       err;
    b32       unbound;
    i32       nest;
};

struct ArithExpr {  // one expression being evaluated
    ArithCtx *c;
    u8       *beg;
    u8       *end;
    u8       *tp;        // read position
    u8       *lasttp;    // start of the current token, for messages
    i32       curtok;
    i32       lasttok;
    i32       assigntok; // operator of an AR_OPASSIGN
    i32       noeval;    // nonzero: parse only, no reads or writes
    i64       tokval;    // value of an AR_NUM or AR_STR
    ArithRef  tokref;    // the last identifier read
    b32       hastok;
};

static u8 arithat(ArithExpr *x, u8 *p)
{
    return p<x->end ? *p : 0;  // a NUL ends the text, as in C
}

static b32 arithblank(u8 c)
{
    return c==' ' || c=='\t' || c=='\n';
}

static b32 arithvarstart(u8 c)
{
    return letter(c) || c=='_';
}

static b32 arithvarchar(u8 c)
{
    return alnum(c) || c=='_';
}

static b32 arithnumchar(u8 c)
{
    return alnum(c) || c=='_' || c=='@';
}

// Text from p up to the end or the first NUL.
static Str arithcstr(u8 *p, u8 *end)
{
    u8 *e = p;
    for (; e<end && *e; e++) {}
    return span(p, e);
}

// Record bash's message for an error at the current token of x.
static void arithmessage(ArithCtx *c, ArithExpr *x, Str msg)
{
    if (c->err.data) {
        return;
    }
    Str expr = arithcstr(x->beg, x->end);
    for (; expr.len && (*expr.data==' ' || *expr.data=='\t'); expr = cuthead(expr, 1)) {}
    Str tok = arithcstr(x->lasttp, x->end);
    Buf b(c->a, expr.len + msg.len + tok.len + 24);
    print(&b, expr);
    print(&b, ": ");
    print(&b, msg);
    print(&b, " (error token is \"");
    print(&b, tok);
    print(&b, "\")");
    c->err = finish(&b);
}

static void aritherror(ArithExpr *x, Str msg)
{
    x->curtok = AR_END;
    arithmessage(x->c, x, msg);
}

static b32 arithenter(ArithExpr *x)
{
    if (++x->c->nest > ARITH_MAXNEST) {
        aritherror(x, "expression recursion level exceeded");
        return 0;
    }
    return 1;
}

static void arithleave(ArithExpr *x)
{
    x->c->nest--;
}


// Tokens

// Convert a number token: decimal, 0 octal, 0x hex, or base#digits.
static i64 arithstrlong(ArithExpr *x, Str s)
{
    iz  i         = 0;
    i64 base      = 10;
    b32 foundbase = 0;
    if (s[0] == '0') {
        if (++i == s.len) {
            return 0;
        }
        if (s[i]=='x' || s[i]=='X') {
            base = 16;
            i++;
        } else {
            base = 8;
        }
        foundbase = 1;
    }

    u64 val = 0;
    for (; i < s.len; i++) {
        u8 c = s[i];
        if (c == '#') {
            if (foundbase) {
                aritherror(x, "invalid number");
                return 0;
            }
            if ((i64)val<2 || (i64)val>64) {
                aritherror(x, "invalid arithmetic base");
                return 0;
            }
            base      = (i64)val;
            val       = 0;
            foundbase = 1;
            if (i+1==s.len || !arithnumchar(s[i+1])) {
                aritherror(x, "invalid integer constant");
                return 0;
            }
            continue;
        }
        i64 d = 0;
        if (digit(c)) {
            d = c - '0';
        } else if (c>='a' && c<='z') {
            d = c - 'a' + 10;
        } else if (c>='A' && c<='Z') {
            d = c - 'A' + (base<=36 ? 10 : 36);
        } else if (c == '@') {
            d = 62;
        } else {
            d = 63;  // _
        }
        if (d >= base) {
            aritherror(x, "value too great for base");
            return 0;
        }
        val = val*(u64)base + (u64)d;
    }
    return (i64)val;
}

// Read the number token at p, returning its end. Bash terminates the
// token in place while converting it, so messages end with the token.
static u8 *arithnumber(ArithExpr *x, u8 *p, i64 *val)
{
    u8 *e = p;
    for (u8 c; (c = arithat(x, e)) && (arithnumchar(c) || c=='#'); e++) {}
    u8 *end = x->end;
    x->end = e;
    *val   = arithstrlong(x, span(p, e));
    x->end = end;
    return e;
}

// Subscript skipping follows the shell's quoting rules, as bash's does:
// quotes, escapes, backquotes, $(...), ${...} and nested brackets. The
// modes are the construct being skipped, and the states track whether
// a ${...} is still in its parameter name, where a '[' opens a subscript.
// Command substitutions are only matched for parentheses, as bash does
// outside of a full parse.
enum : i32 { AR_SKSUB, AR_SKDQ, AR_SKCMD, AR_SKBRACE };
enum : i32 { AR_DBPARAM, AR_DBOP, AR_DBWORD, AR_DBQUOTE, AR_DBQUOTE2 };
enum : i32 { AR_SKMAXDEPTH = 64 };

static b32 arithdbop(u8 c)
{
    switch (c) {
    case '#': case '%': case '^': case ',': case '~': case ':': case '-':
    case '=': case '?': case '+': case '/':
        return 1;
    }
    return 0;
}

// From p, just inside an opener, return the matching closer or the end
// of the text. For AR_SKBRACE, start is just inside the outermost "${".
static u8 *arithskip(ArithExpr *x, u8 *p, i32 mode, i32 state, u8 *start, i32 depth)
{
    Str rest = arithcstr(p, x->end);
    u8 *eot  = rest.data + rest.len;
    if (depth > AR_SKMAXDEPTH) {
        return eot;  // treat as unterminated
    }

    i32 count = 1;  // brackets, for AR_SKSUB
    for (;;) {
        u8 c  = arithat(x, p);
        u8 c1 = arithat(x, p+1);
        b32 cmd = mode==AR_SKCMD || mode==AR_SKBRACE;
        if (!c) {
            return p;

        } else if (c=='\\' || (cmd && c==1)) {  // 1 is bash's CTLESC
            p += 1 + (c1 != 0);
            continue;

        } else if (mode==AR_SKCMD && c=='#' &&
                   (p[-1]=='\n' || p[-1]==' ' || p[-1]=='\t')) {
            for (; arithat(x, p) && *p!='\n'; p++) {}
            continue;

        } else if (c == '`') {
            for (p++; arithat(x, p) && *p!='`'; p++) {
                p += *p=='\\' && arithat(x, p+1);
            }
            if (!arithat(x, p)) {
                return eot;
            }
            p++;
            continue;

        } else if (c=='\'' && mode!=AR_SKDQ) {
            for (p++; arithat(x, p) && *p!='\''; p++) {}
            if (!arithat(x, p)) {
                return eot;
            }
            p++;
            continue;

        } else if (c == '"') {
            if (mode == AR_SKDQ) {
                return p;
            }
            p = arithskip(x, p+1, AR_SKDQ, 0, 0, depth+1);
            if (!arithat(x, p)) {
                return eot;
            }
            p++;
            continue;

        } else if (c=='$' && c1=='(') {
            p = arithskip(x, p+2, AR_SKCMD, 0, 0, depth+1);
            if (!arithat(x, p)) {
                return eot;
            }
            p++;
            continue;

        } else if (c=='$' && c1=='{') {
            u8 c2 = arithat(x, p+2);
            b32 funsub = c2==' ' || c2=='\t' || c2=='\n' || c2=='|';
            if (mode!=AR_SKCMD || funsub) {
                i32 inner = AR_DBPARAM;
                u8 *beg   = p + 2;
                if (mode == AR_SKBRACE) {
                    b32 reset = state==AR_DBQUOTE || state==AR_DBWORD;
                    inner = reset ? AR_DBPARAM : state;
                    beg   = start;
                }
                p = arithskip(x, p+2, AR_SKBRACE, inner, beg, depth+1);
                if (!arithat(x, p)) {
                    return eot;
                }
                p++;
                continue;
            }

        } else if (mode==AR_SKBRACE && (c=='<' || c=='>') && c1=='(') {
            p = arithskip(x, p+2, AR_SKCMD, 0, 0, depth+1);
            if (!arithat(x, p)) {
                return eot;
            }
            p++;
            continue;

        } else if (mode==AR_SKCMD && c=='(') {
            p = arithskip(x, p+1, AR_SKCMD, 0, 0, depth+1);
            if (!arithat(x, p)) {
                return eot;
            }
            p++;
            continue;

        } else if ((mode==AR_SKCMD && c==')') || (mode==AR_SKBRACE && c=='}')) {
            return p;

        } else if (mode==AR_SKSUB && c=='[') {
            count++;

        } else if (mode==AR_SKSUB && c==']') {
            if (!--count) {
                return p;
            }

        } else if (mode==AR_SKBRACE && c=='[' && state==AR_DBPARAM) {
            u8 *e = arithskip(x, p+1, AR_SKSUB, 0, 0, depth+1);
            if (!arithat(x, e)) {
                return eot;
            }
            p = e;
            c = ']';
        }

        p++;
        if (mode == AR_SKBRACE) {
            b32 first = p - start == 1;
            if (state==AR_DBPARAM && !first &&
                (c=='%' || c=='#' || c=='^' || c==',')) {
                state = AR_DBQUOTE;
            } else if (state==AR_DBPARAM && !first && c=='/') {
                state = AR_DBQUOTE2;
            } else if (state==AR_DBPARAM && arithdbop(c)) {
                state = AR_DBOP;
            } else if (state==AR_DBOP && !arithdbop(c)) {
                state = AR_DBWORD;
            }
        }
    }
}

// Find the ']' closing the subscript opened at p, or the end of the text.
static u8 *arithskipsub(ArithExpr *x, u8 *p)
{
    return arithskip(x, p+1, AR_SKSUB, 0, 0, 0);
}

// Scan an identifier, with its subscript, starting at p, into the
// token reference unless only peeking.
static u8 *arithscanident(ArithExpr *x, u8 *p, b32 peek)
{
    u8 *beg = p;
    for (p++; arithvarchar(arithat(x, p)); p++) {}
    if (!peek) {
        x->tokref = {};
        x->tokref.name = span(beg, p);
    }
    if (arithat(x, p) == '[') {
        u8 *e = arithskipsub(x, p);
        if (arithat(x, e) != ']') {
            aritherror(x, "bad array subscript");
            return p;
        }
        if (!peek) {
            x->tokref.sub    = span(p+1, e);
            x->tokref.hassub = 1;
        }
        p = e + 1;
    }
    return p;
}

static b32 arithisop(i32 c)
{
    switch (c) {
    case '=': case '>': case '<': case '+': case '-': case '*': case '/':
    case '%': case '!': case '(': case ')': case '&': case '|': case '^':
    case '~': case '?': case ':': case ',':
        return 1;
    }
    return 0;
}

static b32 arithmultiop(i32 t)
{
    switch (t) {
    case AR_EQEQ: case AR_NEQ: case AR_LEQ: case AR_GEQ: case AR_LAND:
    case AR_LOR: case AR_LSH: case AR_RSH: case AR_OPASSIGN: case AR_COND:
    case AR_POWER: case AR_PREINC: case AR_PREDEC: case AR_POSTINC:
    case AR_POSTDEC:
        return 1;
    }
    return 0;
}

// Read an operator at *pp given the two previous tokens, advancing *pp.
static i32 arithlexop(ArithExpr *x, u8 **pp, i32 curtok, i32 lasttok)
{
    u8 *p  = *pp;
    u8  c  = *p++;
    u8  c1 = arithat(x, p);
    i32 t  = c;
    if (c=='=' && c1=='=') {
        t = AR_EQEQ;
        p++;
    } else if (c=='!' && c1=='=') {
        t = AR_NEQ;
        p++;
    } else if (c=='>' && c1=='=') {
        t = AR_GEQ;
        p++;
    } else if (c=='<' && c1=='=') {
        t = AR_LEQ;
        p++;
    } else if ((c=='<' || c=='>') && c1==c) {
        t = c=='<' ? AR_LSH : AR_RSH;
        p++;
        if (arithat(x, p) == '=') {
            x->assigntok = t;
            t = AR_OPASSIGN;
            p++;
        }
    } else if (c=='&' && c1=='&') {
        t = AR_LAND;
        p++;
    } else if (c=='|' && c1=='|') {
        t = AR_LOR;
        p++;
    } else if (c=='*' && c1=='*') {
        t = AR_POWER;
        p++;
    } else if ((c=='-' || c=='+') && c1==c && curtok==AR_STR) {
        t = c=='-' ? AR_POSTDEC : AR_POSTINC;
        p++;
    } else if ((c=='-' || c=='+') && c1==c && curtok==AR_NUM &&
               (lasttok==AR_PREINC || lasttok==AR_PREDEC)) {
        aritherror(x, c=='-' ? Str("--: assignment requires lvalue")
                            : Str("++: assignment requires lvalue"));
        return AR_END;
    } else if ((c=='-' || c=='+') && c1==c) {
        u8 *q = p + 1;
        for (; arithblank(arithat(x, q)); q++) {}
        if (arithvarstart(arithat(x, q))) {
            t = c=='-' ? AR_PREDEC : AR_PREINC;
            p++;
        }
    } else if (c1=='=' && (c=='*' || c=='/' || c=='%' || c=='+' ||
                           c=='-' || c=='&' || c=='^' || c=='|')) {
        x->assigntok = c;
        t = AR_OPASSIGN;
        p++;
    } else if (!arithisop(c)) {
        if (!curtok || arithisop(curtok) || arithmultiop(curtok)) {
            aritherror(x, "arithmetic syntax error: operand expected");
        } else {
            aritherror(x, "arithmetic syntax error: invalid arithmetic operator");
        }
        return AR_END;
    }
    *pp = p;
    return t;
}

// The token after an identifier, read without evaluation. Like bash,
// it reads through a run of identifiers, so that a bad token after
// them is reported first.
static i32 arithpeek(ArithExpr *x, u8 *p)
{
    u8 *lasttp = x->lasttp;
    i32 first  = -1;
    i32 t      = AR_END;
    for (;;) {
        u8 c;
        for (; (c = arithat(x, p)) && arithblank(c); p++) {}
        if (!c) {
            break;
        }
        x->lasttp = p;
        if (arithvarstart(c)) {
            p = arithscanident(x, p, 1);
            if (x->c->err.data) {
                return AR_END;
            }
            first = first<0 ? AR_STR : first;
            continue;
        } else if (digit(c)) {
            i64 v;
            arithnumber(x, p, &v);
            t = AR_NUM;
        } else {
            t = arithlexop(x, &p, AR_STR, AR_STR);
        }
        break;
    }
    x->lasttp = lasttp;
    return first<0 ? t : first;
}

static i64 arithevalstr(ArithCtx *, Str);

// Evaluate a variable's value as an expression.
static i64 arithevalident(ArithExpr *x, ArithRef *ref)
{
    ArithCtx *c = x->c;
    if (x->noeval || c->err.data) {
        return 0;
    }
    ArithEnv *env = c->env;
    b32 isset = 1;
    Str v = env->get(env->ctx, ref, &isset, c->a);
    if (!isset && env->nounset) {
        c->err     = concat(c->a, ref->name, ": unbound variable");
        c->unbound = 1;
        x->curtok  = AR_END;
        return 0;
    }

    // Fast path for a plain decimal value
    b32 neg = v.len && v[0]=='-';
    iz  i   = neg;
    if (i<v.len && env->depth<ARITH_MAXDEPTH && (v[i]!='0' || i+1==v.len)) {
        u64 r = 0;
        for (; i<v.len && digit(v[i]); i++) {
            r = r*10 + (v[i] - '0');
        }
        if (i == v.len) {
            return (i64)(neg ? -r : r);
        }
    }
    return arithevalstr(c, v);
}

static void arithreadtok(ArithExpr *x)
{
    if (x->c->err.data) {
        x->lasttok = x->curtok;
        x->curtok  = AR_END;
        return;
    }

    u8 *p = x->tp;
    u8  c = 0;
    for (; (c = arithat(x, p)) && arithblank(c); p++) {}
    if (!c) {
        x->lasttok = x->curtok;
        x->curtok  = AR_END;
        x->tp      = p;
        return;
    }
    x->lasttp = x->tp = p;

    if (arithvarstart(c)) {
        p = arithscanident(x, p, 0);
        x->hastok = 1;
        if (x->c->err.data) {
            return;
        }
        i32 peek = arithpeek(x, p);
        if (x->c->err.data) {
            return;
        }
        if (x->lasttok==AR_PREINC || x->lasttok==AR_PREDEC || peek!='=') {
            x->tokval = arithevalident(x, &x->tokref);
        } else {
            x->tokval = 0;
        }
        x->lasttok = x->curtok;
        x->curtok  = AR_STR;

    } else if (digit(c)) {
        p = arithnumber(x, p, &x->tokval);
        if (x->c->err.data) {
            return;
        }
        x->lasttok = x->curtok;
        x->curtok  = AR_NUM;

    } else {
        i32 t = arithlexop(x, &p, x->curtok, x->lasttok);
        if (x->c->err.data) {
            return;
        }
        x->lasttok = x->curtok;
        x->curtok  = t;
    }
    x->tp = p;
}


// Evaluation

static void arithbind(ArithExpr *x, ArithRef *ref, i64 v)
{
    ArithCtx *c   = x->c;
    ArithEnv *env = c->env;
    if (c->err.data) {
        return;
    }
    Str err = env->set(env->ctx, ref, v, c->a);
    if (err.data) {
        c->err    = err;
        x->curtok = AR_END;
    }
}

static i64 arithcomma(ArithExpr *);

static i64 arithprimary(ArithExpr *x)
{
    i64 val = 0;
    if (x->curtok==AR_PREINC || x->curtok==AR_PREDEC) {
        i32 stok = x->lasttok = x->curtok;
        arithreadtok(x);
        if (x->curtok != AR_STR) {
            aritherror(x, "identifier expected after pre-increment or pre-decrement");
            return 0;
        }
        val = (i64)((u64)x->tokval + (stok==AR_PREINC ? 1 : -1));
        if (!x->noeval && x->hastok) {
            arithbind(x, &x->tokref, val);
        }
        x->curtok = AR_NUM;  // so --x=7 is an error
        arithreadtok(x);

    } else if (x->curtok == '(') {
        arithreadtok(x);
        if (!arithenter(x)) {
            return 0;
        }
        val = arithcomma(x);
        arithleave(x);
        if (x->curtok != ')') {
            aritherror(x, "missing `)'");
            return 0;
        }
        arithreadtok(x);

    } else if (x->curtok == AR_NUM) {
        val = x->tokval;
        arithreadtok(x);

    } else if (x->curtok == AR_STR) {
        // Bash reads the next token without evaluation to look for a
        // postfix operator, then reads it again. After an identifier,
        // a doubled + or - is always postfix, and any other token
        // reads the same way twice, so only check for those.
        val = x->tokval;
        u8 *p = x->tp;
        for (; arithblank(arithat(x, p)); p++) {}
        u8 c = arithat(x, p);
        if ((c=='+' || c=='-') && arithat(x, p+1)==c) {
            x->lasttp  = p;
            x->tp      = p + 2;
            x->lasttok = AR_STR;
            i64 v2 = (i64)((u64)val + (c=='+' ? 1 : -1));
            if (!x->noeval) {
                arithbind(x, &x->tokref, v2);
            }
            x->curtok = AR_NUM;  // so x++=7 is an error
        }
        arithreadtok(x);

    } else {
        aritherror(x, "arithmetic syntax error: operand expected");
    }
    return val;
}

static b32 arithunaryop(i32 t)
{
    return t=='!' || t=='~' || t=='-' || t=='+';
}

static i64 arithunary(ArithExpr *x)
{
    u8  ops[32];
    i32 n = 0;
    for (; n<countof(ops) && arithunaryop(x->curtok); n++) {
        ops[n] = (u8)x->curtok;
        arithreadtok(x);
    }

    i64 val = 0;
    if (n==countof(ops) && arithunaryop(x->curtok)) {
        if (!arithenter(x)) {
            return 0;
        }
        val = arithunary(x);
        arithleave(x);
    } else {
        val = arithprimary(x);
    }

    while (n) {
        switch (ops[--n]) {
        case '!': val = !val;           break;
        case '~': val = ~val;           break;
        case '-': val = (i64)-(u64)val; break;
        case '+':                       break;
        }
        x->lasttok = AR_NUM;
    }
    return val;
}

static i32 arithprec(i32 t)
{
    switch (t) {
    case AR_LOR:                    return 1;
    case AR_LAND:                   return 2;
    case '|':                       return 3;
    case '^':                       return 4;
    case '&':                       return 5;
    case AR_EQEQ: case AR_NEQ:      return 6;
    case AR_LEQ:  case AR_GEQ:
    case '<':     case '>':         return 7;
    case AR_LSH:  case AR_RSH:      return 8;
    case '+':     case '-':         return 9;
    case '*':     case '/':
    case '%':                       return 10;
    case AR_POWER:                  return 11;
    }
    return 0;
}

// Raise to a non-negative power, wrapping.
static i64 arithpow(i64 base, i64 exp)
{
    u64 r = 1;
    u64 b = (u64)base;
    for (; exp; exp >>= 1) {
        if (exp & 1) {
            r *= b;
        }
        b *= b;
    }
    return (i64)r;
}

static i64 arithshift(i32 op, i64 a, i64 b)
{
    i32 n = (i32)(b & 63);  // as x86-64 shifts
    return op==AR_LSH ? (i64)((u64)a << n) : a >> n;
}

// Division and remainder with bash's rules for INTMAX_MIN and -1.
static i64 arithdiv(i32 op, i64 a, i64 b)
{
    if (b==-1 && a==(i64)((u64)1<<63)) {
        return op=='/' ? a : 0;
    }
    return op=='/' ? a/b : a%b;
}

// Binary operators from || down to **, by precedence climbing.
static i64 arithbinary(ArithExpr *x, i32 minprec)
{
    i64 val = arithunary(x);
    for (;;) {
        i32 op   = x->curtok;
        i32 prec = arithprec(op);
        if (!prec || prec<minprec) {
            return val;
        }

        b32 skip = (op==AR_LAND && !val) || (op==AR_LOR && val);
        u8 *stp  = x->tp;
        x->noeval += skip;
        arithreadtok(x);
        i64 rhs = 0;
        if (op == AR_POWER) {
            if (!arithenter(x)) {
                return 0;
            }
            rhs = arithbinary(x, prec);  // right associative
            arithleave(x);
        } else {
            rhs = arithbinary(x, prec+1);
        }
        x->noeval -= skip;
        x->lasttok = AR_NUM;

        switch (op) {
        case AR_LOR:  val = val || rhs; x->lasttok = AR_LOR;  break;
        case AR_LAND: val = val && rhs; x->lasttok = AR_LAND; break;
        case '|':     val |= rhs;                             break;
        case '^':     val ^= rhs;                             break;
        case '&':     val &= rhs;                             break;
        case AR_EQEQ: val = val == rhs;                       break;
        case AR_NEQ:  val = val != rhs;                       break;
        case AR_LEQ:  val = val <= rhs;                       break;
        case AR_GEQ:  val = val >= rhs;                       break;
        case '<':     val = val <  rhs;                       break;
        case '>':     val = val >  rhs;                       break;
        case AR_LSH:
        case AR_RSH:  val = arithshift(op, val, rhs);         break;
        case '+':     val = (i64)((u64)val + (u64)rhs);       break;
        case '-':     val = (i64)((u64)val - (u64)rhs);       break;
        case '*':     val = (i64)((u64)val * (u64)rhs);       break;
        case '/':
        case '%':
            if (!rhs) {
                if (!x->noeval) {
                    u8 *lasttp = x->lasttp;
                    for (x->lasttp = stp;
                         arithat(x, x->lasttp)==' ' || arithat(x, x->lasttp)=='\t';
                         x->lasttp++) {}
                    aritherror(x, "division by 0");
                    x->lasttp = lasttp;
                    return 0;
                }
                rhs = 1;
            }
            val = arithdiv(op, val, rhs);
            break;
        case AR_POWER:
            if (x->noeval) {
                val = 1;
            } else if (rhs < 0) {
                aritherror(x, "exponent less than 0");
                return 0;
            } else {
                val = arithpow(val, rhs);
            }
            break;
        }
    }
}

static i64 arithcond(ArithExpr *x)
{
    i64 cval = arithbinary(x, 1);
    if (x->curtok != '?') {
        return cval;
    }

    b32 skip = !cval;
    x->noeval += skip;
    arithreadtok(x);
    if (x->curtok==AR_END || x->curtok==':') {
        aritherror(x, "expression expected");
        return 0;
    }
    if (!arithenter(x)) {
        return 0;
    }
    i64 val1 = arithcomma(x);
    x->noeval -= skip;
    if (x->curtok != ':') {
        arithleave(x);
        aritherror(x, "`:' expected for conditional expression");
        return 0;
    }

    skip = !!cval;
    x->noeval += skip;
    arithreadtok(x);
    if (x->curtok == AR_END) {
        arithleave(x);
        aritherror(x, "expression expected");
        return 0;
    }
    i64 val2 = arithcond(x);
    x->noeval -= skip;
    arithleave(x);
    x->lasttok = AR_COND;
    return cval ? val1 : val2;
}

static i64 arithassign(ArithExpr *x)
{
    i64 val = arithcond(x);
    if (x->curtok!='=' && x->curtok!=AR_OPASSIGN) {
        return val;
    }

    b32 special = x->curtok == AR_OPASSIGN;
    i32 op      = x->assigntok;
    i64 lval    = val;
    if (x->lasttok != AR_STR) {
        aritherror(x, "attempted assignment to non-variable");
        return 0;
    }
    if (!x->hastok) {
        aritherror(x, "arithmetic syntax error in variable assignment");
        return 0;
    }
    ArithRef lhs = x->tokref;
    arithreadtok(x);
    if (!arithenter(x)) {
        return 0;
    }
    val = arithassign(x);
    arithleave(x);

    if (special) {
        if ((op=='/' || op=='%') && !val) {
            if (!x->noeval) {
                aritherror(x, "division by 0");
                return 0;
            }
            val = 1;
        }
        switch (op) {
        case '*':    lval = (i64)((u64)lval * (u64)val); break;
        case '/':
        case '%':    lval = arithdiv(op, lval, val);     break;
        case '+':    lval = (i64)((u64)lval + (u64)val); break;
        case '-':    lval = (i64)((u64)lval - (u64)val); break;
        case AR_LSH:
        case AR_RSH: lval = arithshift(op, lval, val);   break;
        case '&':    lval &= val;                        break;
        case '|':    lval |= val;                        break;
        case '^':    lval ^= val;                        break;
        }
        val = lval;
    }

    if (!x->noeval) {
        arithbind(x, &lhs, val);
    }
    x->hastok = 0;
    return val;
}

static i64 arithcomma(ArithExpr *x)
{
    i64 val = arithassign(x);
    while (x->curtok == ',') {
        arithreadtok(x);
        val = arithassign(x);
    }
    return val;
}

// Evaluate text as a new expression, nested in env->cur if not null.
static i64 arithevalstr(ArithCtx *c, Str s)
{
    u8 *p   = s.data;
    u8 *end = s.data + s.len;
    for (; p<end && arithblank(*p); p++) {}
    if (p==end || !*p) {
        return 0;
    }

    ArithExpr x = {};
    x.c      = c;
    x.beg    = s.data;
    x.end    = end;
    x.tp     = s.data;
    x.lasttp = s.data;

    // Like bash, report exceeding the limit in the enclosing expression,
    // even from a call nested in a callback (a subscript)
    ArithEnv  *env   = c->env;
    ArithExpr *outer = env->cur;
    if (env->depth >= ARITH_MAXDEPTH) {
        arithmessage(c, outer ? outer : &x, "expression recursion level exceeded");
        return 0;
    }
    env->depth++;
    env->cur = &x;
    arithreadtok(&x);
    i64 val = arithcomma(&x);
    if (x.curtok != AR_END) {
        aritherror(&x, "arithmetic syntax error in expression");
    }
    env->cur = outer;
    env->depth--;
    return val;
}

static Arith arith(ArithEnv *env, Arena *a, Str expr)
{
    Arith    r = {};
    ArithCtx c = {};
    i32        depth = env->depth;
    ArithExpr *cur   = env->cur;
    c.env  = env;
    c.a    = a;
    c.nest = cur ? cur->c->nest : 0;  // a subscript continues the nesting
    r.value = arithevalstr(&c, expr);
    env->depth = depth;
    env->cur   = cur;
    if (c.err.data) {
        r.value   = 0;
        r.err     = c.err;
        r.unbound = c.unbound;
        return r;
    }
    r.ok = 1;
    return r;
}
