GNU bash 5.3 performance analysis: where bash is slow and how the clone can beat it.

Source root below is abbreviated as $B = C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3

======================================================================
1. PROCESS CREATION (largest cost, especially on Windows)
======================================================================

1.1 Command substitution always forks, even for builtins
- $B/subst.c:7136 command_substitute(). It calls pipe() (7208) and then make_child() for every $(...). `x=$(echo hi)` and `x=$(printf %s "$y")` each pay a full fork. Only ksh93 (virtual subshells) and mksh avoid this.
- Before forking it calls maybe_make_export_env() (7199-7200) "so children don't have to". That is an O(#vars) rebuild whenever array_needs_making is set.
- Only optimization: `$(<file)` with nothing else. command_substitute parses the string (7157), checks can_optimize_cat_file ($B/builtins/evalstring.c:199-208: a single r_input_direction redirect, no words), and reads the file in-process through optimize_cat_file/read_comsub ($B/subst.c:6664, 6680). `$(cat file)` and `$(< file; true)` still fork.
- Comsub bodies are re-parsed on every evaluation. At parse time parse_comsub ($B/parse.y:4451) parses the body and then print_comsub turns it back into text (parse.y:4632). At expansion time extract_command_subst ($B/subst.c:1260-1322) calls xparse_dolparen (parse.y:4686), which parses it again to find the closing paren. The child then runs parse_and_execute on the string, a third parse. So a $(...) inside a loop is parsed at least twice per iteration on top of the fork.
- read_comsub reads the pipe in 4096-byte chunks (COMSUB_PIPEBUF, subst.c:6638) and copies byte by byte. Per byte it checks CTLESC quoting and, in UTF-8 locales, does multibyte handling (6731-6739).
- Buffer growth is linear: RESIZE_MALLOCED_BUFFER ($B/general.h:181-189) does `csize += sincr` (512 for comsub). Capturing N bytes therefore costs O(N^2/512) in realloc copies, partly hidden by realloc growing in place.

1.2 Bash 5.3 nofork function substitution, ${ cmd; } and ${| cmd; }
- $B/subst.c:6923 function_substitute(). It does not fork. It pushes a function context (push_context, about line 7007), runs the string with parse_and_execute, and captures stdout into an anonymous file: anonopen ($B/lib/sh/anonfile.c:104) uses memfd_create, or an unlinked temp file as fallback. It then lseeks back and read_comsub's that file. That is still kernel I/O plus a re-parse of the text on every evaluation.
- ${| cmd; } returns $REPLY and needs no capture.
- Clone: run builtin output straight into an arena buffer (an in-memory "fd") with no kernel round trip.

1.3 Fork-avoidance bash already does (we must match or beat it)
- $B/builtins/evalstring.c:106 should_optimize_fork() covers simple commands with no EXIT/ERR/RETURN traps, no trapped signals, no `time`, and no `!`.
- should_suppress_fork (125): `bash -c 'cmd'` execs the last simple command directly. startup_state==2 has been extended to comsubs and procsubs.
- optimize_connection_fork (148): the last command after &&, || or ; in -c strings and subshells.
- optimize_subshell_command (162): `( simple )` execs directly.
- optimize_shell_function (180): the last command in a function run in a subshell.
- CMD_NO_FORK handling: $B/execute_cmd.c:663, 932, 1803, 4553, 4927 (disabled while procsub fifos are pending), 5220, 5782.
- lastpipe (shopt; execute_cmd.c:2735) runs the last pipeline element in the current shell, but only with job control off.
- Every other pipeline element forks, even pure builtins: `echo foo | read x`, `printf ... | while read`. `( ... )` with more than a simple command forks.

1.4 Per-fork overhead inside bash
- make_child ($B/jobs.c:2265, fork loop at 2305) blocks signals, does job/process-table bookkeeping (bgpids storage, bgp_* functions around jobs.c:767-838, DEFAULT_CHILD_MAX 4096), sets the pgrp, and handles SIGCHLD.
- execute_simple_command does savestring(the_printed_command) for the job's command text (execute_cmd.c:4549).

1.5 Windows: MSYS2/Cygwin
- fork() is emulated. A new process is created, then the parent's data, bss, heap and stack are copied into it through the Cygwin DLL, with DLL-base fixups. It costs milliseconds per fork, compared with sub-100 µs on Linux, and gets worse with AV/EDR scanning, AD lookups (Git for Windows #4459, a 2.41 regression from querying Active Directory for the current user) and long session uptime.
- Git's own mailing list measured __git_ps1 at over 400 ms on Windows vs about 26 ms on Linux, almost all of it fork/exec. Pure-bash replacements run in 1-3 ms.
- Exec of a native (non-msys) program also passes through msys2-runtime's heuristic argv/env path conversion (/c/x -> C:\x, PATH lists, MSYS2_ARG_CONV_EXCL), adding per-exec string work.
- Git Bash startup: msys-2.0.dll init (fstab, passwd/group or AD lookups, env conversion), then /etc/profile and /etc/profile.d/*.sh, git-prompt.sh and bash-completion, all of which run subshells and comsubs.
- busybox-w32 ash has no fork. Its "forkshell" spawns a new sh.exe and copies a serialized snapshot of shell state into it, so every subshell, comsub or builtin-in-pipeline costs a full CreateProcess.
- Design consequence: the biggest Windows win is never creating a process except to run an external program. That means:
  - comsubs, subshells and builtin pipeline stages run in-process as "virtual subshells": save the arena watermark and copy-on-write the variable/function/cwd/fd/trap state, then restore;
  - pipelines of builtins or functions run either sequentially through in-memory buffers when semantics allow, or on threads, each with its own shell-state arena;
  - CreateProcess is reserved for external commands;
  - the env block is cached and rebuilt only when exported state changes.

======================================================================
2. VARIABLES, SCOPING, HASH TABLES
======================================================================

- Hash table: $B/hashlib.c. Chained buckets, each a malloc'd BUCKET_CONTENTS (hash_search 238-275, xmalloc at 266).
- Hash function: 32-bit FNV-1a over a NUL-terminated string (hashlib.c:192-216). It is recomputed on every hash_search call (HASH_BUCKET macro, hashlib.c:51), and nothing is interned.
- 5.3 added rehash: grow 4x when entries >= 2x buckets (hashlib.c:38-47, 123-169). The 5.3 hash_copy (hashlib.c:95-121) savestring's every key and value.
- Table sizes: globals 1024 buckets, functions 512, tempenv and function locals 4 (variables.c:82-84, 4560), assoc arrays 1024 (assoc.h:28).
- Dynamic scoping is a chain of VAR_CONTEXTs (deep binding). var_lookup ($B/variables.c:1942-1953) walks vc->down and does a hash_search, which re-hashes the name, for every context that has a table. A global read at call depth d costs up to d hashes plus probes. Empty contexts are skipped cheaply (table==0, or HASH_ENTRIES==0 in hash_search:244).
- find_variable_internal (variables.c:1963) also checks temporary_env first in subshell and function cases.
- push_var_context (variables.c:5215) allocates a context per function call and flattens tempvars into it.
- Values are always char* strings, including `declare -i` integers. Every arithmetic assignment converts with itos, then xmalloc's the new string and frees the old.
- x+=y: make_variable_value ($B/variables.c:2985-2995) xmallocs olen+len+1 and strcpy's the old value each time. Building a string in N appends is O(N^2) bytes copied, plus N malloc/free pairs.
- Export env: maybe_make_export_env (variables.c:5065-5133) flushes and rebuilds the whole envp from every context whenever array_needs_making is set (set from about 30 sites). It is called before every fork (execute_cmd.c:4544).
- Clone design:
  - Intern identifiers at parse time with a precomputed hash and a direct symbol pointer.
  - Use shallow binding: one global table where each entry keeps a stack of saved shadowed values; `local` pushes, function return pops. Lookup is O(1) regardless of depth.
  - Use an MSI hash trie or open addressing in the arena.
  - Store integer-attributed values natively as int64 and produce the string lazily.
  - Do appends in place when the value is the last allocation in its arena (skeeto concat-at-tip), or use per-variable geometric capacity.
  - Keep the flat env block up to date incrementally.

======================================================================
3. ARRAYS
======================================================================

- Indexed arrays: a doubly-linked circular list sorted by index ($B/array.c). The only accelerator is a single roving "lastref" pointer (array.c:68-78).
- array_reference (array.c:657-703) starts at lastref or at the head (heuristic `i < startind/2`, line 669) and walks linearly. Sequential forward or backward access is amortized O(1). Random access, or alternating between two distant indices, is O(n) per access, so loops with random access are O(n^2).
- Append past max_index is O(1) (array.c:525-541). Insertion in the middle walks the list.
- array_push/array_pop (array.h:155-157) are array_rshift/shift. They renumber every element (array.c:260-277: `element_index(ae) += n` loop).
- One malloc per element plus one per value string.
- --enable-alt-array-implementation ($B/array2.c, configure.ac:219,371, CHANGES "There is an alternate array implementation... optimizes access speed over memory use") uses a flat vector, but it is not the default. Distro bash uses the linked list.
- Assoc arrays are plain hash tables (assoc.h:33 assoc_create == hash_create, 1024 buckets) with the same malloc and rehash costs. Iteration order follows bucket order, which we must reproduce if we want byte-identical `${!a[@]}` output. That is worth checking against bash in tests.
- Clone: use a dense vector when the array is dense, a sorted sparse representation (hash trie of index to value, or chunked blocks) when sparse, and arena storage. For assoc iteration order, decide whether to mimic bash's FNV bucket order. "Faithful" probably means yes for common small cases.

======================================================================
4. PER-COMMAND AND FUNCTION CALL OVERHEAD
======================================================================

- Every simple command is pretty-printed to a string: execute_simple_command ($B/execute_cmd.c:4489-4502) runs print_simple_command, then savestring into the_printed_command_except_trap. This happens even with no DEBUG trap and no xtrace.
- Every command also does bind_lastarg ($_, execute_cmd.c:4188, 4943), which is a variable assignment with malloc.
- Function call, execute_function ($B/execute_cmd.c:5181):
  - copy_command(function_cell(var)) at line 5214 deep-copies the whole function body tree on every call, with a malloc per node, word and string. It is freed later through an unwind-protect (5246).
  - It pushes FUNCNAME, BASH_SOURCE and BASH_LINENO with array_push (5311-5316). That is O(depth) renumbering per call, so deep recursion is O(d^2).
  - It sets up 5-10 unwind-protects (5226-5347), each malloc'd ($B/unwind_prot.c), and copies positional parameters with remember_args/push_args (5340-5345).
- Loops are not re-parsed. The parser builds a COMMAND tree once (make_cmd.c), and for/while/arith-for execute that tree (execute_cmd.c:2990, 3219).
- What is "re-parsed" is word text. Words are stored as strings with CTLESC/CTLNUL quoting, and expand_word_internal ($B/subst.c:11222) re-lexes each word on every expansion, including ${...} operators, $(( )) text and comsub text. Each expansion makes several malloc'd copies (quote_string 4771, dequote_string 4805, remove_quoted_nulls 4992, quote_escapes 4620) plus WORD_DESC and WORD_LIST nodes.
- Arithmetic, $B/expr.c:
  - evalexp (417) and subexpr (459) savestring the expression and tokenize it on every evaluation (readtok 1319, savestring of each token 1377).
  - Evaluation is interleaved recursive descent with no AST and no cache.
  - Each variable reference runs find_variable, then expr_streval (1150), which recursively evaluates the variable's string value as an expression. Assignments go through itos, bind_variable and malloc.
  - So `((i++))` means: parse, look up, recursively parse the value, compute, format, malloc, free.
- Clone:
  - Pre-parse words into expansion node trees at parse time.
  - Compile literal $((...))/((...)) bodies to an AST once. Only bodies containing $-expansions need runtime parsing, and they can be cached by text.
  - Keep int variables native.
  - Don't copy function bodies: the tree is immutable, so reference-count or arena-pin it for redefinition-during-execution.
  - Maintain FUNCNAME/BASH_LINENO as a stack and materialize the arrays lazily.
  - Do no per-command printing unless a DEBUG trap, xtrace or a job needs the text.
  - Reset the per-command scratch arena after each simple command, as in the shellexec idea: allocations happen through a by-value scratch arena and vanish on return.

======================================================================
5. PATTERN MATCHING, ${var//pat/rep}, SUBSTRINGS, WORD SPLITTING
======================================================================

- pat_subst ($B/subst.c:9177-9306) loops calling match_pattern(str, ...) once per replacement, from the current position.
- match_upattern (5328-5473):
  - First it wraps the pattern in `*...*` (xmalloc, 5349) and runs a full strmatch over the remaining string just to test for a match (5380). That is O(remaining) per replacement.
  - MATCH_ANY (5395-5429): for each start position p, if the first character can match (match_pattern_char), it tries every end p1 from end down to p, running strmatch on each candidate with temporary NUL insertion (5411-5427). Variable-length patterns (anything with *) cost O(n^2) strmatch calls, each O(n), per replacement.
  - Fixed-length patterns are short-circuited by umatchlen (5389).
- So `${s//x/y}` with k matches in an n-byte string is at least O(n*k), and `${s//a*b/}`-style patterns can approach O(n^3).
- Chet acknowledged the matcher "traverses the string ... more than it needs to" (bug-bash 2010-09: about 10 s for one ${//} on a 7000-char string).
- Multibyte: match_pattern (5623-5661) converts both the pattern and the whole string to wchar_t with xdupmbstowcs on every call whenever either contains a non-ASCII byte (5638-5650). In a global replace that full conversion is redone on every iteration. xstrmatch ($B/lib/glob/smatch.c:585-604) does the same.
- Replacement output grows by fixed increments (RESIZE_MALLOCED_BUFFER(..., 64), 9255, 9286, 9304), and rep with & is strcreplace'd and malloc'd per match (9237-9248).
- Substrings: ${#v} and ${v:i:n} use MB_STRLEN and mb_substring (subst.c:8301-8419, 9015), which walk from the start in multibyte locales. Iterating over characters with ${s:i:1} is O(n^2).
- Word splitting: list_string (subst.c:3117) mallocs a node per word. A historic quadratic case was fixed (CHANGES:6792).
- Clone:
  - Compile glob patterns once per expansion into a small matcher: literal prefix and suffix, segment memchr, star-backtracking linear algorithm (Kirk Krauss / "wildcard" two-pointer). Precompute a literal-substring fast path, which is the most common ${//}.
  - Find the leftmost-longest match by scanning start positions with a matcher that reports all match ends in one pass, rather than O(n) separate end probes.
  - Work on UTF-8 bytes directly. Never convert whole strings to wchar_t; decode on the fly only when the pattern contains `?`, a bracket expression or case folding.
  - Track character counts lazily.

======================================================================
6. INPUT: read BUILTIN, SCRIPT INPUT, mapfile, HEREDOCS, printf
======================================================================

- read builtin ($B/builtins/read.def):
  - Unbuffered mode is chosen when input is a pipe, or when the input is a tty and -n/-d is given (663-670). It then calls zread(fd, &c, 1), one read(2) per byte (734-739).
  - Seekable files use zreadc with a static 4096-byte buffer ($B/lib/sh/zread.c:41, 191-208), then zsyncfd (read.def:937-938, zread.c:294-301) lseeks back over the unread bytes after every read. That is one extra syscall per line.
  - `while read` over `cmd | ...` or `< <(cmd)` therefore makes one syscall per byte. On Windows a ReadFile per byte is far worse.
- mapfile got fully buffered reads in more cases in 5.3 (CHANGES:1226).
- Script input: fd_to_buffered_stream ($B/input.c:380-394) uses buffer size 1 when the fd is not seekable (`bash < pipe`, `curl | bash`), so the script is read byte by byte. b_fill_buffer (488-510) falls back to unbuffered when a text-mode lseek mismatch is seen. Seekable input lseeks back on sync (562).
- Clone improvements:
  - Windows: PeekNamedPipe can look at pipe contents without consuming. Peek, find the delimiter, then ReadFile exactly that many bytes. This gives full speed with exact POSIX consumption semantics.
  - Linux: pipes the shell itself creates can be socketpairs, so recv(MSG_PEEK) works (ksh93 does this). Foreign pipes fall back to 1 byte.
  - When no other process can observe the fd (the shell-created procsub end and only builtins reading it), buffer freely.
  - Seekable files: buffer and lseek back once at the end of the builtin, as bash does.
- Heredocs: here_document_to_fd ($B/redir.c:423-554) uses a pipe if the expanded document is at most HEREDOC_PIPESIZE (PIPE_BUF, typically 4096; redir.c:63-82, 454). Otherwise it makes a temp file with sh_mktmpfd, then writes, reopens and unlinks it (492-554). The body is expanded through the full expand_string path each time.
  - Clone: for builtins and functions reading a heredoc, give them an in-memory fd. Only externals need a real pipe or file. On Windows, prefer a pipe fed by a writer thread, or an anonymous FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE file.
- printf ($B/builtins/printf.def):
  - Each conversion goes through the libc printf with a format substring (PF macro, 153-171).
  - clearerr/ferror checks run around every conversion, and fflush(stdout) runs at every builtin return (PRETURN, 98-133).
  - printf -v uses a vbuf with 64-byte rounding (1257-1260).
  - Clone: a hand-rolled formatter into an arena buffer, flushed once per builtin, or batched while stdout is not shared with a concurrently running child.

======================================================================
7. GLOBBING
======================================================================

- glob_vector ($B/lib/glob/glob.c:641, readdir at 803) strmatch'es every directory entry. Any non-ASCII name or pattern triggers per-name xdupmbstowcs (glob.c:402-414, 478-486).
- `*/` and GLOBIGNORE paths call stat() per result (glob.c:1044, 1090; GLOB_TESTNAME lstat at 525). Each match is malloc'd.
- Sorting: qsort with strvec_posixcmp, which is strcoll ($B/lib/sh/stringvec.c:152-160; pathexp.c:761, 879-881). In UTF-8 locales strcoll is expensive.
- Windows clone: FindFirstFileExW(FindExInfoBasic, FIND_FIRST_EX_LARGE_FETCH) gives attributes without a stat. Match on UTF-8 converted names in the arena.
- Collation: bash's order under LANG=en_US.UTF-8 is locale-dependent. Decide a deterministic order (byte order in the C locale; the default is ASCII range unless globasciiranges is off, $B/lib/glob/smatch.c:62,114,393) and document it. Sort with a radix or merge sort on counted strings.

======================================================================
8. MEMORY MANAGEMENT AND LOCALE
======================================================================

- Everything goes through xmalloc/xfree ($B/xmalloc.c:94). Bash can use its own malloc (--with-bash-malloc, configure.ac:114-128), but many distros disable it.
- Each simple command does dozens of mallocs: words, lists, quoted, dequoted and expanded copies, printed command, $_, the unwind-protect frames, and for functions a full body copy.
- With per-command scratch arenas and immutable parse trees in a long-lived arena, a command's whole allocation footprint becomes a pointer bump and a reset.
- Multibyte: macros in $B/include/shmbutil.h (ADVANCE_CHAR at 114-149) take an is_basic and UTF-8 ASCII fast path but fall back to mbrlen per character for any high byte. glibc mbrlen goes through locale dispatch, which is slow. Many sites do whole-string wide conversion (see sections 5 and 7).
- Clone: treat the shell's internal encoding as UTF-8 always, with an inline decoder. Support only "C/POSIX" and "UTF-8" behaviours. On Windows, convert to and from UTF-16 only at OS boundaries.

======================================================================
9. COMMAND LOOKUP
======================================================================

- search_for_command ($B/findcmd.c:348) uses the phash table (hashcmd.c), which is good.
- On Windows the cost multiplies by PATHEXT probing: each PATH directory × each extension.
- Clone:
  - Hash hits.
  - Cache directory listings per PATH entry, or probe with one GetFileAttributesW per candidate.
  - Never search PATH for builtins or functions.
  - Pre-build the CreateProcess command line in the scratch arena.
  - MSYS2-path mode: translate /c/... to C:/... with a single linear pass, only where needed (redirect targets, cd, command paths, and argv for native programs if we emulate msys arg conversion).

======================================================================
10. PROPOSED BENCHMARK SUITE
======================================================================

Each benchmark is a self-contained script. It prints a checksum so outputs can be diffed for identical results before any timing is trusted. Run every shell with no rc files: bash --norc --noprofile, or `bash script.sh`, which is non-interactive.

Micro:
B1. arith loop: `i=0; while ((i<1000000)); do ((i++)); done` and a variant `for ((i=0;i<1e6;i++)); do x=$((x+i*3%7)); done`. Tests the arith re-parse and string/int conversion.
B2. string concat: `for i in {1..200000}; do s+=x; done; echo ${#s}`, plus the `s="$s$i"` variant. Tests O(N^2) append.
B3. indexed array: fill a[i]=i for 1e6 sequentially, then sum; then 1e5 random reads (LCG indices); then reverse traversal. Exposes array.c random access.
B4. assoc array: insert 2e5 keys "k$i", look all up, `${!a[@]}` iterate, unset half.
B5. pattern substitution: build a 100 KB string. `${s//ab/x}` (literal), `${s//a*b/x}`, `${s//[0-9]/}`, `${s#*:}` in a loop, and `${s:i:1}` char iteration, each in both LC_ALL=C and C.UTF-8. Also a variant with one non-ASCII character at the end, which triggers wide conversion in bash.
B6. read loop: `while IFS= read -r l; do n=$((n+${#l})); done < bigfile` (50 MB, seekable). Same over a pipe: `cat bigfile | while read`, and `< <(cat bigfile)`. Plus `mapfile -t`.
B7. comsub in loop: `for i in {1..10000}; do x=$(echo $i); done`, also with `$(printf ...)`, `${ echo $i; }` (5.3 funsub; skip on shells without it), `$(<smallfile)`, and `$(/bin/true)` (external).
B8. pipeline of builtins: `for i in {1..5000}; do echo $i | read x; done` with lastpipe off and on, plus `printf '%s\n' {1..1e5} | while read x; do :; done`.
B9. function calls: recursive fib 24 (about 75k calls), and a call depth test that recurses to 5000 and returns (exposes O(d^2) FUNCNAME). Also 1e5 calls of a 20-line function (body copy cost).
B10. dynamic scope lookup: a global read inside a function at depth 100 with locals at each level, 1e5 reads.
B11. glob: a directory with 50k files: `a=(*); echo ${#a[@]}`, `a=(*.c)`, `a=(*/)`, `[[ $f == *.@(c|h) ]]` over the list.
B12. heredoc: 1e4 iterations of `cat <<EOF` (external) vs `read -r -d '' x <<EOF` (builtin) with a 2 KB and a 64 KB expanded body.
B13. printf/echo: `for i in {1..1e5}; do printf '%5d %s\n' $i foo; done > file` and `printf -v`.

Macro:
M1. startup: `sh -c :` × 1000, and `bash -i -c exit` with a typical .bashrc. On Windows also measure Git Bash with the stock /etc/profile, and the shell's own `-l` startup.
M2. source bash-completion (/usr/share/bash-completion/bash_completion, about 100 KB of function definitions) 50 times. Tests parse throughput and function definition.
M3. configure-style workload: run an autoconf-generated configure in a cached, prepared tree, or a synthetic generator that emits a 20k-line configure-like script (case/test/sed/comsub heavy). Measure with externals stubbed out by builtin replacements to isolate shell overhead, and also with real externals.
M4. A realistic AI-agent workload: a batch of 200 short `bash -c` commands with cd, git-style path arguments, pipes and redirects, typical of agent tool calls on Windows.
M5. Optional: bats/test-suite style runs. $B/tests/*.tests timed (run only the oracle bash on them, in a sandbox).

Measurement methodology:
- Linux: hyperfine --warmup 3 --runs 20 -N, reporting median and min. Pin with taskset -c 2 and set the performance governor. Compare against bash 5.3, built from this tarball with the default configure, and the distro bash, plus dash, mksh, ksh93 and busybox ash as reference points.
- Run each benchmark in LC_ALL=C and LC_ALL=C.UTF-8.
- Gather `perf stat -e instructions,cycles` for instructions retired, which is less noisy than wall time, plus `strace -fc` syscall counts, `/usr/bin/time -v` max RSS, and counts of fork/clone/execve.
- Windows: hyperfine works natively. Compare against Git-for-Windows bash, MSYS2 /usr/bin/bash and w64devkit busybox sh.exe.
- Exclude AV noise: run from a Defender-excluded directory, or report both with and without the exclusion.
- Count process creations with ETW / Process Monitor or a Job-object accounting query (JOBOBJECT_BASIC_ACCOUNTING_INFORMATION.TotalProcesses). Measure PeakWorkingSetSize.
- Test x86-64 and the x86 XP-compatible build separately.
- Verify identical stdout checksums across shells before timing.
- Make benchmark scripts generate their own input files into a temp dir. Never point fuzzers or benchmarks at real directories beyond that temp dir.
- Regression gate: CI runs B1-B13 on the virtual-OS layer for instruction counts, which are deterministic (cachegrind or perf instructions), and fails on regressions over 5%.

======================================================================
11. SUMMARY OF CLONE DESIGN WINS (priority order)
======================================================================

1. No processes except for external programs: in-process comsubs, subshells and builtin pipeline stages. Biggest win, and decisive on Windows.
2. Parse once: word expansion trees, compiled arithmetic, compiled patterns, comsub bodies stored as trees, function bodies never copied.
3. Shallow-bound interned variables with O(1) lookup, native ints, and in-place append.
4. A vector-backed array representation.
5. Linear-ish pattern matching on UTF-8 bytes with no wchar_t conversion.
6. Peek-based exact reads on pipes (PeekNamedPipe; socketpair+MSG_PEEK on POSIX) and buffered builtin output.
7. Per-command scratch arena reset: no malloc/free churn, and no per-command pretty-printing unless needed.
8. Cached env block, PATH/PATHEXT lookup cache, and FindFirstFileEx globbing.

Sources:
- [nullprogram: shellexec / arena example](https://nullprogram.com/blog/2025/01/19/)
- [Git for Windows #4459 (AD lookup slowdown)](https://github.com/git-for-windows/git/issues/4459)
- [Windows-Dev-Performance #15 (fork emulation, stat cost)](https://github.com/microsoft/Windows-Dev-Performance/issues/15)
- [JDK-6875240 (Cygwin fork vs exec timings)](https://bugs.openjdk.org/browse/JDK-6875240)
- [git mailing list: bash prompt speedup (400 ms vs 26 ms)](https://public-inbox.org/git/20130618193551.GB13480@xs4all.nl/T/)
- [Thomas Rast on Windows prompt cost](https://public-inbox.org/git/87ehq8its8.fsf@thomas.inf.ethz.ch/)
- [MSYS2 __git_ps1 timing gist](https://gist.github.com/Ragnoroct/c4c3bf37913afb9469d8fc8cffea5b2f)
- [bug-bash 2010-09: slow ${//} and Chet's reply](https://mail.gnu.org/archive/html/bug-bash/2010-09/msg00086.html)
- [bug-bash 2010-09 msg00078](https://lists.endsoftwarepatents.org/archive/html/bug-bash/2010-09/msg00078.html)
- [bug-bash 2016-09 slow pattern removal](https://lists.nongnu.org/archive/html/bug-bash/2016-09/msg00066.html)
- [Wooledge ReplacingStrings](https://mywiki.wooledge.org/ReplacingStrings)
