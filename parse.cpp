// Lexer and parser
//
// The lexer mirrors the structure of bash's: input is consumed a line at
// a time, reserved words are recognized from the previous two tokens and
// a set of parser-state flags, aliases are expanded by pushing their text
// onto the input, and here-document bodies are read when a newline token
// is returned. The parser is recursive descent over the same grammar and
// reads tokens in the same order, so that state-dependent tokenization
// and error reporting line up with bash's.
//
// This is free and unencumbered software released into the public domain.


// Syntax tree

struct Word {
    Word *next;
    Str   text;
    i32   flags;
};

enum : i32 {
    W_HASDOLLAR  = 1<<0,
    W_QUOTED     = 1<<1,
    W_ASSIGNMENT = 1<<2,
    W_NOSPLIT    = 1<<3,
    W_COMPASSIGN = 1<<4,
    W_NOGLOB     = 1<<5,
    W_NOBRACE    = 1<<6,
    W_NOTILDE    = 1<<7,
    W_NOPROCSUB  = 1<<8,
};

enum : i32 {  // redirection instructions
    R_OUTPUT,          // >
    R_INPUT,           // <
    R_APPEND,          // >>
    R_HEREDOC,         // <<
    R_HEREDOCSTRIP,    // <<-
    R_HERESTRING,      // <<<
    R_DUPIN,           // <&N
    R_DUPOUT,          // >&N
    R_DUPINWORD,       // <&word
    R_DUPOUTWORD,      // >&word
    R_CLOSE,           // <&- >&-
    R_ERRANDOUT,       // &>
    R_APPENDERRANDOUT, // &>>
    R_INOUT,           // <>
    R_CLOBBER,         // >|
    R_MOVEIN,          // <&N-
    R_MOVEOUT,         // >&N-
    R_MOVEINWORD,      // <&word-
    R_MOVEOUTWORD,     // >&word-
};

struct Redir {
    Redir *next;
    i32    op;
    i32    src;      // source fd
    Word  *srcvar;   // {var} source, or null
    i32    dst;      // destination fd for duplications
    Word  *word;     // target, or here-document delimiter as written
    Str    body;     // here-document body
    Str    eof;      // here-document delimiter after quote removal
    b32    gathered; // body already read (it may be read again)
};

enum : i32 {  // command kinds
    N_SIMPLE,
    N_CONNECT,
    N_FOR,
    N_SELECT,
    N_ARITHFOR,
    N_CASE,
    N_WHILE,
    N_UNTIL,
    N_IF,
    N_GROUP,
    N_SUBSHELL,
    N_FUNCDEF,
    N_ARITH,
    N_COND,
    N_COPROC,
};

enum : i32 {  // command flags
    CMD_INVERT    = 1<<0,
    CMD_TIME      = 1<<1,
    CMD_TIMEPOSIX = 1<<2,
};

enum : i32 {  // case clause terminators
    CASE_BREAK,        // ;;
    CASE_FALLTHROUGH,  // ;&
    CASE_TESTNEXT,     // ;;&
};

struct Node;

struct Case {
    Case *next;
    Word *patterns;
    Node *body;
    i32   term;
};

enum : i32 {  // [[ ]] node kinds
    C_AND,
    C_OR,
    C_UNARY,
    C_BINARY,
    C_TERM,
    C_EXPR,
};

struct Cond {
    i32   kind;
    b32   invert;
    Word *op;
    Cond *left;
    Cond *right;
};

struct Node {
    i32    kind;
    i32    flags;
    i32    line;
    i32    connector;  // N_CONNECT: ';' '&' '|' '\n' T_ANDAND T_OROR
    Node  *a;          // first child: left, test, body, function body
    Node  *b;          // second child: right, then, loop body
    Node  *c;          // else
    Word  *words;      // simple command words, for/select list, case word
    Word  *name;       // for/select variable, function, coproc name
    Redir *redirs;
    Case  *cases;
    Cond  *cond;
    Word  *arith;      // arithmetic command or for expressions
    b32    inlist;     // for/select had an explicit "in"
};


// Tokens

enum : i32 {
    T_ERROR     = -1,
    // single characters stand for themselves: \n ; ( ) | & < > - { }
    T_EOF       = 256,
    T_WORD,
    T_ASSIGN,
    T_NUMBER,
    T_REDIRWORD,
    T_ARITHCMD,
    T_ARITHFOR,
    T_CONDCMD,
    T_IF,
    T_THEN,
    T_ELSE,
    T_ELIF,
    T_FI,
    T_CASE,
    T_ESAC,
    T_FOR,
    T_SELECT,
    T_WHILE,
    T_UNTIL,
    T_DO,
    T_DONE,
    T_IN,
    T_FUNCTION,
    T_TIME,
    T_BANG,
    T_CONDSTART,
    T_CONDEND,
    T_COPROC,
    T_TIMEOPT,
    T_TIMEIGN,
    T_ANDAND,
    T_OROR,
    T_DGREAT,
    T_DLESS,
    T_LESSAND,
    T_GREATAND,
    T_DSEMI,
    T_SEMIAND,
    T_DSEMIAND,
    T_DLESSDASH,
    T_TLESS,
    T_ANDGREAT,
    T_ANDDGREAT,
    T_LESSGREAT,
    T_CLOBBER,
    T_BARAND,
    T_DOLPAREN,
    T_DOLBRACE,
};

struct Reserved {
    Str word;
    i32 token;
};

static Reserved reserved[] = {
    {"if", T_IF},         {"then", T_THEN},      {"else", T_ELSE},
    {"elif", T_ELIF},     {"fi", T_FI},          {"case", T_CASE},
    {"esac", T_ESAC},     {"for", T_FOR},        {"select", T_SELECT},
    {"while", T_WHILE},   {"until", T_UNTIL},    {"do", T_DO},
    {"done", T_DONE},     {"in", T_IN},          {"function", T_FUNCTION},
    {"time", T_TIME},     {"{", '{'},            {"}", '}'},
    {"!", T_BANG},        {"[[", T_CONDSTART},   {"]]", T_CONDEND},
    {"coproc", T_COPROC},
};

static Reserved operators[] = {
    {"--", T_TIMEIGN},    {"-p", T_TIMEOPT},     {"&&", T_ANDAND},
    {"||", T_OROR},       {">>", T_DGREAT},      {"<<", T_DLESS},
    {"<&", T_LESSAND},    {">&", T_GREATAND},    {";;", T_DSEMI},
    {";&", T_SEMIAND},    {";;&", T_DSEMIAND},   {"<<-", T_DLESSDASH},
    {"<<<", T_TLESS},     {"&>", T_ANDGREAT},    {"&>>", T_ANDDGREAT},
    {"<>", T_LESSGREAT},  {">|", T_CLOBBER},     {"|&", T_BARAND},
    {"EOF", T_EOF},       {">", '>'},            {"<", '<'},
    {"-", '-'},           {"{", '{'},            {"}", '}'},
    {";", ';'},           {"(", '('},            {")", ')'},
    {"|", '|'},           {"&", '&'},            {"newline", '\n'},
};

// Spelling of a token for messages, or a null string.
static Str tokenname(i32 token)
{
    for (iz i = 0; i < countof(reserved); i++) {
        if (reserved[i].token == token) return reserved[i].word;
    }
    for (iz i = 0; i < countof(operators); i++) {
        if (operators[i].token == token) return operators[i].word;
    }
    return {};
}


// Lexer state

enum : i32 {  // parser state flags (bash's PST_*)
    PST_CASEPAT    = 1<<0,
    PST_ALEXPNEXT  = 1<<1,
    PST_ALLOWOPNBRC= 1<<2,
    PST_SUBSHELL   = 1<<3,
    PST_CMDSUBST   = 1<<4,
    PST_CASESTMT   = 1<<5,
    PST_CONDCMD    = 1<<6,
    PST_CONDEXPR   = 1<<7,
    PST_EXTPAT     = 1<<8,
    PST_COMPASSIGN = 1<<9,
    PST_ASSIGNOK   = 1<<10,
    PST_EOFTOKEN   = 1<<11,
    PST_REGEXP     = 1<<12,
    PST_HEREDOC    = 1<<13,
    PST_REDIRLIST  = 1<<14,
    PST_COMMENT    = 1<<15,
    PST_ENDALIAS   = 1<<16,
    PST_NOEXPAND   = 1<<17,
    PST_NOERROR    = 1<<18,
    PST_FUNSUBST   = 1<<19,
    PST_CMDBLTIN   = 1<<20,
};

struct Alias {
    Alias *child[4];
    Str    name;
    Str    value;
    b32    expanding;  // currently being expanded
    uz     seq;        // order of definition, for BASH_ALIASES
};

// Look up an alias by name. Returns null if not found or unaliased.
static Alias *findalias(Alias *t, Str name)
{
    for (u64 h = hash(name); t; h <<= 2) {
        if (t->name == name) return t->value.data ? t : 0;
        t = t->child[h>>62];
    }
    return 0;
}

struct Pushed {   // a string pushed onto the input: an alias or (( ... ))
    Pushed *next;
    Str     line;
    iz      idx;
    Alias  *alias;  // alias being expanded, if any
    b32     dparen;
    b32     expandnext;  // the alias value ended in a blank
};

struct Token {
    i32   kind;
    Word *word;    // T_WORD, T_ASSIGN, T_REDIRWORD, T_ARITHCMD, T_ARITHFOR
    i64   number;  // T_NUMBER
    Cond *cond;    // T_CONDCMD
};

enum : i32 { MSG_ERROR, MSG_WARNING };

struct Message {   // a diagnostic from the parser
    Message *next;
    i32      line;
    i32      kind;
    Str      text;
};

enum { HEREDOCMAX = 16, NESTMAX = 257 };

struct Lexer {
    Arena   *a;

    // Input
    Str      src;        // the whole input text
    iz       srcpos;     // next unread byte of src
    Str      line;       // current line, with its newline (shell_input_line)
    iz       idx;        // position in line
    b32      lineeof;    // the current "line" marks end of input
    i32      ungot;      // pushed-back character after the line
    Pushed  *pushed;
    i32      lineno;
    b32      unquotedbs; // unquoted_backslash
    b32      heredocstring;  // reading a here-document from alias text
    u8       dstack[64]; // open quoting delimiters, innermost last
    i32      ddepth;
    // Input read on demand: fill appends at least a line to src, and
    // returns false at the end of input
    b32    (*fill)(void *ctx, Lexer *);
    void    *fillctx;
    b32      newunit;    // the next fill starts a command (PS1, not PS2)

    // Token history and state
    i32      current;
    i32      last;
    i32      before;
    i32      twoago;
    i32      pst;
    i32      braces;     // open_brace_count
    i32      esacs;      // esacs_needed_count
    i32      expectin;   // expecting_in_token
    i32      expectcmd;  // expecting_in_command
    i32      eoftoken;   // shell_eof_token
    i32      toread;     // token_to_read
    Token    toreadval;
    Token    val;        // value of the current token (yylval)
    b32      eofreached;
    b32      discard;    // error that only discards the rest of the line
    b32      reported;   // the last error went through the generic report
    b32      aliasopt;   // expand_aliases as set by shopt
    b32      extglobopt; // extglob as set by shopt
    b32      comsuberr;  // syntax error inside a command substitution

    Redir   *heredocs[HEREDOCMAX];
    i32      nheredoc;

    struct { i32 token, lineno; } nest[NESTMAX];
    i32      ntop;

    // Options and environment
    b32      extglob;
    b32      extquote;   // translate $'...' in double-quoted ${...}
    b32      utf8;       // UTF-8 locale, else C
    b32      expandaliases;
    b32      posix;
    Alias  **aliases;

    // Diagnostics
    Message *msgs;
    Message **msgtail;
    void    *jmp[5];     // where syntax errors escape to

    // Recursion limit: bash has none and crashes on absurd nesting
    byte    *stacktop;   // stack position at entry to the parse
    iz       stackmax;   // bytes of stack the parse may use, 0 for any
    b32      overflow;   // the limit was hit
};

static void   synerror(Lexer *);
static Lexer *newlexer(Arena *, Str);
static Node  *parsecomlist(Lexer *);
static Str    printcomsub(Arena *, Node *);

static void message(Lexer *L, i32 line, Str text, i32 kind = MSG_ERROR)
{
    Message *m = alloc<Message>(L->a);
    m->line = line;
    m->kind = kind;
    m->text = text;
    *L->msgtail = m;
    L->msgtail = &m->next;
}

// Unwind to the parser's entry point after a syntax error.
[[noreturn]] static void parsefail(Lexer *L)
{
    __builtin_longjmp(L->jmp, 1);
}

// Give up on nesting deep enough to exhaust the stack, where bash would
// crash. The whole parse fails.
static void checkstack(Lexer *L)
{
    byte here = 0;
    if (L->stackmax && L->stacktop-&here > L->stackmax) {
        L->overflow = 1;
        parsefail(L);
    }
}


// Character classes

static b32 shellblank(i32 c) { return c==' ' || c=='\t'; }

static b32 shellmeta(i32 c)
{
    return c=='(' || c==')' || c=='<' || c=='>' || c==';' || c=='&' || c=='|';
}

static b32 shellbreak(i32 c)
{
    return shellmeta(c) || c==' ' || c=='\t' || c=='\n';
}

static b32 shellquote(i32 c) { return c=='\'' || c=='"' || c=='`'; }
static b32 shellexp(i32 c)   { return c=='$' || c=='<' || c=='>'; }

static b32 patternchar(i32 c)
{
    return c=='*' || c=='?' || c=='+' || c=='@' || c=='!';
}

static b32 funsubchar(i32 c)
{
    return c==' ' || c=='\t' || c=='\n' || c=='|';
}

static b32 identstart(u8 c) { return letter(c) || c=='_'; }
static b32 identchar(u8 c)  { return alnum(c) || c=='_'; }

static b32 validident(Str s)
{
    if (!s.len || !identstart(s[0])) return 0;
    for (iz i = 1; i < s.len; i++) {
        if (!identchar(s[i])) return 0;
    }
    return 1;
}

