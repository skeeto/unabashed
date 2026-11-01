// Brace expansion tests against expansions recorded from bash 5.3
// $ g++ -std=c++20 -g3 -Wall -Wextra -Wconversion -Wno-sign-conversion -Wno-unused-function
//       -fsanitize=undefined -fsanitize-trap -o brace_test t/unit/brace_test.cpp
// $ ./brace_test [DIR]    (DIR holds the data files, default t/unit)
//
// brace.golden (from brace_gen.sh) gives fields after quote removal, so
// the harness applies quote removal to the module's output and drops
// empty raw words, as bash does with null fields. brace_raw.golden gives
// raw words compared exactly, for words with other expansions, checked
// against bash by brace_rawcheck.sh.
//
// This is free and unencumbered software released into the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../base.cpp"
#include "../../brace.cpp"

[[noreturn]] static void os_oom()
{
    fputs("brace_test: out of memory\n", stderr);
    exit(2);
}

static b32 os_commit(Os *, byte *, iz)
{
    return 0;
}

static i32 ntests;
static i32 nfailed;

static Str loadfile(Arena *a, char const *dir, char const *name)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "brace_test: cannot open %s\n", path);
        exit(2);
    }
    Str r  = {};
    r.data = (u8 *)a->beg;
    r.len  = (iz)fread(r.data, 1, (uz)(a->end - a->beg), f);
    fclose(f);
    a->beg += r.len;
    return r;
}

static void printstr(FILE *f, Str s)
{
    fwrite(s.data, 1, (uz)s.len, f);
}

// Quote removal for words without expansions: '...', "..." and \.
static Str unquote(Arena *a, Str s)
{
    Str r  = {};
    r.data = allocbytes(a, s.len);
    u8 q   = 0;
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (q == '\'') {
            if (c == '\'') {
                q = 0;
            } else {
                r.data[r.len++] = c;
            }
        } else if (c == '\\') {
            if (++i == s.len) {
                break;
            }
            u8 n = s[i];
            if (q=='"' && n!='$' && n!='`' && n!='"' && n!='\\' && n!='\n') {
                r.data[r.len++] = '\\';
            }
            r.data[r.len++] = n;
        } else if (q=='"' && c=='"') {
            q = 0;
        } else if (!q && (c=='"' || c=='\'')) {
            q = c;
        } else {
            r.data[r.len++] = c;
        }
    }
    return r;
}

struct Case {
    Str        word;
    Slice<Str> want;
    i32        line;
};

// Parse "> WORD" headers each followed by lines of the given form:
// "[field]" when brackets is set, otherwise "= raw".
static Slice<Case> parse(Arena *a, Str text, b32 brackets)
{
    Slice<Case> r = {};
    i32 line = 0;
    while (text.len) {
        Cut c = cut(text, '\n');
        text  = c.tail;
        Str l = c.head;
        line++;
        if (l.len && l[l.len-1]=='\r') {
            l = cuttail(l, 1);
        }
        if (!l.len || l[0]=='#') {
            continue;
        } else if (startswith(l, "> ")) {
            Case k = {};
            k.word = cuthead(l, 2);
            k.line = line;
            r = push(a, r, k);
        } else if (r.len && brackets && l[0]=='[' && l[l.len-1]==']') {
            Case *k = &r[r.len-1];
            k->want = push(a, k->want, cuttail(cuthead(l, 1), 1));
        } else if (r.len && !brackets && startswith(l, "= ")) {
            Case *k = &r[r.len-1];
            k->want = push(a, k->want, cuthead(l, 2));
        } else if (r.len && !brackets && l == "=") {
            Case *k = &r[r.len-1];
            k->want = push(a, k->want, Str{});
        } else {
            fprintf(stderr, "brace_test: bad data line %d\n", line);
            exit(2);
        }
    }
    return r;
}

static void report(char const *file, Case *k, Slice<Str> got)
{
    nfailed++;
    fprintf(stderr, "FAIL %s:%d: ", file, k->line);
    printstr(stderr, k->word);
    fputs("\n  want:", stderr);
    for (iz i = 0; i < k->want.len; i++) {
        fputs(" [", stderr);
        printstr(stderr, k->want[i]);
        fputs("]", stderr);
    }
    fputs("\n  got: ", stderr);
    for (iz i = 0; i < got.len; i++) {
        fputs(" [", stderr);
        printstr(stderr, got[i]);
        fputs("]", stderr);
    }
    fputs("\n", stderr);
}

static b32 same(Slice<Str> a, Slice<Str> b)
{
    if (a.len != b.len) {
        return 0;
    }
    for (iz i = 0; i < a.len; i++) {
        if (!(a[i] == b[i])) return 0;
    }
    return 1;
}

