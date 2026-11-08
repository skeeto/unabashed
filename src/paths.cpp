// Paths: POSIX, msys (Git Bash) and native (busybox-w32) path semantics
//
// Pure functions over a PathCtx, with no os_* calls, so they run and are
// tested anywhere. The platform sees only canonical native paths:
// forward slashes, an uppercase drive letter, no "." or ".." components,
// case kept as typed ("C:/x", "C:/" for a drive root, "//srv/share/x",
// "//./NUL", "//./pipe/x"), or plain POSIX paths in POSIX mode.
//
// The msys rules follow msys2-runtime: mount.cc and path.cc for the
// mount table and cygpath, environ.cc for the environment of native
// children, and msys2_path_conv.cc (CC0) for the argument heuristic,
// misfires included. They were reimplemented from the semantics in
// notes/research/msys2.md and checked against Git Bash recordings
// (t/unit/paths_record.sh). Deviations are listed in notes/m5a.md.
//
// This is free and unencumbered software released into the public domain.

enum PathMode : i32 {
    PM_POSIX,   // paths pass through: Linux and friends
    PM_MSYS,    // a mount table: "/" is the install root, /c/x is C:/x
    PM_NATIVE,  // "/x" is on the current drive, "C:x" uses C:'s directory
};

enum : i32 {  // Mount flags
    MNT_NOCASE    = 1<<0,  // posix=0: the mount point ignores case
    MNT_IMMUTABLE = 1<<1,  // the root and usertemp: fstab cannot replace
};

struct Mount {
    Str posix;   // mount point, no trailing slash except for "/"
    Str native;  // canonical, no trailing slash: "C:" for a drive root
    i32 flags;
};

struct PathCtx {
    PathMode     mode;
    Slice<Mount> mounts;        // from initmounts
    Str          cygdrive;      // drive prefix ending in '/', null for "/"
    Str          cwd;           // native working directory (POSIX in PM_POSIX)
    Str          drivecwd[26];  // per-drive directories (=X: variables)
    Str          temp;          // canonical %TEMP%, for usertemp mounts
    u32          drives;        // existing drives, bit 0 for A:, 0 if unknown
    i64          pid;           // our pid as /proc names it, for /dev/fd/N
    Str          windir;        // the Windows directory, for children
};

enum : i32 {  // PathRes kinds
    PK_FILE,      // native names a file system object
    PK_NULL,      // /dev/null, NUL: native is //./NUL
    PK_TTY,       // /dev/tty, /dev/console, CON: native is //./CON
    PK_FD,        // /dev/fd/N, /dev/std*, /proc/self/fd/N: num is N
    PK_DEVICE,    // another reserved DOS name: native is //./NAME
    PK_DEV,       // another emulated /dev entry: sub is its name
    PK_PROC,      // /proc: num is the pid, -1 for self, 0 for /proc
                  // itself; sub is the rest ("comm", "fd/3", "")
    PK_CYGDRIVE,  // the cygdrive directory itself (prefix not "/")
};

struct PathRes {
    Str native;
    Str sub;
    i64 num;
    i32 kind;
    b32 literal;  // msys: open as named, bypassing Win32 name rules
    b32 ok;       // 0: no such path (empty, unknown /dev or /proc entry)
};


// Helpers

static b32 pathsep(u8 c)
{
    return c=='/' || c=='\\';
}

static b32 hasdrive(Str s)  // starts with "X:"
{
    return s.len>=2 && letter(s[0]) && s[1]==':';
}

static b32 equalfold(Str a, Str b)
{
    if (a.len != b.len) return 0;
    for (iz i = 0; i < a.len; i++) {
        if (lowercase(a[i]) != lowercase(b[i])) return 0;
    }
    return 1;
}

static Str slashify(Arena *a, Str s, u8 to)  // both separators become to
{
    Str r = clone(a, s);
    for (iz i = 0; i < r.len; i++) {
        if (pathsep(r[i])) r[i] = to;
    }
    return r;
}

// The decimal value of all of s, or -1 if empty, not digits, or huge.
static i64 pathnum(Str s)
{
    if (!s.len || s.len>9) return -1;
    i64 r = 0;
    for (iz i = 0; i < s.len; i++) {
        if (!digit(s[i])) return -1;
        r = r*10 + s[i] - '0';
    }
    return r;
}

static Str lastpart(Str path)
{
    iz i = path.len;
    for (; i>0 && !pathsep(path[i-1]); i--) {}
    return cuthead(path, i);
}

// Does path name a directory by its form: a trailing separator, "."
// or ".." at the end?
static b32 dirform(Str path)
{
    if (path.len && pathsep(path[path.len-1])) return 1;
    Str last = lastpart(path);
    return last=="." || last=="..";
}

static b32 hasdotdot(Str path)
{
    for (iz i = 0; i+1 < path.len; i++) {
        if (path[i]=='.' && path[i+1]=='.' && (!i || pathsep(path[i-1]))
                && (i+2==path.len || pathsep(path[i+2]))) {
            return 1;
        }
    }
    return 0;
}

// Does prefix start path at a component boundary? A trailing separator
// on prefix is ignored, and an empty prefix (the root "/") matches any
// path with one leading separator. Like Cygwin's path_prefix_p.
static b32 pathprefix(Str prefix, Str path, b32 nocase)
{
    iz n = prefix.len;
    if (n && pathsep(prefix[n-1])) n--;
    if (!n) {
        return path.len && pathsep(path[0]) && !pathsep(byteat(path, 1));
    }
    if (path.len<n || (path.len>n && !pathsep(path[n]) && prefix[n-1]!=':')) {
        return 0;
    }
    Str head = takehead(path, n);
    Str want = takehead(prefix, n);
    return nocase ? equalfold(head, want) : head==want;
}


// Lexical normalization

// Append the components of path to b, which ends in '/' after a root of
// rootlen bytes that ".." cannot climb above. Both separators count.
static void addparts(Buf *b, iz rootlen, Str path)
{
    for (iz i = 0; i < path.len;) {
        iz j = i;
        for (; j<path.len && !pathsep(path[j]); j++) {}
        Str c = {path.data+i, j-i};
        i = j + 1;
        if (!c.len || c==".") {
            continue;
        } else if (c == "..") {
            if (b->len > rootlen) {
                b->len--;
                for (; b->len>rootlen && b->data[b->len-1]!='/'; b->len--) {}
            }
        } else {
            print(b, c);
            putbyte(b, '/');
        }
    }
}

// Finish addparts output: drop the trailing slash unless the path is a
// directory by form or a bare root ("/", "//", "X:/") of rootlen bytes.
static Str finishparts(Buf *b, iz rootlen, b32 dir)
{
    Str r = finish(b);
    b32 root = r.len==rootlen && rootlen<=3;
    if (!dir && !root && r.len && r[r.len-1]=='/') {
        r.len--;
    }
    return r;
}

// The root of a canonical native path, with its trailing slash if any:
// "X:/", "//srv/share/", "//./NAME/", or "/".
static Str nativeroot(Str path)
{
    if (hasdrive(path)) {
        return takehead(path, path.len>2 && path[2]=='/' ? 3 : 2);
    }
    if (path.len>=2 && path[0]=='/' && path[1]=='/') {
        iz i = 2;
        for (i32 k = 0; k < 2 && i < path.len; k++) {
            for (; i<path.len && path[i]!='/'; i++) {}
            i += i < path.len;
        }
        return takehead(path, i);
    }
    return takehead(path, path.len && path[0]=='/');
}

// Start b with root, ending in '/'. Returns the root length.
static iz putroot(Buf *b, Str root)
{
    print(b, root);
    if (!b->len || b->data[b->len-1]!='/') putbyte(b, '/');
    return b->len;
}

// The next component of s after any separators, and what follows it.
static Cut cutpart(Str s)
{
    iz i = 0;
    for (; i<s.len && pathsep(s[i]); i++) {}
    iz j = i;
    for (; j<s.len && !pathsep(s[j]); j++) {}
    Cut r = {};
    r.head = span(s.data+i, s.data+j);
    r.tail = cuthead(s, j);
    r.ok   = j > i;
    return r;
}

