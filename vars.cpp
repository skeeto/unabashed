// Variables, functions and the environment
//
// A variable is a node in a hash trie in the store, holding a stack of
// bindings: global at the bottom, then one per function that declared
// it local, and temporary bindings from "NAME=value cmd" on top. Lookup
// is a single trie descent whatever the call depth. Bindings are copied
// before modification unless the current subshell level owns them, so
// that the journal only ever records pointer writes.
//
// This is free and unencumbered software released into the public domain.

enum : u32 {  // variable attributes
    V_EXPORT   = 1<<0,
    V_READONLY = 1<<1,
    V_INTEGER  = 1<<2,
    V_LOWER    = 1<<3,
    V_UPPER    = 1<<4,
    V_CAPITAL  = 1<<5,
    V_NAMEREF  = 1<<6,
    V_ARRAY    = 1<<7,
    V_ASSOC    = 1<<8,
    V_TRACE    = 1<<9,
    V_LOCAL    = 1<<10,
    V_TEMP     = 1<<11,  // NAME=value before a builtin or function
    V_NOASSIGN = 1<<12,  // assignments silently do nothing (GROUPS)
};

enum : i32 {  // dynamic variables, computed when read
    DYN_NONE, DYN_LINENO, DYN_RANDOM, DYN_SRANDOM, DYN_SECONDS,
    DYN_EPOCHSECONDS, DYN_EPOCHREALTIME, DYN_BASHPID, DYN_BASH_SUBSHELL,
    DYN_FUNCNAME, DYN_BASH_COMMAND, DYN_HISTCMD, DYN_BASH_LINENO,
    DYN_BASH_SOURCE, DYN_BASH_ARGV0, DYN_PIPESTATUS, DYN_UNDERSCORE,
    DYN_BASH_ARGV, DYN_BASH_ARGC, DYN_DIRSTACK, DYN_BASH_ALIASES,
    DYN_BASH_CMDS,
};

struct Value {
    Value *below;    // the binding this one shadows
    i32    depth;    // function depth of the binding, 0 for global
    u32    attrs;
    Str    str;      // null data: declared without a value
    Array *arr;      // V_ARRAY or V_ASSOC: the elements
    i32    dyn;
    i64    base;     // DYN_SECONDS: assigned value's offset
};

struct Var {
    Var   *child[4];
    Str    name;
    Value *top;
};

// A function body, deep-copied into one store block of its own so that
// redefinition can free it. A body still running, or still in the
// function table of another vproc, is freed when its last reference
// goes.
struct Code {
    Store *store;    // the vproc store holding it
    i32   refs;      // calls in progress, and other vprocs' tables
    b32   dead;      // replaced or unset in its own store's table
    iz    size;      // of the block, for sfree
    Node *body;
    Str   file;      // where it was defined, for BASH_SOURCE
};

struct Func {
    Func *child[4];
    Str   name;
    Node *body;      // null once unset
    Code *code;
    uz    attrs;     // V_EXPORT V_READONLY V_TRACE, pointer-sized for jwrite
};

static Arith evalarith(Shell *, Arena *, Str);
static void  keepcmdtext(Shell *, byte *beg, byte *end);
static Arith evalinteger(Shell *, Arena *, Str);

static b32 validvarname(Str s)
{
    if (!s.len || digit(s[0])) return 0;
    for (iz i = 0; i < s.len; i++) {
        if (!alnum(s[i]) && s[i]!='_') return 0;
    }
    return 1;
}

static Var *findvar(Shell *sh, Str name)
{
    Var *v = sh->vars;
    for (u64 h = hash(name); v; h <<= 2) {
        if (v->name == name) return v;
        v = v->child[h>>62];
    }
    return 0;
}

static Var *upsertvar(Shell *sh, Str name)
{
    Store *st   = sh->store;
    Var  **slot = &sh->vars;
    for (u64 h = hash(name); *slot; h <<= 2) {
        if ((*slot)->name == name) return *slot;
        slot = &(*slot)->child[h>>62];
    }
    Var *v = (Var *)salloc(st, sizeof(Var));
    v->name = sclone(st, name);
    if (slot == &sh->vars) {
        *slot = v;  // the root lives in the Shell struct, not the store
    } else {
        jset(st, slot, v);
    }
    return v;
}

// The visible binding of a variable itself, or null, even a nameref.
static Value *rawlookup(Shell *sh, Str name)
{
    Var *v = findvar(sh, name);
    return v ? v->top : 0;
}


// Namerefs
//
// A nameref's value names another variable, or an array element, which
// stands in for it wherever the shell reads, assigns or unsets it by
// name. Chains are followed to a depth of 8.

enum { NAMEREF_MAX = 8 };

struct Ref {
    Str    name;     // the variable named at the end, null if none
    Str    sub;      // the element named, when hassub
    b32    hassub;
    Str    lastname; // the last nameref followed, if any
    Value *last;
    b32    loop;     // circular or too deep
    b32    global;   // circular inside a function: the global binding
};

static b32 splitref(Str s, Str *name, Str *sub);

static void refwarning(Shell *sh, Str name, Str msg)
{
    byte  mem[1024];
    Arena a = {mem, mem+sizeof(mem)};
    if (name.len > 256) name = takehead(name, 256);
    shellerror(sh, a, concat3(&a, "warning: ", name, msg));
}

// Follow the namerefs from name, as bash does: a chain may not come
// back to where it started or name itself, and is at most 8 long.
static Value *globalvalue(Shell *, Str name);

// With global, only global bindings take part, as for declare -g.
static Ref resolveref(Shell *sh, Str name, b32 global = 0)
{
    Ref    r = {};
    Value *v = global ? globalvalue(sh, name) : rawlookup(sh, name);
    r.name = name;
    for (i32 level = 0; v && (v->attrs & V_NAMEREF); level++) {
        if (level == NAMEREF_MAX) {
            refwarning(sh, name, ": maximum nameref depth (8) exceeded");
            r.name = {};
            r.loop = 1;
            return r;
        }
        Str t = v->str;
        r.lastname = r.name;
        r.last     = v;
        if (!t.len) {
            r.name = {};
            return r;
        }
        Str rn, rs;
        if (!validvarname(t) && splitref(t, &rn, &rs)) {
            r.name   = rn;
            r.sub    = rs;
            r.hassub = 1;
            return r;
        }
        Value *n = global ? globalvalue(sh, t) : rawlookup(sh, t);
        if (n && (t==name || t==r.name)) {
            refwarning(sh, name, ": circular name reference");
            r.loop = 1;
            if (sh->funcdepth && n->depth) {
                r.name   = t;
                r.global = 1;
            } else {
                r.name = {};
            }
            return r;
        }
        r.name = t;
        v = n;
    }
    return r;
}

