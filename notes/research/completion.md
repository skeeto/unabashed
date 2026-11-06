# Bash 5.3 default completion: semantics digest

Purpose: enough detail to clone bash 5.3's non-programmable completion
(readline's completion engine plus bash's word classifier, generators,
quoting and listing) in `edit.cpp`, and to match bash's screens key for
key, without reading the GPL source. Everything below is a paraphrase in
our own words; there is no transliterated code. The only verbatim text is
short user-visible strings (`Display all %d possibilities? (y or n)`,
`--More--`). readline.md §9 is the short summary this file replaces;
emacs.md is the companion for the rest of the keymap and has the general
model (dispatch loop, `last_func`, undo, bell) that this file assumes.

Citations are relative to the `bash-5.3/` root of `bash-5.3.tar.gz`:

- RL = `lib/readline/`: complete.c (most of this file), display.c,
  bind.c, util.c, text.c, terminal.c, emacs_keymap.c.
- BL = `bashline.c`. `DOC` = `doc/bash.1`.
- Not available here, so described from the oracle only: `subst.c`
  (`char_is_quoted`, `skip_to_delim`, `unclosed_pair`), `lib/sh/shquote.c`
  (the backslash, single and double quoting helpers), `variables.c`
  (`all_variables_matching_prefix`), `bracecomp.c` (M-{), `pcomplete.c`.

Claims marked **[oracle]** were checked against real bash 5.3.0 (the
`unabashed/bash53` container, `tools/edrecord`, TERM=xterm, LANG=C.UTF-8,
PATH=/usr/local/bin:/opt/fakes/bin, cwd /home/oracle, `--noprofile
--rcfile`). The screens were read with the vterm renderer, which trims
trailing blanks, so an appended space shows only as the cursor column.
Bell counts in the dumps are cumulative over a session.

---------------------------------------------------------------------------

## 0. Model

Completion is one engine (`rl_complete_internal`, RL/complete.c:2070-2288)
driven by a mode character, `what_to_do`, plus a handful of hooks that
bash fills in. One attempt runs these stages:

1. Defaults reset (RL/complete.c:567-582): "results are filenames" off,
   "quote filenames" on, "full quoting" off, suppress-append and
   suppress-quote off, append character = space, mark-symlinked-dirs from
   the user variable.
2. Save a copy of the line (to decide afterwards whether it changed).
3. Find the word: `start` and the opening quote, if any (§3). `end` is
   point. Only the text before point is the word; text after point is
   never looked at or replaced.
4. Generate matches (§4, §5). The result is either nothing, or an array
   whose slot 0 is the replacement text (the common prefix, or the single
   match) and slots 1.. are the matches.
5. Post-process (§6.1): sort, drop duplicates, apply the ignore filter
   (FIGNORE and friends), recompute the common prefix if the filter
   removed some.
6. Act on `what_to_do` (§2): insert the prefix and maybe an append
   character (§6), list (§7), insert all, or ring the bell.
7. `completion_changed_buffer` := the line differs from the saved copy
   (RL/complete.c:2277-2281). On failure paths it is forced to 0
   (2133, 2150).

State that survives between keys (all private to the completer):

| name | meaning |
|------|---------|
| `completion_changed_buffer` | the last completion attempt changed the line |
| `last_completion_failed` | the last attempt produced no matches (set on failure, cleared when a match list with a non-empty slot 0 is produced, RL/complete.c:2156) |
| menu state | §8 |

`last_func` (emacs.md §0) is what makes TAB TAB work.

Signals: a SIGINT during generation or listing frees the list and aborts
the attempt (RL/complete.c:524-536, 2246-2272). Out of scope for goldens.

---------------------------------------------------------------------------

## 1. Bindings and commands

Bash's emacs map after `initialize_readline` (RL/emacs_keymap.c, BL:527-
591). **[oracle]** `bind -p` in the container lists exactly these:

| key | command | what_to_do / generator |
|-----|---------|------------------------|
| TAB (C-i) | complete | TAB/`?`/`!`/`@` (§2), bash's classifier (§4) |
| ESC ESC (M-C-[) | complete | the same function as TAB, so TAB then ESC ESC counts as "twice in a row" (RL/emacs_keymap.c:346) |
| M-? and M-= | possible-completions | `?` (RL/emacs_keymap.c:386, 388) |
| M-* | insert-completions | `*` (RL/emacs_keymap.c:363) |
| M-/ | complete-filename | mode from §2.2, filename generator only, word breaks reduced to blank and quotes (BL:3908-3939) |
| C-x / | possible-filename-completions | `?`, same setup |
| M-~ | complete-username | readline binds M-~ to tilde-expand; bash replaces it only if it is still that (BL:564-568) |
| C-x ~ | possible-username-completions | `?` |
| M-@ | complete-hostname | hostname generator |
| C-x @ | possible-hostname-completions | `?` |
| M-$ | complete-variable | variable generator |
| C-x $ | possible-variable-completions | `?` |
| M-! | complete-command | command-word generator |
| C-x ! | possible-command-completions | `?` |
| M-{ | complete-into-braces | bracecomp.c (§9.5) |
| M-TAB (M-C-i) | dynamic-complete-history | readline's default is tab-insert; bash rebinds it if still that (BL:587-591) |
| M-g | glob-complete-word | §9.6 |
| C-x * | glob-expand-word | `*` with the glob generator |
| C-x g | glob-list-expansions | `?` with the glob generator |
| M-& | tilde-expand | not completion; stays readline's |

Unbound but nameable: `menu-complete`, `menu-complete-backward` (§8),
`delete-char-or-list`, `dabbrev-expand`, `export-completions`,
`old-menu-complete`.

The "specific" commands (M-/ M-~ M-@ M-$ M-! M-g) choose their mode with
`rl_completion_mode(self)` (RL/complete.c:500-510): `?` if the previous
command was the same function and the line was not changed, else the
show-all variants, else TAB. So M-! M-! lists, like TAB TAB. They replace
the generator for one call and switch off bash's classifier
(`bash_specific_completion`, BL:4097-4118); the ignore function in force
(FIGNORE) stays.

---------------------------------------------------------------------------

## 2. The TAB state machine

### 2.1 complete (TAB) (RL/complete.c:457-476)

In order:

1. `disable-completion` on: behave as self-insert of the key with the
   numeric argument (a literal TAB, drawn as blanks to the next tab stop
   counted from screen column 0 including the prompt). **[oracle]** `ls
   fi` TAB `z` shows `$ ls fi  z` with the z at column 8.
2. If `last_func` is `complete` and `completion_changed_buffer` is 0:
   mode `?` (list).
3. Else if `show-all-if-ambiguous`: mode `!`.
4. Else if `show-all-if-unmodified`: mode `@`.
5. Else mode TAB.

"The previous command was complete" means exactly: the last dispatched
command (emacs.md §0 rules: not a prefix key, not digit-argument) was the
`complete` function, through any key bound to it. A cursor motion, a
self-insert, C-g, M-?, or any other command in between breaks the chain.
`completion_changed_buffer` is a single flag shared by all completion
commands; it only matters here because of the `last_func` test.

The consequence that surprises: when the first TAB inserted something
(a longer common prefix), the second TAB is another plain TAB (which now
inserts nothing and rings), and only the third TAB lists. **[oracle]** with
`file.c file.h fileonly.txt`: `ls fi` TAB gives `ls file` + bell; TAB
gives bell, no list; TAB lists. When the first TAB changes nothing
(`ls f` with f1..f99), the second TAB lists.

The numeric argument is ignored by TAB (except under disable-completion).

### 2.2 The modes (RL/complete.c:2161-2244)

| mode | used by | action |
|------|---------|--------|
| TAB | complete | if slot 0 is non-empty, insert it (§6.2) as SINGLE when there is one match, else MULT. Then: one match: append (§6.4); several: bell (except vi mode). |
| `!` | show-all-if-ambiguous | one match: insert + append. Several: insert slot 0 only if it is at least as long as the (dequoted) typed word, then list. No bell. |
| `@` | show-all-if-unmodified | like `!`, but list only if slot 0 equals the typed word (nothing was added); if something was added, insert it and stop, with no bell. |
| `?` | TAB TAB, M-?, C-x x | list (§7). Special case: if the previous attempt failed and this one finds exactly one match, insert it and append instead of listing (2215-2223). possible-completions clears the failed flag before starting, so the case is reachable only from TAB TAB. |
| `*` | M-*, C-x * | replace the word with every match, each quoted and followed by a space, in one undo group (RL/complete.c:1991-2024). With one match, that match plus a space. |
| `%` | menu completion | list (used for the show-all display in §8) |
| `$` | export-completions | §9.8 |

The "typed word" compared in `!`/`@` (`nontrivial_lcd`, 2111-2122) is the
word dequoted by bash's dequoting function when the word contained any
quoting; otherwise the raw text.

**[oracle]** show-all-if-ambiguous: `ls fi` TAB prints the list
immediately with no bell, and the screen line *above* the list still
shows `$ ls fi`: the prefix was inserted into the buffer, but no redisplay
happens before the list is printed, so the old line stays on screen; the
redrawn prompt below shows `$ ls file`. show-all-if-unmodified: `ls fi` TAB
gives `ls file`, no bell, no list; a second TAB lists. `ls x` (`xa1 xb2`)
lists on the first TAB.

### 2.3 Bells

Each `rl_ding` is one BEL (emacs.md §0 for bell-style). In completion:

| situation | bells |
|-----------|-------|
| no matches (generator returned nothing, or the ignore filter removed everything) | 1, every mode (2129-2154) |
| TAB mode, several matches remain after inserting the prefix | 1 |
| `!`/`@` modes, several matches | 0 |
| `?` mode with matches | 0 |
| y/n prompt or `--More--`: an unrecognised key | 1 per key, and keep asking (RL/complete.c:614) |
| C-g at either prompt | 1 (abort, §7.5) |
| menu-complete wrapping to the original text | 1 (§8) |

**[oracle]** `ls nothing` TAB TAB: 2 bells, no list.
`ls zz` TAB M-?: 2 bells.

### 2.4 completion-query-items

Default 100 (RL/complete.c:307). Setting it parses an integer; negative
becomes 0, an empty value means 100 (RL/bind.c:2139-2151). 0 means never
ask. The question is asked when the number of matches to list (after
duplicates are removed) is >= the value (RL/complete.c:1795).

---------------------------------------------------------------------------

## 3. Finding the word (RL/complete.c:1137-1267)

Inputs: the line, point, the break set (bash: COMP_WORDBREAKS), the
completer quote characters (bash: `'` and `"`, BL:615), the special
prefixes (bash: `$` and `@` with hostcomplete on, `$` with it off,
BL:389-394) and bash's `char_is_quoted`.

Break set: bash's default COMP_WORDBREAKS is blank, tab, newline, `"`,
`'`, `@`, `>`, `<`, `=`, `;`, `|`, `&`, `(`, `:` (BL:313). With
hostcomplete off the `@` is removed (BL:314, 378-447). Assigning
COMP_WORDBREAKS replaces the set. **[oracle]** `COMP_WORDBREAKS=' '` makes
`ls x=fileo` TAB fail: the word is `x=fileo`. `shopt -u hostcomplete`
makes `ls x@fileo` a single word. `$`, `{`, `` ` ``, `\`, `}` and `)` are
not breaks.

Algorithm, with point = `end`:

1. Forward scan from 0 to `end` for an unclosed quote. A backslash (not
   inside single quotes) skips the next character and records "found a
   backslash". Outside quotes, `'` or `"` opens a quoted run, records its
   kind, and sets the tentative start to just after it. Inside a run,
   only the same quote character closes it, which resets the tentative
   start to `end`. Backslash does not escape inside single quotes.