// Index of the closing ] of the subscript starting at s[i]=='[', or
// s.len if unterminated.
static iz skipsubscript(Str s, iz i)
{
    i32 depth = 0;
    for (; i < s.len; i++) {
        u8 c = s[i];
        if (c == '\\') {
            i++;
        } else if (c == '\'') {
            for (i++; i<s.len && s[i]!='\''; i++) {}
        } else if (c == '"') {
            for (i++; i<s.len && s[i]!='"'; i++) {
                if (s[i] == '\\') i++;
            }
        } else if (c=='$' && i+1<s.len && (s[i+1]=='(' || s[i+1]=='{')) {
            // a substitution may hold brackets: a[$(echo ])]=x
            u8  open  = s[i+1];
            u8  close = open=='(' ? ')' : '}';
            i32 inner = 0;
            for (i++; i < s.len; i++) {
                if (s[i] == '\\') {
                    i++;
                } else if (s[i] == open) {
                    inner++;
                } else if (s[i]==close && !--inner) {
                    break;
                }
            }
        } else if (c == '`') {
            for (i++; i<s.len && s[i]!='`'; i++) {
                if (s[i] == '\\') i++;
            }
        } else if (c == '[') {
            depth++;
        } else if (c == ']') {
            if (!--depth) return i;
        }
    }
    return s.len;
}

// name[subscript] with a non-empty subscript ending the string.
static b32 validarrayref(Str s)
{
    Cut c = cut(s, '[');
    if (!c.ok || !validident(c.head)) return 0;
    iz open = c.head.len;
    iz e    = skipsubscript(s, open);
    return e==s.len-1 && e>open+1;
}

// Index of the = in an assignment word, or 0 if not an assignment. In
// a compound assignment, only [sub]=value counts.
static iz assignment(Str s, b32 compound)
{
    if (!s.len) return 0;
    if (compound ? s[0]!='[' : !identstart(s[0])) return 0;
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c == '=') {
            return i;
        }
        if (c == '[') {
            iz e = skipsubscript(s, i);
            if (e==s.len) return 0;
            e++;
            if (e+1<s.len && s[e]=='+' && s[e+1]=='=') return e + 1;
            return e<s.len && s[e]=='=' ? e : 0;
        }
        if (c=='+' && i+1<s.len && s[i+1]=='=') {
            return i + 1;
        }
        if (!identchar(c)) {
            return 0;
        }
    }
    return 0;
}


// Character input (bash's shell_getc)

// Whether all input has been consumed, reading more if there is any.
static b32 srcend(Lexer *L)
{
    while (L->srcpos==L->src.len && L->fill) {
        if (!L->fill(L->fillctx, L)) L->fill = 0;
    }
    return L->srcpos == L->src.len;
}

// Load the next line of the input into L->line, appending a newline to
// an unterminated last line. With remove set, a final backslash is
// doubled instead so that it is not taken as a line continuation.
static void nextline(Lexer *L, b32 remove)
{
    L->lineno++;
    L->idx  = 0;
    if (srcend(L)) {
        L->line    = {};
        L->lineeof = 1;
        return;
    }
    // Copy the line, privately since ungetting writes characters back,
    // dropping NUL bytes as bash does. An unterminated last line gets a
    // newline, or a doubled final backslash when continuations apply.
    Str rest = cuthead(L->src, L->srcpos);
    Cut c    = cut(rest, '\n');
    Str s    = {};
    s.data = allocbytes(L->a, c.head.len+1);
    b32 bs = 0;
    for (iz i = 0; i < c.head.len; i++) {
        u8 b = c.head[i];
        if (b) {
            s.data[s.len++] = b;
            bs = !bs && b=='\\';
        }
    }
    s.data[s.len++] = !c.ok && bs && remove ? '\\' : '\n';
    L->line    = s;
    L->srcpos += c.head.len + c.ok;
}

static void pushstring(Lexer *L, Str s, Alias *alias, b32 dparen)
{
    Pushed *p = alloc<Pushed>(L->a);
    p->line   = L->line;
    p->idx    = L->idx;
    p->alias  = alias;
    p->dparen = dparen;
    p->next   = L->pushed;
    L->pushed = p;
    L->line   = clone(L->a, s);
    L->idx    = 0;
    if (alias) {
        alias->expanding = 1;
        p->expandnext = alias->value.len && shellblank(alias->value[alias->value.len-1]);
    }
}

static void popstring(Lexer *L)
{
    Pushed *p = L->pushed;
    if (p->expandnext) {
        L->pst |= PST_ALEXPNEXT;
    }
    if (p->alias) {
        p->alias->expanding = 0;
    }
    L->line   = p->line;
    L->idx    = p->idx;
    L->pushed = p->next;
}

static void pushdelim(Lexer *L, i32 c)
{
    if (L->ddepth < (i32)sizeof(L->dstack)) {
        L->dstack[L->ddepth] = (u8)c;
    }
    L->ddepth++;
}

static void popdelim(Lexer *L)
{
    if (L->ddepth) L->ddepth--;
}

static i32 currentdelim(Lexer *L)
{
    i32 d = L->ddepth;
    return d && d<=(i32)sizeof(L->dstack) ? L->dstack[d-1] : 0;
}

static b32 expandingalias(Lexer *L)
{
    for (Pushed *p = L->pushed; p; p = p->next) {
        if (p->alias) return 1;
    }
    return 0;
}

static i32 shellgetc(Lexer *L, b32 remove)
{
    if (L->ungot) {
        i32 c = L->ungot;
        L->ungot = 0;
        return c;
    }

    if (L->lineeof && !L->pushed) {
        return -1;
    }
    if (L->idx==L->line.len && !L->pushed) {
        nextline(L, remove);
        if (L->lineeof) {
            return -1;
        }
    }

    for (;;) {
        if (!L->idx) {
            L->unquotedbs = 0;
        }
        i32 c = L->idx<L->line.len ? L->line[L->idx] : 0;
        if (c) {
            L->unquotedbs = !L->unquotedbs && c=='\\';
            L->idx++;
        }

        if (!c && L->pushed && !L->pushed->dparen &&
                !(L->pst & (PST_COMMENT|PST_ENDALIAS)) && !L->heredocstring &&
                currentdelim(L)!='\'' && currentdelim(L)!='"' &&
                L->idx>0 && !shellblank(L->line[L->idx-1]) &&
                L->line[L->idx-1]!='\n' && !L->unquotedbs &&
                !shellmeta(L->line[L->idx-1])) {
            // Delimit the alias's last word, keeping the alias marked
            // as in use until that word is checked for expansion.
            L->pst |= PST_ENDALIAS;
            return ' ';
        }

        if (!c && L->pushed) {
            L->pst &= ~PST_ENDALIAS;
            popstring(L);
            if (L->idx==L->line.len && !L->pushed) {
                if (L->lineeof) return -1;
                nextline(L, remove);
                if (L->lineeof) return -1;
            }
            continue;
        }

        if (c=='\\' && remove && L->idx<L->line.len && L->line[L->idx]=='\n') {
            if (expandingalias(L)) {
                L->lineno++;
                L->idx++;
                continue;
            }
            nextline(L, remove);  // counts the line
            if (L->lineeof) {
                return -1;
            }
            continue;
        }

        if (!c) {
            return -1;
        }
        return c;
    }
}

static void shellungetc(Lexer *L, i32 c)
{
    if (c < 0) {
        return;
    }
    if (L->idx) {
        L->line[--L->idx] = (u8)c;  // like bash, even a different char
    } else {
        L->ungot = c;
    }
}

// The current line, without its newline, for error messages.
static Str currentline(Lexer *L)
{
    Str s = L->line;
    for (; s.len && s[s.len-1]=='\n'; s.len--) {}
    return s;
}

// Guess the offending token from the text before the read position,
// for errors that have no token (bash's error_token_from_text).
static Str errortokenfromtext(Lexer *L)
{
    Str t = L->line;
    iz  i = L->idx;
    iz  e = 0;
    #define AT(k) ((k) < t.len ? t[k] : 0)
    if (i && !AT(i)) i--;
    while (i && (shellblank(AT(i)) || AT(i)=='\n')) i--;
    if (i) e = i + 1;
    while (i && find(" \n\t;|&", Str{t.data+i, 1})<0) i--;
    while (i!=e && (shellblank(AT(i)) || AT(i)=='\n')) i++;
    #undef AT
    if (e) {
        return span(t.data+i, t.data+e);
    }
    if (!i && t.len) {
        return takehead(t, 1);
    }
    return {};
}


// Token buffer

struct Tokbuf {
    u8 *data;
    iz  len;
    iz  cap;
};

static void tokput(Lexer *L, Tokbuf *t, u8 c)
{
    if (t->len == t->cap) {
        iz cap = t->cap ? 2*t->cap : 64;
        u8 *data = allocbytes(L->a, cap);
        copybytes(data, t->data, t->len);
        t->data = data;
        t->cap  = cap;
    }
    t->data[t->len++] = c;
}

static void tokputs(Lexer *L, Tokbuf *t, Str s)
{
    for (iz i = 0; i < s.len; i++) {
        tokput(L, t, s[i]);
    }
}

static Str tokstr(Tokbuf *t)
{
    return {t->data, t->len};
}


// Quoting helpers used while lexing

// Wrap s in single quotes, escaping embedded single quotes.
static Str singlequote(Arena *a, Str s)
{
    if (s == "'") {
        return clone(a, "\\'");
    }
    Buf b(a, s.len+2);
    putbyte(&b, '\'');
    for (iz i = 0; i < s.len; i++) {
        putbyte(&b, s[i]);
        if (s[i] == '\'') {
            print(&b, "\\''");
        }
    }
    putbyte(&b, '\'');
    return finish(&b);
}

// bash's internal form of word text: a CTLESC (0x01) before each 0x01
// and 0x7f byte. It is visible in a few places, such as error messages.
static Str internalform(Arena *a, Str s)
{
    Buf b(a, s.len+8);
    for (iz i = 0; i < s.len; i++) {
        if (s[i]==1 || s[i]==0x7f) putbyte(&b, 1);
        putbyte(&b, s[i]);
    }
    return finish(&b);
}

// Undo the internal form: drop a CTLESC before another CTLESC or a
// CTLNUL (bash's dequote_escapes). Other 0x01 bytes, as in raw
// here-document bodies, are kept.
static Str removeescapes(Arena *a, Str s)
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        if (s[i]==1 && i+1<s.len && (s[i+1]==1 || s[i+1]==0x7f)) i++;
        putbyte(&b, s[i]);
    }
    return finish(&b);
}

// Does s need $'...' quoting to be printable? C locale: any byte
// outside printable ASCII. TODO: UTF-8 locale with iswprint tables.
static b32 ansicshouldquote(Str s)
{
    for (iz i = 0; i < s.len; i++) {
        if (s[i]<0x20 || s[i]>0x7e) return 1;
    }
    return 0;
}

// Quote s as $'...', escaping unprintable bytes.
static Str ansicquote(Arena *a, Str s)
{
    Buf b(a, 4*s.len+4);
    print(&b, "$'");
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        u8 e = 0;
        switch (c) {
        case 27:   e = 'E'; break;
        case 7:    e = 'a'; break;
        case 11:   e = 'v'; break;
        case 8:    e = 'b'; break;
        case 12:   e = 'f'; break;
        case '\n': e = 'n'; break;
        case '\r': e = 'r'; break;
        case '\t': e = 't'; break;
        case '\\':
        case '\'': e = c;   break;
        }
        if (e) {
            putbyte(&b, '\\');
            putbyte(&b, e);
        } else if (c<0x20 || c>0x7e) {
            putbyte(&b, '\\');
            putbyte(&b, (u8)('0' + (c>>6 & 7)));
            putbyte(&b, (u8)('0' + (c>>3 & 7)));
            putbyte(&b, (u8)('0' + (c    & 7)));
        } else {
            putbyte(&b, c);
        }
    }
    putbyte(&b, '\'');
    return finish(&b);
}

static i32 xdigitval(u8 c)
{
    return digit(c) ? c-'0' : (lowercase(c)-'a') + 10;
}

static b32 ishexdigit(u8 c)
{
    return digit(c) || (lowercase(c)>='a' && lowercase(c)<='f');
}

// Translate ANSI-C escapes as in $'...'. Stops at a NUL result.
static Str ansiexpand(Arena *a, Str s, b32 utf8, b32 keepnul = 0)
{
    Buf b(a, s.len+4);
    for (iz i = 0; i < s.len;) {
        u8 c = s[i++];
        if (c!='\\' || i==s.len) {
            putbyte(&b, c);
            continue;
        }
        c = s[i++];
        i32 v = 0;
        switch (c) {
        case 'a': v = 7;    break;
        case 'v': v = 11;   break;
        case 'b': v = 8;    break;
        case 'e':
        case 'E': v = 27;   break;
        case 'f': v = 12;   break;
        case 'n': v = '\n'; break;
        case 'r': v = '\r'; break;
        case 't': v = '\t'; break;
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            v = c - '0';
            for (i32 n = 2; n && i<s.len && s[i]>='0' && s[i]<='7'; n--) {
                v = v*8 + (s[i++] - '0');
            }
            v &= 0xff;
            break;
        case 'x': {
            i32 n = 0;
            if (i<s.len && s[i]=='{') {
                iz  j = i + 1;
                u32 x = 0;  // wraps like bash's int
                for (; j<s.len && ishexdigit(s[j]); j++) {
                    x = x*16 + (u32)xdigitval(s[j]);
                }
                i = j<s.len && s[j]=='}' ? j+1 : j;
                v = (i32)(x & 0xff);
                break;
            }
            for (; n<2 && i<s.len && ishexdigit(s[i]); n++) {
                v = v*16 + xdigitval(s[i++]);
            }
            if (!n) {
                putbyte(&b, '\\');
                v = 'x';
            }
            v &= 0xff;
        } break;
        case 'u':
        case 'U': {
            i32 max = c=='u' ? 4 : 8;
            i32 n   = 0;
            u32 u   = 0;
            for (; n<max && i<s.len && ishexdigit(s[i]); n++) {
                u = u*16 + (u32)xdigitval(s[i++]);
            }
            if (!n) {
                putbyte(&b, '\\');
                v = c;
            } else if (u <= 0x7f) {
                v = (i32)u;
            } else if (utf8 && u <= 0x10ffff) {
                u8 tmp[4];
                iz len = utf8encode(tmp, (i32)u);
                print(&b, Str{tmp, len});
                continue;
            } else {
                // Not representable: an escape, in uppercase hex
                i32 digits = u<=0xffff ? 4 : 8;
                putbyte(&b, '\\');
                putbyte(&b, u<=0xffff ? 'u' : 'U');
                for (i32 k = digits-1; k >= 0; k--) {
                    putbyte(&b, "0123456789ABCDEF"[u>>(4*k) & 15]);
                }
                continue;
            }
        } break;
        case '\\': case '\'': case '"': case '?':
            v = c;
            break;
        case 'c':
            if (i < s.len) {
                u8 x = s[i++];
                if (x=='\\' && i<s.len && s[i]=='\\') i++;  // $'\c\\'
                u8 up = x>='a' && x<='z' ? (u8)(x - 32) : x;
                v = x=='?' ? 0x7f : up&0x1f;
                break;
            }
            putbyte(&b, '\\');
            v = 'c';
            break;
        default:
            putbyte(&b, '\\');
            v = c;
        }
        putbyte(&b, (u8)v);
    }
    Str r = finish(&b);
    for (iz i = 0; !keepnul && i < r.len; i++) {
        if (!r[i]) return takehead(r, i);  // C string semantics
    }
    return r;
}


