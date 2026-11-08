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

Closed: every item was fixed or moved (m6.md, step 10).


## M7 (job control and fidelity)

None: the XP flakes were fixed or not seen again at step 10 (m7.md).


## M8 (performance)

None left: step 10 fixed the rest or accepted it (below).

## M9 (w64devkit)

Found while writing unabashed.1 (2026-11-08), not yet fixed:

- `times` and the `time` keyword report no CPU time in any build: user
  and system are always 0 (exec.cpp, "CPU times are not tracked yet"),
  where plan.md meant job-object accounting on Windows.
- Nesting deep enough to overflow the parser's stack (thousands of
  `( `) ends the shell with status 139; bash's bison stack fills first,
  and it reports `syntax error near unexpected token` and goes on.
- Native mode rewrites colons to semicolons only in PATH; busybox-w32
  also rewrites CDPATH and MANPATH.
- Msys mode discards writes to `/dev/clipboard`; Git Bash sets the
  Windows clipboard.
- A native child of an msys shell whose PATH is missing or empty gets
  `PATH=<root>\usr` (MSYS2's rule), a directory w64devkit lacks.
- The x86_64 build sets no PE subsystem version (the i686 build sets
  5.01), so it is untested on XP x64.
- The shell's own console output shows bytes that are not UTF-8 as the
  Latin-1 character of the same value; the editor shows U+FFFD.
- A `#!/bin/sh` or `#!/bin/bash` script runs in a copy of the shell
  whose argv[0] is the executable's path (spawn.cpp, wintarget), so its
  persona, identity variables and startup rules follow that file name,
  not the parent's: from `lash` the copy is a login shell and reads the
  profiles again. A neutral name per mode would fix it, but it also
  decides the copy's quoting and POSIX mode, so it needs its own step.
- An fstab `cygdrive` line's `posix=0` does not make drive paths match
  ignoring case, as Cygwin's does.
- Mount points under a directory that does not exist are not listed by
  pathname expansion of their parent.
- Native mode keeps a trailing CR on inputrc lines.
- `BASH` drops only a lowercase `.exe` (unabashed.cpp, initvars), where
  the msys first-word conversion drops one in any case.
- In msys mode a first word without `/` or `\`, such as `bash.exe`,
  keeps its `.exe` in `$0` (Git Bash's behaviour unchecked).


## Test infrastructure

- t/exec/634-coproc races, in bash as in ours: the first coprocess
  answers and exits at once, and if the shell reaps it before line 7's
  `read <&${COPROC[0]}` expands, COPROC is unset ("ambiguous
  redirect"). The record check on Linux lost it 10 times in 10 (the
  M8 build 3 in 10) when the shell image was on /mnt/c, whose page
  faults through drvfs slow the shell's first pass through new code;
  from ext4 it passed 20 in 20. Run Linux record checks with the image
  on ext4 (~/ub). The script could keep the coprocess alive until read
  (a second read) if this shows up elsewhere. (M9 step 7)
- The vos's FIFOs are plain files: opening one does not wait for the
  other end, and reading one with no data is the end of file, so a
  test whose point is a reader waiting on a writer (t/exec/938) passes
  on the vos whatever the shell does; RECORD_CHECK on the Linux build
  is what checks it. (m8.md, step 4)
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


- inputrc with CRLF lines: msys mode strips the CR (checked with an
  install's CRLF /etc/inputrc on a Windows console); POSIX keeps it, as
  readline does, but no script records that (the vos runs one
  personality). (m6.md)
- A terminal special character that stops being one (stty) becomes
  self-insert on the next line, as readline; untested, since neither
  the vos nor the oracle's PATH has stty. (m6.md)


## Accepted deviations

- **Accepted: no BOM handling, as bash.** A UTF-8 byte order mark is
  part of the first word of a script, sourced file or -c text, and
  hides a `#!` line from the shell's own sniffer; Git Bash 5.3.15 does
  the same. (2026-11-08)
- **Accepted: mapfile keeps CRs** in every Windows mode, igncr or not,
  as Git Bash 5.3.15 does; read drops them under igncr (msys) and before
  a newline (native, as busybox's read). (2026-11-08)
- **Accepted: native sh is not in POSIX mode by name** (the open
  decision of m9.md step 3, kept): sh, ash and lash behave as
  busybox-w32's sh; `--posix` or `set -o posix` turns POSIX mode on.
  (2026-11-08)

- **Accepted: emulated /dev/zero and kin are pipes.** In the Windows
  modes, /dev/zero, /dev/full, /dev/random and /dev/urandom opened for
  reading are an OS pipe fed by a pump (genfile; the random ones from
  the process's ChaCha8 stream, keyed with 256 bits of os_entropy and
  reset by a fork). So a descriptor on one is a pipe to `test -p` and to
  programs; a native program's writes to /dev/full fail with EBADF (a
  handle opened only for reading), the shell's own with ENOSPC; a
  background job reading one keeps its image child shell alive to feed
  it, as the job would read forever under bash; and named as an argument
  to a native program, msys mode passes MSYS2's /Device/Null and native
  mode the name as typed (busybox's applets emulate it). Native mode
  has only /dev/zero and /dev/urandom of these, as busybox-w32: there
  /dev/full and /dev/random are files on the current drive.
  (2026-11-08)
- **Accepted: RANDOM is PCG64, not Park-Miller.** RANDOM and SRANDOM
  come from PCG64 streams (vars.cpp), so a seed (`RANDOM=n`) gives a
  different sequence than bash's, though the same one on every
  platform. Bash's repeat avoidance and the BASH_COMPAT<=50 output are
  not emulated. As in bash, every subshell, comsub, pipeline stage and
  child process reseeds on first use, from the process's
  os_entropy and its BASHPID; SRANDOM cannot be seeded. Chosen for
  quality over bit-exact values (2026-11-08).
- **Accepted: a here-document's nested $'...' message.** With extquote
  off, a here-document's `${x#${$'x1'}}` is a bad substitution with our
  usual message; bash reports "bad substitution: no closing `}' in"
  followed by the whole body. (m7.md, step 6) Found by fuzzing; it needs
  bash's brace scanner for here-documents with extquote off, for a
  message only. (M7 step 9)
- **Accepted: no stdio flush artifact.** glibc's 4 KiB stdio flush
  before a diagnostic is not emulated (bash prints an error mid-line
  after 4 KiB without a newline). (m4.md) An artifact of glibc's stdout
  buffer, which output that ends a line never shows. (M7 step 9)
- **Accepted: ${ prog & } waits for prog.** `${ prog & }`, a program
  started straight in the background (not a vproc job, procsub or
  coprocess), holds up the substitution until it closes its output: its
  pump has no owner to check, so endcapture joins it; bash reads the
  substitution's file at once. In bash that file is on the lowest free
  descriptor from 3, so `${ exec 3>/dev/null; echo a; }` is empty there,
  and fd 3 is closed after it. (fuzz triage) Bash's temporary file has
  no counterpart here, and draining a pump without waiting for its end
  would need one; a program started in the background straight from
  `${ ...; }` is rare. (M7 step 9)
- **Accepted: eval's EOF line after a backslash.** Eval text that ends
  inside `$(...)` on a backslash: bash's "unexpected EOF" cites the line
  after ours in some cases (`echo $(echo \`, `\\`, `a\`, `$\`, `$(\`
  report the eval's line + 2, we say + 1) but not others (`\\\`, `\\\\`
  are + 1 in both). Follows from how bash's shell_getc pads a final
  backslash and counts reads at end of input, not worth modelling for
  a line number in an error. (review, M7 step 9)
- **Accepted: rare back-reference mismatches.** regex.cpp differs
  from glibc on back-reference patterns without repetition where
  `rxlongest`'s first pass records close nodes reached through
  back-references glibc finds no span for, which then hide the
  span of a later back-reference: `[^a]($|)\1($)\2` on a space (glibc
  (0,1)(1,1)(1,1), ours no match), and `($|)($)($|)\1\2$\`$($)` on an
  empty subject (glibc matches, ours not). Both differ on master too;
  2 in 400k generated cases, all contrived (back-references to groups
  that matched empty beside anchors); a fix would rework rxlongest's
  two passes. (m4.md, M7 step 9)
- **Accepted: slow regexes, as glibc's.** Some patterns run for
  minutes or more here and in glibc alike: `(.{,2}\>a*{1,})*` on `a`
  (the walk, no back-references), compiling
  `(()?((\b){1,2}((\')?(){0,1}(()*a*b*|[ab]?))*))+` (both verified to
  hang bash 5.3 past 60 s too), and `(\b$){1,}*\>\'` on `x\xFFcAB`
  (regex_diff --random 4, case 2936180; --random has no per-case
  timeout, so that seed stops there). Unlike bash, a search stops for
  an untrapped Ctrl-C (rxsigstop, polled at the back-reference and
  submatch steps); compiling does not, nor a search with INT trapped,
  which bash cannot stop either. (m4.md, M7 step 9)
- **Accepted: no users on Windows**: `~user` is left as it is, `~`
  with HOME unset too, and M-~ finds nothing, there being no password
  file; Cygwin's bash makes them up from the account database
  (/home/NAME). Native-mode busybox has no `~user` either. (M7, step 9)
- **Accepted: jobs -l shows a pipeline as one line**, the job's pid
  and state, where bash lists each process with its own pid (and the
  pipeline's text split at each `|`): a background pipeline is one
  process of ours (the image child), whose stages the parent never
  sees, and $! is that process. (M7, step 9)
- **Accepted: job states that race in bash**: the vos and our Windows
  layer see a signalled job's end at once, where bash shows what it
  has reaped so far: `kill %1; wait %1` just before `;` or the prompt
  finds no job (127) where bash usually gives 143; a job killed by TERM
  after a stop gets the mark "+" where bash shows "+" or " "; after
  `kill -STOP` and `kill -9`, `wait` gives 137 where bash sometimes
  gives the stop's 147 (`wait -f` gives 137 in both). (M7, step 9;
  905 hides the mark.)
- **Accepted: fg's Ctrl-C reaches a background shell job only as a
  signal to that shell** on Windows: its INT trap runs, but the
  program it waits for, started ignoring Ctrl-C as a background job's
  programs must there (no process groups), runs on to its end first,
  where bash's terminal ends it at once. Programs started straight in
  the background are ended with all they started (a job object each).
  (m8.md, step 10)
- **Accepted: Windows pipes that take more than they hold.** A writer
  to our own shell's pipe at times gets far more than 64 KiB in before
  the reader exits: the write quota (NtQueryInformationFile) runs down
  to near 0 and comes back to 65536 although neither process calls
  ReadFile (an instrumented build), while with ping.exe as a reader
  that does not read it stays at 0. So `f | reader` where the reader
  quits early may let f finish a fixed amount of output without
  SIGPIPE (status 0, not 141), always on XP; a writer that goes on
  gets 141. t/win32/010-writes writes until then. (m8.md, steps 3
  and 10)
- **Accepted: inputrc settings without effect**, kept for `$if` and
  `bind -v`: blink-matching-paren, colored-stats and
  colored-completion-prefix, enable-keypad, enable-meta-key,
  horizontal-scroll-mode (our sideways display is TERM=dumb's, which
  also drops the active region and bracketed paste),
  revert-all-at-newline, the active region colours, byte-oriented,
  input-meta, output-meta and convert-meta. Colours and terminal modes
  that bash's own tests and bash-completion do not rely on. (m6.md,
  step 10; m8.md, step 10)
- **Accepted: an empty inputrc binding**: `"\C-x":` alone leaves the
  prefix unbound, where readline stores a do-nothing shadow and folds
  an emptied keymap back into its parent. Bash's own keys and the
  arrow keys are bound before the inputrc rather than after it "if
  unbound", which differs only when the inputrc unbinds one (or bind
  does); the terminal's other keys (Delete, Insert, Home, End, Page Up
  and Down as `\e[N~`) are bound after it, as readline does. (m6.md,
  step 10)
- **Accepted: keyboard macros record the keys read**, not readline's
  dispatched keys: readline misses keys inserted together with
  self-insert's type-ahead (`ab` typed at once records only `a`), and
  records a key macro's expansion again as it runs. Readline's quirks
  depend on how fast keys arrive. (m6.md, step 10)
- **Accepted: old-menu-complete runs menu-complete.** Readline keeps
  the old function for compatibility; it differs in details of the
  display prefix nobody binds it for. (m6.md, step 10)
- **Accepted: invalid UTF-8 in the line** is drawn as U+FFFD, a column
  per byte, which is what a terminal shows for the raw bytes readline
  writes; a byte that breaks a sequence in self-insert becomes a key
  of its own. A sequence cut short by the key timeout or the end of
  input is inserted as it is (readline may hold it until complete),
  and in the C locale readline (output-meta off) shows bytes past 0x7f
  as `\NNN` where ours draws UTF-8 whatever the locale. A first
  recording was inconclusive (\xe6 ran M-f, which readline's C
  defaults do not explain together with the rest). (m6.md, step 10)
- **Accepted: Win32 console Ctrl-C limits**: a Ctrl-C from
  GenerateConsoleCtrlEvent does not end a console read in progress (a
  typed one does); Ctrl-C during CreateProcess counts against the
  shell even if the program handled it. (m3b.md; accepted in M7, step
  8: the first is the console's, the second the unseen rule's price.)
- **Accepted: Win32 Ctrl-C a starting program can still miss** (m3b.md, "Programs
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
  Accepted in M7 (step 8): a console delivers Ctrl-C only to the
  processes attached at that moment, and each case above is
  narrowed as far as the console lets a shell see; the 008 failure
  (also 1 run in 12 on Windows 11, i686 in XP I/O mode) is the
  wrapper case: w64devkit's cat.exe stub starting busybox late.
  Native mode, which skips the stubs, does not have it.
- **Accepted: Win32: a foreign program's Ctrl-C handler that starts a program is
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
  Accepted in M7 (step 8): the watch is what catches the stubs'
  late busybox; a cleanup program started within 50 ms of a Ctrl-C
  is the rarer case.
- **Accepted: Win32 Ctrl-C before SIGPIPE, single-task shell** (unobserved): a
  shell process writing to a native program that dies of Ctrl-C can
  see the broken pipe before its own control handler thread runs, and
  die of SIGPIPE (only PIPESTATUS differs; the parent still acts on the
  failed program stage). Waiting for a late Ctrl-C before dying of
  SIGPIPE would delay every `yes`-style loop into `head`. Moved here
  from Next: never seen, and its only fix costs throughput; revisit
  with the Ctrl-C work above. (m3b.md)
  Accepted in M7 (step 8): never seen, and only PIPESTATUS differs.
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
- help and `--help` text is our own (plan.md: original help text), so
  the suite's builtins test stays FAIL on its help lines.
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
  part, intl's ru_RU.CP1251): such a locale is as one not installed,
  with bash's setlocale warning.
- Locales: every UTF-8 locale counts as installed (bash warns for one
  that is not, and keeps the old one); LC_NUMERIC's decimal point comes
  from a table of languages and territories, and `%'` groups nothing;
  messages are never translated (bash under de_DE says "Zeile").
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
- **Accepted: the line counted in an interactive command substitution's
  EOF message** (M7 step 9): each shell-expand-line that hits an
  unterminated `$(` advances bash's line counter, so the message says
  line 3, 4, 5 in turn and $LINENO lags one behind ours afterwards;
  ours says line 2 each time. An artifact of bash's parser state. And
  a `${x@P}` whose unterminated `$(` holds a `${` gets a second
  message about the brace from bash's re-parse; we give one.
- **Accepted: a history file's owner is not restored after it is
  replaced** (M7 step 9): readline chowns the new file back to the old
  owner, which matters only to root sharing a history file (sudo). And
  on Windows a symbolic link is written through in place: the Win32
  layer reads no reparse points.
- **Accepted: traps for TERM, HUP, QUIT, ALRM and the stop signals
  while a line is edited** (M7 step 9): readline catches these itself
  and hands them to bash at once, which runs the trap mid-line (and, as
  probed, exits on TERM even with a trap); ours keeps them for the line
  to be done, as every other trap. WINCH and INT are bash's.
- **Accepted: native cd and a UNC path's case** (M7 step 9): busybox-w32
  gives a share's name its listed case and carries a server's case over
  from the last directory; ours keeps both as typed (shares are not
  listed).
- **Accepted: time zone gaps** (M7 step 9, from M5a): no Windows-to-IANA
  mapping (Cygwin's tzmap, for an MSYS2 install that has zoneinfo);
  glibc's cache of an unchanged TZ value and bash's stale export
  environment right after `export TZ=x` are not emulated (the recordings
  avoid both); native mode's `%Z` for a TZ busybox-w32/msvcrt would read
  differently (a zone name, a long Windows name) is unverified. (m5a.md,
  step 9) No Windows install the persona targets ships zoneinfo; the
  glibc and export-timing quirks are artifacts the recordings avoid.
- **Accepted: startup gaps** (M7 step 9, from M5a): a syntax error in a
  native-mode profile is not fatal and is reported bash's way (busybox
  exits 2); native mode accepts `--noprofile`/`--norc` and has `logout`
  (busybox has neither); the `-i` job-control complaints test stdin and
  stderr for a terminal, not a controlling terminal. (m5a.md, step 9)
  The native persona keeps bash's messages and options by design (plan:
  bash POSIX-mode semantics); bash's ssh bashrc is off in upstream's
  config-top.h (SSH_SOURCE_BASHRC), as ours; the job-control complaint
  differs only for a terminal that is not controlling.
- **Accepted: `--debugger` and bashdb** (M7 step 9, from M5a):
  `--debugger` looks only for `/usr/local/share/bashdb/bashdb-main.inc`,
  bash's default prefix. bashdb is not shipped for Windows; the prefix
  is upstream's default. (The eval line count is accepted above.)
- **Accepted: splitcygwin gaps** (M7 step 9, from M5a): it assumes
  Cygwin's globbing is on (with `MSYS`/`CYGWIN=noglob` Cygwin keeps the
  quotes); the program name, a non-ASCII byte before `:` as a drive
  word, and tilde with an existing HOME were not recorded. msysargv's
  message and status on the 11th response file are unverified. (m5a.md,
  steps 2, 5) Each needs a Cygwin setting or input no caller of the
  shell gives (noglob, a non-ASCII drive word, an 11th response file).
