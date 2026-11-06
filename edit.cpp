// The line editor: readline's emacs mode for an interactive shell on a
// terminal (notes/m6.md, notes/research/readline.md)
//
// The editor draws through the terminal's VT subset and reads xterm key
// bytes from descriptor 0, writing to descriptor 2 as bash's readline
// does. The line, the prompt's last line and the cursor are laid out
// into rows of the screen's width (one less without deferred wrap); a
// redisplay compares the new rows with those on screen and rewrites
// only what changed, moving the cursor by CR, LF, and CSI A/C/D.
//
// This is free and unencumbered software released into the public domain.

// Bindable functions, with readline's names (edfuncs).
enum : i32 {
    EF_NONE,  // unbound: ring the bell
    EF_SELF_INSERT,
    EF_ACCEPT_LINE,
    EF_BEGINNING_OF_LINE,
    EF_END_OF_LINE,
    EF_FORWARD_CHAR,
    EF_BACKWARD_CHAR,
    EF_FORWARD_WORD,
    EF_BACKWARD_WORD,
    EF_DELETE_CHAR,
    EF_BACKWARD_DELETE_CHAR,
    EF_KILL_LINE,
    EF_UNIX_LINE_DISCARD,
    EF_UNIX_WORD_RUBOUT,
    EF_KILL_WORD,
    EF_BACKWARD_KILL_WORD,
    EF_CLEAR_SCREEN,
    EF_ABORT,
    EF_TRANSPOSE_CHARS,
    EF_QUOTED_INSERT,
    EF_COMPLETE,
    EF_TAB_INSERT,
    EF_PREVIOUS_HISTORY,
    EF_NEXT_HISTORY,
    EF_REDRAW_CURRENT_LINE,
    EF_COUNT,
};

[[maybe_unused]]  // until inputrc (step 6)
static Str edfuncs[EF_COUNT] = {
    "",
    "self-insert",
    "accept-line",
    "beginning-of-line",
    "end-of-line",
    "forward-char",
    "backward-char",
    "forward-word",
    "backward-word",
    "delete-char",
    "backward-delete-char",
    "kill-line",
    "unix-line-discard",
    "unix-word-rubout",
    "kill-word",
    "backward-kill-word",
    "clear-screen",
    "abort",
    "transpose-chars",
    "quoted-insert",
    "complete",
    "tab-insert",
    "previous-history",
    "next-history",
    "redraw-current-line",
};

static iz readstdin(Shell *, File *, u8 *, iz);  // unabashed.cpp

// Key bindings: a trie of key sequences. A node with children is a
// prefix: the editor waits for the next key, at most keyseq-timeout if
// the node is bound too (ESC alone, or ESC [ A).
struct KeyNode {
    KeyNode *kids;
    KeyNode *sib;
    i32      fn;    // EF_*, or EF_NONE
    u8       key;
};

static KeyNode *keychild(KeyNode *n, u8 key)
{
    for (KeyNode *k = n->kids; k; k = k->sib) {
        if (k->key == key) return k;
    }
    return 0;
}

static void keybind(Arena *a, KeyNode *root, Str seq, i32 fn)
{
    KeyNode *n = root;
    for (iz i = 0; i < seq.len; i++) {
        KeyNode *k = keychild(n, seq[i]);
        if (!k) {
            k = alloc<KeyNode>(a);
            k->key  = seq[i];
            k->sib  = n->kids;
            n->kids = k;
        }
        n = k;
    }
    n->fn = fn;
}

// One physical row of the display: its cells' bytes, where each cell
// ends in them and at which column, and its width in columns.
struct DRow {
    Str  text;
    i32 *ends;   // byte offset after each cell
    i32 *cols;   // column after each cell
    i32  ncells;
    i32  width;
};

struct Layout {
    DRow *rows;
    i32   nrows;
    i32   prow;  // the cursor's position for point
    i32   pcol;
};

