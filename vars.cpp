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
    DYN_BASH_SOURCE, DYN_BASH_ARGV0, DYN_PIPESTATUS,
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

// The visible binding of a variable, or null.
static Value *lookup(Shell *sh, Str name)
{
    Var *v = findvar(sh, name);
    return v ? v->top : 0;
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
        return sh->frame ? sh->frame->name : Str{};
    case DYN_BASH_LINENO:
        if (sh->frame) return fmti64(a, sh->frame->line);
        return !sh->cflag && !sh->interactive ? Str("0") : Str{};
    case DYN_BASH_SOURCE:
        if (sh->frame) return sh->frame->file.len ? sh->frame->file : sh->name;
        return !sh->cflag && !sh->interactive ? sh->name : Str{};
    case DYN_BASH_COMMAND:  return cmdtext(sh, a);
    case DYN_HISTCMD:       return "1";
    case DYN_BASH_ARGV0:    return dollar0(sh);
    case DYN_PIPESTATUS:    return fmti64(a, sh->npstat ? sh->pstat[0] : 0);
    }
    return v->str;
}

struct Got {
    Str value;
    b32 set;
};

// A variable's value as a string, computing dynamic ones.
static Got getvar(Shell *sh, Arena *a, Str name)
{
    Got    r = {};
    Value *v = lookup(sh, name);
    if (!v) return r;
    if (v->dyn) {
        r.value = dynvalue(sh, a, v);
        r.set   = 1;
        if (v->dyn==DYN_FUNCNAME || v->dyn==DYN_BASH_LINENO || v->dyn==DYN_BASH_SOURCE) {
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
    if (d!=DYN_FUNCNAME && d!=DYN_BASH_LINENO && d!=DYN_BASH_SOURCE) return 0;
    iz n = 0;
    for (Frame *f = sh->frame; f; f = f->up) n++;
    b32 main = !sh->cflag && !sh->interactive && (n || d!=DYN_FUNCNAME);
    Array *r = alloc<Array>(a);
    r->cap = n + main;
    r->e   = alloc<Elem>(a, r->cap);
    for (Frame *f = sh->frame; f; f = f->up) {
        r->e[r->len].idx = r->len;
        r->e[r->len].val = d==DYN_FUNCNAME   ? f->name
                         : d==DYN_BASH_LINENO ? fmti64(a, f->line)
                         : f->file.len        ? f->file
                         :                      sh->name;
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
    Value *val = assignslot(sh, scratch, name, flags);
    if (!val) return 0;
    if (val->attrs & V_NOASSIGN) return 1;
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
               val->dyn==DYN_BASH_SOURCE || val->dyn==DYN_BASH_COMMAND) {
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
static b32 pushtemp(Shell *sh, Arena scratch, Str name, Str value)
{
    Store *st = sh->store;
    Var   *v  = upsertvar(sh, name);
    if (v->top && (v->top->attrs & V_READONLY)) {
        shellerror(sh, scratch, concat(&scratch, name, ": readonly variable"));
        return 0;
    }
    Value *n = (Value *)salloc(st, sizeof(Value));
    n->below = v->top;
    n->depth = sh->funcdepth + 1;
    n->attrs = V_TEMP|V_EXPORT | (v->top ? v->top->attrs & ~(V_TEMP|V_LOCAL|V_ARRAY|V_ASSOC) : 0);
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
