Windows platform mechanics for a CRT-free bash clone on XP–Win11: research report

Basis: mingw-w64 headers in C:/Users/wellons/w64devkit/include, the w64devkit README and Dockerfile, the bash 5.3 source tree (read only for semantics), a few web sources, and my own knowledge. Unless marked "verified", version numbers and API behaviour come from memory and should be checked against the docs. I did not write or run anything.

======================================================================
0. TARGET CONSTRAINTS
======================================================================
- Supported Windows versions (C:/Users/wellons/src/w64devkit/README.md, lines 80-92):
  - The x64 kit needs Windows 7 or later.
  - The x86 kit needs Windows XP or later and an SSE2 CPU.
  - The C11 threads.h runtime is Vista+ only (README ~line 127). Do not use it if the shell must run on XP.
- Manifest difference (Dockerfile lines ~240-265):
  - The x86 variant sets GCC_MANIFEST_FLAG=--disable-win32-utf8-manifest, so its binaries get no UTF-8 activeCodePage manifest.
  - The x64 build keeps the default UTF-8 manifest, which only takes effect on Win10 1903+.
  - A shell that uses W APIs throughout does not care about the ANSI code page, but children inherit nothing from the shell's manifest.
- XP rule: any Vista+ import linked directly into kernel32 stops the program loading on XP ("procedure entry point not found"). Resolve every newer API with GetProcAddress at startup and keep an XP fallback. Also check the PE subsystem version in the header (XP needs 5.01 or lower on x86); `peports` or `objdump -p` will show it.
- Bash already has a few Windows-related spots worth knowing, for semantics only:
  - general.h:260-276 makes ABSPATH accept "X:" and backslash on Cygwin.
  - lib/glob/smatch.c:306, 551 (ISDIRSEP).
  - lib/readline/complete.c:187 sets `_rl_completion_case_fold=1` on native _WIN32.
  - complete.c:2521 handles drive-relative completion.
  - subst.c:7214-7234 notes that passing a pipe through the std fds "can cause hangs when talking to a non-cygwin child".
  - variables.c:4860 skips the exportstr cache on Cygwin.
  - Exit status mapping: jobs.c:2961-2963 and nojobs.c:219-221 (128+WTERMSIG, else WEXITSTATUS). shell.h:63-66 defines EX_NOEXEC=126 and EX_NOTFOUND=127.

======================================================================
1. CreateProcessW
======================================================================
Signature: processthreadsapi.h:289.

Arguments and limits:
- lpCommandLine must be writable, because the kernel32 path can modify it in place.
- The command line is limited to 32767 UTF-16 units including the terminator. cmd.exe's own limit is 8191.
- Always pass a fully resolved lpApplicationName. With NULL, CreateProcess searches the exe's own directory, then the cwd, system32, and the Windows directory before PATH, and appends ".exe" only when there is no extension. That violates bash PATH semantics (findcmd.c) and is a planting risk.

Useful dwCreationFlags:
- CREATE_UNICODE_ENVIRONMENT 0x400: required with a wide environment block.
- CREATE_SUSPENDED 0x4: lets you assign the child to a job before it runs.
- CREATE_NEW_PROCESS_GROUP 0x200.
- CREATE_BREAKAWAY_FROM_JOB 0x01000000.
- CREATE_NO_WINDOW 0x08000000: when the shell has no console.
- DETACHED_PROCESS 0x8.
- EXTENDED_STARTUPINFO_PRESENT 0x80000: Vista+.

Handle inheritance:
- bInheritHandles=TRUE makes the child inherit every handle marked inheritable. Handle values are identical in the child.
- STARTF_USESTDHANDLES sets the child's hStdInput, hStdOutput and hStdError. The docs require those handles to be inheritable and bInheritHandles=TRUE.
- A closed fd (bash `>&-`) is passed as NULL or INVALID_HANDLE_VALUE.
- Before Win8, console handles are pseudo-handles (low bits == 3, e.g. 3/7/11), not kernel objects. DuplicateHandle special-cases them, so they can be duplicated and passed via STARTF_USESTDHANDLES, but they cannot be waited on or put in a HANDLE_LIST.
- Recommended discipline for a single-threaded spawner:
  1. Create every shell handle non-inheritable.
  2. Per spawn, DuplicateHandle(..., bInheritHandle=TRUE) the 0-2 handles (and any N>2).
  3. CreateProcess.
  4. Close the duplicates.
  5. Close the shell's copy of the pipe write ends immediately, or readers never see EOF.
  - Alternative: SetHandleInformation(h, HANDLE_FLAG_INHERIT, ...) to flip and unflip (handleapi.h:28, XP OK).
