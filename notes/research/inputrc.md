# Readline 8.3 inputrc files (as bash 5.3 uses them): semantics digest

Purpose: enough detail to reimplement readline's init-file reader in
`edit.cpp` and match bash 5.3's observable behaviour, error messages
included, without reading the GPL source. Everything below is our own
paraphrase. There is no transliterated code. The only verbatim text is short
user-visible strings: the error messages, variable and function names, and
key names. This file replaces readline.md §5. emacs.md and completion.md
describe what the bound commands and the completion variables do once they
are set.

Citations are relative to the `bash-5.3/` root of `bash-5.3.tar.gz`:

- RL = `lib/readline/`. The files used are bind.c (nearly all of this
  file), readline.c, terminal.c, rltty.c, nls.c, funmap.c, parens.c,
  complete.c, util.c, chardefs.h, rlconf.h and readline.h.
- BL = `bashline.c`.
- Not available here, so described from the oracle only: `parse.y`
  (where bash first calls `initialize_readline`), `builtins/read.def`,
  `builtins/bind.def`, `variables.c`, and the `sh_get_env_value` hook that
  bash supplies.

Claims marked **[oracle]** were checked against real bash 5.3.0 (the
`unabashed/bash53` container, `tools/edrecord`, TERM=xterm, LANG=C.UTF-8,
HOME=/home/oracle, `--noprofile --rcfile ~/.edrc`, no /etc/inputrc in the
image). Claims marked **[git]** were checked against Git for Windows'
bash 5.3.15 (MSYS) on this machine, through `bind -f` and `bind -v` in
`bash -c`.

---------------------------------------------------------------------------

## 0. Model

The reader is a line-at-a-time interpreter that writes into global state. It
has no transaction and no reset:

- **Keymaps.** The emacs standard, meta (ESC) and C-x maps, and three vi
  maps. Each has 257 slots: 256 byte values plus one "any other key" slot
  that holds a shadowed binding (§4.6). A slot is a function, a macro or a
  nested keymap.
- **The current binding keymap** (`_rl_keymap`). Key bindings go into it.
  `set keymap` and `set editing-mode` change it.
- **Variables.** 36 booleans and 15 string or integer variables (§8).
- **The conditional state.** A stack of "parsing is off" flags, and the
  current flag. Both are global and survive across files (§5.6).
- **The current file name, line number and include level**, used in error
  messages (§6).
- **`last_readline_init_file`.** The top-level file that was last read
  successfully. C-x C-r reads it again (§1.3).

Every line is either skipped (blank or comment), a directive (`$...`), a
`set`, or a key binding. Directives are always looked at. Everything else is
ignored while a conditional has turned parsing off. Errors print one line to
stderr, and parsing continues with the next line (§6).

---------------------------------------------------------------------------

## 1. When and which file is read

### 1.1 Lookup (RL/bind.c:1042-1064, RL/rlconf.h:43-46)

When no file name is given (startup, and C-x C-r), the first rule that
applies chooses the file:

1. The file last read successfully at the top level, if there was one
   (`last_readline_init_file`).
2. Otherwise `INPUTRC`, looked up with the host's environment hook. In
   bash that hook is the **shell variable**, so an unexported `INPUTRC=...`
   in .bashrc counts **[oracle]**: `INPUTRC=~/ira` in the rc file, then
   `export -n INPUTRC` (`declare -- INPUTRC=...`), and ~/ira was read. A
   non-empty INPUTRC is final. If that file cannot be read, nothing is read,
   and there is no fallback to ~/.inputrc **[oracle]**: with INPUTRC naming
   a missing file and a ~/.inputrc present, ~/.inputrc's C-t binding was
   not in effect.
3. An unset or empty INPUTRC means `~/.inputrc`. Only if that file cannot
   be opened, stat'ed or read is `/etc/inputrc` tried. When ~/.inputrc
   exists, /etc/inputrc is never read unless ~/.inputrc includes it. An
   empty ~/.inputrc counts as read: `INPUTRC=/dev/null` disabled ~/.inputrc
   **[oracle]**.

msys mode reads the install's own `etc/inputrc` as the system file. This
was confirmed with Git for Windows **[git]**: with no ~/.inputrc,
`bind -v` showed `completion-ignore-case on`, `bell-style visible` and
`completion-query-items 40`, all from its /etc/inputrc (§9).

The name is tilde-expanded only for opening (RL/bind.c:1076). Messages
show the name as it was given: `~/.inputrc`, or the INPUTRC value, which
bash had already tilde-expanded at assignment (`/home/oracle/ira`)
**[oracle]**.

Reading (RL/bind.c:963-1021) loads the whole file into memory: open
(retried once on EINTR), fstat, then one read. Any failure means "could
not read" and returns errno silently. A missing file never prints anything,
at the top level or through `$include`. A directory fails at the read step,
so it falls through to /etc/inputrc like a missing file. There are no size
limits other than memory.

### 1.2 When (RL/readline.c:1202-1357, BL:451-647)

The file is read inside readline's one-time initialisation. That happens on
the first `rl_initialize`, which bash's `initialize_readline` calls
(BL:523-524). Bash calls that function lazily the first time it needs
readline: the first interactive prompt, the first `read -e`, or `bind`.
That has three consequences:

- **The rc file runs first.** Anything .bashrc does before the first prompt
  counts: setting INPUTRC, TERM or HOME, or writing ~/.inputrc **[oracle]**.
  Every t/edit scenario relies on this: an rc that writes ~/.inputrc is
  enough, with no `bind` needed. (A `bind` in the rc initialises readline
  at that point, so later rc lines would be too late.)
- Error messages appear **before the first prompt**. A non-interactive
  `bash -c 'read -e ...'` reads ~/.inputrc too, and prints its errors after
  the command line and before the `read` prompt **[oracle]**.
