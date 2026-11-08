// test and [ builtins, and the primaries shared with [[ ]]
//
// test follows the POSIX rules by argument count: up to four arguments
// are decided by position, and longer lists (or anything the positional
// rules hand off) by a recursive-descent grammar with ! ( ) -a -o. Both
// paths reproduce bash's quirks: a parenthesized group of one to four
// arguments is re-dispatched by count (and a one-argument group tests
// the first argument of the whole command), -t takes an optional fd
// outside POSIX mode, and errors stop evaluation at the first one.
//
// Runs of ! and chains of -a and -o are loops. Only parenthesized groups
// recurse, and their nesting is limited to TEST_MAXNEST, past which the
// error is "expression recursion level exceeded" (bash has no limit and
// recurses until its stack runs out).
//
// The file system, terminals, options and variables are reached through
// TestEnv callbacks, so the module itself has no side effects. Paths go
// to the callbacks exactly as given; the caller makes them absolute and
// emulates /dev/fd/N and /dev/std* as needed.
//
// This is free and unencumbered software released into the public domain.

struct TestEnv {
    void *ctx;
    Stat  (*stat)(void *ctx, Str path, b32 follow);
    b32   (*access)(void *ctx, Str path, i32 mode);  // 4 r, 2 w, 1 x, 0 exists
    b32   (*isatty)(void *ctx, i64 fd);
    i32   (*option)(void *ctx, Str name);            // 1 on, 0 off, -1 unknown
    b32   (*varset)(void *ctx, Str name);            // raw -v operand
    b32   (*nameref)(void *ctx, Str name);
    i64   euid;
    i64   egid;
};

// Status 0 true, 1 false, 2 error. The message has no "NAME: " prefix.
struct TestResult {
    i32 status;
    Str err;
};

struct TestNum {
    i64 value;
    b32 ok;
};

// The text of the error the core prints when [ lacks its final ].
static Str testnobracket = "missing `]'";

// Parse an integer operand as bash does: optional leading whitespace,
// an optional sign, decimal digits, optional trailing whitespace, and
// no overflow. The -v callback may use it for positional parameters.
static TestNum testparseint(Str s)
{
    TestNum r = {};
    iz      i = 0;
    for (; i<s.len && whitespace(s[i]); i++) {}
    b32 neg = 0;
    if (i<s.len && (s[i]=='-' || s[i]=='+')) {
        neg = s[i++] == '-';
    }
    iz  beg = i;
    i64 v   = 0;  // accumulated negatively to reach the minimum
    i64 min = (i64)((u64)1 << 63);
    for (; i<s.len && digit(s[i]); i++) {
        i32 d = s[i] - '0';
        if (v < (min + d)/10) {
            return r;  // overflow
        }
        v = v*10 - d;
    }
    if (i == beg) {
        return r;
    }
    for (; i<s.len && whitespace(s[i]); i++) {}
    if (i < s.len) {
        return r;
    }
    if (!neg) {
        if (v == min) return r;
        v = -v;
    }
    r.value = v;
    r.ok    = 1;
    return r;
}

// Is op one of the unary primaries?
static b32 testunaryop(Str op)
{
    if (op.len!=2 || op[0]!='-') return 0;
    switch (op[1]) {
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'k': case 'n': case 'o': case 'p': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'z':
    case 'G': case 'L': case 'N': case 'O': case 'R': case 'S':
        return 1;
    }
    return 0;
}

static b32 teststrop(Str op)
{
    return op=="=" || op=="==" || op=="!=" || op=="<" || op==">";
}

static b32 testintop(Str op)
{
    return op=="-eq" || op=="-ne" || op=="-lt" ||
           op=="-le" || op=="-gt" || op=="-ge";
}

static b32 testfileop(Str op)
{
    return op=="-nt" || op=="-ot" || op=="-ef";
}

// Is op a binary primary? With cond, also those only [[ ]] knows.
static b32 testbinaryop(Str op, b32 cond)
{
    return teststrop(op) || testintop(op) || testfileop(op) ||
           (cond && op=="=~");
}

static b32 testintcmp(Str op, i64 a, i64 b)
{
    if (op == "-eq") return a == b;
    if (op == "-ne") return a != b;
    if (op == "-lt") return a <  b;
    if (op == "-le") return a <= b;
    if (op == "-gt") return a >  b;
    if (op == "-ge") return a >= b;
    return 0;
}

static TestResult testerror(Arena *a, Str pre, Str mid, Str post)
{
    TestResult r = {};
    r.status = 2;
    r.err    = concat(a, concat(a, clone(a, pre), mid), post);
    return r;
}

static TestResult testbool(b32 v)
{
    TestResult r = {};
    r.status = !v;
    return r;
}

