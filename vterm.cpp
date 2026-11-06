// vterm: a terminal screen for tests, interpreting the VT subset that
// the editor (and readline, with TERM=xterm) draws with, and rendering
// it as text. Used by the vos as its terminal and by tools/ptyrun.
// Needs base.cpp and unicode.cpp.
// This is free and unencumbered software released into the public domain.

enum : u8 { VA_REVERSE = 1, VA_BOLD = 2, VA_UNDER = 4 };

struct VCell {
    i32 c;     // code point, 0 if blank, -1 for a wide character's right half
    i32 comb;  // a combining mark drawn over it, or 0
    u8  attr;  // VA_*
};

enum : i32 { VT_GROUND, VT_ESC, VT_CSI, VT_OSC, VT_OSCESC, VT_CHARSET };

struct VTerm {
    VCell *cells;     // rows*cols
    i32    rows;
    i32    cols;
    i32    row;
    i32    col;
    b32    xenl;      // deferred wrap, as xterm: a write to the last
                      // column leaves the cursor there until the next
    b32    wrapnext;  // xenl: the next character goes to the next line
    b32    insert;    // insert mode (CSI 4 h)
    b32    paste;     // bracketed paste enabled (CSI ? 2004 h)
    u8     attr;      // VA_* for what is written next
    i32    bells;
    i32    scrolled;  // lines scrolled off the top
    i32    srow;      // saved cursor (ESC 7, CSI s)
    i32    scol;
    // Parser
    i32    state;
    i32    params[16];
    i32    nparams;
    u8     prefix;    // CSI private marker (?, >, =, <) or 0
    u8     inter;     // CSI intermediate byte or 0
    i32    cp;        // UTF-8 sequence being decoded
    i32    need;      // its continuation bytes still to come
    i32    least;     // the least code point its length may encode
};

static VTerm *vtnew(Arena *a, i32 rows, i32 cols, b32 xenl)
{
    VTerm *t = alloc<VTerm>(a);
    t->rows  = rows;
    t->cols  = cols;
    t->xenl  = xenl;
    t->cells = alloc<VCell>(a, (iz)rows*cols);
    return t;
}

static VCell *vtcell(VTerm *t, i32 row, i32 col)
{
    return t->cells + (iz)row*t->cols + col;
}

static void vtblank(VTerm *t, i32 row, i32 from, i32 to)  // [from, to)
{
    for (i32 c = from; c < to; c++) *vtcell(t, row, c) = {};
}

// Scroll the lines [top, bottom] up by one, blanking the bottom.
static void vtscrollup(VTerm *t, i32 top, i32 bottom)
{
    for (i32 r = top; r < bottom; r++) {
        for (i32 c = 0; c < t->cols; c++) *vtcell(t, r, c) = *vtcell(t, r+1, c);
    }
    vtblank(t, bottom, 0, t->cols);
}

static void vtscrolldown(VTerm *t, i32 top, i32 bottom)
{
    for (i32 r = bottom; r > top; r--) {
        for (i32 c = 0; c < t->cols; c++) *vtcell(t, r, c) = *vtcell(t, r-1, c);
    }
    vtblank(t, top, 0, t->cols);
}

static void vtlinefeed(VTerm *t)
{
    t->wrapnext = 0;
    if (t->row < t->rows-1) {
        t->row++;
    } else {
        vtscrollup(t, 0, t->rows-1);
        t->scrolled++;
    }
}

static void vtmoveto(VTerm *t, i32 row, i32 col)
{
    t->row      = row<0 ? 0 : row>=t->rows ? t->rows-1 : row;
    t->col      = col<0 ? 0 : col>=t->cols ? t->cols-1 : col;
    t->wrapnext = 0;
}

static void vtput(VTerm *t, i32 c)
{
    i32 w = cpwidth(c);
    if (w < 0) return;
    if (w == 0) {
        // Over the character before the cursor
        i32 col = t->wrapnext ? t->col : t->col-1;
        if (col < 0) return;
        VCell *p = vtcell(t, t->row, col);
        if (p->c==-1 && col>0) p--;
        p->comb = c;
        return;
    }
    if (t->wrapnext) {
        t->col = 0;
        vtlinefeed(t);
    }
    if (t->col+w > t->cols) {
        // A wide character at the last column goes to the next line
        vtblank(t, t->row, t->col, t->cols);
        t->col = 0;
        vtlinefeed(t);
    }
    VCell *line = vtcell(t, t->row, 0);
    if (t->insert) {
        for (i32 i = t->cols-1; i >= t->col+w; i--) line[i] = line[i-w];
    }
    // Overwriting half of a wide character blanks its other half
    if (line[t->col].c == -1 && t->col>0) line[t->col-1] = {};
    i32 end = t->col + w;
    if (end<t->cols && line[end].c==-1) line[end] = {};
    line[t->col] = {c, 0, t->attr};
    if (w == 2) line[t->col+1] = {-1, 0, t->attr};
    t->col = end;
    if (t->col >= t->cols) {
        if (t->xenl) {
            t->col      = t->cols - 1;
            t->wrapnext = 1;
        } else {
            t->col = 0;
            vtlinefeed(t);
        }
    }
}