struct Editor {
    Arena    perm;      // bindings, settings
    Arena    lay[2];    // layouts: the one on screen and the next
    i32      cur;       // which of lay is on screen
    KeyNode  root;
    i32      keytimeout;  // keyseq-timeout, ms
    // The line being edited
    Buf      line;
    iz       point;
    Arena    linemem;
    // The terminal
    Tty      tty;
    i32      width;     // usable columns: one less without deferred wrap
    Handle   in;
    Handle   out;
    // The display
    Str      prompt;    // the prompt's last line, \1 \2 marking the invisible
    Layout   shown;
    i32      crow;      // where the cursor is, relative to the prompt's row
    i32      ccol;
    b32      pending;   // the terminal has a wrap pending (crow, ccol at
                        // the last column)
    Buf      out_;      // output for one redisplay
    b32      done;
    i32      result;    // ED_*
};

enum : i32 { ED_LINE, ED_EOF, ED_INTR, ED_NOTTY };

static void edbindings(Editor *ed)
{
    Arena   *a = &ed->perm;
    KeyNode *r = &ed->root;
    for (i32 c = 0x20; c < 0x100; c++) {
        u8 k = (u8)c;
        if (c != 0x7f) keybind(a, r, {&k, 1}, EF_SELF_INSERT);
    }
    struct { char const *seq; i32 fn; } map[] = {
        {"\x01", EF_BEGINNING_OF_LINE},   {"\x02", EF_BACKWARD_CHAR},
        {"\x04", EF_DELETE_CHAR},         {"\x05", EF_END_OF_LINE},
        {"\x06", EF_FORWARD_CHAR},        {"\x07", EF_ABORT},
        {"\x08", EF_BACKWARD_DELETE_CHAR},{"\x09", EF_COMPLETE},
        {"\x0a", EF_ACCEPT_LINE},         {"\x0b", EF_KILL_LINE},
        {"\x0c", EF_CLEAR_SCREEN},        {"\x0d", EF_ACCEPT_LINE},
        {"\x0e", EF_NEXT_HISTORY},        {"\x10", EF_PREVIOUS_HISTORY},
        {"\x11", EF_QUOTED_INSERT},       {"\x14", EF_TRANSPOSE_CHARS},
        {"\x15", EF_UNIX_LINE_DISCARD},   {"\x16", EF_QUOTED_INSERT},
        {"\x17", EF_UNIX_WORD_RUBOUT},    {"\x7f", EF_BACKWARD_DELETE_CHAR},
        {"\x1b\x08", EF_BACKWARD_KILL_WORD}, {"\x1b\x7f", EF_BACKWARD_KILL_WORD},
        {"\x1b\x07", EF_ABORT},
        {"\x1b" "b", EF_BACKWARD_WORD},   {"\x1b" "f", EF_FORWARD_WORD},
        {"\x1b" "d", EF_KILL_WORD},
        {"\x18\x07", EF_ABORT},
        {"\x1b[A", EF_PREVIOUS_HISTORY},  {"\x1b[B", EF_NEXT_HISTORY},
        {"\x1b[C", EF_FORWARD_CHAR},      {"\x1b[D", EF_BACKWARD_CHAR},
        {"\x1b[H", EF_BEGINNING_OF_LINE}, {"\x1b[F", EF_END_OF_LINE},
        {"\x1bOA", EF_PREVIOUS_HISTORY},  {"\x1bOB", EF_NEXT_HISTORY},
        {"\x1bOC", EF_FORWARD_CHAR},      {"\x1bOD", EF_BACKWARD_CHAR},
        {"\x1bOH", EF_BEGINNING_OF_LINE}, {"\x1bOF", EF_END_OF_LINE},
        {"\x1b[1;5C", EF_FORWARD_WORD},   {"\x1b[1;5D", EF_BACKWARD_WORD},
        {"\x1b[1;3C", EF_FORWARD_WORD},   {"\x1b[1;3D", EF_BACKWARD_WORD},
        {"\x1b[3;5~", EF_KILL_WORD},      {"\x1b[3~", EF_DELETE_CHAR},
        {"\x1b[1~", EF_BEGINNING_OF_LINE},{"\x1b[4~", EF_END_OF_LINE},
    };
    for (iz i = 0; i < countof(map); i++) {
        Str seq = {(u8 *)map[i].seq, (iz)__builtin_strlen(map[i].seq)};
        keybind(a, r, seq, map[i].fn);
    }
}

