# M3a: concurrency

The working plan for M3a, refining notes/plan.md §2 and
notes/design/synthesis.md §3.1. M3a replaces M2's stage-by-stage
pipelines with concurrent execution. This file documents the
foundation (scheduler, in-memory pipes, fiber vprocs, the virtual OS's
processes, the platform API) and the plan for the remaining M3a
features. Later agents must use the APIs below and keep this file up
to date.

## Status

Done (core foundation):
- `sched.cpp`: the scheduler, tasks, in-memory `Pipe`s, task I/O, pumps,
  safe points, chaos mode, deadlock recovery.
- `proc.cpp`: the copier (`vproccopy`), fiber vprocs (`spawnvproc`),
  concurrent pipelines (`runpipeline`), external stages started
  directly (`spawndirect`).
- `exec.cpp`: `runprogram` materializes shell objects for externals
  (pumps); safe points in simple commands, loops and function calls.
- `vos.cpp`: processes on fibers, blocking 64 KiB pipes, SIGPIPE for
  fakes, `os_submit`/`os_wait`/`os_cancel`, deadlock reports, chaos.
- Platform API (`api.cpp`): `Io`, `os_submit`, `os_wait`, `os_cancel`,
  `OsInfo.chaos`, `E_DEADLK`; implemented on the vos, POSIX and Win32.
- `fakes.cpp`: `yes`.
- PIPESTATUS, pipefail, lastpipe, `time` and `!` on pipelines, SIGPIPE
  statuses (see "PIPESTATUS and SIGPIPE" below); tests `t/exec/612`–`615`.
- Tests: `t/exec/607`–`611`; `test_sched` unit tests (pipes, joins,
  EPIPE, deadlock recovery); `tests.exe --chaos N` reruns every spec
  with N chaos seeds, comparing stdout, stderr and status (a spec
  containing `# nochaos` is exempt: its stages race on one stream);
  `tests.exe --run FILE [SEED]`.
