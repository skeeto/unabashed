# Open issues

Every known bug, gap and deviation from bash that is not fixed yet, in
one place, each assigned to the milestone that owns it. The milestone
notes (m3a.md, m3b.md, m4.md, ...) keep the details; this list is what
gets checked. Rules:

- A milestone is not closed until every item assigned to it is fixed
  (and removed from here) or deliberately moved to a later milestone
  with a reason.
- Anything found in passing (reviews, fuzzing, oracle diffs) that is
  not fixed on the spot is added here, not only to a milestone's notes.
- "Accepted" items are deliberate deviations; they stay listed so they
  are not rediscovered as bugs.


## Next (before M5a starts)

- **Ctrl-C into a pipeline races with SIGPIPE** (t/win32/005, fails
  intermittently under load): a stage notices Ctrl-C only at its next
  safe point, so it can die of the broken pipe first ("130 141 130"
  instead of "130 130 130"). Bash's terminal signals every process at
  once. A level with a pending SIGINT should die of it before a write
  error. (m3b.md)
- **Exponential back-reference matching** in regex.cpp:
  `((a|aa)*)\1\1c` against 40 a's takes about a minute (glibc: 300 a's
  in a second). `condregex` passes no budget to `rxexec`; needs a step
  budget or a better algorithm. (m4.md, review)
- **Spec 213-trailing-backslash** has failed since M2 (the "line -1"
  quirk). Fix it or record why it is accepted.
- **Fuzz the M4 changes**: the run, chaos and isolate campaigns have not
  seen namerefs, regex.cpp, glob.cpp, float.cpp or mapfile. Rerun them
  (tools/fuzz) and triage.
- **Win11 keyboard Ctrl-C check with winctl** (typed into conhost),
  skipped in M3b because the desktop was busy; passed on XP.


## M5a (Windows paths and spawning)

- **Inherited descriptors above 2 are invisible**:
  `unabashed -c 'echo hi >&3' 3>&1` fails with "Bad file descriptor"
  on every platform; sh_main imports only 0-2. Import the open
  descriptors at startup (POSIX: probe; Win32: lpReserved2/MSVCRT
  handles). Real scripts hit this. (m4.md)
- **Programs cannot open `/dev/fd/N` on Win32**, so `diff <(a) <(b)`
  fails there: pass a named pipe path bridged by a pump
  (`os_pipe(named)`, ConnectNamedPipe in a helper). (m3a.md)
- **Time zones**: TZ unset or unexported means UTC, on Windows too; use
  the system zone. Zone names glibc reads from zoneinfo files are read
  as POSIX strings (DST before 2007 can differ). (m4.md)


## M5b (real background processes)

- A background vproc still running when the shell exits is waited for;
  bash leaves it running. (m3a.md)
- `exec prog < <(...)` forks while the procsub is alive, so the
  program's pid differs from bash's. (m3a.md)
- POSIX: if another process reads the same pipe as a shell task, a read
  after poll can block the whole shell until data arrives. (m3a.md)


## M6 (editor)

- `read -p/-s/-e/-i` do nothing yet (need the terminal layer). (m4.md)
- history and histexpand oracles.


## M7 (job control and fidelity)

- **UTF-8 character classes and case folding are ASCII-only** in
  pattern.cpp, glob.cpp and regex.cpp: `[[:alpha:]]` does not match é,
  where bash (glibc) does. (m4.md)
- POSIX: a terminal SIGINT is the process's own signal (no `si_code`
  check), so a trapped INT at top level does not reach a running
  subshell level. (m3b.md)
- Win32: a Ctrl-C from GenerateConsoleCtrlEvent does not end a console
  read in progress; Ctrl-C during CreateProcess counts against the
  shell even if the program handled it. (m3b.md)
- Globbing does not check for Ctrl-C between directory entries.
- Job control: `set -m`, fg/bg, stopped jobs, suspend (jobs oracle).
- dirstack (pushd/popd/dirs), ulimit, help (dirstack, redir, vredir
  oracles).
- redir12 line numbers: bash cites the line where the parser finished
  the top-level command; we cite the last simple command's. (m4.md)
- `let` with an unterminated quote in an associative key (without
  assoc_expand_once) differs. (m4.md)