// Canonical form of an absolute Windows path, or of one rooted on the
// current drive or share ("\x"), resolved lexically. Accepts \\?\,
// \??\ and \\.\ prefixes. Null if path is relative or a UNC path names
// no server.
static Str winnorm(Arena *a, Str path, Str cwd)
{
    Buf b(a, path.len + cwd.len + 8);
    Str rest    = {};
    iz  rootlen = 0;
    Str p       = path;
    b32 unc     = p.len>=2 && pathsep(p[0]) && pathsep(p[1]) && !pathsep(byteat(p, 2));
    b32 lit     = p.len>=4 && pathsep(p[0]) && (p[1]=='?' || pathsep(p[1]))
                  && p[2]=='?' && pathsep(p[3]);
    b32 dev     = p.len>=4 && pathsep(p[0]) && pathsep(p[1]) && p[2]=='.'
                  && pathsep(p[3]);
    if (lit || dev) {
        p = cuthead(p, 4);
        unc = 0;
        if (hasdrive(p) && (p.len==2 || pathsep(p[2]))) {
            lit = dev = 0;  // just a drive path
        } else if (lit && p.len>=4 && equalfold(takehead(p, 3), "UNC")
                       && pathsep(p[3])) {
            lit = 0;
            unc = 1;
            p   = cuthead(p, 3);  // "\srv\share"
        }
    }

    if (lit || dev) {
        print(&b, lit ? Str("//?/") : Str("//./"));
        Cut c = cutpart(p);
        if (c.ok) {
            print(&b, c.head);
            putbyte(&b, '/');
        }
        rootlen = b.len;
        rest    = c.tail;
    } else if (unc) {
        Cut server = cutpart(p);
        if (!server.ok) return {};  // no server
        print(&b, "//");
        print(&b, server.head);
        putbyte(&b, '/');
        Cut share = cutpart(server.tail);
        if (share.ok) {
            print(&b, share.head);
            putbyte(&b, '/');
        }
        rootlen = b.len;
        rest    = share.ok ? share.tail : server.tail;
    } else if (hasdrive(p)) {
        putbyte(&b, upcase(p[0]));
        print(&b, ":/");
        rootlen = b.len;
        rest    = cuthead(p, 2);
    } else if (p.len && pathsep(p[0])) {
        Str root = nativeroot(cwd);
        rootlen = putroot(&b, root.len ? root : Str("C:/"));
        rest    = p;
    } else {
        return {};
    }
    addparts(&b, rootlen, rest);
    return finishparts(&b, rootlen, dirform(rest));
}

// Normalize an absolute POSIX path: runs of separators fold (keeping
// exactly two leading slashes), "." and ".." resolve lexically, and a
// directory by form keeps a trailing slash.
static Str posixnorm(Arena *a, Str path)
{
    Buf b(a, path.len + 2);
    b32 two = path.len>=2 && path[1]=='/' && !pathsep(byteat(path, 2));
    iz  rootlen = putroot(&b, two ? Str("//") : Str("/"));
    Str rest = cuthead(path, two ? 2 : 1);
    addparts(&b, rootlen, rest);
    return finishparts(&b, rootlen, dirform(rest));
}

// Join a relative native path onto a canonical directory.
static Str joinnative(Arena *a, Str dir, Str rel)
{
    Buf b(a, dir.len + rel.len + 4);
    Str root    = nativeroot(dir);
    iz  rootlen = putroot(&b, root.len ? root : Str("C:/"));
    addparts(&b, rootlen, cuthead(dir, root.len));
    addparts(&b, rootlen, rel);
    return finishparts(&b, rootlen, dirform(rel));
}

static Str joinposix(Arena *a, Str dir, Str rel)
{
    Buf b(a, dir.len + rel.len + 1);
    print(&b, dir);
    if (!dir.len || dir[dir.len-1]!='/') putbyte(&b, '/');
    print(&b, rel);
    return finish(&b);
}


// Mount table

enum : i32 {  // posixnative flavours
    PX_OPEN,     // topath: devices and /proc are handled by the caller
    PX_CYGPATH,  // cygpath -w
    PX_ARG,      // an argument for a native child
};

static Str posixnative(PathCtx *, Arena *, Str, i32);

static Str cygprefix(PathCtx *ctx)
{
    return ctx->cygdrive.len ? ctx->cygdrive : Str("/");
}

// Add or replace a mount. Both paths must be absolute: native a drive
// or UNC path, posix a rooted path. Returns 0 if rejected, including
// replacing an immutable mount without override.
static b32 addmount(PathCtx *ctx, Arena *a, Str native, Str posix,
                    i32 flags, b32 override)
{
    b32 nok = hasdrive(native) || (native.len>=2 && pathsep(native[0])
                                   && pathsep(native[1]));
    b32 pok = posix.len && posix[0]=='/' && !(hasdrive(posix)
              || (posix.len>2 && pathsep(posix[1]) && !pathsep(posix[2])));
    if (!nok || !pok) {
        return 0;
    }
    Str n = winnorm(a, native, "C:/");
    if (n.len>1 && n[n.len-1]=='/') n.len--;
    Str p = posixnorm(a, posix);
    if (p.len>1 && p[p.len-1]=='/') p.len--;
    for (iz i = 0; i < ctx->mounts.len; i++) {
        Mount *m = &ctx->mounts[i];
        if (m->posix == p) {
            if ((m->flags & MNT_IMMUTABLE) && !override) {
                return 0;
            }
            m->native = n;
            m->flags  = flags | (override ? MNT_IMMUTABLE : 0);
            return 1;
        }
    }
    Mount m = {p, n, flags};
    ctx->mounts = push(a, ctx->mounts, m);
    return 1;
}

static Str unescape040(Arena *a, Str s)  // fstab's "\040" for a space
{
    if (find(s, "\\040") < 0) return s;
    Buf b(a, s.len);
    for (iz i = 0; i < s.len; i++) {
        if (s[i]=='\\' && startswith(cuthead(s, i), "\\040")) {
            putbyte(&b, ' ');
            i += 3;
        } else {
            putbyte(&b, s[i]);
        }
    }
    return finish(&b);
}

static b32 fstabblank(u8 c)
{
    return c==' ' || c=='\t';
}

// The next blank-separated fstab field.
static Cut fstabfield(Str s)
{
    iz i = 0;
    for (; i<s.len && fstabblank(s[i]); i++) {}
    iz j = i;
    for (; j<s.len && !fstabblank(s[j]); j++) {}
    Cut r = {};
    r.head = span(s.data+i, s.data+j);
    r.tail = cuthead(s, j);
    r.ok   = j > i;
    return r;
}

// Apply one fstab file. Lines are "NATIVE POSIX TYPE OPTIONS ..." with
// \040 for a space. Two fields make a posix=0 mount; three are ignored,
// like a line with an unknown option. Type "cygdrive" sets the drive
// prefix and "usertemp" mounts ctx->temp.
static void parsefstab(PathCtx *ctx, Arena *a, Str fstab)
{
    static Str known[] = {
        "acl", "auto", "binary", "bind", "cygexec", "dos", "exec", "ihash",
        "noacl", "nosuid", "notexec", "nouser", "override", "posix=0",
        "posix=1", "sparse", "text", "user",
    };
    for (Cut line = {.head={}, .tail=fstab, .ok=1}; line.ok;) {
        line = cut(line.tail, '\n');
        Str l = line.head;
        if (l.len && l[l.len-1]=='\r') l.len--;

        Cut f1 = fstabfield(l);
        if (!f1.ok || f1.head[0]=='#') continue;
        Cut f2 = fstabfield(f1.tail);
        if (!f2.ok) continue;
        Str native = unescape040(a, f1.head);
        Str posix  = unescape040(a, f2.head);
        Cut f3 = fstabfield(f2.tail);
        if (!f3.ok) {
            addmount(ctx, a, native, posix, MNT_NOCASE, 0);
            continue;
        }
        Cut f4 = fstabfield(f3.tail);
        if (!f4.ok) continue;

        Str type  = f3.head;
        i32 flags = type=="cygdrive" ? MNT_NOCASE : 0;
        flags |= type=="usertemp" ? MNT_IMMUTABLE : 0;
        b32 override = 0;
        b32 bind     = 0;
        b32 valid    = 1;
        Str opts     = f4.head;
        for (Cut o = {.head={}, .tail=opts, .ok=1}; valid && o.ok;) {
            o = cut(o.tail, ',');
            if (!o.ok && !o.head.len) break;  // a trailing comma
            b32 hit = 0;
            for (iz i = 0; i < countof(known); i++) {
                hit |= o.head == known[i];
            }
            valid = hit;
            if (o.head == "posix=0")  flags |= MNT_NOCASE;
            if (o.head == "posix=1")  flags &= ~MNT_NOCASE;
            if (o.head == "override") override = 1;
            if (o.head == "bind")     bind = 1;
        }
        if (!valid) continue;

        if (type == "cygdrive") {
            Str p = posixnorm(a, posix[0]=='/' ? posix : Str("/"));
            if (p[p.len-1] != '/') p = concat(a, p, "/");
            ctx->cygdrive = p;
        } else if (type == "usertemp") {
            if (ctx->temp.len) addmount(ctx, a, ctx->temp, posix, flags, override);
        } else {
            if (bind) {
                if (native[0] != '/') continue;
                native = posixnative(ctx, a, posixnorm(a, native), PX_OPEN);
                if (!native.data) continue;
            }
            addmount(ctx, a, native, posix, flags, override);
        }
    }
}

