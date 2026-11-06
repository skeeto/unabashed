# Readline 8.3 emacs-mode editing commands: semantics digest

Purpose: enough detail to clone the behaviour of readline's emacs-mode
editing commands, as bash 5.3 uses them, without reading the GPL source.
Everything below is a paraphrase. The pseudocode is ours, not readline's.
The only verbatim text is short user-visible strings such as prompts and
messages. readline.md §2-3 already has the default key tables. This file
covers exact behaviour.

Citations are relative to the `bash-5.3/` root of `bash-5.3.tar.gz`:

- RL = `lib/readline/`. The files used are text.c, kill.c, undo.c,
  misc.c, search.c, isearch.c, readline.c, display.c, terminal.c,
  rltty.c, bind.c, util.c, mbutil.c, history.c, histsearch.c,
  histexpand.c, emacs_keymap.c and nls.c.
- BL = `bashline.c`, BH = `bashhist.c`, VER = `version.c`.

Claims marked **[oracle]** were checked against a real bash 5.3.15 (the
Cygwin `bash` on this machine). The checks piped keystrokes into
`bash --norc -i` with a scratch INPUTRC, then read the executed output and
counted BEL bytes on stderr. Piped input is all typeahead, which has one
side effect, described in §3.2.

---------------------------------------------------------------------------

## 0. Model and dispatch loop

State: `line` (byte buffer), `point`, `end`, `mark`, the undo list, the
kill ring, the numeric argument (`arg`, `sign`, `explicit`), `last_func`,
and the `last_was_kill` counter.

Per key read at top level (RL/readline.c:576-728):

1. If no key was pushed back by a prefix command (`pending_input == 0`),
   reset the argument to `arg=1, sign=1, explicit=0` (RL/readline.c:621-626;
   RL/misc.c:219-224).
2. Remember `lk = last_was_kill`.
3. Dispatch the key through the keymap. The command is called with
   `count = arg * sign` (RL/readline.c:941).
4. After a function returns, `last_func` becomes that function, unless
   input is pending (a prefix command) or the function is `digit-argument`
   (RL/readline.c:945-953). `universal-argument` is **not** excluded. Its
   inner dispatch sets `last_func` to the real command, then the outer
   dispatch overwrites it with `universal-argument`. So `C-u C-y` then
   `M-y` fails, if C-u is bound to universal-argument.
5. If no input is pending and `last_was_kill == lk`, set
   `last_was_kill = 0` (RL/readline.c:712-716). "Is a kill" therefore means
   "this command incremented the counter".
6. Mark-activity cleanup (§5.3), then redisplay (RL/readline.c:532-568).

Some general rules:

- Point clamping: after kills and undos, `point` and `mark` are clamped
  into `[0, end]` (RL/text.c:167-188). `rl_delete_text` clamps only the
  mark (RL/text.c:159). Insertion never adjusts the mark (§5.1).
- The bell: `rl_ding` writes BEL (`\007`) to **stderr**. Under
  `bell-style visible` it instead sends termcap `vb` if the terminal has it,
  and falls back to BEL otherwise. `none`/`off` sends nothing. All of this
  happens only while the terminal is echoing (RL/terminal.c:868-893).
  bell-style is `audible` by default (RL/readline.c:235). The accepted
  values are `none`/`off`, `audible`/`on` and `visible`, all
  case-insensitive. An empty value means audible, and any other value is
  rejected (RL/bind.c:2227-2240). `prefer-visible-bell` is the old boolean
  for the same setting.
- abort (`C-g`, `M-C-g`, `C-x C-g`), also used internally as "fail
  loudly" (RL/util.c:99-120): bell (unless a timeout is in progress); clear
  the message area; reset the argument; drop pending input; deactivate the
  mark; stop any keyboard macro; set `last_func = NULL`; then unwind to the
  top of the key loop, which redisplays. The line text and point are
  untouched.
- Unbound key: abort, with its bell (RL/readline.c:984-992).

---------------------------------------------------------------------------

## 1. Numeric arguments (RL/misc.c:70-291)

### 1.1 Entry points

- `digit-argument` is bound to M-0..M-9 and M--. It starts argument mode,
  pushes its own key (the digit or `-` with meta stripped) back as input,
  and runs the argument loop (RL/misc.c:228-243).
- `universal-argument` is unbound in emacs mode by default. It starts
  argument mode with `arg *= 4`, without setting `explicit`, and runs the
  same loop (RL/misc.c:249-255).
- Starting argument mode saves the prompt (RL/misc.c:93-98). While it is
  active the prompt area shows the message `(arg: N) `, where N is
  `sign*arg`, followed by the line (RL/misc.c:105). **[oracle]**: the
  echoed display read `(arg: 1) XX`.

### 1.2 The loop (RL/misc.c:117-215)

Each iteration first checks the cap. If `arg > 1000000`: bell, then
`arg = explicit = 0`, restore the prompt, leave argument mode, and return
without dispatching anything (RL/misc.c:77-90). The check runs **before**
the next key is read, so the key that overflowed has already been consumed,
and the keys after it are handled as ordinary input. 1000000 itself is
allowed. **[oracle]**: `M-1 0000000 a` inserts a single `a`.

Then the loop reads a key `k` and classifies it with meta stripped:

```
if k is bound (in the current keymap) to universal-argument:
    if no digit seen yet: arg *= 4; continue
    else: read one more key, leave arg mode, dispatch that key; stop
          (the universal-argument key itself only terminates the digits)
if k is '0'..'9':  arg = explicit ? arg*10 + d : d; explicit = 1; continue
if k is '-' and explicit == 0:  arg = 1; sign = -1; continue
otherwise:
    if a '-' was seen and arg == 1 and explicit == 0: explicit = 1
    leave arg mode (restore prompt, clear message); dispatch k; stop
```

Consequences, all **[oracle]**:

- Once in argument mode, plain digits accumulate. `M-1 2 a` and `M-1 M-2 a`
  both insert 12 `a`s. A further meta digit re-enters `digit-argument`
  without resetting the value, because the reset happens only at top level.
- A `-` after digits is not special. It ends the argument and is
  dispatched, so `M-3 -` inserts `---`.
- `M--` alone means count -1 with `explicit = 1`. `M-- 5` gives -5. A
  second `-` before any digit just sets `arg = 1, sign = -1` again; it does
  not toggle.
- `C-u` (if bound): 4, 16, 64... `C-u C-u a` inserts 16 `a`s.
  `C-u 1 2 C-u 3` inserts twelve `3`s. A `-` after `C-u` discards the 4:
  arg 1, sign -1.
- An abort key inside the loop is just dispatched: `M-5 C-g` rings once
  and resets the argument.
- EOF while reading the argument aborts (RL/misc.c:203-207).

### 1.3 The explicit flag

`explicit` is 1 once a digit has been typed, or after a lone `M--`. It is
**not** set by `C-u` without digits. Commands that look at it:

