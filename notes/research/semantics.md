Bash 5.3 language-semantics inventory for unabashed (planning reference)

Source root: `C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3` (paths below are relative to it). This report was gathered by reading only: no files were changed and no scripts from the tree were run.

Tiers: P0 means needed for typical scripts and AI one-liners, P1 means common, P2 means rare or interactive-only.

======================================================================
0. EFFORT HINTS (line counts)
======================================================================
Core files:
- `subst.c`: 13264 lines (expansions). `parse.y`: 7530 (`y.tab.c` is 9889, generated). `variables.c`: 6521. `execute_cmd.c`: 6409. `jobs.c`: 5480. `bashline.c`: 5023 (readline glue and completion).
- `shell.c` 2129, `arrayfunc.c` 1699, `expr.c` 1694 (arithmetic), `pcomplete.c` 1686, `trap.c` 1675, `print_cmd.c` 1648 (needed for `declare -f`, `type`, `set -x`), `redir.c` 1520, `general.c` 1458, `array2.c` 1288 / `array.c` 1230, `bashhist.c` 1070, `nojobs.c` 1048.
- `test.c` 950, `pathexp.c` 942, `braces.c` 931, `sig.c` 859, `make_cmd.c` 862, `findcmd.c` 704, `input.c` 604, `alias.c` 580, `assoc.c` 573.
- Top level total: about 91.7k lines.

Builtins (`builtins/*.def`): 19106 lines in total.
- Largest: `printf` 1669, `read` 1394, `declare` 1113, `set` 1047, `shopt` 930, `complete` 918, `ulimit` 820, `fc` 804, `pushd` 803, `setattr` 680, `cd` 665, `enable` 600, `help` 546, `history` 485, `wait` 476, `type` 437, `bind` 428, `umask` 375, `trap` 368, `mapfile` 351, `getopts` 330.

Libraries:
- `lib/readline`: about 34.3k lines.
- `lib/glob` plus `lib/sh`: about 19k lines.

Documentation:
- `doc/bashref.texi`: 11553 lines. `doc/bash.1`: 13495 lines.

Tests: `tests/` has 726 files, including the `*.tests` suites (with matching `.right` files). Names: alias, appendop, arith-for, arith, array, assoc, attr, braces, builtins, case, casemod, complete, comsub, comsub2, comsub-eof, comsub-posix, cond, coproc, cprint, dbg-support(2), dstack(2), dynvar, errors, exp, exportfunc, extglob(2,3), func, getopts, glob, glob-bracket, globstar, heredoc, herestr, histexp, history, ifs, ifs-posix, intl, invert, invocation, iquote, jobs, lastpipe, mapfile, more-exp, nameref, new-exp, nquote(1-5), parser, posix2, posixexp(2), posixpat, posixpipe, precedence, printf, procsub, quote, quotearray, read, redir, rhs-exp, rsh, set-e, set-x, shopt, strip, test, tilde(2), trap, type, varenv, vredir. These `.right` files are usable as oracle expectations (they are data, not code). Suggestion: run them inside the virtual OS layer.

Documentation map (bashref.texi line numbers):
| Line | Section |
|---|---|
| 404 | Quoting |
| 489 | ANSI-C quoting |
| 546 | Locale translation `$"..."` |
| 686 | Comments |
| 728 | Reserved words |
| 769 | Pipelines |
| 850 | Lists |
| 907 | Compound commands |
| 1442 | Coprocesses |
| 1536 | Functions |
| 1751 | Parameters |
| 1890 | Special parameters |
| 1982 | Expansions |
| 2044 | Brace expansion |
| 2153 | Tilde expansion |
| 2234 | Parameter expansion |
| 2818 | Command substitution |
| 2920 | Arithmetic expansion |
| 2954 | Process substitution |
| 2993 | Word splitting |
| 3048 | Filename expansion |
| 3131 | Pattern matching |
| 3273 | Quote removal |
| 3280 | Redirections |
| 3611 | Executing commands |
| 3741 | Execution environment |
| 3890 | Exit status |
| 3939 | Signals |
| 4170 | Bourne builtins |
| 4879 | Bash builtins |
| 5887 | `set` |
| 6206 | `shopt` |
| 6621 | Special builtins |
| 6664 | Bourne shell variables |
| 6735 | Bash variables |
| 7740 | Invocation |
| 7924 | Startup files |
| 8065 | Interactive shells |
| 8407 | Shell arithmetic |
| 8531 | Aliases |
| 8606 | Arrays |
| 8832 | Directory stack |
| 8959 | Prompt |
| 9047 | Restricted shell |
| 9121 | POSIX mode |
| 9630 | Compatibility mode |
| 9861 | Job control |

======================================================================
1. LEXING / PARSING (P0 unless noted)
======================================================================
Grammar: `parse.y`.
- Tokens (lines 389-406): IF THEN ELSE ELIF FI CASE ESAC FOR SELECT WHILE UNTIL DO DONE FUNCTION COPROC, COND_START COND_END COND_ERROR, IN BANG TIME TIMEOPT TIMEIGN, WORD ASSIGNMENT_WORD REDIR_WORD NUMBER ARITH_CMD ARITH_FOR_EXPRS COND_CMD, AND_AND OR_OR GREATER_GREATER LESS_LESS LESS_AND LESS_LESS_LESS GREATER_AND SEMI_SEMI SEMI_AND SEMI_SEMI_AND LESS_LESS_MINUS AND_GREATER AND_GREATER_GREATER LESS_GREATER GREATER_BAR BAR_AND, DOLPAREN DOLBRACE.
- Rules: inputunit 435, redirection 532, simple_command_element 804, simple_command 831, command 837, shell_command 863, function_def 1054, coproc 1125, arith_command 1203, cond_command 1207, case_clause 1222, pipeline_command 1408.
- A hand-written recursive-descent parser is fine. Bash itself already recursively re-enters the parser for `$( )`: `parse_comsub` at 4451 and `xparse_dolparen` at 4686.

