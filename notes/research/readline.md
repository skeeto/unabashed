Report: line editor requirements ("feel the same" as bash 5.3 + readline 8.3)

Abbreviations: RL = `<bash-5.3>/lib/readline`, DOC = `<bash-5.3>/doc`, where `<bash-5.3>` = `C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3`.

Priority tiers:
- P0: needed for a first interactive release.
- P1: regular users expect it; needed before it can stand in for bash.
- P2: power-user features.
- P3: rare or legacy.

====================================================================
1. KEYMAP ARCHITECTURE (P0)
====================================================================
- Keymaps are 256-entry tables. Each entry is one of three types: function, sub-keymap (prefix), or macro string. Built-in keymaps: `emacs_standard_keymap`, `emacs_meta_keymap` (reached through ESC), `emacs_ctlx_keymap` (reached through C-x), `vi_movement_keymap`, `vi_insertion_keymap`, plus `vi_escape_keymap`. Sources: RL/emacs_keymap.c:32,315,595 and RL/vi_keymap.c:33,312,597.
- Keymap names accepted by `bind -m` and by `set keymap`: emacs, emacs-standard, emacs-meta, emacs-ctlx, vi, vi-move, vi-command, vi-insert. Here vi = vi-command = vi-move, and emacs = emacs-standard (DOC/bash.1:9096).
- Meta is ESC-prefixed. With convert-meta, an 8-bit input byte becomes ESC+byte. Upper-case M-A..M-Z and C-x A..Z are bound to `do-lowercase-version`.
- Ambiguous sequences (for example ESC alone versus ESC [ A) are settled by `keyseq-timeout`, default 500 ms (RL/readline.c:272). This matters for vi-mode ESC and for terminals that send Alt as an ESC prefix.
- The canonical names of all 145 bindable functions are in RL/funmap.c (`bind -l` prints them). Bash adds about 30 more with `rl_add_defun` (DOC is bashline.c:468-520 in the bash root).

====================================================================
2. DEFAULT EMACS BINDINGS (P0 unless marked)
====================================================================
Source: RL/emacs_keymap.c, plus the bash additions in `<bash-5.3>/bashline.c:527-635`.

Control keys:

| Key | Command | Tier |
|---|---|---|
| C-@ | set-mark | |
| C-a | beginning-of-line | |
| C-b | backward-char | |
| C-d | delete-char (EOF on an empty line) | |
| C-e | end-of-line | |
| C-f | forward-char | |
| C-g | abort | |
| C-h | backward-delete-char | |
| C-i (TAB) | complete | |
| C-j, C-m | accept-line | |
| C-k | kill-line | |
| C-l | clear-screen | |
| C-n | next-history | |
| C-o | operate-and-get-next | P1 |
| C-p | previous-history | |
| C-q, C-v | quoted-insert | |
| C-r | reverse-search-history | |
| C-s | forward-search-history | |
| C-t | transpose-chars | |
| C-u | unix-line-discard | |
| C-w | unix-word-rubout | |
| C-x | prefix | |
| C-y | yank | |
| C-] | character-search | P2 |
| C-_ | undo | |
| DEL | backward-delete-char | |

C-c, C-z and C-\ are unbound; the tty driver handles them.

Meta (ESC-prefixed) keys:

| Key | Command | Tier |
|---|---|---|
| M-C-g | abort | |
| M-C-h, M-DEL | backward-kill-word | |
| M-TAB | dynamic-complete-history (bash rebinds it from tab-insert) | |
| M-C-l | clear-display (clears scrollback too) | |
| M-C-r, M-r | revert-line | |
| M-C-y | yank-nth-arg | |
| M-C-[ | complete | |
| M-C-] | character-search-backward | |
| M-SPC | set-mark | |
| M-# | insert-comment | P1 |
| M-& | tilde-expand | |
| M-* | insert-completions | |
| M-- and M-0..M-9 | digit-argument | |
| M-. and M-_ | yank-last-arg | P1, heavily used |
| M-< | beginning-of-history | |
| M-> | end-of-history | |
| M-= and M-? | possible-completions | |
| M-\ | delete-horizontal-space | |
| M-b | backward-word | |
| M-c | capitalize-word | |
| M-d | kill-word | |
| M-f | forward-word | |
| M-l | downcase-word | |
| M-n | non-incremental-forward-search-history | |
| M-p | non-incremental-reverse-search-history | |
| M-t | transpose-words | |
| M-u | upcase-word | |
| M-x | execute-named-command (new in 8.3) | P3 |
| M-y | yank-pop | |
| M-~ | complete-username (bash replaces tilde-expand) | |

