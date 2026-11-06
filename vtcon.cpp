// vtcon: the editor's VT subset on a Windows console without VT
// processing (XP, 7, or VT refused), through the console primitives of
// os_con (notes/m6.md, step 8). Text and the controls the console
// processes itself (CR, LF, BS, BEL) pass through; the escapes become
// cursor moves, fills and attributes. The editor flushes whole
// buffers, so no UTF-8 sequence or escape is split between writes,
// though the parser state carries over anyway.
// This is free and unencumbered software released into the public domain.

enum : i32 { VC_GROUND, VC_ESC, VC_CSI, VC_OSC, VC_OSCESC };

struct VtCon {
    i32 state;
    i32 params[8];
    i32 nparams;
    u8  prefix;    // a CSI private marker, or 0
    b32 haveattr;
    i32 base;      // the attribute before any SGR
    i32 fg;        // ANSI colors 0-15, or -1 for base's
    i32 bg;
    b32 bold;
    b32 reverse;
};

static i32 vcclamp(i32 v, i32 lo, i32 hi)
{
    return v<lo ? lo : v>hi ? hi : v;
}

static void vcmove(Os *os, Handle h, i32 x, i32 y)
{
    ConOp m = {};
    m.op = CON_MOVE;
    m.x  = x;
    m.y  = y;
    os_con(os, h, &m);
}

static void vcfill(Os *os, Handle h, VtCon *v, i32 x, i32 y, iz n)
{
    if (n <= 0) return;
    ConOp f = {};
    f.op   = CON_FILL;
    f.x    = x;
    f.y    = y;
    f.n    = n;
    f.attr = v->base;
    os_con(os, h, &f);
}

// An ANSI color as the console's: red and blue trade places
static i32 vccolor(i32 c)
{
    return (c&8) | (c&1)<<2 | (c&2) | (c&4)>>2;
}

// SGR as busybox's console emulation does it: colors, bold as the
// foreground's intensity, reverse swapping the two; the rest ignored.
static void vcsgr(Os *os, Handle h, VtCon *v)
{
    if (!v->nparams) v->nparams = 1;  // params[0] is 0
    for (i32 i = 0; i < v->nparams; i++) {
        i32 p = v->params[i];
        if (p == 0) {
            v->fg = v->bg = -1;
            v->bold = v->reverse = 0;
        } else if (p == 1) {
            v->bold = 1;
        } else if (p == 22) {
            v->bold = 0;
        } else if (p == 7) {
            v->reverse = 1;
        } else if (p == 27) {
            v->reverse = 0;
        } else if (p>=30 && p<=37) {
            v->fg = p - 30;
        } else if (p == 39) {
            v->fg = -1;
        } else if (p>=40 && p<=47) {
            v->bg = p - 40;
        } else if (p == 49) {
            v->bg = -1;
        } else if (p>=90 && p<=97) {
            v->fg = p - 90 + 8;
        } else if (p>=100 && p<=107) {
            v->bg = p - 100 + 8;
        } else if (p==38 || p==48) {
            break;  // extended colors: the rest is theirs
        }
    }
    i32 f = v->fg<0 ? v->base&15      : vccolor(v->fg);
    i32 b = v->bg<0 ? v->base>>4 & 15 : vccolor(v->bg);
    if (v->bold) f |= 8;
    if (v->reverse) {
        i32 t = f;
        f = b;
        b = t;
    }
    i32 a = (v->base & 0xff00) | b<<4 | f;
    ConOp o = {};
    o.op   = CON_ATTR;
    o.attr = a;
    os_con(os, h, &o);
}

