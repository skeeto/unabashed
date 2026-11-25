// shgen: print a small random shell script, chosen by a seed
// $ cc -o shgen tools/shgen.c
// $ ./shgen 1234
//
// The scripts are for comparing shells (tools/diffuzz): each crosses a
// few constructs with the contexts bash treats apart (a pipeline stage,
// a command substitution, a subshell, a job, eval, a function, a nested
// bash), with errors (${x?}, set -u, a readonly or badly named variable,
// a bad descriptor) and with redirections of the standard descriptors,
// then prints statuses and state so that a difference shows.
//
// A script is the same for a seed wherever it is made. It names only
// builtins and the oracle's fakes, a few files in its own directory,
// and nothing that differs between runs of one shell: no pids, clocks
// or random numbers, and no job left writing as the shell goes on.
// Loops are bounded and functions do not recurse, so scripts end,
// though one may still fill a file or wait on a pipe: run them with
// limits.
//
// This is free and unencumbered software released into the public domain.
#include <stddef.h>
#include <stdint.h>

typedef uint8_t     u8;
typedef int32_t     b32;
typedef int32_t     i32;
typedef int64_t     i64;
typedef uint64_t    u64;
typedef char        byte;
typedef ptrdiff_t   iz;
typedef size_t      uz;

#define countof(a)      (iz)(sizeof(a) / sizeof(*(a)))
#define affirm(c)       while (!(c)) __builtin_unreachable()
#define new(a, n, t)    (t *)alloc(a, n, sizeof(t), _Alignof(t))
#define S(s)            (Str){(u8 *)s, sizeof(s)-1}
#define STR(s)          {(u8 *)s, sizeof(s)-1}
#define xset(d, c, n)   __builtin_memset(d, c, n)
#define xcpy(d, s, n)   __builtin_memcpy(d, s, n)

typedef struct {
    byte *beg;
    byte *end;
} Arena;

// Scripts are bounded (MAXFUEL, MAXDEPTH, Out.cap), so the arena cannot
// run out
static void *alloc(Arena *a, iz count, iz size, iz align)
{
    iz pad = -(uz)a->beg & (align - 1);
    affirm(count < (a->end - a->beg - pad)/size);
    byte *r = a->beg + pad;
    a->beg += pad + count*size;
    return xset(r, 0, count*size);
}

typedef struct {
    u8 *data;
    iz  len;
} Str;


// Output

typedef struct {
    u8 *data;
    iz  len;
    iz  cap;
    Str here;  // here-document bodies waiting for the next newline
} Out;

enum { OUTCAP = 1<<14, HERECAP = 1<<11 };

static Out newout(Arena *a)
{
    Out o = {0};
    o.cap       = OUTCAP;
    o.data      = new(a, o.cap, u8);
    o.here.data = new(a, HERECAP, u8);
    return o;
}

// What does not fit is dropped: the script is then cut short, which
// is one more script
static void print(Out *o, Str s)
{
    iz n = s.len<o->cap-o->len ? s.len : o->cap-o->len;
    if (n) xcpy(o->data+o->len, s.data, n);
    o->len += n;
}

#define put(o, s) print(o, S(s))

// Begin a new line, where the waiting here-documents go
static void newline(Out *o)
{
    put(o, "\n");
    print(o, o->here);
    o->here.len = 0;
}

// Before a construct closes that must hold its here-documents
static void flush(Out *o)
{
    if (o->here.len) newline(o);
}

// In single quotes, as one word
static void printquoted(Out *o, Str s)
{
    put(o, "'");
    for (iz i = 0; i < s.len; i++) {
        if (s.data[i] == '\'') {
            put(o, "'\\''");
        } else {
            print(o, (Str){s.data+i, 1});
        }
    }
    put(o, "'");
}


// Generator

typedef struct {
    Arena *a;
    u64    rng;
    i32    fuel;   // constructs left to spend
    i32    depth;
    b32    infn;   // in a function's body: call none
    b32    ext;    // extglob is on: its patterns parse
    b32    stage;  // a pipeline's later stage is next: no "!"
    b32    waits;  // the command ends by waiting for its job: a
                   // redirection could fail the wait and leave the job
} Gen;

enum { MINFUEL = 4, MAXFUEL = 24, MAXDEPTH = 5 };

static i32 rnd(Gen *g, i32 n)  // [0, n)
{
    g->rng = g->rng*0x5851f42d4c957f2d + 0x14057b7ef767814f;
    u64 r  = g->rng;
    r ^= r >> 32;
    r *= 0xd6e8feb86659fd93;
    r ^= r >> 32;
    return (i32)((r>>32) * (u64)n >> 32);
}

static b32 odds(Gen *g, i32 percent)
{
    return rnd(g, 100) < percent;
}

// Whether there is room for another construct that nests
static b32 room(Gen *g)
{
    return g->fuel-->0 && g->depth<MAXDEPTH;
}

#define any(g, o, t) print(o, t[rnd(g, (i32)countof(t))])

static void word(Gen *, Out *);
static void list(Gen *, Out *);
static void command(Gen *, Out *);
static void arith(Gen *, Out *, i32);