// Build the msys mount table: "/" at root (canonical native), then the
// fstab text (may be null), then /bin at root/usr/bin unless fstab
// mounted /bin. The default drive prefix is /cygdrive/.
static void initmounts(PathCtx *ctx, Arena *a, Str root, Str fstab)
{
    ctx->mounts   = {};
    ctx->cygdrive = "/cygdrive/";
    addmount(ctx, a, root, "/", MNT_IMMUTABLE, 0);
    parsefstab(ctx, a, fstab);
    for (iz i = 0; i < ctx->mounts.len; i++) {
        if (ctx->mounts[i].posix == "/bin") return;
    }
    addmount(ctx, a, joinnative(a, root, "usr/bin"), "/bin", 0, 0);
}

static Str rootnative(PathCtx *ctx)
{
    for (iz i = 0; i < ctx->mounts.len; i++) {
        if (ctx->mounts[i].posix == "/") return ctx->mounts[i].native;
    }
    return {};
}


// POSIX to native

enum : i32 {  // Device kinds under /dev, exact names only
    DV_NONE,   // not a device: the mount table applies
    DV_NULL,   // null
    DV_SINK,   // zero, full, random, urandom (readable: genfile), clipboard
    DV_TTY,    // tty, console
    DV_STD,    // stdin, stdout, stderr: num is the fd
    DV_FDDIR,  // fd
    DV_FD,     // fd/N: num is N, -1 if not a number
    DV_OTHER,  // ptmx, conin, conout, consN, ptyN, ttySN, windows
};

struct Device {
    Str name;
    i64 num;
    i32 kind;
    b32 more;  // the path continues past a non-directory device
};

// Classify a normalized absolute POSIX path under /dev.
static Device devicename(Str p)
{
    static struct { Str name; i32 kind; } table[] = {
        {"null", DV_NULL}, {"zero", DV_SINK}, {"full", DV_SINK},
        {"random", DV_SINK}, {"urandom", DV_SINK}, {"clipboard", DV_SINK},
        {"tty", DV_TTY}, {"console", DV_TTY}, {"stdin", DV_STD},
        {"stdout", DV_STD}, {"stderr", DV_STD}, {"fd", DV_FDDIR},
        {"ptmx", DV_OTHER}, {"conin", DV_OTHER}, {"conout", DV_OTHER},
        {"windows", DV_OTHER},
    };
    static Str numbered[] = {"cons", "pty", "ttyS"};

    Device r = {};
    if (!startswith(p, "/dev/")) return r;
    Cut c = cut(cuthead(p, 5), '/');
    r.name = c.head;
    for (iz i = 0; i < countof(table); i++) {
        if (c.head == table[i].name) r.kind = table[i].kind;
    }
    for (iz i = 0; !r.kind && i < countof(numbered); i++) {
        Str n = numbered[i];
        if (startswith(c.head, n) && pathnum(cuthead(c.head, n.len)) >= 0) {
            r.kind = DV_OTHER;
        }
    }
    if (r.kind == DV_STD) {
        r.num = c.head=="stdin" ? 0 : c.head=="stdout" ? 1 : 2;
    }
    if (r.kind==DV_FDDIR && c.ok && c.tail.len) {
        Str t = c.tail;
        if (t[t.len-1] == '/') t.len--;
        r.kind = DV_FD;
        r.num  = pathnum(t);
    } else if (r.kind != DV_FDDIR) {
        r.more = c.ok;
    }
    return r;
}

// Map the drive part of a cygdrive path: letter, then rest after it.
static Str drivepath(PathCtx *ctx, Arena *a, u8 letter, Str rest, b32 dir)
{
    Buf b(a, rest.len + 5);
    putbyte(&b, upcase(letter));
    print(&b, ":/");
    if (rest.len) print(&b, cuthead(rest, 1));
    Str r = finish(&b);
    i32  bit     = upcase(letter) - 'A';
    b32  missing = ctx->drives && !(ctx->drives>>bit & 1);
    if (dir && missing && r.len==3) {
        r = concat(a, r, "/");  // Cygwin doubles it for a missing drive
    }
    return r;
}

// Map a normalized absolute POSIX path to its canonical native form in
// msys mode. Null if it has none in this flavour (unknown /dev and
// /proc entries, /dev/tty), so the caller keeps the path as typed.
static Str posixnative(PathCtx *ctx, Arena *a, Str p, i32 how)
{
    b32 dir = p.len && p[p.len-1]=='/';

    if (how != PX_OPEN) {
        Device d = devicename(p);
        if (d.more) return {};
        switch (d.kind) {
        case DV_NONE:   break;
        case DV_NULL:
        case DV_SINK:   return how==PX_ARG ? Str("/Device/Null") : Str("//./NUL");
        case DV_STD:
            if (how != PX_ARG) return {};
            return clone(a, d.num==0 ? Str("/proc/self/fd/0")
                          : d.num==1 ? Str("/proc/self/fd/1")
                          :            Str("/proc/self/fd/2"));
        case DV_FDDIR:
        case DV_FD:
            if (how==PX_ARG && d.kind==DV_FDDIR) return Str("/proc/self/fd");
            if (how!=PX_ARG && d.kind==DV_FD)    return {};
            if (d.kind==DV_FD && d.num<0)        return {};
            {
                Buf b(a, 48);
                print(&b, "/proc/");
                print(&b, ctx->pid);
                print(&b, "/fd");
                if (d.kind == DV_FD) {
                    putbyte(&b, '/');
                    print(&b, d.num);
                }
                return finish(&b);
            }
        case DV_TTY:
        case DV_OTHER:
            return {};
        }

        if (pathprefix("/proc", p, 0)) {
            Str rest = cuthead(p, 5);
            if (pathprefix("/cygdrive", rest, 0) && !(how==PX_ARG && rest=="/cygdrive")) {
                Str pre = cygprefix(ctx);
                Str q   = concat(a, clone(a, takehead(pre, pre.len-1)), cuthead(rest, 9));
                return posixnative(ctx, a, posixnorm(a, q.len ? q : Str("/")), how);
            }
            return {};
        }
    }

    if (p.len>=2 && p[1]=='/') {
        return p;  // UNC
    }

    Str pre = cygprefix(ctx);
    if (pre == "/") {
        if (p.len>=2 && letter(p[1]) && (p.len==2 || p[2]=='/')) {
            return drivepath(ctx, a, p[1], cuthead(p, 2), dir && how!=PX_OPEN);
        }
    } else if (pathprefix(takehead(pre, pre.len-1), p, 0)) {
        Str rest = cuthead(p, pre.len-1);
        if (rest.len>=2 && letter(rest[1]) && (rest.len==2 || rest[2]=='/')) {
            return drivepath(ctx, a, rest[1], cuthead(rest, 2), dir && how!=PX_OPEN);
        }
        return {};  // the drive directory itself, or no such drive
    }

    Mount *best = 0;
    for (iz i = 0; i < ctx->mounts.len; i++) {
        Mount *m = &ctx->mounts[i];
        if (pathprefix(m->posix, p, m->flags&MNT_NOCASE)
                && (!best || m->posix.len>best->posix.len)) {
            best = m;
        }
    }
    Str native = {};
    Str rel    = p;
    if (best) {
        native = best->native;
        rel    = cuthead(p, best->posix.len);
    } else {
        native = nativeroot(ctx->cwd);
        if (native.len && native[native.len-1]=='/') native.len--;
        if (!native.len) native = "C:";
    }
    for (; rel.len && rel[0]=='/'; rel = cuthead(rel, 1)) {}

    Buf b(a, native.len + rel.len + 2);
    print(&b, native);
    if (rel.len || (native.len==2 && native[1]==':')) putbyte(&b, '/');
    print(&b, rel);
    if (dir && b.data[b.len-1]!='/') putbyte(&b, '/');
    return finish(&b);
}

