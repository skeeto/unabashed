# unabashed: architecture design (priority: faithfulness)

## 0. Summary of decisions

- **Execution:** a hybrid of three ways to run a "subshell", all sharing one state model:
  1. **Synchronous in-process subshells** for `( )`, `$( )`, backquotes and the last pipeline stage. These run on the caller's stack with a snapshot of the state.
  2. **Cooperative fibers** with a core scheduler for concurrent shell-code pipeline stages and process substitutions.
  3. **Real child processes** for `&` jobs and coprocs. On POSIX this is `fork()`. On Windows and in the virtual OS the shell re-spawns itself with a serialized state image.

  External programs are always spawned directly from the shell, never through an intermediate shell process.
- **State isolation:** every piece of subshell-visible state lives in reference-counted persistent structures, so a snapshot is O(1). A node whose count is 1 is mutated in place, so the parent pays nothing until a snapshot exists.
- **Memory:** arenas for everything transient: one per-process scratch arena reset per command, and per-parse Code arenas. One arena-backed size-class Heap holds the long-lived mutable store, so loops that run for hours stay bounded. This is the one deliberate departure from "arenas only" (open question 1).
- **Paths:** all path semantics (msys/native modes, mount table, argument and environment conversion) live in the core. The platform only sees canonical native absolute paths. The virtual OS can pretend to be Windows, so all of this is tested on Linux.
- **Reference platform** for bit-exact quirks is bash 5.3 on x86-64 Linux with glibc. Two examples: signed-char FNV-1 hashing for assoc order, and 80-bit `long double` in printf.

---

## 1. Execution model without fork

### 1.1 Three kinds of virtual process

| Construct | Vehicle | Why |
|---|---|---|
| `( list )` in the foreground, `$(...)`, `` `...` ``, the last stage of a pipeline (or the current shell under lastpipe) | **Synchronous Proc**: same fiber and C stack, new `Shell` snapshot, own escape boundary | The parent is blocked anyway. Costs a snapshot and nothing more. |
| The other shell-code pipeline stages, `<(...)` and `>(...)` | **Fiber Proc**: own stack and scratch, scheduled cooperatively | True concurrency is needed (`f \| while read`, a function standing in for `yes \| head`), but the pipeline is bounded by the shell waiting on it. |
| `list &`, `coproc`, a backgrounded pipeline's shell stages | **Real process**: `fork()` on POSIX; self-spawn plus state image on Win32 and in the virtual OS | Faithful lifetime (survives the shell's exit), real `$!`, real signals, multicore. |
| Simple external command, including `ext &` and `(ext)` | Direct spawn, no intermediate shell | Matches bash's exec optimizations: `$!` is the real pid. |

A pipeline runs its external stages first, then runs one shell stage (the last) on the current fiber and starts fibers for the remaining shell stages. Concretely:

- `ext | while read` uses no fiber at all.
- `f | ext` runs f on the current fiber, writing an OS pipe.
- A pipeline of externals only creates OS pipes and waits.
- Word expansion for an external stage still happens inside a snapshot Shell, so `ls ${x:=y} | cat` does not leak `x` into the parent.

### 1.2 Scheduler, fibers and blocking

Fibers are a platform primitive (`os_fiber`/`os_switch`):

- **Win32:** `CreateFiberEx`/`ConvertThreadToFiber`, which exist on XP.
- **POSIX:** about 30 lines of assembly per architecture.
- **Test layer:** the host implementation.

The core owns a single-threaded scheduler:

```cpp
struct Proc {
    Proc   *next;       // run queue or wait list
    Fiber  *fiber;      // 0: runs synchronously on its parent's stack
    Proc   *parent;
    Shell   sh;         // snapshot-isolated semantic state (section 2)
    Arena   scratch;    // pooled reservation, reset per command
    Escape *esc;        // innermost catch point
    Code   *code;       // retained while executing
    Node   *cmd;        // current simple command, BASH_COMMAND printed lazily
    i32     vpid;       // BASHPID
    i32     state;      // P_READY, P_BLOCKED, P_DONE
    i32     status;
    u64     sigpend;    // virtual signals, Linux numbering
    i32     waitkind;   // W_PIPEREAD, W_PIPEWRITE, W_PROC, W_IO, W_TIMER
    void   *waiton;
};

struct Sched {
    Proc *current;
    Proc *ready;
    Proc *readytail;
    i32   nlive;        // live fibers, including blocked ones
    i64   ticks;        // safe points executed
    u32   chaos;        // test mode: seeded random yields at safe points
};
```

Blocking rules:

- A Proc blocks on an in-memory pipe, a child Proc, an `Io` completion, a timer, or a signal.
- When every fiber is blocked, the scheduler calls `os_wait(timeout)`, which returns completed `Io`s, process exits, or pending signals.
- **Preemption:** every safe point (simple command, loop back-edge, function entry, builtin I/O) increments `ticks`. Every 1024 ticks the Proc yields if others are ready, and polls `os_wait(0)`. This keeps an interactive prompt responsive beside a CPU-bound fiber.
- Heavy builtins (`${s//...}`, globbing, printf loops, mapfile) also call `safepoint` inside their loops.

**Determinism.** Scheduling depends only on the order of OS events. In the virtual OS that order is deterministic. A `chaos` seed injects yields at random safe points to explore interleavings reproducibly. For scripts without races, output must not depend on the seed, which makes it a fuzz oracle for isolation bugs. If nothing in the virtual world is runnable and no timer is pending, the virtual OS reports deadlock. Real bash would simply hang there.

### 1.3 Deadlock-free I/O with OS handles

Without care, a fiber that blocks inside a raw `ReadFile` would stall all the others. The rules are:

- **POSIX:** use `poll()` readiness, which does not consume data, and read only once data is ready. There are no helper threads. Pipes the shell creates are `socketpair`s, so `MSG_PEEK` gives exact line reads. Foreign pipes are read one byte at a time, as bash does.
- **Win32 reads from a pipe:**
  - `PeekNamedPipe` reports how many bytes are available. If any, read exactly what is needed directly; this cannot block.
  - If the pipe is empty, hand a **1-byte** `ReadFile` to a pooled helper thread. The thread posts its completion to an IOCP (`GetQueuedCompletionStatus`, available on XP). `os_wait` also wakes on the Ctrl-C event.
  - That one consumed byte always belongs to the reader that asked, because `read` needs at least one byte. The only exception is an interrupted or timed-out `read -t`. Then the byte lands in the File's read-ahead buffer.
  - If that File is later handed to an external program: on Vista and later, `CancelSynchronousIo` on the helper first. On XP, interpose a pump. This is a documented, narrow divergence.
