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
    // Named for bind and inputrc, not all of them implemented yet
    EF_ALIAS_EXPAND_LINE,
    EF_ARROW_KEY_PREFIX,
    EF_BACKWARD_BYTE,
    EF_BASH_VI_COMPLETE,
    EF_CALL_LAST_KBD_MACRO,
    EF_DABBREV_EXPAND,
    EF_DISPLAY_SHELL_VERSION,
    EF_DUMP_FUNCTIONS,
    EF_DUMP_MACROS,
    EF_DUMP_VARIABLES,
    EF_EMACS_EDITING_MODE,
    EF_END_KBD_MACRO,
    EF_EXECUTE_NAMED_COMMAND,
    EF_EXPORT_COMPLETIONS,
    EF_FETCH_HISTORY,
    EF_FORWARD_BACKWARD_DELETE_CHAR,
    EF_FORWARD_BYTE,
    EF_HISTORY_AND_ALIAS_EXPAND_LINE,
    EF_NEXT_SCREEN_LINE,
    EF_NON_INCREMENTAL_FORWARD_SEARCH_HISTORY_AGAIN,
    EF_NON_INCREMENTAL_REVERSE_SEARCH_HISTORY_AGAIN,
    EF_OLD_MENU_COMPLETE,
    EF_PREVIOUS_SCREEN_LINE,
    EF_PRINT_LAST_KBD_MACRO,
    EF_SKIP_CSI_SEQUENCE,
    EF_SPELL_CORRECT_WORD,
    EF_START_KBD_MACRO,
    EF_TILDE_EXPAND,
    EF_TTY_STATUS,
    EF_VI_APPEND_EOL,
    EF_VI_APPEND_MODE,
    EF_VI_ARG_DIGIT,
    EF_VI_BACK_TO_INDENT,
    EF_VI_BACKWARD_BIGWORD,
    EF_VI_BACKWARD_WORD,
    EF_VI_BWORD,
    EF_VI_BWORD_CAP,
    EF_VI_CHANGE_CASE,
    EF_VI_CHANGE_CHAR,
    EF_VI_CHANGE_TO,
    EF_VI_CHAR_SEARCH,
    EF_VI_COLUMN,
    EF_VI_COMPLETE,
    EF_VI_DELETE,
    EF_VI_DELETE_TO,
    EF_VI_EDIT_AND_EXECUTE_COMMAND,
    EF_VI_EDITING_MODE,
    EF_VI_END_BIGWORD,
    EF_VI_END_WORD,
    EF_VI_EOF_MAYBE,
    EF_VI_EWORD,
    EF_VI_EWORD_CAP,
    EF_VI_FETCH_HISTORY,
    EF_VI_FIRST_PRINT,
    EF_VI_FORWARD_BIGWORD,
    EF_VI_FORWARD_WORD,
    EF_VI_FWORD,
    EF_VI_FWORD_CAP,
    EF_VI_GOTO_MARK,
    EF_VI_INSERT_BEG,
    EF_VI_INSERTION_MODE,
    EF_VI_MATCH,
    EF_VI_MOVEMENT_MODE,
    EF_VI_NEXT_WORD,
    EF_VI_OVERSTRIKE,
    EF_VI_OVERSTRIKE_DELETE,
    EF_VI_PREV_WORD,
    EF_VI_PUT,
    EF_VI_REDO,
    EF_VI_REPLACE,
    EF_VI_RUBOUT,
    EF_VI_SEARCH,
    EF_VI_SEARCH_AGAIN,
    EF_VI_SET_MARK,
    EF_VI_SUBST,
    EF_VI_TILDE_EXPAND,
    EF_VI_UNDO,
    EF_VI_UNIX_WORD_RUBOUT,
    EF_VI_YANK_ARG,
    EF_VI_YANK_POP,
    EF_VI_YANK_TO,
    EF_VI_OVERSTRIKE_KILL_LINE,  // vi replace mode's own, unnamed
    EF_VI_OVERSTRIKE_KILL_WORD,
    EF_VI_OVERSTRIKE_YANK,
    EF_VI_OVERSTRIKE_PASTE,
    EF_POSIX_EDIT_MACROS,  // bash's vi @: the alias _c as keys
    EF_NULL_FUNCTION,  // does nothing: a bound prefix that only waits
    EF_MACRO,  // a key bound to text (inputrc)
    EF_SHELLCMD,  // bind -x: a shell command, the text in macro
    EF_COUNT,
};

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
    "alias-expand-line",
    "arrow-key-prefix",
    "backward-byte",
    "bash-vi-complete",
    "call-last-kbd-macro",
    "dabbrev-expand",
    "display-shell-version",
    "dump-functions",
    "dump-macros",
    "dump-variables",
    "emacs-editing-mode",
    "end-kbd-macro",
    "execute-named-command",
    "export-completions",
    "fetch-history",
    "forward-backward-delete-char",
    "forward-byte",
    "history-and-alias-expand-line",
    "next-screen-line",
    "non-incremental-forward-search-history-again",
    "non-incremental-reverse-search-history-again",
    "old-menu-complete",
    "previous-screen-line",
    "print-last-kbd-macro",
    "skip-csi-sequence",
    "spell-correct-word",
    "start-kbd-macro",
    "tilde-expand",
    "tty-status",
    "vi-append-eol",
    "vi-append-mode",
    "vi-arg-digit",
    "vi-back-to-indent",
    "vi-backward-bigword",
    "vi-backward-word",
    "vi-bword",
    "vi-bWord",
    "vi-change-case",
    "vi-change-char",
    "vi-change-to",
    "vi-char-search",
    "vi-column",
    "vi-complete",
    "vi-delete",
    "vi-delete-to",
    "vi-edit-and-execute-command",
    "vi-editing-mode",
    "vi-end-bigword",
    "vi-end-word",
    "vi-eof-maybe",
    "vi-eword",
    "vi-eWord",
    "vi-fetch-history",
    "vi-first-print",
    "vi-forward-bigword",
    "vi-forward-word",
    "vi-fword",
    "vi-fWord",
    "vi-goto-mark",
    "vi-insert-beg",
    "vi-insertion-mode",
    "vi-match",
    "vi-movement-mode",
    "vi-next-word",
    "vi-overstrike",
    "vi-overstrike-delete",
    "vi-prev-word",
    "vi-put",
    "vi-redo",
    "vi-replace",
    "vi-rubout",
    "vi-search",
    "vi-search-again",
    "vi-set-mark",
    "vi-subst",
    "vi-tilde-expand",
    "vi-undo",
    "vi-unix-word-rubout",
    "vi-yank-arg",
    "vi-yank-pop",
    "vi-yank-to",
    "", "", "", "", "",
    "", "",
    "",
};

// Every function name, in the order bind -l and bind -p list them
static Str rlfunmap[] = {
    "abort", "accept-line", "alias-expand-line", "arrow-key-prefix",
    "backward-byte", "backward-char", "backward-delete-char",
    "backward-kill-line", "backward-kill-word", "backward-word",
    "bash-vi-complete", "beginning-of-history", "beginning-of-line",
    "bracketed-paste-begin", "call-last-kbd-macro", "capitalize-word",
    "character-search", "character-search-backward", "clear-display",
    "clear-screen", "complete", "complete-command", "complete-filename",
    "complete-hostname", "complete-into-braces", "complete-username",
    "complete-variable", "copy-backward-word", "copy-forward-word",
    "copy-region-as-kill", "dabbrev-expand", "delete-char",
    "delete-char-or-list", "delete-horizontal-space", "digit-argument",
    "display-shell-version", "do-lowercase-version", "downcase-word",
    "dump-functions", "dump-macros", "dump-variables",
    "dynamic-complete-history", "edit-and-execute-command",
    "emacs-editing-mode", "end-kbd-macro", "end-of-history", "end-of-line",
    "exchange-point-and-mark", "execute-named-command", "export-completions",
    "fetch-history", "forward-backward-delete-char", "forward-byte",
    "forward-char", "forward-search-history", "forward-word",
    "glob-complete-word", "glob-expand-word", "glob-list-expansions",
    "history-and-alias-expand-line", "history-expand-line",
    "history-search-backward", "history-search-forward",
    "history-substring-search-backward", "history-substring-search-forward",
    "insert-comment", "insert-completions", "insert-last-argument",
    "kill-line", "kill-region", "kill-whole-line", "kill-word", "magic-space",
    "menu-complete", "menu-complete-backward", "next-history",
    "next-screen-line", "non-incremental-forward-search-history",
    "non-incremental-forward-search-history-again",
    "non-incremental-reverse-search-history",
    "non-incremental-reverse-search-history-again", "old-menu-complete",
    "operate-and-get-next", "overwrite-mode", "possible-command-completions",
    "possible-completions", "possible-filename-completions",
    "possible-hostname-completions", "possible-username-completions",
    "possible-variable-completions", "previous-history",
    "previous-screen-line", "print-last-kbd-macro", "quoted-insert",
    "redraw-current-line", "re-read-init-file", "reverse-search-history",
    "revert-line", "self-insert", "set-mark", "shell-backward-kill-word",
    "shell-backward-word", "shell-expand-line", "shell-forward-word",
    "shell-kill-word", "shell-transpose-words", "skip-csi-sequence",
    "spell-correct-word", "start-kbd-macro", "tab-insert", "tilde-expand",
    "transpose-chars", "transpose-words", "tty-status", "undo",
    "universal-argument", "unix-filename-rubout", "unix-line-discard",
    "unix-word-rubout", "upcase-word", "vi-append-eol", "vi-append-mode",
    "vi-arg-digit", "vi-back-to-indent", "vi-backward-bigword",
    "vi-backward-word", "vi-bword", "vi-bWord", "vi-change-case",
    "vi-change-char", "vi-change-to", "vi-char-search", "vi-column",
    "vi-complete", "vi-delete", "vi-delete-to", "vi-edit-and-execute-command",
    "vi-editing-mode", "vi-end-bigword", "vi-end-word", "vi-eof-maybe",
    "vi-eword", "vi-eWord", "vi-fetch-history", "vi-first-print",
    "vi-forward-bigword", "vi-forward-word", "vi-fword", "vi-fWord",
    "vi-goto-mark", "vi-insert-beg", "vi-insertion-mode", "vi-match",
    "vi-movement-mode", "vi-next-word", "vi-overstrike",
    "vi-overstrike-delete", "vi-prev-word", "vi-put", "vi-redo", "vi-replace",
    "vi-rubout", "vi-search", "vi-search-again", "vi-set-mark", "vi-subst",
    "vi-tilde-expand", "vi-undo", "vi-unix-word-rubout", "vi-yank-arg",
    "vi-yank-pop", "vi-yank-to", "yank", "yank-last-arg", "yank-nth-arg",
    "yank-pop",
};

static iz readstdin(Shell *, File *, u8 *, iz);  // unabashed.cpp