// Like bash's stat wrapper, an empty path never exists.
static Stat teststat(TestEnv *env, Str path, b32 follow)
{
    Stat st = {};
    if (!path.len) {
        st.err = E_NOENT;
        return st;
    }
    return env->stat(env->ctx, path, follow);
}

static TestResult testunary(TestEnv *env, Arena *a, Str op, Str arg)
{
    if (!testunaryop(op)) {
        return testerror(a, op, ": unary operator expected", "");
    }

    switch (op[1]) {
    case 'n': return testbool(arg.len != 0);
    case 'z': return testbool(arg.len == 0);
    case 'o': return testbool(env->option(env->ctx, arg) == 1);
    case 'v': return testbool(env->varset(env->ctx, arg));
    case 'R': return testbool(env->nameref(env->ctx, arg));
    case 't': {
        TestNum fd = testparseint(arg);
        if (!fd.ok) {
            return testerror(a, arg, ": integer expected", "");
        }
        b32 ok = fd.value == (i32)fd.value;
        return testbool(ok && env->isatty(env->ctx, fd.value));
    }
    case 'r': return testbool(arg.len && env->access(env->ctx, arg, 4));
    case 'w': return testbool(arg.len && env->access(env->ctx, arg, 2));
    case 'x': return testbool(arg.len && env->access(env->ctx, arg, 1));
    }

    b32  follow = op[1]!='h' && op[1]!='L';
    Stat st     = teststat(env, arg, follow);
    if (st.err) {
        return testbool(0);
    }
    switch (op[1]) {
    case 'a':
    case 'e': return testbool(1);
    case 'f': return testbool(st.type==FT_FILE || st.type==FT_NONE);
    case 'd': return testbool(st.type == FT_DIR);
    case 'c': return testbool(st.type == FT_CHAR);
    case 'b': return testbool(st.type == FT_BLOCK);
    case 'p': return testbool(st.type==FT_FIFO || st.type==FT_PIPE);
    case 'S': return testbool(st.type == FT_SOCK);
    case 'h':
    case 'L': return testbool(st.type == FT_LINK);
    case 's': return testbool(st.size > 0);
    case 'u': return testbool(st.mode & 04000);
    case 'g': return testbool(st.mode & 02000);
    case 'k': return testbool(st.mode & 01000);
    case 'O': return testbool(st.uid == env->euid);
    case 'G': return testbool(st.gid == env->egid);
    case 'N': return testbool(st.mtime > st.atime);
    }
    assert(0);
}

static TestResult testbinary(TestEnv *env, Arena *a, Str op, Str lhs, Str rhs)
{
    if (op=="=" || op=="==") return testbool(lhs == rhs);
    if (op == "!=")          return testbool(!(lhs == rhs));
    if (op == "<")           return testbool(compare(lhs, rhs) < 0);
    if (op == ">")           return testbool(compare(lhs, rhs) > 0);

    if (testintop(op)) {
        TestNum l = testparseint(lhs);
        if (!l.ok) {
            return testerror(a, lhs, ": integer expected", "");
        }
        TestNum r = testparseint(rhs);
        if (!r.ok) {
            return testerror(a, rhs, ": integer expected", "");
        }
        return testbool(testintcmp(op, l.value, r.value));
    }

    if (testfileop(op)) {
        // A missing file is older than any existing one, and -ef is
        // false unless both exist.
        Stat sl = teststat(env, lhs, 1);
        Stat sr = teststat(env, rhs, 1);
        b32  el = !sl.err;
        b32  er = !sr.err;
        if (op == "-ef") {
            return testbool(el && er && sl.dev==sr.dev && sl.ino==sr.ino);
        } else if (op == "-nt") {
            return testbool(el>er || (el && er && sl.mtime>sr.mtime));
        } else {
            return testbool(el<er || (el && er && sl.mtime<sr.mtime));
        }
    }

    return testerror(a, op, ": binary operator expected", "");
}


// Expression parser

enum : i32 {
    TEST_MAXNEST = 1024,  // parenthesized groups, to bound stack use
};

struct TestState {
    TestEnv   *env;
    Arena     *perm;
    Str       *args;
    iz         nargs;
    iz         pos;
    i32        depth;    // open parenthesized groups
    b32        bracket;  // the ] removed after args still shows in errors
    TestResult err;      // first error, status 2 when set
};

static Str testarg(TestState *t, iz i)
{
    assert(i>=0 && i<t->nargs);
    return t->args[i];
}

static b32 testtoken(Str s, u8 c)
{
    return s.len==1 && s[0]==c;
}

static b32 testandor(Str s)
{
    return s=="-a" || s=="-o";
}

