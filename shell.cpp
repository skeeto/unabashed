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
    O_FUNCTRACE, O_HASHALL, O_HISTEXPAND, O_HISTORY, O_IGNCR, O_IGNOREEOF,
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
    {"igncr", 0},        {"ignoreeof", 0},     {"interactive-comments", 0},
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
    S_COMPLETION_STRIP_EXE, S_COMPLETE_FULLQUOTE, S_DIREXPAND, S_DIRSPELL, S_DOTGLOB, S_EXECFAIL,
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
    "completion_strip_exe", "complete_fullquote", "direxpand", "dirspell", "dotglob", "execfail",
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
    byte    *floor;         // outermost subshell's watermark, or 0
    b32      vproc;         // a vproc's: may end before vprocs it starts
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
    st->mark  = st->a.beg;
    st->floor = st->floor ? st->floor : st->mark;
    st->log   = 0;
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
    st->floor  = w->mark ? st->floor : 0;
    st->log    = w->log;
    st->serial = w->serial;
    for (i32 i = 0; i < NFREE; i++) {
        st->free[i] = w->free[i];
    }
}


// Open files and the descriptor table

enum : i32 { F_OS, F_CAPTURE, F_MEM, F_PIPE };

// An open file description, shared by descriptors. Files belong to one
// vproc's store: another vproc gets its own File for the same object
// (see vproccopy), with a duplicated handle or another Pipe end.
struct File {
    i32    kind;
    i32    refs;
    i32    flags;    // OF_READ OF_WRITE OF_APPEND
    i32    tty;      // F_OS: isatty, cached by fileistty (0 unknown)
    i32    seek;     // F_OS: seekable, cached by fileseekable (0 unknown)
    Handle h;        // F_OS
    Buf   *cap;      // F_CAPTURE: command substitution output
    Str    mem;      // F_MEM: here-document or pipeline text
    Pipe  *pipe;     // F_PIPE: the read end with OF_READ, else the write end
    iz     pos;
    b32    ownmem;   // mem is store memory to free with the file
};

// A command substitution's output: what its File (F_CAPTURE) points at.
// Writers that outlive a synchronous call, such as a program started
// without waiting, get an OS pipe whose pump appends here; the capture
// ends once every pump has seen EOF from every writer (capturewriter).
struct Capture {
    Buf    b;        // first, so a File's Buf * is the Capture
    Task  *pumps;    // linked by Task.sib
    Wtrace wtrace;   // chaos: its last writer
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
    b32   psub;      // a procsub's /dev/fd/N: in the store, freed on unwind
};

struct Fdsave {      // descriptors to restore when a subshell ends
    File **fd;
    i32    top;
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
    Str    file;     // where the function was defined, for messages
    Str    src;      // the same for BASH_SOURCE (Code.src)
    Args   args;     // its arguments, for BASH_ARGV and BASH_ARGC
    b32    argpushed;  // there (extdebug was on at the call)
    Var  **locals;   // variables localized at this level
    iz     nlocals;
    iz     caplocals;
    b32   *opts;     // local -: set options to restore, in the store
    i32    optchar;  // local OPTIND: getopts position to restore
    b32    optsaved;
    u8    *trapat[3];  // DEBUG, ERR and RETURN actions at the call: those
                       // run in it only if it set them (or with -T/-E)
};

struct Srcframe {    // a file being sourced: a frame of FUNCNAME ("source"),
    Srcframe *up;    // BASH_SOURCE and BASH_LINENO, as bash
    Frame    *frame; // the function call it runs in, if any
    Str       file;
    i32       line;  // where it was sourced
    Args      args;  // in BASH_ARGV and BASH_ARGC when argpushed
    b32       argpushed;
    u8       *trapat[3];  // DEBUG, ERR and RETURN actions at the start: as
                          // in a function, only its own run in it (or -T)
};

enum : i32 {
    FLOW_NONE, FLOW_BREAK, FLOW_CONTINUE, FLOW_RETURN,
};