| command | with explicit arg |
|---|---|
| delete-char, backward-delete-char (and overwrite-mode rubout) | kill (to the kill ring) instead of plain delete, even for count 1 (RL/text.c:1210,1264,1307) |
| clear-screen | redraw only the current line (RL/text.c:596) |
| insert-comment | toggle instead of insert (RL/text.c:1392) |
| set-mark | mark = count, an absolute position (RL/text.c:1884) |
| overwrite-mode | set instead of toggle (RL/misc.c:845) |
| operate-and-get-next | count is a history number (RL/misc.c:787) |
| yank-last-arg | use word `count` instead of `$` (RL/kill.c:680) |
| edit-and-execute-command | `fc ... count` (BL:954) |
| shell-expand-line | no command or process substitution (BL:3024-3025) |
| glob-expand-word / glob-list-expansions | append `*` to the word before globbing (BL:4024) |
| blink-matching-paren | an explicit argument suppresses the blink (RL/parens.c:110) |
| dump-functions/-variables/-macros | print in inputrc form |

Commands that only test `count > 1` (for example `C-u C-d`, count 4,
explicit 0) still take the kill path, because the test is
`count > 1 || explicit`.

---------------------------------------------------------------------------

## 2. Undo (RL/undo.c, RL/text.c:86-161)

### 2.1 Records

The undo list is a stack (newest first) of records:

- `INSERT(start,end)`: text was inserted at [start,end).
- `DELETE(start,end,text)`: text was removed from [start,end).
- `BEGIN` and `END`: group brackets.

Only two primitives record edits: `insert_text(s)` at point and
`delete_text(from,to)` (RL/text.c:86-161). Neither records while an undo
is running. `delete_text` swaps reversed bounds, clamps to `[0,end]`,
records `DELETE` with the removed text, and clamps the mark. A bulk
replace is done as a delete followed by an insert.

**Insert merging** (RL/text.c:106-114). Inserting a string of exactly one
byte extends the top record instead of pushing a new one when all of these
hold:

- the top record is an `INSERT`;
- its `end == point` (the insertion point);
- `end - start < 20`.

In that case `end` grows by 1. A record therefore holds at most 20 bytes,
and the 21st character starts a new record. Nothing else matters: merging
crosses command boundaries and survives cursor motion away and back.
Multibyte characters (more than one byte), yanks longer than one byte,
and anything inside a group (the top record is then `END`) never merge.
**[oracle]**: typing `echo OUT abcdefghijklmnopqrstuvwxy` (34 bytes) and
undoing once leaves the first 20 bytes. The same happens with a C-e in
between.

### 2.2 Undoing one unit (RL/undo.c:169-272)

Each record is popped in turn:

- `DELETE`: set point = start, clamp point and mark, then re-insert the
  text. Point ends at `start + len`.
- `INSERT`: delete [start,end). Point = start.
- `END`: increase the nesting depth and keep popping until the matching
  `BEGIN`.
- `BEGIN` reached when no group is open: bell, and the record is consumed.
  This only happens for an unbalanced list.

Positions -1 and -2 in a record mean "point" and "end" (vi only). A whole
group counts as one unit. Point after a group is whatever its oldest record
left. When the popped record is the head of the current history entry's
saved undo list, that entry's text and data are updated to match
(RL/undo.c:232-265).

- `undo` (C-_, C-x C-u; RL/undo.c:351-367): a negative count does nothing,
  and so does 0. Otherwise it undoes `count` units. When the list runs out
  it rings the bell and stops. **[oracle]**: undo on a fresh line rings
  once. Undo is not itself recorded, and there is no redo.
- `revert-line` (M-r, M-C-r; RL/undo.c:332-347): bell if the list is
  empty; otherwise undo until empty. **[oracle]**: on an edited history
  line it restores the original text, and point ends where the first edit's
  undo leaves it (at end, in the test).

### 2.3 Which commands group

Groups (BEGIN/END) are made by:

- transpose-chars and transpose-words;
- kill-whole-line;
- yank-nth-arg and yank-last-arg;
- the case commands, through `rl_modifying`, which pushes BEGIN, a
  DELETE+INSERT pair covering the region, then END. It does this only when
  start != end (RL/undo.c:312-328), and the bytes are then changed in place
  without further records. Undoing it ends with point at the region's end;
- overwrite-mode self-insert and rubout (one group per command invocation);
- `_rl_replace_text`, used by completion;
- bash's expand-line commands (BL:2808-2820) and shell-transpose-words.

Not grouped:

- a kill: one DELETE;
- yank: one INSERT;
- yank-pop: a DELETE and then an INSERT, as two units. **[oracle]**: undo
  after `C-y M-y` removes the M-y text, and a second undo brings back the
  C-y text;
- self-insert with a count: one INSERT for count up to 1024, then one
  INSERT per 1024-byte chunk (RL/text.c:72,826-911);
- delete-horizontal-space, quoted-insert and tab-insert;
- a bracketed paste: one INSERT for the whole pasted text (§8.7).

### 2.4 Undo lists and history motion (RL/misc.c:303-469)

The current line's undo list is `rl_undo_list`. Each history entry has a
`data` slot that holds that entry's undo list once it has been edited.
There is also one *saved line* holding the text and undo list of the line
that was being typed before history motion started.

- **Leaving a line** (`_rl_maybe_replace_line`, RL/misc.c:335-358): if the
  current position is a history entry and the entry's data differs from
  `rl_undo_list` (meaning it was edited), replace the entry's text with
  the buffer and its data with `rl_undo_list`, keeping the timestamp
  (RL/history.c:487-500). Then set `rl_undo_list = NULL`. This uses the
  `(1)` form, which every motion command uses. An unedited entry is left
  alone.
- **Saving the new line** (`rl_maybe_save_line`): the first backward
  motion stores the buffer and its undo list as the saved line. That
  happens only if no saved line exists yet (RL/misc.c:410-416).
- **Arriving at an entry** (`rl_replace_from_history`, RL/misc.c:453-469):
  buffer = entry text, `rl_undo_list = entry.data` (possibly NULL),
  point = end, mark = 0. So undo works on an edited entry across visits,
  and on an unedited one it rings.
- **Returning to the new line** (`rl_maybe_unsave_line`,
  RL/misc.c:382-392): restore the saved text and undo list, then
  point = end and the mark is clamped. With no saved line: bell.
- **At accept** (`readline_common_teardown`, RL/readline.c:477-508):
  1. If the current position is a history entry with a non-empty undo
     list, that entry is reverted: all its edits are undone, its text is
     restored and data is set to NULL. The buffer, holding the edited text,
     is what readline returns, and bash adds it as a new entry.
  2. If `revert-all-at-newline` is on (default off, RL/readline.c:313),
     every entry from the current position back is reverted and its data
     freed (RL/misc.c:477-532).
  3. The remaining undo list is freed.
- So with the default settings, **entries edited and then left (not the
  one accepted) stay edited**: they keep the new text and their undo lists
  across commands. `history` and `fc -l` show them with `*`.
  **[oracle]**: after `C-p C-p X C-n DEL DEL Y RET`, `history` printed
  `1* echo OUT oneX`, `2  echo OUT two`, `3  echo OUT tY`. A later C-p back
  to entry 1 shows `echo OUT oneX`.
