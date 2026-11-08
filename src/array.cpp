// Arrays: indexed and associative, in the store
//
// An indexed array is a vector of (index, value) elements sorted by
// index. While it is dense, element i sits at position i and lookup is
// direct; otherwise lookup is a binary search. Appending is amortized
// constant either way.
//
// An associative array reproduces bash's hash table exactly, because
// its iteration order is visible: FNV-1 over signed chars, a power of
// two buckets, insertion at the head of a chain, and growth by four
// when the entries reach twice the buckets. Iteration walks the buckets
// in order and each chain from its head.
//
// Arrays belong to the Value that holds them. A subshell that modifies
// an array its parent owns first copies it (see ownvalue), so arrays
// are mutated in place without journaling.
//
// This is free and unencumbered software released into the public domain.

struct Elem {
    i64 idx;
    Str val;
};

struct HItem {
    HItem *next;
    Str    key;
    Str    val;
    u32    hash;
};

struct Array {
    Elem   *e;          // indexed: sorted by idx
    iz      len;
    iz      cap;
    HItem **bucket;     // associative
    iz      nbuckets;
    iz      n;
    b32     isset;      // assigned since declared, so declare -p shows =()
};

enum { ASSOC_BUCKETS = 1024, ASSOC_CONVERTED = 128 };

static Array *newarray(Store *st)
{
    return (Array *)salloc(st, sizeof(Array));
}

static Array *newassoc(Store *st, iz nbuckets)
{
    Array *a = newarray(st);
    a->nbuckets = nbuckets;
    a->bucket   = (HItem **)salloc(st, nbuckets*(iz)sizeof(HItem *));
    return a;
}

static i64 amax(Array *a)
{
    return a->len ? a->e[a->len-1].idx : -1;
}

