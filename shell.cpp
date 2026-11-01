// Shell state: options, the journaled store, open files, diagnostics
//
// Long-lived state (variables, functions, aliases, positional
// parameters, open files) lives in the store, an arena with size-class
// free lists. A synchronous subshell such as ( ) or $( ) shares the
// store: it records the arena's watermark, logs every write it makes to
// memory below that mark in a journal, and on exit replays the journal
// backwards and resets the arena to the mark. The parent's free lists
// are set aside meanwhile so the child never reuses memory the parent
// still references.
//
// This is free and unencumbered software released into the public domain.


// Options

enum : i32 {  // set -o, in listing order
    O_ALLEXPORT, O_BRACEEXPAND, O_EMACS, O_ERREXIT, O_ERRTRACE,
    O_FUNCTRACE, O_HASHALL, O_HISTEXPAND, O_HISTORY, O_IGNOREEOF,
    O_INTERACTIVECOMMENTS, O_KEYWORD, O_MONITOR, O_NOCLOBBER, O_NOEXEC,
    O_NOGLOB, O_NOLOG, O_NOTIFY, O_NOUNSET, O_ONECMD, O_PHYSICAL,
    O_PIPEFAIL, O_POSIX, O_PRIVILEGED, O_VERBOSE, O_VI, O_XTRACE,
    O_COUNT
};

struct SetOption {
    Str name;
    u8  letter;  // 0 if none
};

static SetOption setoptions[O_COUNT] = {
    {"allexport", 'a'},  {"braceexpand", 'B'}, {"emacs", 0},
    {"errexit", 'e'},    {"errtrace", 'E'},    {"functrace", 'T'},
    {"hashall", 'h'},    {"histexpand", 'H'},  {"history", 0},
    {"ignoreeof", 0},    {"interactive-comments", 0},
    {"keyword", 'k'},    {"monitor", 'm'},     {"noclobber", 'C'},
    {"noexec", 'n'},     {"noglob", 'f'},      {"nolog", 0},
    {"notify", 'b'},     {"nounset", 'u'},     {"onecmd", 't'},
    {"physical", 'P'},   {"pipefail", 0},      {"posix", 0},
    {"privileged", 'p'}, {"verbose", 'v'},     {"vi", 0},
    {"xtrace", 'x'},
};

enum : i32 {  // shopt, in listing order
    S_ARRAY_EXPAND_ONCE, S_ASSOC_EXPAND_ONCE, S_AUTOCD,
    S_BASH_SOURCE_FULLPATH, S_CDABLE_VARS, S_CDSPELL, S_CHECKHASH,
    S_CHECKJOBS, S_CHECKWINSIZE, S_CMDHIST, S_COMPAT31, S_COMPAT32,
    S_COMPAT40, S_COMPAT41, S_COMPAT42, S_COMPAT43, S_COMPAT44,
    S_COMPLETE_FULLQUOTE, S_DIREXPAND, S_DIRSPELL, S_DOTGLOB, S_EXECFAIL,
    S_EXPAND_ALIASES, S_EXTDEBUG, S_EXTGLOB, S_EXTQUOTE, S_FAILGLOB,
    S_FORCE_FIGNORE, S_GLOBASCIIRANGES, S_GLOBSKIPDOTS, S_GLOBSTAR,
    S_GNU_ERRFMT, S_HISTAPPEND, S_HISTREEDIT, S_HISTVERIFY,
    S_HOSTCOMPLETE, S_HUPONEXIT, S_INHERIT_ERREXIT, S_INTERACTIVE_COMMENTS,
    S_LASTPIPE, S_LITHIST, S_LOCALVAR_INHERIT, S_LOCALVAR_UNSET,
    S_LOGIN_SHELL, S_MAILWARN, S_NO_EMPTY_CMD_COMPLETION, S_NOCASEGLOB,
    S_NOCASEMATCH, S_NOEXPAND_TRANSLATION, S_NULLGLOB,
    S_PATSUB_REPLACEMENT, S_PROGCOMP, S_PROGCOMP_ALIAS, S_PROMPTVARS,
    S_RESTRICTED_SHELL, S_SHIFT_VERBOSE, S_SOURCEPATH, S_VARREDIR_CLOSE,
    S_XPG_ECHO,
    S_COUNT
};

