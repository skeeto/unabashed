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

Closed: every item was fixed or moved (m5a.md, exit criteria).


## M5b (real background processes)

Closed: every item was fixed or moved (m5b.md, step 10).


## M6 (editor)

- `read -e -t`: the timeout does not apply while the editor reads.
  (step 2)
- `read -s` in a non-interactive script killed by Ctrl-C: the terminal
  is restored on the escape out of read, but not tested on a real
  terminal; Windows `read -n` echoes the bytes itself (the console
  echoes only whole lines). (step 2)
- run-history: history4 and history7 both need readline on a
  non-terminal under `-i` (the item below). (step 2)
- The history file is overwritten in place; readline writes a
  `FILE-NNNNN.tmp` beside it and renames it over (keeping a symlink's
  target and the owner). Needs a rename in the platform API. (step 10)
- `bash -c` with `history 1 2`: bash's "too many arguments" jumps to
  top level and abandons the rest of the -c string; ours goes on to
  the next command. (step 10)
- fc's temp file is `bash-fc-` plus 10 random characters, not
  sh_mktmpfp's form; a failed editor leaves no message, as bash. (step 10)
- `bash -i` whose input is not a terminal still reads with readline
  and echoes each line it reads; ours reads plainly. (step 2)
- Unbound key sequences: readline's handling (bell, which bytes are
  dropped or inserted) not yet checked against recordings. (step 2)
- Invalid UTF-8 in the line is drawn as U+FFFD, a column per byte,
  which is what a terminal shows for the raw bytes readline writes;
  a byte that breaks a sequence in self-insert becomes a key of its
  own. Not checked against recordings: a sequence cut short by the key
  timeout or the end of input is inserted as it is (readline may hold
  it until complete), and in the C locale readline (output-meta off)
  shows bytes past 0x7f as `\NNN` where ours draws UTF-8 whatever the
  locale. (step 10)
- checkwinsize: LINES and COLUMNS follow the size each time the
  editor starts and ends a line, not after each foreground program as
  bash's checkwinsize does; a script that resizes and reads COLUMNS
  within one line sees the old size. (step 2)
- A WINCH trap: the editor only catches SIGWINCH itself (POSIX) when
  its disposition is the default, and its trap action runs later than
  bash's. (step 2)