- **Win32 writes to a pipe:** if `NtQueryInformationFile(FilePipeLocalInformation).WriteQuotaAvailable` (ntdll, XP) is at least the length, write directly. Otherwise use a helper thread.
- **Disk files** never block.
- **Console:** wait on the handle together with the signal event.
- Even when only one fiber exists, pipe reads on Windows go through the async path, because Ctrl-C must interrupt `read`. The fast path (data already available) costs one Peek plus one ReadFile, against bash's one `read(2)` per byte.

### 1.4 The File and fd-table model

```cpp
enum : i32 { F_OS, F_PIPEEND, F_MEM, F_CAPTURE, F_DEVNULL };

struct Pipe {               // in-memory pipe between shell fibers
    u8    *buf;             // ring of PIPECAP = 65536 bytes (Linux default)
    iz     head;
    iz     len;
    i32    readers;
    i32    writers;
    Proc  *rwait;
    Proc  *wwait;
    File  *os;              // set once materialized into an OS pipe
};

struct File {               // one "open file description", shared by dup
    i32     refs;           // fd-table entries across all Procs
    i32     kind;
    i32     flags;          // FL_READ, FL_WRITE, FL_APPEND
    i32     ftype;          // cached os_ftype: file, pipe, console, pty...
    Handle  h;              // F_OS, or after materialization
    Pipe   *pipe;
    Str     mem;            // F_MEM (here-doc) contents
    iz      pos;
    Buf    *cap;            // F_CAPTURE destination (comsub)
    u8     *ahead;          // read-ahead: consumed from the OS, not yet delivered
    iz      naheadlen;
    Io     *inflight;       // outstanding helper-thread operation
};

struct Fds {
    File  *low[16];         // fds 0..15
    FdMap *high;            // sparse map for >= 16 (procsub 63, 62, ...)
    u16    internal;        // shell-private fds, never inherited
};
```

- **Redirections** save and restore entries in the virtual table: pointer swaps on an undo list, with no `dup2` syscalls. The `{var}>f` form allocates the lowest free fd from 10 up. Process substitution allocates 63 and counts down, matching what bash prints.
- **Here-docs and here-strings** read by builtins are `F_MEM`. Comsub stdout is `F_CAPTURE`: shell-code writes append straight to the caller's buffer, with zero syscalls.
- **Materialization rule:** anything that crosses a process boundary (external child, forked or self-spawned child) becomes a real OS object first.
  - In-memory pipe: create an OS pipe. Already-buffered bytes stay as the reader's read-ahead prefix, and later in-process writes go to the OS write end.
  - Here-doc: a pipe if it is at most 4096 bytes, otherwise a delete-on-close temp file. This is bash's own rule, so seekability seen by externals matches.
  - Capture: an OS pipe drained into the buffer. Before any builtin appends, the core drains what is already available, which keeps `$(echo a; ls; echo b)` in order.
  - EOF waits until both the in-process refcount and the OS pipe are closed, so `x=$(sleep 9 &)` waits 9 s, as in bash.
- **SIGPIPE:** a write to a pipe with no readers (memory or OS) raises a virtual SIGPIPE in the writing Proc (exit 141, or the trap).
- **Stdio buffering:** builtin stdout is buffered in a 4096-byte Buf flushed when the builtin ends (line-buffered on a tty), and stderr is unbuffered. This reproduces glibc stdio interleaving, for example `printf` errors appearing before its earlier stdout output.

### 1.5 Externals: spawn and wait

The core resolves the program (hash, PATH, exe kind, shebang) and builds a platform-neutral `Spawn` (section 4).

