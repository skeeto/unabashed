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

- Jobs (`jobs.cpp`, see "Jobs" below): `&` on fiber vprocs or as
  programs started directly, `$!`, the job table, `jobs`, `wait` (pids,
  job specs, `-n`, `-p`, `-f`), `disown`, `kill %N`, reports of jobs
  killed by signals, coprocesses (several, as bash 5.3), `fg`/`bg`/
  `suspend` without job control; timers in the scheduler and `read -t`
  (see "Timers" below); tests `t/exec/631`-`636`.

- Oracle pass (see "Bash test suite on the vos" below): category a
  green except for M4/M8 features, a bash bug and two quirk sets; chaos
  check (every t/exec spec, 20 seeds); benchmarks b8 and b12; tests
  `t/exec/637`-`645`.

- Exit criteria pass (see "Closing M3a" below): the chaos and isolate
  fuzzers (`main_fuzz.cpp`, `tools/fuzz`), fork-on-exec on POSIX, B12
  at 12x bash, and the arith and dbg-support oracles; tests
  `t/exec/646`-`654`.

Remaining M3a work (see "Plan for the rest of M3a" below): launcher
fibers for external stages with comsubs (an optimization).

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
   while every fiber is joined within the command that created it. A
   background job breaks it, so it copies everything it references, and
   its captures become OS pipes plus pumps (`detachcaptures`); its
   level's end hands it to the Sched as an orphan (see "Jobs").
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
| `b32 taskwaitany(Sched *)` | block until any task finishes (`wait -n`); false if a signal interrupted it |
| `Task.deadline` | an `os_now` time: the task's next pipe or OS wait fails there with `Task.timedout` set (see "Timers") |
| `b32 taskreadable(Sched *, Handle)` | could a read complete now, with data or EOF (`read -t 0`) |
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
   backquote, procsub, funsub or `@P` in its words (`runscode`), found
   on PATH, and no inherited descriptor needing a pump) expands in a
   synchronous subshell with the stage's descriptors and starts
   without waiting (`Shell.spawnto`, which `runsimple1` hands only to
   the command's own `runexternal`, makes `runprogram` return the
   process; programs the words run are waited for); anything else
   becomes a fiber vproc, followed by a `yield` so that it gets going
   first, as a forked child does (this matches bash's usual xtrace
   order). A direct stage that turns out to be a script without `#!`
   falls back to a vproc. Then drop the parent's references to that
   stage's pipes at once: words that run code `runscode` misses
   (`i='a[$(cat)]'; ${a[i]}`) may read the next stage's input while it
   expands, and must see EOF when the writers on its left finish.
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

Not done: Ctrl-C on Win32, extdebug, the
`declare -t` trace attribute, SIGCHLD traps, bash's quirk that a
subshell resetting an inherited ignored SIGQUIT still ignores it. The
execscript oracle differs only in the order of trap listings
(exec.right was made where SIGUSR1 > SIGTERM) and getconf (no fake).

Bash 5.3 oddity, not copied: run as a script file, `shopt -s lastpipe;
echo e | exit 7` exits 0; with `-c` (how specs run) it exits 7, which is
what unabashed does.

## Jobs

`jobs.cpp`; `Job`, `Bgstat`, `Coproc` are in shell.cpp. Interim until
M5b makes them real processes.