static i32 vtparam(VTerm *t, i32 i, i32 dflt)
{
    i32 p = i<t->nparams ? t->params[i] : 0;
    return p ? p : dflt;
}

static void vtsgr(VTerm *t)
{
    if (!t->nparams) t->attr = 0;
    for (i32 i = 0; i < t->nparams; i++) {
        switch (t->params[i]) {
        case 0:  t->attr = 0;                         break;
        case 1:  t->attr |= VA_BOLD;                  break;
        case 4:  t->attr |= VA_UNDER;                 break;
        case 7:  t->attr |= VA_REVERSE;               break;
        case 22: t->attr &= (u8)~VA_BOLD;             break;
        case 24: t->attr &= (u8)~VA_UNDER;            break;
        case 27: t->attr &= (u8)~VA_REVERSE;          break;
        case 38: case 48: i = t->nparams;             break;  // colors
        }
    }
}

static void vtmode(VTerm *t, b32 on)
{
    for (i32 i = 0; i < t->nparams; i++) {
        i32 p = t->params[i];
        if (!t->prefix && p==4) t->insert = on;
        if (t->prefix=='?' && p==2004) t->paste = on;
    }
}

static void vtcsi(VTerm *t, u8 final)
{
    if (t->inter) return;
    i32 n = vtparam(t, 0, 1);
    VCell *line = vtcell(t, t->row, 0);
    switch (final) {
    case 'A': vtmoveto(t, t->row-n, t->col);            break;
    case 'B': vtmoveto(t, t->row+n, t->col);            break;
    case 'C': vtmoveto(t, t->row, t->col+n);            break;
    case 'D': vtmoveto(t, t->row, t->col-n);            break;
    case 'E': vtmoveto(t, t->row+n, 0);                 break;
    case 'F': vtmoveto(t, t->row-n, 0);                 break;
    case 'G': vtmoveto(t, t->row, n-1);                 break;
    case 'd': vtmoveto(t, n-1, t->col);                 break;
    case 'H':
    case 'f': vtmoveto(t, n-1, vtparam(t, 1, 1)-1);     break;
    case 'J':
        switch (vtparam(t, 0, 0)) {
        case 0:
            vtblank(t, t->row, t->col, t->cols);
            for (i32 r = t->row+1; r < t->rows; r++) vtblank(t, r, 0, t->cols);
            break;
        case 1:
            for (i32 r = 0; r < t->row; r++) vtblank(t, r, 0, t->cols);
            vtblank(t, t->row, 0, t->col+1);
            break;
        case 2:
            for (i32 r = 0; r < t->rows; r++) vtblank(t, r, 0, t->cols);
            break;
        }
        break;
    case 'K':
        switch (vtparam(t, 0, 0)) {
        case 0: vtblank(t, t->row, t->col, t->cols); break;
        case 1: vtblank(t, t->row, 0, t->col+1);     break;
        case 2: vtblank(t, t->row, 0, t->cols);      break;
        }
        break;
    case 'L':
        for (i32 i = 0; i<n && i<t->rows-t->row; i++) vtscrolldown(t, t->row, t->rows-1);
        break;
    case 'M':
        for (i32 i = 0; i<n && i<t->rows-t->row; i++) vtscrollup(t, t->row, t->rows-1);
        break;
    case 'S': for (i32 i = 0; i<n && i<t->rows; i++) vtscrollup(t, 0, t->rows-1);   break;
    case 'T': for (i32 i = 0; i<n && i<t->rows; i++) vtscrolldown(t, 0, t->rows-1); break;
    case 'P':
        n = n<t->cols-t->col ? n : t->cols-t->col;
        for (i32 i = t->col; i < t->cols; i++) line[i] = i+n<t->cols ? line[i+n] : VCell{};
        t->wrapnext = 0;
        break;
    case '@':
        n = n<t->cols-t->col ? n : t->cols-t->col;
        for (i32 i = t->cols-1; i >= t->col; i--) line[i] = i-n>=t->col ? line[i-n] : VCell{};
        t->wrapnext = 0;
        break;
    case 'X':
        vtblank(t, t->row, t->col, t->col+n<t->cols ? t->col+n : t->cols);
        break;
    case 'm': if (!t->prefix) vtsgr(t);                 break;
    case 'h': vtmode(t, 1);                             break;
    case 'l': vtmode(t, 0);                             break;
    case 's': t->srow = t->row; t->scol = t->col;       break;
    case 'u': vtmoveto(t, t->srow, t->scol);            break;
    }
}