**Win32:**
- Duplicate fds 0-2 to inheritable handles, plus an MSVCRT `lpReserved2` blob only when user fds of 3 or more exist (slot width from the child's PE machine type).
- `CREATE_UNICODE_ENVIRONMENT|CREATE_SUSPENDED`, adding `CREATE_NEW_PROCESS_GROUP` when the child must ignore SIGINT (background jobs of a non-interactive shell, or `trap '' INT`).
- `lpCurrentDirectory` is set from the Proc's virtual cwd. Resume after any job-object assignment (interactive job control only).
- Wait through `RegisterWaitForSingleObject`, which posts to the IOCP. All of this is XP-compatible.
- Exit codes map 0xC000013A to SIGINT, 0xC0000005 to SIGSEGV, and kills the shell performed to the recorded signal.
- `.bat`/`.cmd` files run through `%ComSpec% /d /s /c` with BatBadBut-safe escaping.

**POSIX:** `fork`/`vfork` plus exec in the platform layer applies the fd map, chdir, umask, signal defaults and ignores, setpgid and tcsetpgrp. A SIGCHLD self-pipe plus `waitpid(WNOHANG)` feeds `os_wait`.

**Virtual OS:** the VFS file content names a fake program (`cat`, `recho`...) or `self`. Spawning creates a virtual process on a virtual-OS fiber with its own virtual fd table. `self` starts a fresh core instance (`sh_main` on a sub-arena), which provides the suite's `$THIS_SH`.

### 1.6 Real child processes for `&` and coproc

| Platform | Mechanism |
|---|---|
| POSIX | `fork()`. Materialize in-memory Files first. The child discards the other Procs and continues at the async node. |
| Win32 | Spawn our own exe with `--unabashed-image=<inherited handle>`. The parent streams a serialized image over a pipe, using an async write when it is large. |
| Virtual OS | Self-spawn, which tests the image path. POSIX CI also forces the image path with a flag. |

The image is a position-independent serialization of the entire `Shell`, plus the AST subtree to run, the fd map and the parent's `$$`. Rules for `Shell` fields:
- Every field has a serializer.
- A round-trip fuzz test compares `declare -p; declare -f; alias; trap -p; set -o; shopt; dirs; jobs` before and after.
- The AST is serialized in binary, keeping line numbers and raw text, so LINENO and BASH_SOURCE are exact.

Signals to our own children on Windows go through a named event per pid (`Local\unabashed-<pid>-sig`) read by the child's signal thread, so `trap ... TERM` in a background subshell works. Signals to foreign processes use TerminateProcess with the recorded signal, or CTRL_BREAK for process groups.

### 1.7 Signals, traps, `exit`, `$$`/`$BASHPID`

- **Real signals:**
  - POSIX handlers set bits and write to the self-pipe.
  - Win32 maps Ctrl-C to SIGINT, Ctrl-Break to SIGQUIT, CTRL_CLOSE to SIGHUP, buffer resize to SIGWINCH, and named events to other signals.
  - Terminal SIGINT fans out to every Proc in the foreground "process group". Other real signals go to the main Proc.
- **Delivery** happens at safe points: run the trap, or unwind the Proc with status 128+N.
- **Cooperative-exit rule:** a SIGINT that arrives while waiting on a foreground external is held. It is delivered only if the child itself died of SIGINT, as with bash's `wait_sigint_handler`.
- **Subshell traps:** the snapshot transforms caught traps back to default and keeps ignored ones, but remembers the parent's text for bash's "`trap` in a subshell prints parent traps" behaviour.
- **`exit`, `return`, `break`, errors, fatal signals and OOM** all unwind with `__builtin_longjmp` (platform `setjmp` on arm64 macOS) to the nearest matching `Escape`:

```cpp
struct Escape {
    Escape *prev;
    void   *jmp[5];
    i32     catches;    // ESC_EXIT|ESC_RETURN|ESC_LOOP|ESC_ERROR|ESC_SIGNAL|ESC_OOM
    i32     frames;     // explicit-stack depths to truncate to, so refcounts
    i32     undos;      //   held by frames and redirection undos get released
    i32     value;
};
```

  Nothing that needs cleanup lives only on the C stack. Call frames, redirection undo, scope pushes and retained Code are on explicit Proc stacks that the catcher truncates, the equivalent of bash's unwind-protect.
- **`$$`** stays the shell's pid in every Proc. **`BASHPID`** is a virtual pid for in-process Procs: odd numbers on Windows (real pids are multiples of 4) and values above `pid_max` on Linux, so an external `kill` can never hit an innocent process. Self-spawned and forked children have real pids.
- **`exec ext` in a subshell Proc** turns the Proc into a forwarder: signals go to the child and its status becomes the Proc's.
- **`exec` in the main shell:**
  - POSIX: real `execve` when no live fibers exist.
  - Otherwise, and always on Windows: spawn, wait, exit with the child's status, as busybox does.

### 1.8 Every place an in-process design could diverge, and the defence

| # | Divergence | Prevention or containment |
|---|---|---|
| 1 | Subshell sees later parent mutations | Persistent snapshots; the parent path-copies after any snapshot. |
| 2 | Background subshell dies when the shell exits | `&` and coproc are real processes. |
| 3 | `$!`, `kill`, `wait` semantics | Real pids for jobs; a virtual-pid table for in-process Procs in the kill and wait builtins. |
| 4 | Ctrl-C and the cooperative exit | Foreground-group fan-out plus the held-SIGINT rule. |
| 5 | SIGPIPE in shell stages | Virtual SIGPIPE on writing to a pipe with no readers (exit 141). |
| 6 | Pipe capacity and resulting deadlocks | 64 KiB memory pipes; `CreatePipe` with a 64 KiB size hint. |
| 7 | Script input shared with children (`bash < script`, `{ read; cat; } < f`) | Exact consumption: seek back on seekable input, peek on pipes. |
| 8 | Here-doc pipe vs temp file seen by externals | bash's 4096 rule. |
| 9 | cwd/umask/env inherited by externals | Per-Proc virtual cwd, umask and env passed explicitly at spawn. Our own process cwd never changes, which also allows `cd d; rm -rf ../d`. |
| 10 | Trap reset in subshells, special `trap` output | Snapshot transform. |
| 11 | Comsub waits for background holders | Refcount plus OS EOF. |
| 12 | `/dev/fd/N` | Opened internally from the virtual table. Native Windows children get `\\.\pipe\...` paths substituted at spawn, so `echo <(:)` still prints `/dev/fd/63`. |
| 13 | Residual: pipeline fibers die if the shell exits mid-pipeline (lastpipe + `exit`, `exec 3< <(long)` then exit) | Rare. Documented. |
| 14 | Residual: a long builtin is killable only at safe points | Safe points inside heavy loops. |
| 15 | Residual: shell stages of one pipeline share one core | Background jobs are parallel; a future multi-threaded scheduler (see 2.5). |
| 16 | Residual: `ulimit` per in-process Proc is virtual | Documented. |
| 17 | Residual: deep recursion | A stack-left check raises an emulated SIGSEGV (Proc exits 139), at a different depth than bash. |
| 18 | Residual: OOM message lacks bash's file:line | Documented. |
| 19 | Residual: on XP, an interrupted pipe read may need a pump | Documented. |
| 20 | Residual: `ps`/procfs views of BASHPID | Virtual pids are not real processes. |

---

## 2. Memory model

### 2.1 Arenas

The foundation is base.cpp, with one change: `alloc`'s slow path calls `arenagrow` instead of `os_oom`, keeping the fast path identical.

```cpp
struct Grow {               // lazily committed contiguous reservation
    Os     *os;
    byte   *commit;
    byte   *limit;
    Escape *oom;            // owning Proc's ESC_OOM catch point
};

struct Arena {
    byte *beg = {};
    byte *end = {};         // committed end; may be stale in by-value copies
    Grow *g   = {};
};

static void arenagrow(Arena *a, iz need)
{
    Grow *g = a->g;
    if (!g || need > g->limit-a->beg) oomescape(a);
    if (a->beg+need > g->commit) {
        iz len = (a->beg + need - g->commit + (1<<20) - 1) & -(iz)(1<<20);
        len = len<g->limit-g->commit ? len : g->limit-g->commit;
        if (!os_commit(g->os, g->commit, len)) oomescape(a);
        g->commit += len;
    }
    a->end = g->commit;
}
```

A by-value scratch copy shares the `Grow`, so growth stays valid and "discard on return" still works.

| Arena | Lifetime |
|---|---|
| **Proc scratch** (pooled reservations) | `exec(Proc *, Node *, Arena scratch)` takes it by value, so every simple command, loop iteration and function body sweeps its expansions, argv, env block and command line on return. The top level is `for (;;) { Arena cmd = p->scratch; Code *c = parseone(...); exec(p, c->root, cmd); release(c); }`. |
| **Code arenas** (chained 64 KiB heap blocks, refcounted) | One per parsed top-level command, `eval`/`source` chunk, trap-string cache entry, or function definition. A function body is copied into its own Code at definition time, which bash also does. A top-level Code is released when its command finishes, unless a fiber still executes in it. Redefining a function while it runs is safe because running frames retain the old Code. |
| **Comsub capture** | Grows at the tip of the *caller's* scratch (the caller is blocked). The child gets its own pooled scratch, so the capture buffer extends in place. |

### 2.2 The Heap: long-lived mutable state

```cpp
struct Heap {               // segregated free lists over one lazily committed arena
    Arena  mem;
    void  *free[44];        // class k holds blocks of 16<<k bytes
    iz     live;            // outstanding bytes, for the leak checker and stats
};
static void *heapalloc(Heap *, iz size);           // zeroed; escapes on OOM
static void  heapfree(Heap *, void *, iz size);    // pushes onto a class list
```

The Heap holds variables, functions, aliases, the command hash, positional arrays, call frames, Files, history, the job table and Code blocks.

**Why not pure arenas.** `while :; do local x; s=$(date); unset "v$i"; done` creates and destroys cells forever. Free lists bound that. The alternative is copying compaction, which needs a global safe point that every fiber's C stack must respect, plus pauses proportional to the size of the state. That pause would be noticeable with bash-completion loaded, so I rejected it.

**Debug builds** poison freed blocks. Every test asserts that `heap.live` returns to its baseline after the shell instance is torn down, which catches refcount leaks.

### 2.3 O(1) snapshots: refcounted persistent structures

```cpp
struct VarNode {            // persistent 4-ary hash trie (house style), path-copied
    i32      refs;
    VarNode *child[4];
    Str      name;
    u64      hash;
    Var     *top;           // shallow-binding stack, innermost first
};

struct Var {                // one binding
    i32    refs;
    i32    depth;           // 0 global, n = function scope, DEPTH_TEMPENV
    u32    attrs;           // V_EXPORT V_READONLY V_INTEGER V_LOWER V_UPPER V_NAMEREF
                            // V_TRACE V_LOCAL V_INVISIBLE V_IMPORTED V_CAPCASE ...
    i32    kind;            // K_UNSET K_STR K_NUM K_INDEXED K_ASSOC K_DYNAMIC
    Var   *down;            // shadowed binding
    u8    *data;            // K_STR value, owned, capacity-doubling
    iz     len;
    iz     cap;
    i64    num;             // K_NUM: arithmetic results and -i, string formed lazily
    Array *arr;             // refcounted persistent radix trie
    Assoc *map;             // refcounted bash-order table (3.6)
    i32    dyn;             // DYN_RANDOM DYN_SECONDS DYN_LINENO DYN_FUNCNAME ...
};

struct Shell {              // everything a subshell may change; snapshot = copy + retain
    VarNode   *vars;
    FuncNode  *funcs;
    AliasNode *aliases;
    HashNode  *hashed;
    Frame     *frames;      // persistent call stack: FUNCNAME, BASH_SOURCE,
                            //   BASH_LINENO, BASH_ARGV, BASH_ARGC
    Args      *args;        // positional parameters, immutable, refcounted
    Cwd       *cwd;         // logical, physical, Windows per-drive cwds
    Dirs      *dirs;
    Fds        fds;
    Trap       traps[NTRAP];
    u64        opts[2];     // set -o and shopt bits; $- derived
    i32        umask;
    i32        bashpid;
    i32        subshell;
    i32        status;
    i32        lastsub;
    i32        pathmode;    // PM_MSYS or PM_NATIVE
    b32        utf8;
    u32        rand;
    i32        optind;
    i32        optchar;     // getopts hidden state
    u32        envgen;      // export generation, for the env cache
    Str        underscore;  // $_
};

static Shell forkshell(Heap *, Shell *);   // memberwise copy plus retain: O(#open fds)
static void  dropshell(Heap *, Shell *);   // release; frees nodes whose count reaches 0
```

**Writes:** walk from the root. A node with `refs==1` is mutated in place; otherwise it is copied, its children are retained, and the parent's count drops. A tight loop with no live snapshot therefore allocates nothing:
- `i=$((i+1))` writes the `num` field in place.
- `s+=x` amortizes into `cap`.
- `line=$(...)` reuses the buffer when the new value fits.

**Dynamic scope** uses shallow binding:
- `local` pushes a Var with the current depth, and the frame records the names it localized.
- Return pops them.
- `declare -g` edits the bottom of the stack.
- bash's unset corner cases (pop, `V_INVISIBLE` "unset but local", unsetting a caller's local, `localvar_unset`/`localvar_inherit`) are all expressible as stack operations.