static Editor *editor(Shell *sh)
{
    static Editor *ed;  // one per process: the top level's
    if (!ed) {
        Arena perm = newarena(sh->os, (iz)1<<24);
        ed = alloc<Editor>(&perm);
        ed->perm       = perm;
        ed->lay[0]     = newarena(sh->os, (iz)1<<24);
        ed->lay[1]     = newarena(sh->os, (iz)1<<24);
        ed->linemem    = newarena(sh->os, (iz)1<<24);
        ed->keytimeout = 500;
        edbindings(ed);
    }
    return ed;
}


// Layout

struct Layouter {
    Arena  *a;
    Layout  l;
    i32     cap;     // rows allocated
    Buf     text;    // the row being built
    Buf     ends;
    Buf     cols;
    i32     col;
    i32     width;
    i32     vcol;    // column on the logical line, for tabs
};

static void laynewrow(Layouter *y)
{
    if (y->l.nrows == y->cap) {
        i32   cap  = y->cap ? y->cap*2 : 8;
        DRow *rows = alloc<DRow>(y->a, cap);
        for (i32 i = 0; i < y->l.nrows; i++) rows[i] = y->l.rows[i];
        y->l.rows = rows;
        y->cap    = cap;
    }
    y->text = Buf(y->a, 64);
    y->ends = Buf(y->a, 64);
    y->cols = Buf(y->a, 64);
    y->col  = 0;
    y->l.nrows++;
}

static void layfinish(Layouter *y)
{
    DRow *r   = y->l.rows + y->l.nrows - 1;
    r->text   = finish(&y->text);
    Str e     = finish(&y->ends);
    Str c     = finish(&y->cols);
    r->ends   = (i32 *)e.data;
    r->cols   = (i32 *)c.data;
    r->ncells = (i32)(e.len / (iz)sizeof(i32));
    r->width  = y->col;
}

// Add a cell of the given bytes and width, wrapping to a new row when
// it does not fit.
static void laycell(Layouter *y, Str bytes, i32 w)
{
    if (w>0 && y->col+w > y->width && y->col) {
        layfinish(y);
        laynewrow(y);
    }
    print(&y->text, bytes);
    y->col  += w;
    y->vcol += w;
    i32 end = (i32)y->text.len;
    i32 col = y->col;
    print(&y->ends, {(u8 *)&end, sizeof(end)});
    print(&y->cols, {(u8 *)&col, sizeof(col)});
}

// Bytes that draw nothing (the prompt's invisible parts): with the
// next cell, or a cell of their own at the row's end.
static void layinvisible(Layouter *y, Str bytes)
{
    laycell(y, bytes, 0);
}

static Layout layout(Arena *a, Str prompt, Str line, iz point, i32 width)
{
    Layouter y = {};
    y.a     = a;
    y.width = width;
    laynewrow(&y);
    b32 hidden = 0;
    for (iz i = 0; i < prompt.len;) {
        u8 c = prompt[i];
        if (c==1 || c==2) {
            hidden = c == 1;
            i++;
            continue;
        }
        iz  beg = i;
        i32 cp  = utf8decode(prompt, &i);
        Str s   = {prompt.data+beg, i-beg};
        if (hidden) {
            layinvisible(&y, s);
        } else {
            i32 w = cpwidth(cp);
            laycell(&y, s, w<0 ? 1 : w);  // as readline counts the visible
        }
    }
    y.l.prow = -1;
    for (iz i = 0; i <= line.len;) {
        if (i == point) {
            // The cursor goes where the next cell would: past a full
            // row, at the start of the next
            if (y.col >= y.width) {
                layfinish(&y);
                laynewrow(&y);
            }
            y.l.prow = y.l.nrows - 1;
            y.l.pcol = y.col;
        }
        if (i == line.len) break;
        u8  c   = line[i];
        iz  beg = i;
        i32 cp  = utf8decode(line, &i);
        if (c == '\t') {
            i32 n = 8 - y.vcol%8;
            for (i32 k = 0; k < n; k++) laycell(&y, " ", 1);  // as readline, may wrap
        } else if (c<0x20 || c==0x7f) {
            u8 buf[2] = {'^', (u8)(c==0x7f ? '?' : c|0x40)};
            laycell(&y, {buf, 2}, 2);
        } else {
            i32 w = cpwidth(cp);
            laycell(&y, {line.data+beg, i-beg}, w<0 ? 1 : w);
        }
    }
    layfinish(&y);
    return y.l;
}


