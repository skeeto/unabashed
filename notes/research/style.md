Report: code patterns to follow, from the user's exemplars on this machine

## 1. Multi-file C++ unity-build layout (ssg is the newest and best model)

Dir: C:/Users/wellons/src/skeeto.github.com/_ssg/

- Include chain: platform file → app core → foundation. Each executable is one translation unit (TU).
  - `main_win32.cpp:8-9`: `#include "ssg.cpp"` then `#include "wide.cpp"`
  - `main_posix.cpp`: system headers (`<dirent.h>`, `<sys/mman.h>`…) then `#include "ssg.cpp"`
  - `main_test.cpp`: `<stdio.h>` and `<stdlib.h>`, then `#include "ssg.cpp"`, then `#include "test/*_test.cpp"`
  - `ssg.cpp:7`: `#include "base.cpp"`. Then the Platform API block (lines 10-46), then shared diagnostics (`Log`, `warn`, `error`), then the feature modules at lines 87-92: `highlight.cpp`, `markdown.cpp`, `frontmatter.cpp`, `site.cpp`, `newpost.cpp`, `templates.cpp`. After that come `build()`, `watch()`, `usage` and the entry point `ssg_main(Os *, Arena mem, Str *args, i32 nargs)` at line 309.
  - The feature modules have no includes or guards. They rely on being included in order.
  - `wide.cpp` is the "portable half of the Win32 layer". It makes no Win32 calls, so `test/wide_test.cpp` includes `../wide.cpp` and tests it on every host.
- Platform API table, `ssg.cpp:10-46`:
  - Forward `struct Os;`.
  - Shared types (`Dirent` linked list, a `COPY_*` enum).
  - One commented `static` prototype per function. All take `Os *` first: `os_read(Os *, Arena *, Str path)`, `os_write(Os *, Arena, Str, Str)`, `os_mkdirs`, `os_list`, `os_copy`, `os_now`, `os_clock`, `os_print(Os *, i32 fd, Str)`, `os_sleep`.
  - `os_oom` is declared `[[noreturn]] static void os_oom();` in `base.cpp:26`, because `alloc` needs it, and it takes no `Os`.
- Arena-passing convention (consistent everywhere):
  - `Arena *` means results persist (perm).
  - `Arena` by value means scratch, freed automatically on return: `os_write(Os *, Arena scratch, …)`.
  - Functions copy `Arena tmp = *a;` for temporary allocations that must not persist (`os_read`, `main_win32.cpp:183`).
- Each `main_*.cpp` defines `struct Os {};`. It is empty in ssg; u-config/rexxd put state in it.
  - The entry point allocates one big arena and calls the app main:
    - Win32: `VirtualAlloc` 1<<28 with `MEM_COMMIT|MEM_RESERVE`.
    - POSIX: `mmap MAP_PRIVATE|MAP_ANON`.
    - Test: `malloc`.
  - `ssg_main` splits memory at lines 312-316: perm = first 3/4, scratch = last 1/4.
  - `watch()` (lines 266-294) rebuilds forever "from the same arenas, so memory use does not grow". It does `Arena p = perm;` each iteration. This is the same per-command arena-reset pattern wanted for the shell.
  - `EMIT` macro (lines 154-159): `Arena tmp = scratch; Buf b(&tmp, 1<<16); call; emit(...)`.
- Amalgamation precedent:
  - `C:/Users/wellons/src/sshd/Makefile` builds `sshd.cpp` by `cat`-ing `base/crypto/ssh/sftp.cpp`, then `grep -vE '^#include "(base|crypto|ssh|sftp)\.cpp"$$' src/win32.cpp`. It prepends a header that carries the build line `// $ cc -nostartfiles -o sshd.exe sshd.cpp -lws2_32`.
  - u-config `Makefile:77-84` does the same with awk.
  - Useful for building on the XP laptop.

## 2. Foundation (base.cpp), key excerpts

