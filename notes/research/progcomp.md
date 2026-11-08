# Bash 5.3 programmable completion: semantics digest

Purpose: enough detail to clone bash 5.3's programmable completion
(`complete`, `compgen`, `compopt`, compspec lookup, the generation
pipeline and its hand-off to readline) without reading the GPL source.
Everything is paraphrase; the only verbatim text is user-visible output
(`-p` formats, usage lines, error messages), which the clone must match.
completion.md is the companion for the non-programmable engine (word
finding, the classifier, filename generation, quoting, insertion and
listing); this file assumes it and points into it as `C§n`.

Citations are relative to the `bash-5.3/` root of `bash-5.3.tar.gz`:

- PC = `pcomplete.c`, PH = `pcomplete.h`, PL = `pcomplib.c`,
  CD = `builtins/complete.def`, BL = `bashline.c`, SU = `subst.c`,
  HL = `hashlib.c`, SQ = `lib/sh/shquote.c`, SL = `lib/sh/stringlist.c`,
  SB = `stringlib.c`, CM = `builtins/common.c`, GO =
  `builtins/bashgetopt.c`, VA = `variables.c`, PX = `pathexp.c`,
  RL = `lib/readline/` (complete.c unless named), RU =
  `lib/readline/doc/rluser.texi` (the Programmable Completion chapter
  is there, included into bashref), BR = `doc/bashref.texi`.

Claims marked **[oracle]** were checked against GNU bash 5.3.9 in the
local `ubuntu-fuzz` docker image (LANG=C.UTF-8). Builtin behaviour was
run non-interactively (`bash -s`, so error lines carry `line N:`);
TAB behaviour was driven through a pty (python `pty.fork`, `bash -i
--rcfile`) with a `-F` function logging its arguments and variables.
This is a different oracle from completion.md's `unabashed/bash53`
container; re-record goldens there.

---------------------------------------------------------------------------

## 0. Model

A **compspec** is a record attached to a name in one global table. When
readline asks bash for matches (bash's attempted-completion hook,
`attempt_shell_completion`, BL:1587-1825), bash decides whether the word
is eligible for programmable completion (§8), finds the command name of
the current simple command, looks up a compspec (§1.3), and runs its
pipeline (§5): actions, `-G`, `-W`, `-F`, `-C`, then `-X`, `-P`/`-S`,
then the `dirnames`/`plusdirs` additions. The resulting list goes back to
readline as an ordinary match list; the compspec's `-o` options set the
readline variables that govern sorting, quoting and the append character
(§5.9). With no compspec found, or when the compspec asks for it, bash's
default completion (C§4) runs instead.

`compgen` runs the same pipeline on a word without readline, printing
the list. `compopt` edits compspec options, either stored ones or the
copy belonging to the completion in progress.

---------------------------------------------------------------------------

## 1. The compspec and the table

### 1.1 Fields (PH:28-40, PL:48-111)

| field | set by | meaning |
|-------|--------|---------|
| actions | `-A name`, `-abcdefgjksuv` | bit set, 24 actions (§7), PH:44-67 |
| options | `-o name` | bit set, 9 options, PH:70-79 |
| globpat | `-G` | string or absent |
| words | `-W` | string or absent |
| prefix, suffix | `-P`, `-S` | strings or absent |
| funcname | `-F` | string or absent |
| command | `-C` | string or absent |
| filterpat | `-X` | string or absent |
| refcount | | sharing, see below |

Option bits in table order (this is also the print order, CD:140-154):
`bashdefault default dirnames filenames fullquote noquote nosort nospace
plusdirs`. Bit 0 is reserved (PH:70); the lookup code reuses it to mean
"a compspec was found" (§8.3). Two more bits above the options are used
internally: "function asked for retry" and "not found" (PH:83-84).

Absent and empty differ for `-W`: an empty `-W ''` is stored, printed
as `-W ''`, and generates nothing (PC:870-871). A later option of the
same letter replaces an earlier one (**[oracle]** `compgen -W 'a a b'
-W 'c'` prints only `c`).

**Sharing.** One `complete` command builds one compspec and inserts that
same object under every name it lists (CD:466-485, PL:170-190). `compopt
NAME` edits the stored object in place (CD:894-912), so it changes every
name defined by the same `complete` command. **[oracle]** after
`complete -o filenames -o nospace ... foo 'b ar' ''`, `compopt -o nospace
+o filenames foo` also turns `filenames` off for `'b ar'`. A later
`complete ... foo` gives `foo` a new object and breaks the link.

### 1.2 The table (PL:40, 113-218; HL)

A hash table keyed by the exact name string (case-sensitive), created on
first insert with 512 buckets. Insert replaces the value of an existing
key in place, otherwise adds a new entry. Remove deletes one key. Flush
(`complete -r` with no names) empties it.

Special entries use reserved key strings (PH:109-111): `-D` is
`_DefaultCmD_`, `-E` is `_EmptycmD_`, `-I` is `_InitialWorD_`. They are
ordinary keys: a user could type `complete -W x _DefaultCmD_` and get the
`-D` entry, and the error messages print the raw key (§2.6).

**Listing order** (`complete` / `complete -p` with no names) is the hash
walk order, which the clone must reproduce to match output:

- Bucket index = hash & (nbuckets-1), buckets visited 0 upward
  (HL:51, 397-412).
- Hash: 32-bit FNV-1: start at 2166136261; for each byte, multiply by
  16777619 (mod 2^32), then XOR the byte (HL:208-219). The byte is a
  plain `char`, signed on x86/x86-64 gcc, so a byte >= 0x80 is XORed
  sign-extended (0xFFFFFF80 | b). Match that if names may be non-ASCII.
- A new key goes to the **head** of its bucket chain (HL:335-339); so
  within a bucket, most recently added first. Re-defining an existing key
  keeps its position; removing and re-adding moves it to the head.
  **[oracle]** defining `a b c d e` in that order lists `e d c b a`:
  they land in distinct buckets (FNV-1 of `a` is 0x050c5d7e, bucket 382;
  `b` 381 ... `e` 378), so this checks the hash, not chain order. A
  later `complete -W 2 c` leaves `c` in place.
- The table grows (bucket count x4, chains rebuilt by prepending in old
  walk order, which reverses within-bucket order) when an insert finds
  entries >= 2 x buckets, i.e. at the 1025th compspec (HL:40-41,
  130-160, 330-333). Irrelevant in practice; note it.

**[oracle]** `complete -F _f ... foo 'b ar' ''; complete -D ...;
complete -E ...; complete -I ...` lists in the order `foo`, `-I`,
`'b ar'`, `-D`, `-E`, `''`. Use this as a hash-order test.

### 1.3 Lookup for a command word (PC:1552-1684, 1491-1546)

`programmable_completions(cmd, word, start, end)` where `cmd` is the
command name (or one of the special keys, §8.2):