// Reserved words and special tokens

static b32 reservedok(Lexer *L, i32 t)  // reserved_word_acceptable
{
    switch (t) {
    case '\n': case ';': case '(': case ')': case '|': case '&':
    case '{': case '}': case T_ANDAND: case T_ARITHCMD: case T_BANG:
    case T_BARAND: case T_CONDEND: case T_DO: case T_DONE: case T_ELIF:
    case T_ELSE: case T_ESAC: case T_FI: case T_IF: case T_OROR:
    case T_DSEMI: case T_SEMIAND: case T_DSEMIAND: case T_THEN:
    case T_TIME: case T_TIMEOPT: case T_TIMEIGN: case T_COPROC:
    case T_UNTIL: case T_WHILE: case 0: case T_DOLPAREN: case T_DOLBRACE:
        return 1;
    }
    if (L->last==T_WORD && L->before==T_COPROC) return 1;
    if (L->last==T_WORD && L->before==T_FUNCTION) return 1;
    return 0;
}

static b32 parsingredir(i32 t)
{
    return t=='<' || t=='>' || t==T_DGREAT || t==T_CLOBBER ||
           t==T_LESSGREAT || t==T_DLESSDASH || t==T_DLESS ||
           t==T_TLESS || t==T_LESSAND || t==T_GREATAND || t==T_ANDGREAT;
}

static b32 cmdposition(Lexer *L, i32 t)  // command_token_position
{
    return t==T_ASSIGN ||
           ((L->pst & PST_REDIRLIST) && !parsingredir(t)) ||
           (t!=T_DSEMI && t!=T_SEMIAND && t!=T_DSEMIAND && reservedok(L, t));
}

static b32 assignok(Lexer *L, i32 t)  // assignment_acceptable
{
    return cmdposition(L, t) && !(L->pst & PST_CASEPAT);
}

static void pushnest(Lexer *L, i32 t)  // set_compoundcmd_top
{
    switch (t) {
    case T_CASE: case T_SELECT: case T_FOR: case T_IF: case T_WHILE:
    case T_UNTIL: case '{': case '(': case T_CONDSTART:
        if (L->ntop < NESTMAX-1) L->ntop++;
        L->nest[L->ntop].lineno = L->lineno;
        L->nest[L->ntop].token  = t;
        break;
    }
}

static void popnest(Lexer *L)
{
    if (L->ntop >= 0) L->ntop--;
}

static b32 timeok(Lexer *L)  // time_command_acceptable
{
    if (L->posix) {
        iz i = L->idx;
        for (; i<L->line.len && shellblank(L->line[i]); i++) {}
        if (i<L->line.len && L->line[i]=='-') return 0;
    }
    switch (L->last) {
    case 0: case ';': case '\n':
        if (L->before == '|') return 0;
        return 1;
    case T_ANDAND: case T_OROR: case '&': case T_WHILE: case T_DO:
    case T_UNTIL: case T_IF: case T_THEN: case T_ELIF: case T_ELSE:
    case '{': case '(': case ')': case T_BANG: case T_TIME:
    case T_TIMEOPT: case T_TIMEIGN: case T_DOLPAREN: case T_DOLBRACE:
        return 1;
    }
    return 0;
}

// Returns a reserved-word token, or 0 if s is not one here.
static i32 checkreserved(Lexer *L, Str s, b32 dollar, b32 quoted)
{
    if (dollar || quoted || !reservedok(L, L->last)) {
        return 0;
    }
    for (iz i = 0; i < countof(reserved); i++) {
        if (!(reserved[i].word == s)) continue;
        i32 t = reserved[i].token;
        if ((L->pst & PST_CASEPAT) && t!=T_ESAC) return 0;
        if (t==T_TIME && !timeok(L)) return 0;
        if ((L->pst & PST_CASEPAT) && L->last=='|' && t==T_ESAC) return 0;
        if ((L->pst & PST_CASEPAT) && L->last=='(' && t==T_ESAC) return 0;
        switch (t) {
        case T_ESAC:
            L->pst &= ~(PST_CASEPAT|PST_CASESTMT);
            L->esacs--;
            break;
        case T_CASE:
            L->pst |= PST_CASESTMT;
            L->expectcmd = T_CASE;
            break;
        case T_CONDEND:
            L->pst &= ~(PST_CONDCMD|PST_CONDEXPR);
            break;
        case T_CONDSTART:
            L->pst |= PST_CONDCMD;
            break;
        case T_FOR:
            L->expectcmd = T_FOR;
            break;
        case '{':
            L->braces++;
            break;
        case '}':
            if (L->braces) L->braces--;
            break;
        }
        pushnest(L, t);
        if (L->posix) {
            L->pst &= ~PST_ALEXPNEXT;
        }
        return t;
    }
    return 0;
}

static i32 specialcase(Lexer *L, Str s)  // special_case_tokens
{
    if (L->last==T_WORD && s=="in" &&
            (L->before==T_FOR || L->before==T_CASE || L->before==T_SELECT)) {
        if (L->expectcmd == T_CASE) {
            L->pst |= PST_CASEPAT;
            L->esacs++;
        }
        if (L->expectin) L->expectin--;
        L->expectcmd = 0;
        return T_IN;
    }
    if (L->expectin && (L->last==T_WORD || L->last=='\n') && s=="in") {
        if (L->expectcmd==T_CASE && (L->pst & PST_CASESTMT)) {
            L->pst |= PST_CASEPAT;
            L->esacs++;
        }
        L->expectin--;
        L->expectcmd = 0;
        return T_IN;
    } else if (L->expectin && (L->last=='\n' || L->last==';') && s=="do") {
        L->expectin--;
        L->expectcmd = 0;
        return T_DO;
    }
    if (L->last==T_WORD && s=="do" &&
            (L->before==T_FOR || L->before==T_SELECT)) {
        if (L->expectin) L->expectin--;
        L->expectcmd = 0;
        return T_DO;
    }
    if (L->esacs && L->last==T_IN && s=="esac") {
        L->esacs--;
        L->pst &= ~PST_CASEPAT;
        return T_ESAC;
    }
    if (L->pst & PST_ALLOWOPNBRC) {
        L->pst &= ~PST_ALLOWOPNBRC;
        if (s == "{") {
            L->braces++;
            return '{';
        }
    }
    if (L->last==T_ARITHFOR && s=="do") {
        return T_DO;
    }
    if (L->last==T_ARITHFOR && s=="{") {
        L->braces++;
        return '{';
    }
    if (L->braces && reservedok(L, L->last) && s=="}") {
        L->braces--;
        return '}';
    }
    if (L->last==T_TIME && s=="-p")   return T_TIMEOPT;
    if (L->last==T_TIME && s=="--")   return T_TIMEIGN;
    if (L->last==T_TIMEOPT && s=="--") return T_TIMEIGN;
    if ((L->pst & PST_CONDEXPR) && s=="]]") return T_CONDEND;
    return -1;
}


// Grouping constructs (bash's parse_matched_pair)

enum : i32 {
    P_FIRSTCLOSE = 1<<0,
    P_ALLOWESC   = 1<<1,
    P_DQUOTE     = 1<<2,
    P_COMMAND    = 1<<3,
    P_BACKQUOTE  = 1<<4,
    P_ARRAYSUB   = 1<<5,
    P_DOLBRACE   = 1<<6,
    P_ARITH      = 1<<7,
};

enum : i32 { DB_PARAM = 1, DB_OP, DB_WORD, DB_QUOTE, DB_QUOTE2 };

struct Matched {   // result of a grouping construct
    Str  text;     // contents including the closing delimiter
    b32  ok;
};

static Matched parsecomsub(Lexer *, i32 qc, i32 open, i32 close, i32 flags);

static void eofmatching(Lexer *L, i32 line, i32 close)
{
    if (!(L->pst & PST_NOERROR)) {
        Buf b(L->a, 64);
        print(&b, "unexpected EOF while looking for matching `");
        putbyte(&b, (u8)close);
        print(&b, "'");
        message(L, line, finish(&b));
    }
    L->eofreached = 1;
    L->pst |= PST_NOERROR;
}

static Str parsematched(Lexer *L, i32 qc, i32 open, i32 close, i32 flags)
{
    checkstack(L);
    i32 count  = 1;
    b32 ckcomment = (flags & P_COMMAND) && qc!='`' && qc!='\'' && qc!='"' &&
                    !(flags & P_DQUOTE);
    b32 incomment = 0;
    b32 passnext  = 0;
    b32 wasdol    = 0;
    b32 gtlt      = 0;
    i32 rflags    = qc=='"' ? P_DQUOTE : (flags & P_DQUOTE);
    i32 dolstate  = (flags & P_DOLBRACE) ? DB_PARAM : 0;
    i32 start     = L->lineno;
    Tokbuf r      = {};

    while (count) {
        i32 ch = shellgetc(L, qc!='\'' && !passnext);
        if (ch < 0) {
            eofmatching(L, start, close);
            return {};
        }

        if (incomment) {
            tokput(L, &r, (u8)ch);
            if (ch == '\n') incomment = 0;
            continue;
        } else if (ckcomment && ch=='#' &&
                   (!r.len || r.data[r.len-1]=='\n' || shellblank(r.data[r.len-1]))) {
            incomment = 1;
        }

        if (passnext) {
            passnext = 0;
            if (qc!='\'' && ch=='\n') {
                if (r.len) r.len--;  // \<newline> disappears
                continue;
            }
            tokput(L, &r, (u8)ch);
            continue;
        } else if (ch == close) {
            count--;
        } else if (open!=close && wasdol && open=='{' && ch==open) {
            count++;
        } else if (!(flags & P_FIRSTCLOSE) && ch==open) {
            count++;
        }

        tokput(L, &r, (u8)ch);
        if (!count) {
            break;
        }

        if (open == '\'') {
            if ((flags & P_ALLOWESC) && ch=='\\') passnext = 1;
            continue;
        }
        if (ch == '\\') {
            passnext = 1;
        }

        if (flags & P_DOLBRACE) {
            if (dolstate==DB_PARAM && ch=='%' && r.len>1) {
                dolstate = DB_QUOTE;
            } else if (dolstate==DB_PARAM && ch=='#' && r.len>1) {
                dolstate = DB_QUOTE;
            } else if (dolstate==DB_PARAM && ch=='/' && r.len>1) {
                dolstate = DB_QUOTE2;
            } else if (dolstate==DB_PARAM && ch=='^' && r.len>1) {
                dolstate = DB_QUOTE;
            } else if (dolstate==DB_PARAM && ch==',' && r.len>1) {
                dolstate = DB_QUOTE;
            } else if (dolstate==DB_PARAM && find("#%^,~:-=?+/", Str{(u8 *)&ch, 1})>=0) {
                dolstate = DB_OP;
            } else if (dolstate==DB_OP && find("#%^,~:-=?+/", Str{(u8 *)&ch, 1})<0) {
                dolstate = DB_WORD;
            }
        }

        if (open != close) {
            if (ch=='\'' && L->posix && !wasdol && (flags & P_DQUOTE) &&
                    (flags & P_DOLBRACE) && dolstate!=DB_QUOTE && dolstate!=DB_QUOTE2) {
                // POSIX: a single quote in a double-quoted ${...} is
                // literal, except in a pattern. Only quotes passed down
                // count, not those around a command substitution (which
                // leave the delimiter qc a double quote).
                wasdol = gtlt = 0;
                continue;
            }
            if (shellquote(ch)) {
                Str nest;
                pushdelim(L, ch);
                if (wasdol && ch=='\'') {
                    nest = parsematched(L, ch, ch, ch, P_ALLOWESC|rflags);
                } else {
                    nest = parsematched(L, ch, ch, ch, rflags);
                }
                popdelim(L);
                if (!nest.data) {
                    return {};
                }
                if (wasdol && ch=='\'' && (L->extquote || !(rflags & P_DQUOTE) ||
                        dolstate==DB_QUOTE || dolstate==DB_QUOTE2)) {
                    // Translate $'...' here.
                    // The bare insertion keeps a NUL, which later ends the
                    // whole token; the quoted forms stop at it.
                    Str t = ansiexpand(L->a, cuttail(nest, 1), L->utf8, 1);
                    Str q = t;
                    for (iz k = 0; k < q.len; k++) {
                        if (!q[k]) q = takehead(q, k);
                    }
                    if ((rflags & P_DQUOTE) &&
                            (dolstate==DB_QUOTE2 || dolstate==DB_QUOTE) &&
                            (flags & P_DOLBRACE)) {
                        nest = singlequote(L->a, q);
                    } else if (!(rflags & P_DQUOTE)) {
                        nest = singlequote(L->a, q);
                    } else {
                        nest = t;
                    }
                    r.len -= 2;  // back up before the $'
                } else if (wasdol && ch=='"' && (L->extquote || !(rflags & P_DQUOTE))) {
                    // Locale translation is the identity: keep "..."
                    Tokbuf t = {};
                    tokput(L, &t, '"');
                    tokputs(L, &t, cuttail(nest, 1));
                    tokput(L, &t, '"');
                    nest = tokstr(&t);
                    r.len -= 2;  // back up before the $"
                }
                tokputs(L, &r, nest);
                wasdol = gtlt = 0;
                continue;
            } else if ((flags & (P_ARRAYSUB|P_DOLBRACE)) && wasdol &&
                       (ch=='(' || ch=='{' || ch=='[')) {
                goto dollarword;
            } else if ((flags & P_ARITH) && wasdol && ch=='(') {
                goto dollarword;
            } else if ((flags & P_ARITH) && wasdol && ch=='{') {
                i32 n = shellgetc(L, 1);
                shellungetc(L, n);
                if (funsubchar(n)) goto dollarword;
            } else if ((flags & (P_ARRAYSUB|P_DOLBRACE)) && gtlt && ch=='(') {
                goto dollarword;
            }
        } else if (open=='"' && ch=='`') {
            Str nest = parsematched(L, 0, '`', '`', rflags);
            if (!nest.data) {
                return {};
            }
            tokputs(L, &r, nest);
        } else if (open!='`' && wasdol && (ch=='(' || ch=='{' || ch=='[')) {
            goto dollarword;
        }

        gtlt   = (ch=='<' || ch=='>') && !gtlt;
        wasdol = ch=='$' && !wasdol;
        continue;

    dollarword:
        if (open == ch) {
            count--;  // undo previous increment
        }
        {
            Str nest = {};
            if (ch == '(') {
                Matched m = parsecomsub(L, 0, '(', ')', (rflags|P_COMMAND) & ~P_DQUOTE);
                nest = m.ok ? m.text : Str{};
            } else if (ch == '{') {
                i32 n = shellgetc(L, 1);
                shellungetc(L, n);
                if (funsubchar(n)) {
                    Matched m = parsecomsub(L, 0, '{', '}', rflags|P_COMMAND);
                    nest = m.ok ? m.text : Str{};
                } else {
                    nest = parsematched(L, 0, '{', '}', P_FIRSTCLOSE|P_DOLBRACE|rflags);
                }
            } else {
                nest = parsematched(L, 0, '[', ']', rflags|P_ARITH);
            }
            if (!nest.data) {
                return {};
            }
            tokputs(L, &r, nest);
        }
        gtlt = wasdol = 0;
    }
    return tokstr(&r);
}