// Does a relative POSIX path climb above its start?
static b32 climbs(Str path)
{
    iz depth = 0;
    for (Cut c = cutpart(path); c.ok; c = cutpart(c.tail)) {
        if (c.head == "..") {
            if (!depth--) return 1;
        } else if (c.head != ".") {
            depth++;
        }
    }
    return 0;
}

// cygpath -u: the POSIX form of a Windows path through the mount table.
// Relative paths stay relative, with \ as /. Null if path is empty.
static Str toposix(PathCtx *ctx, Arena *a, Str path)
{
    if (!path.len) return {};
    Str p = path;
    if (p.len>=4 && p[0]=='\\' && p[1]=='\\' && p[2]=='?' && p[3]=='\\') {
        p = cuthead(p, 4);
        if (p.len>=4 && equalfold(takehead(p, 3), "UNC") && pathsep(p[3])) {
            Buf b(a, p.len);  // \\?\UNC\srv\share: \\srv\share
            print(&b, "\\\\");
            print(&b, cuthead(p, 4));
            p = finish(&b);
        }
    }
    if (p.len>=4 && pathsep(p[0]) && pathsep(p[1]) && p[2]=='.' && pathsep(p[3])) {
        return slashify(a, p, '/');  // the device namespace stays as is
    }
    b32 abs = (p.len && pathsep(p[0])) || (hasdrive(p) && (p.len==2 || pathsep(p[2])));
    if (!abs) {
        return slashify(a, p, '/');
    }

    // Leading separators: one or three or more fold to one, which roots
    // the path on the current drive if \, or leaves it driveless if /.
    iz n = 0;
    for (; n<p.len && pathsep(p[n]); n++) {}
    if (n!=2 && n && p[n-1]=='/') {
        return slashify(a, p, '/');
    }
    Str norm = winnorm(a, n>2 ? cuthead(p, n-1) : p, ctx->cwd);
    b32 appendslash = p.len>1 && pathsep(p[p.len-1]);

    // Mounts in Cygwin's order: longest posix minus native length first
    Mount *best = 0;
    for (iz i = 0; i < ctx->mounts.len; i++) {
        Mount *m = &ctx->mounts[i];
        if (!pathprefix(m->native, norm, m->flags&MNT_NOCASE)) continue;
        iz key  = m->posix.len - m->native.len;
        iz bkey = best ? best->posix.len - best->native.len : 0;
        if (!best || key>bkey || (key==bkey && compare(m->posix, best->posix)<0)) {
            best = m;
        }
    }
    if (best) {
        Str rest  = cuthead(norm, best->native.len);
        Str posix = best->posix;
        Buf b(a, posix.len + rest.len + 2);
        if (rest.len <= 1) {
            print(&b, posix);
            if (appendslash && posix[posix.len-1]!='/') putbyte(&b, '/');
        } else if (rest[0] == '/') {
            print(&b, posix=="/" ? Str{} : posix);
            print(&b, rest);
        } else {
            print(&b, posix);
            putbyte(&b, '/');
            print(&b, rest);
        }
        return finish(&b);
    }

    if (hasdrive(norm)) {
        Str pre = cygprefix(ctx);
        Buf b(a, pre.len + norm.len + 2);
        print(&b, pre);
        putbyte(&b, lowercase(norm[0]));
        if (norm.len > 3) {
            putbyte(&b, '/');
            print(&b, cuthead(norm, 3));
        }
        if (appendslash && b.data[b.len-1]!='/') putbyte(&b, '/');
        return finish(&b);
    }
    return slashify(a, p, '/');
}

// The working directory in POSIX form (msys mode).
static Str posixcwd(PathCtx *ctx, Arena *a)
{
    Str r = toposix(ctx, a, ctx->cwd);
    return r.data ? r : Str("/");
}

// cygpath -w (back) or -m: the Windows form of a POSIX or Windows path.
// Relative paths stay relative unless they climb above the working
// directory. Paths with no Windows form (/proc, /dev/tty) come back as
// typed. Null if path is empty.
static Str towin(PathCtx *ctx, Arena *a, Str path, b32 back)
{
    if (!path.len) return {};
    Str r        = {};
    b32 prefixed = path.len>=4 && pathsep(path[0]) && pathsep(path[1])
                   && (path[2]=='?' || path[2]=='.') && pathsep(path[3]);
    if ((hasdrive(path) && path.len>2 && pathsep(path[2])) || path[0]=='\\'
            || prefixed) {
        r = winnorm(a, path, ctx->cwd);
        if (!r.data) return path;
    } else if (hasdrive(path)) {
        r = slashify(a, path, '/');  // drive-relative: kept as is
        r[0] = upcase(r[0]);
    } else if (path[0] == '/') {
        r = posixnative(ctx, a, posixnorm(a, path), PX_CYGPATH);
        if (!r.data) return path;  // no Windows form: as typed
    } else if (climbs(path)) {
        Str abs = joinposix(a, posixcwd(ctx, a), path);
        r = posixnative(ctx, a, posixnorm(a, abs), PX_CYGPATH);
        if (!r.data) return abs;
    } else {
        Buf b(a, path.len+1);
        addparts(&b, 0, path);
        r = finishparts(&b, 0, dirform(path));
        r = r.len ? r : Str(".");
    }
    return back ? slashify(a, r, '\\') : r;
}

// towin for an absolute result: relative paths start at the working
// directory, as environ.cc's env_path_to_win32 does.
static Str towinabs(PathCtx *ctx, Arena *a, Str path, b32 back)
{
    if (path.len && path[0]!='/' && path[0]!='\\' && !hasdrive(path)) {
        path = joinposix(a, posixcwd(ctx, a), path);
    }
    return towin(ctx, a, path, back);
}


// Path lists

// cygpath -wp/-mp: a POSIX list split on every ':' to a Windows list
// joined by ';'. Empty elements become ".". Null if list is empty.
static Str towinlist(PathCtx *ctx, Arena *a, Str list, b32 back)
{
    if (!list.len) return {};
    Buf b(a, 2*list.len);
    for (Cut c = {.head={}, .tail=list, .ok=1}; c.ok;) {
        c = cut(c.tail, ':');
        Str e = towin(ctx, b.a, c.head.len ? c.head : Str("."), back);
        print(&b, e);
        if (c.ok) putbyte(&b, ';');
    }
    return finish(&b);
}

// cygpath -up: a Windows list split on ';' to a POSIX list joined by
// ':', dropping empty elements. With env (startup PATH import), an
// element's surrounding quotes and trailing backslashes go first.
static Str toposixlist_(PathCtx *ctx, Arena *a, Str list, b32 env)
{
    if (!list.len) return {};
    Buf b(a, 2*list.len);
    b32 first = 1;
    for (Cut c = {.head={}, .tail=list, .ok=1}; c.ok;) {
        c = cut(c.tail, ';');
        Str e = c.head;
        if (env && e.len && e[0]=='"') {
            e = takehead(cuthead(e, 1), e.len>=2 ? e.len-2 : 0);
        }
        if (env) {
            for (; e.len && e[e.len-1]=='\\'; e.len--) {}
        }
        if (!e.len) continue;
        if (!first) putbyte(&b, ':');
        first = 0;
        print(&b, toposix(ctx, b.a, e));
    }
    Str r = finish(&b);
    return r.data ? r : Str("");
}