`base.cpp:4-15`:
```cpp
using u8 = unsigned char;  using b32 = decltype(0);  using i32 = decltype(0);
using u32 = decltype(0u);  using i64 = decltype(0ll); using u64 = decltype(0ull);
using iz = decltype((u8 *)0 - (u8 *)0);  using uz = decltype(sizeof(0));
using byte = decltype('0');
#define assert(c)   while (!(c)) __builtin_unreachable()
#define countof(a)  (iz)(sizeof(a) / sizeof(*(a)))
template<typename T> void *operator new(uz, T *p) noexcept { return p; }
```
- sshd's `base.cpp` adds `i8`, `u16`, `i16`, and `c16 = char16_t`.
- ssg's `wide.cpp:6` uses `using c16 = decltype(u'0');`.

Arena and alloc (`base.cpp:20-53`):
- `struct Arena { byte *beg = {}; byte *end = {}; };`
- `template<typename T> static T *alloc(Arena *a, iz count = 1)`: pads with `-(uz)a->beg & (alignof(T)-1)`. On `count >= (end-beg-pad)/size` it calls `os_oom()`. It placement-news each element, so default member initializers apply.
- `allocbytes(Arena*, iz)` does not zero.
- `copybytes` wraps `__builtin_memcpy` and guards len 0.

Str (`base.cpp:63-87`):
```cpp
struct Str {
    union { u8 *data = 0; char const *cdata; };
    iz len = 0;
    Str() = default;
    constexpr Str(u8 *data, iz len) : data{data}, len{len} {}
    template<iz N> constexpr Str(char const (&s)[N]) : cdata{s}, len{N-1} {}
    u8 &operator[](iz i) { assert(i >= 0 && i < len); return data[i]; }
    bool operator==(Str s) { return len==s.len && (!len || !__builtin_memcmp(data, s.data, (uz)len)); }
};
```
- Helpers: `span`, `takehead`, `cuthead`, `taketail`, `cuttail`, `startswith`, `endswith`, `compare`, `struct Cut {Str head, tail; b32 ok;}` with `cut(Str, u8)`, `find`, `trim*`, `clone`.
- `concat(Arena*, head, tail)` extends in place when head ends at `a->beg` (lines 221-228).
- `hash`: `h=0x100; h^=c; h*=1111111111111111111u`.
- `utf8decode(Str, iz *i)`: invalid bytes decode as Latin-1, one byte at a time. `utf8prev` walks backward, which a line editor will need.

Slice (`base.cpp:283-313`):
```cpp
template<typename T> struct Slice { T *data = {}; iz len = {}; iz cap = {}; T &operator[](iz i){assert(...);return data[i];} };
template<typename T> static Slice<T> push(Arena *a, Slice<T> s, T v)
{   if (s.len == s.cap) {
        if (!s.data || (byte *)(s.data+s.cap) != a->beg) { T *data = alloc<T>(a, s.cap); /*copy*/ s.data = data; }
        iz extend = s.cap ? s.cap : 8;  alloc<T>(a, extend);  s.cap += extend; }
    s.data[s.len++] = v;  return s; }
```

Map, a 4-ary hash trie (`base.cpp:318-340`):
```cpp
template<typename V> struct Map { Map *child[4]; Str key; V val; };
template<typename V> static V *upsert(Map<V> **m, Str key, Arena *a)
{   for (u64 h = hash(key); *m; h <<= 2) {
        if ((*m)->key == key) return &(*m)->val;
        m = &(*m)->child[h>>62]; }
    if (!a) return 0;                       // null arena = lookup only
    *m = alloc<Map<V>>(a); (*m)->key = key; return &(*m)->val; }
```

Buf, an output builder that grows in the arena (`base.cpp:345-460`):
```cpp
struct Buf { u8 *data = {}; iz len = {}; iz cap = {}; Arena *a = {};
    Buf() = default;
    Buf(Arena *a, iz cap = 1<<12) : data{allocbytes(a, cap)}, cap{cap}, a{a} {} };
```
- `reserve` extends in place if `data+cap == a->beg`, otherwise copies.
- Overloads: `print(Buf*, Str)`, `print(Buf*, i64)` (negative-accumulator itoa), `putbyte`, `printpad`, `printhex`, `printhtml`, `printattr`.
- `finish(Buf*) -> Str`.
- Stable merge sort: `sort(T*, iz, b32(*less)(T*,T*), Arena scratch)`.