Lookup is one trie descent with a hash pre-computed at parse time for literal names, so its cost does not depend on call depth.

**Tempenv:** `VAR=x cmd` pushes `DEPTH_TEMPENV` bindings. For functions they become call-local; for special builtins in POSIX mode they persist.

**Safety invariant:** the core never holds a `Var *` or a value pointer across anything that can run code or block. It re-looks-up, and expansion copies values into scratch.

### 2.4 Long-running loops and background jobs

- **One top-level `while` for hours:** its Code is fixed. Each iteration's scratch resets because `exec(body, scratch)` takes the arena by value. Heap cells are reused in place or recycled. Memory stays flat.
- **Background jobs** take nothing from the parent's arenas. They are processes holding a serialized copy (Win32) or a COW fork (POSIX).
- **Pipeline fibers and procsub fibers** retain the Code and the Shell snapshot they run. Both are released when the Proc ends.

### 2.5 OOM and 32-bit budgets

**OOM:**
- Scratch or Heap exhaustion inside a sub-Proc escapes to that Proc's `ESC_OOM`. It prints `bash: xmalloc: cannot allocate N bytes` and the Proc exits 2. Its released references free Heap memory, so the parent survives, matching a forked bash child dying.
- In the main Proc the same message is printed and the shell exits 2.
- Failure to reserve space for a new fiber or Proc prints `bash: fork: retry: Resource temporarily unavailable` and then `bash: fork: Resource temporarily unavailable`, which is bash's own wording.

**Budgets:**

| | 64-bit | 32-bit (XP, linked `--large-address-aware`) |
|---|---|---|
| Heap reservation | 64 GiB | 768 MiB |
| Main scratch | 4 GiB | 256 MiB |
| Pooled Proc scratch | 1 GiB each | 64 MiB, shrinking to 8 MiB as the count grows |
| Fiber stacks | 8 MiB reserve | 1 MiB reserve |
| Main thread stack | 64 MiB | 16 MiB |

Committed pages above a high-water mark are decommitted at the idle prompt.

**Future multicore:** the share-nothing discipline makes an M:N scheduler possible later. Procs share only immutable snapshots, so it would need atomic refcounts and per-thread Heaps.

---

## 3. Core representations

### 3.1 Lexer, parser, aliases

The parser is hand-written recursive descent and incremental: it reads one complete command, executes it, then reads the next. That is required for aliases, `shopt -s extglob` taking effect on later lines, and syntax errors appearing after earlier output.

The lexer reads from an **input stack**. Alias expansion pushes the alias text, which handles the trailing-blank rule and the recursion guard, as bash does. Raw token text therefore reflects post-alias text, which is also what `declare -f` prints. The parser consults the current option state while parsing (extglob, posix, `expand_aliases`).

### 3.2 AST shaped like bash's printing structure

