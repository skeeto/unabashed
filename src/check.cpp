// Test-only checks shared by the test and fuzz layers
// This is free and unencumbered software released into the public domain.

enum : i32 { FIX_OK, FIX_SYNTAX, FIX_REPARSE, FIX_DIFFER };

struct Fixed {
    i32 status;
    Str first;   // printed form of the offending command
    Str second;  // its reprint
};

// Print a command as the body of a function, as declare -f does.
static Str printasfunc(Arena *a, Node *cmd)
{
    Node *g = alloc<Node>(a);
    g->kind = N_GROUP;
    g->a    = cmd;
    return printfunction(a, "f", g, 0);
}

// Parse src one command at a time; each printed command must reparse
// to a function whose printed form is identical. Syntax errors in src
// itself are not failures.
static Fixed fixedpoint(Arena *a, Str src, b32 extglob)
{
    Fixed r = {};
    Lexer *L = newlexer(a, src);
    L->extglob  = extglob;
    L->stackmax = (iz)1<<20;
    for (i32 n = 0; n < 1000; n++) {
        Parsed p = parseunit(L);
        if (p.status == PARSE_EOF) {
            return r;
        } else if (p.status != PARSE_OK) {
            r.status = FIX_SYNTAX;
            return r;
        } else if (!p.cmd) {
            continue;
        }

        Str first = printasfunc(a, p.cmd);
        Lexer *M  = newlexer(a, first);
        M->extglob  = extglob;
        M->stackmax = (iz)1<<20;
        Parsed q  = parseunit(M);
        if (q.status!=PARSE_OK || !q.cmd || q.cmd->kind!=N_FUNCDEF || M->msgs) {
            r.status = FIX_REPARSE;
            r.first  = first;
            return r;
        }
        Str second = printfunction(a, q.cmd->name->text, q.cmd->a, 0);
        if (!(first == second)) {
            r.status = FIX_DIFFER;
            r.first  = first;
            r.second = second;
            return r;
        }
    }
    return r;
}
