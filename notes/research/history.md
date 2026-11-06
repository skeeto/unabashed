# Bash 5.3 command history: semantics digest

Purpose: enough detail to clone bash 5.3's history list, history file,
`history`, `fc`, and `!` history expansion without reading the GPL source.
Everything below is a paraphrase. The only verbatim text is short
user-visible strings: error messages, usage lines, and output formats.

Abbreviations for citations (all relative to the `bash-5.3/` root of
`bash-5.3.tar.gz`):

- BH = `bashhist.c`, PY = `parse.y`, VA = `variables.c`, SH = `shell.c`,
  EV = `eval.c`, BL = `bashline.c`
- HD = `builtins/history.def`, FC = `builtins/fc.def`, CM = `builtins/common.c`,
  SET = `builtins/set.def`, SHO = `builtins/shopt.def`
- RH = `lib/readline/history.c`, RF = `lib/readline/histfile.c`,
  RX = `lib/readline/histexpand.c`, RS = `lib/readline/histsearch.c`
- MAN = `doc/bash.1`

Claims marked **[oracle]** were checked against a real bash 5.3.15 (the
Cygwin `bash` on this machine). Interactive checks fed stdin to
`bash --norc -i`.

---------------------------------------------------------------------------

## 0. Data model

The history library state is shared by everything below.

- **The list**: an array of entries. Each entry has `line` (text without a
  trailing newline, though it may contain embedded newlines), `timestamp`
  (a string or empty), and `data`. `data` is non-null when readline has
  edited the entry, which is what makes `history` print `*`.
- **history_length**: the number of entries.
- **history_base**: the number shown for index 0. It starts at 1 (RH:113).
  It goes up by 1 each time a full stifled list drops its oldest entry
  (RH:389-405). `history -c` resets it to 1 (RH:733).
- **history_offset**: a cursor. Bash resets it to `history_length` (the
  `using_history()` call) after nearly every operation, so "where_history()"
  usually equals `history_length`.
- **Stifling**: when HISTSIZE is a number of 0 or more, the list holds at
  most HISTSIZE entries ("stifled", RH:675-697). When HISTSIZE is unset,
  empty, non-numeric, or negative, the list is unbounded (VA:6066-6101).
- **The HISTSIZE quirk** **[oracle]**: assigning a HISTSIZE smaller than the
  current length drops the oldest entries. It then sets `history_base` to
  *the number of entries dropped*. It does not add that count to the old
  base (RH:685-688). Starting from 10 entries numbered 1..10, `HISTSIZE=4`
  leaves the last four numbered 6,7,8,9 rather than 7..10. Copy this
  off-by-one-looking behavior faithfully.
- When a stifled list has max 0, `add_history` is a no-op (RH:389-394;
  BH:944-945).
- **Timestamps**: every `add_history` stamps the new entry with the
  history comment character followed by the decimal epoch seconds (RH:322-337).
  If the comment character is NUL at that moment, the timestamp string is
  effectively empty. Section 2.4 shows why that matters.
- **Session counters** (bash side, BH:98,101):
  - `history_lines_this_session`: entries added this session and not yet
    written. It is incremented per real add (BH:959-965), decremented per
    `history -d` deletion (BH:361-389), zeroed by `history -c` (BH:352-357),
    capped by a HISTSIZE assignment (VA:6083-6086), and zeroed after
    `-a` or the exit save.
  - `history_lines_in_file`: the number of lines bash believes it has read
    from or written to HISTFILE. `history -n` uses it as the resume point.

---------------------------------------------------------------------------

## 1. The history list

### 1.1 Enabling: when lines are remembered

- Two flags. `enable_history_list` is the user's `set -o history` state.
  `remember_on_history` is the effective "save lines now" flag, which is
  forced off temporarily, for example inside `eval` or command substitution
  (BH:91-92).
- Interactive shells default to history on (SH:1831-1842). Non-interactive
  shells (scripts, `-c`) default to off (SH:1845-1851). For `bash -c` the
  command string is run with history disabled (SH:1483).
- `set -o history` (SET:457-472) does four things:
  - sets both flags;
  - re-enables expansion processing, so `history_expansion_inhibited` becomes
    0 (BH:306-316);
  - re-reads HISTCONTROL and HISTIGNORE;
  - calls **load_history if history_lines_this_session == 0**.

  So a script that runs `set -o history` gets HISTSIZE=500 and HISTFILESIZE
  set as variables, and reads HISTFILE if it is set, non-empty and exists.
  **[oracle]**: before the call HISTSIZE and HISTFILESIZE are unset; after
  it both are `500`, and HISTFILE stays unset in a script.
- `set +o history` clears both flags and inhibits history expansion
  (BH:297-303). The existing list stays, and `history` and `fc -l` still
  work on it. **[oracle]**: the `set -o history` line typed while history was
  off is not saved, because the decision is made when the line is read.
- `eval`/`parse_and_execute` with the NOHIST flag disables history for the
  nested parse. Lines read from strings (eval, command substitution, traps,
  `fc -s` re-execution) are not added.
- What gets added: every **physical input line read by the parser's line
  reader (`shell_getc`)** while `remember_on_history` is set. Before adding,
  the line goes through history expansion (PY:2645-2680). That includes
  comment-only lines, but only at the start of a command
  (**[oracle]**/tests: script comments appear as entries). Lines read from
  startup files are not saved.
- Empty lines are not added (BH:642 requires a non-empty line). Blank lines
  inside a multi-line command are handled in §1.5.
- The line stored is the post-expansion text without its trailing newline.

### 1.2 HISTSIZE

Hook: `sv_histsize` (VA:6066-6101), which runs on assignment and on unset.

| HISTSIZE value | effect |
|---|---|
| unset | unstifle (unbounded) |
| empty string | unstifle |
| non-numeric | **no change at all** (the previous limit stays) |
| negative number | unstifle |
| 0..N | stifle to N; trim the oldest entries now (base quirk in §0); cap lines_this_session at the new length |

- "Numeric" means a strtoimax-style base-10 number: optional leading
  whitespace and sign, digits, and optional trailing whitespace. It must not
  be empty (general.c `valid_number`). The value is truncated to `int`.
- Default: interactive startup does `HISTSIZE=${HISTSIZE:-500}`, but only
  if HISTSIZE is unset (`set_if_not`), and it does so **after the startup
  files run** (BH:328; SH:722 vs 811).
- A HISTSIZE inherited from the environment by a non-interactive shell is
  not applied until it is reassigned or `set -o history` runs load_history.

### 1.3 HISTCONTROL (VA:6112-6138; BH:694-743)

- The value is split on `:`. The recognized words are `ignorespace`,
  `ignoredups`, `ignoreboth` (= the first two), and `erasedups`. Unknown
  words are ignored. Unset or empty means "save all".
- These checks apply only to the **first line** of a command. Continuation
  lines of a multi-line command are never checked: if the first line was
  saved, the continuations are added; if not, they are dropped too
  (BH:770-777).
- Order of checks for a first line (BH:789-808):
  1. **ignorespace**: drop the line if its first character is exactly a
     space `' '`. A leading tab does not count. **[oracle]**: a
     tab-indented line was saved.
  2. **ignoredups**: drop the line if it is byte-identical to the current
     last entry. Only the immediately previous entry is compared.
  3. **HISTIGNORE** patterns (§1.4).
  4. The line will be saved. If **erasedups** is set, first remove *every*
     existing entry equal to the line, scanning from newest to oldest
     (BH:726-743). These removals do **not** decrement
     history_lines_this_session. Then add.
- **[oracle]**: with `ignoreboth:erasedups`, the sequence dup, dup, other,
  dup leaves `other`, `dup`.

