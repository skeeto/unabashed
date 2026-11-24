# libunabashed: the shell inside a host process

libunabashed runs bash scripts inside the calling process, on the real
OS. One run, `ub_run(os, cmd)`, is one "bash" process: bash's
arguments, environment, directory and std handles in, its status out.
Programs it starts are real processes, while subshells, pipelines,
command substitutions and background levels run inside the run. When
the run ends, everything it started has ended too, and the host is as
it was. unabashed.h is the API; this note covers the contract around it,
how it is built, and where it differs from the executable.

The first host in view is Prospero (~/src/harness), which runs each tool
command as `SHELL -c CMD` and needs a soft stop, a hard stop, and no
leftovers.


## Building

The platform's root, `libunabashed_linux.cpp`, `libunabashed_darwin.cpp`
or `libunabashed_win32.cpp`, is one translation unit, compiled as an
object and linked into a C or C++ host with the C compiler:

    c++ -std=c++20 -O2 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
        -c libunabashed_linux.cpp

- POSIX: it needs only libc.
- Windows: it needs kernel32, plus advapi32 at run time (for entropy,
  loaded once per handle). It needs memcpy, memmove, memset, memcmp and
  strlen from the host's CRT or the host's own (tools/ubhostw.c), and
  libgcc on i686. Its imports stay within what XP SP3's kernel32
  exports; anything newer is looked up at run time.
- Exports: only `ub_*`. Every name of the shell's is in
  `namespace unabashed { namespace { ... } }`, so none merges with a
  host's, not even the inline members of its types. The asm labels are
  local. There are no global constructors, no C++ runtime, no entry
  point, and TLS only on POSIX.

With CMake (CMakeLists.txt), a host takes the library as a subproject,
which defines only the library: `unabashed::unabashed`, an object
library with unabashed.h on its include path, built with the host's
build type.

    FetchContent_Declare(unabashed URL ... URL_HASH SHA256=...)
    FetchContent_MakeAvailable(unabashed)
    target_link_libraries(host PRIVATE unabashed::unabashed)

A host built with ASan adds it to the library's target as well:
`target_compile_options(libunabashed PRIVATE -fsanitize=address)`.

At the top level, CMake also builds the hosts below: `ubhost` and
`ubcollide` (POSIX), `ubhost.exe`, and in an i686 build
(cmake/mingw32.cmake) `ubhost32.exe` and `ubtiny.exe`. Target
`check-lib` checks the objects at -O2 and -O0 and, on POSIX, runs
`ubcollide` and `ubhost`.

`tools/check-lib` checks the objects:
- `nm` shows only `ub_*` and imports, at -O2 and -O0;
- no static constructors and no C++ runtime symbols;
- no `.unabash` section.

`ubcollide` links a translation unit that defines the shell's own names
(`Str`, `Arena`, `fiberswap`, `jmpsetasm`, placement new) beside the
library.


## How it fits the shell

The library is the shell's core, plus the real platform layer
(src/posix.cpp with src/linux.cpp or src/darwin.cpp, or src/win32.cpp)
and a library module (src/libposix.cpp with src/liblinux.cpp or
src/libdarwin.cpp, or src/libwin32.cpp). Each root includes the pieces
for its platform, then src/ub.cpp, the entry points.

**The platform table.** The module's table, `libvtable`, is the
executable's `pltvtable` with some entries replaced:
- refusals for whatever would change the host process: exec, images,
  signal dispositions, terminals, process groups, limits;
- records around each entry that creates a resource.

Both tables are constexpr, so calls through either fold to direct calls.

**LIBMODE.** Library-only code in the core and the layers sits in
`if constexpr (LIBMODE)` branches, or in conditions the front end folds
(`!LIBMODE || ...`). `UB_LIBRARY` is 1 only in the libunabashed_*.cpp roots. A name
used only in such a branch is declared for every build (api.cpp, the
`lib_*` hooks) but defined only in the module, so the executables never
reference it. The executables stay byte-identical across library work.
This is checked by per-function disassembly of release builds with
clang, g++ and both mingw targets.