// A word that changed must have passed the cheap test.
static void checkhasbrace(char const *file, Case *k, Slice<Str> raw)
{
    b32 changed = raw.len!=1 || !(raw[0]==k->word);
    if (changed && !hasbrace(k->word)) {
        nfailed++;
        fprintf(stderr, "FAIL %s:%d: hasbrace missed ", file, k->line);
        printstr(stderr, k->word);
        fputs("\n", stderr);
    }
}

static void rungolden(Arena scratch, char const *dir)
{
    char const *file  = "brace.golden";
    Slice<Case> cases = parse(&scratch, loadfile(&scratch, dir, file), 1);
    for (iz i = 0; i < cases.len; i++) {
        Arena      tmp = scratch;
        Case      *k   = &cases[i];
        Slice<Str> raw = braceexpand(&tmp, k->word);
        Slice<Str> got = {};
        for (iz j = 0; j < raw.len; j++) {
            if (raw[j].len) {
                got = push(&tmp, got, unquote(&tmp, raw[j]));
            }
        }
        ntests++;
        if (!same(got, k->want)) {
            report(file, k, got);
        }
        checkhasbrace(file, k, raw);
    }
}

static void runraw(Arena scratch, char const *dir)
{
    char const *file  = "brace_raw.golden";
    Slice<Case> cases = parse(&scratch, loadfile(&scratch, dir, file), 0);
    for (iz i = 0; i < cases.len; i++) {
        Arena      tmp = scratch;
        Case      *k   = &cases[i];
        Slice<Str> got = braceexpand(&tmp, k->word);
        ntests++;
        if (!same(got, k->want)) {
            report(file, k, got);
        }
        checkhasbrace(file, k, got);
    }
}

static void expect(b32 ok, char const *what)
{
    ntests++;
    if (!ok) {
        nfailed++;
        fprintf(stderr, "FAIL %s\n", what);
    }
}

// A word that is a list of "a" and a multi-line command substitution,
// checked by hand against bash 5.3 (the data files are line-based).
static void multiline(Arena scratch, Str sub, Str rest = "")
{
    Arena tmp  = scratch;
    Buf   word = Buf(&tmp, 256);
    print(&word, "{a,");
    print(&word, sub);
    print(&word, rest);
    print(&word, "}");
    Slice<Str> r = braceexpand(&tmp, finish(&word));
    b32 ok = r.len==2+!!rest.len && r[0]=="a" && r[1]==sub;
    ok = ok && (!rest.len || r[2]==cuthead(rest, 1));
    ntests++;
    if (!ok) {
        nfailed++;
        fputs("FAIL multi-line word: ", stderr);
        printstr(stderr, finish(&word));
        fputs("\n", stderr);
    }
}