// Redisplay

static void edput(Editor *ed, Str s)
{
    print(&ed->out_, s);
}

static void edcsi(Editor *ed, i32 n, u8 final)
{
    edput(ed, "\x1b[");
    if (n != 1) print(&ed->out_, (i64)n);
    putbyte(&ed->out_, final);
}

// Move the cursor to row, col of the display.
static void edmove(Editor *ed, i32 row, i32 col)
{
    if (ed->pending) {
        edput(ed, "\r");
        ed->ccol    = 0;
        ed->pending = 0;
    }
    if (row < ed->crow) {
        edcsi(ed, ed->crow-row, 'A');
    } else if (row > ed->crow) {
        if (ed->ccol) edput(ed, "\r");
        ed->ccol = 0;
        for (i32 i = ed->crow; i < row; i++) edput(ed, "\n");
    }
    ed->crow = row;
    if (col == ed->ccol) return;
    if (!col) {
        edput(ed, "\r");
    } else if (col > ed->ccol) {
        edcsi(ed, col-ed->ccol, 'C');
    } else if (ed->ccol-col == 1) {
        edput(ed, "\b");
    } else {
        edcsi(ed, ed->ccol-col, 'D');
    }
    ed->ccol = col;
}

// Write cells [from, ncells) of a row, the cursor at their start.
static void edwrite(Editor *ed, DRow *r, i32 from)
{
    i32 beg = from ? r->ends[from-1] : 0;
    edput(ed, {r->text.data+beg, r->text.len-beg});
    ed->ccol = r->width;
    if (ed->ccol >= ed->tty.cols) {
        ed->ccol    = ed->tty.cols - 1;
        ed->pending = 1;
    }
}

static void edflush(Editor *ed, Shell *sh)
{
    Str s = finish(&ed->out_);
    if (s.len) fdwrite(sh, 2, s);
    ed->out_ = Buf(&ed->linemem, 256);
}

static i32 cellcol(DRow *r, i32 i)  // the column where cell i starts
{
    return i ? r->cols[i-1] : 0;
}

// Bring the screen from what is shown to the next layout.
static void edupdate(Editor *ed, Layout *nl)
{
    Layout *ol = &ed->shown;
    i32 n = nl->nrows>ol->nrows ? nl->nrows : ol->nrows;
    for (i32 row = 0; row < n; row++) {
        DRow *o = row<ol->nrows ? ol->rows+row : 0;
        DRow *w = row<nl->nrows ? nl->rows+row : 0;
        if (!w) {
            if (o->width) {
                edmove(ed, row, 0);
                edput(ed, "\x1b[K");
            }
            continue;
        }
        i32 same = 0;  // cells in common
        if (o) {
            for (; same<o->ncells && same<w->ncells; same++) {
                i32 ob = same ? o->ends[same-1] : 0;
                i32 wb = same ? w->ends[same-1] : 0;
                Str oc = {o->text.data+ob, o->ends[same]-ob};
                Str wc = {w->text.data+wb, w->ends[same]-wb};
                if (oc!=wc || o->cols[same]!=w->cols[same]) break;
            }
            if (same==w->ncells && same==o->ncells) continue;
        }
        if (same < w->ncells) {
            edmove(ed, row, cellcol(w, same));
            edwrite(ed, w, same);
        }
        if (o && o->width>w->width) {
            edmove(ed, row, w->width);
            edput(ed, "\x1b[K");
        }
    }
    edmove(ed, nl->prow, nl->pcol);
    ed->shown = *nl;
}

