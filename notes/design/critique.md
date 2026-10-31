# Adversarial review of the unabashed architecture

**Verdict.** The overall shape holds up: a vproc core, fibers only when work is concurrent, gen-COW stores, and real processes for `&` from M5. But the memory and aliasing rules are underspecified. Several of the gaps below break isolation or cause use-after-reset in common scripts. Four of the proposal's own decisions are wrong when checked against the bash source or a live Git Bash:
- **OSTYPE/MACHTYPE (§4.7).** Recommendation 7 is wrong. Git Bash 5.3.15 prints `cygwin x86_64-pc-cygwin` (probe below).
- **Assoc bucket count.** It is not a constant 1024 (C7).
- **Alias expansion in comsubs.** Default mode does not expand aliases at parse time (M7).
- **Option lists.** They depend on the mode (M11).

Probes run against `C:/Users/wellons/git/usr/bin/bash.exe`:
- `echo $OSTYPE $MACHTYPE` prints `cygwin x86_64-pc-cygwin`.
- `set -o` contains `igncr` (28 lines).
- `shopt` contains `completion_strip_exe` (60 lines).

---

## CRITICAL

**C1. The binding cache is keyed by the address of a `Name` in scratch, and it writes past COW.**
- *Scenario A:* top-level command 1 is `echo $foo`. Its `Name` sits at address X and is cached to foo's `Value**`. Scratch resets. Command 2, `echo $bar`, allocates its `Name` at the same address X and hits the cache, so it prints `$foo`.
- *Scenario B:* the parent caches `i`. A fiber is forked, both sides take new gens, and the parent's later `i=5` goes through the cached `Value**`. That mutates a trie node below the pin, which the fiber can see.
- *Fix:* each cache entry stores `{Name*, hash, scratch epoch, Vars node gen}`, and a hit requires all of them to match. Bump `varsver` on every gen change, every fork and pin, and every top-level scratch reset. A cached write must still path-copy when `node->gen != p->gen`. Child Procs start with an empty cache; never copy it by value.

**C2. String buffers are aliased, and they are mutated in place.**
- "Reuse the old buffer when the new value fits" combined with zero-copy assignment or expansion breaks:
  - `y=$x; x=$(cmd)` overwrites y's bytes.
  - In `f "$x"`, where f then reassigns x, `$1` changes under it.
- `s+=x` "appends in place up to `cap`" also breaks. After `y=$s`, both values share data and both have `cap > len`, so `y+=a` and `s+=b` write the same bytes. Across a fiber pin this is a cross-vproc race. Copying the `Value` on COW does not help, because `str.data` still points at the shared buffer.
- *Fix:* define ownership. `cap > 0` means the value is the unique owner. Any copy of a `Str` into another Value, Args or argv, and any COW copy of a Value, sets `cap = 0` (shared and immutable). Only the owner may append or reuse.

**C3. Asynchronous work outlives the scratch it points into.**
- Every one of these keeps running after the top-level scratch resets:
  - `while :; do date; sleep 1; done &` (a fiber until M5);
  - `exec 3< <(producer_fn)`;
  - `coproc f`;
  - a procsub created inside a function: `f(){ exec 4< <(g); }`.
- They hold the AST of the top-level command, the caller's `frames` (for FUNCNAME), argv, and the tempenv, all of which live in that scratch. The pin mechanism only covers the store.
- The same problem hits the lazily filled caches on long-lived AST nodes: the compiled pattern, the "last text, last code" arithmetic cache, and the re-parsed comsub tree. If a fiber or command allocates the cache in its own transient arena and stores a pointer on a shared function-body node, that pointer dangles after the arena is reset.
- *Fix:*
  - When a fiber that may outlive the command is created, copy its AST subtree, a snapshot of its frames, and its args and tempenv into the fiber's own Mem. Use the image copier, which is cheap for small bodies.
  - Node caches either use a fixed slot owned by the node's arena (reuse it if the new value fits, otherwise don't cache), or live in a per-vproc side table keyed by `(node, epoch)` that is reset together with scratch. Never store a pointer to transient memory in shared AST.