static Str toposixlist(PathCtx *ctx, Arena *a, Str list)
{
    return toposixlist_(ctx, a, list, 0);
}

// A Windows environment variable's value in POSIX form, as an msys
// process imports it at startup (environ.cc conv_envvars): PATH and
// ORIGINAL_PATH as path lists, LD_LIBRARY_PATH as a plain list, HOME,
// SHELL, TMPDIR, TMP and TEMP as absolute paths. Others unchanged.
static Str envtoposix(PathCtx *ctx, Arena *a, Str name, Str value)
{
    if (!value.len) {
        return value;
    } else if (name=="PATH" || name=="ORIGINAL_PATH") {
        return toposixlist_(ctx, a, value, 1);
    } else if (name == "LD_LIBRARY_PATH") {
        return toposixlist_(ctx, a, value, 0);
    } else if (name=="HOME" || name=="SHELL" || name=="TMPDIR"
               || name=="TMP" || name=="TEMP") {
        b32 abs = pathsep(value[0]) || (hasdrive(value)
                  && (value.len==2 || pathsep(value[2])));
        Str v = abs ? value : joinnative(a, ctx->cwd, value);
        return toposix(ctx, a, v);
    }
    return value;
}


// Opening: canonical native paths and virtual files

// The device a reserved DOS name opens, or null. Classic Win32 rules
// (native mode) ignore an extension and trailing dots and spaces; msys
// mode takes only the bare name, as Git Bash agents write 2>nul.
static Str reservedname(Str name, b32 classic)
{
    static Str names[] = {
        "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5",
        "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4",
        "LPT5", "LPT6", "LPT7", "LPT8", "LPT9", "CONIN$", "CONOUT$",
    };
    Str base = name;
    if (classic) {
        for (; base.len && (base[base.len-1]=='.' || base[base.len-1]==' '); base.len--) {}
        iz i = 0;
        for (; i<base.len && base[i]!='.' && base[i]!=':'; i++) {}
        base = takehead(base, i);
        for (; base.len && base[base.len-1]==' '; base.len--) {}
    }
    for (iz i = 0; i < countof(names); i++) {
        if (equalfold(base, names[i])) return names[i];
    }
    return {};
}

// Settle a canonical native path: reserved names become devices, and
// in msys mode a name Win32 would alter (trailing dot or space, a
// reserved name with an extension) is marked literal.
static PathRes settle(PathCtx *ctx, Arena *a, Str native)
{
    PathRes r = {};
    r.ok     = 1;
    r.native = native;
    if (startswith(native, "//./") || startswith(native, "//?/")) {
        Str dev = reservedname(cuthead(native, 4), 0);
        if (dev.data && startswith(native, "//./")) {
            r.kind = dev=="NUL" ? PK_NULL : dev=="CON" ? PK_TTY : PK_DEVICE;
        }
        r.literal = native[2] == '?';
        return r;
    }
    b32 msys = ctx->mode == PM_MSYS;
    Str last = lastpart(native);
    Str dev  = reservedname(last, !msys);
    if (dev.data) {
        r.kind   = dev=="NUL" ? PK_NULL : dev=="CON" ? PK_TTY : PK_DEVICE;
        r.native = concat(a, Str("//./"), dev);
        return r;
    }
    if (msys) {
        r.literal = reservedname(last, 1).data != 0;
        Str rest  = cuthead(native, nativeroot(native).len);
        for (Cut c = cutpart(rest); c.ok; c = cutpart(c.tail)) {
            u8 e = c.head[c.head.len-1];
            r.literal |= e=='.' || e==' ';
        }
    }
    return r;
}

static PathRes virtualpath(PathCtx *ctx, Arena *a, Str p);

// A drive's working directory in native mode: the =X: variable, else
// the working directory if on that drive, else the drive root.
static Str drivecwd(PathCtx *ctx, Arena *a, u8 drive)
{
    Str d = ctx->drivecwd[upcase(drive) - 'A'];
    if (d.len) return d;
    if (hasdrive(ctx->cwd) && upcase(ctx->cwd[0])==upcase(drive)) return ctx->cwd;
    Str r = clone(a, Str("X:/"));
    r[0] = upcase(drive);
    return r;
}

// Resolve a path for opening: absolute, canonical and native, with
// virtual files (/dev, /proc, reserved DOS names) identified. In POSIX
// mode a relative path is only joined to the working directory.
static PathRes topath(PathCtx *ctx, Arena *a, Str path)
{
    PathRes r = {};
    if (!path.len) return r;

    if (ctx->mode == PM_POSIX) {
        r.ok     = 1;
        r.native = path[0]=='/' ? path : joinposix(a, ctx->cwd, path);
        return r;
    }

    b32 msys  = ctx->mode == PM_MSYS;
    b32 two   = path.len>=2 && pathsep(path[0]) && pathsep(path[1])
                && !pathsep(byteat(path, 2));
    b32 drive = hasdrive(path);
    if (drive && (path.len==2 || !pathsep(path[2]))) {
        // "C:" and "C:x": the drive root in msys mode, else its directory
        Str dir = msys ? Str("X:/") : drivecwd(ctx, a, path[0]);
        if (msys) {
            dir = clone(a, dir);
            dir[0] = upcase(path[0]);
        }
        return settle(ctx, a, joinnative(a, dir, cuthead(path, 2)));
    } else if (drive || two || path[0]=='\\') {
        Str n = winnorm(a, path, ctx->cwd);
        return n.data ? settle(ctx, a, n) : r;
    } else if (path[0] == '/') {
        Str p = posixnorm(a, path);
        r = virtualpath(ctx, a, p);
        if (r.ok || r.kind) return r;
        if (!msys) return settle(ctx, a, winnorm(a, p, ctx->cwd));
        Str n = posixnative(ctx, a, p, PX_OPEN);
        if (!n.data) {
            Str pre = cygprefix(ctx);
            r.ok    = p==pre || p==takehead(pre, pre.len-1);
            r.kind  = r.ok ? PK_CYGDRIVE : PK_FILE;
            return r;  // the cygdrive directory, or no such drive
        }
        return settle(ctx, a, n);
    } else if (msys && hasdotdot(path)) {
        // ".." resolves in the POSIX namespace, across mount points
        Str abs = joinposix(a, posixcwd(ctx, a), path);
        return topath(ctx, a, abs);
    }
    return settle(ctx, a, joinnative(a, ctx->cwd, path));
}

// The virtual file a normalized absolute POSIX path names, if any. A
// result with ok and kind both zero means a real path; kind set without
// ok means a virtual name that does not exist.
static PathRes virtualpath(PathCtx *ctx, Arena *a, Str p)
{
    PathRes r = {};
    b32 msys = ctx->mode == PM_MSYS;
    Device d = devicename(p);
    if (d.kind && !d.more) {
        r.ok = 1;
        switch (d.kind) {
        case DV_NULL:  r.kind = PK_NULL; r.native = "//./NUL"; break;
        case DV_TTY:   r.kind = PK_TTY;  r.native = "//./CON"; break;
        case DV_STD:   r.kind = PK_FD;   r.num = d.num; break;
        case DV_FD:    r.kind = PK_FD;   r.num = d.num; r.ok = d.num >= 0; break;
        case DV_FDDIR:
        case DV_SINK:
        case DV_OTHER: r.kind = PK_DEV;  r.sub = d.name; break;
        }
        if (!msys && r.kind==PK_DEV && d.name!="zero" && d.name!="urandom") {
            r = {};  // busybox emulates only these
        }
        return r;
    } else if (d.kind) {
        r.kind = PK_DEV;  // /dev/null/x: not a directory
        return r;
    }
    if (!msys || !pathprefix("/proc", p, 0)) {
        return r;
    }

    Str rest = cuthead(p, 5);
    if (rest.len && rest[0]=='/') rest = cuthead(rest, 1);
    if (rest.len && rest[rest.len-1]=='/') rest.len--;
    Cut c = cut(rest, '/');
    if (c.head == "cygdrive") {
        Str pre = cygprefix(ctx);  // an alias for the cygdrive prefix
        Str q   = concat(a, clone(a, pre), c.tail);
        return topath(ctx, a, posixnorm(a, q));
    }
    r.kind = PK_PROC;
    r.ok   = 1;
    if (!rest.len) {
        return r;
    } else if (c.head == "self") {
        r.num = -1;
    } else if (pathnum(c.head) > 0) {
        r.num = pathnum(c.head);
    } else {
        r.sub = rest;  // /proc/cpuinfo and the like
        return r;
    }
    r.sub = c.tail;
    if (startswith(c.tail, "fd/") && (r.num<0 || r.num==ctx->pid)) {
        i64 fd = pathnum(cuthead(c.tail, 3));
        if (fd >= 0) {
            r.kind = PK_FD;
            r.num  = fd;
            r.sub  = {};
        }
    }
    return r;
}


