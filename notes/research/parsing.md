bash 5.3 lexer and parser: a spec for the unabashed M1 parser

This note specifies the lexing and parsing behaviour that a hand-written recursive-descent parser needs to match bash 5.3 exactly: tokens, word scanning, reserved words, aliases, here-documents, command substitution, `[[ ]]`, `(( ))`, and every syntax-error message with its line number, echoed text and exit status.

Sources:
- `parse.y`, `syntax.h`, `parser.h`, `make_cmd.c`, `error.c`, `eval.c`, `shell.c` and `builtins/evalstring.c` from the bash-5.3 tree in the scratchpad (paths below are relative to it). Line numbers are for that tree.
- `doc/bashref.texi`: Reserved Words at line 728, Aliases at 8531, Command Substitution at 2818.
- Oracle probes against bash 5.3.0(1)-release in the `unabashed/bash53` image. Every output quoted below was produced by the oracle. The probe harness is described in section 0.

Everything here is a paraphrase. No bash code, tests or documentation text is copied. The only verbatim strings are error-message formats, which are observable output that we must reproduce.

======================================================================
0. PROBE HARNESS AND CONVENTIONS
======================================================================

Probes were plain files run inside one container per batch. `run.sh` iterated over them with one of four modes:
- **n** (the `tools/record` mode): `bash --norc --noprofile -c "$script" bash`, with stdin from /dev/null and `2>&1`.
- **c**: the same, but without the `$0` argument.
- **f**: script mode, `bash ./s.sh`.
- **s**: `bash -s < file`.

Unless a mode is named, outputs below are from mode n, and "[status N]" is the shell's exit status.

Prefix of diagnostics (error.c:300 `parser_error`, plus the runtime `error_prolog`):

| How bash was started | Parser errors | Runtime errors and warnings |
|---|---|---|
| `-c STR` or `-c STR bash` | `bash: -c: line N: ` | `bash: line N: ` |
| `-c STR foo` | `foo: -c: line N: ` | `foo: line N: ` |
| script `./s.sh` | `./s.sh: line N: ` | `./s.sh: line N: ` |
| stdin (`-s`, or `bash < f`) | `bash: line N: ` | `bash: line N: ` |
| inside `eval` | `bash: eval: line N: ` | |
| inside `. /tmp/x` | `/tmp/x: line N: ` | |
| backquote or comsub reparsed at run time | `bash: command substitution: line N: ` | |

Rules behind the table:
- `parser_error` prints `NAME: INPUT: line N: ` when the shell name (`$0`) and the input name differ, and `NAME: line N: ` when they are equal. When the shell is interactive it prints just `NAME: `.
- In `-c` mode the input name is `-c`. That is why parse errors read `bash: -c: line 1:` even though `tools/README.md` says that diagnostics read `bash: line N:`. That README statement holds only for runtime errors such as "command not found" and for warnings such as the here-document EOF warning (probe t50 in section 9).
- The heredoc EOF warning is an `internal_warning`, so it uses the runtime prefix: `bash: line 2: warning: ...`.

```
$ bash -c 'if then' foo
foo: -c: line 1: syntax error near unexpected token `then'
foo: -c: line 1: `if then'
$ bash -c 'nosuch' foo
foo: line 1: nosuch: command not found
```

======================================================================
1. INPUT LAYER (shell_getc, parse.y:2475)
======================================================================

The lexer never sees the raw stream. It reads from a **line buffer** (`shell_input_line`) that `shell_getc` refills one physical line at a time.

Reading a line (parse.y:2549-2637):
- Bytes are read until `\n` or EOF. NUL bytes are dropped. On `st_string` input (`-c`, eval), a NUL ends the string.
- `\r` is dropped only in `__MSYS__` builds (line 2596). Our plan drops `\r` always (plan section "Parser input").
- `line_number` is incremented each time a new line is fetched, including the fetch that hits EOF. Consequence: an EOF error in a file of K lines that ends in a newline is reported at line K+1 (probe e5 in section 12).
- After a line is read, `\n` is appended to the buffer, unless the line ended at EOF.
- Special case at EOF (lines 2739-2746). Suppose the final line of string, buffered or non-interactive stream input ends with an unescaped backslash and no newline. Then the appended character is a second `\` instead of `\n`. The backslash survives to the word: `-c 'echo end\'` prints `end\`, and `-c 'echo end \'` prints `end \`.

Handing out characters (lines 2759-2905):
- `shell_getc(remove_quoted_newline)` returns the next byte.
- When its argument is true and the byte is a backslash followed by `\n`, both are skipped, the next line is read and reading continues. This is **line continuation at the input layer**, and it is invisible to the lexer.
  - The lexer passes false while inside single quotes, and for the character after a backslash (`LEX_PASSNEXT` or `pass_next_character`). So `\<newline>` survives in `'...'` and is removed everywhere else, inside double quotes included.
- When the buffer is exhausted:
  - If an alias expansion string is on the input stack, it is popped and reading resumes in the saved line (section 10).
  - Otherwise, at EOF, `shell_getc` returns `\n` if the partial last line was non-empty and EOF if it was empty. A final line without a trailing newline therefore still produces a newline token.