static void vtbyte(VTerm *t, u8 b)
{
    switch (t->state) {
    case VT_ESC:
        t->state = VT_GROUND;
        switch (b) {
        case '[':
            t->state   = VT_CSI;
            t->nparams = 0;
            t->prefix  = 0;
            t->inter   = 0;
            t->params[0] = 0;
            break;
        case ']': t->state = VT_OSC;                         break;
        case '(': case ')': case '*': case '+': t->state = VT_CHARSET; break;
        case '7': t->srow = t->row; t->scol = t->col;        break;
        case '8': vtmoveto(t, t->srow, t->scol);             break;
        case 'D': vtlinefeed(t);                             break;
        case 'E': t->col = 0; vtlinefeed(t);                 break;
        case 'M':
            t->wrapnext = 0;
            if (t->row) {
                t->row--;
            } else {
                vtscrolldown(t, 0, t->rows-1);
            }
            break;
        case 'c':
            for (i32 r = 0; r < t->rows; r++) vtblank(t, r, 0, t->cols);
            vtmoveto(t, 0, 0);
            t->attr   = 0;
            t->insert = 0;
            break;
        }
        return;
    case VT_CSI:
        if (b>='0' && b<='9') {
            if (!t->nparams) t->nparams = 1;
            i32 *p = t->params + t->nparams - 1;
            *p = *p<100000 ? *p*10 + (b - '0') : *p;
        } else if (b == ';') {
            if (!t->nparams) t->nparams = 1;
            if (t->nparams < countof(t->params)) t->params[t->nparams++] = 0;
        } else if (b>='<' && b<='?') {
            t->prefix = b;
        } else if (b>=0x20 && b<=0x2f) {
            t->inter = b;
        } else if (b>=0x40 && b<=0x7e) {
            t->state = VT_GROUND;
            vtcsi(t, b);
        } else if (b == 0x1b) {
            t->state = VT_ESC;
        } else if (b < 0x20) {
            t->state = VT_GROUND;  // an abandoned sequence
            vtbyte(t, b);
        }
        return;
    case VT_OSC:
        if (b == 7)    t->state = VT_GROUND;
        if (b == 0x1b) t->state = VT_OSCESC;
        return;
    case VT_OSCESC:
        t->state = b=='\\' ? VT_GROUND : VT_OSC;
        return;
    case VT_CHARSET:
        t->state = VT_GROUND;
        return;
    }

    if (t->need) {
        if ((b & 0xc0) == 0x80) {
            t->cp = t->cp<<6 | (b & 0x3f);
            if (!--t->need) {
                // Overlong, a surrogate or past U+10FFFF: invalid
                i32 c = t->cp;
                b32 bad = c<t->least || (c>=0xd800 && c<=0xdfff) || c>0x10ffff;
                vtput(t, bad ? 0xfffd : c);
            }
            return;
        }
        t->need = 0;
        vtput(t, 0xfffd);  // a truncated sequence, then this byte
    }
    if (b >= 0x80) {
        if (b>=0xc2 && b<=0xdf) {
            t->cp    = b & 0x1f;
            t->need  = 1;
            t->least = 0x80;
        } else if (b>=0xe0 && b<=0xef) {
            t->cp    = b & 0x0f;
            t->need  = 2;
            t->least = 0x800;
        } else if (b>=0xf0 && b<=0xf4) {
            t->cp    = b & 0x07;
            t->need  = 3;
            t->least = 0x10000;
        } else {
            vtput(t, 0xfffd);
        }
        return;
    }
    switch (b) {
    case 0x07: t->bells++;                                   break;
    case 0x08:
        if (t->wrapnext) {
            t->wrapnext = 0;
            t->col = t->cols>1 ? t->cols-2 : 0;
        } else if (t->col) {
            t->col--;
        }
        break;
    case 0x09: {
        i32 next = (t->col/8 + 1) * 8;
        t->col = next<t->cols ? next : t->cols-1;
        t->wrapnext = 0;
    } break;
    case 0x0a: case 0x0b: case 0x0c: vtlinefeed(t);          break;
    case 0x0d: t->col = 0; t->wrapnext = 0;                  break;
    case 0x1b: t->state = VT_ESC;                            break;
    default:
        if (b >= 0x20 && b != 0x7f) vtput(t, b);
    }
}