static void vccsi(Os *os, Handle h, VtCon *v, u8 final)
{
    if (v->prefix) return;  // private modes: bracketed paste, flash
    if (final == 'm') {
        vcsgr(os, h, v);
        return;
    }
    ConOp c = {};
    c.op = CON_INFO;
    if (!os_con(os, h, &c) || c.cols<1 || c.rows<1) return;
    i32 p0 = v->nparams>0 ? v->params[0] : 0;
    i32 p1 = v->nparams>1 ? v->params[1] : 0;
    i32 n  = p0 ? p0 : 1;
    i32 x  = vcclamp(c.x, 0, c.cols-1);
    i32 y  = vcclamp(c.y, 0, c.rows-1);
    switch (final) {
    case 'A': vcmove(os, h, x, vcclamp(y-n, 0, c.rows-1)); break;
    case 'B': vcmove(os, h, x, vcclamp(y+n, 0, c.rows-1)); break;
    case 'C': vcmove(os, h, vcclamp(x+n, 0, c.cols-1), y); break;
    case 'D': vcmove(os, h, vcclamp(x-n, 0, c.cols-1), y); break;
    case 'G': vcmove(os, h, vcclamp(n-1, 0, c.cols-1), y); break;
    case 'H':
        vcmove(os, h, vcclamp((p1 ? p1 : 1)-1, 0, c.cols-1),
                      vcclamp(n-1, 0, c.rows-1));
        break;
    case 'K':
        switch (p0) {
        case 0: vcfill(os, h, v, x, y, c.cols-x); break;
        case 1: vcfill(os, h, v, 0, y, x+1);      break;
        case 2: vcfill(os, h, v, 0, y, c.cols);   break;
        }
        break;
    case 'J':
        switch (p0) {
        case 0: vcfill(os, h, v, x, y, c.cols-x + (iz)(c.rows-1-y)*c.cols); break;
        case 1: vcfill(os, h, v, 0, 0, (iz)y*c.cols + x+1);                 break;
        case 2: vcfill(os, h, v, 0, 0, (iz)c.rows*c.cols);                  break;
        }
        break;
    }
}

static void vctext(Os *os, Handle h, Str s)
{
    if (!s.len) return;
    ConOp w = {};
    w.op   = CON_WRITE;
    w.text = s;
    os_con(os, h, &w);
}

// Draw s on the console h.
static void vtconwrite(Os *os, Handle h, VtCon *v, Str s)
{
    if (!v->haveattr) {
        ConOp c = {};
        c.op = CON_INFO;
        if (os_con(os, h, &c)) v->base = c.attr;
        v->haveattr = 1;
        v->fg = v->bg = -1;
    }
    iz run = 0;  // the start of text not yet written
    for (iz i = 0; i < s.len; i++) {
        u8 b = s[i];
        switch (v->state) {
        case VC_GROUND:
            if (b == 0x1b) {
                vctext(os, h, span(s.data+run, s.data+i));
                v->state = VC_ESC;
                run      = i + 1;
            }
            continue;
        case VC_ESC:
            if (b == '[') {
                v->state   = VC_CSI;
                v->nparams = 0;
                v->prefix  = 0;
                for (i32 j = 0; j < countof(v->params); j++) v->params[j] = 0;
            } else if (b == ']') {
                v->state = VC_OSC;
            } else {
                v->state = VC_GROUND;  // ESC 7 and the like: ignored
            }
            break;
        case VC_CSI:
            if (b>='0' && b<='9') {
                if (!v->nparams) v->nparams = 1;
                i32 *p = v->params + v->nparams - 1;
                if (*p < 100000) *p = *p*10 + b - '0';
            } else if (b == ';') {
                if (!v->nparams) v->nparams = 1;
                if (v->nparams < countof(v->params)) v->params[v->nparams++] = 0;
            } else if (b>='<' && b<='?') {
                v->prefix = b;
            } else if (b>=0x40 && b<=0x7e) {
                vccsi(os, h, v, b);
                v->state = VC_GROUND;
            }
            break;
        case VC_OSC:
            if (b == 0x07) v->state = VC_GROUND;
            if (b == 0x1b) v->state = VC_OSCESC;
            break;
        case VC_OSCESC:
            v->state = b=='\\' ? VC_GROUND : VC_OSC;
            break;
        }
        run = i + 1;
    }
    if (v->state == VC_GROUND) vctext(os, h, span(s.data+run, s.data+s.len));
}


// Console key events (KEY_EVENT_RECORD) as xterm bytes, for consoles
// without VT input; tested from tables, used by the Win32 layer.

struct ConKey {
    b32 down;
    i32 vk;     // virtual key code
    i32 ch;     // UTF-16 unit, or 0
    i32 state;  // control key state
};

enum : i32 {
    CK_RALT = 0x01, CK_LALT = 0x02, CK_RCTRL = 0x04, CK_LCTRL = 0x08,
    CK_SHIFT = 0x10,
};

