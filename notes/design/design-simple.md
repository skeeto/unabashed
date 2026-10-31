# unabashed: architecture design

**Approach in one paragraph.** The core is single-threaded and deterministic. It runs every piece of shell code as an in-process *virtual process* (a "vproc"). A vproc runs on the caller's stack when nothing else needs to run alongside it: command substitution, `( … )` and most other subshells. It gets its own **fiber** only when it must run concurrently: pipeline stages, `&`, coprocs and process substitution. A core-owned cooperative scheduler is the only place that blocks, through one platform call, `os_wait`. Shell state lives in **persistent (path-copying) hash tries**, so creating any subshell costs O(1). Each vproc owns its arenas, and every command runs in a by-value scratch arena that is swept when the command finishes. Real processes are created only for external programs. The virtual OS provides fake programs, an in-memory filesystem, a virtual clock and a virtual terminal on the same fiber primitive, so tests and AFL runs are bit-for-bit reproducible.

---

## 1. Execution model without fork

### Options considered and rejected
- **Forkshell (busybox):** it costs one CreateProcess plus a full serialization per subshell. That is slow, and it is the exact thing we are trying to beat.
- **OS threads per stage:** nondeterministic under the virtual OS, hostile to AFL, and the core would need locks.
- **Sequential stages through unbounded buffers:** wrong for `yes | head` and `tail -f | while read`.
- **Real fork on POSIX:** two execution models means two sets of bugs, and differential testing on Linux would stop exercising the Windows model. **Rejected.** The POSIX layer runs the same vproc model.
- **Rewriting the interpreter as an explicit state machine so it can suspend anywhere:** a large complexity tax on every expansion path. Fibers keep the interpreter plain recursive C++.

### Chosen model
- **Vproc kinds.**
  - *Synchronous:* comsub, foreground `( )`, `${ }` (which is not a subshell, just a capture), and command-substitution-like internal uses. It runs on the parent's fiber under a catch point while the parent is suspended. No fiber and no scheduler involvement.
  - *Concurrent:* every pipeline stage except a lastpipe final stage, `&`, `coproc`, and `<( )`/`>( )`. Each gets a fiber, with stacks taken from a pool.
- **Scheduler (`sched.cpp`).**
  - A FIFO run queue of tasks. A task is either a fiber (a vproc) or a **pump**: a small state machine that copies between a core buffer and an OS fd.
  - Blocking points are in-memory pipe read/write, OS fd read/write, child process exit, vproc exit, and timers (`sleep`, `read -t`, `wait -n`).
  - When nothing can run, the scheduler calls `os_wait` with the union of OS wait items. Without a deadline, the call returns only when something becomes ready or a signal arrives.
- **Edges between pipeline stages.**
  - Shell code ↔ shell code: an in-memory `Pipe` (a 64 KiB ring, matching the Linux pipe capacity so PIPESTATUS/SIGPIPE behaviour matches).
  - Any edge touching a likely external program: a real OS pipe. "Likely external" is decided at setup time by a literal command word that is not a function or builtin.
  - Correctness never depends on that guess. If a shell-code stage holding an in-memory fd spawns an external program, the fd is **materialized** into an OS pipe with a pump task. If a shell-code stage holds an OS pipe end, it simply uses it through the platform.
- **A shell stage feeding an external program** (`f | grep x`): f's fiber writes to the OS pipe through `os_write(..., block=0)`. A full pipe returns `E_AGAIN`, and f parks on a WRITE wait item.
  - When grep exits, the write fails with `E_PIPE`. That delivers a virtual SIGPIPE to f's vproc, which unwinds with status 141.
- **Comsub capture.** fd 1 is a `FILE_MEM` capture buffer in the arena. Builtins write straight into it with no kernel round trip. An external program in the comsub gets an OS pipe, plus a pump that drains it into the buffer. A CR before a trailing LF is stripped in msys mode.
- **Here-docs.**
  - Builtin or function consumers get an in-memory `FILE_MEM` with no temp file.
  - External consumers get an OS pipe plus a pump task. That gives no size limit and no deadlock, even with `$(ext <<EOF …)`, because the pump and the capture are both scheduler tasks.
- **Process substitution.** The body runs as a concurrent vproc.
  - When the consumer is internal (`< <(…)`), the core recognizes `/dev/fd/N` and uses an in-memory pipe.
  - When the consumer is external, `os_pipe` with a name is used: `\\.\pipe\unabashed-<pid>-<n>` on Win32, `/dev/fd/N` on POSIX, and a named pipe in the virtual FS.
- **`$$`, `$BASHPID` and `$!`.**
  - `$$` is the real pid of the shell process.
  - Each vproc gets a *virtual pid*: `vpidbase + k*vpidstep`.
    - Win32: real pids are multiples of 4, so `base ≡ 2 (mod 4)` never collides.
    - POSIX: start above `pid_max`.
    - Virtual OS: allocated deterministically.
  - `kill`/`wait` on a vpid goes to the core's process table, and on a real pid to `os_kill`/`os_status`.