- The order inside initialisation matters for which bindings win
  (RL/readline.c:1310-1343, BL:466-610):
  1. Bash registers its own command names (`shell-expand-line`,
     `complete-filename`, `dabbrev-expand`, ... BL:466-517). An inputrc can
     therefore name them, and `$if Bash` works because `rl_readline_name`
     is set to "Bash" first (BL:464). The terminal name is bash's TERM
     *shell* variable (BL:459).
  2. The terminal is initialised. Termcap's arrow, Home, End, Delete,
     Insert, PgUp and PgDn keys are bound **if unbound**
     (RL/terminal.c:729-735, 755-775). A non-ANSI terminal turns
     enable-bracketed-paste and enable-active-region off, and the region
     colours are taken from termcap so/se (RL/terminal.c:741-748).
  3. The tty's special characters are bound (§4.8).
  4. The locale sets input-meta, output-meta and convert-meta (§4.4).
  5. **The init file is read.**
  6. If horizontal-scroll-mode is on and the terminal autowraps, the usable
     width shrinks by one (once, at startup only).
  7. The keymap is reset from the editing mode, which undoes any trailing
     `set keymap` (RL/readline.c:1336).
  8. The fixed arrow-key sequences (`\e[A`..`\e[D`, `\e[H`, `\e[F`, the
     `\eO` forms, `\e[1;5C`/`D`, `\e[3;5~`, `\e[1;3C`/`D`) and the
     bracketed-paste prefix are bound **if unbound** (RL/readline.c:1339-1343,
     1385-1481). An inputrc binding of `"\e[A"` therefore wins **[oracle]**.
  9. LS_COLORS is parsed if colored-stats or colored-completion-prefix is
     on. This happens at startup only, so turning those on later with
     C-x C-r finds no colours parsed.
  10. Back in bash, its extra keys (M-C-e, M-^, C-x C-v, M-{, M-/, C-x /,
      M-~, M-@, M-$, M-!, M-g, C-x *, C-x g, C-x C-e, M-C-b/d/f/t, C-x s)
      are bound **if unbound** (BL:527-640). An inputrc binding of any of
      them wins. One exception: if M-TAB is unbound or `tab-insert` after
      the inputrc, bash binds it to `dynamic-complete-history`. Likewise
      M-~ goes to `complete-username` when it is unbound or still
      `tilde-expand`.

Each new bash process reads the file again. Nothing in bash watches INPUTRC
after startup.

### 1.3 C-x C-r, re-read-init-file (RL/bind.c:1025-1031)

C-x C-r is bound in the C-x map (RL/emacs_keymap.c:616). It runs the lookup
of §1.1 again and then resets the binding keymap from the editing mode.

- **It reads the last file read successfully, not INPUTRC.** After
  `INPUTRC=~/irb`, C-x C-r read the old ~/ira again **[oracle]**. INPUTRC is
  consulted only if no file was ever read: with INPUTRC naming a missing
  file at startup, `INPUTRC=~/ir2` and then C-x C-r read ~/ir2 **[oracle]**.
  An included file never becomes the remembered one.
- **Nothing is reset first.** No default bindings are restored and no
  variables are reset. Lines simply run again on top of the current state.
  A setting the new contents no longer mention keeps its value **[oracle]**:
  completion-ignore-case stayed on after re-reading a file that mentioned
  only mark-directories.
- The conditional stack is not reset either (§5.6).
- Step 6 of §1.2 (horizontal scroll), the LS_COLORS parse, the arrow-key
  defaults and bash's own bindings are not redone.
- **Display.** Error messages are written at the cursor, right after the
  prompt and line text, without a newline before them. The line is not
  redrawn afterwards. The cursor is left at column 0 of a fresh row while
  readline still believes the old layout. In the oracle, `abc C-t Y` then
  drew `Yabc` at column 0 with no prompt **[oracle]**. A C-x C-r that reads
  a file with no errors writes nothing. Scenarios should use only error-free
  re-reads, or follow them with C-l.
- In emacs.md's terms, the command is an ordinary command: it sets
  `last_func` and returns 0.

### 1.4 read -e

`read -e` uses the same readline state as the shell. If it is the first use
of readline in the process, it triggers the initialisation and reads the
file then **[oracle]** (§1.2). `$if Bash` holds there too.

---------------------------------------------------------------------------

## 2. Line syntax (RL/bind.c:1092-1125, 1594-1615)

- **Lines** end at LF. The last line needs no newline. **CR is not
  stripped on Linux**: the CR stays part of the line **[oracle]**. The
  results:
  - `set show-all-if-ambiguous on␍` turns the variable off, because `on␍`
    is not `on`.
  - `set comment-begin XX␍` made M-# insert `XX^M`.
  - `"\C-t": beginning-of-line␍` names an unknown function, so C-t rang the
    bell.

  A build for Cygwin, which includes MSYS, removes one CR before the LF
  (RL/bind.c:1102-1106). Git's bash read a CRLF file correctly **[git]**. Our
  msys mode should strip it and our POSIX build should not.
- A NUL byte ends the line early, because the rest of the processing works
  on C strings.
- **Leading whitespace** (space and tab only) is skipped. It is skipped
  again inside the parser, and also after the `$` of a directive:
  `  $ if Bash` and `$ENDIF` both work **[oracle]**.
- **Blank lines and comments.** A line whose first non-blank character is
  `#` is a comment. `#` anywhere else is not syntax. Trailing comments work
  only where the parser happens to stop reading at whitespace:
  - after a function name (`"\e[5~": beginning-of-history   # Page up`);
  - after a boolean `set` value;
  - after a `$if term=`, `mode=` or application name, which use only their
    first word;
  - after a `$if version` test, which explicitly allows `#`;
  - on `$else` and `$endif`, which ignore their arguments.

  Comments **break** the value of a string `set` (§3.3: `set bell-style
  none # x` is an error **[oracle]**), a `$if var == value` test (only the
  value's first word is used, so that one is fine), and **`$include`** (§5.5:
  the comment becomes part of the file name, which silently fails
  **[oracle]**).
- **No continuation lines.** A backslash at the end of a line has no
  meaning.
- **No length limits.**
- **Classification** (RL/bind.c:1600-1636). After leading blanks:
  - `$` makes the line a directive (§5). Directives are handled even inside
    a false conditional.
  - Otherwise, if a conditional has parsing off, the line is ignored with no
    checking at all.
  - A line starting with `"` is scanned to its closing quote (a backslash
    passes the next character). If there is no closing quote, the error is
    `%s: no closing `"' in key binding`, where the whole line is the `%s`
    **[oracle]**.
  - Then the scan runs to the first `:`, space or tab. That word is the
    "key part". If the line starts with `:` (the key part is empty), the
    error is ``%s': invalid key binding: missing key sequence``
    **[oracle]**: ``readline: ~/.inputrc: line 16: `:x': invalid key
    binding: missing key sequence``.
  - `:=` right after the key part makes an "equivalency" line. It is parsed
    and then ignored, with no error (`a:=b` printed nothing **[oracle]**).
  - If the key part is `set`, compared case-insensitively, the line is a
    variable assignment (§3). The separator may be a space, a tab or even a
    colon (`set:name value`).
  - Otherwise it is a key binding (§4). The separator may be `:`, a space
    or a tab. `Control-y beginning-of-line`, with no colon, binds C-y
    **[oracle]**. A line with no separator at all is the error
    `%s: no key sequence terminator`, which names the whole line: `nokey`,
    or `"\C-t"` for a quoted sequence with nothing after it **[oracle]**.

---------------------------------------------------------------------------

## 3. `set NAME VALUE` (RL/bind.c:1656-1711, 2033-2361)

### 3.1 Splitting

- NAME is the next blank-delimited word, and VALUE starts at the next
  non-blank character. Both may be empty.
- Variable names are matched case-insensitively against both tables (§8).
  `SET Bell-Style None` works.
- **Unknown name**: the error `%s: unknown variable name`, and the line is
  dropped. `set` alone gives `readline: ~/.inputrc: line 17: : unknown
  variable name` **[oracle]**.
- **Boolean variable**: VALUE is cut at its first blank, so the rest of the
  line is ignored.
- **String or integer variable**:
  - If VALUE starts with `"`, it runs to the next unescaped `"` (a
    backslash passes the next character, and the backslash is kept).
    Anything after the closing quote is ignored. A missing close quote
    takes the rest of the line. `set comment-begin "# x" trailing` made M-#
    insert `# x` **[oracle]**.
  - Otherwise VALUE is the rest of the line with trailing spaces and tabs
    removed. Inner blanks and `#` are kept: `set bell-style none # comment`
    gives ``bell-style: could not set value to `none # comment'``
    **[oracle]**.
- The value is then passed to the variable's setter. A setter that rejects
  the value gives `%s: could not set value to `%s'`, with NAME as written
  in the file (RL/bind.c:2085-2087). The variable keeps its old value.

### 3.2 Booleans (RL/bind.c:2033-2038)

On means: the value is empty, or `on` in any case, or exactly `1`.
Everything else means off, with no error. That includes `yes`, `true`,
`2`, `off`, `0`, `on␍` and `onx`. **[oracle]**: empty, `1` and `ON` turned
completion-ignore-case on, and `yes` and `0` turned it off.

Five booleans have side effects (RL/bind.c:1938-1957):

- **blink-matching-paren** rebinds `)`, `]` and `}` in the emacs standard
  and vi-insert maps. On means `insert-close` and off means `self-insert`
  (RL/parens.c:64-90). Any inputrc binding of those keys made earlier in
  the file is overwritten.
- **prefer-visible-bell** sets bell-style: on means visible and off means
  audible. Its own reported value is a separate flag that starts on.
  `bind -v` shows `prefer-visible-bell on` next to `bell-style audible` by
  default **[oracle]**.
- **show-mode-in-prompt** re-expands the prompt.
- **enable-bracketed-paste** also sets **enable-active-region** to the same
  value.
- Two variables share one flag: **meta-flag** is another name for
  input-meta.

### 3.3 String and integer variables (RL/bind.c:2092-2361)

"atoi" below means: optional blanks, an optional sign, then leading digits;
anything after them is ignored, and no digits gives 0. None of the integer
setters ever fails. `set completion-query-items abc` is silent and gives 0
**[oracle]**.

| variable | parsing | on empty value | error when |
|---|---|---|---|
| active-region-start-color, active-region-end-color | key-sequence escapes (§4.3), stored as bytes | colour removed (no highlight escape) | never |
| bell-style | `none` or `off` means none; `audible` or `on` means audible; `visible` means visible; case-insensitive | audible | anything else (``could not set value to `loud'`` **[oracle]**) |
| comment-begin | stored as is | error | the value is empty (``comment-begin: could not set value to `'`` **[oracle]**) |
| completion-display-width | atoi; negative values are kept (any negative means the screen width) | -1 | never |
| completion-prefix-display-length | atoi; negative becomes 0 | 0 | never |
| completion-query-items | atoi; negative becomes 0 | 100 | never |
| editing-mode | **prefix** match, case-insensitive: anything starting `vi` means vi, anything starting `emacs` means emacs; also sets the binding keymap to vi-insert or emacs-standard | error | no match (``editing-mode: could not set value to `foo'`` **[oracle]**) |
| emacs-mode-string, vi-ins-mode-string, vi-cmd-mode-string | key-sequence escapes; `\1` and `\2` give the prompt's invisible-start and invisible-end markers | the **empty string** (not the default) | never |
| history-size | atoi; negative unstifles (no limit); otherwise the history list is cut to that many entries | 500 | never |
| isearch-terminators | see below | empty set | never |
| keymap | a keymap name (§4.7), case-insensitive; changes the binding keymap | error | unknown name (``keymap: could not set value to `foo'`` **[oracle]**) |
| keyseq-timeout | atoi in milliseconds; negative becomes 0, and 0 waits forever for the rest of a sequence | **0** | never |

isearch-terminators gets the value after §3.1's quote handling. If what is
left still starts with `"` or `'`, it runs to the matching quote.
Otherwise it runs to the first blank. That text goes through the
key-sequence escapes. So `set isearch-terminators "a b"` keeps only `a`.
The default (unset) means ESC and C-J (emacs.md).

**history-size and HISTSIZE are separate.** The inputrc limits readline's
history list, and bash's HISTSIZE variable does not change. **[oracle]**:
with `set history-size 3`, `history` listed only 3 entries while
`echo $HISTSIZE` printed 500. In an interactive shell the value is 500 at
inputrc time: `$if history-size == 500` was true **[oracle]**, because bash
had already applied HISTSIZE. In a non-interactive `bash -c`, `bind -v`
shows -1.

---------------------------------------------------------------------------

## 4. Key bindings

### 4.1 The right-hand side (RL/bind.c:1713-1756)

After the separator, blanks are skipped. Then:

- If the right-hand side starts with `"` or `'`, it is a **macro**. It runs
  to the matching quote (backslash passes the next character). If there is
  no closing quote, the error is ``%s': missing closing quote for macro``,
  where `%s` starts at the opening quote: ``readline: ~/.inputrc: line 18:
  `"macro': missing closing quote for macro`` **[oracle]**.
- In both cases the text then continues up to the next blank and is cut
  there. A function name therefore ends at the first blank, and trailing
  words or comments are ignored. A macro keeps any characters glued after
  its closing quote. `"\C-]": "a b"c` made C-] insert `a b"c` **[oracle]**,
  because only a final quote is removed.

Function names are matched case-insensitively against the funmap
(RL/bind.c:778-788). `ctrl-r: Forward-Char` bound C-r to forward-char
**[oracle]**.

**An unknown or empty function name is not an error.** It binds the key to
"nothing" with no message. Typing that key then runs readline's abort: a
bell, and the key sequence and argument are discarded. **[oracle]**:
`"\C-o": no-such-function` made C-o ring. Three common accidents produce
this:

- `C-o : end-of-line` (a blank before the colon): the key part ends at the
  blank, and the function name becomes `:` **[oracle]**.
- `"\C-t":` with nothing after it.
- A trailing CR on Linux (§2).

This is also the only way an inputrc can unbind a key.

**For a macro, which comes in a later milestone:** the bound value is the
text between the quotes, put through the key-sequence escapes (§4.3) when
it is bound. When the key is typed, that text is fed back as input. Until
macros exist, our reader must still parse the line (so that the quote scan
and the errors match) and either skip it or bind the key to a placeholder.
Skipping it leaves the old binding in place where bash would replace it. A
placeholder that does nothing differs only when that key is typed.

### 4.2 `Keyname: function` (RL/bind.c:1804-1858, 2363-2396, 3087-3096)

This form binds **exactly one key**, in the current binding keymap.

1. The base key is the text after the **last** `-` in the key part, or the
   whole key part if it has no `-`.
2. The base key is looked up case-insensitively in this table. A name that
   is not in the table means **its first character**:

   | name | key |
   |---|---|
   | DEL, Rubout | 0x7F |
   | ESC, Escape | 0x1B |
   | LFD, Newline | 0x0A |
   | RET, Return | 0x0D |
   | SPC, Space | 0x20 |
   | Tab | 0x09 |

   So `Home`, `End`, `Up`, `Left` and `F1` silently bind `H`, `E`, `U`,
   `L` and `F`. **[oracle]**: after `Home: beginning-of-line`, typing `H`
   moved to the start of the line and inserted nothing.
3. Modifiers are found by **case-insensitive substring search over the
   whole key part**, not by parsing prefixes:
   - If it contains `Control-`, `C-` or `CTRL-` anywhere, the key becomes
     the control form of its upper-case letter (`key & 0x1f`). So
     `Control-?` is 0x1F, **not** DEL. Other quirks follow: `abc-x`
     contains `c-`, so it means C-x.
   - If it contains `Meta` or `M-` anywhere (note: `Meta` has no dash), the
     meta bit (0x80) is set. So `Meta` alone is M-M.
4. If the key part had a `-` but neither modifier was found, the error is
   `%s: unknown key modifier` **[oracle]**: `readline: ~/.inputrc: line
   19: foo-a: unknown key modifier`.
5. The key is bound as in §4.4. If that slot is already a nested keymap
   (`ESC: ...`, `C-x: ...`), the function goes into that keymap's
   any-other-key slot (§4.6).

### 4.3 `"keyseq": function`: the escapes (RL/bind.c:1760-1802, 516-649)

The text between the quotes is copied first, with backslash pairs left in
place, up to the first unescaped `"`. Anything between that quote and the
separator is ignored. Then the escapes are translated:

| escape | byte(s) |
|---|---|
| `\C-` | a prefix: the next character (itself possibly an escape) becomes `toupper(c) & 0x1f`, except `?`, which gives DEL (0x7F) |
| `\M-` | a prefix: sets the meta bit (0x80) on the next character, applied after `\C-`; for how that byte is then bound, see §4.4 |
| `\a` `\b` `\d` `\e` `\f` `\n` `\r` `\t` `\v` | 07, 08, **7F (\d is DEL)**, 1B, 0C, 0A, 0D, 09, 0B |
| `\\` | a backslash |
| `\nnn` | octal: the first digit 0-7, then up to two more octal digits |
| `\xHH` | one or two hex digits; `\x` with no hex digit is a plain `x` |
| a backslash before anything else | the backslash is dropped: `\"` is `"`, `\'` is `'`, `\z` is `z` |
| a backslash as the last character | a literal backslash (it cannot occur inside a closed quote) |

More details of the translation:

- `\C-` and `\M-` combine in either order (`\M-\C-f`, `\C-\M-f`).
- A `\C-` or `\M-` at the very end of the sequence applies to a NUL byte.
- `\C-a` and `\C-A` are the same key.
- **[oracle]**: `"\x14"` bound C-t, `"\017"` bound C-o, and `"\M-\C-f"`
  bound ESC C-f once force-meta-prefix was on.
- The same translation is used for macro bodies, isearch-terminators, the
  mode strings and the region colours.
- **An empty sequence** (`"": end-of-line`) is silently ignored
  (RL/bind.c:394-399) **[oracle]**: no error.

### 4.4 Meta characters: convert-meta and force-meta-prefix

A meta byte (`\M-x`, or a `Meta-` keyname) is turned into the two keys ESC
x only when force-meta-prefix is on, **or** when convert-meta is on
(RL/bind.c:105-108, 149-169, 622-634). Otherwise the byte 0x80|x is bound
directly in the current keymap.

The locale decides convert-meta before the init file is read (RL/nls.c:170-187):

- Any locale other than `C` or `POSIX` (so **C.UTF-8**, the oracle's)
  turns input-meta and output-meta on and convert-meta **off**.
- The C locale turns convert-meta on. That is also the compiled default.

force-meta-prefix starts off.

The consequence in bash 5.3 under a UTF-8 locale: **`"\M-t": ...` and
`Meta-u: ...` bind bytes 0xF4 and 0xF5, which the Alt key (ESC t) never
sends.** **[oracle]**: after both bindings, `\et` still ran transpose-words
and `\eu` still ran upcase-word. With `set force-meta-prefix on` placed
before them in the file, both bindings took effect as ESC t and ESC u
**[oracle]**.

The order matters: force-meta-prefix applies when a binding line is read,
not afterwards. Under convert-meta off, a binding on a high byte also takes
over that byte in UTF-8 input (for example 0xE1, which starts the
characters U+1000..U+1FFF), because the dispatcher looks every input byte
up in the keymap.

Git's /etc/inputrc sets convert-meta off explicitly (§9).

### 4.5 Function names

The funmap is readline's table (RL/funmap.c:55-200) plus bash's additions
(BL:466-517, 637). It has 176 names on the oracle (`bind -l` **[oracle]**).
The full list is there. Notes:

- Bash's additions include alias-expand-line, history-and-alias-expand-line,
  insert-last-argument (an alias of yank-last-arg), display-shell-version,
  edit-and-execute-command, the complete-* / possible-*-completions family,
  glob-*, dynamic-complete-history, dabbrev-expand, magic-space,
  shell-*-word, spell-correct-word, bash-vi-complete and
  vi-edit-and-execute-command.
- `paste-from-clipboard` exists only in Windows builds (RL/funmap.c:121-123).
  Git's MSYS bash has it **[git]**. On Linux the name is unknown, so the
  binding becomes nothing (§4.1).
- The vi-* names must resolve even though we have no vi mode, because
  inputrc files bind them inside `$if mode=vi` or `set keymap vi`.

What our reader needs: a name-to-command table that includes every name in
`bind -l`. Names whose commands we don't implement can bind to a stub
(bell), or to nothing. These behave the same except for the shadowing rule
of §4.6, which keeps a real function but not nothing.

### 4.6 Prefixes, shadowing and unbinding (RL/bind.c:383-510, RL/readline.c:943-1098)

The binding walks the sequence through the keymaps, creating nested keymaps
as needed.

**A longer sequence over an existing binding.** Example: `"\e\e[C"` when
ESC ESC is `complete`. The existing function or macro at the prefix moves
into the new nested keymap's any-other-key slot. The one exception is
`do-lowercase-version`, which is dropped. When dispatch later finds no match
in that nested keymap, it runs the shadowed binding and pushes back the
non-matching key.

**A sequence that ends on an existing keymap.** Example: `"\C-x": ...`,
`ESC: ...`, `"\e": ...`. The function goes into that keymap's any-other-key
slot. **[oracle]**, with `"\C-x": backward-char`:

- `C-x C-x` still ran exchange-point-and-mark.
- `C-x` followed by a 1-second pause and then `Z` ran backward-char after
  keyseq-timeout (500 ms) and then inserted `Z`.
- `C-x Z` typed at once inserted a **lower-case** `z` after the
  backward-char. The C-x map binds upper-case letters to
  do-lowercase-version, so Z was retried as z, found nothing, and fell
  back.

**Waiting.** When a typed prefix has a shadowed binding and no more input
arrives within keyseq-timeout ms, the shadowed binding runs. With a timeout
of 0 it waits forever (RL/readline.c:1063-1077).

**Unbinding.** Binding nothing (§4.1) to a single key clears it.

- If the key is a keymap, readline stores a do-nothing function in the
  any-other-key slot instead (RL/bind.c:470-477). Typing just the prefix
  then does nothing, with no bell.
- Binding nothing to the last key of a sequence, when that leaves its
  nested keymap empty and the parent's any-other-key held a function, folds
  the keymap back into the parent. This only works one level up
  (RL/bind.c:496-506).

**An unbound key** (nothing in its slot and no shadow) aborts with a bell.
An unbound key *inside* a sequence backs up to the longest bound prefix.
**[oracle]**: on Linux, with `"\e[2;2~": paste-from-clipboard` (unknown
there), typing that sequence inserted only `~`, with no bell. That is the
same as any unbound CSI sequence.

### 4.7 Keymaps (RL/bind.c:2404-2416, 2514-2535)

`set keymap NAME` sets the binding keymap for the following lines. The
names, case-insensitive:

- `emacs`, `emacs-standard`
- `emacs-meta` (the ESC map)
- `emacs-ctlx` (the C-x map)
- `vi`, `vi-move`, `vi-command` (the vi movement map)
- `vi-insert`

`set editing-mode` also sets the binding keymap. **[oracle]**:

- `set keymap emacs-ctlx` then `"q": beginning-of-line` made C-x q work.
- `set keymap vi-insert` then `"\C-t": end-of-line` left emacs C-t as
  transpose-chars.
- A later `set editing-mode emacs` sent the next binding back to the emacs
  map.
- A file ending with `set keymap emacs-meta` did not affect C-x C-r's
  re-read, because the keymap is reset after every read (§1.2 step 7,
  §1.3).

The keymap is not reset when a file *starts*, and `$include` inherits it.
Our editor has no vi mode, but it must keep vi maps (or a sink) so that
vi-only bindings never land in the emacs map.

### 4.8 The tty's special characters override the inputrc (RL/rltty.c:638-661, 881-947)

With bind-tty-special-chars on (the default), every time readline prepares
the terminal for a line, it rebinds the tty's special characters:

- erase (normally DEL) to backward-delete-char;
- kill (C-u) to unix-line-discard;
- lnext (C-v) to quoted-insert;
- werase (C-w) to unix-word-rubout.

It first resets the characters it had saved to self-insert. Only slots that
hold a function are touched, not keymaps.

So **an inputrc binding of C-w, C-u, C-v or DEL is undone on every line**.
**[oracle]**: `"\C-w": backward-char` still killed a word. With
`set bind-tty-special-chars off` earlier in the file, it moved back a
character **[oracle]**. For us that means: when a line starts, if the
variable is on, apply the terminal's special characters on top of the
keymap. Our vos terminal must report the usual stty values (^? ^U ^V ^W),
and Windows has none, so nothing changes there.

---------------------------------------------------------------------------

## 5. Directives (RL/bind.c:1210-1557)

The text after `$` (blanks allowed) is split into a directive word and
arguments, with leading blanks skipped. The directive word is matched
case-insensitively against `if`, `endif`, `else` and `include`. Anything
else gives `%s: unknown parser directive` **[oracle]**:
`readline: ~/.inputrc: line 6: foo: unknown parser directive`.

### 5.1 `$if` (RL/bind.c:1240-1441)

Each `$if` pushes the current "off" flag. If parsing is already off, the
`$if` just pushes, and its test is **not evaluated**, so a malformed test
inside a false branch produces no error. Otherwise the first blank-delimited
word of the arguments picks the test, in this order:

1. **`term=NAME`** (prefix `term=` in any case). True if NAME equals,
   case-insensitively, either the terminal name or the part of the terminal
   name before its first `-`. The `-` split applies to the terminal name,
   not to NAME. **[oracle]**, with TERM=xterm-256color set in the rc:
   `term=xterm` and `term=xterm-256color` were true, `term=XTERM` was true,
   and `term=256color` was false. If bash has no TERM at all, `term=` is
   not recognised and the `$if` is false. Only the first word counts, so
   `$if term=msys # RXVT` is fine.
2. **`mode=emacs` / `mode=vi`.** True if it matches the current editing
   mode. Any other mode name is always false.
3. **`version`** (case-insensitive prefix `version`, so the operator may
   touch it). The whole argument string is re-joined, and blanks are
   allowed around the operator and the number. Then:
   - The operator is one of `=`, `==`, `!=`, `<`, `<=`, `>`, `>=`. If none
     is there: ``comparison operator expected, found `%s'``, showing the
     rest of the line or `end-of-line` (``found `8.3'`` for
     `$if version 8.3` **[oracle]**).
   - Then a number. If it is missing or does not start with a digit:
     ``numeric argument expected, found `%s'`` (``found `x'``
     **[oracle]**). A `.` followed by a non-digit gives the same error.
   - After the number, only blanks and an optional `#` comment are allowed.
     Anything else: ``trailing garbage on line: `%s'``. The text shown
     starts at the first character after the number, blanks included:
     ``trailing garbage on line: ` junk'`` for `>= 8.3 junk`, and
     ``found `x'`` for `> 8.3x` **[oracle]**.
   - **The comparison is on major×10+minor** against readline's 8×10+3 =
     83. The minor part may have several digits and simply adds, so
     `8.10` is 90 and `$if version < 8.10` is **true** **[oracle]**. `8.`
     and `8` both mean 80, and `9.0` means 90.
   - **On any of these errors the `$if` counts as false** (it still
     pushes, so its `$endif` balances).
4. **The application name.** True if the first word equals "Bash",
   case-insensitively (`$if bash` was true **[oracle]**). Extra words are
   ignored (`$if Bash extra words` was true **[oracle]**).
5. **A variable test**, `NAME OP VALUE`. This applies only when the
   *whole* first word is a readline variable name (§8), so
   `$if completion-ignore-case==on` is not recognised and is simply false
   **[oracle]**.
   - OP must be `=`, `==` or `!=`. Otherwise:
     ``equality comparison operator expected, found `%s'`` (``found
     `end-of-line'`` for a bare `$if bell-style`, and ``found `< 600'``
     for `$if keyseq-timeout < 600` **[oracle]**).
   - VALUE is the next blank-delimited word. If it is missing:
     ``argument expected, found `'`` **[oracle]**.
   - The comparison is a case-insensitive string comparison of VALUE with
     the variable's current value as `bind -v` would print it. Booleans
     read `on`/`off`. Integers read in decimal. bell-style reads
     `none`/`visible`/`audible`. editing-mode reads `emacs`/`vi`. keymap
     reads the current binding keymap's first name (`emacs`, `emacs-meta`,
     ...). The mode strings and comment-begin read their text.
   - **[oracle]**: `completion-ignore-case == on` (after setting it),
     `mark-directories != Off`, `bell-style == audible`,
     `editing-mode == emacs`, `keymap == emacs`, `comment-begin = #` and
     (interactive) `history-size == 500` were all true.
   - Errors make the `$if` false.
   - isearch-terminators and the region colours can have no value at all.
     Comparing against them is undefined in readline and may crash it. Our
     reader should treat that as false.
6. **Anything else** (an unknown word, an empty `$if`, `other`) is false.
   There is no error.

### 5.2 `$else` (RL/bind.c:1445-1470)

With an empty stack: `$else found without matching $if` **[oracle]**.
Otherwise the current flag is flipped, **unless any enclosing level
(anything on the stack) is off**. Arguments are ignored. A second `$else`
flips again. **[oracle]**: inside a false `$if mode=vi`, a nested
`$if Bash ... $else ... $endif` set nothing, and the outer `$else` branch
did run.

### 5.3 `$endif` (RL/bind.c:1475-1482)

Pops the flag. With an empty stack: `$endif without matching $if`
**[oracle]**. Arguments are ignored.

### 5.4 Nesting

There is no depth limit (the stack grows by 20 at a time). The test of
§5.1 is evaluated only when the `$if` sits in a live region.

### 5.5 `$include FILE` (RL/bind.c:1485-1508)

- Skipped while parsing is off.
- FILE is **the whole rest of the line**: leading blanks are removed but
  trailing blanks and comments are kept. It is then tilde-expanded. A
  relative name is relative to the shell's current directory, not to the
  including file.
- **[oracle]**: `$include ~/inc1` worked, and so did `$include inc1` from
  the home directory. `$include ~/inc1 # trailing comment` and
  `$include ~/missing` both did nothing, silently.
- The file is read like a top-level file but at include level n+1. It is
  not remembered for C-x C-r. Its messages carry its own name as written
  and its own line numbers. The includer's name, line number and level are
  restored afterwards.
- **No recursion guard.** A file that includes itself recurses until
  resources run out. Our reader should cap the depth (say 32) and treat
  deeper includes as missing files.
- **The location prefix is lost after an `$include`.** The include turns
  off the "currently reading a file" flag when it finishes, and the
  includer does not turn it back on. Every later error in the including
  file (and after nested includes) prints with no file and line.
  **[oracle]**: `readline: e-after-include: unknown variable name`.
  Messages from a *later* included file have their prefix again, because
  reading that file turns the flag back on. The flag stays off until the
  next top-level read.

### 5.6 The conditional state is global

- The "off" flag is cleared at the start of **every** readline call
  (RL/readline.c:1238), so an unterminated false `$if` at the end of a file
  stops mattering once the next line is read.
- **The stack is never cleared.** An unbalanced `$if` leaves its entry
  there. That swallows a later stray `$endif` without the error, including
  one in another file, on a C-x C-r, or in an included file. A later
  `$else` then also sees that entry.
- **[oracle]**: an included file containing only `$if foo` turned off the
  rest of its includer until the includer's own `$endif`. That `$endif`
  popped the included `$if`, and the outer `$if Bash` then stayed on the
  stack.

To match this exactly, our reader keeps one stack for the whole process and
resets only the flag.

---------------------------------------------------------------------------

## 6. Error reporting (RL/bind.c:1133-1148)

Every message is one line on **stderr**, flushed:

    readline: FILE: line N: MESSAGE

- FILE is the name as given to the reader (§1.1, §5.5). N is the 1-based
  line number in that file.
- Without the "currently reading" flag (after an `$include`, §5.5, and for
  `bind` command lines) the line is `readline: MESSAGE`.
- Parsing always continues with the next line, and nothing is undone.
- At startup the messages come before the first prompt (§1.2). For
  C-x C-r they come mid-line with no redraw (§1.3).

The complete set of messages (`%s` is filled in as described):

| message | cause |
|---|---|
| `%s: unknown variable name` | `set` with an unknown name (§3.1) |
| `%s: could not set value to `%s'` | setter rejected the value (§3.3) |
| `%s: no closing `"' in key binding` | unterminated quoted key sequence; the whole line |
| ``%s': invalid key binding: missing key sequence`` | line starts with `:`; the whole line |
| `%s: no key sequence terminator` | no `:`, space or tab after the key part; the whole line |
| ``%s': missing closing quote for macro`` | unterminated macro; from its quote |
| `%s: unknown key modifier` | `x-y` keyname without Control/Meta |
| `%s: unknown parser directive` | `$word` not if/else/endif/include |
| `$else found without matching $if` | |
| `$endif without matching $if` | |
| ``comparison operator expected, found `%s'`` | `$if version` |
| ``numeric argument expected, found `%s'`` | `$if version` |
| ``trailing garbage on line: `%s'`` | `$if version` |
| ``equality comparison operator expected, found `%s'`` | `$if var` |
| ``argument expected, found `%s'`` | `$if var OP` with no value |

Unknown function names, empty key sequences, missing or unreadable files
(including `$include`) and `:=` lines are all **silent**.

---------------------------------------------------------------------------

## 7. What bash changes from readline's defaults

Bash sets no readline variable at startup. The relevant differences come
from the environment and the initialisation order:

- **history-size**: bash applies HISTSIZE (500 by default) before the
  inputrc, so an interactive shell reads 500. Without that it would be
  "unstifled" (-1, as in `bash -c` **[oracle]**). §3.3 explains the
  separation from HISTSIZE.
- **convert-meta off, input-meta on, output-meta on** come from the UTF-8
  locale (§4.4). The compiled defaults are the opposite.
- **completion-ignore-case** defaults to on only in native Windows builds
  that are not Cygwin (RL/complete.c:186-190). bash on MSYS has it off. Git
  turns it on through its /etc/inputrc.
- **comment-begin** is set to `#` when posix mode is turned on (BL:354),
  which is no change.
- **TERM, the application name and the extra bindings** come from bash
  (§1.2).
- **enable-bracketed-paste and enable-active-region** are forced off for a
  terminal that is not ANSI-like (dumb, for example), before the inputrc.
- **The region colours** default to termcap's standout on/off: `\e[7m` and
  `\e[27m` on the oracle's xterm.

`bind -v` on the oracle (bash 5.3.0, TERM=xterm, LANG=C.UTF-8) is the
reference for every default in §8 **[oracle]**.

---------------------------------------------------------------------------

## 8. The variable table (RL/bind.c:1869-2010)

Booleans. The default is the oracle's effective value, followed by readline's
compiled default where they differ:

| name | default | notes |
|---|---|---|
| bind-tty-special-chars | on | §4.8 |
| blink-matching-paren | off | side effect §3.2 |
| byte-oriented | off | |
| colored-completion-prefix | off | LS_COLORS parsed at startup only |
| colored-stats | off | same |
| completion-ignore-case | off | on in native Windows builds |
| completion-map-case | off | |
| convert-meta | off (compiled: on) | locale, §4.4 |
| disable-completion | off | |
| echo-control-characters | on | |
| enable-active-region | on | off for non-ANSI terminals |
| enable-bracketed-paste | on | also sets enable-active-region |
| enable-keypad | off | |
| enable-meta-key | on | |
| expand-tilde | off | |
| force-meta-prefix | off | new in 8.3, §4.4 |
| history-preserve-point | off | |
| horizontal-scroll-mode | off | |
| input-meta | on (compiled: off) | locale |
| mark-directories | on | |
| mark-modified-lines | off | |
| mark-symlinked-directories | off | |
| match-hidden-files | on | |
| menu-complete-display-prefix | off | |
| meta-flag | on | another name for input-meta |
| output-meta | on (compiled: off) | locale |
| page-completions | on | |
| prefer-visible-bell | on (reported) | sets bell-style, §3.2 |
| print-completions-horizontally | off | |
| revert-all-at-newline | off | |
| search-ignore-case | off | |
| show-all-if-ambiguous | off | |
| show-all-if-unmodified | off | |
| show-mode-in-prompt | off | |
| skip-completed-text | off | |
| visible-stats | off | |

String and integer variables (§3.3 has the parsing):

| name | type | default |
|---|---|---|
| active-region-end-color | string | termcap `se` (`\e[27m`) |
| active-region-start-color | string | termcap `so` (`\e[7m`) |
| bell-style | string | audible |
| comment-begin | string | `#` |
| completion-display-width | int | -1 |
| completion-prefix-display-length | int | 0 |
| completion-query-items | int | 100 |
| editing-mode | string | emacs |
| emacs-mode-string | string | `@` |
| history-size | int | 500 in an interactive bash (HISTSIZE), else -1 |
| isearch-terminators | string | unset (ESC and C-J), not listed by `bind -v` |
| keymap | string | emacs |
| keyseq-timeout | int | 500 |
| vi-cmd-mode-string | string | `(cmd)` |
| vi-ins-mode-string | string | `(ins)` |

For step 6, the variables that steps 2-5 implement are the ones whose
setting changes our behaviour. Every other name must still be accepted
silently, with its value stored, so that `$if NAME == VALUE` and a later
`bind -v` behave.

---------------------------------------------------------------------------

## 9. Git for Windows' /etc/inputrc

The file (`<git>/etc/inputrc`, 82 lines, LF line endings, from the install
on this machine) does the following:

- **Sets** bell-style visible, completion-query-items 40,
  completion-ignore-case on, input-meta on, output-meta on, convert-meta
  off, visible-stats off, mark-directories on, mark-symlinked-directories
  on and show-all-if-ambiguous off. Each `set` is preceded by explanatory
  comment lines.
- **Inside `$if mode=emacs`**, binds:
  - Shift-Insert (`\e[2;2~`) to paste-from-clipboard;
  - PgUp and PgDn (`\e[5~`, `\e[6~`) to beginning-of-history and
    end-of-history;
  - Shift-Tab (`\e[Z`) to complete;
  - the mintty Ctrl-arrow and Alt-arrow sequences (`\e[1;5A`..`D`,
    `\e[1;3C`/`D`) to **macros** that re-send other sequences (`"\eOD"`,
    `"\e\e[D"`, ...).

  Each of these lines has a trailing `# comment` after the right-hand side.
- **Nested `$if term=msys # RXVT`** (with a trailing comment), with a
  `$else` branch:
  - The msys branch binds the rxvt Home and End keys (`\e[7~`, `\e[8~`),
    F1 to display-shell-version and F5 to re-read-init-file, and binds F2-F4
    and F6-F10 to text macros (`"Function Key 2"`, ...).
  - The `$else` branch binds `\e[1~`/`\e[4~` to beginning-of-line and
    end-of-line, `\e[3~` to delete-char, and `\e[1;5C`/`D` to forward-word
    and backward-word (replacing the macros above). It binds `\e\e[C` and
    `\e\e[D` to forward-word and backward-word (which shadows M-ESC
    `complete` under ESC ESC, §4.6), and binds F6-F11 to text macros. The
    branch also holds an indented, commented-out `# $if term=cygwin`,
    which is just a comment.
- **Nested `$if term=cygwin`**, which binds the four `\e[A`..`\e[D`
  arrows to their usual commands.

Every construct in the file must parse: `set` with booleans, integers and
bell-style; indented lines; trailing comments after function names, macros
and `$if term=`; `$if mode=`; nested `$if term=` with `$else`; quoted key
sequences with `\e`; macros as right-hand sides; and the function names
paste-from-clipboard, display-shell-version and re-read-init-file.

**[oracle]**: fed to Linux bash 5.3 as ~/.inputrc, the file produced no
messages. completion-ignore-case took effect, and `\e[1;5D` (with
TERM=xterm, so the `$else` branch) moved back a word. Shift-Insert inserted
`~`, because paste-from-clipboard does not exist on Linux (§4.6). **[git]**:
Git's own bash read it with no ~/.inputrc present, and
`bind -q paste-from-clipboard` reported `"\C-t", "\e[2;2~"`. (The `\C-t`
came from our test file.)

For our msys mode: until macros exist, the macro lines must parse and be
accepted (§4.1). paste-from-clipboard should exist as a name, even as a
stub. Typical TERM values in Git Bash (xterm, xterm-256color) select the
`$else` branch.

---------------------------------------------------------------------------

## 10. Implementation sketch (ours)

- `inputrc_read(path, level)`:
  - Read the whole file. On failure, return failure silently.
  - Iterate over the lines. In msys mode, drop one CR before each LF. Skip
    leading space and tab. Skip empty lines and `#` lines. Dispatch the
    rest.
  - Keep the "reading" flag, file name and line number as globals, with
    the include quirk of §5.5.
- A lookup function for §1.1, using our shell variable INPUTRC and HOME
  for `~`. Keep a `last_file` string for C-x C-r.
- Run at the editor's first use (prompt, `read -e`), after the default
  keymap and before the "if unbound" defaults of §1.2.
- Keymaps as tries of 256 slots plus an any-other-key slot, with the
  shadowing and timeout behaviour of §4.6 and the dispatcher of emacs.md.
  The vi maps exist only as binding targets.
- A variable table that keeps every name in §8 (value and printed form),
  with setters as in §3.3, so that `$if NAME == VALUE` reads them back.
- Errors go to fd 2 in the exact form of §6.

---------------------------------------------------------------------------

## 11. Things not determined

- **HOME for `~`.** Whether readline's tilde expansion (of `~/.inputrc`, an
  INPUTRC value, or an `$include`) uses bash's HOME shell variable or the
  password database. Bash's tilde hooks are in files not available here,
  and the oracle was not tested with a changed HOME.