### 1.4 HISTIGNORE (BH:989-1069; pathexp.c:605-680)

- Split on `:`, but a colon inside a bracket expression or an extglob group
  does not split (the split honors glob/extglob syntax). The pattern list is
  rebuilt only when the string value changes. An empty value means no
  patterns.
- A pattern containing an unescaped `&` is marked for expansion. At check
  time, every unescaped `&` is replaced by the **previous history line**.
  If that line contains glob metacharacters or backslashes, they are quoted
  so they match literally. `\&` becomes a literal `&` with the backslash
  removed. If there is no previous entry, the pattern is used as written,
  with no substitution (BH:1026-1039; stringlib.c:180-236).
- Each pattern must match the **whole line** (anchored, no implicit `*`). It
  uses shell pattern matching, with extglob syntax honored iff
  `shopt extglob` is on at match time. The first match drops the line.
- `&` alone acts as ignoredups and `[ ]*` acts as ignorespace (MAN:2670-2685).
- **[oracle]**: `HISTIGNORE='&:ls*:\&x'` dropped a duplicate `echo z`,
  `ls -d .`, and the literal line `&x`.

### 1.5 cmdhist / lithist: multi-line commands

State:

- `current_command_line_count` (PY:279) is reset to 0 before each top-level
  parse (EV:368, 413). It goes up by 1 for each physical line read
  (PY:2632), and for each here-document body line (PY:2340). A line counts
  as a continuation when the count is greater than 1.
- `current_command_first_line_saved` (BH:146) records whether the first line
  passed the §1.3 checks.
- `cmdhist` (`command_oriented_history`) defaults to **on** (BH:141;
  SHO:195). `lithist` (`literal_history`) defaults to off.

`maybe_add_history(line)` (BH:760-783):

- If this is a continuation line, it is appended to the entry only if the
  first line was saved **and** one of these holds: we are in a here-doc
  body, lithist is on, we are inside a quoted or delimited construct, or the
  line is *not* a pure comment line. Pure comment line means the first
  non-blank character is `#`, and the check is skipped inside quotes or
  here-docs (BH:658-673). Either way the "last line was a comment" marker is
  recorded.
- Otherwise it is a first line: run the HISTCONTROL/HISTIGNORE checks and
  remember the result.

`bash_add_history(line)` (BH:869-956), with **cmdhist off**: every line
becomes its own entry. With **cmdhist on** and a continuation line, the
separator `sep` is chosen in this order:

1. In a here-doc body, past its first line, when the line already ends in
   `\n`: `""`.
2. The previous line was a comment line (including a trailing `# ...`
   comment after code), that is, this line number = the comment line's
   number + 1: `"\n"`. This keeps the comment from swallowing later text.
3. lithist on: `"\n"`.
4. Otherwise: `history_delimiting_chars(line)` (§1.5.1).

Then the separator is adjusted against the current last entry:

- If not inside quotes, and the entry ends in a single unescaped backslash
  (not `\\`): remove that backslash and use `""`. This is a backslash-newline
  continuation, and the pieces are glued directly. **[oracle]**: lines
  `echo u \` and `v` become `echo u v`; the space came from before the
  backslash.
- If not inside quotes, the entry ends with `\n`, and `sep` starts with `;`:
  drop the `;` (so `"; "` becomes `" "`).
- The new entry text is entry + sep + line, replacing the last entry in
  place (timestamp kept). If there is no last entry (it was deleted), the
  line is added as a new entry.
- Continuation lines do not change history_lines_this_session (only real
  adds do).

#### 1.5.1 history_delimiting_chars (PY:6002-6137)

The parser's last three tokens are visible here. `last_read_token` is the
newline just consumed, `token_before_that` is the last token of the previous
line, and `two_tokens_ago` is the one before it. Rules, first match wins:

1. Inside a quoted or delimited construct (open `'`, `"`, `` ` ``, `$(`,
   etc.):
   - a shell quote character: `"\n"`;
   - `(` (command substitution and the like): `""` if the current entry
     already ends in `\n`, else `"\n"`;
   - anything else: `"\n"`.

   **[oracle]**: `echo "x` + `y"` becomes `echo "x\ny"`, and
   `echo $(echo` + `hi)` becomes `echo $(echo\nhi)`.
2. In a here-doc body: `"\n"` for the first body line (the line after the
   `<<` line), else `""`. Body lines keep their own trailing newline, so the
   stored entry is `cat <<EOF\nh1\nh2\nEOF\n`, ending in a newline
   **[oracle]**. `history` then prints an extra empty line after it.
3. Inside a compound array assignment `a=(...)`: `" "` **[oracle]**:
   `a=(1 2)`.
4. The previous line ended with `)`:
   - if the token before that was `(` (function definition `f()`): `" "`
     **[oracle]**: `g() { echo g; }`;
   - inside a case pattern: `" "`;
   - otherwise (a subshell): `"\n"` if the new line starts, after blanks,
     with `&`, `|` or `;`, else `"; "`.
5. The previous line was `function NAME` without parentheses: `" "`.
6. A continuation line whose text contains `<<`: `"\n"`. A private flag is
   set so the first here-doc body line gets `"\n"`.
7. A continuation line while here-docs are pending: `"\n"`.
8. The previous line was `for NAME`: `" "` if the new line starts (after
   whitespace) with `in`, else `";"` (no space).
9. The previous line was `case WORD` inside a case: `" "`.
10. The previous line's last token is one of these:
    - newline, `{`, `(`, `)`, `;`, `&`, `|`
    - `case`, `do`, `else`, `if`, `;;`, `;&`, `;;&`, `then`, `until`,
      `while`, `&&`, `||`, `in`
    - `$(`, `${`

    Result: `" "`.
11. The new line is blank (spaces and tabs only): `"; "` if the previous
    token was not a newline, else `""`.
12. Default: `"; "`.

Blank continuation lines (PY:2683-2700) are passed to maybe_add_history only
when we are inside a quoted construct (giving an embedded empty line) or
when rule 11 would yield `"; "`. **[oracle]**: a blank line inside
`while false; do` + blank + `:` + `done` vanished, giving
`while false; do :; done`.

Examples, all **[oracle]** with cmdhist on and lithist off:

```
for i in 1 / do / echo $i / done        -> for i in 1; do echo $i; done
if true / then / echo y / else / echo n / fi
                                        -> if true; then echo y; else echo n; fi
f() { / echo f / }                      -> f() { echo f; }
echo a | / cat                          -> echo a | cat
case x in / x) echo x;; / esac          -> case x in x) echo x;; esac
{ echo s / # a comment / echo t / }     -> "{ echo s\necho t; }"  (comment line dropped)
echo q; # comment  (single line)        -> echo q; # comment
```

With lithist on, the same `for` loop is stored as `for i in 1\ndo\necho $i\ndone`.

#### 1.5.2 Expansion inside a multi-line command

While expanding a continuation line, if cmdhist is on and the first line was
saved, `history_length` is temporarily reduced by 1 (BH:582-587). As a
result, `!!` and the other event references see the entry *before* the
command being built, not the partial entry. Test histexp6: in a `for`
loop, `echo !!` on the body line expands to the previous command.

### 1.6 History numbers, HISTCMD, `\!`, `\#`

- `history_number()` (BH:968-972): if either `remember_on_history` or
  `enable_history_list` is set, the result is `history_base + history_length`
  (the number the next entry will get). Otherwise it is **1**.