- **exit, errors and OOM in subshells.** These `jmpgo` to the vproc's catch point.
  - The catch point lives on whichever stack the vproc runs on: the parent's stack for synchronous vprocs, the fiber base for concurrent ones.
  - The rule is never to jump across fibers.
  - `Jmp` is `__builtin_setjmp` on GCC, with a tiny asm save/restore in `fiber.cpp` for clang/aarch64 (see the skill note on arm64).
- **Traps in subshells.** Each vproc has its own trap table. On subshell entry, caught traps are reset and ignored ones stay ignored. A pointer to the parent's table is kept so that `trap` with no arguments inside `$( )` prints the parent's traps, as bash does. The EXIT trap runs when the vproc ends.
- **Signals.**
  - The core keeps a per-vproc `sigpending` bitmask. Signals are delivered at safe points: command boundaries and wake-ups from blocking waits.
  - Win32: the Ctrl-C handler (on a system thread) sets a flag and an event, and `os_wait` returns `OS_INTERRUPT`. The core delivers SIGINT to the foreground job's vprocs, and the console delivers it to external children.
  - A child exit code of `0xC000013A` maps to "died of SIGINT", which drives bash's cooperative-exit rule (the shell itself acts as interrupted only if the child died of SIGINT).
  - POSIX: a SIGINT/SIGCHLD self-pipe wakes `poll`.
  - `trap '' INT` calls `os_sigignore`, which children inherit (on Win32, `SetConsoleCtrlHandler(0,1)`).
- **exec.**
  - POSIX: `os_exec` really calls execve.
  - Win32: spawn, wait, exit with the child's status, ignoring Ctrl-C while waiting (busybox's approach).
  - In a vproc, `(exec ext)` spawns the program and the vproc ends when it ends.
- **Coprocs.** A concurrent vproc (or an external program) with two edges. Its fds are installed in the parent's fd table, and `COPROC[0/1]` hold their numbers.
- **Background shell jobs that outlive the script** (`f & exit`). The process stays alive until those vprocs finish. Bash would orphan them; see Risks and Open Questions.

### Spawning and waiting
- **Win32.**
  - The resolved path is always passed as `lpApplicationName`.
  - The core builds the final UTF-8 command line, because the quoting scheme depends on the target: MSVCRT for native programs, Cygwin for msys PE files, and `%ComSpec% /d /e:on /v:off /s /c` with BatBadBut-safe escaping for .bat/.cmd.
  - The environment block is cached by generation. `lpCurrentDirectory` comes from the vproc's virtual cwd.
  - Std handles are inheritable duplicates made just for the spawn. Because the core is single-threaded, no spawn lock is needed: platform helper threads never create inheritable handles.
  - `CREATE_NEW_PROCESS_GROUP` is used for background jobs.
  - Deferred: job objects (an XP nesting problem; use TerminateProcess first) and `lpReserved2` for fds ≥ 3 (a bitness risk).
- **Win32 non-blocking I/O, without overlapped I/O or IOCP.**
  - Shell-side pipe ends are ordinary synchronous handles, so they can be inherited by later children with exact byte semantics.
  - `os_read(block=0)` first tries `PeekNamedPipe`. If data is available it reads exactly that much. Otherwise it queues the blocking ReadFile on a pooled **helper thread** (CRITICAL_SECTION plus events, XP-safe), returns `E_AGAIN`, and the helper's event becomes the wait item. Writes work the same way.
  - When the scheduler knows it is the only runnable task with a single waiter, it passes `block=1` and the main thread blocks directly. This is the common fast path.
  - `os_wait` is `WaitForMultipleObjects` over process handles, helper events, console input and the Ctrl-C event. Above 63 items it falls back to chunked polling with a 1-10 ms backoff (busybox does the same; this can be improved later with RegisterWaitForSingleObject).
- **POSIX.** fork + dup2 + execve inside `main_posix`; the platform layer may fork, the core never does. `poll()` checks readiness, writes are capped at PIPE_BUF after POLLOUT, and the SIGCHLD self-pipe reports exits. `O_NONBLOCK` is never set on shared descriptions.
- **Virtual OS.** Fake programs run as fibers inside the virtual kernel.
  - `os_wait` runs ready fake processes round-robin until a waited item is ready.
  - If none can make progress and there is a deadline, it advances the virtual clock, so `sleep 100` takes zero real time.
  - If nothing can ever progress, it reports `OS_DEADLOCK`, which is a test failure instead of a hang.

### Fd table virtualization
```cpp
struct File {                 // shared, refcounted, kernel-owned record
    i32    kind;              // FILE_OS, FILE_PIPER, FILE_PIPEW, FILE_MEM, FILE_NULL
    i32    refs;
    i32    flags;             // OS_APPEND, OS_RDWR, ...
    OsFd   fd;                // FILE_OS
    Pipe  *pipe;              // FILE_PIPE*
    Buf   *mem;               // here-doc body (read) or comsub capture (write)
    iz     off;               // read offset into mem
};

struct Fdtab {
    File **slot;              // index = shell fd number, 0 = closed
    i32    len;
    i32    cap;
};

struct Pipe {
    u8    *buf;               // ring, PIPE_CAP bytes
    iz     head;
    iz     len;
    i32    readers;
    i32    writers;
    Proc  *rwait;             // procs parked on this pipe
    Proc  *wwait;
};
```
- Redirections edit the vproc's `Fdtab`, recording undo entries in scratch that are restored after the command. `exec` makes them permanent.
- `{var}>f` allocates slots from 10 upward.
- Spawning maps slots 0-2 to `Spawn.fds`. Any in-memory kind is materialized first.