static Str shoptnames[S_COUNT] = {
    "array_expand_once", "assoc_expand_once", "autocd",
    "bash_source_fullpath", "cdable_vars", "cdspell", "checkhash",
    "checkjobs", "checkwinsize", "cmdhist", "compat31", "compat32",
    "compat40", "compat41", "compat42", "compat43", "compat44",
    "complete_fullquote", "direxpand", "dirspell", "dotglob", "execfail",
    "expand_aliases", "extdebug", "extglob", "extquote", "failglob",
    "force_fignore", "globasciiranges", "globskipdots", "globstar",
    "gnu_errfmt", "histappend", "histreedit", "histverify",
    "hostcomplete", "huponexit", "inherit_errexit", "interactive_comments",
    "lastpipe", "lithist", "localvar_inherit", "localvar_unset",
    "login_shell", "mailwarn", "no_empty_cmd_completion", "nocaseglob",
    "nocasematch", "noexpand_translation", "nullglob",
    "patsub_replacement", "progcomp", "progcomp_alias", "promptvars",
    "restricted_shell", "shift_verbose", "sourcepath", "varredir_close",
    "xpg_echo",
};


// Store: an arena with size-class free lists, journaled in subshells

enum { NFREE = 28 };

struct Free {
    Free *next;
};

struct alignas(16) Block {  // the store's allocation unit
    byte b[16];
};

struct Journal {
    Journal *next;
    uz      *slot;
    uz       old;
};

struct Store {
    Arena    a;
    byte    *base;          // start of the store's memory
    byte    *mark;          // innermost subshell's watermark, or 0
    Journal *log;           // innermost first
    Free    *free[NFREE];   // by size class: 16 << i bytes
    u32      serial;        // innermost subshell, for deduplication
};

static i32 sizeclass(iz size)
{
    i32 c = 0;
    for (iz s = 16; s < size; s <<= 1) c++;
    return c;
}

// Allocate zeroed store memory, reusing freed blocks of the same class.
static byte *salloc(Store *st, iz size)
{
    i32 c = sizeclass(size);
    if (c < NFREE && st->free[c]) {
        Free *f = st->free[c];
        st->free[c] = f->next;
        __builtin_memset(f, 0, (uz)16<<c);
        return (byte *)f;
    }
    iz real = c<NFREE ? (iz)16<<c : size;
    return (byte *)alloc<Block>(&st->a, (real + 15)/16);
}

// Return a block to its free list. Memory the parent of the current
// subshell still owns is left alone: rollback may need it.
static void sfree(Store *st, void *p, iz size)
{
    i32 c = sizeclass(size);
    if (!p || c>=NFREE || (byte *)p<st->mark) {
        return;
    }
    Free *f = (Free *)p;
    f->next = st->free[c];
    st->free[c] = f;
}

static Str sclone(Store *st, Str s)
{
    Str r = {};
    if (s.data) {
        r.data = (u8 *)salloc(st, s.len?s.len:1);
        r.len  = s.len;
        copybytes(r.data, s.data, s.len);
    }
    return r;
}

static void sfreestr(Store *st, Str s)
{
    if (s.data) sfree(st, s.data, s.len?s.len:1);
}

// Write a pointer-sized slot, logging the old value when the slot
// belongs to the parent of the current subshell.
static void jwrite(Store *st, void *slot, uz value)
{
    uz *p = (uz *)slot;
    if ((byte *)p<st->mark && (byte *)p>=st->base) {
        Journal *j = alloc<Journal>(&st->a);
        j->slot = p;
        j->old  = *p;
        j->next = st->log;
        st->log = j;
    }
    *p = value;
}

template<typename T>
static void jset(Store *st, T **slot, T *value)
{
    jwrite(st, slot, (uz)value);
}

struct Watermark {
    Arena    a;
    byte    *mark;
    Journal *log;
    Free    *free[NFREE];
    u32      serial;
};

static Watermark storeenter(Store *st)
{
    Watermark w = {};
    w.a      = st->a;
    w.mark   = st->mark;
    w.log    = st->log;
    w.serial = st->serial;
    for (i32 i = 0; i < NFREE; i++) {
        w.free[i]   = st->free[i];
        st->free[i] = 0;
    }
    st->mark = st->a.beg;
    st->log  = 0;
    st->serial++;
    return w;
}

static void storeleave(Store *st, Watermark *w)
{
    for (Journal *j = st->log; j; j = j->next) {
        *j->slot = j->old;
    }
    st->a.beg  = w->a.beg;
    st->mark   = w->mark;
    st->log    = w->log;
    st->serial = w->serial;
    for (i32 i = 0; i < NFREE; i++) {
        st->free[i] = w->free[i];
    }
}