- Process substitution, materialization of here-documents, in-memory
  pipes and captures for programs, `wait` for procsubs (see "Process
  substitution" and "Materialization" below); `os_tempfile` on every
  layer, `/dev/fd/N` in the vos; tests `t/exec/616`-`621`.
- Signals and traps (`trap.cpp`, see "Signals and traps" below): the
  trap and kill builtins, EXIT/ERR/DEBUG/RETURN, signals to virtual
  processes, the vos, POSIX and Win32 layers, subshell trap rules,
  per-spawn ignored signals, reports of deaths by signals, SIGPIPE
  dispositions, wait and read interrupted; tests `t/exec/622`-`630`.

Remaining M3a work (see "Plan for the rest of M3a" below): launcher
fibers for external stages with comsubs, fork-on-exec, interim fiber
`&`/wait/coproc, the isolate fuzzer, timers.

## Model

One OS thread per shell process. Everything that runs concurrently in
that process is a **task** (`Task`, sched.cpp), and every task is a
fiber:
- a **fiber vproc** runs shell code with a private copy of the shell
  state (proc.cpp);
- a **pump** copies bytes between an in-memory `Pipe` (or a string) and
  an OS handle, so that an external program can be given a shell
  object (sched.cpp, `Pump`).

The plan allowed pumps as non-fiber state machines. They are small
fibers instead (256 KiB stacks, pooled), which keeps one blocking
model for everything; a state-machine form can replace them later
without changing callers.

The scheduler (`Sched`) is per shell process and shared by all of its
vprocs (`Shell.sched`). It is single-threaded and FIFO:
- `s->cur` is the running task; `s->main` is the thread's own fiber.
- A task blocks on: a `Pipe` (wait lists `rwait`/`wwait`, `W_PIPE`), an
  OS operation (`Io`, `W_IO`), or another task's completion (`W_JOIN`).
- `schedule()` runs on the blocking task's own stack: it dequeues the
  next runnable task and switches to it; when the queue is empty it
  calls `os_wait(WAIT_FOREVER)` and wakes the task whose `Io` completed.
- Finished tasks are not freed: their fibers wait on a free list
  (`freebig`/`freesmall`) and are reused by the next `taskspawn` of the
  same stack class. `freesched` releases them when the shell exits.

**Fast path.** While `s->ntasks == 1`, `taskread`, `taskwrite` and
`taskwaitproc` call `os_read`, `os_write` and `os_waitproc` directly:
a plain script never touches the queue. A safe point costs a decrement
and a branch (`safepoint`): benchmarks b1 and b9 are unchanged.

**Safe points:** `runsimple`, every loop iteration (for, arithmetic
for, while/until, select) and `callfunction`. When the budget (1024)
runs out, `safepointslow` polls completed operations
(`os_wait(WAIT_POLL)`) and yields if another task is runnable.

**Chaos mode** (`OsInfo.chaos` seed, test layers only): the run queue
is dequeued at random, safe points yield at random with tiny budgets,
pipe reads and writes move random amounts and may yield, and the vos
picks the next process at random. Output per writer must not depend
on the seed.

**Deadlock.** When no task is runnable and no `Io` is pending, or the
vos's `os_wait` returns null (nothing can ever complete), `deadlock()`
writes `unabashed: deadlock: every task is blocked` to the process's
original stderr, then fails every blocked operation (pipe waits return
-1/`E_DEADLK`, pending `Io`s are withdrawn with `os_cancel` and fail
with `E_DEADLK`) so that the script carries on and the test shows the
message. The vos separately reports `vos: deadlock` in the captured
stderr when no vos process can run.

## Invariants

1. **No store memory is shared between live vprocs.** A fiber vproc has
   its own `Store` (in a pooled `Vproc`), its own scratch pool
   (`Shell.pool`), its own `Fdtab` and `File`s. `File`s are per store:
   another vproc gets its own `File` for the same object (`copyfile`):
   a duplicated OS handle (`os_dup`), the same `Pipe` with its reader or
   writer count raised, a copy of `F_MEM` text, or the same capture
   `Buf` (see 4).
2. **Code blocks are shared by reference count.** `Code.store` is the
   owning store. `Code.refs` counts calls in progress plus the function
   tables of other vprocs that hold it (`copyfuncs` increments,
   `releasefuncs` at vproc exit and `releasecode` decrement). `dead`
   means the owner's table dropped it. Whoever drops the last reference
   of a dead block frees it into its owner's store (`freecode`).
   `releasecode` ignores a `Func` below the current subshell's
   watermark: rollback restores it, so neither `dead` nor `refs` may
   change (this also fixed M2 setting `dead` from inside a subshell).
3. **Every task is joined by its creator** (`taskjoin`) before the
   creator's scratch is reset, so a fiber vproc may point into its
   parent's memory only for things the parent cannot free before the
   join: function bodies (refcounted anyway) and capture buffers. The
   stage's own AST is copied into the vproc (`copynode`). This holds
   while every fiber is joined within the command that created it; an
   interim fiber `&` breaks it and must copy everything it references
   (it already does, except capture `Buf`s, which a background job must
   not inherit: give it the capture as an OS pipe plus pump instead).
4. **A capture `Buf` may be written by several vprocs** (`F_CAPTURE` in
   a fiber's fd table). This is safe because a `print` to a `Buf` never
   yields, and the capture outlives every writer: the capturing
   subshell joins its pipeline before reading the `Buf`.
5. **Escapes never cross fibers.** A fiber vproc's top `Escape` is in
   `vprocmain`; `vproccopy` clears `esc`. `exit`, fatal errors and
   virtual SIGPIPE in a stage longjmp only within its fiber, which then
   closes every descriptor (EOF or EPIPE for its neighbors), drops its
   code references and returns its memory to the pool. Under lastpipe
   the last stage runs in the parent's shell: `runpipeline` catches any
   escape there, joins all stages, then re-raises.
6. **Pipe ends are counted per `File`, not per descriptor.** Closing the
   last `File` reference of an end calls `pipeclose`, which wakes the
   other side: readers see EOF when `writers` reaches 0; writers get
   `E_PIPE` when `readers` reaches 0.
7. **The parent holds no pipe end it does not use.** `runpipeline`
   creates all pipes, spawns the fibers (which take their own copies),
   then drops its references except the last stage's input, which the
   last stage's descriptor 0 takes over and releases when it ends.
8. **One `Io` per task**, embedded in the `Task` (`t->io`). An `Io` stays
   in place until it completes or is withdrawn.

## Core API (for later agents)

Scheduler (sched.cpp):

| Function | Use |
|---|---|
| `Task *taskspawn(Sched *, void (*fn)(Task *), void *arg, iz stack)` | start a task (`BIGSTACK` for shell code, `SMALLSTACK` for pumps); it runs once the current task blocks or yields |
| `i32 taskjoin(Sched *, Task *)` | wait for it, return `t->status`, recycle it; every task must be joined exactly once |
| `void taskcancel(Sched *, Task *)` | make its next (or current) `Pipe` wait fail; used to stop input pumps |
| `void yield(Sched *)` / `void safepoint(Sched *)` | give way |
| `iz taskread(Sched *, Handle, u8 *, iz)` | 0 EOF, -1 error |
| `i32 taskwrite(Sched *, Handle, u8 *, iz)` | 0, or `E_PIPE`/`E_*` |
| `i32 taskwaitproc(Sched *, Handle proc)` | exit status |
| `Pipe *newpipe(Sched *)` | one reader, one writer |
| `iz piperead(Sched *, Pipe *, u8 *, iz)` / `i32 pipewrite(...)` / `void pipeclose(Sched *, Pipe *, b32 writer)` | |
| `Task *spawnpump(Sched *, Pump *)` | `PUMP_TOHANDLE` (Pipe → handle), `PUMP_FROMHANDLE` (handle → Pipe), `PUMP_TEXT` (string → handle, unused now), `PUMP_TOBUF` (handle → capture); the pump owns `h` and its pipe end |
| `Pump *newpump(Sched *)` / `void endpump(Sched *, Task *)` | a pooled Pump that outlives its starter's arena / cancel, join and recycle it |
| `Handle pipeosread(Sched *, Pipe *)` | give a Pipe its shared OS form for programs (Materialization) |

Shell I/O never calls `os_read`/`os_write` directly: `fileread` and
`fdwrite` (shell.cpp) route `F_OS` through `taskread`/`taskwrite` and
`F_PIPE` through `piperead`/`pipewrite`. A write failing with `E_PIPE`
calls `sigpipe()`: the vproc ends with status 141, as a process killed
by SIGPIPE (traps, and `trap '' PIPE` turning it into a write error,
are still to do).

Vprocs (proc.cpp):

| Function | Use |
|---|---|
| `Task *spawnvproc(Shell *, Node *, File *in, File *out, b32 newloops)` | run `node` concurrently in a subshell of `sh` (a deep copy), with fd 0/1 replaced when `in`/`out` are given; join the task for its status |
| `Shell *vproccopy(Shell *, Vproc *)` | the copier, if a feature needs a vproc with a different entry point (procsub, `&`, coproc) |
| `Vproc *vprocwith(Shell *, File *in, File *out)` | a copied vproc not yet started: set `node` or `src`, then `taskspawn(s, vprocmain, m, BIGSTACK)` |
| `Str procsubst(Shell *, Arena *, Str body, b32 output)` | start a procsub, return `/dev/fd/N` |
| `void joinpsubs(Shell *)` / `i32 waitpsub(Shell *, i64 pid, b32 *found)` | join this level's procsubs / one by pid |
| `Handle capturewriter(Shell *, File *cap)` | an OS write handle into a capture, pumped (exec.cpp) |

Rules for code that runs in a vproc: allocate only from the vproc's
own arenas (`sh->store`, `scratchat(sh, …)`), never keep pointers to
another vproc's store, and never block except through the functions
above.

`BASH_SUBSHELL`: bash does not count a simple command run as a pipeline
stage. `Shell.simplestages` counts such stages and `BASH_SUBSHELL`
reports `subshell - simplestages`.

## Pipelines

`runpipeline` (proc.cpp):
1. Flatten the stages, left to right, along the parser's right-nested
   chain (`a | b | c` is `a | (b | c)` in the tree, but one pipeline);
   `$?` in every stage is the status from before the pipeline.
2. For each edge: an OS pipe if a stage next to it is a likely external
   (`likelyexternal`: a simple command whose command word is a plain
   literal naming no builtin or function), else an in-memory `Pipe`. A
   wrong guess only costs speed: shell code can use an OS pipe, and an
   external given an in-memory pipe gets an OS pipe plus a pump.
3. For every stage but the last, in order: a **direct** external
   (`directstage`: a likely external with no redirections, no `$(`,
   backquote or procsub in its words, found on PATH, and no inherited
   descriptor needing a pump) expands in a synchronous subshell with
   the stage's descriptors and starts without waiting
   (`Shell.spawnto` makes `runprogram` return the process); anything
   else becomes a fiber vproc, followed by a `yield` so that it gets
   going first, as a forked child does (this matches bash's usual
   xtrace order). A direct stage that turns out to be a script without
   `#!` falls back to a vproc. Then drop the parent's pipe references.
4. Run the last stage on the current stack: in a synchronous subshell,
   or in the shell itself under lastpipe (without monitor mode).
5. Close the last stage's input, join the stages right to left (fibers
   with `taskjoin`, direct externals with `taskwaitproc`). `$?` is the
   last stage's status, or with pipefail the rightmost nonzero.

`runprogram` (exec.cpp) gives every descriptor of an external an OS
object; see "Materialization".

## Materialization

What a program is given must behave as one shared open file would
under bash: what it leaves unread is there for the next reader, shell
or program, and what it writes comes before what the shell writes
next. `runprogram`, for every descriptor:
- `F_OS`: passed as is.
- `F_MEM` (here-documents, here-strings): converted **in place** to
  `F_OS` (`materialize`), as bash 5.3 makes them: an empty document is
  `/dev/null`; one of at most `HEREDOCPIPE` (64 KiB) that also fits
  `os_pipecap` is a pipe filled before the program starts (bash checks
  `F_GETPIPE_SZ` the same way); a larger one is an `os_tempfile`
  (deleted on close, mode 0400) seeked to the File's position. Since
  the File itself changes, builtins and later programs read on from
  where the program stopped (`{ head -c3; wc -c; } <<<"$big"`). Until
  then the text stays in memory, and `test` reports the type a program
  would see (`memfiletype`). A vproc's copy of an `F_MEM` File is a
  separate copy (invariant 1), so a pipeline stage consuming a
  here-document does not advance its parent's copy (bash's forked
  stage would): a known difference.
