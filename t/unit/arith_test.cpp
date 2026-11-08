// Unit tests for arith.cpp against expectations recorded from bash 5.3
//   c++ -std=c++20 -o arith_test t/unit/arith_test.cpp && ./arith_test
// Run from the repository root, or pass the golden file path.
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/base.cpp"
#include "../../src/arith.cpp"

static void os_oom()
{
    fputs("arith_test: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}

static i32 ntests;
static i32 nfailed;

static void showstr(Str s)
{
    fwrite(s.data, 1, (uz)s.len, stderr);
}


// Golden file escapes: \\ \n \t \r

static Str unescape(Arena *a, Str s)
{
    Str r = {allocbytes(a, s.len), 0};
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c=='\\' && i+1<s.len) {
            switch (s[++i]) {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case 'r': c = '\r'; break;
            default:  c = s[i];
            }
        }
        r.data[r.len++] = c;
    }
    return r;
}

static Str escape(Arena *a, Str s)
{
    Buf b(a, 2*s.len + 1);
    for (iz i = 0; i < s.len; i++) {
        switch (s[i]) {
        case '\\': print(&b, "\\\\"); break;
        case '\n': print(&b, "\\n");  break;
        case '\t': print(&b, "\\t");  break;
        case '\r': print(&b, "\\r");  break;
        default:   putbyte(&b, s[i]);
        }
    }
    return finish(&b);
}


// A tiny variable store: scalars and indexed array elements, keyed by
// "name" or "name[N]" (N > 0), the way bash lists them.

struct Var {
    Str key;
    Str val;
    b32 ro;
};

struct Store {
    Slice<Var> vars;
    ArithEnv  *env;
    Str        suberr;    // a subscript error aborts the command
    b32        subunbound;
    byte      *stacklow;
};

static Str keyname(Str key)
{
    return cut(key, '[').head;
}

static Str makekey(Arena *a, Str name, i64 index)
{
    if (!index) {
        return name;
    }
    Buf b(a, name.len + 24);
    print(&b, name);
    putbyte(&b, '[');
    print(&b, index);
    putbyte(&b, ']');
    return finish(&b);
}

// Map a reference to its key, evaluating the subscript and rewriting
// it to the index, as the core does.
static Str resolve(Store *s, ArithRef *ref, Arena *a)
{
    if (!ref->hassub) {
        return ref->name;
    }
    Arith r = arith(s->env, a, ref->sub);
    if (!r.ok || r.value<0) {
        if (!s->suberr.data) {
            s->suberr     = r.ok ? Str("negative subscript") : r.err;
            s->subunbound = r.unbound;
        }
        return ref->name;
    }
    Buf b(a, 24);
    print(&b, r.value);
    ref->sub = finish(&b);
    return makekey(a, ref->name, r.value);
}

static Str storeget(void *ctx, ArithRef *ref, b32 *isset, Arena *a)
{
    Store *s = (Store *)ctx;
    byte *sp = (byte *)__builtin_frame_address(0);
    if (!s->stacklow || sp<s->stacklow) {
        s->stacklow = sp;
    }
    // Like bash, under set -u a missing array is an error before its
    // subscript is evaluated
    *isset = 0;
    for (iz i = 0; i < s->vars.len; i++) {
        *isset |= keyname(s->vars[i].key) == ref->name;
    }
    if (!*isset && s->env->nounset) {
        return {};
    }
    Str key = resolve(s, ref, a);
    Str r   = {};
    for (iz i = 0; i < s->vars.len; i++) {
        if (s->vars[i].key == key) {
            r = s->vars[i].val;
        }
    }
    return r;
}

static Str storeset(void *ctx, ArithRef *ref, i64 value, Arena *a)
{
    Store *s   = (Store *)ctx;
    Str    key = resolve(s, ref, a);
    for (iz i = 0; i < s->vars.len; i++) {
        if (s->vars[i].ro && keyname(s->vars[i].key)==ref->name) {
            return concat(a, ref->name, ": readonly variable");
        }
    }
    Buf b(a, 24);
    print(&b, value);
    Str val = finish(&b);
    for (iz i = 0; i < s->vars.len; i++) {
        if (s->vars[i].key == key) {
            s->vars[i].val = val;
            return {};
        }
    }
    Var v = {};
    v.key = key;
    v.val = val;
    s->vars = push(a, s->vars, v);
    return {};
}