2. If the scan ended inside an open quote, the word starts just after
   that quote, and `quote_char` is that quote. Break characters inside the
   run do not matter.
3. Otherwise scan backwards from `end` for a break character. A break
   character is skipped (treated as part of the word) only when the line
   before point contains quoting somewhere (step 1 found any quote or
   backslash) *and*
   `char_is_quoted` says that position is quoted. So `a\ b` is one word,
   and a closing quote of a closed quoted run is part of the word.
4. If the scan stopped on a break character (not at end of line), and it
   is a word break per the same quoted test: if it is `'` or `"` and the
   word after it is at least 2 characters long, remember it as the
   *delimiter* (§6.4); then step past it, unless it is a special prefix
   (`$`, `@`), which stays in the word. (Since `$` is not in the break
   set, only `@` actually stays: `x@lo` gives the word `@lo`.)

Results: `start`, `quote_char` (the open quote or none), `found_quote`
(bit set: backslash / single / double seen anywhere on the line before
point, in earlier words too: step 1 scans from position 0), and
the delimiter. Readline stores `quote_char` and `found_quote` where bash's
hooks read them.

**[oracle]** observations of the resulting word:

| line before point | word | result |
|-------------------|------|--------|
| `ls x=fi` | `fi` | `x=file...` |
| `ls a:fi` | `fi` | |
| `echo "fi` | `fi`, quote `"` | |
| `: x"A` | `A`, quote `"` | `: x"A z" ` (the quote is absorbed, §6.3) |
| `echo a"fileo` | `fileo`, quote `"` | bell (no `fileo*`) |
| `echo "a"fileo` | `"a"fileo` (whole) | `echo afileonly ` (the quotes are dropped!) |
| `echo 'a'fileo` | `'a'fileo` | `echo afileonly ` |
| `echo "a"` with `a b`, `afileonly` | `"a"` | `echo a` + bell (quotes stripped, LCD `a`) |
| `: A\ ` (backslash-space) with `A y`, `A z` | `A\ ` | unchanged + bell, then TAB lists `A y  A z` |
| `ls m\ z` TAB TAB | `m\ z` | lists `m z1  m z2` |

`char_is_quoted` itself (subst.c) is not in this tree. The behaviour seen
is consistent with "inside a quoted run, including its closing quote, or
the character after a backslash".

The empty word: at point 0 the scan is skipped and the word is empty.
After a break character at point the word is empty.

---------------------------------------------------------------------------

## 4. Choosing the generator: bash's classifier

Readline first calls bash's `attempt_shell_completion(text, start, end)`
(BL:1587-1825). If it returns matches, they are used. If it returns
nothing and did not set "attempted completion over", readline falls back
to its own filename generator (§5) on the same text (RL/complete.c:1280-
1301). The specific commands (§1) bypass all of §4.

Every attempt first resets: ignore function = FIGNORE filter, quote
characters = the default set (§6.3), full quoting on, filename hooks,
sorting on (BL:1598-1605).

### 4.1 Command position (BL:1611-1667)

Let `ti` = the index before `start`, skipping blanks (space and tab).
If the character there is an opening `"` or `'`, remember it as `qc` and
skip it and blanks before it too.

- `ti < 0` (only blanks, or a quote, before the word): command position
  when the prompt being read is PS1 (or the parser says a command word is
  expected, for PS2 inside a compound command).