**C4. The `Escape` handler truncates `undo` and `frames` instead of unwinding them.**
- *Scenario 1:* `{ echo a; exit 3; } | cat`. The fiber's escape must close its pipe write end, otherwise `cat` never sees EOF.
- *Scenario 2:* in an interactive shell, `f(){ local x=1; { : ${y?}; } >out; }; f`. Bash restores fd 1 and pops `x`. Truncation leaves stdout redirected and the local still bound.
- *Fix:* the catcher replays the undo records down to the recorded depth (restore fds, release `File` refs, restore tempenv). It then pops frames, which restores locals, `local -` options, positional parameters and RETURN traps. When a vproc dies, all of its `Fdtab` is released.

**C5. The global `u32 gen` wraps around.**
- In-process `$(echo $i)` runs at about 1 µs, so `c.gen = ++k->gen` wraps in roughly an hour of B7-style looping.
- After the wrap, a child can get a gen equal to that of a surviving old parent node and mutate the node in place.
- *Fix:* use a u64 gen in nodes. Alternatively, give synchronous children stack-scoped gens (parent+1, reusable after return) and reserve unique gens for fibers.

**C6. A pooled `Mem` is released while descendants still pin it.**
- A fiber stage F runs `g &` (a fiber, before M5), then F exits or hits OOM, and "frees its pooled Mem". g's roots point into F's store.
- The "adopt the child's tip" rule exists only for synchronous children.
- *Fix:* keep a pin refcount on each `Mem`, and return a Mem to the pool only at refcount 0. Apply the same rule on the OOM path.

**C7. Assoc iteration order: `nbuckets` is not a constant 1024.**
- `make_new_assoc_variable` uses 1024 buckets (variables.c:2857).
- `convert_var_to_assoc` uses `assoc_create(0)`, which gives 128 buckets (hashlib.h:72). It is reached from declare.def:957, arrayfunc.c:497 and subst.c:3490. For example, `declare m; declare -A m` gets 128 buckets.
- Compound reassignment keeps the grown size: `nhash = assoc_create(h->nbuckets)` at arrayfunc.c:725.
- `BASH_CMDS` and `BASH_ALIASES` take the source table's size (variables.c:1692 and 1762).
- *Fix:* `Assoc.nbuckets` is set by the creation path and carried across reassignment. Add golden tests for each path.

---

## MAJOR

**M1. External-stage expansion before fibers start causes deadlock and serialization.**
- Step 3, "spawn externals" (with their expansion done in the parent), runs before step 4 starts the fibers.
- `echo hi | ext "$(read x; echo $x)"` deadlocks. The comsub reads the stage's stdin, which is an in-memory pipe fed by a fiber that does not exist yet. The vos would report `OS_DEADLOCK`, while bash prints `hi`.
- `a $(sleep 1) | b $(sleep 1)` takes 2 s instead of 1 s.
- *Fix:* create every stage task first. Expand external stages whose words or redirections contain comsubs or procsubs inside a short-lived "launcher" fiber that has that stage's fd table. Constant words and plain parameter words can still be expanded inline.

**M2. On POSIX, a shell task's write to an OS pipe blocks the whole process.**
- `fn_big_writer | ext_slow | while read l; do …; done`: the fiber blocks in `write()`. `ext` blocks on its full output pipe, because the main stack (the reader) never gets scheduled.
- The proposal only addresses writes on Win32.
- *Fix:*
  - When more than one task exists, `poll(POLLOUT)` and write at most PIPE_BUF per write (atomic once POLLOUT is reported, on both Linux and macOS).
  - Set `O_NONBLOCK` only on ends that no external process holds: the heredoc writer, the capture reader, and the pump ends.

**M3. Win32 helper reads collide with synchronous file-object semantics.**
- The I/O manager serializes all I/O on a synchronous file object, including duplicated handles. While a helper has a 1-byte `ReadFile` pending, the main thread blocks on any of these on that handle:
  - `PeekNamedPipe` (an FSCTL);
  - a second `ReadFile`;
  - `CloseHandle`, which hangs on XP with a pending synchronous read. Verify this on the XP laptop.