- inputrc variables kept for `$if` (and M8's `bind -v`) but without
  effect: blink-matching-paren, colored-stats and
  colored-completion-prefix, echo-control-characters, enable-keypad,
  enable-meta-key, expand-tilde, history-preserve-point,
  horizontal-scroll-mode, mark-modified-lines, revert-all-at-newline,
  show-mode-in-prompt and the mode strings, the active region colours,
  byte-oriented, input-meta, output-meta and convert-meta. (step 10)
- editing-mode vi is recorded and vi keymaps take bindings, but the
  editor stays in emacs mode; vi-* and other unimplemented function
  names (dabbrev-expand, paste-from-clipboard, ...) bind nothing, so
  they ring and do not shadow. (M8)
- inputrc binding nothing to a prefix (`"\C-x":` alone): readline
  stores a do-nothing shadow, and folds an emptied keymap back into
  its parent; ours leaves the prefix unbound. Bash's own keys and the
  arrow keys are bound before the inputrc rather than after it "if
  unbound", which differs only when the inputrc unbinds one. (step 10)
- inputrc with CRLF lines: msys mode strips the CR (checked with an
  install's CRLF /etc/inputrc on a Windows console); POSIX keeps it, as
  readline does, but no script records that (the vos runs one
  personality). (step 10)
- A terminal special character that stops being one (stty) becomes
  self-insert on the next line, as readline; untested, since neither
  the vos nor the oracle's PATH has stty. (step 10)

Moved from M5a: needs the completion engine.

- completion_strip_exe strips `.exe` from PATH command matches (checked
  on a Windows console), but not from file name matches. (step 10)
- User name completion (`~user`, M-~) finds nothing: no user database
  is read; bash reads the password file. Hosts are read from
  HOSTFILE or /etc/hosts without `$include`. (step 10)
- complete-into-braces (M-{), glob-expand-word (C-x *),
  glob-list-expansions (C-x g), dabbrev-expand and export-completions
  are not implemented. (step 10)
- Command position at PS2 inside a compound command: bash asks the
  parser whether a command word is expected; ours treats only the first
  line of a command as able to start one. (step 10)
- The completion stat hook does not canonicalize against `$PWD`, and
  dirspell is not done. (step 10)


## M7 (job control and fidelity)

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
  may fail there: handled, later messages are ignored). On the XP
  laptop 006 and 012 passed 100 runs, 008 failed 1 in 60 ("after 0":
  `g | "$U" -c "cat >/dev/null"`, the child shell missed the Ctrl-C),
  down from 3 in 40 before programs started after an unseen Ctrl-C were
  ended (os_spawn's unseen rule) and before notemissedspawn compared
  creation times; the cause of the rest is not known. A 32-bit shell
  reading a 64-bit child's PEB (console image, Ctrl-C ignored,
  initializing, same image) gets "unknown". On Windows 11 the i686
  build in XP I/O mode (-x) fails 008 ("after 0") about 3 runs in 10,
  as much with builds from before M5b step 6.
  Moved from M5b: the Ctrl-C work belongs with job control, and none of
  it is about background processes.
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
  Moved from M5b: with the other Ctrl-C items.
- **Win32 Ctrl-C before SIGPIPE, single-task shell** (unobserved): a
  shell process writing to a native program that dies of Ctrl-C can
  see the broken pipe before its own control handler thread runs, and
  die of SIGPIPE (only PIPESTATUS differs; the parent still acts on the
  failed program stage). Waiting for a late Ctrl-C before dying of
  SIGPIPE would delay every `yes`-style loop into `head`. Moved here
  from Next: never seen, and its only fix costs throughput; revisit
  with the Ctrl-C work above. (m3b.md)
  Moved from M5b: with the other Ctrl-C items.
- **XP, in the sandbox only: a killed background job's status is
  often 0**: `sleep 3 & kill $!; wait $!` (also `"$U" -c "sleep 3" &`)
  ends the job at once but `wait` gives 0 for most iterations after the
  first, where Windows 11, and XP outside tools/sandbox, give 143 every
  time. Seen with binaries from before M5b too, so not the image or
  SIGEXIT; makes t/win32/016's "TERM 143" line flaky on XP. Likely the same
  cause: 010's SIGPIPE check gave `0 0` for `141 0` about 1 run in 6
  on XP in the sandbox (M5b step 10).
  Moved from M5b: seen only on XP and only under tools/sandbox, with
  binaries from before M5b too; status reporting is job control.
- **Win32: a bare `wait` can return before every background program
  has been reported** (XP laptop, t/win32/003, about 1 run in 4 since
  at least step 6): `wait -n` afterwards yields one extra status 0
  from the first batch of 70 (`n=67` for 66 programs). Not seen on
  Windows 11. Rechecked with real background processes (M5b step 10):
  still about 1 run in 4 on XP, never on Windows 11.
  Moved from M5b: XP only, and older than real background processes.
- **XP: t/win32/014-spawn failed once in 13 runs** (M5b step 10, with
  UNABASHED_XPIO=1); the diff was lost and twelve reruns passed. Look
  for it with the other XP flakes, which may share a cause.
- **`read -t` never times out on input that never ends a line**: `read
  -t 0.01 x </dev/zero` hangs on POSIX, where bash fails it with 142;
  the deadline is checked only while waiting for data, and /dev/zero
  always has some. (m5b.md, step 8)
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
Moved from M5a (fidelity gaps that neither the w64devkit trial nor
the Git Bash corpus reaches; M7 is where fidelity is finished):

- **Time zone gaps** (step 9): no Windows-to-IANA mapping (Cygwin's
  tzmap, for an MSYS2 install that has zoneinfo); glibc's cache of an
  unchanged TZ value and bash's stale export environment right after
  `export TZ=x` are not emulated (the recordings avoid both); native
  mode's `%Z` for a TZ busybox-w32/msvcrt would read differently (a
  zone name, a long Windows name) is unverified. (m5a.md, step 9)
- **Startup gaps** (step 9): a syntax error in a native-mode profile is
  not fatal and is reported bash's way (busybox exits 2); native mode
  accepts `--noprofile`/`--norc` and has `logout` (busybox has
  neither); long options after short ones are accepted (bash prints
  its usage, 2); bash's bashrc for ssh/rshd-started shells
  (SSH_CLIENT) is not done; the `-i` job-control complaints test
  stdin and stderr for a terminal, not a controlling terminal.
  (m5a.md, step 9)
- **MSYS2 programs get no `/dev/fd/N` bridge** (step 8): they are
  passed `/dev/fd/N` unchanged, which they cannot open, so `cat <(x)`
  with Git's cat fails. They take `\\.\pipe\NAME` (not
  `//./pipe/NAME`), but open it three times (look, use, look again),
  read an unbuffered instance as empty, fail to open one for writing
  (`tee >(x)`: EINVAL), take it for a file head seeks back in, and with
  buffered instances diff and an empty procsub hung at times. Nor do
  they get descriptors above 2: a Cygwin child ignores an MSVCRT
  lpReserved2 blob (checked: busybox to Git's bash, fd 4 is "Bad file
  descriptor"); Git Bash passes them in the cygheap. (m5a.md, step 8)
- **Bridge gaps** (step 8): busybox diff reads a bridged pipe as empty
  (it copies a pipe to a temporary file; the unbuffered instance
  apparently looks empty to it), recorded as such in t/win32/015; a
  program reading one path twice gets the stream split; a program
  that keeps a looking handle open while another waits on data
  stalls; direct pipeline stages (spawnto) get no bridge; at most 16
  bridges per command; a grandchild still using a reading bridge's
  path when its program exits is cut off. (m5a.md, step 8)
- **splitcygwin gaps** (the msys re-split): it assumes Cygwin's
  globbing is on (with `MSYS`/`CYGWIN=noglob` Cygwin keeps the quotes);
  the program name, a non-ASCII byte before `:` as a drive word, and
  tilde with an existing HOME were not recorded. msysargv's message and
  status on the 11th response file are unverified. (m5a.md, steps 2, 5)
- **Spawning gaps** (m5a.md, step 7): the missing-DLL search ignores
  KnownDLLs, side-by-side manifests and the loader's other rules; the
  same Cygwin message is printed in native mode (busybox-w32 would print
  "Error 0xc0000135" style text, unverified); ELOOP's text for a fifth
  nested `#!` is Linux's, unverified against busybox-w32; Git Bash's
  E2BIG limits for MSYS2 children (the cygheap, not the command line)
  are not modelled. Exec's XP/7 job path (breakaway) passes on
  the XP laptop (t/win32/014 with sandbox -B, and outside the sandbox);
  Windows 7 is untested.
- **/proc has names but little content** (step 10a): /proc lists
  Cygwin's fixed entries and the shell's own pids (Git Bash: every
  process of its install, by Cygwin pid), and stat knows their types,
  but no fixed file can be read (EACCES; cpuinfo, meminfo, mounts,
  stat, uptime, version... have contents in Git Bash) and the
  directories net, registry*, sys, sysvipc list nothing; /proc/self
  lists only exename, fd and winexename and serves only the first and
  last (no cmdline, cwd, environ, exe, maps, stat, status, winpid...),
  and /proc/self/fd lists nothing. `/dev/zero`, `/dev/random` and the
  other sinks only accept writes; reads fail. (m5a.md, steps 5, 10a)
- **Native mode skips two busybox-w32 fix-ups**: assigning PATH does
  not turn `:` into `;`, and cd does not fix a path's case to what is
  on disk (PWD keeps the case typed). (m5a.md, step 5)
- **Identity is not re-rendered by msyspath**: toggling it re-renders
  PATH and PWD only; BASH, OSTYPE, MACHTYPE and HOME keep the startup
  persona's forms. pwd's usage text in the Windows modes still says
  `[-LP]` (Git Bash's is unverified). (m5a.md, step 5)
- **cygpath gaps** (m5a.md, step 6): -W and -S come from SYSTEMROOT
  (or WINDIR), not GetWindowsDirectory, so their case is the
  variable's (Cygwin prints `C:\WINDOWS\system32` with -w and the
  on-disk case with -u; t/msys/063 folds case); -l and -s are no-ops
  (no 8.3 names); -a is ignored with -p; -d, -t, -r, -U, -C, -D, -H,
  -O, -P, -F and -A are unknown options; the messages and usage text
  are ours. An empty name stops the run (recorded for the last name
  and for -f only).
- **`/dev/consN`, `/dev/ptyN`, `/dev/ttySN`** are untested against Git
  Bash.
- **An inherited backslashed PWD**: when PWD names the current
  directory as `C:\x\y`, Git Bash keeps it and pwd prints `C:\x/y`
  (only the first separator stays a backslash); ours prints `C:/x/y`.
  Seen while checking Claude Code (m5a.md, exit criteria).


## M8 (performance)

- POSIX: if another process reads the same pipe as a shell task, a read
  after poll can block the whole shell until data arrives. (m3a.md)
  Moved from M5b: never seen in a test; it belongs with the read-ahead
  work on pipes other processes also read.
- **A pipe lent to a program aborts another reader's helper read**
  (step 8): passing a pipe descriptor that a pump also reads made the
  pump's read fail with E_INTR (the lend's CancelSynchronousIo). The
  bridges avoid it by not passing bridged descriptors; any other
  shared reader of a lent pipe would hit it. (m5a.md, step 8)
  Moved from M5b: no test of real background processes hit it; it
  belongs with the read-ahead work on shared pipes.
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