// Key bindings: a trie of key sequences. A node with children is a
// prefix: the editor waits for the next key, at most keyseq-timeout if
// the node is bound too (ESC alone, or ESC [ A).
struct KeyNode {
    KeyNode *kids;
    KeyNode *sib;
    Str      macro; // EF_MACRO: the text; EF_SHELLCMD: the command
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

// Keymaps (inputrc.cpp)
enum : i32 { KM_EMACS, KM_META, KM_CTLX, KM_VIMOVE, KM_VIINS };

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
    u8       curkey;      // the key that ran the current command
    i64      deadline;    // read -t: os_now time when reading ends, or 0
    b32      timedout;    // and it has
    u8       pushed[32];  // keys read ahead and put back, the next last
    i32      unread;    // how many
    Str      macro;     // a macro's keys, read before the terminal's
    iz       macropos;
    b32      kbddef;    // defining a keyboard macro
    Buf      kbdrec;    // its keys so far
    Str      kbdmacro;  // the last one defined
    iz       seqstart;  // kbdrec's length where this command's keys began
    // The init file
    b32      inited;
    b32      termkeys;  // edtermkeys done
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
    Str      hlast;     // notty: what was drawn, and the cursor in it
    iz       hlastcur;
    b32      notty;     // input not a terminal, read with the editor
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
    iz       hpoint;    // history-preserve-point: where point goes, or -1
    b32      starred;   // mark-modified-lines: the star is shown
    Str      hsprefix;  // ^ and the line up to point (or without ^)
    iz       hsplen;    // the length of the line up to point
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
    // vi mode, as readline's vi_mode.c
    i32      ulevel;    // undo groups open, as readline's group level
    b32      vicmd;     // in command mode (the vi-command keymap)
    b32      vireplmode;  // R: replacing, in vireplmap
    KeyNode *vireplmap;   // made at the first R, as readline's
    b32      vicmdonce; // command mode was entered on this line
    b32      viredoing; // . is running the last command again
    b32      vidoing;   // c, s, S, R: the insert after them is saved
    b32      vifail;    // the last character search failed
    u8       vilastcmd; // the last text-changing command, for .
    i32      vilastcount;
    u8       vilastmotion;
    u8       vilastins; // the key that last entered insert mode
    i32      vireplcount;  // characters replaced since R
    i32      vicsdir;   // character search: FTO and the like
    i32      vicsorig;
    u8       vicschar[4];
    i32      vicslen;
    u8       virepl[4]; // r's last replacement
    i32      vireplen;
    Str      vibuf;     // the text last inserted, for .
    iz       vibufcap;
    iz       vimarks[26];
    Str      lineprompt;  // the line's own prompt (show-mode-in-prompt)
    b32      ttyvi;     // the tty's characters were bound in vi-insert
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
        if (c != 'O') keybind(a, r, {meta, 2}, EF_DO_LOWERCASE_VERSION);
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
        {"\x1b[3;5~", EF_KILL_WORD},
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
        {"\x18" "e", EF_CALL_LAST_KBD_MACRO},
        {"\x18(", EF_START_KBD_MACRO},   {"\x18)", EF_END_KBD_MACRO},
        {"\x18\x16", EF_DISPLAY_SHELL_VERSION},
        {"\x18" "s", EF_SPELL_CORRECT_WORD},
        {"\x1b" "x", EF_EXECUTE_NAMED_COMMAND},
        {"\x1b&", EF_TILDE_EXPAND},
    };
    for (iz i = 0; i < countof(map); i++) {
        keybind(a, r, map[i].seq, map[i].fn);  // literal lengths, NUL too
    }

    // vi, as readline has it with no terminal
    struct { Str seq; i32 fn; } viinsert[] = {
        {"\x04", EF_VI_EOF_MAYBE},
        {"\x08", EF_BACKWARD_DELETE_CHAR},
        {"\x09", EF_COMPLETE},
        {"\x0a", EF_ACCEPT_LINE},
        {"\x0d", EF_ACCEPT_LINE},
        {"\x0e", EF_MENU_COMPLETE},
        {"\x10", EF_MENU_COMPLETE_BACKWARD},
        {"\x12", EF_REVERSE_SEARCH_HISTORY},
        {"\x13", EF_FORWARD_SEARCH_HISTORY},
        {"\x14", EF_TRANSPOSE_CHARS},
        {"\x15", EF_UNIX_LINE_DISCARD},
        {"\x16", EF_QUOTED_INSERT},
        {"\x17", EF_VI_UNIX_WORD_RUBOUT},
        {"\x19", EF_YANK},
        {"\x1b", EF_VI_MOVEMENT_MODE},
        {"\x1bOA", EF_PREVIOUS_HISTORY},
        {"\x1bOB", EF_NEXT_HISTORY},
        {"\x1bOC", EF_FORWARD_CHAR},
        {"\x1bOD", EF_BACKWARD_CHAR},
        {"\x1bOF", EF_END_OF_LINE},
        {"\x1bOH", EF_BEGINNING_OF_LINE},
        {"\x1b[1;3C", EF_FORWARD_WORD},
        {"\x1b[1;3D", EF_BACKWARD_WORD},
        {"\x1b[1;5C", EF_FORWARD_WORD},
        {"\x1b[1;5D", EF_BACKWARD_WORD},
        {"\x1b[200~", EF_BRACKETED_PASTE_BEGIN},
        {"\x1b[3;5~", EF_KILL_WORD},
        {"\x1b[A", EF_PREVIOUS_HISTORY},
        {"\x1b[B", EF_NEXT_HISTORY},
        {"\x1b[C", EF_FORWARD_CHAR},
        {"\x1b[D", EF_BACKWARD_CHAR},
        {"\x1b[F", EF_END_OF_LINE},
        {"\x1b[H", EF_BEGINNING_OF_LINE},
        {"\x1f", EF_VI_UNDO},
        {"\x7f", EF_BACKWARD_DELETE_CHAR},
    };
    struct { Str seq; i32 fn; } vicommand[] = {
        {"\x04", EF_VI_EOF_MAYBE},
        {"\x07", EF_ABORT},
        {"\x08", EF_BACKWARD_CHAR},
        {"\x0a", EF_ACCEPT_LINE},
        {"\x0b", EF_KILL_LINE},
        {"\x0c", EF_CLEAR_SCREEN},
        {"\x0d", EF_ACCEPT_LINE},
        {"\x0e", EF_NEXT_HISTORY},
        {"\x10", EF_PREVIOUS_HISTORY},
        {"\x11", EF_QUOTED_INSERT},
        {"\x12", EF_REVERSE_SEARCH_HISTORY},
        {"\x13", EF_FORWARD_SEARCH_HISTORY},
        {"\x14", EF_TRANSPOSE_CHARS},
        {"\x15", EF_UNIX_LINE_DISCARD},
        {"\x16", EF_QUOTED_INSERT},
        {"\x17", EF_VI_UNIX_WORD_RUBOUT},
        {"\x19", EF_YANK},
        {"\x1bOA", EF_PREVIOUS_HISTORY},
        {"\x1bOB", EF_NEXT_HISTORY},
        {"\x1bOC", EF_FORWARD_CHAR},
        {"\x1bOD", EF_BACKWARD_CHAR},
        {"\x1bOF", EF_END_OF_LINE},
        {"\x1bOH", EF_BEGINNING_OF_LINE},
        {"\x1b[1;3C", EF_FORWARD_WORD},
        {"\x1b[1;3D", EF_BACKWARD_WORD},
        {"\x1b[1;5C", EF_FORWARD_WORD},
        {"\x1b[1;5D", EF_BACKWARD_WORD},
        {"\x1b[3;5~", EF_KILL_WORD},
        {"\x1b[A", EF_PREVIOUS_HISTORY},
        {"\x1b[B", EF_NEXT_HISTORY},
        {"\x1b[C", EF_FORWARD_CHAR},
        {"\x1b[D", EF_BACKWARD_CHAR},
        {"\x1b[F", EF_END_OF_LINE},
        {"\x1b[H", EF_BEGINNING_OF_LINE},
        {"\x1f", EF_VI_UNDO},
        {" ", EF_FORWARD_CHAR},
        {"#", EF_INSERT_COMMENT},
        {"$", EF_END_OF_LINE},
        {"%", EF_VI_MATCH},
        {"&", EF_VI_TILDE_EXPAND},
        {"*", EF_BASH_VI_COMPLETE},
        {"+", EF_NEXT_HISTORY},
        {",", EF_VI_CHAR_SEARCH},
        {"-", EF_PREVIOUS_HISTORY},
        {".", EF_VI_REDO},
        {"/", EF_VI_SEARCH},
        {"0", EF_BEGINNING_OF_LINE},
        {"1", EF_VI_ARG_DIGIT},
        {"2", EF_VI_ARG_DIGIT},
        {"3", EF_VI_ARG_DIGIT},
        {"4", EF_VI_ARG_DIGIT},
        {"5", EF_VI_ARG_DIGIT},
        {"6", EF_VI_ARG_DIGIT},
        {"7", EF_VI_ARG_DIGIT},
        {"8", EF_VI_ARG_DIGIT},
        {"9", EF_VI_ARG_DIGIT},
        {";", EF_VI_CHAR_SEARCH},
        {"=", EF_BASH_VI_COMPLETE},
        {"?", EF_VI_SEARCH},
        {"A", EF_VI_APPEND_EOL},
        {"B", EF_VI_PREV_WORD},
        {"C", EF_VI_CHANGE_TO},
        {"D", EF_VI_DELETE_TO},
        {"E", EF_VI_END_WORD},
        {"F", EF_VI_CHAR_SEARCH},
        {"G", EF_VI_FETCH_HISTORY},
        {"I", EF_VI_INSERT_BEG},
        {"N", EF_VI_SEARCH_AGAIN},
        {"P", EF_VI_PUT},
        {"R", EF_VI_REPLACE},
        {"S", EF_VI_SUBST},
        {"T", EF_VI_CHAR_SEARCH},
        {"U", EF_REVERT_LINE},
        {"W", EF_VI_NEXT_WORD},
        {"X", EF_VI_RUBOUT},
        {"Y", EF_VI_YANK_TO},
        {"\\", EF_BASH_VI_COMPLETE},
        {"^", EF_VI_FIRST_PRINT},
        {"_", EF_VI_YANK_ARG},
        {"`", EF_VI_GOTO_MARK},
        {"a", EF_VI_APPEND_MODE},
        {"b", EF_VI_PREV_WORD},
        {"c", EF_VI_CHANGE_TO},
        {"d", EF_VI_DELETE_TO},
        {"e", EF_VI_END_WORD},
        {"f", EF_VI_CHAR_SEARCH},
        {"h", EF_BACKWARD_CHAR},
        {"i", EF_VI_INSERTION_MODE},
        {"j", EF_NEXT_HISTORY},
        {"k", EF_PREVIOUS_HISTORY},
        {"l", EF_FORWARD_CHAR},
        {"m", EF_VI_SET_MARK},
        {"n", EF_VI_SEARCH_AGAIN},
        {"p", EF_VI_PUT},
        {"r", EF_VI_CHANGE_CHAR},
        {"s", EF_VI_SUBST},
        {"t", EF_VI_CHAR_SEARCH},
        {"u", EF_VI_UNDO},
        {"v", EF_VI_EDIT_AND_EXECUTE_COMMAND},
        {"w", EF_VI_NEXT_WORD},
        {"x", EF_VI_DELETE},
        {"y", EF_VI_YANK_TO},
        {"|", EF_VI_COLUMN},
        {"~", EF_VI_CHANGE_CASE},
        {"@", EF_POSIX_EDIT_MACROS},
    };

    for (i32 c = 0x20; c < 0x100; c++) {
        u8 k = (u8)c;
        if (c != 0x7f) keybind(a, &ed->viins, {&k, 1}, EF_SELF_INSERT);
    }
    u8 ctl[] = {1, 2, 3, 5, 6, 7, 11, 12, 15, 17, 24, 26, 28, 29, 30};
    for (iz i = 0; i < countof(ctl); i++) {
        keybind(a, &ed->viins, {ctl+i, 1}, EF_SELF_INSERT);
    }
    for (iz i = 0; i < countof(viinsert); i++) {
        keybind(a, &ed->viins, viinsert[i].seq, viinsert[i].fn);
    }
    for (iz i = 0; i < countof(vicommand); i++) {
        keybind(a, &ed->vimove, vicommand[i].seq, vicommand[i].fn);
    }
    // ESC in command mode waits for a sequence, then quietly does nothing
    keyget(a, &ed->vimove, 0x1b)->fn = EF_NULL_FUNCTION;
}