// Add a setup item: NAME=VALUE, NAME[N]=VALUE or readonly:NAME=VALUE.
static void setvar(Store *s, Arena *a, Str item)
{
    Var v = {};
    if (startswith(item, "readonly:")) {
        item = cuthead(item, 9);
        v.ro = 1;
    }
    Cut c = cut(item, '=');
    v.key = c.head;
    v.val = c.tail;
    Cut sub = cut(c.head, '[');
    if (sub.ok) {
        i64 index = 0;
        for (iz i = 0; i<sub.tail.len && digit(sub.tail[i]); i++) {
            index = index*10 + sub.tail[i] - '0';
        }
        v.key = makekey(a, sub.head, index);
    }
    s->vars = push(a, s->vars, v);
}

static b32 lessstr(Str *a, Str *b)
{
    return compare(*a, *b) < 0;
}

static Slice<Str> dump(Store *s, Arena *a)
{
    Slice<Str> lines = {};
    for (iz i = 0; i < s->vars.len; i++) {
        Var *v = &s->vars[i];
        Str  line = concat(a, "state ", escape(a, v->key));
        line = concat(a, line, "=");
        line = concat(a, line, escape(a, v->val));
        lines = push(a, lines, line);
    }
    sort(lines.data, lines.len, lessstr, *a);
    return lines;
}

static void initenv(ArithEnv *env, Store *s, b32 nounset)
{
    *env = {};
    env->ctx     = s;
    env->get     = storeget;
    env->set     = storeset;
    env->nounset = nounset;
    s->env = env;
}


// Golden cases

struct Case {
    Str        flags;
    Str        expr;
    Slice<Str> setup;
    b32        iserr;
    i64        value;
    Str        err;
    Slice<Str> state;
    b32        nostate;
};

static i64 parseint(Str s)
{
    b32 neg = s.len && s[0]=='-';
    u64 v   = 0;
    for (iz i = neg; i < s.len; i++) {
        v = v*10 + (u64)(s[i] - '0');
    }
    return (i64)(neg ? -v : v);
}

static void fail(Case *c, Str why)
{
    nfailed++;
    fputs("FAIL: expr \"", stderr);
    showstr(c->expr);
    fputs("\": ", stderr);
    showstr(why);
    fputs("\n", stderr);
}

static void runcase(Case *c, Arena scratch)
{
    ntests++;
    Store    s   = {};
    ArithEnv env;
    initenv(&env, &s, c->flags == "u");
    for (iz i = 0; i < c->setup.len; i++) {
        setvar(&s, &scratch, c->setup[i]);
    }

    Arith r = arith(&env, &scratch, c->expr);
    if (s.suberr.data) {
        // The core would abort the command with this message
        r = {};
        r.err     = s.suberr;
        r.unbound = s.subunbound;
    }

    Buf b(&scratch);
    if (env.depth || env.cur) {
        print(&b, "depth not restored; ");
    }
    if (c->iserr) {
        if (r.ok) {
            print(&b, "got value ");
            print(&b, r.value);
            print(&b, ", want error: ");
            print(&b, c->err);
        } else if (!(r.err == c->err)) {
            print(&b, "got error: ");
            print(&b, r.err);
            print(&b, "\n      want error: ");
            print(&b, c->err);
        }
        b32 unbound = endswith(c->err, ": unbound variable");
        if (!r.ok && r.unbound!=unbound) {
            print(&b, "unbound flag wrong; ");
        }
    } else {
        if (!r.ok) {
            print(&b, "got error: ");
            print(&b, r.err);
            print(&b, ", want ");
            print(&b, c->value);
        } else if (r.value != c->value) {
            print(&b, "got ");
            print(&b, r.value);
            print(&b, ", want ");
            print(&b, c->value);
        }
    }

    if (!c->nostate) {
        Slice<Str> got = dump(&s, &scratch);
        b32 same = got.len == c->state.len;
        for (iz i = 0; same && i<got.len; i++) {
            same = got[i] == c->state[i];
        }
        if (!same) {
            print(&b, "\n      state got:");
            for (iz i = 0; i < got.len; i++) {
                print(&b, " ");
                print(&b, cuthead(got[i], 6));
            }
            print(&b, "\n      state want:");
            for (iz i = 0; i < c->state.len; i++) {
                print(&b, " ");
                print(&b, cuthead(c->state[i], 6));
            }
        }
    }

    if (b.len) {
        fail(c, finish(&b));
    }
}