- **TERM changing later.** Whether bash re-initialises the terminal name
  (for later `$if term=` on C-x C-r) when TERM changes after startup.
  bash's `sv_terminal` is not available here, and this was not tested.
- **CR under Git's bash at startup.** CRLF stripping on MSYS was confirmed
  only through `bind -f`. It is the same reader, so a CRLF ~/.inputrc
  should read the same way.
- **Comparisons against unset values.** `$if isearch-terminators == x`, or
  the same against an empty region colour. Readline may crash here, and we
  choose false.
- **Self-including files.** Not tried (§5.5).

---------------------------------------------------------------------------

## 12. Scenarios for t/edit

Recording notes:

- The rc runs before readline reads its init file (§1.2), so each scenario
  writes `~/.inputrc` (or sets INPUTRC) in `rc` lines, with a quoted
  heredoc (`rc cat > ~/.inputrc <<'EOF'` ... `rc EOF`) so backslashes
  survive. **No scenario uses `bind`.**
- To change settings mid-scenario, keep variant files and type
  `cp ~/vN ~/.inputrc\r\C-x\C-r`. Re-reading resets nothing, so a variant
  must turn off explicitly what an earlier one turned on (§1.3). Only use
  variants without errors, because re-read errors leave a stale display
  (§1.3). Follow the re-read with `\C-l` to keep the screen small.