static Str scalars[] = {
    STR("x"), STR("x"), STR("y"), STR("z"), STR("u"), STR("r"),
};
static Str badnames[] = {
    STR("0x"), STR("a-b"), STR("x.y"), STR("1"), STR("x[")
};
static Str subscripted[] = {
    STR("arr[@]"), STR("arr[*]"), STR("arr[0]"), STR("arr[1]"),
    STR("arr[-1]"), STR("arr[9]"), STR("arr[x]"), STR("map[k]"),
    STR("map[@]"), STR("map[a b]"), STR("map[nope]"), STR("x[0]"),
    STR("x[@]"), STR("@"), STR("*"), STR("1"), STR("2"), STR("9"),
};
static Str specials[] = {
    STR("$?"), STR("$#"), STR("$@"), STR("$*"), STR("$-"), STR("$0"),
    STR("$1"), STR("$2"), STR("\"$@\""), STR("\"$*\""), STR("$_"),
    STR("$LINENO"), STR("$IFS"), STR("$OPTIND"), STR("$FUNCNAME"),
    STR("${FUNCNAME[@]}"), STR("$BASH_SUBSHELL"), STR("${PIPESTATUS[@]}"),
    STR("$SHLVL"), STR("$PWD"), STR("$OLDPWD"), STR("$HOME"),
    STR("$BASH_COMMAND"), STR("$BASH_ARGV0"), STR("$SHELLOPTS"),
    STR("$BASHOPTS"), STR("${BASH_SOURCE[0]}"), STR("${BASH_LINENO[*]}"),
    STR("$BASH_EXECUTION_STRING"), STR("${#BASH_ARGV[@]}"), STR("$fd"),
    STR("$REPLY"), STR("$OPTARG"), STR("$BASH_VERSION"), STR("$PATH"),
};
static Str literals[] = {
    STR("a"), STR("b"), STR("foo"), STR("bar"), STR("a b"), STR("0"),
    STR("1"), STR("2"), STR("42"), STR("-1"), STR("010"), STR("0x1f"),
    STR("abcabc"), STR("Hello"), STR("-n"), STR("-e"), STR("--"),
    STR("f1"), STR("f2"), STR("d1"), STR("k"), STR("x"), STR("="),
    STR("a=b"), STR("x=1"), STR("a:b:c"), STR("/"), STR("."), STR(".."),
    STR("\\*"), STR("\\\\"), STR("a\\ b"), STR("#"), STR("%s"), STR("!"),
};
static Str globs[] = {
    STR("*"), STR("?"), STR("f*"), STR("[a-f]*"), STR("*/"), STR("d1/*"),
    STR(".*"), STR("[!a]*"), STR("f[12]"), STR("nope*"), STR("**"),
    STR("[[:alpha:]]1"), STR("f\\*"), STR("[a"), STR("*[!1]"),
};
static Str extglobs[] = {
    STR("@(f1|f2)"), STR("*(a)"), STR("!(f1)"), STR("+([a-c])"),
    STR("?(a)b"), STR("*([0-9])"), STR("@(a|b)"), STR("!(*)"),
    STR("@($x|b)"), STR("+(a|\"b c\")"),
};
static Str squoted[] = {
    STR("''"), STR("'a b'"), STR("'$x'"), STR("'*'"), STR("'a\\nb'"),
    STR("' '"), STR("'\"'"), STR("$'a\\nb'"), STR("$'\\t'"),
    STR("$'\\x41\\101'"), STR("$'a\\'b'"), STR("$\"x\""), STR("'-'"),
};
static Str patterns[] = {
    STR("*"), STR("?"), STR("a*"), STR("*b"), STR("[a-c]"), STR("[!a]*"),
    STR("\\*"), STR("a"), STR("b*c"), STR("\"*\""), STR("$x"), STR("\"$x\""),
    STR("${y}"), STR("''"), STR("/"), STR("[[:upper:]]"), STR("o"),
    STR("[a"), STR("a|b"), STR("??"), STR("*\\**"),
};
static Str files[] = {
    STR("f1"), STR("f1"), STR("f2"), STR("d1"), STR("d1/f3"), STR("f9"),
    STR("/dev/null"), STR("/nope/f"), STR("\"$x\""), STR("$u"), STR("''"),
};

static void pattern(Gen *g, Out *o)
{
    if (g->ext && odds(g, 25)) {
        any(g, o, extglobs);
    } else {
        any(g, o, patterns);
    }
}

static void scalar(Gen *g, Out *o)
{
    if (odds(g, 3)) {
        any(g, o, badnames);
    } else {
        any(g, o, scalars);
    }
}

// What ${...} may name: a variable, an element, a positional
static void parameter(Gen *g, Out *o)
{
    if (odds(g, 35)) {
        any(g, o, subscripted);
    } else {
        scalar(g, o);
    }
}

// A word within an expansion's braces
static void inner(Gen *g, Out *o)
{
    if (room(g) && odds(g, 60)) {
        g->depth++;
        word(g, o);
        g->depth--;
    } else {
        any(g, o, literals);
    }
}

static void expansion(Gen *g, Out *o)
{
    static Str tests[] = {
        STR(":-"), STR(":="), STR(":?"), STR(":+"),
        STR("-"), STR("="), STR("?"), STR("+"),
    };
    static Str trims[] = {STR("%"), STR("%%"), STR("#"), STR("##")};
    static Str subst[] = {STR("/"), STR("//"), STR("/#"), STR("/%")};
    static Str cases[] = {
        STR("^"), STR("^^"), STR(","), STR(",,"), STR("^^[a-c]"),
        STR("~"), STR("~~"),
    };
    static Str ops[] = {
        STR("@Q"), STR("@E"), STR("@P"), STR("@A"), STR("@K"), STR("@a"),
        STR("@U"), STR("@u"), STR("@L"), STR("@k"), STR("@Z"),
    };
    static Str offsets[] = {
        STR(":1"), STR(":0:1"), STR(": -1"), STR(":1:-1"), STR(":x"),
        STR(":9"), STR(":-1"), STR(":1:0"), STR(":0:9"), STR(":$y"),
    };
    static Str prefixes[] = {
        STR("!x*"), STR("!a@"), STR("!arr[@]"), STR("!map[*]"),
        STR("!BASH_V*"), STR("!nope*"), STR("#"), STR("#@"), STR("#*"),
        STR("#arr[@]"), STR("#map[@]"), STR("##"), STR("#?"), STR("!#"),
    };
    put(o, "${");
    switch (rnd(g, 12)) {
    case 0:
        parameter(g, o);
        break;
    case 1:
        put(o, "#");
        parameter(g, o);
        break;
    case 2: case 3:
        parameter(g, o);
        any(g, o, tests);
        inner(g, o);
        break;
    case 4:
        parameter(g, o);
        any(g, o, trims);
        if (odds(g, 20)) inner(g, o); else pattern(g, o);
        break;
    case 5:
        parameter(g, o);
        any(g, o, subst);
        pattern(g, o);
        if (odds(g, 80)) {
            put(o, "/");
            inner(g, o);
        }
        break;
    case 6:
        parameter(g, o);
        any(g, o, offsets);
        break;
    case 7:
        parameter(g, o);
        any(g, o, cases);
        break;
    case 8:
        parameter(g, o);
        any(g, o, ops);
        break;
    case 9:
        put(o, "!");
        scalar(g, o);
        break;
    case 10:
        any(g, o, prefixes);
        break;
    case 11:  // an operator that is none
        parameter(g, o);
        any(g, o, ((Str[]){S("@"), S(":"), S("!"), S("@x"), S("&"), S("^^^")}));
        break;
    }
    put(o, "}");
}