- **HISTCMD** (VA:1567-1581) is dynamic: `history_number() - executing`.
  `executing` is 1 while the reader loop runs a command (EV:180) and 0 while
  a prompt is being expanded. So during a command it is that command's own
  number, provided the command was added.
  - **[oracle]**: an interactive `echo $HISTCMD` as the second command
    prints `2`.
  - **[oracle]**: with history off, both in scripts and in `-c`, it is
    `1 - 1 = 0`. Interactive after `set +o history` it is also `0`.
  - Assignment is ignored. Unsetting it removes its special behavior
    (MAN:2095-2105).
- `\!` in prompts (PY:6578-6584 via `prompt_history_number`, PY:6210-6226).
  Let n = history_number():
  - if n == 1, use 1;
  - PS1: n;
  - PS2 with cmdhist off: n;
  - in every other case (PS2 with cmdhist on, PS0, PS4, `${x@P}`): n-1.
    PS2 with cmdhist on thus shows the number of the entry being built.
    **[oracle]**: PS2 `\!` showed the same number the finished `for` entry
    got.
- In posix mode only, a bare `!` in a prompt string means the history number
  and `!!` means a literal `!` (PY:6294-6310).
- `\#` is `current_command_number`. It starts at 1 and is incremented just
  before each command executes in the reader loop (EV:178). Lines with
  failed expansions and empty lines do not increment it. For prompts other
  than PS0/PS1/PS2 it is decremented by 1 (PY:6569-6576).

---------------------------------------------------------------------------

## 2. HISTFILE

### 2.1 Defaults and startup (VA:649-659; BH:320-349; SH:804-812)

- At variable initialization, **only if history is on at that moment**
  (that is, interactive), HISTFILE defaults to `~/.bash_history`, or
  `~/.sh_history` in posix mode. Tilde-expanded; only if unset (BH:420-423).
  Scripts get no default HISTFILE.
- Also for interactive shells, HISTCONTROL, HISTIGNORE and HISTTIMEFORMAT
  inherited from the environment are applied at init (VA:677-684).
- After the startup files, an interactive shell runs `load_history` unless
  the startup files already added history lines (for example via
  `history -s`):
  1. If HISTSIZE is unset, set it to `500`. Apply it.
  2. If HISTFILESIZE is unset, set it to the current HISTSIZE string. Apply
     it: this **truncates the file before reading** (§2.3).
  3. If HISTFILE is non-empty and the file exists, read it all
     (`read_history`, §2.2) and set history_lines_in_file to the line count.
- `bash_initialize_history` (BH:273-282) runs for interactive shells and on
  `set -H`. It turns on single-quote inhibition, sets the search delimiters,
  installs bash's "don't expand here" hook, sets the no-expand character
  set, and applies `histchars`. That makes the comment char `#`. Before this
  runs the comment char is NUL (RX:94), which matters for timestamps in
  scripts (§2.4).
- Restricted shells make HISTFILE readonly (SH:1295).

### 2.2 Reading a history file (RF:267-476)

- A filename of NULL (no HISTFILE) means `$HOME/.history`, falling back to
  `$APPDATA` (RF:152-183). That fallback is reachable from sv_histsize; see
  §2.3.
- Errors return an errno and print nothing:
  - open or stat failure;
  - not a regular file (EINVAL/EFTYPE), so `history -r /dev/null` fails with
    status 1;
  - size overflow.

  An empty file succeeds with 0 lines.
- **Timestamp line**: a line whose first byte is the history comment char
  and whose second byte is a digit (RF:140). If the comment char is NUL and
  the file starts with `#<digit>`, `#` is used for this read only
  (RF:377-381).
- **Multiline mode**: if the first byte of the file is a timestamp and
  HISTTIMEFORMAT is set, a global counter `history_multiline_entries` is
  incremented (RF:384). It is **never reset**, so once one timestamped file
  has been read with HISTTIMEFORMAT set, all later reads in that shell use
  multiline mode.
- Splitting (RF:417-462):
  - Split at `\n`. A `\r` right before the `\n` is removed.
  - **An unterminated final line (no trailing `\n`) is ignored.**
    **[oracle]**: the file `a\nb\nc` reads 2 entries.
  - Blank lines are skipped, except in multiline mode, where they are kept
    unless they directly follow a timestamp line.
  - A timestamp line is held and applied to the next non-timestamp line
    (replacing its add-time stamp). Timestamp lines do not count toward
    line totals.
  - In multiline mode, a non-timestamp line that does **not** directly
    follow a timestamp is appended to the previous entry with `\n`. A line
    that does follow one starts a new entry.
  - In non-multiline mode every non-blank line is its own entry.
  - Each new entry goes through `add_history`, so stifling applies: reading
    more lines than HISTSIZE keeps the last HISTSIZE and raises history_base.
- Ranges (used by `-n`): skip the first FROM non-timestamp lines, then read
  the rest. `history_lines_read_from_file` is set to the final line counter,
  that is, the total non-timestamp lines up to the end, including the
  skipped ones.
- Test history9 shows multiline reading. Blank lines right after a
  timestamp are dropped, blank lines after a command line are kept inside
  that entry, and a here-doc spread over several file lines becomes one
  entry.

### 2.3 HISTFILESIZE and truncation (VA:6066-6101; RF:540-722)

- Assigning HISTFILESIZE a numeric value of 0 or more **immediately
  truncates the history file**, whether or not the shell is interactive.
  - The file is `$HISTFILE`. If HISTFILE is unset this becomes
    `$HOME/.history`, so a script doing `HISTFILESIZE=0` with no HISTFILE
    truncates `~/.history`.
  - If the new size is smaller than history_lines_in_file, that counter is
    lowered.
  - Unset, empty, non-numeric or negative values do nothing (no truncation).
- At startup the default is HISTFILESIZE := HISTSIZE (BH:331-332).
- Truncation algorithm, keeping the last N lines:
  - If N == 0, the file becomes empty.
  - Otherwise scan backward from the end counting newlines that are not
    followed by a timestamp line. When HISTTIMEFORMAT is set, the count
    starts at N+1 and the cut point is moved back to just before the
    timestamp line. Net effect: N counts *entries* (non-timestamp lines),
    and a timestamp stays with its command.
  - If the file already has N or fewer lines, nothing is written.
  - Otherwise the tail is written to a temp file `FILE-NNNNN.tmp`
    (5-digit pid modulo 100000) in the same directory, then renamed over the
    original. Symlinks are followed: the target is replaced. Ownership is
    restored if it differs.
- **[oracle]** (test history7): with HISTFILESIZE=3 and timestamps on, a
  session of 1, 3 or 5 commands leaves a 4-, 6- or 6-line file. Without
  timestamps it leaves 2, 3 or 3 lines; the 5-command case keeps the last
  three. HISTFILESIZE=0 leaves 0 lines.
- **[oracle]**: an interactive `HISTFILESIZE=2` on a 5-line file truncates at
  once to lines 4 and 5. At exit the session line is appended and the file
  is truncated again, to `5` and `HISTFILESIZE=2`.

### 2.4 Writing (RF:760-933)

- An entry is written as its `line` + `\n`. If HISTTIMEFORMAT is set (even
  to empty), entries with a non-empty timestamp are preceded by
  `timestamp\n` (that is, `#<epoch>`). Embedded newlines in a line are
  written raw, so without timestamps a multi-line entry comes back as
  several entries.
- **Overwrite** (`write_history`, used by `history -w` and the stifled exit
  path): if the file exists and is regular, write to `FILE-NNNNN.tmp` and
  rename over the original; otherwise create or truncate it directly with
  mode 0600.
- **Append** (`append_history(n)`): opens with O_APPEND and **without
  O_CREAT**, so the caller must create the file first (bash does). It writes
  the last n entries, n capped at history_length.