**A run.**
1. `ub_run` does its setup on the host's fiber, using raw reserve and
   commit only.
2. `sh_main` runs on a fiber of the run's own. On Windows the thread is
   converted to a fiber for the run, unless it already is one.
3. Teardown runs back on the host's fiber.

**Ending a run from anywhere.** Every resource is recorded the moment
it exists: reservations, descriptors and handles, fibers, helpers,
registered waits, programs. An abort (out of memory, a cancel, a
deadlock) can then switch straight back to the host's fiber,
abandoning every fiber of the run where it stands. Teardown frees what
the records list, without calling the core.

**Teardown order.**
1. Flush output.
2. Kill the run's programs: their sessions on POSIX, the job on
   Windows. Then wait up to 1 s for them.
3. Windows: unregister the pool waits.
4. Close the recorded descriptors and handles.
5. End or orphan the helpers.
6. Free the fibers.
7. Release the reservations.
8. Restore the thread's signal mask (POSIX).


## Decisions

- **Leftovers are killed when a run ends.** Top-level procsubs first get
  up to 1 s to finish, so that `cmd > >(sort > out)` and
  `exec > >(tee log)` complete.
  - POSIX: a program that calls `setsid` itself escapes.
  - Windows: the run's kill-on-close job holds every program.
- **`$$` is a virtual pid**, unique across runs and hosts, and never a
  real pid. `sh -c "kill $$"` cannot reach the host, and concurrent runs
  never collide on `/tmp/x.$$`. `$PPID` is the host's pid. `ub_cmd.pid`
  and `.ppid` override both; the drop-in hosts pass their own.
- **Programs are isolated from the host's terminal, console and group.**
  - POSIX: each program starts a session.
  - Windows: each program gets a console process group and a hidden
    console of its own (CREATE_NEW_PROCESS_GROUP|CREATE_NO_WINDOW),
    unless it is handed a console handle.
  - `UB_SHARECONSOLE` shares the host's console and group, as the
    executable does.
  - No terminal or console mode is ever changed.
- **Soft stop is a signal; hard stop is a cancel.**
  - `ub_signal(os, TERM|UB_GROUP)` reaches every level and every
    program, as a terminal's signal would. For a non-terminal signal it
    follows bash: a level waiting on a program takes the signal once
    the program ends.
  - On Windows a program gets a forwarded signal through, in order:
    - the signal channel, if it is an unabashed shell;
    - the host's `softsig` hook;
    - termination with 128+n.
  - `ub_cancel` ends the run at its next check point and returns
    `-UB_ECANCELED`.
- **Limits are finite by default.**
  - `memlimit`: min(1 GiB, RAM/4), or 256 MiB on 32-bit.
  - `fds`: min(256, RLIMIT_NOFILE/4), or 256 on Windows.

  A run past a limit fails with `-UB_ENOMEM` (or E_MFILE inside the
  run), and the host never fails.
- **On Windows the library never names the host's image.**
  - `root` is the directory of the install's shell. It gives the
    persona, fstab, profile, default PATH and `command -p`.
  - `self` is a real unabashed executable, for `$BASH`, `$SHELL` and
    sh/bash `#!` scripts.
  - With no `self`, such scripts run in-process, as scripts without `#!`
    do, with the `#!` line's options applied.
  - With no `root`, built-in defaults apply, and paths show in msys form
    with `/` as the drive of the working directory.
- **Signal numbers** are the host's on POSIX and Linux's on Windows.
- **The interface has a version, not struct sizes.** `UB_VERSION`
  (0xMMmmpp) names the release that last changed unabashed.h.
  - Its structs only gain fields, at their ends.
  - In the structs the host fills (`ub_config`, `ub_cmd`), zero means
    the default, or off.
  - So a host written for an older version, its structs zeroed, builds
    and runs as before.
  - A host is built with the header beside the library's source, as
    the library is only built from source, so the two cannot disagree
    on a layout.


## The host contract

`ubhost --check` enforces what it can (see Testing).