// Here documents

static Str quoteremoval(Arena *a, Str s)
{
    Buf b(a, s.len);
    i32 quote = 0;
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (quote == '\'') {
            if (c == '\'') quote = 0;
            else putbyte(&b, c);
        } else if (c == '\\') {
            if (++i == s.len) break;
            if (quote=='"' && s[i]!='$' && s[i]!='`' && s[i]!='"' &&
                    s[i]!='\\' && s[i]!='\n') {
                putbyte(&b, '\\');
            }
            putbyte(&b, s[i]);
        } else if (quote=='"' && c=='"') {
            quote = 0;
        } else if (!quote && (c=='\'' || c=='"')) {
            quote = c;
        } else {
            putbyte(&b, c);
        }
    }
    return finish(&b);
}

// Read one raw line of input after the current line, for a here
// document. Returns a null string at end of input.
static Str readrawline(Lexer *L, b32 remove)
{
    if (srcend(L)) {
        return {};
    }
    Buf b(L->a, 64);
    for (b32 passnext = 0;;) {
        if (srcend(L)) {
            putbyte(&b, '\n');
            break;
        }
        u8 c = L->src[L->srcpos++];
        if (!c) {
            continue;  // NUL bytes are dropped from input
        }
        if (passnext) {
            putbyte(&b, c);
            passnext = 0;
        } else if (c=='\\' && remove) {
            if (srcend(L)) {
                putbyte(&b, '\\');
                putbyte(&b, '\\');
                putbyte(&b, '\n');
                break;
            }
            if (L->src[L->srcpos] == '\n') {
                L->srcpos++;
                L->lineno++;
                continue;
            }
            passnext = 1;
            putbyte(&b, c);
        } else {
            putbyte(&b, c);
        }
        if (c=='\n' && !passnext) {
            break;
        }
    }
    return finish(&b);
}

// Read a here-document line through shellgetc, for a body that starts
// inside alias text (bash's read_a_line with heredoc_string set).
static Str readgetcline(Lexer *L, b32 remove)
{
    Buf b(L->a, 64);
    for (b32 passnext = 0;;) {
        i32 c = shellgetc(L, 0);
        if (c < 0) {
            if (!b.len) {
                return {};
            }
            c = '\n';
        }
        if (passnext) {
            putbyte(&b, (u8)c);
            passnext = 0;
        } else if (c=='\\' && remove) {
            i32 peek = shellgetc(L, 0);
            if (peek == '\n') {
                continue;
            } else if (peek < 0) {
                putbyte(&b, (u8)c);
                break;
            }
            shellungetc(L, peek);
            passnext = 1;
            putbyte(&b, (u8)c);
        } else {
            putbyte(&b, (u8)c);
        }
        if (c == '\n') {
            break;
        }
    }
    return finish(&b);
}

// Push text back in front of the remaining input (bash's shell_ungets).
static void shellungets(Lexer *L, Str s)
{
    Str rest = cuthead(L->line, L->idx<L->line.len ? L->idx : L->line.len);
    Str line = {};
    line.data = allocbytes(L->a, s.len+rest.len);
    line.len  = s.len + rest.len;
    copybytes(line.data, s.data, s.len);
    copybytes(line.data+s.len, rest.data, rest.len);
    L->line = line;
    L->idx  = 0;
}

static void readheredoc(Lexer *L, Redir *r, i32 lineno)
{
    b32 strip    = r->op == R_HEREDOCSTRIP;
    b32 unquoted = !(r->word->flags & W_QUOTED);
    Str spec     = r->gathered ? r->body : r->word->text;  // re-read: body
    if (unquoted) {
        r->eof = spec;
    } else {
        // Quote removal keeps bash's internal escapes, so a quoted
        // delimiter holding 0x01 or 0x7f never matches a raw line.
        r->eof = internalform(L->a, quoteremoval(L->a, spec));
    }
    Str eof = r->eof;

    L->heredocstring = expandingalias(L) && L->idx<L->line.len;
    Buf doc(L->a, 64);
    b32 found = 0;
    for (;;) {
        Str line = L->heredocstring ? readgetcline(L, unquoted)
                                    : readrawline(L, unquoted);
        if (!line.data) {
            if (L->heredocstring) {
                L->lineno--;
                L->lineeof = 0;  // the parser's next read counts EOF again
            }
            break;
        }
        if (!L->heredocstring) {
            L->lineno++;
        }
        if (strip && line.len) {
            if (startswith(line, eof) && line.len==eof.len+1 && line[eof.len]=='\n') {
                found = 1;
                break;
            }
            for (; line.len && line[0]=='\t'; line = cuthead(line, 1)) {}
        }
        if (!line.len) {
            continue;
        }
        if (startswith(line, eof) && line.len==eof.len+1 && line[eof.len]=='\n') {
            found = 1;
            break;
        }
        // Backwards compatibility: in a command substitution, the
        // delimiter followed by the closing character ends the body.
        if (startswith(line, eof) && (L->pst & PST_EOFTOKEN) && L->eoftoken) {
            u8  close = (u8)L->eoftoken;
            Str rest  = cuthead(line, eof.len);
            if (find(rest, Str{&close, 1}) >= 0) {
                shellungets(L, rest);
                break;
            }
        }
        print(&doc, line);
    }
    L->heredocstring = 0;
    r->gathered = 1;
    if (!found) {
        Buf b(L->a, 64);
        print(&b, "here-document at line ");
        print(&b, (i64)lineno);
        print(&b, " delimited by end-of-file (wanted `");
        print(&b, eof);
        print(&b, "')");
        message(L, L->lineno, finish(&b), MSG_WARNING);
    }
    r->body = finish(&doc);
}

static void gatherheredocs(Lexer *L)
{
    for (i32 i = 0; i < L->nheredoc; i++) {
        L->pst |= PST_HEREDOC;
        readheredoc(L, L->heredocs[i], L->lineno);
        L->pst &= ~PST_HEREDOC;
        L->heredocs[i] = 0;
    }
    L->nheredoc = 0;
}

static void pushheredoc(Lexer *L, Redir *r)
{
    if (L->nheredoc == HEREDOCMAX) {
        message(L, L->lineno, "maximum here-document count exceeded");
        parsefail(L);
    }
    L->heredocs[L->nheredoc++] = r;
}


// Words (bash's read_token_word)

static Word *newword(Arena *a, Str text, i32 flags)
{
    Word *w = alloc<Word>(a);
    w->text  = text;
    w->flags = flags;
    return w;
}

static Str parsecompoundassign(Lexer *);

static i32 readtoken(Lexer *L);

static i32 readtokenword(Lexer *L, i32 ch)
{
    Tokbuf tok      = {};
    b32    alldigit = digit((u8)ch);
    b32    dollar   = 0;
    b32    quoted   = 0;
    b32    compound = 0;
    b32    passnext = 0;

    for (;;) {
        if (ch < 0) {
            goto gottoken;
        }

        if (passnext) {
            passnext = 0;
            goto escaped;
        }

        if (ch == '\\') {
            if (L->pst & PST_NOEXPAND) {
                passnext = 1;
                quoted   = 1;
                goto character;
            }
            i32 peek = shellgetc(L, 0);
            if (peek == '\n') {
                ch = '\n';
                goto next;
            }
            shellungetc(L, peek);
            // Inside a construct other than quotes (e.g. a compound
            // assignment in $( )), the next character is lexed normally.
            i32 cd = currentdelim(L);
            if (!cd || cd=='`' || (cd=='"' && peek>=0 &&
                    find("$`\"\\\n", Str{(u8 *)&peek, 1})>=0)) {
                passnext = 1;
            }
            quoted = 1;
            goto character;
        }

        if ((L->pst & PST_FUNSUBST) && !tok.len && !quoted &&
                reservedok(L, L->last) && ch=='}') {
            tokput(L, &tok, '}');
            alldigit = dollar = 0;
            goto gottoken;
        }

        if (shellquote(ch)) {
            pushdelim(L, ch);
            Str t = parsematched(L, ch, ch, ch, ch=='`' ? P_COMMAND : 0);
            popdelim(L);
            if (!t.data) {
                return T_ERROR;
            }
            tokput(L, &tok, (u8)ch);
            tokputs(L, &tok, t);
            alldigit = 0;
            if (ch != '`') {
                quoted = 1;
            }
            if (ch=='"' && find(t, "$")>=0) {
                dollar = 1;
            }
            goto next;
        }

        if ((L->pst & PST_REGEXP) && (ch=='(' || ch=='|')) {
            if (ch == '|') {
                goto character;
            }
            i32 cd = currentdelim(L);
            pushdelim(L, ch);
            Str t = parsematched(L, cd, '(', ')', 0);
            popdelim(L);
            if (!t.data) {
                return T_ERROR;
            }
            tokput(L, &tok, (u8)ch);
            tokputs(L, &tok, t);
            dollar = alldigit = 0;
            goto next;
        }

        if (L->extglob && patternchar(ch)) {
            i32 peek = shellgetc(L, 1);
            if (peek == '(') {
                i32 cd = currentdelim(L);
                pushdelim(L, peek);
                Str t = parsematched(L, cd, '(', ')', 0);
                popdelim(L);
                if (!t.data) {
                    return T_ERROR;
                }
                tokput(L, &tok, (u8)ch);
                tokput(L, &tok, (u8)peek);
                tokputs(L, &tok, t);
                dollar = alldigit = 0;
                goto next;
            }
            shellungetc(L, peek);
        }

        if (shellexp(ch)) {
            i32 peek = shellgetc(L, 1);
            if (peek=='(' || ((peek=='{' || peek=='[') && ch=='$')) {
                Str t = {};
                i32 cd = currentdelim(L);
                if (peek == '{') {
                    i32 n = shellgetc(L, 1);
                    shellungetc(L, n);
                    if (funsubchar(n)) {
                        pushdelim(L, peek);
                        Matched m = parsecomsub(L, cd, '{', '}', P_COMMAND);
                        popdelim(L);
                        t = m.ok ? m.text : Str{};
                    } else {
                        t = parsematched(L, cd, '{', '}', P_FIRSTCLOSE|P_DOLBRACE);
                    }
                } else if (peek == '(') {
                    pushdelim(L, peek);
                    Matched m = parsecomsub(L, cd, '(', ')', P_COMMAND);
                    popdelim(L);
                    t = m.ok ? m.text : Str{};
                } else {
                    t = parsematched(L, cd, '[', ']', P_ARITH);
                }
                if (!t.data) {
                    return T_ERROR;
                }
                tokput(L, &tok, (u8)ch);
                tokput(L, &tok, (u8)peek);
                tokputs(L, &tok, t);
                dollar   = 1;
                alldigit = 0;
                goto next;
            } else if (ch=='$' && (peek=='\'' || peek=='"')) {
                pushdelim(L, peek);
                Str t = parsematched(L, peek, peek, peek, peek=='\'' ? P_ALLOWESC : 0);
                popdelim(L);
                if (!t.data) {
                    return T_ERROR;
                }
                if (peek == '\'') {
                    t = singlequote(L->a, ansiexpand(L->a, cuttail(t, 1), L->utf8));
                } else {
                    Tokbuf q = {};
                    tokput(L, &q, '"');
                    tokputs(L, &q, cuttail(t, 1));
                    tokput(L, &q, '"');
                    t = tokstr(&q);
                }
                tokputs(L, &tok, t);
                quoted   = 1;
                alldigit = 0;
                goto next;
            } else if (ch=='$' && peek=='$') {
                tokput(L, &tok, '$');
                tokput(L, &tok, '$');
                dollar   = 1;
                alldigit = 0;
                goto next;
            }
            shellungetc(L, peek);
        }

        if (ch=='[' && ((tok.len && assignok(L, L->last) && validident(tokstr(&tok))) ||
                        (!tok.len && (L->pst & PST_COMPASSIGN)))) {
            Str t = parsematched(L, 0, '[', ']', P_ARRAYSUB);
            if (!t.data) {
                return T_ERROR;
            }
            tokput(L, &tok, '[');
            tokputs(L, &tok, t);
            alldigit = 0;
            goto next;
        } else if (ch=='=' && tok.len &&
                   (assignok(L, L->last) || (L->pst & PST_ASSIGNOK))) {
            // token_is_assignment: the token so far plus '=' assigns
            Str s = {};
            s.data = allocbytes(L->a, tok.len+1);
            s.len  = tok.len + 1;
            copybytes(s.data, tok.data, tok.len);
            s.data[tok.len] = '=';
            iz e = assignment(s, (L->pst & PST_COMPASSIGN)!=0);
            if (e>0 && e==tok.len) {
                i32 peek = shellgetc(L, 1);
                if (peek == '(') {
                    Str body = parsecompoundassign(L);
                    tokput(L, &tok, '=');
                    tokput(L, &tok, '(');
                    tokputs(L, &tok, body);
                    tokput(L, &tok, ')');
                    alldigit = 0;
                    compound = 1;
                    goto next;
                }
                shellungetc(L, peek);
            }
        }

        if (shellbreak(ch)) {
            shellungetc(L, ch);
            goto gottoken;
        }

    character:
    escaped:
        tokput(L, &tok, (u8)ch);
        alldigit &= digit((u8)ch);
        dollar   |= ch=='$';

    next:
        ch = shellgetc(L, currentdelim(L)!='\'' && !passnext);
    }

gottoken:
    Str text = tokstr(&tok);
    for (iz i = 0; i < text.len; i++) {
        if (!text[i]) text = takehead(text, i);  // C string semantics
    }

    if (alldigit && text.len && (ch=='<' || ch=='>' || L->last==T_LESSAND ||
                                 L->last==T_GREATAND)) {
        // Leading zeros are fine; the value must fit an int.
        i64 v  = 0;
        b32 ok = 1;
        for (iz i = 0; ok && i < text.len; i++) {
            v  = v*10 + (text[i] - '0');
            ok = v <= 0x7fffffff;
        }
        if (ok) {
            L->val.number = v;
            return T_NUMBER;
        }
    }

    i32 special = specialcase(L, text);
    if (special >= 0) {
        pushnest(L, special);
        return special;
    }

    if (L->posix) {
        i32 r = checkreserved(L, text, dollar, quoted);
        if (r) return r;
    }

    if (L->expandaliases && !quoted && L->aliases &&
            ((L->pst & PST_ALEXPNEXT) || assignok(L, L->last))) {
        Alias *al = findalias(*L->aliases, text);
        if (al && !al->expanding) {
            pushstring(L, al->value, al, 0);
            return T_ERROR - 1;  // re-read
        }
        L->pst &= ~PST_ALEXPNEXT;
    }

    if (!L->posix) {
        i32 r = checkreserved(L, text, dollar, quoted);
        if (r) return r;
    }

    Word *w = newword(L->a, text, 0);
    if (dollar)  w->flags |= W_HASDOLLAR;
    if (quoted)  w->flags |= W_QUOTED;
    if (compound && text.len && text[text.len-1]==')') {
        w->flags |= W_COMPASSIGN;
    }
    if (assignment(text, (L->pst & PST_COMPASSIGN)!=0)) {
        w->flags |= W_ASSIGNMENT;
        if (assignok(L, L->last) || (L->pst & PST_COMPASSIGN)) {
            w->flags |= W_NOSPLIT;
            if (L->pst & PST_COMPASSIGN) {
                w->flags |= W_NOGLOB|W_NOBRACE;
            }
        }
    }

    if (cmdposition(L, L->last) || (L->pst & PST_CMDBLTIN)) {
        static Str declbuiltins[] = {
            "alias", "declare", "export", "local", "readonly", "typeset",
            "eval", "let",
        };
        b32 decl = 0;
        for (iz i = 0; i < countof(declbuiltins); i++) {
            decl |= text == declbuiltins[i];
        }
        if (decl) {
            L->pst |= PST_ASSIGNOK;
        } else if (L->posix && text=="command") {
            L->pst |= PST_CMDBLTIN;
        } else {
            L->pst &= ~PST_CMDBLTIN;
        }
    } else {
        L->pst &= ~PST_CMDBLTIN;
    }

    L->val.word = w;

    if (text.len>2 && text[0]=='{' && text[text.len-1]=='}' && (ch=='<' || ch=='>')) {
        Str name = cuttail(cuthead(text, 1), 1);
        if (validident(name) || validarrayref(name)) {
            w->text = name;
            return T_REDIRWORD;
        }
    }

    i32 result = (w->flags & (W_ASSIGNMENT|W_NOSPLIT)) == (W_ASSIGNMENT|W_NOSPLIT)
               ? T_ASSIGN : T_WORD;

    switch (L->last) {
    case T_FUNCTION:
        L->pst |= PST_ALLOWOPNBRC;
        break;
    case T_CASE:
    case T_FOR:
        L->expectcmd = L->last;
        L->expectin++;
        break;
    case T_SELECT:
        L->expectin++;
        break;
    }
    return result;
}