Lexer core:
- `read_token` (3557), `read_token_word` (5305), `shell_getc` (2475; handles backslash-newline removal and alias string-stack pops), `parse_matched_pair` (3877; scans `( )`, `{ }`, `[ ]`, quotes, and `$'...'` with nesting flags P_DQUOTE/P_COMMAND/P_BACKQUOTE/P_ARRAYSUB/P_DOLBRACE/P_ARITH).
- Operators recognized (read_token 3650-3740): `<<-`, `<<<`, `<<`, `>>`, `;;&`, `;;`, `&&`, `||`, `<&`, `>&`, `<>`, `>|`, `&>>`, `&>`, `|&`, `;&`. Also `<(` and `>(` (process substitution, lexed as part of a word), and `((`.

Quoting (bashref 404-580):
- Backslash.
- Single quotes.
- Double quotes. Inside them `\` is special only before `$`, backtick, `"`, `\` and newline. `!` is special only when histexpand is on and the shell is interactive.
- `$'...'` ANSI-C: `\a \b \e \E \f \n \r \t \v \\ \' \" \? \nnn \xHH \uHHHH \UHHHHHHHH \cx`.
- `$"..."` locale translation, P2 (can be treated as `"..."`; shopt `noexpand_translation`).

Comments:
- `#` starts a comment only at the start of a word.
- Interactive shells need shopt `interactive_comments` (on by default).
- read_token at about 3625 discards up to EOL and returns a newline.

Line continuation:
- Backslash-newline is removed in `shell_getc` before tokenization, except inside single quotes and here-doc bodies with quoted delimiters.
- In here-docs with unquoted delimiters, backslash-newline is removed in the body.

Reserved-word context:
- Reserved words are recognized only as the first word of a command, or after another reserved word / `;` / newline etc. See `reserved_word_acceptable` (5899) and `special_case_tokens` (3366). Specific rules:
  - `in` is recognized after `for/case/select WORD`.
  - `do` is recognized after `for NAME`, `for NAME;` and `for ((...))`.
  - `esac` is recognized right after `in`.
  - `{` is recognized after `function NAME` or `for ((...))`.
  - `}` is recognized only when open_brace_count > 0 and a reserved word is acceptable.
- `time` (with `-p` and `--` as TIMEOPT/TIMEIGN): `time_command_acceptable` at 3286. POSIX: not a keyword if the next token starts with `-`.
- `!` is BANG.
- `[[` and `]]`: COND_START/COND_END. `]]` is special only in PST_CONDEXPR.
- Case patterns: an optional leading `(`, and the `|` separator.

`((` versus `( (` (P0):
- `parse_dparen` (4901) and `parse_arith_cmd` (4963). After `((`, bash scans for a matching `))`.
- If the scan ends in `)` followed immediately by `)`, it is ARITH_CMD.
- Otherwise the consumed text is pushed back and parsed as nested subshells.
- `$((` is always tried as arithmetic first (POSIX interp 217, parse_comsub 4462). If that fails, it falls back to `$( (`.
- `for ((e1;e2;e3))` produces ARITH_FOR_EXPRS.

`[[ ]]` lexing (P0):
- `parse_cond_command` (5254), cond_expr/cond_or/cond_and/cond_term (5032-5250).
- Inside it, `&&`, `||`, `!`, `(` and `)` are operators. `<` and `>` are string comparisons, not redirections.
- No word splitting or globbing happens on operands.
- The right side of `==`, `!=` and `=` is a pattern (quoted parts are literal).
- `=~`: the right side is lexed in PST_REGEXP mode. Parentheses and `|` are allowed unquoted, and quoted parts match literally (bash escapes them).
- Unary and binary operators are the same as `test` (`test.c`), plus `-v`, `-R`, `-o optname`, `-N`, `-ef/-nt/-ot`, and the arithmetic `-eq` family (whose operands are evaluated as arithmetic expressions in `[[`).