// The binding at the bottom of a variable's stack (the global one).
static Value *globalvalue(Shell *sh, Str name)
{
    Value *v = rawlookup(sh, name);
    while (v && v->depth) v = v->below;
    return v;
}

// The binding of name made at function depth d (0 for global), or null.
static Value *bindingat(Shell *sh, Str name, i32 d)
{
    Value *v = rawlookup(sh, name);
    for (; v && (v->depth>d || (v->attrs & V_TEMP)); v = v->below) {}
    return v && v->depth==d ? v : 0;
}

// Is any binding in this stack a nameref?
static b32 anyref(Value *v)
{
    for (; v; v = v->below) {
        if (v->attrs & V_NAMEREF) return 1;
    }
    return 0;
}

// Where an assignment to name goes, as bash's bind_variable decides.
// A nameref found in a function's scope is followed scope by scope
// toward the global one, so that a chain that loops within a scope
// exceeds the maximum depth instead of being circular, and then the
// global variable of that name is assigned. A nameref without a value
// is itself assigned: the value names its target.
struct Bind {
    Str name;
    Str sub;
    b32 hassub;
    b32 self;      // assign name itself, a nameref
    b32 global;    // the global binding of name
    b32 fail;
    b32 local;     // decided in a function's scope
};

static Bind assignref(Shell *sh, Str name)
{
    Bind   b = {};
    Value *v = rawlookup(sh, name);
    b.name = name;
    for (; v && (v->attrs & V_TEMP); v = v->below) {}
    if (v && v->depth) {
        // found in the scope of function d
        i32 d = v->depth;
        b.local = 1;
        if (!(v->attrs & V_NAMEREF)) return b;
        Value *nv = v;
        Str    nn = name;
        for (i32 c = d; c >= 0; c--) {
            for (i32 level = 1; nv->attrs & V_NAMEREF;) {
                if (++level > NAMEREF_MAX) {
                    refwarning(sh, name, ": maximum nameref depth (8) exceeded");
                    b.global = 1;
                    return b;
                }
                Value *n = nv->str.len ? bindingat(sh, nv->str, c) : 0;
                if (!n) break;
                nn = nv->str;
                nv = n;
            }
            if (!(nv->attrs & V_NAMEREF)) {
                b.name   = nn;
                b.global = !c;
                return b;
            }
        }
        // The last nameref names nothing that exists
        Str t = nv->str;
        Str rn, rs;
        if (!t.len) {
            b.name = nn;
            b.self = 1;
        } else if (!validvarname(t) && splitref(t, &rn, &rs)) {
            b.name   = rn;
            b.sub    = rs;
            b.hassub = 1;
        } else {
            b.name   = t;
            b.global = 1;
        }
        return b;
    }
    Value *g = rawlookup(sh, name);
    if (!g || !(g->attrs & V_NAMEREF)) return b;
    if (!g->str.len) {
        b.self = 1;
        return b;
    }
    Ref r = resolveref(sh, name);
    if (r.hassub) {
        b.name   = r.name;
        b.sub    = r.sub;
        b.hassub = 1;
    } else if (r.name.data) {
        b.name   = r.name;
        b.global = r.global;
    } else {
        b.fail = 1;
    }
    return b;
}

// The visible binding of a variable, after namerefs, or null.
static Value *lookup(Shell *sh, Str name)
{
    Value *v = rawlookup(sh, name);
    if (!v || !(v->attrs & V_NAMEREF)) return v;
    Ref r = resolveref(sh, name);
    if (!r.name.data || r.hassub) return 0;
    return r.global ? globalvalue(sh, r.name) : rawlookup(sh, r.name);
}

// A binding the current subshell level may modify in place. The slot is
// the pointer that refers to it, which is updated if a copy is made.
static Value *ownvalue(Shell *sh, Value **slot)
{
    Store *st = sh->store;
    Value *v  = *slot;
    if ((byte *)v >= st->mark) {
        return v;
    }
    Value *c = (Value *)salloc(st, sizeof(Value));
    *c = *v;
    c->str = sclone(st, v->str);
    if (v->arr) c->arr = acopy(st, v->arr, 1);
    jset(st, slot, c);
    return c;
}

static void freevalue(Shell *sh, Value *v)
{
    Store *st = sh->store;
    if ((byte *)v >= st->mark) {
        sfreestr(st, v->str);
        afree(st, v->arr);
        sfree(st, v, sizeof(Value));
    }
}

static i64 dynnow(Shell *sh)
{
    return os_now(sh->os);
}

// Bash's RANDOM: the Park-Miller "minimal standard" generator, with
// the low bits of each result mixed into the 15-bit value.
static i32 nextrandom(Shell *sh)
{
    i64 h = sh->seed ? sh->seed : 0x123459876;
    i64 l = h % 127773;
    i64 t = 16807*l - 2836*(h/127773);
    if (t < 0) t += 0x7fffffff;
    sh->seed = (u32)t;
    return (i32)((t>>16 ^ (t & 0xffff)) & 0x7fff);
}

static Str fmti64(Arena *a, i64 v)
{
    Buf b(a, 24);
    print(&b, v);
    return finish(&b);
}

// The simple command being run, or the last one, as BASH_COMMAND.
static Str cmdtext(Shell *sh, Arena *a)
{
    if (!sh->cmdnode) return sh->cmdtext.data ? sh->cmdtext : Str("");
    Node n = *sh->cmdnode;
    n.flags &= ~(CMD_INVERT|CMD_TIME|CMD_TIMEPOSIX);
    Printer *p = newprinter(a);
    printcmd(p, &n);
    return finish(&p->b);
}

// $0, which BASH_ARGV0 may have replaced, unlike the name in messages.
static Str dollar0(Shell *sh)
{
    return sh->argv0.data ? sh->argv0 : sh->name;
}

static Array *dynarray(Shell *, Arena *, Value *);