static void vtfeed(VTerm *t, u8 *buf, iz len)
{
    for (iz i = 0; i < len; i++) vtbyte(t, buf[i]);
}

// A new size, keeping the content at the top left, as xterm does.
static void vtresize(VTerm *t, Arena *a, i32 rows, i32 cols)
{
    VCell *cells = alloc<VCell>(a, (iz)rows*cols);
    for (i32 r = 0; r<rows && r<t->rows; r++) {
        for (i32 c = 0; c<cols && c<t->cols; c++) {
            cells[(iz)r*cols + c] = *vtcell(t, r, c);
        }
        if (cols < t->cols && cells[(iz)r*cols + cols-1].c > 0
                && cpwidth(cells[(iz)r*cols + cols-1].c) == 2) {
            cells[(iz)r*cols + cols-1] = {};  // a cut wide character
        }
    }
    t->cells = cells;
    t->rows  = rows;
    t->cols  = cols;
    vtmoveto(t, t->row, t->col);
}

// The screen as text: each line up to the last non-blank one (or the
// cursor's) with trailing blanks dropped, a line of marks under one
// with attributes ('r' reverse, 'b' bold, 'u' underline, by priority),
// then the cursor as "@ ROW,COL" (0-based), a '+' after it while a
// wrap is pending, and the bell count if any.
static Str vtdump(VTerm *t, Arena *a)
{
    i32 last = t->row;
    for (i32 r = t->rows-1; r > last; r--) {
        for (i32 c = 0; c < t->cols; c++) {
            if (vtcell(t, r, c)->c) {
                last = r;
                break;
            }
        }
    }
    Buf b(a, 256);
    for (i32 r = 0; r <= last; r++) {
        VCell *line = vtcell(t, r, 0);
        i32 end   = t->cols;
        i32 aend  = t->cols;
        for (; end>0 && (line[end-1].c<=0 || line[end-1].c==' ') && !line[end-1].comb; end--) {}
        for (; aend>0 && !line[aend-1].attr; aend--) {}
        for (i32 c = 0; c < end; c++) {
            if (line[c].c == -1) continue;
            u8 tmp[8];
            Str s = {tmp, 0};
            s.len = utf8encode(tmp, line[c].c ? line[c].c : ' ');
            print(&b, s);
            if (line[c].comb) {
                s.len = utf8encode(tmp, line[c].comb);
                print(&b, s);
            }
        }
        print(&b, "\n");
        if (aend) {
            print(&b, "~");
            for (i32 c = 0; c < aend; c++) {
                u8 m = line[c].attr;
                print(&b, m&VA_REVERSE ? Str("r") : m&VA_BOLD ? Str("b") : m&VA_UNDER ? Str("u") : " ");
            }
            print(&b, "\n");
        }
    }
    print(&b, "@ ");
    print(&b, (i64)t->row);
    print(&b, ",");
    print(&b, (i64)t->col);
    if (t->wrapnext) print(&b, "+");
    if (t->bells) {
        print(&b, " bell ");
        print(&b, (i64)t->bells);
    }
    print(&b, "\n");
    return finish(&b);
}


// Editor test scripts (t/edit/*.ed): a session typed at a terminal,
// for the vos (main_test.cpp) and for bash on a pty (tools/ptyrun).
// One directive per line, blank lines and '#' comments ignored:
//   size ROWS COLS   the terminal's size (default 24 80)
//   nowrap           no deferred wrap (vos only: no recording)
//   env NAME=VALUE   added to the environment
//   rc LINE          a line of the rc file the shell starts with
//   keys TEXT        typed at once, with escapes: \r \n \t \e \a \b
//                    \\ \xHH \C-x (control) \M-x (ESC x) \s (space,
//                    for a trailing one)
//   pause MS         nothing typed for MS milliseconds
//   snap             a snapshot of the screen once the shell waits
//   resize ROWS COLS a new size, and SIGWINCH
// The snapshots (vtdump, each after "== snap N"), the last one at the
// end of the script, are what a session's .screens file holds.

enum : i32 {
    VKEY_TEXT,    // bytes typed at once
    VKEY_PAUSE,   // ms of time with nothing typed
    VKEY_SNAP,    // a snapshot once everything is waiting
    VKEY_RESIZE,  // rows by cols, then SIGWINCH
};

struct VKey {
    i32 kind;
    Str text;
    i64 ms;
    i32 rows;
    i32 cols;
};