enum : i32 {  // reasons to unwind to an escape point
    ESC_EXIT = 1,   // exit builtin, set -e, or a fatal error
    ESC_DISCARD,    // abandon the current command (expansion errors)
    ESC_FATAL,      // abandon the shell (unbound variable, ${x?})
    ESC_ABORT,      // abandon -c, eval and source text (integer assignment)
    ESC_INTR,       // interactive: Ctrl-C abandons the command line
};

struct BOut {         // output of the running builtin
    Buf b;        // never grows: it shares an arena with the builtin
    b32 failed;   // an early write failed
};

struct Escape {
    void   *jmp[5];
    i32     why;      // ESC_*
    Escape *up;
    Undo   *undo;     // redirections to keep
    Frame  *frame;
    i32     loopdepth;
    BOut   *bout;     // builtin output current when it was set up
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

// Where runprogram leaves a program it started without waiting, for a
// pipeline stage spawned directly (runpipeline).
struct Spawnto {
    Handle proc;   // BADHANDLE if it did not start
    i64    pid;
    b32    retry;  // not a program after all: run the stage as a vproc
};

enum : i32 {  // the trap table: EXIT, signals 1-64, then the others
    TRAP_EXIT   = 0,
    TRAP_DEBUG  = NSIGNAL,
    TRAP_ERR,
    TRAP_RETURN,
    NTRAP
};

// Background jobs and the coprocess (jobs.cpp)
struct Job {
    Job   *next;      // the next older job
    i64    owner;     // BASHPID of the level that started it
    i64    pid;       // its $!
    Task  *task;      // until joined: its vproc, or its program's waiter
    Str    text;      // the command, as jobs lists it
    Str    wd;        // where it started
    i32    id;        // its job number
    i32    status;    // once done: exit status, WSIGNALED for a signal
    b32    done;
    b32    notified;  // listed as done, or waited for
    b32    nohup;     // disown -h
    b32    jc;        // started under job control
    i64    pgid;      // its process group, or 0 if none of its own
    i32    stopsig;   // what stopped it, while stopped
    b32    stopped;
};

struct Bgstat {       // the status of a job no longer listed
    Bgstat *next;
    i64     pid;
    i32     status;
};

struct Coproc {       // bash 5.3 allows several
    Coproc *next;     // the next older
    Str  name;
    i64  owner;
    i64  pid;
    i32  rfd;         // the shell's ends, -1 once closed
    i32  wfd;
    b32  dead;
};

struct SniffCache;  // spawn.cpp
struct Editor;      // edit.cpp
struct CompTable;   // pcomplete.cpp
struct CompSpec;

// The command history (history.cpp)
enum : i32 { HC_IGNSPACE = 1<<0, HC_IGNDUPS = 1<<1, HC_ERASEDUPS = 1<<2 };

struct EdUndo;      // edit.cpp

struct HistEnt {   // in the store, never changed once made
    Str     line;
    Str     stamp;   // the comment character and epoch seconds, or empty
    b32     edited;  // changed in the editor: * in listings
    Str     orig;    // with undo: the line before the editor changed it
    EdUndo *undo;    // the editor's undo list back to orig
};

struct Hist {      // in the Shell by value: a subshell's changes roll back
    HistEnt **ents;    // ents[0..len), the oldest first
    iz        len;
    iz        cap;
    i64       base;    // the number of ents[0]
    i64       max;     // HISTSIZE's limit, or -1 for none
    i64       session; // entries added and not yet written
    i64       infile;  // lines of HISTFILE read or written
    i64       readlines;   // lines the last read of a file went through
    i64       multiline;   // timestamped files read with HISTTIMEFORMAT
    i32       control; // HISTCONTROL: HC_*
    i32       cmdlines;    // lines of the command being read
    i32       commentline; // the last line read that held a comment, or -2
    b32       firstsaved;  // its first line was added
    b32       lastadded;   // the last line read was added as an entry
    b32       lastpushed;  // that entry came from history -s
    b32       lastheredoc; // a here-document starts on the last line
    b32       expinhibit;  // history expansion off with history
    b32       ready;       // set up: variables now steer it
    b32       writets;     // HISTTIMEFORMAT set: files get time stamps
    u8        expchar;     // histchars
    u8        subchar;
    u8        comchar;
    // History expansion's memory
    Str       search;      // the last !?string?
    Str       subold;      // and :s/old/new/
    Str       subnew;
    iz        matchat;     // where !?string? matched, for %
    Str       preload;     // the editor's next line (histverify, histreedit)
    b32       noexpand;    // reading -c's string: no expansion
};

// editline results, and what read -e asks of it (edit.cpp). ED_ABORT:
// an error abandoned the line, its message ending on a new line.
enum : i32 { ED_LINE, ED_EOF, ED_INTR, ED_NOTTY, ED_ABORT, ED_TIMEOUT };

struct EdRead {   // what read -e asks of the editor
    Str init;     // -i: the line to start with
    i32 fd;       // the input descriptor
    i32 delim;    // -d: accepts the line, or -1 for none (a newline)
    i64 nchars;   // -n: accept at this many characters, 0 for no limit
    b32 silent;   // -s: the line is not shown
    b32 shell;    // not read -e: the shell's own line, with init preloaded
    b32 ps1;      // and the first of a command
    i64 deadline; // -t: os_now time when reading gives up, or 0
};

struct Shell {
    Os      *os;
    Sched   *sched;        // shared by every vproc of the process
    Store   *store;
    Fdtab   *fds;
    Lexer   *lexer;        // the innermost parser, for its options
    BOut    *bout;
    Spawnto *spawnto;      // start the next program without waiting
    Arena   *pool[NPOOL];  // scratch arenas for nested captures, by depth
    iz       reserve;      // address space for each pool arena
    i32      capdepth;     // nesting of captures and pipeline stages
    i32      tracedepth;   // nesting of command substitutions, for PS4
    u32      nsubst;       // command substitutions run, for $? after assignments