---

## 2. Memory model

### Arena placement
```cpp
struct Arena {
    byte *beg;
    byte *end;                // committed end; grows by os_commit up to mem limit
    Mem  *mem;                // reservation limits and OOM escape (u-config's ctx)
};

// Memory owned by one vproc, recycled through a kernel pool.
struct Mem {
    Arena  store;             // persistent-trie nodes and values: compacted
    Arena  code;              // function bodies: append-only, never moved
    Arena  scratch;           // per-command temporaries: swept by value
    Rsv    rsv[4];            // store, to-space, code, scratch reservations
    Jmp   *oom;               // where OOM unwinds this vproc
    i32    refs;              // child vprocs sharing structure from here
    Mem   *next;              // pool free list
};
```
- `alloc` slow path: commit more of the reservation with `os_commit(byte *, iz)`, which needs no `Os` (like `os_oom`). If the reservation is exhausted, jump to `mem->oom`.
  - A child vproc that runs out of memory dies with status 2 and the message `unabashed: xmalloc: cannot allocate N bytes`, matching bash's wording.
  - The main vproc does the same: it exits non-interactively, or returns to the prompt when interactive.
  - Persistent updates swap the root pointer only at the end, so an OOM in the middle of an update leaves the old state intact.
- **Reservation sizes come from `OsInfo.memcfg`.**
  - 64-bit: main store 16 GiB, code 1 GiB, scratch 4 GiB; each child 1 GiB per arena; fiber stacks 1 MiB. All reserve-only and committed lazily.
  - 32-bit XP: main 512 MiB/64 MiB/128 MiB; children 32 MiB/16 MiB; stacks 256 KiB. This caps the number of concurrent vprocs at roughly 30, which is documented.
  - Test/fuzz: small malloc'd blocks, so OOM paths are reachable.
- **What lives where.**
  - Kernel arena (session lifetime, fixed-size records with free lists): Proc records, File records, Mem headers, the job table, the history ring header.
  - `store`: variables, arrays, assoc arrays, the function table (name → Func*), aliases, the command hash, positional parameters set by `set --`, the dirstack, trap strings.
  - `code`: function bodies, deep-copied into `code` when defined. A top-level parse tree lives in scratch and is swept, so only definitions persist.
  - `scratch`: everything else, including the top-level parse tree, expansions, argv, globs, redirection undo lists and function-call frames.
    - A frame lives in the scratch of the calling command, which gives it exactly a C-stack lifetime. The frame is still a compaction root.
  - History: a ring of `Str` in its own arena, semispace-copied when full.

### Per-command sweep (the shellexec pattern)
```cpp
// Run commands from an input until EOF. Each complete command's parse
// tree and temporaries are swept before the next one is read.
static i32 runinput(Proc *p, Input *in)
{
    for (;;) {
        Arena scratch = p->mem->scratch;        // by value: swept per command
        Parse r = parsecommand(p, in, &scratch);
        if (r.eof) break;
        if (r.cmd) p->sh.status = exec(p, r.cmd, scratch);
        safepoint(p);                           // signals, compaction
    }
    return p->sh.status;
}
```
- Every `exec*` takes `Arena scratch` by value. Loop bodies use a fresh `Arena tmp = scratch;` per iteration and call `safepoint(p)` after it.
- A top-level `while` that runs for hours therefore sweeps each iteration's expansions. The only growth is in `store`, which is handled by the next two points.

### Bounding the store under mutation
1. **In-place updates when owned.** A value carries `gen` and `cap`. If `v->gen == p->gen` and the new value fits, it is written in place. `s+=x` appends in place, with geometric capacity growth.
   - `i=$((i+1))` writes in place and also caches `num`.
   - `declare -i` values are stored natively as `i64`, and the string is rendered only when needed.
2. **Semispace compaction** (`mem.cpp`).
   - Trigger: at a safe point, when `store.used > max(1 MiB, 2 × live_after_last)`.
   - Action: copy everything reachable from the vproc's roots (State tries, frames, job table, hash, traps) into the to-space, then swap.
   - This is amortized O(1) per allocation. It removes the garbage left by locals that function returns pop off, by unset assoc keys (tombstones), and by old buffers.
   - Safe point conditions: no expansion in progress (`p->expanding == 0`), no re-entrant builtin holding pointers (`p->nocompact == 0`, set by `mapfile -C` and similar callbacks), and `mem->refs == 0` (no live child shares this store).
   - **Invariant enforced from M2:** no C-stack pointer into `store` survives a command boundary. Anything that must persist is reached through `p->sh`.
   - **GC-stress build:** compact at every safe point and fill the from-space with `0x55`. Violations then show up immediately in tests and fuzzing.
   - Function bodies are never moved; they are in `code`. Redefining a function in an unbounded loop leaks its old body. That is accepted, and the `code` arena can be compacted when idle at top level.

