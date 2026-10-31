# unabashed: architecture design (performance perspective)

## 0. Key decisions

1. **No process for shell code, ever.** Subshells, `$(...)`, `<(...)`, pipeline stages that run shell code, and `&` jobs that run shell code are all **virtual processes** (`Proc`) inside the one OS process. Only external programs get `CreateProcessW` or `posix_spawn`. The same design is used on all three platforms, including POSIX (real fork is not used), so behaviour is uniform and deterministic under the virtual OS.
2. **Concurrency uses stackful fibers on one OS thread, created only when needed.** Most commands never create a fiber. A fiber appears only when two or more pieces of shell code must make progress at the same time: two or more shell stages in one pipeline, a `&` job running shell code, a coproc, or a procsub with a shell body. OS threads are used only as I/O proxies and pumps that never touch shell state.
3. **Shell state uses shallow binding over interned symbols, with a versioned undo log.** A variable read is `sym->var`, one pointer load with no hashing at run time. A synchronous subshell is "start a log; run; swap the log back". Concurrent fibers reroot the log tree when they switch, so each fiber's cost is proportional to the number of variables it changed, never to the total state size.
4. **There are two allocation regimes, each with exact lifetimes.**
   - **Scratch** is bump allocation, reset per simple command, per loop iteration and per function call.
   - **Heap** is an arena plus power-of-two free lists, holding only state that outlives a command (variable values, arrays, function blobs, history). Every heap free happens at a known point: overwrite, unset, frame pop, subshell rollback, version collapse. There is no garbage collector and nothing leaks unboundedly.
5. **Parse once, compile everything.** Words become expansion trees, literal words are fully resolved at parse time, arithmetic compiles to postfix code, patterns compile to matchers, and `$(...)` bodies are parsed once. The original source text is kept beside each word so the bash-identical printer (`declare -f`, `type`, `$BASH_COMMAND`, xtrace) can run lazily and only when someone asks.
6. **Path dialects live in the core as pure code.** The platform layer sees only canonical native absolute paths, so msys and native modes are unit-tested on every host, including inside the virtual OS.

---

## 1. Execution model without fork

### 1.1 Virtual processes

```cpp
struct Proc {
    Proc    *parent;
    Version *ver;        // state version this proc sees (see 2.4)
    Heap    *heap;       // storage outliving one command
    Task    *task;       // 0: runs synchronously on its parent's stack
    Fdtab    fds;        // virtual fd table, copied at fork
    Str      pwd;        // logical cwd, canonical native absolute
    Str      ppwd;       // physical cwd (pwd -P)
    i32      umask;
    u64      opts[2];    // set -o and shopt bits, by value
    Trap     traps[NSIG+4];  // copied/reset per subshell rules
    Frame   *frame;      // call stack: positional params, locals
    i32      status;     // $?
    i32      vpid;       // $BASHPID
    u32      sigpend;    // virtual pending signals
    i32      budget;     // back-edges left before a cooperative yield
    Joblist *jobs;
    i32      subshell;   // BASH_SUBSHELL
    u8       sync;       // parent suspended while this runs
};
```

A fork copies about 1 KiB by value: options, the trap array, the fd table (refcounted `Ofile*`), and the cwd strings. It also opens a `Version` (section 2.4). Variables, functions and aliases are never copied. They are copy-on-write at the level of individual variables, and array nodes are copy-on-write individually inside a variable.

How each construct is handled:

| Construct | Mechanism | Cost |
|---|---|---|
| `( list )`, `$(list)`, `${ list; }` | Synchronous child proc on the **same C stack**. Child heap = parent scratch (by value). Version log rolled back at the end. | O(vars written) |
| `( ext )`, `$(ext)`, last command of `-c` | Spawn directly. The capture pipe is read in a blocking loop. | 1 spawn |
| Pipeline, all external | OS pipes, spawn all, wait. | n spawns |
| Pipeline, exactly one shell stage | Externals spawned first. The shell stage runs **inline** on the current stack as a synchronous child, or in the parent itself if it is the last stage under `lastpipe`. Blocking I/O is safe because nothing else in-process needs to run. | 0 fibers |
| Pipeline, two or more shell stages | One fiber per shell stage except the last (the last runs on the caller's stack). `Vpipe` between adjacent shell stages, OS pipes next to externals. | Fiber from a pool, about 100 ns |
| `f &`, `coproc f`, `<(f)` with a shell body | Fiber with its own heap region. Outlives the command. | Pooled fiber + region |
| `exec ext` in the main proc | POSIX: `execve`. Win32: spawn, wait, `ExitProcess(code)`, as busybox does. | |
| `exec ext` in a virtual proc | Spawn; the proc becomes a waiter; its status is the child's. | |
| `exit` / fatal error in a proc | `__builtin_longjmp` to the proc base, then rollback. `break`, `continue` and `return` are propagated return codes, not longjmps. | |

`$$` is the real OS pid. `$BASHPID`, and `$!` for shell jobs, are **virtual pids** that cannot collide with real ones: on Windows they are odd numbers, since real PIDs are multiples of 4; on Linux they are values above `pid_max`, i.e. above 2^22. `kill`, `wait` and `jobs` look up the virtual table first.

### 1.2 Fibers and the scheduler

```cpp
struct Task {            // a proc running on its own fiber
    Task   *next;        // run queue
    Proc   *proc;
    Fiber  *fiber;       // platform object (Win32 fiber, asm context)
    Ioreq  *io;          // pending OS request, or
    Vpipe  *vp;          // blocked on an in-memory pipe
    i32     state;       // TASK_READY, TASK_IO, TASK_PIPE, TASK_CHILD, TASK_DONE
};

struct Vpipe {           // pipe between two shell stages, no syscalls
    u8   *ring;          // 64 KiB, same capacity as a Linux pipe
    iz    head;
    iz    len;
    i32   readers;
    i32   writers;
    Task *rwait;
    Task *wwait;
    Fh    os;            // set if materialized for an external (1.3)
};
```

- **Switch points.** A task switches only when one of these happens:
  - a `Vpipe` is full or empty;
  - an OS request is pending;
  - the task waits for a child;
  - `sleep`, `read -t`, or the line editor waits for input;
  - `budget` reaches 0 at a loop back-edge or function call. That is a decrement and a branch, about every 1 ms of work, and it gives fairness to CPU-bound `&` jobs while the prompt stays responsive.
- **Fast path.** When the run queue holds only the current task, every I/O call is a plain blocking `os_read`/`os_write` with no `Ioreq` and no poll. That is 99% of executions.
- **Async path.** When more than one task is alive, OS I/O goes through completion-style requests (`os_iostart` / `os_poll`, section 4).
  - Pipes the shell creates and keeps one end of use the async-capable end.
    - Win32: an overlapped named-pipe server end, `\\.\pipe\unabashed-<pid>-<n>`. The child always gets a synchronous client end.
    - POSIX: `poll`, then a bounded write.
  - Foreign handles (an inherited pipe or console) are served on Win32 by an **I/O proxy thread**. It performs one blocking `ReadFile`/`WriteFile` into the requesting task's buffer and signals an event. It touches no shell state, so no locks are needed. On XP this uses `CreateThread` and events only.
- **Deadlock.** With everything async, the only way to hang is a genuine cycle: every task blocked on Vpipes, with no OS request and no child pending. Real bash would hang the same way. The virtual OS detects that state and reports `deadlock`, which is a useful test oracle. The real builds wait for Ctrl-C.
- **Determinism.** The run queue is FIFO in creation order. The virtual OS is a co-scheduled participant, and virtual time advances only when everything is blocked. A given script therefore produces the same interleaving every time, which makes fuzzing reproducible.

### 1.3 Fd virtualization

```cpp
enum : i32 { OF_OS, OF_MEM, OF_CAPTURE, OF_VPIPE, OF_NULL };

struct Ofile {           // open file description, refcounted, shared by dup/fork
    i32    kind;
    i32    refs;
    i32    flags;        // OF_RD, OF_WR, OF_APPEND, OF_ASYNC, OF_SEEKABLE
    Fh     fh;           // OF_OS
    Str    mem;          // OF_MEM: here-doc/here-string body
    i64    off;
    Buf   *cap;          // OF_CAPTURE: $(...) output, lives in parent scratch
    Vpipe *vp;           // OF_VPIPE
    Rbuf  *ahead;        // read-ahead for OF_OS input (see below)
};

struct Fdtab {
    Ofile  *lo[10];      // fds 0-9 inline
    Ofile **hi;          // {fd}>x allocations, 10 and up
    i32     nhi;
    u32     cloexec;
};
```

- **The real process fds never change.** `exec 3<f` or `exec >log` only edits the virtual table. Real handles matter only at spawn time, when the core builds the child's fd array from the non-cloexec entries.
- **Here-docs and here-strings** are `OF_MEM` for builtins and functions: no pipe and no temp file. For an external:
  - a body of at most 64 KiB goes into a pipe, written in full, then closed;
  - a larger body is fed by `os_pump`.
- **`$(...)`.** fd 1 is `OF_CAPTURE`, an append-only `Buf`, so `echo` inside a comsub is a `memcpy`.
  - If an external runs inside the comsub, an OS pipe is created lazily and drained into the same `Buf`. Ordering is kept by draining before each later builtin write, and EOF is awaited at the end. This matches bash's blocking on `x=$(sleep 10 &)`.
  - CRLF stripping happens at capture time in msys mode.
- **Materialization.** A `Vpipe` end or a procsub that must reach an external (it is inherited, or a procsub path appears in argv) is converted at spawn time into a real OS pipe. Win32 uses a named pipe and the argv path is `\\.\pipe\...`; POSIX uses a pipe placed at the same fd number, so `/dev/fd/63` stays valid. Bytes already buffered are written first. When the shell itself opens a procsub path, it recognizes its own registered name and attaches the `Vpipe` directly.
- **Read-ahead with pump on hand-off.** This is what beats bash's one-syscall-per-byte reads on pipes.
  - `read`, `mapfile` and the script reader fill a 64 KiB `Rbuf` per input `Ofile` and use `memchr` to find delimiters.
  - Seekable files: the offset is synced lazily, with one `lseek` back at hand-off or exit, instead of one per line as bash does.
  - Pipes: when an external is spawned with an fd whose `Rbuf` is non-empty, the core calls `os_pump(prefix, fh)`. That makes a new pipe, writes the buffered prefix, then copies the rest from the original handle.
  - When the child exits, the pump stops. The bytes it read but the child did not consume (pump buffer plus pipe contents) are pulled back into `Rbuf`. No data is lost even for children that read one byte at a time.
  - With an empty `Rbuf` the raw handle is passed and no pump is needed.
  - Tty input is canonical-line, so a read never overruns a line.

### 1.4 Spawning and waiting

- **Win32**, single-threaded spawner, so no inheritance races:
  1. Every shell handle is created non-inheritable.
  2. For each spawn, `DuplicateHandle(..., inherit=1)` the child's fds 0-2.
  3. `CreateProcessW(app=resolved path, cmdline, CREATE_UNICODE_ENVIRONMENT|CREATE_SUSPENDED, envblock, cwd=proc->pwd)`, then assign the process to a per-pipeline job object, then `ResumeThread`. If `AssignProcessToJobObject` fails (nested jobs before Win8), fall back to breakaway or pid tracking.
  4. Close the duplicates.
  - Proxy threads never create handles, which is why the spawner can stay lock-free.
  - Fds 3 and up are passed through the `lpReserved2` CRT blob only when they are actually open, with slot width matched to the child PE's machine type.
  - `.bat`/`.cmd` files go through an explicit `%ComSpec% /d /s /c` with the Rust-style escaping.
  - Waiting: process handles, the async events, the console handle and the interrupt event go into `WaitForMultipleObjects` when there are 63 or fewer. Beyond that, job-object IOCP notifications are used. Both work on XP.
  - Background jobs get `CREATE_NEW_PROCESS_GROUP`.
- **POSIX**: `posix_spawn` (vfork-class) with `dup2` file actions, `addchdir_np` for the virtual cwd, `SETSIGDEF` so children get default `SIGPIPE`/`SIGINT`, `SETPGROUP`, and the process umask set to `proc->umask` just before the spawn. It falls back to `vfork`+`exec` where `addchdir` is missing.
  - `SIGCHLD` and `SIGINT` write to a self-pipe that is part of every `poll`.
  - The shell ignores `SIGPIPE`, and a builtin getting `EPIPE` makes its proc die with status 141.
- **The command line, env block and msys argument conversion are built by the core** (pure code in `wincmd.cpp`). The platform receives a finished command line or argv.

### 1.5 Signals, Ctrl-C and traps

- **Delivery.** Real signals (POSIX handlers, or the Win32 console control handler thread) only set a bit in `Os` and a wake event or self-pipe byte. The core collects them with `os_sigpending` at checkpoints: loop back-edges, between commands, and whenever `os_poll` wakes.
- **SIGINT** marks `sigpend` on every proc of the foreground job. Each proc then runs its trap or unwinds with status 130.
- **Mixed foreground jobs on POSIX** (fibers plus externals): the externals stay in the shell's own process group for that job, so the terminal's SIGINT reaches the fibers too. Stopping such jobs (Ctrl-Z) is deferred to a later milestone.
- **Traps** are per proc. Subshell reset rules are applied at fork. The EXIT trap of a virtual subshell runs at proc end.
- **Ignored signals.** `trap '' INT` maps to `SetConsoleCtrlHandler(0,1)` on Win32, which children inherit as POSIX expects.

### 1.6 Rejected alternatives

| Alternative | Why rejected |
|---|---|
| Forkshell (busybox) | O(state) serialization plus a `CreateProcess` per subshell. This is the cost we exist to beat. |
| Thread per stage | Shell state would need deep copies or locks; XP has no SRW locks or condition variables; scheduling would not be deterministic. |
| Real fork on POSIX | Page-table cost, and different behaviour from Windows. Kept only as a possible debug comparison switch. |
| Stackless CPS executor | Would make expansion, comsubs and arithmetic resumable state machines. The complexity is not justified when stackful fibers cost about 100 ns. |

---

## 2. Memory model

### 2.1 Arenas and lifetimes

| Region | Contents | Lifetime / reclamation |
|---|---|---|
| `perm` (session) | `Sym` objects and their names, builtin table, mount table, option names, compiled-pattern and eval caches (bounded, flushed when full) | Process |
| Main `Heap` | Variable values, `Var`s, arrays, assoc entries, function **blobs**, aliases, traps, history entries, kill ring, positional parameters set by `set --` | Explicit frees at overwrite, unset, frame pop, version collapse |
| Top-level scratch | AST of the current complete command (including a loop running for hours) | Reset after each top-level command (the shellexec sweep) |
| Per-command scratch (by value) | Expanded words, argv, redirections, env overrides, `$(...)` capture `Buf`, glob results, xtrace text | Vanishes on return from `runsimple` |
| Per-iteration scratch | Everything an iteration allocates | `Arena iter = scratch;` at each back-edge |
| Sync child heap | A subshell's copy-on-write `Var`s and its log | A `Heap` over the parent's scratch, discarded at exit |
| Task region | Fiber proc heap (pipeline stage, `&` job, coproc) | Reserved per task, released when the task ends |

The arena gets **commit-on-grow** without touching the fast path. `end` is the committed limit. On overflow the slow path commits more of the reservation, or longjmps to the proc's OOM handler:

```cpp
struct Region {          // backing for a growable arena
    Os     *os;
    byte   *limit;       // end of reservation
    void  **oom;         // __builtin_setjmp buffer of the owning proc
};
struct Arena {
    byte   *beg = {};
    byte   *end = {};    // end of *committed* memory
    Region *rgn = {};
};
```

`alloc<T>` is skeeto's canonical version, except that `os_oom()` is replaced by `arenagrow(a, need)`. That function either commits more and retries, or calls `__builtin_longjmp(*a->rgn->oom, 1)`.

### 2.2 Heap: free lists over an arena

```cpp
struct Heap {
    Arena  arena;
    void  *free[40];     // classes 2^4 .. 2^27, plus 1.5x half-steps
    iz     live;         // bytes in use (stats, `ulimit -v`-like caps)
};
static u8  *heapget(Heap *, iz size, i32 *cls);  // rounds up, returns class
static void heapput(Heap *, void *, i32 cls);
```

This is the one sanctioned departure from pure bump allocation, and it is used only for mutable state that outlives a command. Class sizes give geometric capacity, so `s+=x` appends in place until the class fills and then moves up one class. That makes it amortized O(1); bash is O(N²) here. Fragmentation is bounded by 1.5x.

### 2.3 Scratch discipline in the executor

```cpp
static i32 runsimple(Proc *p, Simple *c, Arena scratch)
{
    Argv av = expandargs(p, c, &scratch);   // literals: pointer copies
    Cmd  k  = resolve(p, c, av);            // Sym->func, Sym->builtin, hash
    ...
    return status;                          // everything above is freed
}

static i32 runwhile(Proc *p, Loop *n, Arena scratch)
{
    i32 s = 0;
    for (;;) {
        Arena iter = scratch;               // per-iteration sweep
        if (runlist(p, n->cond, iter) != 0) break;
        s = runlist(p, n->body, iter);
        if (checkpoint(p, &s)) break;       // signals, break/continue, yield
    }
    return s;
}
```

The invariant: **anything reachable from `Proc` or `Sym` after a command returns must be in a `Heap`**. Builtins that persist things (`read`, `cd`, `set --`, `exec 3<`, `trap`, `alias`, function definitions) copy once from scratch into the heap. Expansions always copy variable bytes into scratch, so a value buffer can be freed or reused the moment it is overwritten.

### 2.4 Versions: subshell isolation and fiber rerooting

```cpp
struct Undo {            // swap-encoded: applying twice is the identity
    void **slot;
    void  *val;
};

struct Version {
    Version *up;
    Undo    *log;        // heap-grown vector
    iz       len;
    iz       cap;
    u32      minepoch;   // objects with epoch >= minepoch are writable here
    u32      epoch;      // stamp for objects created in this version
    i32      refs;       // live procs and child versions
    b32      applied;    // currently reflected in Sym cells
};

// All shared state is reached through Sym cells:
// sym->var, sym->func, sym->alias, sym->cmd, Var->shadowed.
static Var *writable(Proc *p, Sym *s)
{
    Version *v = p->ver;
    Var     *x = s->var;
    if (x && x->epoch>=v->minepoch) {
        return x;                           // hot path: mutate in place
    }
    Var *n = copyvar(p->heap, x, v->epoch); // header copy; arrays share nodes
    logslot(p, (void **)&s->var, x);        // remember old pointer
    s->var = n;
    return n;
}
```

- **The root version keeps no log** while it has no live concurrent children. Every write is in place, so the common case costs nothing.
- **Synchronous subshell.** It gets a fresh epoch and its own log in the child heap. The parent is suspended, so the parent never freezes anything. At exit, apply the log in reverse (swaps), then drop the child heap. Each variable is logged at most once per subshell, because after copy-on-write its epoch matches.
- **Concurrent fork** (fiber child while the parent can still run, e.g. `lastpipe` or `&`):
  - The current root version becomes a frozen node `V0`.
  - The parent continues in `V1{up=V0}`, the child in `V2{up=V0}`. The Sym cells always reflect the **active** task's path.
  - Switching from task A to task B reroots: unapply A's path up to the lowest common ancestor (swaps in reverse), then apply down to B (swaps forward). The cost is O(variables written by those versions), typically a handful, and it is paid once per 64 KiB of pipe traffic or per time slice.
- **Collapse.** When `V2` dies and `V0` has only `V1` left:
  - The old objects in `V1`'s log entries become unreachable and are `heapput` back to the parent heap, so reclamation is exact.
  - `V1` merges into the root and `minepoch` drops back, so in-place writes resume.
- **Arrays** are copy-on-write per radix node, using the same epoch test. `( a[5]=1 )` copies one path, O(log₆₄ n), not the array.

### 2.5 Long loops and mutation never leak

| Statement in a loop | What happens per iteration |
|---|---|
| `i=$((i+1))` | Writes a native `i64` into `Val`. No allocation. |
| `s+=x` | In-place append within the size class. Amortized O(1), no garbage. |
| `x=$(cmd)` | Capture goes to iteration scratch, then into the heap. The old buffer is freed or reused. |
| `local v` in a called function | Heap `Var`, freed at frame pop. |
| `f &` in a loop | Each job's region is released at its end. Parent garbage is at most (variables written while jobs live) × one header, and is reclaimed at collapse. |
| Redefining a function | A two-pass "measure then relocate" copy into one heap blob. The old blob is freed when its `busy` count reaches 0, which covers redefinition during execution. |
| `eval "$s"` | The AST goes in scratch. A bounded parse cache is keyed by (text, alias generation, parser options). |

The one known slow growth is the `Sym` table, which gains entries for dynamically generated names (`declare "v_$i"`). At top-level boundaries, a sweep removes Syms with all cells empty and zero pins, once the table has grown 2x since the last sweep.

### 2.6 OOM and 32-bit XP

- **OOM** longjmps to the owning proc. A virtual subshell or job dies with `unabashed: xmalloc: cannot allocate N bytes` and status 2. That matches a forked bash child dying while its parent survives. In the main proc: interactive mode aborts the command and returns to the prompt; non-interactive mode exits 2.
- **x86/XP reservations** inside the 2 GiB address space:
  - main heap 512 MiB, top-level scratch 256 MiB;
  - task regions 16 MiB each;
  - fiber stacks 1 MiB reserved, committed by the OS guard page.
- **x64**: 64 GiB main and 1 GiB per task are reserved; Linux uses `MAP_NORESERVE`.
- **Recursion guard.** Deep recursion is checked against remaining stack (sp minus limit under 64 KiB) and gives "maximum function nesting level exceeded" instead of a segfault.

---

## 3. Core representations

### 3.1 AST and words

```cpp
struct Word {
    Str   text;          // original source bytes: printer, set -v, errors
    Part *parts;         // expansion tree; 0 when W_LITERAL
    Str   lit;           // final value when W_LITERAL (quotes removed)
    Sym  *sym;           // plain identifier / assignment target
    i32   flags;         // W_LITERAL, W_ASSIGN, W_GLOB, W_BRACE, W_TILDE, W_DQ
};

struct Part {
    Part  *next;
    i32    kind;         // P_LIT, P_PARAM, P_COMSUB, P_ARITH, P_PROCSUB, P_TILDE, P_GLOB
    i32    flags;        // quoted context, splitting/globbing eligibility
    Str    lit;
    Param *param;        // Sym, subscript Arith/Word, op, operand Words
    Node  *cmd;          // $(...) parsed once at parse time
    Arith *arith;        // compiled, or 0 if the body needs runtime parsing
};

struct Simple {
    Node    hdr;         // kind, line, flags
    Assign *assigns;
    Word   *words;
    i32     nwords;
    Redir  *redirs;
    Sym    *cmdsym;      // set when words[0] is literal: O(1) resolution
};
```

- **The printer** (`print.cpp`) is a from-scratch reproduction of bash's canonical layout: 4-space indent, `;` and newline placement, here-doc bodies, `function` vs `name ()`. It prints `Word.text`, the post-alias token text, exactly as bash does. It is checked by differential tests over generated ASTs.
- **`$BASH_COMMAND`, xtrace, job text.** The proc holds a pointer to the current `Simple*`, and the string is printed only when `$BASH_COMMAND` is read or a DEBUG/ERR trap fires. Xtrace quoting is done only under `set -x`. Background-job text is printed at job creation.
- **Aliases** are expanded textually in the lexer by pushing the alias text onto the input stack, so parse-time semantics match bash. Function blobs store the post-alias tree.

### 3.2 Arithmetic

```cpp
struct Ainst {
    i32 op;              // A_NUM, A_VAR, A_IDX, A_BIN..., A_JZ/A_JNZ (&&, ||, ?:), A_ASSIGN..., A_ERR
    i32 src;             // byte offset into text, for "error token is" messages
    union { i64 num; Sym *sym; Arith *sub; i32 jump; };
};
struct Arith { Ainst *code; i32 len; Str text; };
```

- Literal `$(( ))`, `(( ))`, `let`, and arithmetic `for` compile at parse time.
- Syntax errors compile to an `A_ERR` instruction that emits bash's exact message when evaluated, preserving bash's runtime-error timing.
- Variable operands:
  - a native int is used directly;
  - a decimal-looking string goes through a fast parse;
  - any other string is evaluated recursively as an expression, with bash's recursion limit, through a small text-keyed cache.
- Bodies containing `$` are expanded first, then looked up in the cache by the resulting text.

### 3.3 Patterns and ERE

```cpp
struct Glob {
    Gop *ops;            // LIT(str), ANY1, SET(class), STAR, EXT(kind, alts)
    i32  nops;
    Str  prefix;         // literal anchors: fast reject, memchr seeds
    Str  suffix;
    i32  minlen;
    u32  flags;          // G_EXT, G_NOCASE, G_UTF8, G_HASSLASH
};
```

- **Plain globs** use a two-pointer star backtracker over bytes. UTF-8 is decoded only at `?` and bracket expressions.
- **`${s//pat/rep}`** runs a bitset NFA from each candidate start (prefiltered with `memchr`) and reports every match end in one pass, so the longest match costs O(n·m/64) instead of bash's O(n²) strmatch probes. `#`/`##` run forward from 0; `%`/`%%` run a reversed program from the end.
- **Extglob**, including `!(...)`, uses memoized backtracking over (op, position) in scratch, which gives exact semantics.
- **Literal patterns** in `case`, `[[ == ]]` and parameter expansions compile once.
- **Pathname expansion** reads the directory listing once (`d_type`, or `FindFirstFileExW` attributes, so no per-entry stat), matches, then merge-sorts counted strings in byte order. Locale collation is an open question.
- **`[[ =~ ]]`** uses our own POSIX ERE engine with the glibc extensions (`\w \s \b \< \>`, backrefs). The overall leftmost-longest match comes from a Pike VM. Submatches are resolved inside that known span by a constrained backtracker using POSIX subexpression priority. Correctness against glibc is checked by differential fuzzing.

### 3.4 Symbols, variables and values

```cpp
struct Sym {             // interned once at parse time, stable address
    Str      name;
    u64      hash;
    Var     *var;        // innermost binding in the active view
    Func    *func;
    Alias   *alias;
    Cmdhash *cmd;        // PATH lookup cache (path, PATH generation, kind)
    Builtin *builtin;    // static, set at startup for builtin names
    i32      special;    // RANDOM, SECONDS, LINENO, FUNCNAME, BASHPID, ...
    i32      pins;       // references from persistent ASTs
};

struct Val {             // 24 bytes
    union { u8 *data; i64 num; u8 sso[16]; };
    i32 len;
    u16 kind;            // V_STR, V_INT, V_SSO, V_UNSET
    u16 cls;             // heap class of data, 0 if borrowed or static
};

struct Var {
    Val    val;          // scalar
    Array *arr;          // indexed (radix) or
    Assoc *assoc;        // bash-ordered chained hash
    Var   *shadowed;     // next outer binding (shallow-binding chain)
    Sym   *ref;          // nameref target
    u32    attrs;        // A_INT, A_EXPORT, A_RDONLY, A_LOWER, ...
    u32    epoch;
    i32    depth;        // function depth that created it (local)
};
```

- **Lookup** is O(1) at any call depth. `local` pushes onto `shadowed`, and the frame records its Syms so return can pop them. `declare -g` walks this one Sym's chain only.
- **Dynamic names** (`${!x}`, `declare $n`) intern through an MSI index in O(len).
- **FUNCNAME, BASH_SOURCE and BASH_LINENO** live in the `Frame` chain and are materialized as arrays only when read. A call costs O(1); bash's array renumbering is O(d) per call.
- **Indexed arrays** are a variable-height 64-way radix tree with presence bitmaps and copy-on-write by epoch. `count` and `maxidx` are cached. In-order iteration is a depth-first walk, and sparse indices cost little.
- **Assoc arrays** reproduce bash's observable order by construction:
  - buckets start at 1024; FNV-1a 32-bit;
  - new entries are inserted at the head of their chain;
  - the table grows 4x when `entries >= 2*buckets`;
  - a rehash walks the old buckets 0..n-1 and re-inserts at head;
  - iteration runs over buckets 0..n-1.
  
  Entries are single heap blocks of {next, hash, key, val}. The same structure backs the `hash` builtin's listing order.
- **Env block cache.** An `exportgen` counter is bumped by any change to an exported variable or attribute. The platform block (sorted UTF-16 on Win32, `char**` on POSIX) is rebuilt only when that counter moved. Per-command `FOO=1 ext` merges overrides in scratch.
- **UTF-8 position memo.** Each proc remembers its last (data pointer, char index, byte offset) triple, which makes `${s:i:1}` loops O(1) per step.
- **Numbers.** `printf` integer conversions are hand-written. Floating point never uses the FPU or libc: decimal ↔ (64-bit mantissa, exponent) conversion is exact bignum code that emulates x87 `long double` precision, so `printf '%.20f'` output matches x86-64 Linux bash on every platform. Bash arithmetic is integer-only, so no float operations are needed.

---

## 4. Platform API

```cpp
struct Os;
using Fh = i64;          // fd (POSIX), HANDLE (Win32), node index (vos)

struct Pipe   { Fh rd; Fh wr; i32 err; };
struct Stat   { i64 size; i64 mtime; i64 atime; i64 ctime; i64 dev; i64 ino;
                i32 mode; i32 nlink; i32 uid; i32 gid; i32 attrs; i32 err; };
struct Dirent { Dirent *next; Str name; i32 type; i32 attrs; };
struct Child  { i64 pid; Fh h; i32 err; };
struct Ioreq  { Fh fh; u8 *buf; iz len; iz done; i32 op; i32 err; u8 plat[32]; };
struct Spawn  {
    Str  path;           // resolved native path
    Str  cmdline;        // Win32: fully quoted line (built by core)
    Str *argv;           // POSIX
    i32  argc;
    Str  env;            // block from os_envblock
    Str  cwd;
    Fh  *fds;            // child fd i <- parent handle, -1 closed
    i32  nfds;
    i32  flags;          // SP_BG (ignore ^C, new group), SP_EXEC, SP_CONSOLE
    Fh   job;            // job object / pgid, 0 for new
    i32  umask;
    u32  sigign;
};

// Memory
static byte  *os_reserve(Os *, iz);                  // address space only
static b32    os_commit(Os *, byte *, iz);           // no-op on POSIX
static void   os_release(Os *, byte *, iz);
[[noreturn]] static void os_oom();                   // last resort only

// Files and handles (paths: canonical native absolute, UTF-8/WTF-8)
static Fh     os_open(Os *, Str path, i32 flags, i32 mode, Arena);
static i32    os_close(Os *, Fh);
static iz     os_read(Os *, Fh, u8 *, iz);           // blocking; <0 is -E_*
static iz     os_write(Os *, Fh, u8 *, iz);
static i64    os_seek(Os *, Fh, i64, i32 whence);
static Pipe   os_pipe(Os *, i32 asyncend);           // 0 none, 1 rd, 2 wr
static Fh     os_pump(Os *, Fh src, Str prefix);     // prefix then src
static Str    os_unpump(Os *, Fh pump, Arena *);     // stop, return leftovers
static i32    os_fhkind(Os *, Fh);                   // FH_FILE, FH_PIPE, FH_TTY, FH_NULL
static Stat   os_stat(Os *, Str path, b32 follow, Arena);
static Stat   os_fstat(Os *, Fh);
static Dirent *os_list(Os *, Str dir, Arena *);
static i32    os_access(Os *, Str path, i32 mode, Arena);
static Str    os_readlink(Os *, Str path, Arena *);
static Str    os_canon(Os *, Str path, Arena *);     // pwd -P
static i32    os_mkfifo(Os *, Str path, i32 mode, Arena);  // POSIX only

// Async I/O and waiting (used only when more than one task is alive)
static b32    os_iostart(Os *, Ioreq *);             // 1 if completed now
static i32    os_poll(Os *, Ioreq **, i32 nio, Child *, i32 nchild,
                      Fh tty, i64 timeout_ns);       // index ready, or -1

// Processes
static Str    os_envblock(Os *, Str *pairs, iz n, Arena *);
static Child  os_spawn(Os *, Spawn *, Arena);
[[noreturn]] static void os_exec(Os *, Spawn *, Arena);  // Win32: spawn+wait+exit
static i32    os_reap(Os *, Child, b32 wait);        // status, or -1 running
static b32    os_kill(Os *, i64 pid, i32 sig);
static b32    os_killjob(Os *, Fh job, i32 sig);
static i32    os_tcpgrp(Os *, Fh tty, i64 pgid);     // POSIX job control

// Signals
static u32    os_sigpending(Os *);                   // fetch-and-clear bits
static void   os_sigmode(Os *, i32 sig, i32 mode);   // SIG_DFL/IGN/CATCH

// Terminal (editor)
static b32    os_termraw(Os *, Fh, b32 raw);
static i32    os_termsize(Os *, Fh);                 // rows<<16 | cols
static iz     os_termread(Os *, Fh, u8 *, iz, i64 timeout_ns);  // VT bytes
static b32    os_termwrite(Os *, Fh, Str);           // WriteConsoleW or write
static b32    os_con(Os *, Fh, Conop *);             // legacy console ops

// Fibers
static Fiber *os_fiber(Os *, iz stack, void (*)(void *), void *);
static void   os_switch(Os *, Fiber *);
static void   os_fiberfree(Os *, Fiber *);

// Time, identity, misc
static i64    os_now(Os *);                          // realtime ns
static i64    os_clock(Os *);                        // monotonic ns
static Tz     os_tz(Os *, i64 t);                    // offset, abbrev, isdst
static Times  os_times(Os *);                        // self + children CPU
static void   os_random(Os *, u8 *, iz);
static Str   *os_environ(Os *, Arena *, iz *n);
static Ident  os_ident(Os *, Arena *);               // pid, ppid, uid, host, user, exe
static Str    os_home(Os *, Str user, Arena *);
[[noreturn]] static void os_exit(Os *, i32);
```

- **Errors** are core `E_*` enumerators, mapped from errno or `GetLastError`. The core owns bash's message text.
- **Win32 legacy console.** That layer turns `KEY_EVENT` records into xterm byte sequences (ESC prefix for Alt, AltGr kept as text, `VK_PACKET` passed as UTF-8), so the core has a single VT input decoder and inputrc key sequences work unchanged.
- **Rendering.** VT output is emitted by the core. On XP and legacy consoles, `os_con` handles cursor position, fill and scroll.

**Path translation is entirely in the core.** `path.cpp` has a dialect enum: POSIX, MSYS, or NATIVE.
- MSYS is chosen when argv[0] is `bash`; NATIVE when it is `sh` or `ash`; `shopt msyspath` toggles it.
- `path.cpp` covers the mount table, input normalization (`C:\x`, `C:/x`, `/c/x`), the meaning of a leading `/`, output form, PATH separator, environment translation at import and at spawn, and `cygpath`.
- `wincmd.cpp` covers MSVCRT, Cygwin and cmd quoting, msys2 argument and environment conversion, and PE kind sniffing (cached by path and mtime).
- Both are pure code. The virtual OS has a `windows` flag (drive letters, case-insensitive names, reserved names), so msys-mode scripts are tested on Linux CI.

---

## 5. Testing architecture

1. **Unit tests** (`main_test.cpp`, `test/*.cpp`, ssg-style `expect`), table-driven: base, heap, versions (random write/fork/switch sequences checked against a naive model), lexer, parser↔printer round trip, arithmetic, globs, ERE, number formatting, paths in both dialects, command-line quoting, the VT decoder, and editor commands on a virtual terminal grid.
2. **Virtual OS** (`vos.cpp`):
   - In-memory filesystem (hash trie of nodes with per-directory child maps), virtual clock, virtual pids, virtual console (cell grid plus emulated legacy console API).
   - `os_spawn` runs **fake programs** (`fakes.cpp`) as vos processes on host fibers: cat, echo, printf, true/false, sleep, date, rm, mkdir, ls, grep (using our ERE), sed `s///`, wc, sort, head, tail, tr, cut, env, printenv, recho, zecho, kill, a tiny awk.
   - `THIS_SH` runs a fresh instance of our shell core in a new region.
   - Nothing touches the real filesystem or `CreateProcess`.
3. **Our own script tests** (`tests/*.sh` plus `.out`/`.err`/`.status`). The expected files are generated by running *our* scripts under real bash 5.3 (WSL Debian, and a Docker image built from the tarball with the default configure) and committed with a recorded bash version. They run under the vos in `main_test`.
4. **Bash's test suite as an external oracle, never copied.** A runner reads `$BASHSRC/tests` from outside the repo and reimplements the `run-*` contract. It runs categories (a) and (b), plus the (c) tests that fakes cover, on the vos, and diffs against the `.right` files at run time. Pass rates are tracked per test.
5. **Differential testing.**
   - `tools/difftest` runs a corpus under real bash and under `main_posix` inside the same disposable container: `--network none`, read-only rootfs, tmpfs workdir. It compares stdout, stderr, status and the final `declare -p` dump.
   - MSYS-specific behaviour is compared with Git Bash 5.3.15 in a scratch directory.
6. **Fuzzing** (`main_fuzz.cpp`, AFL++, vos only, 16 MiB arenas, step and output budgets):
   - (a) parse, then print, then parse, then print must be idempotent;
   - (b) execution with a fork-isolation check: `( S ); declare -p` must equal `declare -p` alone;
   - (c) differential: interesting corpus entries from a grammar-based generator limited to deterministic builtins are replayed against bash in the container.
7. **Line editor.**
   - Headless: a key-byte script goes in and the virtual screen is compared, for VT, legacy console and termios-shaped backends.
   - End to end: `winctl run conhost.exe unabashed.exe -i`, then `type` and `key` chords, then `shot` plus a hidden `--dump-console` that dumps the console buffer. Run on Windows 11 conhost, Windows Terminal, and the XP laptop with an i686 build.
8. **Benchmarks** (`bench/B1..B13, M1..M5`): checksum equality first, then hyperfine against bash 5.3, MSYS2/Git Bash and busybox ash. CI gates on instruction counts (cachegrind on `main_posix`) and fails on a regression above 5%.

---

## 6. Source layout and milestones

```
unabashed.cpp     core root: includes modules in order, Platform API table, sh_main
base.cpp          types, Arena(+Region), Str, Slice, Buf, sort, utf8, hash trie
heap.cpp          size-class Heap
sym.cpp           interning, Sym, Version/Undo, writable(), reroot()
vars.cpp          Var/Val, attributes, namerefs, specials, env cache
array.cpp         radix arrays, bash-ordered assoc
lex.cpp parse.cpp print.cpp    input stack, aliases, heredocs, AST, printer
arith.cpp glob.cpp regex.cpp   compilers and matchers
expand.cpp        brace/tilde/param/comsub/arith/split/glob/quote removal
fd.cpp            Ofile, Fdtab, Rbuf read-ahead, capture, Vpipe, pumps
sched.cpp proc.cpp exec.cpp    tasks, procs/jobs/wait, executor, traps
path.cpp wincmd.cpp numfmt.cpp time.cpp                  pure helpers
builtin_*.cpp     grouped: core, vars, io (read/printf/echo/mapfile), jobs, shopt, hist, complete
edit.cpp edit_term.cpp history.cpp inputrc.cpp complete.cpp
main_win32.cpp + wide.cpp   main_posix.cpp   main_test.cpp + vos.cpp + fakes.cpp
main_fuzz.cpp     test/*.cpp  tests/*.sh  bench/  tools/
```

Milestones. Each one ends runnable and tested.

| # | Milestone | Ends with |
|---|---|---|
| M0 | Foundation | base, heap, the `os_*` table with three layers, `-c 'echo hi; exit 3'`, parser fuzz harness |
| M1 | Full grammar | Lexer, parser and printer; `--dump` matches `declare -f` differentially; round-trip fuzzing |
| M2 | Pure shell | Expansions, variables, arrays, arithmetic, globs on the vos filesystem, `[[ ]]`, test, declare, local, read from memory; script tests; first bash-oracle category (a) runs |
| M3 | Externals and fds | Spawn on Win32, POSIX and vos; PATH and hash; redirections; external pipelines; synchronous subshells and comsubs; here-docs; read-ahead and pumps; B1, B2, B7, B9, B10 beating bash; `bash -c` agent workload in msys mode |
| M4 | Concurrency | Fibers, scheduler, Vpipes, async I/O, proxy threads, `&` jobs, procsub, coproc, wait, traps, Ctrl-C; full B1–B13; most of the oracle on the vos |
| M5 | Windows completeness | msys/native modes, mount table, argument conversion, cygpath, `pwd -W`, cmd/bat, shebangs, XP fallbacks; drop-in `sh.exe` for w64devkit with ash-compatibility tests |
| M6 | Editor milestone 1 | Emacs bindings, history and `!` expansion, filename/command completion, inputrc basics; three backends; winctl end-to-end |
| M7 | Fidelity sweep | Job control (POSIX full, Windows approximate), printf floats, ERE vs glibc, full shopt, posix mode, startup files, `time` |
| M8 | Editor milestone 2 | vi mode, `bind`, programmable completion; M2 (bash-completion) benchmark |

---

## 7. Why each benchmark is won by construction

Costs are design targets for `-O2`, not measurements. `-O0` should be about 2–3x slower, because the core uses flat `switch` interpreters, few templates and no deep call chains in hot loops.

| Benchmark | bash 5.3 cost | unabashed mechanism | Expected win |
|---|---|---|---|
| B1 arithmetic loop | Re-tokenizes, converts strings, mallocs per iteration (µs) | Precompiled code, native `i64` in place; about 30–50 ns/iteration | 30–50x |
| B2 `s+=x` | O(N²) copying | Size-class in-place append | Asymptotic |
| B3 arrays | O(n) random access on a list | Radix tree, about 4 loads | Asymptotic on random access |
| B4 assoc | malloc per entry | Same order, heap blocks, cached hash | 3–5x |
| B5 `${//}` | O(n²·m), wchar conversion | NFA, memchr prefilter, UTF-8 in place | Asymptotic |
| B6 read loop | 1 syscall/byte on pipes, lseek/line on files | 64 KiB read-ahead, pump only on hand-off | 10–100x on pipes |
| B7 `$(echo)` | fork (Linux 0.3 ms, MSYS 5–20 ms) | Undo-log subshell plus memcpy capture, about 0.3 µs; externals via vfork / direct CreateProcess | 1000x+ |
| B8 builtin pipeline | 2 forks per iteration | Pooled fibers plus Vpipe | 100x+ |
| B9 function calls | Copies the whole body per call, O(d²) FUNCNAME | Shared blob, lazy frames | 5–20x |
| B10 deep scope lookup | O(depth) hashing | One load | Asymptotic |
| B11 glob | strmatch plus stat per entry, strcoll sort | Compiled match, d_type/FindFirstFileEx attributes, byte merge sort | 3–10x |
| B12 here-docs | Temp file above 4 KiB | OF_MEM for builtins; prefilled pipe for externals | 10x+ for builtins |
| B13 printf | libc printf plus fflush per call | Own formatter, one write per builtin (same syscall count as bash) | 2–4x |
| M1 startup | Dynamic libraries, locale, env | Static, CRT-free, lazy Syms, no startup tables | 2–5x (Windows: kernel32 only) |
| M2 source | Parse plus copy | Single-pass parser, blob relocation | 3x+ |
| M3/M4 configure, agents | Forks everywhere; MSYS fork plus argument conversion | Spawn only for externals, eval cache, path conversion in one pass | Large on Windows |

---

## 8. Top risks

1. **Fiber plus async I/O correctness**: Win32 foreign handles, the XP overlapped quirks (`CancelIo` only works on the issuing thread), and FIFO opens that would block. Mitigation: a single-task fast path, vos deadlock detection, and stress tests.
2. **Every mutation must go through Sym cells and `writable()`.** One missed path leaks subshell state into the parent. Mitigation: a debug build that checksums `declare -p` across every synchronous subshell, plus isolation fuzzing.
3. **Read-ahead and pumps vs exotic children.** Programs that `fstat` stdin and expect the original pipe inode, or `tcgetpgrp`-sensitive tools, may notice.
4. **Printer and error-message fidelity** (`print_cmd`, arithmetic error tokens) is a large surface that depends on differential coverage.
5. **ERE submatch semantics and Unicode classes vs glibc.** Our own Unicode tables (generated from UCD) may disagree with glibc's locale data.
6. **POSIX job control for mixed fiber/external jobs**: stopping with Ctrl-Z, and terminal process-group handling.
7. **x86 address space**: many concurrent shell jobs at 16 MiB plus 1 MiB each.
8. **Observable process-model differences**: no subshell processes in `ps`; virtual `$BASHPID` and `$!` are meaningless to an external `kill`. A few bash tests assume real processes.

## Open questions for the user

1. Is a size-class free-list `Heap` acceptable as the one exception to pure bump arenas, limited to persistent mutable shell state?
2. Output coalescing across builtins (a 64 KiB buffer flushed at spawn, blocking or exit) would make B13 several times faster, but changes *when* write errors such as ENOSPC are reported. Should it be off by default, enabled only for pipes, or opt-in?
3. Pathname-expansion sort order: bash uses `strcoll`, and en_US.UTF-8 collation is glibc-specific (case and punctuation insensitive at the first level). Should we use byte order everywhere, or emulate glibc collation for UTF-8 locales?
4. Is the virtual pid scheme acceptable (odd numbers on Windows, above `pid_max` on Linux)?
5. Should deep recursion give a graceful "maximum nesting" error instead of bash's segfault? Should OOM in interactive mode abort the command instead of exiting?
6. Should `exec ext` at top level on POSIX keep using real `execve` (my recommendation), and is real fork banned entirely on POSIX?
7. Under Git Bash's mintty (msys pty pipes), should unabashed detect the pipe-pty and run the editor in raw VT mode over the pipes?

### Critical Files for Implementation

The repository is empty, so these are the files the design creates:
- C:/Users/wellons/src/unabashed/sym.cpp (Sym cells, `Version`/`Undo`, `writable`, rerooting)
- C:/Users/wellons/src/unabashed/fd.cpp (`Ofile`, `Fdtab`, `Vpipe`, read-ahead, capture, pumps)
- C:/Users/wellons/src/unabashed/sched.cpp and C:/Users/wellons/src/unabashed/exec.cpp (tasks, synchronous and concurrent procs, scratch discipline in the executor)
- C:/Users/wellons/src/unabashed/unabashed.cpp (Platform API table and module order) with C:/Users/wellons/src/unabashed/main_win32.cpp
- C:/Users/wellons/src/unabashed/vos.cpp (virtual OS for tests and fuzzing)

Style and semantics references:
- C:/Users/wellons/src/skeeto.github.com/_ssg/base.cpp
- C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3/hashlib.c (source of the assoc iteration order rules, used for semantics only)