- At the start of each readline call the saved line, and its undo list,
  is freed and the position is reset to the end of history
  (RL/misc.c:309-318; RL/readline.c:1229).
- `mark-modified-lines` (default off): when the current position is a
  history entry and `rl_undo_list` is non-NULL, a `*` is drawn before the
  prompt (RL/display.c:874-880).

---------------------------------------------------------------------------

## 3. Kill ring (RL/kill.c)

### 3.1 Storage

- At most 10 entries (RL/kill.c:59). `rl_set_retained_kills` is a no-op.
  New slots are appended at the end. When all 10 are used, the oldest
  (slot 0) is dropped and the rest shift down (RL/kill.c:97-125).
- `kill_index` points at the newest slot after every kill
  (RL/kill.c:153). It moves only by kill and yank-pop.
- Adding text (RL/kill.c:91-155):
  - if the previous command was a kill (`last_was_kill != 0`) and the ring
    exists, the text joins the newest slot. It is **appended** if the kill
    went forward (from < to) and **prepended** if it went backward;
  - otherwise it goes in a new slot.
  - In vi mode the text never joins.

### 3.2 What counts as a kill

A command is a kill if it calls `kill_text(from,to)` or the region-copy
helper. Both increment `last_was_kill`, even when nothing is removed
(`from == to`; RL/kill.c:168-172, 436). `kill_text` copies [from,to),
deletes it (one DELETE record), and adds it to the ring. The direction is
`from < to`, and by convention `from` is the original point.

Commands that never call `kill_text` when there is nothing to do therefore
**break** the chain: kill-word at end of line, kill-line at end of line,
and the bell cases of the backward kills. kill-whole-line always calls it,
so it never breaks the chain.

Typeahead quirk: self-insert batches already-queued printable input
(RL/text.c:991-1033). When it stops on the first non-insert key, it pushes
that key back as pending input, and the "reset `last_was_kill`" step is
skipped for that iteration. So with fast typeahead (a paste, or piped
input), `M-DEL abc M-DEL` joins both kills into one slot. Typed at human
speed they do not join. **[oracle]** (piped): the kills joined unless a C-e
came between them.

### 3.3 The kill commands

In emacs mode the word, line and rubout kills (kill-line through
unix-filename-rubout below, and the shell-word kills of §4.5) set
`mark = point` afterwards. delete-char, backward-delete-char and the
region and copy commands do not touch the mark.

- **kill-line** (C-k; RL/kill.c:243-260): a count < 0 means
  `backward-kill-line(1)`. Otherwise it kills [point,end) if non-empty and
  appends. Point is unchanged, and the size of the count is ignored. At
  end: nothing, no bell, chain broken.
- **backward-kill-line** (C-x DEL; RL/kill.c:265-286): a count < 0 means
  `kill-line(1)`. At point 0: bell. Otherwise it kills [0,point) and
  prepends. Point = 0.
- **unix-line-discard** (C-u; RL/kill.c:406-418): at 0, bell. Otherwise
  it kills [0,point) and prepends. Point = 0. The count is ignored.
- **kill-whole-line** (unbound; RL/kill.c:290-298): in a group, point = 0,
  then kill [0,end) (appending, even when empty), mark = 0.
- **kill-word** (M-d; RL/kill.c:197-216): a count < 0 means
  backward-kill-word. Otherwise it runs forward-word(count) (§4.1) and kills
  [orig,new) if point moved, appending. Point returns to orig.
- **backward-kill-word** (M-DEL, M-C-h; RL/kill.c:220-238): a count < 0
  means kill-word. backward-word(count), then kill [new,orig) and prepend.
  Point = new. At 0 there is no bell, but the chain breaks.
- **unix-word-rubout** (C-w; RL/kill.c:307-334): at 0, bell. A count <= 0
  is treated as 1. Repeat count times: move back over blanks (space and tab
  only), then back over non-blanks. Kill [new,orig) and prepend.
- **unix-filename-rubout** (unbound; RL/kill.c:339-397): at 0, bell. Count
  as above. Per iteration:

  ```
  back over blanks
  if the char before point is '/':
      scan back over the run of '/'
      if the run reaches column 0, or is preceded by a blank:
          point = just after that blank (or 0); next iteration
  back over blanks and '/'s
  back over chars that are neither blank nor '/'
  ```

  For example, `cd /usr/local/` loses `local/`, and `cd /` loses only `/`.
  Kill and prepend.
- **delete-char** (C-d; RL/text.c:1294-1324): a count < 0 means
  `backward-delete-char(-count)`. At end of line: bell, failure. If
  `count > 1 || explicit`: move forward count characters, which rings if it
  overshoots but clamps; kill [orig,new) and append; point = orig.
  Otherwise delete one character with no ring. **[oracle]**: `M-1 C-d` then
  `C-y` re-inserts the char; with plain `C-d`, C-y finds an empty ring and
  aborts.
- **backward-delete-char** (DEL, C-h; RL/text.c:1230-1289): a count < 0
  means delete-char. At 0: bell. In overwrite mode see §8.5. If
  `count > 1 || explicit`: move back count characters (bell plus clamp to 0
  on overshoot), kill [new,orig) and prepend. Otherwise delete the previous
  character.
- **delete-horizontal-space** (M-\; RL/text.c:1341-1363): back over
  space/tab, then forward over space/tab, delete that span (not a kill),
  point = span start. No bell; the count is ignored.
- **kill-region** (unbound; RL/kill.c:423-458): if mark != point, copy
  [min,max), delete it, and add it (append if point < mark). Point =
  min(point, mark). The mark is not moved, only clamped to end. It counts
  as a kill even when mark == point. **The mark defaults to 0 on every new
  line**, so kill-region without set-mark kills from column 0 to point.
  **[oracle]**.
- **copy-region-as-kill** (unbound): the same without the delete. Point
  and mark are unchanged. Because it counts as a kill, **repeating it
  appends the region again**. **[oracle]**: two M-w on `abc` yank
  `abcabc`.
- **copy-forward-word / copy-backward-word** (unbound;
  RL/kill.c:463-506): a negative count swaps them. forward: run
  forward-word(n) and set a temporary mark there, then backward-word(n), and
  copy that region (append). backward: the mirror image (prepend). Point and
  mark are restored. Starting inside a word copies the whole word,
  including the part before point.
- The minibuffers of the non-incremental search (§6.6) and of
  execute-named-command map `C-w` and `C-u` to unix-word-rubout and
  unix-line-discard, so those **also add to the kill ring**.

### 3.4 Yank and yank-pop

- **yank** (C-y; RL/kill.c:510-521): an empty ring aborts (bell). Otherwise
  mark = point, then the slot at `kill_index` is inserted (one INSERT).
  Point ends after it. The count is ignored.
- **yank-pop** (M-y; RL/kill.c:528-556): allowed only if `last_func` is
  yank or yank-pop and the ring is non-empty. The text just before point
  must be byte-equal to slot `kill_index`. If so, delete it, decrement
  `kill_index` (wrapping to the newest slot), and yank. Anything else
  aborts with a bell. Since `kill_index` is not reset, **a later plain C-y
  yanks the rotated slot**, not the newest kill. **[oracle]**: kills x, a,
  b, c, then `C-y M-y M-y` yields `a`, and a later C-y also yields `a`.