    // Store roots, copied by value into subshells
    Var     *vars;
    Func    *funcs;
    Alias   *aliases;
    uz       aliasseq;     // aliases defined so far
    Args     args;
    Str      name;         // $0
    Str      shellname;    // argv[0], for \s in prompts
    Str      iname;        // input name in syntax errors: -c or a file
    Str      srcname;      // BASH_SOURCE[0]: file of the running code
    Str      defsrc;       // a function's origin while importing one

    // Execution state
    Frame   *frame;
    Srcframe *srcframes;   // files being sourced, innermost first
    Srcframe *retsrc;      // the one whose RETURN action runs
    Args     argvbase;     // the bottom of BASH_ARGV, in the store
    b32      argvinit;     // argvbase is set
    b32      listing;      // listing every variable: BASH_ARGV stays as is
    u64      dynread;      // dynamic variables read so far, by DYN_ bit
    i32      lastrandom;   // RANDOM's last value, for listings
    i64      lastlineno;   // and LINENO's
    Str     *badenv;       // inherited entries that name no variable
    iz       nbadenv;
    Undo    *undo;
    Escape  *esc;
    Node    *current;      // simple command being run, for messages
    Node    *cmdnode;      // BASH_COMMAND, printed when read, or
    Str      cmdtext;      // its text once the node may be freed
    i32      line;         // LINENO
    i32      ctlline;      // bash's line as only some commands set it
                           // (top level, calls, for, case, select)
    i32      status;       // $?
    i32      npstat;       // PIPESTATUS: the last pipeline's statuses
    u8       pstat[256];
    b32      pstatdirty;   // PIPESTATUS assigned or unset meanwhile
    i32      flow;
    i32      flowcount;
    i32      loopdepth;
    i32      noerrexit;    // inside a condition: set -e is off
    i32      subshell;     // subshell depth
    i32      procbase;     // subshell depth of this process's outermost
                           // level: a background job's, in its process
    i32      simplestages; // of which simple commands as pipeline stages,
                           // which BASH_SUBSHELL leaves out, as bash
    i32      evaldepth;
    b32      lexeof;       // the last parse read its input's end (bash's
                           // shared current_token): see resumeaftereof
    i32      sourcedepth;
    i32      funcdepth;
    b32      nowriteerr;   // the builtin ignores a failed write (help)
    b32      viacommand;   // a builtin started by `command`, until it
                           // reads this (POSIX mode's fatal errors)
    b32      restricted;   // set -r, rbash: the restrictions apply now
    b32      rshell;       // started restricted (shopt restricted_shell)
    i32      dumpstrings;  // 1: -D, --dump-strings; 2: in po form too
    b32      prettyprint;  // --pretty-print: print the script, not run it
    i32      redirfail;    // the status of the last failed redirection
    b32      argsset;      // set replaced $@ outside functions, which a
                           // sourced file with arguments then keeps
    b32      exiting;
    b32      interactive;
    b32      login;        // a login shell (logout, ~/.bash_logout)
    b32      loggedout;    // ~/.bash_logout has run
    i32      retprev;      // the status before the last return builtin
    b32      utf8;         // UTF-8 locale, else bytes
    u8       radix;        // LC_NUMERIC's decimal point, 0 for '.'
    b32      win32;        // on Windows: platform facts, not paths
    PathMode pathmode;     // how paths read and print (paths.cpp)
    PathCtx *paths;        // the mount table and the rest, from startup
    Str      exe;          // the shell's own image, native (OsInfo.exe)
    SniffCache *sniffs;    // Windows: what files hold, shared by subshells
    Editor     *editor;    // the line editor, made on first use (edit.cpp)
    CompTable  *comps;     // compspecs (pcomplete.cpp)
    CompSpec   *compcur;   // the running completion's copy, for compopt
    Str         compcurkey;
    i32         comptype;  // COMP_TYPE and COMP_KEY of the last completion
    i32         compkey;
    Hist        hist;      // command history (history.cpp)
    i64         cmdnumber; // \# in prompts: commands the reader ran, plus one
    i32         promptkind;  // the prompt decoding: 1 PS1, 2 PS2, 3 PS0,
                             // 4 PS4, or 0 (${x@P} alone)

