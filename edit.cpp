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
    EF_DIGIT_ARGUMENT,
    EF_UNIVERSAL_ARGUMENT,
    EF_BEGINNING_OF_HISTORY,
    EF_END_OF_HISTORY,
    EF_OPERATE_AND_GET_NEXT,
    EF_YANK,
    EF_YANK_POP,
    EF_UNDO,
    EF_REVERT_LINE,
    EF_SET_MARK,
    EF_EXCHANGE_POINT_AND_MARK,
    EF_CAPITALIZE_WORD,
    EF_UPCASE_WORD,
    EF_DOWNCASE_WORD,
    EF_TRANSPOSE_WORDS,
    EF_DELETE_HORIZONTAL_SPACE,
    EF_BACKWARD_KILL_LINE,
    EF_KILL_WHOLE_LINE,
    EF_KILL_REGION,
    EF_COPY_REGION_AS_KILL,
    EF_COPY_FORWARD_WORD,
    EF_COPY_BACKWARD_WORD,
    EF_UNIX_FILENAME_RUBOUT,
    EF_YANK_LAST_ARG,
    EF_YANK_NTH_ARG,
    EF_INSERT_COMMENT,
    EF_CHARACTER_SEARCH,
    EF_CHARACTER_SEARCH_BACKWARD,
    EF_OVERWRITE_MODE,
    EF_SHELL_FORWARD_WORD,
    EF_SHELL_BACKWARD_WORD,
    EF_SHELL_KILL_WORD,
    EF_SHELL_BACKWARD_KILL_WORD,
    EF_SHELL_TRANSPOSE_WORDS,
    EF_DO_LOWERCASE_VERSION,
    EF_CLEAR_DISPLAY,
    EF_REVERSE_SEARCH_HISTORY,
    EF_FORWARD_SEARCH_HISTORY,
    EF_NON_INCREMENTAL_REVERSE_SEARCH_HISTORY,
    EF_NON_INCREMENTAL_FORWARD_SEARCH_HISTORY,
    EF_HISTORY_SEARCH_BACKWARD,
    EF_HISTORY_SEARCH_FORWARD,
    EF_HISTORY_SUBSTRING_SEARCH_BACKWARD,
    EF_HISTORY_SUBSTRING_SEARCH_FORWARD,
    EF_SHELL_EXPAND_LINE,
    EF_HISTORY_EXPAND_LINE,
    EF_MAGIC_SPACE,
    EF_EDIT_AND_EXECUTE_COMMAND,
    EF_BRACKETED_PASTE_BEGIN,
    EF_POSSIBLE_COMPLETIONS,
    EF_INSERT_COMPLETIONS,
    EF_COMPLETE_FILENAME,
    EF_POSSIBLE_FILENAME_COMPLETIONS,
    EF_COMPLETE_USERNAME,
    EF_POSSIBLE_USERNAME_COMPLETIONS,
    EF_COMPLETE_HOSTNAME,
    EF_POSSIBLE_HOSTNAME_COMPLETIONS,
    EF_COMPLETE_VARIABLE,
    EF_POSSIBLE_VARIABLE_COMPLETIONS,
    EF_COMPLETE_COMMAND,
    EF_POSSIBLE_COMMAND_COMPLETIONS,
    EF_DYNAMIC_COMPLETE_HISTORY,
    EF_GLOB_COMPLETE_WORD,
    EF_GLOB_EXPAND_WORD,
    EF_GLOB_LIST_EXPANSIONS,
    EF_COMPLETE_INTO_BRACES,
    EF_MENU_COMPLETE,
    EF_MENU_COMPLETE_BACKWARD,
    EF_DELETE_CHAR_OR_LIST,
    EF_RE_READ_INIT_FILE,
    EF_MACRO,  // a key bound to text (inputrc)
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
    "digit-argument",
    "universal-argument",
    "beginning-of-history",
    "end-of-history",
    "operate-and-get-next",
    "yank",
    "yank-pop",
    "undo",
    "revert-line",
    "set-mark",
    "exchange-point-and-mark",
    "capitalize-word",
    "upcase-word",
    "downcase-word",
    "transpose-words",
    "delete-horizontal-space",
    "backward-kill-line",
    "kill-whole-line",
    "kill-region",
    "copy-region-as-kill",
    "copy-forward-word",
    "copy-backward-word",
    "unix-filename-rubout",
    "yank-last-arg",
    "yank-nth-arg",
    "insert-comment",
    "character-search",
    "character-search-backward",
    "overwrite-mode",
    "shell-forward-word",
    "shell-backward-word",
    "shell-kill-word",
    "shell-backward-kill-word",
    "shell-transpose-words",
    "do-lowercase-version",
    "clear-display",
    "reverse-search-history",
    "forward-search-history",
    "non-incremental-reverse-search-history",
    "non-incremental-forward-search-history",
    "history-search-backward",
    "history-search-forward",
    "history-substring-search-backward",
    "history-substring-search-forward",
    "shell-expand-line",
    "history-expand-line",
    "magic-space",
    "edit-and-execute-command",
    "bracketed-paste-begin",
    "possible-completions",
    "insert-completions",
    "complete-filename",
    "possible-filename-completions",
    "complete-username",
    "possible-username-completions",
    "complete-hostname",
    "possible-hostname-completions",
    "complete-variable",
    "possible-variable-completions",
    "complete-command",
    "possible-command-completions",
    "dynamic-complete-history",
    "glob-complete-word",
    "glob-expand-word",
    "glob-list-expansions",
    "complete-into-braces",
    "menu-complete",
    "menu-complete-backward",
    "delete-char-or-list",
    "re-read-init-file",
    "",
};

static iz readstdin(Shell *, File *, u8 *, iz);  // unabashed.cpp

