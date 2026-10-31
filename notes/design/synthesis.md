# unabashed: judged and merged architecture

## 0. Verdict in brief

| Question | Base taken from | Grafted from the others |
|---|---|---|
| 1. Execution model | **Simple**: a single-threaded core; shell code runs as virtual processes ("vprocs"); synchronous handles; Peek plus helper threads | **Perf**: no fiber unless two or more shell stages must run at once. **Faithful**: `&` and coproc become real processes (fork on POSIX, self-spawn plus image on Win32 and the vos); seeded "chaos" yields for testing; `RegisterWaitForSingleObject` for process exits |
| 2. Memory model | **Simple**: arenas only, generation-tagged copy-on-write (COW) tries, copying compaction | My own additions: watermark *pins* so concurrent fibers and compaction work together; a synchronous subshell uses the parent's store arena *by value*; one type-directed copier serves both compaction and the Win32 `&` image |
| 3. Core representations | **Simple**: word text with comsub bodies reprinted | **Faithful**: assoc arrays as bucket trie plus immutable chains; lazy-construct principle. **Perf**: postfix arithmetic code, Pike VM, UTF-8 position memo, native integers |
| 4. Platform API | **Faithful**: completion-style `Io` records, the most complete table | **Simple**: a `IO_TRY` fast path; the editor emits only VT and a portable VT-to-console interpreter handles the legacy console. **Perf**: the environment block is cached by generation inside the platform |
| 5. Testing | Union of all three | Chaos-seed invariance, image round trip, isolation fuzzer, GC-stress build |
| 6. Layout and milestones | Faithful's ordering | Compactor lands in M2 (Simple had it at M6, which is too late); the image path is deferred to M5 |

## 1. Claims checked against the bash source and the research

1. **Assoc hash.** bash 5.3 `hashlib.c` `hash_string` is **FNV-1**: multiply first (written as shifts), then `i ^= *s` over `const char *`. On x86 that XOR is sign-extended for bytes of 0x80 and above. Faithful is correct. Perf (and perf.md) say FNV-1a, which is wrong. Other details confirmed:
   - growth: ×4 when `nentries >= 2*nbuckets`, checked before inserting;
   - insertion at the chain head;
   - rehash walks the old buckets in order and head-inserts, which reverses the chains.

   `hash_shrink` is defined but is never called from `hashlib.c`. Because of the signed `char`, the order differs on ARM Linux, so the reference platform has to be x86-64.
2. **Here-doc threshold.** Faithful's "4096 rule" is wrong for Linux. `HEREDOC_PIPESIZE` defaults to `PIPESIZE`, which `builtins/psize.sh` measures at build time as the real pipe capacity (65536 on Linux). 4096 applies only on FreeBSD or as a fallback. The rule is therefore "≤ 64 KiB goes through a pipe, otherwise a temp file". Perf is right here.
3. **Word text of comsubs.** bash 5.2+ re-prints comsub bodies into the word text at parse time (`parse.y`, `print_comsub` around line 4632). So `declare -f` shows the canonical body, not the raw source. Simple is right; Faithful's "print from raw" is wrong for `$(...)` and `${ ...; }`. Faithful's idea of re-parsing at execution time when aliases have changed is still valid, but it must re-parse the *reprinted* text.
4. **Perf's overlapped named-pipe server ends** cannot be handed to a later child. A synchronous `ReadFile` on a `FILE_FLAG_OVERLAPPED` handle is invalid, which breaks `exec 3< <(f); cat <&3`. Rejected.
5. **Faithful's POSIX `socketpair` in place of pipes** makes children see `S_ISSOCK`, so `[ -p /dev/stdin ]` becomes false. Rejected for any pipe an external can see.
6. **Simple's waiting** uses `WaitForMultipleObjects` with 1–10 ms polling beyond 63 handles. `RegisterWaitForSingleObject` (XP) avoids the cap with no polling.
7. **lastpipe** applies only when job control is off (`execute_cmd.c:2735`). All three designs respect this.
8. **WSL Debian** ships bash 5.2 (tests.md). The oracle must be bash 5.3 built from the tarball, in Docker or WSL.

## 2. Scores per question

Scale 1–10. Columns: Fa = faithfulness, Pe = performance, Si = simplicity/testability, Win = Windows/XP viability, St = style fit.

### Q1. Execution model

| Design | Fa | Pe | Si | Win | St | Sum |
|---|---|---|---|---|---|---|
| Faithful | 9 | 7 | 4 | 7 | 7 | 34 |
| Perf | 6 | 9 | 5 | 5 | 7 | 32 |
| Simple | 6 | 7 | 8 | 7 | 8 | 36 |

- **Faithful.** Its three vehicles give real `$!` and jobs that outlive the shell. Costs: IOCP plumbing, `WriteQuotaAvailable`, and a serializer needed from day one.
- **Perf.** `&` jobs die with the shell or block its exit. Overlapped pipe ends cannot be inherited. Read-ahead pumps add risk in general.
- **Simple.** Clean and deterministic. Two weaknesses: `sh -c 'f &'` keeps the process alive, which blocks make and agents; and waits poll beyond 63 handles.

**Decision.** Simple's vproc core, with Perf's "fiber only if concurrent" rule. Faithful's real processes for `&` and coproc, but staged: fibers until M5, then fork or image.

### Q2. Memory model

| Design | Fa | Pe | Si | Win | St | Sum |
|---|---|---|---|---|---|---|
| Faithful (refcounts + size-class Heap) | 8 | 8 | 5 | 7 | 4 | 32 |
| Perf (Sym + undo log + reroot + Heap) | 7 | 10 | 3 | 7 | 4 | 31 |
| Simple (gen-COW tries + compaction) | 8 | 7 | 7 | 7 | 8 | 37 |

Simple has gaps: a pinned store grows without bound while a long-lived procsub lives; the synchronous-child store is unspecified; local values are placed in scratch. I fixed these with watermark pins, the by-value store and the copier.

### Q3. Core representations