static void redisplay(Editor *ed, Shell *sh)
{
    Arena *a = &ed->lay[!ed->cur];
    *a = {a->mem ? (byte *)(a->mem+1) : a->beg, a->end, a->mem};
    Str line = {ed->line.data, ed->line.len};
    Layout nl = layout(a, ed->prompt, line, ed->point, ed->width);
    edupdate(ed, &nl);
    ed->cur = !ed->cur;
    edflush(ed, sh);
}

// Draw the prompt and line afresh on the current row.
static void edredraw(Editor *ed, Shell *sh)
{
    ed->shown   = {};
    ed->crow    = 0;
    ed->ccol    = 0;
    ed->pending = 0;
    edput(ed, "\r");
    redisplay(ed, sh);
}


// Editing functions

static void edbell(Editor *ed)
{
    edput(ed, "\a");
}

static b32 edwordchar(u8 c)
{
    return (c>='0' && c<='9') || ((c|0x20)>='a' && (c|0x20)<='z') || c>=0x80;
}

// Motion by characters, as readline's: a zero-width character (a
// combining mark) goes with the one before it.
static iz edprev(Editor *ed, iz i)  // start of the character before i
{
    Str s = {ed->line.data, ed->line.len};
    while (i > 0) {
        i--;
        while (i>0 && (s[i]&0xc0)==0x80) i--;
        iz j = i;
        if (!i || cpwidth(utf8decode(s, &j))) break;
    }
    return i;
}

static iz ednext(Editor *ed, iz i)  // past the character at i
{
    Str s = {ed->line.data, ed->line.len};
    if (i >= s.len) return s.len;
    utf8decode(s, &i);
    while (i < s.len) {  // and the zero-width ones after it
        iz j = i;
        if (cpwidth(utf8decode(s, &j))) break;
        i = j;
    }
    return i;
}

static iz edfwdword(Editor *ed, iz i)
{
    u8 *s = ed->line.data;
    for (; i<ed->line.len && !edwordchar(s[i]); i++) {}
    for (; i<ed->line.len && edwordchar(s[i]); i++) {}
    return i;
}

static iz edbackword(Editor *ed, iz i)
{
    u8 *s = ed->line.data;
    for (; i>0 && !edwordchar(s[i-1]); i--) {}
    for (; i>0 && edwordchar(s[i-1]); i--) {}
    return i;
}

static void eddelete(Editor *ed, iz beg, iz end)
{
    iz n = end - beg;
    if (n <= 0) return;
    u8 *s = ed->line.data;
    for (iz i = beg; i+n < ed->line.len; i++) s[i] = s[i+n];
    ed->line.len -= n;
    if (ed->point > end) {
        ed->point -= n;
    } else if (ed->point > beg) {
        ed->point = beg;
    }
}

static void edinsert(Editor *ed, Str s)
{
    reserve(&ed->line, s.len);
    u8 *d = ed->line.data;
    for (iz i = ed->line.len-1; i >= ed->point; i--) d[i+s.len] = d[i];
    copybytes(d+ed->point, s.data, s.len);
    ed->line.len += s.len;
    ed->point    += s.len;
}

static i32 edkey(Editor *, Shell *, i64 timeout);