static Str readfile(Arena *a, char const *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return {};
    }
    iz  cap = a->end - a->beg;
    u8 *buf = (u8 *)a->beg;
    iz  len = (iz)fread(buf, 1, (uz)cap, f);
    fclose(f);
    a->beg += len;
    return {buf, len};
}

static void rungolden(Arena *perm, char const *path)
{
    Str src = readfile(perm, path);
    if (!src.data) {
        fprintf(stderr, "arith_test: cannot read %s\n", path);
        nfailed++;
        return;
    }
    Case c = {};
    for (Cut line = {{}, src, 1}; line.tail.len;) {
        line = cut(line.tail, '\n');
        Str l = line.head;
        Cut f = cut(l, ' ');
        Str v = f.tail;
        if (f.head == "case") {
            c = {};
            c.flags = v;
        } else if (f.head == "expr") {
            c.expr = unescape(perm, v);
        } else if (f.head == "setup") {
            c.setup = push(perm, c.setup, unescape(perm, v));
        } else if (f.head == "value") {
            c.value = parseint(v);
        } else if (f.head == "error") {
            c.iserr = 1;
            c.err   = unescape(perm, v);
        } else if (f.head == "state") {
            c.state = push(perm, c.state, l);
        } else if (f.head == "nostate") {
            c.nostate = 1;
        } else if (f.head == "end") {
            runcase(&c, *perm);
        } else if (l.len) {
            fputs("arith_test: bad golden line: ", stderr);
            showstr(l);
            fputs("\n", stderr);
            nfailed++;
        }
    }
}


// Hand-written checks of properties the golden file cannot express

static void check(b32 ok, char const *what)
{
    ntests++;
    if (!ok) {
        nfailed++;
        fprintf(stderr, "FAIL: %s\n", what);
    }
}

static Str repeat(Arena *a, Str pre, Str mid, Str post, iz n)
{
    Buf b(a, (pre.len + post.len)*n + mid.len);
    for (iz i = 0; i < n; i++) print(&b, pre);
    print(&b, mid);
    for (iz i = 0; i < n; i++) print(&b, post);
    return finish(&b);
}