**The library guarantees:**
- **No process-wide state changes:** no signal handler or disposition,
  terminal or console mode, cwd, umask, rlimit or environment. Only
  `ub_run`'s thread mask changes, and it is restored.
- **No process-level calls:** no `waitpid(-1)`, `exit` or `setsid` in
  the host.
- **Host handles are left alone.** `ub_cmd`'s handles are duplicated at
  the start:
  - POSIX: `F_DUPFD_CLOEXEC`, at 10 or above;
  - Windows: non-inheritable.

  The run owns and closes the duplicates. Any other descriptor is EBADF
  inside the run, and nothing of the host's is made inheritable outside
  `spawnlock`.
- **Nothing is left behind.** After `ub_run` returns, no thread,
  mapping, descriptor, handle, child or zombie remains. Two exceptions:
  - a killed straggler that was not yet reapable after 1 s, which a
    later call reaps;
  - the Windows helper orphans described under Known deviations.

**The host must:**
- **Not reap children it did not spawn.** A SIGCHLD of SIG_IGN, or
  SA_NOCLDWAIT, makes `ub_run` return `-UB_ECHILD`.
- **On macOS, ignore SIGPIPE and SIGXFSZ.** macOS directs both at the
  process, not the thread, so `ub_run` refuses with `-UB_EINVAL`
  otherwise. On Linux the run blocks both in its thread and drains them
  afterward.
- **Keep `ub_cmd` valid until `ub_run` returns**, and close its own
  write ends only then.
- **Keep its own descriptors out of programs.** Make them close-on-exec
  (on Windows, non-inheritable), or create them under the same
  `spawnlock`. That lock brackets every moment the library holds a
  handle a program could inherit.
- **Use `flock` or OFD locks.** A POSIX `fcntl` lock belongs to the
  process, so a run that closes a file the host has locked releases the
  host's lock.
- **Build the library with ASan if the host uses it.** Hooks run on
  library fibers, and only an ASan build tells ASan about them (POSIX
  only: the Windows fibers are not annotated).
- **On POSIX, feed the `child` hook to a keeper** if programs must not
  outlive a crashed host. On Windows the job ends them.
- **Not call the library from a hook, nor unwind out of one.** A hook
  must be brief (see Hooks).

**Threading:**
- **`ub_run`** blocks its thread until the run ends. Each handle runs
  one at a time, and separate handles may run concurrently on separate
  threads.
- **`ub_signal` and `ub_cancel`** may be called from any thread, even a
  signal handler or console control handler. They use only atomics and
  one wake. Sent while no run is active, they apply to the next run, so
  a host that reuses a handle should not signal it while idle.
- **`ub_free`** must not race any other call on the handle.


## Hooks

Every hook in `ub_config` is optional. Each runs on `ub_run`'s thread,
inside the run, on whichever of its fibers reached it.
- **Brief.** A hook that blocks stalls the whole run, and `ub_cancel`
  takes effect only once it returns.
- **Small frames.** Deep in a recursion, a hook has only part of the
  64 KiB the shell's depth check keeps in reserve (stackroom,
  src/unabashed.cpp).
- **No reentry.** A hook never calls the library, nor unwinds out.
- **Borrowed strings.** What a hook is given is the library's, for the
  call only.

**`exit`** is called once as a run ends, with `ub_exit`:
- `status`, as `ub_run` will return it;
- `pwd`, the logical directory as `pwd` shows it, or null once the
  shell has lost its directory's name;
- `cwd`, the native directory programs start in, or null where there
  is none (Windows: a virtual directory, such as /proc);
- `env`, the exports as a program the run starts gets them (Windows: a
  native program, so converted from msys form).

A host starts the next run from that state: `cwd` as its `ub_cmd.cwd`,
`env` as its `ub_cmd.env`. Prospero keeps it beside the tool's output,
so that the next command follows the agent's `cd` and `export`.
- **When:** after the EXIT trap, history, the last flush and the procsubs'
  second, before teardown. Programs left in the background are still
  running then.