1. Loop, at most 33 passes:
   1. Look up `cmd` exactly.
   2. If not found and `cmd` contains a `/` with something after the
      last one, look up the part after the last `/`.
   3. If still not found, look up `_DefaultCmD_` (the `-D` entry).
   4. If still not found, nothing asked to retry, the `progcomp_alias`
      shopt is on, and `cmd` is an alias: splice the alias value in
      place of `cmd` in a private copy of the line (shifting point and
      the command end by the length difference), take the value's first
      word (up to the first of `()<>;&|` space tab newline) as the new
      `cmd`, and loop. An empty value, or a value starting with a break
      character, stops the loop (PC:1599-1647). **[oracle]** with
      `alias al='foo -x'` and `shopt -s progcomp_alias`, `al q` TAB calls
      foo's function with `$1=foo`, `COMP_LINE="foo -x q"`,
      `COMP_POINT=8`, `COMP_CWORD=2`. Because step 3 runs first, a `-D`
      compspec prevents alias expansion entirely (**[oracle]**).
   5. Loop again only if a function returned 124 (§5.6) or an alias was
      expanded. After the 33rd pass print the warning
      `programmable_completion: CMD: possible retry loop` (internal
      warning format, `bash: warning: ...`) and stop (PC:1650-1656).
      **[oracle]** a function that re-registers itself and returns 124
      runs 33 times.
2. A compspec counts as "found" only if it is not the same object that
   was used on the previous pass (PC:1504-1516); that is what ends a 124
   loop when the function did not install anything new.
3. Each run uses a **private copy** of the compspec (PC:1518), which is
   what `compopt` with no names edits (§4). `pcomp_curcs`/`pcomp_curcmd`
   are set to that copy and to the key that matched (`foo`,
   `_DefaultCmD_`, ...; PC:1520-1532); the key is what `compopt` with no
   names prints as the name.
4. The function arguments use `cmd` as it was passed in (the full word,
   or the special key), not the key that matched: the basename lookup
   still passes `$1=/usr/bin/foo` (**[oracle]**), and the `-D` entry run
   for an empty line gets `$1=_EmptycmD_` (**[oracle]**).

The "found" result handed back up is: 0 (nothing), or the found bit
OR'd with the copy's final options (after any `compopt` inside the
function), PC:1534-1542. Quirk (source): if the run reported "not
found" (a function returned 127, §5.6) but the compspec has any `-o`
option, the OR makes the result non-zero, so it is treated as found with
no matches and the basename/`-D` lookups are skipped.

---------------------------------------------------------------------------

## 2. The `complete` builtin (CD:23-50, 386-650)

Usage line (CD:26), printed by `builtin_usage` as
`complete: usage: complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]`
(on stderr; the `complete: usage: ` prefix has no `bash:` and no line
number, CM:128-134).

### 2.1 Option parsing (CD:194-383, GO)

Bash's internal getopt with option string `abcdefgjko:prsuvA:G:W:P:S:X:F:C:V:DEI`
(shared with `compgen`). Its rules (GO:48-188):

- Options must precede names; the first word not starting with `-` (or
  a lone `-`) ends them; `--` ends them and is consumed. No `+` options
  for `complete`: `+o` is a name (**[oracle]** `complete +o nospace q`
  is a print request for three names, §2.3).
- Letters cluster (`-ab`); an argument may be attached (`-Wfoo`) or be
  the next word (even if it starts with `-`).
- `--help` (exact word, first option position) prints the long help and
  returns 2.
- Unknown letter: `complete: -z: invalid option` then the usage line;
  status 2 (EX_USAGE is 258 internally and becomes 2).
- Missing argument: `complete: -A: option requires an argument` then
  the usage line; status 2.
- Options are processed left to right and the first error returns
  immediately (earlier options have no effect since nothing is stored).

Per option:

| option | effect |
|--------|--------|
| `-a -b -c -d -e -f -g -j -k -s -u -v` | add action alias, builtin, command, directory, export, file, group, job, keyword, service, user, variable |
| `-A NAME` | add the named action (§2.2). Unknown: `complete: NAME: invalid action name`, status 2, **no** usage line |
| `-o NAME` | add option. Unknown: `complete: NAME: invalid option name`, status 2, no usage |
| `-G -W -P -S -X -C ARG` | set the string (last one wins) |
| `-F NAME` | set function name. Rejected if it contains any of `()<>;&|` space tab newline: `` complete: `NAME': not a valid identifier ``, status 2. In posix mode it must also be a valid identifier and not all digits; then bash prints the identifier message twice (once as `bash: `NAME': not a valid identifier` from the checker, once with the `complete:` prefix), CD:329-337, general.c:356-370 |
| `-p -r` | flags (§2.3) |
| `-D -E -I` | flags (§2.3) |
| `-V NAME` | `complete: -V: invalid option` + usage, status 2 (the argument is consumed first) |

### 2.2 Action names (CD:107-137)

In table order, with the letter shortcut if any (this order matters for
`-p`, §2.4):

| name | letter | name | letter |
|------|--------|------|--------|
| alias | a | helptopic | |
| arrayvar | | hostname | |
| binding | | group | g |
| builtin | b | job | j |
| command | c | keyword | k |
| directory | d | running | |
| disabled | | service | s |
| enabled | | setopt | |
| export | e | shopt | |
| file | f | signal | |
| function | | stopped | |
| | | user | u |
| | | variable | v |

(The table lists hostname before group.) Generators: §7.

### 2.3 What `complete` does (CD:386-489)

1. No arguments at all: print every compspec (§2.4), status 0 (write
   errors not checked).
2. Parse options. `-D`, `-E`, `-I` select one special key, precedence
   D > E > I; when one is given, any names are **ignored** (CD:417-424).
3. `-p` given, or neither options nor names: print. With a special
   key, print just that entry; with names, print each; with neither,
   print all. `-p` beats `-r`. (`complete foo` alone is not a print:
   it defines an empty compspec for foo, status 0, recorded in
   t/exec/940.)
4. `-r` given: remove the special key's entry, or each name, or with
   neither flush the whole table (status 0 even if empty).
5. Options given but no names and no special key: usage line only (no
   message), status 2 (**[oracle]** `complete -o nospace`).
6. Otherwise build one compspec from all options and insert it under the
   special key or each name. Status 0.

Every other flag combination falls into one of these: e.g. `complete -r
-W x foo` removes foo; `complete -p -W x` prints all.

### 2.4 Print format (CD:508-599)

One line per compspec, to stdout:

`complete ` then, each followed by one space:

1. `-o NAME` for each set option, in option-table order (§1.1);
2. `-X` for each set action that has a letter, in action-table order;
   so letters come out as `-a -b -c -d -e -f -g -j -k -s -u -v`;
3. `-A NAME` for each set action without a letter, in table order:
   `arrayvar binding disabled enabled function helptopic hostname
   running setopt shopt signal stopped`;
4. `-G 'ARG'`, `-W 'ARG'`, `-P 'ARG'`, `-S 'ARG'`, `-X 'ARG'`, `-C 'ARG'`
   in that order, each present one always single-quoted;
5. `-F NAME`, quoted only if it contains shell metacharacters;

then the name, then newline. The name prints as `-D`, `-E`, `-I` for the
special keys, `''` if empty, single-quoted if it contains shell
metacharacters, else bare (CD:550-571). An entry with nothing set prints
`complete -D` etc.

**Single quoting** (SQ:95-130): wrap in `'...'`, each embedded `'`
becomes `'\''`; the string consisting of exactly one `'` becomes `\'`
(no surrounding quotes); empty becomes `''`.