// Open files and the descriptor table

enum : i32 { F_OS, F_CAPTURE, F_MEM };

struct File {        // an open file description, shared by descriptors
    i32    kind;
    i32    refs;
    i32    flags;    // OF_READ OF_WRITE OF_APPEND
    Handle h;        // F_OS
    Buf   *cap;      // F_CAPTURE: command substitution output
    Str    mem;      // F_MEM: here-document or pipeline text
    iz     pos;
    b32    ownmem;   // mem is store memory to free with the file
};

enum { FDMAX = 1024, FDUSER = 10 };

struct Fdtab {
    File *fd[FDMAX];
    i32   top;       // one past the highest open descriptor
};

struct Undo {        // a redirection to reverse
    Undo *next;
    i32   fd;
    File *old;       // the descriptor's previous file, or null
};


// Shell

struct Var;
struct Func;
struct Value;

struct Args {        // positional parameters
    Str *v;
    iz   n;
};

struct Frame {       // a function call
    Frame *up;
    Str    name;
    Node  *def;      // the function, for FUNCNAME and LINENO
    i32    line;     // where it was called
    Str    file;     // where the function was defined, for BASH_SOURCE
    Var  **locals;   // variables localized at this level
    iz     nlocals;
    iz     caplocals;
    b32   *opts;     // local -: set options to restore, in the store
    i32    optchar;  // local OPTIND: getopts position to restore
    b32    optsaved;
};

enum : i32 {
    FLOW_NONE, FLOW_BREAK, FLOW_CONTINUE, FLOW_RETURN,
};

enum : i32 {  // reasons to unwind to an escape point
    ESC_EXIT = 1,   // exit builtin, set -e, or a fatal error
    ESC_DISCARD,    // abandon the current command (expansion errors)
    ESC_FATAL,      // abandon the shell (unbound variable, ${x?})
    ESC_ABORT,      // abandon -c, eval and source text (integer assignment)
};

struct Escape {
    void   *jmp[5];
    i32     why;      // ESC_*
    Escape *up;
    Undo   *undo;     // redirections to keep
    Frame  *frame;
    i32     loopdepth;
};

struct BOut {         // output of the running builtin
    Buf b;        // never grows: it shares an arena with the builtin
    b32 failed;   // an early write failed
};

enum { NPOOL = 64 };

// The table of remembered command paths, as hash shows it: bash's
// bucket count and string hash, so that listings come out in its order.
enum { NHASHCMD = 256 };

struct HashCmd {
    HashCmd *next;
    Str      name;
    Str      path;
    uz       hits;   // pointer-sized for jwrite
};

struct Shell {
    Os      *os;
    Store   *store;
    Fdtab   *fds;
    Lexer   *lexer;        // the innermost parser, for its options
    BOut    *bout;
    Arena   *pool[NPOOL];  // scratch arenas for nested captures, by depth
    iz       reserve;      // address space for each pool arena
    i32      capdepth;     // nesting of captures and pipeline stages
    i32      tracedepth;   // nesting of command substitutions, for PS4
    u32      nsubst;       // command substitutions run, for $? after assignments

    // Store roots, copied by value into subshells
    Var     *vars;
    Func    *funcs;
    Alias   *aliases;
    Args     args;
    Str      name;         // $0
    Str      iname;        // input name in syntax errors: -c or a file
    Str      srcname;      // BASH_SOURCE[0]: file of the running code

    // Execution state
    Frame   *frame;
    Undo    *undo;
    Escape  *esc;
    Node    *current;      // simple command being run, for messages
    Node    *cmdnode;      // BASH_COMMAND, printed when read, or
    Str      cmdtext;      // its text once the node may be freed
    i32      line;         // LINENO
    i32      status;       // $?
    i32      flow;
    i32      flowcount;
    i32      loopdepth;
    i32      noerrexit;    // inside a condition: set -e is off
    i32      subshell;     // BASH_SUBSHELL
    i32      evaldepth;
    i32      sourcedepth;
    i32      funcdepth;
    b32      exiting;
    b32      interactive;
    b32      utf8;         // UTF-8 locale, else bytes
    b32      windows;      // native Windows paths (M2's minimal layer)

    // Options
    b32      opt[O_COUNT];
    b32      shopt[S_COUNT];
    b32      cflag;        // started with -c
    b32      sflag;