- Completion scenarios do their work in a subdirectory (`rc mkdir w; cd w`)
  so that `.edrc`, `.inputrc` and the variant files stay out of listings.
- The other pitfalls are as before: all keys on one `keys` line arrive at
  once; C-s is XOFF; type a backslash as `\x5c`; bell counts are
  cumulative; `\M-x` in a `keys` line is ESC x.
- The expected results below were all observed on the oracle during this
  research, except where marked "expected".

1. **inputrc-errors** (size 40 110). ~/.inputrc as in §6:
   - one line for each message: unknown variable; bad bell-style; unclosed
     key quote; `$foo`; stray `$endif` and `$else`; `nokey`; version
     garbage `> 8.3x`; `:x`; bare `set`; an unclosed macro; `foo-a:`;
     `$if version 8.3`; `$if version == x`; bare `$if bell-style`;
     `$if bell-style ==`; empty comment-begin;
     `set bell-style none # comment`; bad editing-mode; bad keymap;
     `"\C-t"` alone;
   - silent lines: `a:=b`, an empty isearch-terminators,
     `set completion-query-items abc`;
   - `  $ if Bash` and `$ENDIF` around a probe line;
   - `"\C-o": no-such-function`.

   Snap 1: the 23 messages with their line numbers, then `$ `. Keys
   `abc\C-tX` (C-t bound in the file) → `$ Xabc`. `\C-a\C-k\C-o` → bell 1.