**Shell metacharacters** (SQ:376-407): any of space tab newline `'` `"`
`\` `|` `&` `;` `(` `)` `<` `>` `!` `{` `}` `*` `[` `?` `]` `^` `$`
`` ` ``; `~` only at the start or after `=` or `:`; `#` only at the
start.

**[oracle]** examples:

```
complete -o filenames -o nospace -a -g -v -A hostname -A signal -G '*.c' -W 'a b' -P 'pre' -S 'suf' -X '!*' -C 'cmd x' -F _f foo
complete -W 'it'\''s' -E
complete -o bashdefault -o default -o dirnames -o fullquote -o noquote -o nosort -o plusdirs -b -c -d -e -f -j -k -s -u -A arrayvar -A binding -A disabled -A enabled -A function -A helptopic -A running -A setopt -A shopt -A stopped x
complete -W 'a' 'x$y'      complete -W 'a' '~a'      complete -W 'a' a~
complete -W 'a' '#a'       complete -W 'a' a#        complete -W 'a' \'
complete -F 'f$x' y
```

(`-F 'f$x'` is accepted outside posix mode because `$` is not in the
break set.)

### 2.5 Exit status

0 on success; 1 if any name in a print or remove request has no compspec
(the rest are still processed) or stdout write fails on a named print
(CD:630-650, CM:320-333); 2 for every usage error above.

### 2.6 Messages

- `complete: NAME: no completion specification` (print or remove of a
  missing name; CD:501, 644). For a special key the raw key shows:
  **[oracle]** `complete -r -I` twice gives `complete: _InitialWorD_: no
  completion specification`.
- Non-interactive shells put `line N: ` after `bash: ` (CM:83-95), e.g.
  `bash: line 7: complete: nope: no completion specification`.

---------------------------------------------------------------------------

## 3. `compgen` (CD:652-783)

Usage (CD:655):
`compgen: usage: compgen [-V varname] [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [word]`

### 3.1 Parsing

Same option string and per-option handling as `complete` (§2.1), except:

- `-p -r -D -E -I` are errors: `compgen: -D: invalid option` + usage,
  status 2 (**[oracle]**).