// A list as a sub-script: for a string to quote, or to give a
// construct that must hold its own here-documents
static Str sublist(Gen *g)
{
    Out sub = newout(g->a);
    g->depth++;
    list(g, &sub);
    g->depth--;
    flush(&sub);
    return (Str){sub.data, sub.len};
}

static void comsub(Gen *g, Out *o)
{
    if (!room(g)) {
        any(g, o, ((Str[]){
            S("$(echo a)"), S("$(false)"), S("$(exit 3)"), S("$(echo a b)"),
            S("$(:)"), S("$(echo a >&2)"), S("$(<f1)"), S("$(nosuch)"),
            S("`echo a`"), S("$(printf 'a\\n\\n')"), S("${ echo a; }"),
            S("${| REPLY=a; }"), S("$(return 2)"), S("$(echo $BASH_SUBSHELL)"),
        }));
        return;
    }
    put(o, "$(");
    print(o, sublist(g));
    put(o, ")");
}

static void arith(Gen *g, Out *o, i32 depth)
{
    static Str numbers[] = {
        STR("0"), STR("1"), STR("2"), STR("3"), STR("7"), STR("-1"),
        STR("010"), STR("0x1f"), STR("2#101"), STR("64#_"), STR("08"),
        STR("9223372036854775807"), STR("1.5"), STR("1 2"), STR("36#z"),
    };
    static Str names[] = {
        STR("x"), STR("y"), STR("z"), STR("u"), STR("r"), STR("arr[0]"),
        STR("arr[1]"), STR("arr[x]"), STR("map[k]"), STR("$x"), STR("${y}"),
        STR("$#"), STR("$?"), STR("i"), STR("arr"), STR("$(echo 2)"),
    };
    static Str binary[] = {
        STR("+"), STR("-"), STR("*"), STR("/"), STR("%"), STR("**"),
        STR("<<"), STR(">>"), STR("&"), STR("|"), STR("^"), STR("&&"),
        STR("||"), STR("=="), STR("!="), STR("<"), STR(">"), STR("<="),
        STR(">="), STR(","),
    };
    static Str assign[] = {
        STR("="), STR("+="), STR("-="), STR("*="), STR("/="), STR("%="),
        STR("<<="), STR("|="), STR("^="),
    };
    static Str unary[] = {
        STR("-"), STR("!"), STR("~"), STR("++"), STR("--"), STR("+"),
    };
    b32 leaf = depth>2 || g->fuel--<=0;
    switch (leaf ? rnd(g, 2) : rnd(g, 9)) {
    case 0:
        any(g, o, numbers);
        break;
    case 1:
        any(g, o, names);
        break;
    case 2: case 3: case 4:
        arith(g, o, depth+1);
        any(g, o, binary);
        arith(g, o, depth+1);
        break;
    case 5:
        put(o, "(");
        arith(g, o, depth+1);
        put(o, ")");
        break;
    case 6:
        if (odds(g, 70)) {
            any(g, o, unary);
            any(g, o, names);
        } else {
            any(g, o, names);
            any(g, o, ((Str[]){S("++"), S("--")}));
        }
        break;
    case 7:
        arith(g, o, depth+1);
        put(o, "?");
        arith(g, o, depth+1);
        put(o, ":");
        arith(g, o, depth+1);
        break;
    case 8:
        any(g, o, names);
        any(g, o, assign);
        arith(g, o, depth+1);
        break;
    }
}

static void dquoted(Gen *g, Out *o)
{
    static Str text[] = {
        STR("a"), STR(" "), STR("a b"), STR("\\$x"), STR("\\\""),
        STR("\\\\"), STR("*"), STR("'"), STR("~"), STR("{a,b}"), STR("!"),
        STR("\\n"), STR("="), STR("#"),
    };
    put(o, "\"");
    for (i32 n = 1 + rnd(g, 3); n; n--) {
        switch (rnd(g, 7)) {
        case 0: case 1:
            any(g, o, text);
            break;
        case 2:
            put(o, "$");
            any(g, o, scalars);
            break;
        case 3:
            expansion(g, o);
            break;
        case 4:
            comsub(g, o);
            break;
        case 5:
            put(o, "$((");
            arith(g, o, 1);
            put(o, "))");
            break;
        case 6:
            any(g, o, ((Str[]){
                S("$@"), S("$*"), S("${arr[@]}"), S("${arr[*]}"), S("$#"),
                S("$?"), S("$1"), S("${map[@]}"), S("${!arr[@]}"), S("$-"),
            }));
            break;
        }
    }
    put(o, "\"");
}