- **Accepted: spawning gaps** (M7 step 9, from M5a): the missing-DLL
  search ignores KnownDLLs, side-by-side manifests and the loader's
  other rules; the same Cygwin message is printed in native mode
  (busybox-w32 would print "Error 0xc0000135" style text, unverified);
  ELOOP's text for a fifth nested `#!` is Linux's, unverified against
  busybox-w32; Git Bash's E2BIG limits for MSYS2 children (the cygheap,
  not the command line) are not modelled. Exec's XP/7 job path
  (breakaway) passes on the XP laptop (t/win32/014 with sandbox -B, and
  outside the sandbox); Windows 7 is untested. The loader's full search,
  busybox's error spelling and the cygheap's limits only change the text
  of a failure that already fails.
- **Accepted: /proc has names but little content** (M7 step 9, from
  M5a): /proc lists Cygwin's fixed entries and the shell's own pids (Git
  Bash: every process of its install, by Cygwin pid), and stat knows
  their types, but no fixed file can be read (EACCES; cpuinfo, meminfo,
  mounts, stat, uptime, version... have contents in Git Bash) and the
  directories net, registry*, sys, sysvipc list nothing; /proc/self
  lists only exename, fd and winexename and serves only the first and
  last (no cmdline, cwd, environ, exe, maps, stat, status, winpid...),
  and /proc/self/fd lists nothing. `/dev/clipboard` and the other
  sinks only accept writes, and discard them (Git Bash sets the
  clipboard); reads fail. (m5a.md, steps 5, 10a)
  Scripts on Windows do not read /proc (nproc and uname answer the usual
  questions); the names keep `test -e` and globbing right.