Here-docs (P0):
- `push_heredoc` (3106) and `gather_here_documents` (3120). Bodies are read after the next unquoted newline, in redirection order.
- `<<-` strips leading tabs from both the body and the delimiter line.
- Any quoting in the delimiter (for example `'EOF'`, `"EOF"` or `\EOF`) disables expansion. Otherwise the body gets parameter, command and arithmetic expansion, and `\` is special only before `$`, backtick, `\` and newline.
- A here-doc inside `$( )` must be terminated inside the substitution. A delimiter can also end at the closing `)` (POSIX-mode difference).
- There is a hard cap (HEREDOC_MAX) with a syntax error when it is exceeded.
- `<<<` here-string: the word is expanded (no splitting or globbing) and a newline is appended.

Command substitution parsing (P0):
- Since 5.2, bash parses `$(...)` by actually running the parser on the contents. That correctly handles `case x in a) ...` (unbalanced `)` in case patterns), comments, here-docs and nested quotes.
- Backquotes: the contents are found by scanning to the next unescaped backtick. Inside them, `\` is special only before `$`, backtick and `\`. Then the contents are re-parsed. Nesting requires escaping.
- New in 5.3, `${ cmd; }` and `${| cmd; }` (funsub):
  - The character after `{` must be a space, tab, newline or `|`.
  - The closing `}` must be in reserved-word position, but it may abut following characters.
  - Grammar has comsub/funsub nonterminals (line 418) and a DOLBRACE token.

Alias expansion (P1; AI scripts rarely rely on it):
- `alias_expand_token` (3249). Expansion happens at lex time, for the first word of a simple command (`assignment_acceptable` / command position).
- If the alias value ends in a blank, the next word is also checked (AL_EXPANDNEXT / PST_ALEXPNEXT).
- Recursion is prevented with AL_BEINGEXPANDED.
- Expansion is done by pushing a string onto the input (push_string 2046).
- Non-interactive shells need `shopt -s expand_aliases`. In POSIX mode it is always on.
- Aliases defined on the same line do not take effect until the next line is read. In particular, aliases inside a function body are bound at definition time.

extglob affects parsing (P1):
- When extglob is on, `?( *( +( @( !(` are lexed as part of words (`read_token_word` handles the pattern parentheses).
- `shopt -s extglob` on the same line as its use does not affect parsing of that line.
- Bash forces extglob on while parsing `[[ ]]` operands. Bash 5.2+ also does this inside `$( )`, tracked in parse_comsub with the variables `local_extglob` and `was_extpat`.

Assignment words:
- `NAME=`, `NAME+=`, `NAME[sub]=` and `NAME[sub]+=` are recognized by `token_is_assignment` (5270) and the `assignment()` helper.
- Compound assignment `a=(...)`: `parse_compound_assignment` (7104).
- Assignment arguments to declaration builtins (`declare`, `local`, `export`, `readonly`, `typeset`) are expanded like assignments: no splitting, tilde after `=` and `:`.

Function definitions: `name() compound-command [redirs]`, `function name [()] compound-command`. In bash mode, names may include almost anything; 5.3 POSIX mode no longer requires identifiers.

Prompt decoding: `decode_prompt_string` (6265) implements `\u \h \H \w \W \$ \t \T \@ \A \d \D{fmt} \n \s \v \V \! \# \[ \] \e \a \nnn \\ \j \l`. P1 for interactive use; also used by `${x@P}`.

======================================================================
2. EXPANSION PIPELINE (subst.c; P0)
======================================================================
Order, per `expand_word_list_internal` (subst.c 13204):
1. Separate leading assignments (`separate_out_assignments` 12456).
2. Brace expansion (`brace_expand_word_list` 12693, `braces.c`).
3. Per word, `expand_word_internal` (11222) does tilde, parameter/variable, arithmetic, command substitution and process substitution, left to right in one pass.
4. Word splitting (`word_list_split` 12382, `list_string` 3117, IFS handling `setifs` 12301).
5. Pathname expansion (`glob_expand_word_list` 12587; skipped under `set -f`).
6. Quote removal (`word_list_quote_removal` 12274).

Bash marks quoted characters internally with CTLESC/CTLNUL bytes. A cleaner design is to carry per-byte "quoted" flags (or a parallel bitmap) in the arena.

Brace expansion (bashref 2044):
- `{a,b,c}`, `{x..y[..incr]}` with integers (zero-padding when either end has a leading 0) or single letters.
- Preamble and postscript, nested.
- Not performed on `${`. Requires an unquoted comma or a valid sequence.
- Controlled by `set -o braceexpand`.
- Done before all other expansions, so `{$a,b}` works.

Tilde expansion (2153):
- `~`, `~user`, `~+`, `~-`, `~N`, `~+N`, `~-N` (dirstack).
- In assignments, applied after `=` and after each unquoted `:`.
- Windows: `~user` needs a design decision; `~` comes from HOME.

Parameter expansion (2234-2817). All forms:
- `$name`, `${name}`, `$1..$9`, `${10}`, special parameters.
- `${!name}` indirection. For a nameref, `${!ref}` gives the target name.
- `${!prefix*}`, `${!prefix@}` (variable names); `${!arr[@]}`, `${!arr[*]}` (keys).
- `${#name}`, `${#arr[@]}`, `${#*}` (character length is multibyte-aware).
- `${p:-w}`, `${p-w}`, `${p:=w}`, `${p=w}` (cannot assign to positional or special parameters), `${p:?w}`, `${p?w}` (a non-interactive shell exits), `${p:+w}`, `${p+w}`.
- `${p:off}`, `${p:off:len}`: arithmetic offset and length, negative offsets need a space or parentheses, negative len means an end offset. For `$@`, offset 0 includes `$0`. Arrays slice by index order.
- `${p#pat}`, `${p##pat}`, `${p%pat}`, `${p%%pat}`.
- `${p/pat/rep}`, `${p//pat/rep}`, `${p/#pat/rep}`, `${p/%pat/rep}`:
  - Longest match.
  - The `/rep` part may be omitted.
  - shopt `patsub_replacement` (on by default in 5.2+) makes an unquoted `&` in rep mean the match, and `\&` a literal `&`. Quoted `&`, including `&` inside a quoted variable expansion, stays literal.
  - Implemented in `pat_subst` 9177.
- `${p^pat}`, `${p^^pat}`, `${p,pat}`, `${p,,pat}` (`parameter_brace_casemod` 9593). The undocumented `${p~}` and `${p~~}` toggle case.
- `${p@op}` with op in U u L Q E P A K a k (`parameter_brace_transform` 8916, `string_transform` 8733):
  - Q quotes in reusable form (single-quote style).
  - E applies `$'...'` escape processing.
  - P applies prompt expansion.
  - A gives an assignment or declare statement.
  - K and k give key/value pairs (k splits them into separate words).
  - a gives the attribute flags (for example `aAilnrtux`).
- All operators apply element-wise for `@` and `*` and for `arr[@]` / `arr[*]`.
- The `word` part undergoes tilde, parameter, command and arithmetic expansion, and quote removal (in context).
- Double-quote rules inside `${...}` within `"..."` are a known minefield; see tests posixexp*, rhs-exp, new-exp, more-exp, exp.
- Errors: "bad substitution", "unbound variable" (`set -u`; `$@` and `$*` are exempt), and a non-interactive exit on `${x?}`.

`$@` and `$*` semantics:
- `"$@"` produces separate words. With no positional parameters it produces nothing, even inside `"x$@y"` splicing.
- `"$*"` joins with the first character of IFS: a space if IFS is unset, nothing if IFS is null.
- Unquoted `$*` and `$@` are split.
- `string_list_dollar_star` 2900, `string_list_dollar_at` 2955, `pos_params` 3743.

Arithmetic expansion: `$(( ))`. The contents are expanded like double quotes (parameter, command substitution, quote removal), then evaluated (section 4). `$[ ]` is the deprecated form (P2).

Command substitution (`command_substitute` 7136, `read_comsub` 6680):
- Runs in a subshell.
- Strips trailing newlines. NUL bytes are dropped with the warning "ignored null byte in input".
- `$(< file)` is a fast path (`optimize_cat_file` 6664).
- The exit status of the last command substitution becomes `$?` when there is no command (for example `x=$(false)` gives `$?`=1).
- `set -e` in a command substitution: inherit_errexit is off by default, so the subshell clears `-e` unless in POSIX mode.

funsub `${ cmd; }` / `${| cmd; }` (`function_substitute` 6923):
- Runs in the current shell with a function-like local scope (`local` works; `return` ends it) but shares the positional parameters.
- stdout is captured (bash uses an anonymous temp file).
- `${|...}` expands to a local REPLY and does not capture stdout.
- This is an ideal fit for arena-scoped execution.

Process substitution `<(cmd)` and `>(cmd)` (`process_substitute` 6360):
- Expands to `/dev/fd/N` or a named FIFO path.
- Windows needs a strategy: a named pipe `\\.\pipe\...` passed as a path works for native tools; MSYS tools expect `/dev/fd`.
- Tier P1 (common in AI one-liners: `diff <(a) <(b)`).

Word splitting (2993):
- Only on unquoted results of parameter expansion, command substitution and arithmetic expansion.
- IFS whitespace (space, tab, newline when present in IFS) collapses and is trimmed. Non-whitespace IFS characters delimit, so adjacent ones produce empty fields.
- An unset IFS means " \t\n". An empty IFS means no splitting.
- Unquoted empty expansions are removed. Quoted empty strings (`""`, `''`) remain.
- `$'...'` and quotes inside a word keep it non-empty.
- Splitting is multibyte-aware in bash mode; POSIX requires byte-oriented splitting.

Pathname expansion (3048-3272; `pathexp.c`, `lib/glob`):
- Matches `* ? [...]` with classes `[:alpha:]` etc., equivalence classes `[=c=]`, collating symbols `[.c.]`, and `!` or `^` negation.
- Options:
  - nullglob, failglob (error "no match"), dotglob, nocaseglob, extglob, globstar.
  - globasciiranges (on by default).
  - globskipdots (on by default in 5.2+; `.` and `..` never match).
  - GLOBIGNORE (setting it implies dotglob semantics).
  - GLOBSORT (new in 5.3): name, size, mtime, atime, ctime, blocks, numeric, nosort, with a `+`/`-` prefix.
- `**` with globstar matches recursively; `**/` matches directories only.
- The result is sorted (collation order).
- extglob patterns: `?(p|q) *(..) +(..) @(..) !(..)`.
- The same matcher is used for case patterns, `[[ == ]]`, `${p#pat}` and `${p/pat}`. Those uses honor nocasematch (case and `[[`), not nocaseglob.

Quote removal: removes unquoted `\`, `'` and `"` that did not result from an expansion.

======================================================================
3. VARIABLES, ARRAYS, SCOPING (P0/P1)
======================================================================
Attributes (`declare` flags):
- -a indexed array, -A associative array.
- -i integer: every assignment is evaluated as arithmetic.
- -l / -u: lower or upper case on assignment.
- -n nameref.
- -r readonly.
- -t trace (functions inherit DEBUG/RETURN traps).
- -x export.
- -I inherit attributes from a previous scope.
- -g global; -f / -F function listing; -p print.

`+=` append: strings concatenate, integers add, arrays append elements (`arr+=(x)`), and associative arrays merge.

Indexed arrays (bashref 8606; `array.c` is a sparse doubly linked list with an index cache):
- Sparse.
- The subscript is an arithmetic expression; negative subscripts count from the end (an error when out of range).
- `arr=(a [5]=b c)`.
- `${arr[@]}`, `${arr[*]}`, `${#arr[@]}`, `${!arr[@]}`.
- `unset arr[i]` (beware globbing of `arr[i]` as an argument).
- `$arr` means `${arr[0]}`.
- Arena note: a sparse array can be done as a hash trie keyed by an int64 plus ordered iteration. A sorted vector or skip list is better for in-order `${arr[@]}`.

Associative arrays (`assoc.c` over `hashlib.c`):
- `declare -A` is required.
- Iteration order is hash-table order. Faithful byte-for-byte ordering is P2, but AI scripts may print `${!m[@]}`. Consider matching bash's hash function (FNV-1 32-bit in `hashlib.c`) plus bucket order if exact output parity matters.
- Keys are expanded without arithmetic. shopt `array_expand_once` (5.3; replaces assoc_expand_once) controls double expansion of subscripts.
- `${m[@]@K}` gives key/value output.

Namerefs (`declare -n`), P1:
- Assignments, unset (unless `unset -n`) and expansions go to the target.
- `for` loop variables can be namerefs.
- Circular reference detection.
- Test: `nameref.tests`.

Scoping:
- Dynamic scoping: `local` creates variables visible to callees.
- shopt `localvar_inherit` (a new local inherits the value and attributes of the previous scope) and `localvar_unset` (unsetting a local at the same scope leaves it "unset-but-local").
- `local -` saves and restores the `set` options.
- Temporary environment: `VAR=x cmd` applies only for that command. For special builtins and functions in POSIX mode the assignment persists; in bash mode, assignments before functions are local to the call.
- The temporary environment is a natural arena scope.

Special parameters: `$* $@ $# $? $- $$ $! $0 $_`. `$_` is the last argument of the previous command; at startup it is the shell path; it is also set in the environment of executed commands.

Shell variables (bashref 6664-7716; full list from the doc items):
- POSIX/Bourne: CDPATH HOME IFS MAIL MAILPATH OPTARG OPTIND PATH PS1 PS2.
- Bash:
  - BASH BASHOPTS BASHPID BASH_ALIASES BASH_ARGC BASH_ARGV BASH_ARGV0 BASH_CMDS BASH_COMMAND BASH_COMPAT BASH_ENV BASH_EXECUTION_STRING BASH_LINENO BASH_LOADABLES_PATH BASH_MONOSECONDS BASH_REMATCH BASH_SOURCE BASH_SUBSHELL BASH_TRAPSIG BASH_VERSINFO BASH_VERSION BASH_XTRACEFD
  - CHILD_MAX COLUMNS COMP_CWORD COMP_KEY COMP_LINE COMP_POINT COMP_TYPE COMP_WORDBREAKS COMP_WORDS COMPREPLY COPROC DIRSTACK EMACS ENV EPOCHREALTIME EPOCHSECONDS EUID EXECIGNORE FCEDIT FIGNORE FUNCNAME FUNCNEST GLOBIGNORE GLOBSORT GROUPS
  - HISTCMD HISTCONTROL HISTFILE HISTFILESIZE HISTIGNORE HISTSIZE HISTTIMEFORMAT HOSTFILE HOSTNAME HOSTTYPE IGNOREEOF INPUTRC INSIDE_EMACS LC_ALL LC_COLLATE LC_CTYPE LC_MESSAGES LC_NUMERIC LC_TIME LINENO LINES MACHTYPE MAILCHECK MAPFILE
  - OLDPWD OPTERR OSTYPE PIPESTATUS POSIXLY_CORRECT PPID PROMPT_COMMAND (may be an array) PROMPT_DIRTRIM PS0 PS3 PS4 PWD RANDOM READLINE_ARGUMENT READLINE_LINE READLINE_MARK READLINE_POINT REPLY SECONDS SHELL SHELLOPTS SHLVL SRANDOM TIMEFORMAT TMOUT TMPDIR UID

Dynamic variables (`variables.c` 1353-1880, INIT_DYNAMIC_VAR):
- SECONDS: assignable; counts from the assigned value.
- RANDOM: 0..32767. Assigning seeds it. The generator is the Park-Miller 16807 LCG with the result masked to 15 bits (`lib/sh/random.c` 62-112; there are details to verify if seed-for-seed parity is wanted). Subshells reseed.
- SRANDOM: 32-bit, from the system RNG.
- LINENO: assignable.
- BASHPID: read-only; differs in subshells, unlike `$$`.
- EPOCHSECONDS, EPOCHREALTIME: `%d.%06d` using the locale decimal point.
- BASH_MONOSECONDS.
- BASH_ARGV0: assigning it sets `$0`.
- BASH_COMMAND.
- BASH_SUBSHELL: increments per subshell nesting.
- HISTCMD.
- FUNCNAME: array; becomes active only while functions run; contains "main" at the bottom.
- BASH_SOURCE, BASH_LINENO: parallel arrays; `${BASH_SOURCE[0]}` is the idiom for the script's own path.
- GROUPS, DIRSTACK, BASH_ALIASES, BASH_CMDS (backed by the alias and hash tables).
- PIPESTATUS: array of the last foreground pipeline's statuses.
- BASH_REMATCH: set by `[[ =~ ]]`; index 0 is the whole match and 1..n the groups. POSIX ERE semantics with leftmost-longest matching, so a built-in ERE engine is needed with no libc regex on Windows. That is a significant subproject: P0 because `=~` is common.
- `_`, SHLVL (incremented at startup), PPID, UID/EUID (Windows needs a design), HOSTNAME, OSTYPE (MSYS2 reports "msys"; Git Bash also "msys"; native mode "cygwin"? Needs a decision).

======================================================================
4. ARITHMETIC (expr.c; P0)
======================================================================
- intmax_t (64-bit) with wraparound and no overflow checks. Recursion limit 1024 (expr.c 104).
- Errors:
  - "division by 0" (552, 917).
  - INTMAX_MIN / -1 gives INTMAX_MIN, and % gives 0 (565, 923-928).
  - "exponent less than 0" (979).
  - "invalid arithmetic base" (1595), "value too great for base" (1620).
  - "attempted assignment to non-variable" (529).
  - "syntax error: operand expected (error token is ...)".
- Precedence, highest first (bashref 8430-8470): id++ id--, ++id --id, unary - +, ! ~, **, * / %, + -, << >>, <= >= < >, == !=, &, ^, |, &&, ||, ?:, the assignment operators (= *= /= %= += -= <<= >>= &= ^= |=), and the comma operator.
- `**` is right-associative.
- `&&`, `||` and `?:` short-circuit: assignments are suppressed in the unevaluated branch (bash uses a "noeval" flag).
- Constants: decimal, octal (leading 0), hex (0x), and `base#n` with base 2..64 using the digits 0-9 a-z A-Z @ _ (for bases at most 36, case-insensitive).
- Variables are referenced by name, and their values are recursively evaluated as expressions (`a=b; b=3; $((a))` gives 3). Null or unset means 0.
- Array elements `a[i]` are allowed, including assoc keys.
- Contexts: `$(( ))`, `(( ))` (status 1 if the result is 0), `let`, `declare -i`, array subscripts, substring offsets, `for (( ))`, and the `[[ -eq ]]` family.
- Errors inside `$(( ))` abort the current command. In POSIX mode a non-interactive shell exits.

======================================================================
5. COMMANDS AND EXECUTION (execute_cmd.c; P0)
======================================================================
`execute_command_internal` (624) dispatches. Handlers:
- `execute_simple_command` 4468, `execute_pipeline` 2620, `execute_connection` 2811 (`; & && ||`).
- for 2990, arith-for 3219, select 3495 (P2), case 3644 (`;;`, `;&` fall-through, `;;&` continue testing), while/until 3796, if 3862, `(( ))` 3893, `[[ ]]` 4132 / 3975.
- Subshells 1576, coproc 2477 (P2; multiple coprocs are enabled in 5.3), `time` 1430 (TIMEFORMAT with `%[p][l]R/U/S/P`; 5.3 allows a precision of up to 6 digits).
- Functions 5181, builtins 4984, disk commands 5771, shell-script fallback when exec fails with ENOEXEC (6001), `shell_execve` 6126.

Simple command order (bashref 3629):
1. Save assignments and redirections.
2. Expand the words.
3. Perform the redirections.
4. Expand the assignments (tilde, parameter, command substitution, arithmetic, quote removal; no splitting or globbing).
5. If there is no command name, the assignments affect the shell, and the status is that of the last command substitution, or 0.

Command lookup (3682):
- If the name contains no slash, the order is: special builtins (POSIX mode first), functions, builtins, the hash table, then PATH search.
- Not found gives 127 and the message `bash: cmd: command not found`. The `command_not_found_handle` function is a hook (P2).
- Not executable gives 126.
- PATH empty or null means "." (5.3).
- Windows: PATHEXT, `.exe` appending, and shebang emulation for scripts (MSYS behavior), all at the platform layer.

Execution environment (3741-3846):
- Subshells inherit everything except traps (caught signals reset to their defaults; ignored stay ignored) and get a new BASHPID.
- `(...)`, pipeline elements (all but the last when lastpipe is set, job control is off and the shell is non-interactive), command substitution, process substitution and `&` all run in subshells.
- On Windows, subshells cannot fork. You must either:
  - interpret in-process with a fully copied or isolated shell state (an arena snapshot), running pipeline stages as threads or sequentially through temp buffers; or
  - re-exec self with serialized state.
- This is the single biggest Windows design issue. Note that `$BASHPID` must still differ, and `exit` inside `( )` must not exit the parent.

Exit status (3890):
- 0..255.
- 128+N for signal death.
- 126/127 as above.
- 2 for builtin misuse (EX_BADUSAGE), syntax errors give 2.
- A failing redirection gives 1 (and makes a non-interactive shell exit for special builtins in POSIX mode).
- `!` inverts.
- PIPESTATUS.
- `set -o pipefail`: the status is the rightmost non-zero.
- shopt lastpipe: the last stage runs in the current shell when job control is off (P1; `cmd | while read` idioms).

`set -e` (bashref 5929):
- Ignored in the condition of while/until/if, in all but the last command of an `&&`/`||` list, in all but the last pipeline element, and under `!`.
- Ignored for the whole body of a compound command or function called in an ignored context, even if `set -e` is run inside it.
- A compound command (other than a subshell) that fails because of an ignored failure does not trigger an exit.
- The ERR trap runs before exit.
- Subshells inherit `-e`. Command substitution clears it unless inherit_errexit or POSIX mode.
- Test: `set-e.tests`.

Functions:
- Positional parameters are saved and restored; FUNCNAME, BASH_SOURCE and BASH_LINENO are pushed.
- `return [n]` (outside a function or `source` it is an error).
- FUNCNEST limit.
- Recursion.
- Redirections on the function definition are applied at each call.
- `export -f` (env `BASH_FUNC_name%%=() {...}`), P2.
- `declare -f` requires the canonical pretty-printer (`print_cmd.c`), P1, because `declare -f`, `type f` and `set -x` output depend on it.

`set -x`: prints PS4 (expanded; default `+ `, the first character repeated per nesting level) and the command words quoted the way bash quotes them (`$'...'` for nonprintables, `'...'` when needed). P0 for debugging parity. Uses BASH_XTRACEFD.

Traps (`trap.c`; bashref 3939):
- `trap [-lpP] [action] sigspec...`. EXIT=0, DEBUG/ERR/RETURN are pseudo-signals (trap.h 40-43).
- EXIT runs at shell exit, including from a subshell's own trap set inside it.
- ERR follows the same conditions as `set -e`; it is inherited by functions only with `set -E` (errtrace).
- DEBUG runs before each simple command, for/case/select, `[[`, `((` and the first command of a function. With extdebug, a status of 1 skips the command and 2 simulates return. It is inherited only with `set -T` (functrace) or `declare -t`.
- RETURN runs after a function or `source`.
- BASH_TRAPSIG is new in 5.3.
- Windows signals: map Ctrl-C/Ctrl-Break to SIGINT. Most others are virtual (`kill -SIGTERM` on a child means TerminateProcess). P1 for EXIT and ERR, P2 for the others.

Redirections (bashref 3281-3610; `redir.c`):
- `[n]<w`, `[n]>w`, `[n]>|w` (overrides noclobber), `[n]>>w`, `&>w` and `>&w` (stdout and stderr), `&>>w`, `[n]<<w`, `[n]<<-w`, `[n]<<<w`, `[n]<&n`, `[n]>&n`, `[n]<&-`, `[n]>&-`, `[n]<&n-` and `[n]>&n-` (move), `[n]<>w`.
- `{varname}>w` allocates an fd of 10 or above and stores it in varname. The redirection persists, it is not undone after the command; it is closed with `{var}>&-`. shopt `varredir_close`.
- The order is left to right (`2>&1 >f` versus `>f 2>&1`).
- The word gets full expansion. If it splits or globs to more than one word, that is an "ambiguous redirect" (in POSIX mode no globbing or splitting).
- `exec` with only redirections makes them permanent.
- Redirections on compound commands and functions.
- Special filenames handled internally (lines 75-87): /dev/fd/N, /dev/stdin, /dev/stdout, /dev/stderr, /dev/tcp/host/port, /dev/udp/host/port (P2). /dev/null must map to NUL on Windows (P0).

======================================================================
6. BUILTINS (full list with synopses from $SHORT_DOC)
======================================================================
P0:
- `:` true false
- echo [-neE] (xpg_echo)
- printf [-v var] format [args]
  - %b %q %Q %(fmt)T, `%*d` widths, reusing the format for extra arguments, `'c` numeric arguments, `\c`.
  - Floats use a long double conversion (strtold), so a built-in float formatter is needed for CRT-free builds.
- cd [-L|[-P [-e]]] [-@] [dir] (CDPATH, OLDPWD, `cd -`); pwd [-LP]
- test / [ expr (`test.c`, POSIX 0-4 argument rules)
- read [-Eers] [-a array] [-d delim] [-i text] [-n|-N n] [-p prompt] [-t timeout] [-u fd] [name...] (IFS splitting, backslash processing without -r, the REPLY default)
- export [-fn] [name[=value]...] / -p; readonly [-aAf] [name...] / -p
- declare / typeset [-aAfFgiIlnrtux] [-p]; local [option] name[=value]...
- unset [-f] [-v] [-n] name...
- set [-abefhkmnptuvxBCEHPT] [-o opt] [--] [-] [args]; shopt [-pqsu] [-o] [optname...]
- shift [n]; exit [n]; return [n]; break [n]; continue [n]
- eval; source / . [-p path] filename [args] (-p is new in 5.3)
- exec [-cl] [-a name] [cmd [args]] [redirs]
- command [-pVv]; type [-afptP]; builtin
- trap [-Plp]; getopts optstring name [args]
- let; mapfile / readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C cb] [-c quantum] [array]
- wait [-fn] [-p var] [id...]
- hash [-lr] [-p path] [-dt]; umask [-p] [-S] [mode]
- alias [-p] / unalias [-a]

P1:
- kill [-s sig | -n num | -sig] pid|jobspec / -l
- jobs [-lnprs] / -x; fg, bg, disown [-h] [-ar] (job control: most is P2 on Windows)
- pushd [-n] [+N|-N|dir], popd, dirs [-clpv]
- caller [expr]; times; enable [-a] [-dnps] [-f file] (no loadables); help [-dms] (doc strings needed: write our own text, not GPL text)
- history [-c] [-d off] [n] / -anrw / -ps
- fc [-e ename] [-lnr] / -s
- ulimit (mostly a stub on Windows)

P2:
- bind (readline), complete / compgen [-V var] / compopt (programmable completion)
- suspend; logout
- coproc (keyword)

Special builtins (POSIX; bashref 6621): break : . continue eval exec exit export readonly return set shift trap unset. In POSIX mode, errors in them exit a non-interactive shell, and assignments before them persist.

======================================================================
7. OPTIONS
======================================================================
`set -o` (`set.def` o_options):
- allexport braceexpand emacs errexit errtrace functrace hashall histexpand history ignoreeof interactive-comments keyword monitor noclobber noexec noglob nolog notify nounset onecmd physical pipefail posix privileged verbose vi xtrace.
- Single-letter forms: a B (braceexpand) e E (errtrace) T h H k m n f u C P t v x p b.
- P0: errexit nounset pipefail xtrace noglob noclobber posix verbose noexec allexport.

`shopt` (`shopt.def` shopt_vars; 5.3):
- array_expand_once assoc_expand_once autocd bash_source_fullpath(5.3) cdable_vars cdspell checkhash checkjobs checkwinsize cmdhist compat31 compat32 compat40 compat41 compat42 compat43 compat44 complete_fullquote direxpand dirspell dotglob execfail expand_aliases extdebug extglob extquote failglob force_fignore globasciiranges globskipdots globstar gnu_errfmt histappend histreedit histverify hostcomplete huponexit inherit_errexit interactive_comments lastpipe lithist localvar_inherit localvar_unset login_shell mailwarn no_empty_cmd_completion nocaseglob nocasematch noexpand_translation nullglob patsub_replacement progcomp progcomp_alias promptvars restricted_shell shift_verbose sourcepath syslog_history varredir_close xpg_echo.
- P0: extglob nullglob failglob dotglob globstar nocaseglob nocasematch expand_aliases lastpipe inherit_errexit patsub_replacement globskipdots globasciiranges sourcepath.
- Recommendation: add a new shopt for the path-style choice (MSYS-style versus native paths). It should be reflected in BASHOPTS.

`$-` flags, SHELLOPTS and BASHOPTS must be kept in sync. Shell compatibility levels (BASH_COMPAT, compat3x/4x; bashref 9630) are P2.

======================================================================
8. POSIX MODE (bashref 9121-9630; P1, but P0 if replacing ash as `sh`)
======================================================================
POSIX mode is entered by invoking the shell as `sh`, by `--posix`, or by `set -o posix`. About 85 itemized differences. Key ones:
- POSIXLY_CORRECT is set.
- ENV is read for interactive shells.
- Aliases are always enabled.
- Aliases are expanded when a `$( )` is first parsed.
- `time` alone is a simple command; `time -x` is not the keyword.
- Redirection words are not globbed or split.
- Function names may not be special builtin names.
- Tilde expansion only in assignments before a command name.
- Command substitutions do not set `$?`.
- Special builtins are found before functions.
- Errors in special builtins, assignment errors, `${x?}`, arithmetic syntax errors, `.` file-not-found and syntax errors in a sourced file all exit a non-interactive shell.
- Assignments before special builtins persist.
- inherit_errexit is on.
- `.` does not search the current directory.
- `echo`, `kill -l`, `trap` and `export -p` output formats differ.
- `fc` uses the `ed` editor.
- `printf` uses strtod.
- `test` `<` and `>` use locale collation (5.3).
- Word splitting is byte-oriented.
- `!` inside `"..."` does not trigger history expansion.

To look identical to busybox ash when invoked as `sh`/`ash`, the planner should consult busybox behavior separately. Bash's POSIX mode is not ash: messages and `$0` formatting differ.

======================================================================
9. MISCELLANEOUS SEMANTICS
======================================================================
- Startup (bashref 7740-8064), P1:
  - `-c cmd [name [args]]`, `-s`, `-i`, `-l` / `--login`, `--norc`, `--noprofile`, `--rcfile`, `--posix`, `-O shopt`, `+O`, `--`.
  - BASH_ENV is used for non-interactive shells.
  - Files: `~/.bashrc`, `/etc/profile`, `~/.bash_profile`, `~/.bash_login`, `~/.profile`.
  - `$0` behavior.
  - SHLVL.
  - Error message formats: `bash: line N: ...` for `-c`, `script: line N: ...` for scripts. P0 for parity, since AI tools parse these.
- Restricted shell (9047): P2.
- Directory stack: P1.
- History expansion `!!`, `!$`, `!n`, `^a^b`, modifiers `:h :t :r :e :p :s :g :&`: interactive only, P2 (`lib/readline/histexpand.c` is the semantics reference).
- `select`, `coproc`, `bind`, `complete`: P2.
- Locale and multibyte: UTF-8 assumed. `${#x}`, substrings, casemod and globs are character-aware. On Windows, decide on UTF-8 internally with conversion to and from UTF-16 at the platform layer (command lines, environment, filenames, console I/O).

======================================================================
10. SUGGESTED PHASING
======================================================================
- Phase 1 (P0 core):
  - Lexer and parser for simple commands, pipelines, lists, all compound commands, functions, here-docs, `$( )`, backquotes, `${ }`, `(( ))`, `[[ ]]`.
  - The full expansion pipeline without process substitution.
  - Arithmetic.
  - Indexed and associative arrays.
  - Local scoping.
  - Redirections including `{var}` and /dev/null.
  - Exit status, set -e/-u/-x/-o pipefail.
  - The P0 builtins and the ERE engine for `=~`.
  - Error message parity.
  - Virtual OS layer first, so all of this is testable deterministically.
- Phase 2:
  - Process substitution, lastpipe, traps (EXIT/ERR/DEBUG/RETURN), namerefs.
  - `printf` and `read` corners, `mapfile`.
  - The `declare -f` pretty-printer.
  - POSIX mode, startup files, the path-mode shopt.
  - Aliases, extglob parse-time effects, globstar/GLOBSORT.
- Phase 3:
  - Interactive features: line editor, history, history expansion, completion (complete/compgen), prompts, job control.
  - select, coproc, dirstack, fc, bind, help text.