// The parenthesized list of a compound array assignment, as text.
static Str parsecompoundassign(Lexer *L)
{
    i32   savelast   = L->last;
    i32   savebefore = L->before;
    i32   savepst    = L->pst;
    Token saveval    = L->val;
    i32   saveesacs  = L->esacs;
    i32   saveexpin  = L->expectin;
    i32   saveexpcmd = L->expectcmd;
    i32   savendoc   = L->nheredoc;
    Redir *savedocs[HEREDOCMAX];
    for (i32 i = 0; i < L->nheredoc; i++) savedocs[i] = L->heredocs[i];
    Tokbuf r         = {};
    L->esacs = L->expectin = L->expectcmd = 0;
    b32   first      = 1;

    i32 start = L->lineno;
    L->pst &= ~(PST_NOEXPAND|PST_CONDCMD|PST_CONDEXPR|PST_REGEXP|PST_EXTPAT);
    L->pst |= PST_COMPASSIGN;
    L->last = T_WORD;
    for (;;) {
        i32 t = readtoken(L);
        if (t == ')') {
            break;
        }
        if (t == '\n') {
            continue;
        }
        if (t!=T_WORD && t!=T_ASSIGN) {
            L->current = t;
            if (t == T_EOF) {
                message(L, start, "unexpected EOF while looking for matching `)'");
            } else {
                synerror(L);
            }
            L->discard = 1;
            parsefail(L);
        }
        if (!first) tokput(L, &r, ' ');
        tokputs(L, &r, L->val.word->text);
        first = 0;
    }
    L->last      = savelast;
    L->before    = savebefore;
    L->pst       = savepst;
    L->val       = saveval;
    L->esacs     = saveesacs;
    L->expectin  = saveexpin;
    L->expectcmd = saveexpcmd;
    // Like bash, restore the pending list even if a newline inside the
    // assignment read those bodies: they will be read again.
    L->nheredoc = savendoc;
    for (i32 i = 0; i < savendoc; i++) L->heredocs[i] = savedocs[i];
    return tokstr(&r);
}


// Arithmetic commands and [[ ]]

// Saw "((": find the matching "))". Returns 1 for an arithmetic
// command, 0 for a nested subshell (text pushed back), -1 on error.
static i32 parsearithcmd(Lexer *L, Str *text)
{
    Str t = parsematched(L, 0, '(', ')', P_ARITH);
    if (!t.data) {
        return -1;
    }
    // bash copies the text as a C string, so a NUL from $'\0' ends it
    Str body = cuttail(t, 1);
    for (iz i = 0; i < body.len; i++) {
        if (!body[i]) body = takehead(body, i);
    }
    i32 c = shellgetc(L, 0);
    if (c == ')') {
        *text = body;
        return 1;
    }
    shellungetc(L, c);
    Tokbuf b = {};
    tokput(L, &b, '(');
    tokputs(L, &b, body);
    tokput(L, &b, ')');
    *text = tokstr(&b);
    return 0;
}

static i32 parsedparen(Lexer *L, i32 c)
{
    Str text = {};
    if (L->last == T_FOR) {
        if (parsearithcmd(L, &text) != 1) {
            return T_ERROR;
        }
        L->val.word = newword(L->a, text, 0);
        return T_ARITHFOR;
    }
    if (reservedok(L, L->last)) {
        i32 r = parsearithcmd(L, &text);
        if (r < 0) {
            return T_ERROR;
        }
        if (r == 1) {
            L->val.word = newword(L->a, text,
                W_QUOTED|W_NOSPLIT|W_NOGLOB|W_NOTILDE|W_NOPROCSUB);
            return T_ARITHCMD;
        }
        pushstring(L, internalform(L->a, text), 0, 1);
        if (!(L->pst & (PST_CASEPAT|PST_CONDCMD))) {
            pushnest(L, c);
            L->pst |= PST_SUBSHELL;
        }
        return c;  // the pushed text begins with the second paren
    }
    return -2;
}

static Cond *newcond(Arena *a, i32 kind, Word *op, Cond *left, Cond *right)
{
    Cond *c = alloc<Cond>(a);
    c->kind  = kind;
    c->op    = op;
    c->left  = left;
    c->right = right;
    return c;
}

static b32 testunop(Str s)
{
    if (s.len!=2 || s[0]!='-') return 0;
    return find("abcdefghknoprstuvwxzGLNORS", takehead(cuthead(s, 1), 1)) >= 0;
}

static b32 testbinop(Str s)
{
    static Str ops[] = {
        "=", "==", "!=", "<", ">", "-nt", "-ot", "-ef", "-eq", "-ne",
        "-lt", "-le", "-gt", "-ge",
    };
    for (iz i = 0; i < countof(ops); i++) {
        if (s == ops[i]) return 1;
    }
    return 0;
}

struct CondParser {
    Lexer *L;
    i32    token;
    i32    lineno;
};

// Inside [[ ]] bash reads tokens without updating the token history, so
// the last token stays [[: no reserved words, subscripts, or compound
// assignments are recognized.
static i32 condread(CondParser *p)
{
    return p->token = readtoken(p->L);
}

static i32 condskip(CondParser *p)
{
    while (condread(p) == '\n') {}
    return p->token;
}

// A lexical error: read_token returns -1, which the parser reports
// generically unless the specific message suppressed that.
[[noreturn]] static void lexfail(Lexer *L)
{
    L->current = T_ERROR;
    synerror(L);
    parsefail(L);
}

// Report an error in a conditional expression and fail the term. The
// error token unwinds through the enclosing terms, each open paren
// adding its own complaint, as in bash's COND_RETURN_ERROR.
static Cond *conderr(CondParser *p, i32 line, Str prefix, Str tokentext, Str suffix)
{
    Lexer *L = p->L;
    Buf b(L->a, 64);
    print(&b, prefix);
    print(&b, tokentext);
    print(&b, suffix);
    message(L, line, finish(&b));
    p->token = T_ERROR;
    return 0;
}

// Spelling of the current token for conditional-command errors.
static Str condtokentext(CondParser *p)
{
    Lexer *L = p->L;
    i32    t = p->token;
    Str    s = tokenname(t);
    if (s.data) return s;
    if (t==T_WORD || t==T_ASSIGN) return internalform(L->a, L->val.word->text);
    if (t == T_NUMBER) {
        Buf b(L->a, 16);
        print(&b, L->val.number);
        return finish(&b);
    }
    return {};
}

static Cond *condor(CondParser *);

// bash's yacc number for tokens that have no spelling.
static i32 yacctoken(i32 t)
{
    switch (t) {
    case T_WORD:      return 281;
    case T_ASSIGN:    return 282;
    case T_REDIRWORD: return 283;
    case T_NUMBER:    return 284;
    case T_ARITHCMD:  return 285;
    case T_ARITHFOR:  return 286;
    case T_CONDCMD:   return 287;
    case T_DOLPAREN:  return 304;
    case T_DOLBRACE:  return 305;
    }
    return t;
}

static Cond *condterm(CondParser *p)
{
    checkstack(p->L);
    Lexer *L = p->L;
    i32 tok  = condskip(p);
    i32 line = L->lineno;
    Cond *term = 0;

    if (tok == T_CONDEND) {
        p->token = T_ERROR;
        return 0;
    } else if (tok == '(') {
        term = condor(p);
        if (p->token != ')') {
            Str t = condtokentext(p);
            return t.data ? conderr(p, line, "unexpected token `", t, "', expected `)'")
                          : conderr(p, line, "expected `)'", "", "");
        }
        term = newcond(L->a, C_EXPR, 0, term, 0);
        condskip(p);
    } else if (tok==T_BANG || (tok==T_WORD && L->val.word->text=="!")) {
        term = condterm(p);
        if (term) term->invert = !term->invert;
    } else if (tok==T_WORD && testunop(L->val.word->text)) {
        Word *op = L->val.word;
        tok = condread(p);
        if (tok != T_WORD) {
            Str t = condtokentext(p);
            return t.data ? conderr(p, L->lineno, "unexpected argument `", t, "' to conditional unary operator")
                          : conderr(p, L->lineno, "unexpected argument to conditional unary operator", "", "");
        }
        Cond *left = newcond(L->a, C_TERM, L->val.word, 0, 0);
        term = newcond(L->a, C_UNARY, op, left, 0);
        condskip(p);
    } else if (tok == T_WORD) {
        Cond *left = newcond(L->a, C_TERM, L->val.word, 0, 0);
        Word *op   = 0;
        tok = condread(p);
        if (tok==T_WORD && testbinop(L->val.word->text)) {
            op = L->val.word;
            Str s = op->text;
            if (s=="=" || s=="==" || s=="!=") {
                L->pst |= PST_EXTPAT;
            }
        } else if (tok==T_WORD && L->val.word->text=="=~") {
            op = L->val.word;
            L->pst |= PST_REGEXP;
        } else if (tok=='<' || tok=='>') {
            op = newword(L->a, tok=='<' ? Str("<") : Str(">"), 0);
        } else if (tok==T_CONDEND || tok==T_ANDAND || tok==T_OROR || tok==')') {
            // [[ x ]] is [[ -n x ]]
            op = newword(L->a, "-n", 0);
            return newcond(L->a, C_UNARY, op, left, 0);
        } else {
            Str t = condtokentext(p);
            return t.data ? conderr(p, L->lineno, "unexpected token `", t, "', conditional binary operator expected")
                          : conderr(p, L->lineno, "conditional binary operator expected", "", "");
        }

        b32 saveglob = L->extglob;
        if (L->pst & PST_EXTPAT) L->extglob = 1;
        tok = condread(p);
        if (L->pst & PST_EXTPAT) L->extglob = saveglob;
        L->pst &= ~(PST_REGEXP|PST_EXTPAT);

        if (tok != T_WORD) {
            Str t = condtokentext(p);
            return t.data ? conderr(p, L->lineno, "unexpected argument `", t, "' to conditional binary operator")
                          : conderr(p, L->lineno, "unexpected argument to conditional binary operator", "", "");
        }
        Cond *right = newcond(L->a, C_TERM, L->val.word, 0, 0);
        term = newcond(L->a, C_BINARY, op, left, right);
        condskip(p);
    } else {
        Str t = condtokentext(p);
        if (tok < 256) {
            u8 c = (u8)tok;  // -1 prints as 0xff, as with bash's %c
            return conderr(p, L->lineno, "unexpected token `", clone(L->a, Str{&c, 1}), "' in conditional command");
        }
        if (t.data) {
            return conderr(p, L->lineno, "unexpected token `", t, "' in conditional command");
        }
        Buf b(L->a, 32);
        print(&b, (i64)yacctoken(tok));
        return conderr(p, L->lineno, "unexpected token ", finish(&b), " in conditional command");
    }
    return term;
}