- POSIX mode does not model source_searches_cwd. (m4.md)
- In a mapfile callback, `return 3 0 x` at top level gives another
  message and does not abandon the rest of the line. (m4.md)
- glibc's 4 KiB stdio flush before a diagnostic is not emulated (bash
  prints an error mid-line after 4 KiB without a newline). (m4.md)
- GLOBIGNORE splitting skips quotes and groups but not `$(...)`.
- Namerefs: assigning through a nameref into an older scope whose
  variable is shadowed sets the visible one (bash sets that scope's);
  bash's silent `typeset +r` after a failed `typeset r=x` on a readonly
  nameref (nameref oracle). (m4.md)
- `${!c[@]:N}` on an unset `c` expands to nothing; bash reports an
  invalid indirect expansion. `read -t` of 2^31 seconds or more is
  rejected; bash wraps. (review)
- A `~user` tilde prefix is left as is; bash looks the user's home
  up (`~root/x` is `/root/x`), in words and in BASH_ENV. (review)
- exec run by eval, a function or `${ ...; }` inside a subshell is
  undone when the enclosing simple command ends:
  `( eval 'exec 2>/dev/null'; nosuch )` reports the error, bash does
  not. The command's unwind pops exec's keep records. (fuzz triage)
- `${ prog & }`, a program started straight in the background (not
  a vproc job, procsub or coprocess), holds up the substitution until
  it closes its output: its pump has no owner to check, so endcapture
  joins it; bash reads the substitution's file at once. In bash that
  file is on the lowest free descriptor from 3, so
  `${ exec 3>/dev/null; echo a; }` is empty there, and fd 3 is closed
  after it. A fatal error
  in `${ ...; }` restores stdout before the EXIT trap, as `bash -c`
  and eval do; reading a script file, bash leaves it on the
  substitution and the trap's output is lost. (fuzz triage)


## M8 (performance)

- Launcher fibers: an external pipeline stage whose words contain
  `$(...)` or a procsub runs as a full vproc. (m3a.md)
- Read-ahead for foreign pipes on Windows (Linux peeks with tee(2)).
- No compile cache for `=~` (bash has none either).


## Test infrastructure

- The chaos fuzzer cannot tell two writers sharing one pipe or
  capture from sequential writers, so `(false)|echo >(echo)|cat`,
  `y=$(echo >(echo deep))` and `cat <(echo x) >(tr a b <<<f) | sh`
  (the reader sees x and f in either order) are false positives.
  Telling them apart needs to know, at each write, whether the
  previous writer may still write: whether it is alive and not waiting
  (through joins, pumps or vos waits) on the current one. Its
  exemptions are coarse (a lone `&` exempts an input), so background
  jobs and file races get little coverage. (m3a.md)
- Chaos replays at -O0 agree on some inputs (a `\<NUL>&&` background
  job plus overflowing output) where the AFL build at -O1 diverged;
  such inputs are now exempt, but the cause of the -O level mattering
  was not traced.
- The test harness runs out of memory with more than about a dozen
  live vprocs or arrays of more than about 50,000 elements. (m3a.md,
  m4.md)
- The oracle Docker image lacks the hexdump fake (rebuild before
  recording a spec that uses it). (m4.md)
- `/tmp/bash-5.3` in WSL vanishes when WSL restarts; bench/run needs
  it rebuilt from the tarball.


## Accepted deviations

- dollars oracle: bash drops a lone DEL from `${*:1}` with IFS='';
  not copied (a bash bug).
- printf7.sub line 88 and other `.right` lines that disagree with real
  bash 5.3: we follow real bash.
- regex.cpp: back-references combined with repetition can give other
  registers than glibc (approximation of glibc's search); patterns
  past 2^20 nodes or 500 nested groups are "Memory exhausted".
- pattern.cpp: a malformed bracket between two stars matches where
  bash's shortcut gives up; more than 10 divergent brackets in one
  sequence are not forked.
- No Big5 or other non-UTF-8 multibyte locale (glob-test's zh_TW
  part).
- Windows: ctime is reported as mtime and blocks estimated from size;
  a program that exits nonzero after Ctrl-C counts as killed by it;
  Ctrl-Break is SIGQUIT; death by SIGINT exits with 0xC000013A.
- Trap actions that signal themselves stop at ten nested levels
  (status 139); bash recurses until it crashes.