- `F_PIPE` read end: the **Pipe** gets an OS form (`pipeosread`): an
  OS pipe fed by a pump (`Pipe.osr`, `Pipe.pump`) from which every
  reader of that Pipe reads from then on (`piperead` switches to
  `taskread(osr)`), in every vproc. The pump ends at the Pipe's EOF, or
  is stopped (`endpump`) when the last reader File closes. This keeps
  `printf 'a\nb\n' | { ext; read x; }` giving `a`: the earlier
  per-call input pump moved data into a pipe that died with the call.
- `F_PIPE` write end: an OS pipe plus a `PUMP_FROMHANDLE` task joined
  after the program, so its output precedes the shell's next write.
- `F_CAPTURE`: an OS pipe read in place until EOF (every writer gone,
  including the program's own background children). A program not
  waited for in place (a direct pipeline stage, `Shell.spawnto`) gets
  `capturewriter`: an OS pipe whose `PUMP_TOBUF` pump appends to the
  capture. A capture's File points at a `Capture` (`Buf` first, plus
  its pumps); `runcapture` joins the pumps before taking the result
  (`x=$(... &)` waits for every writer), and `fdwrite` drains whatever
  they have received before the shell appends (`capturedrain`), so
  `$(exec 2>&1; ls /x | cat; echo after)` keeps its order. **For
  interim `&`:** a background vproc must not share a capture `Buf`
  (invariant 3); give its copy of the descriptor `capturewriter`'s
  handle as an `F_OS` File instead.