Older C variants, for contrast:
- u-config `src/u-config.c:7-37`: `s8 {u8 *s; iz len}`, `S("..")` macro, `new(a,t,n)`. `arena {beg, end, os *ctx}` carries the platform context so `oom()` can reach `os_write` and `os_fail`. `alloc` takes from the END (`a->end -= total+alignment`, 8-byte aligned, lines 142-153).
- rexxd/uuidgen/peports: `new(a, n, t)` → `alloc(a, n, sizeof(t), _Alignof(t))`, `affirm(c)`, and a `maxof(t)` macro.

## 3. Win32 layer conventions (`main_win32.cpp`)

- Header comment (lines 1-7) says "no CRT, no headers, and kernel32.dll the only import". It carries the build line `// $ cmake -S _ssg -B _ssg/build-w32 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake` and the path semantics.
- Constants live in `enum : i32 { … }`, alphabetized, `GENERIC_READ = (i32)0x80000000`. A separate `enum : uz { INVALID_HANDLE_VALUE = (uz)-1 }`.
- Structs are hand-defined with `static_assert` sizes: `FileAttributes == 36`, `FindData == 592` (WIN32_FIND_DATAW).
- Handles are `uz` and booleans are `b32`.
- Import macro (line 70):
```cpp
#define W32(r, p) extern "C" __declspec(dllimport) r __stdcall p noexcept
W32(b32,    CloseHandle(uz));
W32(uz,     CreateFileW(c16 *, i32, i32, uz, i32, i32, uz));
W32(void,   ExitProcess[[noreturn]](i32));
W32(b32,    WriteConsoleW(uz, c16 *, i32, i32 *, uz));
```
- Alternative W32 spellings:
  - `threads.c` and `uuidgen.c`: `#define W32 [[gnu::dllimport, gnu::stdcall]]` used as `W32 uz CreateThread(...)`.
  - u-config `src/miniwin32.h`: `#define W32(r) __declspec(dllimport) r __stdcall`.
- memcpy/memset/memchr/memcmp (lines 97-136): defined under `#if defined(__x86_64__) || defined(__i386__)` as `extern "C"` functions using `rep movsb` / `rep stosb`, because "GCC emits calls to these even without a CRT".
  - w64devkit also ships `libmemory.a` (`src/libmemory.c`: memset, memcpy, memmove, memcmp, strlen via string instructions). sshd links `-lmemory -lchkstk -lgcc` instead of defining them.
- Entry point (lines 393-410):
```cpp
extern "C" __attribute((force_align_arg_pointer))
void mainCRTStartup()
{   iz cap = (iz)1<<28;
    byte *mem = VirtualAlloc(0, cap, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!mem) os_oom();
    Arena a = {mem, mem+cap};
    c16 *cmd = GetCommandLineW(); iz len = 0; for (; cmd[len]; len++) {}
    Slice<Str> args = splitcmdline(&a, fromwide(&a, cmd, len));
    Os os; ExitProcess(ssg_main(&os, a, args.data, (i32)args.len)); }
```
  `force_align_arg_pointer` matters for i686.
- Example os_ function, console output (lines 369-391):
```cpp
static b32 os_print(Os *, i32 fd, Str s)
{   uz h = GetStdHandle(-10 - fd); i32 mode = 0;
    if (!GetConsoleMode(h, &mode)) return writeall(h, s);
    // Consoles display UTF-16 correctly regardless of code page.
    c16 buf[512]; i32 len = 0;
    for (iz i = 0; i < s.len;) {
        len += (i32)utf16encode(buf+len, utf8decode(s, &i));
        if (len>countof(buf)-2 || i==s.len) {
            i32 dummy; if (!WriteConsoleW(h, buf, len, &dummy, 0)) return 0;
            len = 0; } }
    return 1; }
```
- Caveats from u-config `main_windows.c:488-502`:
  - A multi-byte code point split across two `os_write` calls gets mistranslated.
  - Fix it by buffering up to 3 bytes of decoder state per handle and flushing at exit. A shell that writes byte-by-byte must do this.