- **Accepted: identity is not re-rendered by msyspath** (M7 step 9, from
  M5a): toggling it re-renders PATH and PWD only; BASH, OSTYPE, MACHTYPE
  and HOME keep the startup persona's forms. pwd's synopsis follows
  the mode, as help shows it: `[-LPW]` in msys mode (Git Bash's),
  `[-LP]` in native mode, which accepts -W and ignores it. (m5a.md,
  step 5) The shopt is hidden and for testing; the startup persona
  decides identity, as Git Bash and busybox-w32 each have one.
- **Accepted: cygpath gaps** (M7 step 9, from M5a): -W and -S come from
  SYSTEMROOT (or WINDIR), not GetWindowsDirectory, so their case is the
  variable's (Cygwin prints `C:\WINDOWS\system32` with -w and the
  on-disk case with -u; t/msys/063 folds case); -l and -s are no-ops (no
  8.3 names); -a is ignored with -p; -d, -t, -r, -U, -C, -D, -H, -O, -P,
  -F and -A are unknown options; the messages and usage text are ours.
  An empty name stops the run (recorded for the last name and for -f
  only). The options used by scripts (-u -w -m -p -a -W -S) are there;
  the rest are Cygwin's, rarely seen outside Cygwin itself.
- **Accepted: `/dev/consN`, `/dev/ptyN`, `/dev/ttySN` untested** (M7
  step 9, from M5a): never compared with Git Bash. Cygwin's console and
  serial names; nothing in the persona's use reaches them.