static void word(Gen *g, Out *o)
{
    for (i32 n = 1 + odds(g, 25); n; n--) {
        switch (rnd(g, 16)) {
        case 0: case 1: case 2:
            any(g, o, literals);
            break;
        case 3:
            any(g, o, squoted);
            break;
        case 4: case 5:
            dquoted(g, o);
            break;
        case 6: case 7:
            put(o, "$");
            any(g, o, scalars);
            break;
        case 8:
            any(g, o, specials);
            break;
        case 9: case 10:
            expansion(g, o);
            break;
        case 11:
            comsub(g, o);
            break;
        case 12:
            put(o, "$((");
            arith(g, o, 0);
            put(o, "))");
            break;
        case 13:
            any(g, o, ((Str[]){
                S("{a,b}"), S("{1..3}"), S("a{,}"), S("{a,b}{c,d}"),
                S("{x..z..2}"), S("{$x,y}"), S("{a,${x}}"), S("{3..1}"),
                S("{a}"), S("{,}"), S("{01..10..5}"), S("~"), S("~/f1"),
                S("~nobody"), S("~+"), S("x=~"), S("a:~"),
            }));
            break;
        case 14:
            if (g->ext && odds(g, 30)) {
                any(g, o, extglobs);
            } else {
                any(g, o, globs);
            }
            break;
        case 15:
            any(g, o, ((Str[]){
                S("${arr[@]}"), S("\"${arr[@]}\""), S("${arr[*]}"),
                S("\"${!map[@]}\""), S("${#arr[@]}"), S("\"${map[k]}\""),
                S("${arr[@]:1}"), S("\"${@:2}\""), S("${*:1:1}"),
                S("${arr[@]/a/b}"), S("\"${arr[@]^}\""), S("${@#a}"),
            }));
            break;
        }
    }
}

static void words(Gen *g, Out *o, i32 min, i32 max)
{
    for (i32 n = min + rnd(g, max-min+1); n; n--) {
        put(o, " ");
        word(g, o);
    }
}

static void redirect(Gen *g, Out *o)
{
    static Str fixed[] = {
        STR(" 2>&1"), STR(" >&2"), STR(" 2>/dev/null"), STR(" >/dev/null"),
        STR(" <&-"), STR(" >&-"), STR(" 2>&-"), STR(" </dev/null"),
        STR(" <&2"), STR(" >&0"), STR(" 0<&1"), STR(" 2>&9"), STR(" <&9"),
        STR(" 3>&1"), STR(" >&3"), STR(" 3>&-"), STR(" 0>&1"), STR(" 1>&2-"),
        STR(" <&0"), STR(" >&$x"), STR(" <&$u"), STR(" 9>&2"), STR(" 2>&1 >&2"),
    };
    static Str ops[] = {
        STR(" >"), STR(" >>"), STR(" <"), STR(" 2>"), STR(" &>"),
        STR(" <>"), STR(" >|"), STR(" 3>"), STR(" 3<"), STR(" {fd}>"),
        STR(" {fd}<"), STR(" &>>"), STR(" 0>"), STR(" 1<"), STR(" {x}>"),
        STR(" {r}>"), STR(" 10>"),
    };
    switch (rnd(g, 8)) {
    case 0: case 1: case 2:
        any(g, o, fixed);
        break;
    case 3: case 4: case 5:
        any(g, o, ops);
        any(g, o, files);
        break;
    case 6:
        put(o, " <<<");
        word(g, o);
        break;
    case 7:
        switch (rnd(g, 3)) {
        case 0: put(o, " <<E");   break;
        case 1: put(o, " <<'E'"); break;
        case 2: put(o, " <<-E");  break;
        }
        {
            Out body = {0};
            body.data = o->here.data + o->here.len;
            body.cap  = HERECAP - o->here.len;
            for (i32 n = rnd(g, 3); n; n--) {
                any(g, &body, ((Str[]){
                    S("a $x\n"), S("\t$(echo b)\n"), S("${y:-c} \\$x\n"),
                    S("$((1+1)) \"q\" 'q'\n"), S("E \n"), S("\n"),
                }));
            }
            put(&body, "E\n");
            o->here.len += body.len;
        }
        break;
    }
}

static void assignment(Gen *g, Out *o)
{
    switch (rnd(g, 10)) {
    case 0: case 1: case 2:
        scalar(g, o);
        put(o, "=");
        word(g, o);
        break;
    case 3:
        scalar(g, o);
        put(o, "+=");
        word(g, o);
        break;
    case 4:
        any(g, o, ((Str[]){S("arr"), S("arr"), S("x"), S("map"), S("r")}));
        any(g, o, ((Str[]){S("=("), S("+=(")}));
        words(g, o, 0, 3);
        if (odds(g, 20)) put(o, " [5]=e [k]=f");
        put(o, " )");
        break;
    case 5:
        any(g, o, ((Str[]){
            S("arr[0]"), S("arr[3]"), S("arr[-1]"), S("arr[x]"),
            S("map[k]"), S("map[a b]"), S("map[$x]"), S("x[1]"), S("arr[]"),
        }));
        any(g, o, ((Str[]){S("="), S("+=")}));
        word(g, o);
        break;
    case 6:
        any(g, o, ((Str[]){
            S("declare "), S("declare -i "), S("declare -r "),
            S("declare -x "), S("declare -l "), S("declare -u "),
            S("declare -a "), S("declare -A "), S("declare -n "),
            S("declare -g "), S("declare +x "), S("declare +r "),
            S("local "), S("local -i "), S("typeset -i "), S("export "),
            S("readonly "), S("export -n "), S("declare -ia "),
        }));
        scalar(g, o);
        if (odds(g, 70)) {
            put(o, "=");
            word(g, o);
        }
        break;
    case 7:
        any(g, o, ((Str[]){
            S("unset "), S("unset -v "), S("unset -f "), S("unset -n "),
        }));
        any(g, o, ((Str[]){
            S("x"), S("y"), S("r"), S("arr"), S("arr[0]"), S("arr[-1]"),
            S("map[k]"), S("f"), S("u"), S("0x"), S("arr[@]"), S("x y"),
        }));
        break;
    case 8:
        any(g, o, ((Str[]){
            S("IFS="), S("IFS=:"), S("IFS=a"), S("PATH="), S("OPTIND=2"),
            S("LINENO=7"), S("BASH_ARGV0=q"), S("FUNCNAME=q"), S("HOME=/"),
            S("PS4='+ '"), S("POSIXLY_CORRECT=1"), S("GLOBIGNORE='f*'"),
            S("PIPESTATUS=5"), S("BASH_SUBSHELL=4"), S("SHLVL=9"),
            S("_=q"), S("LC_ALL=C"), S("BASHOPTS=q"), S("UID=4"),
            S("FUNCNEST=1"), S("IGNOREEOF=1"), S("CDPATH=/"),
        }));
        break;
    case 9:
        put(o, "printf -v ");
        any(g, o, ((Str[]){S("x"), S("arr[1]"), S("map[k]"), S("r"), S("0x")}));
        put(o, " '%s-%d'");
        words(g, o, 1, 2);
        break;
    }
}