```cpp
enum : i32 {
    N_SIMPLE, N_PIPE, N_AND, N_OR, N_SEQ, N_ASYNC, N_NOT, N_TIME, N_SUBSHELL,
    N_GROUP, N_IF, N_WHILE, N_UNTIL, N_FOR, N_ARITHFOR, N_SELECT, N_CASE,
    N_ARITH, N_COND, N_FUNCDEF, N_COPROC,
};

struct Node {
    i32    kind;
    i32    line;            // bash's notion, for LINENO
    i32    flags;           // pipe |&, time -p, case ;; ;& ;;&, ...
    Redir *redirs;
    Node  *a, *b, *c;       // children (test/then/else, left/right, body)
    Word  *words;           // simple: argv; for: list; case: subject
    iz     nwords;
    Word  *assigns;
    iz     nassign;
    Case  *cases;
    Cond  *cond;
    Arith *arith;
    Str    raw;             // arith/cond/funcname source text for printing
};

struct Word {
    Str   raw;              // token text with quotes intact: declare -f prints this
    Part *parts;            // pre-parsed expansion tree
    Str   lit;              // W_CONST: final value, no expansion needed
    i32   flags;            // W_CONST W_ASSIGN W_BRACE W_TILDE W_GLOBCHARS W_SPLITTABLE
};

struct Part {
    Part *next;
    i32   kind;             // P_LIT P_SQ P_DQ P_ESC P_ANSIC P_PARAM P_COMSUB P_BACKQ
                            // P_FUNSUB P_VALSUB P_ARITH P_PROCIN P_PROCOUT P_TILDE P_BRACE
    Str   text;
    Part *sub;              // nested (P_DQ, brace alternatives)
    Param *param;
    Code  *body;            // comsub/procsub/funsub, parsed at parse time (bash 5.2+)
    u32   aliasgen;         // alias generation when body was parsed
};
```

**Lazy-construct principle.** Whatever bash parses at *expansion* time, we pre-parse speculatively. On failure we keep the raw text and parse at run time, so the error, its timing and its message match bash. This covers:
- here-doc bodies;
- arithmetic bodies containing `$` (expanded as *text* and then parsed, because `x=1+1; $(( $x*2 ))` gives 3 by text substitution, while the bare name in `$((x*2))` gives 4);
- `${...}` operands.

Comsub bodies are re-parsed from `raw` only when aliases are enabled and the alias generation has changed since parse time (or extglob changed). That reproduces bash's execution-time alias behaviour without the common-case cost.

**Brace expansion** is resolved at parse time into a `P_BRACE` structure. Sequences such as `{1..1000000}` are generated lazily.

### 3.3 Printing: `declare -f`, `type`, `$BASH_COMMAND`, `set -x`, jobs

**print.cpp** reimplements bash's canonical formatter from observed behaviour, not from print_cmd.c. It walks the AST:
- words are printed from `raw`;
- conditional and arithmetic commands are rebuilt with bash's spacing;
- here-docs are printed after the command line.

The rules are learned and locked down by differential tests that generate thousands of functions.

**Laziness:**
- `$BASH_COMMAND` and job text are printed only on demand, from `Proc.cmd` plus the retained Code. bash pretty-prints every simple command eagerly; this is a measurable win.
- `set -x` quotes expanded words with bash's xtrace rules (`'...'`, `$'...'` for nonprintables, PS4 repeated per level). It shares the quoting engine with `%q`, `${x@Q}` and `declare -p`.

### 3.4 Arithmetic

`Arith` is a compiled tree with pre-hashed variable references and raw-text token offsets. bash's messages need those offsets (`syntax error: operand expected (error token is "+ ")`).

- Compilation is lazy. Errors are stored and raised only at evaluation time, as bash does.
- Evaluation uses u64 wraparound, `INTMAX_MIN/-1` handling, a `noeval` flag for `&&`, `||` and `?:`, and a recursion limit of 1024.
- A variable's value that is not a plain integer is compiled and cached per Var by value pointer.
- Bodies that require text substitution go through a per-node "last text, compiled" cache. The decimal fast path skips the parser entirely, which suits subscripts like `a[$i]`.

### 3.5 Patterns and the ERE engine

**Patterns** (glob, case, `[[ == ]]`, `${x#p}`, `${x//p/r}`):

```cpp
struct Pat {
    i32  kind;      // PAT_LITERAL, PAT_PREFIXSUFFIX, PAT_STARS (linear two-pointer), PAT_NFA (extglob)
    Str  prefix;
    Str  suffix;
    Seg *segs;      // literal segments between stars
    Ins *prog;      // for brackets/classes/extglob: backtracking VM with (pos, pc) memo
    i32  flags;     // nocase, dotglob rules, globasciiranges, utf8
};
```

- Matching works on UTF-8 bytes, decoding only for `?`, brackets and case folding.
- `${x//p/r}` uses a matcher that reports every match end in one pass, giving leftmost-longest without bash's O(n²) end probing.
- Constant patterns are compiled once and cached on the node.

**ERE for `=~`:** POSIX leftmost-longest with POSIX submatch rules (an Okui-Suzuki-style tagged NFA). It falls back to memoized backtracking when back-references are present; glibc accepts `\1` in EREs. glibc's quirks become the oracle through differential fuzzing.

### 3.6 Arrays and bash-order associative arrays

- **Indexed arrays** are a persistent 16-ary radix trie keyed by index. It gives ordered iteration, sparse and dense storage in one structure, and caches `max` and `count`. Mutation is in place when the node is unique.
- **Associative arrays** reproduce bash's iteration order exactly, which I checked in hashlib.c:
  - Hash: FNV-1 32-bit, multiply then XOR, over **signed** `char` (x86 sign extension for high bytes).
  - 1024 initial buckets; insertion at the chain head.
  - Grow ×4 *before* inserting when `nentries >= 2*nbuckets`.
  - The rehash walks old buckets in order and head-inserts, which reverses chains.
  - No shrink on removal.
  - Iteration runs buckets 0..n-1, each chain front to back.

  Representation: a persistent radix trie over the bucket index whose leaves are immutable refcounted chains. A rehash rebuilds in bash's exact order.

### 3.7 Environment, numbers, locale

- **Environment block:** the export generation `Shell.envgen` invalidates a per-Proc cached list. The platform converts it to native form at spawn: sorted UTF-16 on Windows, keeping `=C:` entries.
- **printf:** integers have their own formatter. Floats emulate x86 `long double`: 64-bit mantissa, correctly rounded parsing, and exact decimal output through an arena bignum with glibc's round-half-even on the exact value. This covers `%f/%e/%g/%a`.
- **Time:** `%(fmt)T` and prompt time use a core strftime (C locale), POSIX TZ string parsing, and TZif files read through `os_read` on POSIX. When TZ is unset, `os_localtz` supplies the zone.
- **Locale:** a UTF-8 mode versus a C byte mode, switched at run time from `LC_ALL`/`LC_CTYPE`/`LANG`. UTF-8 means C.UTF-8 semantics: codepoint collation, plus generated Unicode tables for classes, case mapping and wcwidth.
- **Messages:** glibc `strerror` text and Linux signal numbering and names live in err.cpp.

---

## 4. Platform API