- **Accepted: Git Bash's backslashed PWD** (M7 step 9, from M5a): when
  PWD names the current directory as `C:\x\y`, Git Bash keeps it and pwd
  prints `C:\x/y` (only the first separator stays a backslash); ours
  prints `C:/x/y`. Seen while checking Claude Code (m5a.md, exit
  criteria). Cygwin's half-converted `C:\x/y` is an artifact; ours
  prints the path one way.
- **Accepted: an MSYS2 program's procsub is a file.** An MSYS2 or
  Cygwin program given `/dev/fd/N` gets the path of a temporary file
  (M9 step 5): for `<(cmd)`, cmd runs to its end before the program
  starts, so the two do not overlap, and past 1 GiB (an endless `yes`)
  the shell gives up and passes `/dev/fd/N` unchanged; for `>(cmd)`,
  cmd gets the output once the program has exited. The program sees
  the file's name (diff's header, wc's) where Git Bash shows
  /dev/fd/63. Git Bash passes descriptors in Cygwin's cygheap, which
  no other parent can; the named-pipe bridges that native programs get
  hung or failed with MSYS2 programs (m5a.md, step 8). For the same
  reason descriptors above 2 never reach an MSYS2 program.
- **Accepted: bridge limits.** A native program reading one bridged
  path twice gets the stream split between the opens, as with a pipe's
  /dev/fd/N on Linux; a program that keeps a looking open handle while
  another open waits for data stalls; at most 16 bridges per command
  (more stay /dev/fd/N); a grandchild still reading a bridge's path
  after its program has exited is cut off, as the procsub's descriptor
  closes with the command. (m5a.md step 8, M9 step 5)