- I/O errors return errno. Callers print nothing; only the status shows it.
- **Script timestamp gotcha** **[oracle]**: in a non-interactive shell the
  comment char is NUL until `HISTTIMEFORMAT` is set or `set -H` runs, because
  the HISTTIMEFORMAT hook sets the comment char to `#` if it is NUL
  (VA:6167-6178). Entries added before that have an empty timestamp, so no
  `#` line is written for them, and listing them shows `??`.

### 2.5 Saving at exit (SH:1007-1009; BH:488-530)

- At shell exit (after the EXIT trap), **if remember_on_history is set**:
  - This includes a script that did `set -o history` and also set HISTFILE.
  - If history was turned off with `set +o history`, nothing is saved.
- Nothing happens when `history_lines_this_session == 0` or when HISTFILE is
  unset or empty.
- If the file does not exist, create it (0600, truncate).
- If `history_lines_this_session <= history_length` **or** histappend is on,
  **append** those last N entries and add N to history_lines_in_file.
  Otherwise **overwrite** the whole file with the list
  (history_lines_in_file := lines written).
  - Surprise **[oracle]**: even with histappend **off**, bash normally
    appends. A line another process added to HISTFILE mid-session survived
    exit. Overwrite happens only when the session added more lines than the
    list still holds (HISTSIZE trimmed them). **[oracle]**: after
    `HISTSIZE=2` mid-session, the file was rewritten and an external append
    was lost.
- Then history_lines_this_session := 0, and HISTFILESIZE is re-applied,
  truncating the file (BH:526).
- A non-interactive shell hitting EOF does not save history; it just stops
  (PY:6994-6998), unless it is also `remember_on_history`, through the
  normal exit path above.

---------------------------------------------------------------------------

## 3. The `history` builtin (HD)

Usage line, printed on a bad option:
`history: usage: history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]`

### 3.1 Errors and message format

- Builtin errors (CM:83-109) are written to stderr as
  `NAME: [line N: ]history: MESSAGE`.
  - NAME: interactive shells use the shell name, e.g. `bash`. Otherwise it
    is `$BASH_SOURCE[0]` or `$0`.
  - `line N: ` appears only in non-interactive shells.
- Example: `./history.tests: line 17: history: -x: invalid option`
  followed by the usage line (which has no prefix other than `history: `).
- Option parsing uses bash's internal getopt with option string `acd:npsrw`
  (HD:122). `--help` is handled. Messages:
  - `-x: invalid option` + usage, status 2.
  - `-d` without an argument: `-d: option requires an argument` + usage,
    status 2. **[oracle]**
  - `-d` takes the next word as its argument even if it starts with `-`, so
    `history -d -5` works.
- `-p` is accepted even without BANG_HISTORY; it is then ignored.

### 3.2 Processing order (HD:161-343)

1. If more than one of `-a -n -r -w` is given:
   `cannot use more than one of -anrw`, status 1 (HD:161-166).
2. `-c`: clear the list (base := 1, lines_this_session := 0). If there are no
   operands, return 0. Otherwise continue (HD:169-174).
3. `-s`: if there are operands, push them (§3.5). Return 0 (HD:176-181).
4. `-p`: with operands, expand and print (§3.6). With none, return 0
   (HD:183-188).
5. `-d ARG` where ARG contains a `-` after its first character: a range
   (§3.4).
6. `-d ARG`: a single offset (§3.4).
7. None of `-a -n -r -w -c`: list (§3.3).
8. File operations (§3.7).

Unused extra operands are silently ignored in most paths:
`history -d -1 extra` succeeds **[oracle]**.

### 3.3 Listing (HD:370-416)

- `history [n]`:
  - `n` goes through get_numeric_arg (CM:488-515). A non-number prints
    `abc: numeric argument required` and returns 2 (**[oracle]**:
    `history: abc: numeric argument required`). Note `@42` gives
    `@42: numeric argument required` (test history3).
  - More than one operand prints `too many arguments`, sets status 2, and
    **aborts the rest of the command line** (a jump to top level,
    CM:139-152). **[oracle]**: `history 1 2` prints
    `bash: history: too many arguments`.
  - A negative n is replaced by its absolute value.
  - With n in 0 ≤ n < length, only the last n entries are shown, so
    **`history 0` prints nothing** **[oracle]**. With n ≥ length or no n,
    everything is shown.
- Line format: `printf("%5d%c %s%s\n", index+history_base, mark, timestr,
  line)`.
  - mark is `*` if the entry was modified by readline (data non-null),
    else a space. So a normal line is 5-wide right-aligned number, two
    spaces, then the text, e.g. `    1  echo a`.
  - Embedded newlines are printed raw. Continuation lines are not indented.
  - Numbers wider than 5 digits just widen the field.
- Timestamps (HD:351-367) are shown only when HISTTIMEFORMAT is set **and
  non-empty**. timestr is strftime(HISTTIMEFORMAT, localtime(ts)) into a
  128-byte buffer, with no separator added; users put a trailing space in
  the format.
  - If the stored stamp is non-empty but does not parse (it must begin with
    the current comment char; a value of 0 counts as invalid), timestr is
    `<stamp without leading #>: invalid timestamp`.
  - If the stamp is empty, timestr is `??`.
- Listing works even when history is disabled (`set +o history`) **[oracle]**.
- The status is the stdout write check: a write error prints
  `write error: <strerror>` and returns 1.

### 3.4 `-d` (HD:190-266)

Numbers are parsed like HISTSIZE (valid_number, base 10, surrounding
whitespace allowed). Displayed number = index + history_base.

Single offset:

- A bad number prints `ARG: invalid number`, status 1. If ARG starts with
  `0` followed by a digit, the message is `invalid octal number`; if it
  starts with `0x`, `invalid hex number` (CM:214-225).
  E.g. `history: @42: invalid number`.
- A negative value, where ARG starts with `-`: index = length + value. If
  that is < 0, print `ARG: history position out of range`, status 1. Since
  the `history -d` line itself is usually already in the list, `-d -1`
  deletes that command.
- Otherwise the value must be in [base, base+length), else
  `ARG: history position out of range`. That also catches `-0` and `0`.
  - **[oracle]** (test history8): `history: 72: history position out of
    range`, `history: -72: history position out of range`.
- Delete that index. Status 0, or 1 if the delete failed.

Range `START-END` (the split is at the first `-` after position 0, so
`-3--1` and `6--1` work):

- If either half is not a number:
  `WHOLE-ARG: history position out of range` (not "invalid number").
  E.g. `5-0xaf: history position out of range`.
- START:
  - starts with `-` and negative: += length;
  - otherwise if > 0: -= base;
  - **a START of 0 stays 0, meaning the first entry** **[oracle]**:
    `history -d 0-0` deleted the oldest.

  It must then be in [0, length), else `START: history position out of
  range`.
- END: the same rules; its error message names END.
- If START > END after conversion, nothing is deleted, status 1, no message.
- Deleting decrements history_lines_this_session by the number removed.
  erasedups removals do not.
- Test history3 messages: `16: history position out of range`,
  `200: ...`, `-20: ...`, `-50: ...`.

### 3.5 `-s args` (HD:421-453)

- Joins the args with single spaces into one line.
- First removes the last entry, which is normally the `history -s` command
  itself. The removal happens when history is on and the previous add was
  not a `-s` push, and either:
  - the last line read was added as a separate entry, or
  - we are inside a cmdhist multi-line command whose first line was saved
    (this deletes the whole compound entry).

  If the removal fails, return 0 **without adding**.
- Adds via the §1.3 checks (HISTCONTROL, HISTIGNORE, erasedups) but always
  as a new entry. Marks "last add was a push", so `history -s a; history -s b`
  on one line keeps both a and b.
