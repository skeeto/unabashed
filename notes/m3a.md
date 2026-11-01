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
- Tests: `t/exec/607`–`611`; `test_sched` unit tests (pipes, joins,
  EPIPE, deadlock recovery); `tests.exe --chaos N` reruns every spec
  with N chaos seeds, comparing stdout, stderr and status (a spec
  containing `# nochaos` is exempt: its stages race on one stream);
  `tests.exe --run FILE [SEED]`.

Remaining M3a work (see "Plan for the rest of M3a" below): launcher
fibers for external stages with comsubs, procsub, heredoc and
capture materialization through pumps everywhere, PIPESTATUS, traps and
SIGPIPE dispositions, fork-on-exec, interim fiber `&`/wait/coproc, the
isolate fuzzer, timers.

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
| `Task *spawnpump(Sched *, Pump *)` | `PUMP_TOHANDLE` (Pipe → handle), `PUMP_FROMHANDLE` (handle → Pipe), `PUMP_TEXT` (string → handle); the pump owns `h` and its pipe end |

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

`runprogram` (exec.cpp), for every descriptor of an external:
- `F_OS`: passed as is;
- `F_MEM` (here-documents, here-strings): an OS pipe, filled now up to
  `os_pipecap`, the rest by a `PUMP_TEXT` task;
- `F_PIPE`: an OS pipe plus a pump in the right direction. Input pumps
  are cancelled once the program exits (what it did not read stays in
  the Pipe); output pumps are joined, so the program's output precedes
  anything the shell writes next;
- `F_CAPTURE`: an OS pipe read here until EOF.

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
2. **PIPESTATUS**: `runpipeline` has every stage's status; also needs
   setting after every simple and compound command, cheaply (a dynamic
   variable over a small array in `Shell`).
3. **Process substitution** `<(f)`/`>(f)`: a fiber vproc (`spawnvproc`
   with `out`/`in` set to a `Pipe` end) whose other end is a `File` in
   the parent named `/dev/fd/N`; for an external, materialize as an OS
   pipe (`os_pipe`) plus pump, or a named pipe on Win32 (needs an
   `os_pipe(named)` addition). The fiber outlives the command when
   `exec 3< <(f)`: copy everything it references (invariant 3) and
   join it at shell exit or when the descriptor closes.
4. **Here-documents and captures for externals**: already pumped in
   `runprogram`; heredocs over 64 KiB should become a delete-on-close
   temp file (needs `os_tempfile`). `x=$(sleep 9 &)` must wait for
   every writer of the capture.
5. **Traps and SIGPIPE**: subshells reset caught traps; `sigpipe()`
   must run a PIPE trap, or with `trap '' PIPE` report
   "write error: Broken pipe" instead.
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

- Win32 helper threads issue whole-length reads; the File state machine
  and Peek-exact reads are M3b. A helper read in flight on a handle
  blocks other operations on that handle (critique M3).
- POSIX: if another process reads the same pipe as a shell task, a read
  after poll may block the whole shell until data arrives.
- The vos runs nested shells on fibers; a deadlock inside a nested
  shell's own scheduler is reported by that shell.
- A fiber stage's stack limit is its parent's (`stackmax`), at most the
  fiber stack (8 MiB reserved).