    // Process
    i64      pid;
    i64      bashpid;      // BASHPID: differs in subshells
    i64      lastpid;      // the last pid handed to a subshell
    Str      argv0;        // store copy of $0 after BASH_ARGV0=
    HashCmd **cmdhash;     // NHASHCMD buckets in the store, or null
    b32      inhandler;    // running command_not_found_handle
    i64      ppid;
    i64      lastbg;       // $!
    i64      uid;
    i64      euid;
    i64      gid;
    i64      egid;
    i64      start;        // ns since the epoch
    u32      seed;         // RANDOM
    Str      cwd;          // physical working directory
    Str      underscore;   // $_
    Str      host;
    i32      umask;
    u64      exportgen;
    byte    *stacktop;     // stack position at sh_main
    iz       stackmax;     // usable stack below it
    i32      optchar;      // getopts: next option letter in $OPTIND's word
    b32      ingetopts;    // its own OPTIND updates keep the position
    Slice<u8> compargs;    // declaration builtin: which args are name=(...)
};


// Diagnostics

// Write to one of the shell's descriptors, through its current file.
static b32 fdwrite(Shell *sh, i32 fd, Str s)
{
    if (fd<0 || fd>=FDMAX) return 0;
    File *f = sh->fds->fd[fd];
    if (!f || !(f->flags & OF_WRITE)) return 0;
    switch (f->kind) {
    case F_OS:
        return !s.len || os_write(sh->os, f->h, s.data, s.len);
    case F_CAPTURE:
        print(f->cap, s);
        return 1;
    case F_MEM:
        return 0;
    }
    return 0;
}

// Write out the running builtin's pending output before a diagnostic,
// keeping the order of bash's line-buffered stdout. On failure it stays
// buffered so that the builtin's final write reports the error.
static void flushbout(Shell *sh)
{
    if (sh->bout && sh->bout->b.len && fdwrite(sh, 1, finish(&sh->bout->b))) {
        sh->bout->b.len = 0;
    }
}

// The name that starts a diagnostic: as bash, the file of the running
// code if sourced, else the script, else $0 (which BASH_ARGV0 sets).
static Str errname(Shell *sh)
{
    if (sh->srcname.len) return sh->srcname;
    b32 script = !sh->cflag && !sh->interactive;
    return script || !sh->argv0.data ? sh->name : sh->argv0;
}

// The message prefix: "NAME: line N: " for scripts and -c, as bash
// prints it, or just "NAME: " when interactive.
static void msgprefix(Shell *sh, Buf *b)
{
    print(b, errname(sh));
    print(b, ": ");
    if (!sh->interactive) {
        print(b, "line ");
        print(b, (i64)sh->line);
        print(b, ": ");
    }
}

// Report an error on the shell's standard error.
static void shellerror(Shell *sh, Arena scratch, Str msg)
{
    flushbout(sh);
    Buf b(&scratch, msg.len+64);
    msgprefix(sh, &b);
    print(&b, msg);
    putbyte(&b, '\n');
    fdwrite(sh, 2, finish(&b));
}

// Report "NAME: line N: CMD: MSG", as builtins do.
static void builtinerror(Shell *sh, Arena scratch, Str cmd, Str msg)
{
    flushbout(sh);
    Buf b(&scratch, msg.len+cmd.len+64);
    msgprefix(sh, &b);
    if (cmd.len) {
        print(&b, cmd);
        print(&b, ": ");
    }
    print(&b, msg);
    putbyte(&b, '\n');
    fdwrite(sh, 2, finish(&b));
}

static Str concat3(Arena *a, Str x, Str y, Str z)
{
    Buf b(a, x.len+y.len+z.len);
    print(&b, x);
    print(&b, y);
    print(&b, z);
    return finish(&b);
}

