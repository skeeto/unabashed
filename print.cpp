// Canonical command printing
//
// Reproduces bash's layout for declare -f and type, and for command
// substitution bodies, which the parser replaces with their printed form.
// The printer is a small state machine: indentation, whether to skip the
// next indent, and here-document bodies deferred until after the
// connector that follows them.
//
// This is free and unencumbered software released into the public domain.

struct Printer {
    Buf    b;
    Arena *a;
    i32    indentation;
    i32    amount;
    i32    insidefunc;
    i32    skipindent;
    b32    washeredoc;
    i32    connection;  // printing a connection, depth
    i32    comsub;      // printing a command substitution
    b32    posix;
    b32    external;    // declare -f form: bash removes internal escapes
    Redir *deferred;    // here-document bodies yet to print
};

static Printer *newprinter(Arena *a)
{
    Printer *p = alloc<Printer>(a);
    p->b      = Buf(a, 256);
    p->a      = a;
    p->amount = 4;
    return p;
}

static void put(Printer *p, Str s)
{
    print(&p->b, s);
}

static void putfd(Printer *p, i32 fd)
{
    if (fd < 0) {
        put(p, "4294967295");  // out of range, as an unsigned int
    } else {
        print(&p->b, (i64)fd);
    }
}

static void indent(Printer *p, i32 amount)
{
    for (; amount > 0; amount--) {
        putbyte(&p->b, ' ');
    }
}

static void newline(Printer *p, Str s)
{
    put(p, "\n");
    indent(p, p->indentation);
    put(p, s);
}

static void semicolon(Printer *p)
{
    Str s = finish(&p->b);
    if (s.len>0 && s[s.len-1]=='\n') {
        return;
    }
    if (s.len>1 && s[s.len-1]=='&' && s[s.len-2]==' ') {
        return;
    }
    put(p, ";");
}

static void printwords(Printer *p, Word *w, Str sep)
{
    for (; w; w = w->next) {
        put(p, w->text);
        if (w->next) put(p, sep);
    }
}

static void heredocheader(Printer *p, Redir *r)
{
    if (r->srcvar) {
        put(p, "{");
        put(p, r->srcvar->text);
        put(p, "}");
    } else if (r->src != 0) {
        putfd(p, r->src);
    }
    put(p, r->op==R_HEREDOCSTRIP ? Str("<<-") : Str("<<"));
    Str eof = p->external ? removeescapes(p->a, r->eof) : r->eof;
    if (r->word->flags & W_QUOTED) {
        put(p, singlequote(p->a, eof));
    } else {
        put(p, eof);
    }
}

static void heredocbody(Printer *p, Redir *r)
{
    // A quoted here-document body is stored raw, but bash's external
    // form still removes "escapes" from it.
    b32 quoted = r->word->flags & W_QUOTED;
    put(p, p->external && quoted ? removeescapes(p->a, r->body) : r->body);
    put(p, p->external ? removeescapes(p->a, r->eof) : r->eof);
}

// The redirector, unless it is the default for this operator.
static void redirector(Printer *p, Redir *r, i32 dflt)
{
    if (r->srcvar) {
        put(p, "{");
        put(p, r->srcvar->text);
        put(p, "}");
    } else if (r->src != dflt) {
        putfd(p, r->src);
    }
}

// The redirector, always printed.
static void redirectorfd(Printer *p, Redir *r)
{
    if (r->srcvar) {
        put(p, "{");
        put(p, r->srcvar->text);
        put(p, "}");
    } else {
        putfd(p, r->src);
    }
}