- t/unit/fakes_test.cpp no longer builds: vos.cpp uses paths.cpp
  (equalfold, hasdrive, winnorm, ...) and pe.cpp's PeInfo since M5a
  steps 4-7, and the test does not include them. (m5a.md, step 9)
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

Moved from M5a: limits of the test tiers, not of the shell.

- **t/winvos/ is a hand-checked baseline**, not oracle output (steps
  4-8, specs 001-017 and 024); change it deliberately.
- **FUZZ_WIN only runs the native persona**; the msys persona and the
  msyspath toggle are fuzzed only through the spec replays.
- **The vos's cmd.exe is a small model**: no `&`, `&&`, `||`, `|`,
  redirections, parentheses, `if`, `for`, `goto`, `call`, `setlocal`,
  `echo on`, delayed expansion or the pre-`/s` quote rules (batch
  lines do take `%0`-`%9`, `%~N`, `%*`). Enough for step 7's batch
  quoting tests only. (m5a.md, step 4)
- **The vos's msys children are thin**: they get the native argv split
  by splitcygwin and a `/c/...` cwd, but no mount table, no
  environment conversion (PATH stays Windows form) and no
  `MSYS=noglob`; `@file` stops quietly after ten levels instead of
  dying on the 11th; the glob handles `* ? [...]` and `{a,b}` per
  component, not `**` or character classes. (m5a.md, step 4) The
  vos's env fake searches PATH without adding `.exe`, so a native
  `#!/usr/bin/env PROG` is only tested on the real system (014).