2. **inputrc-keys** (size 20 80). The file:

       set bind-tty-special-chars off
       "\C-w": backward-char
       "\M-t": beginning-of-line
       Meta-u: end-of-line
       Home: beginning-of-line
       C-o : end-of-line
       Control-y beginning-of-line
       ctrl-r: Forward-Char
       "\x14": end-of-line
       "\C-x": backward-char
       "": end-of-line
       "\e[A": beginning-of-line

   Keys, each on a fresh line (`\C-a\C-k`):
   - `ab cd\C-w` → cursor col 6.
   - `ab cd\et` → `cd ab`. The `\M-` binding is unreachable.
   - `ab cd\C-a\eu` → `AB cd`.
   - `ab cdH` → cursor col 2, no H.
   - `ab cd\C-o` → bell.
   - `ab cd\C-y` → col 2.
   - `ab cd\C-a\C-r` → col 3.
   - `ab cd\C-a\C-t` → end of line.
   - `ab cd\C-xZ` → `ab czd`.
   - `ab cd\C-x`, `pause 1000`, `Z` → `ab cZd`.
   - `ab cd\C-x\C-x` → exchange-point-and-mark.
   - `ab cd\e[A` → col 2.

   Then `cp ~/v2 ~/.inputrc\r\C-x\C-r\C-l`, where v2 is
   `set force-meta-prefix on`, `"\M-t": beginning-of-line`,
   `"\M-\C-f": beginning-of-line`. Then `ab cd\et` → col 2 and
   `ab cd\e\C-f` → col 2.