## Process substitution

`<(list)` and `>(list)` (`procsubst`, proc.cpp; expanded in
`expandinto` wherever the parser left `<(`, except in double quotes,
here-documents and arithmetic, so also in assignments and `${x:-...}`,
as in bash):
1. Pick a free descriptor from 63 down (bash's `move_to_high_fd`), else
   the lowest free from 64.
2. Make the pipe: an OS pipe if the body starts with a likely external
   (`likelyexternaltext`), else an in-memory `Pipe` (which a program
   later materializes, see above).
3. Start a fiber vproc (`vprocwith` + `Vproc.src`) that parses and runs
   the body text like a command substitution (`runsource` with
   "process substitution", `tracedepth+1` for `++` traces, `-v` off),
   with its fd 1 (or 0 for `>(...)`) on the other end. It does not
   yield first: as a forked child, it runs after the command started.
4. Open the shell's end at the descriptor with an **Undo record in the
   store** (`fdsetpsub`, `Undo.psub`), so it closes when the command's
   redirections unwind: `runsimple` now takes its undo mark before
   expanding words, and compound commands unwind theirs (`for` words,
   `done < <(...)`). Escapes unwind it too. `exec 3< <(f)` keeps fd 3
   but closes 63 (`closepsubs` before committing).
5. The word becomes `/dev/fd/N`, which `openfile` (redirections),
   `readwholefile` (`source`, `$(<...)`), `test` (`envstat`), and the
   vos's `os_open`/`os_stat` for programs (`/dev/fd/N`, `/dev/std*`,
   reopening a file as a new description as Linux does) resolve; POSIX
   children inherit fd N.