- Race with several spawning threads: an inheritable handle created by thread A leaks into the child thread B spawns at the same moment. The classic symptom is a pipeline that never gets EOF.
  - XP fallback: serialise the whole "dup / CreateProcess / close" sequence under one CRITICAL_SECTION. That is cheap, because spawning is not the bottleneck.
  - Vista+: PROC_THREAD_ATTRIBUTE_HANDLE_LIST (winbase.h:1747) with InitializeProcThreadAttributeList, UpdateProcThreadAttribute and STARTUPINFOEXW plus EXTENDED_STARTUPINFO_PRESENT. Listed handles must still be inheritable.
  - Known HANDLE_LIST pitfalls: on Win7 and earlier, console pseudo-handles in the list fail with ERROR_INVALID_PARAMETER (filter out `(h&3)==3`); duplicate entries can also fail on some versions.
  - Practical recommendation: keep the lock everywhere. The attribute list only adds protection against third-party code in the process, which a CRT-free shell does not have.

Passing fds beyond 0-2 (lpReserved2/cbReserved2, the MSVCRT "C runtime file inheritance" convention):
- Blob layout:
  - `int n`
  - then `n` bytes of per-fd flags
  - then `n` handles, each sizeof(intptr_t) of the child's CRT
  - cbReserved2 = 4 + n*(1+sizeof(HANDLE)). It is a WORD, so it is capped at 65535.
- Flag bits (msvcrt/ucrt internal "osfile"):
  - FOPEN 0x01
  - FEOFLAG 0x02
  - FCRLF 0x04
  - FPIPE 0x08
  - FNOINHERIT 0x10
  - FAPPEND 0x20
  - FDEV 0x40
  - FTEXT 0x80
- msvcrt.dll and UCRT both parse this blob at startup (lowio ioinit). Entries for fds 0-2 marked FOPEN override GetStdHandle, so they must agree with hStd*.
- The handles inside must be inheritable and bInheritHandles=TRUE.
- Bitness risk: the handle-slot width follows the child's CRT, so a 64-bit shell spawning a 32-bit msvcrt program, or the reverse, misreads the blob. Either inspect the child's PE Machine field and size the slots to match, or only send the blob when fds >= 3 are actually redirected. Not verified; test both directions.
- Cygwin/MSYS2 use lpReserved2 for their own child_info struct. Its first DWORD (msv_count) is deliberately zero, so an msvcrt child sees n=0. Cygwin therefore does NOT pass fds > 2 to native children.
- A Cygwin child ignores a foreign blob because it checks its magic number.
- Reference implementations to read for semantics only: gnulib windows-spawn.c (LGPL) and Python's subprocess.

======================================================================
2. ARGUMENT QUOTING
======================================================================
msvcrt / UCRT / CommandLineToArgvW rules:
- Space and tab separate arguments; `"` toggles quoted mode.
- Backslashes are literal unless followed by `"`:
  - 2n backslashes + `"` give n backslashes and a quote toggle.
  - 2n+1 backslashes + `"` give n backslashes and a literal `"`.
- `""` inside a quoted region differs between versions: post-2008 CRTs emit a literal `"` and stay quoted; older msvcrt.dll and CommandLineToArgvW behave differently. Never emit `""`.
- argv[0] is parsed specially, with no backslash escapes and quotes only. Program paths cannot contain `"`, so just wrap argv[0] in quotes when it contains spaces.
- Safe encoder: an empty argument becomes `""`. Otherwise, if the argument contains space, tab, LF, VT or `"`, wrap it in quotes, double any backslash run that precedes a `"` or the closing quote, and write `"` as `\"`.