static void printredir(Printer *p, Redir *r)
{
    switch (r->op) {
    case R_INPUT:
        redirector(p, r, 0);
        put(p, "< ");
        put(p, r->word->text);
        break;
    case R_OUTPUT:
        redirector(p, r, 1);
        put(p, "> ");
        put(p, r->word->text);
        break;
    case R_CLOBBER:
        redirector(p, r, 1);
        put(p, ">| ");
        put(p, r->word->text);
        break;
    case R_APPEND:
        redirector(p, r, 1);
        put(p, ">> ");
        put(p, r->word->text);
        break;
    case R_INOUT:
        redirector(p, r, 0);
        put(p, "<> ");
        put(p, r->word->text);
        break;
    case R_HEREDOC:
    case R_HEREDOCSTRIP:
        heredocheader(p, r);
        put(p, "\n");
        heredocbody(p, r);
        break;
    case R_HERESTRING:
        redirector(p, r, 0);
        put(p, "<<< ");
        put(p, r->word->text);
        break;
    case R_DUPIN:
        redirectorfd(p, r);
        put(p, "<&");
        putfd(p, r->dst);
        break;
    case R_DUPOUT:
        redirectorfd(p, r);
        put(p, ">&");
        putfd(p, r->dst);
        break;
    case R_DUPINWORD:
        if (!r->srcvar && r->src==0) {
            put(p, "<&");
        } else {
            redirectorfd(p, r);
            put(p, "<&");
        }
        put(p, r->word->text);
        break;
    case R_DUPOUTWORD:
        if (!r->srcvar && r->src==1) {
            put(p, ">&");
        } else {
            redirectorfd(p, r);
            put(p, ">&");
        }
        put(p, r->word->text);
        break;
    case R_MOVEIN:
        redirectorfd(p, r);
        put(p, "<&");
        putfd(p, r->dst);
        put(p, "-");
        break;
    case R_MOVEOUT:
        redirectorfd(p, r);
        put(p, ">&");
        putfd(p, r->dst);
        put(p, "-");
        break;
    case R_MOVEINWORD:
        redirectorfd(p, r);
        put(p, "<&");
        put(p, r->word->text);
        put(p, "-");
        break;
    case R_MOVEOUTWORD:
        redirectorfd(p, r);
        put(p, ">&");
        put(p, r->word->text);
        put(p, "-");
        break;
    case R_CLOSE:
        redirectorfd(p, r);
        put(p, ">&-");
        break;
    case R_ERRANDOUT:
        put(p, "&> ");
        put(p, r->word->text);
        break;
    case R_APPENDERRANDOUT:
        put(p, "&>> ");
        put(p, r->word->text);
        break;
    }
}

static void heredocbodies(Printer *p, Redir *list)
{
    put(p, "\n");
    for (Redir *r = list; r; r = r->next) {
        heredocbody(p, r);
        put(p, "\n");
    }
    p->washeredoc = 1;
}

// Print the connector, then any deferred here-document bodies. A lone
// ";" is not printed, but notes not to add a space after the bodies.
static void printdeferred(Printer *p, Str connector)
{
    b32 show = connector.len && !(connector == ";");
    if (show) {
        put(p, connector);
    }
    if (p->deferred) {
        heredocbodies(p, p->deferred);
        if (show) {
            put(p, " ");
        }
        p->washeredoc = 1;
    }
    p->deferred = 0;
}

static void maybedeferred(Printer *p, Str connector)
{
    if (p->deferred) {
        printdeferred(p, connector);
    }
}

static void printredirs(Printer *p, Redir *list)
{
    Redir  *heredocs = 0;
    Redir **tail     = &heredocs;
    p->washeredoc = 0;
    for (Redir *r = list; r; r = r->next) {
        if (r->op==R_HEREDOC || r->op==R_HEREDOCSTRIP) {
            Redir *copy = alloc<Redir>(p->a);
            *copy = *r;
            copy->next = 0;
            heredocheader(p, copy);
            *tail = copy;
            tail  = &copy->next;
        } else {
            printredir(p, r);
        }
        if (r->next) {
            put(p, " ");
        }
    }
    if (heredocs && p->connection) {
        p->deferred = heredocs;
    } else if (heredocs) {
        heredocbodies(p, heredocs);
    }
}

static void printcmd(Printer *p, Node *n);

static void printcond(Printer *p, Cond *c)
{
    if (c->invert) {
        put(p, "! ");
    }
    switch (c->kind) {
    case C_EXPR:
        put(p, "( ");
        printcond(p, c->left);
        put(p, " )");
        break;
    case C_AND:
        printcond(p, c->left);
        put(p, " && ");
        printcond(p, c->right);
        break;
    case C_OR:
        printcond(p, c->left);
        put(p, " || ");
        printcond(p, c->right);
        break;
    case C_UNARY:
        put(p, c->op->text);
        put(p, " ");
        printcond(p, c->left);
        break;
    case C_BINARY:
        printcond(p, c->left);
        put(p, " ");
        put(p, c->op->text);
        put(p, " ");
        printcond(p, c->right);
        break;
    case C_TERM:
        put(p, c->op->text);
        break;
    }
}