- *Scenario:* `read -t 1 x <&3` times out, leaving a helper read pending. Then `read -t 0 <&3` or `exec 3<&-` hangs the shell.
- A child given that handle loses the stolen byte, and its own reads wait behind the helper.
- *Fix:*
  - While a read is in flight, every operation on that `File` goes through its state machine; never call into the OS handle directly.
  - Closing hands the handle to the helper, which closes it after its read returns.
  - Handing the File to a child while a read is in flight or `ahead` is non-empty interposes a fresh pipe and a pump. When the child exits, drain whatever is left in the interposed pipe back into `File.ahead`, so no bytes are lost or over-consumed.
  - On Vista+, prefer `CancelSynchronousIo` for timeouts, retried until acknowledged because a cancel can arrive before the read starts.

**M4. Interactive job control conflicts with synchronous vprocs on the main C stack.**
- Ctrl-Z on `( long loop )`, or on `ext | while read; do …; done`, must stop the job and return to the prompt. That cannot happen while the loop is a frame on the main stack.
- *Fix:* when `set -m` is on, run the shell parts of every foreground job on fibers, with the main stack acting as the prompt task. On POSIX, real fork for foreground subshells is also acceptable. Put this in the job-control milestone, not as "Windows partial" in M6.

**M5. The compaction invariant is not enforceable as written.**
- Expanding `f "$x" "$(g)"` keeps `"$x"` zero-copy (pointing into the store) while `$(g)` runs shell code. If compaction happens at one of g's safe points, the already-expanded argv dangles.
- `execfunction` holding the saved `Args` in a C local is another instance.
- *Fix:*
  - Compact only at clean points: a top-level boundary, or a loop back-edge, with `p->nocompact == 0`. Increment `nocompact` for the whole of any expansion, redirection or builtin invocation.
  - Expansions of store strings either copy into scratch or run under `nocompact`.
  - Make the copier iterative (Cheney scan with a type tag in each object header). Recursion overflows 512 KiB fiber stacks on long Args or `below` chains.
  - Rebuild tries by reinserting live keys, so that unset or dead keys (`eval v_$i=…; unset v_$i` in loops) and their trie nodes are dropped.

**M6. The code arena leaks.**
- "Compacted only at main top level" never fires inside a long-running top-level loop. For example, `while :; do source conf.sh; sleep 5; done` where conf.sh defines functions, or `eval "f(){…}"` per iteration. On 32-bit XP this hits OOM at 64 MiB.
- *Fix:* give each function body its own block chain, refcounted by active frames, and release a block to the Kernel block pool when the function is redefined and not executing.

**M7. Alias semantics for comsubs.**
- In default mode, bash turns alias expansion off while parsing a comsub (parse.y:4529-4534; bashref around line 9205) and expands at execution.
- The proposal re-parses only when the alias generation changes. That is wrong for this case:
  ```
  shopt -s expand_aliases
  alias e=echo
  f(){ x=$(e hi); }    # defined on a later line
  f
  ```
  The generation is unchanged, so the cached tree (with no aliases expanded) gives "e: command not found". Bash prints `hi`.
- *Fix:*
  - Parse the comsub with aliases off and print it unexpanded. In POSIX mode, expand at parse time and print the expanded form.
  - Record the command-position candidate words (including chains that follow a trailing blank).
  - At execution, if `expand_aliases` is on and any candidate is currently an alias, re-parse into scratch; otherwise reuse the cached tree.
  - Backquotes are always parsed at execution.

**M8. POSIX path resolution diverges.**
- Every OS path becomes a "canonical native absolute" path built from the logical PWD. After `cd /a/link`, `cat ../x` resolves lexically to `/a/x`, while the kernel resolves to `<target-parent>/x`.
- After `cd d; mv ../d ../e`, `ls` should still work in the renamed directory.
- Windows resolves `..` lexically, so Win32 is fine.
- *Fix:* on POSIX, the main vproc uses the real `chdir`. Other vprocs hold an `O_PATH`/`O_DIRECTORY` dirfd and use the `*at()` calls, and `fchdir` in the spawned child. The API gains a `(dir handle, relpath)` form, or the core joins with the physical cwd and never collapses `..`.