// A UTF-16 unit as UTF-8 into out, a high surrogate held in *hi for
// the low one after it: the byte count.
static i32 conutf16(i32 ch, i32 *hi, u8 *out)
{
    if (ch>=0xd800 && ch<=0xdbff) {
        *hi = ch;
        return 0;
    }
    i32 c = ch;
    if (ch>=0xdc00 && ch<=0xdfff) {
        if (!*hi) return 0;
        c = 0x10000 + ((*hi - 0xd800)<<10) + (ch - 0xdc00);
    }
    *hi = 0;
    return (i32)utf8encode(out, c);
}

// With VT input the console's characters are already xterm's.
static i32 conchar(ConKey k, i32 *hi, u8 *out)
{
    return k.down && k.ch ? conutf16(k.ch, hi, out) : 0;
}

// The bytes xterm sends for a key, at most 16 into out.
static i32 conkey(ConKey k, i32 *hi, u8 *out)
{
    b32 altgr = (k.state & (CK_RALT|CK_LCTRL)) == (CK_RALT|CK_LCTRL);
    b32 alt   = !altgr && (k.state & (CK_RALT|CK_LALT));
    b32 ctrl  = !altgr && (k.state & (CK_RCTRL|CK_LCTRL));
    b32 shift = !!(k.state & CK_SHIFT);
    i32 n     = 0;

    if (!k.down) {
        // Alt+numpad: the character comes with Alt's release
        return k.vk==0x12 && k.ch ? conutf16(k.ch, hi, out) : 0;
    }

    // Cursor and editing keys, with xterm's modifier parameter
    i32 mod = 1 + shift + 2*alt + 4*ctrl;
    u8  fin = 0;
    i32 num = 0;
    switch (k.vk) {
    case 0x26: fin = 'A'; break;  // up
    case 0x28: fin = 'B'; break;  // down
    case 0x27: fin = 'C'; break;  // right
    case 0x25: fin = 'D'; break;  // left
    case 0x24: fin = 'H'; break;  // home
    case 0x23: fin = 'F'; break;  // end
    case 0x2d: num = 2;   break;  // insert
    case 0x2e: num = 3;   break;  // delete
    case 0x21: num = 5;   break;  // page up
    case 0x22: num = 6;   break;  // page down
    case 0x74: num = 15;  break;  // F5..F12
    case 0x75: num = 17;  break;
    case 0x76: num = 18;  break;
    case 0x77: num = 19;  break;
    case 0x78: num = 20;  break;
    case 0x79: num = 21;  break;
    case 0x7a: num = 23;  break;
    case 0x7b: num = 24;  break;
    }
    if (k.vk>=0x70 && k.vk<=0x73) fin = (u8)('P' + k.vk - 0x70);  // F1..F4
    if (fin || num) {
        out[n++] = 0x1b;
        if (fin>='P' && mod==1) {
            out[n++] = 'O';
            out[n++] = fin;
            return n;
        }
        out[n++] = '[';
        if (num) {
            if (num >= 10) out[n++] = (u8)('0' + num/10);
            out[n++] = (u8)('0' + num%10);
        } else if (mod > 1) {
            out[n++] = '1';
        }
        if (mod > 1) {
            out[n++] = ';';
            out[n++] = (u8)('0' + mod);
        }
        out[n++] = num ? '~' : fin;
        return n;
    }

    if (alt) out[n++] = 0x1b;
    switch (k.vk) {
    case 0x08:  // backspace
        out[n++] = ctrl ? 0x08 : 0x7f;
        return n;
    case 0x09:  // tab
        if (shift) {
            out[n++] = 0x1b;
            out[n++] = '[';
            out[n++] = 'Z';
        } else {
            out[n++] = '\t';
        }
        return n;
    case 0x0d:  // enter
        out[n++] = '\r';
        return n;
    case 0x20:  // space
        out[n++] = ctrl ? 0 : ' ';
        return n;
    }
    if (k.ch) {
        i32 m = conutf16(k.ch, hi, out+n);
        return m ? n+m : 0;
    }
    if (ctrl) {
        // Control characters the console gives no character for
        switch (k.vk) {
        case '2':  out[n++] = 0x00; return n;  // C-@
        case '6':  out[n++] = 0x1e; return n;  // C-^
        case 0xbd: out[n++] = 0x1f; return n;  // C-_ (the minus key)
        }
    }
    return 0;  // a modifier alone, or a key with nothing to send
}