---------------------------------------------------------------------------

## 4. Words, case, transposition

### 4.1 Word characters and motion

- A word character is an alphanumeric (RL/util.c:72-94). In a multibyte
  locale the character at a position is decoded with `mbrtowc`, and the
  test is the locale's `iswalnum`. Invalid sequences, and the final byte of
  the buffer, are taken as the raw (signed) byte, which is never
  alphanumeric (RL/mbutil.c:535-556). In single-byte mode it is `isalnum`
  on the byte. So **in a UTF-8 locale, letters such as é, ü and CJK count
  as word characters**. **[oracle]**: under C.UTF-8, M-u on `Ébc écrit`
  gives `ÉBC ÉCRIT`. Bytes >= 0x80 under the C locale depend on the C
  library: glibc does not class them alphanumeric, but Cygwin's C locale
  treated 0xE9 as part of a word. Steps always move by whole multibyte
  characters. (The pathname-characters extension, `/-_=~.#$`, is off.)
- Under a C/POSIX locale, `convert-meta` is on and 8-bit input bytes act
  as ESC+byte. In other locales convert-meta and input-meta are set for
  8-bit text (RL/nls.c:170-215).
- **forward-word** (M-f; RL/text.c:482-530): a count < 0 means
  backward-word. Per count: at end, stop (no bell). If the character at
  point is not a word character, skip non-word characters. If that reaches
  end, stop. Then step over word characters. Point ends just **after** the
  word.
- **backward-word** (M-b; RL/text.c:534-579): the mirror image, looking at
  the character before point. Point ends at the **start** of the word. At
  0, stop silently.
- forward-char and backward-char ring on overshoot and clamp
  (RL/text.c:264-455). In emacs mode, forward-char at end rings without
  moving.

### 4.2 Case commands (RL/text.c:1447-1581)

`upcase-word` (M-u), `downcase-word` (M-l), `capitalize-word` (M-c):

```
start = point; forward-word(count); end = point
if count < 0: swap(start, end)          # region = previous |count| words
record rl_modifying(start, end)         # one undo group
inword = false
for each char c in [start, end):
    if c is not a word char: inword = false; continue
    op = (capitalize ? (inword ? lower : upper) : op); inword = true
    replace c by its case-mapped form
point = end
```

- With a negative count the region is the |n| words **before** point, and
  point **does not move** (after the swap `end` is the original point).
  **[oracle]**: `M-- M-u` on `foo bar|` gives `foo BAR|`, and `M-- 2 M-u`
  does both words.
- Digits are word characters. They start a capitalized word but have no
  case, so `3abc` capitalizes to `3abc`. **[oracle]**.
- Multibyte case mapping can change the byte length, for example in
  Turkish. The line shifts and `end` is adjusted. Unchanged characters are
  skipped.
- count 0: forward-word(0) does nothing, so the region is empty and nothing
  happens.

### 4.3 transpose-chars (C-t; RL/text.c:1656-1710)

```
if count == 0: return
if point == 0 or end < 2: bell, fail
begin group
if point == end: point = previous char start; count = 1
take the char just before point (c), delete it  -> point = where c was
point = advance count chars from there (count <= 0 means no move; clamp to end)
insert c at point    -> point ends after c
end group
```

So the character before point is dragged forward `count` characters.
- `a|bcde`, count 1, gives `ba|cde`.
- At end of line the last two characters swap and point stays at the end.
- `a|bcde` with count 3 gives `bcda|e`; with count 9 it gives `bcdea|`.
- A negative count deletes and re-inserts the same character: no visible
  change, point unchanged, but an undo group is recorded.

All **[oracle]**.

### 4.4 transpose-words (M-t; RL/text.c:1592-1651)

```
if count == 0: return
forward-word(count);  w2end = point
backward-word(1);     w2beg = point
backward-word(count); w1beg = point
forward-word(1);      w1end = point
if w1beg == w2beg or w2beg < w1end: bell; point = orig; fail
group { replace [w2beg,w2end) by word1; replace [w1beg,w1end) by word2 }
point = w2end
```

- Only the two words swap; words between them stay. `a| b c d` with
  `M-2 M-t` gives `c b a| d`. (Emacs would move `a` past two words.)
  **[oracle]**.
- At end of line it swaps the last two words. Inside the first word of the
  line it rings. Inside a later word, that word is word2 and the previous
  word is word1.

### 4.5 Bash shell-words (BL:1058-1329)

The word delimiters (`WORDDELIM`) are the shell metacharacters plus blanks:
space, tab, newline, `|&;()<>`. Quoting is detected with bash's
`char_is_quoted` (the same scanner completion uses).

- **shell-forward-word** (M-C-f; BL:1064-1169), per count:
  - at end: point = end, done;
  - if the character at point is inside a quoted string (and not just
    backslash-escaped): advance until it is no longer quoted, and that
    counts as one word;
  - otherwise skip delimiters; a `\` skips the next character too, and
    quote characters skip to their closer;
  - if that reaches end: point = end, **bell**;
  - then advance over non-delimiters, skipping backslash pairs and quoted
    strings as units, to the first unquoted delimiter;
  - if that reaches end: point = end.

  Point ends **at** the delimiter after the word.
- **shell-backward-word** (M-C-b; BL:1172-1230), per count: at 0, stop.
  Step back one character, skip unquoted delimiters backward, then move
  back to the first character of the word, stopping just after an unquoted
  delimiter or at 0. No bell.
- **shell-kill-word** (M-C-d) and **shell-backward-kill-word** (unbound):
  the same as kill-word and backward-kill-word, using the shell motions
  (BL:1233-1271).
- **shell-transpose-words** (M-C-t; BL:1274-1329): the same algorithm as
  §4.4 with the shell motions, as one undo group.

---------------------------------------------------------------------------

## 5. Mark and region

### 5.1 The mark

- `mark` is 0 at the start of every line (RL/readline.c:756). It is never
  "unset": rl_mark is always a position.
- **set-mark** (C-@, M-SPC; RL/text.c:1871-1885): mark = point, or
  `count` if explicit. A position outside [0,end] silently fails, with no
  bell. It does not activate the region.
- **exchange-point-and-mark** (C-x C-x; RL/text.c:1889-1907): if mark >
  end, or mark < 0, then bell and mark = 0. Otherwise swap point and mark
  and **activate** the region. In practice there is no bell, because the
  mark is always clamped. **[oracle]**: no bell after typing.
- **Insertion does not shift the mark.** Deletion only clamps it into
  [0,end] (RL/text.c:159). So a mark set before point drifts relative to
  the text if you insert before it.
- Commands that set the mark:
  - yank, yank-nth-arg and yank-last-arg set mark = point before inserting;
  - bracketed paste does too (§8.7);
  - kills set mark = point after killing;
  - history motion sets mark = 0 (RL/misc.c:449,460);
  - history-search sets mark = end;
  - non-incremental search sets point 0 and mark end (or highlights the
    match, §6.6);
  - bash's expand-line commands set mark = 0.

### 5.2 Region kills

See kill-region and copy-region-as-kill in §3.3.

### 5.3 Active region and highlighting

- An active region is highlighted from point to mark. It uses the
  `active-region-start-color` and `active-region-end-color` sequences, or
  standout (`so`/`se`) by default. `enable-active-region` defaults to on
  but is forced off, together with bracketed paste, when the terminal does
  not look ANSI or is dumb (RL/terminal.c:678-681,743).
- Activated by: exchange-point-and-mark, bracketed paste, an incremental
  search match, and a non-incremental search match.
- Lifetime: activation also sets "keep active for this command". After
  each command, if that flag is set it is cleared; otherwise an active mark
  is deactivated (RL/readline.c:534-537). So a highlight survives exactly
  until the end of the next command. clear-screen and redraw-current-line
  keep it alive. accept-line deactivates it and redraws before accepting
  (RL/text.c:1141-1146).

---------------------------------------------------------------------------

## 6. History motion and searching

### 6.1 previous-history and next-history (RL/misc.c:571-717)

These are bound to C-p/C-n and the up and down arrows.

previous-history(count):

```
if count < 0: next-history(-count)
if count == 0 or the history list was never allocated: return  (no bell)
had_saved = saved line exists
save the current line if none is saved yet            # §2.4
leave the current entry (store edits)                 # §2.4
step back up to count times; if it stops early, use the oldest reached
if not even one step was possible: bell; if !had_saved: drop the saved line
else load that entry: point = end, mark = 0
```

next-history(count):

```
if count < 0: previous-history(-count)
if count == 0: return
leave the current entry (store edits)
step forward up to count times
if it ran past the last entry: restore the saved line (point = end), or
    ring the bell if there is no saved line
