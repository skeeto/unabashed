# unabashed: a faithful bash 5.3 clone for w64devkit

## Context

The goal is a from-scratch clone of GNU bash 5.3 that can ship in w64devkit:
- as `bash`: a drop-in for Git-for-Windows/MSYS2 bash, which AI agents rely on, using MSYS2 paths;
- as `sh`/`ash`: a possible replacement for busybox-w32 ash, using native `C:/` paths.

It is written in skeeto-style C++20:
- unity build, arenas, counted strings, signed sizes, no standard library in the core;
- an `os_*` platform API with Win32 (CRT-free, first class), POSIX and fully virtual layers.

It must beat bash on speed by design (no fork, arena reset per command) and include its own readline-equivalent line editor. Bash source and docs serve only as a semantic reference. No GPL code, test files or help text are copied.

**Decided with the user:**
- **Paths:** input in the forms `C:/x`, `C:\x` and `/c/x` is always accepted. One mode setting (default from argv[0], toggled by a hidden shopt) chooses:
  - the output form;
  - what a leading `/` means;
  - the PATH separator;
  - environment translation;
  - MSYS2-style argument conversion for native children.
- **XP baseline:** anything newer is resolved at run time, with a fallback.
- **Locale:** UTF-8 and C/POSIX byte mode only.
- **Editor first milestone:** emacs bindings, history, completion and inputrc basics. vi mode, `bind` and programmable completion come later.
- **Memory model:** a mutable store. Synchronous subshells journal their writes, roll back and reset to a watermark. Concurrent vprocs get a deep copy. Size-class free lists hold the long-lived values.
- **sh/ash persona:** bash POSIX-mode semantics and messages, plus busybox-w32 conventions and a preference for busybox applets.

**Environment facts:**
- WSL Debian 13 has bash 5.2.37, gcc, AFL++ and docker. bash 5.3 must be built from the tarball to serve as the oracle.
- Git Bash on this host is 5.3.15 (cygwin) and is the oracle for MSYS behaviour only.
- w64devkit has x86_64 and i686 g++. The XP laptop is reachable over ssh.
- The repo is empty apart from `bash-5.3.tar.gz`.

Research and design reports (about 440 KB, our own summaries with bash source line references) are in `C:\Users\wellons\AppData\Local\Temp\claude\C--Users-wellons-src-unabashed\49667c1f-f31a-4608-a8d2-7135244c0f96\scratchpad\research\*.md` and `...\scratchpad\design\*.md`. **The first step copies them into `notes/` in the repo**, because the scratchpad is per-session. Later work reads them from there:
- `semantics.md`: full feature inventory with P0/P1/P2 tiers;
- `readline.md`: keymaps, redisplay, completion;
- `msys2.md`: the exact path-conversion rules;
- `win32.md`: APIs with their XP fallbacks;
- `busybox.md`, `tests.md`, `perf.md`, `style.md`;
- the design panel's output, including `critique.md`.


## Architecture

### 1. Layout and build (ssg pattern, flat directory)
- **`base.cpp`**: copied from `~/src/skeeto.github.com/_ssg/base.cpp`. Its `Arena` gains a `Mem*` so the slow path can commit more memory or escape on OOM.
- **`unabashed.cpp`** holds the platform API table, the include order and `sh_main(Os *, OsInfo *)`. It includes these modules:
  - `mem` (Mem pools, the size-class Heap, the copier and image), `unicode` (generated tables), `err`, `num` (integers, bignum, soft x87 `long double`), `wide` (UTF-16/WTF-8, MSVCRT/Cygwin/cmd encoders and splitters), `paths`;
  - `lex`, `parse`, `print`, `vars`, `array`, `arith`, `pattern`, `regex`, `expand`;
  - `io` (File, Fdtab, Pipe, redirections, heredocs), `sched`, `proc` (vprocs, pipelines, procsub, jobs), `exec`, `spawn`, `startup`;
  - `b_*.cpp` builtins;
  - `history`, `histexp`, `edit`, `complete`, `inputrc`, `vtcon`.