struct EdScript {
    i32   rows;
    i32   cols;
    b32   nowrap;
    Str   rc;
    Str  *env;
    iz    nenv;
    VKey *keys;
    iz    nkeys;
    Str   err;   // the first error, or null data
};

static i32 edhex(u8 c)  // -1 if not a hex digit
{
    if (c>='0' && c<='9') return c - '0';
    c |= 0x20;
    return c>='a' && c<='f' ? c - 'a' + 10 : -1;
}

static Str edkeys(Arena *a, Str s)
{
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c!='\\' || i+1==s.len) {
            putbyte(&b, c);
            continue;
        }
        c = s[++i];
        switch (c) {
        case 'r': putbyte(&b, '\r');  break;
        case 'n': putbyte(&b, '\n');  break;
        case 't': putbyte(&b, '\t');  break;
        case 'e': putbyte(&b, 0x1b);  break;
        case 'a': putbyte(&b, 0x07);  break;
        case 'b': putbyte(&b, 0x08);  break;
        case 's': putbyte(&b, ' ');   break;
        case 'x':
            if (i+2<s.len && edhex(s[i+1])>=0 && edhex(s[i+2])>=0) {
                putbyte(&b, (u8)(edhex(s[i+1])<<4 | edhex(s[i+2])));
                i += 2;
            } else {
                putbyte(&b, 'x');
            }
            break;
        case 'C':
        case 'M':
            if (i+2<s.len && s[i+1]=='-') {
                u8 k = s[i+2];
                i += 2;
                if (c == 'M') {
                    putbyte(&b, 0x1b);
                    putbyte(&b, k);
                } else {
                    putbyte(&b, k=='?' ? 0x7f : (u8)(k & 0x1f));
                }
            } else {
                putbyte(&b, c);
            }
            break;
        default:
            putbyte(&b, c);
        }
    }
    return finish(&b);
}

static i32 edint(Str s)  // -1 if not a decimal
{
    if (!s.len || s.len>6) return -1;
    i32 n = 0;
    for (iz i = 0; i < s.len; i++) {
        if (s[i]<'0' || s[i]>'9') return -1;
        n = n*10 + (s[i] - '0');
    }
    return n;
}

static EdScript edparse(Arena *a, Str src)
{
    EdScript r = {};
    r.rows = 24;
    r.cols = 80;
    iz nlines = 1;
    for (iz i = 0; i < src.len; i++) nlines += src[i]=='\n';
    r.keys = alloc<VKey>(a, nlines);
    r.env  = alloc<Str>(a, nlines);
    Buf rc(a, src.len+1);
    for (iz i = 0; i < src.len;) {
        iz end = i;
        for (; end<src.len && src[end]!='\n'; end++) {}
        Str line = {src.data+i, end-i};
        i = end + 1;
        if (line.len && line[line.len-1]=='\r') line.len--;
        if (!line.len || line[0]=='#') continue;
        iz sp = 0;
        for (; sp<line.len && line[sp]!=' '; sp++) {}
        Str word = {line.data, sp};
        Str rest = sp<line.len ? Str{line.data+sp+1, line.len-sp-1} : Str{};
        // two numbers in rest, for size and resize
        iz sp2 = 0;
        for (; sp2<rest.len && rest[sp2]!=' '; sp2++) {}
        i32 n1 = edint({rest.data, sp2});
        i32 n2 = sp2<rest.len ? edint({rest.data+sp2+1, rest.len-sp2-1}) : -1;
        if (word == "size") {
            if (n1<2 || n2<2) return r.err = "bad size", r;
            r.rows = n1;
            r.cols = n2;
        } else if (word == "nowrap") {
            r.nowrap = 1;
        } else if (word == "env") {
            r.env[r.nenv++] = rest;
        } else if (word == "rc") {
            print(&rc, rest);
            print(&rc, "\n");
        } else if (word == "keys") {
            r.keys[r.nkeys].kind = VKEY_TEXT;
            r.keys[r.nkeys++].text = edkeys(a, rest);
        } else if (word == "pause") {
            if (n1 < 0) return r.err = "bad pause", r;
            r.keys[r.nkeys].kind = VKEY_PAUSE;
            r.keys[r.nkeys++].ms = n1;
        } else if (word == "snap") {
            r.keys[r.nkeys++].kind = VKEY_SNAP;
        } else if (word == "resize") {
            if (n1<2 || n2<2) return r.err = "bad resize", r;
            r.keys[r.nkeys].kind = VKEY_RESIZE;
            r.keys[r.nkeys].rows = n1;
            r.keys[r.nkeys++].cols = n2;
        } else {
            r.err = "unknown directive";
            return r;
        }
    }
    r.rc = finish(&rc);
    return r;
}