Additional rules for Cygwin/MSYS2 children (relevant when the shell runs MSYS tools such as Git for Windows' coreutils):
- When the parent is non-Cygwin, the Cygwin runtime splits the command line itself, accepting both `'` and `"` quoting.
- It also globs unquoted wildcards (unless CYGWIN/MSYS=noglob) and expands `@file` response files.
- So always quote any argument containing `* ? [ { '` or starting with `@`. Quoting is harmless for msvcrt children.

MSYS path conversion (design point):
- Real MSYS2 bash converts POSIX-looking arguments and environment variables (/c/x becomes C:/x, `:` path lists become `;`) when it execs a non-MSYS binary.
- Controls: MSYS2_ARG_CONV_EXCL, MSYS2_ENV_CONV_EXCL, MSYS_NO_PATHCONV.
- A native shell in MSYS mode must do this conversion itself for native children, or `gcc /c/x.c` breaks.
- Cygwin's always-converted environment list: PATH, HOME, LD_LIBRARY_PATH, TMPDIR, TMP, TEMP.

cmd.exe and batch files:
- Never let CreateProcess run a .bat implicitly. It silently runs `cmd /c` with your unescaped line; this is BatBadBut, CVE-2024-24576 (Rust, Node and others, April 2024).
- Launch `%ComSpec% /d /e:on /v:off /s /c "<line>"` explicitly. /s gives predictable stripping of the outer quotes; without it cmd applies a heuristic about whether to strip them.
- Batch %1 splitting also breaks on `, ; =` and quotes, so quote arguments containing those.
- `%` cannot be escaped on a cmd command line (no `%%` there). Rust's fix:
  - quotes every argument
  - doubles inner `"`
  - replaces `%` with `%%cd:~,%`, which expands to empty
  - refuses arguments containing CR or LF
- Other cmd metacharacters: `& | < > ^ ( ) !`. `^` is inert inside quotes, but `%` still expands there.
- cmd refuses a UNC cwd, so .bat files run from a UNC directory get C:\Windows.

======================================================================
3. FINDING EXECUTABLES
======================================================================
- PATH separator:
  - In native mode the shell variable uses `;`.
  - In MSYS mode the shell variable uses `:` with /c/ paths, and must be converted to a native `;` list for children.
- MSYS bash behaviour: looks for `foo` then `foo.exe` only. Does not use PATHEXT.
- busybox-w32 behaviour: also tries .com/.exe/.bat/.cmd. Verify the exact list in busybox-w32's win32/ sources, which are not local (only patches live in C:/Users/wellons/src/w64devkit/src/busybox/).
- PATHEXT defaults:
  - XP: `.COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH`
  - Later versions add `.MSC`.
  - Suggestion: honour exe/com/bat/cmd only. Scripts like .vbs/.js would need ShellExecute.
- Shebangs: Windows does not handle `#!`. The shell must read the first line and map interpreters itself, as busybox-w32 and the MSYS runtime do. That includes `/bin/sh` mapping to itself and `/usr/bin/env X` doing a PATH lookup of X.
- The exec bit for `test -x` and PATH search has to be emulated: an executable extension, `MZ` magic, or a `#!` prefix. MSYS2's /etc/fstab uses `noacl`, so MSYS2 decides it the same way.
- App Execution Aliases (Win10 1709+, %LOCALAPPDATA%\Microsoft\WindowsApps\python.exe): zero-byte reparse points with tag IO_REPARSE_TAG_APPEXECLINK 0x8000001B. CreateProcess runs them, so do not treat them as broken links.

======================================================================
4. EXIT CODES
======================================================================
- Wait on the handle first, then call GetExitCodeProcess. The exit code is a DWORD, and STILL_ACTIVE=259 is ambiguous if read before the process ends.
- Bash semantics: $? = status & 0xFF; death by signal gives 128+N.
- Cygwin's status_exit (pinfo.cc) maps native NTSTATUS codes (from memory, verify):
  - 0xC0000005 STATUS_ACCESS_VIOLATION → SIGSEGV (139)
  - STATUS_ILLEGAL_INSTRUCTION → SIGILL
  - 0xC000013A STATUS_CONTROL_C_EXIT → SIGINT (130)
  - 0xC0000135 STATUS_DLL_NOT_FOUND → prints a "cannot open shared object file" message and exits 127
  - everything else → & 0xFF
- Other codes worth recognising:
  - MSVCRT/UCRT abort() exits with 3.
  - __fastfail exits with 0xC0000409.
- Use the same table, so that `wait` reporting and bash's "cooperative SIGINT exit" logic (jobs.c wait_sigint_handler) work.

======================================================================
5. JOB OBJECTS AND KILLING PROCESS TREES
======================================================================
Basics (Win2000+, so XP OK):
- APIs: CreateJobObjectW, AssignProcessToJobObject, TerminateJobObject.
- winnt.h:5311 JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE kills the tree if the shell dies.
- winnt.h:5310 JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK.
- Assign before the child runs: CREATE_SUSPENDED, assign, then ResumeThread. Otherwise grandchildren can escape.

Nesting:
- Nested jobs are Win8+ only. On XP and Win7, if the shell is already in a job (CI runners, IDEs, the Program Compatibility Assistant on Vista/7), AssignProcessToJobObject fails with ERROR_ACCESS_DENIED.
- Fallbacks:
  - CREATE_BREAKAWAY_FROM_JOB, if the outer job allows breakaway.
  - Otherwise track PIDs and walk CreateToolhelp32Snapshot (XP OK) by th32ParentProcessID. Guard against PID reuse by comparing GetProcessTimes creation times.

Job per pipeline as a "process group":
- `kill %1` maps to TerminateJobObject.
- JobObjectBasicAccountingInformation (Win2000+) gives TotalUserTime and TotalKernelTime. That implements the `time` pipeline keyword and `times`; Windows has no RUSAGE_CHILDREN.

Notifications (Win2000+):
- SetInformationJobObject(JobObjectAssociateCompletionPortInformation) delivers these messages to an IOCP, with lpOverlapped = PID:
  - JOB_OBJECT_MSG_NEW_PROCESS 6
  - EXIT_PROCESS 7
  - ABNORMAL_EXIT_PROCESS 8
  - ACTIVE_PROCESS_ZERO 4 (winnt.h:5282)
- Delivery is "not guaranteed" per the docs, so treat a message as a wake-up and confirm with the process handle.
- This replaces SIGCHLD and is not limited to 64 handles.

======================================================================
6. WAITING ON MANY PROCESSES
======================================================================
- WaitForMultipleObjects is capped at MAXIMUM_WAIT_OBJECTS=64.
- Alternatives:
  - (a) Job+IOCP, as in section 5.
  - (b) RegisterWaitForSingleObject (XP OK, legacy thread pool). Use WT_EXECUTEONLYONCE and have the callback PostQueuedCompletionStatus to the main IOCP. Clean up with UnregisterWaitEx.
  - (c) CreateThreadpoolWait (Vista+).
  - (d) Helper threads, each waiting on 63 handles plus a wake event.
- An IOCP handle is not usable in WaitForMultipleObjects, and the console input handle cannot be associated with an IOCP.
- Suggested event loop: one IOCP that receives job messages, wait-callback posts, overlapped pipe completions, and posts from a console-reader thread. Alternatively, the main thread does WaitForMultipleObjects([console input, wake event]).
- GetQueuedCompletionStatusEx is Vista+ (ioapiset.h:26); XP has GetQueuedCompletionStatus.

======================================================================
7. PIPES
======================================================================
- CreatePipe (XP OK) gives synchronous handles; overlapped I/O is not possible on them.
- Pipelines between external children never pass through the shell, so anonymous pipes are fine there.
- The shell multiplexes only in a few cases:
  - writing a large here-doc or here-string to a child while also reading `$(...)` output
  - several process substitutions
  - `read -t` on a pipe
- For those, use a uniquely named pipe:
  - Server end: CreateNamedPipeW(`\\.\pipe\unabashed-<pid>-<n>`, PIPE_ACCESS_INBOUND|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE (XP SP2+), PIPE_TYPE_BYTE|PIPE_WAIT, 1 instance).
  - Client end: CreateFileW without overlapped. Children must never receive an overlapped handle, because their synchronous ReadFile breaks.
  - PIPE_REJECT_REMOTE_CLIENTS is Vista+; leave it off on XP.
- Cancelling I/O: CancelIoEx is Vista+ (ioapiset.h:27). XP only has CancelIo, which cancels I/O issued by the same thread. CancelSynchronousIo is Vista+.
- `read -t 0` and polling: PeekNamedPipe works on anonymous pipes (XP OK).
- Bash 5.1+ here-docs use a pipe if the document fits the pipe buffer, otherwise a temp file (redir.c here_document_to_fd). Equivalent: pass the nSize hint to CreatePipe, or write from a thread.
- Process substitution: give native children the `\\.\pipe\...` path instead of /dev/fd/N.
- There is no SIGPIPE: writers get ERROR_NO_DATA or ERROR_BROKEN_PIPE. Builtins writing to a broken pipe should act as if killed by SIGPIPE (status 141). Some msvcrt children keep running after write errors.

======================================================================
8. CONSOLE (LINE EDITOR)
======================================================================
Detecting a terminal:
- Use GetConsoleMode success for isatty. GetFileType returns FILE_TYPE_CHAR for NUL too, so it is not enough.

mintty / Git Bash:
- stdio may be a named pipe named `msys-<16hex>-pty<N>-{from,to}-master` (or `cygwin-...`).
- Detect it with GetFileInformationByHandleEx(FileNameInfo) on Vista+, or NtQueryObject(ObjectNameInformation) on XP.
- Newer MSYS runtimes may use ConPTY instead, which gives a real console. Verify.
- In pipe-pty mode, run the editor in raw VT mode over the pipes.

Reading keys with ReadConsoleInputW (XP OK):
- KEY_EVENT_RECORD fields: bKeyDown, wRepeatCount, wVirtualKeyCode, uChar.UnicodeChar, dwControlKeyState.
- dwControlKeyState bits: RIGHT_ALT 0x1, LEFT_ALT 0x2, RIGHT_CTRL 0x4, LEFT_CTRL 0x8, SHIFT 0x10, ENHANCED 0x100.
- AltGr arrives as RIGHT_ALT|LEFT_CTRL. Treat it as text, not Meta.
- Characters outside the BMP arrive as two surrogate events.
- Alt+numpad input produces its character on Alt key-up.
- WINDOW_BUFFER_SIZE_EVENT (the SIGWINCH equivalent) needs ENABLE_WINDOW_INPUT.

Input mode bits:
- PROCESSED 0x1: turn off so ^C arrives as a 0x03 key.
- LINE 0x2, ECHO 0x4: off.
- WINDOW 0x8, MOUSE 0x10, INSERT 0x20, QUICK_EDIT 0x40.
- EXTENDED_FLAGS 0x80: needed when changing QUICK_EDIT.
- VIRTUAL_TERMINAL_INPUT 0x200 (consoleapi.h:73): Win10 1511+. Gives xterm key sequences and bracketed paste.

Output mode bits:
- ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4 (consoleapi.h:77): Win10 1511 (build 10586)+.
- DISABLE_NEWLINE_AUTO_RETURN 0x8.
- Detect support by SetConsoleMode failing. The "legacy console" setting on Win10 also disables VT.

Legacy console API (XP fallback, required on XP and Win7):
- GetConsoleScreenBufferInfo (cursor, srWindow, dwSize), SetConsoleCursorPosition, FillConsoleOutputCharacterW/Attribute, ScrollConsoleScreenBufferW, SetConsoleTextAttribute.
- The legacy console wraps immediately at the last column, with no pending wrap. That is readline's autowrap/xenl distinction (lib/readline/display.c, terminal.c).
- Plan for two backends: a VT emitter and legacy console calls.

Mode save and restore:
- Save and restore console modes around every foreground child, as readline does in rltty.c (prep/deprep). Children share the console's mode state.

Ctrl-C and Ctrl-Break:
- SetConsoleCtrlHandler (XP OK). The handler runs on a new thread created by the system, so only set an event or flag there.
- Event types: CTRL_C 0, CTRL_BREAK 1, CTRL_CLOSE 2 (~5 s before forced kill; map to SIGHUP), LOGOFF and SHUTDOWN.
- Every process attached to the console receives Ctrl-C. While waiting on a foreground child, the shell should ignore it, then treat a child exit code of 0xC000013A as SIGINT.
- CREATE_NEW_PROCESS_GROUP turns Ctrl-C off in the child, which suits background jobs (bash ignores SIGINT for `&` when job control is off).
- GenerateConsoleCtrlEvent sends CTRL_C only to group 0 (the whole console). It can send CTRL_BREAK to a specific group, so `kill -INT %job` can only be approximated with CTRL_BREAK, or with TerminateProcess.
- SetConsoleCtrlHandler(NULL, TRUE) ("ignore Ctrl-C") is inherited by children. That matches POSIX's "signals ignored on entry stay ignored", so `trap '' INT` maps to it.

UTF-8 and console text:
- Keep UTF-8 (WTF-8 for unpaired surrogates in filenames) internally.
- The shell's own console output should use WriteConsoleW (XP OK, independent of the code page).
- WriteFile to a console goes through the output code page. With CP 65001 before Win8, WriteFile returns a character count instead of a byte count, which makes callers retry and print garbage.
- ReadConsoleA/ReadFile under CP 65001 return NUL bytes for non-ASCII input until Win10 1903 or later.
- SetConsoleCP/SetConsoleOutputCP(65001) changes the console for children too, and the change persists after the shell exits unless restored. Do not change it by default; at most behind an option. GetConsoleOutputCP is XP OK.

======================================================================
9. FILES, GLOBBING, STAT
======================================================================
Directory enumeration:
- FindFirstFileExW, XP OK, with these Vista+/Win7+ variants:
  - FindExInfoBasic (Win7+) skips 8.3 names; on XP, use FindExInfoStandard.
  - FIND_FIRST_EX_LARGE_FETCH 0x2 (minwinbase.h:109) is Win7+; on XP pass 0, or it fails with ERROR_INVALID_PARAMETER.
- Enumerate `dir\*` and do your own fnmatch, because Win32 wildcards are broken: 8.3 matches, `*.*`, `<`/`>` DOS wildcards.
- Filter "." and ".." yourself (bash 5.2+ has globskipdots).
- Faster alternatives:
  - NtQueryDirectoryFile (ntdll, XP OK) with a large buffer.
  - GetFileInformationByHandleEx(FileFullDirectoryInfo / FileIdBothDirectoryInfo), Vista+.
- Case:
  - NTFS is case-insensitive, and Win10 1803+ adds per-directory case sensitivity.
  - Bash's nocaseglob is off by default.
  - Readline folds case for completion on native Windows (complete.c:187).

Reparse points and links:
- The FILE_ATTRIBUTE_REPARSE_POINT attribute plus WIN32_FIND_DATA.dwReserved0 give the tag:
  - SYMLINK 0xA000000C
  - MOUNT_POINT (junction) 0xA0000003
  - LX_SYMLINK (WSL/Cygwin) 0xA000001D
  - APPEXECLINK 0x8000001B
  - AF_UNIX 0x80000023
- readlink: CreateFileW(FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS) then DeviceIoControl(FSCTL_GET_REPARSE_POINT). XP OK.
- Creating links:
  - CreateSymbolicLinkW is Vista+ and needs SeCreateSymbolicLinkPrivilege, or Developer Mode plus SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2 (winbase.h:3422, Win10 1703+).
  - Junctions via FSCTL_SET_REPARSE_POINT work on Win2000/XP.
- Cygwin legacy symlinks are system-attribute files starting with `!<symlink>` or .lnk shortcuts; MSYS2's default `ln -s` copies. Decide how far `test -L` should go.

Canonical paths (`pwd -P`, `cd -P`):
- GetFinalPathNameByHandleW (fileapi.h:150) is Vista+.
- XP fallback: resolve component by component with readlink.

stat emulation:
- GetFileAttributesExW (XP OK, no handle opened) gives attributes, three FILETIMEs and size. That covers `test -e/-f/-d/-s/-nt/-ot`.
- `-ef`, inode and link count need GetFileInformationByHandle (XP OK). It gives dwVolumeSerialNumber, nFileIndexHigh/Low and nNumberOfLinks. Open with CreateFileW using 0 access plus FILE_FLAG_BACKUP_SEMANTICS so directories work.
- FileIdInfo (128-bit ReFS ids) is Win8+.
- FILETIME to Unix time: (ft - 116444736000000000) / 10^7.
- `-w`: check FILE_ATTRIBUTE_READONLY. A full ACL check (AccessCheck) is overkill.

Device files:
- Map /dev/null → NUL and /dev/tty → CONIN$/CONOUT$.
- Emulate /dev/stdin, /dev/stdout, /dev/stderr and /dev/fd/N internally; bash already special-cases these in redir.c.
- Reserved names (CON, NUL, AUX, PRN, COM1-9, LPT1-9, also with extensions) and trailing dot/space stripping affect globbing and redirection.

Long paths:
- MAX_PATH is 260. The `\\?\` prefix (W APIs, XP OK) bypasses it.
- The process cwd stays limited to about 258 characters unless the program is longPathAware (Win10 1607+ manifest plus registry setting). This applies to lpCurrentDirectory too.

======================================================================
10. CURRENT DIRECTORY AND ENVIRONMENT BLOCK
======================================================================
Current directory:
- The cwd is one global per process, and SetCurrentDirectoryW keeps a handle open to the directory. That blocks deleting or renaming it (`cd d; rm -rf ../d` fails), and GetFullPathNameW is documented as not thread-safe.
- Recommendation: virtualise the cwd.
  - Keep a logical PWD.
  - Resolve all relative paths yourself.
  - Pass lpCurrentDirectory to CreateProcess.
  - Let thread-based subshells each own a cwd.
  - Optionally keep the real process cwd at a neutral directory.
- Drive-relative state lives in hidden environment entries `=C:=C:\dir` (also `=ExitCode`, `=::=::\`). RtlGetFullPathName_U and cmd use them for `D:foo`. Keep them in child environments and update the `=X:` entry for the current drive.

Environment block:
- Format: wide `NAME=value\0...\0\0` with CREATE_UNICODE_ENVIRONMENT.
- The docs require sorting, case-insensitively by uppercase. `=X:` entries sort first naturally. Sort anyway; it is cheap.
- Size limits:
  - XP/2003: the whole block is limited to 32767 characters.
  - Vista+: only individual variables are limited, to 32767 characters.
- Windows variable names are case-insensitive; bash's are case-sensitive.
  - Normalise `Path` to `PATH` on import, as MSYS2 does.
  - Remove case-only duplicates on export.
  - Always keep SYSTEMROOT, or Winsock and other components break.
- Source: GetEnvironmentStringsW (XP OK).

======================================================================
11. TIME, RANDOM, THREADS
======================================================================
Time:
- GetSystemTimeAsFileTime (XP OK) has roughly 1-16 ms granularity.
- GetSystemTimePreciseAsFileTime (sysinfoapi.h:73) is Win8+ and must be resolved dynamically.
- QueryPerformanceCounter/Frequency is XP OK, but can be unreliable on some multi-core XP systems.
- GetTickCount64 is Vista+; XP has GetTickCount, which wraps.
- Uses: EPOCHREALTIME, SECONDS, the `time` keyword.
- Time zones: GetTimeZoneInformation (XP) or GetDynamicTimeZoneInformation (Vista+), for `printf %(..)T` and PS1 `\D{}`.

Randomness for SRANDOM:
- XP: RtlGenRandom (advapi32 SystemFunction036).
- Vista+: BCryptGenRandom.

Threads and synchronisation:
- CreateThread is fine with no CRT.
- XP: CRITICAL_SECTION, events, Interlocked*.
- SRW locks and condition variables are Vista+; WaitOnAddress is Win8+.
- Avoid `__thread`, which uses emutls on mingw; pass context explicitly instead.
- Fibers: CreateFiber/ConvertThreadToFiber are XP OK. CreateFiberEx and ConvertThreadToFiberEx are XP or Server 2003; verify.

Subshells without fork:
- busybox-w32 ash re-spawns itself and serialises the shell state into shared memory ("forkshell").
- Thread subshells with their own arena, fd table, cwd and environment are much cheaper, but need the virtualised cwd from section 10.

======================================================================
12. PROCESS-CREATION COST (benchmarks)
======================================================================
No clean, published microsecond comparison on the same hardware turned up. Data points:
- Poly/ML on Cygwin: fork+join of `true` took about 0.72 s (best about 0.4 s), against about 0.10 s on Linux on similar hardware. Cost grows with the parent's size, because Cygwin's fork copies the parent's address space into a new process and then execs.
- bitsnbites "Benchmarking OS primitives":
  - Linux thread creation is about 3x faster than Windows.
  - Process creation was >20x faster on Linux than Windows in the author's 100-launch benchmark.
  - Windows results are very sensitive to Defender and other antivirus.
- Git for Windows: MSYS is "way slower" than native MinGW, largely because of fork emulation.
- My rough expectations (verify): native CreateProcess+wait of a trivial exe on Win10/11 costs about 1-5 ms, dominated by AV scanning and DLL loading; Linux fork+exec costs about 0.3-1 ms; an MSYS2 fork costs several to tens of ms, and can fail with "address space needed ... already in use" / "fork: retry".
- Bruce Dawson (randomascii.wordpress.com) documented a system-wide lock during process teardown that limits process-creation throughput.

Conclusion: the speed advantage comes from never forking for `$(builtin)`, subshells, pipelines of builtins, or `exec` stubs. MSYS2 bash pays a Cygwin fork for each of those. For external commands, both shells pay the same CreateProcess cost.

======================================================================
MINIMUM VERSION SUMMARY (API → minimum → XP fallback)
======================================================================
| API | Minimum | XP fallback |
|---|---|---|
| InitializeProcThreadAttributeList / HANDLE_LIST | Vista | CRITICAL_SECTION around spawn |
| CancelIoEx, CancelSynchronousIo | Vista | CancelIo on the issuing thread, or a dedicated I/O thread |
| GetQueuedCompletionStatusEx | Vista | GetQueuedCompletionStatus |
| GetFinalPathNameByHandleW | Vista | manual readlink walk |
| CreateSymbolicLinkW | Vista | junctions via FSCTL_SET_REPARSE_POINT |
| GetFileInformationByHandleEx | Vista | NtQueryInformationFile / NtQueryObject |
| SRW locks, condition variables | Vista | CRITICAL_SECTION / events |
| CreateThreadpoolWait | Vista | RegisterWaitForSingleObject |
| GetTickCount64 | Vista | GetTickCount (wraps) |
| BCryptGenRandom | Vista | RtlGenRandom |
| PIPE_REJECT_REMOTE_CLIENTS | Vista | omit |
| FindExInfoBasic, FIND_FIRST_EX_LARGE_FETCH | Win7 | Standard info level, flag 0 |
| GetSystemTimePreciseAsFileTime | Win8 | GetSystemTimeAsFileTime |
| Nested jobs | Win8 | breakaway or PID tracking |
| Real console kernel handles | Win8 | pseudo-handles; filter `(h&3)==3` |
| ENABLE_VIRTUAL_TERMINAL_PROCESSING / _INPUT | Win10 1511 | legacy console API backend |
| SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE | Win10 1703 | requires privilege |
| UTF-8 ReadConsoleA, UTF-8 manifest | Win10 1903 | ReadConsoleInputW / W APIs |
| longPathAware | Win10 1607 | `\\?\` prefix; cwd stays ≤ MAX_PATH |

Available on XP and Win2000 with no fallback needed: CreateProcessW, job objects plus IOCP notifications, RegisterWaitForSingleObject, CreatePipe/CreateNamedPipeW, FILE_FLAG_FIRST_PIPE_INSTANCE (XP SP2), ReadConsoleInputW, WriteConsoleW, legacy console cursor and fill calls, SetConsoleCtrlHandler, GenerateConsoleCtrlEvent, GetFileAttributesExW, GetFileInformationByHandle, FindFirstFileExW (Standard info level), FSCTL reparse ioctls, CreateToolhelp32Snapshot, GetProcessTimes, CreateThread, CreateFiber, PeekNamedPipe.

Sources:
- [Cygwin mailing list, 2009](https://www.sourceware.org/ml/cygwin/2009-07/msg00032.html)
- [Poly/ML list: Cygwin fork timing](https://lists.polyml.org/hyperkitty/list/polyml@lists.polyml.org/message/AHIUDRQTQX4M7OOO63IYEWEZALMABY2F/)
- [Git for Windows: Windows vs Linux fork and exec semantics](https://gitforwindows.org/windows-vs-linux-fork-and-exec-semantics)
- [bitsnbites: Benchmarking OS primitives](https://www.bitsnbites.eu/benchmarking-os-primitives/)
- [Python bug tracker message](https://bugs.python.org/msg133161)
- [lmbench lat_proc man page](https://manpages.ubuntu.com/manpages/focal/man8/lat_proc.8.html)