// glibc's strerror text for the errno values the platforms report.
static Str errtext(i32 err)
{
    switch (err) {
    case E_PERM:   return "Operation not permitted";
    case E_NOENT:  return "No such file or directory";
    case E_SRCH:   return "No such process";
    case E_INTR:   return "Interrupted system call";
    case E_IO:     return "Input/output error";
    case E_NXIO:   return "No such device or address";
    case E_2BIG:   return "Argument list too long";
    case E_NOEXEC: return "Exec format error";
    case E_BADF:   return "Bad file descriptor";
    case E_CHILD:  return "No child processes";
    case E_AGAIN:  return "Resource temporarily unavailable";
    case E_NOMEM:  return "Cannot allocate memory";
    case E_ACCES:  return "Permission denied";
    case E_FAULT:  return "Bad address";
    case E_BUSY:   return "Device or resource busy";
    case E_EXIST:  return "File exists";
    case E_XDEV:   return "Invalid cross-device link";
    case E_NODEV:  return "No such device";
    case E_NOTDIR: return "Not a directory";
    case E_ISDIR:  return "Is a directory";
    case E_INVAL:  return "Invalid argument";
    case E_NFILE:  return "Too many open files in system";
    case E_MFILE:  return "Too many open files";
    case E_NOTTY:  return "Inappropriate ioctl for device";
    case E_TXTBSY: return "Text file busy";
    case E_FBIG:   return "File too large";
    case E_NOSPC:  return "No space left on device";
    case E_SPIPE:  return "Illegal seek";
    case E_ROFS:   return "Read-only file system";
    case E_MLINK:  return "Too many links";
    case E_PIPE:   return "Broken pipe";
    case E_DOM:    return "Numerical argument out of domain";
    case E_RANGE:  return "Numerical result out of range";
    case E_NAMETOOLONG: return "File name too long";
    case E_NOSYS:  return "Function not implemented";
    case E_NOTEMPTY: return "Directory not empty";
    case E_LOOP:   return "Too many levels of symbolic links";
    }
    return "Unknown error";
}


// Files

static File *newfile(Shell *sh, i32 kind, i32 flags)
{
    File *f = (File *)salloc(sh->store, sizeof(File));
    f->kind  = kind;
    f->flags = flags;
    f->refs  = 1;
    return f;
}

static void fileunref(Shell *sh, File *f)
{
    if (!f || --f->refs) return;
    if (f->kind == F_OS) {
        os_close(sh->os, f->h);
    }
    if (f->ownmem) {
        sfreestr(sh->store, f->mem);
    }
    sfree(sh->store, f, sizeof(File));
}

// Point descriptor fd at f, taking over the caller's reference. With
// keep set the change is permanent (exec), otherwise it is recorded in
// the undo list.
static void fdset(Shell *sh, i32 fd, File *f, b32 keep, Arena *scratch)
{
    Fdtab *t = sh->fds;
    if (keep && !sh->subshell) {
        fileunref(sh, t->fd[fd]);
    } else {
        // A permanent change in a subshell is undone when it exits, so
        // its record outlives the command's scratch
        Undo *u = keep ? (Undo *)salloc(sh->store, sizeof(Undo)) : alloc<Undo>(scratch);
        u->fd   = fd;
        u->old  = t->fd[fd];
        u->next = sh->undo;
        sh->undo = u;
    }
    t->fd[fd] = f;
    if (f && fd>=t->top) {
        t->top = fd + 1;
    }
}

// Reverse redirections back to a saved point.
static void unwind(Shell *sh, Undo *to)
{
    Fdtab *t = sh->fds;
    while (sh->undo != to) {
        Undo *u = sh->undo;
        fileunref(sh, t->fd[u->fd]);
        t->fd[u->fd] = u->old;
        sh->undo = u->next;
    }
    while (t->top && !t->fd[t->top-1]) {
        t->top--;
    }
}

// Make the redirections recorded since a saved point permanent, as if
// made with keep set outside a subshell.
static void commitredirs(Shell *sh, Undo *to)
{
    while (sh->undo != to) {
        Undo *u = sh->undo;
        fileunref(sh, u->old);
        sh->undo = u->next;
    }
}

// Read up to len bytes from a file or descriptor. Returns 0 at end of
// file and -1 on error.
static iz fileread(Shell *sh, File *f, u8 *buf, iz len)
{
    if (!f || !(f->flags & OF_READ)) return -1;
    switch (f->kind) {
    case F_OS:
        return os_read(sh->os, f->h, buf, len);
    case F_MEM: {
        iz n = f->mem.len - f->pos;
        n = n<len ? n : len;
        copybytes(buf, f->mem.data+f->pos, n);
        f->pos += n;
        return n;
    }
    case F_CAPTURE:
        return -1;
    }
    return -1;
}

static iz fdread(Shell *sh, i32 fd, u8 *buf, iz len)
{
    return fd<0 || fd>=FDMAX ? -1 : fileread(sh, sh->fds->fd[fd], buf, len);
}

// Unwind to the innermost escape point.
[[noreturn]] static void escape(Shell *sh, i32 why)
{
    sh->esc->why = why;
    __builtin_longjmp(sh->esc->jmp, 1);
}
