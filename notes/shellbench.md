# Shellbench against dash (2026-11-08)

[shellbench](https://github.com/shellspec/shellbench) run with dash,
bash and unabashed (`cc -std=c++20 -fno-exceptions -O2
main_posix.cpp`, WSL Debian 13) showed ours beating bash on every
benchmark but trailing dash on most builtin-only ones, and `eval.sh:
direct assign` once hung.


## The hang

Not ours. shellbench's bench() waits for two SIGHUPs, one from the
benchmark shell when it is ready and one from the stopper process, and
counts them in a trap. Ordinary signals do not queue: when both arrive
before the trap runs, the count stops at 1 and bench() spins forever.
It is a race against any shell; it struck once and could not be made
to happen again. The fix, for shellbench, is a second signal: the
stopper sends USR1 and bench() waits for one of each. The benchmarks
below ran with that patch.


## Method

- shellbench `-t 2 -w 1`, three shells per run in one table, so that
  each row compares runs minutes apart on the same machine.
- Fixed-count loops (`c=1000000; while ((c--)); do BODY; done`), best
  of five user times, for changes too small for shellbench's noise.
- A sampling profiler: SIGPROF from `timer_create(CLOCK_MONOTONIC)`
  every 200 µs records the interrupted program counter, linked into a
  static `-O2 -g -DPROFILE` build and symbolized with addr2line, so
  inlined functions and libc show by name. gprof's flat profile was
  unusable on WSL (a third of the samples, and mcount distorts short
  functions), but its call graph from an `-O1 -fno-inline -pg` build
  gives exact calls per iteration and per caller.
- Every change compared against bash 5.3 on scripts aimed at it, then
  the unit tests, the exec, parse and winvos specs on the vos, and a
  new recorded spec where the change opened a new path.

Profiling found a real bug first: with an EXIT trap set, the shell
caught SIGPROF (bash leaves it alone, `#if 0` in its list of fatal
signals), so the profiler's timer ran the trap; and resetting signals
to their defaults also reset a SIGPROF the shell had never changed,
killing a profiled shell. Both fixed (4009e0d, a83f9b1).


## Changes

Each is a commit of its own, in the order made. Most give a word that
needs none of the general expander (a fresh X, flag bytes per
character, field splitting, a glob check) a shortcut that declines
anything unusual to the full path.

| Commit  | Change | Measured |
|---------|--------|----------|
| e952479 | find the end of a plain `${...}` without lexing it | `${x:-}` 1.42M to 2.41M/s |
| 67771c3 | match a pattern without wildcards by comparing it | case 2.27M to 3.01M/s |
| fef1723 | a word of quoted literal text (`"test"`) skips the expander | `[ "$s" = s ]` 0.47 to 0.36 s per million |
| 62bb538 | size a function body by counting, not by a trial copy | `foo() { :; }` 0.193 to 0.164 s per million |
| 8f6ddf6 | assignments to names starting with `_` settled at the first byte | `__count` skipped ~20 comparisons |
| 0813ec7 | arithmetic text whose only expansions are `$name` | `var=$((var+1))` 0.305 to 0.271 s per million |
| 42e89e0 | literal patterns with stars at their ends found by byte search | `${s//:/\|}` 0.636 to 0.324 s per million |
| 69d1a5c | IFS's table entry looked up once | 4 to 7% on parameter rows |
| 6f1c261 | short strings compared without calling memcmp | 1 to 3% on most rows |
| e7549de | `${x:-word}` and its kin with a plain word | `${no_such_variable:-}` 0.229 to 0.125 s per million |
| 50c3e02 | backslash escapes on the quoted-literal shortcut | `printf "test\n"` 1.65M to 2.05M/s |
| c79b67b | skip the reserved word table for most words | `eval "var=1"` 1.17M to 1.20M/s |
| 58ecfc2 | copy parse trees without zeroing each node first | define function 3.25M to 3.42M/s |
| e4393f5 | load the pending-signal words before exchanging them | 1 to 3% on most rows |
| cc4cf3d | a lone parameter in a word never split (assignment, case word) | `v=$s` 0.35 to 0.30 s per 2M |
| 3eb3969 | pattern words of plain text taken as they are | `v=${s#*:}` 0.48 to 0.44 s per 2M |
| 4247761 | a word that is exactly `$((...))` evaluated directly | every row 20 to 30% |
| 3f7fbca | an assigned variable's table entry found once, not twice | assign variable 4.7M to 5.2M/s |
| 00eabf9 | the builtin registry searched once per command, not twice | func 3.16M to 3.28M/s |

The last one matters most because shellbench counts iterations with
`__count=$(($__count+1))`, which costs more than most of the bodies
it counts: about 200 of the 277 ns of a `:` iteration before it. The
counter went through the expander, scanned the word for the end of
its `$((`, scanned it again to tell arithmetic from a command
substitution, then expanded the body.

cc4cf3d first cost the counter 1 to 3%, which showed on every row: its
check made expandstring too big to inline, and every assignment value
paid a failed call to simplevalue. A test of the first bytes
(maybesimple) recovered part of it, and 4247761 took the counter off
that path entirely.

The last two came from a profile of eval.sh, the one file where dash
still led: about a fifth of each iteration went to name lookups, and
each assignment walked the variable table twice and each command the
builtin registry twice.

Results, iterations per second with `-t 2 -w 1`; ub-old is c05e24a,
the build first benchmarked, and ub-final is 00eabf9:

| Benchmark | dash | ub-old | ub-final | final/dash |
|-----------|-----:|-------:|---------:|-----------:|
| assign.sh: positional params | 2.70M | 2.01M | 3.42M | 1.27 |
| assign.sh: variable | 3.61M | 3.14M | 5.00M | 1.39 |
| assign.sh: local var | 3.64M | 3.14M | 5.06M | 1.39 |
| assign.sh: local var (typeset) | – | 3.14M | 5.02M |  |
| cmp.sh: [ ] | 1.41M | 1.53M | 2.34M | 1.66 |
| cmp.sh: [[ ]] | – | 1.96M | 3.43M |  |
| cmp.sh: case | 3.57M | 2.26M | 4.62M | 1.29 |
| count.sh: posix | 2.52M | 1.92M | 3.28M | 1.30 |
| count.sh: typeset -i | – | 1.79M | 2.93M |  |
| count.sh: increment | – | 2.12M | 2.94M |  |
| eval.sh: direct assign | 2.10M | 1.59M | 2.14M | 1.02 |
| eval.sh: eval assign | 1.23M | 0.84M | 1.21M | 0.98 |
| eval.sh: command subs | 0.01M | 0.64M | 0.75M | 124.85 |
| func.sh: no func | 3.23M | 2.69M | 4.33M | 1.34 |
| func.sh: func | 2.33M | 2.17M | 3.16M | 1.35 |
| null.sh: assign variable | 3.50M | 3.10M | 5.02M | 1.43 |
| null.sh: define function | 3.81M | 2.54M | 4.21M | 1.10 |
| null.sh: undefined variable | 3.46M | 1.44M | 4.93M | 1.42 |
| null.sh: : command | 3.56M | 3.02M | 4.69M | 1.32 |
| output.sh: echo | 2.06M | 1.64M | 2.54M | 1.24 |
| output.sh: printf | 2.02M | 1.54M | 2.26M | 1.12 |
| stringop1.sh: string length | 2.99M | 1.94M | 3.52M | 1.17 |
| stringop2.sh: substr 1 builtin | – | 1.57M | 3.07M |  |
| stringop2.sh: substr 2 builtin | – | 1.53M | 2.68M |  |
| stringop3.sh: str remove ^ shortest builtin | 2.78M | 1.38M | 3.35M | 1.20 |
| stringop3.sh: str remove ^ longest builtin | 2.43M | 1.35M | 3.32M | 1.37 |
| stringop3.sh: str remove $ shortest builtin | 2.86M | 1.34M | 3.30M | 1.15 |
| stringop3.sh: str remove $ longest builtin | 2.87M | 1.32M | 3.28M | 1.14 |
| stringop4.sh: str subst one builtin | – | 1.23M | 2.80M |  |
| stringop4.sh: str subst all builtin | – | 1.00M | 2.64M |  |
| stringop4.sh: str subst front builtin | – | 1.06M | 2.49M |  |
| stringop4.sh: str subst back builtin | – | 1.03M | 2.48M |  |
| subshell.sh: no subshell | 3.49M | 2.89M | 4.62M | 1.32 |
| subshell.sh: brace | 3.48M | 2.89M | 4.61M | 1.32 |
| subshell.sh: subshell | 0.01M | 1.82M | 2.37M | 273.22 |
| subshell.sh: command subs | 0.01M | 0.93M | 1.10M | 129.20 |

Rows that run a program (`echo | cut`, here-documents into `sed`,
an external command) are bound by fork and exec and left out: ours
run 1.0 to 1.8 times dash's rate there, the same before and after.
eval.sh moves with the machine: dash ran 2.10M to 2.33M direct
assigns across runs, and the two shells stay within 3% of each other.


## Tried and dropped

- Not zeroing a reused store block for a function body (every object
  in it is initialized as it is allocated). A process image must be
  closed (m5b.md), and the stale words left in a block's slack point
  outside it; the vos's conservative scan caught it at once.
- Keeping a redefined function's old copy when its body is the same:
  the source tree's address can be reused by another parse, so only a
  full comparison would be safe, which costs about what the copy does.
- A cache for IFS's value: assignments through temporary bindings and
  local unwinding do not all pass through varchanged. Caching IFS's
  table entry (69d1a5c) is safe and gets most of the gain.


## What remains

- No single hot spot is left in the builtin rows. An iteration
  spreads over the simple-command path (runsimple1 alone is ~6%), name
  lookups (now one per use of a name, mostly hits), setting `$_`, and
  polling for signals. Storing each table entry's hash beside its name
  would spare only the comparisons with names of the same length,
  worth about 2%. The builtin registry is a binary search of about
  seven string comparisons; a hash would save perhaps 1%.
- eval builds a fresh lexer (2.7 KB, zeroed) and parses its text every
  time; that and the function definition in each iteration are most
  of `eval assign`, which is level with dash. Nothing in its profile
  is over 6%.
- Rows that run a program are bound by fork and exec and stay level
  with dash. Subshells and command substitutions run in vprocs, two
  orders of magnitude faster than dash's forks.
- In POSIX mode an arithmetic error ends a `( )` subshell with 127,
  where bash uses 1 (issues.md, M9).