// Can the name be printed without the function keyword? Only names
// that would read as assignments need it, in POSIX mode too.
static b32 validfuncname(Str name, b32)
{
    return !assignment(name, 0);
}

// Print a function body: a group's contents, with the group's own
// redirections printed after the closing brace.
static void printfuncbody(Printer *p, Node *body)
{
    Redir *redirs = 0;
    Node  *inner  = body;
    if (body->kind == N_GROUP) {
        redirs = body->redirs;  // printed after the closing brace
        inner  = body->a;
    }
    printcmd(p, inner);
    maybedeferred(p, "");
    p->indentation -= p->amount;
    p->insidefunc--;
    if (redirs) {
        newline(p, "} ");
        printredirs(p, redirs);
    } else {
        newline(p, "}");
        p->washeredoc = 0;
    }
}

static void printfuncdef(Printer *p, Node *f)
{
    if (!p->posix || !validfuncname(f->name->text, 1)) {
        put(p, "function ");
    }
    put(p, f->name->text);
    put(p, " () \n");
    indent(p, p->indentation);
    put(p, "{ \n");
    p->insidefunc++;
    p->indentation += p->amount;
    printfuncbody(p, f->a);
}

static void printcmd(Printer *p, Node *n)
{
    if (!n) {
        return;
    }
    if (p->skipindent) {
        p->skipindent--;
    } else {
        indent(p, p->indentation);
    }

    if (n->flags & CMD_TIME) {
        put(p, "time ");
        if (n->flags & CMD_TIMEPOSIX) {
            put(p, "-p ");
        }
    }
    if (n->flags & CMD_INVERT) {
        put(p, "! ");
    }

    switch (n->kind) {
    case N_FOR:
    case N_SELECT:
        put(p, n->kind==N_FOR ? Str("for ") : Str("select "));
        put(p, n->name->text);
        put(p, " in ");
        printwords(p, n->words, " ");
        put(p, ";");
        newline(p, "do\n");
        p->indentation += p->amount;
        printcmd(p, n->b);
        maybedeferred(p, "");
        semicolon(p);
        p->indentation -= p->amount;
        newline(p, "done");
        break;

    case N_ARITHFOR:
        put(p, "for ((");
        put(p, n->arith->text);
        put(p, "; ");
        put(p, n->arith->next->text);
        put(p, "; ");
        put(p, n->arith->next->next->text);
        put(p, "))");
        newline(p, "do\n");
        p->indentation += p->amount;
        printcmd(p, n->b);
        maybedeferred(p, "");
        semicolon(p);
        p->indentation -= p->amount;
        newline(p, "done");
        break;

    case N_CASE: {
        put(p, "case ");
        put(p, n->words->text);
        put(p, " in ");
        p->indentation += p->amount;
        b32 first = 1;
        for (Case *c = n->cases; c; c = c->next) {
            if (!p->comsub || !first) {
                newline(p, "");
            }
            first = 0;
            if (c->patterns->text == "esac") {
                put(p, "(");
            }
            printwords(p, c->patterns, " | ");
            put(p, ")\n");
            p->indentation += p->amount;
            printcmd(p, c->body);
            p->indentation -= p->amount;
            maybedeferred(p, "");
            switch (c->term) {
            case CASE_FALLTHROUGH: newline(p, ";&");  break;
            case CASE_TESTNEXT:    newline(p, ";;&"); break;
            default:               newline(p, ";;");  break;
            }
        }
        p->indentation -= p->amount;
        newline(p, "esac");
    } break;

    case N_WHILE:
    case N_UNTIL:
        put(p, n->kind==N_WHILE ? Str("while ") : Str("until "));
        p->skipindent++;
        printcmd(p, n->a);
        maybedeferred(p, "");
        semicolon(p);
        if (p->washeredoc) {
            indent(p, p->indentation);
            put(p, "do\n");
            p->washeredoc = 0;
        } else {
            put(p, " do\n");
        }
        p->indentation += p->amount;
        printcmd(p, n->b);
        maybedeferred(p, "");
        p->indentation -= p->amount;
        semicolon(p);
        newline(p, "done");
        break;

    case N_IF:
        put(p, "if ");
        p->skipindent++;
        printcmd(p, n->a);
        maybedeferred(p, "");
        semicolon(p);
        if (p->washeredoc) {
            indent(p, p->amount);  // sic: bash indents by one level
            put(p, "then\n");
            p->washeredoc = 0;
        } else {
            put(p, " then\n");
        }
        p->indentation += p->amount;
        printcmd(p, n->b);
        maybedeferred(p, "");
        p->indentation -= p->amount;
        if (n->c) {
            semicolon(p);
            newline(p, "else\n");
            p->indentation += p->amount;
            printcmd(p, n->c);
            maybedeferred(p, "");
            p->indentation -= p->amount;
        }
        semicolon(p);
        newline(p, "fi");
        break;

    case N_ARITH:
        put(p, "((");
        put(p, n->arith->text);
        put(p, "))");
        break;

    case N_COND:
        put(p, "[[ ");
        printcond(p, n->cond);
        put(p, " ]]");
        break;

    case N_SIMPLE:
        printwords(p, n->words, " ");
        if (n->redirs) {
            if (n->words) put(p, " ");
            printredirs(p, n->redirs);
        }
        return;  // redirections already printed

    case N_CONNECT: {
        p->skipindent++;
        p->connection++;
        printcmd(p, n->a);
        i32 c = n->connector;
        switch (c) {
        case '&':
        case '|': {
            u8  buf[2] = {' ', (u8)c};
            printdeferred(p, Str{buf, 2});
            if (c!='&' || n->b) {
                put(p, " ");
                p->skipindent++;
            }
        } break;
        case T_ANDAND:
            printdeferred(p, " && ");
            if (n->b) p->skipindent++;
            break;
        case T_OROR:
            printdeferred(p, " || ");
            if (n->b) p->skipindent++;
            break;
        case ';':
        case '\n': {
            u8  sep        = p->comsub ? (u8)c : ';';
            b32 wasnewline = !p->deferred && !p->washeredoc && c=='\n';
            if (!p->deferred) {
                if (!p->washeredoc) {
                    put(p, Str{&sep, 1});
                } else {
                    p->washeredoc = 0;
                }
            } else {
                printdeferred(p, p->insidefunc ? Str("") : Str(";"));
            }
            if (p->insidefunc) {
                put(p, "\n");
            } else if (p->comsub && c=='\n' && !wasnewline) {
                put(p, "\n");
            } else {
                if (c == ';') put(p, " ");
                if (n->b) p->skipindent++;
            }
        } break;
        }
        printcmd(p, n->b);
        if (p->connection == 1) {
            maybedeferred(p, "");
        }
        p->connection--;
    } break;

    case N_FUNCDEF:
        printfuncdef(p, n);
        break;

    case N_GROUP:
        put(p, "{ ");
        if (!p->insidefunc) {
            p->skipindent++;
        } else {
            put(p, "\n");
            p->indentation += p->amount;
        }
        printcmd(p, n->a);
        maybedeferred(p, "");
        if (p->insidefunc) {
            put(p, "\n");
            p->indentation -= p->amount;
            indent(p, p->indentation);
        } else {
            semicolon(p);
            put(p, " ");
        }
        put(p, "}");
        p->washeredoc = 0;
        break;

    case N_SUBSHELL:
        put(p, "( ");
        p->skipindent++;
        printcmd(p, n->a);
        maybedeferred(p, "");
        put(p, " )");
        p->washeredoc = 0;
        break;

    case N_COPROC:
        put(p, "coproc ");
        if (n->a->kind != N_SIMPLE) {
            put(p, n->name->text);
            put(p, " ");
        }
        p->skipindent++;
        printcmd(p, n->a);
        break;
    }

    if (n->redirs) {
        put(p, " ");
        printredirs(p, n->redirs);
    }
}

// Command substitution body as reparsed text.
static Str printcomsub(Arena *a, Node *n)
{
    Printer *p = newprinter(a);
    p->comsub = 1;
    printcmd(p, n);
    return finish(&p->b);
}

// A function definition as printed by declare -f, without the final
// newline.
static Str printfunction(Arena *a, Str name, Node *body, b32 posix)
{
    Printer *p = newprinter(a);
    p->posix    = posix;
    p->external = 1;
    if (name.len) {
        if (!validfuncname(name, posix)) {
            put(p, "function ");
        }
        put(p, name);
        put(p, " ");
    }
    put(p, "() \n");
    p->indentation += p->amount;
    p->insidefunc++;
    put(p, "{ \n");
    i32 saved = p->indentation - p->amount;
    printfuncbody(p, body);
    p->indentation = saved;
    return finish(&p->b);
}