static Str dynvalue(Shell *sh, Arena *a, Value *v)
{
    switch (v->dyn) {
    case DYN_LINENO:        return fmti64(a, sh->line);
    case DYN_RANDOM:        return fmti64(a, nextrandom(sh));
    case DYN_SRANDOM: {
        u64 x = (u64)dynnow(sh) * 0x9e3779b97f4a7c15u;
        return fmti64(a, (i64)(u32)(x >> 32));
    }
    case DYN_SECONDS:
        return fmti64(a, (dynnow(sh) - sh->start)/1000000000 + v->base);
    case DYN_EPOCHSECONDS:  return fmti64(a, dynnow(sh)/1000000000);
    case DYN_EPOCHREALTIME: {
        i64 t = dynnow(sh);
        Buf b(a, 32);
        print(&b, t/1000000000);
        putbyte(&b, '.');
        printpad(&b, t/1000%1000000, 6);
        return finish(&b);
    }
    case DYN_BASHPID:       return fmti64(a, sh->bashpid);
    case DYN_BASH_SUBSHELL: return fmti64(a, sh->subshell - sh->simplestages);
    case DYN_FUNCNAME:
    case DYN_BASH_LINENO:
    case DYN_BASH_SOURCE:
    case DYN_BASH_ARGV:
    case DYN_BASH_ARGC: {
        Array *r = dynarray(sh, a, v);
        return r->len ? r->e[0].val : Str{};
    }
    case DYN_BASH_COMMAND:  return cmdtext(sh, a);
    case DYN_HISTCMD:       return "1";
    case DYN_BASH_ARGV0:    return dollar0(sh);
    case DYN_PIPESTATUS:    return fmti64(a, sh->npstat ? sh->pstat[0] : 0);
    case DYN_UNDERSCORE:    return sh->lastarg.data ? sh->lastarg : Str("");
    }
    return v->str;
}

struct Got {
    Str value;
    b32 set;
};

static Got getelem(Shell *, Arena *, Str name, Str sub);  // expand.cpp

// A variable's value as a string, computing dynamic ones.
static Got getvalue(Shell *, Arena *, Value *);

static Got getvar(Shell *sh, Arena *a, Str name)
{
    Value *v = rawlookup(sh, name);
    if (v && (v->attrs & V_NAMEREF)) {
        Ref ref = resolveref(sh, name);
        if (ref.hassub) return getelem(sh, a, ref.name, ref.sub);
        v = !ref.name.data ? 0
          : ref.global     ? globalvalue(sh, ref.name)
          :                  rawlookup(sh, ref.name);
    }
    return getvalue(sh, a, v);
}

// A binding's value as a string, null when unset.
static Got getvalue(Shell *sh, Arena *a, Value *v)
{
    Got r = {};
    if (!v) return r;
    if (v->dyn) {
        r.value = dynvalue(sh, a, v);
        r.set   = 1;
        if (v->dyn==DYN_FUNCNAME || v->dyn==DYN_BASH_LINENO ||
                v->dyn==DYN_BASH_SOURCE || v->dyn==DYN_BASH_ARGV) {
            r.set = !!r.value.data;
        }
        return r;
    }
    if (v->arr) {
        // $a is ${a[0]}
        Str *e = 0;
        if (v->arr->bucket) {
            HItem *t = hfind(v->arr, "0");
            e = t ? &t->val : 0;
        } else {
            e = aget(v->arr, 0);
        }
        if (e) {
            r.value = *e;
            r.set   = 1;
        }
        return r;
    }
    r.value = v->str;
    r.set   = !!v->str.data;
    return r;
}

// Apply the case-conversion attributes to a value.
// Attribute letters in declare's order, as ${x@a} shows them.
static Str attrletters(Arena *a, u32 attrs)
{
    Buf b(a, 12);
    if (attrs & V_ARRAY)    putbyte(&b, 'a');
    if (attrs & V_ASSOC)    putbyte(&b, 'A');
    if (attrs & V_INTEGER)  putbyte(&b, 'i');
    if (attrs & V_NAMEREF)  putbyte(&b, 'n');
    if (attrs & V_READONLY) putbyte(&b, 'r');
    if (attrs & V_TRACE)    putbyte(&b, 't');
    if (attrs & V_EXPORT)   putbyte(&b, 'x');
    if (attrs & V_CAPITAL)  putbyte(&b, 'c');
    if (attrs & V_LOWER)    putbyte(&b, 'l');
    if (attrs & V_UPPER)    putbyte(&b, 'u');
    return finish(&b);
}

static Str convertcase(Arena *a, Str s, u32 attrs)
{
    if (!(attrs & (V_LOWER|V_UPPER|V_CAPITAL)) || !s.len) return s;
    Str r = clone(a, s);
    for (iz i = 0; i < r.len; i++) {
        u8 c = r[i];
        if ((attrs&V_UPPER) || ((attrs&V_CAPITAL) && i==0)) {
            if (c>='a' && c<='z') r[i] = (u8)(c - 32);
        } else if (c>='A' && c<='Z') {
            r[i] = (u8)(c + 32);
        }
    }
    return r;
}

enum : i32 {  // setvar flags
    SV_APPEND = 1<<0,  // +=
    SV_GLOBAL = 1<<1,  // the bottom binding (declare -g)
    SV_QUIET  = 1<<2,  // no readonly message
    SV_FORCE  = 1<<3,  // ignore readonly (internal updates)
    SV_UNDERTEMP = 1<<4,  // the binding below any temporary ones
    SV_NOREF  = 1<<5,  // name itself, even a nameref
    SV_RAWKEY = 1<<6,  // an associative subscript used as it is
};

static void varchanged(Shell *, Str name);

// The binding an assignment to name modifies, owned by the current
// subshell level and created unset if needed. Reports and returns null
// for a readonly variable.
static Value *assignslot(Shell *sh, Arena scratch, Str name, i32 flags)
{
    Store *st = sh->store;
    Var   *v  = upsertvar(sh, name);
    Value **slot = &v->top;
    if (flags & SV_GLOBAL) {
        while (*slot && (*slot)->depth) slot = &(*slot)->below;
    }
    if (flags & SV_UNDERTEMP) {
        while (*slot && ((*slot)->attrs & V_TEMP)) slot = &(*slot)->below;
    }
    if (*slot && ((*slot)->attrs & V_READONLY) && !(flags & SV_FORCE)) {
        if (!(flags & SV_QUIET)) {
            shellerror(sh, scratch, concat(&scratch, name, ": readonly variable"));
        }
        return 0;
    }
    if (!*slot) {
        Value *n = (Value *)salloc(st, sizeof(Value));
        jset(st, slot, n);
    }
    return ownvalue(sh, slot);
}