**Starting** (`runasync`, from `executeone`'s `&`): `a & b &` is two
jobs, `a && b &` and `a | b &` one, as bash. Without job control the
job's input is `/dev/null`, unless the level's input is a pipe or
redirected (`Shell.stdinredir`: pipeline stages after the first, and a
compound command redirecting fd 0, as bash's `stdin_redir`). INT and
QUIT are ignored in the job unless it traps them (`Shell.asyncign`,
folded into `trapignored`, so programs it starts ignore them too). Then:
- a direct external (`directbg`: as a direct pipeline stage, its output
  an OS handle or a capture) starts through `spawndirect` and gets a
  **waiter** task (`spawnwaiter`, SMALLSTACK) whose `IO_PROC` is
  submitted at once, so the program's end is seen even before the
  waiter first runs; `$!` is the program's pid;
- anything else becomes a background vproc (`spawnbg`): no jobs of its
  own, coprocess descriptors closed, captures detached, then a `yield`
  so that it runs until it blocks, as a forked child runs alongside its
  parent; `$!` is its BASHPID. `throttlejobs` makes `&` wait for one to
  finish while `MAXBG` (4) are live (`Sched.nbg`): the test harness
  reserves about 16 MiB per vproc.

**The table** (`Shell.jobs`, newest first, in the store; `curjob`,
`prevjob` follow bash's `set_current_job`/`reset_current` for running
jobs). Each Job has its owner's BASHPID: subshells see the parent's jobs
but cannot wait for them or change them. A command substitution shares
the parent's list (listing it, as bash); a vproc gets a copy
(`copyjobs`, no tasks); a `( list )`, a compound pipeline stage and a
background job start with none (`SubJob.nojobs`), as bash's
`without_job_control`. New job numbers are the highest + 1.
- `polljobs` notices ended jobs: it drains completed `Io`s, joins
  finished vprocs and exited waiters, and records the status (with
  `WSIGNALED`). Ends are noticed only there (jobs, wait, job creation,
  after each foreground program, before each top-level command), which
  keeps scripts deterministic on the vos.
- Bash's non-interactive notification rules: a job that ends stays,
  unreported, until `jobs` lists it or `wait` collects it (`notified`;
  `cleanupjobs` then drops it, keeping its status in `Shell.bgstats` for
  a later `wait PID`). Before each top-level command of the outermost
  `runsource` (`notifyjobs`), a job killed by a signal other than INT,
  TERM, PIPE or one this shell traps is reported on stderr (`NAME: line
  N:   PID Killed    cmd`) and dropped.
- `wait` with no arguments waits for the running jobs, drops them and
  the dead ones, except a dead job `$!` names that was not waited for,
  and forgets the saved statuses; then the last procsub as before.
- Orphans: when a level ends (sync subshell, vproc, the shell), its
  running jobs go to `Sched.orphans`, reaped when done; `sh_main` closes
  its descriptors and joins them before returning, but drops waiters (a
  program outlives the shell; a fiber cannot).

**Coprocesses** (`runcoproc`): two OS pipes, the ends moved to the
highest free descriptors below 64 as bash (so `63 60` first); NAME[0],
NAME[1], NAME_PID. Several may exist (bash 5.3's MULTIPLE_COPROCS),
newest first in `Shell.coprocs`. Their descriptors are close-on-exec
(`runprogram` skips them) and closed in subshells (`closecoproc`,
`hidecoproc` for a `( list )`), not in command substitutions. Closing
or moving one (`n>&-`, `n<&m-`, also `{NAME[1]}>&-`, which redirections
now accept) sets its variable element to -1 (`coprocfdclosed`). When
the coprocess has ended and the shell notices, `cleanupjobs` unsets the
variables and closes the descriptors (`disposecoproc`), as bash's
`coproc_reap`.

Job specs (`findjob`): `%N %% %+ %- %name %?text`, a missing `%` is
accepted with bash's warning, ambiguity reported. `kill %N` uses
`jobspecpid`. `jobs -x` substitutes the job's pid, where bash, without
job control, substitutes its own process group.

## Timers

`Task.deadline` makes a task's pipe or OS wait fail at that `os_now`
time. `schedule()` passes the earliest deadline of the blocked tasks to
`os_wait`; when it returns without an `Io`, `expiretimers` fails the
expired waits (`taskfail`, as signals use) and sets `Task.timedout`.
`safepointslow` expires timers too, so busy tasks do not starve them.
A wait the platform cannot withdraw (a Win32 helper's read) is waited
out instead: its deadline is cleared.
- `read -t T` (and `TMOUT`): the deadline is set around each byte read;
  `taskread` then never takes the single-task fast path. On timeout
  the bytes read so far are assigned and the status is 142. Regular
  files and in-memory text never time out (bash: regular files). `-t 0`
  reports whether input is ready (`inputready`: `taskreadable` submits a
  zero-length read and withdraws it).
- `os_wait` with only a deadline sleeps (POSIX poll on the self-pipe,
  Win32 `Sleep`); on the vos a deadline is a wake time for `VW_IO`, and
  `vadvance` jumps the clock to it when nothing else can run.
- The vos's `/dev/tty` (`VK_TTY`) never delivers input; the oracle's
  standard input is such a terminal (`Vos.ttyin`), as run-all's is, so
  read2.sub and read7.sub time out as recorded.

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
Closing M3a added (see "Closing M3a"):
```
i32  os_exec(Os *, Arena, Spawn *);    // replace the process; E_NOSYS: spawn instead
void os_tasktag(Os *, u64 tag, iz n);  // test layers: running task, task count
void os_racy(Os *);                    // test layers: the outcome may be racy
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
- **Win32** (the M3a simple approach, replaced in M3b: see
  notes/m3b.md): process exits
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
6. **Interim `&`, wait and coproc**: done (see "Jobs").
7. **Fork-on-exec** (POSIX): done (see "Closing M3a").
8. **Timers**: done for `read -t` (see "Timers"); a `sleep` builtin
   (loadable in bash) can use `Task.deadline` with no wait.
9. **Fuzzers**: done (see "Closing M3a").

## Bash test suite on the vos

`tests.exe --oracle SUITE NAME...` runs bash 5.3's run-NAME on the vos
(any output but "warning:" lines fails). Status of every run-* test at
the end of the oracle pass; category letters from
notes/research/tests.md.

The table of every run-* test now lives in notes/m4.md ("Closing
M4"), which updates it; at the end of M3a it read:

49 of 87 pass (category a: 39 of 43). Category a fails only for
complete (M8), nameref and quotearray (namerefs, M4) and dollars (a
bash bug in the .right file). (47 at the end of the oracle pass; arith
and dbg-support were fixed while closing M3a.)

What the pass changed, besides small fixes (see the commit log):
- **Array subscripts expand once.** In arithmetic text, in `[[ ]]`'s
  `-v` and arithmetic operators (`XC_SUBQ`), and for `unset name[sub]`
  (`XW_SUBQ`, compargs 2), a subscript is expanded as a word of its own
  when the expander meets its `[`, quotes removed, and the result's
  special characters (brackets, `$`, backquote, `~`, backslash and
  quotes) backslash-quoted (`arithsubscript`), so that evaluating the
  reference expands it to itself. `[[ -v ]]` only dequotes; test -v and
  quoted unset arguments still expand, except under array_expand_once
  (one option with assoc_expand_once). Indexed subscripts in arithmetic
  are evaluated once (`arithget` rewrites the reference).
- **$_** is the dynamic variable `_` over `Shell.lastarg`, a store
  buffer that `bindlastarg` overwrites after each simple command (never
  in place when the buffer belongs to the level around a synchronous
  subshell, below the store's mark). Assigning, unsetting or declaring
  `_` makes it plain until the next binding restores it. Programs get
  `_=PATH` in their environment.
- **extdebug**: a DEBUG action returning non-zero skips the command (2
  also returns from a function); DEBUG runs as a function starts and
  before each expression of `for ((;;))`; RETURN uses the body's line
  and is not inherited by functions called from the DEBUG action;
  `declare -ft` functions inherit DEBUG and RETURN; extdebug sets
  errtrace and functrace; `caller`.
- **vos clock**: each `os_wait(WAIT_POLL)` (a busy shell's safe point)
  advances the virtual clock 1 ms, so that a busy loop waiting for a
  background `sleep N; kill -USR1 $$` (trap9.sub) ends; otherwise time
  jumps only when nothing can run.
- shellerror and builtinerror allocate past a message built in the
  caller's copy of the same scratch arena: the unspecified evaluation
  order garbled `printf -v` errors.

Chaos: `tests.exe --chaos 20 t/exec/*.sh` (546 specs, 20 seeds each)
differs from the plain run in nothing; the concurrency specs 606-636
also pass with 24 seeds.

Benchmarks (bench/run on Linux in WSL against bash 5.3 built from the
tarball, TMPDIR in the scratch area), unabashed vs bash: b1 1.04s vs
3.15s, b2 1.86 vs 29.3, b7 0.11 vs 30.9, b8 0.19 vs 10.1, b9 1.14 vs
5.78, b10 0.06 vs 0.44, b12 5.40 vs 20.4. At the close of M3a (a
quieter machine, so bash is faster too): b1 0.78 vs 2.34, b2 1.48 vs
22.5, b7 0.07 vs 23.4, b8 0.13 vs 8.21, b9 0.75 vs 4.63, b10 0.03 vs
0.32, b12 1.29 vs 15.7. b8 (perf.md B8) and b12 (B12,
with a 32 KB body in place of 64 KB so that both shells use pipes
rather than temporary files) are new in bench/.

## Closing M3a

**Chaos fuzzer** (`main_fuzz.cpp -DFUZZ_CHAOS`, vos only). Each input
runs as `bash -c` once with the plain schedule, then under
`CHAOS_SEEDS` (3) chaos seeds; a mismatch traps so that AFL records it.
Compared: the exit status, whether a deadlock was reported, and what
each *writer* wrote to the outermost stdout and stderr (FNV hashes per
writer, so legitimately racing writers are compared each on its own,
never interleaved). A writer is a vos process plus a task in it, with
identities that do not depend on the schedule:
- `Task.tag` (sched.cpp): a task's tag mixes its creator's tag with the
  creator's count of tasks started (`nspawn`); the main task's is 0.
  The scheduler reports the running task's tag and the task count to
  the platform (`os_tasktag`, called at each switch and spawn; a no-op
  on POSIX and Win32).
- A vos process's identity (`VProc.lid`) mixes its starter's identity
  and task tag with how many programs that task started (`Vos.spawns`).
  An `Io` remembers the submitting task's tag (`Io.plt[0]`), because
  the vos may perform it later while another task of the process waits
  in `os_wait`.

Runs are not compared when the outcome legitimately depends on the
schedule, in bash too:
- inputs with constructs that observe timing or jobs (`chaosexempt`):
  a lone `&`, coproc, wait, jobs, kill, disown, fg, bg, suspend, sleep,
  SECONDS, EPOCH*, TMOUT, `-t` (read timeouts), time, date, mkfifo,
  `$$` (a nested shell's pid is a vos pid, handed out in start order
  across tasks), `-nt`/`-ot`/`-N` (mtimes follow the vos clock);
- runs that did something racy (`Vos.racy`, also `os_racy` from the
  core): a write that found its reader gone (vos pipes and in-memory
  Pipes: whether `true` exited first in `echo x | true`), bytes of one
  pipe read by two different readers (`{ head -n1; cat; }`), and a file
  system change while more than one task could run (`vconcurrent`: the
  sum of every live process's tasks, not counting a task waiting for a
  program it started).
A deadlock under one schedule only is a bug; one under every schedule
is a script that hangs under bash too (reading a terminal that never
types: `select ... </dev/tty`), which the vos reports instead.
On the spec corpus (556 scripts): 474 compared, 61 exempt, 10 racy, 1
out of memory, no mismatch.

**Isolate fuzzer** (`-DFUZZ_ISOLATE`): one shell prints a state dump,
runs `(eval "$1") </dev/null >/dev/null 2>&1` with the input as `$1`,
and prints the dump again; they must be equal. The dump is `declare
-p; declare -f; alias; trap -p; set +o; shopt -p; umask` (pwd is left
out: the subshell may remove the directory, which the parent then
notices). Excluded variables, which change on their own: `_ RANDOM
SRANDOM SECONDS EPOCHSECONDS EPOCHREALTIME LINENO BASH_COMMAND
PIPESTATUS BASH_LINENO HISTCMD`. If the parent dies (`kill $$`) the
input is skipped.

**Harness fixes.** Every mode now has a 64 MiB heap (half for shells):
with 8 MiB, the arenas of a shell plus its scheduler (four of
`info.reserve` and a pool arena per capture depth) left so little that
most inputs ran out of memory, and every one with a pipeline did, so
earlier run campaigns explored little past startup and simple
commands. OOM exits use `__builtin_longjmp` (it works from fibers) and
free the fiber stacks a run left behind. A plain build (no AFL) replays
the files named on its command line, saying what became of each
(`same`, `racy`, `exempt`, `out of memory`).

**Campaigns** (`tools/fuzz MODE [MINUTES [CORES]]`, WSL, 2 cores each,
from a snapshot in `~/fuzz-MODE`): about an hour of chaos and 50
minutes of isolate, in rounds; the last round of each found nothing. Found: an EXIT action that fails ran the ERR trap in the
same pool arena as itself (`runexittrap` took `capdepth+1` without
claiming it), corrupting its lexer (isolate, `trap 'e' EXIT ERR`).
Chaos crashes were harness false positives: consistent `/dev/tty`
deadlocks (hence the rule above), and background jobs written `\|&`,
which the exemption scan took for `|&` (it now skips escaped
characters). The last isolate rounds found nothing; hangs are infinite
loops.

**Fork-on-exec** (`os_exec`): `exec prog` replaces the process when
the shell is at its top level (no subshell, no capture), its scheduler
has one task, and no pump or capture is needed, so the program keeps
the shell's pid (`$$`), as bash. Otherwise it starts a child as before
(on Win32 and the vos `os_exec` returns `E_NOSYS`). The POSIX layer
saves every target descriptor, marks the others close-on-exec, installs
the map, the directory and the signal actions, and calls `execve`; if
that fails (a script without `#!`, which the shell then runs itself)
everything is put back. Only used when a failed exec ends the shell
(no execfail, not interactive). With a procsub alive (`exec prog <
<(...)`) it forks, so the program's pid differs from bash's. Related
fixes: a found program that fails to exec ends the shell without its
EXIT trap; execfail does not apply in subshells.

**Spawning on POSIX** uses `vfork`: arguments are prepared first, the
child only makes system calls (descriptor moves, `close_range`, signal
actions) and reports a failed exec through a variable it shares with
the parent; signals are blocked around it. 2000 spawns of `/bin/true`
went from 1.59 to 1.02 s.

**B12**: here-documents read by builtins were already in memory
(`F_MEM`); the cost was `read` itself, which now scans text in memory
in place (`b_read`'s fast path when no escape is involved) and splits
fields with a byte table. With the faster spawns, b12 is 1.27 s against
bash's 15.5 s (12x; 5.40 vs 20.4 before). Its external half (2500
`cat`s) is bound by process creation, about 0.5 ms each in WSL for
both shells; the builtin half is 15x.

**Oracle fixes**: arith (empty subscripts: assigning one says "not a
valid identifier" and goes on, reading one says "bad array subscript"
twice and reads unset; `let` under assoc_expand_once evaluates
subscripts as they are; an error evaluating a subscript abandons the
whole -c, eval or source text) and dbg-support (`Srcframe`: a sourced
file is a frame of FUNCNAME ("source"), BASH_SOURCE and BASH_LINENO;
DEBUG and RETURN in a sourced file follow the function rules, RETURN
runs where it was sourced, and DEBUG in that action follows the file's
rules (`Shell.retsrc`); BASH_ARGV/BASH_ARGC, built from the frames'
arguments and a bottom set up from the positional parameters when
first read by name (not by a listing) or when extdebug is turned on;
DEBUG before case and select; EXIT actions run at line 1).

## Known limits

- Jobs: no job control (`set -m` is accepted but changes nothing), so
  the jobs oracle differs where it uses `set -m` (fg, bg, stopped jobs,
  notifications) and suspend. A background vproc
  still running when the shell exits is waited for (a fiber cannot
  outlive its process), unlike bash, which leaves it running. The
  coproc oracle differs only for lack of an awk fake and /etc/passwd.
- Win32: `read -t` on an empty pipe or console could not time out once
  a helper thread owned the read. Fixed in M3b (notes/m3b.md).

- Win32: a program cannot open `/dev/fd/N`, so `diff <(a) <(b)` and
  `cat <(f)` fail there ("No such file or directory"); builtins,
  redirections, `source` and shell code work. The fix belongs with
  M5a's argument conversion: give a native child a named pipe path
  (`\\.\pipe\unabashed-PID-N`) in place of the `/dev/fd/N`
  argument, bridged by a pump, which needs an `os_pipe(named)` whose
  first write waits for the client (`ConnectNamedPipe` in a helper,
  cancelled when the program exits without opening it).
- Each vproc reserves two arenas plus scratch pools (about 16 MiB of
  address space); the test harness's 256 MiB of reservations allows
  a dozen or two live vprocs (procsubs, shell stages and background
  jobs together, nested shells included), and more abort with
  "unexpected out of memory".

- Win32 helper threads issued whole-length reads, and a helper read in
  flight blocked other operations on its handle (critique M3). Fixed
  in M3b's state machine and peek-exact reads (notes/m3b.md).
- POSIX: if another process reads the same pipe as a shell task, a read
  after poll may block the whole shell until data arrives.
- The vos runs nested shells on fibers; a deadlock inside a nested
  shell's own scheduler is reported by that shell.
- A fiber stage's stack limit is its parent's (`stackmax`), at most the
  fiber stack (8 MiB reserved).
- `exit` unwinds the call and source frames before the EXIT action
  runs, so FUNCNAME and BASH_SOURCE are empty there; bash runs the
  action from inside exit, with them in place.
- The chaos fuzzer's exemptions are coarse (a lone `&` anywhere, even
  quoted, exempts an input) and its file-system rule flags some
  deterministic runs as racy (`ls | sort >out`), so background jobs and
  file races get little coverage from it.