**M9. `exec`, or exit, with live fibers on POSIX.**
- `exec > >(while read l; do echo "[$(date)] $l"; done); exec server` cannot `execve`, so `$$` is lost (pidfiles, Docker entrypoints).
- `sh -c 'f &'` blocks make until M5.
- *Fix (POSIX, immediate):* when `exec` or exit happens with live fibers, `fork()`. The child keeps the scheduler minus the main task; the parent execs or exits.

**M10. Process-wide state is not virtualized.**
- `ulimit` is missing entirely. `( ulimit -n 64; ext )` would change the parent. Make it per vproc and apply it in the child before exec, with `Spawn.rlimits`. Several suite tests use `ulimit` (builtins, procsub, redir10, vredir6).
- `trap '' INT` mapped to `SetConsoleCtrlHandler(0,1)` is process-wide. In `( trap '' INT; ext )` it would also make the parent ignore Ctrl-C. Apply it per spawn only, by toggling around `CreateProcess` or using `CREATE_NEW_PROCESS_GROUP`.
- Isolation leaks from Kernel-level shared state:
  - the builtin table in `Kernel.perm`: `( enable -n echo )` disables echo in the parent;
  - history: `( history -s x )` changes the parent's history;
  - the job table.
- Move all three into `Shell` with COW. Also, `$(jobs)` must list the parent's jobs.

**M11. Mode-dependent option lists and the toggle shopt.**
- Msys mode must reproduce Git Bash's lists: `set -o` includes `igncr` and `shopt` includes `completion_strip_exe` (both verified). Linux and native mode use the 5.3 lists.
- The new path-mode shopt must be unlisted: it works with `-s`, `-u` and `-q`, but is absent from bare `shopt`, `shopt -p` and BASHOPTS. Otherwise shopt.tests and BASHOPTS diverge.
- OSTYPE/MACHTYPE in msys mode should be `cygwin` and `x86_64-pc-cygwin`.

**M12. CRLF handling is missing entirely.**
- Msys mode must:
  - drop every `\r` in parser input;
  - strip a trailing `\r\n` from comsub output;
  - provide `set -o igncr`.
- Native mode follows busybox: strip CR in CRLF pairs in script input, in comsub output, and in `read`.
- Line-oriented `ReadFile` reads on the console return `\r\n`, which must be converted.
- Agents produce CRLF scripts, so this matters in practice.

**M13. The ash persona may be slower and behave differently.**
- Busybox `SH_STANDALONE` and `PREFER_APPLETS` run applets before any PATH lookup.
- About 40 applets are NOFORK and run in-process, including `rm mkdir rmdir touch test [ printf echo basename dirname pwd uname which realpath readlink truncate` (grep of busybox-w32 for `APPLET_NOFORK`).
- On a configure workload, `rm -f conftest*` or `mkdir` thousands of times is free in ash, but costs a `CreateProcess` of busybox.exe each time for us. That is a likely loss to ash, contrary to the performance goal.
- PATH-first lookup also changes which `sed` runs.
- *Fix:*
  - Native mode prefers applets (spawn busybox.exe with argv[0] set, honour `BB_OVERRIDE_APPLETS`).
  - Add a gate: the M3 configure benchmark against busybox sh.
  - Consider our own in-process implementations of the hottest NOFORK utilities (rm, mkdir, rmdir, touch, basename, dirname, uname) in native mode only, using busybox's message formats.
  - On legacy consoles, builtin output goes through `vtcon`, as busybox emulates ANSI. Enable VT output mode at startup in native mode, as busybox's `terminal_mode` does.

**M14. Codepoint collation will visibly diverge.**
- Glob results (`for f in *`) and `[[ a < b ]]` use `strcoll`. Under en_US.UTF-8 (the WSL Debian default) mixed-case directories sort differently from codepoint order.
- `printf "%'d"` also groups digits in en_US.
- *Fix:* two UTF-8 sub-modes. `C.UTF-8` uses codepoint order. Other `*.UTF-8` locales get collation derived from DUCET with glibc tailoring, checked by differential fuzzing, plus `,` digit grouping.