static void edrun(Editor *ed, Shell *sh, i32 fn, u8 key)
{
    iz p   = ed->point;
    iz len = ed->line.len;
    switch (fn) {
    case EF_NONE:
        edbell(ed);
        break;
    case EF_SELF_INSERT: {
        // A UTF-8 sequence goes in whole
        u8 buf[4] = {key};
        iz n   = key>=0xf0 ? 4 : key>=0xe0 ? 3 : key>=0xc0 ? 2 : 1;
        iz len = 1;
        while (len < n) {
            i32 c = edkey(ed, sh, -1);
            if (c<0 || (c&0xc0)!=0x80) break;  // a stray byte is dropped
            buf[len++] = (u8)c;
        }
        edinsert(ed, {buf, len});
    } break;
    case EF_ACCEPT_LINE:
        ed->done   = 1;
        ed->result = ED_LINE;
        break;
    case EF_BEGINNING_OF_LINE:
        ed->point = 0;
        break;
    case EF_END_OF_LINE:
        ed->point = len;
        break;
    case EF_FORWARD_CHAR:
        if (p == len) edbell(ed);
        ed->point = ednext(ed, p);
        break;
    case EF_BACKWARD_CHAR:
        if (!p) edbell(ed);
        ed->point = edprev(ed, p);
        break;
    case EF_FORWARD_WORD:
        ed->point = edfwdword(ed, p);
        break;
    case EF_BACKWARD_WORD:
        ed->point = edbackword(ed, p);
        break;
    case EF_DELETE_CHAR:
        if (!len) {
            ed->done   = 1;
            ed->result = ED_EOF;
        } else if (p == len) {
            edbell(ed);
        } else {
            eddelete(ed, p, ednext(ed, p));
        }
        break;
    case EF_BACKWARD_DELETE_CHAR:
        if (!p) {
            edbell(ed);
        } else {
            eddelete(ed, edprev(ed, p), p);
        }
        break;
    case EF_KILL_LINE:
        eddelete(ed, p, len);
        break;
    case EF_UNIX_LINE_DISCARD:
        eddelete(ed, 0, p);
        break;
    case EF_UNIX_WORD_RUBOUT: {
        iz i = p;
        u8 *s = ed->line.data;
        for (; i>0 && (s[i-1]==' ' || s[i-1]=='\t'); i--) {}
        for (; i>0 && s[i-1]!=' ' && s[i-1]!='\t'; i--) {}
        if (i == p) edbell(ed);
        eddelete(ed, i, p);
    } break;
    case EF_KILL_WORD:
        eddelete(ed, p, edfwdword(ed, p));
        break;
    case EF_BACKWARD_KILL_WORD:
        eddelete(ed, edbackword(ed, p), p);
        break;
    case EF_CLEAR_SCREEN:
        edput(ed, "\x1b[H\x1b[2J");
        edflush(ed, sh);
        edredraw(ed, sh);
        return;
    case EF_ABORT:
        edbell(ed);
        break;
    case EF_TRANSPOSE_CHARS: {
        if (!p || len<2) {
            edbell(ed);
            break;
        }
        iz at = p==len ? edprev(ed, p) : p;  // at the end: the last two
        iz a  = edprev(ed, at);
        iz b  = ednext(ed, at);
        // Swapped by three reversals: characters may be any length
        u8 *d = ed->line.data;
        iz  r[3][2] = {{a, b}, {a, a+b-at}, {a+b-at, b}};
        for (i32 k = 0; k < 3; k++) {
            for (iz x = r[k][0], y = r[k][1]-1; x < y; x++, y--) {
                u8 t = d[x]; d[x] = d[y]; d[y] = t;
            }
        }
        ed->point = b;
    } break;
    case EF_QUOTED_INSERT: {
        i32 c = edkey(ed, sh, -1);
        if (c >= 0) {
            u8 b = (u8)c;
            edinsert(ed, {&b, 1});
        }
    } break;
    case EF_TAB_INSERT:
        edinsert(ed, "\t");
        break;
    case EF_COMPLETE:            // step 5
    case EF_PREVIOUS_HISTORY:    // step 3
    case EF_NEXT_HISTORY:
        edbell(ed);
        break;
    case EF_REDRAW_CURRENT_LINE:
        edredraw(ed, sh);
        return;
    }
}


// Input