```cpp
struct Os;
using Handle = i64;                 // fd or HANDLE as an integer; -1 invalid
using Pid    = i64;

enum Err : i32 { E_OK, E_NOENT, E_ACCES, E_EXIST, E_NOTDIR, E_ISDIR, E_INVAL,
                 E_NOSPC, E_PIPE, E_AGAIN, E_INTR, E_NAMETOOLONG, E_LOOP,
                 E_NOEXEC, E_2BIG, E_BADF, E_NOTEMPTY, E_PERM, E_TXTBSY, E_IO,
                 E_ROFS, E_NOTTY, E_CHILD, E_SRCH, E_NOMEM, E_SPIPE, E_OTHER };

struct OsInfo {                     // filled by the platform before sh_main
    b32     windows;                // drive letters, '\\', case-folding FS
    Str     exe;                    // our own absolute native path
    Str     cwd;
    Str    *args;
    iz      nargs;
    Str    *env;                    // "NAME=value", WTF-8
    iz      nenv;
    Pid     pid;
    Pid     ppid;
    i64     uid, euid, gid, egid;
    Str     hostname, hosttype, ostype, machtype, user, home;
    Handle  std[3];
    Str     image;                  // state image from a self-spawn parent, or null
    b32     canfork;                // POSIX only
};
static i32 sh_main(Os *, OsInfo *, Arena mem);          // core entry point

// Memory
static byte   *os_reserve(Os *, iz len);                 // address space; 0 on failure
static b32     os_commit(Os *, byte *, iz len);
static void    os_decommit(Os *, byte *, iz len);
// Also: [[noreturn]] os_oom(), context-free, the last resort only

// Fibers
struct Fiber;
static Fiber  *os_fiber(Os *, iz stack, void (*)(Os *, void *), void *);
static void    os_switch(Os *, Fiber *);                 // suspend caller, resume target
static iz      os_stackleft(Os *);                       // bytes left on the current fiber

// Files and pipes (paths: canonical native absolute UTF-8, see 4.1)
struct Opened { Handle h; Err err; };
static Opened  os_open(Os *, Arena, Str path, i32 flags, i32 mode);
static Opened  os_tempfile(Os *, Arena);                 // anonymous, delete on close
static iz      os_read(Os *, Handle, u8 *, iz);          // >=0 bytes, <0 is -Err
static iz      os_write(Os *, Handle, u8 *, iz);         // console: UTF-8 to UTF-16 with carry
static iz      os_peek(Os *, Handle, u8 *, iz);          // pipe/socket non-consuming; -E_INVAL
static iz      os_avail(Os *, Handle, b32 forwrite);     // readable bytes / write quota; -1 unknown
static i64     os_seek(Os *, Handle, i64, i32 whence);
static Err     os_truncate(Os *, Handle, i64);
static void    os_close(Os *, Handle);
static i32     os_ftype(Os *, Handle);                   // FT_FILE FT_DIR FT_PIPE FT_CONSOLE
                                                         //   FT_PTYPIPE FT_NULL FT_CHAR
struct Pipe2 { Handle rd; Handle wr; Str path; Err err; };
static Pipe2   os_pipe(Os *, Arena *, i32 flags);        // PIPE_NAMED: path for procsub

// Filesystem
struct Stat { Err err; i32 type, mode, attrs; i64 size, blocks, nlink, uid, gid;
              i64 atime, mtime, ctime; u64 dev, ino; };  // times in ns
struct Dirent { Dirent *next; Str name; i32 type; };     // raw order (GLOBSORT=nosort)
struct Listing { Dirent *head; Err err; };
static Stat    os_stat(Os *, Arena, Str path, b32 follow);
static Stat    os_fstat(Os *, Handle);
static Err     os_access(Os *, Arena, Str path, i32 rwx);
static Listing os_list(Os *, Arena *, Str path);
static Str     os_readlink(Os *, Arena *, Str path);
static Str     os_realpath(Os *, Arena *, Str path);     // pwd -P, cd -P
static Err     os_unlink(Os *, Arena, Str path);
static Err     os_rename(Os *, Arena, Str from, Str to);
static Str     os_userhome(Os *, Arena *, Str user);     // ~user; null if unknown

// Processes
struct SpawnFd { i32 fd; Handle h; };
struct Spawn {
    Str      path;                  // resolved native image path
    Str     *argv;                  // already msys-converted by the core
    iz       argc;
    i32      quoting;               // Q_MSVCRT, Q_CYGWIN, Q_CMDBATCH (ignored on POSIX)
    Str     *env;
    iz       nenv;
    Str      cwd;
    SpawnFd *fds;
    iz       nfds;
    u64      sigign;                // signals ignored in the child
    i32      umask;
    i32      flags;                 // SP_NEWGROUP SP_FOREGROUND SP_SELF SP_SUSPENDOK
    Pid      pgid;
    Str      image;                 // SP_SELF: streamed to the child
};
struct Spawned { Pid pid; Handle proc; Err err; };
static Spawned os_spawn(Os *, Arena, Spawn *);
static Pid     os_fork(Os *);                            // POSIX only; -1 unsupported
static Err     os_exec(Os *, Arena, Spawn *);            // POSIX execve; returns on failure
static b32     os_kill(Os *, Pid, i32 sig, b32 group);   // knows which pids are our shells
static b32     os_stopcont(Os *, Pid, b32 stop);         // SIGSTOP/CONT; NtSuspendProcess
[[noreturn]] static void os_exit(Os *, i32);

// Async completion and signals
struct Io { Io *next; i32 op; Handle h; u8 *buf; iz len; iz result;
            i32 status; Pid pid; void *owner; };         // op: IO_READ IO_WRITE IO_PROC IO_READY
static void    os_submit(Os *, Io *);
static Io     *os_wait(Os *, i64 timeout_ns);            // 0 on timeout or new signals
static b32     os_cancel(Os *, Io *);                    // 0 if it may still complete
static u64     os_sigpending(Os *);                      // fetch and clear (Linux numbers)

// Terminal
struct TermSize { i32 cols; i32 rows; };
static i32      os_isatty(Os *, Handle);                 // 0, TTY_VT, TTY_CONSOLE, TTY_PTYPIPE
static b32      os_termmode(Os *, Handle in, Handle out, i32 mode);  // TM_RAW / TM_RESTORE
static TermSize os_termsize(Os *, Handle);
// Legacy console only (XP, Win7, conhost without VT); keys arrive from os_read as xterm bytes
struct ConState { i32 x, y, cols, rows, top, attr; };
static ConState os_conget(Os *, Handle);
static void     os_conmove(Os *, Handle, i32 x, i32 y);
static void     os_conerase(Os *, Handle, i32 x, i32 y, iz n);
static void     os_conscroll(Os *, Handle, i32 lines);

// Time and entropy
struct Times  { i64 user, sys, cuser, csys; };           // ns
struct TzInfo { i32 offset; b32 dst; Str abbr; };
static i64     os_now(Os *);                             // realtime ns
static i64     os_clock(Os *);                           // monotonic ns
static Times   os_times(Os *);                           // Win32: job accounting for children
static TzInfo  os_localtz(Os *, Arena *, i64 t);         // only when TZ is unset
static void    os_entropy(Os *, u8 *, iz);               // SRANDOM; RtlGenRandom on XP
```