static void condition(Gen *g, Out *o, i32 depth)
{
    static Str binary[] = {
        STR(" == "), STR(" = "), STR(" != "), STR(" < "), STR(" > "),
        STR(" -eq "), STR(" -ne "), STR(" -lt "), STR(" -ge "), STR(" =~ "),
        STR(" -nt "), STR(" -ef "),
    };
    static Str unary[] = {
        STR("-z "), STR("-n "), STR("-e "), STR("-f "), STR("-d "),
        STR("-s "), STR("-r "), STR("-v "), STR("-o "), STR("-R "),
        STR("-t "), STR("! "),
    };
    switch (depth>1 ? rnd(g, 3) : rnd(g, 6)) {
    case 0:
        word(g, o);
        break;
    case 1:
        any(g, o, unary);
        if (odds(g, 30)) any(g, o, files); else word(g, o);
        break;
    case 2:
        word(g, o);
        any(g, o, binary);
        if (odds(g, 30)) pattern(g, o); else word(g, o);
        break;
    case 3:
        condition(g, o, depth+1);
        any(g, o, ((Str[]){S(" && "), S(" || ")}));
        condition(g, o, depth+1);
        break;
    case 4:
        put(o, "( ");
        condition(g, o, depth+1);
        put(o, " )");
        break;
    case 5:
        put(o, "! ");
        condition(g, o, depth+1);
        break;
    }
}