// Arguments for native children (msys2_path_conv.cc)

enum : i32 {  // argument path types
    AT_NONE,     // left alone
    AT_SIMPLE,   // copied: C:\x, relative, Windows lists
    AT_ESCAPED,  // first character dropped, \ to /: /C:\x, //c
    AT_UNC,      // \ to /: //srv/share, URLs
    AT_ROOTED,   // /x: through the mount table
    AT_LIST,     // /x:/y: each element, joined by ';'
};

// Like isalnum and isalpha in Cygwin's C locale, which accept bytes
// above 0x7f.
static b32 argalnum(u8 c)
{
    return alnum(c) || c>=0x80;
}

static b32 argalpha(u8 c)
{
    return letter(c) || c>=0x80;
}

static b32 hasbyte(Str s, iz beg, iz end, u8 c)
{
    for (iz i = beg; i < end; i++) {
        if (s[i] == c) return 1;
    }
    return 0;
}

struct ArgType {
    i32 type;
    iz  beg;  // the path starts here; a skipped prefix is copied
};

// Classify s[beg, end) as msys2's find_path_start_and_type does. Peeks
// past end read the underlying string, as the original reads past a
// list element's end.
static ArgType argtype(Str s, iz beg, iz end)
{
    ArgType r = {AT_NONE, end};
    iz      i = beg;
    if (i >= end) {
        r.beg = beg;
        return r;
    }
    if (s[i]=='~' || s[i]==':') return r;

    for (iz j = i; j < end; j++) {
        switch (s[j]) {
        case '`': case '\'': case '*': case '?': case '[': case ']':
            return r;
        case '/':
            if (j+1<end && s[j+1]=='~') return r;
            break;
        case ':':
            if (j+1<end && s[j+1]==':') return r;
            if (j+1<end && s[j+1]=='.') {
                if (j+2<end && s[j+2]=='/') return r;
                if (j+3<end && s[j+2]=='.' && s[j+3]=='/') return r;
            }
            break;
        case '@':
            if (j+1<end && s[j+1]=='@') return r;
            break;
        }
    }

    for (;;) {
        u8 c = byteat(s, i);
        if (argalnum(c) || c=='/' || c=='\\' || c==':' || c=='-' || c=='.') {
            break;
        }
        i++;
        if (i >= end) return r;
    }
    r.beg = i;

    if (i+1 == end) {
        r.type = s[i]=='/' ? AT_ROOTED : AT_SIMPLE;
        return r;
    }
    if (argalpha(s[i]) && byteat(s, i+1)==':') {
        u8 c = byteat(s, i+2);
        if (c == '\\') {
            r.type = AT_SIMPLE;
            return r;
        }
        if (c=='/' && !hasbyte(s, i+2, end, ':')) {
            r.type = AT_SIMPLE;
            return r;
        }
        if (c=='/' && hasbyte(s, i+2, end, ';')) {
            r.type = AT_SIMPLE;
            return r;
        }
    }
    if (s[i]=='.' && (byteat(s, i+1)=='.' || byteat(s, i+1)=='/')
            && !hasbyte(s, i+2, end, ':')) {
        r.type = AT_SIMPLE;
        return r;
    }

    if (s[i] == '/') {
        i++;
        if (argalpha(byteat(s, i)) && byteat(s, i+1)==':') {
            r.type = AT_ESCAPED;
        } else if (byteat(s, i)=='.' && byteat(s, i+1)=='.') {
            r.type = AT_SIMPLE;
        } else if (byteat(s, i) == '/') {
            i++;
            u8 c = byteat(s, i);
            if (c == ':') {
                r.type = AT_UNC;  // a URL
            } else if (c=='/' || !hasbyte(s, i, end, '/')) {
                r.type = AT_ESCAPED;
            } else {
                r.type = AT_UNC;
            }
        } else {
            r.type = AT_ROOTED;
            for (; i<end && s[i]; i++) {
                if (s[i] == ':') {
                    u8 c = byteat(s, i+1);
                    r.type = c=='/' || c==':' || c=='.' ? AT_LIST : AT_SIMPLE;
                    break;
                } else if (s[i] == ';') {
                    r.type = AT_SIMPLE;
                    break;
                }
            }
        }
        return r;
    }

    b32 minus      = 0;
    b32 minusalpha = 0;
    b32 onlydots   = s[i] == '.';
    b32 slashes    = 0;
    if (s[i] == '-') {
        minus = 1;
        i++;
        if (argalpha(byteat(s, i))) {
            i++;
            minusalpha = 1;
            if (hasbyte(s, i, end, ';')) {
                r.type = AT_SIMPLE;
                return r;
            }
        }
    }
    for (iz j = i; j<end && s[j]; j++) {
        u8 c = s[j];
        if (minusalpha) {
            if (argalpha(c) && byteat(s, j+1)==':' && byteat(s, j+2)=='/') {
                r.type = AT_SIMPLE;
                return r;
            }
            if (c=='/' && !hasbyte(s, j, end, ',')) {
                return argtype(s, j, end);
            }
            minusalpha = 0;
        }
        if (c=='\'' || c=='"') {
            minus = 0;
        }
        if (c=='=' || (c==':' && minus) || c=='\'' || c=='"' || (c==',' && minus)) {
            return argtype(s, j+1, end);
        }
        if (c==':' && j+1!=end) {
            c = s[++j];
            if (c=='/' || c==':' || c=='.') {
                if (c=='/' && byteat(s, j+1)=='/') {
                    r.type = AT_UNC;  // a URL
                    return r;
                }
                if (!onlydots && !slashes) {
                    r.beg = end;
                    return r;
                }
                r.type = AT_LIST;
                return r;
            } else if (!hasbyte(s, j, end, '=')) {
                r.type = AT_SIMPLE;
                return r;
            }
        } else if (c != '.') {
            onlydots = 0;
            slashes |= c=='/' || c=='\\';
        }
    }
    r.type = AT_SIMPLE;
    return r;
}

static void argcopy(Buf *b, Str s, iz beg, iz end, b32 flip)
{
    for (iz i = beg; i < end; i++) {
        putbyte(b, flip && s[i]=='\\' ? '/' : s[i]);
    }
}

// Convert a rooted path s[beg, end) for a native child: /dev/null is
// nul, anything with no Windows form stays as typed. A quote ending the
// whole argument is kept out of the path.
static void argrooted(PathCtx *ctx, Buf *b, Str s, iz beg, iz end)
{
    iz realend = end;
    if (end==s.len && end>beg && (s[end-1]=='"' || s[end-1]=='\'')) {
        realend--;
    }
    Str p = span(s.data+beg, s.data+realend);
    if (p == "/dev/null") {
        print(b, "nul");
    } else if (p.len) {
        Str n = posixnative(ctx, b->a, posixnorm(b->a, p), PX_ARG);
        n = n.data ? n : p;
        argcopy(b, n, 0, n.len, 1);
    }
    if (realend != end) putbyte(b, s[realend]);
}