// The string an assignment stores, given the binding's attributes and
// the old value: integer evaluation, += and case conversion.
static Str assignedvalue(Shell *sh, Arena *scratch, u32 attrs, Str old, Str value, i32 flags)
{
    if (attrs & V_INTEGER) {
        Arena a = *scratch;
        Arith r = evalinteger(sh, &a, value);
        i64 n = r.value;
        if (flags & SV_APPEND) {
            Arith o = evalinteger(sh, &a, old.data ? old : Str("0"));
            n = (i64)((u64)n + (u64)(o.ok ? o.value : 0));
        }
        value = fmti64(scratch, n);
    } else if (flags & SV_APPEND) {
        value = concat(scratch, old, value);
    }
    return convertcase(scratch, value, attrs);
}

static void assigned(Shell *sh, Value *val, Str name)
{
    if (sh->opt[O_ALLEXPORT]) {
        val->attrs |= V_EXPORT;
    }
    if (val->attrs & V_EXPORT) {
        sh->exportgen++;
    }
    varchanged(sh, name);
}

// Set the bottom of BASH_ARGV from the positional parameters, once:
// when first read by name, or when extdebug is turned on, as bash.
static void initbashargv(Shell *sh)
{
    if (sh->argvinit) return;
    Store *st = sh->store;
    iz     n  = sh->args.n;
    sh->argvbase.v = (Str *)salloc(st, (n ? n : 1)*(iz)sizeof(Str));
    sh->argvbase.n = n;
    for (iz i = 0; i < n; i++) sh->argvbase.v[i] = sclone(st, sh->args.v[i]);
    sh->argvinit = 1;
}

// BASH_ARGV (each frame's arguments, last first) and BASH_ARGC (their
// counts), innermost first: calls made under extdebug, sourced files
// (their arguments under extdebug, else the file's name when given
// none), then the bottom.
static Array *argvarray(Shell *sh, Arena *a, i32 d)
{
    if (!sh->argvinit && sh->listing) {
        Array *r = alloc<Array>(a);  // a listing does not set it up
        r->isset = 1;
        return r;
    }
    initbashargv(sh);
    iz n = 1;
    for (Frame *f = sh->frame; f; f = f->up) n += f->argpushed ? f->args.n+1 : 0;
    for (Srcframe *s = sh->srcframes; s; s = s->up) n += s->argpushed ? s->args.n+1 : 0;
    n += sh->argvbase.n;
    Array *r = alloc<Array>(a);
    r->cap   = n;
    r->e     = alloc<Elem>(a, n);
    r->isset = 1;
    Frame    *f = sh->frame;
    Srcframe *s = sh->srcframes;
    for (;;) {
        Args args   = sh->argvbase;
        b32  pushed = 1;
        b32  last   = 0;
        if (s && s->frame==f) {
            args   = s->args;
            pushed = s->argpushed;
            s      = s->up;
        } else if (f) {
            args   = f->args;
            pushed = f->argpushed;
            f      = f->up;
        } else {
            last = 1;
        }
        if (pushed && d==DYN_BASH_ARGC) {
            r->e[r->len].idx = r->len;
            r->e[r->len].val = fmti64(a, args.n);
            r->len++;
        } else if (pushed) {
            for (iz i = args.n-1; i >= 0; i--) {
                r->e[r->len].idx = r->len;
                r->e[r->len].val = args.v[i];
                r->len++;
            }
        }
        if (last) return r;
    }
}

enum { NALIASBUCKETS = 64 };  // bash's alias table

// Define an alias, or give it a new value.
static void defalias(Shell *sh, Str name, Str value)
{
    Store  *st   = sh->store;
    Alias **slot = &sh->aliases;
    for (u64 h = hash(name); *slot; h <<= 2) {
        if ((*slot)->name == name) break;
        slot = &(*slot)->child[h>>62];
    }
    if (!*slot) {
        Alias *a = (Alias *)salloc(st, sizeof(Alias));
        a->name = sclone(st, name);
        if (slot == &sh->aliases) *slot = a; else jset(st, slot, a);
    }
    Alias *a = *slot;
    if (!a->value.data) jwrite(st, &a->seq, ++sh->aliasseq);  // new again
    Str v = sclone(st, value);
    if (!v.data) v.data = (u8 *)"";
    jwrite(st, &a->value.data, (uz)v.data);
    jwrite(st, &a->value.len, (uz)v.len);
}

static void undefalias(Shell *sh, Alias *a)
{
    jwrite(sh->store, &a->value.data, 0);
    jwrite(sh->store, &a->value.len, 0);
}

// The defined aliases, oldest first.
static Slice<Alias *> aliasesbyage(Shell *sh, Arena *a)
{
    Slice<Alias *> all = {};
    Alias *stack[256];
    i32 top = 0;
    if (sh->aliases) stack[top++] = sh->aliases;
    while (top) {
        Alias *t = stack[--top];
        if (t->value.data) all = push(a, all, t);
        for (i32 k = 0; k < 4 && top < 252; k++) {
            if (t->child[k]) stack[top++] = t->child[k];
        }
    }
    for (iz k = 1; k < all.len; k++) {
        for (iz j = k; j>0 && all[j]->seq<all[j-1]->seq; j--) {
            Alias *t = all[j]; all[j] = all[j-1]; all[j-1] = t;
        }
    }
    return all;
}

