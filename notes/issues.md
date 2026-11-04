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
<<<<<<< HEAD
- **lpReserved2 slot width unverified** (step 8): `encodecrtfds` and
  `decodecrtfds` are only checked against the documented layout. Check
  with real msvcrt and UCRT children, x64 to i686 and back, that the
  width follows the child's PE32/PE32+ and that 0-2 entries agree
  with hStd*. (m5a.md, step 2)
- **splitcygwin gaps** (step 5's msys re-split): it assumes Cygwin's
  globbing is on (with `MSYS`/`CYGWIN=noglob` Cygwin keeps the quotes);
  the program name, a non-ASCII byte before `:` as a drive word, and
  tilde with an existing HOME were not recorded; the caller still has
  to glob, read response files (dying on the 11th) and map an empty
  command line to the program's path. (m5a.md, step 2)
- **pesniff ignores** delay-load imports, the loader's rounding of
  PointerToRawData to 512 bytes and DLL names over 63 bytes; reads at
  most 1024 section headers. None matters for the binaries tried.
  (m5a.md, step 2)
- **t/msys baseline is red**: unabashed.exe passes 1 of 60 (057); steps
  4-9 turn it green (m5a.md, "Step 3"). Exit criterion 1.
- **The agent-batch gate cannot be met as written**: with process
  creation at about 17 ms, unabashed with Git Bash's externals is about
  1.25x Git Bash per command and at most 2.6x with free externals, not
  5x (m5a.md, "Step 3"). The user picks the new gate (shell-only time,
  one-shell batch, or vos instruction counts). The floor was measured
  from a busybox driver; Claude Code spawns from node, which step 10's
  CRT-free driver should stand in for.
- **t/msys and bench/agent are not write-confined**: MSYS2 programs
  cannot start at Low integrity, so they run under `sandbox -L` (job,
  limits, timeout, restricted token, medium integrity). Only the corpus
  rules (tools/README.md) keep writes in WORKDIR. Untried: an
  AppContainer, or the sandbox pre-creating the msys
  `\BaseNamedObjects\msys-2.0S5-<key>` directory with a Low label.
- **Accepted: `2>nul`**: Git Bash creates a file named `nul`; a native
  shell opens the device. Not in the t/msys corpus.
- **Accepted: BatBadBut quoting**: arguments to .bat/.cmd targets are
  quoted for cmd.exe's safety, unlike Cygwin's. Not in the corpus.
- **Accepted: BASH_VERSION and friends** differ from Git Bash's
  (`5.3.15(1)-release`); the corpus prints only BASH_VERSINFO[5].
- **Accepted: `type cygpath`** says builtin in msys mode (m5a.md,
  Decisions). Not in the corpus, nor is cygpath's usage text (GPL).


## M5b (real background processes)

- **Win32 Ctrl-C a starting program can still miss** (m3b.md, "Programs
  that miss a Ctrl-C"): unnoticed are one over 100 ms old and still
  not attached; the real program of a wrapper that starts it more than
  50 ms after the event; one in a job of over 64 live processes (only
  64 are listed; such a program is not watched either); and every
  descendant when the program's job could not be made (it is taken to
  have had it, with all it started). A process that came and went
  between listing a job and binding its port counts as missed even if
  it was spared or ended. Only the Windows tests cover it: vos has no
  group signals (`os_siggroup` is 0), so the core's `WMISSED` handling
  (the fallback when a missed program cannot be ended) has no tests.exe
  coverage; untested on Windows 8 and 10 (disassociating a job's port
  may fail there: handled, later messages are ignored); not run on the
  XP laptop. A 32-bit shell reading a 64-bit child's PEB (console
  image, Ctrl-C ignored, initializing, same image) gets "unknown".
- **Win32: a foreign program's Ctrl-C handler that starts a program is
  taken for a wrapper**: a young program, attached and childless at a
  Ctrl-C, is watched for 50 ms, since w64devkit's aliases ignore Ctrl-C
  with a handler function and only then start busybox, which missed it.
  A program that handles Ctrl-C by starting another (another shell's
  trap, a cleanup tool) looks the same: what it starts within 50 ms is
  ended, or, if it exits normally first, the parent takes SIGINT. bash
  would run the cleanup and go on. A child running this shell's own
  image is exempt (it judges its own children), as are programs
  started with Ctrl-C ignored; the image path is compared, so another
  copy or build of unabashed is foreign.