static void argconvert(PathCtx *ctx, Buf *b, Str s, i32 type, iz beg, iz end);

// One element of a POSIX path list, its slashes then made backslashes.
static void argelement(PathCtx *ctx, Buf *b, Str s, iz beg, iz end, b32 url)
{
    ArgType t = url ? ArgType{AT_UNC, beg} : argtype(s, beg, end);
    argcopy(b, s, beg, t.beg, 0);
    if (t.type == AT_NONE) return;
    iz start = b->len;
    argconvert(ctx, b, s, t.type, t.beg, end);
    for (iz i = start; !url && i < b->len; i++) {
        if (b->data[i] == '/') b->data[i] = '\\';
    }
}

static void arglist(PathCtx *ctx, Buf *b, Str s, iz beg, iz end)
{
    iz  ebeg = beg;
    b32 sep  = 0;  // the previous byte was a separating ':'
    b32 url  = 0;
    iz  i    = beg;
    for (; i < end; i++) {
        if (s[i] == ':') {
            if (sep) continue;
            if (byteat(s, i+1)=='/' && byteat(s, i+2)=='/' && argalpha(s[ebeg])) {
                url = 1;
                for (iz j = ebeg; j < i; j++) {
                    url &= argalnum(s[j]) || s[j]=='+';
                }
                if (url) continue;
            }
            sep = 1;
            argelement(ctx, b, s, ebeg, i, url);
            url = 0;
            putbyte(b, ';');
        } else if (sep) {
            sep  = 0;
            ebeg = i;
        }
    }
    if (!sep) argelement(ctx, b, s, ebeg, i, url);
}

static void argconvert(PathCtx *ctx, Buf *b, Str s, i32 type, iz beg, iz end)
{
    switch (type) {
    case AT_NONE:    break;
    case AT_SIMPLE:  argcopy(b, s, beg, end, 0); break;
    case AT_ESCAPED: argcopy(b, s, beg+1, end, 1); break;
    case AT_UNC:     argcopy(b, s, beg, end, 1); break;
    case AT_ROOTED:  argrooted(ctx, b, s, beg, end); break;
    case AT_LIST:    arglist(ctx, b, s, beg, end); break;
    }
}

// Does an exclusion list (';'-separated prefixes, or "*") cover s?
static b32 excluded(Str s, Str excl)
{
    for (Cut c = {.head={}, .tail=excl, .ok=1}; c.ok;) {
        c = cut(c.tail, ';');
        if (c.head=="*" || (c.head.len && startswith(s, c.head))) {
            return 1;
        }
    }
    return 0;
}

// The msys heuristic on one string, without exclusions.
static Str argheuristic(PathCtx *ctx, Arena *a, Str s)
{
    b32 need = 0;
    for (iz i = 0; i < s.len; i++) {
        if (s[i]=='/' || s[i]=='\\') {
            need = 1;
            break;
        }
        if (whitespace(s[i])) break;
    }
    if (!need) return s;

    Buf b(a, s.len + 64);
    iz  end = s.len;
    ArgType t = argtype(s, 0, end);
    if (t.type == AT_ROOTED) {
        // Stop at "/.." (even "/..x"): the rest is copied unconverted
        for (iz i = t.beg; i < end; i++) {
            if (s[i]=='.' && byteat(s, i+1)=='.' && byteat(s, i-1)=='/') {
                end = i - 1;
                break;
            }
        }
    }
    argcopy(&b, s, 0, t.beg, 0);
    argconvert(ctx, &b, s, t.type, t.beg, end);
    argcopy(&b, s, end, s.len, 0);
    Str r = finish(&b);
    if (r == s) return s;

    // Windows dislikes empty list entries: ";;" collapses
    iz n = 0;
    for (iz i = 0; i < r.len; i++) {
        if (r[i]!=';' || !n || r[n-1]!=';') r[n++] = r[i];
    }
    r.len = n;
    return r;
}

// An argument for a native child of an msys shell. excl is the child's
// MSYS2_ARG_CONV_EXCL. The caller skips conversion entirely when the
// child's environment has MSYS_NO_PATHCONV.
static Str msysarg(PathCtx *ctx, Arena *a, Str arg, Str excl)
{
    if (!arg.len || excluded(arg, excl)) return arg;
    return argheuristic(ctx, a, arg);
}

// The value of name in a "NAME=value" list, or null.
static Str envlookup(Slice<Str> env, Str name, b32 nocase)
{
    for (iz i = 0; i < env.len; i++) {
        Cut c = cut(env[i], '=');
        if (c.ok && (nocase ? equalfold(c.head, name) : c.head==name)) {
            return c.tail.data ? c.tail : Str("");
        }
    }
    return {};
}

// Convert a native child's argv, following its environment's
// MSYS_NO_PATHCONV and MSYS2_ARG_CONV_EXCL.
static Slice<Str> msysargv(PathCtx *ctx, Arena *a, Slice<Str> argv, Slice<Str> env)
{
    if (envlookup(env, "MSYS_NO_PATHCONV", 0).data) {
        return argv;
    }
    Str excl = envlookup(env, "MSYS2_ARG_CONV_EXCL", 0);
    Slice<Str> r = {};
    for (iz i = 0; i < argv.len; i++) {
        r = push(a, r, msysarg(ctx, a, argv[i], excl));
    }
    return r;
}

static b32 envless(Str *a, Str *b)  // also sorts the shell's exports
{
    return compare(*a, *b) < 0;
}

// The environment block for a native child of an msys shell, from its
// "NAME=value" entries (environ.cc build_env): conv_envvars back to
// Windows form with backslashes, the argument heuristic on the other
// non-empty values (MSYS2_ENV_CONV_EXCL prefixes of "NAME=value"
// excluded, all of them under MSYS_NO_PATHCONV), TZ dropped unless a
// plain POSIX zone, nameless entries dropped, !X: restored to =X:,
// sorted byte-wise, and PATH=<root>\usr appended if PATH is missing
// or empty. The platform adds SYSTEMROOT and WINDIR if absent (not
// MSYSTEM: spawn.cpp, nativechildenv).
static Slice<Str> msysenv(PathCtx *ctx, Arena *a, Slice<Str> env)
{
    static Str lists[] = {"PATH", "ORIGINAL_PATH", "LD_LIBRARY_PATH"};
    static Str paths[] = {"HOME", "SHELL", "TMPDIR", "TMP", "TEMP"};

    Str excl   = envlookup(env, "MSYS2_ENV_CONV_EXCL", 1);
    b32 noconv = envlookup(env, "MSYS_NO_PATHCONV", 0).data != 0;
    b32 path   = 0;

    Slice<Str> r = {};
    for (iz i = 0; i < env.len; i++) {
        Str e = env[i];
        Cut c = cut(e, '=');
        if (!c.head.len) {
            continue;
        }
        if (equalfold(c.head, "TZ") && c.ok && c.tail.len) {
            b32 keep = c.tail[0] != ':';
            for (iz j = 0; j < c.tail.len; j++) {
                u8 t = c.tail[j];
                keep &= alnum(t) || t=='-' || t=='+' || t==':';
            }
            if (!keep) continue;
        }

        b32 conv = 0;
        for (iz j = 0; c.tail.len && j < countof(lists); j++) {
            if (c.head == lists[j]) {
                Str v = towinlist(ctx, a, c.tail, 1);
                path |= c.head == "PATH";
                e = concat(a, concat(a, clone(a, c.head), "="), v);
                conv = 1;
            }
        }
        for (iz j = 0; c.tail.len && j < countof(paths); j++) {
            if (c.head == paths[j]) {
                Str v = towinabs(ctx, a, c.tail, 1);
                e = concat(a, concat(a, clone(a, c.head), "="), v);
                conv = 1;
            }
        }
        if (!conv && c.tail.len && !noconv && !excluded(e, excl)) {
            e = argheuristic(ctx, a, e);
        }
        if (e.len>=4 && e[0]=='!' && hasdrive(cuthead(e, 1)) && e[3]=='=') {
            e = clone(a, e);
            e[0] = '=';
        }
        r = push(a, r, e);
    }
    sort(r.data, r.len, envless, *a);
    if (!path) {
        Str root = rootnative(ctx);
        root = slashify(a, root.len ? root : Str("C:"), '\\');
        r = push(a, r, concat(a, concat(a, Str("PATH="), root), "\\usr"));
    }
    return r;
}