// Is the argument at pos the one-character token c?
static b32 testtokenat(TestState *t, u8 c)
{
    return t->pos<t->nargs && testtoken(t->args[t->pos], c);
}

// Is the argument at pos -c, as in -a or -o?
static b32 testflagat(TestState *t, u8 c)
{
    if (t->pos >= t->nargs) return 0;
    Str s = t->args[t->pos];
    return s.len==2 && s[0]=='-' && s[1]==c;
}

// Record an error; the first one wins and evaluation unwinds.
static b32 testfail(TestState *t, Str pre, Str mid, Str post)
{
    if (!t->err.status) {
        t->err = testerror(t->perm, pre, mid, post);
    }
    return 0;
}

static b32 testbeyond(TestState *t)
{
    return testfail(t, "argument expected", "", "");
}

static b32 testcheck(TestState *t, TestResult r)
{
    if (r.status == 2) {
        if (!t->err.status) t->err = r;
        return 0;
    }
    return !r.status;
}

// Step past an argument that must be followed by another.
static b32 testneed(TestState *t)
{
    t->pos++;
    return t->pos<t->nargs || testbeyond(t);
}

static b32 testexpr(TestState *);
static b32 testposix(TestState *, iz);

// Unary primary at pos with its operand.
static b32 testparseunary(TestState *t)
{
    Str op = testarg(t, t->pos);
    if (op=="-t" && t->env->option(t->env->ctx, Str("posix"))!=1) {
        // Outside POSIX mode the fd is optional, defaulting to 1, when
        // what follows is -a or -o in a long enough expression.
        t->pos++;
        if (t->pos >= t->nargs) {
            return testcheck(t, testunary(t->env, t->perm, op, "1"));
        }
        Str arg = testarg(t, t->pos);
        if (testparseint(arg).ok) {
            t->pos++;
            return testcheck(t, testunary(t->env, t->perm, op, arg));
        } else if (t->nargs>=4 && testandor(arg)) {
            return testcheck(t, testunary(t->env, t->perm, op, "1"));
        }
        return testfail(t, arg, ": integer expected", "");
    }
    if (!testneed(t)) return 0;
    Str arg = testarg(t, t->pos++);
    return testcheck(t, testunary(t->env, t->perm, op, arg));
}

// Binary primary with its operator at pos+1.
static b32 testparsebinary(TestState *t)
{
    Str lhs = testarg(t, t->pos);
    Str op  = testarg(t, t->pos+1);
    Str rhs = testarg(t, t->pos+2);
    if (!testbinaryop(op, 0)) {
        return testfail(t, op, ": binary operator expected", "");
    }
    t->pos += 3;
    return testcheck(t, testbinary(t->env, t->perm, op, lhs, rhs));
}

// Step inside the "(" at pos and count the group's arguments up to the
// matching ")". Like bash, the argument just inside is not examined. The
// count runs to the end when the ")" is missing. Returns 0 on error.
static iz testopen(TestState *t)
{
    if (!testneed(t)) {
        return 0;
    } else if (t->depth >= TEST_MAXNEST) {
        return testfail(t, "expression recursion level exceeded", "", "");
    }
    iz n     = 1;
    iz depth = 1;
    for (; t->pos+n < t->nargs; n++) {
        Str a = testarg(t, t->pos+n);
        if (testtoken(a, ')')) {
            depth--;
        } else if (testtoken(a, '(')) {
            depth++;
        }
        if (!depth) break;
    }
    return n;
}

// Step past the ")" closing a group, or record why it is not there.
static b32 testclose(TestState *t)
{
    if (t->err.status) {
        return 0;
    } else if (t->pos>=t->nargs && t->bracket) {
        return testfail(t, "`)' expected, found ]", "", "");
    } else if (t->pos >= t->nargs) {
        return testfail(t, "`)' expected", "", "");
    }
    Str close = testarg(t, t->pos);
    if (!testtoken(close, ')')) {
        return testfail(t, "`)' expected, found ", close, "");
    }
    t->pos++;
    return 1;
}

// A parenthesized group with pos on the "(". A group of one to four
// arguments is re-dispatched by count. The parser recurses only through
// here, so bounding the nesting bounds the stack.
static b32 testgroup(TestState *t)
{
    iz n = testopen(t);
    if (!n) return 0;
    t->depth++;
    b32 v = t->pos+n<t->nargs && n<=4 ? testposix(t, n) : testexpr(t);
    t->depth--;
    return testclose(t) && v;
}