// Properties that the data files cannot show.
static void runmisc(Arena scratch)
{
    multiline(scratch, "$(echo b # ) },\n)");
    multiline(scratch, "$(echo b\n)", ",c");
    multiline(scratch, "$(cat <<EOF\n},\nEOF\n)");
    multiline(scratch, "$(echo \"\n}\")");
    multiline(scratch, "$(cat <<'E O'\n)'\nE O\n)");
    multiline(scratch, "$(cat <<-EOF\n)\n\tEOF\n)");
    multiline(scratch, "$(x=1; ((x<<2)); echo })");
    multiline(scratch, "$(cat <<<')' ; echo })");
    multiline(scratch, "$(cat <<A <<B\n)\nA\n)'\nB\n)");
    multiline(scratch, "$(case $x in\n(esac) echo },\n;;\nesac)");
    multiline(scratch, "${ cat <<E\n}\nE\n}");
    multiline(scratch, "${ function f\n{ echo }; }; f; }");
    multiline(scratch, "${ f()\n{ echo }; }; f; }");
    multiline(scratch, "${ case x in\n}) echo no;;\nx) echo },;;\nesac\n}");
    multiline(scratch, "${ echo }\n}");

    {
        // An unchanged word comes back as itself, not a copy.
        Arena      tmp = scratch;
        Str        w   = "plain{word}";
        Slice<Str> r   = braceexpand(&tmp, w);
        expect(r.len==1 && r[0].data==w.data && r[0].len==w.len, "unchanged word aliased");
    }

    {
        Arena      tmp = scratch;
        Slice<Str> r   = braceexpand(&tmp, "{1..100000}");
        expect(r.len==100000 && r[99999]=="100000", "long sequence");
    }

    {
        Arena      tmp = scratch;
        Slice<Str> r   = braceexpand(&tmp, "{a,b}{1..3}{x,y,z}");
        expect(r.len==18 && r[0]=="a1x" && r[17]=="b3z", "product order");
    }

    {
        // A sequence too long to fit stays unexpanded instead of
        // exhausting the arena.
        Arena tmp = scratch;
        tmp.end   = tmp.beg + (1<<16);
        Slice<Str> r = braceexpand(&tmp, "x{1..100000}");
        expect(r.len==1 && r[0]=="x{1..100000}", "oversized sequence");
    }

    {
        expect(!hasbrace("abc"), "hasbrace abc");
        expect(!hasbrace("{abc}"), "hasbrace {abc}");
        expect(!hasbrace("a,b}"), "hasbrace a,b}");
        expect(!hasbrace("}{a,b"), "hasbrace }{a,b");
        expect(hasbrace("{a,b}"), "hasbrace {a,b}");
        expect(hasbrace("{1..2}"), "hasbrace {1..2}");
    }

    {
        // Empty input.
        Arena      tmp = scratch;
        Slice<Str> r   = braceexpand(&tmp, Str{});
        expect(r.len==1 && !r[0].len, "empty word");
    }

    {
        // Deep nesting: {a,{a,{a,...}}} expands to depth+1 words.
        Arena tmp   = scratch;
        iz    depth = 450;
        Buf   b     = Buf(&tmp, 8*depth);
        for (iz i = 0; i < depth; i++) print(&b, "{a,");
        print(&b, "z");
        for (iz i = 0; i < depth; i++) putbyte(&b, '}');
        Slice<Str> r = braceexpand(&tmp, finish(&b));
        expect(r.len==depth+1 && r[depth]=="z", "deep nesting");
    }

    {
        // Hostile nesting is cut off at BRACE_MAXDEPTH instead of
        // exhausting the stack: the rest is left as written.
        Arena tmp   = scratch;
        iz    depth = 20000;
        Buf   b     = Buf(&tmp, 8*depth);
        for (iz i = 0; i < depth; i++) print(&b, "{a,");
        print(&b, "z");
        for (iz i = 0; i < depth; i++) putbyte(&b, '}');
        Str        w    = finish(&b);
        Slice<Str> r    = braceexpand(&tmp, w);
        iz         keep = 3*(depth - BRACE_MAXDEPTH);
        b32        ok   = r.len==BRACE_MAXDEPTH+1;
        ok = ok && r[0]=="a" && takehead(r[r.len-1], keep)==takehead(cuthead(w, 3*BRACE_MAXDEPTH), keep);
        expect(ok, "hostile brace nesting");
    }

    {
        // Hostile nesting of constructs inside a word: the brace scan
        // gives up on them as unterminated, without deep recursion.
        static char const *opens[] = {"$(", "\"$(", "${ ", "$((", "<(", "( "};
        for (i32 k = 0; k < countof(opens); k++) {
            Arena tmp   = scratch;
            iz    depth = 100000;
            Str   open  = {(u8 *)opens[k], (iz)strlen(opens[k])};
            Buf   b     = Buf(&tmp, depth*open.len + 16);
            print(&b, "{a,$(");
            for (iz i = 0; i < depth; i++) print(&b, open);
            print(&b, "},b}");
            Str        w = finish(&b);
            Slice<Str> r = braceexpand(&tmp, w);
            expect(r.len==1 && r[0]==w, "hostile construct nesting");
        }
    }

    {
        // Arbitrary text, including unterminated constructs, must not
        // crash, must always produce a word, and must agree with
        // hasbrace. Deterministic, and touches nothing but memory.
        static char const *tokens[] = {
            "{", "}", ",", "..", "a", "1", "-", "\\", "'", "\"", "`",
            "$", "${", "$(", "$((", "(", ")", "<(", "<<", "\n", " ",
            "#", "case", "esac", "in", "|", "${ ", "\t", "0", "z",
        };
        u64 rng = 1;
        b32 ok  = 1;
        for (i32 n = 0; n < 200000; n++) {
            Arena tmp  = scratch;
            Buf   word = Buf(&tmp, 64);
            rng = rng*0x3243f6a8885a308d + 1;
            i32 len = (i32)(rng>>59);
            for (i32 i = 0; i < len; i++) {
                rng = rng*0x3243f6a8885a308d + 1;
                char const *t = tokens[(rng>>33) % countof(tokens)];
                print(&word, Str{(u8 *)t, (iz)strlen(t)});
            }
            Str        w = finish(&word);
            Slice<Str> r = braceexpand(&tmp, w);
            b32 changed  = r.len!=1 || !(r[0]==w);
            if (!r.len || (changed && !hasbrace(w))) {
                ok = 0;
                fputs("FAIL robustness: ", stderr);
                printstr(stderr, w);
                fputs("\n", stderr);
                break;
            }
        }
        expect(ok, "robustness");
    }
}

int main(int argc, char **argv)
{
    char const *dir = argc>1 ? argv[1] : "t/unit";
    iz          cap = (iz)1 << 28;
    Arena       a   = {};
    a.beg = (byte *)malloc((uz)cap);
    a.end = a.beg + cap;
    if (!a.beg) {
        fputs("brace_test: out of memory\n", stderr);
        return 2;
    }

    rungolden(a, dir);
    runraw(a, dir);
    runmisc(a);

    printf("%d tests, %d failed\n", ntests, nfailed);
    return nfailed != 0;
}