### Subshell isolation: O(1) through persistence
```cpp
struct Vars {                 // persistent 4-ary hash trie: Name -> Value stack
    Vars  *child[4];
    Name   key;               // {Str s; u64 hash} precomputed at parse time
    Value *top;               // innermost binding (shallow binding)
    u32    gen;
};

// Slot for key in the current vproc's view, copying shared nodes on the path.
static Value **varslot(Proc *p, Vars **m, Name key)
{
    for (u64 h = key.hash;; h <<= 2) {
        Vars *n = *m;
        if (!n) {
            n = *m = alloc<Vars>(&p->mem->store);
            n->key = key;
            n->gen = p->gen;
            return &n->top;
        }
        if (n->gen != p->gen) {           // shared with a parent or child: copy
            Vars *c = alloc<Vars>(&p->mem->store);
            *c = *n;
            c->gen = p->gen;
            *m = n = c;
        }
        if (n->key.hash==key.hash && n->key.s==key.s) return &n->top;
        m = &n->child[h>>62];
    }
}

static Proc *subshell(Kernel *k, Proc *p, b32 concurrent)
{
    Proc *c = newproc(k, p);              // Mem from the pool, vpid allocated
    c->sh   = p->sh;                      // copies roots only
    c->gen  = ++k->gen;                   // the child copies before writing
    if (concurrent) {
        p->gen = ++k->gen;                // the parent keeps running: it copies too
        p->mem->refs++;                   // pins the parent's store against compaction
    }
    c->sh.fds   = dupfds(k, p->sh.fds);   // refcount Files
    c->sh.traps = subtraps(c, p->sh.traps);
    c->sh.subshell++;
    return c;
}
```
- A synchronous child never mutates nodes the parent owns, and its Mem is reset when it exits. The parent resumes with untouched roots. **No undo log is needed.**
- Lookup is lock-free and costs about log₄(n) node visits, with a hash comparison before any string comparison.
- Background jobs that outlive their parent's command keep the parent's `Mem` alive through `refs`, and the pinned store cannot be compacted until they exit. That is a known growth case (see Risks).

---

## 3. Core representations

- **AST.** `Cmd` nodes live in scratch, or in `code` for function bodies. The kinds are Simple, Pipeline, AndOr, Seq, Async, Subshell, Group, If, While/Until, For, ArithFor, Select, Case, Arith, Cond, FuncDef, Coproc, Time and Bang. Each node carries `line`, and redirections attach to the node.
- **Words** are parsed once into parts:
```cpp
struct Word {
    Str    text;              // canonical source spelling (comsubs re-printed)
    Part  *parts;
    i32    flags;             // HAS_GLOB, HAS_EXPANSION, ASSIGN, ...
};
struct Part {
    Part  *next;
    i32    kind;              // LIT, SQ, DQ, PARAM, CMDSUB, ARITH, PROCSUB, TILDE, BRACE
    Str    lit;               // LIT, SQ: bytes; quoted flag below
    b32    quoted;
    Name   name;              // PARAM
    i32    op;                // ${x:-}, ${x#}, ${x//}, ${x@Q}, ${!x}, ...
    Word  *arg;               // operand words
    Cmd   *cmd;               // CMDSUB, PROCSUB: parsed once, never re-parsed
    Arith *arith;             // ARITH when the text has no expansions; else 0
};
```
  - Literal-only words expand with zero copies (a `Str` pointing into the AST).
- **Canonical printing** (`print.cpp`).
  - Bash prints functions from the tree (`print_cmd.c`), with word text kept as written. We do the same: `Word.text` holds the spelling, and the printer reproduces bash's layout rules.
  - Those rules are written from observed behaviour, with expected outputs captured from real `declare -f`. No GPL code is copied.
  - Comsub bodies are re-printed at parse time and spliced into `Word.text`, as bash 5.2+ does. The printer is therefore needed by M1.
  - `BASH_COMMAND` is a dynamic variable that prints the current `Cmd*` only when referenced, or when a DEBUG/ERR trap is set. Nothing is printed per command otherwise.
  - `set -x` uses a separate quoting routine for expanded words (`'…'`, `$'…'`).
- **Aliases** are textual and expanded at lex time.
  - The lexer has an input stack. When a word in command position names an alias, the alias value is pushed onto that stack, with the trailing-blank and recursion-guard rules.
  - We parse one complete command at a time, then run it, so `alias x=…; x` on the same line behaves as in bash. Function bodies bind their aliases when defined.
- **Arithmetic.**
  - At parse time, `$(( ))`/`(( ))`/`for (( ))` text without `$` or backquotes compiles to an `Arith` tree whose names are pre-hashed.
  - Text containing expansions is expanded first, then parsed into scratch. A text-keyed cache waits until profiling shows a need.
  - Reading a variable uses `Value.num` when it is valid. Otherwise the string is parsed as a number, or, failing that, recursively evaluated as an expression, which is bash's semantics.