- The character at `ti` is one of `;`, `|`, `&`, `{`, `(`, `` ` `` and
  not quoted: command position, except
  - `(` preceded by one of `?*+@!` when extglob is on (a pattern), and
  - `&` after `<` or `>` (`>&`, `<&`), `|` after `>` (`>|`), and `{`
    after `$` (`${`): these mean "not first word" (BL:1444-1461).
- Anything else: not command position (yet; 4.2 may change that).

`(` after something that is not a separator and not `$`, `<`, `>` makes
the completion invalid: no matches and no filename fallback (BL:1561-
1582).

A backquote at `ti` that is closed before point (the word is not inside
an open backquote) cancels command position (BL:1665-1667).

Reserved words do **not** create a command position: the classifier only
looks at the single character before the word. **[oracle]** with a file
`ecfile`: `if ec`, `time ec`, `! ec`, `case ec`, `for x in ec` all give
`...ecfile `, while `: ; ec`, `: | ec`, `: && ec`, `(ec`, `: $(ec`,
`{ ec`, and `   ec` (leading blanks) give `echo `. `: >| ec` gives
`ecfile`.

### 4.2 Assignments before the command (BL:1679-1813)

Bash's programmable-completion block runs even when no compspecs exist
(`have_progcomps` false), as long as the word is not already known to be
in command position or is empty, and the prompt is PS1. Its side effects
on command position are what matter here:

- It finds the start of the current simple command (after the last
  unquoted `;`, `|`, `&`, `{`, `(`, `` ` ``; `>|` is not a separator; a
  `{` counts only where a reserved word may appear), then skips leading
  words that are assignments (`NAME=...`), and takes the next word as
  the command name.
- If the word being completed is that command name, after one or more
  assignments: command position. **[oracle]** `FOO=1 ec` gives
  `FOO=1 echo `.
- An empty word right after only assignments: command position.
- Completing *inside* an assignment's value: `=` is a break, so the word
  is the value, and it is not command position. **[oracle]** `FOO=ec`
  gives `FOO=ecfile`.

With compspecs present (M8) this block also runs `complete -D/-E/-I` and
the command's compspec; out of scope here.

### 4.3 bash_default_completion (BL:1828-1958)

Tried in order; the first that yields matches wins:

1. **`$` words.** The word starts with `$`:
   - `$(` (not inside single quotes) or a `${` nofork form: command
     substitution (§4.4). In practice `(` is a word break, so `$(ec`
     arrives as `ec` in command position (4.1), not here.
   - Otherwise variables (§4.5). If there is exactly one match and its
     expansion names a directory, the append character becomes `/`.
   Variable completion is also done inside single quotes. **[oracle]**
   `echo '$xvar` TAB gives `echo '$xvar'/` (xvar=/home/oracle/dd).
2. **`~` without a `/`**: user names (§4.6).
3. **`@`** (hostcomplete on): host names (§4.7).
4. **Command position**: command words (§4.8). Exceptions:
   - `no_empty_cmd_completion` on and the word is empty: no matches, and
     the ignore function becomes "ignore everything", so the filename
     fallback is emptied too: bell. **[oracle]** TAB TAB on an empty line
     or a line of blanks: 2 bells, nothing listed.
   - No command matched: the ignore function becomes "keep directories
     only" for the filename fallback (BL:1896-1897).
   - One match that is a directory relative to cwd (not absolute, not
     containing `/`, not starting with `~`), found while `.` is not in
     PATH: suppress the append character and treat the result as not a
     filename (BL:1898-1906). Same when three identical matches arrive
     before deduplication (1908-1924).
5. **Glob patterns**: if the word contains unquoted glob characters
   (`*`, `?`, `[`, and extglob forms), expand it as a pattern (§9.6).
   Under TAB mode, more than one match discards the result (and the
   filename fallback then fails on the literal pattern). Under `!`/`@`
   several matches suppress the append. Under `?`, several matches with
   the word ending in `/` suppress the append. **[oracle]** `echo d*` TAB
   gives `echo dd/`; `echo dd/f*` (f1 f2) TAB rings; TAB again lists
   `f1  f2` and leaves `dd/f*` alone; `ls *.c` TAB gives `ls file.c `.
6. Nothing: readline's filename generator (§5) with the ignore function
   chosen above (FIGNORE by default).

### 4.4 Command substitution words (BL:2482-2564)

For a word starting with a backquote that is open at both ends of the
word (or in command position), or `$(`: drop the opener, find the last
blank or separator in the rest, and complete the tail as a command word
(no blank before it) or a filename (after a blank). Each match gets the
original prefix back. Closing quotes are suppressed; the append character
is suppressed unless the single match is a directory (then `/`). Filename
quoting is turned off when the list is exhausted. **[oracle]** ``: `ec``
TAB gives ``: `echo`` with no space.

### 4.5 Variables (BL:2568-2622)

The prefix after `$` (and after `${`) is matched against the names of
set shell variables (`all_variables_matching_prefix`; **[oracle]** a
`declare`d but unset variable is not offered). Matches are returned as
`$NAME` or `${NAME}`. They are not filenames: no quoting, the listing
shows them whole (`$xvar  $xvas`). M-$ uses the same generator on a word
with or without the `$` (**[oracle]** `echo xv` M-$ gives `echo xva` +
bell).

The `/` rule of 4.3: bash expands the single match as a filename would
be (`bash_filename_stat_hook`, §5.4) and tests for a directory.
**[oracle]** `echo $xvar` TAB gives `echo $xvar/` (no space);
`echo "$xvar` gives `echo "$xvar"/` (closing quote, then the slash:
§6.4); `echo $xvar/` TAB completes inside the directory and keeps
`$xvar/f` unexpanded.

### 4.6 User names (RL/complete.c:2377-2425)

`~prefix` is matched against the password database in its order
(`getpwent`); results are `~name`. Because the word starts with `~`, they
are filenames: the listing marks names whose home directory exists with
`/` (tilde expansion, then stat), and a single match gets `/` appended
when its home exists. **[oracle]** `echo ~ora` TAB gives `echo ~oracle/`;
`echo ~` TAB TAB lists 19 users sorted, `~_apt` first, `~root/` marked,
`~nobody` not (home /nonexistent). No user database on native Windows
(the function returns nothing for `_WIN32`).

### 4.7 Host names (BL:749-931, 2626-2661)

The list is read once, from `$HOSTFILE`, else `hostname_completion_file`,
else `/etc/hosts`. Lines: leading blanks skipped; `#` lines and blank
lines skipped; `$include FILE` recurses; a leading address (a word
starting with a digit) is skipped; every other blank-separated word up to
a `#` is a name. Duplicates stay in the list (dedup happens later).
Matches are `@name`. **[oracle]** `echo @loc` TAB gives `echo @localhost `;
`echo x@` TAB TAB lists `@::1 @fe00::0 @ff00::0 @ff02::1 @ff02::2
@ip6-allnodes ...` (a word starting with a digit such as `::1` in a
non-address position counts as a name).

### 4.8 Command words (BL:2005-2478)

If the word contains a `/` (an "absolute program": any slash) or is a
glob pattern: only files. The word is tilde-expanded (and, with
direxpand, directory-expanded), dequoted if it contained quoting, and
completed by the filename generator; a match is kept if it is an
executable file or a directory. A leading `~user` is put back in the
results. **[oracle]** `./te` gives `./texe `; `sub/` TAB gives
`sub/prog ` when `sub/data` is not executable.

Otherwise, in this order, every name that starts with the word:

1. aliases (with completion-ignore-case: case-insensitively),
2. reserved words (always case-sensitive): `if then else elif fi case
   esac for select while until do done in function time { } ! [[ ]]
   coproc`,
3. shell functions (ignore-case honoured),
4. enabled builtins (disabled ones are skipped; case-sensitive),
5. if the word itself names a directory relative to cwd (not absolute,
   no `/`, no leading `~`): the word itself, once,
6. for each PATH element in order (empty element = `.`, `~` expanded),
   files in it that start with the word and are executable files
   (directories in PATH elements are not offered). `.` in PATH is noted
   for the rule in 4.3 step 4.

The results are then sorted and deduplicated by readline (§6.1), so the
order above matters only for which duplicate survives (they are
identical strings). The current directory is **not** searched unless `.`
is in PATH. Executable files in cwd are not offered in command position;
directories are, but only through the fallback when nothing above
matched. **[oracle]** with alias `tfoo`, function `tbar`, dir `tdir`,
executable `texe` in cwd: `t` TAB TAB lists `tail tbar tee test tfoo then
time times touch tr trap true type typeset` (no `tdir`, no `texe`);
`tdi` TAB gives `tdir/`; `tex` TAB rings; `tdir` TAB leaves `tdir` with
no slash and no bell (rule 5 plus the suppress rule of 4.3).

While generating command words bash sets the stat hook to "resolve the
name through PATH" (BL:1961-1981), so `visible-stats` in a command list
shows the type of the program found.

---------------------------------------------------------------------------

## 5. Filename generation

### 5.1 Splitting the word (RL/complete.c:2476-2607)

1. `filename` = the word; `dirname` = the word, or `.` if the word is
   empty.
2. If the word has a `/`, the directory part is everything through the
   last `/` and the filename part is what follows. Otherwise the
   directory is `.` and the filename part is the whole word. (Native
   Windows: `X:` alone is a drive-relative directory, and `//X/` is
   handled; RL/complete.c:2510-2528.)
3. `users_dirname` = the directory part as typed, dequoted if the word
   contained quoting. This is the prefix every result is built on, so the
   user's spelling of the directory (`~/`, `$HOME/`, `../x/`) is kept in
   the inserted text.
4. A leading `~` in the directory is tilde-expanded for opening.
5. Directory hook (bash, BL:3402-3414, 3595-3729): with `direxpand` off
   (default) bash's hook is a *rewrite* hook: it changes only the name
   passed to `opendir`. With `direxpand` on it is a *completion* hook: it
   also replaces `users_dirname`, so the expanded directory is inserted.
   What the hook does:
   - If the directory contains `$` (with a closed `${`/`$(` pair), or a
     closed backquote pair, or starts with `~`, and does not exist
     literally: expand it like a prompt string (no command substitution),
     and note which characters were expansion syntax so they are not
     quoted later (§6.3).
   - Otherwise dequote it.
   - Unless `set -P`: make it absolute against `$PWD` and canonicalize
     (logical `..`); if that fails and `dirspell` and `direxpand` are on,
     spelling-correct it.
   - With direxpand off and a relative name unchanged by
     canonicalization, it is left relative.
6. Without a hook but with quoting in the word, the dequoted directory
   is used (re-tilde-expanded).
7. The filename part is dequoted when the word contained quoting.
8. The filename part and each directory entry go through bash's rewrite
   hook (`fnx_fromfs`: a no-op except on macOS normalization).

**[oracle]** direxpand on: `ls $HOME/fileo` TAB gives
`ls /home/oracle/fileonly.txt `; `ls ~/fileo` keeps `~/` (tilde is not
expanded into the line); direxpand+dirspell: `ls /usr/lcal/b` TAB gives
`ls /usr/local/bin/`. direxpand off: `: $HOME/q/A` TAB gives
`: "$HOME/q/A z" ` (§6.3).

### 5.2 Reading the directory (RL/complete.c:2617-2714)

`readdir` order, one entry per call:

- Empty filename part: every entry except `.` and `..`; hidden entries
  (leading `.`) are skipped when `match-hidden-files` is off.
- Non-empty: an entry matches when it is at least as long and its first
  `len` bytes equal the filename part. With `completion-ignore-case`,
  compare case-insensitively (`tolower`, or `towlower` per character in a
  multibyte locale); with `completion-map-case` as well, `-` and `_` are
  also equal (RL/complete.c:2433-2469, RL/util.c:378-403). `.` and `..`
  *do* match a non-empty filename part: **[oracle]** `ls .` TAB TAB lists
  `./  ../  .edrc  .hid`; `ls ..` TAB gives `ls ../`. match-hidden-files
  only affects the empty part: **[oracle]** with it off, `ls ` TAB TAB
  hides dot files but `ls .` TAB TAB still lists them.
- The result is `users_dirname` + `/` (if it lacks one) + entry, or the
  bare entry when the directory was `.` implicitly (the user typed no
  directory). The entry is the directory's spelling, not the user's: with
  ignore-case the typed case is replaced by the file's.

There is no glob support in the directory part. A directory that cannot
be opened gives no matches.

### 5.3 Case folding of the inserted prefix (RL/complete.c:1465-1508)

With `completion-ignore-case` and several matches, the common prefix is
computed case-insensitively (but not with map-case: `-` and `_` still
differ). Its spelling is then taken from the first match (in sorted
order) whose first `min(len(typed), len(prefix))` bytes equal the typed
(dequoted) word case-sensitively; if none, from the first sorted match.
**[oracle]** with `Makefile`, `makedepend.txt`: `ls ma` TAB gives `ls
make`; `ls MA` gives `ls Make`; `ls mAKEF` gives `ls Makefile `. With
map-case and `foo-bar`, `foo_baz`: `ls foo_b` TAB gives `ls foo` + bell:
the line gets *shorter*, because TAB mode inserts the prefix even when it
is shorter than the word.

### 5.4 Stat hook

Whenever the completer stats a match (append character, listing marks),
bash's stat hook first expands `$`/backquote words (if they don't exist
literally) and canonicalizes against `$PWD` (BL:3516-3588).

---------------------------------------------------------------------------

## 6. Inserting

### 6.1 Post-processing the list (RL/complete.c:1314-1368, 1373-1516, 1519-1570)

1. The generator results are collected; slot 0 = common prefix computed
   by comparing neighbours in generation order (bytes, or wide
   characters in a multibyte locale; ignore-case folds). If several
   matches share no first character and the user typed something, slot 0
   is the typed text itself. One match: slot 0 is that match and there is
   no slot 1.
2. Sort slots 1.. with `strcoll` (locale order; **[oracle]** C.UTF-8 is
   byte order: `.edrc .hid Makefile Mdir/ foo-bar foo_baz
   makedepend.txt`), then drop adjacent duplicates. If exactly one
   distinct match remains and it equals slot 0, it becomes the single
   match.
3. If the results are filenames and an ignore function is set, run it:
   - FIGNORE (`filename_completion_ignore`, BL:3073-3213): colon-
     separated suffixes; a name is dropped if it ends with any non-empty
     suffix. With `force_fignore` on (default): if every name is
     dropped, there are no matches (bell). With it off, all names are
     kept in that case. One remaining name becomes the single match.
     Applies to TAB, `?`, `*` alike. **[oracle]** FIGNORE=.o: `ls pro`
     (prog.c prog.o) TAB gives `ls prog.c `; `ls onl` (only.o) TAB and
     M-? ring; M-? on `pro` lists just `prog.c`.
   - keep directories only (command position, nothing matched): names
     that are not directories (after tilde expansion) are dropped; if
     none remain, no matches.
   - ignore everything (no_empty_cmd_completion).
   If some were removed and more than one remains, slot 0 is recomputed.

### 6.2 Replacing the word (RL/complete.c:1821-1910)

The text to insert, for slot 0 (TAB, `!`, `@`) or for each match (`*`),
is the match passed through bash's quoting function (§6.3) when:
the results are filenames (or full quoting was requested) and quoting is
desired, and the open quote is none or `'`/`"`; and either the match
contains a character from the filename quote set, or there is no open
quote and the line before point contained any quote or backslash
(`found_quote`, §3; not limited to the word). Otherwise the match is inserted as
is (variables, commands that are not filenames, users...).

Then, as one undo group (RL/text.c:194-208):

- If there is an open quote, the character before `start` is that quote,
  and the replacement begins with it: the quote is included in the
  replaced range (no doubled quote).
- If the quoting function changed the quote style (the `!` case in 6.3)
  and the character before `start` is the old open quote: it is replaced
  too (the quote disappears).
- Delete `[start, point)` and insert the replacement; point ends after
  it. Text after point is untouched. **[oracle]** `ls fileo xyz` with
  point after `fileo`: `ls fileonly.txt xyz`, cursor after `.txt`, no
  space; `ls filXonly` with point after `fil`: `ls fileXonly` + bell.
- `skip-completed-text` on: instead of replacing, skip over characters
  after `start` that already equal the replacement, and insert only the
  rest; point after the replacement. **[oracle]** `ls Makefile` with
  point after `Make`: on gives `ls Makefile ` (point at end, space
  appended since point reached the end); off gives `ls Makefilefile`,
  point after the first `Makefile`, no space.

### 6.3 Quoting (BL:4206-4516)

Two different character sets are involved. The *filename quote set*
(BL:317: blank, tab, newline, `\ " ' @ < > = ; | & ( ) # $ ` ? * [ ! : {
~`) decides only *whether* the quoting function runs (6.2): a match
with none of these characters, and no quoting in the typed word, is
inserted as is. Once the function runs, the chosen style's own rules
decide *which* characters get escaped (below).

"Quoting in the typed word" is really `found_quote` from §3: any quote
or backslash *anywhere on the line before point*, not just in the word.
So the same completion quotes differently depending on earlier words.
**[oracle]** on a fresh line `: I` TAB gives `: I,z ` and `: J` gives
`: J]z `; but `: a\ b I` TAB gives `: a\ b I\,z ` and `: "a" J` gives
`: "a" J\]z ` (the earlier quoting made the quoting function run, and
its backslash table escapes `,` and `]`). `: 'a' X` still gives
`X%z` (`%` is not in the backslash table). `: x` with `x ,y` gives
`: x\ \,y ` (the blank triggers quoting; the comma comes along).

Style choice, in order:

1. No open quote, backslash style (the default), direxpand off, and the
   match contains an expansion character (a `$`, a leading `~`, or a
   backquote) and does not exist as a literal file name: if the
   expansion contains quote-set characters, use *double quotes leaving
   `$` and backquote alone*; in any case remove the expansion characters
   (`$` and, when present, `{`/`}` or `(`/`)`; `~`; backquote) from the
   set of characters to escape, and stop full quoting. This keeps
   `$HOME/...` working. **[oracle]** `: $HOME/q/A` TAB gives
   `: "$HOME/q/A z" `; `: ${HOME}/q/D` TAB gives `: ${HOME}/q/D$z ` (the
   file's own `$` is left unescaped: a bash quirk worth cloning).
2. No open quote, backslash style, the match contains a newline: single
   quotes.
3. Open `"`: double quotes; if the match contains `$` or a backquote,
   the variant that leaves them unescaped.
4. Open `'`: single quotes.
5. History expansion on (interactive default) and an open `"` with a `!`
   in the match: backslash style, and the open quote is dropped (6.2).
   **[oracle]** `: "L` (file `L!z`) TAB gives `: L\!z `.

Then:

- A leading `~` in a single match is tilde-expanded first, but only for
  the quote styles (inside quotes `~` would not expand).
- Backslash style: a backslash before each character of the escape set,
  then a backslash before any remaining character that is in
  COMP_WORDBREAKS, and before a leading `~` if a file of that name
  exists. **[oracle]** escape set observed with full quoting: blank, tab,
  `' " $ = : @ , ] [ { } ! & * \ ( ) ; | < > ? ^ ``, `#` and `~` only as
  the first character. Not escaped: `% + - . / _` and alphanumerics, `#`
  and `~` elsewhere. The `:` is escaped only through the COMP_WORDBREAKS
  pass: **[oracle]** M-/ (breaks reduced to blank and quotes) on `h:`
  inserts `h:x` unescaped.
- Double quotes: `"` + text with `"` and `\` (and `$`, backquote in the
  plain variant) escaped + `"`. **[oracle]** `"C\"z"`, `"P\\z"`,
  `"D$z"`, ``"e`z"``, `"B'z"`.
- Single quotes: `'` + text with each `'` written as `'\''` + `'`.
  **[oracle]** `'B'\''z'`, `'D$z'`, `'L!z'`, and the newline name as
  `'b` NEWLINE `z'`.
- Several matches (inserting a prefix, MULT): the closing quote of the
  quoted styles is cut off, so the quote stays open. **[oracle]** `: "m`
  (`m z1 m z2`) gives `: "m z` + bell; `: 'm` gives `: 'm z`; `: m`
  gives `: m\ z`.

**[oracle]** backslash style, each completed in turn on one line
(`: A` TAB `B` TAB ...), so from `B` on the line already holds
backslashes and the quoting function always runs (see above; on a fresh
line `I,z`, `J]z`, `Y}z`, `W^z` would come back unescaped):

| file | inserted |
|------|----------|
| `A z` | `A\ z` |
| `B'z` `C"z` `D$z` | `B\'z` `C\"z` `D\$z` |
| `E#z` `#hz` | `E#z` `\#hz` |
| `F=z` `G:z` `H@z` | `F\=z` `G\:z` `H\@z` |
| `I,z` `J]z` `K{z` `Y}z` `W^z` | `I\,z` `J\]z` `K\{z` `Y\}z` `W\^z` |
| `L!z` `M&z` `N*z` `U?z` `V[z` | `L\!z` `M\&z` `N\*z` `U\?z` `V\[z` |
| `O~z` `~tz` | `O~z` `\~tz` |
| `P\z` `Q(z` `c)z` `R;z` `S|z` `T<z` `d>z` ``e`z`` | `P\\z` `Q\(z` `c\)z` `R\;z` `S\|z` `T\<z` `d\>z` ``e\`z`` |
| `X%z` `Z+z` `f-z` `g.z` | unchanged |
| TAB in name | backslash + TAB (drawn as blanks to the tab stop) |
| newline in name | `'b` NL `z'` (the line shows a real line break) |

Dequoting (BL:4211-4251), used on the typed word: remove unquoted `'`
and `"` that open and close runs; a backslash outside single quotes
escapes the next character (inside double quotes the backslash is kept
unless the next character is one of `$ ` " \` and newline); nothing is
special inside single quotes.

### 6.4 The append character (RL/complete.c:1924-1988)

After a single match is inserted (TAB, `!`, `@`, the `?` special case,
menu):

1. Build the suffix: the open quote character if there is one, the
   quote was not suppressed, and the character before point is not
   already that quote; then the delimiter (§3 step 4) if there was one,
   else the append character (space; `/` for a directory variable; none
   when suppressed).
2. If the results are filenames: stat the match (lstat when the match
   added something to the word and `mark-symlinked-directories` is off;
   stat otherwise), after tilde expansion and the stat hook.
   - A directory: if `mark-directories` is on, insert `/` unless the
     character at point is already `/`, or point is at end of line and
     the character before it is `/`. **Nothing else** is inserted: no
     closing quote, no space. The `/` is inserted even when point is not
     at the end of the line.
   - A symlink to a directory (only seen through lstat): insert nothing.
   - Otherwise: insert the suffix, but only when point is at the end of
     the line.
3. Not filenames: insert the suffix only at end of line.

Since quoted single matches already carry their closing quote (6.3), the
quote part of the suffix only appears for non-filename completions or
unquoted replacements. **[oracle]** `: "sp` (dir `sp dir`) gives
`: "sp dir"/`; `"ech` in command position gives `"echo" `; `echo "$xvar`
gives `echo "$xvar"/`; `ls dir2 xyz` with point after `dir2` gives
`ls dir2/ xyz`; `ls lin` (link1 -> dir1) gives `ls link1` (nothing), and
TAB again gives `ls link1/` (now the match adds nothing, so stat is
used); with mark-symlinked-directories on, `ls lin` gives `ls link1/`;
mark-directories off: `ls dir` gives `ls dir1` (no slash, no space).
Dangling symlink: `ls dan` gives `ls dangle1 `.

### 6.5 Undo and point

The replacement is one undo group; each appended piece (`/`, quote,
space) is a separate insert record after it. **[oracle]** `ls fileo` TAB
(`ls fileonly.txt `) then C-_ removes the space, C-_ again restores
`ls fileo`. `ls fi` TAB (`ls file`) then one C-_ restores `ls fi`. M-*
then C-_ restores the word in one step. Point always ends after the last
inserted character.

---------------------------------------------------------------------------

## 7. The listing

### 7.1 Entry (RL/complete.c:1751-1817)

1. Move the cursor down to the last screen row of the input (not to
   column 0 first; just vertical moves, emitted as newlines and a CR when
   moving down). The cursor's column there does not matter because every
   following line starts with a newline.
2. One match only (after dedup): newline, print it (with its mark),
   newline, redraw. **[oracle]** `ech` C-x ! prints `echo`; `ls fileo` M-?
   prints `fileonly.txt`.
3. Width: the maximum display width of the printable parts.
4. completion-query-items reached (§2.4): newline, print
   `Display all N possibilities? (y or n)` and wait (§7.5). On no:
   newline, redraw, done. On yes: continue (the question stays on
   screen; the list starts on the next line).
5. Print the list (7.2-7.4), then redraw (7.6).

### 7.2 Printable part (RL/complete.c:773-803)

For filename results, only the part after the last `/` is shown; if the
match ends in `/`, the part after the previous `/` (so `/usr/src/` shows
`src/`); a bare `/` shows as is. Non-filename results (variables,
commands found in PATH but returned as bare names, hosts) are shown
whole. Display width counts a control character or DEL as 2 (`^X`), an
invalid byte as 1, a wide character as its wcwidth (RL/complete.c:807-
854). Control characters are printed as `^` + letter, DEL as `^?`.

### 7.3 Layout (RL/complete.c:1573-1737)

- Screen width for the list: `completion-display-width` if it is >= 0
  and <= the screen width; else `$COLUMNS` from the *environment* if set,
  non-empty, >= 0 and <= the screen width; else the screen width.
  **[oracle]** with checkwinsize off and `export COLUMNS=20` on a 30-wide
  screen, the list uses 20 while the line still wraps at 30. With
  checkwinsize on (default) COLUMNS is reset after every command, so in
  practice it equals the screen width. completion-display-width parses
  an integer; empty means -1 (RL/bind.c:2154).
- Column width `w` = max + 2. Columns = width / w; if that is not 1 and
  divides exactly (`columns * w == width`), use one fewer. A width of 0
  gives one per line. **[oracle]** 80 wide, max 14: 5 would fit exactly,
  so 4 columns; display-width 12, max 2: 3 fit exactly, so 2 columns;
  display-width 0: one per line.
- Rows = ceil(N / columns).
- Default (down columns, like `ls`): row `r` holds items `r, r+rows,
  r+2*rows, ...`. Every item except the last in its row is padded with
  blanks to `w` (at least one blank if it is wider). Then a newline.
- `print-completions-horizontally`: items in order across, a newline
  after every `columns` items; same padding. **[oracle]** k1..k9 on 30
  columns: `k1  k2  k3  k4  k5  k6  k7` / `k8  k9`.
- Marks (7.4) are printed after the name and count in the printed
  width for the padding, but not in `max`.
- `completion-prefix-display-length` > 0 (RL/complete.c:1600-1617,
  856-985): compute the common prefix length of the printable parts (for
  filenames, of the part after the last `/` of slot 0); if it is longer
  than the setting and longer than 3, it is replaced in every item by
  `...`, or by `___` when the next character is `.`, and `max` shrinks
  accordingly. An item that *is* the whole prefix is printed in full.
  **[oracle]** setting 3 with `aaa_long_common_{1,2,33,.x}`:
  `___.x  ...1   ...2   ...33`.
- `colored-stats` and `colored-completion-prefix` add SGR colour; not
  covered (off by default).

### 7.4 Marks (RL/complete.c:653-732, 992-1106)

Only for filename results:

- `visible-stats` on: lstat the full path (after tilde expansion and the
  directory hook; the stat hook first if any) and append `/` directory,
  `%` char device, `#` block device, `@` symlink (whatever it points
  to), `=` socket, `|` FIFO, `*` regular and executable (`access(X_OK)`;
  native Windows: extension .exe .cmd .bat .com), nothing otherwise.
  **[oracle]** `dangle1@ dir1/ exe1* fifo1| link1@ plain1`.
- Otherwise, `mark-directories` on (default): `/` if `stat` (following
  links) says directory. **[oracle]** default listing: `dir1/`,
  `link1/`, `exe1` unmarked.

### 7.5 The prompts (RL/complete.c:585-633)

Both read keys raw (no echo):

| key | `Display all...` | `--More--` |
|-----|------------------|------------|
| `y` `Y` SPC | yes | next page |
| `n` `N` DEL | no | stop |
| `q` `Q` | bell, ask again | stop |
| RET (C-m) or C-j | bell, ask again | one more line |
| C-g (abort char) | abort | abort |
| anything else | bell, ask again | bell, ask again |

Paging (`page-completions` on, default): after printing a row, if the
number of rows printed since the last page is >= screen rows - 1 and more
rows remain, print `--More--` and wait. Afterwards erase the line (CR,
clear to end of line, CR) so the next row overwrites it. Next page:
counter to 0. One more line: counter to rows - 1 (the pager returns after
one more row). Stop: abandon the list where it is and go to 7.6; the
erased `--More--` line becomes the prompt line. **[oracle]** 99 files on
8x40: 7 rows then `--More--` (cursor after it); `x` rings; RET shows one
more row and asks again; SPC shows the remaining 7 rows; `q` and DEL stop
with `$ ls f` drawn where `--More--` was. In horizontal mode the pager
counts the same way.

Abort (C-g) at either prompt: bell, then readline's abort unwinds to the
key loop with no newline and no redraw; its display model still thinks
the cursor is on the input line, while the real cursor sits after the
prompt text, so the next redisplay draws at the wrong place
(**[oracle]**: typing after C-g at `Display all ... (y or n)` appended
text after `(y or n)`). Do not golden this; a clone may simply redraw.

### 7.6 Redraw after a listing (RL/display.c:2843-2851)

After the last list row's newline the cursor is at column 0 of a fresh
line. Readline forgets what is on screen and redraws from there: the
whole prompt, including the lines of a multi-line prompt before its last
newline, then the line, with the cursor at point (on the right row of a
wrapped line). No blank line separates list and prompt. **[oracle]**
`PS1='top\n$ '` redraws both `top` and `$ ls file.`; a two-row wrapped
line with point on the first row lists below the second row, redraws
both rows, and puts the cursor back on the first row.

The listing never rings a bell and does not change the line, so the
following TAB lists again.

---------------------------------------------------------------------------

## 8. menu-complete and menu-complete-backward (RL/complete.c:2858-3034)

Unbound in emacs mode. State: the original word, the match array (slot 0
= common prefix), size, index, `full_completion`.

- A fresh cycle starts when the previous command was neither
  menu-complete nor menu-complete-backward, or the previous call
  finished a single match. It finds the word, generates and
  post-processes like §2 (bell and stop on no matches), then:
  - inserts slot 0 if non-empty (the common prefix) and makes the word
    end there;
  - `show-all-if-ambiguous` with several matches: lists them; if their
    number reaches completion-query-items, bell and the next call starts
    afresh; else with `menu-complete-display-prefix` bell and stop here;
  - a single match: append (§6.4), mark the cycle finished, stop;
  - `menu-complete-display-prefix` with several matches: bell, stop.
- Then (and on every following call) index += count (the numeric
  argument; backward negates it), modulo the number of entries counting
  slot 0. Index 0 (back to the prefix) rings and inserts slot 0 with no
  append; any other index inserts that match (as a single match) and
  appends.

**[oracle]** `file.c file.h fileonly.txt`, C-o bound to menu-complete:
`ls fi` C-o C-o C-o C-o C-o gives `ls file.c `, `ls file.h `,
`ls fileonly.txt `, `ls file` + bell, `ls file.c `; menu-complete-backward
then gives `ls file.c ` from `file.h`. M-3 C-o on `ls fi` gives
`ls fileonly.txt `. `ls fd` (dir fdir) gives `ls fdir/`, and the next C-o
starts afresh inside `fdir/` (empty: bell). With
menu-complete-display-prefix: first C-o `ls file` + bell, then `file.c`.

The cycle returns to the common prefix, not to the original word.
old-menu-complete (RL/complete.c:2727-2846) is the older variant that
restores the original text instead.

---------------------------------------------------------------------------

## 9. The other commands

### 9.1 possible-completions (M-?, M-=)

Mode `?` with the failed flag cleared: list, or one match printed alone,
or bell. **[oracle]** `echo dd/f` M-? and M-= both list `f1  f2`.

### 9.2 insert-completions (M-*)

Mode `*` (§2.2). **[oracle]** `echo dd/f` M-* gives `echo dd/f1 dd/f2 `;
`ls fileo` M-* gives `ls fileonly.txt `.

### 9.3 complete-filename (M-/), possible-filename-completions (C-x /)

Filename generator only, FIGNORE on, the directory hook turned off (so
`$VAR/` in the directory is not expanded for opening), word breaks
blank, tab, newline, `"`, `'` (BL:3927). **[oracle]** `echo ab=` M-/
gives `echo ab=cd ` (file `ab=cd`), where TAB rings; `echo h:` M-/ gives
`echo h:x ` (`:` not escaped).

### 9.4 Commands, variables, users, hosts

M-! / C-x !: §4.8 on the word as found with the normal breaks.
**[oracle]** `ech` M-! gives `echo `. M-$ / C-x $: §4.5 (`$` optional).
M-~ / C-x ~: §4.6 (`~` optional; a name without `~` is not a filename).
M-@ / C-x @: §4.7 (`@` optional).

### 9.5 complete-into-braces (M-{)

Filename matches of the word, inserted as a brace expression that
factors common prefixes recursively, sorted, with no append character
(bracecomp.c, not in this tree). **[oracle]** with `dd/f1 dd/f2 dd/g3`:
`echo dd/` M-{ gives `echo dd/{f{1,2},g3}`; `echo dd/f` gives
`echo dd/f{1,2}`.

### 9.6 Glob commands (BL:3996-4094)

glob-complete-word (M-g): the word plus `*` (emacs mode forces the
explicit-argument flag) is globbed; matches are completed in the mode of
§1, and the word itself is not quoted when it comes back unchanged.
**[oracle]** `ls fi` M-g gives `ls file.` + bell; `ls o` M-g gives
`ls other `. glob-expand-word (C-x *): mode `*` on the glob. **[oracle]**
`ls f*` C-x * gives `ls file.c file.h `. glob-list-expansions (C-x g):
mode `?`; **[oracle]** `ls *.?` C-x g lists `file.c  file.h`.

### 9.7 dynamic-complete-history (M-TAB) and dabbrev-expand

M-TAB completes the word against every token of every history line
(history_tokenize), sorted with plain `strcmp`, case-sensitive, as TAB
(or `?` if the previous command was M-TAB). FIGNORE applies. The bash
classifier is off. **[oracle]** after `echo hello`, `ec` M-TAB gives
`echo `, `echo he` M-TAB gives `echo hello `. dabbrev-expand (unbound)
is menu completion over the same tokens in history order, newest first,
unsorted, with no append character (BL:3839-3874).

### 9.8 export-completions and delete-char-or-list (unbound)

export-completions writes, on readline's output (stderr), a CRLF if the
terminal is prepped, then the count, the word, `start:end`, and each
match (slot 0 first) on its own line (RL/complete.c:3057-3089). With an
argument > 1 it then deletes the line. delete-char-or-list: delete-char
except at the end of a non-empty line, where it is possible-completions
(emacs.md §8.6 for EOF).

---------------------------------------------------------------------------

## 10. Windows notes

- Native Windows readline folds case by default
  (`_rl_completion_case_fold = 1` for `_WIN32` without Cygwin,
  RL/complete.c:187-191); MSYS2/Git for Windows instead set
  `completion-ignore-case on` in /etc/inputrc (msys2.md §6), along with
  `completion-query-items 40`, mark-directories and
  mark-symlinked-directories on, and `\e[Z` bound to complete. Our
  default should follow readline (off) and let inputrc turn it on; the
  msys mode reads the install's inputrc (m6.md step 6).
- Drive letters: `printable_part` treats `X:` as a directory prefix and
  the filename generator completes `X:foo` relative to that drive
  (RL/complete.c:781-784, 2510-2528). Decide whether our generator
  accepts `C:/...` and `C:\...` words; a backslash is a quote character
  to bash's dequoting, so `C:\Win` TAB is a problem in any bash.
- Executable test: native readline's visible-stats marks .exe .cmd .bat
  .com as `*`; for command completion in PATH we need the same idea
  (PATHEXT) instead of the X bit.
- No user database (§4.6) on native Windows: `~user` completion returns
  nothing; `~` alone still expands for directories.
- `completion_strip_exe` (Cygwin/MSYS2 bash patch, shopt, off by
  default; issues.md M6): when on, a command or filename candidate ending
  in `.exe` is offered without the suffix when the stripped name refers
  to the same file (Cygwin's stat finds `foo` as `foo.exe`), so `note`
  TAB gives `notepad ` rather than `notepad.exe `. Dedup then merges it
  with any identical name. Not testable on the Linux oracle.
- Case-insensitive file systems: with ignore-case off, `ls make` will
  not offer `Makefile` even though `ls Makefile`/`ls makefile` both work;
  that is bash's behaviour too.
- Sorting uses `strcoll`; under a UTF-8 locale other than C.UTF-8 glibc
  sorts case-insensitively-ish. Our oracle runs C.UTF-8 (byte order);
  pick byte order and document it.

---------------------------------------------------------------------------

## 11. Things not determined

- `char_is_quoted`, `skip_to_delim`, `unclosed_pair` (subst.c) were not
  read; §3 and §4.1-4.2 rely on their observed effects. Corner cases
  (`$'...'`, quotes inside `$(...)`, extglob) are untested.
- The backslash table of `sh_backslash_quote` (shquote.c) is inferred
  from the §6.3 oracle table; characters not in that table (other
  control characters, bytes >= 0x80) were not tried.
- `all_variables_matching_prefix` (variables.c): set variables only was
  observed; arrays, `nameref`s, functions-as-variables were not tried.
- C-g at the y/n or `--More--` prompt leaves the display inconsistent
  (§7.5); exact screens not worth matching.
- COLUMNS assignment without export, and its interplay with
  `checkwinsize` and bash's own window-size variable hooks, beyond the
  one observation in §7.3.
- colored-stats, colored-completion-prefix, wide characters in listings,
  vi-mode completion, programmable completion (M8).
- The `?` special case "last attempt failed, this one has one match"
  (§2.2) needs the file system to change between two TABs; not
  recorded.
- The quote *delimiter* of §3 step 4 (a closed-quote break character
  becoming the appended character) was not reproduced: in the cases
  tried (`echo "a"fileo`) the closing quote counted as quoted and the
  whole word was taken.

---------------------------------------------------------------------------

## 12. Scenarios for t/edit

Recording notes: all `rc` lines go into the rc file and run before the
first key, so per-scenario settings must be typed (`bind '...'\r\C-l`)
or put in a separate script. C-u kills only before point; use
`\C-a\C-k` to clear a line. Type a backslash as `\x5c`. The appended
space is visible only as the cursor column (trailing blanks are
trimmed). The bell count is cumulative. Every scenario runs in
/home/oracle, which also holds `.edrc` (it shows up in hidden-file
listings). Each `keys` line below is followed by a `snap`.

1. **tab-basic** (size 16 60; rc `mkdir dir1 dir2; : > file.c; : >
   file.h; : > fileonly.txt`). Keys: `ls fi\t` → `$ ls file` bell 1;
   `\t` → unchanged, bell 2; `\t` → list `file.c        file.h
   fileonly.txt` (width 14 each) and `$ ls file` redrawn below; `o\t` →
   `ls fileonly.txt ` (cursor col 18); `\C-_` → space removed; `\C-_` →
   `ls fileo`; `\C-a\C-kls nothing\t` → bell; `\t` → bell, no list;
   `\C-a\C-kls d\t` → `ls dir` bell; `1\t` → `ls dir1/`; `\t` → bell
   (empty dir).
2. **tab-midline** (same rc). `ls fileo xyz` + 4×C-b + TAB →
   `ls fileonly.txt xyz`, cursor after `.txt`, no space; `ls dir2 xyz`
   + 4×C-b + TAB → `ls dir2/ xyz`; `ls filXonly` + 5×C-b + TAB →
   `ls fileXonly` bell; with `bind 'set skip-completed-text on'`,
   `ls Makefile` (file Makefile) with point after `Make` → `ls Makefile `.
3. **list-query-pager** (size 8 40; rc `for i in {1..99}; do : > f$i;
   done`). `ls f\t\t` → 7 rows of 7 columns (`f1   f23  ...`) then
   `--More--`, cursor col 8, bell 1; `x` → bell 2; `\r` → one more row
   (`f16 ...`), `--More--` again; `\s` → the last 7 rows and `$ ls f`;
   then `\C-a\C-kls f\t\t` and `q` → `$ ls f` where `--More--` was;
   again with `\x7f` → same. A second script with `rc for i in
   {1..40}...` and `rc bind 'set completion-query-items 30'`: `ls f\t\t`
   → `Display all 40 possibilities? (y or n)` on the next line, cursor
   col 38; `x` → bell; `n` → newline and `$ ls f`; `\t` → asks again;
   `y` → the list (6 rows of 7) under the question.
4. **quote-backslash** (size 24 150; rc creates the 35 names of §6.3 in
   `q/` and `cd q`). `: A\tB\tC\t...R\t` → the first table row set on one
   line; `\C-a\C-k: S\tT\t...b\t` (includes `~t`, `#`, tab and newline
   names) → `S\|z ... \~tz \#hz a\<TAB>z 'b` NL `z'`. Then the
   fresh-line contrast: `\C-a\C-k: I\t` → `: I,z `; `\C-a\C-k: a\x5c\sb
   I\t` → `: a\ b I\,z `.
5. **quote-open** (same rc). `: 'A\t 'B\t 'D\t 'L\t 'b\t` →
   `'A z'  'B'\''z'  'D$z'  'L!z'  'b`NL`z'`; `: "A\t "B\t "C\t "D\t
   "P\t "e\t` → `"A z"  "B'z"  "C\"z"  "D$z"  "P\\z"  "e`z"`; `: "L\t`
   → `: L\!z `; `: x"A\t` → `: x"A z" `; files `m z1 m z2`: `: m\t`,
   `: "m\t`, `: 'm\t` → `m\ z`, `"m z`, `'m z`, each with a bell;
   dir `sp dir`: `: "sp\t` → `: "sp dir"/`.
6. **quote-expansion** (rc `mkdir q; : > 'q/A z'; : > 'q/D$z'`).
   `: ~/q/A\t` → `: ~/q/A\ z `; `: $HOME/q/A\t` → `: "$HOME/q/A z" `;
   `: ${HOME}/q/D\t` → `: ${HOME}/q/D$z `; `: "$HOME/q/A\t` → same as the
   second; `echo "a"fileo\t` with `afileonly` → `echo afileonly `.
7. **command-position** (size 30 100; rc `alias tfoo=ls; tbar() { :; };
   mkdir tdir; : > texe; chmod +x texe; : > ecfile; mkdir sub; : >
   sub/prog; chmod +x sub/prog; : > sub/data`). `ec\t` → `echo `;
   `ze\t` → `zecho `; `t\t\t` → bell then the 14-name list of §4.8;
   `tdi\t` → `tdir/`; `tdir\t` → unchanged, no bell; `tex\t` → bell;
   `./te\t` → `./texe `; `sub/\t` → `sub/prog `; `if ec\t` →
   `if ecfile `; `: ; ec\t`, `: | ec\t`, `(ec\t`, `{ ec\t`, `: $(ec\t`,
   `FOO=1 ec\t` → `echo `; `FOO=ec\t` → `FOO=ecfile`; ``: `ec\t`` →
   ``: `echo`` (no space).
8. **words-vars-users-hosts** (rc `mkdir dd; : > dd/f1; : > dd/f2;
   xvar=/home/oracle/dd; xvas=1; declare xvau`). `echo $xva\t\t` → bell,
   list `$xvar  $xvas`; `echo $xvar\t` → `echo $xvar/`; `\t` →
   `echo $xvar/f` bell; `echo "$xvar\t` → `echo "$xvar"/`; `echo
   ~ora\t` → `echo ~oracle/`; `echo ~\t\t` → user list with `/` marks;
   `echo @l\t` → `echo @localhost `; `echo x@\t\t` → host list;
   `ls x=fi\t` style break with a file `file` → `ls x=file `.
9. **specific-commands** (rc `mkdir dd; : > dd/f1; : > dd/f2; : >
   dd/g3; : > ab=cd; : > h:x`). `echo dd/f\e?`, `\e=` → list `f1  f2`;
   `\e*` → `echo dd/f1 dd/f2 `; `echo ab=\e/` → `echo ab=cd `;
   `echo ab=\t` → bell; `echo h:\e/` → `echo h:x `; `ech\e!` → `echo `;
   `ech\x18!` → prints `echo`; `echo $xv\e$` / `\x18$`; `echo ~ora\e~`;
   `echo ~r\x18~` → `~root/`; `echo @loc\e@`; `echo dd/\x18/` → `f1  f2
   g3`; `echo dd/\e{` → `echo dd/{f{1,2},g3}`; `ech\e\e` → `echo `;
   after `echo hello\r`: `ec\e\t` → `echo `.
10. **case-hidden** (rc `: > Makefile; : > makedepend.txt; : > foo-bar;
    : > foo_baz; mkdir Mdir; : > .hid`). With ignore-case on: `ls ma\t`
    → `ls make`; `ls MA\t` → `ls Make`; `ls mAKEF\t` → `ls Makefile `;
    `ls md\t` → `ls Mdir/`; with map-case: `ls foo_b\t` → `ls foo` bell.
    Off again: `ls \t\t` lists 7 names in 4 columns of 16; `ls .\t\t` →
    `./  ../  .edrc  .hid`; `ls ..\t` → `ls ../`; match-hidden-files off:
    `ls \t\t` without dot files, `ls .\t\t` unchanged.
11. **marks-symlinks** (rc `mkdir dir1; ln -s dir1 link1; : > exe1;
    chmod +x exe1; mkfifo fifo1; ln -s nowhere dangle1; : > plain1`).
    `ls lin\t` → `ls link1`; `\t` → `ls link1/`; `ls \t\t` → `.edrc
    dangle1  dir1/  exe1  fifo1  link1/  plain1`; visible-stats on →
    `dangle1@ dir1/ exe1* fifo1| link1@`; mark-directories off: list
    unmarked, `ls dir\t` → `ls dir1`; mark-symlinked-directories on:
    `ls lin\t` → `ls link1/`; `ls dan\t` → `ls dangle1 `.
12. **fignore-layout** (size 24 30; rc with prog.c prog.o only.o,
    `aaa_long_common_{1,2,33,.x}`, k1..k9). `FIGNORE=.o`: `ls pro\t` →
    `ls prog.c `; `ls onl\t` → bell; `ls pro\e?` → `prog.c`. Horizontal:
    `ls k\t\t` → `k1  k2 ... k7` / `k8  k9`. completion-display-width
    12 → 2 columns; 0 → one per line. prefix-display-length 3: `ls
    aaa\t\t\t` → `___.x  ...1   ...2   ...33`. `shopt -u checkwinsize;
    export COLUMNS=20`: `ls k\t\t` → 3 rows `k1  k4  k7`.
13. **show-all-disable** (size 24 40; rc `file.c file.h fileonly.txt xa1
    xb2`). show-all-if-ambiguous: `ls fi\t` → list under the stale
    `$ ls fi` line, then `$ ls file`, no bell; show-all-if-unmodified:
    `ls fi\t` → `ls file`, no bell; `\t` → list; `ls x\t` → list at
    once; disable-completion: `ls fi\tz` → `ls fi  z`.
14. **menu** (rc as 13 plus `mkdir fdir; bind '"\C-o": menu-complete';
    bind '"\C-xp": menu-complete-backward'`). §8 sequence: 5×C-o, then
    C-x p, `ls fd\C-o`, `\C-o`, `ls fi\e3\C-o`, and with
    menu-complete-display-prefix on `ls fi\C-o\C-o`.
15. **wrapped-redraw** (size 12 30; rc `file.c file.h fileonly.txt`, a
    second script with `PS1='top\n$ '`). `echo file 0123456789012345678
    901234567890` + `\C-a\ef\ef` (point after `file`, row 0) + `\t` →
    bell; `\t` → the list starts below row 1, both rows redrawn, cursor
    back at row 0 col 11 of the new copy; multi-line prompt: `ls
    fi\t\t\t` → `top` and `$ ls file.` redrawn after `file.c  file.h`.
    Also `no_empty_cmd_completion` on: `\t\t` on an empty line → 2 bells;
    `shopt -s direxpand`: `ls $HOME/fileo\t` →
    `ls /home/oracle/fileonly.txt `.