**M15. Gaps in the testing oracles.**
- *Fakes:* recorded Docker outputs only match the vos if the fake programs are identical. Build `fakes.cpp` as a Linux multi-call binary, put only it on PATH in the recording container, and build the container's initial filesystem from the same spec manifest as the vos.
- *Chaos:* chaos-seed invariance reports legitimate races (`f & g & wait`, two stages writing stderr) as bugs. Compare per-writer substreams (tag each write with its vproc) plus the final state.
- *Isolation:* the `isolate` oracle must exclude `_ RANDOM SRANDOM SECONDS EPOCH* BASH_MONOSECONDS LINENO BASH_COMMAND PIPESTATUS BASH_LINENO HISTCMD`.

**M16. No Windows integration tier respects "never touch the real FS".**
- This machine runs Win11 Home, which has no Windows Sandbox, no Hyper-V and no Windows containers.
- `main_win32` (helper threads, cancellation, `RegisterWait`, `CreateProcess`) and the Git Bash differential suite still need real runs.
- *Fix:* a runner that uses a Low-integrity restricted token plus a job object, writing only inside a Low-labelled scratch directory, and/or runs on the XP laptop. Get explicit user sign-off on this tier.

**M17. The Ctrl-C cooperative rule on Windows.**
- MSYS children and many native ones do not exit with 0xC000013A. `while :; do msys_sleep 10; done` then cannot be broken with Ctrl-C.
- *Fix:* treat 0xC000013A, 130 or Cygwin's signal-death code as SIGINT. Alternatively, treat any non-zero exit as SIGINT if CTRL_C arrived during the wait. Measure the Cygwin code first.

**M18. A 512 KiB fiber stack on 32-bit is too small.**
- Recursion inside a pipeline stage "segfaults" long before it would in bash.
- *Fix:* reserve 8 MiB and commit a small amount (`CreateFiberEx` takes separate commit and reserve sizes).

**M19. Nested synchronous comsubs drain the pool.**
- Each level takes a pooled scratch `Mem`. On 32-bit (8 MiB each), recursive walkers such as `walk(){ … $(walk "$d") …; }` exhaust the address space, or hit "fork: retry", within tens of levels. Bash succeeds.
- *Fix:* synchronous children use the parent's free scratch by value. The capture grows from the high end of a double-ended arena, as u-config allocates from the end. Pooled Mems are only for fibers.

**M20. `$BASHPID` must be globally unique.**
- Concurrent scripts that build names like `/tmp/lock.$BASHPID` collide if virtual pids are small odd counters.
- *Fix:* encode the real pid. On Windows use `realpid*65536 + 2n+1`, which is odd and never a real pid. On Linux use `4194304*(n+1) + realpid`.

---

## MINOR

1. **Heredocs.** Bash writes a pipe heredoc synchronously before running the command (redir.c:450-482). Do the same with `CreatePipe(nSize ≥ len)`, with no pump. A failed pump write must never raise a virtual SIGPIPE. For a temp-file heredoc, the duplicated handle shares the file pointer, so seek it back to 0. `${ ext; }` should give externals a temp file (bash uses an anonymous file) rather than a pipe and pump.
2. **Procsub for native children.**
   - Rewrite `/dev/fd/N` occurrences inside argv words (`--f=/dev/fd/63`) at spawn time; the expansion itself must stay `/dev/fd/63`.
   - `ConnectNamedPipe` blocks, so do it in a helper. If the child never opens the path, the main thread opens the client end itself to unblock the helper.
   - `FILE_FLAG_FIRST_PIPE_INSTANCE` needs XP SP2.
   - Verify that MSYS children accept `//./pipe/...`.