- **Platform layers:**
  - `main_win32.cpp`: CRT-free; kernel32 statically linked; advapi32 and ntdll entry points resolved at run time.
  - `main_posix.cpp`.
  - `main_test.cpp`, which includes `vos.cpp` (the virtual OS), `fakes.cpp`, `vterm.cpp` and `test/*_test.cpp`.
  - `main_fuzz.cpp`.
  - `oracle.cpp`, the runner for the bash suite.
- **Other directories:** `t/` (spec tests), `bench/`, `tools/` (record, difftest, ptyrun, sandbox runner, `Dockerfile.bash53`), `notes/`.
- `bash-5.3.tar.gz` and `third_party/` are gitignored. M0 runs `git init`.
- **Builds:**
  - One command each, for example `g++ -std=c++20 -nostartfiles -o bash.exe main_win32.cpp` and `g++ -std=c++20 -o bash main_posix.cpp`.
  - Debug: `-g3 -fsanitize=undefined -fsanitize-trap`.
  - A Makefile and an optional CMakeLists.txt are for convenience only.
  - The i686 build must import no CRT or toolchain DLLs and must have PE subsystem version 5.01 or lower.

### 2. Execution model (no fork for shell code)
- **Kinds of vproc.** A vproc is a virtual process: it owns a `Shell` state, a `Fdtab`, arenas and an escape point. Shell code runs in one of four ways:
  - **Synchronous vproc**, on the caller's stack: `( )` in the foreground, `$( )`, backquotes, and the last shell stage of a pipeline (or the current shell under lastpipe).
  - **Current vproc, with fd 1 sent to a capture buffer:** `${ cmd; }` and `${| cmd; }`.
  - **Fiber vproc:** every shell stage of a pipeline except the last, `<(f)`/`>(f)`, and also `&` and coproc until M5b.
  - **Real process from M5b:** `&`, coproc, and the shell stages of a background pipeline. POSIX uses `fork`. Win32 and the virtual OS self-spawn with a serialized state image.
- **External commands** are always spawned directly, never through an intermediate shell. That includes `ext &`, `(ext)`, `$(ext)` and `< <(ext)`.
- **Pipeline setup:**
  1. Create every stage's task first.
  2. Use in-memory `Pipe`s on shell-to-shell edges and OS pipes on edges that touch an external.
  3. Expand the words of an external stage inline when they are constant or plain parameters. If they contain a comsub or procsub, expand them in a short-lived launcher fiber that has that stage's fds. This avoids deadlock in `echo hi | ext "$(read x; echo $x)"`.
  4. Run the last shell stage on the current stack.
- **Scheduler** (in the core, single-threaded, FIFO):
  - It schedules fibers and small "pump" state machines.
  - A task blocks on a Pipe, an `Io`, a child exit or a timer.
  - When nothing is runnable it calls `os_wait(deadline)`.
  - A safe point (simple command, loop back-edge, function entry, every 4 KiB in heavy builtins) decrements a budget and may yield.
  - **Fast path:** with a single task, I/O is a direct blocking call.
  - **Test mode:** a seeded "chaos" yield, and `OS_DEADLOCK` reported instead of a hang.
