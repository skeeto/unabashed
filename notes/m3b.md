# M3b: Win32 I/O

The working notes for M3b (notes/plan.md §2 "Win32 I/O" and "Signals
and control flow"; the critique's M3 issue). M3b replaces M3a's simple
Win32 helper threads with a per-object state machine, exact reads, thread
pool process waits and a wake queue, and adds Ctrl-C. Later agents must
keep this file up to date.

## Status

Done:
- The per-object state machine (`Wobj`, main_win32.cpp), with helpers
  that read one byte (a console line) into a read-ahead, interposed
  pipes and pumps for children, and drain-back.
- Peek-exact reads: `PeekNamedPipe`, then `ReadFile` of exactly what is
  there, at most what was asked; `os_peek` lets the read builtin take a
  pipe a line at a time without consuming past the delimiter.
- Process exits through `RegisterWaitForSingleObject` (no 64-handle
  limit); a wake queue (helpers, wait callbacks and the console control
  handler push and `SetEvent`; the main thread waits on one event).
- Ctrl-C and Ctrl-Break as group signals (`os_siggroup`), delivered to
  every running level of the shell, with bash's cooperative rule for a
  level waiting on a child; SIGINT traps; interactive shells (`-i`)
  return to the prompt; `read -t` times out on an empty pipe or console
  (CancelSynchronousIo on Vista+, abandonment on XP and for consoles).
- Everything on XP: no import newer than XP (CancelSynchronousIo is
  resolved at run time); CRT-free (`objdump -p`: kernel32.dll only, for
  both unabashed.exe and unabashed32.exe; PE subsystem 5.01 for i686).
- Tests: `t/win32` (10 curated scripts, `t/win32/run`, under the
  sandbox and `tools/conrun`), passing on Windows 11 in both code paths
  and on the XP laptop; specs `t/exec/700`-`703` on the vos.
- Interactive Ctrl-C checked by hand with winctl on XP's console (see
  "Testing").

## The state machine (main_win32.cpp)

A synchronous file object serializes all I/O on it, through every
handle duplicated from it: while a thread blocks in `ReadFile`, a
`PeekNamedPipe`, `NtQueryInformationFile`, `GetFileInformationByHandle`,
a second read, or the last `CloseHandle` on it blocks too. So:

**Registration.** Every handle the shell holds to a pipe or a
character device is registered (`Hent`, a 64-bucket table) with the
`Wobj` of its file object: `os_pipe` (each end), `os_open` (`track`
classifies by `GetFileType` and `GetConsoleMode`), the standard handles
at startup (the same handle twice is one object), and `os_dup`, which
registers the duplicate with the *same* `Wobj`. Disk files are not
registered (they never block). `os_close` unregisters; a `Wobj` with no
handles is released only once unused (`maybefree`: no helper, no
interposition, no child holding it, nothing queued). Kinds: `K_PIPE`,
`K_CONIN`, `K_CONOUT`, `K_CHAR` (NUL). `os_fstat` and `os_seek`
answer for registered handles without touching them.

**Helpers** (`Helper`, at most 64, threads made on demand and pooled)
perform one operation on a `Wobj` through its private duplicate
(`Wobj.own`, made on first use), so that the core closing its handle
is never the last close while a read is pending:
- `H_READ`: one byte from a pipe; `H_CONREAD`: one console line (UTF-8,
  CRLF as LF). The result goes to the **read-ahead** (`Wobj.ahead`),
  then the Io waiting for it (`Wobj.active`) takes what it needs from
  there plus whatever the pipe holds (peek-exact). An abandoned read
  (`os_cancel`: read -t timing out, a signal) clears `active`; its byte
  or line stays in the read-ahead for the next reader. On Vista+ a
  pipe read is also cancelled (`cancelhelper`: `CancelSynchronousIo`,
  retried for up to 20 ms while the helper is live, since a cancel can
  come before the call starts). A console read is never cancelled: the
  console throws away the next line typed after a cancel (observed on
  Windows 11), so it runs on, abandoned.
- `H_WRITE`: the core's bytes, whole (`os_cancel` refuses it).
- `H_PUMP`: see Interposition.

While a helper operation is in flight or an interposition is active
(`busy`), every operation on the object goes through the state
machine: reads and writes queue behind it (`Wobj.queue`, started in
order by `runqueue` when it goes idle), `read -t 0` reports not ready,
`os_peek` reports only the read-ahead, closing is deferred.