The vproc is not waited for at the end of the command (bash does not
either): it goes on its creator's list (`Shell.psubs`, linked by
`Task.sib`, pid in `Task.pid`, which is also `$!`). Finished ones are
reaped when the next procsub starts (after a `yield`, so that earlier
ones can finish as forked children would; otherwise a run of
non-blocking commands piles up vprocs). `wait PID` and `wait` (the last
one, if `$!` names it, as bash) join one (`waitpsub`; the last joined
status is remembered for a repeated `wait`). The end of the creating
level joins the rest once its descriptors are closed: a synchronous
subshell (`subshell()` saves and clears `psubs`), a fiber vproc
(`vprocmain`: `closeall` then `joinpsubs`), a script run without `#!`,
and `sh_main` (closing every descriptor but the platform's own). A
procsub in a redirection of an external does not set `$!` (bash
redirects in the forked child).

Bash's `-c` mode keeps procsub descriptors open across top-level
builtins until something forks, so its later procsubs there are
`/dev/fd/62`, `61`, ...; scripts (and functions and loops everywhere)
close them per command as unabashed does. Specs avoid printing such
numbers.
## PIPESTATUS and SIGPIPE

PIPESTATUS is a dynamic array (`DYN_PIPESTATUS`, vars.cpp) over
`Shell.pstat` (u8[256], exit statuses are 8 bits) and `Shell.npstat`,
so that setting it costs a few stores and no store memory. Being in the
Shell struct, subshells and command substitutions restore it with the
rest of the struct, and vprocs copy it.
- `setpstat1(sh)` (one status, `$?`) runs after every simple command
  (both `runsimple` call sites in exec.cpp, so functions and builtins
  report their own status, not their last pipeline's), `(( ))`, `[[ ]]`
  and `( )`; other compound commands, function definitions, `&` and a
  failed redirection on a compound command leave it alone, as bash.
- `setpstat(sh, statuses, n)` runs in `runpipeline` once every stage is
  joined, before pipefail and `!` change `$?` (`! true | false` gives
  `$?` 0 and PIPESTATUS `0 1`). Under lastpipe the last stage's own
  commands set it first, then the pipeline overwrites it.
- An assignment or unset of PIPESTATUS (`varchanged`) sets
  `Shell.pstatdirty`; the next `setpstat*` puts the dynamic binding back
  (`restorepstat`), so `PIPESTATUS=(9 8) x=${PIPESTATUS[1]}` sees 8 and
  the next command recreates it, as bash. `declare -a` and `readonly`
  keep it dynamic.
- Expansions that read an array's elements go through `varray`, so
  slices, `@A`, `@K`/`@k` work on dynamic arrays (FUNCNAME too).

SIGPIPE: a fake, a direct external or a shell stage writing to a pipe
without readers ends with status 141 and prints nothing: bash, built
with its default `DONT_REPORT_SIGPIPE`, reports no SIGPIPE deaths of
pipeline members (checked against the oracle for first, middle and last
stages). A shell stage ends through `sigpipe()` (shell.cpp) from
`fdwrite`; the code after the write in that vproc does not run
(`{ yes; echo "after $?" >&2; } | { read x; }` prints `after 141`
because `yes` is a separate program). With `trap '' PIPE` the write
fails instead ("write error: Broken pipe"), and programs started then
ignore SIGPIPE too; with a PIPE trap the write fails and the trap runs
at the next safe point.

## Signals and traps

`trap.cpp`. Numbers and names are Linux x86-64's everywhere.

**Trap table.** `Shell.traps` is null or `NTRAP` `Str`s in the store:
index 0 EXIT, 1-64 signals, then `TRAP_DEBUG`, `TRAP_ERR`,
`TRAP_RETURN`. Null data is the default, empty (non-null) data ignores.
`settrap` changes one; at the top level (`subshell == 0`) it also sets
the process's disposition (`os_sigmode`: caught when trapped, or when an
EXIT trap must run on a fatal signal). Signals ignored on entry
(`OsInfo.sigign`, `Shell.hardign`) cannot be trapped.
- Subshells (`subshell()`, `vproccopy`): `subshelltraps` gives a new
  table with caught signals reset and ignored ones kept (ERR with
  errtrace, DEBUG/RETURN with functrace); `Shell.shown` keeps pointing
  at the parent's table, which `trap`/`trap -p` list until the subshell
  changes any trap (bash 5.3). Sync subshells allocate above the store
  mark, so rollback frees it; vprocs copy (`copytraps`).
- A program started gets `Spawn.sigign`: the signals the starting level
  ignores (`trap '' INT` applied per spawn, never process-wide). POSIX
  sets them to SIG_IGN in the child and resets the others the shell
  changed; Win32 handles INT by `SetConsoleCtrlHandler(0, 1)` around
  `CreateProcess`; the vos makes the fake ignore them.

**Virtual processes.** Every level that can receive a signal has a
`Sigproc` (sched.cpp) on `Sched.sigprocs`: the shell (`Sched.top`), each
synchronous subshell (on `subshell()`'s stack), each fiber vproc (in its
`Vproc`, registered from creation, so `kill $!` reaches a procsub that
has not run yet). `kill PID` looks there first (`$$` is `top`, as bash
signals the parent process), then `os_kill`. Real signals arrive through
`os_sigpending` and go to `top`. A level takes its pending signals only
when it runs (`checksigs`): `top` while a sync subshell runs waits for
it, as bash takes a signal once its foreground child is done.

**Safe points** (`checksigs`): after each simple command (`runsimple`
wraps `runsimple1`), after `( )` and pipelines, before the EXIT trap.
`runsigs`: ignored: dropped; trapped: `runtrap`; default and harmless
(CHLD CONT URG WINCH and the stop signals, since stopping is not
modelled): dropped; else `sigdie`: the EXIT trap (not for KILL), then
`ESC_EXIT` with `Shell.killsig`, status 128+N. A trap may run inside
another, even its own (bash recurses too).

**Interrupting waits** (`taskinterrupt`, sched.cpp). `sigsend` to a
blocked task: a signal that will kill the level fails any pipe or OS
wait (`all`); a trapped one only the waits of a `read` or `wait`
builtin (`Task.introk`). The wait returns an error with `Task.intr`
set; `fileread`/`fdwrite`/`runprogram` call `sigtaken`, which kills the
level now if it should die; then a read resumes, after running the trap
actions at once if it is the read builtin's (`Shell.intrread`; bash runs
them during read, which goes on reading). `waitpsub` uses
`taskjoinintr` and returns 128+N, the trap running after wait. When
`os_wait` returns early for a signal, `schedule()` hands it to `top` and
interrupts a `read`/`wait` there.