- **Win32: every program runs in a job of its own** (Windows 8 and
  later, see judge): `IsProcessInJob` is always true;
  `QueryInformationJobObject(NULL)` describes that job, not the shell's,
  so runtimes that size themselves by the job's memory limit (.NET) see
  none; breakaway is allowed only as the shell's own job allows it
  (`JOB_OBJECT_LIMIT_BREAKAWAY_OK`, or always when the shell is in no
  job), and a descendant that breaks away is no longer judged; under a
  shell job with silent breakaway, children that were in no job are
  now in ours, and their children stay in it.
- **Win32 Ctrl-C before SIGPIPE, single-task shell** (unobserved): a
  shell process writing to a native program that dies of Ctrl-C can
  see the broken pipe before its own control handler thread runs, and
  die of SIGPIPE (only PIPESTATUS differs; the parent still acts on the
  failed program stage). Waiting for a late Ctrl-C before dying of
  SIGPIPE would delay every `yes`-style loop into `head`. Moved here
  from Next: never seen, and its only fix costs throughput; revisit
  with the Ctrl-C work above. (m3b.md)

- A background vproc still running when the shell exits is waited for;
  bash leaves it running. (m3a.md)
- `exec prog < <(...)` forks while the procsub is alive, so the
  program's pid differs from bash's. (m3a.md)
- POSIX: if another process reads the same pipe as a shell task, a read
  after poll can block the whole shell until data arrives. (m3a.md)


## M6 (editor)

- `read -p/-s/-e/-i` do nothing yet (need the terminal layer). (m4.md)
- An interactive shell started with Ctrl-C ignored (Win32: the
  inherited ConsoleFlags bit, from a parent's SetConsoleCtrlHandler(0,
  1); POSIX: SIGINT ignored on entry) never sees Ctrl-C, so typing it
  does nothing. bash catches SIGINT whenever it is interactive, even if
  ignored on entry (its programs still start ignoring it). Win32 needs
  SetConsoleCtrlHandler(0, 0) at startup when interactive; anything
  run from Claude Code's shell starts that way. (m3b.md)
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
- `[[ =~ ]]` does not check for Ctrl-C (signals act only at safe points
  between commands). Some patterns run for minutes or more here and in
  glibc alike: `(.{,2}\>a*{1,})*` on `a` (the walk, no
  back-references), and compiling
  `(()?((\b){1,2}((\')?(){0,1}(()*a*b*|[ab]?))*))+` (found by an
  old-against-new comparison of the regex.cpp memo; both verified to
  hang bash 5.3 past 60 s too), and `(\b$){1,}*\>\'` on `x\xFFcAB`
  (regex_diff --random 4, case 2936180: glibc and ours both still
  running after 15 s in a few MB; --random has no per-case timeout,
  so that seed stops there). (m4.md)
- regex.cpp differs from glibc on back-reference patterns without
  repetition where `rxlongest`'s first pass records close nodes reached
  through back-references glibc finds no span for, which then hide the
  span of a later back-reference: `[^a]($|)\1($)\2` on a space (glibc
  (0,1)(1,1)(1,1), ours no match), and `($|)($)($|)\1\2$\`$($)` on an
  empty subject (glibc matches, ours not). Both differ on master too;
  2 in 400k generated cases. (m4.md)