- If HISTIGNORE dropped the `history -s ...` line, there is nothing to
  remove and nothing is removed (test history.tests).
- Always returns 0. It works with history disabled too, but then nothing is
  removed.

### 3.6 `-p args` (HD:457-483)

- Before expanding, if the last line read was added and was not a push,
  delete it (it is the `history -p` line). If that deletion fails, return 1.
  So `!!` refers to the command *before* `history -p`, and **the
  `history -p` command does not stay in history**. With no args nothing is
  deleted, so a bare `history -p` stays.
- Each arg is history-expanded with the library defaults (§5), and the
  result is printed on its own line on stdout. `:p` results print normally.
- On failure: `ARG: history expansion failed` (e.g.
  `history: !!:z: history expansion failed`), status 1. The library's own
  message (`event not found`, ...) is **not** printed. Remaining args are
  still processed.
- The args are usually single-quoted on the command line; otherwise the
  shell's line-level expansion would expand them first.

### 3.7 `-a -n -r -w [file]` (HD:273-341)

- The file is the operand, or else `$HISTFILE`. If the result is null or
  empty:
  - If not (interactive shell between commands), print an error. In
    practice it is printed both in scripts and at the interactive prompt:
    - explicit empty operand: `empty filename`;
    - otherwise: `HISTFILE: parameter null or not set`.
  - Return **0 if interactive, 1 otherwise** **[oracle]**: in a script,
    `history -a` gives status 1 and `history -w ""` gives
    `history: empty filename`, status 1.
- Restricted shell with a `/` in the name: `NAME: restricted`, status 1.
- `-a`:
  - If lines_this_session > 0: create the file if missing (0600). On
    failure print `FILE: cannot create: <strerror>` and return 1.
  - Cap n at history_length, append the last n entries, add n to
    lines_in_file, set lines_this_session := 0.
  - Otherwise just set lines_this_session := 0.
  - The status is from the append: 1 on I/O error, with no message.
- `-w`: overwrite the file with the whole list (§2.4).
  - Does **not** reset lines_this_session.
  - Does **not** apply HISTFILESIZE **[oracle]**: with HISTFILESIZE=1,
    `-w` wrote 2 lines. MAN claims the builtin truncates; the code does not.
- `-r`: read the whole file and append it to the list.
  - history_lines_in_file := lines read.
  - lines_this_session is unchanged.
  - Errors give a silent status 1 (e.g. a nonexistent file).
- `-n`: read the file starting at line history_lines_in_file
  (non-timestamp lines), then set history_lines_in_file to the file's total
  line count.
  - If histappend is off, lines_this_session also grows by
    (new lines_in_file − old) + (base growth). So the newly read lines will
    be written again at exit or by `-a`.

---------------------------------------------------------------------------

## 4. `fc` (FC)

Usage: `fc: usage: fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]`

### 4.1 Options (FC:204-246)

- Option string `:e:lnrs`. `-e` takes an argument.
- Option parsing **stops at the first word that is a number with an
  optional leading `-`**, so `fc -l -5` and `fc -5` treat `-5` as an operand
  (FC:211, 550-560). `--` also ends options.
- A bad option prints `-v: invalid option` + usage, status 2.
- `-e -` is the same as `-s`.
- `-l` lists, `-n` omits numbers, `-r` reverses.

### 4.2 `fc -s [pat=rep ...] [command]` (FC:250-313)

1. Leading operands containing `=` become substitution pairs, split at the
   first `=` and applied in order.
2. The command spec is the next operand, or the most recent command if there
   is none. It is resolved like *first* (§4.4, execute mode). A failure
   (no history, not found, or `-0`) prints `no command found`, status 1.
3. In posix mode, more operands after the command spec print
   `too many arguments`, status 1. Otherwise extra operands are ignored.
4. Each pair replaces **every occurrence** of pat with rep: a global
   literal string replace, left to right, non-overlapping.
   - An empty pat replaces every character with rep (the replacement
     consumes one character).
   - **[oracle]**/test: `r a=x` on `echo aa ab ac` gives `echo xx xb xc`.
   - A pat that does not occur is not an error (**[oracle]**: the command
     re-ran unchanged).
5. Print the resulting command to **stderr** followed by `\n`.
6. Replace the history entry: remove the last entry (the `fc -s` line),
   drop one trailing newline from the command, and add the command through
   the full §1.3 logic (HISTCONTROL/HISTIGNORE apply). So `fc` never
   appears in history, only the re-run command.
7. Execute it as a string with history disabled (parse_and_execute,
   NOHIST). The status is the command's status (e.g. 42 for `(exit 42)`,
   test history1).

### 4.3 List and edit forms (FC:317-545)

- If the history list does not exist at all, return 0 silently.
- **last_hist**: count = number of entries. `rh` is 1 if history is
  currently on, and also inside a command substitution when
  `set -o history` is on. Then last_hist = count − rh −
  (1 if the line being executed was added as its own entry, else 0). This
  skips the `fc` command itself when it was recorded. Clamp it to at least
  0. `real_last` = index of the newest entry.
- Defaults:
  - no operands, listing: first = last_hist − 15 (clamped to 0),
    last = last_hist (16 entries);
  - no operands, editing: first = last = last_hist;
  - one operand: last = first for editing, or last_hist for listing.
    But if first resolved to real_last, last = real_last.
- Errors after resolution, all status 1:
  - an invalid spec (`-0` when not listing):
    `history specification out of range`;
  - a string not found: `no command found`.
  - Test history5: `fc -0` gives `history specification out of range`;
    `fc -s -0` gives `no command found`.
- Negative or underflowing results clamp to 0. **Out-of-range numbers are
  not errors** (POSIX).
- When editing (not listing) and the current `fc` line was recorded, that
  entry is **deleted** before editing (FC:403-421), and the indexes are
  re-clamped. This happens even if the editor later fails.
- If first > last, they are swapped and reverse order is forced.
- Listing output (FC:468-484), to stdout:
  - Number: `%d` of index+base, no padding, omitted with `-n`.
  - Then, if not posix: a TAB followed by `*` or a space (`*` = modified
    entry). If posix: just a TAB.
  - Then the line and `\n`.
  - So `fc -l` lines look like `12\t echo hi` and `fc -nl` lines look like
    `\t echo hi`.
  - The status is from the write check.
- Edit (FC:454-545):
  - Write the selected lines, without numbers, one per line, to a temp file
    from `sh_mktmpfp("bash-fc", ...)` in TMPDIR. If that fails:
    `FILE: cannot open temp file: <strerror>`, status 1.
  - The editor command is the string `ENAME FILE` when `-e` is given,
    else `${FCEDIT:-${EDITOR:-vi}} FILE`. In posix mode the default is
    `${FCEDIT:-${EDITOR:-ed}}`, or `${FCEDIT:-ed}` with STRICT_POSIX. It
    runs as a string with history off.
  - If the editor's status is non-zero, remove the file and return 1
    (no message). **[oracle]**: `fc -e false` returned 1 silently.
  - Otherwise force history on, turn on verbose echo (each line is printed
    to stderr as it is read, like `set -v`), and source the file.
    - Commands from the file **are added to history** (FEVAL_HISTORY;
      builtins/evalfile.c:368-377).
    - The status is that of the last command.
    - The temp file is removed afterwards, and the verbose flag and the
      DEBUG-trap verbosity are restored.
  - Test history.tests: `fc -e cat` echoes the command through `cat` and
    the verbose echo, then runs it.
- `fc` with no history-capable state (history off) still lists. **[oracle]**:
  `fc -l 1 2 3` (extra operand) is ignored and lists 1..2.