// DIRSTACK, BASH_ALIASES and BASH_CMDS, built as bash builds them when
// read: the directory stack (the working directory alone, until pushd
// exists), and the alias and command tables walked in bash's bucket
// order into associative arrays of the same size. A listing shows them
// empty, as bash shows the values it built when created.
static Array *tablearray(Shell *sh, Arena *a, i32 d)
{
    Store st = {};
    st.a = *a;  // a scratch store, discarded with the scratch arena
    Array *r = 0;
    if (d == DYN_DIRSTACK) {
        r = newarray(&st);
        Value *pwd = rawlookup(sh, "PWD");
        if (!sh->listing) aput(&st, r, 0, pwd && pwd->str.data ? pwd->str : sh->cwd);
    } else if (d == DYN_BASH_CMDS) {
        r = newassoc(&st, NHASHCMD);
        for (iz b = 0; sh->cmdhash && !sh->listing && b < NHASHCMD; b++) {
            for (HashCmd *h = sh->cmdhash[b]; h; h = h->next) {
                hput(&st, r, h->name, h->path);
            }
        }
    } else {
        r = newassoc(&st, NALIASBUCKETS);
        Slice<Alias *> all = sh->listing ? Slice<Alias *>{} : aliasesbyage(sh, &st.a);
        // bash's own table, 64 buckets with the newest first in each,
        // is walked into the array, which reverses each chain again
        for (iz b = 0; b < NALIASBUCKETS; b++) {
            for (iz k = all.len-1; k >= 0; k--) {
                if ((hashkey(all[k]->name) & (NALIASBUCKETS-1)) == (u32)b) {
                    hput(&st, r, all[k]->name, all[k]->value);
                }
            }
        }
    }
    *a = st.a;
    r->isset = 1;
    return r;
}

// The elements of FUNCNAME and BASH_LINENO, built in scratch from the
// call frames, with "main" at the bottom when running a script. Null
// for other variables.
static Array *dynarray(Shell *sh, Arena *a, Value *v)
{
    i32 d = v ? v->dyn : 0;
    if (d == DYN_PIPESTATUS) {
        Array *r = alloc<Array>(a);
        r->cap   = sh->npstat;
        r->e     = alloc<Elem>(a, r->cap);
        for (; r->len < sh->npstat; r->len++) {
            r->e[r->len].idx = r->len;
            r->e[r->len].val = fmti64(a, sh->pstat[r->len]);
        }
        r->isset = 1;
        return r;
    }
    if (d==DYN_BASH_ARGV || d==DYN_BASH_ARGC) return argvarray(sh, a, d);
    if (d==DYN_DIRSTACK || d==DYN_BASH_ALIASES || d==DYN_BASH_CMDS) {
        return tablearray(sh, a, d);
    }
    if (d!=DYN_FUNCNAME && d!=DYN_BASH_LINENO && d!=DYN_BASH_SOURCE) return 0;
    iz n = 0, nsrc = 0;
    for (Frame *f = sh->frame; f; f = f->up) n++;
    for (Srcframe *s = sh->srcframes; s; s = s->up) nsrc++;
    // FUNCNAME exists only inside functions, then with the sourced
    // files among them ("source")
    nsrc = n || d!=DYN_FUNCNAME ? nsrc : 0;
    b32 main = !sh->cflag && !sh->interactive && (n || d!=DYN_FUNCNAME);
    Array *r = alloc<Array>(a);
    r->cap = n + nsrc + main;
    r->e   = alloc<Elem>(a, r->cap);
    Frame    *f = sh->frame;
    Srcframe *s = nsrc ? sh->srcframes : 0;
    while (f || s) {
        r->e[r->len].idx = r->len;
        if (s && s->frame==f) {
            // started in this call (or outside any): the newer
            r->e[r->len].val = d==DYN_FUNCNAME   ? Str("source")
                             : d==DYN_BASH_LINENO ? fmti64(a, s->line)
                             :                      s->file;
            s = s->up;
        } else {
            r->e[r->len].val = d==DYN_FUNCNAME   ? f->name
                             : d==DYN_BASH_LINENO ? fmti64(a, f->line)
                             : f->file.len        ? f->file
                             :                      sh->name;
            f = f->up;
        }
        r->len++;
    }
    if (main) {
        r->e[r->len].idx = r->len;
        r->e[r->len].val = d==DYN_FUNCNAME   ? Str("main")
                         : d==DYN_BASH_LINENO ? Str("0")
                         :                      sh->name;
        r->len++;
    }
    r->isset = r->len || d!=DYN_FUNCNAME;
    return r;
}

// A binding's elements: its array, or a dynamic array built in scratch.
static Array *varray(Shell *sh, Arena *a, Value *v)
{
    return !v ? 0 : v->arr ? v->arr : dynarray(sh, a, v);
}

// PIPESTATUS: bash sets it after every pipeline, simple command, (( )),
// [[ ]] and subshell, but not after other compound commands. The
// statuses live in the Shell, so that setting it costs a few stores,
// and the variable is a dynamic array over them. An assignment or unset
// makes it an ordinary variable until the command ends, when bash
// recreates it.
static void restorepstat(Shell *sh)
{
    Store *st = sh->store;
    Var   *v  = upsertvar(sh, "PIPESTATUS");
    sh->pstatdirty = 0;
    if (v->top && v->top->dyn==DYN_PIPESTATUS && !v->top->arr) return;
    while (v->top) {
        Value *t = v->top;
        jset(st, &v->top, t->below);
        freevalue(sh, t);
    }
    Value *n = (Value *)salloc(st, sizeof(Value));
    n->dyn   = DYN_PIPESTATUS;
    n->attrs = V_ARRAY;
    jset(st, &v->top, n);
}

static void setpstat1(Shell *sh)
{
    if (sh->pstatdirty) restorepstat(sh);
    sh->npstat   = 1;
    sh->pstat[0] = (u8)sh->status;
}

static void setpstat(Shell *sh, i32 *statuses, i32 n)
{
    if (sh->pstatdirty) restorepstat(sh);
    sh->npstat = n;
    for (i32 i = 0; i < n; i++) sh->pstat[i] = (u8)statuses[i];
}

static b32 assignelem(Shell *, Arena, Str name, Str sub, Str lhs, Str value, i32 flags);
static void hashenter(Shell *, Str name, Str path, uz hits);  // exec.cpp
static Value *setattrs(Shell *, Str name, u32 on, u32 off);

// Can a nameref name this: a variable or an array element?
static b32 validnameref(Str s)
{
    Str rn, rs;
    return validvarname(s) || splitref(s, &rn, &rs);
}