- `-V NAME` is accepted; NAME must be a valid identifier, else
  `` compgen: `1x': not a valid identifier ``, status 2 (**[oracle]**).
- No arguments: status 0, no output. Options absent but words present
  (`compgen foo`, `compgen -- foo`): status 0, no output (**[oracle]**
  for `--`), CD:682-696.
- The word is the first non-option argument, or empty; further
  arguments are ignored (**[oracle]** `compgen -W 'aa ab' a b c` prints
  `aa ab`). Use `--` to pass a word starting with `-`.
- `-F` prints `compgen: warning: -F option may not work as you expect`
  and `-C` prints `compgen: warning: -C option may not work as you
  expect` to stderr before generating (both if both), CD:702-705.

### 3.2 Generation

Build a temporary compspec from the options and run the pipeline (§5)
with command name `compgen`, an empty line, point 0 (CD:707-736). Then
(CD:738-755):

- If the result is empty and `-o bashdefault` was given: run bash's
  default completion (C§4.3) on the word with no command position (so
  only the `$var`, `~user`, `@host` and glob branches can match).
  **[oracle]** `compgen -o bashdefault '$HOM'` prints `$HOME`;
  `'~roo'` prints `~root`.
- If still empty and `-o default`: readline's filename generator on the
  word, **not dequoted**, in directory order (**[oracle]** `-o default
  f` printed `f2 f1`; `-o default 'a\ '` found nothing although `a b`
  exists).
- `filenames noquote nospace nosort fullquote` have no effect on
  compgen output (readline's variables are saved and restored around the
  call, CD:723-759). `dirnames` and `plusdirs` work as in §5.8.

Output is the list in generation order, **not sorted and not
deduplicated** (**[oracle]** `compgen -d -o plusdirs d` prints `d1 d2 d1
d2`), one per line, no quoting, no prefix (SL:214-222). Embedded
newlines are printed raw.

### 3.3 `-V varname` (5.3)

The named variable is found or created as an indexed array and **emptied
first**, even when nothing matches (CM:988-1013); then the matches are
stored at indices 0.. and nothing is printed. An existing associative
array gives `compgen: NAME: not an indexed array`; readonly gives the
usual readonly error; status 1 in both. **[oracle]** `arr=(old); compgen
-V arr -W 'b1 b2 c' b` gives status 0 and `arr=(b1 b2)`; then `compgen
-V arr -W q z` gives status 1 and `${#arr[@]}` = 0.

### 3.4 Status

0 if at least one match was printed or stored, 1 if none (**[oracle]**
`compgen -W 'x y z' q` → 1, `compgen -W ''` → 1), 2 for usage errors.

### 3.5 `-F` and `-C` in compgen

They run as in §6 but with the empty line: **[oracle]** `compgen -F f
wd` calls `f compgen wd ''` with `COMP_LINE=""`, `COMP_POINT=0`,
`COMP_TYPE` and `COMP_KEY` = whatever the last real completion left
(0 if none), `COMP_WORDS` empty (an existing `COMP_WORDS` is **not**
cleared, only overwritten from index 0, VA/arrayfunc.c:527-549, so
inside a completion function the caller's words remain visible) and
`COMP_CWORD=-1`. Afterwards all six `COMP_*` variables are **unset**,
which clobbers them for a completion function that called `compgen -F`.
A missing function prints `bash: completion: function `nof' not found`
(internal error format, no `compgen:` prefix) and the other sources'
matches still count (§5.6). `compopt` with no names inside such a
function fails (not completing, §4).

---------------------------------------------------------------------------

## 4. `compopt` (CD:785-918)

Usage (CD:788): `compopt: usage: compopt [-o|+o option] [-DEI] [name ...]`

Getopt string `+o:DEI` with `+` options allowed (GO:52-54): `-o NAME`
turns an option on, `+o NAME` off (accumulated in two sets; a name in
both ends up off since "off" is applied second, CD:884-889, 910-911).
`+D`, `+E`, `+I` behave like `-D` etc. Unknown option name: `compopt:
NAME: invalid option name`, status 2, no usage. Unknown letter
(`-p`): `compopt: -p: invalid option` + usage, status 2 (**[oracle]**).

Targets: `-D`/`-E`/`-I` select one special key (D > E > I) and names are
then ignored; else the names; else the current completion.

**No target** (CD:868-892):
- If readline is not in the middle of a completion, or no compspec is
  running: `compopt: not currently executing completion function`,
  status 1 (**[oracle]**). This includes `compgen -F` from the command
  line.
- With no `-o`/`+o`: print the running copy's options in full form
  (below), with the name being the key that matched (`foo`, or `-D`,
  `-E`, `-I`). **[oracle]** inside a `-I` function: `compopt +o
  bashdefault +o default +o dirnames +o filenames +o fullquote +o
  noquote +o nosort +o nospace +o plusdirs -I`.
- Otherwise set/clear the bits on the **running copy only** (the stored
  compspec is untouched, so it lasts for this one completion) and
  immediately apply the readline-variable effects (§5.9) for the
  changed options. The final copy's options also go into the "found"
  value (§1.3), so e.g. `compopt -o default` from the function enables
  the filename fallback for this attempt.

**Named targets** (CD:894-912): for each, a missing compspec prints
`compopt: NAME: no completion specification` (raw key for -D/-E/-I,
**[oracle]** `compopt: _DefaultCmD_: no completion specification`) and
sets status 1. With no `-o`/`+o`: print `compopt ` + all nine options as
`-o NAME ` or `+o NAME ` in table order + name + newline (CD:601-610;
**[oracle]** `compopt +o bashdefault +o default +o dirnames -o filenames
+o fullquote +o noquote +o nosort -o nospace +o plusdirs foo`).
Otherwise edit the stored compspec (shared, §1.1). Editing the stored
compspec of the command being completed, from inside its function, does
not affect the current attempt (it runs on a copy).

Status: 0, or 1 for a missing compspec / the "not currently executing"
case, 2 for usage errors. Printing the current completion checks for
write errors; the named-print path does not.

---------------------------------------------------------------------------

## 5. The generation pipeline (PC:1250-1454)

`gen_compspec_completions(cs, cmd, word, start, end)`. `word` is the
word to complete exactly as readline handed it (C§3: an opening quote
is excluded, backslashes and any other quoting are kept); for compgen it
is the argument. Sources are tried in this fixed order and **appended**
into one list; nothing is sorted or deduplicated here:

### 5.1 Actions (PC:786-841)

In this fixed order regardless of how they were given: alias, arrayvar,
binding, builtin, disabled, enabled, export, function, helptopic,
hostname, job, keyword, running, setopt, shopt, signal, stopped,
variable; then command, file, user, group, service; then directory.
**[oracle]** `compgen -a -A function -W zw -G 'x*' z` prints alias
`zz`, function `zfun`, glob `x.c`, word `zw`.

The first group (list actions) filters a cached or regenerated list by
prefix: the word is first **dequoted** (unquoted `'`, `"` and `\`
removed, bash's filename dequoter, BL:4210-4250, 4958-4965), then each
item that begins with it, compared **byte-wise and case-sensitively**
(no completion-ignore-case, no nocasematch), is kept in list order; an
empty word keeps everything (PC:656-688). **[oracle]** `compgen -W "ab
'c d'" "'c"` prints `c d`; `compgen -A shopt '"no'` lists the `no...`
options.

The second group (command, file, user, group, service) runs readline's
match driver over a generator (`rl_completion_matches`), so they behave
like interactive completion of that kind (PC:772-782, 816-820):
case-insensitive when completion-ignore-case is on, and **sorted** in
that case (the lowest-common-prefix code sorts the list in place only
under case folding, RL:1395-1490); otherwise in generator order. The
driver's prefix entry (slot 0) is dropped (PC:314-340).

Directory (PC:822-838): filename generation with the word dequoted
(only when bash is actually dispatching a completion and readline found
no quote, BL:4930-4953), then non-directories removed (each name
tilde-expanded and stat-hooked, then tested), and the readline flag
"mark symlinked directories" forced on for this attempt
(`rl_completion_mark_symlink_dirs`, so a symlink to a directory gets `/`
like a directory, RU "When a compspec indicates..."). If filename
completion was not wanted before and this produced nothing, the
"results are filenames" flag that filename generation switched on is
switched back off.

Note: the file and directory generators turn on readline's "results are
filenames" flag as a side effect (RL filename generator), so `-f`/`-d`
matches are quoted, get `/` on directories and are listed by basename
even without `-o filenames`.

### 5.2 `-G globpat` (PC:848-858, 1279-1296, PX:472-489)

Pathname expansion of the pattern **as written** (no quote removal: a
backslash is a glob escape), not using the word at all. Honours dotglob,
globstar, extglob, nocaseglob and GLOBSORT ordering; ignores GLOBIGNORE
and nullglob/failglob (no match = no entries, never the literal pattern).
Results are not filtered by the word. Whenever `-G` is set, readline's
"results are filenames" flag is turned on, even if nothing matched
(PC:1282-1294 test a list that always exists). FIGNORE then applies via
the ignore hook (§8.4). **[oracle]** `compgen -G '*'` omits `.hid`;
`compgen -G 'nomatch*'` prints nothing, status 1.

### 5.3 `-W wordlist` (PC:862-903)

1. Split the string into words at IFS characters (unset IFS = space tab
   newline), honouring quotes, backslashes, `$(...)`, `${...}` and
   backquotes so they are not split (SU:2614-2771 with the IFS rules:
   leading IFS whitespace skipped, a non-whitespace IFS char plus
   adjacent IFS whitespace is one delimiter). Quotes are kept for step 2.
   **[oracle]** `IFS=:` splits `a:b c` into `a`, `b c`; `IFS=` keeps
   `a b` whole.
2. Expand each word with brace, tilde, parameter, command substitution
   and arithmetic expansion, word splitting of the results, and quote
   removal; **no pathname expansion** (`expand_words_shellexp`).
   **[oracle]** `-W '$x "a b" {1,2} ~ $(echo c d) *.c'` gives `val`,
   `a b`, `1`, `2`, `/root`, `c`, `d`, `*.c`.
3. Keep words that begin with the dequoted word (byte prefix,
   case-sensitive), in order.

An expansion error aborts like any expansion error at top level: in a
non-interactive shell the shell exits (**[oracle]** `compgen -W
'${zz?boom} b'` prints `bash: line N: zz: boom` and the script ends);
interactively the completion attempt is abandoned and the line is
redisplayed (source comment PC:883).

### 5.4 Splitting the line for `-F`/`-C` (PC:1316-1352, 1237-1246)

Only when `-F` or `-C` is set: take the current command's text from
`start` to `end` (§8.2: from the command name to the end of the simple
command, which can extend past point) and split it into words with
`split_at_delims` using the current COMP_WORDBREAKS set
(`rl_completer_word_break_characters`), with "quote characters are not
delimiters" and "completion mode" flags (SU:2614-2771, 2197-2340):

- Leading whitespace that is also a break character is skipped. A line
  of only such whitespace gives no words and leaves the counts
  undefined (source bug); bash never passes one in practice.
- A word ends at an unquoted break character. Backslash escapes the
  next character; `'...'`, `"..."`, `$'...'`, `` `...` ``, `$(...)`,
  `${...}`, `<(...)`, `>(...)` are skipped whole. `'` and `"` in the
  break set never delimit.
- A run of consecutive non-whitespace break characters (from the break
  set, other than quotes) is one word of its own: `=`, `>`, `>>`, `&&`,
  `:`, `;`, `|` ... (and `@` when hostcomplete is on).
- Whitespace break characters between words are dropped.
- Current word (`cw`, 1-based internally): the word whose span
  [start, end] contains point, the end inclusive (point just after a
  word belongs to it). If point is in whitespace before a word, an empty
  word is inserted there and is current. If point is past the last word
  and the character before point is whitespace, an empty word is
  appended and is current; if point is right after a word, that word is
  current.

**[oracle]** `foo a=b 'c d' x\ y $(e f) >g ` TAB: `COMP_WORDS=(foo a = b
"'c d'" 'x\ y' '$(e f)' '>' g '')`, `COMP_CWORD=9`. `bar --opt=` TAB:
`(bar --opt =)`, `COMP_CWORD=2`, `$2` empty, `$3=--opt`. `foo ab  cd`
with point between the two blanks: `(foo ab '' cd)`, `COMP_CWORD=2`.
`foo abcd` with point after `ab`: `COMP_CWORD=1`, `COMP_WORDS[1]=abcd`,
`$2=ab`. `foo "ab c` TAB: `COMP_WORDS[1]="ab c` (with the quote),
`$2=ab c` (without). Setting `COMP_WORDBREAKS=' '` makes `foo a=b:c` one
word (**[oracle]**).

Words keep their quoting (they are raw text), unlike `$2` which is
readline's word.

If the command name passed in is empty and the first split word is not,
an empty word is prepended (PC:1331-1338) so word 0 is always the
command slot. (Occurs when the "command name" is empty, e.g. a line
starting with a redirection.)

### 5.5 `-F function` (PC:1045-1144)

1. Look up the function. Missing: print `bash: completion: function
   `NAME' not found` (internal error; non-interactive adds `line N:`),
   ring the bell, redraw the prompt line, contribute nothing, and still
   count as found (so the other sources' matches are used and no `-D`
   fallback happens).
2. Bind the variables (§6.1), with `COMP_CWORD` = cw - 1.
3. Call the function with `$1` = command name, `$2` = word, `$3` =
   previous word (§6.2). The parser state is saved and restored around
   the call, including `$?` and PIPESTATUS (parse.y save_parser_state:
   **[oracle]** `false; compgen -F s >/dev/null; echo $?` prints 0 for
   the compgen, and interactive `$?` survives TAB). Readline's signal
   handlers are removed during the call when completing (not for
   compgen) and reinstalled after; an interrupt unwinds through the same
   cleanup (variables unset, parser state restored).
4. Status: 127 means "not found" (§1.3); 124 means "retry" (§5.6).
5. Unset the `COMP_*` variables (§6.1).
6. Read `COMPREPLY` (normal dynamic lookup; the function's locals are
   gone by now, so a `local COMPREPLY` is lost). A scalar is converted
   to a one-element array (**[oracle]** `COMPREPLY=one` gives `one`).
   An associative array gives nothing (**[oracle]**). Elements in index
   order, skipping holes (**[oracle]** `([5]=five [2]=two)` gives `two
   five`). Nothing is used if the status was 124 or 127. Not filtered
   by the word.
7. `COMPREPLY` is then **unset** whenever it existed (**[oracle]**
   unset after compgen -F), PC:1140.

### 5.6 Status 124 and the `-D` loader

If the function returns 124 the whole run yields nothing (even matches
from actions, -G, -W) and the lookup loop (§1.3) starts again from the
same command name. If the function defined a new compspec for that name
(any `complete ... NAME` creates a new object) it is found and run;
otherwise the same object is skipped, then the basename and `-D` are
tried, and if nothing new is found the result is "no compspec" and bash
default completion runs. The documented loader idiom (RU, after the
"dynamically modifying completions" paragraph):

    _completion_loader() { . "/etc/bash_completion.d/$1.sh" >/dev/null 2>&1 && return 124; }
    complete -D -F _completion_loader -o bashdefault -o default

**[oracle]** `-D` loader that runs `complete -F _show -o nospace "$1";
return 124`: `zap q` TAB calls the loader with `$1=zap`, then `_show`
for zap (whose `compopt` shows `-o nospace`); the next `zap q` TAB calls
`_show` directly. On an empty line the loader is called with
`$1=_EmptycmD_`, installs `complete ... _EmptycmD_` (that is, a `-E`
spec), and `_show`'s `compopt` prints the name `-E`.

Status 127 (source, not verified): the run reports "not found", its
matches are discarded, and (only if the compspec has no `-o` options,
§1.3) lookup continues with the basename and `-D`; if nothing else is
found, bash default completion runs as if no compspec existed. A
function whose last command is an unknown command returns 127 by
accident.

### 5.7 `-C command` (PC:1155-1235)

1. Bind the variables with "export" on: `COMP_LINE`, `COMP_POINT`,
   `COMP_TYPE`, `COMP_KEY` get the export attribute; `COMP_WORDS` and
   `COMP_CWORD` are not set (PC:930-977).
2. Build the shell text: the `-C` string verbatim, a space, then `$1
   $2 $3` each single-quoted (§2.4 quoting) and space-separated. So the
   string is shell code: `-C 'echo'` runs `echo 'cmd' 'word' 'prev'`
   (**[oracle]** `compgen -C echo w` prints `compgen w ` with the
   trailing blank from the empty third argument). A `#` in the string
   comments the arguments out.
3. Run it as a command substitution (subshell; trailing newlines
   stripped; exit status ignored; stderr not captured).
4. Unset the four variables.
5. Split the output at newlines into items; a backslash immediately
   before a newline keeps that newline (and the backslash) inside the
   item; runs of newlines produce no empty items; no other processing
   (no IFS, no dequoting, blanks kept). **[oracle]** output `a\<nl>b<nl>c`
   gives two items, `a\<nl>b` and `c`; `<nl><nl>  l3  ` gives `  l3  `.
   Empty output gives nothing.

### 5.8 Filters and additions (PC:1400-1453)

After all sources, in order:

1. If a function asked for retry or reported not found: discard
   everything, return nothing.
2. **`-X filterpat`** (PC:243-309, SB:180-235): if the pattern contains
   an unescaped `&`, replace each unescaped `&` with the word (the raw
   word, not dequoted); if the word contains glob characters or a
   backslash they are backslash-escaped so the word matches literally;
   `\&` becomes a literal `&` (other backslashes stay, as glob escapes).
   Then a leading `!` means "negate" unless extglob is on and the next
   character is `(` (an extglob `!(...)` pattern). Each item is matched
   against the (rest of the) pattern as a whole string with the shell
   pattern matcher: extglob forms when extglob is on, case-folded when
   **nocasematch** is on; `*` matches `/` and leading dots. Without
   negation matching items are removed; with negation non-matching ones
   are. Order is preserved. **[oracle]** `-W 'foo bar baz' -X 'b*'` →
   `foo`; `-X '!b*'` → `bar baz`; `-W 'foo fob bar' -X '!&*' fo` → `foo
   fob`; `-W 'a& b' -X '*\&'` → `b`; `-X '!'` keeps only empty items;
   extglob on `-X '!(ab)'` on `ab ac ad` → `ab`, off → nothing;
   nocasematch on, `-X 'f*'` removes `Foo` and `foo`.
3. **`-P`/`-S`**: prepend/append to every remaining item (SL:182-211).
   **[oracle]** `-W 'a b' -P '<' -S '>' -X a` → `<b>`.
4. **`-o dirnames`**: if the list is now empty, replace it with the
   directory action's results for the word (§5.1) — no `-X`, `-P`, `-S`
   applied (**[oracle]** `-W zz -o dirnames -P pre q` → `qd`).
   **`-o plusdirs`** (only when dirnames did not fire): append the
   directory action's results, likewise unfiltered and unprefixed
   (**[oracle]** `-W dx -o plusdirs -P p d` → `pdx d1 d2`).

`default` and `bashdefault` are not handled here: they act in bashline
(§8.3) or compgen (§3.2) when the final list is empty.

### 5.9 Options as readline settings (PC:1456-1476)

When a compspec was found, bash applies its options (from the copy,
after any compopt) to readline before handing over the list (BL:1801-
1803); `compopt` inside a function applies changes immediately both
ways. Only "on" bits are pushed by the final application:

| option | readline effect (C§6) |
|--------|----------------------|
| filenames | "results are filenames": FIGNORE filter, `/` on directories, quoting, listing by basename, symlink/dir marking (C§6.3, 6.4, 7.2) |
| fullquote | quote every match as a filename would be even when results are not filenames (`rl_full_quoting_desired`). Distinct from bash's internal `complete_fullquote` shopt, which concerns `$` in filenames |
| noquote | turn off filename quoting (`rl_filename_quoting_desired` = 0) |
| nospace | suppress the appended space (`rl_completion_suppress_append`); a directory still gets `/` when results are filenames (RL:1924-1967) |
| nosort | no sorting before dedup and listing (`rl_sort_completion_matches` = 0); then dedup removes only adjacent duplicates (RL:1314-1340) and the list shows in generation order |
| default, bashdefault, dirnames, plusdirs | no readline variable; see §5.8, §8.3 |

Each attempt starts with sorting on, quoting on, full quoting off,
append not suppressed and "results are filenames" off (C§0 step 1,
BL:1598-1605).

---------------------------------------------------------------------------

## 6. The function and command interface

### 6.1 Variables (PC:930-992)

Bound with ordinary assignment in the current scope (so a `local` of the
same name in a calling function is the one set):

| variable | value |
|----------|-------|
| COMP_LINE | the current simple command's text from the command name to the command's end (§8.2), not the whole input line. Assignments before the command name and earlier commands on the line are **not** included (**[oracle]** `x; FOO=1 foo ba` → `foo ba`; `foo a \| foo b` with point after `a` → `foo a `). Can extend past point |
| COMP_POINT | point relative to the start of COMP_LINE, in **characters** (multibyte-aware count of the bytes before point; changed from bytes after 4.2). **[oracle]** `foo éé ` → 7 |
| COMP_TYPE | readline's completion type as a decimal integer: 9 (TAB, normal), 63 `?` (list after the second TAB / possible-completions), 33 `!` (show-all-if-ambiguous), 64 `@` (show-all-if-unmodified), 37 `%` (menu completion), and 42 `*` for insert-completions (RL:567-582; not in the docs). **[oracle]** TAB → 9, TAB TAB → 63 |
| COMP_KEY | the key that invoked the command (the last key of a sequence), decimal; 9 for TAB |
| COMP_WORDS | (functions only) indexed array of the split words (§5.4); a nameref attribute on an existing COMP_WORDS is removed; made visible |
| COMP_CWORD | (functions only) index of the current word, cw - 1; -1 when there are no words (empty line, or empty command after `;`, **[oracle]**) |

`-C` commands get the first four exported and never get the arrays.

Afterwards all six are **unset** in the current scope, whatever their
previous values (PC:979-992). Inside the function they are ordinary
variables the function may change; changes have no effect.

`COMP_WORDBREAKS` (VA:1587-1607, BR COMP_WORDBREAKS) is a dynamic
variable reading and writing readline's word break set. Default ` \t\n"'@><=;|&(:`,
without `@` when hostcomplete is off (BL:313-314). Unsetting it removes
the special behaviour permanently.

### 6.2 Arguments (PC:1006-1031)

`$1` = the command name as passed to the lookup (§1.3 step 4: the full
command word, an alias's first word after progcomp_alias, or
`_EmptycmD_` / `_InitialWorD_` for -E/-I runs). `$2` = the word being
completed, as readline isolated it (opening quote removed, backslashes
kept; empty at a word boundary). `$3` = the word before the current one
in COMP_WORDS (raw text), or empty if there are no words. Quirk: when
the current word is word 0, `$3` is word 0 itself (**[oracle]** `-I`
on `fo` → `$3=fo`; point at column 0 of `foo x` → `$2=''`, `$3=foo`).
`$0` stays the shell's; FUNCNAME[0] is the function.

### 6.3 Errors, traps, set -e

Not special-cased (source): the function runs like any other function
call in the interactive shell; its error messages go to the terminal
in the middle of the line (and bash does not redraw unless the function
was missing, §5.5). `set -e` and traps apply as usual (not tested). An
interrupt aborts the completion attempt.

---------------------------------------------------------------------------

## 7. Action generators

"Prefix" below means §5.1's dequoted byte-prefix filter. "Order" is the
order compgen prints.

| action | list and order | notes |
|--------|----------------|-------|
| alias | all aliases, sorted (first byte compared as signed char, then strcmp; alias.c:244-274) | cached, invalidated on alias/unalias |
| arrayvar | visible indexed and associative array variables, sorted like variables | `declare -A x` without a value is invisible and omitted (**[oracle]**) |
| binding | readline's bindable command names, sorted (`rl_funmap_names`, strcoll) | `abort accept-line arrow-key-prefix ...` (**[oracle]**) |
| builtin | every builtin that has an implementation, **including disabled ones**, in the builtin table's order, which is sorted by name in byte order | **[oracle]** `. : [ alias bg bind break builtin caller cd command compgen complete compopt continue declare dirs disown echo enable eval exec exit export false fc fg getopts hash help history jobs kill let local logout mapfile popd printf pushd pwd read readarray readonly return set shift shopt source suspend test times trap true type typeset ulimit umask unalias unset wait`; `compgen -b ec` still gives `echo` after `enable -n echo` |
| command | readline driver over bash's command-word generator (C§4.8): aliases, keywords, functions, enabled builtins, the word itself if a directory, PATH executables (or files/dirs for a word with `/`), in that order, duplicates kept, unsorted unless ignore-case | |
| directory | §5.1 | names without trailing `/` (**[oracle]** `compgen -d` → `d1 d2`) |
| disabled | builtins with an implementation and disabled | |
| enabled | builtins with an implementation and enabled | |
| export | visible exported variables, sorted | |
| file | readline filename generator over the word (C§5), directory order, names without `/` | dequoting: in compgen typed at the prompt the word is dequoted only when bash's readline hooks are initialised, i.e. interactive shells (PC:692-757; **[oracle]** non-interactive `compgen -f 'a\ b'` finds nothing). Two bash-completion workarounds: inside a completion, `''` or `""` as the whole word means empty; inside a completion function, a compgen word that differs from the original word and contains quotes is dequoted |
| function | visible functions, sorted | |
| group | `getgrent` order, prefix, case-sensitive; **nothing on Windows** (BL:2723-2756) | |
| helptopic | every builtin table entry including the help-only reserved-word entries, byte order | **[oracle]** `! % (( ... )) . : [ [[ ... ]] alias ... for for (( function ... variables wait while { ... }` |
| hostname | the host list (C§4.7: `$HOSTFILE`, else `hostname_completion_file`, else `/etc/hosts`; read once, re-read after HOSTFILE changes), file order, duplicates kept, bare names (no `@`) | |
| job | every job in the table, **newest first** (slots scanned from the highest down), each as the first blank-delimited word of the job's command text (PC:529-567) | duplicates kept (**[oracle]** two `sleep` jobs → `sleep sleep`); not dependent on `set -m` |
| keyword | `if then else elif fi case esac for select while until do done in function time { } ! [[ ]] coproc` (**[oracle]**) | |
| running | jobs whose state is running, as for job | |
| service | `getservent` order; a name matches by its primary name or, failing that, by an alias, and the alias is returned in that case; **nothing on Windows** (BL:2667-2717) | |
| setopt | `set -o` names in table order: `allexport braceexpand emacs errexit errtrace functrace hashall histexpand history ignoreeof interactive-comments keyword monitor noclobber noexec noglob nolog notify nounset onecmd physical pipefail posix privileged verbose vi xtrace` (**[oracle]**) | |
| shopt | shopt names in table order (alphabetical in 5.3; **[oracle]** 57 names `array_expand_once ... xpg_echo`, our list must match ours) | |
| signal | the signal-name table: index 0 `EXIT`, then by number (`SIGHUP`...), gaps as `SIGJUNK(n)`, real-time as `SIGRTMIN`, `SIGRTMIN+1`...`SIGRTMAX-1`, `SIGRTMAX`, then `DEBUG ERR RETURN` (support/signames.c) | Linux **[oracle]**: `EXIT SIGHUP SIGINT SIGQUIT SIGILL SIGTRAP SIGABRT SIGBUS SIGFPE SIGKILL SIGUSR1 SIGSEGV SIGUSR2 SIGPIPE SIGALRM SIGTERM SIGSTKFLT SIGCHLD SIGCONT SIGSTOP SIGTSTP SIGTTIN SIGTTOU SIGURG SIGXCPU SIGXFSZ SIGVTALRM SIGPROF SIGWINCH SIGIO SIGPWR SIGSYS SIGJUNK(32) SIGJUNK(33) SIGRTMIN SIGRTMIN+1 ... SIGRTMIN+15 SIGRTMAX-14 ... SIGRTMAX-1 SIGRTMAX DEBUG ERR RETURN`. Use whatever `kill -l`/`trap -l` uses in our shell |
| stopped | jobs whose state is stopped | |
| user | `getpwent` order, prefix (a leading `~` on the word is kept on results and makes them filenames, RL:2377-2425); **nothing on Windows** | |
| variable | all visible shell variables (set ones, any scope, unique names; declared-but-unset excluded) sorted by first byte then strcmp (VA:4250-4320) | **[oracle]** `declare unsetv; compgen -v unsetv` → status 1. Includes dynamic variables and, during completion, the COMP_* ones |

Sorting of the variable-backed lists compares the first byte as a signed
char, so names starting with bytes >= 0x80 sort first; otherwise plain
byte order.

Windows decisions: users, groups and services are compile-time absent
in native Windows bash (`_WIN32`/`__WIN32__`); simplest is to return
nothing, matching. An optional extension could enumerate local accounts
(NetUserEnum), local groups and `%SystemRoot%\System32\drivers\etc\services`
/ `hosts` (the latter is a natural default for the hostname list when
HOSTFILE is unset). Signals: emit our own signal table in the order our
`kill -l` uses. Jobs: depends on our job table.

---------------------------------------------------------------------------

## 8. Integration with readline (BL:1587-1825)

### 8.1 When programmable completion is tried

Inside `attempt_shell_completion`, after the classifier has computed
command position (C§4.1) and handled backquote words (C§4.4), the
programmable block runs if all hold (BL:1680-1685):

- no matches yet (a backquote command substitution word did not
  already produce some);
- not in command position, **or** the word is empty, **or** in command
  position and a `-I` compspec exists;
- the prompt being read is PS1 (never on PS2 continuation lines).

The block itself always runs those steps (it also refines command
position, C§4.2). Compspecs are consulted only if the `progcomp` shopt
is on (default on) and the table is non-empty ("have_progcomps").

### 8.2 Finding the command and choosing the key (BL:1463-1540, 1686-1793)

1. `s` = start of the current simple command: scan from the line start
   for unquoted `;|&{(` and backquote (skipping quoted strings, `$(...)`
   and `${...}` as units) up to the word start; `>|` is not a
   separator; a `{` counts only where a reserved word may appear
   (preceded by a separator or first in the command, and followed by a
   shell break character). `e` = the next such separator at or after
   the word end, or the line end.
2. Skip assignments: repeatedly take the next word (leading blanks
   skipped, ends at the first of `()<>;&|` space tab newline) while it
   is an assignment; stop early if past the line end, or (after an
   assignment) past point; if the scan passes `e`, reset to the word
   being completed. `s1`/`e1` = the bounds of the command name `n`; `s`
   is set to `s1`.
3. Choose (first matching case; `start`/`end` = the word's bounds, text
   = the word):
   1. word empty at column 0 of a non-empty line whose command name ends
      before the line end → none;
   2. empty word exactly at the command name's start, name follows →
      none;
   3. empty word at column 0, command name starts later (leading
      blanks) and follows → **-E** (key `_EmptycmD_`);
   4. command ends at 0 and starts at 0 (empty line, or a line starting
      with a separator), empty word → **-E**;
   5. empty word on a blank before the command name → none;
   6. a real command name, no assignment pending, name ends at the word
      end, the command ends at the line end and the last character is
      not blank (i.e. completing the command name itself at end of
      line) → none; command position := (name starts at word start and
      name equals word);
   7. a real command name, no assignment pending → **the command's
      compspec** with `cmd = n`, `start = s`, `end = e`; then command
      position := name starts at word start and (a `-I` spec exists or
      name equals word); if that is true and `-I` exists, the result is
      dropped in favour of `-I`;
   8. no command name, empty word, after a separator character →
      none; command position := 1;
   9. no command name, empty word, not at column 0 → none; command
      position += "an assignment was pending";
   10. name equals word, spans the whole command, not at column 0 →
       none; command position := 1;
   11. otherwise none.
4. If command position is true, compspecs exist, nothing was found and
   a `-I` spec exists: run with key `_InitialWorD_` and `cmd =
   _InitialWorD_`, `start = min(s, point)`, `end = min(e, line end)`.

So: -E only for an empty line or column 0 before leading blanks; a
line of blanks with point at the end is **not** -E (**[oracle]**: no
call); -I for the command word, an empty word after `;` (**[oracle]**
`ls; ` → `-I` with `COMP_LINE=""`, `COMP_CWORD=-1`), point at column 0
before a command (**[oracle]** `foo x` with point at 0 → `-I`, `$2=''`,
`$3=foo`, `COMP_LINE="foo x"`, `COMP_POINT=0`), and on an empty line
when neither -E nor -D matched. `X=1 ` TAB (empty word after an
assignment) does **not** call -I (**[oracle]**). An `-E` or command
lookup falls back to `-D` (§1.3), so with only `-D` defined the `-D`
function also handles the empty line (with `$1=_EmptycmD_`).

Quirk: a word inside `$(...)` in an argument is not its own command for
this purpose (`$(...)` is skipped as a unit when finding `s`):
**[oracle]** `echo $(fo` TAB runs echo's (here `-D`) compspec with
`$2=fo`, `COMP_WORDS=(echo '$(fo')`.

### 8.3 Handing the result to readline (BL:1795-1825, 1543-1554)

`found` = 0 or (found bit | final options) (§1.3).

- found: apply the options to readline (§5.9), then build readline's
  match array from the list (readline's driver computes the common
  prefix in slot 0; a single item becomes slot 0 itself; an empty list
  gives no array). Unless the options include `default`, set
  "attempted completion over" so readline will **not** fall back to
  filename completion. If there are matches, or `bashdefault` is not
  set, return them (possibly none → bell).
- found with no matches and `bashdefault`: fall through to bash's
  default completion (C§4.3) with the classifier's command position.
  If that also yields nothing and `default` is set, readline's filename
  completion runs (attempted-over is clear); otherwise nothing.
- not found: bash default completion as without compspecs.

Readline then post-processes as usual (C§6.1): sort unless nosort,
deduplicate, FIGNORE/ignore hook when results are filenames, common
prefix, insert (C§6.2: the common prefix **replaces** the word, so
`COMPREPLY` items that do not start with the word can delete typed
text), quote per §5.9, append character, listing on TAB TAB. The list
items are not copied: bash hands readline pointers into its array
(freed at the next attempt).

### 8.4 Ignore hooks

The FIGNORE ignore hook is installed for every attempt (BL:1598) and
readline calls it only when results are filenames, so FIGNORE filters
`-f`, `-d`, `-G` and `-o filenames` results. The directory action
additionally removes non-directories itself (§5.1). The `default`
fallback uses whatever ignore hook the default classifier left
(C§4.3).

---------------------------------------------------------------------------

## 9. Scenarios worth testing

### 9.1 Builtin specs (no terminal; run as a script, compare stdout+stderr)

1. `complete -p` hash order: define `foo`, `'b ar'`, `''`, `-D`, `-E`,
   `-I` (the §1.2 set) and compare the exact listing; `a b c d e` →
   `e d c b a`; redefine `c` keeps place; `complete -r c; complete -W x
   c` moves it.
2. Every option/action combination printed (§2.4 examples), name
   quoting (`x$y ~a a~ #a a# '` and empty), `-W "it's"`, `-F 'f$x'`.
3. `complete foo` (print), `complete -o nospace` (usage only, 2),
   `complete -A bogus x`, `-o bogus`, `-z`, `-A` without argument, `-F
   'a b'`, `-V v`, `complete -r nope`, `complete -p nope`, `complete -r
   -I` on a missing -I (`_InitialWorD_` in the message), `complete -p
   -D -E` (only -D), `complete -D -W x foo` then `complete -p foo` (1).
4. Sharing: `complete -W x a b; compopt -o nospace a; complete -p b`.
5. compopt: print named (full `+o` form), `-o`/`+o` mix, `-p` invalid,
   no name outside completion (`not currently executing completion
   function`, 1), `compopt -D` with no -D spec.
6. compgen statuses: no args 0; `compgen -- foo` 0; no match 1; `-W ''`
   1; `-D` 2; bad `-V` name 2.
7. compgen each action: `-k`, `-A signal`, `-b` (and after `enable -n`),
   `-A disabled/enabled`, `-A helptopic`, `-A setopt`, `-A shopt`, `-v`
   with a declared-unset variable, `-e`, `-A arrayvar`, `-a` order,
   `-A function`, `-j`/`-A running` with background jobs.
8. Pipeline order (`-a -A function -W -G` together); `-X` cases of §5.8
   including `&`, `\&`, `!`, `!(...)` with and without extglob,
   nocasematch; `-P/-S` with `-X`; dirnames/plusdirs with `-P`;
   duplicates kept.
9. `-W` splitting and expansion: IFS variants, quotes, `$(...)`, braces,
   tilde, no globbing; `${x?msg}` error.
10. `-F` from compgen: argument/variable dump, COMPREPLY scalar, assoc,
    sparse, status 124 and 127 (status 1), missing function message,
    variables unset afterwards, `$?` preserved, warnings on stderr.
11. `-C`: backslash-newline joining, blank lines dropped, blanks kept,
    `echo` argument quoting.
12. `-V`: fills, empties on no match, assoc target error.
13. `compgen -G` with dotglob on/off, GLOBIGNORE ignored; `-f` and `-d`
    output (sort it: directory order is not portable).
14. bashdefault (`$HOM`, `~roo`) and default (raw word, no dequoting).

### 9.2 Interactive TAB (t/edit, pty)

Use a logging `-F` function (as in the oracle runs: print `$1 $2 $3`,
COMP_LINE, COMP_POINT, COMP_TYPE, COMP_KEY, COMP_CWORD and each
COMP_WORDS element to a file or to the screen followed by a redraw).

1. Word splitting: `foo a=b 'c d' x\ y $(e f) >g ` TAB; `bar --opt=`
   TAB; `foo ab  cd` with point between blanks; point inside a word;
   open double quote; `COMP_WORDBREAKS=' '`.
2. COMP_LINE scoping: `x; FOO=1 foo ba`, `foo a | foo b` with point in
   the first command; COMP_POINT with multibyte characters.
3. TAB vs TAB TAB (`COMP_TYPE` 9 then 63), menu-complete (37),
   show-all-if-ambiguous (33), M-* (42).
4. -E / -I / -D selection: empty line; blank line (no call); `ls; `;
   point at column 0; `fo` (`$3` quirk); `X=1 ` (no -I); `echo $(fo`
   (runs echo's spec).
5. Basename lookup (`/usr/bin/foo x` → `$1` full path); progcomp_alias
   on/off, and blocked by -D.
6. 124 loader: install-and-retry, loop warning after 33 calls.
7. Options at insertion: `-o nospace`, `-o filenames` (dir gets `/`,
   quoting of `a b`), `-o noquote`, `-o fullquote` on non-filenames,
   `-o nosort` (listing order, adjacent-only dedup), `compopt -o
   nospace` from inside the function affecting only that attempt.
8. Fallbacks: no matches with/without `-o default` (filename fallback vs
   bell), `-o bashdefault` with `$HO`, `-o dirnames`, `-o plusdirs`.
9. COMPREPLY items not sharing the typed prefix (the word is replaced
   by their common prefix, possibly shorter than what was typed).
10. FIGNORE with `-G`, `-f`, `-o filenames`.
11. Missing `-F` function: message mid-line, bell, prompt redrawn.

---------------------------------------------------------------------------

## 10. Things not determined

- Status 127 fallback (§5.6) and its interaction with `-o` options were
  read from source only; the pty test was inconclusive.
- `set -e`, ERR/RETURN/DEBUG traps and `return` from a trap inside a
  completion function: untested.
- An interrupt (C-c) during a slow `-F`/`-C`: source says the attempt is
  abandoned with cleanup; screen not recorded.
- `-o fullquote` and `-o noquote` insertion screens, and `nosort`
  listing order, are described from readline source (C§6.3) and not yet
  recorded against the oracle.
- The `-I` case table (§8.2) beyond the recorded cases (`X=1 ` in
  particular contradicts the reading of case 9 that "command position
  += assignment pending" would give; the assignment flag is cleared by
  the final empty-name pass of the skip loop).
- Hash order for non-ASCII names (signed char XOR) is from source.
- Exact readline redisplay after a function writes to the terminal.