Bash rebinds M-C-j and M-C-m away from vi-editing-mode (bashline.c:538-547).

C-x prefix:

| Key | Command |
|---|---|
| C-x C-g | abort |
| C-x C-r | re-read-init-file |
| C-x C-u | undo |
| C-x C-x | exchange-point-and-mark |
| C-x ( | start-kbd-macro |
| C-x ) | end-kbd-macro |
| C-x e | call-last-kbd-macro |
| C-x DEL | backward-kill-line |

Bash-only bindings (bashline.c:527-635):

| Key | Command | Tier |
|---|---|---|
| M-C-e | shell-expand-line | |
| M-^ | history-expand-line | |
| C-x C-v | display-shell-version | |
| M-{ | complete-into-braces | |
| M-/ | complete-filename | |
| C-x / | possible-filename-completions | |
| M-@ / C-x @ | hostname completion | |
| M-$ / C-x $ | variable completion | |
| M-! / C-x ! | command completion | |
| C-x ~ | possible-username-completions | |
| M-g | glob-complete-word | |
| C-x * | glob-expand-word | |
| C-x g | glob-list-expansions | |
| C-x C-e | edit-and-execute-command (uses `$VISUAL`/`$EDITOR`/emacs) | P1 |
| C-x s | spell-correct-word | |
| M-C-b | shell-backward-word | |
| M-C-f | shell-forward-word | |
| M-C-d | shell-kill-word | |
| M-C-t | shell-transpose-words | |

Arrow and editing keys (RL/readline.c:1385-1440 and RL/terminal.c:763-775). These are hardwired whether or not termcap knows them, in the emacs, vi-move and vi-insert maps:

| Sequence | Command |
|---|---|
| `ESC[A` / `ESC[B` | previous-history / next-history |
| `ESC[C` / `ESC[D` | forward-char / backward-char |
| `ESC[H` / `ESC[F` | beginning-of-line / end-of-line |
| `ESCOA`..`ESCOD`, `ESCOH`, `ESCOF` | same as the CSI forms |
| `ESC[1;5C` / `ESC[1;5D` (Ctrl-arrows) | forward-word / backward-word |
| `ESC[1;3C` / `ESC[1;3D` (Alt-arrows) | forward-word / backward-word |
| `ESC[3;5~` | kill-word |

From termcap: Delete = delete-char, Insert = overwrite-mode, PageUp = history-search-backward, PageDown = history-search-forward.

MinGW builds also bind the raw `\340H`/`P`/`M`/`K`/`G`/`O`/`S`/`R` scancode sequences (readline.c:1422-1440). A Win32 build therefore expects the console layer to translate VK codes into these, or better into the ANSI forms.

Bracketed paste: the prefix `ESC[200~` is bound to bracketed-paste-begin, the suffix is `ESC[201~`. Enable with `ESC[?2004h` and disable with `ESC[?2004l\r` (RL/rlprivate.h:365-372). It defaults to on, but is turned off for dumb or non-ANSI terminals (terminal.c:678, 743). Pasted text becomes the active region (highlighted). P1.

TTY special characters (RL/rltty.c:883-896): when `bind-tty-special-chars` is on (the default), the stty erase character maps to backward-delete-char, kill to unix-line-discard, lnext to quoted-insert, and werase to unix-word-rubout.

Windows: readline has a `paste-from-clipboard` function (RL/kill.c:864, funmap.c:121). It is unbound by default and stops at the first CR. P3.

====================================================================
3. EDITING SEMANTICS THAT AFFECT FEEL (P0/P1)
====================================================================
- Kill ring: 10 entries (RL/kill.c:59). Consecutive kill commands append or prepend to the same entry (kill.c:131), except in vi mode. yank-pop only works right after yank or yank-pop. P1.
- Undo:
  - Consecutive single-character inserts merge into one undo record until it reaches 20 characters (RL/text.c:106-111).
  - Commands build undo groups.
  - Each history line keeps its own undo list.
  - revert-line undoes everything.
  - `revert-all-at-newline` (off by default) discards all history-line edits on accept.
  - P1.
- Numeric arguments: M-digit and M--. `universal-argument` is unbound in emacs mode. C-l with an argument redraws only the current line (RL/text.c:594).
- Word definitions:
  - readline word commands (M-f, M-b, M-d) treat alphanumerics as word characters.
  - unix-word-rubout (C-w) splits on whitespace.
  - unix-filename-rubout splits on whitespace and `/`.
  - The shell-* word commands use shell tokenization.
- `yank-last-arg` (M-.) repeated walks back through history. With an argument N it inserts word N.
- `operate-and-get-next` (C-o) accepts the line, then loads the next history line.
- C-d on an empty line returns EOF. Bash prints `exit` (subject to IGNOREEOF). Otherwise C-d deletes a character.
- Incremental search (RL/isearch.c):
  - Prompt text is `(reverse-i-search)\`str': ` with a `failed ` variant; the forward form is `(i-search)` (isearch.c:174-195).
  - Typing C-r or C-s again searches for the next match. C-r or C-s with an empty search string reuses the last search string.
  - DEL removes the last search character. C-w pulls the next word from the line into the search string, C-y pulls the rest of the line, and C-v inserts a character literally.
  - C-g aborts and restores the original line.
  - ESC and C-j terminate the search without executing a command. These come from `isearch-terminators`, default `"\033\012"` (isearch.c:77). Any other bound command ends the search and then executes, so RET accepts the line.
  - ESC followed by quickly arriving input is treated as a prefix, so arrow keys end the search and move (isearch.c:530-545).
  - Matches are highlighted when the active region is enabled. `search-ignore-case` is off by default.
- Non-incremental search (M-p, M-n): reads the search string in a `:` minibuffer prompt. P2.
- `history-search-backward`/`-forward` (prefix search, bound to PgUp/PgDn) and `history-substring-search-*` are unbound by default but very common in inputrc files. P1.

====================================================================
4. VI MODE ESSENTIALS (P1 basic, P2 complete)
====================================================================
Sources: RL/vi_keymap.c, RL/vi_mode.c, rluser.texi:2111.

- Lines start in insert mode. ESC switches to command mode and moves the cursor back one character.
- `set -o vi` and `set -o emacs` switch modes. In vi-move, C-e (emacs-editing-mode) is unbound by bash (bashline.c:548).
- Insert map: C-d is vi-eof-maybe, C-h/DEL rubout, C-i complete, C-n/C-p menu-complete and menu-complete-backward, C-r/C-s isearch, C-t, C-u, C-v, C-w (vi-unix-word-rubout), C-y, C-_ undo, and ESC enters vi-movement-mode.
- Movement map:
  - Motion: h l SPC 0 ^ $ | w W b B e E f F t T ; , %
  - History: j k + - G, and / ? n N for search
  - Insert entry: i I a A
  - Operators: c C d D y Y, plus x X s S r R ~ p P u U . m \` # &
  - Completion: * = \ (bash rebinds these to bash-vi-complete, bashline.c:609-612)
  - `v` runs vi-edit-and-execute-command and `@` runs posix-edit-macros (bash)
  - Counts 1-9, `_` (vi-yank-arg), C-l, C-r, C-s, C-_
- `.` repeats the last text-modifying command (vi_mode.c:125 lists them: `_*\AaIiCcDdPpYyRrSsXx~`).
- POSIX behavior: no bell when completion is ambiguous in vi mode (complete.c:2203).

====================================================================
5. INPUTRC FORMAT (P1)
====================================================================
Source: rluser.texi:367-1182; parser in RL/bind.c.

- File lookup: `$INPUTRC` (a bash shell variable), then `~/.inputrc`, then `/etc/inputrc`. C-x C-r re-reads it.
- Comments start with `#`. Directives start with `$`. Variables use `set var value`. Names and values are case-insensitive, and unknown variables are ignored. A boolean is on if the value is empty, `on`, or `1`; anything else is off.
- Bindings:
  - `Keyname: function-or-macro`. Recognized names include Control-, Meta-, DEL, ESC, ESCAPE, LFD, NEWLINE, RET, RETURN, RUBOUT, SPACE, SPC, TAB. No space is allowed before the colon.
  - `"keyseq": function-or-"macro"`. Escapes: `\C-`, `\M-`, `\e`, `\\`, `\"`, `\'`, `\a \b \d \f \n \r \t \v`, `\nnn` (octal), `\xHH`.
  - A quoted right-hand side is a macro.
  - `\M-` becomes ESC-prefixed or sets the 8th bit, depending on `force-meta-prefix`/`convert-meta` (rluser.texi:674).
- Conditionals:
  - `$if mode=emacs|vi`
  - `$if term=xterm` (matches the full name or the part before `-`)
  - `$if version >= 7.0` (operators `= == != <= >= < >`)
  - `$if Bash` (application name)
  - `$if varname == value` (operators `=`, `==`, `!=`)
  - `$else`, `$endif`, `$include file`

Variables. The table is at RL/bind.c:1874-2008; documented defaults are at rluser.texi:454-922 and DOC/bash.1:6853-7290. Settings people commonly use:

| Variable | Default | Tier |
|---|---|---|
| bell-style | audible (none, visible, audible) | P0 |
| editing-mode | emacs | P0 |
| enable-bracketed-paste | On | P1 |
| enable-active-region | On (start/end colors default to termcap so/se) | P2 |
| completion-ignore-case | off | P0, very common on Windows |
| completion-map-case | off | |
| show-all-if-ambiguous | off | P0 |
| show-all-if-unmodified | off | |
| completion-query-items | 100 | P0 |
| page-completions | on | P1 |
| completion-display-width | -1 | |
| completion-prefix-display-length | 0 | |
| print-completions-horizontally | off | |
| mark-directories | on | P0 |
| mark-symlinked-directories | off | |
| match-hidden-files | on | |
| visible-stats | off | |
| colored-stats | off (uses LS_COLORS) | P2 |
| colored-completion-prefix | off | P2 |
| menu-complete-display-prefix | off | |
| skip-completed-text | off | P2 |
| expand-tilde | off | |
| disable-completion | off | |
| horizontal-scroll-mode | off (forced on when the terminal height is 1) | P2 |
| history-preserve-point | off | |
| history-size | follows HISTSIZE | |
| mark-modified-lines | off (shows `*` at the start of modified history lines) | |
| revert-all-at-newline | off | |
| search-ignore-case | off | |
| isearch-terminators | ESC and C-j | |
| keyseq-timeout | 500 | |
| keymap | (current keymap) | |
| comment-begin | `#` | |
| show-mode-in-prompt | off | P2 |
| emacs-mode-string | `@` | |
| vi-ins-mode-string | `(ins)` | |
| vi-cmd-mode-string | `(cmd)` | |
| blink-matching-paren | off | |
| echo-control-characters | on | |
| bind-tty-special-chars | on | |
| convert-meta, input-meta (meta-flag), output-meta | depend on locale; with a UTF-8 locale they are effectively convert-meta off, input-meta on, output-meta on | P1: just support UTF-8 |
| enable-keypad, enable-meta-key, force-meta-prefix, byte-oriented | | P3 |

MSYS2 and Git-for-Windows ship an `/etc/inputrc` with their own settings: meta and 8-bit options, Ctrl-arrow bindings, and probably completion-ignore-case. That file is not in this tree, so check it against an installed copy.

====================================================================
6. bind BUILTIN (P1)
====================================================================
Source: DOC/bash.1:9088-9203.

- `bind [-m keymap] [-lpsvPSVX]`
- `bind [-m keymap] [-q func] [-u func] [-r keyseq]`
- `bind [-m keymap] -f file`
- `bind [-m keymap] -x keyseq:shell-command`
- `bind [-m keymap] keyseq:function | readline-command-line` (any single line accepted in inputrc)

Options:
- `-l` lists function names.
- `-p`/`-P` list bindings, reusable or human-readable. With trailing arguments they filter by name.
- `-s`/`-S` list macros. `-v`/`-V` list variables.
- `-q` shows which keys invoke a function. `-u` unbinds a function. `-r` removes a key sequence.
- `-x` binds a key to a shell command:
  - The separator is whitespace (the command is double-quoted and escapes are expanded) or a colon (the command is not expanded).
  - The command sees READLINE_LINE, READLINE_POINT, READLINE_MARK and READLINE_ARGUMENT; changes to the first three are copied back into the editor.
  - `-X` lists these bindings.
  - The implementation uses a separate per-keymap cmd_xmap (bashline.c:4865-4920).
  - fzf and atuin depend on `bind -x`, so it is P1.

====================================================================
7. PROMPTS (P0)
====================================================================
Source: DOC/bash.1:6360-6529; decoding in parse.y:6265 `decode_prompt_string`.

- Defaults: PS1 = `\s-\v\$ ` (config-top.h:82), PS2 = `> `, PS4 = `+ ` (variables.c:582). PS0 is unset.
- PROMPT_COMMAND can be a scalar or an array; each element runs before PS1 is shown. PS0 is shown after a line is read and before it runs. When PS4 is printed, its first character is repeated for each nesting level.
- Escapes: `\a \d \D{fmt} \e \h \H \j \l \n \r \s \t \T \@ \A \u \v \V \w \W \! \# \$ \nnn \\ \[ \]`.
  - `\w` honors PROMPT_DIRTRIM and abbreviates `~`.
  - `\!` is the history number and `\#` is the command number.
- After decoding, the prompt goes through parameter, command, and arithmetic expansion plus quote removal when `shopt promptvars` is on (the default).
- `\[` and `\]` become `\001` and `\002` (RL_PROMPT_START_IGNORE/END_IGNORE, RL/readline.h:908). The display code drops these markers and does not count the bytes between them as visible (display.c:356 `expand_prompt`).
  - A faithful clone must reproduce the miscounting that happens when users leave out `\[ \]`: everything outside the markers counts as visible.
- Multi-line prompts: everything up to the last `\n` is the prompt prefix. Only the last line is redrawn during editing, on SIGWINCH, and for isearch prompts (display.c:599-660, 3487, 3511).
- With `show-mode-in-prompt`, the mode string is placed before the last line of the prompt.

====================================================================
8. REDISPLAY: WHAT USERS NOTICE (P0)
====================================================================
- Long lines wrap across physical lines by default. Readline keeps the previous and new screen images and redraws only the differences (display.c:808 `rl_redisplay`, 1772 `update_line`).
  - It needs: cursor up (`up`), CR, clear to end of line (`ce`), cursor left and right.
  - Insert and delete-character capabilities (`ic`/`IC`/`dc`/`DC`) are optional optimizations.
- Autowrap is used only if the terminal has both `am` and `xn` (deferred wrap), see terminal.c:706. Otherwise the usable width is one less than the screen width.
  - On the legacy XP console (no deferred wrap) use width-1.
  - On Windows 10+ VT mode, `DISABLE_NEWLINE_AUTO_RETURN` gives xterm-style deferred wrap.
  - When the cursor sits exactly at the right margin, readline forces the wrap by writing the next character (display.c:1785-1800).
- Control characters display as `^X` and meta bytes as `M-x` when output-meta is off (display.c:3077-3130). TAB expands to the next multiple of 8. Multibyte and wide characters use wcwidth.
- Horizontal-scroll mode: one physical line, `<` and `>` markers at the edges, and the cursor kept about 2/3 of the way across (display.c:1600-1655).
- C-l clears the screen and redraws the full prompt and line. M-C-l also clears scrollback (`E3`).
- SIGWINCH: the new size is fetched, the visible line is cleared, the cursor moves to the top line, and only the last prompt line is redrawn (display.c:3511).
  - Bash's `checkwinsize` updates LINES and COLUMNS after each command.
  - `rl_change_environment` exports LINES and COLUMNS (terminal.c:90, 373).
- When a line is accepted, the cursor moves past the last line and a newline is written (`_rl_update_final`, display.c:3431).
- C-c: the tty echoes `^C` (`echo-control-characters`), the line is discarded, a newline is written, and a fresh prompt appears. On Windows, read 0x03 as a key while editing rather than relying on the CTRL_C event.

====================================================================
9. COMPLETION (P0 default, P1 programmable)
====================================================================
TAB logic (RL/complete.c:458-476, 2070-2240):
- TAB right after a TAB that did not change the buffer lists the matches.
- Otherwise, with `show-all-if-ambiguous`, insert the longest common prefix and list if there are several matches. With `show-all-if-unmodified`, list only if the prefix added nothing. Otherwise insert the longest common prefix and ring the bell when ambiguous.
- No matches: ring the bell.
- After a single unique match (`append_to_match`, complete.c:1924-1988):
  - A closing quote is added if the user opened one.
  - A directory gets `/`, unless a `/` already follows.
  - A symlink to a directory gets nothing, unless mark-symlinked-directories is on.
  - Anything else gets a space, but only when the cursor is at the end of the line.
  - Bash sets the append character to `/` for `$VAR` completions that expand to a directory (bashline.c:1849-1856).

Listing (complete.c:1594-1817):
- Move below the last line of the input.
- If the match count is at least completion-query-items (100), print `Display all N possibilities? (y or n)`. y, Y or SPC means yes; n, N or DEL means no; C-g aborts; any other key rings the bell (complete.c:586-616).
- Column width is the longest display name + 2. Columns = width / colwidth, minus one if the fit is exact. Width comes from completion-display-width, then `$COLUMNS`, then the screen width (complete.c:1573).
- Matches are sorted and filled down the columns like `ls`, unless print-completions-horizontally is on.
- Only the part after the last `/` is shown for filenames (`printable_part`, complete.c:773).
- With page-completions, `--More--` appears when the printed lines reach screen height - 1. SPC or y shows the next page, RET one more line, and q, Q, n or DEL stops (complete.c:619-633).
- Afterwards the prompt and line are redrawn below the list.
- visible-stats suffixes: `@` symlink, `/` directory, `*` executable, `=` socket, `|` FIFO, `%` character device, `#` block device (complete.c:644-653).

Default bash completion order (bashline.c:1587 `attempt_shell_completion`, 1830-1880, 2005+):
1. A compspec for the command, if programmable completion is on (`progcomp`, on by default).
2. Words starting with `$` (variables) or `$(`/backquote (command substitution).
3. Words starting with `~` and containing no `/`: usernames.
4. Words starting with `@`: hostnames from HOSTFILE or /etc/hosts (`hostcomplete`, on by default).
5. Command position: aliases, reserved words, functions, builtins, `$PATH` executables, then files in the current directory that are executable or directories (`executable_or_directory`, bashline.c:1995).
   - On Windows, decide how PATHEXT and executable bits work.
   - `no_empty_cmd_completion` affects empty command words.
6. Filename completion as the fallback. FIGNORE filters it (`force_fignore`, on by default).

Word breaking and quoting:
- Word breaks come from COMP_WORDBREAKS, default `" \t\n\"'@><=;|&(:"` (bashline.c:313). The `:` explains the well-known `scp host:file` quirks.
- Quote characters are `'` and `"`.
- Characters that need quoting in filenames: `" \t\n\\\"'@<>=;|&()#$\`?*[!:{~"` (bashline.c:317).
- Default quoting style is backslash. If the user opened `'` or `"`, that style continues and the quote is closed on a unique match. A name containing a newline switches to single quotes (bashline.c:4394-4482).
- With `direxpand` off, `$var/` completion is left unexpanded.

Other completion commands:
- `menu-complete` and `menu-complete-backward` are unbound in emacs mode but commonly bound to TAB. They cycle through matches, ring the bell at the end, then restore the original text. P1.
- insert-completions (M-*), possible-completions (M-?/M-=), delete-char-or-list, dabbrev-expand, and glob-*-word are P2.

Programmable completion (DOC/bash.1:8225-8467, 9397-9560; rluser.texi:2141-2663):
- `complete [-abcdefgjksuv] [-o opt] [-DEI] [-A action] [-G glob] [-W words] [-F func] [-C cmd] [-X filter] [-P prefix] [-S suffix] name...`
- `complete -pr [-DEI] [name...]`
- `-o` options: bashdefault, default, dirnames, filenames, fullquote, noquote, nosort, nospace, plusdirs.
- `-A` actions: alias, arrayvar, binding, builtin, command, directory, disabled, enabled, export, file, function, group, helptopic, hostname, job, keyword, running, service, setopt, shopt, signal, stopped, user, variable.
- `compgen [-V varname] [opts] [word]` (`-V` is new in 5.3).
- `compopt [-o|+o opt] [-DEI] [name]`.
- Generation order:
  1. Actions, then the `-G` glob, then `-W` words (IFS-split with quoting honored, then expanded).
  2. `-F` function: receives `$1` command, `$2` word, `$3` previous word, plus COMP_WORDS, COMP_CWORD, COMP_LINE, COMP_POINT, COMP_KEY and COMP_TYPE. Results go in COMPREPLY.
  3. `-C` command.
  4. `-X` filter (`&` stands for the word; a leading `!` negates it).
  5. `-P`/`-S` prefix and suffix.
  6. Fallbacks: dirnames, plusdirs, bashdefault, default.
- Compspec lookup: the full path, then the basename, then `-D` (default), then the alias-expanded command word. `-E` covers an empty line and `-I` the initial command word.
- If the function returns 124, the compspec lookup is retried (this is how lazy loading works).
- bash-completion compatibility needs:
  - `complete -D -F _comp_complete_load` with return 124.
  - compopt inside functions.
  - COMP_WORDBREAKS handling.
  - compgen honoring quoting.
  - Correct COMP_WORDS splitting, which uses COMP_WORDBREAKS.
  - The language features bash-completion relies on (extglob, arrays, `[[`, `printf -v`, `declare -F`, `${var@Q}`).
  - This is P1 for drop-in status, since many distros source it.

====================================================================
10. HISTORY (P0 list/file, P1 builtins and expansion details)
====================================================================
Sources: DOC/bash.1:8468-8947 and 10536-10655; RL/histexpand.c; RL/histfile.c.

Variables:
- HISTFILE defaults to `~/.bash_history`. HISTSIZE defaults to 500. HISTFILESIZE defaults to HISTSIZE; when unset, empty or negative, the file is not truncated.
- HISTCONTROL accepts ignorespace, ignoredups, ignoreboth and erasedups (colon-separated).
- HISTIGNORE is a colon-separated list of anchored glob patterns. `&` stands for the previous line. extglob is honored. Continuation lines follow whatever was decided for the first line.
- HISTTIMEFORMAT: if set, timestamps are written as `#<epoch>` lines before entries. Those lines also delimit multi-line entries when the file is read back.
- histchars defaults to `!^#`. HISTCMD is the current history number.

Shell options:
- `histappend`: append to HISTFILE on exit instead of overwriting.
- `cmdhist` (on by default, bashhist.c:141): save multi-line commands as one entry joined with `;`.
- `lithist`: keep embedded newlines instead.
- `histverify`: put the expanded line back in the editor instead of running it.
- `histreedit`: put a failed expansion back in the editor.
- `set -o history` and `set -H`/`+H` (expansion is on for interactive shells).

`history` builtin:
- `history [n]`: modified entries are marked `*`. With HISTTIMEFORMAT, no space is printed after the timestamp.
- `-c`, `-d offset`, `-d start-end` (negative offsets count from the end, and -1 is the `history -d` command itself), `-a`, `-n`, `-r`, `-w [file]`, `-p args`, `-s args` (replaces the last entry).

`fc`:
- `fc [-e ename] [-lnr] [first] [last]` and `fc -s [pat=rep] [cmd]`.
- Editor is `${FCEDIT:-${EDITOR:-vi}}`, or `${FCEDIT:-ed}` in POSIX mode (builtins/fc.def:184-188).

History expansion:
- Runs on each complete line before tokenizing. Only backslash and single quotes quote `!`; a `!` just before a closing `"` is also left alone.
- `!` does not start an expansion when followed by space, tab, newline, CR, `=`, or `(` with extglob, or another metacharacter.
- Events: `!n`, `!-n`, `!!`, `!str`, `!?str[?]`, `^old^new^`, `!#`.
- Word designators follow `:`, which may be left out before `^ $ * - %`. Forms: `0`, `n`, `^`, `$`, `%`, `x-y`, `-y`, `*`, `x*`, `x-`. A designator with no event means `!!`.
- Modifiers: `:h :t :r :e :p :q :x :s/old/new/ :& :g`/`:a` and `:G`.
  - Any delimiter can replace `/`, and the final delimiter is optional at end of line.
  - In new text, `&` stands for old. An empty old means the last old string or the last `?str` search.
- `:p` prints the result without running it. The expanded line is echoed before it runs.
- Readline history commands: `history-preserve-point`, `fetch-history`, `operate-and-get-next`, `mark-modified-lines`.

====================================================================
11. TERMINAL ASSUMPTIONS AND DESIGN NOTES
====================================================================
- Readline reads termcap through tgetent and tgetstr (terminal.c:454-492). Capabilities used:

| Capability | Purpose |
|---|---|
| ce, cl, cr, up, le, nd | clear to end of line, clear screen, CR, cursor up, left, right |
| ho | home |
| ic, im, ei, IC, dc, DC | insert and delete characters (optional) |
| so, se | active region highlight |
| vb | visible bell |
| ks, ke | keypad mode |
| mm, mo | meta mode |
| kh, @7, kD, kI, kN, kP, ku, kd, kl, kr | Home, End, Delete, Insert, PgDn, PgUp, arrows |
| BE, BD, PE, PS | bracketed paste enable/disable and paste markers |
| vs, ve | cursor shape for overwrite mode |
| E3 | clear scrollback |
| co, li | columns and lines |
| am, xn, km | autowrap, deferred wrap, has meta |

- With TERM=dumb or unknown, all of these are cleared and bracketed paste and the active region are turned off (terminal.c:655-690).
- Suggested approach for our build:
  - Hardcode a VT100/xterm subset: `\r`, `ESC[nA/B/C/D`, `ESC[K`, `ESC[H ESC[2J`, `ESC[3J`, `ESC[7m`/`ESC[27m`, `ESC[?2004h/l`. Fall back to dumb mode when TERM=dumb or output is not a terminal.
  - Windows 10+: enable `ENABLE_VIRTUAL_TERMINAL_PROCESSING`. Optionally also `ENABLE_VIRTUAL_TERMINAL_INPUT`, which makes the console send xterm key sequences including bracketed paste.
  - XP and legacy console: emulate the small set of operations directly with SetConsoleCursorPosition, FillConsoleOutputCharacter, GetConsoleScreenBufferInfo and ScrollConsoleScreenBuffer. Use width-1 because there is no deferred wrap. Read input with ReadConsoleInputW and turn KEY_EVENT records (VK_*, Alt as ESC prefix, Ctrl+arrows) into the ANSI sequences above, so inputrc bindings behave the same.
  - Use WINDOW_BUFFER_SIZE_EVENT in place of SIGWINCH. Readline's own Win32 code sizes the screen from `dwSize.X` and the window height (terminal.c:245-262).
- The virtual OS layer should model a terminal (screen size, input byte stream, output capture), so redisplay and completion can be tested and fuzzed against golden screen images without touching a real console, files, or processes.

Tier summary:
- P0: section 1, the core emacs keys and arrows in section 2, the basics of section 3, prompts (section 7), wrapping and redisplay (section 8), default TAB completion and listing (section 9), and the history list/file plus basic `!` expansion (section 10).
- P1: kill ring, undo, isearch, M-., C-o, inputrc with the common variables and `$if`, `bind` including `-x`, bracketed paste, complete/compgen/compopt and bash-completion, the history and fc builtins, HISTIGNORE, histappend, histverify, menu-complete, basic vi mode.
- P2: full vi mode, keyboard macros, colors (LS_COLORS), active region, show-mode-in-prompt, horizontal-scroll-mode, glob/dabbrev/spell commands, edit-and-execute-command details.
- P3: the meta and keypad variables, execute-named-command, export-completions, dump-*, tty-status, paste-from-clipboard.