### 4.4 Resolving first/last (fc_gethnum, FC:569-666)

Here `i` = last_hist as in §4.3. If that is < 0, the result is −1 (treated
as index 0 for list/edit, and as "no command found" for `-s`). A missing
spec gives `i`.

- **Number** (an optional `-` followed by digits; `atoi` reads the digits
  and ignores trailing junk):
  - Negative `-k`: index = i + 1 − k, clamped to ≥ 0. So `-1` is the
    previous command.
  - Zero:
    - `0`: index = i;
    - `-0` when listing: real_last, which usually is the `fc` command
      itself (**[oracle]**/test: `fc -l -0` listed its own line);
    - `-0` otherwise: invalid.
  - Positive `n`: idx = n − base. If idx < 0 or idx ≥ i (sic: `>=` against
    last_hist), the result is **0 for the first operand, and i for the
    second operand or the execute form**. Otherwise idx.
    - So `fc -l 502 498` lists everything, and `fc -s 48` (out of range)
      re-runs the most recent command (test history5).
- **String**: search backward from index i to 0 for an entry whose line
  starts with the string (byte prefix). Not found means "not found".
  `fc -l one=two ...` gives `no command found`.

---------------------------------------------------------------------------

## 5. History expansion

### 5.1 When it runs

- On each physical line read by the main line reader, only when
  `remember_on_history` is set. It runs before tokenizing (PY:2645-2680),
  and only if:
  - `history_expansion` (`set -H` / `histexpand`) is on;
  - `history_expansion_inhibited` is 0;
  - the line contains the expansion char **or** the quick-substitution char
    anywhere (BH:544-552, 574).
- Interactive shells: on by default (HISTEXPAND_DEFAULT=1, config-top.h).
- Non-interactive shells need **both** `set -o history` and `set -H` (any
  order). `set -H` alone does nothing, because the line hook only runs when
  history is remembered and inhibition is cleared by `set -o history`
  (BH:285-316; SH:1845-1851; flags.c:246-251). Tests histexp1-7 always
  do both.
- `set -H` also re-runs `bash_initialize_history` (§2.1).
- **Not expanded**:
  - here-document bodies: they are read by a separate reader
    (PY:2323-2345), so test histexp7 shows `echo !!` passing through
    untouched in here-docs, even inside `$(...)` and `<(...)`;
  - strings run by `eval`, `-c` and `fc -s`;
  - lines inside an open single-quoted string carried over from an earlier
    line. The quoting state of the parser's open delimiter is passed in
    (PY:2652-2657): the part up to the closing `'` is skipped, and inside
    an open `"` the double-quote rules apply.
- `histchars` (VA:6143-6164):
  - When unset: `!`, `^`, `#`.
  - When set: its first char is the expansion char, the second (if present)
    the quick-subst char, the third (if present) the comment char.
  - Chars not given keep their **previous** values.
  - `histchars=` (empty) makes the expansion char NUL, which **disables
    expansion entirely** (RX:930-934).

### 5.2 Where `!` is (and is not) an expansion

The library scans the line once to find whether any real expansion exists
(RX:952-1080). If there is none, it returns the line unchanged with status 0.
Otherwise a second pass copies and expands. In bash the rules are:

- **Comment char** (`#`): it stops all further expansion on the line when it
  appears outside double quotes and at a word start, meaning position 0 or
  right after one of ` \t\n;&()|<>` (RX:1006-1017, 1196-1208).
  E.g. `echo ok 3 # !1200` is not expanded.
- **Single quotes** suppress expansion until the closing `'`. Inside
  double quotes, `'` is ordinary. After a `$` (that is, `$'...'`) a
  backslash can escape a `'` inside (RX:1052-1063, 1161-1193).
- **Backslash**: outside quotes the next char is copied literally, so `\!`
  is not expanded and the backslash is kept in the output for the parser
  to remove later (RX:1151-1154). Inside double quotes the same applies.
  `"\!"` stays `"\!"`, and the shell then prints `\!`, because bash keeps a
  backslash before `!` in double quotes (tests histexp1: `echo "\!"` prints
  `\!`).
- **Double quotes**: `!` **is** expanded inside `"..."` (non-posix), except
  when the `!` is immediately followed by the closing `"`. In **posix mode**
  `!` inside double quotes is never expanded (bash hook; subst.c:2443-2452).
  - **[oracle]**/test histexp2: in posix mode `echo "#!/bin/bash" !!` turns
    into `echo "#!/bin/bash" set -o posix`.
- **Characters after `!` that prevent expansion**: NUL (end of line), space,
  tab, `\n`, `\r`, `=`, `;`, `&`, `|`, `(`, `)`, `<`, `>` (BH:198). So `!;`,
  `!&`, `!(` (extglob or not), `!=x`, `! ` and a trailing `!` are literal.
  **[oracle]**: `echo !=x !` prints `!=x !`.
- **Bash's veto hook** (BH:215-269). It is called on the output assembled so
  far (with `!c` appended), and it refuses to expand when:
  - the `!` follows `[` and a `]` occurs later on the line (glob
    negation `[!a]`);
  - the `!` follows `${` and a `}` occurs later (indirection `${!var}`);
  - the `!` follows `$` (`$!`);
  - extglob is on, the `!` is at position > 1, followed by `(`, and a `)`
    occurs later. (This is redundant, since `(` is a no-expand char.)
  - the `!` is inside quoting as tracked by a bash-side scanner (below).
- The bash scanner:
  - backslash skips the next char;
  - `'...'` is skipped outside double quotes;
  - backquotes and `$(` reset the double-quote state inside them and
    restore it after;
  - in non-posix mode `"` just toggles a flag, so a `!` inside double quotes
    is still found (except `!"`);
  - in posix mode a whole `"..."` is skipped.

  Net effect: `!` inside `$(...)` or backticks within double quotes is
  expanded. Test histexp3: `echo "!!" "$(echo !!)"` expands both.
- The quick-substitution form applies only when the **first character of the
  line** is the quick-subst char (`^`), and the line is not continuing a
  single-quoted string. The line `^old^new^rest` is treated as
  `!!:s^old^new^rest` (RX:952-961).

### 5.3 Event designators (RX:530-565, 138-311)

`!` is followed by:

- **`!`**: the last entry (`history_length − 1`).
- **One of `:` `$` `*` `%` `^`**: an implied `!!`. The word designator
  starts at that char. So `!$`, `!^`, `!*`, `!:2`, `!%` all refer to the
  previous command.
- **`#`**: the event is **the output line built so far** for this line,
  i.e. everything before this `!#` with earlier expansions already applied.
  Word designators and modifiers may follow. **[oracle]**: `echo !#:1`
  gives `:1: bad word specifier` (the text so far was just `echo `).
- **`-N`** (only when a digit follows the `-`): number = base + length − N.
- **`N`** (digits): absolute number N. If out of range:
  `!N: event not found`. `!0` is always not found when base ≥ 1.
- **`?str?`** or **`?str`** (the closing `?` can be omitted at end of
  line): the most recent entry *containing* str.
  - Entries are searched newest to oldest. Within a line the match is the
    **rightmost** occurrence (RS:74-200).
  - An empty str reuses the previous `?` search string; if there was none,
    `event not found`.
  - On success it records the search string, and the "matched word" `%`:
    the token containing the match position, using the tokenizer in §5.4.
- **`str`**: the most recent entry *starting with* str. The string ends at
  the first of:
  - whitespace, newline, or `:`;
  - `-` when it is not the first char (so `!shopt-1` is event `shopt` with
    designator `-1`);
  - `^`, `$`, `*`, `%`;
  - `;`, `&`, `(`, `)`, `|`, `<`, `>`;
  - the enclosing double quote, when inside `"..."`.

  Multibyte chars are skipped whole.