- I/O helpers:
  - `chunk()` bounds each ReadFile/WriteFile to 1<<21.
  - `writeall` loops.
  - `os_read` checks `GetFileType==FILE_TYPE_DISK` and `GetFileSizeEx`, then reads exactly that size.
- Paths, `widepath()` (lines 151-167):
  - Calls `GetFullPathNameW(rel, cap, buf+6, 0)` into the free arena.
  - Then `extendpath()` (`wide.cpp:141-160`) prepends `\\?\` or `\\?\UNC\` in the 6 reserved units.
  - Falls back to the plain conversion on failure. `rootlen()` is at `wide.cpp:162-182`.
- XP compatibility in `os_list` (lines 285-290): retries `FindFirstFileExW` with `FindExInfoStandard` and no LARGE_FETCH when it gets `ERROR_INVALID_PARAMETER` ("Before Windows 7").
  - It skips `FILE_ATTRIBUTE_DEVICE` entries and name-surrogate reparse points.
  - uuidgen (`uuidgen.c:458-464`) looks up optional APIs at run time: `LoadLibraryA("advapi32")` + `GetProcAddress("SystemFunction036")`.
- UTF-16 / WTF-8 (`wide.cpp`):
  - `utf8encode`.
  - `utf16encode`: surrogate code points from WTF-8 encode as themselves.
  - `towide(Arena*, Str) -> Str16`: null-terminated, allocates `s.len+1` units.
  - `fromwide(Arena*, c16*, iz)`: allocates 3*len, pairs surrogates, leaves unpaired ones as WTF-8, then returns the unused tail (`a->beg = r.data+r.len`).
- Command line, `splitcmdline(Arena*, Str cmd)` (`wide.cpp:84-139`):
  - Copies `CommandLineToArgvW` quirks: backslashes are literal except before a quote, `""` inside quotes yields a literal quote and ends quoting, argv[0] has no escapes.
  - Unescapes in place in the UTF-8 string and returns `Slice<Str>`.
  - Older tools (rexxd 2917-2925, uuidgen) call `CommandLineToArgvW` (a shell32 dependency) + `WideCharToMultiByte`. u-config uses its own `src/cmdline.c` (`cmdline_to_argv8`).
- Environment: u-config `main_windows.c:217-243` uses `GetEnvironmentVariableW(name,0,0)` for the length (0 = unset, 1 = empty). It stores the wide value temporarily at `perm->beg`, converts, then restores `beg`.

## 4. Process spawning precedent (sshd, XP-capable)

`C:/Users/wellons/src/sshd/src/win32.cpp:842-925`:
- Builds the command line with `append_arg` and a `CREATE_UNICODE_ENVIRONMENT` env block (`build_env`).
- Calls `CreatePipe` with inheritable SECURITY_ATTRIBUTES, then `SetHandleInformation(HANDLE_FLAG_INHERIT, 0)` on the parent's ends.
- Does all of this under a `CRITICAL_SECTION spawn_lock`, so concurrent spawns don't leak inheritable handles.
- `CreateProcessW(... CREATE_SUSPENDED ...)`, then `CreateJobObjectW` with `JOB_OBJECT_LIMIT_BREAKAWAY_OK` (0x800), because "XP cannot nest jobs". Then `AssignProcessToJobObject`, `ResumeThread`.
- Kills the tree with `TerminateJobObject` (`proc_kill`, line 1010).
- Polls with `PeekNamedPipe`.
- Feeds stdin from a writer thread (`CreateThread` + ring buffer) to avoid deadlock.
- Imports are kernel32, ws2_32, advapi32 and shell32. Struct names: `StartupInfo`, `ProcessInfo`, `JobLimits`, `SecurityAttributes`.
- sshd runs `sh.exe -c cmd` for commands and `sh -i` on pipes for pty requests (`README.md:96-113`). Its default `-s sh.exe` is found next to `sshd.exe` / `bin\`. So a replacement `sh` must handle `-i` on non-console stdin: prompts printed, no line editor, line-at-a-time reads. sshd's Ctrl-C there terminates processes started since the last line.

## 5. Test layers and virtual OS

**ssg `main_test.cpp`** (simplest; it stubs I/O, it does not virtualize):
- Lines 8-21 stub every `os_*`: return `{}`/0, `os_print` → `fwrite`, `os_oom` → `__builtin_trap()`.
- Lines 23-50:
```cpp
struct Test { Arena perm; i32 run; i32 failed; };
static b32 expect(Test *t, Str name, Str got, Str want)
{   t->run++;
    if (got == want) return 1;
    t->failed++;
    fprintf(stderr, "FAIL: "); printstr(stderr, name);
    fprintf(stderr, "\n  got:  ["); printstr(stderr, got);
    fprintf(stderr, "]\n  want: ["); printstr(stderr, want);
    fprintf(stderr, "]\n"); return 0; }