static void simple(Gen *g, Out *o)
{
    static Str tests[] = {
        STR(" = "), STR(" != "), STR(" -eq "), STR(" -lt "), STR(" -a "),
        STR(" -o "), STR(" \\< "), STR(" -nt "),
    };
    static Str flags[] = {
        STR("set -e"), STR("set -u"), STR("set -o pipefail"), STR("set -f"),
        STR("set -x"), STR("set +e"), STR("set -C"), STR("set -a"),
        STR("set -o posix"), STR("set -eu"), STR("set -B"), STR("set +B"),
        STR("set -o nounset"), STR("set -E"), STR("set -T"), STR("set -n"),
        STR("set -v"), STR("set -k"), STR("set -o errtrace"), STR("set -h"),
        STR("shopt -s extglob"), STR("shopt -s nullglob"),
        STR("shopt -s failglob"), STR("shopt -s dotglob"),
        STR("shopt -s lastpipe"), STR("shopt -s inherit_errexit"),
        STR("shopt -s nocasematch"), STR("shopt -s globstar"),
        STR("shopt -u sourcepath"), STR("shopt -s expand_aliases"),
        STR("shopt -s xpg_echo"), STR("shopt -s nocaseglob"),
        STR("shopt -s array_expand_once"), STR("shopt -s localvar_inherit"),
        STR("shopt -s localvar_unset"), STR("shopt -q extglob"),
    };
    static Str control[] = {
        STR("return"), STR("return 3"), STR("exit"), STR("exit 3"),
        STR("break"), STR("continue"), STR("break 2"), STR("return x"),
        STR("exit 300"), STR("continue 0"), STR("return -1"), STR("false"),
        STR("true"), STR("nosuch"), STR("./f1"), STR("d1"), STR("/nope/x"),
    };
    static Str misc[] = {
        STR("shift"), STR("shift 2"), STR("shift 9"), STR("pwd"),
        STR("cd d1"), STR("cd .."), STR("cd /nope"), STR("cd -"), STR("cd"),
        STR("type f"), STR("type echo cat nosuch"), STR("command -v f g cat"),
        STR("command -V echo"), STR("declare -p x"), STR("declare -p arr map"),
        STR("declare -f f"), STR("declare -F"), STR("export -p | grep ' [xyz]='"),
        STR("readonly -p"), STR("trap"), STR("trap -p EXIT"), STR("alias a=echo"),
        STR("a b"), STR("unalias a"), STR("hash -r"), STR("hash cat"),
        STR("umask 077"), STR("umask"), STR("wait"), STR("wait %1"),
        STR("kill -TERM $$"), STR("kill -USR1 $$"), STR("kill -0 $$"),
        STR("getopts ab: o"), STR("getopts ab: o -a -b c"), STR("echo $o"),
        STR("let x++"), STR("let 'x = 1/0'"), STR("let"), STR("times >/dev/null"),
        STR("builtin echo a"), STR("builtin nosuch"), STR("command nosuch"),
        STR("command -p cat f1"), STR("enable -n echo"), STR("exec"),
        STR("exec echo a"), STR("exec nosuch"), STR("exec 3>f2"),
        STR("exec 3>&-"), STR("exec >f2"), STR("exec 2>&1"), STR("exec <f1"),
        STR("exec {fd}>f2"), STR("echo a >&$fd"), STR("exec {fd}>&-"),
        STR("caller"), STR("caller 0"), STR("local"), STR("local -"),
        STR("set +o"), STR("set --"), STR("set -- a 'b c' d"), STR("set -- \"$@\" e"),
        STR("eval"), STR("eval 'if'"), STR("eval '('"), STR("eval 'echo $('"),
        STR("eval ')'"), STR("eval 'x=('"), STR("eval 'for ((;;'"),
        STR(". f1"), STR(". ./f2 a b"), STR("source nope"), STR(". d1"),
        STR("mapfile -t arr <f1"), STR("mapfile arr <<<$x"), STR("readarray -n1 arr <f1"),
        STR("read"), STR("read -r x y <f1"), STR("read x <<<'a b'"),
        STR("read -a arr <<<'a b c'"), STR("read -n1 x <<<abc"), STR("read r"),
        STR("read -d: x <<<a:b"), STR("read -p q x"), STR("read 0x <f1"),
        STR("test"), STR("[ ]"), STR("[ a"), STR("[ -v x ]"), STR("[ -v arr[0] ]"),
        STR("true & wait $!"), STR("false & wait $!"), STR("(exit 4) & wait -n"),
        STR("sleep 0"), STR("sleep 0 & wait"), STR("pushd d1"), STR("popd"),
        STR("dirs"), STR("help nosuch"), STR("bind"), STR("fc"), STR("history"),
        STR("ulimit -n"), STR("printf"), STR("echo"), STR("unset"),
        STR("compgen -v x"), STR("compgen -A function"), STR("disown"),
        STR("suspend"), STR("logout"), STR("fg"), STR("bg"), STR("jobs"),
        STR("time -p : 2>/dev/null"), STR("! :"), STR("coproc :; wait"),
    };
    static Str fakes[] = {
        STR("cat f1"), STR("cat"), STR("cat f9"), STR("cat <f2"),
        STR("wc -l <f1"), STR("sort f1"), STR("tr a-z A-Z"), STR("head -n1"),
        STR("sed s/a/b/"), STR("grep a"), STR("grep -c a f1"), STR("seq 3"),
        STR("ls"), STR("ls d1"), STR("touch f2"), STR("mkdir d1"),
        STR("mkdir -p d1/d2"), STR("rm f1"), STR("rm -r d1"), STR("cp f1 f2"),
        STR("mv f2 d1"), STR("tee f2"), STR("true"), STR("false"),
        STR("env | grep '^[xyz]='"), STR("printenv x"), STR("uniq"),
        STR("cut -c1"), STR("tail -n1 f1"), STR("yes | head -n2"),
        STR("chmod 000 f1"), STR("chmod +x f1"), STR("ln -s f1 f2"),
        STR("basename $x"), STR("expr 1 + 1"), STR("mkfifo f2"),
        STR("recho"), STR("echo a | cat"), STR("cat <(echo a)"),
        STR("cat <(echo a) <(echo b)"), STR("tee >(cat) <f1"),
    };
    switch (rnd(g, 24)) {
    case 0: case 1: case 2:
        put(o, "echo");
        words(g, o, 0, 3);
        break;
    case 3:
        any(g, o, ((Str[]){
            S("printf '%s\\n'"), S("printf '<%s>'"), S("printf '%d\\n'"),
            S("printf '%q\\n'"), S("printf '%5.2s|%-3s|\\n'"), S("printf %b"),
            S("printf '%(%Y)T\\n'"), S("printf '%x %o %c\\n'"), S("recho"),
        }));
        words(g, o, 0, 3);
        break;
    case 4: case 5: case 6:
        assignment(g, o);
        break;
    case 7:
        put(o, ":");
        words(g, o, 1, 2);
        break;
    case 8:
        put(o, "[");
        if (odds(g, 40)) {
            any(g, o, ((Str[]){
                S(" -z"), S(" -n"), S(" -e"), S(" -f"), S(" -d"), S(" !"),
                S(" -v"), S(" -s"), S(" -t"), S(" -x"),
            }));
            words(g, o, 1, 1);
        } else {
            words(g, o, 1, 1);
            any(g, o, tests);
            word(g, o);
        }
        put(o, " ]");
        break;
    case 9:
        put(o, "[[ ");
        condition(g, o, 0);
        put(o, " ]]");
        break;
    case 10:
        put(o, "((");
        arith(g, o, 0);
        put(o, "))");
        break;
    case 11:
        any(g, o, flags);
        break;
    case 12:
        any(g, o, control);
        break;
    case 13: case 14: case 15:
        any(g, o, misc);
        break;
    case 16: case 17:
        any(g, o, fakes);
        break;
    case 18:  // no command name: its redirections and substitutions alone
        if (odds(g, 50)) comsub(g, o);
        redirect(g, o);
        if (odds(g, 30)) redirect(g, o);
        break;
    case 19:  // temporary bindings
        assignment(g, o);
        put(o, " ");
        if (odds(g, 30)) {
            assignment(g, o);
            put(o, " ");
        }
        any(g, o, ((Str[]){
            S("echo $x"), S("printenv x"), S("f"), S("eval 'echo $x'"),
            S(":"), S("declare -p x"), S("nosuch"), S("export x"),
            S("readonly x"), S("unset x"), S("read x"), S("local x"),
            S("exec"), S("command echo $x"), S("return"), S("break"),
            S(". ./f1"), S("eval"), S("set -u"), S("x=3"),
        }));
        break;
    case 20:
        any(g, o, ((Str[]){
            S("trap 'echo exit $?' EXIT"), S("trap 'echo err' ERR"),
            S("trap 'echo dbg' DEBUG"), S("trap 'echo ret' RETURN"),
            S("trap 'echo usr' USR1"), S("trap '' TERM"), S("trap - EXIT"),
            S("trap 'echo term; exit 5' TERM"), S("trap 'exit' EXIT"),
            S("trap : 0"), S("trap 'return 2' ERR"), S("trap 'echo $x' EXIT"),
            S("trap nosuch EXIT"), S("trap 'x=t' DEBUG"), S("trap echo NOPE"),
            S("trap 'false' EXIT"), S("trap 'trap - ERR; false' ERR"),
        }));
        break;
    case 21:  // a word as the command
        word(g, o);
        words(g, o, 0, 1);
        break;
    case 22:
        any(g, o, ((Str[]){S("f"), S("g"), S("f a b"), S("g 'a b'")}));
        break;
    case 23:
        put(o, "declare -p x y arr map");
        break;
    }
}