| Design | Fa | Pe | Si | Win | St | Sum |
|---|---|---|---|---|---|---|
| Faithful | 8 (raw printing is wrong) | 8 | 6 | 8 | 7 | 37 |
| Perf | 6 (FNV-1a is wrong) | 10 | 6 | 8 | 7 | 37 |
| Simple | 8 | 7 | 8 | 8 | 8 | 39 |

Assoc order by sorting on (bucket, rank) at every iteration costs O(n log n) per iteration, so Faithful's structure is used instead.

### Q4. Platform API

| Design | Fa | Pe | Si | Win | St | Sum |
|---|---|---|---|---|---|---|
| Faithful | 9 | 8 | 6 | 8 | 7 | 38 |
| Perf | 7 | 8 | 6 | 5 | 7 | 33 |
| Simple | 7 | 7 | 8 | 8 | 8 | 38 |

Perf puts pump logic in the platform. Faithful's legacy-console operations double the editor's backends. Simple's VT-subset interpreter keeps one editor backend.

### Q5. Testing

| Design | Fa | Pe | Si | Win | St | Sum |
|---|---|---|---|---|---|---|
| Faithful | 9 | 8 | 8 | 8 | 8 | 41 |
| Perf | 8 | 9 | 7 | 8 | 8 | 40 |
| Simple | 8 | 7 | 9 | 8 | 8 | 40 |

The three are compatible, so the union is taken.

### Q6. Layout and milestones

| Design | Fa | Pe | Si | Win | St | Sum |
|---|---|---|---|---|---|---|
| Faithful | 8 | 7 | 7 | 8 | 8 | 38 |
| Perf | 7 | 9 | 7 | 7 | 8 | 38 |
| Simple | 7 | 6 | 8 | 7 | 8 | 36 |

Simple puts compaction at M6, after code that must respect its invariant has been written. That is too late.

---

## 3. Final architecture

### 3.1 Execution model

**Kinds of process.**

| Construct | Vehicle |
|---|---|
| `( list )` in the foreground, `$(...)`, backquotes, the last shell stage of a pipeline | **Synchronous vproc**: same C stack, new `Shell` roots, its own `Escape` |
| `${ cmd; }` and `${\| cmd; }` | Not a subshell. The current vproc runs it with fd 1 set to an `F_CAPTURE` File |
| All shell stages of a pipeline except the last one; `<(f)` and `>(f)` with a shell body | **Fiber vproc**, cooperatively scheduled |
| `list &`, `coproc`, the shell stages of a backgrounded pipeline | M3–M4: a fiber vproc. **M5 onward: a real process**, `os_fork` on POSIX and self-spawn plus image on Win32 and the vos |
| External programs, including `ext &`, `(ext)`, `$(ext)`, `< <(ext)` | Spawned directly by the shell. Never an intermediate shell process |

**Pipeline setup.**
1. Resolve each stage.
2. Create OS pipes on every edge that touches an external, and in-memory `Pipe`s on shell-to-shell edges.
3. Spawn the externals.
4. Start fibers for the first k−1 shell stages.
5. Run the last shell stage on the current stack: as a synchronous vproc, or directly in the parent under lastpipe.

So `ext | while read` and `f | ext` create no fiber at all. Word expansion for an external stage still happens inside an O(1) snapshot, so `${x:=y}` does not leak into the parent.

**Materialization.** Anything that crosses a process boundary becomes a real OS object first:
- An in-memory pipe becomes an OS pipe. Bytes already buffered become the File's read-ahead prefix, which a pump task drains first.
- A here-doc ≤ 64 KiB goes into an OS pipe filled by a pump task. A larger one goes into a delete-on-close temp file, so externals see the same seekability as under bash.
- A capture becomes an OS pipe plus a pump task that drains it into the capture `Buf`. Before any builtin appends to the `Buf`, whatever is available is drained first, so `$(echo a; ls; echo b)` stays in order. EOF waits for every writer, so `x=$(sleep 9 &)` waits 9 s, as in bash.

**Scheduler.** It is core-owned, single-threaded and FIFO. A task is a fiber vproc or a pump, a small state machine that copies between a core buffer and an OS handle.

*Blocking points:*
- an in-memory pipe that is full or empty;
- an `Io` waiting to complete;
- a child exit;
- a timer: `sleep`, `read -t`, `wait -n`, the editor's `keyseq-timeout`.

*Waiting:* when nothing is runnable, the scheduler calls `os_wait(deadline)`.

*Safe points:* simple commands, loop back-edges, function entry, and every 4 KiB inside heavy builtins. Each costs a `budget--` and a branch. When the budget reaches 0 the task yields if another is ready and polls `os_wait(0)`.

*Fast path:* with exactly one task and no pending `Io`, I/O is a direct `IO_BLOCK` call.

*Test mode:*
- `Kernel.chaos` injects seeded yields at safe points. Output must be identical for any seed, which serves as the isolation oracle.
- The vos reports `OS_DEADLOCK` instead of hanging.

**Win32 I/O.** All shell-held handles are synchronous and non-inheritable, so any of them can later be duplicated to a child.
- *Reads* use `PeekNamedPipe` first, then a `ReadFile` of exactly the needed byte count. This gives exact consumption at full speed, even on foreign pipes and msys pty pipes.
- *A read on an empty pipe:*
  - when the shell has a single waiter on Vista+, block on the main thread; the Ctrl-C handler cancels it with `CancelSynchronousIo` (resolved at run time);
  - otherwise, and always on XP, a pooled helper thread (`CreateThread` + `CRITICAL_SECTION` + event) issues a 1-byte `ReadFile`.
- *A pending helper read whose reader goes away* (interrupted `read`, timed-out `read -t`): any byte it later delivers goes into the File's read-ahead. If that File is then handed to an external with a non-empty read-ahead, a pump task bridges the gap.
- *Writes* go direct when the shell is the only task. Otherwise they go through a helper thread.
- *Process exits* use `RegisterWaitForSingleObject`, so there is no 64-handle cap.
- *One wake mechanism for everything:* helper threads, wait callbacks and the console control handler push onto a `CRITICAL_SECTION` queue and `SetEvent`. The main thread does `WaitForMultipleObjects([wake, console-if-pending], deadline)`.
- Every API used here exists on XP.

