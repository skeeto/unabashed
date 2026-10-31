# t/parse: M1 parser and printer goldens

Original scripts that exercise the lexer, parser, canonical printer and
syntax-error reporting. Each `NNN-topic.sh` has a `NNN-topic.spec`
recorded from real bash 5.3 by `tools/record` (see `tools/README.md`
for the run environment and spec format). Scripts run as
`bash -c SCRIPT bash`, so diagnostics read `bash: -c: line N: ...`.

## Command constraint

The M1 shell executes almost nothing, so the top level of every script
uses only:

- function definitions;
- `declare -f`, with or without names;
- `echo` with plain arguments;
- `exit N`;
- `alias name='value'`;
- `shopt -s expand_aliases` and `shopt -s extglob`;
- deliberately malformed syntax.

Function bodies are only printed, never called, so they may hold any
bash syntax. The grammar is covered by defining functions and printing
them with `declare -f`.

## Layout

- `001`–`146`: valid scripts. Functions and simple commands,
  pipelines and lists, every compound command and their nesting,
  redirections, heredocs (`<<-`, quoted delimiters, several per line,
  in pipelines, loops and command substitutions, end-of-file warning),
  words and quoting, parameter and arithmetic expansion, command
  substitution, backquotes, `${ ;}` and `${| ;}`, process substitution,
  line continuations, aliases, extglob, and a few `echo`/`exit`
  interleavings that pin the order of stdout against stderr.
- `147`–`219` (`*-err-*`): one syntax error each: unexpected tokens,
  unexpected end of file, unterminated quotes and substitutions,
  `[[ ]]` errors, bad loop names, errors on later lines after output,
  errors inside function bodies and command substitutions, and a few
  edge cases where bash warns or never reaches the error.

## Re-recording

From the repository root:

    sh tools/record t/parse/*.sh

Write new scripts from scratch; never copy from bash's own tests.