static Cond *condand(CondParser *p)
{
    Cond *l = condterm(p);
    if (p->token == T_ANDAND) {
        Cond *r = condand(p);
        l = newcond(p->L->a, C_AND, 0, l, r);
    }
    return l;
}

static Cond *condor(CondParser *p)
{
    Cond *l = condand(p);
    if (p->token == T_OROR) {
        Cond *r = condor(p);
        l = newcond(p->L->a, C_OR, 0, l, r);
    }
    return l;
}

static i32 condcommand(Lexer *L)
{
    CondParser p = {};
    p.L      = L;
    p.lineno = L->lineno;
    L->pst  &= ~PST_CMDBLTIN;
    L->pst  |= PST_CONDEXPR;
    Cond *c  = condor(&p);
    if (p.token != T_CONDEND) {
        if (L->eofreached && p.token!=T_ERROR) {
            message(L, p.lineno, "unexpected EOF while looking for `]]'");
        } else if (p.token != T_ERROR) {
            Str t = condtokentext(&p);
            if (t.data) {
                conderr(&p, p.lineno, "syntax error in conditional expression: unexpected token `", t, "'");
            } else {
                message(L, p.lineno, "syntax error in conditional expression");
            }
        }
        lexfail(L);
    }
    L->val.cond   = c;
    L->toread     = T_CONDEND;
    L->pst       &= ~(PST_CONDEXPR|PST_CONDCMD);
    return T_CONDCMD;
}


// Tokens (bash's read_token)

static i32 readtoken(Lexer *L)
{
    if (L->toread) {
        i32 t = L->toread;
        if (t==T_WORD || t==T_ASSIGN) {
            L->val = L->toreadval;
        }
        L->toread = 0;
        return t;
    }

    if ((L->pst & (PST_CONDCMD|PST_CONDEXPR)) == PST_CONDCMD) {
        return condcommand(L);
    }

    for (;;) {
        i32 c;
        while ((c = shellgetc(L, 1))>=0 && shellblank(c)) {}
        if (c < 0) {
            L->eofreached = 1;
            return T_EOF;
        }

        if (c == '#') {
            L->pst |= PST_COMMENT;
            while ((c = shellgetc(L, 0))>=0 && c!='\n') {}
            if (c < 0) {
                L->pst &= ~PST_COMMENT;
                L->eofreached = 1;
                return T_EOF;
            }
            L->pst &= ~PST_COMMENT;
            c = '\n';
        }

        if (c == '\n') {
            if (L->nheredoc) {
                gatherheredocs(L);
            }
            L->pst &= ~(PST_ALEXPNEXT|PST_ASSIGNOK|PST_CMDBLTIN);
            return '\n';
        }

        if (L->pst & PST_REGEXP) {
            i32 r = readtokenword(L, c);
            if (r == T_ERROR-1) continue;
            return r;
        }

        if (shellmeta(c)) {
            L->pst &= ~(PST_ALEXPNEXT|PST_ASSIGNOK|PST_CMDBLTIN);
            i32 peek;
            if ((L->pst & PST_CMDSUBST) && c==L->eoftoken) {
                peek = shellgetc(L, 0);
            } else {
                peek = shellgetc(L, 1);
            }

            if (c == peek) {
                switch (c) {
                case '<':
                    peek = shellgetc(L, 1);
                    if (peek == '-') return T_DLESSDASH;
                    if (peek == '<') return T_TLESS;
                    shellungetc(L, peek);
                    return T_DLESS;
                case '>':
                    return T_DGREAT;
                case ';':
                    L->pst |= PST_CASEPAT;
                    peek = shellgetc(L, 1);
                    if (peek == '&') return T_DSEMIAND;
                    shellungetc(L, peek);
                    return T_DSEMI;
                case '&':
                    return T_ANDAND;
                case '|':
                    return T_OROR;
                case '(': {
                    i32 r = parsedparen(L, c);
                    if (r != -2) return r;
                } break;
                }
            } else if (c=='<' && peek=='&') {
                return T_LESSAND;
            } else if (c=='>' && peek=='&') {
                return T_GREATAND;
            } else if (c=='<' && peek=='>') {
                return T_LESSGREAT;
            } else if (c=='>' && peek=='|') {
                return T_CLOBBER;
            } else if (c=='&' && peek=='>') {
                peek = shellgetc(L, 1);
                if (peek == '>') return T_ANDDGREAT;
                shellungetc(L, peek);
                return T_ANDGREAT;
            } else if (c=='|' && peek=='&') {
                return T_BARAND;
            } else if (c==';' && peek=='&') {
                L->pst |= PST_CASEPAT;
                return T_SEMIAND;
            }

            shellungetc(L, peek);

            if (c==')' && L->last=='(' && L->before==T_WORD) {
                L->pst |= PST_ALLOWOPNBRC;
            }
            if (c=='(' && !(L->pst & (PST_CASEPAT|PST_CONDCMD)) && L->last!=T_WORD) {
                pushnest(L, c);
                L->pst |= PST_SUBSHELL;
            } else if ((L->pst & PST_CASEPAT) && c==')') {
                L->pst &= ~PST_CASEPAT;
            } else if ((L->pst & PST_SUBSHELL) && c==')') {
                L->pst &= ~PST_SUBSHELL;
            }

            if ((c!='>' && c!='<') || peek!='(') {
                return c;
            }
            // Process substitution: read it as a word.
        }

        if (c=='-' && (L->last==T_LESSAND || L->last==T_GREATAND)) {
            L->pst &= ~PST_CMDBLTIN;
            return c;
        }

        i32 r = readtokenword(L, c);
        if (r == T_ERROR-1) {
            continue;  // alias expanded: re-read
        }
        return r;
    }
}


// Parser

// Read the next token, updating the history used for tokenization.
static i32 lex(Lexer *L)  // yylex
{
    L->twoago  = L->before;
    L->before  = L->last;
    L->last    = L->current;
    L->current = readtoken(L);
    return L->current;
}

struct Parser {
    Lexer *L;
    i32    tok;    // lookahead token, valid when have is set
    b32    have;
};

static i32 peek(Parser *p)
{
    if (!p->have) {
        p->tok  = lex(p->L);
        p->have = 1;
    }
    return p->tok;
}

static void advance(Parser *p)
{
    peek(p);
    p->have = 0;
}

// Report a syntax error at the lookahead token and unwind.
[[noreturn]] static void syntaxerror(Parser *p)
{
    synerror(p->L);
    parsefail(p->L);
}

static void synerror(Lexer *L)
{
    if ((L->pst & PST_CMDSUBST) && !L->eofreached) {
        L->comsuberr = 1;
    }
    if (L->pst & PST_NOERROR) {
        return;
    }
    L->reported = 1;
    i32 t = L->current;
    Str msg = {};
    if (t && !L->eofreached) {
        msg = tokenname(t);
        if (!msg.data) {
            switch (t) {
            case T_WORD: case T_ASSIGN:
                msg = internalform(L->a, L->val.word->text);
                break;
            case T_NUMBER: {
                Buf b(L->a, 16);
                print(&b, L->val.number);
                msg = finish(&b);
            } break;
            case T_ARITHCMD:
                msg = L->val.word->text;
                break;
            case T_ARITHFOR:
                msg = L->val.word->text;  // TODO: " ; " joined
                break;
            }
        }
    }
    if (msg.data) {
        if (ansicshouldquote(msg)) {
            msg = ansicquote(L->a, msg);
        }
        Buf b(L->a, 64);
        print(&b, "syntax error near unexpected token `");
        print(&b, msg);
        if (L->eoftoken && t!=L->eoftoken) {
            print(&b, "' while looking for matching `");
            putbyte(&b, (u8)L->eoftoken);
        }
        print(&b, "'");
        message(L, L->lineno, finish(&b));
        Buf l(L->a, 64);
        print(&l, "`");
        print(&l, currentline(L));
        print(&l, "'");
        message(L, L->lineno, finish(&l));
    } else if (L->line.len) {
        Str near = errortokenfromtext(L);
        if (near.data) {
            Buf b(L->a, 64);
            print(&b, "syntax error near `");
            print(&b, near);
            print(&b, "'");
            message(L, L->lineno, finish(&b));
        }
        Buf l(L->a, 64);
        print(&l, "`");
        print(&l, currentline(L));
        print(&l, "'");
        message(L, L->lineno, finish(&l));
    } else if (L->eofreached && L->eoftoken && t!=L->eoftoken) {
        Buf b(L->a, 64);
        print(&b, "unexpected EOF while looking for matching `");
        putbyte(&b, (u8)L->eoftoken);
        print(&b, "'");
        message(L, L->lineno, finish(&b));
    } else if (L->eofreached && L->ntop >= 0) {
        Buf b(L->a, 64);
        Str name = tokenname(L->nest[L->ntop].token);
        print(&b, "syntax error: unexpected end of file from ");
        if (name.data) {
            print(&b, "`");
            print(&b, name);
            print(&b, "' ");
        }
        print(&b, "command on line ");
        print(&b, (i64)L->nest[L->ntop].lineno);
        message(L, L->lineno, finish(&b));
    } else {
        message(L, L->lineno, L->eofreached ? Str("syntax error: unexpected end of file")
                                            : Str("syntax error"));
    }
}

static void expect(Parser *p, i32 t)
{
    if (peek(p) != t) {
        syntaxerror(p);
    }
    advance(p);
}

static void newlines(Parser *p)
{
    while (peek(p) == '\n') {
        advance(p);
    }
}

static Node *newnode(Lexer *L, i32 kind)
{
    Node *n = alloc<Node>(L->a);
    n->kind = kind;
    n->line = L->lineno;
    return n;
}

static Node *connect(Lexer *L, Node *a, Node *b, i32 connector)
{
    Node *n = newnode(L, N_CONNECT);
    n->a = a;
    n->b = b;
    n->connector = connector;
    return n;
}

// "a && b &": make the & apply to the whole and-or list (bash's
// connect_async_list).
static Node *connectasync(Lexer *L, Node *a, Node *b)
{
    if (a->kind!=N_CONNECT || !a->b || a->connector!=';') {
        return connect(L, a, b, '&');
    }
    // a ; b ; c &  ->  a ; b ; (c &)
    Node *t1 = a;
    Node *t  = a->b;
    while (t->kind==N_CONNECT && t->connector==';') {
        t1 = t;
        t  = t->b;
    }
    t1->b = connect(L, t, b, '&');
    return a;
}

static b32 isredirstart(i32 t)
{
    switch (t) {
    case '>': case '<': case T_NUMBER: case T_REDIRWORD: case T_DGREAT:
    case T_CLOBBER: case T_LESSGREAT: case T_DLESS: case T_DLESSDASH:
    case T_TLESS: case T_LESSAND: case T_GREATAND: case T_ANDGREAT:
    case T_ANDDGREAT:
        return 1;
    }
    return 0;
}

static Word *expectword(Parser *p)
{
    if (peek(p) != T_WORD) {
        syntaxerror(p);
    }
    Word *w = p->L->val.word;
    advance(p);
    return w;
}

static Redir *parseredir(Parser *p)
{
    Lexer *L = p->L;
    Redir *r = alloc<Redir>(L->a);
    i32    t = peek(p);
    r->src = -1;

    if (t == T_NUMBER) {
        r->src = (i32)L->val.number;
        advance(p);
        t = peek(p);
    } else if (t == T_REDIRWORD) {
        r->srcvar = L->val.word;
        advance(p);
        t = peek(p);
    }

    switch (t) {
    case '>':        r->op = R_OUTPUT;     if (r->src<0) r->src = 1; break;
    case '<':        r->op = R_INPUT;      if (r->src<0) r->src = 0; break;
    case T_DGREAT:   r->op = R_APPEND;     if (r->src<0) r->src = 1; break;
    case T_CLOBBER:  r->op = R_CLOBBER;    if (r->src<0) r->src = 1; break;
    case T_LESSGREAT:r->op = R_INOUT;      if (r->src<0) r->src = 0; break;
    case T_DLESS:    r->op = R_HEREDOC;    if (r->src<0) r->src = 0; break;
    case T_DLESSDASH:r->op = R_HEREDOCSTRIP; if (r->src<0) r->src = 0; break;
    case T_TLESS:    r->op = R_HERESTRING; if (r->src<0) r->src = 0; break;
    case T_LESSAND:  r->op = R_DUPIN;      if (r->src<0) r->src = 0; break;
    case T_GREATAND: r->op = R_DUPOUT;     if (r->src<0) r->src = 1; break;
    case T_ANDGREAT:
    case T_ANDDGREAT:
        if (r->src>=0 || r->srcvar) syntaxerror(p);
        r->op  = t==T_ANDGREAT ? R_ERRANDOUT : R_APPENDERRANDOUT;
        r->src = 1;
        break;
    default:
        syntaxerror(p);
    }
    advance(p);

    if (r->op==R_DUPIN || r->op==R_DUPOUT) {
        i32 n = peek(p);
        if (n == T_NUMBER) {
            r->dst = (i32)L->val.number;
            advance(p);
        } else if (n == '-') {
            r->op = R_CLOSE;
            advance(p);
        } else if (n == T_WORD) {
            b32 in  = r->op == R_DUPIN;
            r->op   = in ? R_DUPINWORD : R_DUPOUTWORD;
            r->word = L->val.word;
            advance(p);
            Str w = r->word->text;
            if (w.len && w[w.len-1]=='-') {
                // N>&M- moves a descriptor
                Str num = cuttail(w, 1);
                b32 digits = num.len>0 && num.len<=10;
                i64 v = 0;
                for (iz i = 0; digits && i < num.len; i++) {
                    digits = digit(num[i]);
                    v = v*10 + (num[i] - '0');
                }
                if (digits && v <= 0x7fffffff) {
                    r->op   = in ? R_MOVEIN : R_MOVEOUT;
                    r->dst  = (i32)v;
                    r->word = 0;
                } else {
                    r->op   = in ? R_MOVEINWORD : R_MOVEOUTWORD;
                    r->word = newword(L->a, num, r->word->flags);
                }
            }
        } else {
            syntaxerror(p);
        }
        return r;
    }

    r->word = expectword(p);
    if (r->op==R_HEREDOC || r->op==R_HEREDOCSTRIP) {
        pushheredoc(L, r);
    }
    return r;
}