```
- `main` mallocs 1<<28, calls `test_site(&t)` etc., and prints "%d tests, %d failed".
- Each test does `Arena a = t->perm;`. Test files hold helpers like `render(Arena*, fn, v) -> Str`, and use `wide(Arena*, u"...")` / `hex16` / `splitshow` (`test/wide_test.cpp`).

**u-config `main_test.c`** (the user's named model, a virtual FS):
```c
struct os { jmp_buf exit; arena perm; s8 outbuf; s8 output; s8 outavail; env *filesystem; b32 active; };
static void os_fail(os *ctx) { assert(ctx->active); ctx->active = 0; longjmp(ctx->exit, 1); }
static filemap os_mapfile(os *ctx, arena *perm, s8 path)
{   ... path.len--;  // trim null terminator
    s8 *data = insert(&ctx->filesystem, path, 0);      // hash-trie lookup only
    if (!data) { r.status = filemap_NOTFOUND; return r; }
    r.data = *data; r.status = filemap_OK; return r; }
```
- The filesystem is the same 4-ary hash trie `env` (`src/u-config.c:445-469`), mapping full path string → contents.
- `newfile_` (line 203) does `*insert(&ctx->filesystem, path, &conf->perm) = contents;`. Overwriting is allowed.
- `os_listing` (122-143) walks the whole trie, collects names with a `startswith(dir)` prefix, and merge-sorts "for determinism".
- `os_write` (145-151) appends into the fixed `outbuf` and asserts there is capacity.
- `run(config, ...)` (209-237):
  - Takes a varargs `s8` list terminated by `E` (= `S("")`).
  - Resets output.
  - `fillbytes(conf.perm.beg, 0x55, …)` poisons the arena to expose uninitialized reads.
  - Sets `active=1`, calls `uconfig`, asserts still active.
- Macros (17-40):
```c
#define SHOULDPASS for (i32 r = setjmp(conf.perm.ctx->exit); !r || (r>0 && (__builtin_trap(), 0)); r = -1)
#define SHOULDFAIL for (i32 r = setjmp(conf.perm.ctx->exit); !r; __builtin_trap())
#define EXPECT(w)  if (!s8equals(ctx->output, S(w))) { printf("EXPECT..."); __builtin_trap(); }
#define MATCH(w)   /* substring search via rolling hash s8find_ */
```
- Each `test_*(arena a)` takes the arena by value. `main` prints "all tests pass".
- Build: `$(CC) -g3 -Wall -Wextra -Wconversion -fsanitize=undefined -fsanitize-undefined-trap-on-error -Wno-clobbered -o tests main_test.c` (Makefile 90-94).
- There are OOM tests: `test_lol` (billion laughs) is expected to `MATCH("out of memory")`. A test arena of 1<<21 makes OOM reachable.

**rexxd `-DTEST`** (`w64devkit/src/rexxd.c:1745-2290`), a richer virtual stdio:
- `struct Plt` (1754-1770) holds: output with a seek offset, `cap` to simulate ENOSPC, `outpipe`/`inpipe` flags that make seeks fail like pipes, `pipebuf = 80` to force short reads, `status`, and `jmp_buf *oom` ("pointer hides ugly GDB printout").
- `plt_exit` does `longjmp(*plt->oom, r)`, so exit status travels through longjmp.
- `expect(r, s, ...)` macro (1885-1895) builds `argv` from `__VA_ARGS__`, `setjmp`s, runs `xxd(argc, argv, plt, a.beg, a.end-a.beg)`, then `affirm`s status and output.
- The header comment says: "All tests run on a virtual file system… main program is also run repeatedly in the same process with the environment reset between tests."
- One file holds every platform, selected by `#if NO_PLATFORM / TEST / RANDTEST / __AFL_COMPILER / BENCH / DLL / _WIN32 / __linux && __amd64 && !__STDC_HOSTED__ / else POSIX`.
- The file header lists build lines for w64dk/unix/bench/tests/fuzz (lines 8-12):
  - `// tests: $ cc -DTEST -g3 -fsanitize=undefined -fsanitize-trap rexxd.c`
  - `// fuzz:  $ afl-gcc-fast -g3 -fsanitize=undefined -fsanitize-trap rexxd.c`