- **Windows path gaps in the vos**: trailing dots and spaces are not
  stripped from names, `C:x` resolves against the drive root rather
  than that drive's cwd, rooted paths (`/x`, `\x`) always land on C:,
  case folding is ASCII only, `CON` and the other devices are all the
  null device, and `//./pipe/` cannot be listed. (m5a.md, step 4)
- **t/msys and bench/agent are not write-confined**: MSYS2 programs
  cannot start at Low integrity, so they run under `sandbox -L` (job,
  limits, timeout, restricted token, medium integrity). Only the corpus
  rules (tools/README.md) keep writes in WORKDIR. Untried: an
  AppContainer, or the sandbox pre-creating the msys
  `\BaseNamedObjects\msys-2.0S5-<key>` directory with a Low label.


## Accepted deviations

- **Accepted: Win32 programs run in a job of their own** (Windows 8 and
  later, see judge): `IsProcessInJob` is always true;
  `QueryInformationJobObject(NULL)` describes that job, not the shell's,
  so runtimes that size themselves by the job's memory limit (.NET) see
  none; breakaway is allowed only as the shell's own job allows it
  (`JOB_OBJECT_LIMIT_BREAKAWAY_OK`, or always when the shell is in no
  job), and a descendant that breaks away is no longer judged; under a
  shell job with silent breakaway, children that were in no job are
  now in ours, and their children stay in it.
