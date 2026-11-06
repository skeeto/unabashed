# M5b: real background processes

Status: steps 1-3 done; implementation in progress (see "Steps").

Scope and exit criteria: plan.md §2 (execution model: "Real process
from M5b"), §3 (the copier writes the `&` image) and the M5b row of its
milestone table: copier image; self-spawn `&` and coproc; `os_fork`;
signal channel (named section bitmask plus event); real-pid kill, wait
and jobs. Exit: category c in the container; image fuzzer clean; `sh -c
'server &'` returns immediately. Open items: the M5b section of
issues.md.


## Where the code stood at the start (02d5f2b)

- `cmd &`, `( list ) &`, background pipelines and coproc are fiber
  vprocs (jobs.cpp `runasync1`, `spawnbg`, `runcoproc`; "Interim M3a
  jobs"). At most `MAXBG`=4 live background vprocs, then `&` blocks.
  A likely-external `prog &` with plain words is spawned directly
  (`directbg`, `spawndirect`, `spawnwaiter`) and `$!` is its real pid;
  anything else (`"$U" -c x &`) runs in a vproc and `$!` is the
  vproc's virtual pid, which `kill` signals in-process only: the
  program it started runs on (issues.md, `kill $!`).
- The copier (proc.cpp `vproccopy`) is hand-written per type: `*c =
  *sh`, then deep copies of the variable, function, alias and hash
  tries, args, frames, source frames, fdtab and traps into the vproc's
  store and perm arenas. Fibers share by plain pointer `os`, `sched`,
  `paths`, `sniffs`, `exe`, string literals and every scalar; `Code`
  blocks are shared by refcount. No serialization, no relocation.
- No `os_fork`, no image, no `OsInfo.image`. POSIX spawns with vfork
  plus execve; `exec` with other tasks alive starts a child and waits
  (plan.md's fork-on-exec is not done).
- Signals: every live level registers a `Sigproc`; `kill` of a vproc
  pid is `sigsend`. Win32 `os_kill` is TerminateProcess(128+N);
  `os_sigpending` reads `sigown`, which nothing sets ("the signal
  channel to our own shells is M5b"). The vos's `os_kill` already
  delivers to its shell processes.
- At exit the shell joins every orphan vproc ("a fiber cannot outlive
  its process"); waiters for programs are cancelled and the programs
  run on.
- The vos can run the shell as its own child: `fake_bash` calls
  `sh_main` nested on the vos process's fiber (argv only).
- Both builds are DYNAMIC_BASE (ASLR): a child's exe, string literals
  and code may sit at other addresses than the parent's.


## Principles

1. One copier. The image is `vproccopy` in an "image" mode that also
   deep-copies everything a fiber shares (code blocks, PathCtx, exe,
   literals) into one region; fibers keep the cheap mode.
2. The image is closed: no word in it may point outside its own region
   (not into the exe, a stack, or the parent's other arenas). The
   child maps the region at the parent's address, so nothing is
   relocated, and ASLR stays on. Test and debug builds audit closure
   with a conservative scan of every aligned word.
3. Randomized tests (the image fuzzer, chaos) only on the vos, where
   the child shell is `fake_bash` on the same machine model.
   Real processes only through curated scripts: t/win32 in the
   sandbox, the POSIX build in a disposable container.
4. bash is the reference for what a background job sees (it forks):
   variables with their dynamic scope, functions, aliases, options,
   traps (caught reset, ignored kept, texts listed), cwd, umask,
   positional parameters, FUNCNAME/BASH_SOURCE/BASH_LINENO, LINENO,
   `$_`, `$$` (the parent's), BASHPID and `$!` (the child's real pid).
5. Fibers stay for procsubs and pipeline stages, and as the fallback
   when an image cannot be made or mapped (counted and reported in
   debug builds, listed in issues.md).


## Steps

Waves: 1; then 2 and 5 in parallel; then 3; then 4 and 6 in parallel;
then 7; then 8 and 9 in parallel; then 10.

1. **Image copier and adoption, in process.** An image mode of
   `vproccopy` writing into a dedicated region (a new `Mem` over one
   reservation): deep copies of shared state, literal strings cloned,
   sniff cache and Sched dropped. `imageadopt`: turn the image's
   `Shell` into a running top level (fresh Sched, Os, scratch arenas,
   store arena re-homed to the child's own reservation, a Sigproc,
   pids). The closure audit (debug and test builds). Tests: tests.exe
   unit tests that build, audit and adopt in process and compare
   `declare -p; declare -f; alias; trap -p; set -o; shopt; dirs`
   (minus the dynamic variables) between parent and child.
2. **Platform API.** `os_imagenew`/`os_imagemap` (a pagefile-backed
   section on Win32, mapped copy-on-write in the child at the same
   address before any other reservation; a fake in the vos), the
   `Spawn.image` handle and `OsInfo.image` (base, length, root);
   `os_fork` (POSIX; E_NOSYS elsewhere); a signal-death status
   convention for our own children on Win32. Self-spawn command line:
   `--unabashed-image=...` recognized only from our own parent.
3. **vos self-spawn.** `&`, coproc and background pipelines as vos
   processes running `fake_bash` with the image; descriptors passed as
   inherited handles (in-memory Pipes, heredocs and captures
   materialized first, as for programs); `$!` and BASHPID the vos pid;
   the job waits on the process (IO_PROC); MAXBG goes; the shell exits
   without waiting for them. Every t/exec job spec still passes; new
   specs for `$!`, `kill $!` to a trapping background shell and `sh -c
   'f &'` returning.
4. **Win32 self-spawn.** The section at a top-down address, the
   child's early map, inherited handles, CREATE_NEW_PROCESS_GROUP and
   INT/QUIT ignored as bash does for `&`, the job object (a background
   shell must outlive ours: no kill-on-close), the fiber fallback.
   t/win32 tests; the XP laptop.
5. **POSIX `os_fork`.** Materialize, fork, the child drops every other
   task and fiber and continues at the async node; signal handlers and
   the wake pipe re-created. Fork-on-exec: `exec prog` with tasks alive
   forks, the child keeps the tasks, the parent execs (so `exec prog <
   <(f)` keeps `$$`).
6. **Signal channel (Win32).** A named section `Local\unabashed-<pid>`
   with a pending bitmask and an event, created by every shell;
   `os_kill` to one of our shells sets bits (KILL and STOP-likes still
   TerminateProcess); `os_sigpending` reads them; the wake wait
   includes the event. A child dying of a signal exits with the
   convention's code so the parent's `wait`, `jobs` and `$?` see it.
7. **Real-pid jobs.** kill, wait (with -n, -f, -p), jobs, disown and
   notifications over processes; `$!`; DEBUG trap before `&`; `f &
   kill -9 $!; wait` reporting; orphans left running at exit. The M5b
   items in issues.md that this closes.
8. **Category c in the container.** A runner for bash's run-* tests on
   the POSIX build in a disposable non-root container (`--network
   none`, tmpfs workdir), next to the vos's `--oracle`; t/exec specs on
   the POSIX build there too. Manifest of results.
9. **Image fuzzer.** `FUZZ_IMAGE`: run the input, then image, audit,
   adopt in a vos child and compare the listings. tools/fuzz mode,
   AFL campaign in WSL, FUZZ_WIN replay.
10. **Closing.** Win32 M5b issues (Ctrl-C misses, the XP `wait` flake,
    a lent pipe's other readers, Ctrl-C before SIGPIPE) fixed or moved;
    cost of `&` against Git Bash (fork) and busybox; `sh -c 'server &'`
    on every platform; exit criteria.


## Decisions

- ASLR stays: the image is made closed rather than the exe based at a
  fixed address (principle 2).
- Win32 transfer by copying: the parent starts the child suspended,
  reserves the image's address range in it (VirtualAllocEx), commits
  as much as is committed here, writes the image (WriteProcessMemory)
  and resumes it. No relocation. If the range is taken in the child,
  the spawn fails and the parent runs the job as a fiber. This replaced
  the planned shared section: the child's store goes on in the region,
  and a mapped view can be committed but never decommitted or released
  piecemeal, and copying an image of a few hundred KiB costs little next
  to starting a process.


## Exit criteria checks

1. Category c: bash's run-* category c tests on the POSIX build in the
   container, every failure attributed to a later milestone.
2. Image fuzzer clean (AFL, vos only), and its replay under FUZZ_WIN.
3. `sh -c 'server &'` returns immediately (vos, Win32, POSIX), with the
   server still running.
4. No regressions: tests.exe, t/exec, t/parse, t/unit, t/winvos,
   t/msys, chaos, the vos oracle counts, t/win32 (Win11 and XP),
   kernel32-only imports, PE 5.01.
5. No M5b items left in issues.md.


## Implementation

### Step 1: the image copier, and the vos's self-spawn

The copier is now `shellcopy(sh, a, st, image)` (proc.cpp), with
`vproccopy` the fiber case. Image mode copies every function body
(never shares `Code`), the PathCtx, `exe` and `shellname`; leaves no
Os, Sched, Sigproc, sniff cache, stack top or tail node; gives an
empty fdtab and no coprocesses (the descriptors go to the process);
and makes `Frame.trapat`/`Srcframe.trapat` a stand-in (1) where set.
Those markers are only compared with the running trap action, which
in any copy is another string, so a vproc's copy never matched them
either: the stand-in keeps that.

`imagebuild` lays out one region: an `ImageHead` (magic, a build
stamp from `__DATE__ __TIME__` and the copied types' sizes, the Shell,
the node), the `Mem` behind it, the `Store`, then the copy, all
bump-allocated from the store's arena. The region is the child's store
from then on: the child goes on allocating above the image, so the
watermark rule (an object below a subshell's mark is the parent's,
and its writes are journaled) holds without moving anything; a child
store in a separate arena would put the image's objects above or below
marks by accident of addresses. `Mem.os` is cleared (the child's).

`spawnbg` (jobs.cpp) first tries `spawnimage`: `os_imagenew`, the
descriptors a program would get (`bgfds`: F_MEM materialized, captures
through `capturewriter`, a pipe read through `pipeosread`, a pipe
written through a pump left to the Sched's orphans), the image, and
`os_spawn` with `Spawn.image`; the job then waits on the process with
`spawnwaiter`, as for a program, and `$!` is its pid. Where the
platform has no images (POSIX and Win32 for now) or the spawn fails,
the job is a vproc as before. `sh_main` hands an `OsInfo.image` to
`imagemain` (unabashed.cpp): it checks the head, re-points `Mem.os`,
makes a Sched, a Sigproc and fresh scratch, takes BASHPID from its own
pid (`$$` stays the parent's), sets the descriptors as startup does
(`startfds`, shared with sh_main), and runs the node with its EXIT
trap, ending as sh_main does.

The vos: `os_imagenew` reserves the region from the shells' heap (one
address space: the child takes it in place) when `Vos.images` is set;
`os_spawn` with an image starts the `bash` fake, passing every listed
descriptor, and `fake_bash` (tests and fuzz layers) passes
`FakeCtx.image` on. `tests.exe --images ...` (first argument) turns it
on for every mode; the default stays vprocs until the specs agree.
Before each such spawn the vos audits closure (`vimageescape`): any
aligned word of the image pointing into the shells' heap outside the
region (its end included) or into the test program's own image is
reported in the spec's output.

Results: default mode unchanged (tests.exe 159/0, t/exec 660 specs, 0
failed, t/parse, t/winvos, t/msys vos, t/win32 -x 30/0). With
`--images`, no audit failures in t/exec and t/winvos. Fixed on the way:
- 634-coproc: the coprocess answered and exited before the parent's
  `read <&${COPROC[0]}`, and the parent reaped it (unsetting COPROC)
  first; bash's child is slower to start. A vos image child now starts
  after 5 ms of virtual time, and a vos poll that yielded to other
  processes returns nothing, leaving what they did to the next poll (in
  every mode: no spec changed).
- 726-funsub-procsub-outlives: a capture's pump had no owner, so the
  capture's EOF waited on nothing; the pumps `bgfds` makes are owned by
  the child's pid once it is spawned.

One spec still fails with `--images`: 692-killed-level-program-output.
`kill $p` from the background job of a procsub or comsub level, whose
BASHPID is a virtual pid of its process, is "No such process" from
another process (steps 6 and 7: virtual pids naming their process,
signals carrying the level).

New: t/exec/875-bg-state-image (recorded): every listing (declare -p
less the dynamic variables, declare -f, alias, trap -p, set -o, shopt,
hash, `$-`, the positional parameters, FUNCNAME, BASH_SOURCE, PWD,
OLDPWD, getopts' state) the same in a background job as in its
parent, from functions with locals called from a sourced file.


### Step 2: the platform API, and Win32 self-spawn

`ImageRef` lost its handle: no platform needs one. `Spawn.image` now
says that a started child owns the region (the parent's copy may be
gone) and that the caller frees it if the spawn fails.

Win32 (main_win32.cpp): `os_imagenew` is a plain reservation; `os_spawn`
with an image (`startproc`) runs this program again (`Os.exe`) with the
one argument `--unabashed-image=BASE,CAP,LEN` (hex), always suspended,
passing descriptors above 2 in the CRT blob at this build's width;
`imagecopy` then does what the decision above says, the thread is
resumed, and the parent's region is released. The child's startup
takes the argument only as the sole argument, and only for a region
that is this process's own private memory, committed from its
allocation base as far as LEN (`imagearg`); `imagemain` then checks the
head's magic and build stamp. Otherwise the argument is an ordinary
(invalid) option.

The signal-death convention: a shell dying by signal N, and a program
`os_kill` ends, exit with `SIGEXIT|128+N` (0xe0550000, an error
NTSTATUS with the customer bit; its low byte is the status for a parent
that keeps only that), SIGINT keeping 0xc000013a; `exitstatus` maps
both back to `WSIGNALED|128+N`. So `sleep 5 & kill $!; wait $!` is 143
from a signal, as in bash.

This already does most of step 4: `&`, coproc and background pipeline
stages are processes on Windows, ignoring Ctrl-C (`Spawn.sigign`, as
for programs: no job object, so they outlive the shell). Left for step
4: CREATE_NEW_PROCESS_GROUP, counting fiber fallbacks in debug builds,
and handles lent to the child (a pipe with a read in flight).

A background job's own redirections (`{ ...; } >/dev/null &`, `f
>/dev/null &`) are now made for good, as in bash's forked child
(`Shell.keepredirs`, set by `bgnode`): the descriptor they replace is
closed at once instead of being saved for a restore that never comes.
In a process that held the parent's pipe open, so `"$U" -c '{ sleep 2;
} >/dev/null & echo s' | cat` waited 2 s; as a fiber, `x=$( { sleep 2;
} >/dev/null & )` did too. bash returns at once in both; a redirection
inside the job's command (`{ sleep 2 >/dev/null; } &`) still holds it,
in bash too. New spec t/exec/876-bg-redir-closes (recorded).

The vos `bash` fake (tests and fuzz layers) now hands its process's
descriptors to the shell instead of duplicates, so that, as in a real
process, the shell's references are the only ones and closing one
closes it; without that the image child of 876 kept the pipe open
however its redirections went. No spec changed.

`os_fork` moves to step 5, its first caller (an unused static function
would only warn).

Cost: 100 `{ :; } & wait` take 0.33 s on Windows 11 (3.3 ms each)
against 0.018 s as fibers.

Tests: t/win32/016-bg-image (`$!` the job's BASHPID, `$$` the parent's,
locals, arrays and functions in the job, a descriptor above 2, a pipe
in, a here-string, a capture, `kill` status, `"$U" -c '... &'`
returning at once with the job finishing later). t/win32 -x 32/0 on
Windows 11 (x64 and i686 builds); on XP 29/3 with -x: 003-waits (the
known XP `wait` flake, passing on a rerun) and 016's "TERM 143" in the
sandbox (issues.md, also with binaries from before M5b). The vos suites
unchanged; with `--images` only 692 fails.


### Step 3: the vos self-spawns by default, and virtual pids name their process

`tests.exe` now runs background jobs as vos processes from their image
in every mode; `--fibers` (first argument) runs them as vprocs, as on a
platform without images. Every t/exec and t/winvos spec passes either
way, plus chaos, except that 877's job outliving its `bash -c` cannot
under `--fibers` (a fiber dies with its process).

Virtual pids (BASHPID of a subshell level or a vproc, and the `$!` of a
spawn that failed) were the parent's counter from `$$` up, so they
collided with real pids, and two concurrent vprocs could hand out the
same one. Now `newvpid` (trap.cpp) numbers them per process in the
Sched and encodes the process: `pid<<16 | 2n+1` on Windows (odd, never
a real pid), `2^22*(n+1) + pid` on POSIX (above Linux's pid_max), so
`vpidowner` finds the process. `kill` (`killpid`) sends to a level of
this process directly, to a level of another of our processes through
the new `os_siglevel` (the receiver's `ossignals` takes them with
`os_levelsig` and hands each to that level), and to anything else with
`os_kill`. A level that has ended is "No such process". `kill $$` from
a job reaches the parent process (it no longer means the process's
outermost level). Only the vos has the channel; on Win32 and POSIX
`os_siglevel` is E_NOSYS, which `kill` reports as "No such process"
until step 6 (Win32) and step 5 (POSIX, where the fork shares nothing).

That fixes 692 (a job killing the procsub or comsub level that started
it). `MAXBG` now only throttles the fiber fallback: image jobs are
waiters, not counted, and at exit the shell drops waiters, so it no
longer waits for its background jobs.

New spec t/exec/877-bg-process (recorded): `$!` is the job's BASHPID,
`$$` the parent's; TERM to a job reaches its trap, ignored stays
ignored; USR1 from the parent to a subshell level inside a job, USR2
from a job to the parent's level; `bash -c '... &'` returns before its
job finishes.

Results: tests.exe 159/0, t/exec and t/winvos 687 specs, 0 failed (also
with `--fibers` but 877, and `--chaos 20`), t/parse, t/unit, t/msys;
t/win32 -x 32/0 on Windows 11 (x64 and i686).