// Key bindings: a trie of key sequences. A node with children is a
// prefix: the editor waits for the next key, at most keyseq-timeout if
// the node is bound too (ESC alone, or ESC [ A).
struct KeyNode {
    KeyNode *kids;
    KeyNode *sib;
    Str      macro; // EF_MACRO: the text
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

// The child for key, made if missing
static KeyNode *keyget(Arena *a, KeyNode *n, u8 key)
{
    KeyNode *k = keychild(n, key);
    if (!k) {
        k = alloc<KeyNode>(a);
        k->key  = key;
        k->sib  = n->kids;
        n->kids = k;
    }
    return k;
}

static void keybind(Arena *a, KeyNode *root, Str seq, i32 fn)
{
    KeyNode *n = root;
    for (iz i = 0; i < seq.len; i++) n = keyget(a, n, seq[i]);
    n->fn = fn;
}

// EdUndo records, as readline's: an insert, a delete with its text, and
// the bounds of a group undone as one.
enum : i32 { U_INSERT, U_DELETE, U_BEGIN, U_END };

struct EdUndo {
    EdUndo *next;  // older
    Str   text;  // U_DELETE: what went
    iz    beg;
    iz    end;
    i32   kind;
};

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

enum : i32 { KILLMAX = 10 };

// Readline's variables (readline.md §5, inputrc.md §8), which the
// init file sets. Many are only kept, for $if.
struct EdVars {
    b32 ignorecase;     // completion-ignore-case
    b32 mapcase;        // completion-map-case
    i32 queryitems;     // completion-query-items
    b32 pagecompl;      // page-completions
    i32 displaywidth;   // completion-display-width
    i32 prefixdisplay;  // completion-prefix-display-length
    b32 horizontal;     // print-completions-horizontally
    b32 showall;        // show-all-if-ambiguous
    b32 showunmod;      // show-all-if-unmodified
    b32 markdirs;       // mark-directories
    b32 marksymdirs;    // mark-symlinked-directories
    b32 matchhidden;    // match-hidden-files
    b32 visiblestats;   // visible-stats
    b32 skipcompleted;  // skip-completed-text
    b32 nocompletion;   // disable-completion
    b32 menuprefix;     // menu-complete-display-prefix
    b32 ttyspecial;     // bind-tty-special-chars
    b32 blink;          // blink-matching-paren
    b32 byteoriented;
    b32 colprefix;      // colored-completion-prefix
    b32 colstats;       // colored-stats
    b32 convertmeta;
    b32 echoctl;        // echo-control-characters
    b32 activeregion;   // enable-active-region
    b32 bracketpaste;   // enable-bracketed-paste
    b32 keypad;         // enable-keypad
    b32 metakey;        // enable-meta-key
    b32 expandtilde;
    b32 forcemeta;      // force-meta-prefix
    b32 histpoint;      // history-preserve-point
    b32 hscrollmode;    // horizontal-scroll-mode
    b32 inputmeta;      // input-meta, meta-flag
    b32 markmodified;   // mark-modified-lines
    b32 outputmeta;
    b32 visiblebell;    // prefer-visible-bell, as reported
    b32 revertall;      // revert-all-at-newline
    b32 searchcase;     // search-ignore-case
    b32 showmode;       // show-mode-in-prompt
    i32 bellstyle;      // 0 none, 1 visible, 2 audible
    Str comment;        // comment-begin
    i32 histsize;       // history-size
    Str isterm;         // isearch-terminators, null for ESC and C-j
    Str regionstart;    // active-region-start-color
    Str regionend;
    Str emacsstr;       // emacs-mode-string
    Str vicmdstr;
    Str viinsstr;
    i32 vimode;         // editing-mode vi
    i32 keymap;         // KM_*: where bindings go
};

struct Comp;

struct Editor {
    Arena    perm;      // bindings, settings
    Arena    lay[2];    // layouts: the one on screen and the next
    i32      cur;       // which of lay is on screen
    KeyNode  root;
    KeyNode  vimove;    // vi bindings, kept for nothing yet
    KeyNode  viins;
    i32      keytimeout;  // keyseq-timeout, ms, 0 forever
    i64      deadline;    // read -t: os_now time when reading ends, or 0
    b32      timedout;    // and it has
    u8       pushed[32];  // keys read ahead and put back, the next last
    i32      unread;    // how many
    Str      macro;     // a macro's keys, read before the terminal's
    iz       macropos;
    // The init file
    b32      inited;
    Str      rllast;    // the file last read, for C-x C-r
    b32      rlreading; // messages give the file and line
    b32      ifoff;     // in a false $if
    Slice<b32> ifstack; // the enclosing $ifs' ifoff, never reset
    u8       ttysaved[4];  // the terminal's erase, kill, lnext, werase
                           // as last bound
    // The line being edited
    Buf      line;
    iz       point;
    Arena    linemem;
    // The terminal
    Tty      tty;
    VtCon    vtcon;     // tty.legacy: the console's interpreter
    i32      width;     // usable columns: one less without deferred wrap
    Handle   in;
    Handle   out;
    i32      setrows;   // the size last put in LINES and COLUMNS
    i32      setcols;
    // The display
    Str      prompt;    // the prompt's last line, \1 \2 marking the invisible
    File    *fin;       // where keys come from
    i32      delim;     // read -d: this byte accepts, CR inserts; or -1
    i64      nchars;    // read -n: accept at this many characters, or 0
    b32      silent;    // read -s: the line is not shown
    b32      hscroll;   // TERM=dumb: one row, scrolled sideways
    i32      lmargin;   // hscroll: the first column shown
    i32      hshown;    // hscroll: columns drawn
    Layout   shown;
    i32      crow;      // where the cursor is, relative to the prompt's row
    i32      ccol;
    b32      pending;   // the terminal has a wrap pending (crow, ccol at
                        // the last column)
    Buf      out_;      // output for one redisplay
    b32      done;
    i32      result;    // ED_*
    u8       eof;       // the terminal's EOF character
    u8       lastc;     // the first key of the last command
    // EdUndo, and the kill ring, lasting from line to line
    Arena    hmem;      // undo lists, which history entries keep
    EdUndo    *undo;
    b32      undoing;
    Arena    kmem[2];   // the kill ring, compacted from one to the other
    i32      kcur;
    Str      kills[KILLMAX];  // the oldest first
    i32      nkills;
    i32      killidx;   // the one yank inserts
    iz       mark;
    b32      overwrite;
    // The command running and the one before
    i32      count;     // the numeric argument
    b32      argset;    // given explicitly
    b32      argneg;    // while reading it: a minus seen
    i32      lastfn;
    b32      killed;    // this command killed
    b32      lastkill;  // the last command killed: kills join it
    // History
    iz       hpos;      // the entry edited, or the list's length
    b32      hsaved;    // the new line, kept while on an entry
    Str      hsavedline;
    EdUndo    *hsavedundo;
    i64      getnext;   // operate-and-get-next: the entry to start on, or -1
    b32      active;    // the region from mark to point is highlighted
    b32      keepactive;  // and stays so past this command
    Str      lastisearch; // the last incremental search's string
    Str      noninc;    // the last non-incremental search's string
    iz       nonincpos; // and the entry it found
    iz       hspos;     // history-search: the entry last found
    Str      hsprefix;  // ^ and the line up to point
    Str      hsprev;    // the last line found
    i64      ylskip;    // yank-last-arg's walk back
    i32      yldir;
    i32      ylcount;
    b32      ylarg;
    b32      ylundo;
    // Completion
    EdVars   vars;
    Str      phead;     // the prompt's lines before its last
    b32      shellline; // the shell's own line, not read -e
    b32      ps1;       // and its first: a command word may start it
    b32      cchanged;  // the last completion changed the line
    b32      cfailed;   // and found nothing
    Arena    cmem;      // menu completion's matches
    Comp    *menu;
    i32      menuidx;
    b32      menudone;
};

static void edbindings(Editor *ed)
{
    Arena   *a = &ed->perm;
    KeyNode *r = &ed->root;
    for (i32 c = 0x20; c < 0x100; c++) {
        u8 k = (u8)c;
        if (c != 0x7f) keybind(a, r, {&k, 1}, EF_SELF_INSERT);
    }
    for (u8 c = 'A'; c <= 'Z'; c++) {
        u8 meta[] = {0x1b, c};
        keybind(a, r, {meta, 2}, EF_DO_LOWERCASE_VERSION);
        u8 ctlx[] = {0x18, c};
        keybind(a, r, {ctlx, 2}, EF_DO_LOWERCASE_VERSION);
    }
    for (u8 c = '0'; c <= '9'; c++) {
        u8 seq[] = {0x1b, c};
        keybind(a, r, {seq, 2}, EF_DIGIT_ARGUMENT);
    }
    struct { Str seq; i32 fn; } map[] = {
        {"\x00", EF_SET_MARK},
        {"\x01", EF_BEGINNING_OF_LINE},   {"\x02", EF_BACKWARD_CHAR},
        {"\x04", EF_DELETE_CHAR},         {"\x05", EF_END_OF_LINE},
        {"\x06", EF_FORWARD_CHAR},        {"\x07", EF_ABORT},
        {"\x08", EF_BACKWARD_DELETE_CHAR},{"\x09", EF_COMPLETE},
        {"\x0a", EF_ACCEPT_LINE},         {"\x0b", EF_KILL_LINE},
        {"\x0c", EF_CLEAR_SCREEN},        {"\x0d", EF_ACCEPT_LINE},
        {"\x0e", EF_NEXT_HISTORY},        {"\x0f", EF_OPERATE_AND_GET_NEXT},
        {"\x10", EF_PREVIOUS_HISTORY},    {"\x11", EF_QUOTED_INSERT},
        {"\x12", EF_REVERSE_SEARCH_HISTORY},
        {"\x13", EF_FORWARD_SEARCH_HISTORY},
        {"\x14", EF_TRANSPOSE_CHARS},     {"\x15", EF_UNIX_LINE_DISCARD},
        {"\x16", EF_QUOTED_INSERT},       {"\x17", EF_UNIX_WORD_RUBOUT},
        {"\x19", EF_YANK},                {"\x1d", EF_CHARACTER_SEARCH},
        {"\x1f", EF_UNDO},                {"\x7f", EF_BACKWARD_DELETE_CHAR},
        {"\x18\x07", EF_ABORT},           {"\x18\x15", EF_UNDO},
        {"\x18\x18", EF_EXCHANGE_POINT_AND_MARK},
        {"\x18\x7f", EF_BACKWARD_KILL_LINE},
        {"\x18\x05", EF_EDIT_AND_EXECUTE_COMMAND},
        {"\x1b\x05", EF_SHELL_EXPAND_LINE},
        {"\x1b\x02", EF_SHELL_BACKWARD_WORD},
        {"\x1b\x04", EF_SHELL_KILL_WORD},
        {"\x1b\x06", EF_SHELL_FORWARD_WORD},
        {"\x1b\x14", EF_SHELL_TRANSPOSE_WORDS},
        {"\x1b\x07", EF_ABORT},           {"\x1b\x08", EF_BACKWARD_KILL_WORD},
        {"\x1b\x0c", EF_CLEAR_DISPLAY},   {"\x1b\x12", EF_REVERT_LINE},
        {"\x1b\x19", EF_YANK_NTH_ARG},    {"\x1b\x1b", EF_COMPLETE},
        {"\x1b\x1d", EF_CHARACTER_SEARCH_BACKWARD},
        {"\x1b ", EF_SET_MARK},           {"\x1b#", EF_INSERT_COMMENT},
        {"\x1b-", EF_DIGIT_ARGUMENT},     {"\x1b.", EF_YANK_LAST_ARG},
        {"\x1b<", EF_BEGINNING_OF_HISTORY},
        {"\x1b>", EF_END_OF_HISTORY},     {"\x1b\\", EF_DELETE_HORIZONTAL_SPACE},
        {"\x1b^", EF_HISTORY_EXPAND_LINE},{"\x1b_", EF_YANK_LAST_ARG},
        {"\x1b" "b", EF_BACKWARD_WORD},   {"\x1b" "c", EF_CAPITALIZE_WORD},
        {"\x1b" "d", EF_KILL_WORD},       {"\x1b" "f", EF_FORWARD_WORD},
        {"\x1b" "l", EF_DOWNCASE_WORD},
        {"\x1b" "n", EF_NON_INCREMENTAL_FORWARD_SEARCH_HISTORY},
        {"\x1b" "p", EF_NON_INCREMENTAL_REVERSE_SEARCH_HISTORY},
        {"\x1b" "r", EF_REVERT_LINE},     {"\x1b" "t", EF_TRANSPOSE_WORDS},
        {"\x1b" "u", EF_UPCASE_WORD},     {"\x1b" "y", EF_YANK_POP},
        {"\x1b\x7f", EF_BACKWARD_KILL_WORD},
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
        {"\x1b[2~", EF_OVERWRITE_MODE},
        {"\x1b[5~", EF_HISTORY_SEARCH_BACKWARD},
        {"\x1b[6~", EF_HISTORY_SEARCH_FORWARD},
        {"\x1b[200~", EF_BRACKETED_PASTE_BEGIN},
        {"\x1b?", EF_POSSIBLE_COMPLETIONS},  {"\x1b=", EF_POSSIBLE_COMPLETIONS},
        {"\x1b*", EF_INSERT_COMPLETIONS},    {"\x1b/", EF_COMPLETE_FILENAME},
        {"\x18/", EF_POSSIBLE_FILENAME_COMPLETIONS},
        {"\x1b~", EF_COMPLETE_USERNAME},
        {"\x18~", EF_POSSIBLE_USERNAME_COMPLETIONS},
        {"\x1b@", EF_COMPLETE_HOSTNAME},
        {"\x18@", EF_POSSIBLE_HOSTNAME_COMPLETIONS},
        {"\x1b$", EF_COMPLETE_VARIABLE},
        {"\x18$", EF_POSSIBLE_VARIABLE_COMPLETIONS},
        {"\x1b!", EF_COMPLETE_COMMAND},
        {"\x18!", EF_POSSIBLE_COMMAND_COMPLETIONS},
        {"\x1b\t", EF_DYNAMIC_COMPLETE_HISTORY},
        {"\x1b" "g", EF_GLOB_COMPLETE_WORD},
        {"\x18*", EF_GLOB_EXPAND_WORD},
        {"\x18" "g", EF_GLOB_LIST_EXPANSIONS},
        {"\x1b{", EF_COMPLETE_INTO_BRACES},
        {"\x18\x12", EF_RE_READ_INIT_FILE},
    };
    for (iz i = 0; i < countof(map); i++) {
        keybind(a, r, map[i].seq, map[i].fn);  // literal lengths, NUL too
    }
}

static Editor *editor(Shell *sh)
{
    Editor *ed = sh->editor;  // shared by subshells
    if (!ed) {
        // Each 16 MiB, or less where the shell's own are (tests, fuzzing)
        iz    cap  = sh->reserve<((iz)1<<24) ? sh->reserve : (iz)1<<24;
        Arena perm = newarena(sh->os, cap);
        ed = alloc<Editor>(&perm);
        ed->perm       = perm;
        ed->lay[0]     = newarena(sh->os, cap);
        ed->lay[1]     = newarena(sh->os, cap);
        ed->linemem    = newarena(sh->os, cap);
        ed->hmem       = newarena(sh->os, cap);
        ed->kmem[0]    = newarena(sh->os, cap);
        ed->kmem[1]    = newarena(sh->os, cap);
        ed->keytimeout = 500;
        ed->cmem       = newarena(sh->os, cap);
        ed->vars.queryitems   = 100;
        ed->vars.pagecompl    = 1;
        ed->vars.displaywidth = -1;
        ed->vars.markdirs     = 1;
        ed->vars.matchhidden  = 1;
        ed->vars.ttyspecial   = 1;
        ed->vars.echoctl      = 1;
        ed->vars.activeregion = 1;
        ed->vars.bracketpaste = 1;
        ed->vars.metakey      = 1;
        ed->vars.inputmeta    = 1;
        ed->vars.outputmeta   = 1;
        ed->vars.visiblebell  = 1;
        ed->vars.bellstyle    = 2;
        ed->vars.comment      = "#";
        ed->vars.regionstart  = "\x1b[7m";
        ed->vars.regionend    = "\x1b[27m";
        ed->vars.emacsstr     = "@";
        ed->vars.vicmdstr     = "(cmd)";
        ed->vars.viinsstr     = "(ins)";
        ed->getnext    = -1;
        edbindings(ed);
        sh->editor = ed;
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

// The active region, line[rbeg, rend), is drawn in standout: each of
// its cells carries the sequences that turn it on and off, so that any
// cell may be redrawn alone. Built in buf (24 bytes), not the arena,
// whose buffers are growing.
static Str layregion(u8 *buf, Str bytes)
{
    Str on  = "\x1b[7m";
    Str off = "\x1b[27m";
    iz  n   = 0;
    copybytes(buf+n, on.data, on.len);
    n += on.len;
    copybytes(buf+n, bytes.data, bytes.len);
    n += bytes.len;
    copybytes(buf+n, off.data, off.len);
    n += off.len;
    return {buf, n};
}

// An invalid byte is drawn as the terminal would show it, U+FFFD, so
// that one cut short at the end of the output is not held back.
static Str badbyte = "\xef\xbf\xbd";

static Layout layout(Arena *a, Str prompt, Str line, iz point, i32 width,
                     iz rbeg, iz rend)
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
        i32 cp  = utf8strict(prompt, &i);
        Str s   = {prompt.data+beg, i-beg};
        if (cp<0 && !hidden) {
            laycell(&y, badbyte, 1);
        } else if (hidden) {
            layinvisible(&y, s);
        } else {
            // As readline counts the visible, a C0 control is a column;
            // C1 controls, which the terminal does not show, are bad
            i32 w = cpwidth(cp);
            if (w<0 && cp>=0x80) s = badbyte;
            laycell(&y, s, w<0 ? 1 : w);
        }
    }
    y.l.prow = -1;
    for (iz i = 0; i <= line.len;) {
        // Bytes typed apart may join into one character around point:
        // the cursor goes after it
        if (y.l.prow<0 && i>=point) {
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
        i32 cp  = utf8strict(line, &i);
        b32 hi  = beg>=rbeg && beg<rend;
        u8  hbuf[24];
        if (c == '\n') {
            // A newline in the line starts a row of its own
            layfinish(&y);
            laynewrow(&y);
            y.vcol = 0;
        } else if (c == '\t') {
            i32 n = 8 - y.vcol%8;
            for (i32 k = 0; k < n; k++) {
                laycell(&y, hi ? layregion(hbuf, " ") : Str(" "), 1);  // as readline, may wrap
            }
        } else if (cp < 0) {
            laycell(&y, hi ? layregion(hbuf, badbyte) : badbyte, 1);
        } else if (c<0x20 || c==0x7f) {
            u8  buf[2] = {'^', (u8)(c==0x7f ? '?' : c|0x40)};
            Str t      = {buf, 2};
            laycell(&y, hi ? layregion(hbuf, t) : t, 2);
        } else {
            i32 w = cpwidth(cp);
            Str t = {line.data+beg, i-beg};
            t = w<0 ? badbyte : t;  // a C1 control
            laycell(&y, hi ? layregion(hbuf, t) : t, w<0 ? 1 : w);
        }
    }
    if (y.col >= y.width) {
        // As readline, a full last row has an empty one after it,
        // wherever the cursor is
        layfinish(&y);
        laynewrow(&y);
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

// Test layers (EDIT_CHECK) check each redisplay against the layout
// drawn afresh: told where a line starts, where a redraw starts from
// the left margin, when a redisplay is done, and which output is the
// editor's own.
#ifdef EDIT_CHECK
enum : i32 {
    EDCHECK_START, EDCHECK_MARGIN, EDCHECK_DRAWN, EDCHECK_WRITE, EDCHECK_WROTE
};
static void edcheck(Editor *, Shell *, i32 how);
#endif

static void edflush(Editor *ed, Shell *sh)
{
    Str s = finish(&ed->out_);
    #ifdef EDIT_CHECK
    edcheck(ed, sh, EDCHECK_WRITE);
    #endif
    if (!s.len) {
        // nothing
    } else if (ed->tty.legacy) {
        vtconwrite(sh->os, ed->out, &ed->vtcon, s);
    } else {
        fdwrite(sh, 2, s);
    }
    #ifdef EDIT_CHECK
    edcheck(ed, sh, EDCHECK_WROTE);
    #endif
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
            // An empty row is the one past a full row: it may hold the
            // space that made the terminal wrap
            edmove(ed, row, 0);
            edput(ed, "\x1b[K");
            continue;
        }
        if (!w->ncells && ed->pending && ed->crow==row-1) {
            // As readline: a space on the empty row past a full one
            // makes the terminal wrap
            edput(ed, " ");
            ed->crow    = row;
            ed->ccol    = 1;
            ed->pending = 0;
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
            // A zero-width cell (a combining mark) is drawn over the one
            // before it: redraw from that one, which drops an old mark
            while (same > 0 && ((same<o->ncells && cellcol(o, same)==o->cols[same]) ||
                                (same<w->ncells && cellcol(w, same)==w->cols[same]))) {
                same--;
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

// A terminal that cannot move the cursor up (TERM=dumb), as readline
// draws for it: the line on one row, scrolled sideways to keep the
// cursor about two thirds of the way across once it would leave the
// screen, '<' and '>' over the first and last columns where the line
// goes on past them. Drawn again whole from the left margin with CR,
// spaces over what is left of the old, and the text up to the cursor.
static void hredisplay(Editor *ed, Layout *nl)
{
    DRow *r     = nl->rows;
    i32   w     = ed->tty.cols - 1;
    i32   t     = w / 3;
    i32   cpos  = nl->pcol;
    i32   total = r->width;
    i32   phys  = cpos - ed->lmargin;
    i32   lm    = ed->lmargin;
    if (phys > w-2) {
        lm = cpos - 2*t;
        lm = lm<0 ? 0 : lm;
    } else if (cpos < w-2) {
        lm = 0;
    } else if (phys < 1) {
        lm = (cpos - 1) / t * t;
    }
    ed->lmargin = lm;

    Buf  vis(&ed->linemem, 64);
    iz   atcur = -1;  // bytes of vis before the cursor
    for (i32 i = 0; i < r->ncells; i++) {
        i32 beg = cellcol(r, i);
        if (beg == cpos) atcur = atcur<0 ? vis.len : atcur;
        if (beg<lm || r->cols[i]>lm+w) continue;
        i32 ob = i ? r->ends[i-1] : 0;
        Str cell = {r->text.data+ob, r->ends[i]-ob};
        if (beg==lm && lm>0) cell = "<";
        if (r->cols[i]==lm+w && lm+w<total) cell = ">";
        print(&vis, cell);
    }
    if (atcur < 0) atcur = vis.len;
    i32 shown = (total<lm+w ? total : lm+w) - lm;
    edput(ed, "\r");
    print(&ed->out_, {vis.data, vis.len});
    for (i32 i = shown; i < ed->hshown; i++) edput(ed, " ");
    edput(ed, "\r");
    print(&ed->out_, {vis.data, atcur});
    ed->hshown = shown;
    ed->crow   = 0;
    ed->ccol   = cpos - lm;
}

static void redisplay(Editor *ed, Shell *sh)
{
    Arena *a = &ed->lay[!ed->cur];
    *a = {a->mem ? (byte *)(a->mem+1) : a->beg, a->end, a->mem};
    Str line = {ed->line.data, ed->line.len};
    iz  point = ed->point;
    if (ed->silent) line = {}, point = 0;
    iz  rbeg = 0, rend = 0;
    if (ed->active && !ed->hscroll) {
        iz m = ed->mark<line.len ? ed->mark : line.len;
        rbeg = m<point ? m : point;
        rend = m<point ? point : m;
    }
    Layout nl = layout(a, ed->prompt, line, point, ed->hscroll ? 1<<30 : ed->width,
                       rbeg, rend);
    if (ed->hscroll) {
        hredisplay(ed, &nl);
    } else {
        edupdate(ed, &nl);
    }
    ed->cur = !ed->cur;
    edflush(ed, sh);
    #ifdef EDIT_CHECK
    edcheck(ed, sh, EDCHECK_DRAWN);
    #endif
}

// Draw the prompt and line afresh on the current row.
static void edredraw(Editor *ed, Shell *sh)
{
    ed->shown   = {};
    ed->crow    = 0;
    ed->ccol    = 0;
    ed->pending = 0;
    edput(ed, "\r");
    #ifdef EDIT_CHECK
    edcheck(ed, sh, EDCHECK_MARGIN);
    #endif
    redisplay(ed, sh);
}

// Clear the rows of the line from its last up to its first, as
// readline's rl_clear_visible_line, then draw it all again from there.
static void edrefresh(Editor *ed, Shell *sh)
{
    edput(ed, "\r");
    ed->ccol    = 0;
    ed->pending = 0;
    for (i32 row = ed->shown.nrows-1; row >= 0; row--) {
        edmove(ed, row, 0);
        edput(ed, "\x1b[K");
    }
    edredraw(ed, sh);
}

static void edsize(Editor *, Shell *, Tty);

// The terminal may have a new size (SIGWINCH): as readline does, clear
// the rows of the line from its last up to its first, then draw it all
// again for the new width from there.
static void edresize(Editor *ed, Shell *sh)
{
    Tty t = os_tty(sh->os, ed->in, ed->out, TTY_QUERY);
    if (t.err || t.cols<2) return;
    if (t.rows==ed->tty.rows && t.cols==ed->tty.cols) return;
    ed->tty.rows = t.rows;
    ed->tty.cols = t.cols;
    ed->width    = ed->tty.xenl ? t.cols : t.cols-1;
    edsize(ed, sh, t);
    edrefresh(ed, sh);
}

// A WINCH trap runs as soon as the size is taken, as readline calls the
// shell's handler from its own: its output lands at the cursor, and the
// line is not drawn again. Other traps wait for the line to be done.
static void edwinch(Editor *ed, Shell *sh)
{
    u64 bit = sigbit(28);
    if (!(sh->sig->pending & bit) || !trapset(sh, 28)) return;
    sh->sig->pending &= ~bit;
    edflush(ed, sh);
    runtrap(sh, 28);
}


// Editing functions

static i32 edkey(Editor *, Shell *, i64 timeout);

// Put back a key to be read next.
static void edpush(Editor *ed, i32 c)
{
    if (ed->unread < countof(ed->pushed)) ed->pushed[ed->unread++] = (u8)c;
}

// The bell, as bell-style says: a flash of reverse video for visible
static void edbell(Editor *ed)
{
    switch (ed->vars.bellstyle) {
    case 0: break;
    case 1: edput(ed, "\x1b[?5h\x1b[?5l"); break;
    case 2: edput(ed, "\a"); break;
    }
}

static b32 edwordchar(u8 c)
{
    return (c>='0' && c<='9') || ((c|0x20)>='a' && (c|0x20)<='z') || c>=0x80;
}

static Str edtext(Editor *ed)
{
    return {ed->line.data, ed->line.len};
}

// Motion by characters, as readline's: a zero-width character (a
// combining mark) goes with the one before it.
static iz edprev(Editor *ed, iz i)  // start of the character before i
{
    Str s = edtext(ed);
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
    Str s = edtext(ed);
    if (i >= s.len) return s.len;
    utf8decode(s, &i);
    while (i < s.len) {  // and the zero-width ones after it
        iz j = i;
        if (cpwidth(utf8decode(s, &j))) break;
        i = j;
    }
    return i;
}

// Readline's word motions, count words at a time.
static iz edfwdword(Editor *ed, iz i, i32 count)
{
    u8 *s = ed->line.data;
    for (; count > 0; count--) {
        for (; i<ed->line.len && !edwordchar(s[i]); i++) {}
        for (; i<ed->line.len && edwordchar(s[i]); i++) {}
    }
    return i;
}

static iz edbackword(Editor *ed, iz i, i32 count)
{
    u8 *s = ed->line.data;
    for (; count > 0; count--) {
        for (; i>0 && !edwordchar(s[i-1]); i--) {}
        for (; i>0 && edwordchar(s[i-1]); i--) {}
    }
    return i;
}

static iz edwordmove(Editor *ed, iz i, i32 count)  // negative: back
{
    return count<0 ? edbackword(ed, i, -count) : edfwdword(ed, i, count);
}

// Readline's forward-char and backward-char from p (negative count:
// back), the bell at an end that stops it.
static iz edcharmove(Editor *ed, iz p, i32 count)
{
    if (count > 0) {
        if (p == ed->line.len) {
            edbell(ed);
            return p;
        }
        iz q = p;
        for (i32 i = 0; i<count && q<ed->line.len; i++) q = ednext(ed, q);
        if (q == p) edbell(ed);
        return q;
    }
    iz q = p;
    for (; count<0 && q>0; count++) q = edprev(ed, q);
    if (count) {
        edbell(ed);
        return 0;
    }
    return q;
}

static EdUndo *edrecord(Editor *ed, i32 kind, iz beg, iz end)
{
    EdUndo *u = alloc<EdUndo>(&ed->hmem);
    u->kind  = kind;
    u->beg   = beg;
    u->end   = end;
    u->next  = ed->undo;
    ed->undo = u;
    return u;
}

static void edgroup(Editor *ed, i32 kind)  // U_BEGIN or U_END
{
    if (!ed->undoing) edrecord(ed, kind, 0, 0);
}

// Insert at point, past it. As readline's, a single byte inserted where
// the last insert ended joins its undo record, up to 20 bytes.
static void edinsert(Editor *ed, Str s)
{
    if (!s.len) return;
    if (!ed->undoing) {
        EdUndo *u = ed->undo;
        if (s.len==1 && u && u->kind==U_INSERT && u->end==ed->point &&
            u->end-u->beg<20) {
            u->end++;
        } else {
            edrecord(ed, U_INSERT, ed->point, ed->point+s.len);
        }
    }
    reserve(&ed->line, s.len);
    u8 *d = ed->line.data;
    for (iz i = ed->line.len-1; i >= ed->point; i--) d[i+s.len] = d[i];
    copybytes(d+ed->point, s.data, s.len);
    ed->line.len += s.len;
    ed->point    += s.len;
}

// Delete [beg, end), either way round; point stays on the same text.
static void eddelete(Editor *ed, iz beg, iz end)
{
    if (beg > end) {
        iz t = beg; beg = end; end = t;
    }
    if (end > ed->line.len) end = ed->line.len;
    iz n = end - beg;
    if (n <= 0) return;
    u8 *s = ed->line.data;
    if (!ed->undoing) {
        EdUndo *u = edrecord(ed, U_DELETE, beg, end);
        u->text = clone(&ed->hmem, {s+beg, n});
    }
    for (iz i = beg; i+n < ed->line.len; i++) s[i] = s[i+n];
    ed->line.len -= n;
    if (ed->point > end) {
        ed->point -= n;
    } else if (ed->point > beg) {
        ed->point = beg;
    }
}

// EdUndo one record or group: false with nothing to undo.
static b32 edundo1(Editor *ed)
{
    if (!ed->undo) return 0;
    ed->undoing = 1;
    i32 depth = 0;
    do {
        EdUndo *u  = ed->undo;
        ed->undo = u->next;
        switch (u->kind) {
        case U_INSERT:
            eddelete(ed, u->beg, u->end);
            ed->point = u->beg;
            break;
        case U_DELETE:
            ed->point = u->beg<ed->line.len ? u->beg : ed->line.len;
            edinsert(ed, u->text);
            break;
        case U_END:
            depth++;
            break;
        case U_BEGIN:
            if (depth) {
                depth--;
            } else {
                edbell(ed);
            }
            break;
        }
    } while (depth && ed->undo);
    ed->undoing = 0;
    return 1;
}

// The line replaced whole, undone as one, as bash's expansions do.
static void edreplace(Editor *ed, Str s)
{
    if (s == edtext(ed)) return;
    edrecord(ed, U_BEGIN, 0, 0);
    eddelete(ed, 0, ed->line.len);
    ed->point = 0;
    edinsert(ed, s);
    edrecord(ed, U_END, 0, 0);
    ed->mark = 0;
}


// The kill ring

// Kill [from, to): from past to prepends to the last kill when that was
// the last command, else appends; otherwise a new kill.
static void edkill(Editor *ed, iz from, iz to)
{
    ed->killed = 1;
    if (from == to) return;
    iz  beg  = from<to ? from : to;
    iz  end  = from<to ? to : from;
    Str text = {ed->line.data+beg, end-beg};

    // Compact into the other arena when this one has grown
    Arena *k = &ed->kmem[ed->kcur];
    if (k->end-k->beg < ((iz)1<<23)) {
        ed->kcur = !ed->kcur;
        Arena *n = &ed->kmem[ed->kcur];
        *n = {(byte *)(n->mem+1), n->end, n->mem};
        for (i32 i = 0; i < ed->nkills; i++) ed->kills[i] = clone(n, ed->kills[i]);
        k = n;
    }

    if (ed->lastkill && ed->nkills) {
        Str *s = &ed->kills[ed->killidx];
        *s = from<to ? concat(k, *s, text) : concat(k, text, *s);
    } else {
        if (ed->nkills == KILLMAX) {
            for (i32 i = 1; i < KILLMAX; i++) ed->kills[i-1] = ed->kills[i];
            ed->nkills--;
        }
        ed->killidx = ed->nkills;
        ed->kills[ed->nkills++] = clone(k, text);
    }
    eddelete(ed, beg, end);
}

static void edyank(Editor *ed)
{
    if (!ed->nkills) {
        edbell(ed);
        return;
    }
    ed->mark = ed->point;
    edinsert(ed, ed->kills[ed->killidx]);
}


// History

// Readline's rl_maybe_replace_line: the entry left keeps its changes.
static b32 edkeepline(Editor *ed, Shell *sh)  // true if it did
{
    Hist *h = &sh->hist;
    if (ed->hpos >= h->len) return 0;
    if (h->ents[ed->hpos]->undo == ed->undo) return 0;
    histedit(sh, ed->hpos, edtext(ed), ed->undo);
    return 1;
}

static void edload(Editor *ed, Shell *sh, iz i)
{
    HistEnt *e = sh->hist.ents[i];
    ed->hpos     = i;
    ed->line.len = 0;
    print(&ed->line, e->line);
    ed->undo  = e->undo;
    ed->point = ed->line.len;
    ed->mark  = 0;
}

static void edsaveline(Editor *ed)
{
    if (ed->hsaved) return;
    ed->hsaved     = 1;
    ed->hsavedline = clone(&ed->linemem, edtext(ed));
    ed->hsavedundo = ed->undo;
}

static void edunsaveline(Editor *ed)
{
    if (!ed->hsaved) {
        edbell(ed);
        return;
    }
    ed->hsaved   = 0;
    ed->line.len = 0;
    print(&ed->line, ed->hsavedline);
    ed->undo  = ed->hsavedundo;
    ed->point = ed->line.len;
    ed->mark  = 0;
}

static void ednexthist(Editor *, Shell *, i64);

static void edprevhist(Editor *ed, Shell *sh, i64 count)
{
    if (count < 0) return ednexthist(ed, sh, -count);
    Hist *h = &sh->hist;
    if (!count || !h->len) return;
    b32 had = ed->hsaved;
    edsaveline(ed);
    edkeepline(ed, sh);
    iz to = count>ed->hpos ? 0 : ed->hpos-(iz)count;
    if (to == ed->hpos) {
        ed->hsaved = had;
        edbell(ed);
        return;
    }
    edload(ed, sh, to);
}

static void ednexthist(Editor *ed, Shell *sh, i64 count)
{
    if (count < 0) return edprevhist(ed, sh, -count);
    if (!count) return;
    Hist *h = &sh->hist;
    edkeepline(ed, sh);
    if (count >= h->len-ed->hpos) {
        ed->hpos = h->len;
        edunsaveline(ed);
    } else {
        edload(ed, sh, ed->hpos+(iz)count);
    }
}

// Leaving the line: as readline does when it returns one, an entry
// changed and being accepted gets its own line back (the line goes in
// as a new entry), and on an interrupt it only loses its undo list.
static void edleave(Editor *ed, Shell *sh, b32 intr)
{
    Hist *h = &sh->hist;
    if (ed->hpos < h->len) {
        HistEnt *e = h->ents[ed->hpos];
        if (intr) {
            if (e->undo) histedit(sh, ed->hpos, e->line, 0);
        } else if (ed->undo || e->undo) {
            histedit(sh, ed->hpos, e->undo ? e->orig : e->line, 0);
        }
    }
    ed->hsaved = 0;
    ed->undo   = 0;
}

// The words of the entry skip entries back from the last, for
// yank-last-arg and yank-nth-arg: n counts from 0, '$' the last, and a
// negative from the end. False and the bell when there is none.
static b32 edyankarg(Editor *ed, Shell *sh, i64 n, i64 skip)
{
    Hist *h  = &sh->hist;
    iz    at = ed->hpos - 1 - (iz)skip;
    if (skip>=ed->hpos || at<0 || at>=h->len) {
        edbell(ed);
        return 0;
    }
    Arena  a = ed->linemem;
    HWords w = histwords(&a, h->ents[at]->line, h->comchar, -1);
    if (n == '$') {
        n = w.len - 1;
    } else if (n < 0) {
        n = w.len + n - 1;
    }
    if (n<0 || n>=w.len || !w.word[n].len) {
        edbell(ed);
        return 0;
    }
    Str arg = clone(&ed->hmem, w.word[n]);
    edgroup(ed, U_BEGIN);
    ed->mark = ed->point;
    edinsert(ed, arg);
    edgroup(ed, U_END);
    return 1;
}


// Shell words, as bash's: runs of characters other than unquoted blanks
// and metacharacters

static b32 edwdelim(u8 c)
{
    switch (c) {
    case ' ': case '\t': case '\n': case '|': case '&': case ';':
    case '(': case ')': case '<': case '>':
        return 1;
    }
    return 0;
}

// Past a quoted string opened before i, or the end.
static iz edskipquote(Str s, iz i, u8 q)
{
    for (; i < s.len; i++) {
        if (s[i]=='\\' && q=='"') {
            i++;
        } else if (s[i] == q) {
            return i + 1;
        }
    }
    return s.len;
}

// Is s[at] inside quotes or escaped, as bash's char_is_quoted?
static b32 edquoted(Str s, iz at)
{
    for (iz i = 0; i <= at && i < s.len;) {
        u8 c = s[i];
        if (c == '\\') {
            if (i+1 >= at) return i+1 == at;
            i += 2;
        } else if (c=='$' && i+2<s.len && s[i+1]=='\'') {
            i = edskipquote(s, i+2, '\'');
            if (i > at) return 1;
        } else if (c=='\'' || c=='"') {
            i = edskipquote(s, i+1, c);
            if (i > at) return 1;
        } else {
            i++;
        }
    }
    return 0;
}

// Past one character, or a quoted string, in a scan for shell words.
static iz edshellstep(Str s, iz p)
{
    u8 c = s[p];
    if (c == '\\') {
        p = p+2<s.len ? p+2 : s.len;
    } else if (c=='\'' || c=='"') {
        p = edskipquote(s, p+1, c);
    } else {
        p++;
    }
    return p;
}

static iz edshellfwd(Editor *ed, iz p, i32 count, b32 *bell)
{
    Str s = edtext(ed);
    for (; count > 0; count--) {
        if (p == s.len) return p;
        if (edquoted(s, p) && p>0 && s[p-1]!='\\') {
            do p++; while (p<s.len && edquoted(s, p));
            continue;
        }
        while (p<s.len && edwdelim(s[p])) p = edshellstep(s, p);
        if (p >= s.len) {
            *bell = 1;
            return s.len;
        }
        while (p<s.len && !edwdelim(s[p])) p = edshellstep(s, p);
        if (p >= s.len) return s.len;
    }
    return p;
}

static iz edshellback(Editor *ed, iz p, i32 count)
{
    Str s = edtext(ed);
    for (; count > 0; count--) {
        if (!p) return 0;
        p--;
        while (p>0 && edwdelim(s[p]) && !edquoted(s, p)) p--;
        if (!p) return 0;
        iz prev = p;
        while (p > 0) {
            if (edwdelim(s[p]) && !edquoted(s, p)) {
                p = prev;
                break;
            }
            prev = p--;
        }
    }
    return p;
}

static iz edshellmove(Editor *ed, iz p, i32 count, b32 *bell)
{
    return count<0 ? edshellback(ed, p, -count) : edshellfwd(ed, p, count, bell);
}

// Swap two words found by a pair of motions, as readline's
// transpose-words and bash's shell-transpose-words.
static void edtransposewords(Editor *ed, i32 count, b32 shell)
{
    if (!count) return;
    b32 bell = 0;
    iz  p    = ed->point;
    iz  w2e  = shell ? edshellmove(ed, p, count, &bell)   : edwordmove(ed, p, count);
    iz  w2b  = shell ? edshellmove(ed, w2e, -1, &bell)    : edwordmove(ed, w2e, -1);
    iz  w1b  = shell ? edshellmove(ed, w2b, -count, &bell): edwordmove(ed, w2b, -count);
    iz  w1e  = shell ? edshellmove(ed, w1b, 1, &bell)     : edwordmove(ed, w1b, 1);
    if (w1b==w2b || w2b<w1e) {
        edbell(ed);
        return;
    }
    Str w1 = clone(&ed->hmem, {ed->line.data+w1b, w1e-w1b});
    Str w2 = clone(&ed->hmem, {ed->line.data+w2b, w2e-w2b});
    edgroup(ed, U_BEGIN);
    ed->point = w2b;
    eddelete(ed, w2b, w2e);
    edinsert(ed, w1);
    ed->point = w1b;
    eddelete(ed, w1b, w1e);
    edinsert(ed, w2);
    ed->point = w2e;
    edgroup(ed, U_END);
}

// Readline's rl_change_case over count words from point.
enum : i32 { CASE_UP, CASE_DOWN, CASE_CAP };

static void edcase(Editor *ed, i32 count, i32 op)
{
    iz beg = ed->point;
    iz end = edwordmove(ed, beg, count);
    if (count < 0) {
        iz t = beg; beg = end; end = t;
    }
    // Recorded as a delete and insert of the span, as rl_modifying
    if (beg < end) {
        edgroup(ed, U_BEGIN);
        if (!ed->undoing) {
            EdUndo *u = edrecord(ed, U_DELETE, beg, end);
            u->text = clone(&ed->hmem, {ed->line.data+beg, end-beg});
            edrecord(ed, U_INSERT, beg, end);
        }
        edgroup(ed, U_END);
    }
    b32 inword = 0;
    for (iz i = beg; i < end; i++) {
        u8 *c = ed->line.data + i;
        if (!edwordchar(*c)) {
            inword = 0;
            continue;
        }
        i32 nop = op==CASE_CAP ? (inword ? CASE_DOWN : CASE_UP) : op;
        inword  = 1;
        if (nop==CASE_UP && *c>='a' && *c<='z') {
            *c -= 0x20;
        } else if (nop==CASE_DOWN && *c>='A' && *c<='Z') {
            *c += 0x20;
        }
    }
    ed->point = end;
}

// Readline's character search for the next key's character, count
// times: the cursor on it.
static void edcharsearch(Editor *ed, Shell *sh, i32 count, i32 dir)
{
    i32 c = edkey(ed, sh, -1);
    if (c < 0) return;
    u8  buf[4] = {(u8)c};
    iz  n = c>=0xf0 ? 4 : c>=0xe0 ? 3 : c>=0xc0 ? 2 : 1;
    iz  len = 1;
    while (len < n) {
        i32 k = edkey(ed, sh, -1);
        if (k<0 || (k&0xc0)!=0x80) break;
        buf[len++] = (u8)k;
    }
    Str ch = {buf, len};
    if (count < 0) {
        count = -count;
        dir   = -dir;
    }
    iz pos = ed->point;
    for (; count > 0; count--) {
        if ((dir<0 && pos<=0) || (dir>0 && pos>=ed->line.len)) {
            edbell(ed);
            return;
        }
        for (;;) {
            iz next = dir>0 ? ednext(ed, pos) : edprev(ed, pos);
            if (next == pos) break;
            pos = next;
            if (pos+len<=ed->line.len && Str{ed->line.data+pos, len}==ch) {
                ed->point = pos;
                break;
            }
        }
    }
}


// Searching the history

static void edrun(Editor *, Shell *, i32 fn, u8 key);
static void edcomplete(Editor *, Shell *, i32 fn, i32 count);  // complete.cpp
static void rlinit(Editor *, Shell *);       // inputrc.cpp
static void rlstart(Editor *, Shell *);
static void rlttychars(Editor *);

// Bracketed paste: the bytes up to ESC [ 201 ~, CR as LF.
static Str edpaste(Editor *ed, Shell *sh, Arena *a)
{
    Buf  b(a, 256);
    Str  end = "\x1b[201~";
    for (;;) {
        i32 c = edkey(ed, sh, -1);
        if (c < 0) break;
        putbyte(&b, c=='\r' ? '\n' : (u8)c);
        if (b.len>=end.len && Str{b.data+b.len-end.len, end.len}==end) {
            b.len -= end.len;
            break;
        }
    }
    return finish(&b);
}


static void edactivate(Editor *ed)
{
    ed->active = ed->keepactive = 1;
}

// Readline's incremental search (C-r, C-s): a modal loop with its own
// keys, the prompt replaced by the search's message, the match shown in
// the line it was found in. Ending it moves through the history to that
// entry; C-g puts everything back.
// Equal, or equal but for ASCII case (search-ignore-case)
static b32 edsame(Str a, Str b, b32 fold)
{
    if (!fold) return a == b;
    if (a.len != b.len) return 0;
    for (iz i = 0; i < a.len; i++) {
        u8 x = a[i]>='A' && a[i]<='Z' ? a[i]+32 : a[i];
        u8 y = b[i]>='A' && b[i]<='Z' ? b[i]+32 : b[i];
        if (x != y) return 0;
    }
    return 1;
}

static void edisearch(Editor *ed, Shell *sh, i64 count)
{
    Hist *h = &sh->hist;
    if (edkeepline(ed, sh)) ed->undo = 0;

    // Every entry, and the line typed: the saved one or this one
    iz   hlen  = h->len + 1;
    Str *lines = alloc<Str>(&ed->linemem, hlen);
    for (iz i = 0; i < h->len; i++) lines[i] = h->ents[i]->line;
    lines[h->len] = ed->hsaved ? ed->hsavedline : clone(&ed->linemem, edtext(ed));

    iz   saveline  = ed->hpos;
    iz   savepoint = ed->point;
    iz   savemark  = ed->mark;
    Str  prompt    = ed->prompt;
    iz   pos       = saveline;
    iz   lastfound = saveline;
    Str  sline     = clone(&ed->linemem, edtext(ed));
    iz   idx       = ed->point;
    // The line searched from is the one shown, which its entry may not
    // hold yet: inserts that extend the last undo record leave it alone
    lines[saveline] = sline;
    Str  prevfound = {};
    i32  dir       = count>=0 ? 1 : -1;  // C-r passes -count
    b32  failed    = 0;
    Buf  str(&ed->linemem, 64);
    b32  fold      = ed->vars.searchcase;

    for (;;) {
        // The message in place of the prompt
        Arena a = ed->linemem;
        Buf   m(&a, str.len+40);
        print(&m, "(");
        if (failed) print(&m, "failed ");
        if (dir < 0) print(&m, "reverse-");
        print(&m, "i-search)`");
        for (iz i = 0; i < str.len; i++) {
            // Controls as on the line, which the prompt would not show
            u8 c = str.data[i];
            if (c<0x20 || c==0x7f) {
                putbyte(&m, '^');
                putbyte(&m, c==0x7f ? '?' : (u8)(c|0x40));
            } else {
                putbyte(&m, c);
            }
        }
        print(&m, "': ");
        ed->prompt = finish(&m);
        redisplay(ed, sh);
        ed->prompt = prompt;

        i32 c = edkey(ed, sh, -1);
        if (c==-2 || c==ed->tty.intr) {
            ed->done   = 1;
            ed->result = ED_INTR;
            return;
        }
        if (c < 0) return;

        enum { OP_ADD, OP_AGAIN, OP_SWITCH, OP_DEL, OP_ABORT, OP_WORD, OP_REST,
               OP_QUOTE, OP_PASTE, OP_END, OP_ENDRUN };
        i32 op  = OP_ADD;
        u8  seq[16] = {(u8)c};
        i32 nseq    = 1;
        Str term = ed->vars.isterm.data ? ed->vars.isterm : Str("\x1b\n");
        b32 isterm = 0;
        for (iz i = 0; i < term.len; i++) isterm |= term[i] == c;
        if (isterm) {
            op = OP_END;
            if (c == 0x1b) {
                // More keys soon after ESC: the search ends and they run
                i32 n = edkey(ed, sh, 100);
                if (n >= 0) {
                    edpush(ed, n);
                    edpush(ed, 0x1b);
                }
            }
        } else {
            KeyNode *k = keychild(&ed->root, (u8)c);
            i64 wait   = ed->keytimeout>0 ? ed->keytimeout : -1;
            while (k && k->kids && nseq<countof(seq)) {
                i32 n = edkey(ed, sh, k->fn ? wait : -1);
                if (n == -3) break;
                if (n < 0) return;
                seq[nseq++] = (u8)n;
                k = keychild(k, (u8)n);
            }
            i32 fn = k ? k->fn : EF_NONE;
            if (fn==EF_REVERSE_SEARCH_HISTORY || fn==EF_FORWARD_SEARCH_HISTORY) {
                op = (fn==EF_REVERSE_SEARCH_HISTORY) == (dir<0) ? OP_AGAIN : OP_SWITCH;
            } else if (fn == EF_BACKWARD_DELETE_CHAR) {
                op = OP_DEL;
            } else if (fn==EF_ABORT || c==0x07) {
                op = OP_ABORT;
            } else if (fn==EF_UNIX_WORD_RUBOUT || c==0x17) {
                op = OP_WORD;
            } else if (fn==EF_YANK || c==0x19) {
                op = OP_REST;
            } else if (fn==EF_QUOTED_INSERT || c==0x16) {
                op = OP_QUOTE;
            } else if (fn == EF_BRACKETED_PASTE_BEGIN) {
                op = OP_PASTE;
            } else if (nseq > 1) {
                op = OP_ENDRUN;
            } else if (c<0x20 || c==0x7f) {
                op = OP_ENDRUN;
            }
        }

        if (op==OP_END || op==OP_ENDRUN) {
            // Back to the line searched from, then by history motion to
            // the one found
            ed->line.len = 0;
            print(&ed->line, lines[saveline]);
            ed->point = ed->line.len;
            Str last = {str.data, str.len};
            ed->lastisearch = clone(&ed->perm, last);
            if (lastfound < saveline) {
                edprevhist(ed, sh, saveline-lastfound);
            } else {
                ednexthist(ed, sh, lastfound-saveline);
            }
            ed->point  = idx<ed->line.len ? idx : ed->line.len;
            ed->active = 0;
            if (op == OP_ENDRUN) {
                for (i32 i = nseq-1; i >= 0; i--) edpush(ed, seq[i]);
            }
            return;
        }

        switch (op) {
        case OP_AGAIN:
            if (!str.len) {
                if (ed->lastisearch.data) {
                    print(&str, ed->lastisearch);
                    break;
                }
                continue;
            } else if (dir < 0) {
                if (idx >= 0) idx--;
            } else if (idx != sline.len) {
                idx++;
            } else {
                edbell(ed);
            }
            break;
        case OP_SWITCH:
            dir = -dir;
            break;
        case OP_DEL:
            if (!str.len) {
                edbell(ed);
            } else {
                Str s = {str.data, str.len};
                iz  i = s.len - 1;
                while (i>0 && (s[i]&0xc0)==0x80) i--;
                str.len = i;
            }
            if (!str.len) edbell(ed);
            break;
        case OP_ABORT:
            ed->line.len = 0;
            print(&ed->line, lines[saveline]);
            ed->point  = savepoint<ed->line.len ? savepoint : ed->line.len;
            ed->mark   = savemark;
            ed->active = 0;
            return;
        case OP_WORD: {
            iz w = ed->point + str.len;
            if (w>=ed->line.len || !edwordchar(ed->line.data[w])) {
                edbell(ed);
                break;
            }
            iz e = w;
            for (; e<ed->line.len && edwordchar(ed->line.data[e]); e++) {}
            print(&str, {ed->line.data+w, e-w});
        } break;
        case OP_REST: {
            iz w = ed->point + str.len;
            if (w >= ed->line.len) {
                edbell(ed);
                break;
            }
            print(&str, {ed->line.data+w, ed->line.len-w});
        } break;
        case OP_PASTE:
            print(&str, edpaste(ed, sh, &ed->linemem));
            break;
        case OP_QUOTE: {
            i32 k = edkey(ed, sh, -1);
            if (k < 0) return;
            putbyte(&str, (u8)k);
        } break;
        case OP_ADD: {
            putbyte(&str, (u8)c);
            iz n = c>=0xf0 ? 4 : c>=0xe0 ? 3 : c>=0xc0 ? 2 : 1;
            for (iz i = 1; i < n; i++) {
                i32 k = edkey(ed, sh, -1);
                if (k<0 || (k&0xc0)!=0x80) {
                    if (k >= 0) edpush(ed, k);
                    break;
                }
                putbyte(&str, (u8)k);
            }
        } break;
        }

        // The search: from idx in sline, then line by line
        Str  s     = {str.data, str.len};
        b32  found = 0;
        failed = 0;
        for (;;) {
            if (!s.len) {
                failed = 1;
                break;
            }
            iz limit = sline.len - s.len + 1;
            while (dir<0 ? idx>=0 : idx<limit) {
                if (idx>=0 && idx+s.len<=sline.len && edsame(Str{sline.data+idx, s.len}, s, fold)) {
                    found = 1;
                    break;
                }
                idx += dir;
                if (idx<0 || idx>sline.len) {
                    idx = 0;
                    break;
                }
            }
            if (found) break;
            do {
                pos += dir;
                if (dir<0 ? pos<0 : pos==hlen) {
                    failed = 1;
                    break;
                }
                sline = lines[pos];
            } while ((prevfound.data && prevfound==sline) || s.len>sline.len);
            if (failed) {
                if (idx<0 || idx>sline.len) idx = 0;
                break;
            }
            idx = dir<0 ? sline.len-s.len : 0;
        }
        if (failed) {
            edbell(ed);
            pos = lastfound;
            ed->active = 0;
        } else {
            prevfound    = lines[pos];
            ed->line.len = 0;
            print(&ed->line, lines[pos]);
            ed->point = idx;
            ed->mark  = idx + s.len;
            edactivate(ed);
            lastfound = pos;
        }
    }
}

// Search the history from entry pos in direction dir for one holding s
// (at its start if s starts with ^): its index, -1 if none, and where in
// it the match is (backward: the last in the line).
static iz edfindhist(Shell *sh, Str s, iz pos, i32 dir, iz *at, b32 fold)
{
    Hist *h  = &sh->hist;
    b32   anchor = s.len && s[0]=='^';
    if (anchor) s = cuthead(s, 1);
    for (; pos>=0 && pos<h->len; pos += dir) {
        Str line = h->ents[pos]->line;
        if (anchor) {
            if (line.len>=s.len && edsame(Str{line.data, s.len}, s, fold)) {
                *at = 0;
                return pos;
            }
            continue;
        }
        if (s.len > line.len) continue;
        for (iz i = 0; i <= line.len-s.len; i++) {
            iz j = dir<0 ? line.len-s.len-i : i;
            if (edsame(Str{line.data+j, s.len}, s, fold)) {
                *at = j;
                return pos;
            }
        }
    }
    return -1;
}

static void edgohist(Editor *ed, Shell *sh, iz to)
{
    if (to < ed->hpos) {
        edprevhist(ed, sh, ed->hpos-to);
    } else {
        ednexthist(ed, sh, to-ed->hpos);
    }
}

// Readline's non-incremental search (M-p, M-n): the string is read on
// the line, the prompt's last line with ':' after it.
static void ednoninc(Editor *ed, Shell *sh, i32 dir)
{
    iz     savepoint = ed->point;
    iz     savemark  = ed->mark;
    iz     savepos   = ed->hpos;
    Str    saved     = clone(&ed->linemem, edtext(ed));
    EdUndo *saveundo = ed->undo;
    Str    prompt    = ed->prompt;
    ed->line.len = ed->point = 0;
    ed->undo     = 0;
    ed->prompt   = concat(&ed->linemem, prompt, ":");

    b32 ok = 0;
    for (;;) {
        redisplay(ed, sh);
        i32 c = edkey(ed, sh, -1);
        if (c==-2 || c==ed->tty.intr) {
            ed->prompt = prompt;
            ed->done   = 1;
            ed->result = ED_INTR;
            return;
        }
        if (c < 0) break;
        if (c==0x17 || c==0x15) {
            ed->killed = 0;
            edrun(ed, sh, c==0x17 ? EF_UNIX_WORD_RUBOUT : EF_UNIX_LINE_DISCARD, (u8)c);
        } else if (c==0x11 || c==0x16) {
            i32 k = edkey(ed, sh, -1);
            if (k < 0) break;
            u8 b = (u8)k;
            edinsert(ed, {&b, 1});
        } else if (c=='\r' || c=='\n') {
            ok = 1;
            break;
        } else if (c==0x08 || c==0x7f) {
            if (!ed->line.len) break;
            eddelete(ed, edprev(ed, ed->point), ed->point);
        } else if (c==0x03 || c==0x07) {
            edbell(ed);
            break;
        } else {
            u8 b = (u8)c;
            edinsert(ed, {&b, 1});
        }
    }
    Str typed = clone(&ed->linemem, edtext(ed));
    ed->prompt   = prompt;
    ed->line.len = 0;
    print(&ed->line, saved);
    ed->undo  = saveundo;
    ed->point = savepoint;
    ed->mark  = savemark;
    if (!ok) return;

    iz from = savepos;
    if (!typed.len) {
        if (!ed->noninc.data) {
            edbell(ed);
            return;
        }
        from = ed->nonincpos;
    } else {
        ed->noninc = clone(&ed->perm, typed);
    }
    iz at  = 0;
    iz got = edfindhist(sh, ed->noninc, from+dir, dir, &at, ed->vars.searchcase);
    if (got < 0) {
        edbell(ed);
        return;
    }
    ed->nonincpos = got;
    edgohist(ed, sh, got);
    if (!ed->hscroll && at<ed->line.len) {
        ed->point = at;
        ed->mark  = at + ed->noninc.len;
        edactivate(ed);
    } else {
        ed->point = 0;
        ed->mark  = ed->line.len;
    }
}

// history-search-backward and -forward: entries starting with the line
// up to point.
static void edhistsearch(Editor *ed, Shell *sh, i32 fn, i32 count)
{
    if (!count) return;
    if (ed->lastfn!=EF_HISTORY_SEARCH_BACKWARD && ed->lastfn!=EF_HISTORY_SEARCH_FORWARD) {
        ed->hspos    = ed->hpos;
        ed->hsprefix = concat(&ed->linemem, "^", Str{ed->line.data, ed->point});
        ed->hsprev   = {};
    }
    if (ed->hsprefix.len == 1) {
        if (fn == EF_HISTORY_SEARCH_BACKWARD) {
            edprevhist(ed, sh, count);
        } else {
            ednexthist(ed, sh, count);
        }
        return;
    }
    i32 dir = (fn==EF_HISTORY_SEARCH_BACKWARD) == (count>0) ? -1 : 1;
    i32 n   = count<0 ? -count : count;
    b32 any = 0;
    while (n) {
        iz at  = 0;
        iz got = edfindhist(sh, ed->hsprefix, ed->hspos+dir, dir, &at, ed->vars.searchcase);
        if (got < 0) break;
        ed->hspos = got;
        any = 1;
        Str line = sh->hist.ents[got]->line;
        if (ed->hsprev.data && ed->hsprev==line) continue;
        ed->hsprev = clone(&ed->linemem, line);
        n--;
    }
    iz plen = ed->hsprefix.len - 1;
    if (!any) {
        edbell(ed);
        ed->point = plen<ed->line.len ? plen : ed->line.len;
        ed->mark  = ed->line.len;
        return;
    }
    edgohist(ed, sh, ed->hspos);
    ed->point = plen<ed->line.len ? plen : ed->line.len;
    ed->mark  = ed->line.len;
}


// Bash's expansions of the line

// History expansion of the line, as bash's pre_process_line without
// verification: false on an error or a :p, which are shown under the
// line, the line then drawn again. Quiet: the line as it is instead.
static b32 edhistexpand(Editor *ed, Shell *sh, Arena *a, Str *out, b32 quiet = 0)
{
    Hist *h = &sh->hist;
    Str line = edtext(ed);
    *out = line;
    if (!sh->opt[O_HISTEXPAND] || h->expinhibit || !h->expchar) return 1;
    HResult r = histexpand(sh, a, line, h->len, 0);
    if (r.r>=0 && r.r!=2) {
        if (r.r) *out = r.line;
        return 1;
    }
    if (quiet) return 1;  // the line as it is
    edput(ed, "\r\n");
    edflush(ed, sh);
    if (r.r < 0) {
        shellerror(sh, *a, r.err);
    } else {
        fdwrite(sh, 2, concat(a, r.line, "\n"));
    }
    edredraw(ed, sh);
    return 0;
}

// Point after bash's set_up_new_line: kept the same distance from the
// end, or put past the word it lands in.
static void edexpandpoint(Editor *ed, iz oldpoint, iz oldend)
{
    iz dist = oldend - oldpoint;
    iz len  = ed->line.len;
    u8 *s   = ed->line.data;
    if (!dist) {
        ed->point = len;
    } else if (!oldpoint || len<oldpoint) {
        ed->point = oldpoint<len ? oldpoint : len;
        if (ed->point<len && s[ed->point]!=' ' && s[ed->point]!='\t') {
            ed->point = edfwdword(ed, ed->point, 1);
        }
    } else {
        ed->point = len - dist;
    }
    if (ed->point < 0) ed->point = 0;
    if (ed->point > len) ed->point = len;
    ed->mark = 0;
}

static void edexpandhistline(Editor *ed, Shell *sh)
{
    Arena a = *scratchat(sh, sh->capdepth+1);
    Str   r = {};
    if (!edhistexpand(ed, sh, &a, &r)) return;
    if (r == edtext(ed)) return;
    iz oldpoint = ed->point;
    iz oldend   = ed->line.len;
    edreplace(ed, r);
    edexpandpoint(ed, oldpoint, oldend);
}

// Bash's shell-expand-line: history expansion, then the whole line
// expanded as one word, its fields joined by spaces.
static b32 edin(Str set, u8 c)
{
    for (iz i = 0; i < set.len; i++) {
        if (set[i] == c) return 1;
    }
    return 0;
}

// Aliases in the line, as bash's alias_expand for shell-expand-line:
// a word in command position (first, or after one of the separators
// ;|&( or a newline) with no quoting in it is replaced by its alias
// once, not again within the value, and a value ending in a blank
// puts the next word in command position too.
static Str edaliases(Shell *sh, Arena *a, Str s)
{
    Str breaks = "()<>;&| \t\n";
    Buf b(a, s.len+16);
    b32 cmd  = 1;
    b32 next = 0;
    for (iz i = 0; i < s.len;) {
        u8 c = s[i];
        if (c==' ' || c=='\t') {
            putbyte(&b, c);
            i++;
            continue;
        }
        iz  beg    = i;
        b32 quoted = 0;
        while (i<s.len && !edin(breaks, s[i])) {
            u8 q = s[i];
            if (q=='\\' && i+1<s.len) {
                quoted = 1;
                i += 2;
            } else if (q=='\'' || q=='"' || q=='`') {
                quoted = 1;
                for (i++; i<s.len && s[i]!=q; i++) {
                    if (s[i]=='\\' && q!='\'' && i+1<s.len) i++;
                }
                i += i < s.len;
            } else {
                quoted |= q == '$';
                i++;
            }
        }
        if (i == beg) {
            // A break character alone: a separator starts a command
            putbyte(&b, c);
            i++;
            cmd  = edin(";|&(\n", c);
            next = 0;
            continue;
        }
        Str    word = span(s.data+beg, s.data+i);
        Alias *al   = (cmd||next) && !quoted ? findalias(sh->aliases, word) : 0;
        if (al) {
            Str v = al->value;
            print(&b, v);
            next = v.len && (v[v.len-1]==' ' || v[v.len-1]=='\t');
        } else {
            print(&b, word);
            next = 0;
        }
        cmd = 0;
    }
    return finish(&b);
}

static void edshellexpand(Editor *ed, Shell *sh)
{
    // Its command substitutions run at the next depth: claim this one
    i32   depth = ++sh->capdepth;
    Arena a     = *scratchat(sh, depth);
    Str   r     = {};
    // As bash's shell-expand-line: an event not found is kept, unsaid
    edhistexpand(ed, sh, &a, &r, 1);
    if (sh->shopt[S_EXPAND_ALIASES]) r = edaliases(sh, &a, r);
    iz  oldpoint = ed->point;
    b32 atend    = ed->point == ed->line.len;
    edreplace(ed, r);

    Word w = {};
    w.text = clone(&a, edtext(ed));
    Slice<Str> fields = {};
    Escape e = {};
    e.up    = sh->esc;
    e.bout  = sh->bout;
    e.undo  = sh->undo;
    e.frame = sh->frame;
    sh->esc = &e;
    i32  status = sh->status;
    b32  ok     = 0;
    if (!__builtin_setjmp(e.jmp)) {
        fields = expandwords(sh, &a, &w, XW_NOBRACE|XW_NOGLOB);
        ok = 1;
    } else {
        unwind(sh, e.undo);
    }
    sh->esc      = e.up;
    sh->status   = status;
    sh->capdepth = depth - 1;
    if (!ok) {
        // As bash's jump to the top level: the line is given up
        ed->done   = 1;
        ed->result = ED_ABORT;
        return;
    }
    Buf b(&a, w.text.len+16);
    for (iz i = 0; i < fields.len; i++) {
        if (i) putbyte(&b, ' ');
        print(&b, fields[i]);
    }
    edreplace(ed, finish(&b));
    if (atend) {
        ed->point = ed->line.len;
    } else if (oldpoint < ed->line.len) {
        ed->point = oldpoint;
        u8 c = ed->line.data[oldpoint];
        if (c!=' ' && c!='\t') ed->point = edfwdword(ed, ed->point, 1);
    } else {
        ed->point = ed->line.len;
    }
}

// edit-and-execute-command (C-x C-e), as bash's: the line goes into
// history with an empty entry after it standing for fc's own, which fc
// drops; with the terminal restored, fc edits the line in $VISUAL or
// $EDITOR and runs what comes back; then a new empty line follows
// under the same prompt.
static void ededitexec(Editor *ed, Shell *sh)
{
    // The cursor past the line, as accept-line leaves it
    Layout *l = &ed->shown;
    if (ed->hscroll) {
        edput(ed, "\r\n");
    } else {
        edmove(ed, l->nrows-1, l->rows[l->nrows-1].width);
        edput(ed, "\r\n");
    }
    b32 bracket = !ed->hscroll && !ed->tty.legacy && ed->vars.bracketpaste;
    if (bracket) edput(ed, "\x1b[?2004l\r");
    edflush(ed, sh);
    reallyadd(sh, clone(&ed->linemem, edtext(ed)));
    reallyadd(sh, "");
    os_tty(sh->os, ed->in, ed->out, TTY_RESTORE);

    i32   depth  = ++sh->capdepth;
    Arena a      = *scratchat(sh, depth);
    i32   line   = sh->line;
    b32   lexeof = sh->lexeof;
    sh->evaldepth++;
    runsource(sh, "fc -e \"${VISUAL:-${EDITOR:-emacs}}\"", "C-xC-e", a);
    sh->evaldepth--;
    sh->capdepth = depth - 1;
    sh->line     = line;
    sh->lexeof   = lexeof;

    // The program may have resized the terminal
    Tty t = os_tty(sh->os, ed->in, ed->out, TTY_RAW);
    if (!t.err && t.cols>=2) {
        ed->tty.rows = t.rows;
        ed->tty.cols = t.cols;
        ed->width    = ed->tty.xenl ? t.cols : t.cols-1;
        edsize(ed, sh, t);
    }
    if (bracket) edput(ed, "\x1b[?2004h");
    ed->line.len = ed->point = 0;
    ed->mark     = 0;
    ed->undo     = 0;
    ed->hpos     = sh->hist.len;
    ed->hsaved   = 0;
    edredraw(ed, sh);
}


static void edarg(Editor *, Shell *, i32 key);

// Insert key count times, with the rest of a UTF-8 sequence it starts.
static void edselfinsert(Editor *ed, Shell *sh, u8 key, i32 count)
{
    u8 buf[4] = {key};
    iz n   = key>=0xf0 ? 4 : key>=0xe0 ? 3 : key>=0xc0 ? 2 : 1;
    iz len = 1;
    while (len < n) {
        i32 c = edkey(ed, sh, -1);
        if (c < 0) break;
        if ((c&0xc0) != 0x80) {
            edpush(ed, c);  // the sequence was cut short: a key of its own
            break;
        }
        buf[len++] = (u8)c;
    }
    if (count <= 0) return;
    Str ch = {buf, len};
    if (ed->overwrite) {
        edgroup(ed, U_BEGIN);
        for (i32 i = 0; i < count; i++) {
            edinsert(ed, ch);
            if (ed->point < ed->line.len) {
                eddelete(ed, ed->point, ednext(ed, ed->point));
            }
        }
        edgroup(ed, U_END);
    } else if (count == 1) {
        edinsert(ed, ch);
    } else {
        Buf b(&ed->hmem, len*count);
        for (i32 i = 0; i < count; i++) print(&b, ch);
        edinsert(ed, finish(&b));
    }
}

static void edrun(Editor *ed, Shell *sh, i32 fn, u8 key)
{
    iz  p     = ed->point;
    iz  len   = ed->line.len;
    i32 count = ed->count;
    switch (fn) {
    case EF_NONE:
    case EF_COMPLETE:
    case EF_POSSIBLE_COMPLETIONS:
    case EF_INSERT_COMPLETIONS:
    case EF_COMPLETE_FILENAME:
    case EF_POSSIBLE_FILENAME_COMPLETIONS:
    case EF_COMPLETE_USERNAME:
    case EF_POSSIBLE_USERNAME_COMPLETIONS:
    case EF_COMPLETE_HOSTNAME:
    case EF_POSSIBLE_HOSTNAME_COMPLETIONS:
    case EF_COMPLETE_VARIABLE:
    case EF_POSSIBLE_VARIABLE_COMPLETIONS:
    case EF_COMPLETE_COMMAND:
    case EF_POSSIBLE_COMMAND_COMPLETIONS:
    case EF_DYNAMIC_COMPLETE_HISTORY:
    case EF_GLOB_COMPLETE_WORD:
    case EF_GLOB_EXPAND_WORD:
    case EF_GLOB_LIST_EXPANSIONS:
    case EF_COMPLETE_INTO_BRACES:
    case EF_MENU_COMPLETE:
    case EF_MENU_COMPLETE_BACKWARD:
        edcomplete(ed, sh, fn, ed->argset ? ed->count : 1);
        break;
    case EF_RE_READ_INIT_FILE:
        rlinit(ed, sh);
        break;
    case EF_DELETE_CHAR_OR_LIST:
        if (ed->point<ed->line.len || !ed->line.len) {
            edrun(ed, sh, EF_DELETE_CHAR, key);
        } else {
            edcomplete(ed, sh, fn, 1);
        }
        break;
    default:
        edbell(ed);
        break;
    case EF_SELF_INSERT:
        edselfinsert(ed, sh, key, count);
        if (count!=1 || ed->overwrite) break;
        // Keys already typed that insert themselves go in too, as
        // readline batches typeahead, so the line is shown once
        while (!ed->nchars) {
            i32 c = edkey(ed, sh, 0);
            if (c < 0) break;
            KeyNode *k = keychild(&ed->root, (u8)c);
            if (!k || k->kids || k->fn!=EF_SELF_INSERT || c==ed->delim ||
                c==ed->tty.intr || (c=='\r' && ed->delim>=0)) {
                edpush(ed, c);  // dispatched next
                break;
            }
            edselfinsert(ed, sh, (u8)c, 1);
        }
        break;
    case EF_ACCEPT_LINE:
        ed->done   = 1;
        ed->result = ED_LINE;
        break;
    case EF_OPERATE_AND_GET_NEXT: {
        Hist *h = &sh->hist;
        ed->done    = 1;
        ed->result  = ED_LINE;
        ed->getnext = ed->argset ? count : h->base + ed->hpos + 1;
    } break;
    case EF_BEGINNING_OF_LINE:
        ed->point = 0;
        break;
    case EF_END_OF_LINE:
        ed->point = len;
        break;
    case EF_FORWARD_CHAR:
        ed->point = edcharmove(ed, p, count);
        break;
    case EF_BACKWARD_CHAR:
        ed->point = edcharmove(ed, p, -count);
        break;
    case EF_FORWARD_WORD:
        ed->point = edwordmove(ed, p, count);
        break;
    case EF_BACKWARD_WORD:
        ed->point = edwordmove(ed, p, -count);
        break;
    case EF_DELETE_CHAR:
    case EF_BACKWARD_DELETE_CHAR: {
        // Readline's rl_delete and rl_rubout: a negative count turns
        // either into the other (without overwrite's rubout)
        b32 back = (fn==EF_BACKWARD_DELETE_CHAR) != (count<0);
        b32 over = ed->overwrite && fn==EF_BACKWARD_DELETE_CHAR && count>0;
        i32 n    = count<0 ? -count : count;
        if (back ? !p : p==len) {
            edbell(ed);
        } else if (over) {
            iz  q = p;
            i32 w = 0;
            for (i32 i = 0; i < n; i++, w++) q = edcharmove(ed, q, -1);
            edgroup(ed, U_BEGIN);
            if (n>1 || ed->argset) {
                edkill(ed, p, q);
            } else {
                eddelete(ed, q, p);
            }
            ed->point = q;
            if (q < ed->line.len) {
                for (i32 i = 0; i < w; i++) edinsert(ed, " ");
                ed->point = q;
            }
            edgroup(ed, U_END);
        } else if (n>1 || ed->argset) {
            iz q = edcharmove(ed, p, back ? -n : n);
            edkill(ed, p, q);
            ed->point = back ? q : p;
        } else {
            iz q = back ? edprev(ed, p) : ednext(ed, p);
            eddelete(ed, q<p ? q : p, q<p ? p : q);
        }
    } break;
    case EF_KILL_LINE:
        if (count < 0) {
            edkill(ed, p, 0);
            ed->point = 0;
        } else {
            edkill(ed, p, len);
            ed->point = p;
        }
        ed->mark = ed->point;
        break;
    case EF_BACKWARD_KILL_LINE:
        if (count < 0) {
            edkill(ed, p, len);
            ed->point = p;
        } else if (!p) {
            edbell(ed);
        } else {
            edkill(ed, p, 0);
            ed->point = 0;
        }
        ed->mark = ed->point;
        break;
    case EF_KILL_WHOLE_LINE:
        edgroup(ed, U_BEGIN);
        ed->point = 0;
        edkill(ed, 0, len);
        ed->mark = 0;
        edgroup(ed, U_END);
        break;
    case EF_UNIX_LINE_DISCARD:
        if (!p) {
            edbell(ed);
        } else {
            edkill(ed, p, 0);
            ed->point = ed->mark = 0;
        }
        break;
    case EF_UNIX_WORD_RUBOUT:
    case EF_UNIX_FILENAME_RUBOUT: {
        if (!p) {
            edbell(ed);
            break;
        }
        u8 *s = ed->line.data;
        iz  i = p;
        for (i32 n = count<=0 ? 1 : count; n > 0; n--) {
            if (fn == EF_UNIX_WORD_RUBOUT) {
                for (; i>0 && (s[i-1]==' ' || s[i-1]=='\t'); i--) {}
                for (; i>0 && s[i-1]!=' ' && s[i-1]!='\t'; i--) {}
                continue;
            }
            for (; i>0 && (s[i-1]==' ' || s[i-1]=='\t'); i--) {}
            if (i>0 && s[i-1]=='/') {
                iz j = i - 1;
                for (; j>0 && s[j-1]=='/'; j--) {}
                if (!j || s[j-1]==' ' || s[j-1]=='\t') {
                    i = j;  // slashes only
                    continue;
                }
            }
            for (; i>0 && (s[i-1]==' ' || s[i-1]=='\t' || s[i-1]=='/'); i--) {}
            for (; i>0 && s[i-1]!=' ' && s[i-1]!='\t' && s[i-1]!='/'; i--) {}
        }
        edkill(ed, p, i);
        ed->point = ed->mark = i;
    } break;
    case EF_KILL_WORD:
    case EF_BACKWARD_KILL_WORD: {
        if (fn == EF_BACKWARD_KILL_WORD) count = -count;
        iz q = edwordmove(ed, p, count);
        if (q != p) edkill(ed, p, q);
        ed->point = ed->mark = q<p ? q : p;
    } break;
    case EF_SHELL_FORWARD_WORD:
    case EF_SHELL_BACKWARD_WORD: {
        b32 bell = 0;
        ed->point = edshellmove(ed, p, fn==EF_SHELL_FORWARD_WORD ? count : -count, &bell);
        if (bell) edbell(ed);
    } break;
    case EF_SHELL_KILL_WORD:
    case EF_SHELL_BACKWARD_KILL_WORD: {
        b32 bell = 0;
        if (fn == EF_SHELL_BACKWARD_KILL_WORD) count = -count;
        iz q = edshellmove(ed, p, count, &bell);
        if (bell) edbell(ed);
        if (q != p) edkill(ed, p, q);
        ed->point = ed->mark = q<p ? q : p;
    } break;
    case EF_KILL_REGION:
    case EF_COPY_REGION_AS_KILL: {
        iz m = ed->mark<len ? ed->mark : len;
        if (m != p) {
            if (fn == EF_KILL_REGION) {
                edkill(ed, p, m);
            } else {
                // A copy: killed and put straight back
                EdUndo *u = ed->undo;
                b32   o = ed->undoing;
                ed->undoing = 1;
                Str t = clone(&ed->hmem, {ed->line.data+(p<m?p:m), p<m?m-p:p-m});
                edkill(ed, p, m);
                ed->point = p<m ? p : m;
                edinsert(ed, t);
                ed->undoing = o;
                ed->undo    = u;
            }
        }
        ed->killed = 1;
        ed->point  = fn==EF_KILL_REGION ? (p<m ? p : m) : p;
    } break;
    case EF_COPY_FORWARD_WORD:
    case EF_COPY_BACKWARD_WORD: {
        if (fn == EF_COPY_BACKWARD_WORD) count = -count;
        iz    q = edwordmove(ed, p, count);
        EdUndo *u = ed->undo;
        b32   o = ed->undoing;
        ed->undoing = 1;
        Str t = clone(&ed->hmem, {ed->line.data+(p<q?p:q), p<q?q-p:p-q});
        edkill(ed, p, q);
        ed->point = p<q ? p : q;
        edinsert(ed, t);
        ed->undoing = o;
        ed->undo    = u;
        ed->killed  = 1;
        ed->point   = p;
    } break;
    case EF_YANK:
        edyank(ed);
        break;
    case EF_YANK_POP: {
        if ((ed->lastfn!=EF_YANK && ed->lastfn!=EF_YANK_POP) || !ed->nkills) {
            edbell(ed);
            break;
        }
        Str k = ed->kills[ed->killidx];
        iz  n = p - k.len;
        if (n<0 || !(Str{ed->line.data+n, k.len}==k)) {
            edbell(ed);
            break;
        }
        eddelete(ed, n, p);
        ed->point   = n;
        ed->killidx = ed->killidx ? ed->killidx-1 : ed->nkills-1;
        edyank(ed);
    } break;
    case EF_YANK_LAST_ARG:
        if (ed->lastfn != EF_YANK_LAST_ARG) {
            ed->ylskip  = 0;
            ed->ylarg   = ed->argset;
            ed->ylcount = count;
            ed->yldir   = 1;
        } else {
            if (ed->ylundo) edundo1(ed);
            if (count < 0) ed->yldir = -ed->yldir;
            ed->ylskip += ed->yldir;
            if (ed->ylskip < 0) ed->ylskip = 0;
        }
        ed->ylundo = edyankarg(ed, sh, ed->ylarg ? ed->ylcount : '$', ed->ylskip);
        break;
    case EF_YANK_NTH_ARG:
        edyankarg(ed, sh, count, 0);
        break;
    case EF_UNDO:
        for (i32 n = count; n > 0; n--) {
            if (!edundo1(ed)) {
                edbell(ed);
                break;
            }
        }
        break;
    case EF_REVERT_LINE:
        if (!ed->undo) {
            edbell(ed);
        } else {
            while (ed->undo) edundo1(ed);
        }
        break;
    case EF_SET_MARK: {
        iz at = ed->argset ? count : p;
        if (at>=0 && at<=len) ed->mark = at;
    } break;
    case EF_EXCHANGE_POINT_AND_MARK:
        if (ed->mark<0 || ed->mark>len) {
            edbell(ed);
            ed->mark = 0;
        } else {
            ed->point = ed->mark;
            ed->mark  = p;
            if (!ed->hscroll) edactivate(ed);
        }
        break;
    case EF_CAPITALIZE_WORD:
        edcase(ed, count, CASE_CAP);
        break;
    case EF_UPCASE_WORD:
        edcase(ed, count, CASE_UP);
        break;
    case EF_DOWNCASE_WORD:
        edcase(ed, count, CASE_DOWN);
        break;
    case EF_TRANSPOSE_WORDS:
        edtransposewords(ed, count, 0);
        break;
    case EF_SHELL_TRANSPOSE_WORDS:
        edtransposewords(ed, count, 1);
        break;
    case EF_DELETE_HORIZONTAL_SPACE: {
        u8 *s = ed->line.data;
        iz  b = p;
        for (; b>0 && (s[b-1]==' ' || s[b-1]=='\t'); b--) {}
        iz  e = b;
        for (; e<len && (s[e]==' ' || s[e]=='\t'); e++) {}
        eddelete(ed, b, e);
        ed->point = b;
    } break;
    case EF_INSERT_COMMENT: {
        Str com = ed->vars.comment;
        ed->point = 0;
        if (!ed->argset) {
            edinsert(ed, com);
        } else if (len>=com.len && Str{ed->line.data, com.len}==com) {
            eddelete(ed, 0, com.len);
        } else {
            edinsert(ed, com);
        }
        ed->done   = 1;
        ed->result = ED_LINE;
    } break;
    case EF_CHARACTER_SEARCH:
        edcharsearch(ed, sh, count, 1);
        break;
    case EF_CHARACTER_SEARCH_BACKWARD:
        edcharsearch(ed, sh, count, -1);
        break;
    case EF_OVERWRITE_MODE:
        if (!ed->argset) {
            ed->overwrite = !ed->overwrite;
        } else {
            ed->overwrite = count > 0;
        }
        break;
    case EF_CLEAR_SCREEN:
        if (ed->argset) {
            ed->keepactive = ed->active;
            edrefresh(ed, sh);
            return;
        }
        // fallthrough
    case EF_CLEAR_DISPLAY:
        ed->keepactive = ed->active;
        edput(ed, fn==EF_CLEAR_SCREEN ? Str("\x1b[H\x1b[2J") : "\x1b[H\x1b[2J\x1b[3J");
        edflush(ed, sh);
        edredraw(ed, sh);
        return;
    case EF_ABORT:
        edbell(ed);
        break;
    case EF_TRANSPOSE_CHARS: {
        if (!count) break;
        if (!p || len<2) {
            edbell(ed);
            break;
        }
        edgroup(ed, U_BEGIN);
        if (p == len) {
            ed->point = edprev(ed, p);
            count = 1;
        }
        iz at = ed->point;
        iz a  = edprev(ed, at);
        Str c = clone(&ed->hmem, {ed->line.data+a, at-a});
        eddelete(ed, a, at);
        ed->point = a;
        if (count > 0) {
            for (i32 i = 0; i < count; i++) ed->point = ednext(ed, ed->point);
        } else {
            for (i32 i = 0; i < -count; i++) ed->point = edprev(ed, ed->point);
        }
        edinsert(ed, c);
        edgroup(ed, U_END);
    } break;
    case EF_QUOTED_INSERT: {
        for (i32 n = count<0 ? -count : 1;  n > 0; n--) {
            i32 c = edkey(ed, sh, -1);
            if (c < 0) break;
            edselfinsert(ed, sh, (u8)c, count<0 ? 1 : count);
        }
    } break;
    case EF_TAB_INSERT:
        edselfinsert(ed, sh, '\t', count);
        break;
    case EF_PREVIOUS_HISTORY:
        edprevhist(ed, sh, count);
        break;
    case EF_NEXT_HISTORY:
        ednexthist(ed, sh, count);
        break;
    case EF_BEGINNING_OF_HISTORY:
        edprevhist(ed, sh, ed->hpos+1);
        break;
    case EF_END_OF_HISTORY:
        edkeepline(ed, sh);
        ed->hpos = sh->hist.len;
        edunsaveline(ed);
        break;
    case EF_REDRAW_CURRENT_LINE:
        ed->keepactive = ed->active;
        edrefresh(ed, sh);
        return;
    case EF_REVERSE_SEARCH_HISTORY:
        edisearch(ed, sh, -(i64)count);
        break;
    case EF_FORWARD_SEARCH_HISTORY:
        edisearch(ed, sh, count);
        break;
    case EF_NON_INCREMENTAL_REVERSE_SEARCH_HISTORY:
        ednoninc(ed, sh, -1);
        break;
    case EF_NON_INCREMENTAL_FORWARD_SEARCH_HISTORY:
        ednoninc(ed, sh, 1);
        break;
    case EF_HISTORY_SEARCH_BACKWARD:
    case EF_HISTORY_SEARCH_FORWARD:
        edhistsearch(ed, sh, fn, count);
        break;
    case EF_HISTORY_EXPAND_LINE:
        edexpandhistline(ed, sh);
        break;
    case EF_MAGIC_SPACE: {
        Arena a = *scratchat(sh, sh->capdepth+1);
        Str   r = {};
        if (!edhistexpand(ed, sh, &a, &r)) break;
        if (!(r == edtext(ed))) {
            iz dist = len - p;
            edreplace(ed, r);
            ed->point = p ? ed->line.len-dist : 0;
            if (ed->point < 0) ed->point = 0;
        }
        edselfinsert(ed, sh, ' ', 1);
    } break;
    case EF_SHELL_EXPAND_LINE:
        edshellexpand(ed, sh);
        break;
    case EF_EDIT_AND_EXECUTE_COMMAND:
        ededitexec(ed, sh);
        return;
    case EF_BRACKETED_PASTE_BEGIN: {
        Str t = edpaste(ed, sh, &ed->hmem);
        ed->mark = ed->point;
        edinsert(ed, t);
        if (!ed->hscroll) edactivate(ed);
    } break;
    }
}


// Input

// The next key byte, waiting at most timeout ms (-1: forever): -1 at
// end of input or on error, -2 on an interrupt, -3 on timeout. A
// signal meanwhile (SIGWINCH) checks the terminal's size.
static i32 edkey(Editor *ed, Shell *sh, i64 timeout)
{
    if (ed->unread) return ed->pushed[--ed->unread];
    if (ed->macropos < ed->macro.len) return ed->macro[ed->macropos++];
    if (ed->timedout) return -1;
    if (!timeout && !taskreadable(sh->sched, ed->fin->h)) return -3;
    Task *self = sh->sched->cur;
    i64   when = timeout<0 ? 0 : os_now(sh->os) + timeout*1000000;
    b32   last = ed->deadline && (!when || ed->deadline<=when);
    if (last) when = ed->deadline;
    u8    c;
    iz    n;
    for (;;) {
        self->deadline = when;
        self->introk   = self->anysig = sh->intredit = 1;
        n = readstdin(sh, ed->fin, &c, 1);
        self->introk   = self->anysig = sh->intredit = 0;
        if (n != -4) break;
        edresize(ed, sh);
        edwinch(ed, sh);
    }
    self->deadline = 0;
    if (self->timedout) {
        self->timedout = 0;
        if (!last) return -3;
        ed->timedout = 1;  // read -t: as at the end of input
        return -1;
    }
    return n==1 ? c : n==-2 ? -2 : -1;
}

// A key sequence read and looked up: its function and last key, or a
// status (ED_EOF at the end of input, ED_INTR on an interrupt).
struct EdSeq {
    Str macro;   // EF_MACRO
    i32 fn;
    u8  key;
    u8  first;   // its first key
    i32 status;  // -1 for a sequence
};

static EdSeq edreadseq(Editor *ed, Shell *sh)
{
    EdSeq r = {};
    r.status = -1;
    i32 c = edkey(ed, sh, -1);
    if (c == -2 || c == ed->tty.intr) {
        r.status = ED_INTR;
        return r;
    }
    if (c < 0) {
        r.status = ED_EOF;
        return r;
    }
    r.first = (u8)c;
    if (c == ed->delim) {
        r.fn  = EF_ACCEPT_LINE;
        r.key = (u8)c;
        return r;
    } else if (c=='\r' && ed->delim>=0) {
        r.fn  = EF_SELF_INSERT;
        r.key = '\r';
        return r;
    }
    KeyNode *parent = &ed->root;
    KeyNode *n      = keychild(parent, (u8)c);
    u8 last = (u8)c;
    i64 wait = ed->keytimeout>0 ? ed->keytimeout : -1;
    while (n && n->kids) {
        i32 next = edkey(ed, sh, n->fn ? wait : -1);
        if (next == -3) break;  // the prefix's own binding
        if (next == -2) {
            r.status = ED_INTR;
            return r;
        }
        if (next < 0) {
            r.status = ED_EOF;
            return r;
        }
        KeyNode *k = keychild(n, (u8)next);
        if (!k && n->fn && n->fn!=EF_DO_LOWERCASE_VERSION) {
            // The prefix's own binding, shadowed by longer ones, and
            // the key runs after it
            edpush(ed, next);
            break;
        } else if (!k) {
            // An unbound sequence: the bell, the rest dropped (also
            // under M-O, the arrow keys' prefix, as readline)
            n = 0;
            break;
        }
        parent = n;
        n      = k;
        last   = (u8)next;
    }
    r.fn    = n ? n->fn : EF_NONE;
    r.macro = n ? n->macro : Str{};
    r.key   = last;
    if (r.fn==EF_DO_LOWERCASE_VERSION && last>='A' && last<='Z') {
        KeyNode *k = keychild(parent, last+0x20);
        r.fn    = k ? k->fn : EF_NONE;
        r.macro = k ? k->macro : Str{};
        r.key   = last + 0x20;
        if (!k && parent->fn) {
            // The prefix's shadowed binding, then the lower-case key
            edpush(ed, last+0x20);
            r.fn    = parent->fn;
            r.macro = parent->macro;
            r.key   = parent->key;
        }
        if (r.fn == EF_DO_LOWERCASE_VERSION) r.fn = EF_NONE;
    }
    return r;
}

static void edresetarg(Editor *ed)
{
    ed->count  = 1;
    ed->argset = 0;
    ed->argneg = 0;
}

// Read and run one command, a numeric argument first if it starts with
// one. False at the end of input.
static b32 eddispatch(Editor *ed, Shell *sh)
{
    EdSeq s = edreadseq(ed, sh);
    if (s.status == ED_INTR) {
        ed->done   = 1;
        ed->result = ED_INTR;
        return 1;
    }
    if (s.status == ED_EOF) return 0;
    // The terminal's EOF character, not twice in a row, on an empty line
    u8 first  = s.first;
    b32 again = first == ed->lastc;
    ed->lastc = first;
    if (first==ed->eof && !again && !ed->line.len) {
        ed->done   = 1;
        ed->result = ED_EOF;
        return 1;
    }
    switch (s.fn) {
    case EF_DIGIT_ARGUMENT:
        edarg(ed, sh, s.key);
        break;
    case EF_MACRO: {
        // Its keys come next, before the rest of any running macro
        Str rest = cuthead(ed->macro, ed->macropos);
        ed->macro    = rest.len ? concat(&ed->perm, s.macro, rest) : s.macro;
        ed->macropos = 0;
    } break;
    case EF_UNIVERSAL_ARGUMENT:
        ed->count *= 4;
        edarg(ed, sh, -1);
        ed->lastfn = EF_UNIVERSAL_ARGUMENT;
        break;
    default:
        ed->killed = 0;
        edrun(ed, sh, s.fn, s.key);
        ed->lastkill = ed->killed;
        ed->lastfn   = s.fn;
    }
    return 1;
}

// Readline's numeric argument loop: digits and a leading minus, with
// "(arg: N) " shown in place of the prompt, until another key, which
// runs with it. key starts it (a digit or '-'), or -1.
static void edarg(Editor *ed, Shell *sh, i32 key)
{
    b32 sawdigits = 0;
    b32 sawminus  = 0;
    Str prompt    = ed->prompt;
    for (;;) {
        Arena a = ed->linemem;
        Buf   b(&a, 32);
        print(&b, "(arg: ");
        print(&b, (i64)ed->count*(ed->argneg ? -1 : 1));
        print(&b, ") ");
        ed->prompt = finish(&b);
        redisplay(ed, sh);
        ed->prompt = prompt;
        i32 c = key;
        key   = -1;
        if (c < 0) {
            c = edkey(ed, sh, -1);
            if (c == -2 || c == ed->tty.intr) {
                ed->done   = 1;
                ed->result = ED_INTR;
                return;
            }
            if (c < 0) {
                edbell(ed);
                edresetarg(ed);
                return;
            }
            KeyNode *k = keychild(&ed->root, (u8)c);
            if (k && !k->kids && k->fn==EF_UNIVERSAL_ARGUMENT) {
                if (!sawdigits) {
                    ed->count *= 4;
                    continue;
                }
                ed->prompt = prompt;
                redisplay(ed, sh);
                eddispatch(ed, sh);
                return;
            }
        }
        if (c>='0' && c<='9') {
            // An argument already negated (M-- then a key) grows away
            // from zero too; past a million in size, as readline, none
            i64 d = c - '0';
            i64 n = !ed->argset ? d : ed->count<0 ? ed->count*10-d : ed->count*10+d;
            ed->argset = 1;
            sawdigits  = 1;
            if (n<-1000000 || n>1000000) {
                edbell(ed);
                edresetarg(ed);
                ed->count = 0;
                ed->prompt = prompt;
                return;
            }
            ed->count = (i32)n;
        } else if (c=='-' && !ed->argset) {
            ed->count  = 1;
            ed->argneg = 1;
            sawminus   = 1;
        } else {
            // M-- alone is M--1
            if (sawminus && ed->count==1 && !ed->argset) ed->argset = 1;
            if (ed->argneg) ed->count = -ed->count;
            ed->argneg = 0;
            ed->prompt = prompt;
            redisplay(ed, sh);  // as readline, the prompt back first
            edpush(ed, c);
            eddispatch(ed, sh);
            return;
        }
    }
}

// LINES and COLUMNS (plain shell variables) follow the terminal's size
// when it changes, as readline sets them at its start and on a resize.
static void edsize(Editor *ed, Shell *sh, Tty t)
{
    if (t.err || t.rows<1 || t.cols<1) return;
    if (t.rows==ed->setrows && t.cols==ed->setcols) return;
    ed->setrows = t.rows;
    ed->setcols = t.cols;
    Arena a = *scratchat(sh, sh->capdepth+1);
    setvar(sh, a, "LINES",   fmti64(&a, t.rows));
    setvar(sh, a, "COLUMNS", fmti64(&a, t.cols));
}

// The number of characters in the line, for read -n.
static i64 edchars(Editor *ed)
{
    i64 n = 0;
    for (iz i = 0; i < ed->line.len; i++) n += (ed->line.data[i]&0xc0) != 0x80;
    return n;
}

// Read a line on the terminal with the editor, the prompt shown first.
// ED_NOTTY if the input (descriptor 0, or opts->fd) and descriptor 2
// are not one terminal. With opts, for read -e.
// Whether to draw as on a dumb terminal: TERM=dumb and, as readline
// does when the terminal database knows no such terminal, on POSIX an
// unset TERM or one without a terminfo entry. A Windows console needs
// no TERM.
static b32 eddumb(Shell *sh, Arena a)
{
    Got term = getvar(sh, &a, "TERM");
    Str name = term.value;
    if (name == "dumb") return 1;
    if (sh->pathmode != PM_POSIX) return 0;
    if (!name.len) return 1;
    for (iz i = 0; i < name.len; i++) {
        if (name[i] == '/') return 1;
    }

    // ncurses's search: TERMINFO, ~/.terminfo, TERMINFO_DIRS (an empty
    // element and an unset variable meaning the system's), the system's
    Str dirs[16];
    i32 ndirs = 0;
    Got ti = getvar(sh, &a, "TERMINFO");
    if (ti.value.len) dirs[ndirs++] = ti.value;
    Got home = getvar(sh, &a, "HOME");
    if (home.value.len) dirs[ndirs++] = concat(&a, home.value, "/.terminfo");
    Str sys[] = {"/etc/terminfo", "/lib/terminfo", "/usr/share/terminfo"};
    Got td = getvar(sh, &a, "TERMINFO_DIRS");
    b32 addsys = !td.set;
    for (Str rest = td.value; td.set;) {
        Cut c = cut(rest, ':');
        if (!c.head.len) {
            addsys = 1;
        } else if (ndirs < countof(dirs)-countof(sys)) {
            dirs[ndirs++] = c.head;
        }
        if (!c.ok) break;
        rest = c.tail;
    }
    if (addsys) {
        for (iz i = 0; i < countof(sys); i++) dirs[ndirs++] = sys[i];
    }

    u8 hex[] = "0123456789abcdef";
    for (i32 i = 0; i < ndirs; i++) {
        u8  c = name[0];
        Str sub[] = {{&c, 1}, {}};
        u8  h[2] = {hex[c>>4], hex[c&15]};
        sub[1] = {h, 2};
        for (iz j = 0; j < countof(sub); j++) {
            Arena t = a;
            Buf   b(&t, 64);
            print(&b, dirs[i]);
            print(&b, "/");
            print(&b, sub[j]);
            print(&b, "/");
            print(&b, name);
            if (!pathstat(sh, t, finish(&b), 1).err) return 0;
        }
    }
    return 1;
}

static i32 editline(Shell *sh, Str prompt, Buf *out, EdRead *opts)
{
    EdRead none = {};
    none.delim = -1;
    opts = opts ? opts : &none;
    b32 isread = opts!=&none && !opts->shell;
    File *fin  = sh->fds->fd[opts->fd];
    File *ferr = sh->fds->fd[2];
    if (!fin || !ferr || fin->kind!=F_OS || ferr->kind!=F_OS) return ED_NOTTY;
    Editor *ed = editor(sh);
    ed->in  = fin->h;
    ed->out = ferr->h;
    ed->tty = os_tty(sh->os, ed->in, ed->out, TTY_RAW);
    if (ed->tty.err) return ED_NOTTY;
    edsize(ed, sh, ed->tty);
    if (ed->tty.cols < 2) ed->tty.cols = 80;
    ed->width = ed->tty.xenl ? ed->tty.cols : ed->tty.cols-1;

    ed->linemem = {(byte *)(ed->linemem.mem+1), ed->linemem.end, ed->linemem.mem};
    prompt      = clone(&ed->linemem, prompt);  // callers' scratch is reused
    ed->line    = Buf(&ed->linemem, 256);
    ed->out_    = Buf(&ed->linemem, 256);
    ed->done    = 0;
    ed->fin     = fin;
    ed->delim   = opts->delim;
    ed->nchars  = opts->nchars;
    ed->silent  = opts->silent;
    ed->deadline = opts->deadline;
    ed->timedout = 0;
    ed->hscroll = ed->tty.rows<=1 || eddumb(sh, ed->linemem);
    ed->lmargin = ed->hshown = ed->unread = 0;
    if (!ed->inited) {
        ed->inited = 1;
        if (ed->hscroll) ed->vars.bracketpaste = ed->vars.activeregion = 0;
        rlstart(ed, sh);
    }
    rlttychars(ed);
    print(&ed->line, opts->init);
    ed->point   = ed->line.len;
    ed->eof     = ed->tty.eof ? ed->tty.eof : 0x04;
    ed->lastc   = 0;
    ed->mark    = ed->lastfn = 0;
    ed->lastkill = ed->overwrite = ed->hsaved = 0;
    ed->undo    = 0;
    ed->active  = ed->keepactive = 0;
    ed->hpos    = sh->hist.len;
    edresetarg(ed);
    // EdUndo lists last while history entries keep them
    b32 kept = 0;
    for (iz i = 0; i < sh->hist.len; i++) kept |= !!sh->hist.ents[i]->undo;
    if (!kept) ed->hmem = {(byte *)(ed->hmem.mem+1), ed->hmem.end, ed->hmem.mem};

    // Lines of the prompt before its last are written once
    iz nl = prompt.len;
    while (nl>0 && prompt[nl-1]!='\n') nl--;
    Str head = takehead(prompt, nl);
    for (iz i = 0; i < head.len; i++) {
        if (head[i]!=1 && head[i]!=2) putbyte(&ed->out_, head[i]);
    }
    ed->prompt = cuthead(prompt, nl);
    ed->phead  = head;
    ed->shellline = opts->shell;
    ed->ps1       = opts->ps1;
    ed->cchanged  = ed->cfailed = 0;
    ed->menudone  = 1;
    if (ed->getnext >= 0) {
        // operate-and-get-next: start on the entry after the one run
        i64 count = sh->hist.base + sh->hist.len - ed->getnext;
        ed->getnext = -1;
        edprevhist(ed, sh, count);
    }
    // Bracketed paste, as readline's enable-bracketed-paste, except where
    // there are no escape sequences
    b32 bracket = !ed->hscroll && !ed->tty.legacy && ed->vars.bracketpaste;
    if (bracket) edput(ed, "\x1b[?2004h");

    // As readline, the first draw starts wherever the cursor is, taken
    // as column 0, so a prompt follows output with no final newline.
    ed->shown   = {};
    ed->crow    = 0;
    ed->ccol    = 0;
    ed->pending = 0;
    #ifdef EDIT_CHECK
    edflush(ed, sh);
    edcheck(ed, sh, EDCHECK_START);
    #endif
    redisplay(ed, sh);

    for (;;) {
        edresetarg(ed);
        b32 ok = eddispatch(ed, sh);
        if (ed->keepactive) {
            ed->keepactive = 0;
        } else {
            ed->active = 0;
        }
        if (ed->timedout) {
            ed->result = ED_TIMEOUT;
            break;
        }
        if (!ok) {
            ed->result = ED_EOF;
            break;
        }
        if (!ed->done && ed->nchars && edchars(ed)>=ed->nchars) {
            ed->done   = 1;  // shown first
            ed->result = ED_LINE;
        }
        if (ed->done && ed->result!=ED_LINE) break;
        redisplay(ed, sh);
        if (ed->done) break;
    }

    edleave(ed, sh, ed->result==ED_INTR);

    // Leave the cursor past the line
    Layout *l = &ed->shown;
    b32 crlf = 1;
    if (ed->result == ED_ABORT) {
        ed->line.len = ed->point = 0;  // the message is past the line
        crlf = 0;
    } else if (ed->result == ED_TIMEOUT) {
        // As readline's timeout: the line goes unused, and what follows
        // is written over it from the cursor's row
        ed->line.len = ed->point = 0;
        edput(ed, "\r");
        crlf = 0;
    } else if (ed->hscroll) {
        if (ed->result == ED_INTR) edput(ed, "^C");
    } else if (ed->result == ED_INTR) {
        // At the prompt with INT trapped, as bash: the line goes, and
        // the trap writes over it from the start of its row (fillline)
        b32 trapped = opts->shell && trapset(sh, 2) && interactivetop(sh);
        if (isread || trapped) {
            // read -e: as readline on SIGINT, the line goes first
            ed->line.len = ed->point = 0;
            redisplay(ed, sh);
        }
        // Echoed at the cursor, over the rest of the line, as by the
        // terminal's own line discipline
        edput(ed, "^C");
        if (ed->crow < l->nrows-1) edmove(ed, l->nrows-1, 0);
        if (trapped) {
            edput(ed, "\r");
            crlf = 0;
        }
    } else if (l->nrows>1 && !l->rows[l->nrows-1].ncells && !ed->ccol && !ed->pending) {
        // As readline: the cursor alone at the start of the empty row
        // past a full one is already past the line
        edmove(ed, l->nrows-1, 0);
        crlf = 0;
    } else {
        edmove(ed, l->nrows-1, l->rows[l->nrows-1].width);
    }
    // read -e: the shell's interrupt writes the newline
    if (crlf && !(ed->result==ED_INTR && isread)) edput(ed, "\r\n");
    if (bracket) edput(ed, "\x1b[?2004l\r");
    edflush(ed, sh);
    edsize(ed, sh, os_tty(sh->os, ed->in, ed->out, TTY_RESTORE));
    print(out, {ed->line.data, ed->line.len});
    return ed->result;
}