- regex.cpp's back-reference span records (`RxEntry`, kept once each
  per start) are not bounded by the memo's 256 MiB: with two groups a
  search records a span for most pairs of position and length, which
  costs time (`(a*)(a*)\1\2x` on 600 a's: 10.6 s on master, 15.0 s)
  and memory. (m4.md)
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
- `${ prog & }`, a program started straight in the background (not
  a vproc job, procsub or coprocess), holds up the substitution until
  it closes its output: its pump has no owner to check, so endcapture
  joins it; bash reads the substitution's file at once. In bash that
  file is on the lowest free descriptor from 3, so
  `${ exec 3>/dev/null; echo a; }` is empty there, and fd 3 is closed
  after it. (fuzz triage)
- Eval text that ends inside `$(...)` on a backslash: bash's
  "unexpected EOF" cites the line after ours in some cases
  (`echo $(echo \`, `\\`, `a\`, `$\`, `$(\` report the eval's line + 2,
  we say + 1) but not others (`\\\`, `\\\\` are + 1 in both). Follows
  from how bash's shell_getc pads a final backslash and counts reads
  at end of input; not modelled. (review)
- `f & kill -9 $!; wait` prints no "Killed" report; bash reports the
  job when wait collects it. (`sleep 1` instead of wait works, spec
  636.) (review)
- DEBUG before a background simple command (`true &`) or each simple
  stage of a background pipeline (`a | b &`) does not run: bash runs
  it in the parent before forking, as for a foreground pipeline's
  stages (`stagedebug`, spec 858), so it runs even without functrace.
  Here the job's vproc drops the trap. (review)


## M8 (performance)

- Launcher fibers: an external pipeline stage whose words contain
  `$(...)`, a procsub, a funsub or `@P` runs as a full vproc. (m3a.md)
  The other external stages before the last expand synchronously
  before the stages to their right start, so a comsub `runscode`
  cannot see (`i='a[$(cmd)]'; x | prog ${a[i]} | y`, from the
  variable's value) hangs if it waits on a later stage, as through a
  FIFO the later stage writes; bash runs it concurrently. A launcher
  fiber per such stage fixes this too, and would let a stage with
  `command_not_found_handle` defined start directly again (now a
  vproc, so the handler cannot fill the stage's pipe before its
  reader starts). (m3a.md, fuzz triage, review)
- Read-ahead for foreign pipes on Windows (Linux peeks with tee(2)).
- No compile cache for `=~` (bash has none either).
- **Arenas never decommit**: a scratch arena keeps its peak commit for
  the shell's life, so one `[[ =~ ]]` whose back-reference memo grows
  (up to 256 MiB; `^(a+)+\1$` on 1000 a's takes 96 MB) leaves that
  much committed. plan.md's `os_decommit` is not implemented. (m4.md,
  review)


## Test infrastructure

- The chaos fuzzer flags every run in which two writer groups (a
  procsub, a pipeline stage before the last, and the command that
  started them; m3a.md) write one pipe or capture, even where their
  order is forced: by start order (`{ echo a; echo >(echo s)
  >/dev/null; } | cat`), by a wait (`wait $!`), or by pipe data
  (`{ cat <(echo a >&2); echo b >&2; } 2>&1 | cat`). Telling those
  apart needs happens-before tracking (spawns, joins, pipe data, vos
  waits). It also trusts that every way output reaches a pipe or
  capture is traced (`fdwrite`, `pipewrite`, capture pumps, the
  capture drain in `runprogram`, vos pipe writes): a new path that
  appends to a capture without `tracewrite` brings back false
  positives. Its exemptions are coarse (a lone `&` exempts an input),
  so background jobs and file races get little coverage. (m3a.md)
- Chaos replays at -O0 agree on some inputs (a `\<NUL>&&` background
  job plus overflowing output) where the AFL build at -O1 diverged;
  such inputs are now exempt, but the cause of the -O level mattering
  was not traced. It is deterministic per build: with the exemptions
  off, the campaign's 50 such inputs never trap, and every run gives
  the same verdicts, all "same" at -O0 and 11 "racy" at -O1 (e.g.
  m/id_000040). Only the plain schedule's racy flag differs. A guess:
  checkstack's limit, which depends on frame sizes.
- The test harness runs out of memory with more than about a dozen
  live vprocs or arrays of more than about 50,000 elements. (m3a.md,
  m4.md)
- The oracle Docker image lacks the hexdump fake (rebuild before
  recording a spec that uses it). (m4.md)
- The vos's FIFOs (`mkfifo`) are plain files: an open does not wait
  for the other end, a reader at the end sees EOF instead of waiting
  for a writer, and data stays after both close: `mkfifo f; echo y >f;
  cat f` prints y where bash waits forever at the first open, and
  `cat f | prog | { echo x >f; cat; }` loses x when cat opens first.
  Nothing that waits on a FIFO can be tested there. (fuzz triage)
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
  a program under 100 ms old that has not yet attached to the console
  when Ctrl-C comes is ended as Ctrl-C would have ended it (also a
  descendant of one that has, and anything a childless one starts
  within 50 ms, which can end a cleanup program an early handler
  starts; m3b.md);
  Ctrl-Break is SIGQUIT; death by SIGINT exits with 0xC000013A.
- Trap actions that signal themselves stop at ten nested levels
  (status 139); bash recurses until it crashes.
- After eval text that ends at its end of input (resumeaftereof),
  `jobs -x` drops the carried end-of-input token, since it parses its
  command where bash runs it directly. Interactive shells were not
  compared with bash for this quirk.