3. **Dangling text pointers.** `$BASH_COMMAND` in an EXIT trap after the last command, `jobs` text for background jobs, and `$_` all point into reset scratch. Print or copy them at the reset or at job creation.
4. **Arithmetic.** "u64 wraparound" must still keep `>>`, `/`, `%` and comparisons signed (`$((-8>>1))` is -4).
5. **RANDOM.** Implement the exact Park–Miller step and `(rseed>>16)^(rseed&65535)` when compat > 50 (lib/sh/random.c), plus subshell reseeding.
6. **Win32 error texts and status codes.** Map Win32 errors to errno and then to glibc text (Cygwin's mapping in msys mode). Map NTSTATUS beyond 0xC000013A (access violation gives 139 and "Segmentation fault"; DLL not found gives 127 and a message).
7. **Spawning details.**
   - Use `CREATE_NO_WINDOW` when the shell has no console.
   - Put children of `exec` into a `KILL_ON_JOB_CLOSE` job, so agent timeouts that kill the shell do not orphan them.
   - Never `KILL_ON_JOB_CLOSE` background jobs.
   - Handle the XP environment-block limit of 32767 characters in total.
   - A command line over 32767 characters gives "Argument list too long" and status 126.
8. **Signals to our own shell children.** One named event carries no signal number. Use a named section holding a bitmask, plus an event.
9. **Script input.**
   - Read scripts in 64 KiB chunks, not whole files (makeself installers, 32-bit address space).
   - Sync the stdin-script offset before each command (the suite's input-test).
   - The parse unit must be bash's `inputunit` (a whole line), so that the same-line rules for alias and extglob hold.
   - Inherited overlapped std handles need `ReadFile` with an `OVERLAPPED` and an event. Detect them with `NtQueryInformationFile(FileModeInformation)`.
10. **Time accounting.** `time`/`times` count CPU time of in-process subshells as shell time rather than children's. Document this.
11. **Style.** `Handle` should be `uz` per the house style. On POSIX, reserve with `PROT_NONE` plus `MAP_NORESERVE` so that `overcommit_memory=2` works.
12. **Msys `/proc`.** Provide a minimal `/proc` (`/proc/self`, `/proc/PID/comm`, `/proc/cygdrive`), which Claude Code's snapshot reads.
13. **Oracle input.** The oracle harness, not the vos, reads `BASH_TESTS` read-only and copies it into the VFS.

---

## Milestones and effort

- **M2.** "Non-subshell run-minimal" covers almost nothing, because nearly every test uses `$( )` or `$THIS_SH`.
  - Move synchronous `( )` and `$( )` (no concurrency) and `[[ ]]` without `=~` into M2.
  - M2 already spawns on Win32, so it also needs the minimal native path layer (both input forms, `;` PATH, `.exe` probing, MSVCRT quoting).
- **M3 is overloaded.** Split it:
  - M3a: scheduler, Pipes, POSIX I/O rules (M2), pipeline ordering (M1), fork-on-exec (M9).
  - M3b: Win32 helper and file-object state machine (M3), Ctrl-C.
- **M5.** Split Windows paths and spawning from real-process jobs and the image.
- **M6 is badly underestimated.** Readline's redisplay alone (prompts with `\[ \]`, wide characters, wrapping on xenl and legacy consoles) is a milestone by itself. Job control must be designed with M4 and its own milestone.
- **New items:**
  - add the Windows sandbox runner (M16) at M0;
  - gate M5 on the busybox configure benchmark (M13);
  - add an editor fuzzer: random key bytes into the vterm, checking for crashes.

### Critical files for implementation
- C:/Users/wellons/src/unabashed/mem.cpp
- C:/Users/wellons/src/unabashed/vars.cpp
- C:/Users/wellons/src/unabashed/proc.cpp
- C:/Users/wellons/src/unabashed/io.cpp
- C:/Users/wellons/src/unabashed/main_win32.cpp

Semantic references verified for this review:
- C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3/parse.y (lines 4529-4534)
- C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3/arrayfunc.c (lines 117, 725)
- C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3/redir.c (lines 450-482)
- C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3/lib/sh/random.c
- C:/Users/wellons/src/busybox-w32 (`APPLET_NOFORK` list)