// Run TEXT some other way than writing it in place
static void indirect(Gen *g, Out *o, Str text)
{
    static Str shells[] = {
        STR("bash -c "), STR("bash -c "), STR("bash -ec "), STR("bash -uc "),
        STR("bash -xc "), STR("bash --posix -c "), STR("bash -o pipefail -c "),
        STR("bash -nc "), STR("bash -rc "), STR("bash +B -c "), STR("sh -c "),
        STR("bash -O extglob -c "), STR("bash -vc "), STR("bash -fc "),
    };
    static Str env[] = {
        STR("BASH_ARGV0=q "), STR("FUNCNAME=q "), STR("IFS=: "), STR("x=env "),
        STR("POSIXLY_CORRECT=1 "), STR("BASH_ENV=f1 "), STR("ENV=f1 "),
        STR("SHELLOPTS=errexit "), STR("SHELLOPTS=nounset:xtrace "),
        STR("BASHOPTS=extglob "), STR("LINENO=5 "), STR("OPTIND=3 "),
        STR("PWD=/x "), STR("OLDPWD=/tmp "), STR("SHLVL=5 "), STR("_=q "),
        STR("PIPESTATUS=3 "), STR("GROUPS=q "), STR("DIRSTACK=q "),
        STR("PS4='> ' "), STR("HOME=/tmp "), STR("PATH= "), STR("arr=q "),
        STR("BASH_SUBSHELL=3 "), STR("BASH_COMMAND=q "), STR("BASH_ARGV=q "),
        STR("BASH_SOURCE=q "), STR("BASH_LINENO=9 "), STR("BASHPID=1 "),
        STR("SHLVL=x "), STR("SHLVL=-3 "), STR("UID=3 "), STR("PPID=3 "),
        STR("BASH_VERSION=q "), STR("OPTERR=0 "), STR("HISTFILE=f2 "),
        STR("TIMEFORMAT= "), STR("FUNCNEST=0 "), STR("r=env "),
        STR("BASH_FUNC_f%%='() { echo env; }' "), STR("BASH_XTRACEFD=1 "),
        STR("LC_ALL=C.UTF-8 "), STR("TERM=dumb "), STR("EXECIGNORE='*' "),
        STR("BASH_COMPAT=31 "), STR("BASH_COMPAT=x "), STR("GLOBSORT=-name "),
    };
    switch (rnd(g, 8)) {
    case 0: case 1:
        put(o, "eval ");
        printquoted(o, text);
        break;
    case 2: case 3: case 4:
        if (odds(g, 50)) any(g, o, env);
        if (odds(g, 20)) any(g, o, env);
        any(g, o, shells);
        printquoted(o, text);
        if (odds(g, 40)) words(g, o, 1, 3);
        break;
    case 5:  // a script file, as a program or sourced
        put(o, "echo ");
        printquoted(o, text);
        put(o, " >f2; ");
        if (odds(g, 30)) any(g, o, env);
        any(g, o, ((Str[]){
            S("bash f2"), S("bash ./f2 a b"), S("bash <f2"), S(". ./f2"),
            S("source f2 a"), S("bash -s a <f2"), S("chmod +x f2; ./f2"),
            S("bash -e f2"), S("cat f2 | bash"), S("bash -x f2"),
            S("BASH_ENV=f2 bash -c :"), S("bash --rcfile f2 -c :"),
        }));
        break;
    case 6:
        put(o, "bash <<<");
        printquoted(o, text);
        break;
    case 7:
        put(o, "trap ");
        printquoted(o, text);
        any(g, o, ((Str[]){
            S(" EXIT"), S(" ERR; false"), S(" USR1; kill -USR1 $$"),
            S(" DEBUG; :"), S(" RETURN; f"), S(" TERM; kill $$"),
            S(" EXIT; exit 2"), S(" ERR; set -e; false"),
        }));
        break;
    }
}