- The app entry takes raw memory: `xxd(i32, u8 **, Plt *, byte *, iz)`.

**Non-local error escape for non-OOM errors** (peports.c:43-75):
- `typedef struct { void *jmp[5]; s8 err; } escape;`
- `throw(escape*, s8)` → `__builtin_longjmp`.
- `#define catch(e) __builtin_setjmp((e)->jmp)`.
- The arena carries `escape *esc`.
- This maps naturally onto shell `exit` / `set -e` / syntax errors / `return` unwinding, with no destructors to skip.

## 6. Fuzz harness shape

u-config `main_fuzz.c`, build line `afl-gcc-fast -fsanitize=undefined -fsanitize-trap main_fuzz.c` / `afl-fuzz -i … -o fuzzout ./a.out`:
```c
__AFL_FUZZ_INIT();
struct os { jmp_buf ret; s8 pcfile; };
static void os_fail(os *ctx) { longjmp(ctx->ret, 1); }
static void os_write(os *ctx, i32 fd, s8 s) {}            // discard
static filemap os_mapfile(...) { r.data = ctx->pcfile; ... } // every file = test case
static s8node *os_listing(...) { assert(0); }              // forbidden in fuzz
int main(void) { __AFL_INIT(); ... perm = malloc(1<<16);
    ctx.pcfile.s = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) { ctx.pcfile.len = __AFL_FUZZ_TESTCASE_LEN;
        config conf = {0}; conf.perm = perm; ...; if (!setjmp(ctx.ret)) uconfig(&conf); } }
```
- rexxd's AFL layer (2421-2476) is the same shape but uses `uz oom[5]` + `__builtin_setjmp/__builtin_longjmp`. Its `plt_open` is `affirm(0)` and its writes return 1.
- This matches the user's requirement: in the fuzz layer, any spawn or real file access should be `affirm(0)` / trap, or go to a virtual FS. Never real `CreateProcess`.
- `#define FUZZTEST` before the include lets the core shrink limits.

## 7. Freestanding Linux layer

`u-config/main_linux_amd64.c` (46 lines):
- Defines a `SYS_*` enum.
- `#include "src/linux_noarch.c"` holds the shared `os_*` and `entrypoint(stack)`.
- `asm(".globl _start\n_start: mov %rsp, %rdi\n call entrypoint\n")`, plus inline `syscall1`/`syscall3`.
- `src/memory.c` defines `memcpy`/`memset` in their own sections.
- Build (Makefile 16-18): `-fno-builtin -fno-pie -fno-lto -fcf-protection=none -fno-stack-protector -fno-asynchronous-unwind-tables` + `-static -s -no-pie -nostdlib -Wl,--gc-sections`.
- rexxd 2933-3009: the same via `static byte heap[1<<24]` with `asm("" : "+r"(mem))` to launder the pointer.