- **What calls it:** `exit`, set -e, `exec` (a spawn and a wait here),
  a signal's death, and the end of the script.
- **Never on an abort** (a cancel, out of memory, a deadlock) nor when
  `ub_run` refuses to start, so its status is never negative.
- **Without the hook** the run builds no environment at its end.

**`access`** is asked of each file the script names, before the
platform's call, with `ub_access`: an op, flags, and the path.
- **Asked:** redirections and `$(<f)`, `source`, `test`, `cd`, globs,
  PATH and hash lookups, a script without `#!` as it is read, the
  history file, and a here-document's anonymous file (`UB_TEMPFILE`,
  with the directory it is made in). A rename is asked as an open of
  its target that replaces it.
- **Not asked:** what the shell opens or examines on its own, marked
  `OF_OWN` or `ST_OWN` where it calls the platform: a program's first
  bytes (the `#!` sniff), `/dev/null`, the password, services and hosts
  files, zoneinfo, the inputrc, `BASH_ENV`, the checks that its
  directory still leads there. A site never marked is merely shown to
  the host, the safe direction for a mistake.
- **Paths** are absolute, as the system gets them. On POSIX they are
  joined, not resolved: `.`, `..` and links remain, so `source ./s`
  shows `$PWD/./s`. A policy that cares resolves them itself.
  On Windows they are native, with forward slashes, devices as
  `//./NUL`; virtual paths (/proc, /dev/clipboard) are never asked;
  nothing reads a link; and the file msys mode makes for a program to
  read a procsub by name is asked as `UB_TEMPFILE`, in its directory.

**`spawn`** is asked of each program the run starts, after the checks
that could refuse it anyway and before the platform's spawn, with
`ub_spawn`: the image, argv, environment and directory as the program
will get them. Pipeline stages, `&` jobs, coprocs and `exec` (a spawn
and a wait here) are all asked.
- **A script without `#!`** is never asked. The kernel would refuse
  it, and the shell runs it itself; the access hook sees it read. On
  POSIX the library tells first with a raw look at its first 80 bytes
  (no `#!`, no NUL), outside the run's descriptors and the access hook.
  A file the kernel would run anyway as text (a Linux binfmt_misc
  format) runs as shell code instead when the hook is set.
- **A refusal** fails as the system's would: `path: REASON` (or the
  error's text), status 126, or 127 for `-UB_ENOENT`. It is always a
  host's error (`E_HOST`), so the shell reports it in the hook's words
  rather than guessing at a cause ("bad interpreter", a lost working
  directory). PIPESTATUS keeps it, an `&` job ends with it, and a
  refused `exec` ends the run with it.
- **A wrapper** (`wrap`) starts instead: `wrap[0]`, absolute or the
  spawn is refused with `UB_EINVAL`. On POSIX its argv is `wrap[0]`,
  `wrap[1...]`, the path, then the program's argv, from `argv[0]`;
  everything else (descriptors, environment, directory, ignored
  signals, limits, its own session, an empty mask) is as the program's
  would be. A wrapper must:
  - inherit all of that and exec the program, so that `$!`, `kill` and
    the run's kill of its programs reach it;
  - otherwise stay in its session and exit as the program did,
    re-raising a signal it died of;
  - report its own failure to exec as bash does: 127 if the program is
    not there, else 126.

  `env` cannot serve: it loses the split between the path and
  `argv[0]`. `ubhost --wrap` is the test's wrapper.
- **A refusal** fails the access as its error would, so a script sees
  what bash shows for that error: `f: Permission denied` and status 1
  for a redirection, no match for a glob, false for `test`, `command not
  found` for a PATH search, 126 for a script read (127 for
  `-UB_ENOENT`), "cannot create temp file for here-document" for
  `UB_TEMPFILE`.
- **A reason** replaces the error's text: its first line, at most 255
  bytes, cut before a character it would split. The run keeps the last
  16 reasons, each with its error, as error codes of their own above
  any the system reports (`E_HOST`, src/libhooks.cpp); `errtext` shows
  the reason. Without a reason the plain error stands, so `-UB_ENOENT`
  still looks absent: the msys `.exe` retry and a PATH search move on.