// The rest of a simple command. When started, its first word was
// already read by the caller.
static Node *parsesimple(Parser *p, b32 started)
{
    Lexer *L = p->L;
    Node  *n = 0;
    Word **wtail = 0;
    Redir **rtail = 0;
    for (;;) {
        i32 t = peek(p);
        if (t==T_WORD || t==T_ASSIGN) {
            if (!n) {
                n = newnode(L, N_SIMPLE);
                wtail = &n->words;
                rtail = &n->redirs;
            }
            *wtail = L->val.word;
            wtail = &(*wtail)->next;
            advance(p);
            L->pst &= ~PST_REDIRLIST;
        } else if (isredirstart(t)) {
            b32 first = !n && !started;
            if (!n) {
                n = newnode(L, N_SIMPLE);
                wtail = &n->words;
                rtail = &n->redirs;
            }
            Redir *r = parseredir(p);
            *rtail = r;
            rtail = &r->next;
            if (first) {
                L->pst |= PST_REDIRLIST;
            }
        } else {
            break;
        }
    }
    return n;
}

static Redir *parseredirlist(Parser *p)
{
    Redir  *head = 0;
    Redir **tail = &head;
    while (isredirstart(peek(p))) {
        *tail = parseredir(p);
        tail = &(*tail)->next;
    }
    return head;
}

static void appendredirs(Node *n, Redir *r)
{
    Redir **tail = &n->redirs;
    while (*tail) tail = &(*tail)->next;
    *tail = r;
}

static Node *parseshell(Parser *p);
static Node *parsepipeline(Parser *p);
static Node *parsecompound(Parser *p);

// Can this token begin a pipeline_command?
static b32 startscommand(i32 t)
{
    switch (t) {
    case T_WORD: case T_ASSIGN: case T_IF: case T_CASE: case T_FOR:
    case T_SELECT: case T_WHILE: case T_UNTIL: case '{': case '(':
    case T_FUNCTION: case T_TIME: case T_BANG: case T_CONDSTART:
    case T_COPROC: case T_ARITHCMD:
        return 1;
    }
    return isredirstart(t);
}

static b32 startsshell(i32 t)
{
    switch (t) {
    case T_FOR: case T_CASE: case T_WHILE: case T_UNTIL: case T_SELECT:
    case T_IF: case '(': case '{': case T_ARITHCMD: case T_CONDSTART:
        return 1;
    }
    return 0;
}

static Node *parsecommand(Parser *p)
{
    checkstack(p->L);
    Lexer *L = p->L;
    i32    t = peek(p);

    if (startsshell(t)) {
        Node  *n = parseshell(p);
        Redir *r = parseredirlist(p);
        if (r) appendredirs(n, r);
        return n;
    }

    if (t == T_FUNCTION) {
        advance(p);
        Node *f = newnode(L, N_FUNCDEF);
        f->name = expectword(p);
        if (peek(p) == '(') {
            advance(p);
            if (peek(p) != ')') {
                // function NAME ( list ): the paren began a subshell
                Node *sub = newnode(L, N_SUBSHELL);
                sub->a = parsecompound(p);
                expect(p, ')');
                popnest(L);
                f->a = sub;
                Redir *r = parseredirlist(p);
                if (r) appendredirs(f->a, r);
                return f;
            }
            advance(p);
            newlines(p);
        } else if (peek(p) == '\n') {
            newlines(p);
        }
        if (!startsshell(peek(p))) {
            syntaxerror(p);
        }
        f->a = parseshell(p);
        Redir *r = parseredirlist(p);
        if (r) appendredirs(f->a, r);
        return f;
    }

    if (t == T_COPROC) {
        advance(p);
        Node *c = newnode(L, N_COPROC);
        c->name = newword(L->a, "COPROC", 0);
        t = peek(p);
        if (startsshell(t)) {
            c->a = parseshell(p);
            Redir *r = parseredirlist(p);
            if (r) appendredirs(c->a, r);
            return c;
        }
        if (t == T_WORD) {
            Word *w    = L->val.word;
            i32   line = L->lineno;  // a command's line is its first word's
            advance(p);
            if (startsshell(peek(p))) {
                c->name = w;
                c->a = parseshell(p);
                Redir *r = parseredirlist(p);
                if (r) appendredirs(c->a, r);
                return c;
            }
            Node *s = parsesimple(p, 1);
            if (!s) {
                s = newnode(L, N_SIMPLE);
            }
            s->line  = line;
            w->next  = s->words;
            s->words = w;
            c->a = s;
            L->pst &= ~PST_REDIRLIST;
            return c;
        }
        Node *s = parsesimple(p, 0);
        if (!s) {
            syntaxerror(p);
        }
        c->a = s;
        L->pst &= ~PST_REDIRLIST;
        return c;
    }

    if (t == T_WORD) {
        Word *w    = L->val.word;
        i32   line = L->lineno;  // a command's line is its first word's
        advance(p);
        if (peek(p) == '(') {
            advance(p);
            expect(p, ')');
            newlines(p);
            if (!startsshell(peek(p))) {
                syntaxerror(p);
            }
            Node *f = newnode(L, N_FUNCDEF);
            f->name = w;
            f->a = parseshell(p);
            Redir *r = parseredirlist(p);
            if (r) appendredirs(f->a, r);
            return f;
        }
        L->pst &= ~PST_REDIRLIST;
        Node *s = parsesimple(p, 1);
        if (!s) {
            s = newnode(L, N_SIMPLE);
        }
        s->line  = line;
        w->next  = s->words;
        s->words = w;
        L->pst &= ~PST_REDIRLIST;
        return s;
    }

    Node *s = parsesimple(p, 0);
    if (!s) {
        syntaxerror(p);
    }
    L->pst &= ~PST_REDIRLIST;
    return s;
}

static Node *parsepipeline(Parser *p)  // pipeline
{
    Lexer *L    = p->L;
    Node  *left = parsecommand(p);
    i32    t    = peek(p);
    if (t!='|' && t!=T_BARAND) {
        return left;
    }
    advance(p);
    newlines(p);
    if (t == T_BARAND) {
        // cmd1 |& cmd2 is cmd1 2>&1 | cmd2
        Redir *r = alloc<Redir>(L->a);
        r->op  = R_DUPOUT;
        r->src = 2;
        r->dst = 1;
        appendredirs(left, r);
    }
    Node *right = parsepipeline(p);
    return connect(L, left, right, '|');
}

static b32 nullterminator(i32 t)  // nullcmd_terminator
{
    return t=='\n' || t==';' || t==T_EOF || t=='&';
}

static Node *parsepipecommand(Parser *p)  // pipeline_command
{
    Lexer *L = p->L;
    i32    t = peek(p);
    if (t == T_BANG) {
        advance(p);
        if (nullterminator(peek(p))) {
            Node *n = newnode(L, N_SIMPLE);
            n->flags |= CMD_INVERT;
            return n;
        }
        Node *n = parsepipecommand(p);
        n->flags ^= CMD_INVERT;
        return n;
    }
    if (t == T_TIME) {
        advance(p);
        i32 flags = CMD_TIME;
        if (peek(p) == T_TIMEOPT) {
            advance(p);
            flags |= CMD_TIMEPOSIX;
        }
        if (peek(p) == T_TIMEIGN) {
            advance(p);
            flags |= CMD_TIMEPOSIX;  // bash's grammar: time -- is time -p
        }
        if (nullterminator(peek(p))) {
            Node *n = newnode(L, N_SIMPLE);
            n->flags |= flags;
            return n;
        }
        Node *n = parsepipecommand(p);
        n->flags |= flags;
        return n;
    }
    return parsepipeline(p);
}

static Node *parseandor(Parser *p)
{
    Lexer *L    = p->L;
    Node  *left = parsepipecommand(p);
    for (;;) {
        i32 t = peek(p);
        if (t!=T_ANDAND && t!=T_OROR) {
            return left;
        }
        advance(p);
        newlines(p);
        Node *right = parsepipecommand(p);
        left = connect(L, left, right, t);
    }
}

// list1 when compound, else simple_list1. Consumes a trailing ; or &
// terminator, and in a compound list also the newlines after it.
static Node *parselist(Parser *p, b32 compound)
{
    Lexer *L    = p->L;
    Node  *left = parseandor(p);
    for (;;) {
        i32 t = peek(p);
        if (t!=';' && t!='&' && !(compound && t=='\n')) {
            if (!compound && L->nheredoc) {
                gatherheredocs(L);
            }
            return left;
        }
        advance(p);
        if (compound) {
            newlines(p);
        }
        if (!startscommand(peek(p))) {
            if (t == '&') {
                left = left->kind==N_CONNECT ? connectasync(L, left, 0)
                                             : connect(L, left, 0, '&');
            }
            if (L->nheredoc && (!compound || L->last=='\n')) {
                gatherheredocs(L);
            }
            return left;
        }
        Node *right = parseandor(p);
        if (t == '&') {
            left = left->kind==N_CONNECT ? connectasync(L, left, right)
                                         : connect(L, left, right, '&');
        } else if (t=='\n' && (L->pst & PST_CMDSUBST)) {
            left = connect(L, left, right, '\n');
        } else {
            left = connect(L, left, right, ';');
        }
    }
}

static Node *parsecompound(Parser *p)  // compound_list
{
    newlines(p);
    return parselist(p, 1);
}

// do compound_list done, or { compound_list }
static Node *parseloopbody(Parser *p)
{
    Lexer *L = p->L;
    i32    t = peek(p);
    if (t == T_DO) {
        advance(p);
        Node *body = parsecompound(p);
        expect(p, T_DONE);
        return body;
    }
    if (t == '{') {
        advance(p);
        Node *body = parsecompound(p);
        expect(p, '}');
        popnest(L);  // the brace
        return body;
    }
    syntaxerror(p);
}

// Index just past the $(...) or ${...} at s[i], found by lexing it as
// the parser would, or s.len if it does not end (and then ended, if
// given, is cleared).
static iz skipexpansion(Lexer *outer, Str s, iz i, b32 dquote, b32 *ended = 0)
{
    b32 dummy;
    ended  = ended ? ended : &dummy;
    *ended = 0;
    Lexer *L = newlexer(outer->a, cuthead(s, i+2));
    L->extglob  = outer->extglob;
    L->posix    = outer->posix;
    L->utf8     = outer->utf8;
    L->stacktop = outer->stacktop;
    L->stackmax = outer->stackmax;
    if (__builtin_setjmp(L->jmp)) {
        if (L->overflow) {
            outer->overflow = 1;
            parsefail(outer);
        }
        return s.len;
    }
    b32 ok = 1;
    if (s[i+1] == '(') {
        ok = parsecomsub(L, 0, '(', ')', P_COMMAND).ok;
    } else {
        i32 c = shellgetc(L, 1);
        shellungetc(L, c);
        if (funsubchar(c)) {
            ok = parsecomsub(L, 0, '{', '}', P_COMMAND).ok;
        } else {
            i32 dq = dquote ? P_DQUOTE : 0;
            ok = !!parsematched(L, 0, '{', '}', P_FIRSTCLOSE|P_DOLBRACE|dq).data;
        }
    }
    if (!ok) {
        return s.len;
    }
    *ended = 1;
    // Bytes consumed: everything read minus what remains in the line,
    // which may carry a newline that the source lacked.
    iz unread = L->line.len - L->idx + (L->ungot != 0);
    if (L->srcpos==L->src.len && L->src.len && L->src[L->src.len-1]!='\n') {
        unread--;
    }
    iz end = i + 2 + L->srcpos - unread;
    return end<s.len ? end : s.len;
}

// Index of the first delim in s outside quotes, backquotes, $(...),
// and ${...}, or s.len (bash's skip_to_delim).
static iz skipdelim(Lexer *L, Str s, u8 delim)
{
    for (iz i = 0; i < s.len;) {
        u8 c = s[i];
        if (c == '\\') {
            i += 2;
        } else if (c == '`') {
            for (i++; i<s.len && s[i]!='`'; i++) {
                if (s[i] == '\\') i++;
            }
            i++;
        } else if (c == delim) {
            return i;
        } else if (c == '\'') {
            for (i++; i<s.len && s[i]!='\''; i++) {}
            i++;
        } else if (c == '"') {
            for (i++; i<s.len && s[i]!='"'; i++) {
                if (s[i] == '\\') {
                    i++;
                } else if (s[i]=='$' && i+1<s.len && (s[i+1]=='(' || s[i+1]=='{')) {
                    i = skipexpansion(L, s, i, 1) - 1;
                }
            }
            i++;
        } else if (c=='$' && i+1<s.len && (s[i+1]=='(' || s[i+1]=='{')) {
            i = skipexpansion(L, s, i, 0);
        } else {
            i++;
        }
    }
    return s.len;
}

// Split for ((...)) into exactly three expressions at top-level ';'.
static Word *splitarithfor(Lexer *L, Word *w, i32 line)
{
    Str    s     = w->text;
    Word  *parts = 0;
    Word **tail  = &parts;
    i32    count = 0;
    for (;;) {
        for (; s.len && shellblank(s[0]); s = cuthead(s, 1)) {}
        iz i = skipdelim(L, s, ';');
        if (i > s.len) {
            i = s.len;
        }
        Str e = takehead(s, i);
        if (++count <= 3) {
            *tail = newword(L->a, e.len ? e : Str("1"), 0);
            tail  = &(*tail)->next;
        }
        if (i == s.len) {
            break;
        }
        s = cuthead(s, i+1);
    }
    if (count != 3) {
        message(L, line, count<3 ? Str("syntax error: arithmetic expression required")
                                 : Str("syntax error: `;' unexpected"));
        Buf b(L->a, 64);
        print(&b, "syntax error: `((");
        print(&b, w->text);
        print(&b, "))'");
        message(L, line, finish(&b));
        parsefail(L);
    }
    return parts;
}