**Trap actions** (`runtrap`): parsed and run like eval in a fresh pool
arena (`capdepth+1`), keeping `$?` (unless the action returns from a
function: `return` in a trap returns from the function), LINENO and
BASH_COMMAND (simple commands in actions do not change it). `exit`
without a status in an action uses the status from before it
(`Shell.trapstatus`); so does `return` without a status in the action
itself (not in a function it calls, `Shell.trapfuncs`). BASH_TRAPSIG is
bound (unexported) to the trap's index while it runs (EXIT 0, DEBUG 65,
ERR 66, RETURN 67, as bash 5.3). ERR runs from `checkerrexit` where set
-e would act; DEBUG before simple commands, `(( ))`, `[[ ]]` and each
`for` iteration (BASH_COMMAND the header); RETURN at the end of a
function or a sourced file. In a function they run only if the function
set them (`Frame.trapat` records them at the call) or with functrace
(errtrace for ERR); `pseudotrap`. None of these runs inside its own
action (`Shell.trapbusy`); DEBUG does run inside other actions.

**EXIT** (`runexittrap`): at `sh_main`'s end, at the end of a sync
subshell or vproc whose own table has one, and from `sigdie`. Not after
exec (`Shell.execed`). After set -e (`Shell.errexited`) the abandoned
commands' redirections are undone first (`unwindtemp`, keeping exec's
in a subshell, `Undo.keep`); after `exit` they stay in force, as in
bash. A script without `#!` run in-process (`runexternal`) starts as a
new shell: what its parent ignores is ignored on entry, caught traps
reset, only its own listed; it runs its own EXIT trap.