- **Glob and extglob** (`pattern.cpp`).
  - Patterns compile from quoted/unquoted parts into a small program: literal runs, `?`, `*`, bracket sets (UTF-8 or bytes; `globasciiranges`), and extglob groups.
  - Patterns without extglob match with the linear star-backtracking two-pointer method. Extglob matching memoizes (node × position) in scratch.
  - `${s//pat/rep}`: a literal fast path (memmem), otherwise one NFA pass per start position that reports the longest end. That is O(n·m), against bash's O(n²·m).
  - Matching runs on UTF-8 bytes and decodes inline only for `?`, brackets and case folding. Strings are never converted to wide characters.
- **ERE for `=~`** (`regex.cpp`): compile to an NFA and do a leftmost-longest search, with submatches chosen by POSIX rules through a backtracking tie-break under a step limit. glibc's backreference extension is supported, and the oracle is differential tests against Linux bash.
- **Variables.**
```cpp
struct Value {
    Value  *below;            // shadowed binding (local pushes, return pops)
    i32     depth;            // frame depth that declared it
    i32     attr;             // ATTR_INT|EXPORT|READONLY|NAMEREF|ARRAY|ASSOC|LOWER|UPPER|...
    u32     gen;
    Str     str;              // data==0: unset
    iz      cap;              // in-place growth when gen matches
    i64     num;
    b32     numok;
    Array  *array;
    Assoc  *assoc;
    i32     dyn;              // index into the dynamic-variable getter/setter table
};
```
  - **Shallow binding:** lookup is O(1) at any call depth. Each frame records the names it localized so that return can pop them.
  - Namerefs are chased with a depth cap and a cycle error.
  - Dynamic variables (RANDOM, SECONDS, LINENO, FUNCNAME, BASH_LINENO, BASH_SOURCE, PIPESTATUS, …) dispatch on `dyn`. The FUNCNAME family is materialized lazily from the frame stack, which removes bash's O(d²) cost.