static Node *parsefor(Parser *p, i32 kind)
{
    Lexer *L = p->L;
    Node  *n = newnode(L, kind);
    advance(p);  // for or select

    if (kind==N_FOR && peek(p)==T_ARITHFOR) {
        n->kind = N_ARITHFOR;
        Word *exprs = L->val.word;
        i32   line  = L->lineno;
        advance(p);
        i32 t = peek(p);
        if (t==';' || t=='\n' || t==T_EOF) {
            advance(p);
            newlines(p);
        }
        n->b = parseloopbody(p);
        n->arith = splitarithfor(L, exprs, line);
        popnest(L);
        return n;
    }

    n->name = expectword(p);
    if (peek(p) == ';') {
        advance(p);
        newlines(p);
        n->words = newword(L->a, "\"$@\"", 0);
    } else {
        newlines(p);
        if (peek(p) == T_IN) {
            advance(p);
            n->inlist = 1;
            Word **tail = &n->words;
            while (peek(p) == T_WORD) {
                *tail = L->val.word;
                tail  = &(*tail)->next;
                advance(p);
            }
            i32 t = peek(p);
            if (t!='\n' && t!=';' && t!=T_EOF) {
                syntaxerror(p);
            }
            advance(p);
            newlines(p);
        } else {
            n->words = newword(L->a, "\"$@\"", 0);
        }
    }
    n->b = parseloopbody(p);
    popnest(L);
    return n;
}

static Node *parsecase(Parser *p)
{
    Lexer *L = p->L;
    Node  *n = newnode(L, N_CASE);
    advance(p);  // case
    n->words = expectword(p);
    newlines(p);
    expect(p, T_IN);

    Case **tail = &n->cases;
    for (;;) {
        newlines(p);
        if (peek(p) == T_ESAC) {
            advance(p);
            break;
        }
        Case *c = alloc<Case>(L->a);
        if (peek(p) == '(') {
            advance(p);
        }
        Word **ptail = &c->patterns;
        for (;;) {
            *ptail = expectword(p);
            ptail  = &(*ptail)->next;
            if (peek(p) != '|') break;
            advance(p);
        }
        expect(p, ')');
        newlines(p);
        i32 t = peek(p);
        if (t!=T_ESAC && t!=T_DSEMI && t!=T_SEMIAND && t!=T_DSEMIAND) {
            c->body = parselist(p, 1);
        }
        *tail = c;
        tail  = &c->next;
        t = peek(p);
        if (t == T_ESAC) {
            advance(p);
            break;
        } else if (t == T_DSEMI) {
            c->term = CASE_BREAK;
        } else if (t == T_SEMIAND) {
            c->term = CASE_FALLTHROUGH;
        } else if (t == T_DSEMIAND) {
            c->term = CASE_TESTNEXT;
        } else {
            syntaxerror(p);
        }
        advance(p);
    }
    popnest(L);
    return n;
}

static Node *parseif(Parser *p)  // after IF or ELIF
{
    Lexer *L = p->L;
    Node  *n = newnode(L, N_IF);
    advance(p);  // if or elif
    n->a = parsecompound(p);
    expect(p, T_THEN);
    n->b = parsecompound(p);
    i32 t = peek(p);
    if (t == T_ELSE) {
        advance(p);
        n->c = parsecompound(p);
    } else if (t == T_ELIF) {
        n->c = parseif(p);
    }
    return n;
}

static Node *parseshell(Parser *p)
{
    Lexer *L = p->L;
    i32    t = peek(p);
    Node  *n = 0;
    switch (t) {
    case T_FOR:
        return parsefor(p, N_FOR);
    case T_SELECT:
        return parsefor(p, N_SELECT);
    case T_CASE:
        return parsecase(p);
    case T_WHILE:
    case T_UNTIL:
        n = newnode(L, t==T_WHILE ? N_WHILE : N_UNTIL);
        advance(p);
        n->a = parsecompound(p);
        expect(p, T_DO);
        n->b = parsecompound(p);
        expect(p, T_DONE);
        popnest(L);
        return n;
    case T_IF:
        n = parseif(p);
        expect(p, T_FI);
        popnest(L);
        return n;
    case '(':
        n = newnode(L, N_SUBSHELL);
        advance(p);
        n->a = parsecompound(p);
        expect(p, ')');
        popnest(L);
        return n;
    case '{':
        n = newnode(L, N_GROUP);
        advance(p);
        n->a = parsecompound(p);
        expect(p, '}');
        popnest(L);
        return n;
    case T_ARITHCMD:
        n = newnode(L, N_ARITH);
        n->arith = L->val.word;
        advance(p);
        return n;
    case T_CONDSTART:
        advance(p);
        if (peek(p) != T_CONDCMD) {
            syntaxerror(p);
        }
        n = newnode(L, N_COND);
        n->cond = L->val.cond;
        advance(p);
        expect(p, T_CONDEND);
        popnest(L);
        return n;
    }
    syntaxerror(p);
}


// Command substitution: parse the body with a recursive parser, then
// replace the text with its canonical printed form, as bash does.

struct LexState {
    i32    pst, current, last, before, twoago;
    i32    esacs, expectin, expectcmd, eoftoken, toread;
    Token  toreadval, val;
    b32    expandaliases, extglob;
    i32    nheredoc;
    Redir *heredocs[HEREDOCMAX];
};

static LexState savelex(Lexer *L)
{
    LexState s = {};
    s.pst       = L->pst;
    s.current   = L->current;
    s.last      = L->last;
    s.before    = L->before;
    s.twoago    = L->twoago;
    s.esacs     = L->esacs;
    s.expectin  = L->expectin;
    s.expectcmd = L->expectcmd;
    s.eoftoken  = L->eoftoken;
    s.toread    = L->toread;
    s.toreadval = L->toreadval;
    s.val       = L->val;
    s.expandaliases = L->expandaliases;
    s.extglob   = L->extglob;
    s.nheredoc  = L->nheredoc;
    for (i32 i = 0; i < L->nheredoc; i++) {
        s.heredocs[i] = L->heredocs[i];
    }
    return s;
}

static void restorelex(Lexer *L, LexState *s)
{
    L->pst       = s->pst;
    L->current   = s->current;
    L->last      = s->last;
    L->before    = s->before;
    L->twoago    = s->twoago;
    L->esacs     = s->esacs;
    L->expectin  = s->expectin;
    L->expectcmd = s->expectcmd;
    L->eoftoken  = s->eoftoken;
    L->toread    = s->toread;
    L->toreadval = s->toreadval;
    L->val       = s->val;
    L->expandaliases = s->expandaliases;
    L->extglob   = s->extglob;
    L->nheredoc  = s->nheredoc;
    for (i32 i = 0; i < s->nheredoc; i++) {
        L->heredocs[i] = s->heredocs[i];
    }
}

static Matched parsecomsub(Lexer *L, i32 qc, i32 open, i32 close, i32 flags)
{
    checkstack(L);
    Matched r = {};
    i32 spec  = 0;
    if (open == '(') {
        i32 c = shellgetc(L, 1);
        shellungetc(L, c);
        if (c == '(') {
            r.text = parsematched(L, qc, open, close, P_ARITH);
            r.ok   = r.text.data != 0;
            return r;
        }
    } else {
        i32 c = shellgetc(L, 1);
        if (!funsubchar(c)) {
            shellungetc(L, c);
            r.text = parsematched(L, qc, open, close, flags);
            r.ok   = r.text.data != 0;
            return r;
        }
        spec = c;
        if (c == '\n') {
            shellungetc(L, c);
        }
    }

    LexState saved = savelex(L);
    L->pst &= ~(PST_REGEXP|PST_EXTPAT|PST_CONDCMD|PST_CONDEXPR|PST_COMPASSIGN);
    L->pst &= ~(PST_CASEPAT|PST_ALEXPNEXT|PST_SUBSHELL|PST_REDIRLIST);
    L->pst |= PST_CMDSUBST|PST_EOFTOKEN|PST_NOEXPAND;
    if (open == '{') {
        L->pst |= PST_FUNSUBST;
    }
    L->eoftoken = close;
    L->nheredoc = 0;
    L->esacs = L->expectin = L->expectcmd = 0;
    if (L->expandaliases) {
        L->expandaliases = L->posix;
    }
    L->current = '\n';
    L->toread  = open=='(' ? T_DOLPAREN : T_DOLBRACE;

    void *outer[5];
    for (i32 i = 0; i < 5; i++) outer[i] = L->jmp[i];
    if (__builtin_setjmp(L->jmp)) {
        for (i32 i = 0; i < 5; i++) L->jmp[i] = outer[i];
        if (!L->eofreached) {
            parsefail(L);  // fatal: unwind the whole parse
        }
        L->eoftoken      = saved.eoftoken;
        L->expandaliases = saved.expandaliases;
        L->pst          |= PST_NOERROR;
        return r;  // not ok
    }
    Parser p = {};
    p.L = L;
    advance(&p);  // the DOLPAREN or DOLBRACE marker
    newlines(&p);
    Node *body = 0;
    if (peek(&p) != close) {
        body = parsecompound(&p);
    }
    if (peek(&p) != close) {
        syntaxerror(&p);
    }
    for (i32 i = 0; i < 5; i++) L->jmp[i] = outer[i];
    b32 wassemi = 0, wasnewline = 0, wasword = 0;
    if (open == '{') {
        wassemi    = L->last == ';';
        wasnewline = L->last == '\n';
        wasword    = (wassemi || wasnewline) &&
                     (L->before==T_WORD || L->before==T_ASSIGN);
    }

    if (L->nheredoc > 0) {
        Buf b(L->a, 64);
        print(&b, "command substitution: ");
        print(&b, (i64)L->nheredoc);
        print(&b, L->nheredoc==1 ? Str(" unterminated here-document")
                                 : Str(" unterminated here-documents"));
        message(L, L->lineno, finish(&b), MSG_WARNING);
        gatherheredocs(L);
    }
    restorelex(L, &saved);

    Str text = printcomsub(L->a, body);
    Tokbuf t = {};
    if (open == '(') {
        if (text.len && text[0]=='(') {
            tokput(L, &t, ' ');
        }
        tokputs(L, &t, text);
    } else if (!text.len) {
        tokput(L, &t, ' ');
    } else {
        u8 lastc = text[text.len-1];
        tokput(L, &t, spec=='|' ? '|' : ' ');
        tokputs(L, &t, text);
        if (wasnewline) {
            tokput(L, &t, '\n');
        } else if (wasword || wassemi) {
            tokput(L, &t, ';');
        } else if (lastc!='\n' && lastc!=';' && lastc!='&') {
            tokput(L, &t, ';');
        }
        tokput(L, &t, ' ');
    }
    tokput(L, &t, (u8)close);
    r.text = tokstr(&t);
    r.ok   = 1;
    return r;
}


// Entry points

enum : i32 { PARSE_OK, PARSE_EOF, PARSE_ERROR, PARSE_DISCARD };

struct Parsed {
    Node *cmd;
    i32   status;
};

static Lexer *newlexer(Arena *a, Str src)
{
    Lexer *L = alloc<Lexer>(a);
    L->a       = a;
    L->src     = src;
    L->ntop    = -1;
    L->extquote = 1;
    L->msgtail = &L->msgs;
    return L;
}

// Parse the next complete command (one input unit). Nodes and messages
// are allocated in L->a.
static Parsed parseunit(Lexer *L)
{
    Parsed r = {};
    if (__builtin_setjmp(L->jmp)) {
        // bash's reset_parser: resynchronize at the next line
        r.status = L->discard ? PARSE_DISCARD : PARSE_ERROR;
        if (L->discard) {
            L->comsuberr = 0;  // the discard skipped parse_comsub's exit
        }
        L->discard = 0;
        L->expandaliases = L->aliasopt;
        L->extglob       = L->extglobopt;
        L->ddepth  = 0;
        L->heredocstring = 0;
        L->braces = L->esacs = L->expectin = L->expectcmd = 0;
        L->pst      = 0;
        L->nheredoc = 0;
        L->pushed   = 0;
        L->toread   = 0;
        L->ungot    = 0;
        L->current  = L->last = '\n';
        L->idx      = L->line.len;
        return r;
    }

    L->reported = 0;
    byte top = 0;
    L->stacktop = &top;
    Parser p = {};
    p.L = L;
    i32 t = peek(&p);
    if (t == T_EOF) {
        r.status = PARSE_EOF;
        return r;
    }
    if (t == '\n') {
        advance(&p);
        return r;
    }
    r.cmd = parselist(&p, 0);
    t = peek(&p);
    if (t!='\n' && t!=T_EOF) {
        syntaxerror(&p);
    }
    advance(&p);
    return r;
}


// Deep copies, for function bodies that outlive the parse

static Word *copywords(Arena *a, Word *w)
{
    Word  *head = 0;
    Word **tail = &head;
    for (; w; w = w->next) {
        Word *c = alloc<Word>(a);
        c->text  = clone(a, w->text);
        c->flags = w->flags;
        *tail = c;
        tail  = &c->next;
    }
    return head;
}

static Word *copyword(Arena *a, Word *w)
{
    if (!w) return 0;
    Word *c = alloc<Word>(a);
    c->text  = clone(a, w->text);
    c->flags = w->flags;
    return c;
}

static Redir *copyredirs(Arena *a, Redir *r)
{
    Redir  *head = 0;
    Redir **tail = &head;
    for (; r; r = r->next) {
        Redir *c = alloc<Redir>(a);
        *c = *r;
        c->next   = 0;
        c->srcvar = copyword(a, r->srcvar);
        c->word   = copyword(a, r->word);
        c->body   = clone(a, r->body);
        c->eof    = clone(a, r->eof);
        *tail = c;
        tail  = &c->next;
    }
    return head;
}

static Cond *copycond(Arena *a, Cond *c)
{
    if (!c) return 0;
    Cond *r = alloc<Cond>(a);
    *r = *c;
    r->op    = copyword(a, c->op);
    r->left  = copycond(a, c->left);
    r->right = copycond(a, c->right);
    return r;
}

static Node *copynode(Arena *a, Node *n)
{
    if (!n) return 0;
    Node *r = alloc<Node>(a);
    *r = *n;
    r->a      = copynode(a, n->a);
    r->b      = copynode(a, n->b);
    r->c      = copynode(a, n->c);
    r->words  = copywords(a, n->words);
    r->name   = copyword(a, n->name);
    r->redirs = copyredirs(a, n->redirs);
    r->cond   = copycond(a, n->cond);
    r->arith  = copywords(a, n->arith);
    Case **tail = &r->cases;
    *tail = 0;
    for (Case *c = n->cases; c; c = c->next) {
        Case *d = alloc<Case>(a);
        d->patterns = copywords(a, c->patterns);
        d->body     = copynode(a, c->body);
        d->term     = c->term;
        *tail = d;
        tail  = &d->next;
    }
    return r;
}