**Deaths by signals.** `os_waitproc` and `IO_PROC` return 128+N with
`WSIGNALED`; `sh_main` returns that too when the shell dies by a signal,
so POSIX re-raises it, fake_bash passes it on and the test harness
masks it. `runprogram` turns it into `Shell.childsig`/`childpid`; a
sync subshell that dies sets them in its parent; vproc task statuses
carry `WSIGNALED`. Reports (`reportsig`, `reportpipeline`) follow bash:
none for INT and PIPE, nor inside command or process substitutions
(`Shell.noreport`); SIGTERM, or a signal the reporting shell traps or
ignores, gets the short form (description padded to 27, the command),
others `NAME: line N: PID description command`; a pipeline reports only
when its last stage died, one line per stage in the long form. The
last simple command of a `( list )`, a simple pipeline stage, or the
last of a `( list )` stage is what bash execs, so its death is the
subshell's, reported by the parent with the subshell's text
(`tailcommand`, `Shell.tailnode`/`taillevel`).

**Platforms.** API: `os_kill`, `os_sigpending`, `os_sigmode`,
`Spawn.sigign`, `Spawned.pid`, `OsInfo.sigign`, `WSIGNALED`.
- vos: processes have pids (from 10000; the outer shell 2, nested
  shells their own). Shells get every signal as a pending bit and are
  woken from a pipe read or `os_wait` (`VProc.intr`), never from a write
  or `os_waitproc`; fakes die (128+N) unless the signal is harmless or
  in their `sigign`. `os_sleep` blocks in virtual time (`VW_SLEEP`), so
  that a sleeping program can be killed; when nothing can run the clock
  jumps to the earliest sleeper (`vadvance`), which also means sleepers
  never look like a deadlock.
- POSIX: `os_sigmode` installs `sigaction` (SA_RESTART) handlers that
  set bits and write the self-pipe; `os_wait` returns null once bits
  are set; `os_kill` is kill(2).
- Win32: `os_kill` is TerminateProcess with exit code 128+N; nothing
  arrives through `os_sigpending` yet (Ctrl-C and the signal channel
  to our own shells are M3b/M5b); crash codes carry `WSIGNALED`.

The trap oracle (`run-trap`, which prints a binary diff: run it with
`diff -a` to see) differs only in `declare -ft` and SIGCHLD traps of
background jobs.

Not done: job specs (`kill %1`: no job table yet), `wait -n` and
signals during `read -t` (timers), Ctrl-C on Win32, extdebug, the
`declare -t` trace attribute, SIGCHLD traps, bash's quirk that a
subshell resetting an inherited ignored SIGQUIT still ignores it. The
execscript oracle differs only in the order of trap listings
(exec.right was made where SIGUSR1 > SIGTERM), getconf (no fake) and
coproc.

Bash 5.3 oddity, not copied: run as a script file, `shopt -s lastpipe;
echo e | exit 7` exits 0; with `-c` (how specs run) it exits 7, which is
what unabashed does.

## Platform API additions

```
struct Io { i32 op; Handle h; u8 *buf; iz len; iz n; i32 err; b32 done;
            void *ctx; Io *next; uz plt[4]; };
IO_READ   at least one byte, or EOF (n = 0)
IO_WRITE  at least one byte (n), perhaps fewer than len
IO_PROC   the process's exit; n is os_waitproc's status; releases it
void os_submit(Os *, Io *);        // may complete at once (done)
Io  *os_wait(Os *, i64 deadline);  // WAIT_FOREVER, WAIT_POLL or an os_now time
b32  os_cancel(Os *, Io *);        // false if it may still complete
```
- **vos**: an `Io` completes when the vos pipe has data/room, the file
  is a regular file, or the child exited. `os_wait` blocks the calling
  vos process; at `WAIT_POLL` it lets the other processes run once.
  Null with `WAIT_FOREVER` means deadlock.
- **POSIX**: `poll` on the pending descriptors plus a self-pipe written
  by the SIGCHLD handler, then one `read`, or one `write` of at most
  `PIPE_BUF` bytes, which cannot block once poll reports room; exits by
  `waitpid(WNOHANG)`. `O_NONBLOCK` is set only on the self-pipe. The
  shell ignores SIGPIPE so that a broken pipe in an in-process subshell
  is an `EPIPE` (status 141), and children get SIGPIPE back at its
  default unless it was ignored on entry.