3. **inputrc-tty-chars** (size 12 60). The same C-w binding without
   `bind-tty-special-chars off`. `ab cd\C-w` → `ab ` (word killed).
   `"\C-u": backward-char` → `ab cd\C-u` still discards the line. The
   expected result is from §4.8, not recorded in that form.
4. **inputrc-conditionals** (size 50 100; rc `TERM=xterm-256color`). The
   `$if` probes of §5.1, each guarding `set T-name` so that the live
   branches show up as `unknown variable name` lines:
   - mode=emacs (with an `$else`), mode=vi;
   - term=xterm, term=xterm-256color, term=XTERM, term=256color;
   - version==8.3, `< 8.10`, `>8`, `= 8.`, `!= 9 # comment`;
   - bash, `Bash extra words`, other;
   - completion-ignore-case == on (after setting it), `...==on`;
   - mark-directories != Off, bell-style == audible,
     editing-mode == emacs, keymap == emacs, comment-begin = #,
     history-size == 500;
   - the nested `$if mode=vi { $if Bash ... $else ... } $else`;
   - an empty `$if`;
   - `version >= 8.3 junk` and `keyseq-timeout < 600` (errors).

   Snap 1 lists exactly the 18 true probes plus the two error lines (§5).
5. **inputrc-include** (size 30 100). rc sets `INPUTRC=~/ira` (and
   `export -n` it). ira has, in order:
   - a binding;
   - a probe;
   - `$include ~/inc1`;
   - a probe;
   - `$include ~/inc1 # trailing comment`;
   - `$include ~/missing`;
   - `$include inc1`;
   - a probe;
   - `$if Bash`, `$include ~/inc2` (which contains just `$if foo` and a
     probe), a probe, `$endif`;
   - a probe.

   inc1 contains a probe, `"\C-o": end-of-line` and another probe. The
   messages show the lost location prefix after the first include, and the
   unbalanced `$if` hiding the following probe. Then `abc\C-tX\C-o` →
   `$ Xabc`, cursor at the end. Keep C-x C-r out of this one.