// Keys a terminal sends, which readline binds from its description
// where nothing else has
static void edtermkeys(Editor *ed)
{
    struct { Str seq; i32 fn; } map[] = {
        {"\x1b[3~", EF_DELETE_CHAR},       {"\x1b[2~", EF_OVERWRITE_MODE},
        {"\x1b[1~", EF_BEGINNING_OF_LINE}, {"\x1b[4~", EF_END_OF_LINE},
        {"\x1b[5~", EF_HISTORY_SEARCH_BACKWARD},
        {"\x1b[6~", EF_HISTORY_SEARCH_FORWARD},
    };
    KeyNode *maps[] = {&ed->root, &ed->viins, &ed->vimove};
    for (iz m = 0; m < countof(maps); m++) {
        for (iz i = 0; i < countof(map); i++) {
            if (m && i>=4) continue;  // the page keys: emacs only
            KeyNode *n = maps[m];
            for (iz k = 0; n && k < map[i].seq.len; k++) {
                n = keychild(n, map[i].seq[k]);
            }
            if (n && (n->fn || n->kids)) continue;  // bound already
            keybind(&ed->perm, maps[m], map[i].seq, map[i].fn);
        }
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
        ed->hpoint     = -1;
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
        ed->vars.vimode  = sh->opt[O_VI];
        ed->vars.keymap  = sh->opt[O_VI] ? KM_VIINS : KM_EMACS;
        ed->vilastcmd    = 'i';
        ed->vilastcount  = 1;
        ed->vilastins    = 'i';
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
    if (!col || col+1<ed->ccol-col) {
        // A carriage return is shorter, as readline reckons
        edput(ed, "\r");
        ed->ccol = 0;
    }
    if (col == ed->ccol) {
        // there
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
    i32   oldlm = lm;
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
    Str now = {vis.data, vis.len};
    if (ed->notty && lm==oldlm && ed->hlastcur==ed->hlast.len &&
            atcur==now.len && startswith(now, ed->hlast)) {
        // As readline, which draws only what changed: what was added at
        // the end (input that is no terminal shows these bytes)
        print(&ed->out_, cuthead(now, ed->hlast.len));
        ed->hlast    = now;
        ed->hlastcur = atcur;
        ed->hshown   = shown;
        ed->crow     = 0;
        ed->ccol     = cpos - lm;
        return;
    }
    ed->hlast    = now;
    ed->hlastcur = atcur;
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
    Str prompt = ed->prompt;
    if (ed->vars.showmode && prompt.data==ed->lineprompt.data) {
        // The mode before the line's own prompt (not a search's)
        Str mode = !ed->vars.vimode ? ed->vars.emacsstr
                 : ed->vicmd || ed->vireplmode ? ed->vars.vicmdstr
                 : ed->vars.viinsstr;
        prompt = concat(a, mode, prompt);
    }
    ed->starred = ed->vars.markmodified && ed->hpos<sh->hist.len && ed->undo;
    if (ed->starred) prompt = concat(a, "*", prompt);
    Layout nl = layout(a, prompt, line, point, ed->hscroll ? 1<<30 : ed->width,
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
        b32 vi = ed->vars.vimode;
        if (p==ed->line.len && !vi) {
            edbell(ed);
            return p;
        }
        iz q = p;
        for (i32 i = 0; i<count && q<ed->line.len; i++) q = ednext(ed, q);
        if (vi && ed->vicmd && q>=ed->line.len) q = edprev(ed, ed->line.len);
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
    if (ed->undoing) return;
    edrecord(ed, kind, 0, 0);
    ed->ulevel += kind==U_BEGIN ? 1 : -1;
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

    if (ed->lastkill && ed->nkills && !ed->vars.vimode) {
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
    if (ed->vars.vimode) {
        ed->point = 0;
        ed->mark  = ed->line.len;
    }
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

// Point for history-preserve-point: kept from before the first move
// (or from the last line accepted), else at the end, as readline's
static void edhistpoint(Editor *ed)
{
    if (ed->hpoint<0 && (ed->point || ed->line.len)) {
        ed->hpoint = ed->point==ed->line.len ? -1 : ed->point;
    }
}

static void edhistset(Editor *ed)
{
    iz p = ed->vars.histpoint && ed->hpoint>=0 ? ed->hpoint : ed->line.len;
    ed->point = p<ed->line.len ? p : ed->line.len;
    if (ed->vars.vimode && (ed->vicmd || ed->vireplmode)) ed->point = 0;
}

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
    edhistpoint(ed);
    edload(ed, sh, to);
    edhistset(ed);
}

static void ednexthist(Editor *ed, Shell *sh, i64 count)
{
    if (count < 0) return edprevhist(ed, sh, -count);
    if (!count) return;
    Hist *h = &sh->hist;
    edkeepline(ed, sh);
    edhistpoint(ed);
    if (count >= h->len-ed->hpos) {
        ed->hpos = h->len;
        edunsaveline(ed);
    } else {
        edload(ed, sh, ed->hpos+(iz)count);
        edhistset(ed);
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
static void vistartins(Editor *, u8, i32);
static iz   viadvance(Editor *, iz);

static b32 edyankarg(Editor *ed, Shell *sh, i64 n, i64 skip, u8 key)
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
    if (ed->vars.vimode && ed->vicmd && !ed->vireplmode) {
        ed->point = viadvance(ed, ed->point);
        vistartins(ed, key, ed->count<0 ? -1 : 1);
        edinsert(ed, " ");
    }
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
static iz edfindhist(Shell *sh, Str s, iz pos, i32 dir, iz *at, b32 fold,
                     b32 pat = 0)
{
    Hist *h  = &sh->hist;
    b32   anchor = s.len && s[0]=='^';
    if (anchor) s = cuthead(s, 1);
    if (pat) {
        // vi's searches, as readline's: a glob matching the whole line,
        // with a star added at the end, and at the start unless anchored
        if (!s.len) return -1;
        b32 esc = s[s.len-1] == '\\';
        for (iz i = s.len-1; esc && i>0 && s[i-1]=='\\'; i--) esc = !esc;
        if (esc) return -1;  // fnmatch rejects a trailing backslash
        Arena a = *scratchat(sh, sh->capdepth+1);
        Str   p = !anchor && s[0]!='*' ? concat(&a, "*", s) : clone(&a, s);
        if (p[p.len-1] != '*') p = concat(&a, p, "*");
        i32 flags = sh->utf8 ? PM_UTF8 : 0;
        for (; pos>=0 && pos<h->len; pos += dir) {
            if (patmatch(p, h->ents[pos]->line, flags, a)) {
                *at = 0;
                return pos;
            }
        }
        return -1;
    }
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
// M-x's TAB: the longest prefix the function names share with the
// line, the bell when none starts with it.
static void edcompletename(Editor *ed)
{
    Str typed = edtext(ed);
    Str best  = {};
    for (iz i = 0; i < countof(rlfunmap); i++) {
        Str n = rlfunmap[i];
        if (!startswith(n, typed)) continue;
        if (!best.data) {
            best = n;
            continue;
        }
        iz k = 0;
        for (; k<best.len && k<n.len && best[k]==n[k]; k++) {}
        best.len = k;
    }
    if (!best.data) {
        edbell(ed);
    } else if (best.len > typed.len) {
        ed->point = ed->line.len;
        edinsert(ed, cuthead(best, typed.len));
    }
}

// Read a string in place of the line, after the prompt and pchar, as
// readline's readstr: false if given up (and on an interrupt, which
// ends the line). names: TAB completes a function name, as for M-x.
static b32 edreadstr(Editor *ed, Shell *sh, Str pchar, Str *out, b32 names = 0)
{
    iz     savepoint = ed->point;
    iz     savemark  = ed->mark;
    Str    saved     = clone(&ed->linemem, edtext(ed));
    EdUndo *saveundo = ed->undo;
    Str    prompt    = ed->prompt;
    ed->line.len = ed->point = 0;
    ed->undo     = 0;
    ed->prompt   = concat(&ed->linemem, prompt, pchar);

    b32 ok = 0;
    for (;;) {
        redisplay(ed, sh);
        i32 c = edkey(ed, sh, -1);
        if (c==-2 || c==ed->tty.intr) {
            ed->prompt = prompt;
            ed->done   = 1;
            ed->result = ED_INTR;
            return 0;
        }
        if (c < 0) break;
        if (names && c=='\t') {
            edcompletename(ed);
        } else if (names && c==' ') {
            edbell(ed);
        } else if (c==0x17 || c==0x15) {
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
    *out = clone(&ed->linemem, edtext(ed));
    ed->prompt   = prompt;
    ed->line.len = 0;
    print(&ed->line, saved);
    ed->undo  = saveundo;
    ed->point = savepoint;
    ed->mark  = savemark;
    return ok;
}

// The non-incremental searches; again: with the last string, from
// where it was last found. pat: vi's glob searches.
static void ednoninc(Editor *ed, Shell *sh, i32 dir, b32 again = 0,
                     Str pchar = ":", b32 pat = 0)
{
    iz  savepos = ed->hpos;
    Str typed   = {};
    if (!again && !edreadstr(ed, sh, pchar, &typed)) return;

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
    iz got = edfindhist(sh, ed->noninc, from+dir, dir, &at, ed->vars.searchcase, pat);
    if (got < 0) {
        edbell(ed);
        if (again) ed->point = 0;  // as readline's, left for its caller
        return;
    }
    ed->nonincpos = got;
    edgohist(ed, sh, got);
    if (ed->vars.vimode) ed->undo = 0;  // U goes back no further
    if (!pat && !ed->hscroll && at<ed->line.len) {
        ed->point = at;
        ed->mark  = at + ed->noninc.len;
        edactivate(ed);
    } else {
        ed->point = 0;
        ed->mark  = ed->line.len;
    }
}

// history-search-backward and -forward: entries starting with the line
// up to point; history-substring-search-*: entries holding it.
static void edhistsearch(Editor *ed, Shell *sh, i32 fn, i32 count)
{
    if (!count) return;
    b32 sub  = fn==EF_HISTORY_SUBSTRING_SEARCH_BACKWARD ||
               fn==EF_HISTORY_SUBSTRING_SEARCH_FORWARD;
    b32 back = fn==EF_HISTORY_SEARCH_BACKWARD ||
               fn==EF_HISTORY_SUBSTRING_SEARCH_BACKWARD;
    i32 f0   = sub ? EF_HISTORY_SUBSTRING_SEARCH_BACKWARD : EF_HISTORY_SEARCH_BACKWARD;
    i32 f1   = sub ? EF_HISTORY_SUBSTRING_SEARCH_FORWARD : EF_HISTORY_SEARCH_FORWARD;
    if (ed->lastfn!=f0 && ed->lastfn!=f1) {
        Str typed    = {ed->line.data, ed->point};
        ed->hspos    = ed->hpos;
        ed->hsprefix = sub ? clone(&ed->linemem, typed)
                           : concat(&ed->linemem, "^", typed);
        ed->hsplen   = typed.len;
        ed->hsprev   = {};
    }
    if (!ed->hsplen) {
        if (back) {
            edprevhist(ed, sh, count);
        } else {
            ednexthist(ed, sh, count);
        }
        return;
    }
    i32 dir = back == (count>0) ? -1 : 1;
    i32 n   = count<0 ? -count : count;
    b32 any = 0;
    iz  col = -1;
    while (n) {
        iz at  = 0;
        iz got = edfindhist(sh, ed->hsprefix, ed->hspos+dir, dir, &at, ed->vars.searchcase);
        if (got < 0) break;
        ed->hspos = got;
        col = at;
        any = 1;
        Str line = sh->hist.ents[got]->line;
        if (ed->hsprev.data && ed->hsprev==line) continue;
        ed->hsprev = clone(&ed->linemem, line);
        n--;
    }
    iz plen = ed->hsplen;
    if (!any) {
        edbell(ed);
        ed->point = plen<ed->line.len ? plen : ed->line.len;
        ed->mark  = ed->line.len;
        return;
    }
    edgohist(ed, sh, ed->hspos);
    if (sub) plen = col>=0 ? col : ed->line.len;
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

// alias-expand-line and history-and-alias-expand-line, as bash's: the
// line replaced whether or not it changed, and point placed as after
// any expansion of the line
static void edexpandaliases(Editor *ed, Shell *sh, b32 hist)
{
    Arena a = *scratchat(sh, sh->capdepth+1);
    Str   r = edtext(ed);
    if (hist && !edhistexpand(ed, sh, &a, &r)) return;
    r = edaliases(sh, &a, r);
    iz oldpoint = ed->point;
    iz oldend   = ed->line.len;
    edreplace(ed, r);
    edexpandpoint(ed, oldpoint, oldend);
}

// Readline's tilde-expand: the word around point, if it starts with a
// tilde, replaced with its expansion, point after it.
static void edtildeexpand(Editor *ed, Shell *sh)
{
    Str s     = edtext(ed);
    iz  start = ed->point - 1;
    if (start>=0 && s[start]!='~') {
        for (; start>=0 && s[start]!=' ' && s[start]!='\t'; start--) {}
        start++;
    } else if (start < 0) {
        start = 0;
    }
    iz end = start;
    do end++; while (end<s.len && s[end]!=' ' && s[end]!='\t');
    if (end>=s.len || s[end]==' ' || s[end]=='\t') end--;
    if (start>=s.len || s[start]!='~') return;
    Arena a    = *scratchat(sh, sh->capdepth+1);
    Str   word = clone(&a, span(s.data+start, s.data+end+1));
    Str   home = tildefilename(sh, &a, word);
    edrecord(ed, U_BEGIN, 0, 0);
    eddelete(ed, start, end+1);
    ed->point = start;
    edinsert(ed, home);
    edrecord(ed, U_END, 0, 0);
}

// Bash's spell-correct-word: directory spelling correction, as cd's,
// of the shell word at or after point and the count-1 after it, or of
// the -count words before it.
static void edspellword(Editor *ed, Shell *sh, i32 count)
{
    b32 bell = 0;
    Str s    = edtext(ed);
    if (count < 0) {
        i32 n = 0;
        while (ed->point>0 && count++) {
            iz p = ed->point;
            ed->point = edshellback(ed, ed->point, 1);
            if (!ed->point && edwdelim(s[0])) {
                ed->point = p;
                break;
            }
            n++;
        }
        count = n;
    } else {
        while (ed->point<s.len && edwdelim(s[ed->point])) ed->point++;
    }
    s = edtext(ed);
    if (ed->point<s.len && !edwdelim(s[ed->point])) {
        ed->point = edshellfwd(ed, ed->point, 1, &bell);
    }
    while (count) {
        iz wbeg = edshellback(ed, ed->point, 1);
        iz wend = edshellfwd(ed, wbeg, 1, &bell);
        ed->point = wend;
        if (wbeg >= wend) break;
        Arena a     = *scratchat(sh, sh->capdepth+1);
        Str   text  = clone(&a, span(ed->line.data+wbeg, ed->line.data+wend));
        Str   fixed = spellpath(sh, &a, text);
        if (fixed.data) {
            edrecord(ed, U_BEGIN, 0, 0);
            eddelete(ed, wbeg, wend);
            ed->point = wbeg;
            edinsert(ed, fixed);
            ed->mark = wbeg;
            edrecord(ed, U_END, 0, 0);
        }
        if (ed->point >= ed->line.len) break;
        count--;
        if (count) ed->point = edshellfwd(ed, ed->point, 1, &bell);
    }
}

// edit-and-execute-command (C-x C-e), as bash's: the line goes into
// history with an empty entry after it standing for fc's own, which fc
// drops; with the terminal restored, fc edits the line in $VISUAL or
// $EDITOR and runs what comes back; then a new empty line follows
// under the same prompt.
static void vidoneins(Editor *);
static void viacceptline(Editor *);
static void viinsmode(Editor *, u8);
static void edoptsync(Editor *, Shell *);
static b32  compglobchars(Str);  // complete.cpp

static void ededitexec(Editor *ed, Shell *sh, b32 vi = 0, u8 key = 0)
{
    if (ed->vars.vimode) viacceptline(ed);
    i32 count  = ed->count;
    b32 argset = ed->argset;
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
    if (!argset) {
        reallyadd(sh, clone(&ed->linemem, edtext(ed)));
        reallyadd(sh, "");
    }
    os_tty(sh->os, ed->in, ed->out, TTY_RESTORE);

    i32   depth  = ++sh->capdepth;
    Arena a      = *scratchat(sh, depth);
    i32   line   = sh->line;
    b32   lexeof = sh->lexeof;
    sh->evaldepth++;
    Str cmd = !vi ? Str("fc -e \"${VISUAL:-${EDITOR:-emacs}}\"")
            : sh->opt[O_POSIX] ? Str("fc -e vi")
            : Str("fc -e \"${VISUAL:-${EDITOR:-vi}}\"");
    if (argset) {
        // An argument names the history entry instead
        Buf b(&a, cmd.len+24);
        print(&b, cmd);
        print(&b, " ");
        print(&b, (i64)count);
        cmd = finish(&b);
    }
    runsource(sh, cmd, vi ? Str("v") : Str("C-xC-e"), a);
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
    if (vi) viinsmode(ed, key);
    edredraw(ed, sh);
}


// A byte offset into the line as characters, and back, for bind -x's
// READLINE_POINT and READLINE_MARK
static i64 edcharoff(Editor *ed, Shell *sh, iz pos)
{
    if (!sh->utf8) return pos;
    i64 n = 0;
    for (iz i = 0; i < pos; i++) n += (ed->line.data[i]&0xc0) != 0x80;
    return n;
}

static iz edbyteoff(Editor *ed, Shell *sh, i64 n)
{
    Str s = edtext(ed);
    if (n <= 0) return 0;
    if (!sh->utf8) return n<s.len ? (iz)n : s.len;
    iz i = 0;
    for (; n>0 && i<s.len; n--) utf8decode(s, &i);
    return i;
}

// bind -x: run cmd with the line in READLINE_LINE, READLINE_POINT and
// READLINE_MARK, taking back what it leaves in them.
static void edshellcmd(Editor *ed, Shell *sh, Str cmd)
{
    // Clear the line's rows, so the command's output takes their place
    if (ed->hscroll) {
        edput(ed, "\r\n");
    } else {
        edput(ed, "\r");
        ed->ccol    = 0;
        ed->pending = 0;
        for (i32 row = ed->shown.nrows-1; row >= 0; row--) {
            edmove(ed, row, 0);
            edput(ed, "\x1b[K");
        }
    }
    edflush(ed, sh);

    i32   depth = ++sh->capdepth;
    Arena a     = *scratchat(sh, depth);
    Str   names[] = {"READLINE_LINE", "READLINE_POINT", "READLINE_MARK",
                     "READLINE_ARGUMENT"};
    setvar(sh, a, names[0], edtext(ed));
    setvar(sh, a, names[1], fmti64(&a, edcharoff(ed, sh, ed->point)));
    setvar(sh, a, names[2], fmti64(&a, edcharoff(ed, sh, ed->mark)));
    i32 nvars = 3;
    if (ed->argset || ed->count!=1) {
        setvar(sh, a, names[3], fmti64(&a, ed->count));
        nvars = 4;
    }
    for (i32 i = 0; i < nvars; i++) setattrs(sh, names[i], V_EXPORT, 0);

    os_tty(sh->os, ed->in, ed->out, TTY_RESTORE);
    i32 line   = sh->line;
    b32 lexeof = sh->lexeof;
    sh->evaldepth++;
    runsource(sh, cmd, "bash_execute_unix_command", a);
    sh->evaldepth--;
    sh->line   = line;
    sh->lexeof = lexeof;
    os_tty(sh->os, ed->in, ed->out, TTY_RAW);

    Got g = getvar(sh, &a, names[0]);
    if (g.set) edreplace(ed, g.value);
    i64 n = 0;
    g = getvar(sh, &a, names[1]);
    if (g.set && parsei64(g.value, &n)) ed->point = edbyteoff(ed, sh, n);
    g = getvar(sh, &a, names[2]);
    if (g.set && parsei64(g.value, &n)) ed->mark = edbyteoff(ed, sh, n);
    for (i32 i = 0; i < 4; i++) unsetvar(sh, names[i], 1);
    sh->capdepth = depth - 1;

    edredraw(ed, sh);
}


static void edarg(Editor *, Shell *, i32 key);
static void edresetarg(Editor *);

static i32      rlnamedfn(Str);                                // inputrc.cpp
static Str      bindfunctions(KeyNode *, b32, Arena *);        // inputrc.cpp
static Str      bindmacros(KeyNode *, i32, b32, Arena *);      // inputrc.cpp
static Str      bindvariables(Editor *, b32, Arena *);         // inputrc.cpp
static KeyNode *rlkeymap(Editor *, i32);                       // inputrc.cpp
static void     rlputkeys(Buf *, Str);                         // inputrc.cpp

// Text shown under the line, the line then drawn again below it, as
// readline's dump functions and display-shell-version
static void edshowtext(Editor *ed, Shell *sh, Str text)
{
    Layout *l = &ed->shown;
    if (!ed->hscroll) edmove(ed, l->nrows-1, l->rows[l->nrows-1].width);
    edput(ed, "\r\n");
    Buf b(&ed->linemem, text.len+16);
    for (iz i = 0; i < text.len; i++) {
        if (text[i] == '\n') putbyte(&b, '\r');
        putbyte(&b, text[i]);
    }
    edput(ed, finish(&b));
    edredraw(ed, sh);
}

// Keys to read next, ahead of the rest of any running macro
static void edplay(Editor *ed, Str keys)
{
    Str rest = cuthead(ed->macro, ed->macropos);
    ed->macro    = rest.len ? concat(&ed->perm, keys, rest) : clone(&ed->perm, keys);
    ed->macropos = 0;
}

// As readline's abort: the bell, and any keyboard macro, running or
// being defined, is gone
static void edabort(Editor *ed)
{
    edbell(ed);
    ed->macropos = ed->macro.len;
    ed->kbddef   = 0;
    ed->kbdmacro = {};
    ed->active   = 0;
    edresetarg(ed);
}

// Readline's _rl_overwrite_char: ch in place of the character at
// point, count times, undone as one.
static void edoverchar(Editor *ed, Str ch, i32 count)
{
    edgroup(ed, U_BEGIN);
    for (i32 i = 0; i < count; i++) {
        edinsert(ed, ch);
        if (ed->point < ed->line.len) {
            eddelete(ed, ed->point, ednext(ed, ed->point));
        }
    }
    edgroup(ed, U_END);
}

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
        edoverchar(ed, ch, count);
    } else if (count == 1) {
        edinsert(ed, ch);
    } else {
        Buf b(&ed->hmem, len*count);
        for (i32 i = 0; i < count; i++) print(&b, ch);
        edinsert(ed, finish(&b));
    }
}

// vi mode, as readline's vi_mode.c

enum : i32 { VIM_DELETE, VIM_CHANGE, VIM_YANK };
enum : i32 { CS_BFIND = -2, CS_BTO = -1, CS_FTO = 1, CS_FFIND = 2 };

static b32 vitextmod(u8 c)  // a command . can repeat
{
    return c && edin("_*\\AaIiCcDdPpYyRrSsXx~", c);
}

static b32 vimotionkey(u8 c)  // a motion for d, c and y
{
    return c && edin(" hl^$0ftFT;,%wbeWBE|`", c);
}

static b32 viblank(u8 c) { return c==' ' || c=='\t'; }
static b32 viupper(u8 c) { return c>='A' && c<='Z'; }

static b32 viident(u8 c)  // readline's _rl_isident, by bytes
{
    return (c>='0' && c<='9') || ((c|0x20)>='a' && (c|0x20)<='z') || c=='_';
}

static b32 vipunct(u8 c)  // ispunct, ASCII only as in a UTF-8 locale
{
    return (c>0x20 && c<0x30) || (c>0x39 && c<0x41) ||
           (c>0x5a && c<0x61) || (c>0x7a && c<0x7f);
}

// The byte at i, NUL past the end, as readline reads its buffer
static u8 vich(Editor *ed, iz i)
{
    return i>=0 && i<ed->line.len ? ed->line.data[i] : 0;
}

// The keymap keys are looked up in now
static KeyNode *edmap(Editor *ed)
{
    if (!ed->vars.vimode) return &ed->root;
    if (ed->vireplmode)   return ed->vireplmap;
    return ed->vicmd ? &ed->vimove : &ed->viins;
}

// set -o vi and set -o emacs follow the editing mode, as bash's report
// readline's, while line editing is on
static void edoptsync(Editor *ed, Shell *sh)
{
    if (!sh->opt[O_EMACS] && !sh->opt[O_VI]) return;
    sh->opt[O_VI]    = !!ed->vars.vimode;
    sh->opt[O_EMACS] = !ed->vars.vimode;
}

// After set -o vi or set -o emacs
static void edmodechanged(Shell *sh)
{
    Editor *ed = sh->editor;
    if (!ed) return;
    if (sh->opt[O_VI]) {
        ed->vars.vimode = 1;
        ed->vars.keymap = KM_VIINS;
        ed->vicmd = ed->vireplmode = 0;
    } else if (sh->opt[O_EMACS]) {
        ed->vars.vimode = 0;
        ed->vars.keymap = KM_EMACS;
        ed->vicmd = ed->vireplmode = 0;
    }
}

static iz viadvance(Editor *ed, iz p)  // _rl_vi_advance_point
{
    return p<ed->line.len ? ednext(ed, p) : p;
}

static iz viback(Editor *ed, iz p)  // _rl_vi_backup_point
{
    return p>0 ? edprev(ed, p) : p;
}

// UTF-8 steps that count zero-width characters, as MB_FIND_ANY
static iz vinextany(Editor *ed, iz i)
{
    if (i >= ed->line.len) return ed->line.len;
    for (i++; i<ed->line.len && (ed->line.data[i]&0xc0)==0x80; i++) {}
    return i;
}

static iz viprevany(Editor *ed, iz i)
{
    if (i <= 0) return 0;
    for (i--; i>0 && (ed->line.data[i]&0xc0)==0x80; i--) {}
    return i;
}

static void viinsmode(Editor *ed, u8 key)  // rl_vi_insertion_mode
{
    ed->vicmd = ed->vireplmode = 0;
    ed->vars.keymap = KM_VIINS;
    ed->vilastins   = key;
}

static void visetlast(Editor *ed, u8 key, i32 count)
{
    ed->vilastcmd   = key;
    ed->vilastcount = count;
}

static void viresetlast(Editor *ed)
{
    ed->vilastcmd    = 'i';
    ed->vilastcount  = 1;
    ed->vilastmotion = 0;
}

static void vistartins(Editor *ed, u8 key, i32 count)
{
    visetlast(ed, key, count);
    edgroup(ed, U_BEGIN);  // so inserts don't join what came before
    viinsmode(ed, key);
}

static void visetbuf(Editor *ed, Str t)
{
    if (t.len > ed->vibufcap) {
        iz cap = ed->vibufcap*2>t.len ? ed->vibufcap*2 : t.len+64;
        ed->vibuf.data = alloc<u8>(&ed->perm, cap);
        ed->vibufcap   = cap;
    }
    if (t.len) copybytes(ed->vibuf.data, t.data, t.len);
    ed->vibuf.len = t.len;
}

// The text of an insert record, as readline saves it for .
static void visaveins(Editor *ed, EdUndo *u)
{
    if (!u || u->kind!=U_INSERT) {
        ed->vibuf.len = 0;
        return;
    }
    iz beg = u->beg<ed->line.len ? u->beg : ed->line.len;
    iz end = u->end<ed->line.len ? u->end : ed->line.len;
    visetbuf(ed, {ed->line.data+beg, end>beg ? end-beg : 0});
}

// R's replacement, by the count of characters replaced, as bytes
static void visavereplace(Editor *ed)
{
    EdUndo *u = ed->undo;
    if (!u || u->kind!=U_END || ed->vireplcount<=0) {
        ed->vibuf.len = 0;
        return;
    }
    iz end   = ed->point;
    iz start = end - ed->vireplcount + 1;
    iz len   = ed->vireplcount + 1;
    if (start < 0) {
        len   = end + 1;
        start = 0;
    }
    iz n = len - 1;
    if (start+n > ed->line.len) n = ed->line.len - start;
    visetbuf(ed, {ed->line.data+start, n>0 ? n : 0});
}

// Leaving insert mode: the groups closed, the text kept for .
static void vidoneins(Editor *ed)
{
    if (ed->vidoing) {
        edgroup(ed, U_END);
        ed->vidoing = 0;
        if (ed->vilastins == 'R') {
            visavereplace(ed);
        } else {
            visaveins(ed, ed->undo ? ed->undo->next : 0);
        }
        if (ed->ulevel > 0) edgroup(ed, U_END);
    } else if (ed->undo && edin("iaIA", ed->vilastins)) {
        visaveins(ed, ed->undo);
    } else if (ed->vilastins == 'C') {
        edgroup(ed, U_END);
    }
    while (ed->ulevel > 0) edgroup(ed, U_END);
}

// The line accepted, as readline's newline in vi mode
static void viacceptline(Editor *ed)
{
    vidoneins(ed);
    if (!vitextmod(ed->vilastcmd)) viresetlast(ed);
}

static void vimovement(Editor *ed)  // rl_vi_movement_mode
{
    if (ed->point > 0) ed->point = edcharmove(ed, ed->point, -1);
    ed->vicmd       = 1;
    ed->vireplmode  = 0;
    ed->vars.keymap = KM_VIMOVE;
    vidoneins(ed);
    // As POSIX has U: nothing before the first command mode is undone
    if (!ed->vicmdonce) ed->undo = 0;
    ed->vicmdonce = 1;
}

// A key, or -1 at the end of input, or on an interrupt, which ends
// the line
static i32 vireadkey(Editor *ed, Shell *sh)
{
    i32 c = edkey(ed, sh, -1);
    if (c==-2 || c==ed->tty.intr) {
        ed->done   = 1;
        ed->result = ED_INTR;
        return -1;
    }
    return c<0 ? -1 : c;
}

// A character's bytes, as readline's _rl_read_mbchar: its length or -1
static i32 vireadchar(Editor *ed, Shell *sh, u8 *buf)
{
    i32 c = vireadkey(ed, sh);
    if (c < 0) return -1;
    buf[0] = (u8)c;
    i32 n   = c>=0xf0 ? 4 : c>=0xe0 ? 3 : c>=0xc0 ? 2 : 1;
    i32 len = 1;
    while (len < n) {
        i32 k = edkey(ed, sh, -1);
        if (k < 0) break;
        if ((k&0xc0) != 0x80) {
            edpush(ed, k);
            break;
        }
        buf[len++] = (u8)k;
    }
    return len;
}

// Word motions, as readline's vi_mode.c has them
static void vifWord(Editor *ed, i32 count)
{
    iz p = ed->point, end = ed->line.len;
    while (count-- > 0 && p<end-1) {
        while (!viblank(vich(ed, p)) && p<end) p = viadvance(ed, p);
        while (viblank(vich(ed, p)) && p<end) p = viadvance(ed, p);
    }
    ed->point = p;
}

static void vibWord(Editor *ed, i32 count)
{
    iz p = ed->point;
    while (count-- > 0 && p>0) {
        if (!viblank(vich(ed, p)) && viblank(vich(ed, p-1))) p--;
        while (p>0 && viblank(vich(ed, p))) p = viback(ed, p);
        if (p > 0) {
            do p = viback(ed, p); while (p>0 && !viblank(vich(ed, p)));
            if (p > 0) p++;  // onto the blank's next
        }
    }
    ed->point = p;
}

static void vieWord(Editor *ed, i32 count)
{
    iz p = ed->point, end = ed->line.len;
    while (count-- > 0 && p<end-1) {
        if (!viblank(vich(ed, p))) p = viadvance(ed, p);
        while (p<end && viblank(vich(ed, p))) p = viadvance(ed, p);
        if (p && p<end) {
            iz o = p;
            while (p<end && viblank(vich(ed, p))) {
                o = p;
                p = viadvance(ed, p);
            }
            while (p<end && !viblank(vich(ed, p))) {
                o = p;
                p = viadvance(ed, p);
            }
            p = o;
        }
    }
    ed->point = p;
}

static void vifword(Editor *ed, i32 count)
{
    iz p = ed->point, end = ed->line.len;
    while (count-- > 0 && p<end-1) {
        if (viident(vich(ed, p))) {
            while (viident(vich(ed, p)) && p<end) p = viadvance(ed, p);
        } else {
            while (!viident(vich(ed, p)) && !viblank(vich(ed, p)) && p<end) {
                p = viadvance(ed, p);
            }
        }
        while (viblank(vich(ed, p)) && p<end) p = viadvance(ed, p);
    }
    ed->point = p;
}

static void vibword(Editor *ed, i32 count)
{
    iz p = ed->point;
    while (count-- > 0 && p>0) {
        if (!viblank(vich(ed, p)) && viblank(vich(ed, p-1))) {
            if (--p == 0) break;
        }
        // Between a word and punctuation, back already
        b32 cur = viident(vich(ed, p));
        iz  o   = p;
        p = viback(ed, p);
        b32 prev = viident(vich(ed, p));
        if (cur == prev) p = o;
        while (p>0 && viblank(vich(ed, p))) p = viback(ed, p);
        if (p > 0) {
            o = p;
            if (viident(vich(ed, p))) {
                do {
                    o = p;
                    p = viback(ed, p);
                } while (p>0 && viident(vich(ed, p)));
            } else {
                do {
                    o = p;
                    p = viback(ed, p);
                } while (p>0 && !viident(vich(ed, p)) && !viblank(vich(ed, p)));
            }
            if (p > 0) p = o;
        }
    }
    ed->point = p;
}

static void vieword(Editor *ed, i32 count)
{
    iz p = ed->point, end = ed->line.len;
    while (count-- > 0 && p<end-1) {
        if (!viblank(vich(ed, p))) p = viadvance(ed, p);
        while (p<end && viblank(vich(ed, p))) p = viadvance(ed, p);
        iz o = p;
        if (p < end) {
            if (viident(vich(ed, p))) {
                do {
                    o = p;
                    p = viadvance(ed, p);
                } while (p<end && viident(vich(ed, p)));
            } else {
                do {
                    o = p;
                    p = viadvance(ed, p);
                } while (p<end && !viident(vich(ed, p)) && !viblank(vich(ed, p)));
            }
        }
        p = o;
    }
    ed->point = p;
}

static i32 vibrack(u8 c)
{
    switch (c) {
    case '(': return  1;
    case ')': return -1;
    case '[': return  2;
    case ']': return -2;
    case '{': return  3;
    case '}': return -3;
    }
    return 0;
}

// %: the bracket matching the one at point, or the first after it
static void vimatch(Editor *ed)
{
    iz  pos   = ed->point;
    i32 brack = vibrack(vich(ed, pos));
    if (!brack) {
        for (;;) {
            brack = vibrack(vich(ed, ed->point));
            if (brack) break;
            iz pre = ed->point;
            ed->point = edcharmove(ed, ed->point, 1);
            if (pre == ed->point) break;
        }
        if (brack <= 0) {  // a closing one found ahead fails too
            ed->point = pos;
            edbell(ed);
            return;
        }
    }
    pos = ed->point;
    for (i32 depth = 1; depth;) {
        pos += brack<0 ? -1 : 1;
        if (pos<0 || pos>=ed->line.len) {
            edbell(ed);
            return;
        }
        i32 b = vibrack(ed->line.data[pos]);
        if (b == -brack) {
            depth--;
        } else if (b == brack) {
            depth++;
        }
    }
    ed->point = pos;
}

// Readline's _rl_char_search_internal: true when it failed
static b32 vicsearch(Editor *ed, i32 count, i32 dir, Str ch)
{
    if (!dir) return 1;
    iz pos = ed->point;
    while (count) {
        if ((dir<0 && pos<=0) || (dir>0 && pos>=ed->line.len)) {
            edbell(ed);
            return 1;
        }
        pos = dir>0 ? vinextany(ed, pos) : viprevany(ed, pos);
        for (;;) {
            if (pos+ch.len<=ed->line.len && Str{ed->line.data+pos, ch.len}==ch) {
                count--;
                if (dir < 0) {
                    ed->point = dir==CS_BTO ? vinextany(ed, pos) : pos;
                } else {
                    ed->point = dir==CS_FTO ? viprevany(ed, pos) : pos;
                }
                break;
            }
            iz pre = pos;
            pos = dir<0 ? viprevany(ed, pos) : vinextany(ed, pos);
            if (pos == pre) break;
        }
    }
    return 0;
}

// f F t T ; , : true when it failed
static b32 vicharsearch(Editor *ed, Shell *sh, i32 count, u8 key)
{
    if (key==';' || key==',') {
        if (!ed->vicsorig || !ed->vicslen) return 1;
        ed->vicsdir = key==';' ? ed->vicsorig : -ed->vicsorig;
    } else {
        switch (key) {
        case 't': ed->vicsorig = ed->vicsdir = CS_FTO;   break;
        case 'T': ed->vicsorig = ed->vicsdir = CS_BTO;   break;
        case 'f': ed->vicsorig = ed->vicsdir = CS_FFIND; break;
        case 'F': ed->vicsorig = ed->vicsdir = CS_BFIND; break;
        }
        if (!ed->viredoing) {
            i32 n = vireadchar(ed, sh, ed->vicschar);
            if (n <= 0) return 1;
            ed->vicslen = n;
        }
    }
    return vicsearch(ed, count, ed->vicsdir, {ed->vicschar, ed->vicslen});
}

// x: count characters killed from point
static void videlete(Editor *ed, i32 count);

static void virubout(Editor *ed, i32 count)  // X
{
    if (count < 0) {
        videlete(ed, -count);
        return;
    }
    if (!ed->point) {
        edbell(ed);
        return;
    }
    iz o = ed->point;
    if (count > 1) {
        ed->point = edcharmove(ed, ed->point, -count);
    } else {
        ed->point = edprev(ed, ed->point);
    }
    edkill(ed, ed->point, o);
}

static void videlete(Editor *ed, i32 count)
{
    if (count < 0) {
        virubout(ed, -count);
        return;
    }
    if (!ed->line.len) {
        edbell(ed);
        return;
    }
    iz end = ed->point;
    for (i32 i = 0; i < count && end<ed->line.len; i++) end = ednext(ed, end);
    edkill(ed, ed->point, end);
    if (ed->point>0 && ed->point==ed->line.len) {
        ed->point = edcharmove(ed, ed->point, -1);
    }
}

// The cursor stays on the line's last character in command mode
static void vicheck(Editor *ed)
{
    if (ed->point && ed->point==ed->line.len) ed->point = edprev(ed, ed->point);
}

static void viput(Editor *ed, u8 key, i32 count)  // p and P
{
    if (!viupper(key) && ed->point+1<=ed->line.len) {
        // yank-pop of a put at column 0 leaves the point at -1
        ed->point = ed->point<0 ? 0 : ednext(ed, ed->point);
    }
    while (count-- > 0) edyank(ed);
    ed->point = edcharmove(ed, ed->point, -1);
}

static void viunixword(Editor *ed, i32 count)  // vi's C-w
{
    if (!ed->point) {
        edbell(ed);
        return;
    }
    iz o = ed->point, q = o;
    if (count <= 0) count = 1;
    #define vibound(c) (viblank(c) || vipunct(c))
    while (count--) {
        if (q>0 && !vich(ed, q) && viblank(vich(ed, q-1))) {
            while (--q>0 && viblank(vich(ed, q))) {}
        }
        if (q>0 && !vibound(vich(ed, q)) && vibound(vich(ed, q-1))) q--;
        if (q>0 && vibound(vich(ed, q))) {
            while (q && vibound(vich(ed, q-1))) q--;
        } else if (q>0 && !vibound(vich(ed, q))) {
            while (q>0 && !vibound(vich(ed, q-1))) q = viback(ed, q);
        }
    }
    #undef vibound
    ed->point = q;
    edkill(ed, o, q);
}

// An operator's motion, its keys and where it began, as readline's
// _rl_vimotion_cxt
struct ViMove {
    i32 op;      // VIM_*
    u8  key;     // the operator's key
    u8  motion;
    iz  start;
    iz  end;     // the line's length
};

static void vikey(Editor *, Shell *, u8);

// d, c or y over point to mark, as readline's vi_*_dispatch
static void viop(Editor *ed, ViMove *m)
{
    iz  len  = ed->line.len;
    Str keep = m->op==VIM_DELETE ? Str(" l|h^0bBFT`")
             : m->op==VIM_CHANGE ? Str(" l|hwW^0bBFT`")
             : Str(" l|h^0%bBFT`");
    if (m->motion && !edin(keep, m->motion) && ed->point>=m->start && ed->mark<len) {
        ed->mark = vinextany(ed, ed->mark);
    }
    switch (m->op) {
    case VIM_DELETE:
        edkill(ed, ed->point, ed->mark);
        break;
    case VIM_CHANGE:
        if ((m->motion|0x20)=='w' && ed->point<m->start) ed->point = m->start;
        if (ed->viredoing) {
            if (ed->vibuf.len) edgroup(ed, U_BEGIN);
            eddelete(ed, ed->point, ed->mark);
            if (ed->vibuf.len) {
                edinsert(ed, ed->vibuf);
                edgroup(ed, U_END);
            }
        } else {
            edgroup(ed, U_BEGIN);  // so u undoes the change
            edkill(ed, ed->point, ed->mark);
            if (!viupper(m->key)) ed->vidoing = 1;  // C keeps no text
            vistartins(ed, m->key, ed->count);
        }
        break;
    case VIM_YANK:
        edgroup(ed, U_BEGIN);
        edkill(ed, ed->point, ed->mark);
        edgroup(ed, U_END);
        edundo1(ed);
        ed->point = m->start;
        if (ed->point > ed->line.len) ed->point = ed->line.len;
        if (ed->mark > ed->line.len)  ed->mark  = ed->line.len;
        break;
    }
}

// The motion c, run with a blank on the end of the line so it works
// there, then the operator: -1 when it went nowhere.
static i32 vimotion(Editor *ed, Shell *sh, ViMove *m, u8 c)
{
    ed->vilastmotion = m->motion = c;
    reserve(&ed->line, 1);
    ed->line.data[ed->line.len++] = ' ';
    iz o = ed->point;
    ed->vifail = 0;
    vikey(ed, sh, c);
    b32 failed = ed->vifail && ed->point==o && edin("fFtT;,", c);
    ed->line.len = m->end;
    if (ed->point > ed->line.len) ed->point = ed->line.len;

    if (ed->mark == ed->point) {
        // c, d and y go ahead when a good motion didn't move
        u8 k = m->key & ~0x20;
        if ((k=='C' || k=='D' || k=='Y') && vimotionkey(c) && !failed) {
            viop(ed, m);
            return 0;
        }
        return -1;
    }
    // w leaves point on the next word: back onto the blank
    if ((c|0x20)=='w' && ed->point<ed->line.len && ed->point>ed->mark &&
        !viblank(vich(ed, ed->point))) {
        ed->point--;
    }
    // cw and cW change to the end of the word, as ce
    if (m->key=='c' && ed->point>=ed->mark && (c|0x20)=='w') {
        while (ed->point>ed->mark && viblank(vich(ed, ed->point))) ed->point--;
        if (ed->point == ed->mark) {
            ed->point = viadvance(ed, ed->point);
        } else if (ed->point>=0 && ed->point<ed->line.len-1 &&
                   !viblank(vich(ed, ed->point))) {
            ed->point = viadvance(ed, ed->point);
        }
    }
    if (ed->mark < ed->point) {
        iz t = ed->point;
        ed->point = ed->mark;
        ed->mark  = t;
    }
    viop(ed, m);
    return 0;
}

// The motion read: a motion key, the operator again for the whole line,
// or a count then a motion. 1 when it was none of those.
static i32 viread(Editor *ed, Shell *sh, ViMove *m)
{
    i32 c = vireadkey(ed, sh);
    if (c < 0) return -1;
    if (vimotionkey((u8)c)) return vimotion(ed, sh, m, (u8)c);
    if (c==m->key && edin("dyc", m->key)) {
        ed->mark = ed->line.len;
        ed->point = 0;
        ed->vilastmotion = m->motion = (u8)c;
        viop(ed, m);
        return 0;
    }
    if (c<'1' || c>'9') return 1;

    // A count for the motion, multiplying the operator's
    i32 save = ed->count;
    i64 n    = c - '0';
    i32 sign = save<0 ? -1 : 1;
    ed->argset = 1;
    Str prompt = ed->prompt;
    for (;;) {
        if (n > 1000000) {
            edbell(ed);
            n = 1;
            ed->argset = 0;
            save = sign = 1;
        }
        Arena a = ed->linemem;
        Buf   b(&a, 32);
        print(&b, "(arg: ");
        print(&b, n*sign);
        print(&b, ") ");
        ed->prompt = finish(&b);
        redisplay(ed, sh);
        ed->prompt = prompt;
        c = vireadkey(ed, sh);
        if (c < 0) return -1;
        KeyNode *k = keychild(edmap(ed), (u8)c);
        if (k && !k->kids && k->fn==EF_UNIVERSAL_ARGUMENT) {
            n *= 4;
        } else if (c>='0' && c<='9') {
            n = n*10 + c - '0';
        } else {
            break;
        }
    }
    redisplay(ed, sh);
    ed->count = (i32)(n * (save<0 ? -save : save) * sign);
    if (!vimotionkey((u8)c)) {
        m->motion = 0;
        return 1;
    }
    return vimotion(ed, sh, m, (u8)c);
}

// d, c, y and D, C, Y
static void vioperator(Editor *ed, Shell *sh, i32 op, u8 key)
{
    u8 again = op==VIM_DELETE ? 'd' : op==VIM_CHANGE ? 'c' : 'y';
    ViMove m = {};
    m.op    = op;
    m.key   = key;
    m.start = ed->point;
    m.end   = ed->line.len;
    ed->mark = ed->point;
    i32 r = 0;
    if (viupper(key)) {
        r = vimotion(ed, sh, &m, '$');
    } else if (ed->viredoing && ed->vilastmotion!=again) {
        r = vimotion(ed, sh, &m, ed->vilastmotion);
    } else if (ed->viredoing) {
        m.motion  = ed->vilastmotion;
        ed->mark  = ed->line.len;
        ed->point = 0;
        viop(ed, &m);
    } else {
        r = viread(ed, sh, &m);
    }
    if (r<0 && !ed->done) edbell(ed);
}

// R: overstrike, in a keymap of its own made once from vi-insert's
static void vireplace(Editor *ed, u8 key)
{
    ed->vireplcount = 0;
    if (!ed->vireplmap) {
        Arena   *a = &ed->perm;
        KeyNode *m = alloc<KeyNode>(a);
        for (i32 i = 0; i < 0x20; i++) {
            KeyNode *k = keychild(&ed->viins, (u8)i);
            if (!k || k->kids || !k->fn || k->fn==EF_MACRO || k->fn==EF_SHELLCMD) continue;
            u8 b = (u8)i;
            keybind(a, m, {&b, 1}, k->fn);
        }
        for (i32 i = 0x20; i < 0x100; i++) {
            u8 b = (u8)i;
            keybind(a, m, {&b, 1}, i==0x7f ? EF_VI_OVERSTRIKE_DELETE : EF_VI_OVERSTRIKE);
        }
        keybind(a, m, "\x1b", EF_VI_MOVEMENT_MODE);
        keybind(a, m, "\r", EF_ACCEPT_LINE);
        keybind(a, m, "\n", EF_ACCEPT_LINE);
        struct { u8 c; i32 from, to; } over[] = {
            {0x08, EF_BACKWARD_DELETE_CHAR, EF_VI_OVERSTRIKE_DELETE},
            {0x15, EF_UNIX_LINE_DISCARD,    EF_VI_OVERSTRIKE_KILL_LINE},
            {0x17, EF_VI_UNIX_WORD_RUBOUT,  EF_VI_OVERSTRIKE_KILL_WORD},
            {0x19, EF_YANK,                 EF_VI_OVERSTRIKE_YANK},
        };
        for (iz i = 0; i < countof(over); i++) {
            KeyNode *k = keychild(&ed->viins, over[i].c);
            if (k && !k->kids && k->fn==over[i].from) {
                keybind(a, m, {&over[i].c, 1}, over[i].to);
            }
        }
        ed->vireplmap = m;
    }
    vistartins(ed, key, ed->count<0 ? -1 : 1);
    ed->vilastins  = 'R';  // in case another key is bound to it
    ed->vireplmode = 1;
    if (ed->vars.bracketpaste) {
        KeyNode *n = ed->vireplmap;
        Str      p = "\x1b[200~";
        for (iz i = 0; n && i < p.len; i++) n = keychild(n, p[i]);
        if (!n || (!n->fn && !n->kids)) {
            keybind(&ed->perm, ed->vireplmap, p, EF_VI_OVERSTRIKE_PASTE);
        }
    }
}

static void vioverstrike(Editor *ed, Str ch, i32 count)
{
    if (!ed->vidoing) {
        ed->vidoing = 1;
        edgroup(ed, U_BEGIN);
    }
    if (count > 0) {
        edoverchar(ed, ch, count);
        ed->vireplcount += count;
    }
}

// Run key as it is bound in the current keymap, as readline's
// _rl_dispatch: a motion for an operator, or . repeating a command
static void vikey(Editor *ed, Shell *sh, u8 key)
{
    KeyNode *map = edmap(ed);
    KeyNode *k   = keychild(map, key);
    i32      fn  = k ? k->fn : EF_NONE;
    if (fn == EF_MACRO) {
        edplay(ed, k->macro);
        return;
    }
    if (fn == EF_SHELLCMD) fn = EF_NONE;
    edrun(ed, sh, fn, key);
    ed->lastfn = fn;
    if (ed->vicmd && !ed->vireplmode && map==&ed->vimove && vitextmod(key)) {
        visetlast(ed, key, ed->count);
    }
}

// . : the last command again
static void viredo(Editor *ed, Shell *sh, i32 count)
{
    if (!ed->argset) ed->count = ed->vilastcount;
    ed->viredoing = 1;
    u8  last = ed->vilastcmd;
    b32 text = ed->vibuf.len > 0;
    if (text && edin("iIaA", last)) {
        if (last == 'I') ed->point = 0;
        if (last == 'a') ed->point = viadvance(ed, ed->point);
        if (last == 'A') ed->point = ed->line.len;
        edgroup(ed, U_BEGIN);
        for (i32 i = 0; i < count; i++) edinsert(ed, ed->vibuf);
        edgroup(ed, U_END);
        if (ed->point > 0) ed->point = edprev(ed, ed->point);
    } else if (text && last=='R') {
        iz n = ed->vibuf.len;
        edgroup(ed, U_BEGIN);
        for (i32 i = 0; i < count; i++) {
            edgroup(ed, U_BEGIN);
            iz at = ed->point;
            eddelete(ed, at, at+n);
            ed->point = at;
            edinsert(ed, ed->vibuf);
            edgroup(ed, U_END);
        }
        edgroup(ed, U_END);
        if (ed->point > 0) ed->point = edprev(ed, ed->point);
    } else if (last=='.' && ed->vicmd) {
        edbell(ed);
    } else {
        vikey(ed, sh, last);
    }
    ed->viredoing = 0;
}

// ~ : the case of count characters swapped, point past them
static void vichangecase(Editor *ed, Shell *sh, i32 count)
{
    if (ed->point >= ed->line.len) return;
    while (count-- && ed->point<ed->line.len) {
        iz  j  = ed->point;
        i32 cp = utf8decode(edtext(ed), &j);
        i32 to = -1;
        if (sh->utf8 || cp<0x80) {
            if (cplower(cp) != cp) {
                to = cplower(cp);
            } else if (cpupper(cp) != cp) {
                to = cpupper(cp);
            }
        }
        if (to <= 0) {
            ed->point = edcharmove(ed, ed->point, 1);
            continue;
        }
        u8 buf[4];
        iz n = utf8encode(buf, to);
        iz q = ed->point;
        edgroup(ed, U_BEGIN);
        videlete(ed, 1);
        if (ed->point < q) ed->point = viadvance(ed, ed->point);
        edinsert(ed, {buf, n});
        edgroup(ed, U_END);
        vicheck(ed);
    }
}

// r: count characters replaced by one read
static void vichangechar(Editor *ed, Shell *sh, i32 count)
{
    if (!ed->viredoing) {
        u8  buf[4];
        i32 n = vireadchar(ed, sh, buf);
        if (n < 0) return;
        copybytes(ed->virepl, buf, n);
        ed->vireplen = n;
    }
    if (!ed->vireplen) return;
    u8 c = ed->virepl[0];
    if (c==0x1b || c==0x03) return;
    Str ch = {ed->virepl, ed->vireplen};
    edgroup(ed, U_BEGIN);
    while (count-- && ed->point<ed->line.len) {
        iz p = ed->point;
        videlete(ed, 1);
        if (ed->point < p) ed->point = viadvance(ed, ed->point);
        edinsert(ed, ch);
    }
    ed->point = edcharmove(ed, ed->point, -1);  // on the last one changed
    edgroup(ed, U_END);
}

// vi-complete and bash-vi-complete (\ * =)
static void vicomplete(Editor *ed, Shell *sh, i32 fn, u8 key)
{
    iz p = ed->point;
    if (p<ed->line.len && !viblank(vich(ed, p))) {
        if (!viblank(vich(ed, p+1))) vieWord(ed, 1);
        ed->point = viadvance(ed, ed->point);
    }
    i32 f = EF_COMPLETE;
    if (fn == EF_BASH_VI_COMPLETE) {
        // A word with no glob characters gets a star, as with an argument
        if (ed->point > 0) {
            iz q = ed->point;
            vibWord(ed, 1);
            iz r = ed->point;
            ed->point = q;
            if (!compglobchars(span(ed->line.data+r, ed->line.data+q))) ed->argset = 1;
        }
        switch (key) {
        case '*':  f = EF_GLOB_EXPAND_WORD;     break;
        case '=':  f = EF_GLOB_LIST_EXPANSIONS; break;
        case '\\': f = EF_GLOB_COMPLETE_WORD;   break;
        }
    } else {
        switch (key) {
        case '*': f = EF_INSERT_COMPLETIONS;   break;
        case '=': f = EF_POSSIBLE_COMPLETIONS; break;
        }
    }
    edcomplete(ed, sh, f, ed->argset ? ed->count : 1);
    if (key=='*' || key=='\\') {
        vistartins(ed, key, fn==EF_BASH_VI_COMPLETE ? 1 : ed->count<0 ? -1 : 1);
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
    case EF_DABBREV_EXPAND:
    case EF_EXPORT_COMPLETIONS:
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
            KeyNode *k = keychild(edmap(ed), (u8)c);
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
        if (ed->vars.histpoint) ed->hpoint = p==len ? -1 : p;
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
        ed->ylundo = edyankarg(ed, sh, ed->ylarg ? ed->ylcount : '$', ed->ylskip, key);
        break;
    case EF_YANK_NTH_ARG:
        edyankarg(ed, sh, count, 0, key);
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
            if (ed->vars.vimode) ed->point = ed->mark = 0;
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
        edabort(ed);
        break;
    case EF_START_KBD_MACRO:
        if (ed->kbddef) {
            edabort(ed);
            break;
        }
        ed->kbddef = 1;
        ed->kbdrec = Buf(&ed->perm, 64);
        if (ed->argset && ed->kbdmacro.len) {
            // Run again, and so kept, then added to
            print(&ed->kbdrec, ed->kbdmacro);
            edplay(ed, ed->kbdmacro);
        }
        break;
    case EF_END_KBD_MACRO:
        if (!ed->kbddef) {
            edabort(ed);
            break;
        }
        ed->kbddef   = 0;
        ed->kbdmacro = clone(&ed->perm, Str{ed->kbdrec.data, ed->seqstart});
        for (i32 i = 1; i < count; i++) edplay(ed, ed->kbdmacro);
        break;
    case EF_CALL_LAST_KBD_MACRO:
        if (ed->kbddef) {
            edbell(ed);  // not within itself
            ed->kbdrec.len = ed->seqstart;
        } else if (!ed->kbdmacro.data) {
            edabort(ed);
        } else {
            for (i32 i = 0; i < count; i++) edplay(ed, ed->kbdmacro);
        }
        break;
    case EF_PRINT_LAST_KBD_MACRO: {
        if (!ed->kbdmacro.data) {
            edbell(ed);
            break;
        }
        Buf b(&ed->linemem, 64);
        rlputkeys(&b, ed->kbdmacro);
        putbyte(&b, '\n');
        edshowtext(ed, sh, finish(&b));
    } break;
    case EF_DISPLAY_SHELL_VERSION: {
        Arena a = *scratchat(sh, sh->capdepth+1);
        edshowtext(ed, sh, concat(&a, helpversion(sh, &a), "\n"));
    } break;
    case EF_DUMP_FUNCTIONS:
    case EF_DUMP_MACROS:
    case EF_DUMP_VARIABLES: {
        Arena    a   = *scratchat(sh, sh->capdepth+1);
        KeyNode *map = rlkeymap(ed, ed->vars.keymap);
        Str      t   = {};
        switch (fn) {
        case EF_DUMP_FUNCTIONS: t = bindfunctions(map, ed->argset, &a); break;
        case EF_DUMP_MACROS:    t = bindmacros(map, EF_MACRO, ed->argset, &a); break;
        case EF_DUMP_VARIABLES: t = bindvariables(ed, ed->argset, &a); break;
        }
        edshowtext(ed, sh, t);
    } break;
    case EF_EXECUTE_NAMED_COMMAND: {
        Str name = {};
        if (!edreadstr(ed, sh, "!", &name, 1) || !name.len) break;
        i32 f = rlnamedfn(name);
        if (f<=0 || f==EF_EXECUTE_NAMED_COMMAND) {
            edbell(ed);
        } else {
            edrun(ed, sh, f, key);
        }
    } break;
    case EF_BACKWARD_BYTE:
    case EF_FORWARD_BYTE: {
        i32 n = fn==EF_FORWARD_BYTE ? count : -count;
        if (n < 0) {
            if (p < -n) {
                ed->point = 0;
                edbell(ed);
            } else {
                ed->point = p + n;
            }
        } else if (p+n > len) {
            ed->point = len;
            edbell(ed);
        } else {
            ed->point = p + n;
        }
    } break;
    case EF_FORWARD_BACKWARD_DELETE_CHAR:
        edrun(ed, sh, p==len && len ? EF_BACKWARD_DELETE_CHAR : EF_DELETE_CHAR, key);
        break;
    case EF_NEXT_SCREEN_LINE:
    case EF_PREVIOUS_SCREEN_LINE: {
        i32 save  = ed->count;
        i32 cols  = ed->tty.cols>0 ? ed->tty.cols : 80;
        ed->count = ed->tty.xenl ? cols : cols+1;
        edrun(ed, sh, fn==EF_NEXT_SCREEN_LINE ? EF_FORWARD_CHAR : EF_BACKWARD_CHAR, key);
        ed->count = save;
    } break;
    case EF_SKIP_CSI_SEQUENCE: {
        i32 c;
        do c = edkey(ed, sh, -1); while (c>=0x20 && c<0x40);
    } break;
    case EF_ARROW_KEY_PREFIX: {
        i32 c = edkey(ed, sh, -1);
        if (c < 0) break;
        switch (c & ~0x20) {
        case 'A': edrun(ed, sh, EF_PREVIOUS_HISTORY, key); break;
        case 'B': edrun(ed, sh, EF_NEXT_HISTORY, key);     break;
        case 'C': edrun(ed, sh, EF_FORWARD_CHAR, key);     break;
        case 'D': edrun(ed, sh, EF_BACKWARD_CHAR, key);    break;
        default:  edbell(ed);
        }
    } break;
    case EF_OLD_MENU_COMPLETE:
        edcomplete(ed, sh, EF_MENU_COMPLETE, ed->argset ? ed->count : 1);
        break;
    case EF_ALIAS_EXPAND_LINE:
        edexpandaliases(ed, sh, 0);
        break;
    case EF_HISTORY_AND_ALIAS_EXPAND_LINE:
        edexpandaliases(ed, sh, 1);
        break;
    case EF_TILDE_EXPAND:
        edtildeexpand(ed, sh);
        break;
    case EF_SPELL_CORRECT_WORD:
        edspellword(ed, sh, count);
        break;
    case EF_EMACS_EDITING_MODE:
        ed->vars.vimode = 0;
        ed->vars.keymap = KM_EMACS;
        ed->vicmd = ed->vireplmode = ed->overwrite = 0;
        edoptsync(ed, sh);
        break;
    case EF_VI_EDITING_MODE:
        ed->vars.vimode = 1;
        ed->overwrite   = 0;
        viinsmode(ed, key);
        edoptsync(ed, sh);
        break;
    case EF_VI_MOVEMENT_MODE:
        vimovement(ed);
        break;
    case EF_VI_INSERTION_MODE:
        vistartins(ed, key, count<0 ? -1 : 1);
        break;
    case EF_VI_INSERT_BEG:
        ed->point = 0;
        vistartins(ed, key, count<0 ? -1 : 1);
        break;
    case EF_VI_APPEND_MODE:
        ed->point = viadvance(ed, p);
        vistartins(ed, key, count<0 ? -1 : 1);
        break;
    case EF_VI_APPEND_EOL:
        ed->point = len;
        vistartins(ed, key, count<0 ? -1 : 1);
        break;
    case EF_VI_EOF_MAYBE:
        ed->done   = 1;
        ed->result = ED_LINE;
        break;
    case EF_VI_UNDO:
        edrun(ed, sh, EF_UNDO, key);
        break;
    case EF_VI_REDO:
        viredo(ed, sh, count);
        break;
    case EF_VI_DELETE_TO:
        vioperator(ed, sh, VIM_DELETE, key);
        break;
    case EF_VI_CHANGE_TO:
        vioperator(ed, sh, VIM_CHANGE, key);
        break;
    case EF_VI_YANK_TO:
        vioperator(ed, sh, VIM_YANK, key);
        break;
    case EF_VI_SUBST:
        if (!ed->viredoing) edpush(ed, key=='S' ? 'c' : 'l');
        vioperator(ed, sh, VIM_CHANGE, 'c');
        break;
    case EF_VI_DELETE:
        videlete(ed, count);
        break;
    case EF_VI_RUBOUT:
        virubout(ed, count);
        break;
    case EF_VI_PUT:
        viput(ed, key, count);
        break;
    case EF_VI_YANK_POP: {
        if ((ed->lastfn!=EF_VI_YANK_POP && ed->lastfn!=EF_VI_PUT) || !ed->nkills) {
            edabort(ed);
            break;
        }
        Str k = ed->kills[ed->killidx];
        iz  n = p - k.len + 1;
        if (n<0 || n+k.len>len || !(Str{ed->line.data+n, k.len}==k)) {
            edabort(ed);
            break;
        }
        eddelete(ed, n, n+k.len);
        ed->point   = p - k.len;
        ed->killidx = ed->killidx ? ed->killidx-1 : ed->nkills-1;
        viput(ed, 'p', 1);
    } break;
    case EF_VI_CHANGE_CASE:
        vichangecase(ed, sh, count);
        break;
    case EF_VI_CHANGE_CHAR:
        vichangechar(ed, sh, count);
        break;
    case EF_VI_REPLACE:
        vireplace(ed, key);
        break;
    case EF_VI_OVERSTRIKE: {
        // The rest of a UTF-8 character it starts
        u8  buf[4] = {key};
        i32 n   = key>=0xf0 ? 4 : key>=0xe0 ? 3 : key>=0xc0 ? 2 : 1;
        i32 got = 1;
        while (got < n) {
            i32 c = edkey(ed, sh, -1);
            if (c < 0) break;
            if ((c&0xc0) != 0x80) {
                edpush(ed, c);
                break;
            }
            buf[got++] = (u8)c;
        }
        vioverstrike(ed, {buf, got}, count);
    } break;
    case EF_VI_OVERSTRIKE_DELETE:
        for (i32 i = 0; i < count; i++) {
            if (!ed->vireplcount) {
                edbell(ed);
                break;
            }
            iz at = ed->point;
            if (edundo1(ed)) ed->vireplcount--;
            if (ed->point == at) ed->point = edcharmove(ed, ed->point, -1);
        }
        if (!ed->vireplcount && ed->vidoing) {
            edgroup(ed, U_END);
            edundo1(ed);
            ed->vidoing = 0;
        }
        break;
    case EF_VI_OVERSTRIKE_KILL_LINE:
        edrun(ed, sh, EF_UNIX_LINE_DISCARD, key);
        ed->vireplcount -= (i32)(len - ed->line.len);
        break;
    case EF_VI_OVERSTRIKE_KILL_WORD:
        viunixword(ed, count);
        ed->vireplcount -= (i32)(len - ed->line.len);
        break;
    case EF_VI_OVERSTRIKE_YANK:
        edyank(ed);
        ed->vireplcount += (i32)(ed->line.len - len);
        break;
    case EF_VI_OVERSTRIKE_PASTE: {
        Str t = edpaste(ed, sh, &ed->hmem);
        for (iz i = 0; i < t.len;) {
            iz j = i;
            for (j++; j<t.len && (t[j]&0xc0)==0x80; j++) {}
            vioverstrike(ed, {t.data+i, j-i}, 1);
            i = j;
        }
    } break;
    case EF_VI_UNIX_WORD_RUBOUT:
        viunixword(ed, count);
        break;
    case EF_VI_COLUMN:
        if (count > len) {
            ed->point = len;
        } else {
            iz q = 0;
            for (i32 i = 1; i<count && q<len; i++) q = ednext(ed, q);
            if (ed->vicmd && q>=len) q = edprev(ed, len);
            ed->point = q;
        }
        break;
    case EF_VI_FIRST_PRINT:
    case EF_VI_BACK_TO_INDENT: {
        iz q = 0;
        for (; q<len && viblank(ed->line.data[q]); q++) {}
        ed->point = q;
    } break;
    case EF_VI_MATCH:
        vimatch(ed);
        break;
    case EF_VI_CHAR_SEARCH:
        ed->vifail = vicharsearch(ed, sh, count, key);
        break;
    case EF_VI_NEXT_WORD:
    case EF_VI_PREV_WORD: {
        b32 fwd = (fn==EF_VI_NEXT_WORD) != (count<0);
        i32 n   = count<0 ? -count : count;
        if (fwd ? p>=len-1 : !p) {
            edbell(ed);
        } else if (fwd) {
            if (viupper(key)) vifWord(ed, n); else vifword(ed, n);
        } else {
            if (viupper(key)) vibWord(ed, n); else vibword(ed, n);
        }
    } break;
    case EF_VI_END_WORD:
        if (count < 0) {
            edbell(ed);
        } else if (viupper(key)) {
            vieWord(ed, count);
        } else {
            vieword(ed, count);
        }
        break;
    // Named alone, the capital forms win: readline looks names up
    // ignoring case, and finds vi-bWord before vi-bword
    case EF_VI_FORWARD_WORD:    vifword(ed, count); break;
    case EF_VI_FWORD:
    case EF_VI_FWORD_CAP:
    case EF_VI_FORWARD_BIGWORD: vifWord(ed, count); break;
    case EF_VI_BACKWARD_WORD:   vibword(ed, count); break;
    case EF_VI_BWORD:
    case EF_VI_BWORD_CAP:
    case EF_VI_BACKWARD_BIGWORD: vibWord(ed, count); break;
    case EF_VI_EWORD:
    case EF_VI_EWORD_CAP:
    case EF_VI_END_BIGWORD:     vieWord(ed, count); break;
    case EF_VI_SET_MARK: {
        i32 c = vireadkey(ed, sh);
        if (c<'a' || c>'z') {
            if (!ed->done) edbell(ed);
            break;
        }
        ed->vimarks[c-'a'] = p;
    } break;
    case EF_VI_GOTO_MARK: {
        i32 c = vireadkey(ed, sh);
        iz  to;
        if (c == '`') {
            to = ed->mark;
        } else if (c<'a' || c>'z' || ed->vimarks[c-'a']<0) {
            if (!ed->done) edbell(ed);
            break;
        } else {
            to = ed->vimarks[c-'a'];
        }
        ed->point = to<0 ? 0 : to>len ? len : to;
        if (ed->mark > len) ed->mark = len;
        if (ed->mark < 0)   ed->mark = 0;
    } break;
    case EF_VI_SEARCH:
        if (key == '?') {
            ednoninc(ed, sh, 1, 0, "?", ed->vicmd);
        } else if (key == '/') {
            ednoninc(ed, sh, -1, 0, "/", ed->vicmd);
        } else {
            edbell(ed);
        }
        break;
    case EF_VI_SEARCH_AGAIN:
        if (key == 'n') {
            if (!ed->noninc.data) edbell(ed); else ednoninc(ed, sh, -1, 1, {}, ed->vicmd);
        } else if (key == 'N') {
            if (!ed->noninc.data) edbell(ed); else ednoninc(ed, sh, 1, 1, {}, ed->vicmd);
        }
        break;
    case EF_VI_YANK_ARG:
        if (ed->argset) {
            edyankarg(ed, sh, count-1, 0, key);
        } else {
            edyankarg(ed, sh, '$', 0, key);
        }
        break;
    case EF_VI_COMPLETE:
    case EF_BASH_VI_COMPLETE:
        vicomplete(ed, sh, fn, key);
        break;
    case EF_VI_TILDE_EXPAND:
        edtildeexpand(ed, sh);
        vistartins(ed, key, count<0 ? -1 : 1);
        break;
    case EF_VI_EDIT_AND_EXECUTE_COMMAND:
        ededitexec(ed, sh, 1, key);
        return;
    case EF_NULL_FUNCTION:
        return;
    case EF_POSIX_EDIT_MACROS: {
        i32 c = vireadkey(ed, sh);
        if (c <= 0) break;
        u8     name[] = {'_', (u8)c};
        Alias *al     = findalias(sh->aliases, {name, 2});
        if (al && al->value.len) edplay(ed, al->value);
    } break;
    case EF_TTY_STATUS:
        edbell(ed);  // readline's without TIOCSTAT, as on Linux
        break;
    case EF_NON_INCREMENTAL_REVERSE_SEARCH_HISTORY_AGAIN:
        ednoninc(ed, sh, -1, 1);
        break;
    case EF_NON_INCREMENTAL_FORWARD_SEARCH_HISTORY_AGAIN:
        ednoninc(ed, sh, 1, 1);
        break;
    case EF_FETCH_HISTORY:
    case EF_VI_FETCH_HISTORY:
        if (ed->argset) {
            // The entry numbered as history lists it; negative from the end
            i64 nhist  = sh->hist.base + ed->hpos;
            i64 wanted = count>=0 ? nhist-count : -(i64)count;
            if (wanted<=0 || wanted>=nhist) {
                if (ed->vars.vimode) {
                    edbell(ed);
                } else {
                    edprevhist(ed, sh, ed->hpos+1);
                }
            } else {
                edprevhist(ed, sh, (i32)wanted);
            }
        } else {
            edprevhist(ed, sh, ed->hpos+1);
        }
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
    case EF_HISTORY_SUBSTRING_SEARCH_BACKWARD:
    case EF_HISTORY_SUBSTRING_SEARCH_FORWARD:
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
    if (n==1 && ed->kbddef) putbyte(&ed->kbdrec, c);
    return n==1 ? c : n==-2 ? -2 : -1;
}

// A key sequence read and looked up: its function and last key, or a
// status (ED_EOF at the end of input, ED_INTR on an interrupt).
struct EdSeq {
    Str      macro;   // EF_MACRO
    KeyNode *in;      // the keymap the last key was found in
    i32      fn;
    u8       key;
    u8       first;   // its first key
    i32      status;  // -1 for a sequence
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
    KeyNode *top    = edmap(ed);
    KeyNode *parent = top;
    KeyNode *n      = keychild(parent, (u8)c);
    u8 last = (u8)c;
    i64 wait = ed->keytimeout>0 ? ed->keytimeout : -1;
    KeyNode *path[32];  // the nodes passed, and the keys into them
    u8       keys[32];
    i32      depth = 1;
    path[0] = n;
    keys[0] = (u8)c;
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
        if (!k) {
            // The deepest prefix with a binding of its own, shadowed by
            // longer ones, runs, and the keys after it come again
            i32 i = depth - 1;
            for (; i>=0 && (!path[i]->fn || path[i]->fn==EF_DO_LOWERCASE_VERSION); i--) {}
            if (i < 0) {
                // An unbound sequence: the bell, the rest dropped (also
                // under M-O, the arrow keys' prefix, as readline)
                n = 0;
                break;
            }
            edpush(ed, next);
            for (i32 j = depth-1; j > i; j--) edpush(ed, keys[j]);
            parent = i ? path[i-1] : top;
            n      = path[i];
            last   = keys[i];
            break;
        }
        if (depth == countof(path)) {
            n = 0;
            break;
        }
        path[depth]   = k;
        keys[depth++] = (u8)next;
        parent = n;
        n      = k;
        last   = (u8)next;
    }
    r.fn    = n ? n->fn : EF_NONE;
    r.macro = n ? n->macro : Str{};
    r.key   = last;
    r.in    = parent;
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
    ed->seqstart = ed->kbdrec.len;
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
    ed->curkey = s.key;
    switch (s.fn) {
    case EF_DIGIT_ARGUMENT:
        edarg(ed, sh, s.key);
        break;
    case EF_VI_ARG_DIGIT:
        if (s.key=='0' && ed->count==1 && !ed->argset) {
            ed->point  = 0;
            ed->lastfn = s.fn;
        } else {
            edarg(ed, sh, s.key);
        }
        break;
    case EF_MACRO: {
        // Its keys come next, before the rest of any running macro
        Str rest = cuthead(ed->macro, ed->macropos);
        ed->macro    = rest.len ? concat(&ed->perm, s.macro, rest) : s.macro;
        ed->macropos = 0;
    } break;
    case EF_SHELLCMD:
        edshellcmd(ed, sh, s.macro);
        ed->lastfn = s.fn;
        break;
    case EF_UNIVERSAL_ARGUMENT:
        ed->count *= 4;
        edarg(ed, sh, -1);
        ed->lastfn = EF_UNIVERSAL_ARGUMENT;
        break;
    default:
        ed->killed = 0;
        edrun(ed, sh, s.fn, s.key);
        ed->lastkill = ed->killed;
        ed->lastfn   = s.fn==EF_OLD_MENU_COMPLETE ? EF_MENU_COMPLETE : s.fn;
        if (ed->vars.vimode && ed->vicmd && !ed->vireplmode &&
            s.in==&ed->vimove && vitextmod(s.key)) {
            visetlast(ed, s.key, ed->count);
        }
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
            KeyNode *k = keychild(edmap(ed), (u8)c);
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
    ed->tty   = os_tty(sh->os, ed->in, ed->out, TTY_RAW);
    ed->notty = 0;
    if (ed->tty.err) {
        // The shell's input that is no terminal is still read with the
        // editor, as bash's readline does, drawing as on a dumb
        // terminal COLUMNS wide
        if (!opts->shell || os_isatty(sh->os, ed->in)) return ED_NOTTY;
        Tty t = {};
        t.cols = 80;
        Arena a = *scratchat(sh, sh->capdepth+1);
        Str cols = getvar(sh, &a, "COLUMNS").value;
        i64 v = 0;
        for (iz i = 0; i<cols.len && v<100000; i++) {
            if (cols[i]<'0' || cols[i]>'9') { v = 0; break; }
            v = v*10 + cols[i] - '0';
        }
        if (v > 1) t.cols = (i32)v;
        t.rows = 24;
        ed->tty   = t;
        ed->notty = 1;
    } else {
        edsize(ed, sh, ed->tty);
    }
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
    ed->hscroll = ed->notty || ed->tty.rows<=1 || eddumb(sh, ed->linemem);
    ed->lmargin = ed->hshown = ed->unread = 0;
    ed->hlast   = {};
    ed->hlastcur = 0;
    if (!ed->inited) {
        ed->inited = 1;
        if (ed->hscroll) ed->vars.bracketpaste = ed->vars.activeregion = 0;
        rlstart(ed, sh);
    }
    if (!ed->termkeys && !ed->hscroll) {
        ed->termkeys = 1;
        edtermkeys(ed);
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
    ed->ulevel  = 0;
    if (ed->vars.vimode) {
        // Each line starts in insert mode, the marks unset
        for (i32 i = 0; i < 26; i++) ed->vimarks[i] = -1;
        ed->vicmdonce = ed->vidoing = 0;
        viinsmode(ed, 'i');
    }
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
    ed->lineprompt = ed->prompt;
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
        if (ed->vars.vimode && ed->vicmd && !ed->vireplmode) vicheck(ed);
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

    if (ed->vars.vimode && ed->result==ED_LINE) viacceptline(ed);
    edleave(ed, sh, ed->result==ED_INTR);

    // Leave the cursor past the line
    Layout *l = &ed->shown;
    b32 crlf = 1;
    if (ed->result==ED_EOF && ed->notty) {
        crlf = 0;  // the end of input that is no terminal: "exit" follows
    } else if (ed->result == ED_ABORT) {
        ed->line.len = ed->point = 0;  // the message is past the line
        crlf = 0;
    } else if (ed->result == ED_TIMEOUT) {
        // As readline's timeout: the line goes unused, and what follows
        // is written over it from the cursor's row
        ed->line.len = ed->point = 0;
        edput(ed, "\r");
        crlf = 0;
    } else if (ed->hscroll) {
        if (ed->result==ED_INTR && ed->vars.echoctl) edput(ed, "^C");
    } else if (ed->result == ED_INTR) {
        if (ed->starred) {
            // Readline drops the line's undo list first, and its star
            ed->vars.markmodified = 0;
            redisplay(ed, sh);
            ed->vars.markmodified = 1;
        }
        // At the prompt with INT trapped, as bash: the line goes, and
        // the trap writes over it from the start of its row (fillline)
        b32 trapped = opts->shell && trapset(sh, 2) && interactivetop(sh);
        if (isread || trapped) {
            // read -e: as readline on SIGINT, the line goes first
            ed->line.len = ed->point = 0;
            redisplay(ed, sh);
        }
        // Echoed at the cursor, over the rest of the line, as by the
        // terminal's own line discipline, unless echo-control-characters
        // is off
        if (ed->vars.echoctl) edput(ed, "^C");
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
    if (crlf && !(ed->result==ED_INTR && isread)) {
        edput(ed, ed->notty ? Str("\n") : "\r\n");  // readline's crlf
    }
    if (bracket) edput(ed, "\x1b[?2004l\r");
    edflush(ed, sh);
    if (!ed->notty) edsize(ed, sh, os_tty(sh->os, ed->in, ed->out, TTY_RESTORE));
    print(out, {ed->line.data, ed->line.len});
    return ed->result;
}