**POSIX I/O.**
- `poll` plus a self-pipe for SIGCHLD and SIGINT, then `waitpid(WNOHANG)`.
- Foreign pipes are read one byte at a time after `poll` says readable, as bash does. M8 adds read-ahead with a hand-off pump to win benchmark B6 on Linux.
- `O_NONBLOCK` is never set on a shared file description.

**Spawning externals.**
- *Win32:*
  - the resolved path always goes in `lpApplicationName`;
  - the core builds the command line, quoting for MSVCRT, Cygwin or `cmd /d /s /c` with BatBadBut escaping, depending on the target's kind;
  - `CREATE_UNICODE_ENVIRONMENT|CREATE_SUSPENDED`, with inheritable duplicates of fds 0–2;
  - an `lpReserved2` blob only when fds ≥ 3 are explicitly redirected and the child is an MSVCRT program, with the slot width taken from the child's PE machine field;
  - `lpCurrentDirectory` comes from the vproc's virtual cwd. The process's own cwd never changes, so `cd d; rm -rf ../d` works;
  - `CREATE_NEW_PROCESS_GROUP` for background jobs.
- *POSIX:* fork or vfork plus exec inside `main_posix`, which applies the fd map, chdir, umask, signal defaults and ignores, and the process group.

**`&` and coproc as real processes (M5).**
- *POSIX:* `os_fork`. In-memory Files are materialized first. The child drops the other tasks and continues at the async node.
- *Win32 and the vos:* self-spawn with `--unabashed-image=<inherited handle>` and stream the image (section 3.2).
- *Signals to our own shell children on Win32* go through a named event `Local\unabashed-<pid>-sig`, so `trap ... TERM` works in a background subshell.
- *Tail optimization:* a body whose last action is an external (`(cd x && server) &`) still gets a real process. Its pid is `$!`.

**Signals, traps, `exit`.**
- *Real signals* (POSIX handlers; on Win32 Ctrl-C → INT, Break → QUIT, CLOSE → HUP, buffer resize → WINCH) only set bits and wake `os_wait`.
- *SIGINT* goes to every vproc of the foreground job. Bash's cooperative-exit rule applies: while a foreground external runs, SIGINT is held and delivered only if the child died of it. On Win32 that means exit code 0xC000013A.
- *Subshell traps:* caught traps reset to default, ignored ones stay ignored. The parent's trap text is kept so that `trap` with no arguments inside `$( )` prints it.
- *Writing to a pipe with no readers* (in-memory, `EPIPE`, `ERROR_NO_DATA`) raises a virtual SIGPIPE: status 141, or the trap.
- *`break`, `continue`, `return`* propagate as `Proc.flow` codes, with no `setjmp` per call or loop.
- *`exit`, fatal errors, killing signals and OOM* `__builtin_longjmp` to the vproc's base `Escape`. On arm64 macOS the platform `setjmp` is used. A jump never crosses fibers. The catcher truncates the explicit stacks (`undo`, `frames`) to the depths it recorded.
- *Pids:* `$$` is the real pid. `$BASHPID` for an in-process vproc is a virtual pid: odd numbers on Windows, values above `pid_max` on Linux, deterministic in the vos. `kill` and `wait` consult the vproc table first.
- *`exec ext`:* on POSIX, `execve` when no other task is alive. Otherwise, and always on Win32, spawn, wait with Ctrl-C ignored, then exit with the child's status.

### 3.2 Memory model

```cpp
// Memory: one reservation per Mem, committed lazily, with an OOM escape.
struct Mem {
    Os     *os;
    byte   *base;       // reservation start
    byte   *commit;     // committed end
    byte   *limit;      // reservation end
    Escape *oom;        // owning vproc's catch point
    Pin    *pins;       // watermarks shared with live concurrent children
    Mem    *next;       // Kernel pool free list
};

struct Pin {
    Pin  *next;
    byte *mark;         // store tip at fork time: everything below is shared
};

struct Arena {
    byte *beg = {};
    byte *end = {};     // committed end, may be stale in by-value copies
    Mem  *mem = {};
};
// alloc<T> is base.cpp's alloc. Only its slow path changes: it calls
// arenagrow(a, need), which commits more or longjmps to mem->oom.
```

**Where things live.**

| Region | Contents | Reclamation |
|---|---|---|
| `Kernel.perm` | builtin table, mount table, option names, bounded caches | process lifetime |
| Kernel record free lists | `File`, `Pipe`, `Job`, `Proc` records for fibers, fiber stacks | fixed-size free lists |
| Kernel history arena | history ring | semispace copy when full |
| Per-vproc `store` | `Vars` trie, `Value`s, arrays, assoc arrays, functions, aliases, command hash, positional parameters, dirstack, trap strings | in-place when the gen matches; compaction at safe points |
| Per-vproc `code` | function bodies, deep-copied at definition time | compacted only at main top level with no live fibers |
| Per-vproc `scratch` | the AST of the top-level command, expansions, argv, env overrides, glob results, call frames, redirection undo | by value per command and per loop iteration (`Arena it = scratch;`) |
| Caller's scratch tip | `$( )` capture `Buf` | the child gets a pooled scratch, so the capture grows in place |

**Generation copy-on-write (no refcounts, no undo log).** Every store node carries `gen`.
- *Write:* walk from the root. A node whose `gen == p->gen` is mutated in place. Otherwise it is copied into `p->store`.
- *Synchronous subshell:*
  - `c.store = p->store` (by value), `c.gen = ++k->gen`, `c.sh = p->sh` (roots only), and `c.scratch` from the pool;
  - on return, everything the child allocated vanishes, because the parent's `Arena` was never advanced;
  - if live descendant fibers registered pins in that region, the parent adopts the child's tip instead (`p->store.beg = c.store.beg`).