    // Options
    b32      opt[O_COUNT];
    b32      shopt[S_COUNT];
    b32      posixsaved;     // posixshopts below hold the settings
    b32      posixshopts[5]; // before a temporary POSIXLY_CORRECT
    b32      cflag;        // started with -c
    b32      sflag;

    // Process
    i64      pid;
    i64      bashpid;      // BASHPID: differs in subshells
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
    Str      cwd;          // native working directory, for the platform
                           // (empty: a virtual msys directory, as /proc)
    Str      wd;           // logical working directory (bash's
                           // the_current_working_directory), as PWD shows
    Str      host;
    Str      hosts;        // completion's host names, one per line,
    b32      hostsread;    // kept from HOSTFILE once there are any
    Str     *dirs;         // the directory stack under the working
    iz       ndirs;        // directory, bottom first (vars.cpp)
    i32      umask;
    u64      disabled[2];  // enable -n: builtins by registry index
    Rlimit   rl[RL_COUNT]; // ulimit's table, read once (rlhave); in a
    b32      rlhave;       // subshell it changes alone and programs get
    b32      rlmod;        // it (rlmod), as bash's forked subshell's
    u64      exportgen;
    byte    *stacktop;     // stack position at sh_main
    iz       stackmax;     // usable stack below it
    i32      optchar;      // getopts: next option letter in $OPTIND's word
    b32      ingetopts;    // its own OPTIND updates keep the position
    Slice<u8> compargs;    // declaration builtin: which args are name=(...)
    u8       cmdname[32];  // bash's this_command_name: a function's name
    i32      tracefd;      // BASH_XTRACEFD, or 2 (or 0 when never set)
    Str      lastarg;      // $_, in a store buffer of lastargcap bytes,
    iz       lastargcap;   // read through the dynamic variable _ while
    b32      lastargdirty; // no one assigned, unset or declared _
    i32      cmdnamelen;   // from its call until a simple command ends
    Str      bltname;      // the builtin running (or "((") for messages
    Str     *bltargs;      // and its arguments, which compargs describes
    iz       nbltargs;
    Task    *psubs;        // procsubs started at this subshell level, to join
    i64      psubpid;      // the last procsub joined, and its status,
    i32      psubstatus;   // for a repeated wait
    Job     *jobs;         // background jobs, newest first (jobs.cpp)
    i32      curjob;       // job numbers of %+ and %-, or 0
    i32      prevjob;
    Bgstat  *bgstats;      // statuses of jobs no longer listed, for wait
    Coproc  *coprocs;      // coprocesses, newest first
    b32      stdinredir;   // input is a pipe or redirected: & keeps it,
                           // rather than reading /dev/null
    b32      jobctl;       // job control is active: set -m, but not in a
                           // subshell other than a command substitution
    b32      incomsub;     // in a command substitution: fg and bg find
                           // no job of their own
    b32      forked;       // the next simple command is a stage or job
                           // bash forks first: %N is no command there
    b32      asyncpct;     // the next simple command is "%N &", so bg
    i64      shellpgid;    // the shell's process group under job control
                           // (once set -m turned it on), else 0: no groups
    i64      startpgid;    // the group it started in, or 0 if the
                           // platform has none (OsInfo.pgid)
    Handle   jctty;        // with shellpgid: the terminal it hands to
                           // foreground jobs, or BADHANDLE
    i64      spawngrp;     // Spawn.pgid for the job being started
    b32      spawnfg;      // and it takes the terminal (a pipeline's first)
    b32      pipetail;     // the program is a grouped pipeline's last:
                           // its stop is reported (Sched.stopproc)
    i64      exitwarned;   // cmdnumber when exit warned of stopped jobs
    b32      bifatal;      // the builtin's error ends a POSIX shell, if
                           // the builtin is special (usage, identifier)