6. **inputrc-reread** (size 20 80). rc:
   - `INPUTRC=~/nothere`;
   - ~/.inputrc with `"\C-t": beginning-of-line`;
   - ~/ir2 with `"\C-t": end-of-line`;
   - ~/ir3 with `"\C-t": backward-char`.

   Keys:
   - `abc\C-tX` → `acbX`: no fallback, so C-t is transpose.
   - `\C-a\C-kINPUTRC=~/ir2\r`, then `\C-x\C-r`, then `abc\C-a\C-tX` →
     `abcX`.
   - `\C-a\C-kINPUTRC=~/ir3\r\C-x\C-r`, then `abc\C-tY` → `abcY` (still
     ir2's binding: the remembered file wins).
   - Also history-size: ir2 has `set history-size 3`, so after a few
     commands `history` lists 3 entries and `echo $HISTSIZE` prints 500.
7. **inputrc-values** (size 20 80). rc:
   - `: > Makefile`;
   - variants for completion-ignore-case with values empty, `1`, `yes`,
     `ON`, `0`, and `On garbage` (written as `~/von`, a name without a
     blank);
   - one variant mentioning only mark-directories;
   - `set comment-begin "# x" trailing`;
   - `set keymap emacs-ctlx` / `"q": beginning-of-line` /
     `set keymap vi-insert` / `"\C-t": end-of-line` /
     `set editing-mode emacs` / `"\C-o": beginning-of-line`.

   Keys:
   - `ls ma\t` → `ls Makefile ` for the "on" values, and a bell with the
     line unchanged for `yes` and `0`;
   - the mark-directories-only variant keeps the previous value;
   - `ab cd\C-xq` → col 2;
   - `ab cd\C-t` → `ab dc`;
   - `ab cd\C-o` → col 2;
   - `ab cd\e#` → `# xab cd`.

   A CRLF variant (written with `printf '%s\r\n'`): show-all-if-ambiguous
   `on␍` stays off (`ls f\t` bells, no list), `comment-begin XX␍` makes
   M-# give `XX^Mab cd`, and a CR-terminated binding makes its key ring.
   This variant is POSIX only. In msys mode the CR is stripped, so mark it
   for the Linux personality, or keep it out of the vos run.
8. **inputrc-completion-case** (completion.md scenario 10; size 24 60; rc
   `mkdir w; cd w` plus that scenario's files). ~/.inputrc:
   completion-ignore-case on. Keys: `ls ma\t`, `ls MA\t`, `ls mAKEF\t`,
   `ls md\t`. Then a variant adding completion-map-case on: `ls foo_b\t`.
   Then a variant with ignore-case off, map-case off and match-hidden-files
   off: `ls \t\t`, `ls .\t\t`, `ls ..\t`. Results as in completion.md §12
   (10).
9. **inputrc-completion-marks-layout** (completion.md 11, 12 and 13; size
   24 40; rc `mkdir w; cd w` plus the files of 11). Variants in this order:
   - visible-stats on;
   - visible-stats off with mark-directories off;
   - mark-directories on with mark-symlinked-directories on;
   - print-completions-horizontally on with
     completion-prefix-display-length 3;
   - completion-display-width 12, then 0;
   - show-all-if-ambiguous on;
   - show-all-if-ambiguous off with show-all-if-unmodified on;
   - disable-completion on (`ls fi\tz` → `ls fi  z`);
   - skip-completed-text on (`ls Make|file` TAB → `ls Makefile `,
     observed).

   With files `aaa_long_{1,2,33}` and a directory `aaa_long_dir`, under
   horizontal and prefix-3 settings: `ls aaa\t\t\t` (the first TAB inserts
   and rings, the second rings, the third lists) → `...1    ...2    ...33
   ...dir/` (observed via possible-completions).
10. **inputrc-menu** (completion.md 14; size 24 40). ~/.inputrc:
    - `"\C-o": menu-complete`
    - `"\C-xp": menu-complete-backward`
    - `set menu-complete-display-prefix on`
    - `Control-t: possible-completions`

    Files `aaa_long_{1,2,33}` and `aaa_long_dir/`. Keys:
    - `ls aaa\C-o` → `ls aaa_long_`, bell (the prefix first, observed);
    - `\C-o` → `ls aaa_long_1`;
    - `\C-xp` → `ls aaa_long_`, bell;
    - then the §8 sequence of completion.md, with display-prefix off in a
      second variant.