## 8. Build conventions

ssg `CMakeLists.txt`:
- C++20, extensions OFF, default Release.
- `-fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-asynchronous-unwind-tables -Wall -Wextra -Wconversion -Wno-sign-conversion -Wno-unused-parameter -Wno-unused-function`.
- Debug: `-g3 -fsanitize=undefined -fsanitize-trap` (falls back to `-fsanitize-undefined-trap-on-error`), so asserts trap.
- `-nostdlib++` when supported.
- WIN32: `add_executable(ssg main_win32.cpp)` with `-nostartfiles -static-libgcc` ("helpers e.g. 64-bit division on i686") and kernel32. Otherwise `main_posix.cpp`.
- Tests are built only when not cross-compiling: `add_executable(ssg_test main_test.cpp)` + `add_test`.
- Toolchain file `cmake/mingw-w64.cmake` sets `x86_64-w64-mingw32-g++`.

sshd `Makefile`: `$(CXX) -nostdlib -O2 -fno-exceptions -fno-rtti -fno-threadsafe-statics … -o sshd.exe src/win32.cpp -lkernel32 … -lmemory -lchkstk -lgcc`. XP build: `make -B CXX=i686-w64-mingw32-g++`.

u-config `Makefile`:
- `default:` refuses to build anything.
- Separate debug/release targets per platform.
- `WIN32_LIBS = -s -nostdlib -Wl,--gc-sections -lkernel32`.
- An `install` target writes into `$(W64DEVKIT_HOME)/bin`.

Every source file starts with a one-line purpose, `$ build line(s)`, and "This is free and unencumbered software released into the public domain."

## 9. Threads

- `w64devkit/src/threads.c` / `threads.h` implement C11 threads.
  - `thrd_create` uses a `CreateThread` trampoline with an SRW-lock + condvar handshake, so the stack-allocated `Spawn` stays valid until the thread has consumed it.
  - Mutexes are SRWLOCK, `call_once` is `InitOnceExecuteOnce`, `cnd_*` are condition variables, `tss` uses `FlsAlloc`.
  - On i386 there are both 32- and 64-bit `time_t` variants (`thrd_sleep64` etc.).
  - Timeouts degrade to infinite on overflow.
- **Caution: SRW locks, condition variables, InitOnce and FLS are Vista+.** They are not available on XP, so the shell cannot rely on `threads.h` if XP support matters.
  - sshd (XP-capable) uses `CreateThread` + `CRITICAL_SECTION` + events. Follow that for any helper threads, such as a Ctrl-C watcher or pipe pumps.

## 10. winctl (`C:/Users/wellons/src/sshd/winctl.md`) and how to use it for the line editor

What it is:
- A standalone `winctl.exe` (`src/winctl.cpp`, 2140 lines) that runs on XP and later. Imports: kernel32, user32, gdi32, shell32; newer APIs are looked up at run time.
- It is driven over the user's own sshd: `ssh host winctl …`. sshd runs as the console user, so windows appear on that desktop.
- Build: `cc -nostartfiles -o winctl.exe winctl.cpp -lgdi32`. An i686 build runs everywhere.