- Searches start at the end of the current list. The line being expanded is
  not in the list yet. For continuation lines, the partial compound entry is
  hidden (§1.5.2).
- On failure the error is the text from the `!` up to where scanning
  stopped, plus `: event not found`. E.g. `!nosuch: event not found`, and
  `!': event not found` for `echo "'!'"` (test histexp4: inside double
  quotes `'` is ordinary, and the search string stopped at `"`).

### 5.4 Word designators (RX:1313-1480)

The event line is split into words by `history_tokenize` (RX:1483-1687),
which mimics the shell's tokenizer:

- Whitespace and newlines separate words. A word starting with the comment
  char ends tokenization: the rest of the line is ignored for word
  purposes.
- Operators are separate words:
  - `;` `&` `|` `<` `>` alone;
  - doubled `;;` `&&` `||` `<<` `>>`, plus `<<-` and `<<<`;
  - `>&`, `<&` followed by digits and an optional `-`;
  - `&>`, `>|`;
  - `(` and `)` alone;
  - a run of adjacent delimiter characters.
- A leading digit run directly followed by `<` or `>` is part of the
  redirection word: `2>/dev/null` gives words `2>` and `/dev/null`. Test
  histexp: `!shopt*` produced `a b c d 2> /dev/null`.
- Quotes (`'`, `"`, `` ` ``) group, with backslash rules like the shell's.
- `$(...)`, `<(...)`, `>(...)`, `@(...)`, `!(...)`, `?(...)`, `+(...)` and
  `*(...)` are kept as one word with nesting, so `echo $((1+2))` has 2
  words (tests histexp, histexp5).

Designators, after an optional `:`:

- `%`: the word matched by the last `?str?` search, or empty.
- `*`: words 1..last, or empty if there is only one word. Never an error.
- `$`: the last word. For a one-word line that is word 0.
- `^`: word 1. `n`: word n, digits only, and **only after `:`** (`!!2` is
  `!!` followed by a literal `2`).
- `x-y`: words x through y. `-y` means `0-y`, `x-$`, `x-^`.
- `x*` means `x-$`. `x-` means `x` through the second-to-last word.
  Any other char after `-` ends the designator.
- Selected words are joined with **single spaces**, whatever the original
  spacing was.
- If the range is impossible (past the end, or first > last), the error is
  `<designator text>: bad word specifier`. The text runs from where the
  designator began, including any `:`. E.g. `:5: bad word specifier`
  **[oracle]**.
- With no designator the whole event line is used.

### 5.5 Modifiers (RX:592-865)

Each modifier is `:` followed by a letter. They are applied left to right to
the selected text. Before the letter there may be a prefix: `g` or `a`
(global) or `G` (each word).

- `h`: cut at the last `/`, keeping the head. Unchanged if there is no `/`.
- `t`: keep what follows the last `/`. Unchanged if there is no `/`.
- `r`: cut at the last `.`, anywhere in the text, even inside a directory
  name: `/a/b.c/d` with `:r` gives `/a/b`.
- `e`: keep the last `.` and what follows it (`.c`). Unchanged if there is
  no `.`.
- `p`: print, don't execute (§5.6).
- `q`: wrap the result in single quotes, writing each embedded `'` as
  `'\''`.
- `x`: like `q`, but break at blanks and newlines. Each whitespace char c
  becomes `'c'`, so `a b` turns into `'a' 'b'`, and `'` becomes `'\''`.
  Only the last of `q`/`x` takes effect. Quoting is applied after all
  modifiers.
- `s/old/new/`:
  - Any delimiter character may be used. The final delimiter is optional at
    end of line.
  - A backslash before the delimiter quotes it, in both old and new.
  - `&` in new means old; `\&` is a literal `&`.
  - An empty old means "previous old", else the last `?` search string.
  - If there is no old at all: `<mods>: no previous substitution`.
  - If old is longer than the text, or not found: `<mods>: substitution
    failed`.
  - Only the first occurrence is replaced.
  - The old/new pair is remembered for `&` and later `s`.
  - If `s` is the last char of the line (no delimiter), the modifier is
    silently ignored.
- `&`: repeat the previous substitution. With no previous one:
  `:&: no previous substitution` **[oracle]**.
- `gs` / `g&`: replace every occurrence (non-overlapping, left to right).
  It fails only if there was no occurrence.
- `Gs` / `G&`: in each whitespace-separated word, replace the first
  occurrence. **[oracle]**: `xa xa xa` with `Gs/a/Q/` gives `xQ xQ xQ`.
- Any other letter: `<letter>: unrecognized history modifier`, quoting just
  that letter (e.g. `z: unrecognized history modifier` **[oracle]**).
- The `<mods>` text in errors runs from the first modifier's `:` to the
  failure point, e.g. `:s/x/y/: substitution failed` **[oracle]**.

### 5.6 Results, printing, and what goes into history (BH:562-651; PY:2661-2725)

`history_expand` returns −1 (error), 0 (unchanged), 1 (expanded), or 2 (at
least one `:p` anywhere on the line; the whole line becomes print-only, test
histexp `echo !:2:p ; echo !$`). Bash then does:

- **Error (−1)**:
  - Print `NAME: [line N: ]<message>` to stderr. In interactive shells NAME
    is `bash`; in scripts it is `script: line N: `. Example:
    `bash: !nosuch: event not found`.
  - The line is **discarded**: not executed, not added to history.
    `current_command_line_count` is decremented.
  - **$? is unchanged** **[oracle]**: `(exit 3)`, then a failed expansion,
    then `echo $?` printed 3.
  - The reader prompts again with the *current* prompt (PS2 if inside a
    multi-line command) and reads a new line. The multi-line command
    continues.
  - In a script the next line is read and execution continues.
  - With `shopt -s histreedit` and readline active, the original failed
    line is preloaded into the next readline buffer (BH:618-619).
- **Print-only (2)**:
  - Print the expanded line to stderr (always, even with histverify).
  - If it is non-empty, pass it to the normal add logic, so the `:p`
    result **is added to history** **[oracle]**: `echo !!:p` added
    `echo echo two words`.
  - Not executed. The prompt is reprinted.
- **Expanded (1)**:
  - If `histverify` is on, the expanded line is loaded into the next
    readline buffer. Nothing is printed, executed or added. Without
    readline (st_stdin input), the line is silently lost.
    **[oracle]** quirk: with `bash -i` on a pipe, the preloaded text was
    glued in front of the next input line.
  - Otherwise print the expanded line to stderr, then add it to history and
    parse it.
- **Unchanged (0)**: add the original line.
- The text added to history is always the **expanded** line.
- An expansion that produces an empty line is read as an empty line
  (line count decremented).

**[oracle]** interactive transcript of the stderr echo (`>` is PS1):

```
> echo !$:h          (after: echo /a/b.c/d.e f)
echo f ...           <- expanded line echoed to stderr, then executed
> ^2^3
echo echo 3 words
```

### 5.7 Readline-only bits (BL)

These are not needed for the batch semantics.

- `M-^` history-expand-line, history-and-alias-expand-line, and
  dynamic-complete-history, which tokenizes the history with
  `history_tokenize`.
- `edit-and-execute-command` (`C-x C-e` / vi `v`) adds the current buffer
  plus a dummy empty entry, then runs
  `fc -e "${VISUAL:-${EDITOR:-emacs}}"` (vi: `...:-vi`). With a numeric
  argument it runs `fc -e ... N` (BL:936-1000).
- Readline sets `data` on edited history lines, which produces the `*`
  mark in `history` and `fc -l`.

---------------------------------------------------------------------------

## 6. Options and variables summary

| Control | Default | Effect |
|---|---|---|
| `set -o history` | on if interactive | §1.1. Turning it on runs load_history if nothing was saved yet this session |
| `set -H` / `-o histexpand` | on if interactive | §5.1. Scripts also need `-o history` |
| `shopt cmdhist` | on | §1.5 |
| `shopt lithist` | off | §1.5. Newlines instead of `;` |
| `shopt histappend` | off | §2.5. Forces append at exit, and affects `history -n` session accounting |
| `shopt histreedit` | off | §5.6. Readline only |
| `shopt histverify` | off | §5.6 |
| HISTFILE | `~/.bash_history` (interactive only) | §2. Readonly in rbash |
| HISTSIZE | 500 (set after rc files, interactive or on `set -o history`) | §1.2 |
| HISTFILESIZE | = HISTSIZE at load | §2.3. Assigning it truncates the file immediately |
| HISTCONTROL | unset | §1.3 |
| HISTIGNORE | unset | §1.4 |
| HISTTIMEFORMAT | unset | Set (even empty): write timestamps, enable multiline reads, set the comment char to `#` if it was NUL. Non-empty: show timestamps in `history` |
| histchars | unset (`!^#`) | §5.1 |
| HISTCMD | dynamic | §1.6 |

- Unsetting HISTFILE means nothing is saved at exit, and `-a/-n/-r/-w`
  without a filename print the "parameter null or not set" error (§3.7).
- Unsetting HISTSIZE unstifles the list.
- All of these hooks fire on assignment, unset, and `local` scopes, like any
  special variable (VA:5781-5837).

---------------------------------------------------------------------------

## 7. Upstream tests (no content reproduced)

The tests run by `run-history` and `run-histexp` diff against
`history.right` / `histexp.right`. Unless noted, each runs non-interactively
with `set -o history`. Expected error lines use the script's
`NAME: line N: builtin: msg` prefix.

### history.tests (uses `history.list`, a 4-line history file)

- Bad options for `history` and `fc` (usage lines), and the `-r -w`
  exclusivity error.
- `history -c`, then `-r`/`-n` on `/dev/null`: the list must stay empty.
- With HISTFILE=history.list, HISTCONTROL=ignoreboth, an `&:history*:fc*`
  HISTIGNORE, HISTSIZE=32 and cmdhist, enabling `set -o history` loads the
  file. Then:
  - `history`, `fc -l`, `fc -nl`, `fc -lr`, `fc -nlr` formats;
  - `history -s`, and `history -p` with and without args;
  - `history -a` into a fresh file, then `cat`;
  - `history -w`.
- A multi-line `for` loop must be one entry. The tab before `:` is kept and
  joined after `do`.
- `set -H` then `!!` and `!e` in a script: the echo of the expanded line,
  and dedup through HISTIGNORE `&`.
- `fc -l N`, `fc -l N M`. `fc -l` with `pat=rep`-looking operands gives
  `no command found`. `history 4`.
- `alias r='fc -s'`: `r a=x` (global replace), multiple pairs,
  `r cc` failing with `no command found`.
- `fc -e cat` and `fc -e cat -1` must produce identical output: the editor
  output, the verbose echo, and the execution.
- `set +o history` then `shopt -qo history` reports 1.
- Sub-scripts:
  - **history1.sub**: a here-doc command saved as one entry including its
    trailing newline; `fc -s cat` re-runs it; `fc -s -1` returns the
    re-run command's status (42).
  - **history2.sub**: `fc -nl -1` inside a command substitution versus at
    top level (the "rh" adjustment for comsubs).
  - **history3.sub**: `history -d` ranges: `2-4`, `6--1`, and out-of-range
    and malformed ranges (`16-40`, `1-200`, `-20-50`, `1--50`, `5-0xaf`);
    the `@42` invalid-number and numeric-argument errors.
  - **history4.sub** (**interactive, needs readline**): writes a history
    with a multi-line quoted entry, then pipes keystrokes (`C-r` search,
    `C-o` operate-and-get-next, `C-p`) into `bash --norc -i` under
    different HISTSIZE values, including empty and small. It exercises
    stifling and operate-and-get-next across the HISTSIZE boundary.
  - **history5.sub**: `fc` number semantics: `-0` errors, `fc -l 0`,
    `fc -l -0`, `fc -s 0`, out-of-range first/last pairs with a small
    HISTSIZE, `fc -e - 48` and `fc -s -- -42` clamping.
  - **history6.sub**: `history -d -1` and `-2--1` with HISTSIZE=4 and
    `HISTIGNORE=history*`, so the deleting command is not itself in the
    list; numbering after deletes under stifling.
  - **history7.sub** (**interactive**, `bash --norc -in` with here-string
    input): HISTFILESIZE truncation at exit with and without HISTTIMEFORMAT
    (exact line counts), and HISTFILESIZE=0.
  - **history8.sub**: `history -d 2`, then out-of-range `72` and `-72`.
  - **history9.sub**: `history -r` of a timestamped file with multi-line
    entries and blank lines while HISTTIMEFORMAT is empty-but-set
    (multiline grouping, leading-blank skipping, kept interior blanks).

### histexp.tests (sets `histchars='!^#'`, LC_ALL=C)

- `history -p '!!'`, and `history -p '!!:z'` giving the
  `history expansion failed` message.
- `history -a`/`-w`/`-s`.
- Then `set -H` and a long sequence of expansions in the script. Each line
  prints its expanded form to stderr (merged into the .right) and then its
  output:
  - events `!!`, `!e`, `!?ch?`, `!-2`, `^2^8`, `!2`;
  - word designators `:0:t`, `:0:h`, `!?d?:5`, `2-$`, `2-4`, `3*`, `:*`,
    `2-`;
  - modifiers `:r`, `:e`, `:q`, `:x`;
  - substitutions `s///`, `gs`, `&` in replacement, `g&`, alternate
    delimiter `+`, `:p`;
  - `%` after `!?.o?`, `!!:0 !?.h?:%:q`, `!!:-$`, `!:-$`.
- Single quotes and backslash inhibit expansion; other expansions on the
  same line still happen.
- Bash special cases that must not expand: `[!A-Z]` in a case pattern,
  `${!var}`, and `!` after the comment char.
- Tokenization of `2>/dev/null` (`!shopt-1`, `!shopt*`).
- A `:p` anywhere on the line making the whole line print-only.
- `$((1+2))` as a single word.
- Sub-scripts:
  - **histexp1.sub**: `!` in single quotes, double quotes, bare, and inside
    `$(...)` in each quoting context; `"\!"`; the post-5.2 stop characters
    `!;`, `!&`, `(echo !)`, `!>`, `!<`.
  - **histexp2.sub**: posix mode: `!` inside double quotes is not
    expanded, but outside it is.
  - **histexp3.sub**: `!!` inside `$(...)` and backticks, combined with
    single and double quotes and `eval`.
  - **histexp4.sub**: `<(...)` with `!!`, backticks, `"$(echo "!")"`, and
    `'!'` inside double quotes producing `!': event not found`.
  - **histexp5.sub**: `$(...)`, `<(...)` and extglob groups at mid-word and
    word start are single words for `!:*`.
  - **histexp6.sub**: `!!` and `!-1` on a continuation line of a multi-line
    `for` refer to the command before the loop; checks the resulting single
    history entry.
  - **histexp7.sub**: here-document bodies inside `$(...)` and
    `<(...)` (quoted and unquoted delimiters) are not history-expanded.

None of the histexp tests are interactive. Only history4 and history7 start
interactive shells, and only history4 needs readline key handling.