**Notes:**
- Win32 resolves every API newer than XP through `GetProcAddress` into `struct Os`, with an XP fallback.
- wide.cpp is the portable half of the Win32 layer, included by the virtual OS test layer too. It holds UTF-16, the MSVCRT/Cygwin/cmd quoting encoders, our own command-line parser in both modes, and PE inspection for exe kind and machine type. The virtual OS encodes with it and decodes with a simulated CRT parser, so command-line round trips are tested.

### 4.1 Where path translation sits

Entirely in the core (paths.cpp). The platform contract is one canonical form:
- Windows: `C:/a/b`, `//srv/share/x`, `//./pipe/x`, `//./NUL`, which the platform converts to `\\?\` wide paths.
- POSIX: `/a/b`.

**The core handles:**
- Accepting `C:/x`, `C:\x` and `/c/x` in both modes.
- The meaning of a leading `/`: the msys mount table (install root from the exe, built-in `/tmp` and `/usr/bin`, fstab) versus the current drive's root in native mode.
- Logical `..` handling and per-drive cwds.
- Output rendering, `:` versus `;` in PATH, and environment translation at startup and at spawn.
- The MSYS2 argument-conversion heuristic for native children, including `MSYS2_ARG_CONV_EXCL` and the heuristic's misfires.
- Choosing the quoting kind from the exe kind (msys-2.0.dll import, cached by path and mtime).
- Device names: msys mode lets `\\?\` create a literal file called `nul`, as Git Bash does. Native mode maps it to `//./NUL`, as busybox does.
- Shebang mapping.

`OsInfo.windows` is a run-time flag, not an `#ifdef`. A "Windows-flavoured" virtual OS (drives, case folding, reserved names, pipes without readiness) exercises all of it on Linux.

---

## 5. Testing architecture