- `shell_ungetc` writes the byte back into the buffer at `index-1`. This matters for the alias quirk in section 12.6.
- Comments are discarded with `discard_until('\n')`, which calls `shell_getc(0)`. A backslash at the end of a comment does not continue the comment: `echo a # x \` followed by `echo b` prints `a` then `b`.

Unit of execution:
- The grammar's start symbol is `inputunit` (parse.y:435). It is one `simple_list` ended by a newline or EOF.
- A newline ends the unit only where the grammar is at "top level": after a complete list, or after `;` or `&`. Newlines after `&&`, `||`, `|` and `|&`, or inside any compound command, are absorbed (`newline_list`), and more lines are read.
- `reader_loop` (eval.c) and `parse_and_execute` (`-c`, eval, source; builtins/evalstring.c) alternate: parse one inputunit, execute it, repeat.

Verified consequences:
```
# -c 'echo a; if then'          (one line: nothing runs)
bash: -c: line 1: syntax error near unexpected token `then'
bash: -c: line 1: `echo a; if then'
[status 2]
# -c $'echo a\nif then'         (two lines: line 1 runs first)
a
bash: -c: line 2: syntax error near unexpected token `then'
bash: -c: line 2: `if then'
[status 2]
```
The same holds in modes f and s. A syntax error at top level aborts the whole `-c` string or script, with status 2. Section 12.8 has the exceptions.

======================================================================
2. CHARACTER CLASSES (syntax.h)
======================================================================

- **Metacharacters** (`shellmeta`, `CSHMETA`): `( ) < > ; & |`.
- **Word breakers** (`shellbreak`): the metacharacters plus space, tab and newline.
- **Blanks** (`shellblank`, `CBLANK`): space and tab. These are skipped between tokens.
- **Quote characters** (`shellquote`): `"`, `'` and backquote.
- **Expansion starters** (`shellexp`): `$`, `<` and `>`. `<` and `>` count only because of process substitution `<(` and `>(`.
- **Extglob prefixes** (`PATTERN_CHAR`): `@ * + ? !`. They matter only when `extended_glob` is on at lex time and the next character is `(`.
- **Backslash-escapable inside double quotes** (`CBSDQUOTE`, from `slashify_in_quotes`): `\` backquote `$` `"` newline. A backslash before any other character is kept literally.
- **Backslash-escapable in an unquoted here-document body** (`CBSHDOC`): `\` backquote `$`, plus newline handled by `read_a_line`.
- **`FUNSUB_CHAR`** (parser.h:83). These are the characters that may follow `${` to make a function substitution `${ cmd; }` instead of a parameter expansion: space, tab, newline, `|` and `(`.
- **Internal escapes**: the parser stores literal bytes 0x01 (CTLESC) and 0x7f (CTLNUL) in a word with a CTLESC in front. This shows in error messages (section 12.4).

======================================================================
3. TOKEN SET
======================================================================

`other_token_alist` (parse.y:2390) and `word_token_alist` (parse.y:2355) give the **text used in error messages**.

| Token | Spelling | Error-message name |
|---|---|---|
| `IF THEN ELSE ELIF FI CASE ESAC FOR SELECT WHILE UNTIL DO DONE IN FUNCTION TIME COPROC` | the word | the word |
| `'{'` `'}'` | `{` `}` | `{` `}` |
| `BANG` | `!` | `!` |
| `COND_START` `COND_END` | `[[` `]]` | `[[` `]]` |
| `AND_AND OR_OR` | `&&` `\|\|` | same |
| `GREATER_GREATER LESS_LESS LESS_LESS_MINUS LESS_LESS_LESS` | `>>` `<<` `<<-` `<<<` | same |
| `LESS_AND GREATER_AND LESS_GREATER GREATER_BAR` | `<&` `>&` `<>` `>\|` | same |
| `AND_GREATER AND_GREATER_GREATER BAR_AND` | `&>` `&>>` `\|&` | same |
| `SEMI_SEMI SEMI_AND SEMI_SEMI_AND` | `;;` `;&` `;;&` | same |
| `TIMEOPT TIMEIGN` | `-p` `--` after `time` | `-p` `--` |
| single characters | `< > ; ( ) \| & -` | the character |
| `'\n'` | newline | `newline` |
| `yacc_EOF` | end of input | `EOF` (appears only in `[[` messages) |
| `WORD ASSIGNMENT_WORD` | | the word's stored text (after `$'..'` translation) |
| `NUMBER` | digits right before `<` or `>`, or right after `<&` or `>&` | decimal value |
| `REDIR_WORD` | `{name}` right before `<` or `>` | not reachable in practice |
| `ARITH_CMD` | `(( expr ))` | the expression text, e.g. `2` |
| `ARITH_FOR_EXPRS` | `for (( a;b;c ))` | the parts joined with ` ; ` |
| `COND_CMD` | the parsed `[[ ]]` body | none, falls back to the text scan of 12.3 |
| `DOLPAREN DOLBRACE` | internal, start a recursive comsub parse | |

Observed error names:
```
`;;'  `|'  `)'  `fi'  `then'  `newline'  `;'  `&'  `||'  `}'  `;&'
`[['  `('  `>'  `echo' (WORD)  `a=b' (ASSIGNMENT_WORD)  `2' (ARITH_CMD from "((1)) ((2))")
```

======================================================================
4. read_token (parse.y:3557)
======================================================================

The lexer works in this order. A recursive-descent port should keep the same order, because each step changes what later steps see.

1. If `token_to_read` is set, return it. It is used for pushback by the grammar actions for `time`/`!` with nothing after them (1422-1468), and after `[[`.
2. If `PST_CONDCMD` is set and `PST_CONDEXPR` is not, run the `[[` sub-parser (section 7). It returns `COND_CMD` and queues `COND_END`.
3. Skip blanks with `shell_getc(1)`.
4. On EOF, set `EOF_Reached` and return `yacc_EOF`.
5. On `#`, a comment starts (non-interactive shells, or `interactive_comments`). Discard through end of line and treat it as `\n`. If EOF comes first, return `yacc_EOF`.
6. On `\n`:
   - collect pending here-documents (section 9);
   - clear `PST_ALEXPNEXT`, `PST_ASSIGNOK` and `PST_CMDBLTIN`;
   - return `'\n'`.
7. In `PST_REGEXP` mode (the right side of `=~`), go straight to word reading.
8. On a metacharacter, clear `PST_ALEXPNEXT`, `PST_ASSIGNOK` and `PST_CMDBLTIN`, then peek one character:
   - Same character doubled:
     - `<<`: peek once more. `-` gives `<<-`, `<` gives `<<<`, anything else is pushed back and gives `<<`.
     - `>>`, `&&`, `||`.
     - `;;`: sets `PST_CASEPAT`, then peeks for `&`, which gives `;;&`.
     - `((`: call `parse_dparen` (section 8). If it declines (returns -2), fall through and return a single `(`.
   - `<&`, `>&`, `<>`, `>|`, `|&`.
   - `&>`, then peek: `>` gives `&>>`.
   - `;&`: sets `PST_CASEPAT`.
   - Otherwise push the peeked character back.
   - Side effects for single `(` and `)`:
     - A `)` read when the last token was `(` and the one before was a WORD marks a function definition. It sets `PST_ALLOWOPNBRC` and records the definition line.
     - A `(` outside a case pattern list and outside `[[`, whose previous token was not a WORD, pushes `(` on the compound stack used for EOF messages (section 12.2).
   - `<` or `>` followed by `(` is **not** an operator. It falls through to word reading as process substitution.
   - Otherwise return the single character.
9. A `-` right after `<&` or `>&` is returned as the token `'-'`.
10. Otherwise read a word with `read_token_word` (section 5). If alias expansion pushed new input, restart at step 3.

Notes:
- The peeks use `shell_getc(1)`, so `\<newline>` can sit inside an operator. Verified: `echo a &\`+newline+`& echo b` runs as `&&` and prints `a b`; `cat <\`+newline+`<-A` is `<<-A`; `echo c |\`+newline+`| echo d` is `||`.
- In `$(...)`, when the metacharacter is the closing `)`, the peek uses `shell_getc(0)` (line 3683).

======================================================================
5. read_token_word (parse.y:5305)
======================================================================

The word scanner accumulates bytes until an unquoted break character. Flags kept per word:
- `all_digit_token`: for NUMBER detection;
- `dollar_present`: a word with `$` is never a reserved word;
- `quoted`: set by any quote character or backslash. It blocks reserved words and aliases and sets `W_QUOTED`;
- `compound_assignment`.

5.1 Per-character rules, in order:

1. **Backslash** (when `PST_NOEXPAND` is clear):
   - `\<newline>` is removed. In practice `shell_getc` already removed it, except where it was called with 0.
   - Otherwise the backslash is kept in the word and marks it quoted.
   - The next character is taken literally if we are not inside double quotes (also inside backquotes), or if it is in the `CBSDQUOTE` set.
   - Word text keeps the backslash. Quote removal happens at expansion.
2. **Function substitution close**: in a `${ ...; }` funsub, a `}` at the very start of an unquoted word, in reserved-word position, is returned alone as the `}` token, even when more characters follow (line 5407). `${ echo b;}x` prints `bx`.
3. **Quote characters** `'`, `"` and backquote: call `parse_matched_pair(c, c, c)` (section 6). Append the opening character and the returned text, which includes the closer. Single and double quotes set `quoted`. Backquotes do not, so a backquoted word can still be a reserved word or an alias in principle.
4. **`PST_REGEXP`** (right side of `=~`): `|` is an ordinary character, and `(` starts a balanced `( ... )` group read verbatim. So `[[ a =~ ^(a|b)$ ]]` is one word.
5. **Extglob**: if `extended_glob` is on **at lex time** and the character is one of `@ * + ? !` followed by `(`, a balanced `( ... )` is read verbatim (section 6, open `(`) and appended. See 5.4.
6. **`$` `<` `>` expansions**, after a one-character peek:
   - `$(`, `<(`, `>(` are read by `parse_comsub` (section 8.1).
     - Process substitution `<(...)` and `>(...)` can sit **inside** a word: `echo a<(echo b)` printed `a/dev/fd/63`.
   - `${`: if the following character is a `FUNSUB_CHAR`, call `parse_comsub(..., '{', '}')` (funsub). Otherwise call `parse_matched_pair(..., '{', '}', P_FIRSTCLOSE|P_DOLBRACE)`.
   - `$[` (old arithmetic): `parse_matched_pair` with `P_ARITH`.
   - All of these set `dollar_present`.
   - `$'...'`: read with backslash escapes allowed, then **translated at lex time** (`ansiexpand`) and re-quoted as a single-quoted string (`sh_single_quote`). The word text no longer contains `$'`. Example: `$'a\tb'` becomes `'a<TAB>b'`. It sets `quoted`.
   - `$"..."`: locale translation at lex time. The result is re-wrapped in double quotes, or in single quotes when `singlequote_translations` is on and the text changed. It sets `quoted`.
   - `$$` is consumed as a unit.
7. **Array subscript `[`**. It starts a subscript read by `parse_matched_pair(..., '[', ']', P_ARRAYSUB)` when either:
   - the word so far is a valid identifier and we are in assignment position (`assignment_acceptable(last_read_token)`, i.e. command position and not in a case pattern); or
   - the word is empty and we are inside a compound assignment `( ... )`.
   - Inside the subscript, blanks and metacharacters do not break the word: `a[1 + 2]=x` and `a[ 3 ]=y` are single assignment words. Outside assignment position they split: `echo a[1 + 2]=x` prints `a[1 + 2]=x` as 3 arguments.
   - `declare b[1 + 2]=z` also splits, into `b[1`, `+`, `2]=z`, because after `declare` we are no longer in command position:
     ```
     bash: line 5: declare: `b[1': not a valid identifier
     bash: line 5: declare: `+': not a valid identifier
     bash: line 5: declare: `2]=z': not a valid identifier
     ```
8. **Compound assignment `=(`**. Conditions:
   - the character is `=`;
   - the word so far forms an assignment LHS (`token_is_assignment`: valid identifier, optional `[sub]`, optional `+`);
   - we are in assignment position or `PST_ASSIGNOK` is set (after a declaration builtin);
   - the next character is `(`.
   - Then `parse_compound_assignment` (parse.y:7104) **re-lexes the contents as a stream of WORD tokens**:
     - newlines are allowed;
     - comments are dropped;
     - any other token is a syntax error.
   - The word text becomes `NAME=(` + the words joined with single spaces + `)`, and the word gets `W_COMPASSIGN`. Example: `local z=(1   2 "3 4")` is stored and printed as `local z=(1 2 "3 4")`, and `c=( )` as `c=()`.
   - Error behaviour is unusual (section 12.8): an error in a compound assignment discards the rest of the line, sets status 1 and continues with the next line.
9. **Break**: an unquoted `shellbreak` character is pushed back and ends the word.
10. Anything else is appended. CTLESC and CTLNUL get a CTLESC in front.

5.2 After the word ends (`got_token`, line 5722):

- **NUMBER**:
  - all characters are digits;
  - and either the terminating character is `<` or `>`, or the previous token was `<&` or `>&`;
  - and the value fits in an `int`.
  - Otherwise it is a WORD: `echo 99999999999>f` writes `99999999999` into f.
- **Special-case tokens** (`special_case_tokens`, line 3366). Each is checked in order and returns before reserved-word lookup (section 6.3).
- **Alias expansion** (section 10). If the word was expanded, the token is re-read.
- **Reserved word check** (`CHECK_FOR_RESERVED_WORD`, line 3168). In POSIX mode this check happens before alias expansion. In normal mode it happens after, so an alias **can** shadow a reserved word, and an alias can expand **to** a reserved word (probe a05: `alias i=if t=then` works).
- **Assignment** (`assignment()` in general.c):
  - A word is an assignment if it has an unquoted `=` after a valid identifier, optionally with `[subscript]` and/or `+`.
  - It is an `ASSIGNMENT_WORD` only if it is also in assignment position (`W_ASSIGNMENT|W_NOSPLIT`), or inside a compound assignment.
  - Otherwise it is a plain WORD that only carries `W_ASSIGNMENT`. `echo x=1` prints `x=1`.
  - Assignment position: a command word position, after another ASSIGNMENT_WORD, or after a redirection when the simple command so far has only redirections (`PST_REDIRLIST`). Never in a case pattern.
- **Declaration builtins**: in command position, a builtin with the `ASSIGNMENT_BUILTIN` flag, or `eval`, or `let`, sets `PST_ASSIGNOK`. That allows `=(` in its arguments. `command` does the same in POSIX mode only. The builtins with the flag are alias, declare, export, local, readonly and typeset; check `builtins/*.def` when implementing.
- **REDIR_WORD**: a word that starts with `{` and ends with `}`, followed immediately by `<` or `>`, whose inside is a valid identifier or array reference. It becomes the `{var}` redirection source. Otherwise it stays a WORD: `echo {1}>f7` writes `{1}`, and `echo {v b}>f6` writes `{v b}`.
- **State updates**:
  - after `function`, the next word allows `{` (`PST_ALLOWOPNBRC`);
  - after `for` or `case`, set `expecting_in_command`;
  - after `for`, `case` or `select`, increment `expecting_in_token`.

5.3 Word-level verified examples:
```
echo a&&echo b||echo c;echo d|cat     -> a b d   (operators split words)
echo a#b ; echo #x ; x=1#2; echo $x   -> a#b, (empty line), 1#2
echo \#notcomment                     -> #notcomment
echo $'\'' $"dq" "a"'b'c\ d           -> ' dq abc d
echo ${x:-}} ${x-{a}}                 -> } {a}      (P_FIRSTCLOSE: the first } closes)
echo ${x:-$(echo a)} ${x:-`echo b`} "${x:-"c d"}" ${x:-'e f'} "${x:-'g'}"
                                      -> a b c d e f 'g'
```

5.4 Extglob must be on before the line is read:
```
# -c 'shopt -s extglob; echo @(a|b)'
bash: -c: line 1: syntax error near unexpected token `('
bash: -c: line 1: `shopt -s extglob; echo @(a|b)'
# the same with the shopt on its own line works, and so does
# case ab in @(a|b)b) ... once the shopt ran on an earlier line.
```
Points:
- The right-hand side of `=`, `==` and `!=` in `[[ ]]` always lexes with extglob on (parse.y:5202-5211): `[[ ab == @(a|b)b ]]` works with extglob off.
- In bash 5.3 (compatibility level 53), a command substitution does **not** force extglob on. That was done only up to compat 51 (parse_comsub 4538). So `echo $(echo @(a))` with extglob off is an error:
  ```
  bash: -c: line 1: syntax error near unexpected token `(' while looking for matching `)'
  bash: -c: line 1: `echo $(echo @(a))'
  [status 127]
  ```
- With extglob on, `!(false)` at the start of a command is a single **word**, not `!` plus a subshell. It glob-expanded to a file name in the probe: `bash: line 3: g01.sh: command not found`. With extglob off, it is a negated subshell.
- A function body lexed with extglob on keeps its pattern text even if extglob is turned off later. At run time the match then fails, but the function still prints correctly.

======================================================================
6. parse_matched_pair (parse.y:3877) AND SPECIAL WORDS
======================================================================

6.1 `parse_matched_pair(qc, open, close, flags)` reads up to the matching close and returns the text **including** the closer.

Counting rules:
- `count` starts at 1 and drops at each `close`.
- It rises at each nested `open` unless `P_FIRSTCLOSE` is set. There is one exception: under `P_FIRSTCLOSE`, a `{` right after `$` still counts.
- If `open == close` (the quote cases), there is no nesting.

Character rules:
- **Backslash**: the next character is passed through (`LEX_PASSNEXT`). Inside double-quote-like contexts (`qc != '\''`), a backslash-newline pair is deleted. In single quotes everything is literal, except that `$'...'` allows `\'` (`P_ALLOWESC`).
- **Comments**: tracked only for `P_COMMAND` contexts that are not inside any quote. In practice that means backquotes, since `$(` goes through `parse_comsub`. A `#` at the start or after a blank/newline begins a comment, and text up to the newline is not scanned for the closer.

Nested constructs, when `open != close`:
- a quote character starts a nested matched quote. `$'` inside a group is translated in place in some contexts (lines 4053-4094);
- `$(`, `${` and `$[` recurse (`parse_comsub` / `parse_matched_pair`);
- inside `${...}` and `[...]` subscripts, `<(` and `>(` recurse too.

Inside a double-quoted string (`open == '"'`):
- a backquote recurses so that an inner `"` does not end the string: `echo "`echo "inner dq"`"` works;
- `$(`, `${` and `$[` recurse, so `"$(echo ")")"` prints `)`.

Dolbrace state machine (`P_DOLBRACE`, lines 4007-4028): it tracks param, op and word so that, in POSIX mode only, single quotes inside a double-quoted `${...}` are handled per interp 221. Outside POSIX mode it does not change scanning.

EOF error: `unexpected EOF while looking for matching `C'`, where C is the **closing** character. The line number is **the line where the construct started** (`start_lineno`). After the error it sets `PST_NOERROR`, so no second message appears.
```
# -c $'echo "a\nb\n'         -> bash: -c: line 1: unexpected EOF while looking for matching `"'
# -c $'echo a\necho "abc\n'  -> a / bash: -c: line 2: unexpected EOF while looking for matching `"'
# -c $'echo ${a\nb\n'        -> bash: -c: line 1: unexpected EOF while looking for matching `}'
# -c $'echo `\n'             -> bash: -c: line 1: unexpected EOF while looking for matching ``'
# -c $"echo a\necho 'x\n"    -> bash: -c: line 2: unexpected EOF while looking for matching `''
```
All of these exit with status 2, and no offending line is printed.

6.2 Backquotes:
- The text is read with `parse_matched_pair(..., '`', '`', P_COMMAND)`. It is **not parsed** at parse time and is stored verbatim.
- Escaped backquotes nest: `` echo `echo \`echo nested\`` `` prints `nested`.
- Errors inside backquotes appear only at execution, with the "command substitution" prefix. The outer command then continues with empty output and the outer status is unaffected:
```
# -c $'echo `if`\necho after $?'
bash: command substitution: line 2: syntax error: unexpected end of file from `if' command on line 1
(empty line)
after 0
```
- `declare -f` prints backquotes exactly as written: `` echo `echo   a` `` keeps its three spaces.

6.3 Special-case tokens (`special_case_tokens`, parse.y:3366), in order:
1. `in` is IN when:
   - the last token was WORD and the one before was FOR, CASE or SELECT (POSIX rule 6). After `case`, IN also enters pattern-list mode (`PST_CASEPAT`, `esacs_needed_count++`); or
   - `expecting_in_token` is set and the last token was WORD or newline. This is what allows `case a<newline>in` and `for x<newline>in a b`.
2. `do` is DO when:
   - `expecting_in_token` is set and the last token was newline or `;` (`for x; do`, `for x<newline>do`); or
   - the last token was WORD and the one before was FOR or SELECT (`for x do`).
3. `esac` is ESAC right after IN (`case x in esac`).
4. `{` is a brace when `PST_ALLOWOPNBRC` is set, i.e. right after `function NAME` or after `NAME()`.
5. After `for ((...))` (`ARITH_FOR_EXPRS`), `do` is DO and `{` is a brace, with no separator needed.
6. `}` is a brace when `open_brace_count > 0` and the reserved-word position test passes.
7. `-p` after TIME is TIMEOPT. `--` after TIME or TIMEOPT is TIMEIGN.
8. `]]` is COND_END while inside a conditional expression (`PST_CONDEXPR`).

======================================================================
6A. RESERVED WORDS: EXACTLY WHERE THEY ARE RECOGNIZED
======================================================================

The table (parse.y:2355): `if then else elif fi case esac for select while until do done in function time { } ! [[ ]] coproc`.

A word is checked against the table only if it has no `$` and no quoting, and the **previous token** is one of these (`reserved_word_acceptable`, parse.y:5899):

    newline ; ( ) | & { } && || |& ;; ;& ;;& ! time -p -- if then elif else fi
    do done while until esac coproc ]] ((arith)) start-of-input $( ${

It is also checked when the previous token was a WORD whose predecessor was `function` or `coproc`. That allows `coproc NAME {` and the body after `function NAME`.

Further filters inside `CHECK_FOR_RESERVED_WORD`:
- Inside a case pattern list (`PST_CASEPAT`), only `esac` is recognized. Even `esac` is not recognized right after `|` or `(`, so `case esac in (esac) ...` and `case esac in x|esac) ...` both match the word `esac`.
- `time` is recognized only when `time_command_acceptable()` (parse.y:3286) holds:
  - the previous token is start-of-input, `;` or newline (and the token before that is not `|`), or one of `&& || & while do until if then elif else { ( ) ! time -p -- $( ${`;
  - after `|`, `time` is an ordinary word: `echo a | time echo b` gives `bash: line 1: time: command not found` (127);
  - POSIX mode with compat > 41 also refuses `time` when the next non-blank character is `-`.
- Recognizing `case` sets `PST_CASESTMT`. `esac` clears pattern mode. `[[` sets `PST_CONDCMD` and `]]` clears it. `{` and `}` adjust `open_brace_count`.
- `if while until for select case { ( [[` push onto the compound stack (`set_compoundcmd_top`), which is used for EOF messages.

Consequences (all probed):
```
echo if then else fi { } ! [[ ]] in do done esac time coproc function   -> printed verbatim
{ echo a; }; { echo b<newline>}      -> a, b
echo }                               -> }
{echo a; }                           -> error near unexpected token `}'   ({echo is a word)
{ echo a }                           -> EOF: unexpected end of file from `{' command on line 1
{ echo a; } }                        -> error near unexpected token `}'
if echo then; then echo fi; fi       -> then, fi
for in in in; do echo $in; done      -> in
case in in in) echo in-case;; esac   -> in-case
function if { echo func-if; }        -> defines a function named if
if() { :; }                          -> error near unexpected token `)'  (if is IF, then a subshell)
true | ! false                       -> error near unexpected token `!'
! ! true; echo $?                    -> 0 ;  ! true -> 1
time ; ! ; echo ok                   -> both are valid null commands; time prints its report
```

Notes on `!` and `time` (grammar 1408-1469):
- `!` and `time [-p] [--]` prefix a `pipeline_command`, and they stack (`! ! x`, `time time x`, `time ! x`).
- Alone before `;`, `&` or newline, they form a null command: `!` alone gives status 1, and `time` alone prints a zero report. The grammar action then **pushes the terminator back** as the next token.

`[[` and `]]`: `[[` is COND_START only in reserved position, and `]]` outside a conditional is an ordinary word. Inside a conditional, `]]` is recognized by the special case.

`in`, `do` and `esac` in odd places: `for x y` gives error near unexpected token `y`. `case x y in` gives error near `y`.

======================================================================
7. CONDITIONAL COMMAND `[[ ... ]]` (parse.y:5013-5260, read_token 3583)
======================================================================

After the COND_START token, the next `read_token` call runs a small recursive-descent parser:

    cond_expr := cond_or
    cond_or   := cond_and [ '||' cond_or ]        (right recursive)
    cond_and  := cond_term [ '&&' cond_and ]
    cond_term := skip-newlines then one of:
        ']]'                    -> error (COND_ERROR, no message here)
        '(' cond_expr ')'       -> must see ')', else error; then skip newlines
        '!' (BANG or word "!")  -> cond_term, then toggle negation
        UNOP WORD               -> UNOP is a word "-X" (2 chars) that test_unop accepts
        WORD BINOP WORD         -> BINOP is a word that test_binop accepts, or =~, or token '<' / '>'
        WORD followed by ']]' '&&' '||' ')'  -> implicit "-n WORD"; the token is kept
        anything else           -> "unexpected token ... in conditional command"

Lexing details:
- The sub-parser calls `read_token` directly. So **`last_read_token` stays at COND_START for the whole expression**, and that has three effects:
  - no reserved words are recognized inside (`[[ { ]]` is `-n {`; `]]` comes only from the special case);
  - no aliases are expanded;
  - `((` is never an arithmetic command, because COND_START is not in the reserved-position set and `parse_dparen` declines. `[[ (( a )) ]]` is valid nested grouping.
- Newlines are skipped only **before a term** and **after a complete term** (`cond_skip_newlines`). A newline between an operand and its operator, or after a unary or binary operator, is an error.
  - `[[ a &&<newline>b ]]` works;
  - `[[ a<newline>&& b ]]` and `[[ x<newline>]]` fail.
- The operand after `=`, `==` or `!=` lexes with extglob on. After `=~` it lexes in `PST_REGEXP` mode (5.1 rule 4).
- `<` and `>` arrive as operator tokens and become string-comparison operators. `<(` is still process substitution.

Error messages. Each is printed with `parser_error` at the line shown. The cond parser returns -1, so `yylex` yields YYUNDEF (or YYEOF at EOF). `report_syntax_error` then cannot name a token and falls back to the **text scan** (`syntax error near `X'`, section 12.3) plus the offending line. Exit status is 2. Observed:
```
[[ ]]          -> syntax error near `]]' / `[[ ]]'          (COND_END as first term: no specific message)
[[ -f ]]       -> unexpected argument `]]' to conditional unary operator / syntax error near `]]' / `[[ -f ]]'
[[ -n<nl>      -> unexpected argument `newline' to conditional unary operator / syntax error near `-n' / `[[ -n'
[[ a b ]]      -> unexpected token `b', conditional binary operator expected / syntax error near `b' / `[[ a b ]]'
[[ a<nl>       -> unexpected token `newline', conditional binary operator expected / syntax error near `a' / `[[ a'
[[ ( a ]]      -> unexpected token `]]', expected `)' / syntax error near `]]' / `[[ ( a ]]'
[[ a && ]]     -> syntax error near `]]' / `[[ a && ]]'
[[ < ]]        -> unexpected token `<' in conditional command / syntax error near `<' / `[[ < ]]'
[[ ; ]]        -> unexpected token `;' in conditional command / ...
[[ | ]]        -> unexpected token `|' in conditional command / ...
[[ a == ;      -> unexpected argument `;' to conditional binary operator / syntax error near `;'
[[ a ==<nl>    -> unexpected argument `newline' to conditional binary operator / syntax error near `=='
[[ a -nt ]]    -> unexpected argument `]]' to conditional binary operator / syntax error near `]]'
[[ a ) ]]      -> syntax error in conditional expression: unexpected token `)' / syntax error near `)'
[[ ( a ) b ]]  -> syntax error in conditional expression: unexpected token `b' / syntax error near `b'
[[ x ]] ]]     -> syntax error near unexpected token `]]'   (]] after COND_END is reserved: a normal yacc error)
[[<nl>]]       -> line 2: syntax error near `]]' / line 2: `]]'
```

At EOF:
```
[[ a &&<EOF>   -> line 2: unexpected token `EOF' in conditional command
                  line 2: syntax error: unexpected end of file from `[[' command on line 1
[[ (<EOF>      -> line 2: unexpected token `EOF' in conditional command
                  line 1: expected `)'                       (uses the line of the "(")
                  line 2: syntax error: unexpected end of file from `[[' command on line 1
[[ a = b<EOF>  -> line 1: unexpected EOF while looking for `]]'   (cond_lineno = line of the [[)
                  line 2: syntax error: unexpected end of file from `[[' command on line 1
[[ a =~ x( ]]  -> line 1: unexpected EOF while looking for matching `)'
                  line 2: unexpected argument to conditional binary operator
```

Which line number each message uses:
- "unexpected token / argument" messages use the current `line_number`;
- "expected `)'" uses the line where the `(` was read;
- "syntax error in conditional expression" and "unexpected EOF while looking for `]]'" use `cond_lineno`, the line where the expression began.

Variants exist without the token (`conditional binary operator expected`, `expected `)'`, `syntax error in conditional expression`, `unexpected argument to conditional unary operator`, `unexpected argument to conditional binary operator`). They are used when no name can be derived, for example when the inner result was COND_ERROR or the token was a `$(...)`-less internal token.

`[[ x ]]` is stored as `[[ -n x ]]`, and `declare -f` prints it that way: `r() [[ 1 ]]` printed `[[ -n 1 ]]`.

======================================================================
8. `((`, `$((`, `$(`, `for ((`, AND THE PAREN AMBIGUITIES
======================================================================

8.1 `$( ... )`: `parse_comsub` (parse.y:4451).

If the character after `$(` is another `(`:
- the lexer assumes arithmetic (POSIX interp 217) and reads a balanced paren group verbatim (`P_ARITH`);
- whether it really is arithmetic is decided **at expansion time**: `$((echo a); echo b)` printed `a b`, and `$(( 1 + 2 ) )` ran a subshell (`bash: line 1: 1: command not found`);
- a space after `$(` forces a comsub: `$( (echo f) )`.

Otherwise the comsub is parsed by a **recursive `yyparse`**:
- The parser state is saved, and `shell_eof_token = ')'`. The state flags for case patterns, cond, regexp and extpat are cleared.
- **Aliases are off** during this parse unless in POSIX mode (`expand_aliases = posixly_correct`).
- extglob is not forced (compat 53).
- Here-documents inside are collected inside (section 9).
- The comsub body ends at the `)` token that the grammar accepts in `comsub: DOLPAREN compound_list ')'`. Case patterns with an unbalanced `)` therefore just work: `$(case a in a) echo pc;; esac)` gives `pc`.
- Inside the comsub, a newline separating commands is kept as a newline connector instead of `;` (grammar 1299-1305).
- **The word text is replaced by the canonical reprint** of the parsed command (`print_comsub`). A `(` at the start gets a space in front.

This is visible in `declare -f` and in `$BASH_COMMAND`:
```
f() { echo $(  echo   a;echo b  ) "$( echo  c )" $(echo d
echo e) $( (echo f) ) $(case x in x) echo g;; esac) $(echo h # cmt
) `echo   i` $(( 1 + 2 )) $(<file) ... ${ echo j; } ${| REPLY=k; } ${ echo l;} ${x:-$( echo m )} <( echo n ) "${x:-$(echo   o)}"; }
declare -f f   ->
    echo $(echo a; echo b) "$(echo c)" $(echo d
echo e) $( ( echo f )) $(case x in x)
        echo g
    ;;
esac) $(echo h) `echo   i` $(( 1 + 2 )) $(< file) $(cat <<EOF
hd
EOF
) ${ echo j; } ${|REPLY=k; } ${ echo l; } ${x:-$(echo m)} <(echo n) "${x:-$(echo o)}"
```
Notes on the reprint:
- Comments are gone.
- Inner indentation continues from the function's printer state.
- Backquotes and `$((` are verbatim.
- Procsub `<( echo n )` is reprinted too, because `parse_comsub` handles both.

Funsub `${ cmd; }` and `${| cmd; }` (and `${(`, where `(` is a `FUNSUB_CHAR`):
- The same recursive parse, ended by `}`.
- Reprinted as `${` + (`|` or space) + command + a terminator + ` }`.
- The terminator is `\n` if the source ended with a newline, `;` if it ended with `;` or a word, and nothing if the text already ends in `;`, `&` or newline.

8.2 Errors inside `$(`. The recursive parse reports through the normal `report_syntax_error`. Because `shell_eof_token` is set:
- if the offending token is not the closer, the message gets the suffix ` while looking for matching `)'` (or `` `}' `` for a funsub);
- if the token **is** the closer, there is no suffix.
- EOF inside a comsub is reported by the outer logic as `unexpected EOF while looking for matching `)'` at the **current** line, i.e. the EOF line. Contrast quotes, which use the start line.
```
echo $(fi)       -> syntax error near unexpected token `fi' while looking for matching `)'
                    `echo $(fi)'                                 [status 127 in -c, 2 in script/stdin]
echo $(if)       -> syntax error near unexpected token `)'      [127 in -c]
echo $(for)      -> syntax error near unexpected token `)'
echo ${ fi; }    -> syntax error near unexpected token `fi' while looking for matching `}'   [127]
x=$(<nl>echo a<nl>fi<nl>)  -> line 3: ... `fi' while looking for matching `)' / line 3: `fi'   [127]
echo $(<nl>      -> line 2: unexpected EOF while looking for matching `)'   [2]
echo $(if true; then<nl>   -> line 2: unexpected EOF while looking for matching `)'   [2]
echo $(echo ${a<nl>        -> line 1: unexpected EOF while looking for matching `}'   [2]
```

**Exit status 127.** A non-EOF syntax error inside a comsub calls `jump_to_top_level(FORCE_EOF)` (parse_comsub 4592-4596).
- In `-c` mode, `run_one_command` (shell.c:1471) maps FORCE_EOF to **127**.
- In script and stdin mode, `reader_loop` just stops, and the status stays at the 2 set by the error report.
- This also happens inside `eval`: in `-c` mode, `eval 'while :; do echo $(fi); done'` exits the **whole shell** with 127, and the next line never runs.

8.3 Arithmetic command `((`. In `read_token`, a `((` in reserved-word position calls `parse_dparen` (parse.y:4901). It reads a balanced group with `parse_matched_pair(0, '(', ')', P_ARITH)` starting after the first `(`. Then (`parse_arith_cmd`, 4963):
- If the very next character is `)`, it is an **arithmetic command**: `ARITH_CMD` with the inner text, flagged `W_QUOTED|W_NOSPLIT|W_NOGLOB|W_NOTILDE|W_NOPROCSUB`.
- Otherwise it is a **nested subshell**. The text `( ... )` is pushed back as input (`PSH_DPAREN`) and the first `(` is returned.

So `((echo a); echo b)` is a subshell containing a subshell, while `((echo a))` is arithmetic and fails at run time:
```
((echo a)); echo after $?
bash: line 1: ((: echo a: arithmetic syntax error in expression (error token is "a")
after 1
```
`( (echo c) )` is subshells. The decision is purely lexical: the group must balance and be followed directly by `)`.

If the previous token is not in reserved position, `((` is two `(` tokens. Example: `echo ((` is an error near `(`.

EOF inside `((`: `((1 +` gives `unexpected EOF while looking for matching `)'` at the start line, status 2. Arithmetic syntax errors are never parse-time errors: `echo $(( 1 + ))` prints `bash: line 1: 1 + : arithmetic syntax error: operand expected (error token is "+ ")` and execution continues.

8.4 `for ((;;))`. When the previous token is FOR, `parse_dparen` always takes the arithmetic path and returns `ARITH_FOR_EXPRS`. If no `))` follows, it returns -1, which gives YYUNDEF and the text-scan message:
```
for ((;;)<nl>   -> syntax error near `;)' / `for ((;;)'
```
The body may follow as `; do`, `<newline> do`, ` do` (no separator), or `{ ... }`. All were verified.

`make_arith_for_command` (make_cmd.c:~255-295) splits the text on `;` at top level. It requires exactly 3 parts. Missing parts become `1`, which `declare -f` shows as `for ((1; 1; 1))`. Errors:
```
for ((i=0;i<2)); do :; done
bash: -c: line 1: syntax error: arithmetic expression required
bash: -c: line 1: syntax error: `((i=0;i<2))'
[status 2]
for ((i=0;i<2;i++;)); do :; done
bash: -c: line 1: syntax error: `;' unexpected
bash: -c: line 1: syntax error: `((i=0;i<2;i++;))'
[status 2]
```
These use `YYERROR`, so there is no "near token" line and no offending line. The shell still aborts with 2.

8.5 Single `(` versus case and functions:
- `WORD (` followed by `)` is a function definition. `echo ()` is the definition of a function named `echo`, which then wants a body: EOF gives `syntax error: unexpected end of file` (no compound name).
- `echo a (` and `echo a(b)` give error near unexpected token `(`. A `(` in the middle of a word is not an extglob unless extglob is on.

======================================================================
9. HERE-DOCUMENTS
======================================================================

Collection:
- Each `<<` or `<<-` redirection is pushed on a FIFO when the grammar reduces it (`push_heredoc`, parse.y:3106). At most 16 can be pending (`HEREDOC_MAX`). The 17th gives:
  ```
  bash: -c: line 2: maximum here-document count exceeded
  [status 2]
  ```
  This uses the message form, with no offending line, and the shell exits.
- Bodies are read (`gather_here_documents`, 3120) when the lexer is **about to return an unquoted newline token**. They are also read when a `simple_list` or `compound_list` reduction finds some pending (input ending at EOF).
- Bodies are read in FIFO order, i.e. **left to right as the operators appeared on the line**. That holds across pipelines, `&&`, `;` and compound commands: `cat <<A; cat <<B`, `cat <<A | cat - <<B`, `cat <<A <<B` (only B reaches cat), `if cat <<A; then`, and `for ...; do cat <<A; done` all behave this way.
- A here-doc operator inside a backslash-continued line is collected after the next **real** newline:
  ```
  cat <<A \
  | tr a-z A-Z
  lower
  A          -> LOWER
  ```

Delimiter:
- The delimiter word is lexed normally.
- If any part of it is quoted (`W_QUOTED`: a quote character or a backslash), the delimiter is the word after quote removal, and the body is literal.
- Otherwise the delimiter is the word as written (a `$x` stays the literal text `$x`), and the body is subject to expansion later.
- Verified: `<<"A"`, `<<\A`, `<<A"B"` (delimiter `AB`), `<<'A'B` (delimiter `AB`), `<<$x` (delimiter `$x`), `<< "A B"`.

Reading the body (`make_here_document`, make_cmd.c:512; `read_a_line`, parse.y:2197):
- Lines are read raw from the input, not through the token lexer.
- For an unquoted delimiter, `\<newline>` is removed while reading, and other backslashes are kept for later expansion. For a quoted delimiter nothing is removed.
- `<<-`: leading tabs are stripped from each body line. The delimiter test runs on the line **both before and after** stripping (a historical compatibility hack).
- A line ends the body only if it equals the delimiter exactly, apart from the newline. Leading or trailing spaces do not match (probe h09: `  A` and `A ` are body lines).
- A final line without a newline at EOF still counts: `-c $'cat <<A\nA'` with no final newline prints nothing and ends cleanly.
- The body is **not parsed** at parse time. `cat <<A` + `$(echo a` + `A` fails only when expanded: `bash: command substitution: line 3: unexpected EOF while looking for matching `)'`, status 1.

EOF before the delimiter is a warning, not an error. The status is unaffected and the partial body is used:
```
# -c $'cat <<A\nnot terminated\n'
bash: line 2: warning: here-document at line 1 delimited by end-of-file (wanted `A')
not terminated
[status 0]
```
- The first number is the line where EOF was hit (the runtime prefix).
- The second is `line_number` at the moment collection started, i.e. the line holding the operator.
- The text in `` `...' `` is the delimiter after quote removal.

Inside a command substitution:
- The comsub saves the outer pending list and starts its own.
- Here-docs inside `$( )` are collected at the newlines inside it: `echo $(cat <<A` + `inner` + `A` + `) outer` prints `inner outer`.
- An outer here-doc whose operator precedes the comsub on the same line is read **after** the comsub's closing line:
  ```
  cat <<A; echo $(cat <<B
  b
  B
  )
  a
  A            -> a, then b
  ```
- If a comsub ends with here-docs still pending, they are read with a warning `command substitution: N unterminated here-document(s)`. Not probed.
- Compatibility: inside a comsub, a body line that starts with the delimiter followed by text containing `)` ends the here-doc. The rest (`)...`) is pushed back as input. This path also prints the EOF warning:
  ```
  echo $(cat <<A
  x
  A)
  bash: line 3: warning: here-document at line 1 delimited by end-of-file (wanted `A')
  x
  [status 0]
  ```
- A comsub that hits EOF with an open here-doc gives the warning first, then the comsub EOF error:
  ```
  # $'echo $(\ncat <<A\nx\n)\n'
  bash: line 4: warning: here-document at line 2 delimited by end-of-file (wanted `A')
  bash: -c: line 5: unexpected EOF while looking for matching `)'
  [status 2]
  ```

Aliases may contain here-doc operators. The body is then read from the remaining alias text if any, else from the input (`heredoc_string`): `alias x="cat <<EOF"` + `x` + `hello` + `EOF` prints `hello`.

Printer note for M1: `declare -f` prints a here-doc body right after its command line. A lone `EOF` line follows, then an **empty line** before the closing brace when the here-doc is the last command:
```
f () 
{ 
    cat <<EOF
body $1
EOF

}
```

======================================================================
10. ALIAS EXPANSION
======================================================================

Enabling:
- Aliases are expanded only when `expand_aliases` is on. Non-interactive shells have it off by default:
  `-c $'alias x=echo\nx hi'` prints `bash: line 2: x: command not found` with status 127.
- When `-c` and string parses run, the global setting is restored on reset (`expaliases_flag`).

Eligibility (`alias_expand_token`, parse.y:3249, called from `read_token_word`):
- The word is unquoted. Any quote or backslash disqualifies it: `\p`, `"p"` and `p\ x` are not expanded.
- Either `PST_ALEXPNEXT` is set (the previous alias ended in a blank), or the word is in assignment position (`assignment_acceptable(last_read_token)`). That means command position and not a case pattern.
  - Command position includes:
    - after `; & | && || ( { ! time` and other reserved words (5899);
    - after an ASSIGNMENT_WORD: `x=1 x assign` expanded;
    - after a redirection when only redirections precede.
  - Not eligible:
    - case patterns;
    - the name after `function` (`function x { }` defines x even when x is an alias);
    - `for` variables and word lists;
    - redirection targets;
    - anything inside `[[ ]]` (last_read_token is frozen at COND_START);
    - inside `$( )` at parse time (aliases off, non-POSIX).
- The name `x` in `x() { ...; }` **is** in command position and is expanded: with `alias x="echo a"`, the line `x() { echo func; }` gives error near unexpected token `(`.
- The alias is not currently being expanded (`AL_BEINGEXPANDED`). This is the recursion guard: `alias echo="echo X"; echo hi` prints `X hi`, and `alias foo=bar bar=foo; foo` gives `foo: command not found`.

Mechanism:
- The alias value is pushed on the input stack (`push_string`) and the lexer restarts token reading. The expansion is re-tokenized from scratch, so it may contain anything:
  - several commands: `alias x="echo a; echo b"`;
  - newlines: `alias y="echo c<nl>echo d"`;
  - operators at the end: `alias x="echo a |"` followed by `x cat`;
  - reserved words: `i true; t echo yes; fi`, `alias o="{"`;
  - complete loops;
  - here-doc operators;
  - unbalanced quotes, which continue into the following input lines (`alias x="echo \"unterminated"` gives `line 3: unexpected EOF while looking for matching `"'`).
- The first word of the expansion is itself eligible, so chains work: `alias x="echo a" y="x b"` and `y c` gives `a b c`.
- **Trailing blank**: if the value ends in a blank, `PST_ALEXPNEXT` is set when its text is exhausted, and the next word is eligible regardless of position. `alias a="echo " b=hello c=world; a b c` gives `hello c`: b expanded, c did not, because "hello" does not end in a blank.
- **End-of-alias space** (shell_getc 2795-2826). When the alias text runs out, the alias must stay "being expanded" until its last word is complete. So before popping, `shell_getc` returns one synthetic space, once, if all of these hold:
  - the last character of the value is not a blank, newline, metacharacter or unquoted backslash;
  - we are not inside a single- or double-quoted string, a comment or a here-doc.
  Then it pops. The pop also clears the recursion flag of that alias.
- **Comment in an alias swallows the rest of the original line**, because `discard_until('\n')` keeps reading past the popped alias into the saved line. With `alias x="echo # comment"`, the line `x arg` prints an empty line, not `arg`.
- The value is whatever `alias` stored. Expansions in a double-quoted definition happen at definition time: `alias e="echo $(echo sub)"` is stored as `echo sub`.

Timing:
- Aliases are expanded while lexing. A definition affects only lines that are **read after the alias command executes**. Everything parsed as part of the same inputunit, the same compound command, or the same function body is unaffected:
```
shopt -s expand_aliases; alias x=echo; x hi     -> bash: line 1: x: command not found
x there                                          -> there
if true; then alias x=echo<nl>x one<nl>fi<nl>x two -> line 3: x: command not found, then two
f() { alias y=echo; y three; }<nl>f; y four      -> y: command not found twice (lines 6 and 7)
```

Command substitution and aliases (non-POSIX mode):
- At parse time the comsub body is parsed with aliases off and stored as the canonical reprint, aliases **unexpanded**: `declare -f` shows `$(x hi)`.
- At execution the text is parsed again, and aliases are expanded if `expand_aliases` is on and the alias exists **then**:
  ```
  alias x=echo
  f() { echo $(x hi) `x bq`; x direct; }
  declare -f f    -> echo $(x hi) `x bq`; echo direct      (the direct use was expanded at definition)
  f               -> hi bq / direct
  unalias x; f    -> bash: line 3: x: command not found (twice), an empty line, then direct
  ```
- In POSIX mode, aliases are expanded during the parse-time comsub parse (parse_comsub 4533). Not probed.

======================================================================
11. GRAMMAR NOTES FOR A RECURSIVE-DESCENT PORT
======================================================================

The yacc grammar is at parse.y:435-1508. bash is LALR(1). An error is detected at the first lookahead token that cannot extend a valid prefix, and that token is `current_token`, the one the message names. A recursive-descent parser has to report at the same token, so every alternative must be decided with one token of lookahead, the way yacc does it.

Main rules, paraphrased:
- **inputunit**: `simple_list` then newline or EOF; or a bare newline; or EOF; or the `error` recovery productions. In non-interactive mode, recovery aborts the parse.
- **simple_list**: `simple_list1` optionally followed by `&` or `;`, then the terminator.
  - Inside `simple_list1`, `&&` and `||` may be followed by newlines.
  - `;` and `&` may not be followed by newlines inside the list. A newline after them ends the inputunit, so `echo a &` + newline + `& echo b` is an error near `&` on line 2.
- **compound_list** (bodies of `if`, `while` and so on, and comsub contents): leading newlines are allowed, and newline, `;` and `&` all separate. It must end with a terminator before the closing reserved word, **except** for the `)` of a subshell or comsub and the `}` of a group.
  - The terminator rule really applies only to reserved words, because `}` and `fi` are recognized only in reserved position, which requires a preceding separator. `{ echo a }` treats `}` as an argument.
- **pipeline**: `pipeline | newlines pipeline`, or `pipeline |& newlines pipeline`. `|&` appends a `2>&1` redirection to the left command.
- **pipeline_command**: optional `!` and `time` prefixes (6A).
- **command**: a simple command; a shell_command with an optional redirection list; a function definition; or a coproc.
- **simple_command**: one or more elements, each a WORD, ASSIGNMENT_WORD or redirection, in any order.
- **redirection**: an optional NUMBER or REDIR_WORD source, the operator, then a WORD target. The `<&`/`>&` targets may be a NUMBER, a WORD or `-`. `&>` and `&>>` take no source number.
- **shell_command**: `for`, `case`, `while`, `until`, `select`, `if`, subshell `( compound_list )`, group `{ compound_list }`, ARITH_CMD, COND, arith-for.
  - `for NAME [newlines] [in words... terminator] [newlines] do ... done`, or the same with `{ ... }` instead of `do ... done`. A `;` may directly follow NAME. `for x in` + terminator gives an empty list. `for x do` and `for x; do` mean `in "$@"`.
  - `select` has the same shapes.
  - `case WORD [newlines] in [clauses] esac`:
    - each clause is `[newlines] [(] pattern {| pattern} ) compound_list-or-newlines`, ended by `;;`, `;&` or `;;&`;
    - the last clause's terminator is optional before `esac`;
    - `case x in esac` is valid;
    - patterns are WORDs. `)` alone, `a|)` and `| a` are errors.
  - `if list then list {elif list then list} [else list] fi`. Empty lists are errors: `if true; then fi` gives error near `fi`, and so does `if true; then :; else fi`.
  - `while; do` gives error near `;`. `{ }` gives error near `}`. `( )` gives error near `)`.
- **function_def**:
  - `WORD ( ) newlines function_body`;
  - `function WORD ( ) newlines function_body`;
  - `function WORD function_body`;
  - `function WORD newline newlines function_body`.
  - The body must be a **shell_command** (any compound), optionally with redirections, which are attached to the body command. `f() echo hi` and `function f echo hi` give error near unexpected token `echo`. `f() ( ... )`, `f() if ...; fi`, `f() for ...`, `f() [[ ... ]]` and `f() ((1))` are all valid bodies. In `r() [[ 1 ]] && echo r1`, only the `[[ ]]` is the body.
  - Function names can be reserved words after `function`, and can contain `-` or `=` (`f-x`, `function a=b` are accepted in non-POSIX mode).
- **coproc**:
  - `coproc shell_command [redirs]` (name COPROC);
  - `coproc WORD shell_command [redirs]`;
  - `coproc simple_command`.
  - The WORD form is chosen when the token after the WORD is a reserved word or the start of a compound command. Reserved words are accepted there because of the `WORD after COPROC` rule in 5899.
  - `coproc` alone gives error near `newline`.

Line numbers attached to commands (for LINENO and `declare -f` are a separate topic):
- `simplecmd_lineno` is the line of the first word or redirection.
- Compound commands use the line of their opening reserved word (`compoundcmd_lineno`).
- `function_dstart` and `function_bstart` record where a definition and its body start.

======================================================================
12. SYNTAX ERRORS: MECHANISM AND EXACT TEXT
======================================================================

12.1 The decision tree in `report_syntax_error(NULL)` (parse.y:6833), reached through `yyerror`:

1. If `current_token` is non-zero, `EOF_Reached` is clear, and `error_token_from_token(current_token)` gives a name (section 3):
   - print `syntax error near unexpected token `NAME'`;
   - or, when a comsub or funsub parse is active (`shell_eof_token` set) and the token is not its closer, `syntax error near unexpected token `NAME' while looking for matching `)'`;
   - then, if non-interactive, print the offending line (12.5).
2. Else, if the line buffer is non-empty:
   - print `syntax error near `TEXT'` using the text scan (12.3), if it finds something;
   - then print the offending line if non-interactive.
3. Else (empty buffer, typically at EOF):
   - EOF inside a comsub: `unexpected EOF while looking for matching `)'`;
   - else EOF with a non-empty compound stack: `syntax error: unexpected end of file from `KW' command on line N`, where KW is the innermost open compound keyword (`if while until for select case { ( [[`) and N its line. The form `... from command on line N` exists for unnamed entries;
   - else `syntax error: unexpected end of file` if at EOF, or plain `syntax error` otherwise.
   No offending line is printed in this branch.

In every case `last_command_exit_value` becomes 2 (EX_BADUSAGE), and `PIPESTATUS` is set from it. The code uses EX_BADSYNTAX (257) instead while a builtin is executing under parse_and_execute (eval or source). The observed eval and source return value is still 2 (u09-u11), because `parse_and_execute` returns EX_BADUSAGE (evalstring.c:586). See open question 10.

`report_syntax_error(message)` with an explicit message, used for the here-doc limit, prints only `MESSAGE`.

12.2 The compound stack (`compoundcmd_lineno`, parse.y:347-359 and 5872):
- Pushed when `if while until for select case { ( [[` are lexed. A `{` after `function NAME`, or after `for ((...))`, pushes too.
- Popped when the grammar reduces the construct.
- **It is not reset by `reset_parser`.** After a syntax error inside a compound, stale entries survive into later parses in the same shell, which shows up with `eval`:
  ```
  eval 'if then'      -> bash: eval: line 1: syntax error near unexpected token `then'
                         bash: eval: line 1: `if then'
  eval 'echo a |'     -> bash: eval: line 3: syntax error: unexpected end of file from `if' command on line 1
  eval 'echo b &&'    -> bash: eval: line 4: syntax error: unexpected end of file from `if' command on line 1
  # whereas on its own:  eval 'echo b &&' -> bash: eval: line 2: syntax error: unexpected end of file
  ```
  We should reproduce this leak.

EOF forms observed. Each probe ends in a newline, so EOF is on the line after the last one:
```
echo a |            -> line 2: syntax error: unexpected end of file
echo a &&<EOF>      -> line 2: syntax error: unexpected end of file
function f / f()    -> line 2: syntax error: unexpected end of file
echo ()             -> line 2: syntax error: unexpected end of file
(                   -> line 2: syntax error: unexpected end of file from `(' command on line 1
{                   -> ... from `{' command on line 1
while true; do      -> ... from `while' command on line 1
until false; do     -> ... from `until' command on line 1
for x in a; do      -> ... from `for' command on line 1
for x in            -> ... from `for' command on line 1
select x in a; do   -> ... from `select' command on line 1
case x in           -> ... from `case' command on line 1
case x in a) echo a esac  -> ... from `case' command on line 1
if                  -> ... from `if' command on line 1
if true; then while :; do -> ... from `while' command on line 1      (innermost)
f() {               -> ... from `{' command on line 1
echo a\n\nf() {\n  echo b  -> a / line 5: ... from `{' command on line 3
echo a\nif true; then\necho b  -> a / line 4: syntax error: unexpected end of file from `if' command on line 2
```

12.3 The text scan (`error_token_from_text`, parse.y:6772). It is used when the token has no name: YYUNDEF after a `[[` or `((`-for failure, or a COND_CMD.
- Start at the current buffer index. If that index is at the NUL terminator, step back one.
- Step back over blanks and newlines. The end of the token is just after this point.
- Step back further until a character from the set space, newline, tab, `;`, `|`, `&`, or index 0.
- Step forward over blanks.
- The result is the text between, or the single character at index 0 if both ends are 0.

Exact matching needs the same buffer index as bash at the time of the error, i.e. how much lookahead the lexer consumed. Examples:
- `[[ a` + newline: the newline was consumed, so the scan backs up over it and finds `a`;
- `[[ a == ;`: gives `;`;
- `for ((;;)` + newline: the newline was pushed back, so the index is at it. The scan finds `)`, walks back to the second `;` and yields `;)`.

12.4 Token names in messages:
- WORD tokens print their stored text: after `$'...'` translation, with CTLESC bytes, but not dequoted.
- If the name contains non-printable characters (`ansic_shouldquote`), it is printed ANSI-C quoted (`ansic_quote`):
```
f() { :; } $'a\tb'   -> syntax error near unexpected token `$'\'a\tb\'''
                        `f() { :; } $'a	b''          (offending line printed raw, with the TAB)
f() { :; } $'\001'   -> syntax error near unexpected token `$'\'\001\001\'''   (CTLESC doubled)
```

12.5 The offending line (`print_offending_line`, parse.y:6814):
- It is printed only by branches 1 and 2 of 12.1, and only when non-interactive.
- It is the **current line buffer** with trailing newlines removed, printed as `` `TEXT' `` with the same `line N` prefix.
- It is the physical line where the error token was read, not the whole command. Leading and trailing blanks are kept: `   echo a ; fi   ` is echoed exactly. Multi-line constructs echo only that line:
```
if true<nl>then<nl>fi fi        -> line 3: ... `fi' / line 3: `fi fi'
if true; then<nl>fi             -> line 2: ... `fi' / line 2: `fi'
echo a<nl><nl><nl>fi            -> a / line 4: ... `fi' / line 4: `fi'
echo b; fi (line 2)             -> a / line 2: ... `fi' / line 2: `echo b; fi'
echo a |<nl>| b                 -> line 2: ... `|' / line 2: `| b'
echo a<nl>&& echo b             -> a / line 2: ... `&&' / line 2: `&& echo b'
f() {<nl>  echo $(<nl>    if true<nl>  )<nl>}   -> a / line 5: ... `)' / line 5: `  )'
```

12.6 Alias quirk: while an alias is being read, the line buffer **is the alias text**. When the error token came from it, the "offending line" is the alias buffer, after `shell_ungetc` has written its synthetic space back into it:
```
shopt -s expand_aliases
alias x=fi
if true; then echo y; x      (x closes the if)
x                            -> y
                                bash: -c: line 4: syntax error near unexpected token `fi'
                                bash: -c: line 4: `f '
```

12.7 `set -e`. `parser_error` itself calls `exit_shell(2)` when `exit_immediately_on_error` is set (error.c:324). The first diagnostic line is printed, then the shell exits **before** the offending line:
```
set -e<nl>if then          -> bash: -c: line 2: syntax error near unexpected token `then'   [2]
set -e<nl>eval "if then"   -> bash: eval: line 2: syntax error near unexpected token `then'  [2]
set -e<nl>echo $(fi)       -> bash: -c: line 2: syntax error near unexpected token `fi' while looking for matching `)'  [2]
```
Note: eval line numbers continue from the outer line, so a one-line eval string on line 2 reports `line 2`, or `line 3` at its EOF.

12.8 What runs after an error, and the status:

| Situation | `-c` | script / stdin | Notes |
|---|---|---|---|
| top-level syntax error | stop, 2 | stop, 2 | earlier complete lines already ran |
| EOF inside quote, `${`, `$(`, `((` | stop, 2 | stop, 2 | |
| non-EOF error inside `$(` or `${ }` | stop, **127** | stop, 2 | FORCE_EOF; also when it happens inside eval |
| compound-assignment error `a=(1 ; 2)` | rest of line discarded, status 1, **next line runs** | same | DISCARD (7167-7179) |
| compound assignment hits EOF `a=(1 2` | status 1, stop | same | newlines are allowed inside, so it swallows later lines |
| `for ((` part-count error | stop, 2 | stop, 2 | two messages, no offending line |
| here-doc EOF | warning, status unaffected | same | |
| here-doc limit (17) | message, exit 2 | same | |
| inside `eval` | eval returns 2, shell continues | same | except comsub errors (exit 127 in -c) |
| inside `. file` | source returns 2, shell continues | same | prefix is the file name |
| inside backquotes, or a comsub reparsed at run time | message with `command substitution:` prefix, empty output, outer command continues | same | outer status 0 in the probes |
| `set -e` active | first message line only, exit 2 | same | |

Probes for the compound-assignment row:
```
a=(1 ; 2); echo same<nl>echo after   -> line 1: syntax error near unexpected token `;'
                                        line 1: `a=(1 ; 2); echo same'
                                        after                                [status 0]
echo x; a=(1 ; 2)                    -> (error, two lines; "x" is NOT printed) [status 1]
eval 'a=(1 ; 2)'; echo same $?       -> (error lines with the eval: prefix) / same 1 / after
a=(1 2<nl>echo after                 -> line 1: unexpected EOF while looking for matching `)'  [status 1]
```

12.9 Errors that are **not** parse errors in bash, so the parser must accept the input:
- `${a b}`, `${}`, and any bad `${...}`: `bash: line 1: ${a b}: bad substitution` at expansion. A function containing one defines fine and fails when called.
- Arithmetic syntax in `$(( ))`, `(( ))`, `$[ ]` and `for (( ))` parts (except the part count, 8.4).
- `for 1x in a`: `bash: line 1: `1x': not a valid identifier`, status 1, at run time.
- The contents of backquotes and of here-doc bodies.
- `$((...)` text that turns out not to be arithmetic (8.1).

======================================================================
13. REQUIREMENTS FOR THE M1 PARSER
======================================================================

- Lex from a line buffer with the same refill points and pushback semantics. Error positions (12.3), offending lines (12.5, 12.6) and line numbers depend on them.
- Keep `last_read_token`, `token_before_that` and `two_tokens_ago` exactly as yylex does. Reserved words, `in`/`do`/`esac`, `time`, aliases, assignments and `((` all key off them. `[[` must **not** update them.
- Keep these parser-state flags with the same set and clear points: CASEPAT, CASESTMT, ALEXPNEXT, ASSIGNOK, CMDBLTIN, ALLOWOPNBRC, REDIRLIST, CONDCMD, CONDEXPR, REGEXP, EXTPAT, CMDSUBST, FUNSUBST, COMPASSIGN, NOERROR, SUBSHELL, ENDALIAS, COMMENT, HEREDOC.
- Keep the counters `open_brace_count`, `esacs_needed_count`, `expecting_in_token` and `expecting_in_command`, and the compound stack. The compound stack must survive resets (12.2).
- Comsub text is the canonical reprint. Backquotes, `$((`, `${...}` and `$[` are verbatim. `$'..'` and `$"..."` are translated at lex time. Compound assignments are re-joined with single spaces.
- Parse status codes: 2 for syntax errors; 127 for comsub errors under `-c`; 1 and continue for compound-assignment errors.

======================================================================
14. CHECKLIST
======================================================================

Verified against the oracle (probe ids in parentheses):
- [x] Parse unit is one line: `echo a; if then` prints nothing, while `echo a` then `if then` on the next line prints `a` (e1, e2; modes c/n/f/s).
- [x] Prefixes for -c, -c with $0, script, stdin, eval, source, comsub, and runtime messages (section 0).
- [x] Unexpected-token names: `;;` `|` `)` `fi` `then` `newline` `;` `&` `||` `}` `;&` `[[` `(` `>` `!` `done` WORDs, ASSIGNMENT_WORD, ARITH_CMD (t01-t48, w02-w09, y05-y30, s08-s11, r05).
- [x] ANSI-C quoted token names, with CTLESC doubling (u05, u06).
- [x] All `unexpected end of file from `KW' command on line N` keywords: if while until for select case { ( [[. Innermost wins (t05-t19, c05).
- [x] Plain `syntax error: unexpected end of file` (t03, t04, t20, t21, t34).
- [x] `unexpected EOF while looking for matching` for " ' ` } ) and its start-line versus EOF-line rule (e6-e8, t25, t26, t29, y01-y03, u21, u22).
- [x] Every `[[` message form, including EOF variants and line-number sources (c01-c30).
- [x] `for ((` errors (t43, t44, t46), arithmetic command vs subshell (r13, r14, z08, z09, x02).
- [x] Comsub errors: "while looking for matching" suffix, no suffix on the closer, status 127 in -c versus 2 in script/stdin, exit even from eval (t27, t28, u01-u04, z01-z07, s01-s06, q03).
- [x] Compound-assignment error recovery with status 1 (u18, u19, d01-d03, q04).
- [x] Backquote errors deferred to run time (t49, u08, z05).
- [x] `set -e` truncates the message (u12, s03, s04).
- [x] Compound stack leaks across evals (q01, q02).
- [x] Alias rules: expand_aliases needed, same-line and same-compound rules, trailing blank, recursion guard, chains, reserved-word aliases, aliases ending in operators, multi-command and multi-line aliases, unterminated quote, here-doc in alias, quoting disables, function name forms, comment swallowing, comsub deferral (a01-a22, x01).
- [x] Alias offending-line quirk `f ` (a08).
- [x] Here-docs: FIFO order, pipelines, two on one command, in `$( )`, outer-pending around a comsub, `<<-`, quoted and unquoted delimiters, partial quoting, `$x` delimiter, exact-line match, EOF warning text and numbers, `DELIM)` compatibility, comsub EOF with an open here-doc, max 16, continuation lines, body not parsed (h01-h23, x03, x05, z04).
- [x] Line continuation inside and outside quotes, inside comments, and trailing `\` at the end of a -c string (l01-l03).
- [x] Comments inside `$( )` swallow `)` until the newline (l05).
- [x] Word scanning: `${}` nesting/first-close, `$'..'`, `$".."`, array subscripts with spaces in assignment position only, process substitution mid-word, NUMBER versus overflow, `{var}` redirections versus `{1}` and `{v b}` (l06, w01, x06, x07, x02).
- [x] Extglob: needs an earlier line, forced on for `[[ == ]]`, not forced in comsub under compat 53, `!(` becomes a word, function text survives `shopt -u` (l07-l11, g01-g04).
- [x] Reserved-word positions and the special cases for in/do/esac/{/}/time/!/[[ (r01-r16, w04-w07, y25).
- [x] Function-definition forms and invalid bodies (r07-r09, w04-w07, a17, a18).
- [x] coproc forms (r10, t24).

Open questions (not yet probed, or unclear):
1. POSIX mode: aliases expanded inside `$( )` at parse time; reserved words checked before aliases; `time -` rule; single quotes inside a double-quoted `${}` (interp 221). These are needed for the sh/ash persona and deferred to M7.
2. Interactive behaviour: no offending line, `NAME:` prefix only, `ignoreeof`, error-recovery rule `error '\n'` accepting and continuing, PS2 prompting points. Needed for M6.
3. (Resolved: `\<newline>` inside an operator joins it, as section 4 shows.) Still open: `\<newline>` between `$` and `(`/`{`, and inside `((`.
4. Exact `error_token_from_text` results for other YYUNDEF sources, for example `((` EOF inside `for`, `[[` after alias text. The algorithm is as described, but the buffer index must match bash's consumption.
5. Funsub reprint terminators in all cases (`${ a & }`, newline-terminated funsubs), `${(` forms, and `${|` with an empty body.
6. `command substitution: N unterminated here-document(s)` warning: when it triggers in 5.3, and its exact prefix.
7. Line numbers stored on commands for LINENO and `BASH_LINENO` inside multi-line constructs, here-docs and aliases. Section 11 lists only the sources. This needs its own probe set in M2.
8. CTLESC/CTLNUL bytes in source text, such as raw 0x01 in a script: their effect on words, here-docs and error echo.
9. Behaviour on lines longer than 32 KiB, which free the buffer; probably irrelevant.
10. The `EX_BADSYNTAX` (257) path in POSIX mode, where a syntax error in eval or source aborts a non-interactive shell (evalstring.c:588).
11. The full canonical printer layout for `declare -f`, including here-doc trailing blank lines, case and coproc indentation, and redirection order (w01 shows redirections reordered after the words: `echo a2 99999999999 {1} 2> f1 > f2 ...`). That layout belongs to a separate printer spec.