else load the entry: point = end, mark = 0
```

- Overshooting backwards lands on the oldest entry without a bell.
  Overshooting forward lands on the saved line without a bell.
  **[oracle]**: C-p at the oldest entry rings; C-n on the new line rings;
  C-p with an empty history list does not ring.
- `history-preserve-point` (default off): when on, point is kept at the
  same column across motion if possible (RL/misc.c:435-450,
  594-595, 651-652). When off, point always goes to end.
- **beginning-of-history** (M-<): `previous-history(1 + current index)`,
  which goes to the oldest entry. It rings if already there. **[oracle]**.
- **end-of-history** (M->): store edits, go to the end, restore the saved
  line. It rings if there is no saved line, for example when already on the
  new line. **[oracle]**.

### 6.2 operate-and-get-next (C-o; RL/misc.c:755-793)

```
accept-line
target = explicit ? count : current_index + history_base + 1
on the next readline call (once, after the normal startup hook):
    abs = target - history_base     # history_base read at that time
    previous-history(where_history() - abs)   # where = history_length
```

- With no argument, the next prompt shows the entry **after** the one just
  executed. That holds even when the new line was added and the oldest
  entry was dropped by stifling, because `history_base` absorbs the shift.
  If the accepted line was the new line (not from history), the target is
  one past the end: count 0, so the next line is simply empty.
- With an explicit argument N, the next line is the history entry
  **numbered N** (as `history` prints it), and the current line is still
  accepted. A target beyond the end makes previous-history get a negative
  count, which becomes next-history, which rings. A target before the
  oldest lands on the oldest entry.
- **[oracle]**: `C-p C-p C-p C-o C-o RET` runs entries 1, 2 and 3 in
  order. `M-2 C-o` on an empty line pre-loads entry 2.
- After an incremental search, C-o ends the search (§7.4), which moves to
  the found entry, so the line after it comes next. **[oracle]**.

### 6.3 yank-nth-arg (M-C-y) and yank-last-arg (M-., M-_)

- **yank-nth-arg** (RL/kill.c:604-662) works on the entry *before the
  current history position*. That is the previous command when on the new
  line, or the entry before the one being edited.
  - No entry: bell.
  - Word n comes from history-style tokenization (history.md §5.4). n = 0
    is the command word, `$` the last word, and negative n means
    `len + n - 1`, so -1 is the second-to-last.
  - An empty or missing word: bell.
  - Otherwise, in a group: mark = point, insert the word.
  - Without an argument the count is 1, the first argument.
  - **[oracle]**: on `echo OUT a b c d`, count 1 gives `OUT`, 3 gives `b`,
    -1 gives `c`, -2 gives `b`, and 0 gives `echo`.
- **yank-last-arg** (RL/kill.c:668-702) keeps static state across
  consecutive calls:

  ```
  if last_func != yank-last-arg:            # first call
      skip = 0; use_n = explicit; n = count; dir = +1
  else:                                     # repeat
      if the previous call inserted something: undo it (one group)
      if count < 0: dir = -dir
      skip = max(0, skip + dir)
  insert word (use_n ? n : '$') of the entry `skip` steps before the
      previous one
  ```

  - The argument given on later repeats is used only for its sign, which
    reverses the walk.
  - Walking past the oldest entry undoes the previous insertion and then
    rings, so **the text disappears**. Further repeats keep ringing.
  - **[oracle]**: on history `a1 b1`, `a2 b2`, `a3 b3`, M-. x3 gives b3, b2,
    b1; a 4th M-. leaves nothing. `M-. M-. M-- M-.` gives b2. `M-2 M-.`
    then `M-.` takes word 2 from successively older entries.

### 6.4 history-search-backward and history-search-forward (unbound; RL/search.c:584-725)

These are unbound by default. readline.md lists PgUp/PgDn termcap bindings
for them.

- The search string is the line **from 0 to point**, anchored. It is
  recomputed only when `last_func` is not one of this pair. It is stored
  with the original position, and the "previous match" text is reset.
- If the prefix is empty (point 0), the command is just
  previous-history or next-history(count).
- Otherwise it searches |count| times from the remembered position. A
  negative count reverses the direction. Each search step starts one entry
  past the last match. A match whose text equals the previous match's text
  is skipped and does not consume the count.
- Found: load it through history motion (so the §2.4 rules apply).
  Point = prefix length, mark = end.
- Not found at all: bell, and the line stays as it is (the last match, if
  any). Point = prefix length, mark = end. If some matches were found
  before failing, it uses the last one, with no bell.
- **[oracle]**: prefix `echo OUT a` steps through `apricot`, then `apple`,
  then rings and stays on `apple` with point after the prefix.
- `search-ignore-case` (default off) makes the comparison
  case-insensitive.

### 6.5 history-substring-search-backward and -forward (unbound)

These are the same as §6.4, except the match is unanchored and point is
the **start of the match** within the found line. Backward searches use the
**last** occurrence in a line and forward searches the **first**
(RL/histsearch.c:160-195; RL/search.c:647-658). **[oracle]**: prefix `an`
lands on `echo OUT ban|ana`.

### 6.6 Non-incremental search (M-p reverse, M-n forward; RL/search.c:56-560)

Starting the search (RL/search.c:227-264):

- remember point, mark and the history position;
- stash the line and its undo list;
- empty the buffer, with a fresh undo list;
- show the prompt: the **last line of the current prompt string** (the
  part after its final newline) followed by `:` (RL/display.c:3264-3298).
  With bash's default PS1 (`\s-\v\$ `) the minibuffer looks like
  `bash-5.3$ :`, and the typed string is drawn after it.

Minibuffer keys (RL/search.c:305-388). These are not keymap lookups:

| key | action |
|---|---|
| C-w | unix-word-rubout (adds to the kill ring) |
| C-u | unix-line-discard (adds to the kill ring) |
| C-q, C-v | quoted-insert of the next key |
| RET, C-j | finish |
| C-h, DEL | delete the previous char; **if the string is empty, abandon the search silently** (restore line, point and mark, no bell) |
| C-c, C-g | bell and abandon |
| ESC | if bracketed paste is on and at least 5 more bytes are queued, try to read a paste and insert it; otherwise insert a literal ESC |
| anything else | inserted literally, including control chars (shown as `^X`) |

Finishing (RL/search.c:398-454):

- **empty string**: reuse the previous search string, continuing from the
  **position of the previous match** (not from the current position). If
  there was never a previous string: bell, restore everything, fail.
  **[oracle]**: after `M-p apple` found the newer `apple y` and that line
  ran, `M-p RET` found the older `apple x` even though the history had a
  newer matching entry by then.
- **non-empty**: it becomes the remembered string, and the search starts
  from the history position at which M-p was pressed.
- The original line is restored, then the search runs one step from
  `pos + dir`. A leading `^` anchors the match to the start of a line.
  Otherwise any substring matches.
- Found: move there by history motion. If the active region is enabled,
  point = match offset and mark = offset + strlen(string), with the region
  active. The length counts a leading `^`, so an anchored match highlights
  one extra character. Otherwise point = 0, mark = end. **[oracle]**:
  `M-p pple RET` then `_` gives `echo OUT a_pple x`.
- Not found: bell. The line, point and mark stay as before the search.
- The "again" variants (non-incremental-*-search-history-again) reuse the
  string without prompting. With no string: bell. They are unbound in emacs
  mode.

---------------------------------------------------------------------------

## 7. Incremental search (C-r, C-s; RL/isearch.c)

### 7.1 Start (RL/isearch.c:133-144, 206-266, 907-937)

- direction = -count for C-r and +count for C-s, then normalized:
  >= 0 is forward. So `M-- C-r` searches forward and **`M-0 C-r` searches
  forward**.
- The current history entry's edits are stored first (§2.4 "leaving").
- A snapshot array is built: every history line, plus one extra slot for
  "the line being typed". That slot is the saved line if one exists,
  otherwise a copy of the buffer.
- The search starts in the **current line at point**: index = point,
  history position = current. So C-r can match text earlier in the line
  being edited.
- Display: the prompt is replaced by a message, with the line still drawn
  after it:
  - `(reverse-i-search)`` + search + `': ` for reverse;
  - `(i-search)`` + search + `': ` for forward;
  - prefixed with `failed ` while failing, giving for example
    `(failed reverse-i-search)`foo': ` (RL/isearch.c:152-203).

  The cursor is at the match start in the displayed line. If
  enable-active-region is on, the match is highlighted (mark = point +
  length; RL/isearch.c:873-884).