static void runextra(Arena scratch)
{
    Store    s = {};
    ArithEnv env;
    initenv(&env, &s, 0);
    setvar(&s, &scratch, "i=41");
    setvar(&s, &scratch, "x=x");
    setvar(&s, &scratch, "n=-7");

    // A simple expression allocates nothing
    byte *mark = scratch.beg;
    Arith r = arith(&env, &scratch, "i+1");
    check(r.ok && r.value==42, "i+1");
    check(scratch.beg == mark, "i+1 allocates nothing");
    r = arith(&env, &scratch, "n*2 + (i<<1) - 3");
    check(r.ok && r.value==65, "n*2 + (i<<1) - 3");
    check(scratch.beg == mark, "plain arithmetic allocates nothing");
    r = arith(&env, &scratch, "i++, i");
    check(r.ok && r.value==42, "i++, i");

    // The text ends at a NUL, as in bash's C strings
    r = arith(&env, &scratch, Str{(u8 *)"1+2\0+5", 6});
    check(r.ok && r.value==3, "NUL ends the expression");
    r = arith(&env, &scratch, Str{(u8 *)"7/0\0junk", 8});
    check(!r.ok && r.err=="7/0: division by 0 (error token is \"0\")",
          "NUL ends the error token");

    // Deep variable recursion, measuring stack use
    byte top;
    s.stacklow = 0;
    r = arith(&env, &scratch, "x");
    check(!r.ok && r.err=="x: expression recursion level exceeded (error token is \"x\")",
          "self-referential variable");
    check(env.depth == 0, "depth restored after recursion error");
    iz used = s.stacklow ? &top - s.stacklow : 0;
    fprintf(stderr, "info: 1024-level variable recursion used %lld stack bytes\n",
            (long long)used);
    check(used < 1<<20, "variable recursion stack use under 1 MiB");

    // Deep syntactic nesting up to the limit, with a variable read at the bottom
    s.stacklow = 0;
    Str e = repeat(&scratch, "(", "i", ")", ARITH_MAXNEST-1);
    r = arith(&env, &scratch, e);
    check(r.ok && r.value==42, "deep parentheses");
    used = s.stacklow ? &top - s.stacklow : 0;
    fprintf(stderr, "info: %d nested parentheses used %lld stack bytes\n",
            ARITH_MAXNEST-1, (long long)used);
    check(used < 1<<20, "parenthesis stack use under 1 MiB");
    e = repeat(&scratch, "(", "1", ")", ARITH_MAXNEST+1);
    r = arith(&env, &scratch, e);
    check(!r.ok && find(r.err, ": expression recursion level exceeded")>=0,
          "nesting limit is an error");
    e = repeat(&scratch, "!", "1", "", 30000);
    r = arith(&env, &scratch, e);
    check(r.ok && r.value==1, "30000 unary operators");
    e = repeat(&scratch, "x=", "1", "", 1000);
    r = arith(&env, &scratch, e);
    check(r.ok && r.value==1, "1000 chained assignments");
    e = repeat(&scratch, "2**", "1", "", 1000);
    r = arith(&env, &scratch, e);
    check(r.ok && r.value==65536, "1000 chained powers");
    e = repeat(&scratch, "1?", "1", ":0", 1000);
    r = arith(&env, &scratch, e);
    check(r.ok && r.value==1, "1000 nested conditionals");

    // Recursion through subscripts shares the nesting limit, so that
    // hostile input cannot multiply the depth of nested parentheses
    s.stacklow = 0;
    Str sub = repeat(&scratch, "(", "y", ")", 1000);
    setvar(&s, &scratch, concat(&scratch, concat(&scratch, "y=a[", sub), "]"));
    r = arith(&env, &scratch, "y");
    check(find(s.suberr, ": expression recursion level exceeded")>=0,
          "nesting through subscripts is limited");
    check(env.depth==0 && !env.cur, "depth restored after subscript recursion");
    used = s.stacklow ? &top - s.stacklow : 0;
    fprintf(stderr, "info: recursion through subscripts used %lld stack bytes\n",
            (long long)used);
    check(used < 1<<20, "subscript recursion stack use under 1 MiB");
    s.suberr = {};

    // Worst case: near the variable recursion limit, then the nesting
    // limit. Too deep for a default 1 MiB stack, so only on request.
    if (getenv("ARITH_DEEP")) {
        s.stacklow = 0;
        setvar(&s, &scratch, "dn=1020");
        Str deep = repeat(&scratch, "(", "1", ")", ARITH_MAXNEST-1);
        setvar(&s, &scratch, concat(&scratch, "w=", deep));
        setvar(&s, &scratch, "z=dn-- > 0 && z || w");
        r = arith(&env, &scratch, "z");
        check(r.ok && r.value==1, "deepest evaluation");
        used = s.stacklow ? &top - s.stacklow : 0;
        fprintf(stderr, "info: deepest evaluation used %lld stack bytes\n",
                (long long)used);
    }

    // Skipping deeply nested shell syntax in a subscript is bounded
    Str quotes = repeat(&scratch, "$(", "", ")", 100000);
    r = arith(&env, &scratch, concat(&scratch, concat(&scratch, "0 && a[", quotes), "]"));
    check(!r.ok && find(r.err, ": bad array subscript")>=0, "deep subscript syntax");
    quotes = repeat(&scratch, "$(", "", ")", AR_SKMAXDEPTH);
    r = arith(&env, &scratch, concat(&scratch, concat(&scratch, "0 && a[", quotes), "]"));
    check(r.ok && !r.value, "nested subscript syntax");

    // Nested calls share the recursion depth through the env
    env.depth = ARITH_MAXDEPTH - 1;
    r = arith(&env, &scratch, "1");
    check(r.ok && r.value==1 && env.depth==ARITH_MAXDEPTH-1, "depth 1023 still evaluates");
    env.depth = ARITH_MAXDEPTH;
    r = arith(&env, &scratch, "1");
    check(!r.ok && env.depth==ARITH_MAXDEPTH, "depth 1024 is an error");
    r = arith(&env, &scratch, "  ");
    check(r.ok && !r.value, "empty expression needs no depth");
    env.depth = 0;
}

int main(int argc, char **argv)
{
    iz    cap = (iz)1<<28;
    Arena perm = {};
    perm.beg = (byte *)malloc((uz)cap);
    perm.end = perm.beg + cap;
    if (!perm.beg) {
        os_oom();
    }

    char const *path = argc>1 ? argv[1] : "t/unit/arith.golden";
    rungolden(&perm, path);
    runextra(perm);

    fprintf(stderr, "%d tests, %d failed\n", ntests, nfailed);
    return nfailed != 0;
}