// The next key byte, waiting at most timeout ms (-1: forever): -1 at
// end of input or on error, -2 on an interrupt, -3 on timeout.
static i32 edkey(Editor *, Shell *sh, i64 timeout)
{
    Task *self = sh->sched->cur;
    self->deadline = timeout<0 ? 0 : os_now(sh->os) + timeout*1000000;
    u8 c;
    iz n = readstdin(sh, sh->fds->fd[0], &c, 1);
    self->deadline = 0;
    if (self->timedout) {
        self->timedout = 0;
        return -3;
    }
    return n==1 ? c : n==-2 ? -2 : -1;
}

// Read a key sequence and run what it is bound to. False at the end of
// input.
static b32 eddispatch(Editor *ed, Shell *sh)
{
    i32 c = edkey(ed, sh, -1);
    if (c == -2) {
        ed->done   = 1;
        ed->result = ED_INTR;
        return 1;
    }
    if (c < 0) return 0;
    if (c == ed->tty.intr) {
        ed->done   = 1;
        ed->result = ED_INTR;
        return 1;
    }
    KeyNode *n = keychild(&ed->root, (u8)c);
    u8 last = (u8)c;
    while (n && n->kids) {
        i32 next = edkey(ed, sh, n->fn ? ed->keytimeout : -1);
        if (next == -3) break;  // the prefix's own binding
        if (next < 0) return 0;
        KeyNode *k = keychild(n, (u8)next);
        if (!k) {
            // An unbound sequence: the bell, the rest dropped
            n = 0;
            break;
        }
        n    = k;
        last = (u8)next;
    }
    edrun(ed, sh, n ? n->fn : EF_NONE, last);
    return 1;
}

// Read a line on the terminal with the editor, the prompt shown first.
// ED_NOTTY if descriptors 0 and 2 are not one terminal.
static i32 editline(Shell *sh, Str prompt, Buf *out)
{
    File *fin  = sh->fds->fd[0];
    File *ferr = sh->fds->fd[2];
    if (!fin || !ferr || fin->kind!=F_OS || ferr->kind!=F_OS) return ED_NOTTY;
    Editor *ed = editor(sh);
    ed->in  = fin->h;
    ed->out = ferr->h;
    ed->tty = os_tty(sh->os, ed->in, ed->out, TTY_RAW);
    if (ed->tty.err) return ED_NOTTY;
    if (ed->tty.cols < 2) ed->tty.cols = 80;
    ed->width = ed->tty.xenl ? ed->tty.cols : ed->tty.cols-1;

    ed->linemem = {(byte *)(ed->linemem.mem+1), ed->linemem.end, ed->linemem.mem};
    ed->line    = Buf(&ed->linemem, 256);
    ed->out_    = Buf(&ed->linemem, 256);
    ed->point   = 0;
    ed->done    = 0;

    // Lines of the prompt before its last are written once
    iz nl = prompt.len;
    while (nl>0 && prompt[nl-1]!='\n') nl--;
    Str head = takehead(prompt, nl);
    for (iz i = 0; i < head.len; i++) {
        if (head[i]!=1 && head[i]!=2) putbyte(&ed->out_, head[i]);
    }
    ed->prompt = cuthead(prompt, nl);
    edredraw(ed, sh);

    for (;;) {
        if (!eddispatch(ed, sh)) {
            ed->result = ED_EOF;
            break;
        }
        if (ed->done) break;
        redisplay(ed, sh);
    }

    // Leave the cursor past the line
    Layout *l = &ed->shown;
    if (ed->result == ED_INTR) {
        // Echoed at the cursor, over the rest of the line, as by the
        // terminal's own line discipline
        edput(ed, "^C");
        if (ed->crow < l->nrows-1) edmove(ed, l->nrows-1, 0);
    } else {
        edmove(ed, l->nrows-1, l->rows[l->nrows-1].width);
    }
    edput(ed, "\r\n");
    edflush(ed, sh);
    os_tty(sh->os, ed->in, ed->out, TTY_RESTORE);
    print(out, {ed->line.data, ed->line.len});
    return ed->result;
}