### 7.2 Key classification (RL/isearch.c:343-571)

Each key is looked up in the keymap (following prefix maps). The function
it is bound to decides the opcode:

| bound function / key | effect |
|---|---|
| reverse-search-history | search again in the reverse sense (or switch to reverse) |
| forward-search-history | search again forward (or switch to forward) |
| backward-delete-char (DEL, C-h) | remove the last search char |
| abort, or C-g | cancel |
| unix-word-rubout, or C-w | pull a word from the line |
| yank, or C-y | pull the rest of the line |
| quoted-insert, or C-v | add the next key literally |
| bracketed-paste-begin | add the pasted text |

Otherwise:

- **isearch-terminators** (default ESC and C-j, `"\033\012"`,
  RL/isearch.c:77): end the search and **do not** execute the key. ESC is
  special: if more input is already queued, or arrives within 0.1 s (the
  keyboard input timeout, RL/input.c:109), the ESC is pushed back so the
  ESC-sequence runs after the search ends. So arrow keys and M-f end the
  search and act. **[oracle]**: `C-r foo ESC _` ran `M-_` rather than
  inserting `_`. A prefix keymap key that is a terminator does not enter
  its keymap.
- Any **control character**, meta character or DEL that is not one of the
  opcodes above (C-g excepted) ends the search, and the key is then
  executed normally on the found line. So RET accepts it, C-a and C-e move
  in it, and C-n and C-p move relative to the found entry
  (RL/isearch.c:336-337, 549-570).
- A prefix key (C-x, or ESC when ESC is not a terminator) reads the next
  key. If the sequence resolves to a command other than the opcodes and
  self-insert, the search ends and that command runs once the search has
  finished. If the prefix key alone would have ended the search, the search
  ends and both keys are re-read.
- Printable characters, including ones bound to other commands, are
  appended to the search string. A multibyte character is appended whole.

### 7.3 Opcodes (RL/isearch.c:577-771)

- **Again** (C-r in reverse, C-s in forward):
  - if the search string is empty: load the last search string from the
    previous isearch, show it and search. If there is none, do nothing;
  - else in reverse: index -= 1, then search;
  - forward: index += 1 if not at line end, otherwise ring (and the search
    then continues into later lines).
- **Switch** (C-s while reverse, or the opposite): flip the direction and
  **search from the same index**, so the first press re-finds the same
  match and only changes the prompt. The second press moves on.
  **[oracle]**: `C-r foo C-r C-s` stays on `foo1`; another C-s moves to
  `foo2`.
- **DEL**: if the string is empty, bell. Otherwise drop its last
  character. If that leaves it empty, bell again. The search then re-runs
  from the current index, so the position does not go back to where the
  shorter string first matched. An empty string is reported as a failure:
  another bell and the `failed` prompt.
- **C-w**: from the character after the current match (point + string
  length): if that is past the end or not a word character, bell.
  Otherwise append the run of word characters starting there.
- **C-y**: from the same position, append the rest of the line. Bell if
  nothing is left.
- **C-v**: read the next key with tty signals disabled and append it.
- **C-g** (RL/isearch.c:636-645): restore the original line text, point
  and mark, and the original prompt. **No history motion and no bell.**
  The search string is **not** saved as the "last search string".

### 7.4 The search step and finishing

Search step (RL/isearch.c:773-855): match the string at the index in the
current snapshot line, moving one byte at a time in the current direction.
When the line is exhausted, move to the next snapshot line in that
direction, starting at the end (reverse) or at 0 (forward). Lines shorter
than the string are skipped, and so are lines byte-identical to the last
found line, so duplicate entries are not revisited. `search-ignore-case`
gives a case-insensitive match.

- **Found**: the displayed buffer becomes that line and point = match
  start. This is a *display* change only: the history position is not
  changed yet.