**Lending.** A child given a pipe or console read end directly (`lend`
in `os_spawn`) may itself block in a read, holding the file object, so
while any child holds it (`Wobj.lent`, `Lend` records keyed by the
child's process handle) the shell reads it only through helpers.
Holds end when the core observes the child's exit (`os_waitproc` or a
completed `IO_PROC`, `releaselends`), which every child eventually gets.

**Interposition.** A child must see the bytes the shell read ahead
first, and must not have a byte stolen by a helper read in flight. So
if the object has read-ahead, a pending end of file, or a read in
flight (after trying to cancel an abandoned pipe read), the child gets
an interposed pipe (`Interpose`, created with room for the read-ahead
plus 64 KiB): the read-ahead is written into it at once, and a pump
(`H_PUMP` on a helper, started when the read in flight ends) copies the
object into it. Children spawned meanwhile share it. When the last
exits (`droplend`, `stopinterpose`): the pump is told to stop, the
shell drains the interposed pipe into the read-ahead until the pump is
no longer writing (the pump announces `P_WRITING` before its last look
at `stop`, so the shell never sees `P_READING` with a write still to
come), cancels the pump's source read where possible, and drains once
more. Bytes the pump reads after the stop come back with its
completion, after the drained ones. Nothing is lost or consumed twice.

**Reads** (`os_read`, the single-task fast path): read-ahead first,
then whatever the pipe holds; with nothing there and the object idle
and not lent: on Vista+ a blocking `ReadFile` on the main thread
(`mainread`), which Ctrl-C cancels (the control handler retries
`CancelSynchronousIo` while `Os.mainio` is odd and unchanged, the
ticket protocol that keeps a late cancel from hitting the next read);
on XP a helper reads the first byte while the main thread waits on the
wake event, returning -1 if a signal comes. Consoles are read a line at
a time on the main thread (`conread`): a Ctrl-C typed at the console
ends the read with `ERROR_OPERATION_ABORTED`, and the shell waits up to
100 ms for the control handler's thread to note the signal.