// The variable an array assignment to name goes to, after namerefs, or
// null after an error. A nameref without a value, given a whole list,
// stops being a nameref; given an element, it is an error, as is a
// list for a nameref to an element.
static Str arraytarget(Shell *sh, Arena scratch, Str name, i32 *flags, b32 list = 0)
{
    Value *v = rawlookup(sh, name);
    if (!v || !(v->attrs & V_NAMEREF)) return name;
    Ref r = resolveref(sh, name);
    if (r.hassub) {
        Str msg = concat3(&scratch, "`", r.last->str, "': not a valid identifier");
        builtinerror(sh, scratch, sh->bltname, msg);
        return {};
    } else if (!r.name.data && r.last && !r.last->str.len && list) {
        shellerror(sh, scratch, concat3(&scratch, "warning: ", name, ": removing nameref attribute"));
        Value *nv = setattrs(sh, name, 0, V_NAMEREF);
        nv->str = {};
        return name;
    } else if (!r.name.data && r.last && !r.last->str.len) {
        builtinerror(sh, scratch, sh->bltname, "`': not a valid identifier");
        return {};
    } else if (!r.name.data) {
        return name;
    }
    if (r.global) *flags |= SV_GLOBAL;
    *flags |= SV_NOREF;
    return r.name;
}

// Make an owned binding an indexed array, its value becoming element 0.
static void toarray(Shell *sh, Value *val)
{
    if (val->arr) return;
    Store *st = sh->store;
    val->arr = newarray(st);
    if (val->str.data) {
        aput(st, val->arr, 0, val->str);
        val->arr->isset = 1;
    }
    val->str    = {};
    val->dyn    = 0;
    val->attrs |= V_ARRAY;
}

// Make an owned binding associative, its value becoming key "0". A new
// variable gets bash's 1024 buckets, a converted one 128.
static void toassoc(Shell *sh, Value *val, b32 fresh)
{
    if (val->arr) return;
    Store *st = sh->store;
    val->arr = newassoc(st, fresh ? ASSOC_BUCKETS : ASSOC_CONVERTED);
    if (val->str.data) {
        hput(st, val->arr, "0", val->str);
        val->arr->isset = 1;
    }
    val->str    = {};
    val->dyn    = 0;
    val->attrs |= V_ASSOC;
}

// Assign one element of an array: by index, or by key when the array
// is associative. A scalar becomes an indexed array.
static b32 setelem(Shell *sh, Arena scratch, Str name, i64 idx, Str key, Str value, i32 flags = 0)
{
    Store *st  = sh->store;
    if (!(flags & SV_NOREF)) name = arraytarget(sh, scratch, name, &flags);
    if (!name.data) return 0;
    Value *val = assignslot(sh, scratch, name, flags);
    if (!val) return 0;
    if (val->attrs & V_NOASSIGN) return 1;
    switch (val->dyn) {
    case DYN_BASH_ALIASES: defalias(sh, key, value);   return 1;
    case DYN_BASH_CMDS:    hashenter(sh, key, value, 0); return 1;
    case DYN_DIRSTACK:     return 1;  // no pushd yet
    }
    toarray(sh, val);
    Array *arr = val->arr;
    Str old = {};
    if (arr->bucket) {
        HItem *t = hfind(arr, key);
        if (t) old = t->val;
    } else {
        Str *e = aget(arr, idx);
        if (e) old = *e;
    }
    value = assignedvalue(sh, &scratch, val->attrs, old, value, flags);
    if (arr->bucket) {
        hput(st, arr, key, sclone(st, value));
    } else {
        aput(st, arr, idx, sclone(st, value));
    }
    arr->isset = 1;
    assigned(sh, val, name);
    return 1;
}

// Assign a scalar, or element 0 of an array. Reports and returns false
// for a readonly variable.
static b32 setvar(Shell *sh, Arena scratch, Str name, Str value, i32 flags = 0)
{
    Store *st  = sh->store;
    Value *top = rawlookup(sh, name);
    if (top && !(flags & SV_NOREF) && anyref(top)) {
        Bind b = assignref(sh, name);
        if (b.fail) return 0;
        if (b.hassub) {
            Str    word = concat3(&scratch, b.name, "[", concat(&scratch, b.sub, "]"));
            Value *base = rawlookup(sh, b.name);
            if (base && (base->attrs & V_NAMEREF) && b.local) {
                return assignelem(sh, scratch, b.name, b.sub, word, value, flags);
            } else if (base && (base->attrs & V_NAMEREF)) {
                // a nameref cannot be an array
                shellerror(sh, scratch, concat3(&scratch, "warning: ", b.name, ": removing nameref attribute"));
                Value *nv = setattrs(sh, b.name, 0, V_NAMEREF);
                nv->str = {};
            }
            return assignelem(sh, scratch, b.name, b.sub, word, value, flags|SV_NOREF);
        }
        Value *self = b.self ? rawlookup(sh, b.name) : 0;
        if (self && (self->attrs & V_READONLY) && !(flags & SV_FORCE)) {
            return 0;  // bash finds no variable to assign, and says nothing
        }
        if (b.self && !validnameref(value)) {
            // A nameref without a value: the assignment gives it one
            Str msg = concat3(&scratch, "`", value, "': not a valid identifier");
            builtinerror(sh, scratch, sh->bltname, msg);
            return 0;
        }
        name   = b.name;
        flags |= SV_NOREF | (b.global ? SV_GLOBAL : 0);
    }
    Value *val = assignslot(sh, scratch, name, flags);
    if (!val) return 0;
    if (val->attrs & V_NOASSIGN) return 1;
    if (val->arr) {
        return setelem(sh, scratch, name, 0, "0", value, flags|SV_FORCE);
    }
    Str old = val->dyn && (flags & SV_APPEND) ? dynvalue(sh, &scratch, val) : val->str;
    value = assignedvalue(sh, &scratch, val->attrs, old, value, flags);
    if (val->dyn == DYN_RANDOM) {
        Arena a = scratch;
        Arith r = evalarith(sh, &a, value);
        sh->seed = (u32)(r.ok ? r.value : 0);
    } else if (val->dyn == DYN_SECONDS) {
        Arena a = scratch;
        Arith r = evalarith(sh, &a, value);
        val->base = (r.ok ? r.value : 0) - (dynnow(sh) - sh->start)/1000000000;
    } else if (val->dyn == DYN_BASH_ARGV0) {
        // $0 follows, and keeps the value if BASH_ARGV0 is unset
        Str prev = sh->argv0;
        sh->argv0 = sclone(st, value);
        if ((byte *)prev.data >= st->mark) sfreestr(st, prev);
    } else if (val->dyn==DYN_LINENO || val->dyn==DYN_BASH_SUBSHELL ||
               val->dyn==DYN_FUNCNAME || val->dyn==DYN_BASH_LINENO ||
               val->dyn==DYN_BASH_SOURCE || val->dyn==DYN_BASH_COMMAND ||
               val->dyn==DYN_BASH_ARGV || val->dyn==DYN_BASH_ARGC) {
        // bash ignores these assignments
    } else {
        val->dyn = 0;
    }
    old = val->str;
    val->str = sclone(st, value);
    if ((byte *)old.data >= st->mark) {
        sfreestr(st, old);
    }
    assigned(sh, val, name);
    return 1;
}