## Known deviations and limits

**From the executable and bash, by design.** tools/ubhost.expect lists
the suite tests that fail through the drop-in, with reasons:
- No job control or terminal. `set -m` and `suspend` are refused, and
  nothing stops.
- A background level is a fiber, not a process, so `$!` is a virtual
  pid, as under `tests --fibers`.
- `exec` does not replace the host. The program runs while the run
  waits, so the shell keeps its pid.
- A subshell is a level, so a stop signal it sends to `$$` is refused.
  Under bash the parent process would run its trap.
- Windows: no Ctrl-C judging, and no signal channel between separate
  shells.

**Under a group TERM.** bash's top level dies at once, even while an
in-process subshell traps the signal; the library waits for the nested
level. "Terminated" job messages differ in a few cases.

**Residual delays.** A cancel or signal is seen at the next check point:
- every simple command;
- every 1024 loop iterations;
- every 4096 matcher steps;
- `plt_wait`, `plt_commit` and `plt_spawn`.

Still uninterruptible:
- opening a FIFO with no peer;
- a stuck kernel call;
- a long builtin other than a loop or a match.

**Speed.** The library always submits reads and writes that may block.
`read` takes a pipe a byte at a time, as in bash. While data is waiting,
a submitted pipe read is a FIONREAD and a read, so a read loop over a
pipe costs about 1.5x the executable's direct read (bench/b6: 9.3 s
through ubhost, 12.5 s through the executable, whose procsub reads
still poll first). A count of waiting bytes is never kept across reads:
another reader (a task, a program, the host) could empty the pipe in
between and leave a read blocked where a cancel cannot reach it.

**POSIX:**
- A program that calls `setsid` itself escapes the final kill. Hosts
  that care use the `child` hook.
- A fired one-shot macOS NOTE_EXIT can come before the program is
  reapable. Such a program drops to the backoff scan.

**Windows:**
- **Console writes** stay direct on the run thread, as the executable's
  are, so a console blocked by a selection blocks the run.
- **A console read in flight** at teardown is orphaned, not cancelled:
  cancelling would make the console drop the next line typed. The
  orphaned helper takes that line, and the handle is never freed, since
  the helper may still signal it.
- **Overlapped std handles** keep the executable's null-OVERLAPPED
  behaviour; bridging them is deferred.
- **XP (no CancelSynchronousIo).** A pipe read, write or connect in
  flight whose peer is outside the run orphans its helper. The helper
  frees itself on the peer's next operation, and that operation's data
  may be consumed. ubhost's "hard: read a host pipe" case checks this.
- **XP (no handle list).** Programs inherit by the spawn lock alone, so
  any inheritable host handle reaches them.
- **Before Windows 8 jobs do not nest.**
  - The run's job allows breakaway, so that a nested shell's programs
    can join that shell's own job.
  - A program created with CREATE_BREAKAWAY_FROM_JOB sits in no job
    until it is assigned to the nested run's job. If an outer run kills
    the nested host in that window, the suspended program is left in
    neither job, holding its inherited handles.
  - Inside a host job that forbids breakaway, a program runs jobless on
    a fallback list. Its own descendants then escape the final kill.
- **Vista and 7** are untested (no machine).


## Testing