// Personas

// The path mode a Windows shell takes from its program name: sh, ash
// and lash are native unless the install has an etc/fstab (Git and
// MSYS2, where sh is bash); bash and anything else is msys.
// A program's name without its directory and a .exe suffix.
static Str progbase(Str argv0)
{
    iz i = argv0.len;
    for (; i>0 && !pathsep(argv0[i-1]) && argv0[i-1]!=':'; i--) {}
    Str name = cuthead(argv0, i);
    if (name.len>4 && equalfold(taketail(name, 4), ".exe")) {
        name = cuttail(name, 4);
    }
    return name;
}

static PathMode personaof(Str argv0, b32 fstab)
{
    Str name = progbase(startswith(argv0, "-") ? cuthead(argv0, 1) : argv0);
    b32 sh =equalfold(name, "sh") || equalfold(name, "ash") || equalfold(name, "lash");
    return sh && !fstab ? PM_NATIVE : PM_MSYS;
}


// cygpath (msys2.md §5): the shell's own conversions, so that its
// answers always agree with the shell's view of the mounts

enum : i32 { CYG_UNIX, CYG_WINDOWS, CYG_MIXED };

struct CygOpts {
    Str err;     // what is wrong with the arguments, unless ok
    Str file;    // -f FILE, "-" for standard input
    iz  first;   // index of the first name
    i32 form;    // CYG_*
    b32 abs;     // -a
    b32 list;    // -p
    b32 windir;  // -W
    b32 sysdir;  // -S
    b32 help;    // -h
    b32 ok;
};

// cygpath's arguments, args[0] being its name. Short options cluster,
// long ones are spelled out (--file=FILE too), and -l and -s (long and
// short names) are accepted but change nothing.
static CygOpts cygpathopts(Arena *a, Str *args, iz nargs)
{
    static struct { Str name; u8 letter; } longs[] = {
        {"--absolute", 'a'}, {"--file", 'f'},       {"--help", 'h'},
        {"--long-name", 'l'}, {"--mixed", 'm'},     {"--path", 'p'},
        {"--short-name", 's'}, {"--sysdir", 'S'},   {"--unix", 'u'},
        {"--windir", 'W'},   {"--windows", 'w'},
    };
    CygOpts r = {};
    r.form = -1;
    iz i = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        Str opt = args[i];
        if (opt == "--") {
            i++;
            break;
        }
        Str letters = cuthead(opt, 1);
        Cut value   = {};  // --file=FILE
        if (opt[1] == '-') {
            value   = cut(opt, '=');
            letters = {};
            for (iz k = 0; k < countof(longs); k++) {
                if (longs[k].name == value.head) letters = {&longs[k].letter, 1};
            }
            if (!letters.len || (value.ok && letters[0]!='f')) {
                r.err = concat(a, value.head, ": unknown option");
                return r;
            }
        }
        for (iz k = 0; k < letters.len; k++) {
            i32 form = -1;
            switch (letters[k]) {
            case 'u': form = CYG_UNIX;    break;
            case 'w': form = CYG_WINDOWS; break;
            case 'm': form = CYG_MIXED;   break;
            case 'a': r.abs    = 1;       break;
            case 'p': r.list   = 1;       break;
            case 'W': r.windir = 1;       break;
            case 'S': r.sysdir = 1;       break;
            case 'h': r.help   = 1;       break;
            case 'l': case 's':           break;
            case 'f':
                if (value.ok) {
                    r.file = value.tail;
                } else if (k+1 < letters.len) {
                    r.file = cuthead(letters, k+1);
                } else if (i+1 < nargs) {
                    r.file = args[++i];
                } else {
                    r.err = "-f: a file name is required";
                    return r;
                }
                k = letters.len;
                break;
            default:
                r.err = concat(a, concat(a, "-", takehead(cuthead(letters, k), 1)),
                               ": unknown option");
                return r;
            }
            if (form>=0 && r.form>=0 && form!=r.form) {
                r.err = "only one of -u, -w and -m may be given";
                return r;
            }
            r.form = form>=0 ? form : r.form;
        }
    }
    r.form  = r.form<0 ? CYG_UNIX : r.form;
    r.first = i;
    b32 dir   = r.windir || r.sysdir;
    b32 names = i<nargs || r.file.data;
    if (!r.help && dir && names) {
        r.err = "-W and -S take no names";
    } else if (!r.help && !dir && !names) {
        r.err = "no names to convert";
    } else {
        r.ok = 1;
    }
    return r;
}

// One name converted as the options say, or null for an empty name,
// which cygpath cannot convert.
static Str cygpathname(PathCtx *ctx, Arena *a, CygOpts *o, Str name)
{
    b32 back = o->form == CYG_WINDOWS;
    if (o->list) {
        return o->form==CYG_UNIX ? toposixlist(ctx, a, name)
                                 : towinlist(ctx, a, name, back);
    } else if (!o->abs || !name.len) {
        return o->form==CYG_UNIX ? toposix(ctx, a, name) : towin(ctx, a, name, back);
    }
    Str p = {};
    if (o->form != CYG_UNIX) {
        p = towinabs(ctx, a, name, back);
    } else if (!pathsep(name[0]) && !hasdrive(name)) {
        p = posixnorm(a, joinposix(a, posixcwd(ctx, a), slashify(a, name, '/')));
    } else {
        p = toposix(ctx, a, name);
        p = p.len && p[0]=='/' ? posixnorm(a, p) : p;
    }
    // Made absolute, "." and ".." name the directory without a slash
    b32 root = p.len==1 || (p.len==3 && hasdrive(p));
    if (!pathsep(name[name.len-1]) && !root && p.len && pathsep(p[p.len-1])) {
        p.len--;
    }
    return p;
}

// -W or -S: the Windows directory (windir, from the environment) or the
// system directory under it, in the requested form.
static Str cygpathdir(PathCtx *ctx, Arena *a, CygOpts *o, Str windir)
{
    Str dir = winnorm(a, windir.len ? windir : Str("C:/Windows"), ctx->cwd);
    dir = dir.data ? dir : Str("C:/Windows");
    if (o->sysdir) {
        dir = concat(a, dir, dir[dir.len-1]=='/' ? Str("System32") : Str("/System32"));
    }
    switch (o->form) {
    case CYG_UNIX:    return toposix(ctx, a, dir);
    case CYG_WINDOWS: return slashify(a, dir, '\\');
    }
    return dir;
}

struct CygRun {
    Str out;
    Str err;     // a message for standard error, or null
    i32 status;
};

// Run cygpath over its names, or with -f over the lines of in (the
// file's contents; a CR ending a line is dropped). It stops at the
// first name it cannot convert.
static CygRun cygpathrun(PathCtx *ctx, Arena *a, CygOpts *o, Str *args, iz nargs,
                         Str in, Str windir)
{
    CygRun r = {};
    if (o->windir || o->sysdir) {
        r.out = concat(a, cygpathdir(ctx, a, o, windir), "\n");
        return r;
    }
    Slice<Str> names = {args+o->first, nargs-o->first};
    if (o->file.data) {
        names = {};
        for (Cut c = {.head={}, .tail=in, .ok=1}; c.ok;) {
            c = cut(c.tail, '\n');
            if (!c.ok && !c.head.len) break;
            Str line = c.head;
            if (line.len && line[line.len-1]=='\r') line.len--;
            names = push(a, names, line);
        }
    }
    Slice<Str> outs = {};
    for (iz i = 0; i < names.len; i++) {
        Str s = cygpathname(ctx, a, o, names[i]);
        if (!s.data) {
            r.err    = "cygpath: can't convert empty path\n";
            r.status = 1;
            break;
        }
        outs = push(a, outs, s);
    }
    Buf b(a, 256);
    for (iz i = 0; i < outs.len; i++) {
        print(&b, outs[i]);
        putbyte(&b, '\n');
    }
    r.out = finish(&b);
    return r;
}