- **Failed** (ran off either end): bell, `failed` prompt. The display stays
  on the last successful line and point is not moved.

Finishing, on a terminator or another key (RL/isearch.c:269-312):

1. Restore the original buffer text.
2. Save the search string for the next "again with empty string".
3. Move by ordinary history motion from the original position to the last
   found entry. That stores edits, saves the new line, and sets the
   per-entry undo list (§2.4). **The found entry becomes the current
   history position**, so C-n, C-p and C-o continue from there.
4. Point = the index where the scan stopped, clamped. After a successful
   match that is the match start. After a failure it is a leftover scan
   index, which in practice is **0**. **[oracle]**: after
   `C-r fooz C-j`, and after `C-r foo C-r C-r` (no third match) then C-j,
   the next typed character landed in column 0 of the last found line.
5. Deactivate the region and clear the message.

Edge: when the search ends without moving (a match in the line where it
started, or no match) while the starting line was a history entry, step 1
of §7.1 has already stored the entry's undo list and cleared the live one.
The live line then has no undo history. Edits made before the search can
no longer be undone from this visit.

---------------------------------------------------------------------------

## 8. Miscellaneous commands

### 8.1 Self-insert, quoted-insert, tab-insert

- **self-insert**:
  - count <= 0 inserts nothing;
  - count 1 inserts the character;
  - count 2..1024 inserts one string of count copies;
  - larger counts insert in 1024-copy chunks (RL/text.c:714-943).

  A multibyte character is assembled from its bytes before insertion.
  An invalid sequence inserts its first byte as a character.
- **quoted-insert** (C-q, C-v; RL/text.c:1039-1126): disable tty signal
  characters, read one key, and insert it count times. A **negative** count
  reads and inserts -count keys, once each. **[oracle]**: `M-3 C-v x`
  gives `xxx`, and `M-- 2 C-v y z` gives `yz`.
- **tab-insert** (bash moves it off M-TAB): inserts count tabs.

### 8.2 insert-comment (M-#; RL/text.c:1384-1407)

```
point = 0
s = comment-begin (default "#"; bash leaves it unset except posix mode sets "#", BL:354)
if not explicit: insert s at 0
else if the line starts with s exactly: delete it      # no whitespace skipping
else: insert s at 0
redisplay; accept-line
```

It always accepts the line. **[oracle]**: `M-1 M-#` on `#echo OUT x` ran
`echo OUT x`. On an uncommented line it inserted `#` and accepted a
comment.

### 8.3 character-search (C-]) and character-search-backward (M-C-])

From RL/text.c:1720-1861:

- Read one character. A multibyte character is read whole, and a bracketed
  paste supplies its first character.
- A count < 0 searches in the other direction. A count of 0 does nothing.
- Forward: per count, start at point+1 (the character under point is never
  matched) and stop **on** the next occurrence.
- Backward: start at point-1 and land **on** the occurrence.
- Each found occurrence moves point immediately. If the next search fails
  (or point is already at the edge): bell, and point stays at the last
  found occurrence.
- **[oracle]**: `ab|cabc` with `C-] c` puts point on the first `c`, a
  second `C-] c` on the last `c`, and a third rings.

### 8.4 Screen commands

- **clear-screen** (C-l; RL/text.c:594-608): with an explicit argument it
  acts as redraw-current-line. Otherwise it clears the screen with termcap
  `cl` (just a newline if the terminal lacks it), redraws prompt and line
  at the top, and keeps the region active.
- **clear-display** (M-C-l; RL/text.c:611-618; RL/display.c:3344-3359):
  `cl`, plus `E3` (erase scrollback) if the terminal has it, then redraw.
- **redraw-current-line** (unbound; RL/text.c:583-588,
  RL/display.c:3590-3595): clear the screen rows the current line
  occupies, redraw the last line of the prompt and the buffer in place, and
  keep the region active.

### 8.5 overwrite-mode (unbound, Insert key via termcap `kI`; RL/misc.c:843-853)

- No explicit argument: toggle. An explicit positive argument turns it on;
  an explicit argument <= 0 turns it off. Every readline call starts in
  insert mode (RL/readline.c:1246).
- Self-insert in overwrite mode (RL/text.c:949-982), in one group: per
  count, insert the character, then if point < end delete one character
  after it. At end of line it appends.
- backward-delete-char in overwrite mode (RL/text.c:1188-1226): at 0, bell.
  In one group: move back count characters, summing their display widths
  (a control character counts 2). Remove them, as a kill if
  `count > 1 || explicit`. If point < end, insert that many spaces and
  leave point before them. At end of line it just deletes.
- delete-char and the rest behave normally. The cursor-shape switch
  (`vs`/`ve`) is compiled out by default (RL/rlconf.h:63).

### 8.6 delete-char-or-list and EOF

- **delete-char-or-list** (unbound; RL/text.c:1369-1375): if the line is
  non-empty and point is at end, run possible-completions; otherwise
  delete-char.
- The tty EOF character (usually C-d) typed on an **empty** line makes
  readline return EOF before any dispatch (RL/readline.c:677-687). Bash
  handles IGNOREEOF.

### 8.7 Bracketed paste

- When `enable-bracketed-paste` is on (default on, but forced off for dumb
  or non-ANSI terminals), each readline call's terminal setup writes
  `ESC[?2004h`, and the restore writes `ESC[?2004l` followed by `\r`. If
  readline is returning EOF, or the terminal is not echoing, a `\n` is
  written after that (RL/rltty.c:676-711; RL/rlprivate.h:365-372).
- `ESC[200~` is bound to bracketed-paste-begin (RL/kill.c:754-768). It
  reads bytes until `ESC[201~`, turning every CR into LF
  (RL/kill.c:708-747). Then: mark = point; insert the whole text with
  **one** insert call, which is one INSERT undo record (merge-eligible only
  if the text is one byte); activate the region if enable-active-region is
  on. **Newlines are inserted into the buffer**; the line is not accepted.
  Bash later executes the multi-line buffer when RET is pressed. No
  1024-byte chunking applies.
- Commands that read a single character (character-search, the
  digit/universal argument terminator, and others) use a paste-aware read:
  the first pasted byte is the character and the rest is pushed back as
  input (RL/kill.c:806-838). The isearch and minibuffer ESC handling is
  described in §6.6 and §7.2.

### 8.8 Keys, ESC and timeouts

- Meta bytes with convert-meta on: an 8-bit byte is converted to ESC followed by
  the byte with bit 7 cleared, if ESC is a prefix in the current map;
  otherwise it rings (RL/readline.c:889-904). Terminals that send ESC+key
  for Alt reach the meta map directly.
- **do-lowercase-version** is bound to M-A..M-Z and C-x A..Z
  (RL/emacs_keymap.c:651-676 for C-x). It re-dispatches the lowercase key
  in the same map. If the key has no lowercase form: bell
  (RL/readline.c:916-928). So `M-F` = `M-f` and `C-x E` = `C-x e`.