- **Win32** (the M3a simple approach; M3b replaces it): process exits
  through the process handle; a pipe read completes at once when
  `PeekNamedPipe` reports data or a broken pipe; a pipe write completes
  at once when `NtQueryInformationFile(FilePipeLocalInformation)`
  reports write quota; otherwise a pooled helper thread performs the
  blocking `ReadFile`/`WriteFile` and signals an event. Disk files and
  console writes complete at once; console reads go to a helper.
  `os_wait` is one `WaitForMultipleObjects` over process handles and
  helpers' events (at most 64). `os_cancel` refuses an operation a
  helper owns. With one task the shell blocks directly, as before.

## The virtual OS

- Slot 0 of `Vos.procs` is the outermost shell; spawned programs get
  slots from 1, released by `os_waitproc` or a completed `IO_PROC`.
  `v->cur` is the running process. Each spawned process runs on its own
  fiber (1 MiB, or 32 MiB for nested shells), with its own 4 MiB memory
  block and copies of its arguments.
- A process runs until it blocks; `vschedule` then picks the next
  runnable process round robin (randomly under chaos), resuming it on
  the fiber it blocked on. For a shell, that is whichever of its own
  task fibers entered the vos.
- Pipes hold `VPIPECAP` (64 KiB). Reads block while empty with writers
  left; writes block while full with readers left. A fake writing to a
  pipe without readers dies with status 141; a shell gets an error.
- When nothing can run, the blocked call fails (`-1`, write failure, or
  the waited-for process is killed with status 137) and `vos: deadlock`
  goes to the captured stderr.
- Nested shells (`fake_bash` in main_test.cpp) release their heap
  share when they exit only if nothing reserved meanwhile by another
  process outlives them (`HeapMark`, `Os.pinned`).

## Plan for the rest of M3a

In suggested order, each with the API to use:
1. **Launcher fibers**: an external stage whose words contain comsubs
   or procsubs runs as a full vproc today (correct, only slower). A
   launcher would copy less state, or the copier could become
   copy-on-first-write (M8). The last stage, when external, could also
   be started directly instead of in a synchronous subshell.
2. **PIPESTATUS**: done (see above).
3. **Process substitution**: done (see above), except on Win32 for
   programs (Known limits).
4. **Here-documents and captures for externals**: done (see
   "Materialization"); `&` must use `capturewriter`.
5. **Traps and SIGPIPE**: done (see "Signals and traps").
6. **Interim `&`, wait and coproc** as fiber vprocs: `spawnvproc` with a
   job table entry instead of a join; `wait` joins. These outlive the
   command, so see invariant 3.
7. **Fork-on-exec** (POSIX): `exec ext` while other tasks are alive
   forks so that `$$` is kept; otherwise `execve` directly.
8. **Timers**: `read -t`, `sleep` builtin, `wait -n` need a deadline in
   `schedule()` (a timer list in `Sched`, `os_wait(deadline)`); the vos
   advances its clock to the deadline when nothing else can run.
9. **Fuzzers**: chaos (same output for every seed, per writer) and
   isolate (`( S ); declare -p` equals `declare -p`), both vos only.

## Known limits

- Win32: a program cannot open `/dev/fd/N`, so `diff <(a) <(b)` and
  `cat <(f)` fail there ("No such file or directory"); builtins,
  redirections, `source` and shell code work. The fix belongs with
  M5a's argument conversion: give a native child a named pipe path
  (`\\.\pipe\unabashed-PID-N`) in place of the `/dev/fd/N`
  argument, bridged by a pump, which needs an `os_pipe(named)` whose
  first write waits for the client (`ConnectNamedPipe` in a helper,
  cancelled when the program exits without opening it).
- Each vproc reserves two arenas plus scratch pools (about 16 MiB of
  address space); the test harness's 128 MiB of reservations allows
  about a dozen live vprocs (procsubs and shell stages together), and
  more abort with "unexpected out of memory".

- Win32 helper threads issue whole-length reads; the File state machine
  and Peek-exact reads are M3b. A helper read in flight on a handle
  blocks other operations on that handle (critique M3).
- POSIX: if another process reads the same pipe as a shell task, a read
  after poll may block the whole shell until data arrives.
- The vos runs nested shells on fibers; a deadlock inside a nested
  shell's own scheduler is reported by that shell.
- A fiber stage's stack limit is its parent's (`stackmax`), at most the
  fiber stack (8 MiB reserved).