Commands:
- `list [-a] [WINDOW]`: tab-separated hwnd, pid, X,Y, WxH, class, title.
- `shot [WINDOW [X Y W H]] > f.png`: copies screen pixels, so it includes overlays; stdout must be clean.
- `wait [-gone|-change|-idle] [-t S] WINDOW`: exit status 3 on timeout.
- `click/dclick/rclick/wheel/drag`.
- `key WINDOW CHORD...`: e.g. `ctrl+a`, `alt+f`, `up`, `home`, `f1`-`f24`; letters never imply shift.
- `type WINDOW TEXT|-`: sends `KEYEVENTF_UNICODE`; newline becomes Enter, tab becomes Tab; `-` reads UTF-8 from stdin.
- `raise`, `cmd`, `close`, `move`.
- `run PROGRAM [ARGS]`: detached, outside sshd's job object. **A console program gets its own console window** (`CREATE_NEW_CONSOLE`, winctl.cpp:1958). Prints the PID.
- `ls`.
- WINDOW selectors: title substring, `class=`, `pid=`, `hwnd=`, comma-combined, `screen`, `active`.
- Exit status: 0 ok, 1 error, 2 no match or ambiguous, 3 timeout.

Pitfalls it documents:
- Two clicks close together become a double-click.
- Something else may be on top of the target.
- Never kill by image name; close only what you started.
- Programs persist settings (registry, %APPDATA%), so restore them.
- DPI: 150% on this machine, 100% on the XP laptop. Take coordinates only from a `shot` on the same machine.
- On XP, non-ASCII arguments pass through the ANSI code page: pipe UTF-8 into `type W -` from the local side.
- `wait` instead of sleeping.
- The PID from `run` may not own the window.
- The desktop must be unlocked; elevated windows refuse input.
- On XP, home is `C:/Documents and Settings/USER`, so quote paths.

Proposed use for testing the built-in readline replacement in a real console:
1. Run `ssh host ./winctl.exe run unabashed.exe -i`, which prints a PID, then `wait pid=N`.
   - On Windows 11 the default terminal may be Windows Terminal, in which case the console window is not owned by that PID (the documented pitfall).
   - Then wait by class. Not verified here, from general knowledge: conhost's class is `ConsoleWindowClass`, Windows Terminal's is `CASCADIA_HOSTING_WINDOW_CLASS`.
   - Alternatively launch `conhost.exe unabashed.exe` explicitly to force classic conhost.
   - On XP it is always conhost (via csrss).
2. Type text: `type pid=N "echo hello"`, or `printf 'ü✓\n' | ssh host winctl type pid=N -` for Unicode.
3. Edit keys: `key pid=N ctrl+a ctrl+k`, `key pid=N up up enter`, `key pid=N tab tab` for completion, plus `alt+b`, `ctrl+r`, `esc`, `left`, `home`, and so on.
4. Verify with `shot pid=N 0 0 W H > s.png` (cropped, which avoids scaling), viewed with the Read tool.
   - For assertions that don't depend on reading pixels, have the shell's test build (or a hidden builtin/env flag) dump the console buffer via `ReadConsoleOutputCharacterW` to a file. Then `ssh host cat` it. This is a suggestion; no such tool exists yet.
5. Implication for the input layer: `KEYEVENTF_UNICODE` input reaches `ReadConsoleInputW` as KEY_EVENT records with `wVirtualKeyCode == VK_PACKET` (0xE7) and the character in `uChar.UnicodeChar`. Possibly a surrogate pair split across two events.
   - The editor must key off UnicodeChar for text and off VKs and modifier state for chords.
   - This matches how pasted text and IME input arrive, so it is a realistic test.
6. Clean up: `close pid=N` or type `exit`, then `wait -gone pid=N`.
7. XP laptop: i686 build (or copy the amalgamated .cpp and build there with `cc -nostartfiles`), 100% DPI, busybox `sh` as the remote shell, quote paths.
   - XP conhost lacks VT processing, so the editor must use console APIs there (`SetConsoleCursorPosition`, `FillConsoleOutputCharacterW`, `WriteConsoleW`), not ANSI escapes.
   - Exercise both code paths: VT on Windows 10+ with ENABLE_VIRTUAL_TERMINAL_PROCESSING, and API-only on XP.

Most of the line-editor logic should still be tested headlessly. Use the virtual OS layer: a scripted sequence of key events in, and a virtual screen or byte stream out, compared with `expect()`. Use winctl only for end-to-end checks in real conhost and Windows Terminal.