    // Signals and traps (trap.cpp)
    Str     *traps;        // NTRAP actions in the store, or null
    Str     *shown;        // what trap lists in a subshell until it sets one
    Sigproc *sig;          // this level's pending signals
    u64      hardign;      // ignored on entry: cannot be trapped
    u64      entryign;     // ignored on entry, as programs start
    u64      asyncign;     // INT and QUIT in a background job: ignored
                           // unless trapped, as bash without job control
    i32      intrap;       // running trap actions
    i32      trapbusy;     // DEBUG, ERR, RETURN actions running: not again
    b32      debugdone;    // the parent ran DEBUG for this stage's command
    i32      trapstatus;   // $? before the running trap, for its exit
    i32      trapfuncs;    // funcdepth where it runs, for its return
    i32      killsig;      // this level is dying by this signal
    i32      childsig;     // the last foreground child died by this signal
    i64      childpid;     // and had this pid
    i32      taillevel;    // subshell level + 1 where tailnode is the last
    Node    *tailnode;     // command, which bash would exec: its death by a
                           // signal is the subshell's (reported by its parent)
    b32      attail;       // running tailnode as bash would exec it
    Node    *keepredirs;   // a background job's command: its redirections
                           // are not undone, as in bash's forked child, so
                           // a descriptor they replace closes at once
    b32      noreport;     // in command and process substitutions: no reports
    b32      intrread;     // in the read builtin: trap actions run at once
    b32      nolocwarn;    // binding a program's temporary environment
    b32      lcall;        // LC_ALL assigned, not empty: categories quiet
    Str     *undonames;    // a program's temporary environment, undone
    iz       nundo;        // ... once it is spawned (runprogram)
    Str     *undoargv;     // ... for this argv only
    b32      intredit;     // in the editor's read: a signal ends it (-4)
    i32      chldtraps;    // CHLD trap actions owed, one per child reaped
    b32      inchld;       // running one of them
    b32      execed;       // replaced by exec: no EXIT trap
    b32      exitcalled;   // the exit builtin: EXIT trap in the function
    b32      execing;      // exec's program may replace the process
    b32      errexited;    // set -e is ending this level
    i32      writeerr;     // why the last fdwrite failed
};

// Cygwin's extra options, set -o igncr and shopt completion_strip_exe,
// exist only in msys mode, as in Git Bash. Elsewhere the lists, names
// and SHELLOPTS/BASHOPTS are bash 5.3's.
static b32 hasoption(Shell *sh, i32 k)
{
    return k!=O_IGNCR || sh->pathmode==PM_MSYS;
}

static b32 hasshopt(Shell *sh, i32 k)
{
    return k!=S_COMPLETION_STRIP_EXE || sh->pathmode==PM_MSYS;
}

// The options a shell starts with, which a script without #! also
// starts from (as bash's reinitialized subshell); with fresh, those a
// shell keeps across that, as bash, survive: the editing mode, history,
// igncr, login_shell and completion_strip_exe.
static void defaultoptions(Shell *sh, b32 fresh)
{
    static i32 optkeep[] = {O_EMACS, O_HISTORY, O_IGNCR, O_NOLOG, O_VI};
    static i32 shoptkeep[] = {S_COMPLETION_STRIP_EXE, S_LOGIN_SHELL};
    static i32 shoptson[] = {
        S_CHECKWINSIZE, S_CMDHIST, S_COMPLETE_FULLQUOTE, S_EXTQUOTE,
        S_FORCE_FIGNORE, S_GLOBASCIIRANGES, S_GLOBSKIPDOTS, S_HOSTCOMPLETE,
        S_INTERACTIVE_COMMENTS, S_PATSUB_REPLACEMENT, S_PROGCOMP,
        S_PROMPTVARS, S_SOURCEPATH,
    };
    b32 opt[O_COUNT]   = {};
    b32 shopt[S_COUNT] = {};
    if (fresh) {
        for (iz i = 0; i < countof(optkeep); i++) {
            opt[optkeep[i]] = sh->opt[optkeep[i]];
        }
        for (iz i = 0; i < countof(shoptkeep); i++) {
            shopt[shoptkeep[i]] = sh->shopt[shoptkeep[i]];
        }
    }
    for (i32 i = 0; i < O_COUNT; i++) sh->opt[i] = opt[i];
    for (i32 i = 0; i < S_COUNT; i++) sh->shopt[i] = shopt[i];
    sh->opt[O_BRACEEXPAND] = 1;
    sh->opt[O_HASHALL]     = 1;
    sh->opt[O_INTERACTIVECOMMENTS] = 1;
    for (iz i = 0; i < countof(shoptson); i++) {
        sh->shopt[shoptson[i]] = 1;
    }
    sh->posixsaved = 0;
}

// set -o igncr in effect: CRs vanish from command substitution output
// and from what read splits, as in Cygwin's bash.
static b32 igncr(Shell *sh)
{
    return sh->opt[O_IGNCR] && sh->pathmode==PM_MSYS;
}

// What the parser drops from its input: every CR in msys mode, each CR
// of a CRLF pair in native mode, nothing in POSIX mode.
static i32 crmode(Shell *sh)
{
    switch (sh->pathmode) {
    case PM_MSYS:   return CR_ALL;
    case PM_NATIVE: return CR_CRLF;
    case PM_POSIX:  return CR_KEEP;
    }
    return CR_KEEP;
}


// Diagnostics

[[noreturn]] static void escape(Shell *, i32 why);

// Before the shell appends to a capture that programs also write
// through pumps, let the pumps append whatever has arrived, so that
// output keeps the order in which it was written.
static void capturedrain(Sched *s, Capture *c)
{
    for (;;) {
        while (s->npending) {
            Io *io = os_wait(s->os, WAIT_POLL);
            if (!io) break;
            s->npending--;
            wake(s, (Task *)io->ctx);
        }
        b32 ready = 0;
        for (Task *t = c->pumps; t; t = t->sib) {
            ready |= t->state == TS_READY;
        }
        if (!ready) return;
        yield(s);
    }
}

static b32  sigtaken(Shell *);
static void waitdone(Shell *, u64 died, b32 failed);
static void takeossigs(Shell *);
static void sigfatal(Shell *);
static u64  sigbit(i32);
static void runsigs(Shell *, Arena);
static void runexittrap(Shell *, Arena);

// A write to a pipe without readers: as a process killed by SIGPIPE,
// the vproc ends silently with status 141. With a trap the write fails
// (EPIPE) and the trap runs at the next safe point; ignored, it fails.
// But a signal already sent to this level that would end it wins: a
// stage takes Ctrl-C only at a safe point, and its reader may die of
// it first, where in bash the terminal signals every stage at once.
static void sigpipe(Shell *sh)
{
    takeossigs(sh);
    sigfatal(sh);
    Str *t = sh->traps;
    if (t && t[13].data) {
        if (t[13].len) sh->sig->pending |= (u64)1 << 12;
        return;
    }
    if (sh->hardign & ((u64)1 << 12)) return;
    sh->status  = 128 + 13;
    sh->exiting = 1;
    sh->killsig = 13;
    escape(sh, ESC_EXIT);
}

// Write to one of the shell's descriptors, through its current file.
static b32 fdwrite(Shell *sh, i32 fd, Str s)
{
    sh->writeerr = E_BADF;
    if (fd<0 || fd>=FDMAX) return 0;
    File *f = sh->fds->fd[fd];
    if (!f || !(f->flags & OF_WRITE)) return 0;
    i32 err = 0;
    switch (f->kind) {
    case F_OS:
        if (!s.len) return 1;
        err = taskwrite(sh->sched, f->h, s.data, s.len);
        break;
    case F_CAPTURE:
        if (((Capture *)f->cap)->pumps) {
            capturedrain(sh->sched, (Capture *)f->cap);
        }
        tracewrite(sh->sched, &((Capture *)f->cap)->wtrace);
        print(f->cap, s);
        return 1;
    case F_MEM:
        return 0;
    case F_PIPE:
        err = pipewrite(sh->sched, f->pipe, s.data, s.len);
        break;
    }
    if (err == E_PIPE) {
        sigpipe(sh);
    } else if (err && sh->sched->cur->intr) {
        sigtaken(sh);  // a signal that kills this level does so now
    }
    sh->writeerr = err;
    return !err;
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
    if (sh->interactive) return progbase(sh->shellname);  // as bash
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
// A message built with the caller's copy of scratch, as in
// f(sh, scratch, concat(&scratch, ...)), may sit where this copy starts
// (the order of evaluation is unspecified): allocate past it.
static void skipmsg(Arena *scratch, Str msg)
{
    byte *end = (byte *)(msg.data + msg.len);
    if ((byte *)msg.data>=scratch->beg && end>scratch->beg && end<=scratch->end) {
        scratch->beg = end;
    }
}

// Unlike a builtin's error, a shell error at a line below 1 (after a
// parse that ended at end of input) drops "line N: ", as bash.
static void shellerror(Shell *sh, Arena scratch, Str msg)
{
    flushbout(sh);
    skipmsg(&scratch, msg);
    Buf b(&scratch, msg.len+64);
    if (sh->line > 0) {
        msgprefix(sh, &b);
    } else {
        print(&b, errname(sh));
        print(&b, ": ");
    }
    print(&b, msg);
    putbyte(&b, '\n');
    fdwrite(sh, 2, finish(&b));
}

// Report "NAME: line N: CMD: MSG", as builtins do.
static void builtinerror(Shell *sh, Arena scratch, Str cmd, Str msg)
{
    flushbout(sh);
    skipmsg(&scratch, msg);
    skipmsg(&scratch, cmd);
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
    case E_DEADLK: return "Resource deadlock avoided";
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

// Is the file a terminal? Asked once per File, since a handle never
// changes what it is (read asks on every call).
static b32 fileistty(Shell *sh, File *f)
{
    if (!f || f->kind!=F_OS) return 0;
    if (!f->tty) f->tty = os_isatty(sh->os, f->h) ? 2 : 1;
    return f->tty == 2;
}

// Can the file seek? Asked once per File, as fileistty.
static b32 fileseekable(Shell *sh, File *f)
{
    if (!f || f->kind!=F_OS) return 0;
    if (!f->seek) f->seek = os_seek(sh->os, f->h, 0, 1)>=0 ? 2 : 1;
    return f->seek == 2;
}

static void fileunref(Shell *sh, File *f)
{
    if (!f || --f->refs) return;
    if (f->kind == F_OS) {
        os_close(sh->os, f->h);
    } else if (f->kind == F_PIPE) {
        pipeclose(sh->sched, f->pipe, !(f->flags & OF_READ));
    }
    if (f->ownmem) {
        sfreestr(sh->store, f->mem);
    }
    sfree(sh->store, f, sizeof(File));
}

// Point descriptor fd at f, taking over the caller's reference. With
// keep set the change is permanent (exec), otherwise it is recorded in
// the undo list. A subshell restores its descriptors with restorefds.
static void fdset(Shell *sh, i32 fd, File *f, b32 keep, Arena *scratch)
{
    Fdtab *t = sh->fds;
    if (keep) {
        fileunref(sh, t->fd[fd]);
    } else {
        Undo *u = alloc<Undo>(scratch);
        u->fd   = fd;
        u->old  = t->fd[fd];
        u->psub = 0;
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
        if (u->psub) sfree(sh->store, u, sizeof(Undo));
    }
    while (t->top && !t->fd[t->top-1]) {
        t->top--;
    }
}

// Open a procsub's descriptor until the command ends: its record is in
// the store, since expansions may run in short-lived arenas.
static void fdsetpsub(Shell *sh, i32 fd, File *f)
{
    Fdtab *t = sh->fds;
    Undo  *u = (Undo *)salloc(sh->store, sizeof(Undo));
    u->fd   = fd;
    u->old  = t->fd[fd];
    u->psub = 1;
    u->next = sh->undo;
    sh->undo = u;
    t->fd[fd] = f;
    if (fd >= t->top) t->top = fd + 1;
}

// Close the procsub descriptors opened since a saved point, keeping
// the other redirections (exec makes those permanent).
static void closepsubs(Shell *sh, Undo *to)
{
    Fdtab *t = sh->fds;
    for (Undo **p = &sh->undo; *p != to;) {
        Undo *u = *p;
        if (!u->psub) {
            p = &u->next;
            continue;
        }
        *p = u->next;
        fileunref(sh, t->fd[u->fd]);
        t->fd[u->fd] = u->old;
        sfree(sh->store, u, sizeof(Undo));
    }
    while (t->top && !t->fd[t->top-1]) {
        t->top--;
    }
}

// The descriptors of a subshell's parent, each with a reference of
// its own: exec's changes in the subshell last until it ends.
static Fdsave savefds(Shell *sh, Arena *a)
{
    Fdtab *t = sh->fds;
    Fdsave r = {};
    r.top = t->top;
    r.fd  = alloc<File *>(a, r.top);
    for (i32 fd = 0; fd < r.top; fd++) {
        r.fd[fd] = t->fd[fd];
        if (r.fd[fd]) r.fd[fd]->refs++;
    }
    return r;
}

// Put back the saved descriptors once the subshell's own redirections
// are undone, closing whatever exec left open there.
static void restorefds(Shell *sh, Fdsave s)
{
    Fdtab *t = sh->fds;
    for (i32 fd = 0; fd < t->top; fd++) {
        fileunref(sh, t->fd[fd]);
        t->fd[fd] = fd<s.top ? s.fd[fd] : 0;
    }
    for (i32 fd = t->top; fd < s.top; fd++) {
        t->fd[fd] = s.fd[fd];
    }
    t->top = s.top;
}

// Make the redirections recorded since a saved point permanent, as if
// made with keep set.
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
    case F_PIPE:
        for (;;) {
            iz n = f->kind==F_OS ? taskread(sh->sched, f->h, buf, len)
                                 : piperead(sh->sched, f->pipe, buf, len);
            // A signal may have failed the read: one that kills this
            // level does so, the read builtin runs trap actions at
            // once, as bash, and the read resumes
            if (n>=0 || !sigtaken(sh)) return n;
            if (sh->intrread) runsigs(sh, {});
            if (sh->intredit) return -4;  // the editor looks at its size
        }
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
    // A builtin's output buffer lives in its frame, which is now gone
    sh->bout     = sh->esc->bout;
    sh->esc->why = why;
    __builtin_longjmp(sh->esc->jmp, 1);
}