// Set attributes on the visible binding, creating it unset if needed.
// Turning on V_ARRAY or V_ASSOC converts the value.
static Value *setattrs(Shell *sh, Str name, u32 on, u32 off)
{
    Store *st = sh->store;
    Var   *v  = upsertvar(sh, name);
    b32 fresh = !v->top;
    if (!v->top) {
        Value *n = (Value *)salloc(st, sizeof(Value));
        jset(st, &v->top, n);
    }
    Value *val = ownvalue(sh, &v->top);
    if ((on & V_ASSOC) && !val->arr) {
        toassoc(sh, val, fresh);
    } else if ((on & V_ARRAY) && !val->arr && val->dyn!=DYN_PIPESTATUS) {
        toarray(sh, val);
    }
    val->attrs = (val->attrs | on) & ~off;
    if ((on|off) & V_EXPORT) {
        sh->exportgen++;
    }
    return val;
}

// Remove the visible binding. Returns false if it is readonly.
static b32 unsetvar(Shell *sh, Str name, b32 force = 0)
{
    Var *v = findvar(sh, name);
    if (!v || !v->top) return 1;
    Value *top = v->top;
    if ((top->attrs & V_READONLY) && !force) return 0;
    if (top->depth && top->depth==sh->funcdepth && !(top->attrs & V_TEMP)) {
        // A local at the current level stays local, but loses its value
        Value *val = ownvalue(sh, &v->top);
        afree(sh->store, val->arr);
        val->str   = {};
        val->arr   = 0;
        val->dyn   = 0;
        val->attrs &= V_LOCAL;
    } else {
        if (top->attrs & V_EXPORT) sh->exportgen++;
        jset(sh->store, &v->top, top->below);
        freevalue(sh, top);
    }
    varchanged(sh, name);
    return 1;
}

// Make a variable local to the current function. Returns its binding.
static Value *makelocal(Shell *sh, Arena *scratch, Str name)
{
    Store *st = sh->store;
    Var   *v  = upsertvar(sh, name);
    if (v->top && v->top->depth==sh->funcdepth && !(v->top->attrs & V_TEMP)) {
        return ownvalue(sh, &v->top);
    }
    Value *n = (Value *)salloc(st, sizeof(Value));
    n->below = v->top;
    n->depth = sh->funcdepth;
    n->attrs = V_LOCAL;
    Value *t = v->top;
    if (t && sh->shopt[S_LOCALVAR_INHERIT]) {
        n->str   = sclone(st, t->str);
        if (t->arr) n->arr = acopy(st, t->arr, 0);
        n->attrs |= t->attrs & ~(V_READONLY|V_TEMP);
    } else if (t) {
        // As bash: the export attribute carries over, and the value of
        // a temporary binding made for this very call
        n->attrs |= t->attrs & V_EXPORT;
        if ((t->attrs & V_TEMP) && t->depth==sh->funcdepth) {
            n->str = sclone(st, t->str);
        }
    }
    jset(st, &v->top, n);

    // The list outlives the declaring builtin's scratch: keep it in the
    // store until the function returns
    Frame *f = sh->frame;
    if (f && name=="OPTIND" && !f->optsaved) {
        // As bash, the function's return restores getopts' position
        f->optsaved = 1;
        f->optchar  = sh->optchar;
    }
    if (f) {
        if (f->nlocals == f->caplocals) {
            iz    cap = f->caplocals ? 2*f->caplocals : 8;
            Var **na  = (Var **)salloc(st, cap*(iz)sizeof(Var *));
            for (iz i = 0; i < f->nlocals; i++) na[i] = f->locals[i];
            sfree(st, f->locals, f->caplocals*(iz)sizeof(Var *));
            f->locals    = na;
            f->caplocals = cap;
        }
        f->locals[f->nlocals++] = v;
    }
    (void)scratch;
    return n;
}

// Pop the bindings a returning function made.
static void poplocals(Shell *sh, Frame *f)
{
    for (iz i = f->nlocals-1; i >= 0; i--) {
        Var *v = f->locals[i];
        Value **slot = &v->top;
        while (*slot && (*slot)->depth>sh->funcdepth) {
            slot = &(*slot)->below;  // a deeper binding left behind
        }
        Value *top = *slot;
        if (top && top->depth==sh->funcdepth && !(top->attrs & V_TEMP)) {
            if (top->attrs & V_EXPORT) sh->exportgen++;
            jset(sh->store, slot, top->below);
            freevalue(sh, top);
            varchanged(sh, v->name);
        }
    }
    if (f->optsaved) {
        sh->optchar = f->optchar;
        f->optsaved = 0;
    }
    sfree(sh->store, f->locals, f->caplocals*(iz)sizeof(Var *));
    f->locals    = 0;
    f->nlocals   = 0;
    f->caplocals = 0;
}

// Push a temporary binding (NAME=value before a builtin or function).
// The variable a temporary binding of name binds: what a nameref names.
static Str tempname(Shell *sh, Str name)
{
    Value *t = rawlookup(sh, name);
    if (!t || !(t->attrs & V_NAMEREF)) return name;
    Ref r = resolveref(sh, name);
    return r.name.data && !r.hassub ? r.name : name;
}