static void compound(Gen *g, Out *o, b32 stage)
{
    g->depth++;
    switch (rnd(g, 14)) {
    case 0:
        put(o, "( ");
        list(g, o);
        put(o, " )");
        break;
    case 1:
        put(o, "{ ");
        list(g, o);
        put(o, "; }");
        break;
    case 2:
        put(o, "if ");
        command(g, o);
        put(o, "; then ");
        list(g, o);
        if (odds(g, 40)) {
            put(o, "; else ");
            list(g, o);
        }
        put(o, "; fi");
        break;
    case 3:
        put(o, "for ");
        if (odds(g, 10)) any(g, o, badnames); else any(g, o, scalars);
        if (odds(g, 85)) {
            put(o, " in");
            words(g, o, 0, 3);
        }
        put(o, "; do ");
        list(g, o);
        put(o, "; done");
        break;
    case 4:
        any(g, o, ((Str[]){
            S("for ((i=0; i<2; i++)); do "), S("for ((i=2; i--;)); do "),
            S("i=0; while ((i++<2)); do "), S("i=0; until ((i++>1)); do "),
            S("while read -r x; do "), S("i=0; while [ $((i+=1)) -lt 3 ]; do "),
        }));
        list(g, o);
        put(o, "; done");
        if (odds(g, 30)) put(o, " <f1");
        break;
    case 5:
        put(o, "case ");
        word(g, o);
        put(o, " in ");
        for (i32 n = 1 + rnd(g, 2); n; n--) {
            pattern(g, o);
            if (odds(g, 20)) {
                put(o, "|");
                pattern(g, o);
            }
            put(o, ") ");
            command(g, o);
            any(g, o, ((Str[]){S(";; "), S(";; "), S(";& "), S(";;& ")}));
        }
        put(o, "*) ");
        command(g, o);
        put(o, ";; esac");
        break;
    case 6:
        if (g->infn) {
            put(o, "{ ");
            list(g, o);
            put(o, "; }");
        } else {
            g->infn = 1;
            any(g, o, ((Str[]){
                S("f() { "), S("f() { "), S("g() { "), S("function f { "),
                S("f() ( "), S("f() { local x=l; "), S("f() { local -; "),
            }));
            b32 sub = o->data[o->len-2] == '(';
            list(g, o);
            if (sub) put(o, " )"); else put(o, "; }");
            g->infn = 0;
            if (odds(g, 20)) redirect(g, o);
            any(g, o, ((Str[]){
                S("; f"), S("; f a b"), S("; f; echo $?"), S("; g; f"),
                S("; x=t f"), S("; f | cat"), S("; echo $(f)"), S("; f && f"),
                S("; declare -f f"), S("; export -f f; bash -c f"),
                S("; f >f1"), S("; f; f"), S("; trap f EXIT"), S(""),
            }));
        }
        break;
    case 7:
        command(g, o);
        any(g, o, ((Str[]){S(" | "), S(" | "), S(" |& ")}));
        g->stage = 1;
        command(g, o);
        if (odds(g, 25)) {
            put(o, " | ");
            g->stage = 1;
            command(g, o);
        }
        break;
    case 8:
        command(g, o);
        any(g, o, ((Str[]){S(" && "), S(" || ")}));
        command(g, o);
        break;
    case 9:
        if (!stage) put(o, "! ");
        command(g, o);
        break;
    case 10:  // a job, waited for before anything else runs
        put(o, "{ ");
        list(g, o);
        put(o, "; } &");
        any(g, o, ((Str[]){S(" wait"), S(" wait $!"), S(" wait -n")}));
        g->waits = 1;
        break;
    case 11: case 12:
        indirect(g, o, sublist(g));
        break;
    case 13:
        any(g, o, ((Str[]){S("x="), S("echo "), S("echo \""), S(": "), S("")}));
        b32 quoted = o->len && o->data[o->len-1]=='"';
        comsub(g, o);
        if (quoted) put(o, "\"");
        break;
    }
    g->depth--;
}

static void command(Gen *g, Out *o)
{
    b32 stage = g->stage;
    g->stage  = 0;
    if (room(g) && odds(g, 45)) {
        compound(g, o, stage);
    } else {
        simple(g, o);
    }
    if (!g->waits && odds(g, 18)) redirect(g, o);
    g->waits = 0;
}

static void list(Gen *g, Out *o)
{
    for (i32 n = 1 + rnd(g, 3); n; n--) {
        command(g, o);
        if (odds(g, 30)) {
            any(g, o, ((Str[]){
                S("; echo $?"), S("; echo \"s=$?\""), S(" || echo no"),
                S(" && echo yes"), S("; echo ${PIPESTATUS[@]}"),
            }));
        }
        if (n > 1) {
            if (odds(g, 35)) newline(o); else put(o, "; ");
        }
    }
}

static Str generate(Arena *a, u64 seed)
{
    static Str setup[] = {
        STR("x=abc"), STR("x='a b'"), STR("x="), STR("y=2"), STR("y=b"),
        STR("z='*'"), STR("arr=(a 'b c' d)"), STR("arr=()"),
        STR("declare -A map=([k]=v ['a b']=w)"), STR("readonly r=1"),
        STR("set -- a 'b c'"), STR("echo 'a b' >f1; echo c >>f1"),
        STR("mkdir d1"), STR("f() { echo f \"$@\"; return 3; }"),
        STR("g() { echo g; }"), STR("set -u"), STR("set -e"),
        STR("set -o pipefail"), STR("shopt -s nullglob"), STR("declare -i y=1"),
        STR("export x"), STR("declare -n z=x"), STR("exec 3>&1"),
        STR("echo 'echo sourced $# $0; return 2' >f1"), STR("arr[3]=e"),
        STR("trap 'echo exit=$?' EXIT"), STR("local x 2>/dev/null"),
        STR("shopt -s lastpipe"), STR("set -E; trap 'echo err' ERR"),
    };
    Gen g  = {0};
    g.a    = a;
    g.rng  = seed*1111111111111111111u + 0x9e3779b97f4a7c15;
    rnd(&g, 1);
    rnd(&g, 1);
    g.fuel = MINFUEL + rnd(&g, MAXFUEL-MINFUEL+1);

    Out o = newout(a);
    g.ext = odds(&g, 40);
    if (g.ext) put(&o, "shopt -s extglob\n");
    for (i32 n = rnd(&g, 5); n; n--) {
        any(&g, &o, setup);
        newline(&o);
    }
    list(&g, &o);
    newline(&o);
    switch (rnd(&g, 5)) {
    case 0: put(&o, "echo \"end $?\"\n");                           break;
    case 1: put(&o, "declare -p x y z arr map 2>&1\n");             break;
    case 2: put(&o, "echo \"$x|$y|${arr[*]}|$#|$-|$?\"; ls -A\n");  break;
    }
    return (Str){o.data, o.len};
}


// Platform

#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    static byte mem[1<<24];
    Arena a = {mem, mem+countof(mem)};

    u64 seed = 0;
    b32 ok   = argc == 2;
    for (char *s = ok ? argv[1] : ""; *s; s++) {
        ok   = ok && *s>='0' && *s<='9';
        seed = seed*10 + (u64)(*s - '0');
    }
    if (!ok) {
        static char usage[] = "usage: shgen SEED\n";
        write(2, usage, sizeof(usage)-1);
        return 2;
    }

    Str s = generate(&a, seed);
    for (iz off = 0; off < s.len;) {
        iz r = write(1, s.data+off, (uz)(s.len-off));
        if (r < 0) return 1;
        off += r;
    }
    return 0;
}