**Asynchronous operations** (`os_submit`/`os_wait`/`os_cancel`): an
`Io`'s place is in `plt[0]` (`S_ACTIVE`, `S_QUEUED`, `S_DONE`,
`S_PROC`, `S_ZERO`), its object or wait in `plt[1]`, the lists linked
by `Io.next`. Completed Ios wait on `Os.done` (FIFO) for `os_wait`.
`Os.nwait` counts the Ios that will complete by themselves; `os_wait`
with no deadline returns null when it is 0 (the core's deadlock).
Disk files and console output complete at once.

**Process exits**: `IO_PROC` registers a thread pool wait
(`RegisterWaitForSingleObject`, `WT_EXECUTEONLYONCE |
WT_EXECUTEINWAITTHREAD`) whose callback pushes the `Pwait` on the wake
queue. `os_cancel` unregisters it (blocking until a running callback
ends); one that already fired is left for `procwaited` to discard.
`os_waitproc` waits on the process and the wake event together, so
helpers and interpositions keep going.

**The wake queue** (`Os.wakeq`) is a lock-free stack (CAS push from any
thread, one exchange by the main thread, reversed to FIFO); the wake
event is auto-reset, and the main thread always drains the queue and
checks the signal bits before waiting again.

## Signals and Ctrl-C

**Platform.** `SetConsoleCtrlHandler(onctrl)`: Ctrl-C is SIGINT,
Ctrl-Break SIGQUIT, closing the console SIGHUP (the handler then
sleeps, as the system ends the process about 5 s later anyway). They
go to `Os.siggrp`, read by the new `os_siggroup`; `os_sigpending`
(`Os.sigown`) has nothing yet (M5b's signal channel). A process started
with Ctrl-C ignored (`SetConsoleCtrlHandler(0, 1)` in its parent, which
the shell does for `trap '' INT` and background jobs) has bit 0 of
`RTL_USER_PROCESS_PARAMETERS.ConsoleFlags` set, read from the PEB, and
reports INT in `OsInfo.sigign`; the system then never calls the handler
for Ctrl-C. A shell that dies of SIGINT exits with `0xC000013A`
(`STATUS_CONTROL_C_EXIT`), which native shells, Cygwin and unabashed
itself take for death by SIGINT; other signals exit 128+N.

**Group signals in the core** (`ossignals`, trap.cpp, called from
`schedule()` when `os_wait` returns early, from `checksigs` and from
`sigtaken`). Own signals go to the outermost level as before. A group
signal goes, like the terminal's SIGINT to bash's whole foreground job,
to every *running* level (`Sigproc` whose Shell runs it and which is
not waiting on a program): fiber stages, the innermost synchronous
subshell, background vprocs (which ignore INT without job control).
A level busy with a foreground child (an outer level during a
synchronous subshell, `Sigproc.fgwait` while waiting on a program or on
a pipeline's stages) only notes it (`Sigproc.waitsig`).

**The cooperative rule** (`waitdone`): when the child finishes, the
level acts on the noted signals the child died of, and drops the rest,
assuming the child handled them (bash's `wait_sigint_received` and
`child_caught_sigint`). On Windows a program's nonzero status also
counts as death by INT (plan §2: a native program may end any way it
likes on Ctrl-C). Applied after programs (`runprogram`), synchronous
subshells and scripts without `#!` (unless the level is a pipeline's,
which decides once all stages are joined), and pipelines (any stage
that died of it, or a failed program stage). Signals taken before the
wait starts are the level's own.

**Ctrl-C before SIGPIPE** (`sigpipe`, shell.cpp): a stage acts on
Ctrl-C only at a safe point, so its reader could die of it first and
leave it to die of the broken pipe on its next write (005 under load:
"130 141 130"). Bash's terminal signals every stage at once. So before
a level dies of SIGPIPE it takes the platform's signals, and a pending
one whose default action ends the level (`sigfatal`) ends it instead.
A trapped INT does not: the level dies of SIGPIPE as a bash process
would. Shell-side only; a program's status is its own. The unit test
`test_sigpipe` (main_test.cpp) reproduces the race on the vos with
`kill -INT`; it is not a spec, as bash itself races there when the
signal comes from kill (it gave 141 once in twenty runs under load).

**Interactive shells** (`-i`; the shell is not yet interactive by
itself on a terminal): PS1 (default `\s-\v\$ `) and PS2 are shown on
standard error, expanded as `${PS1@P}` (`showprompt`, `Lexer.newunit`
tells a command's first line from its continuations). At the outermost
level INT abandons the command line (`ESC_INTR`, caught by the reader's
`runcommand`, status 130), at the prompt it discards the line typed,
prints a newline and shows a new prompt (`readstdin`); TERM and QUIT do
nothing there (`signoop`). Subshells die of INT as in scripts.

Also fixed on the way: trap actions keep PIPESTATUS as they keep `$?`
(bash saves and restores it around every trap); signals ignored on
entry are listed by `trap` as ignored, as bash does.

## The read builtin

`read` takes input a line at a time wherever that consumes nothing past
the delimiter (and `-n`/`-N` counts): a regular file in blocks, seeking
back to just past the delimiter as bash does (all platforms); a pipe
the platform can look into (`os_peek`: Win32 pipes and console
read-ahead, vos pipes) through the delimiter exactly. Peeks start at
128 bytes and double while lines are longer. Otherwise a byte at a
time, as before. 500,000 lines from a file on Windows: 9.1 s before,
2.7 s now.

## Platform API additions

```
static u64 os_siggroup(Os *);                  // group signals since the last call
static iz  os_peek(Os *, Handle, u8 *, iz);    // look without consuming; -1 if unsupported
```
POSIX: `os_siggroup` returns 0 (a terminal's SIGINT arrives as the
process's own: with no handler installed it ends the process, as it
ends bash), `os_peek` -1. The vos: `os_siggroup` 0, `os_peek` on its
pipes, so specs exercise the peek path.

## Testing

`t/win32/run [-s SHELL] [-x] [-u] WORKDIR [TEST.sh...]` (documented in
tools/README.md): each test runs inside `tools/sandbox`, optionally
under `tools/conrun` (a hidden console it types into and sends Ctrl-C
to), output compared with `NAME.out`. `-x` repeats every test with
`UNABASHED_XPIO=1`, a test-only switch read at startup that disables
`CancelSynchronousIo`, so the XP code paths (helper-waited blocking
reads, abandonment, interposition) run on Windows 11 too.

| test | covers |
|---|---|
| 001-pipes | exact consumption between the shell and programs, captures, here-documents, full pipes, mixed stages |
| 002-read-timeout | read -t on an empty pipe: no byte lost to the shell or a child (drain-back), `read -t 0` while a read is in flight, closing after a timeout, TMOUT |
| 003-waits | 70 and 66 concurrent background programs, `wait`, `wait -n`, `wait PID` |
| 004-ctrlc-loop | Ctrl-C ends a busy script with SIGINT (status 130) |
| 005-ctrlc-pipeline | Ctrl-C reaches every stage; a trap runs once, the script goes on |
| 006-ctrlc-child | the cooperative rule: child handled it (0), died of it, failed (Windows rule) |
| 007-ctrlc-wait | a typed line, then Ctrl-C ends `wait`; the background program ignores it |
| 008-interactive | `-i`: prompts, Ctrl-C back to the prompt with 130, a trap in an interactive shell |
| 009-console | UTF-8 console input, read -t on a console losing nothing, Ctrl-Z as end of file |
| 010-writes | concurrent shell writers into a full program pipe, SIGPIPE status |

Results: all 10 pass on Windows 11 in both modes (20 runs) and on the
XP laptop (`~/src/scratch/m3b`, unabashed32.exe; sandbox without
integrity levels there).

`conrun`'s Ctrl-C is `GenerateConsoleCtrlEvent`, which does not end a
console read in progress as a typed Ctrl-C does, so the tests send it
only while the shell runs commands. The keyboard was checked by hand
with winctl in XP's console window (`winctl run unabashed.exe -i`,
`type`, `key ctrl+c`, `shot`): `f() { while :; do echo x; done; }; f |
while read l; do :; done` then Ctrl-C gives the prompt and `$?` 130;
Ctrl-C in `read v` gives the prompt and 130; Ctrl-C after typing part
of a line gives a new prompt and 130; Ctrl-C during `sleep 5` gives the
prompt and 130. The same four checks pass on Windows 11's conhost
(after M4), with the shell started from Explorer through a batch file:
a shell started by winctl from Claude Code's shell inherits the
"ignore Ctrl-C" console flag, under which neither unabashed nor cmd's
ping sees a typed Ctrl-C at all (an M6 item in notes/issues.md).

## Numbers at the end of M3b

- vos: t/parse 2915 tests, 0 failed; t/exec 560 specs, only
  213-trailing-backslash fails; `--chaos 3` the same; oracle 49 of 86
  run-* (minimal excluded), as at the end of M3a.
- Linux (WSL): the same suites pass; warning-free; bench/run against
  bash 5.3: b1 0.80 vs 2.49 s, b2 1.47 vs 23.6, b7 0.08 vs 25.2, b8 0.12
  vs 8.62, b9 0.80 vs 4.87, b10 0.04 vs 0.36, b12 1.49 vs 16.6 (no
  change from M3a beyond noise).
- Windows: t/win32 10 of 10 on Windows 11 (x64 in both code paths,
  i686) and on XP (i686).

## Decisions to revisit

- **Ctrl-C ignored at the top level still interrupts**: the platform
  always records Ctrl-C and the core drops it where INT is ignored; a
  subshell that sets its own INT trap under `trap '' INT` gets it, as in
  bash.
- **Windows cooperative rule**: a program that handles Ctrl-C and exits
  nonzero kills the script (bash on Linux would go on, as the child did
  not die of SIGINT). The plan asked for this, since native programs
  report Ctrl-C deaths inconsistently (busybox's applet stubs exit 0 or
  130, MSVCRT programs `0xC000013A`).
- **Ctrl-Break is SIGQUIT** (the terminal's `^\`): it kills a script and
  does nothing at an interactive prompt.
- **Exit code `0xC000013A`** for a shell killed by SIGINT, rather than
  130, so that parents of every kind see a death by Ctrl-C.
- **Console reads are never cancelled**, at the price of an abandoned
  read lingering (read -t on a console, or a Ctrl-C event that was not
  typed): a program started meanwhile gets an interposed pipe for its
  input instead of the console.
- **Interactive mode needs `-i`**: detecting a terminal on standard
  input and error is left for M6.

## Known limits

- A Ctrl-C event from `GenerateConsoleCtrlEvent` (not typed) does not
  end a console read in progress; the shell acts on it when a line
  arrives.
- Ctrl-C arriving while `CreateProcess` runs counts as the level's own
  signal, so the shell dies after the program finishes even if the
  program handled it.
- A child that held a pipe is released when the core observes its exit;
  a background program whose waiter was dropped at shell exit keeps its
  hold (irrelevant then).
- A helper stuck in an abandoned read on a pipe nobody writes (XP, or a
  console) stays stuck until the process exits; at most 64 helpers,
  after which reads block on the main thread as before M3a.
- With a background program holding the shell's input pipe, the
  shell's own reads go through helpers and race with it (as in bash);
  with an interposition active, the shell's reads wait until its
  children exit.
- POSIX: a terminal's SIGINT is the process's own signal (no SA_SIGINFO
  `si_code` check yet), so a trapped INT at the top level does not
  reach a running subshell level.
- `read -p` and `-s` still never apply (no terminal detection yet).
- Programs still cannot open `/dev/fd/N` on Win32 (M5a).