static b32 pushtemp(Shell *sh, Arena scratch, Str name, Str value)
{
    Store *st = sh->store;
    name = tempname(sh, name);
    Var   *v  = upsertvar(sh, name);
    if (v->top && (v->top->attrs & V_READONLY)) {
        shellerror(sh, scratch, concat(&scratch, name, ": readonly variable"));
        return 0;
    }
    Value *n = (Value *)salloc(st, sizeof(Value));
    n->below = v->top;
    n->depth = sh->funcdepth + 1;
    n->attrs = V_TEMP|V_EXPORT | (v->top ? v->top->attrs & ~(V_TEMP|V_LOCAL|V_ARRAY|V_ASSOC|V_NAMEREF) : 0);
    jset(st, &v->top, n);
    Value *val = n;
    if (val->attrs & V_INTEGER) {
        Arith r = evalinteger(sh, &scratch, value);
        value = fmti64(&scratch, r.ok ? r.value : 0);
    }
    val->str = sclone(st, convertcase(&scratch, value, val->attrs));
    sh->exportgen++;
    varchanged(sh, name);
    return 1;
}

static void poptemp(Shell *sh, Str name)
{
    name = tempname(sh, name);
    Var *v = findvar(sh, name);
    if (!v) return;
    Value **slot = &v->top;
    while (*slot && !((*slot)->attrs & V_TEMP)) slot = &(*slot)->below;
    if (*slot) {
        Value *t = *slot;
        jset(sh->store, slot, t->below);
        freevalue(sh, t);
        sh->exportgen++;
        varchanged(sh, name);
    }
}


// Functions

static Func *findfunc(Shell *sh, Str name)
{
    Func *f = sh->funcs;
    for (u64 h = hash(name); f; h <<= 2) {
        if (f->name == name) return f->body ? f : 0;
        f = f->child[h>>62];
    }
    return 0;
}

static void freecode(Shell *sh, Code *c)
{
    keepcmdtext(sh, (byte *)c, (byte *)c+c->size);
    sfree(c->store, c, c->size);
}

// The function table entry f lets go of its body.
static void releasecode(Shell *sh, Func *f)
{
    Store *st = sh->store;
    Code  *c  = f->code;
    if (!c || (byte *)f<st->mark) {
        return;  // a subshell's change, which rollback reverses
    }
    if (c->store != st) {
        // another vproc's: shared by reference
        if (!--c->refs && c->dead) freecode(sh, c);
    } else if (c->refs) {
        c->dead = 1;
    } else {
        freecode(sh, c);
    }
}

// Copy a body into a block of exactly the size needed: measure with a
// trial copy in scratch at the same alignment, then copy for real.
static Code *newcode(Shell *sh, Arena scratch, Node *body)
{
    scratch.beg += -(uz)scratch.beg & 15;
    byte *start = scratch.beg;
    alloc<Code>(&scratch);
    copynode(&scratch, body);
    // alloc refuses the final object of a full arena, so leave room for
    // one more of the largest type
    iz size = scratch.beg - start + (iz)sizeof(Node) + (iz)sizeof(Redir);
    size += sh->srcname.len;

    Arena block = {};
    block.beg = salloc(sh->store, size);
    block.end = block.beg + size;
    Code *c = alloc<Code>(&block);
    c->store = sh->store;
    c->size  = size;
    c->body = copynode(&block, body);
    c->file = clone(&block, sh->srcname);
    return c;
}

static void definefunc(Shell *sh, Arena scratch, Str name, Node *body)
{
    Store *st   = sh->store;
    Func **slot = &sh->funcs;
    for (u64 h = hash(name); *slot; h <<= 2) {
        if ((*slot)->name == name) break;
        slot = &(*slot)->child[h>>62];
    }
    Code *code = newcode(sh, scratch, body);
    if (!*slot) {
        Func *f = (Func *)salloc(st, sizeof(Func));
        f->name = sclone(st, name);
        f->body = code->body;
        f->code = code;
        if (slot == &sh->funcs) *slot = f; else jset(st, slot, f);
    } else {
        Func *f = *slot;
        releasecode(sh, f);
        jset(st, &f->body, code->body);
        jset(st, &f->code, code);
    }
}

static void unsetfunc(Shell *sh, Str name)
{
    Func *f = sh->funcs;
    for (u64 h = hash(name); f; h <<= 2) {
        if (f->name == name) {
            releasecode(sh, f);
            jset(sh->store, &f->body, (Node *)0);
            jset(sh->store, &f->code, (Code *)0);
            jwrite(sh->store, &f->attrs, 0);
            return;
        }
        f = f->child[h>>62];
    }
}


// Environment

static void collectexports(Var *v, Buf *names, Arena *a, Slice<Str> *out)
{
    if (!v) return;
    Value *t = v->top;
    if (t && (t->attrs & V_EXPORT) && t->str.data && !(t->attrs & (V_ARRAY|V_ASSOC))) {
        Buf b(a, v->name.len + t->str.len + 1);
        print(&b, v->name);
        putbyte(&b, '=');
        print(&b, t->str);
        *out = push(a, *out, finish(&b));
    }
    for (i32 i = 0; i < 4; i++) {
        collectexports(v->child[i], names, a, out);
    }
}

// Exported functions, as bash passes them: BASH_FUNC_name%%=() {...}
static void exportfuncs(Func *f, Arena *a, Slice<Str> *out)
{
    if (!f) return;
    if (f->body && (f->attrs & V_EXPORT)) {
        Str text = printfunction(a, {}, f->body, 0);
        // bash joins the first body line to the brace
        Str head = "() \n{ \n";
        if (startswith(text, head)) {
            text = cuthead(text, head.len);
            iz i = 0;
            while (i<text.len && text[i]==' ') i++;
            text = concat(a, "() {  ", cuthead(text, i));
        }
        Buf b(a, f->name.len + text.len + 13);
        print(&b, "BASH_FUNC_");
        print(&b, f->name);
        print(&b, "%%=");
        print(&b, text);
        *out = push(a, *out, finish(&b));
    }
    for (i32 i = 0; i < 4; i++) {
        exportfuncs(f->child[i], a, out);
    }
}

static b32 envless(Str *a, Str *b)
{
    return compare(*a, *b) < 0;
}

// The environment for a child process, sorted by name.
static Slice<Str> makeenv(Shell *sh, Arena *a)
{
    Slice<Str> env = {};
    collectexports(sh->vars, 0, a, &env);
    exportfuncs(sh->funcs, a, &env);
    sort(env.data, env.len, envless, *a);
    return env;
}