1. **Unit tests** (`main_test.cpp` plus `test/*_test.cpp`, ssg's count-and-continue `expect` style, table-driven):
   - lexer and parser;
   - print round trips;
   - arithmetic;
   - patterns, checked against a naive reference;
   - ERE, checked against a backtracking reference;
   - float formatting, checked against the bignum;
   - paths in both modes and both flavours;
   - wide.cpp quoting;
   - persistent-structure invariants;
   - Heap leak-to-baseline after every case, with arenas filled with 0x55 before each run.
2. **Virtual-OS script tests:**
   - The world: in-memory VFS (`/tmp`, `/dev/*`, a synthetic `/etc/passwd`), deterministic pids, a virtual clock (`sleep` advances it), and fake programs: `recho`, `zecho`, `printenv`, `xcase` and roughly 25 coreutils subsets. `self` runs nested instances.
   - Cases are our own scripts, embedded as raw-string tables. `tools/record` regenerates expected stdout, stderr and status from real bash 5.3 in a container, so the expectations are ours to commit.
   - The shell instance never touches the host.
3. **bash's own suite as an external oracle:**
   - A separate `oracle` binary takes `BASH_TESTS=<extracted path>`, which is never committed. Only the harness reads host files (read-only), loading them into the VFS. It reimplements the `run-*` contract: `2>&1`, `grep -v '^expect'`, `cat -v`, the version variables.
   - Categories a and b run in the virtual OS. Categories c, d and e run on the POSIX build inside a disposable non-root Docker container on tmpfs, with no network.
   - The committed manifest holds test names, expected status and a reason. `bash-5.3.tar.gz` is gitignored.
4. **Differential testing:**
   - bash 5.3 built from the tarball in WSL Debian, whose packaged bash is 5.2, and in Docker.
   - Inputs: a snippet corpus plus each `.tests`/`.sub` file run standalone by path.
   - Compare stdout, stderr, status and the merged stream, normalized for TMPDIR, pids and time.
   - Git Bash on the Windows host is used for msys path, argument and environment behaviour only, with safe snippets in a temp directory.
5. **Fuzzing** (AFL++, virtual OS only, persistent mode, small arenas, tick and virtual-time budgets). One target per `-DFUZZ=` value:
   - `parse`: parse, print, reparse, print must be stable;
   - `run`: full execution with fake programs; crashes, leaks, deadlock reports;
   - `chaos`: the same script under different yield seeds must give the same output;
   - `image`: Shell round-trip identity;
   - `arith`, `glob`, `regex`, `printf`: differential against the in-tree references.

   Differential fuzzing against bash uses a grammar-based generator of safe scripts run in the container against both shells.
6. **Line editor:**
   - vterm.cpp, used by tests and tools, models a VT screen with xenl, plus a legacy-console model, so both editor backends run headless. Inputs are scripted keys; checks are golden screens.
   - Differential "feel": a small `forkpty` runner feeds the same keys to bash plus readline and to ours, both renders go through vterm, and the screens are compared.
   - End to end: winctl drives conhost and Windows Terminal on Windows 11, and an i686 build on the XP laptop, through `type`/`key`/`shot`. A debug-only, hidden buffer dump avoids reading pixels.
7. **Benchmarks:**
   - The suite: perf.md B1-B13 and M1-M4.
   - Runner: hyperfine against bash 5.3, Git Bash, MSYS2 bash and busybox sh, with checksums verified before timing.
   - Counters: instructions (`perf stat`), process-creation counts, peak RSS.
   - CI gate: deterministic instruction counts on the virtual OS, failing on regressions over 5%.

---

## 6. Source layout and milestones

### Layout

All files are under `C:/Users/wellons/src/unabashed/`.

**Core:**
- `base.cpp`: foundation, with the `arenagrow` hook.
- `unabashed.cpp`: Platform API table, includes, `sh_main`.
- Modules, in include order: `heap.cpp`, `unicode.cpp` (generated tables), `err.cpp`, `num.cpp` (integers, bignum, x87 floats), `tz.cpp`, `paths.cpp`, `lex.cpp`, `parse.cpp`, `print.cpp`, `vars.cpp`, `array.cpp`, `arith.cpp`, `pattern.cpp`, `regex.cpp`, `expand.cpp`, `io.cpp`, `sched.cpp`, `proc.cpp` (snapshots, Procs, pipelines, procsub, image serialization), `exec.cpp`, `spawn.cpp` (lookup, PE, shebang, argv and env), `jobs.cpp`, `startup.cpp` (persona, options, rc files, env import).
- Builtins: `b_io.cpp`, `b_vars.cpp`, `b_flow.cpp`, `b_dir.cpp`, `b_test.cpp`, `b_job.cpp`, `b_misc.cpp`.
- Interactive: `history.cpp`, `histexp.cpp`, `edit.cpp`, `term.cpp`, `complete.cpp`, `inputrc.cpp`.

**Platform layers:**
- `main_win32.cpp` plus the pure `wide.cpp`;
- `main_posix.cpp`;
- `main_test.cpp` plus `vos.cpp` (VFS, virtual processes and scheduler), `fakes.cpp`, `vterm.cpp`, `test/*.cpp`;
- `main_fuzz.cpp`;
- `oracle.cpp` (`main_test` with the suite runner).

**Other:** `tools/` (record, difftest, difffuzz, ptyrun, bench) and `CMakeLists.txt`, which is optional. Every program builds with one compiler command.

### Milestones

Each milestone ends runnable and tested.

| | Milestone | Ends with |
|---|---|---|
| **M0** | Foundation | All four platform skeletons. Heap with leak checker. Fibers ping-pong on every platform. VFS skeleton. `unabashed -c 'echo hi'`. CI cross-builds x64 and i686 with w64devkit, checks imports (no CRT DLLs) and the XP subsystem version. |
| **M1** | Parser and printer | Full grammar, here-docs, aliases via the input stack, parse-time comsub, bash-identical syntax errors (`-n`), `declare -f` printer differential-tested. Parse fuzzer running. |
| **M2** | Sequential interpreter | Persistent variable store, expansions except comsub, arithmetic, compound commands, functions and locals, redirections, synchronous externals (POSIX, Win32, fakes), P0 builtins, LINENO and message parity, `set -e/-u/-x`. Most of run-minimal passes in the virtual OS. |
| **M3** | Processes without fork | Snapshots; synchronous subshells, comsub and funsub; scheduler and fibers; memory pipes and materialization; PIPESTATUS, pipefail, lastpipe; traps and virtual signals; SIGPIPE; Ctrl-C (POSIX and Win32); Win32 async I/O; exact reads. Suite category a green. Chaos fuzzer running. Benchmark gate starts here. |
| **M4** | Data and matching | Arrays with bash assoc order, namerefs, `[[ ]]` and ERE, glob/extglob/globstar/GLOBSORT, printf with floats, read/mapfile/getopts, `declare -p`, shopt and set lists. Category b green. Differential fuzz for expansions and regex. |
| **M5** | Asynchronous jobs and Windows | `&` and coproc (fork or image), wait/kill/jobs, procsub, Windows signal channel, path modes, mount table, argument and environment conversion, exe kinds, shebangs, `.bat`, persona from argv[0], startup files. Category c in the container. Git Bash path differentials. Trial drop-in as w64devkit `sh.exe` (running autoconf and make workloads). |
| **M6** | Interactive | termios, VT and legacy-console backends; emacs bindings; history and `!` expansion; completion; basic inputrc; prompts; job control (POSIX full, Windows partial). Golden vterm tests, pty differentials, winctl end to end including XP. |
| **M7** | POSIX mode and polish | POSIX-mode details, invocation and shopt suites, remaining builtins. Categories d and e. |
| **M8** | Performance and later features | 32-bit tuning, then vi mode, `bind` and programmable completion. |

---

## Top risks

1. **Refcounted persistent store:** a leak or use-after-free bug would be catastrophic. Mitigation: debug poisoning, leak-to-baseline checks on every test, fuzzing.
2. **Self-spawn image completeness:** any `Shell` field missed by the serializer is a silent divergence. Mitigation: one `Shell` struct and round-trip fuzzing.
3. **Windows pipes have no non-consuming readiness wait:** helper-thread consumption on interrupted reads on XP.
4. **Printer fidelity:** the `declare -f`, `set -x` and `declare -p` formats need large differential corpora. AST shape mismatches will show.
5. **Fiber stacks and recursion depth on 32-bit,** and running out of address space with many Procs.
6. **Cooperative scheduling latency:** a long builtin running inside a fiber cannot be preempted until its next safe point.
7. **Locale:** C.UTF-8 collation and classification differ from en_US.UTF-8.
8. **glibc regex quirks.**
9. **Interactive feel on the legacy console:** it has no deferred wrap.
10. **GPL hygiene:** help text, and tests that must never be copied.
11. **Scope:** bash plus readline is about 150k lines.

## Open questions for the user

1. Is the size-class Heap with free lists and refcounts acceptable for long-lived state, as the one exception to "arenas only"? The alternative is copying compaction, which needs global safe points and pauses.
2. Should `&` and coproc be real processes (fork on POSIX, self-spawn plus image on Windows), as recommended? Or in-process fibers: cheaper, but they die with the shell and get no parallelism.
3. Should UTF-8 mode mean C.UTF-8 semantics (codepoint collation)? en_US.UTF-8 glob and `[[ < ]]` order would then differ.
4. `help` and usage text: write our own long help, but reproduce the one-line usage synopses that appear in error messages?
5. The sh/ash persona: bash POSIX-mode semantics and messages with busybox-w32 environment conventions, or ash-style message formats? Do we want an opt-in in-process fast path for busybox NOFORK applets, to avoid a performance regression against ash in w64devkit?
6. In msys mode, should we parse our own Windows command line with Cygwin rules (unquoted-wildcard globbing, `@file`), as a Git Bash drop-in would?
7. OSTYPE and MACHTYPE per mode: `cygwin` or `msys`?
8. Is x86-64 Linux the bit-exact reference: signed-char FNV-1 for assoc order, 80-bit `long double` printf, Linux signal numbers?
9. Virtual BASHPID scheme: odd pids on Windows, above `pid_max` on Linux. Acceptable?
10. Should `exec` in the main shell use a real `execve` on POSIX? On Windows it has to be spawn, wait, exit.

### Critical files for implementation

The project directory is currently empty, so these are the files to create first.
- C:/Users/wellons/src/unabashed/unabashed.cpp (Platform API table, module order, `sh_main`)
- C:/Users/wellons/src/unabashed/proc.cpp (Shell snapshots, Procs, pipelines, state image)
- C:/Users/wellons/src/unabashed/sched.cpp (fibers, Io completion, safe points, signals)
- C:/Users/wellons/src/unabashed/vars.cpp (persistent refcounted variable store and binding stacks)
- C:/Users/wellons/src/unabashed/vos.cpp (virtual OS: VFS, virtual processes, fake programs, deterministic scheduler)

The references the design draws on are:
- C:/Users/wellons/src/skeeto.github.com/_ssg/base.cpp
- C:/Users/wellons/src/skeeto.github.com/_ssg/main_win32.cpp
- C:/Users/wellons/src/sshd/src/win32.cpp (XP-capable spawn)
- C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3/hashlib.c (semantic reference only, for assoc order)