- *Concurrent child (fiber):* gets a pooled store; the parent pushes `Pin{p->store.beg}`, and both take new gens. The parent then copies-on-write, and the child sees only nodes below the pin.
- *Compaction region* of vproc P is [max(P's start mark, the highest live pin), P.store.beg). The roots are `P->sh`, `P->frames` and `P->undo`.
  - It is triggered at a safe point when the region holds more than 2× the bytes that were live after the previous compaction, and `p->nocompact == 0`. `mapfile -C` and other callbacks set `nocompact`.
  - The copier is type-directed and recursive, with a forwarding map (an MSI table in scratch). It copies into a to-space, assigning final addresses relative to the region start, then copies the block back.
  - Compaction never touches pinned memory or other vprocs, so it needs no global stop.
- **Invariant, enforced from M2:** no C local holds a store pointer across a safe point or across anything that runs shell code.
  - The GC-stress build compacts at every safe point and fills the from-space with `0x55`.
- *Image* (`&` on Win32 and the vos): the same copier writes `Shell` + `code` + the AST subtree + the fd map into a buffer and records the offset of every pointer it writes. The child copies the buffer into its store reservation and rebases each recorded pointer. This is busybox's bitmap scheme, but generated by the compactor's own walk, so the compaction tests exercise the image code as well.

**Long loops.**

| Statement | Effect |
|---|---|
| `i=$((i+1))` | writes `Value.num` in place |
| `s+=x` | appends in place up to `cap`, then doubles; the old buffer is reclaimed at the next compaction |
| `x=$(cmd)` | capture lands in the iteration's scratch; the old value buffer is reused when the new one fits |
| `local v` | pushes a `Value` in the store; popped on return; the garbage is compacted |
| A top-level `while` running for hours | AST in top-level scratch; each iteration swept; store flat |

**OOM.** The vproc prints bash's xmalloc message and dies with status 2, freeing its pooled `Mem`. The main vproc exits 2, or returns to the prompt when interactive. Pool exhaustion prints `fork: retry: Resource temporarily unavailable`, then `fork: Resource temporarily unavailable`.

**Reservation budgets.**

| | 32-bit XP, LAA | 64-bit |
|---|---|---|
| Main store | 256 MiB, plus an equal to-space committed only during compaction | 16 GiB |
| Main scratch | 128 MiB | 4 GiB |
| Code | 64 MiB | 1 GiB |
| Each pooled vproc | 16 MiB store + 8 MiB scratch, about 30 concurrent | 1 GiB each |
| Fiber stacks | 512 KiB | 8 MiB |
| Main stack | 16 MiB | 64 MiB |

The test and fuzz builds use small, malloc'd arenas so that the OOM paths are reachable.

### 3.3 Core representations

```cpp
struct Name {             // literal names get their hash at parse time
    Str s;
    u64 hash;
};

struct Vars {             // persistent 4-ary hash trie, gen-COW
    Vars  *child[4];
    Name   key;           // key bytes live in the store
    Value *top;           // innermost binding (shallow binding)
    u32    gen;
};

struct Value {
    Value *below;         // shadowed binding: local pushes, return pops
    i32    depth;         // 0 global, n function depth, DEPTH_TEMPENV
    u32    attrs;         // V_EXPORT V_READONLY V_INTEGER V_NAMEREF V_LOWER ...
    u32    gen;
    i32    kind;          // K_UNSET K_STR K_INT K_INDEXED K_ASSOC K_DYN
    Str    str;           // data==0: unset
    iz     cap;           // in-place append capacity
    i64    num;           // K_INT, or cached when numok
    b32    numok;
    Array *arr;           // 16-ary persistent radix trie, count/max cached
    Assoc *map;
    i32    dyn;           // RANDOM SECONDS LINENO FUNCNAME BASHPID ...
};

struct Assoc {            // bash hashlib iteration order, persistent
    Btrie *buckets;       // radix trie over bucket index, gen-COW
    i32    nbuckets;      // 1024; x4 when nentries >= 2*nbuckets, before insert
    i32    nentries;
    u32    gen;
};

struct Chain {            // immutable cons list: insert at head is O(1)
    Chain *next;
    Str    key;
    Str    val;
    u32    khash;         // FNV-1 over signed char (x86-64 reference)
};

struct Shell {            // a subshell copies these roots, O(1)
    Vars    *vars;
    Funcs   *funcs;
    Aliases *aliases;
    Hashed  *hashed;      // bash bucket order too (`hash` listing)
    Args    *args;        // positional parameters, immutable
    Cwd     *cwd;         // logical, physical, per-drive (=C:) cwds
    Dirs    *dirs;
    Trap     traps[NTRAP];
    u64      opts[2];     // set -o and shopt bits; $- derived
    i32      umask;
    i32      status;
    i32      subshell;
    i32      pathmode;    // PM_MSYS, PM_NATIVE
    b32      utf8;
    u32      exportgen;   // invalidates the env block cache
    u32      rand;
    i32      optind;
    i32      optchar;
    Str      underscore;
};

struct Proc {
    Proc   *parent;
    Proc   *next;         // run queue or wait list
    Fiber  *fiber;        // 0: synchronous on the parent's stack
    Shell   sh;
    Fdtab   fds;
    Arena   store;
    Arena   code;
    Arena   scratch;
    Frame  *frames;       // call stack in scratch; compaction roots
    Undo   *undo;         // redirection/tempenv undo; truncated by Escape
    Escape *esc;
    Node   *cmd;          // current simple command, printed lazily
    Bind    cache[256];   // Name* -> Value**, valid while ver==varsver
    u32     gen;
    u32     varsver;      // bumped on structural trie change or compaction
    i32     vpid;
    i32     state;
    i32     status;
    i32     flow;         // FLOW_BREAK, FLOW_CONTINUE, FLOW_RETURN
    i32     flowcount;
    u64     sigpend;
    i32     budget;
    i32     nocompact;
};

enum : i32 { F_OS, F_PIPEEND, F_MEM, F_CAPTURE, F_NULL };

struct File {             // open file description, refcounted Kernel record
    i32     kind;
    i32     refs;
    i32     flags;        // FL_READ FL_WRITE FL_APPEND
    i32     ftype;        // cached os_ftype
    Handle  h;
    Pipe   *pipe;
    Str     mem;          // F_MEM here-doc body
    iz      pos;
    Buf    *cap;          // F_CAPTURE
    Str     ahead;        // bytes taken from the OS, not yet delivered
    Io     *inflight;     // pending helper-thread read
};

struct Fdtab {
    File  *low[16];
    FdMap *high;          // sparse: {var}> from 10 up, procsub from 63 down
};

struct Pipe {             // in-memory, between shell stages
    u8    *ring;          // 65536, the Linux capacity
    iz     head;
    iz     len;
    i32    readers;
    i32    writers;
    Proc  *rwait;
    Proc  *wwait;
    File  *os;            // set once materialized
};
```

**Lookup.** One trie descent, comparing the hash before the bytes. A hot loop hits `Proc.cache` (1–2 loads), keyed by the address of the AST's `Name`. A write in place does not bump `varsver`. The cost does not depend on call depth.

`FUNCNAME`, `BASH_SOURCE` and `BASH_LINENO` are materialized from `frames` only when read, which removes bash's O(d²).

**Parser.**
- Hand-written, incremental: parse one complete command, run it, parse the next.
- The input stack does alias expansion: trailing-blank rule, recursion guard.
- The parser reads extglob, posix and `expand_aliases` live as it goes.
- Function bodies are copied into `code` at definition time.

**`Word`.**
- `text` holds the spelling with comsub bodies reprinted canonically, which is what `declare -f` prints.
- `parts` holds the pre-parsed expansion tree.
- `lit` holds the final value of constant words, which expand with zero copies.
- Comsub `Part`s keep their parsed `Node *` and the alias generation. When aliases are on and the generation has changed, the body is re-parsed from the reprinted text.

**Lazy-construct principle.** Anything bash parses at expansion time is pre-parsed speculatively. On failure the raw text is kept, and the error is raised at run time with bash's message and timing. This covers `$x`-dependent arithmetic, `${...}` operands and here-doc bodies.

**Printing.**
- `print.cpp` re-implements bash's layout from observed behaviour only, locked down by recorded golden cases.
- `$BASH_COMMAND` and job text are printed only on demand.
- One quoting engine serves xtrace, `%q`, `@Q` and `declare -p`, with a separate mode for each (their rules differ).

**Arithmetic.** Postfix code, `Ainst{op, src offset, operand}`.
- Compiled at parse time. A syntax error compiles to an `A_ERR` instruction that raises bash's exact message when evaluated.
- Arithmetic is u64 wraparound, `INTMAX_MIN/-1` follows bash's handling, `noeval` covers short-circuit operators, and recursion is limited to 1024.
- A body containing `$` is expanded to text first, then looked up in a "last text, last code" cache on the node.
- Plain decimal values skip the parser.

**Patterns.**
- Compiled once, with literal prefix and suffix.
- Plain globs use a two-pointer star matcher. Extglob uses memoized backtracking over (op, position) in scratch.
- `${s//p/r}` uses a single pass that reports every match end, giving leftmost-longest without bash's O(n²) end probing.
- Work is on UTF-8 bytes; characters are decoded only for `?`, brackets and case folding.
- Globbing reads each directory listing once, taking file types from `d_type` or the find data, with no per-entry stat.

**ERE for `=~`.**
- A Pike VM finds the leftmost-longest span. A memoized backtracker then assigns POSIX submatches inside that span.
- glibc extensions are included: `\w \s \b \< \>` and back-references.
- glibc is the oracle, through differential fuzzing.

**Other representations.**
- *Environment block:* cached by `exportgen`. The platform caches its UTF-16 (sorted) or `char **` form by the same generation.
- *printf floats:* a soft x87 80-bit `long double`, with correctly rounded parsing and exact output through an arena bignum.
- *Locale:* switched at run time between a UTF-8 mode and a C byte mode, with generated Unicode tables.
- *Messages:* glibc strerror text and Linux signal numbers.
- *Indexing:* a UTF-8 position memo makes `${s:i:1}` loops O(1) per step.

### 3.4 Platform API

```cpp
struct Os;
using Handle = i64;       // fd, HANDLE, or vos object index; -1 none

struct OsInfo {           // filled by the platform before sh_main
    b32     windows;      // drive letters, case folding, no kernel shebang
    Str     exe;          // own absolute native path
    Str     cwd;
    Str     cmdline;      // Win32 raw line (msys mode may re-split it)
    Str    *args;
    iz      nargs;
    Str    *env;          // WTF-8 "NAME=value"
    iz      nenv;
    i64     pid;
    i64     ppid;
    i64     uid, euid, gid, egid;
    Str     host;
    Str     user;
    Str     home;
    Handle  std[3];
    Str     image;        // state image from a self-spawning parent
    MemCfg  mem;          // reservation sizes: 32-bit, 64-bit, test
};
static i32 sh_main(Os *, OsInfo *);
```

**Memory**

| Function | Purpose |
|---|---|
| `byte *os_reserve(Os *, iz)` | address space; 0 on failure |
| `b32 os_commit(Os *, byte *, iz)` | |
| `void os_decommit(Os *, byte *, iz)` | idle prompt, after compaction |
| `[[noreturn]] void os_oom()` | last resort only |

**Fibers**

| Function | Purpose |
|---|---|
| `Fiber *os_fiber(Os *, iz stack, void (*)(Os *, void *), void *)` | Win32: CreateFiberEx (XP); POSIX: asm |
| `void os_switch(Os *, Fiber *)` | |
| `void os_fiberfree(Os *, Fiber *)` | |
| `iz os_stackleft(Os *)` | recursion guard |

**Files** (paths are canonical native absolute: `C:/a`, `//srv/s/x`, `//./NUL`, `/a`)

| Function | Purpose |
|---|---|
| `Opened os_open(Os *, Arena, Str path, i32 flags, i32 mode)` | `{Handle h; Err err;}` |
| `Opened os_tempfile(Os *, Arena)` | delete-on-close |
| `IoResult os_read(Os *, Handle, u8 *, iz, i32 mode)` | `IO_BLOCK` / `IO_TRY` (`E_AGAIN`) |
| `IoResult os_write(Os *, Handle, u8 *, iz, i32 mode)` | console: UTF-8 → UTF-16 with carry |
| `IoResult os_peek(Os *, Handle, u8 *, iz)` | `E_NOSYS` where unsupported |
| `i64 os_seek(Os *, Handle, i64, i32 whence)` | |
| `Err os_truncate(Os *, Handle, i64)` | |
| `void os_close(Os *, Handle)` | |
| `i32 os_ftype(Os *, Handle)` | `FT_FILE` `FT_DIR` `FT_PIPE` `FT_CONSOLE` `FT_PTYPIPE` `FT_NULL` `FT_CHAR` |
| `OsPipe os_pipe(Os *, Arena *, b32 named)` | named: `\\.\pipe\...` for a procsub given to a native child |
| `Stat os_stat(Os *, Arena, Str, b32 follow)` | `os_fstat(Os *, Handle)` |
| `Dirent *os_list(Os *, Arena *, Str dir)` | raw order, with type and attributes |
| `Err os_access(Os *, Arena, Str, i32 rwx)` | |
| `Str os_readlink(Os *, Arena *, Str)` | `os_realpath(Os *, Arena *, Str)` |
| `Str os_userhome(Os *, Arena *, Str user)` | |

**Processes**

| Function | Purpose |
|---|---|
| `Spawned os_spawn(Os *, Arena, Spawn *)` | `{Pid pid; Handle proc; Err err;}` |
| `Pid os_fork(Os *)` | POSIX only; −1 if unsupported |
| `Err os_exec(Os *, Arena, Spawn *)` | POSIX execve; returns only on failure |
| `b32 os_kill(Os *, Pid, i32 sig, b32 group)` | Win32: named event for our own shells, TerminateProcess or CTRL_BREAK otherwise |
| `b32 os_stopcont(Os *, Pid, b32 stop)` | |
| `[[noreturn]] void os_exit(Os *, i32)` | |

`Spawn` fields:
- `Str path`, `Str cmdline` (built by the core for Win32), `Str *argv`, `i32 argc`;
- `Str env` with `u64 envgen` as the platform's cache key;
- `Str cwd`, `SpawnFd *fds`, `iz nfds`;
- `u64 sigign`, `i32 umask`;
- `i32 flags`: `SP_NEWGROUP`, `SP_FOREGROUND`, `SP_SELF`;
- `Pid pgid`, `Str image`.

**Asynchronous completion and signals**

| Function | Purpose |
|---|---|
| `void os_submit(Os *, Io *)` | `IO_READ`, `IO_WRITE`, `IO_PROC`, `IO_TTY` |
| `Io *os_wait(Os *, i64 deadline)` | 0 on timeout or new signal; vos: `OS_DEADLOCK` |
| `b32 os_cancel(Os *, Io *)` | 0 if it may still complete |
| `u64 os_sigpending(Os *)` | fetch and clear, Linux numbering |
| `void os_sigmode(Os *, i32 sig, i32 mode)` | `trap ''` on Win32 maps to `SetConsoleCtrlHandler(0,1)` |

**Terminal**

| Function | Purpose |
|---|---|
| `TtyInfo os_tty(Os *, Handle)` | `{kind: VT, LEGACY or PTYPIPE; cols; rows; xenl}` |
| `b32 os_ttymode(Os *, Handle in, Handle out, i32 mode)` | `TM_RAW` / `TM_RESTORE` |

Input always arrives as xterm bytes; the legacy console layer converts `KEY_EVENT` records. Output is always the core's VT subset. On a legacy console, `vtcon.cpp` interprets that subset (CUU/CUD/CUF/CUB/CHA, EL, ED, SGR) into console calls, with `xenl=0`.

**Time and entropy**

| Function | Purpose |
|---|---|
| `i64 os_now(Os *)` | realtime, ns |
| `i64 os_clock(Os *)` | monotonic, ns |
| `Times os_times(Os *)` | Win32: job-object accounting for children |
| `TzInfo os_localtz(Os *, Arena *, i64 t)` | only when TZ is unset |
| `void os_entropy(Os *, u8 *, iz)` | RtlGenRandom on XP |

**Placement of the remaining logic.**
- Every API newer than XP is resolved through `GetProcAddress` into `struct Os`, with a fallback.
- Core (pure code) owns:
  - `paths.cpp`: modes, mount table (`<root>/etc/fstab` via `os_open`), the `C:\x`, `C:/x` and `/c/x` forms, the meaning of a leading `/`, per-drive cwds, `/dev/*`, reserved names, PATH separators, environment translation, MSYS2 argument conversion with `MSYS2_ARG_CONV_EXCL`, `cygpath`, shebang mapping, PE sniffing (cached by path and mtime), busybox-stub bypass.
  - `wide.cpp`: UTF-16, the MSVCRT, Cygwin and cmd encoders, our own command-line splitters.
  - `vtcon.cpp`.
- `wide.cpp` and `vtcon.cpp` are also included by `main_test`, so the vos's "Windows personality" (`OsInfo.windows=1`) tests all of it on Linux.

### 3.5 Source layout

All files are under `C:/Users/wellons/src/unabashed/`.

**Core**
- `base.cpp`: ssg's base, with `Arena.mem` and the `arenagrow` hook.
- `unabashed.cpp`: Platform API table, include order, `sh_main`.
- Modules, in include order: `mem.cpp` (pool, pins, compactor and image copier), `unicode.cpp` (generated tables), `err.cpp`, `num.cpp` (integers, bignum, x87), `tz.cpp`, `wide.cpp`, `paths.cpp`, `lex.cpp`, `parse.cpp`, `print.cpp`, `vars.cpp`, `array.cpp`, `arith.cpp`, `pattern.cpp`, `regex.cpp`, `expand.cpp`, `io.cpp` (File, Fdtab, Pipe, pumps, redirections, here-docs), `sched.cpp`, `proc.cpp` (vprocs, pipelines, procsub, jobs, image), `exec.cpp`, `spawn.cpp`, `startup.cpp`.
- Builtins: `b_io.cpp`, `b_vars.cpp`, `b_flow.cpp`, `b_dir.cpp`, `b_test.cpp`, `b_job.cpp`, `b_misc.cpp`.
- Interactive: `history.cpp`, `histexp.cpp`, `edit.cpp`, `complete.cpp`, `inputrc.cpp`, `vtcon.cpp`.

**Platform layers**
- `main_win32.cpp`: CRT-free; kernel32 plus advapi32 and ntdll resolved at run time.
- `main_posix.cpp`.
- `main_test.cpp` with `vos.cpp` (VFS, virtual processes on fibers, clock, Windows personality), `fakes.cpp`, `vterm.cpp` and `test/*_test.cpp`.
- `main_fuzz.cpp`.
- `oracle.cpp`: `main_test` plus the runner for bash's suite.

**Other**
- `t/*.spec`, `bench/`, `tools/` (record, difftest, difffuzz, ptyrun, bench, `Dockerfile.bash53`), and an optional `CMakeLists.txt`.
- `bash-5.3.tar.gz` is gitignored.

### 3.6 Testing architecture

1. **Unit tests** (ssg-style `expect`, table-driven, arenas filled with `0x55`):
   - lexer, parser, and print round trips;
   - arithmetic, patterns (against a naive reference), ERE (against a backtracking reference);
   - the bignum float formatter;
   - paths in both modes and both personalities;
   - `wide.cpp` quoting round trips through a simulated CRT parser;
   - `vtcon`;
   - the gen-COW model: random write, fork and compact sequences checked against a naive deep-copy model;
   - assoc order.
2. **Virtual-OS specs** (`t/*.spec`: script, stdout, stderr, status, personality):
   - The vos provides an in-memory VFS (`/tmp`, `/dev`, a synthetic `/etc/passwd`, FIFOs, symlinks, illegal-on-Windows names), deterministic pids, and a virtual clock that `sleep` advances.
   - Fake programs: recho, zecho, printenv, xcase, and about 30 coreutils subsets, per tests.md §6.
   - `THIS_SH`, `bash` and `sh` run a fresh core instance.
   - Real filesystem calls and `CreateProcess` trap.
   - Expected output comes from `tools/record`, which runs real bash 5.3 in Docker on our own scripts and commits only the output.
3. **bash's suite as an external oracle** (`oracle` binary, `BASH_TESTS=` path, read-only):
   - It reimplements the `run-*` contract: `2>&1`, `grep -v '^expect'`, `cat -v`, the version variables.
   - Categories a, b and the faked part of c run on the vos. Categories c, d and e run on `main_posix` in a disposable non-root container with `--network none` and a tmpfs workdir.
   - Only a manifest is committed: test name, status, reason.
4. **Differential testing.**
   - Our snippet corpus, plus each `.tests` and `.sub` file run in place by path.
   - Compare stdout, stderr, status, the merged stream and a final `declare -p`, normalized for TMPDIR, pids and time.
   - Git Bash 5.3.15 on the host only for msys path, argument and environment snippets, in a scratch directory.
5. **Fuzzing** (AFL++, vos only, persistent mode, small arenas, GC-stress on, fuel limits):
   - `parse`: parse, print, reparse, print must be a fixed point.
   - `run`: crashes, deadlocks, OOM paths.
   - `chaos`: output must be invariant under the yield seed.
   - `isolate`: `( S ); declare -p` must equal `declare -p` alone.
   - `image`: round trip of `declare -p; declare -f; alias; trap -p; set -o; shopt; dirs` must be identical.
   - `arith`, `glob`, `regex`, `printf`: against in-tree references.
   - Offline replay of the corpus against bash in a container.
6. **Line editor.**
   - Headless: key bytes in, golden screen out, on the vterm with `xenl` on and off.
   - `ptyrun` gives a "feel" differential against bash plus readline, with both screens rendered through vterm.
   - End to end with winctl: Win11 conhost, Windows Terminal, and the XP laptop with the i686 build. A hidden `--dump-console` helper avoids reading pixels.
7. **Benchmarks.**
   - perf.md B1–B13 and M1–M4, run with hyperfine against bash 5.3, Git Bash, MSYS2 bash and busybox sh.
   - Checksums must match before any timing is trusted.
   - Track instructions, process-creation counts and peak RSS.
   - CI gates on vos instruction counts, failing on regressions over 5%.

### 3.7 Milestones

Each milestone ends with something runnable and with explicit exit criteria.

| | Deliverable | Exit criteria |
|---|---|---|
| **M0** Foundation | base and Mem; API table; the 4 layers plus the oracle stub; fibers on every layer; vos skeleton; Docker bash 5.3 image and `tools/record`; `-c 'echo hi; exit 3'` | x64 and i686 cross-builds; no CRT or toolchain DLL imports; PE subsystem 5.01; runs on the XP laptop; fiber ping-pong passes on all layers |
| **M1** Parser and printer | full grammar, here-docs, aliases, comsub and funsub parse plus reprint, bash syntax-error text, `-n`, a hidden dump flag that prints functions canonically | parse fuzzer reaches a fixed point in 24 h with no crashes; recorded `declare -f` goldens and the error corpus match |
| **M2** Sequential interpreter | store, gen-COW, **compactor and GC-stress build**, expansions except comsub, arithmetic, compound commands, functions and locals, virtual fd redirections, synchronous externals on all layers, P0 builtins, `set -eux`, LINENO, message parity | non-subshell run-minimal items pass on the vos; 1e7-iteration loops keep RSS flat; B1, B2, B9, B10 beat bash |
| **M3** Virtual processes | synchronous subshells, comsub; scheduler, fibers, Pipes, pumps, materialization, here-docs, procsub; PIPESTATUS, pipefail, lastpipe; traps and subshell trap rules, SIGPIPE, Ctrl-C on POSIX and Win32; helper-thread I/O and the completion queue; exact reads; interim fiber `&`, wait and coproc | category a green on the vos; chaos and isolate fuzzers clean; B7, B8, B12 ≥10x bash; `f \| while read` with Ctrl-C correct in conhost |
| **M4** Data and matching | arrays, bash-order assoc, namerefs, `[[ ]]`, ERE, glob, extglob, globstar, GLOBSORT, printf floats, read, mapfile, getopts, `declare -p`, shopt and set lists | category b green; regex and glob differential fuzzing clean; B3–B6, B11, B13 beat bash |
| **M5** Windows and real jobs | path modes, mount table, argument and environment conversion, cygpath, `pwd -W`, exe kinds, shebangs, `.bat`, PATH cache, stub bypass, persona from argv[0], startup files; image copier, self-spawn `&` and coproc, `os_fork` on POSIX, signal channel, real-pid kill, wait and jobs | category c in the container; Git Bash differential corpus green; trial w64devkit `sh.exe` runs autoconf builds and make recipes; M4 agent benchmark ≥5x Git Bash; image fuzzer clean |
| **M6** Editor milestone 1 | termios, VT and legacy backends; emacs bindings, kill ring, undo; history and `!` expansion; filename and command completion; basic inputrc; prompts; POSIX job control; Windows partial | vterm goldens; ptyrun differentials; winctl end to end on Win11 and XP |
| **M7** Fidelity sweep | POSIX mode, invocation, DEBUG/ERR/RETURN traps, extdebug, dirstack, select, remaining builtins, own help text | category d and e pass list frozen in the manifest |
| **M8** Performance and editor milestone 2 | POSIX pipe read-ahead with hand-off pump, output coalescing to regular files, 32-bit tuning; vi mode, `bind`, programmable completion | every benchmark in the table beats bash 5.3 on Linux and Git Bash and busybox on Windows; the bash-completion sourcing benchmark (M2 in perf.md) beats bash |

### 3.8 Areas that do not beat bash "automatically", and their remedies

- **CreateProcess cost** is the same as bash's.
  - Never use an intermediate shell or busybox stub; spawn `busybox.exe` directly with argv[0] set.
  - Cache PATH and PATHEXT probes and the environment block.
  - Run `$(ext)` and `(ext)` as direct spawns.
- **Foreign POSIX pipe reads** are parity until the M8 pump.
- **Builtin output** costs one write per builtin, the same as bash. M8 coalesces writes to regular files only.
- **Compaction pauses** are amortized and limited to one vproc's top region.
- **`&` on Linux** costs one fork, the same as bash. On Win32, self-spawn plus image is still well under an MSYS fork.

## 4. Unresolved decisions for the user (each with a recommendation)

1. **`&` and coproc.** Real processes (fork, or self-spawn plus image) from M5, or fibers only (cheaper, but they die with the shell or block its exit)? *Recommend real processes, staged after the fiber interim.*
2. **Long-lived store.** Is copying compaction at safe points acceptable as the only exception to "pure bump arenas"? The alternative is a size-class free-list Heap with refcounts. *Recommend compaction: it is closest to the arena model, and it is the same copier the image needs.*
3. **UTF-8 collation.** Codepoint order (C.UTF-8) for glob sorting and `[[ < ]]`, or emulate glibc en_US? *Recommend codepoint order, documented.*
4. **Bit-exact reference platform.** x86-64 glibc: signed-char FNV-1 assoc order, 80-bit printf, Linux signal numbers and strerror text, on every platform? *Recommend yes.*
5. **The sh/ash persona.** bash POSIX-mode semantics with bash-style messages (prefix from `$0`) plus busybox-w32 environment conventions (uppercase names, slash-converted values, `;` PATH), or ash message formats too? *Recommend bash messages with busybox environment conventions.*
6. **Msys mode command line.** Should it split its own command line with Cygwin rules (quote handling, unquoted-wildcard globbing, `@file`)? *Recommend yes in msys mode, never in native mode.*
7. **`OSTYPE` and `MACHTYPE`.** *Recommend msys mode: `msys` and `x86_64-pc-msys`, as Git Bash. Native mode: `win32`.*
8. **Output coalescing.** *Recommend coalescing only to regular files, flushed before spawn, block or exit.*
9. **fds ≥ 3 to native children through `lpReserved2`.** *Recommend yes, but only for explicitly redirected fds to MSVCRT children, with the slot width taken from the child's PE machine field.*
10. **Deep recursion.** *Recommend emulating bash's outcome: the vproc dies with status 139 and a `Segmentation fault` report. A clean "maximum nesting" error would be a divergence.*
11. **Windows job control.** *Recommend jobs, wait, kill and fg of running jobs only. Ctrl-Z through NtSuspendProcess deferred.*
12. **mintty pipe-pty.** *Recommend detecting it by pipe name and running the editor in raw VT mode over the pipes, at low priority since newer msys runtimes give a ConPTY console.*

### Critical files for implementation

The repository is empty, so these are the first files to create:
- `C:/Users/wellons/src/unabashed/unabashed.cpp`: Platform API table, include order, `sh_main`.
- `C:/Users/wellons/src/unabashed/mem.cpp`: Mem pool, pins, compactor and image copier.
- `C:/Users/wellons/src/unabashed/sched.cpp` and `C:/Users/wellons/src/unabashed/proc.cpp`: Kernel, vprocs, scheduler, pipelines, materialization.
- `C:/Users/wellons/src/unabashed/vars.cpp`: gen-COW trie, shallow binding, binding cache, bash-order assoc.
- `C:/Users/wellons/src/unabashed/vos.cpp` with `main_test.cpp`: deterministic virtual OS, fake programs, Windows personality.

References:
- `C:/Users/wellons/src/skeeto.github.com/_ssg/base.cpp`, `main_win32.cpp`, `wide.cpp` (foundation to copy)
- `C:/Users/wellons/src/sshd/src/win32.cpp` (XP-capable spawn)
- `C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3/hashlib.c` (semantic reference only: assoc order)