- **Fibers:** `CreateFiberEx` on Win32 (XP). About 30 lines of asm per architecture on POSIX and in the vos. Stacks reserve 8 MiB and commit lazily.
- **Win32 I/O.** Every shell-held handle is synchronous and non-inheritable.
  - *Reads:* `PeekNamedPipe`, then a `ReadFile` of exactly the needed byte count. This gives exact consumption at full speed.
  - *Read from an empty pipe:* with a single task on Vista+, block on the main thread; Ctrl-C cancels it with `CancelSynchronousIo`. Otherwise, and always on XP, a pooled helper thread (`CreateThread` + `CRITICAL_SECTION` + event) issues the read.
  - *File state machine* (the critique's M3 issue): while a helper read is in flight, every operation on that File goes through its state machine, including close, peek and hand-off to a child. If a File is handed to a child while a read is in flight or its read-ahead is non-empty, interpose a pipe and a pump, and drain the leftover back afterward.
  - *Process exits:* `RegisterWaitForSingleObject`, so there is no 64-handle limit.
  - *Waking:* helpers, wait callbacks and the console control handler push onto a queue and `SetEvent`.
- **POSIX I/O:**
  - `poll` plus a self-pipe for signals.
  - When more than one task exists, writes wait for `POLLOUT` and send at most `PIPE_BUF` per write.
  - `O_NONBLOCK` only on ends that no external process holds.
  - Foreign pipes are read one byte at a time, as bash does; M8 adds read-ahead.
  - `exec` or exit while fibers are alive does a `fork()`, so `$$` is kept.
- **Materialization at a process boundary:**
  - An in-memory pipe becomes an OS pipe, with the bytes already buffered pumped through first.
  - A heredoc of at most 64 KiB is written synchronously into `CreatePipe(nSize≥len)` before the command runs, as bash does. A larger one goes to a delete-on-close temp file.
  - A capture used by an external becomes a pipe plus a pump. Whatever is available is drained before each builtin write, to keep ordering. EOF waits for every writer, so `x=$(sleep 9 &)` waits.
- **Spawning on Win32:**
  - Always pass the resolved path as `lpApplicationName`.
  - The core builds the command line by target kind: MSVCRT, Cygwin (detected from PE imports, cached by path and mtime), or `%ComSpec% /d /s /c` with BatBadBut escaping.
  - `CREATE_SUSPENDED|CREATE_UNICODE_ENVIRONMENT`, with inheritable duplicates of fds 0–2 under a spawn lock.
  - `lpReserved2` is passed only for explicitly redirected fds ≥3 going to MSVCRT children, with the slot width taken from the child's PE machine field.
  - `lpCurrentDirectory` comes from the vproc's virtual cwd. The process's own cwd never changes.
  - `CREATE_NEW_PROCESS_GROUP` for background jobs. Children of `exec` go into a `KILL_ON_JOB_CLOSE` job.
- **Spawning on POSIX:** fork or vfork plus exec, applying the fd map, chdir, umask, rlimits, signal dispositions and pgrp.
- **Signals and control flow:**
  - Real signals only set bits.
  - SIGINT follows bash's cooperative rule: if Ctrl-C arrived while a foreground child was being waited for, it counts as SIGINT when the child exits with 0xC000013A, 130, Cygwin's signal code, or any non-zero status.
  - In subshells, caught traps reset to default and ignored traps stay ignored.
  - `trap '' INT` is applied per spawn, never process-wide.
  - A write to a pipe with no reader is a virtual SIGPIPE (status 141).
  - `break`, `continue` and `return` are flow codes.
  - `exit`, fatal errors, signal kills and OOM `__builtin_longjmp` to the vproc's `Escape`. The catcher **replays** the undo records (restoring fds, File refs and the tempenv) and pops frames; truncating them is not enough.
- **Process ids:**
  - `$$` is the real pid.
  - `$BASHPID` is globally unique: `realpid*65536+2n+1` on Windows, `4194304*(n+1)+realpid` on Linux, deterministic in the vos.
- **Per-vproc state, not process-wide:** cwd, umask, ulimits (applied at spawn), the `enable` table, history, job table and traps.

### 3. Memory model (journal + copy + free lists)
- **Regions:**
  - **Per-vproc scratch:** the top-level command's AST, expansions, argv, glob results, frames and undo records. Passed by value per command and per loop iteration, so a top-level `while` running for hours stays flat.
  - **Per-vproc store:** a mutable hash trie of Names with shallow-bound `Value` stacks, plus arrays, assoc arrays, the function table, aliases, the command hash, args, dirstack and trap strings. Bump-allocated with **size-class free lists**.
  - **Code blocks:** function bodies are copied at definition time into refcounted blocks drawn from a pool. A block is released when its function is redefined and no frame or fiber holds it. So `source conf.sh` in a loop does not leak.
  - **Kernel free lists:** for File, Pipe, Proc and fiber records.
  - **History:** a ring.
- **Synchronous subshell:**
  - Copy the small `Shell` struct by value and record a store **watermark**.
  - The child starts with empty free lists, so it only bump-allocates above the watermark.
  - A child write to a parent-owned object (below the watermark) appends `{slot, old}` to a journal and never frees the old value. Each Value or node carries a "logged at depth" tag to avoid logging the same slot twice.
  - On exit: roll back the journal, reset the store to the watermark, and restore the parent's free lists. Nothing is left behind.
- **Concurrent vproc** (fiber, or a real process's image): the type-directed **copier** deep-copies the Shell, store, AST subtree, frames, args and tempenv into the vproc's own Mem. Code blocks are shared by refcount.
  - The same copier writes the Win32/vos `&` image with a list of pointer offsets for relocation.
  - No store memory is ever shared between live vprocs.
  - Copying costs O(state). M8 adds copy-on-first-write.
  - In a pipeline, a builtin-only `echo`/`printf` stage is expanded in the parent and run into an unbounded buffer, with no fiber at all.
- **Ownership:**
  - Every Value owns its buffer. Assignments and positional parameters copy.
  - `s+=x` appends in place within `cap`, then doubles.
  - Freed buffers go on a **deferred** list that is released at the next statement boundary. This keeps zero-copy `"$x"` expansions valid while the same command reassigns `x`.
  - Shared AST nodes never point into transient memory. Node caches (compiled pattern, arithmetic text cache, re-parsed comsub) use fixed slots or per-vproc side tables reset together with scratch.
- **Reservations** (reserve, then commit lazily):
  - 64-bit: store 16 GiB, scratch 4 GiB.
  - 32-bit XP (large-address-aware): store 256 MiB, scratch 128 MiB, about 16 MiB per pooled vproc.
  - Test and fuzz builds use small arenas so the OOM paths are reachable.
  - On OOM the vproc prints bash's xmalloc message and exits 2.
- **Deep recursion:** a stack-left check emulates bash's outcome: "Segmentation fault", status 139.

### 4. Core representations
- **Parser:**
  - Hand-written and incremental. The parse unit is bash's `inputunit`, a whole line, so the same-line rules for aliases and extglob hold.
  - Aliases are expanded on the input stack: trailing-blank rule, recursion guard.
  - A comsub is parsed with aliases off and reprinted canonically into the word text. At execution it is re-parsed into scratch only if `expand_aliases` is on and a command-position word is currently an alias. POSIX mode expands at parse time. Backquotes are always parsed at execution.
  - Scripts are read in 64 KiB chunks, and the offset of a stdin script is synced before each command.
- **Word:**
  - canonical `text`, used for `declare -f`;
  - a pre-parsed `parts` tree;
  - `lit` for constant words, which expand with no copies.
  - "Lazy-construct": anything bash parses at expansion time is pre-parsed speculatively. On failure the text is kept, and the error is raised at run time with bash's message and timing.
- **Printing:** `print.cpp` reproduces bash's layout from observed output, locked by goldens. One quoting engine has separate modes for xtrace, `%q`, `@Q` and `declare -p`. `$BASH_COMMAND`, job text and `$_` are copied out before scratch resets.
- **Arithmetic:**
  - Postfix code compiled at parse time. A syntax error compiles into an error instruction that raises bash's message when evaluated.
  - Values are i64 with wraparound for `+ - *`. `>>`, `/`, `%` and comparisons stay signed. `INTMAX_MIN/-1` follows bash.
  - Integer variables are stored natively.
  - A body containing `$` gets a per-node text cache.
- **Patterns:** compiled once. A two-pointer star matcher; memoized backtracking for extglob; single-pass leftmost-longest `${s//p/r}`. They work on UTF-8 bytes. Globbing reads each directory once, with no per-entry stat.
- **ERE for `=~`:** a Pike VM finds the leftmost-longest span, then a backtracker assigns POSIX submatches. glibc extensions (`\w \s \b \< \>`, back-references) are included. glibc is the oracle through differential fuzzing.
- **Variables:**
  - Names carry a parse-time hash. Lookup is one trie descent, whatever the call depth.
  - `FUNCNAME`, `BASH_SOURCE` and `BASH_LINENO` are built from `frames` only when read.
  - `RANDOM` uses the exact Park–Miller step.
  - Assoc arrays reproduce bash's hashlib order: FNV-1 over signed char, insertion at chain head, growth ×4 when `n ≥ 2*nbuckets`. `nbuckets` comes from the creation path (1024 for new, 128 when converted from an existing variable) and survives reassignment.
- **Indexed arrays:** a dense vector while dense, otherwise a radix trie.
- **Environment block:** cached by an export generation. The platform caches its native form.
- **printf floats:** soft x87 80-bit arithmetic with correctly rounded parsing and exact output, using an arena bignum.
- **Reference platform for bit-exact quirks:** x86-64 glibc. That fixes assoc order, 80-bit printf, Linux signal numbers and glibc `strerror` text, with Win32 errors mapped to errno first. UTF-8 collation is codepoint order; glibc locale collation is a later fidelity item, and oracles run under C/C.UTF-8.

### 5. Platform API (prototype table in `unabashed.cpp`; full sketch in `notes/design/synthesis.md` §3.4)
- **Handle type:** `using Handle = uz;`.
- **Memory:** `os_reserve`, `os_commit`, `os_decommit`, `os_oom`.
- **Fibers:** `os_fiber`, `os_switch`, `os_fiberfree`, `os_stackleft`.
- **Files:**
  - `os_open`, `os_tempfile`, `os_read`/`os_write` with `IO_BLOCK`/`IO_TRY`, `os_peek`, `os_seek`, `os_truncate`, `os_close`, `os_ftype`;
  - `os_pipe(named)`: a named pipe for procsubs given to native children;
  - `os_stat`/`os_fstat`, `os_list` (with type), `os_access`, `os_readlink`, `os_realpath`, `os_userhome`.
- **Processes:** `os_spawn(Spawn*)`, `os_fork` (POSIX only), `os_exec`, `os_kill`, `os_exit`.
- **Asynchronous work:** `os_submit(Io*)`, `os_wait(deadline)`, `os_cancel`, `os_sigpending`, `os_sigmode`.
- **Terminal:** `os_tty` (VT, legacy or pty-pipe; columns, rows, xenl), `os_ttymode`. Input always arrives as xterm bytes; legacy key events are converted in the platform. Output is always the core's VT subset; `vtcon.cpp` interprets it on a legacy console.
- **Time:** `os_now`, `os_clock`, `os_times` (Win32 uses job-object accounting), `os_localtz`, `os_entropy` (RtlGenRandom on XP).
- **Startup:** `OsInfo` carries `exe`, `cwd`, the raw command line, args, env, pids, ids, host, user, home, `std[3]`, the image and the memory configuration.
- **Placement:** the platform sees only canonical native absolute paths. All path semantics live in the core (`paths.cpp`, `wide.cpp`, `vtcon.cpp`), so the vos's Windows personality tests them on Linux.
- **POSIX cwd:** the main vproc uses the real `chdir`. Other vprocs join paths against the physical cwd without collapsing `..`.

### 6. Paths and Windows integration (exact rules in `notes/research/msys2.md`)
- **msys mode** (when invoked as bash):
  - **Mount table:**
    - `/` is the install root: the exe directory minus `/usr/bin` or `/bin`, plus `etc/fstab` if present.
    - `/bin` and `/usr/bin` map to `<root>/usr/bin`; `/tmp` maps to `%TEMP%`.
    - The cygdrive prefix is `/`, so `/c/x` is `C:/x`.
    - Longest prefix wins. Mount points match case-insensitively, but `/dev` and `/proc` are case-sensitive.
  - **Virtual files:** `/dev/null` maps to `NUL`; `/dev/tty`; `/dev/fd`; a minimal `/proc` (self, PID/comm, cygdrive).
  - **Output form:** `/c/...`, matching `cygpath -u`.
  - **Environment:**
    - At startup, PATH and ORIGINAL_PATH are converted Windows → POSIX, as are HOME, SHELL, TMPDIR, TMP and TEMP.
    - The renv names are uppercased; `=X:` entries are kept.
    - Native children get conversion back plus the argument heuristic, honouring `MSYS2_ARG_CONV_EXCL`, `MSYS2_ENV_CONV_EXCL` and `MSYS_NO_PATHCONV`. The heuristic's misfires are reproduced exactly.
    - Cygwin children get only the conv_envvars conversion, plus Cygwin quoting.
  - **Own command line:** split with Cygwin rules (`\\` inside quotes, globbing of unquoted wildcards, `@file`).
  - **Parser input:** every `\r` is dropped; trailing `\r\n` is stripped from comsub output; `set -o igncr`.
  - **Builtins:** `cygpath` (also when invoked under that name) and `pwd -W`.
  - **Identity:** OSTYPE=`cygwin` and MACHTYPE=`x86_64-pc-cygwin`, which Git Bash 5.3.15 printed when probed. Option lists include `igncr` and `completion_strip_exe`.
  - **Executables:**
    - `foo` resolves to `foo.exe`, and `type` shows it without `.exe`.
    - The exec bit comes from the noacl rule: `.exe`/`.com`, or content starting with `MZ`/`#!`, cached.
    - The shell runs shebangs itself: `/bin/sh`, `/usr/bin/env X` and `/bin/bash` map through the mount table or to the shell itself.
- **native mode** (sh, ash, lash):
  - A leading `/` means the root of the current drive; `C:` means that drive's cwd.
  - Output uses `C:/` forward slashes, and PATH is `;`-separated.
  - Busybox-w32 import conventions: uppercased names, forward-slashed values except COMSPEC/SYSTEMROOT, USER/LOGNAME/HOME/SHELL defaults.
  - Extensions are tried in the order `.com .exe .sh .bat .cmd`.
  - Startup files: `<sysdrive>/etc/profile`, `<exe>/../etc/profile`, `~/.profile`, with no chdir on login.
  - The `cmd //c` hack is kept.
  - Busybox applets are preferred: busybox.exe is spawned directly with argv[0] set, bypassing the alias stubs, and `BB_OVERRIDE_APPLETS` is honoured. Hot NOFORK applets may later run in-process.
  - VT output mode is enabled at startup.
- **Both modes:**
  - Inputs `C:/`, `C:\` and `//srv/share` are accepted.
  - CRLF is tolerated: in msys mode as above, in native mode as busybox does it.
  - `2>nul` is the device.
  - Reserved DOS names are handled.
  - `\\?\` long paths are supported.
  - The mode shopt is hidden from `shopt` listings and BASHOPTS. Toggling it re-renders PATH and PWD.

### 7. Line editor
- One VT-emitting core:
  - termios backend on POSIX;
  - Win32 console in VT mode (Win10+) or `vtcon` on the legacy console (XP/7, no xenl);
  - raw VT over the pipes of an mintty pipe-pty (low priority).
- Keymaps and functions are modelled on readline's funmap; `inputrc.cpp` parses inputrc files.
- **Redisplay:** prompt `\[ \]`, wide characters, wrapping with and without xenl, SIGWINCH and `WINDOW_BUFFER_SIZE_EVENT`.
- **Win32 input:** `ReadConsoleInputW`. Text comes from `UnicodeChar`, including `VK_PACKET` and surrogate pairs. AltGr is treated as text.
- Console modes are saved and restored around every foreground child.


## Testing and safety
- **Hard rule:** fuzzing, the chaos tests and randomized tests run **only** on the vos: in-memory filesystem, fake programs, virtual clock. There, a real filesystem call or CreateProcess traps.
- **Unit tests** (ssg-style `expect`, table-driven, arenas poisoned with 0x55):
  - lexer/parser/printer round trips;
  - arithmetic;
  - patterns against a naive reference, ERE against a backtracker;
  - the bignum float formatter;
  - paths in both modes;
  - `wide.cpp` quoting through a simulated CRT parser;
  - `vtcon`;
  - journal and copier invariants;
  - assoc order for every creation path.
- **vos spec tests** (`t/*.spec`: script, stdout, stderr, status, personality):
  - Expected output is recorded by `tools/record`, which runs bash 5.3 in Docker (built from the tarball).
  - Inside that container, `fakes.cpp` is built as a Linux multi-call binary and is the only thing on PATH. The container's initial filesystem is built from the same manifest as the vos, so the fake programs match exactly.
  - Only our own scripts and the recorded outputs are committed.
- **bash's test suite as an external oracle** (`oracle`, `BASH_TESTS=<extracted path>`, read-only):
  - It reimplements the run-* contract.
  - Categories a, b and the faked part of c run on the vos. Categories c, d and e run on `main_posix` in a disposable non-root container with `--network none` and a tmpfs workdir.
  - Only a manifest is committed: test name, status, reason. No test content is ever copied.
- **Differential testing:** stdout, stderr, status, the merged stream and a final `declare -p`, normalized for paths, pids and time. Run against:
  - bash 5.3 in a container;
  - Git Bash 5.3.15, for msys path, argument and environment snippets only.
- **Fuzzing** (AFL++ in WSL, vos only, persistent mode, small arenas). Each fuzzer checks one property:
  - parse: parse→print→parse reaches a fixed point;
  - run: no crashes or deadlocks, and the OOM paths work;
  - chaos: output is the same for every seed, compared per writer;
  - isolate: `( S ); declare -p` equals `declare -p`, excluding `_ RANDOM SRANDOM SECONDS EPOCH* LINENO BASH_COMMAND PIPESTATUS BASH_LINENO HISTCMD`;
  - image: a round trip of the shell state is lossless;
  - arith, glob, regex and printf against references;
  - editor: random key bytes into the vterm.
- **Windows integration tier** (real filesystem and CreateProcess, curated scripts only, never fuzzed):
  - `tools/sandbox` runs tests under a Low-integrity restricted token plus a `KILL_ON_JOB_CLOSE` job, inside a Low-labelled scratch directory.
  - XP spot checks over ssh in `~/src/scratch`.
- **Line editor:** headless vterm goldens, with xenl on and off; `ptyrun` differential against bash with readline; end-to-end runs with `winctl` on Win11 conhost, Windows Terminal and the XP laptop. A hidden console-dump helper avoids pixel reading.
- **Benchmarks** (`bench/`):
  - perf.md B1–B13 plus macros: startup, sourcing bash-completion, a configure-style workload, an AI-agent `-c` batch.
  - Shells compared: bash 5.3 (Linux), Git Bash/MSYS2 bash and busybox sh (Windows).
  - Checksums must match before timing. hyperfine needs installing.
  - Gates use vos instruction counts.


## Milestones (each ends runnable, with exit criteria)

| | Deliverable | Exit criteria |
|---|---|---|
| **M0** Foundation | `git init`; copy reports to `notes/`; base, Mem, API table; the 4 layers plus oracle stub; fibers on all layers; vos skeleton; `Dockerfile.bash53` and `tools/record`; sandbox runner; `-c 'echo hi; exit 3'` | x64 and i686 cross-builds; no CRT imports; PE 5.01; runs on XP; fiber ping-pong on every layer |
| **M1** Parser and printer | full grammar, heredocs, aliases, comsub/funsub parse and reprint, alias-in-comsub rules, syntax-error text, canonical `declare -f` printing | parse fuzzer reaches the fixed point with no crashes in 24 h; recorded goldens match |
| **M2** Sequential interpreter | store and journal; expansions; arithmetic; compound commands; functions and locals; virtual fds; synchronous `( )` and `$( )`; `[[ ]]` without `=~`; synchronous externals with the minimal native path layer; P0 builtins; `set -eux`; LINENO; message parity; CRLF | run-minimal passes on the vos; 1e7-iteration loops keep RSS flat; B1, B2, B7, B9, B10 beat bash |
| **M3a** Concurrency | scheduler and fibers; Pipes; launcher fibers; procsub; heredocs; capture materialization; PIPESTATUS, pipefail, lastpipe; traps and SIGPIPE; POSIX I/O rules; fork-on-exec; interim fiber `&`, wait and coproc | category a green on the vos; chaos and isolate fuzzers clean; B8, B12 ≥10x bash |
| **M3b** Win32 I/O | helpers and the File state machine; Peek-exact reads; RegisterWait; wake queue; Ctrl-C with the cooperative rule | sandbox integration tests pass; `f \| while read` plus Ctrl-C works in conhost and on XP |
| **M4** Data and matching | arrays, assoc, namerefs, ERE, glob/extglob/globstar/GLOBSORT, printf floats, read/mapfile/getopts, `declare -p`, shopt/set lists | category b green; regex and glob differential fuzzing clean; B3–B6, B11, B13 beat bash |
| **M5a** Windows paths and spawning | everything in §6; quoting by target kind; PATH cache; persona from argv[0]; startup files | Git Bash differential corpus green; trial w64devkit `sh.exe` builds autoconf projects and runs make; agent batch ≥5x Git Bash; configure workload vs busybox sh |
| **M5b** Real background processes | copier image; self-spawn `&` and coproc; `os_fork`; signal channel (named section bitmask plus event); real-pid kill, wait and jobs | category c in the container; image fuzzer clean; `sh -c 'server &'` returns immediately |
| **M6** Editor 1 | terminal layer, redisplay, full emacs keymap, kill ring, undo, isearch, history with `!`/fc, completion (columns, query, pager), inputrc basics, prompts, bracketed paste | vterm goldens; ptyrun differentials; winctl end to end on Win11 and XP; editor fuzzer clean |
| **M7** Job control and fidelity | POSIX job control (shell parts of foreground jobs on fibers under `set -m`), POSIX mode, invocation, DEBUG/ERR/RETURN and extdebug, dirstack, select, ulimit, remaining builtins, original help text, rbash | category d and e pass list frozen in the manifest |
| **M8** Performance and editor 2 | read-ahead pump, coalescing output to regular files, binding cache, copy-on-first-write fibers, 32-bit tuning; vi mode, bind, inputrc conditionals and macros, complete/compgen/compopt, bash-completion compatibility | every benchmark beats bash 5.3, Git Bash and busybox sh; the bash-completion sourcing benchmark beats bash |
| **M9** w64devkit | Dockerfile build, bash/sh/ash/lash names, `w64devkit.c` launches the new shell, original man page, x64 and x86 releases | w64devkit runs entirely on unabashed; busybox `ash` removed from the alias list |


## Decisions taken without asking (recommendations from the design panel)
- `&` and coproc become real processes from M5b; they are fibers in the interim.
- Windows job control covers jobs, wait, kill and fg of running jobs. Ctrl-Z is deferred.
- Output is coalesced only to regular files, and flushed before any spawn, block or exit.
- `time`/`times` count in-process subshell CPU as shell time; this is a documented divergence.
- busybox-w32 ash's `ash_test` corpus is a secondary POSIX-subset check.


## Verification
- `make test`: the unit tests plus every `t/*.spec` on the vos, from a debug build with UBSan trapping.
- `tools/oracle`: pass counts on the bash suite per category, tracked in the manifest and never allowed to regress.
- `tools/difftest` against bash 5.3 in Docker, and against Git Bash in the sandbox for msys snippets.
- `afl-fuzz` campaigns, vos only, in WSL.
- `bench/run`: comparison tables on Linux and Windows.
- Release checks:
  - `objdump -p` / `peports` show no CRT or toolchain DLL imports;
  - the i686 build runs on the XP laptop;
  - interactive use is driven through `winctl` on Win11 conhost, Windows Terminal and XP;
  - a w64devkit build with `sh.exe` replaced runs real `make` and configure projects;
  - Claude Code's Bash tool works with `CLAUDE_CODE_GIT_BASH_PATH` pointing at `bash.exe`.