- **Accepted: a background job that could not get a process of its
  own** (the image could not be made or mapped, or the platform has
  no images) runs as a fiber, and the shell waits for it at exit,
  where bash leaves it running. (m3a.md, m5b.md)
- **Accepted: `exec prog < <(...)` on Windows and the vos** runs
  the program as a child while the procsub lives, so its pid is not
  `$$`; POSIX forks the shell first and keeps it (m5b.md, step 5).
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

- **Accepted: pesniff ignores** delay-load imports, the loader's
  rounding of PointerToRawData to 512 bytes and DLL names over 63
  bytes; reads at most 1024 section headers. None matters for the
  binaries tried. (m5a.md, step 2)
- **Accepted: MSYS2 children have their own install's mount table**
  (step 10a): Git Bash's children inherit its table and POSIX environment through
  the cygheap; ours read their install's fstab, or join the table
  that install's first live process set up (its `/tmp` from that
  process's TMP), and convert the Windows forms we pass. So a child
  in another install than the shell's has another `/`, and while the
  user runs Git Bash with another TMP, Git's children started by us
  see that `/tmp` (t/msys isolates them with a clone of Git's
  install). Not modelled: a probe of the child's table.
- **Accepted: Cygwin's options follow the mode, not the persona**: igncr,
  completion_strip_exe and the cygpath builtin exist while msyspath is
  on; switching it off hides them (igncr keeps its value, without
  effect) and a native persona gains them with `shopt -s msyspath`.
- **Accepted: `2>nul`**: Git Bash creates a file named `nul`; a native
  shell opens the device. Not in the t/msys corpus.
- **Accepted: BatBadBut quoting**: arguments to .bat/.cmd targets are
  quoted for cmd.exe's safety, unlike Cygwin's. Not in the corpus.
- **Accepted: BASH_VERSION and friends** differ from Git Bash's
  (`5.3.15(1)-release`); the corpus prints only BASH_VERSINFO[5].
- **Accepted: `type cygpath`** says builtin in msys mode (m5a.md,
  Decisions). Not in the corpus, nor is cygpath's usage text (GPL).
- **Accepted: paths.cpp stays lexical where Cygwin asks the file
  system** (m5a.md steps 1, 5): cygpath -w of `C:` or `C:x` (Cygwin:
  cwd\C), relative paths and `..` through missing directories,
  `\\.\NUL` (Cygwin -w: `\\?\NUL`), cygpath mapping `<>|"` to private-use
  characters, /proc/self (Cygwin: /proc/PID), and a closed /dev/fd/N
  argument. The shell asks the file system only for the `.exe` suffix
  rule (also SHELL and the other single-path variables reaching a
  child, step 7) and the virtual files. None is in the corpus.
- **Accepted: bash's ENOEXEC fallback in native mode**: a file without
  `#!` (and not `.sh`) runs as shell code; busybox-w32 fails to run it.
- **Accepted: a Windows-form value in HOME, TMP, PATH and the other
  conv_envvars reaches an MSYS2 child converted** (step 10a):
  `HOME='C:\Windows' printenv HOME` prints C:\Windows in Git Bash,
  whose children take the value from the cygheap, and /c/Windows from
  us: a child of a non-Cygwin parent converts these variables at
  startup from any Windows spelling (probed: every form tried, and
  MSYS2_ENV_CONV_EXCL and MSYS_NO_PATHCONV do not stop it). Only a
  Cygwin parent (a trampoline such as Git's env.exe) could pass one
  through. Removed from t/msys/037.
- **Accepted: a 32-bit build's identity is i686** (MACHTYPE,
  HOSTTYPE, BASH_VERSINFO[5]) where 64-bit Git Bash says x86_64;
  t/msys/run normalizes it for a 32-bit shell (tools/README.md).
- **Accepted: busybox-w32's read after a backslash CR LF continuation**
  (1.39.0, M5a step 10): it ends the continued line at the next CR and
  leaves that line's LF unread, so the next `read` gets an empty line.
  Not copied (a busybox bug); t/winvos/011 has our reading.