**`ubhost`** (tools/ubhost.c with tools/ubcheck.c, or tools/ubhostw.c
with tools/ubcheckw.c on Windows) is a drop-in for the shell. It passes
its own pid, ppid, descriptors and entry dispositions, and forwards its
signals or console events with `ub_signal`. The suites run through it:
- Linux (oracle image):

      ls t/exec/*.sh t/posix/*.sh |
      xargs -P 8 -n 4 env RECORD_SHELL=$PWD/ubhost RECORD_CHECK=1 tools/record

- Windows:

      t/win32/run -s $PWD/ubhost.exe -x WORKDIR

  `ubhost32.exe` goes beside it; on XP use
  `t/win32/run -s $PWD/ubhost32.exe WORKDIR`.

Anything failing beyond tools/ubhost.expect is a bug. Two tests are
flaky for the executable too: t/exec/931 under the parallel run's load,
and t/win32/015 on XP.

**`ubhost --check [--quick]`** snapshots the host, runs each case
several times after a warm-up, and compares the snapshots by identity:
- mappings;
- each descriptor or handle with its flags;
- threads;
- signal and terminal state, or console state on Windows;
- cwd, umask, rlimits and the environment;
- descendants.

The cases cover:
- statuses and refusals;
- syntax checks (`bash -n -c`);
- the hooks, through a log each case checks, a second run started
  from the state the exit hook was given, refusals by rule, and
  programs wrapped in `ubhost --wrap`, in a directory of files made for
  the cases ($T);
- leftovers and shutdown;
- children's signal state;
- concurrency (8 threads, 4 on 32-bit Windows);
- soft and hard stops with timing;
- out of memory, including a sweep that aborts everywhere, run again
  with every hook set;
- the descriptor budget;
- SIGPIPE;
- on Windows, helper queues, fibers and `ub_system`.

On Windows, a watcher thread samples descendants while each case runs,
so leftovers are caught before `ub_free` closes the job.

Windows system objects the check tolerates:
- thread pool objects that come and go;
- critical sections' lock events, found through the process's
  critical-section list;
- handles protected from closing, which XP caches under load.

Limits of the check:
- Pool wait packets are budgeted (at most 8 more over the whole check).
  Before Windows 8 they are not objects, so a leaked registered wait
  shows only as a wait thread, every 63 waits.
- Images and views are compared whole, by allocation, since their pages
  split as copy-on-write data is first written.

Planted bugs it catches:
- an unrecorded handle;
- the spawn lock released before the inheritable duplicates close;
- no UnregisterWaitEx at teardown (the check crashes).

**`ubtiny.exe`** is a CRT-free i686 host with no `root`, no `self` and
no marker. It checks:
- `echo`;
- `#!/bin/sh` scripts, including a `-e` on the `#!` line;
- a script without `#!`;
- `ub_system`;
- that no program starts as its own image.

**Machines.** `--check` passes on:
- the Mac;
- Linux (the ubuntu VM);
- the Windows 11 ARM64 VM (x64 and WOW64);
- biron (x64; i686 with `--quick`);
- XP (i686).

**AddressSanitizer** (POSIX). Build the library and host with
`-fsanitize=address`, then run `ubhost --check`. Fibers are annotated
when the build has ASan (src/fibers_posix.cpp):
- every switch is announced, so ASan neither warns about
  `__asan_handle_no_return` nor misreads a fiber's stack;
- a fiber's last switch out (back to the host at the end of a run)
  passes no save, which frees its fake stack under
  `detect_stack_use_after_return`;
- a freed fiber that still owns a fake stack is entered once more to
  release it, and teardown does so for every fiber left.

`STACKAT` (src/base.cpp) takes the frame address for the stack-depth
checks instead of a local's, which ASan may move to a fake stack. ubhost
sets `quarantine_size_mb=0` by default: a quarantine holds freed blocks
and maps more memory every case, so snapshots would differ.

ASan passes `--check` on the Mac (clang) and the ubuntu VM (gcc), with
and without `detect_stack_use_after_return=1`, and catches a planted
overflow on the shell fiber. Linux clang cannot build the shell for
arm64 (no `__builtin_setjmp`). On the Mac the 300-programs case runs
about 9x slower under ASan, since each fork copies its shadow mappings.

## Deferred

- persistent shells and input fragments;
- host-supplied tables;
- a table version floor;
- interactive use and terminal sharing;
- a keeper process inside the library;
- `sh -c` in-process with no `self`;
- FIFO opens that a cancel can interrupt;
- bridging overlapped std handles;
- pidfd/kqueue in the executables;
- signals tagged with a run generation for reused handles;
- Vista and 7 testing.