- **Indexed arrays:** a persistent sparse radix tree, 16-ary, with a height that grows to fit the maximum index and `count`/`max` kept in the header. Random access is O(log n) (bash's linked list is O(n)), it iterates in order, and copy-on-write works per path.
- **Assoc arrays:** a persistent hash trie keyed by string. Each entry also stores bash's 32-bit hash, a `rank`, and the table's emulated `nbuckets`, with the grow-4× rule.
  - Iteration sorts entries by (bucket, rank). That reproduces bash's bucket order, including the reordering when a rehash re-inserts entries.
  - Pin this down with differential tests. Until then, iteration order is listed as a known divergence.
- **Environment block cache:** each change to an exported value bumps `p->sh.envgen`. `Env {Str *kv; iz n; u64 gen}` is rebuilt only when that generation changes, and the platform caches its UTF-16 or `char **` form by `gen`.
- **printf numbers:** our own soft extended-precision type (64-bit mantissa, matching x87 `long double`). Decimal parsing is correctly rounded, and %f/%e/%g/%a formatting is exact through a small bignum. It gives identical output on every platform, with no libc and no hardware `long double`.

---

## 4. Platform API

```cpp
// Platform API
struct Os;
struct Fiber;
typedef i32 OsFd;             // platform handle index, -1 = none
typedef i32 OsProc;

struct IoResult   { iz len; Err err; };       // E_AGAIN: started, wait on it
struct OsPipe     { OsFd r; OsFd w; Str name; Err err; };
struct Stat       { Err err; i32 type; i32 mode; i32 nlink; i64 size;
                    i64 atime; i64 mtime; i64 ctime; u64 dev; u64 ino;
                    i32 uid; i32 gid; };      // Win32 synthesizes x bits
struct Dirent     { Dirent *next; Str name; i32 type; };
struct Spawn      { Str path; Str cmdline; Str *argv; i32 argc; Env *env;
                    Str cwd; OsFd fds[3]; i32 umask; b32 background; };
struct SpawnRes   { OsProc proc; i32 pid; Err err; };
struct ProcStatus { b32 done; b32 signaled; u32 code; };  // raw: core maps
struct OsWait     { i32 kind; i32 id; b32 ready; };       // WAIT_READ/WRITE/PROC/TTY
struct TtyInfo    { b32 tty; b32 vt; b32 xenl; i32 cols; i32 rows; };
struct OsInfo     { i32 personality; b32 kernelshebang; i32 pid; i32 ppid;
                    i32 vpidbase; i32 vpidstep; Str exe; Str cwd; Str *env;
                    iz nenv; Str *args; i32 nargs; Str home; Str user;
                    Str host; i32 uid; MemCfg memcfg; i32 stdkind[3]; };

static OsFd       os_open(Os *, Arena, Str path, i32 flags, i32 mode); // -Err on failure
static IoResult   os_read(Os *, OsFd, u8 *, iz, b32 block);
static IoResult   os_peek(Os *, OsFd, u8 *, iz);         // E_NOSYS if unsupported
static IoResult   os_write(Os *, OsFd, u8 *, iz, b32 block);
static i64        os_seek(Os *, OsFd, i64, i32 whence);
static void       os_close(Os *, OsFd);
static OsPipe     os_pipe(Os *, Arena *, b32 named);      // named: for procsub
static Stat       os_stat(Os *, Arena, Str path, b32 follow);
static Stat       os_fstat(Os *, OsFd);
static Dirent    *os_list(Os *, Arena *, Str dir);         // excludes . and ..
static Str        os_readlink(Os *, Arena *, Str path);
static Str        os_realpath(Os *, Arena *, Str path);    // pwd -P, cd -P
static SpawnRes   os_spawn(Os *, Arena, Spawn *);
[[noreturn]] static void os_exec(Os *, Arena, Spawn *);    // Win32: spawn+wait+exit
static ProcStatus os_status(Os *, OsProc);
static void       os_reap(Os *, OsProc);
static Err        os_kill(Os *, i32 pid, i32 sig);
static i32        os_wait(Os *, OsWait *, i32 n, i64 deadline); // idx, OS_TIMEOUT,
                                                                // OS_INTERRUPT, OS_DEADLOCK
static u64        os_signals(Os *);                        // fetch and clear pending
static void       os_sigignore(Os *, i32 sig, b32);        // inherited by children
static Fiber     *os_fiber(Os *, iz stack, void (*)(void *), void *);
static void       os_switch(Os *, Fiber *);
static void       os_fiberfree(Os *, Fiber *);
static byte      *os_reserve(Os *, iz);
static b32        os_commit(byte *, iz);                   // no Os: alloc slow path
static void       os_release(Os *, byte *, iz);
static i64        os_now(Os *);                            // ns since epoch
static i64        os_clock(Os *);                          // monotonic ns
static TimeZone   os_tz(Os *, i64 t);                      // %(..)T, \t, \D{}
static CpuTimes   os_times(Os *);                          // times, time
static u64        os_random(Os *);                         // SRANDOM, seeding
static TtyInfo    os_tty(Os *, OsFd);
static b32        os_ttyraw(Os *, OsFd, b32 raw);          // prep/deprep
[[noreturn]] static void os_exit(Os *, i32);
[[noreturn]] static void os_oom();
static i32        shell_main(Os *, OsInfo *);              // core entry
```

**Terminal model.** The editor only ever reads input bytes in xterm form and writes output in a fixed VT subset.
- Win32 with VT available (Win10 1511+): VT input mode, and output passes through WriteConsoleW with a 3-byte UTF-8 carry.
- Win32 legacy console (XP/7): the platform converts `ReadConsoleInputW` KEY_EVENTs into the same escape sequences, with AltGr treated as text and surrogate pairs joined. A small VT-subset interpreter (CUU/CUD/CUF/CUB/CHA, EL, ED, SGR) drives the console API. `TtyInfo.xenl=0` tells the editor there is no deferred wrap.
- POSIX: termios.
- Virtual OS: a scripted input stream and a VT grid model that **traps on any sequence outside the subset**.

**Path translation sits in the core, not the platform.**
- `path.cpp` holds:
  - msys/native mode;
  - the mount table, read from `<root>/etc/fstab` through `os_open`;
  - the `/c/` and `/tmp` mappings;
  - drive-relative paths and `=X:` entries;
  - reserved device names;
  - `/dev/null`, `/dev/stdin` and `/dev/fd/N` (handled before the platform);
  - case-insensitive comparison;
  - PATH separators;
  - environment translation at import and export;
  - the msys2_path_conv argument heuristic with MSYS2_ARG_CONV_EXCL;
  - the three quoting schemes;
  - PE import sniffing to detect msys targets, cached by path and mtime;
  - shebang emulation when `!kernelshebang`;
  - .exe/.com/.bat/.cmd probing.
- The platform receives only absolute native paths: `C:/a/b` or `//srv/sh/x` on Win32, which it converts to `\\?\` UTF-16, and `/a/b` on POSIX.
- The payoff is that the virtual OS can run with `personality=WIN32`: drive letters, a case-insensitive FS, fake `cmd.exe`, and fake msys-linked binaries. **All Windows logic is then tested on Linux CI and fuzzed.**

---

## 5. Testing architecture

1. **Unit tests** (`main_test.cpp` + `test/*_test.cpp`, ssg-style `expect`, stderr progress):
   - lexer, parser and printer round-trips;
   - arithmetic, patterns, ERE;
   - path modes, command-line quoting (MSVCRT/Cygwin/cmd), msys argument conversion;
   - environment blocks;
   - soft-float printf;
   - persistent tries, including copy-on-write isolation;
   - assoc ordering;
   - `wide.cpp`.
2. **Virtual OS** (`vos.cpp`):
   - Filesystem: a hash trie of path → node, sorted listings, mode bits, symlinks, FIFOs.
   - Processes: fibers in a process table.
   - A virtual clock and seeded random.
   - Fake programs: recho, zecho, printenv, xcase, cat, rm, mkdir, rmdir, touch, ls, chmod, ln, cp, mv, grep, a minimal sed, tr, wc, expr, od, sort, head, cmp, diff, env, tee, mktemp, mkfifo, sleep, date, kill, a minimal awk.
   - `bash`/`sh`/`THIS_SH` run **a fresh core instance** with its own `Os` view. The core keeps no globals, so instances can nest.
   - Synthetic `/etc/passwd`, `/tmp` and `/dev`, with both personalities.
3. **Script specs** (`t/*.spec`):
   - Each case has script, stdout, stderr and status sections, and optionally a personality and a mode.
   - Expected output is captured from real bash 5.3 by `tools/capture` running in a Docker image built from the tarball. These are our own scripts plus bash's output, so they are safe to commit.
4. **Bash's own suite as an external oracle:**
   - `tools/bashtests` takes `BASH_SRC=` pointing at a local copy and reimplements the run-* contract: fixed environment, copied workdir, filters, diff against `.right`.
   - Categories a and b run in the virtual OS. Categories c, d and e run on the POSIX build inside a disposable container (non-root, `--network none`).
   - The repo holds only a manifest (test name, expected status, reason). No GPL text is copied.
5. **Differential testing:**
   - Corpus scripts run under ours (POSIX build) and bash 5.3, side by side in the same container. stdout, stderr, status and the merged `2>&1` stream are compared after normalizing TMPDIR and pids.
   - On Windows, msys behaviours (argument conversion, path output, `cygpath`) are compared against Git Bash 5.3.15, using a curated corpus in a scratch directory only.
6. **Fuzzing** (`main_fuzz.cpp`, virtual OS only, `os_spawn` into fakes, real filesystem calls trap):
   - (a) parse → print → reparse → print must be idempotent;
   - (b) execute with a fuel budget (commands per run), small arenas so OOM paths are reached, and GC-stress on;
   - (c) offline differential replay of the AFL corpus against bash in a throwaway container.
7. **Line editor:**
   - Headless: key bytes in, virtual screen grid plus buffer/point out, compared against golden values.
   - End to end: winctl scripts on conhost, Windows Terminal (launched as `conhost.exe unabashed` to force classic) and the XP laptop's legacy console. Use `type -` for Unicode and `shot` for crops.
8. **Benchmarks** (`bench/` B1-B13, M1-M4):
   - hyperfine against bash 5.3 in WSL, Git Bash, MSYS2 bash and busybox `sh`, with checksums verified before any timing is trusted.
   - Instruction counts on the virtual OS act as a regression gate.

---

## 6. Source layout and milestones

```
unabashed/
  base.cpp          types, Arena(+Mem ctx), Str, Slice, Buf, utf8, sort, Jmp
  fiber.cpp         asm stack switch + jmp for x86/x64/aarch64 (posix/test/fuzz)
  wide.cpp          UTF-16/WTF-8, Win32 cmdline split (portable half of win32)
  unabashed.cpp     Platform API table, includes modules, shell_main
    err.cpp         Err codes, glibc strerror text, signal names/numbers
    mem.cpp         reservations pool, arena growth, compaction
    lex.cpp parse.cpp print.cpp
    arith.cpp pattern.cpp regex.cpp
    var.cpp         tries, Value stacks, arrays, assoc, namerefs, dynamic vars
    expand.cpp      brace, tilde, params, comsub, arith, split, glob, quote removal
    path.cpp        modes, mounts, cwd, arg conversion, quoting, PE sniff, shebang
    io.cpp          File/Fdtab, Pipe, pumps, redirections, here-docs
    sched.cpp       Kernel, Proc, scheduler, signals, jobs, wait
    exec.cpp        commands, functions, pipelines, subshells, lookup, spawn
    builtins.cpp + builtin_{vars,io,test,cd,trap,hist,comp}.cpp
    printf.cpp      formatter, soft extended float
    hist.cpp edit.cpp shell.cpp
  main_win32.cpp  main_posix.cpp  main_test.cpp vos.cpp test/  main_fuzz.cpp
  t/  bench/  tools/ (capture, difftest, bashtests, Dockerfile.bash53)
```

| M | Deliverable (runnable and testable) |
|---|---|
| 0 | `base`/`fiber`/`wide`, the API table, and all four platform layers building, including the i686 XP build, with `unabashed -c` echoing its arguments. Also the Docker bash 5.3 image and `tools/capture`. |
| 1 | Lexer, parser and printer. `unabashed -n` parses scripts, and a hidden dump flag prints them canonically. Golden tests use captured `declare -f` output, and the parser fuzzer runs. |
| 2 | Main vproc only: variables (persistent tries from day one), expansions without comsub, arithmetic, compound commands, functions, file redirections, core builtins, synchronous external spawn. Exact error-message formats. Virtual FS and basic fakes. Script specs start. |
| 3 | Vprocs and the scheduler: comsub, `( )`, fiber pipelines, in-memory and OS pipes, pumps, here-docs, procsub, `&`/`wait`, `$BASHPID`, the EXIT trap, SIGPIPE. Fake programs become fibers. `run-minimal` passes in the virtual OS. |
| 4 | P0 completeness: indexed and assoc arrays, glob/extglob, ERE, `set -euo pipefail`/`-x`, `local`/`declare`/namerefs, `read`/`mapfile`, full printf, `[[ ]]`. Categories a and b of the bash suite. |
| 5 | Windows as a first-class target: path modes, msys conversion, quoting, PATH/.exe/shebang, env translation, Ctrl-C, console output, helper-thread I/O. A drop-in `sh.exe` for w64devkit (make's `sh -c`). Differential runs against Git Bash. |
| 6 | Compaction with the GC-stress mode, plus the performance pass and the benchmark suite. |
| 7 | Interactive use: the emacs-mode editor, history and history expansion, completion, prompts, basic inputrc; VT, legacy-console and termios backends; winctl end-to-end tests. |
| 8 | Remaining features: DEBUG/ERR/RETURN traps, extdebug, coproc, POSIX mode details, dirstack, select, vi mode, `bind`, programmable completion, POSIX job control. |

**Deliberately deferred or rejected for simplicity:**
- real fork;
- threads in the core;
- IOCP/overlapped I/O (helper threads instead);
- job objects;
- `lpReserved2`;
- termcap;
- the sequential-pipeline trick;
- an arithmetic text cache;
- avoiding fibers for trivially external pipeline stages (later: stages whose words contain no comsub can be expanded synchronously in an O(1) subshell);
- compaction before M6, though its invariant is enforced from M2.

**Where we would not beat bash automatically, and the remedy:**
- **CreateProcess cost:** ours equals theirs. Avoid extra processes instead: never route through cmd.exe or a busybox stub (spawn busybox.exe directly with argv[0]), and cache PATH lookups and the environment block.
- **`read` on foreign POSIX pipes:** byte at a time, like bash. On pipes we create it is exact and fast through PeekNamedPipe, or a socketpair with MSG_PEEK on POSIX.
- **Compaction pauses:** proportional to live state, amortized.
- **Fiber stack commit:** pooled.

---

## Top risks
1. **Compaction invariant.** A stray C-stack pointer into `store` causes memory corruption. Mitigation: the GC-stress build, poisoning, and fuzzing with it on.
2. **Fiber stacks.** Deep recursion in a non-main fiber can overflow. Mitigation: a check of the stack pointer against the fiber's limit that raises a bash-style error, guard pages, and a large main stack. ASan needs fiber-switch annotations (an issue only in test builds).
3. **Orphaned background shell jobs.** `sh -c 'f &'` keeps our process alive where bash returns immediately. This could block callers such as make or agents.
4. **Win32 helper-thread I/O on XP.** There is no CancelSynchronousIo, so a killed fiber can leave a helper thread blocked until data or EOF arrives.
5. **Pump over-read** when an in-memory fd is materialized for an external program (as with `{ head -1; read y; }`): a small semantic divergence.
6. **Exact formatting fidelity** across `declare -f`, error messages, `set -x`, shopt lists, ERE submatches (glibc quirks) and assoc order. This is a large surface, and differential testing is the only real defence.
7. **32-bit address space.** The cap of about 30 concurrent vprocs and the 32 MiB per-child store on XP.
8. **Virtual pids.** `$BASHPID` and `$!` of shell jobs are invisible to `ps`/taskkill and to `/proc/$pid`.

## Open questions for the user
1. Should `sh`/`ash` invocation use bash `--posix` semantics and messages with native paths, or also imitate ash's message formats (`sh: foo: not found`)?
2. In msys mode, should `OSTYPE` be `msys` or `cygwin`, and should `MACHTYPE` follow Git Bash?
3. Are virtual pids for shell-code jobs acceptable? And at exit with running shell jobs, should we wait for them (proposed), kill them, or do something else?
4. Should fds ≥ 3 be passed to native children (`lpReserved2`, with a bitness risk), or only 0-2, as Cygwin does?
5. `help` text cannot be copied (GPL). Is our own wording acceptable, or should `help` be minimal?
6. Job control (stop/fg/bg) on Windows: out of scope permanently?
7. What priority should byte-exact assoc iteration order get (proposed: M4 stretch goal)?
8. With argv[0] `unabashed`, should the error prefix follow `$0` as bash does, or always say `bash:`?

### Critical Files for Implementation
- C:/Users/wellons/src/unabashed/unabashed.cpp (Platform API table, module includes, shell_main)
- C:/Users/wellons/src/unabashed/sched.cpp (Kernel, Proc/vproc, scheduler, subshell creation)
- C:/Users/wellons/src/unabashed/var.cpp and mem.cpp (persistent tries, gen-based copy-on-write, compaction)
- C:/Users/wellons/src/unabashed/vos.cpp with main_test.cpp (virtual OS: FS, fake-program fibers, clock, terminal, WIN32 personality)
- C:/Users/wellons/src/skeeto.github.com/_ssg/main_win32.cpp and base.cpp (house-style exemplars to copy the foundation from)