// Position of idx, or -(insertion point)-1 when absent.
static iz afind(Array *a, i64 idx)
{
    if (idx>=0 && idx<a->len && a->e[idx].idx==idx) {
        return (iz)idx;  // dense so far
    }
    if (!a->len || idx>amax(a)) {
        return -a->len - 1;
    }
    iz lo = 0, hi = a->len;
    while (lo < hi) {
        iz mid = lo + (hi - lo)/2;
        if (a->e[mid].idx < idx) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo<a->len && a->e[lo].idx==idx ? lo : -lo-1;
}

static Str *aget(Array *a, i64 idx)
{
    iz i = afind(a, idx);
    return i<0 ? 0 : &a->e[i].val;
}

// Store a value (already store memory) at idx, freeing any old one.
static void aput(Store *st, Array *a, i64 idx, Str val)
{
    iz i = afind(a, idx);
    if (i >= 0) {
        sfreestr(st, a->e[i].val);
        a->e[i].val = val;
        return;
    }
    i = -i - 1;
    if (a->len == a->cap) {
        iz    cap = a->cap ? 2*a->cap : 8;
        Elem *ne  = (Elem *)salloc(st, cap*(iz)sizeof(Elem));
        copybytes(ne, a->e, a->len*(iz)sizeof(Elem));
        sfree(st, a->e, a->cap*(iz)sizeof(Elem));
        a->e   = ne;
        a->cap = cap;
    }
    for (iz k = a->len; k > i; k--) {
        a->e[k] = a->e[k-1];
    }
    a->e[i].idx = idx;
    a->e[i].val = val;
    a->len++;
}

static void adel(Store *st, Array *a, i64 idx)
{
    iz i = afind(a, idx);
    if (i < 0) return;
    sfreestr(st, a->e[i].val);
    for (iz k = i+1; k < a->len; k++) {
        a->e[k-1] = a->e[k];
    }
    a->len--;
}

// Bash's string hash: FNV-1 over the bytes as signed chars.
static u32 hashkey(Str key)
{
    u32 h = 2166136261u;
    for (iz i = 0; i < key.len; i++) {
        h *= 16777619u;
        h ^= (u32)(i32)(signed char)key[i];
    }
    return h;
}

static HItem *hfind(Array *a, Str key)
{
    if (!a->n) return 0;
    u32 h = hashkey(key);
    for (HItem *t = a->bucket[h & (a->nbuckets-1)]; t; t = t->next) {
        if (t->hash==h && t->key==key) return t;
    }
    return 0;
}

static void hgrow(Store *st, Array *a)
{
    iz      n   = a->nbuckets * 4;
    HItem **nb  = (HItem **)salloc(st, n*(iz)sizeof(HItem *));
    for (iz j = 0; j < a->nbuckets; j++) {
        HItem *next = 0;
        for (HItem *t = a->bucket[j]; t; t = next) {
            next = t->next;
            iz i = t->hash & (n-1);
            t->next = nb[i];
            nb[i]   = t;
        }
    }
    sfree(st, a->bucket, a->nbuckets*(iz)sizeof(HItem *));
    a->bucket   = nb;
    a->nbuckets = n;
}

// Store a value (already store memory) under key, freeing any old one.
static void hput(Store *st, Array *a, Str key, Str val)
{
    HItem *t = hfind(a, key);
    if (t) {
        sfreestr(st, t->val);
        t->val = val;
        return;
    }
    if (a->n >= a->nbuckets*2) {
        hgrow(st, a);
    }
    t = (HItem *)salloc(st, sizeof(HItem));
    t->hash = hashkey(key);
    t->key  = sclone(st, key);
    t->val  = val;
    iz i = t->hash & (a->nbuckets-1);
    t->next = a->bucket[i];
    a->bucket[i] = t;
    a->n++;
}

static void hdel(Store *st, Array *a, Str key)
{
    if (!a->n) return;
    u32 h = hashkey(key);
    for (HItem **p = &a->bucket[h & (a->nbuckets-1)]; *p; p = &(*p)->next) {
        HItem *t = *p;
        if (t->hash==h && t->key==key) {
            *p = t->next;
            sfreestr(st, t->key);
            sfreestr(st, t->val);
            sfree(st, t, sizeof(HItem));
            a->n--;
            return;
        }
    }
}

// Number of elements.
static iz acount(Array *a)
{
    return a->bucket ? a->n : a->len;
}

// Copy an array. With share set, the strings are shared rather than
// cloned: a subshell's copy of its parent's array, which never frees
// the parent's memory.
static Array *acopy(Store *st, Array *a, b32 share)
{
    Array *r = newarray(st);
    r->isset = a->isset;
    if (a->bucket) {
        r->nbuckets = a->nbuckets;
        r->bucket   = (HItem **)salloc(st, a->nbuckets*(iz)sizeof(HItem *));
        r->n        = a->n;
        for (iz j = 0; j < a->nbuckets; j++) {
            HItem **tail = &r->bucket[j];
            for (HItem *t = a->bucket[j]; t; t = t->next) {
                HItem *c = (HItem *)salloc(st, sizeof(HItem));
                c->hash = t->hash;
                c->key  = share ? t->key : sclone(st, t->key);
                c->val  = share ? t->val : sclone(st, t->val);
                *tail = c;
                tail  = &c->next;
            }
        }
        return r;
    }
    r->cap = r->len = a->len;
    if (a->len) {
        r->e = (Elem *)salloc(st, a->len*(iz)sizeof(Elem));
        for (iz i = 0; i < a->len; i++) {
            r->e[i].idx = a->e[i].idx;
            r->e[i].val = share ? a->e[i].val : sclone(st, a->e[i].val);
        }
    }
    return r;
}

static void afree(Store *st, Array *a)
{
    if (!a) return;
    if (a->bucket) {
        for (iz j = 0; j < a->nbuckets; j++) {
            HItem *next = 0;
            for (HItem *t = a->bucket[j]; t; t = next) {
                next = t->next;
                sfreestr(st, t->key);
                sfreestr(st, t->val);
                sfree(st, t, sizeof(HItem));
            }
        }
        sfree(st, a->bucket, a->nbuckets*(iz)sizeof(HItem *));
    } else {
        for (iz i = 0; i < a->len; i++) {
            sfreestr(st, a->e[i].val);
        }
        sfree(st, a->e, a->cap*(iz)sizeof(Elem));
    }
    sfree(st, a, sizeof(Array));
}

// Remove every element, keeping an associative array's bucket count.
static void aclear(Store *st, Array *a)
{
    if (a->bucket) {
        for (iz j = 0; j < a->nbuckets; j++) {
            HItem *next = 0;
            for (HItem *t = a->bucket[j]; t; t = next) {
                next = t->next;
                sfreestr(st, t->key);
                sfreestr(st, t->val);
                sfree(st, t, sizeof(HItem));
            }
            a->bucket[j] = 0;
        }
        a->n = 0;
    } else {
        for (iz i = 0; i < a->len; i++) {
            sfreestr(st, a->e[i].val);
        }
        a->len = 0;
    }
}

struct AList {      // an array's elements in order, in scratch
    Str *keys;      // associative keys, or formatted indices
    i64 *idx;       // indexed: the indices
    Str *vals;
    iz   n;
};

static AList alist(Arena *a, Array *arr, b32 wantkeys)
{
    AList r = {};
    iz    n = arr ? acount(arr) : 0;
    r.vals = alloc<Str>(a, n);
    r.idx  = alloc<i64>(a, n);
    if (wantkeys) r.keys = alloc<Str>(a, n);
    if (!arr) return r;
    if (arr->bucket) {
        for (iz j = 0; j < arr->nbuckets; j++) {
            for (HItem *t = arr->bucket[j]; t; t = t->next) {
                if (wantkeys) r.keys[r.n] = t->key;
                r.idx[r.n]    = r.n;
                r.vals[r.n++] = t->val;
            }
        }
    } else {
        for (iz i = 0; i < arr->len; i++) {
            if (wantkeys) {
                Buf b(a, 24);
                print(&b, arr->e[i].idx);
                r.keys[r.n] = finish(&b);
            }
            r.idx[r.n]    = arr->e[i].idx;
            r.vals[r.n++] = arr->e[i].val;
        }
    }
    return r;
}

// A value as declare -p shows it: double quotes, or $'...' when it
// has unprintable characters.
static Str declquote(Arena *a, Str s)
{
    if (ansicshouldquote(s)) return ansicquote(a, s);
    Buf b(a, s.len+2);
    putbyte(&b, '"');
    for (iz i = 0; i < s.len; i++) {
        u8 c = s[i];
        if (c=='"' || c=='\\' || c=='$' || c=='`') putbyte(&b, '\\');
        putbyte(&b, c);
    }
    putbyte(&b, '"');
    return finish(&b);
}

static b32 shellmetas(Str s)
{
    for (iz i = 0; i < s.len; i++) {
        switch (s[i]) {
        case ' ':  case '\t': case '\n': case '\'': case '"': case '\\':
        case '|':  case '&':  case ';':  case '(':  case ')': case '<':
        case '>':  case '!':  case '{':  case '}':  case '*': case '[':
        case '?':  case ']':  case '^':  case '$':  case '`':
            return 1;
        case '~':
            if (!i || s[i-1]=='=' || s[i-1]==':') return 1;
            break;
        case '#':
            if (!i) return 1;
            break;
        }
    }
    return 0;
}

// An associative key as declare -p shows it.
static Str keyquote(Arena *a, Str key)
{
    if (ansicshouldquote(key)) return ansicquote(a, key);
    if (shellmetas(key) || key=="@" || key=="*") return declquote(a, key);
    return key;
}

// An array as declare -p shows it: ([key]="value" ...).
static Str arrayassign(Arena *a, Array *arr)
{
    Buf b(a, 64);
    putbyte(&b, '(');
    if (arr->bucket) {
        for (iz j = 0; j < arr->nbuckets; j++) {
            for (HItem *t = arr->bucket[j]; t; t = t->next) {
                putbyte(&b, '[');
                print(&b, keyquote(a, t->key));
                print(&b, "]=");
                print(&b, declquote(a, t->val));
                putbyte(&b, ' ');
            }
        }
    } else {
        for (iz i = 0; i < arr->len; i++) {
            if (i) putbyte(&b, ' ');
            putbyte(&b, '[');
            print(&b, arr->e[i].idx);
            print(&b, "]=");
            print(&b, declquote(a, arr->e[i].val));
        }
    }
    putbyte(&b, ')');
    return finish(&b);
}

// An array as ${a[@]@K} shows it: key "value" ...
static Str arraykv(Arena *a, Array *arr)
{
    Buf b(a, 64);
    if (arr->bucket) {
        for (iz j = 0; j < arr->nbuckets; j++) {
            for (HItem *t = arr->bucket[j]; t; t = t->next) {
                print(&b, keyquote(a, t->key));
                putbyte(&b, ' ');
                print(&b, declquote(a, t->val));
                putbyte(&b, ' ');
            }
        }
    } else {
        for (iz i = 0; i < arr->len; i++) {
            if (i) putbyte(&b, ' ');
            print(&b, arr->e[i].idx);
            putbyte(&b, ' ');
            print(&b, declquote(a, arr->e[i].val));
        }
    }
    return finish(&b);
}