// A binary or unary primary, or a lone string, at pos.
static b32 testprimary(TestState *t)
{
    Str s = testarg(t, t->pos);
    if (t->pos+3<=t->nargs && testbinaryop(testarg(t, t->pos+1), 0)) {
        return testparsebinary(t);
    } else if (t->pos+2<=t->nargs && testunaryop(s)) {
        return testparseunary(t);
    }
    t->pos++;
    return s.len != 0;
}

// Any number of "!", then a group or a primary.
static b32 testterm(TestState *t)
{
    if (t->pos >= t->nargs) {
        return testbeyond(t);
    }
    b32 invert = 0;
    for (; testtokenat(t, '!'); invert = !invert) {
        if (!testneed(t)) return 0;
    }
    b32 v = testtokenat(t, '(') ? testgroup(t) : testprimary(t);
    return invert ? !v : v;
}

// Terms joined by -a, binding tighter, and -o. Like bash, every term is
// evaluated: there is no short circuit, so later errors still count.
static b32 testexpr(TestState *t)
{
    if (t->pos >= t->nargs) {
        return testbeyond(t);
    }
    b32 any = 0;
    for (;;) {
        b32 all = testterm(t);
        while (!t->err.status && testflagat(t, 'a')) {
            t->pos++;
            all = testterm(t) && all;
        }
        if (t->err.status) {
            return 0;
        }
        any = any || all;
        if (!testflagat(t, 'o')) {
            return any;
        }
        t->pos++;
    }
}

static b32 testtwo(TestState *t)
{
    Str s = testarg(t, t->pos);
    if (testtoken(s, '!')) {
        t->pos++;
        return !testarg(t, t->pos++).len;
    } else if (testunaryop(s)) {
        return testparseunary(t);
    }
    return testfail(t, s, ": unary operator expected", "");
}

static b32 testthree(TestState *t)
{
    Str s  = testarg(t, t->pos);
    Str op = testarg(t, t->pos+1);
    if (testbinaryop(op, 0)) {
        return testparsebinary(t);
    } else if (testandor(op)) {
        b32 l = testarg(t, t->pos).len != 0;
        b32 r = testarg(t, t->pos+2).len != 0;
        t->pos += 3;
        return op=="-a" ? l&&r : l||r;
    } else if (testtoken(s, '!')) {
        t->pos++;
        b32 v = testtwo(t);
        return !v;
    } else if (testtoken(s, '(') && testtoken(testarg(t, t->pos+2), ')')) {
        b32 v = testarg(t, t->pos+1).len != 0;
        t->pos += 3;
        return v;
    }
    return testfail(t, op, ": binary operator expected", "");
}

// Decide n arguments starting at pos by the POSIX positional rules.
static b32 testposix(TestState *t, iz n)
{
    switch (n) {
    case 0:
        return 0;
    case 1:
        // bash tests the command's first argument here, even when this
        // is a parenthesized group further along.
        t->pos++;
        return testarg(t, 0).len != 0;
    case 2:
        return testtwo(t);
    case 3:
        return testthree(t);
    case 4: {
        Str s = testarg(t, t->pos);
        if (testtoken(s, '!')) {
            t->pos++;
            b32 v = testthree(t);
            return !v;
        } else if (testtoken(s, '(') && testtoken(testarg(t, t->pos+3), ')')) {
            t->pos++;
            b32 v = testtwo(t);
            if (t->err.status) return 0;
            t->pos++;
            return v;
        }
    } break;
    }
    return testexpr(t);
}

static TestResult testrun(TestEnv *env, Arena *a, Str *args, iz nargs, b32 bracket)
{
    TestState t = {};
    t.env     = env;
    t.perm    = a;
    t.args    = args;
    t.nargs   = nargs;
    t.bracket = bracket;
    if (!nargs) {
        return testbool(0);
    }

    b32 v = testposix(&t, nargs);
    if (t.err.status) {
        return t.err;
    } else if (t.pos < nargs) {
        Str s = testarg(&t, t.pos);
        if (s.len && s[0]=='-') {
            return testerror(a, "syntax error: `", s, "' unexpected");
        }
        return testerror(a, "too many arguments", "", "");
    }
    return testbool(v);
}

// The test builtin over its arguments (after the command name).
static TestResult testcmd(TestEnv *env, Arena *a, Str *args, iz nargs)
{
    return testrun(env, a, args, nargs, 0);
}

// The [ builtin over its arguments, including the final ], which must
// be present. Prefer it to testcmd: bash's messages can mention the ].
static TestResult testbracket(TestEnv *env, Arena *a, Str *args, iz nargs)
{
    if (!nargs || !(args[nargs-1] == "]")) {
        TestResult r = {};
        r.status = 2;
        r.err    = testnobracket;
        return r;
    }
    return testrun(env, a, args, nargs-1, 1);
}