- **keyseq-timeout** (default 500 ms; values <= 0 mean wait forever;
  RL/readline.c:272; RL/bind.c:2199-2212). In emacs mode it matters only
  when a prefix key **also has a function bound to the prefix itself**
  (the keymap's "shadow" slot). That happens, for example, after the user
  binds `"\e": something` in addition to `\e`-sequences. Then, if no
  further input arrives within the timeout, the shadowed function runs
  (RL/readline.c:1064-1078). With the default bindings no prefix has a
  shadow, so **a lone ESC waits indefinitely and the next key, however
  late, is read as Meta-key**. This corrects readline.md §1, which implied
  that the timeout separates ESC from `ESC [ A` by default. The 0.1 s ESC
  rule in isearch (§7.2) is separate.
- **prefix-meta**: there is no command function. An inputrc line
  `key: prefix-meta` binds that key to the meta keymap, so the key acts
  like ESC (RL/bind.c:1845-1855).
- C-x map defaults: see readline.md §2. Besides the uppercase letters
  above, it has C-g abort, C-r re-read-init-file, C-u undo, C-x
  exchange-point-and-mark, `(` `)` `e` keyboard macros, and DEL
  backward-kill-line. Bash adds C-e, C-v, `*`, `g`, `/`, `~`, `@`, `$`,
  `!` and `s`.
- **execute-named-command** (M-x, new in 8.3; RL/text.c:2306-2392): the
  prompt is the last prompt line plus `!`. It uses the same minibuffer keys
  as §6.6, except that SPACE and TAB complete the command name. An unknown
  name rings. The name runs with the current count.

---------------------------------------------------------------------------

## 9. Bash's own commands (BL)

### 9.1 Line replacement helper

All expand commands except shell-expand-line use one helper
(BL:2808-2875):

- If the new text equals the buffer: nothing happens, no undo record, and
  point stays.
- Otherwise: one undo group that deletes the whole line and inserts the
  new text, then mark = 0. Point placement, with `dist = old_end -
  old_point`:
  - if point was at end: end;
  - else if old point was 0: 0, then forward-word if the character there
    is not a blank;
  - else if the new end < old point: old point, then forward-word if
    non-blank. A variable meant to account for the length change is
    computed before the change and is always 0, so this branch triggers
    only when the line got shorter than the old point;
  - otherwise: `new_end - dist`, the same distance from the end;
  - clamp.

  **[oracle]**: `echo OUT !! zz` with point before ` zz`, after M-^, has
  point before ` zz`. With point at column 2 it lands at column 10
  (`new_end - dist`).
- Errors (`cleanup_expansion_error`, BL:2783-2802): write `\r\n`, re-run
  the history expansion with error printing (for example
  `bash: !x: event not found`, or the `:p` text) to stderr, write `\r`, and
  redraw. The line is unchanged and the command returns failure.

### 9.2 The expansion commands

- **history-expand-line** (M-^; BL:2902-2918): history expansion of the
  whole line with `histverify` forced off. `:p` and errors take the error
  path (BH:562-631).
- **magic-space** (unbound; BL:2923-2940): history-expand-line. On success
  point = (old point 0 ? 0 : new_end - old_distance_from_end), then
  self-insert a space. On failure no space is inserted.
- **alias-expand-line** (unbound) and **history-and-alias-expand-line**
  (unbound): the alias expansion that the parser would perform (bash's
  `alias_expand` on the line), optionally after history expansion
  (BL:2880-2975).
- **shell-expand-line** (M-C-e; BL:2982-3062):
  1. Apply history expansion, then `$'...'`/`$"..."` translation, then
     alias expansion, and install the result as one undo unit.
  2. Separately, expand the whole buffer as one word: parameter, command
     and arithmetic substitution, word splitting, quote removal, and so on.
     The resulting words are joined with spaces and installed as a second
     undo unit.
  3. Point: at end if it was at end. Otherwise, if old point < new end,
     use old point, plus forward-word if non-blank. Otherwise point is left
     at end. This placement runs even when neither step changed the line,
     so M-C-e with point inside a word moves point to the word's end.
     **[oracle]**: `a|bc def` became `abc| def`.
  4. With an explicit argument, command and process substitution are
     suppressed and the text is expanded like a here-document
     (BL:3024-3025).
- **glob-expand-word** (C-x *) and **glob-list-expansions** (C-x g): run
  completion with bash's glob generator. `*` replaces the word with all
  matches; `?` lists them. With an explicit argument a `*` is appended to
  the word first (BL:4004-4093). glob-complete-word (M-g) always forces
  that in emacs mode.
- **dynamic-complete-history** (M-TAB; BL:3735-3832): completion whose
  candidates are every `history_tokenize` word of every history line,
  sorted. A second consecutive M-TAB lists the matches (completion mode
  `?`) instead of completing.

### 9.3 edit-and-execute-command (C-x C-e; BL:933-1032)

```
accept-line                                     # the line is "done"
if explicit: cmd = 'fc -e "${VISUAL:-${EDITOR:-emacs}}" N'
else:
    add the buffer to history (as a continuation if mid-command)
    add a dummy empty entry; history_lines_this_session++
    cmd = 'fc -e "${VISUAL:-${EDITOR:-emacs}}"'
restore the terminal; run cmd as a nested command string (no history),
    with the name "C-xC-e" for error messages
re-prep the terminal; empty the buffer (point = end = 0); clear "done";
    force a redisplay of an empty line
```

- `fc` with no operands edits the **last** entry. It replaces the dummy
  entry with the edited command, echoes it, and executes it.
- The editor value is one quoted word, so `VISUAL="vim -u NONE"` fails as
  a command name. The vi-mode version uses `vi` as the final fallback, and
  `fc -e vi` in posix mode.
- **[oracle]**:
  - `echo OUT two` then C-x C-e (the editor substitutes `OUT` with
    `OUT edited`) echoed and ran `echo OUT edited two`. History became
    `1 echo OUT one / 2 echo OUT two / 3 echo OUT edited two`, and an empty
    prompt followed.
  - With `M-1` on a line `XX`: entry 1 was edited and run, and `XX` was
    discarded (not in history).
- The prompt after the editor returns is a fresh empty line in the same
  readline call. The executed command is not something the user can undo.

### 9.4 display-shell-version (C-x C-v; BL:725-735)

Writes a newline, then `GNU bash, version 5.3.N(1)-release (MACHTYPE)` to
**stdout** (VER:90). Then it writes `\r` and redraws the prompt and line
below. **[oracle]**: the version string on this machine is
`5.3.15(1)-release (x86_64-pc-cygwin)`.

---------------------------------------------------------------------------

## 10. Things not determined

- The exact screen output of the visible bell (`vb`) depends on termcap.
  The active-region colours depend on the terminal and on user variables.
- Cursor placement after a *failed* incremental search depends on scan
  leftovers (§7.4). Point ends at 0 in the observed cases, but a
  forward-only failure that never reaches a short line could leave another
  value. A clone can simply use 0.
- `isalnum` for bytes >= 0x80 in the C locale is C-library specific
  (§4.1).
- bash's `char_is_quoted` details (backslash/quote parsing for the shell
  word commands) were not traced beyond the summary in §4.5.
