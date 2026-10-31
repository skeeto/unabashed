busybox-w32 ash on Windows and its w64devkit integration: report for the unabashed planner

Sources: busybox-w32 at C:/Users/wellons/src/busybox-w32 (version "v1.39.0.git-6354-g5f09fb446", the same FRP tag w64devkit pins at Dockerfile:100) and w64devkit at C:/Users/wellons/src/w64devkit. In this report, ash.c means shell/ash.c, and process.c, mingw.c, winansi.c and termios.c are under win32/. Side note: this session's Bash tool is itself w64devkit's busybox sh. A bad `grep --include` produced a BusyBox usage message.

======================================================================
1. FORKSHELL: HOW IT EMULATES fork()
======================================================================

Data structure: `struct forkshell` (ash.c:403-446).
- Pointers to the copied state: gmp (globals_misc), gvp (globals_var), cmdtable, atab (aliases), history plus cnt_history, and jobtab/njobs/curjob when JOBS_WIN32 is on.
- hMapFile, old_base, size, relocatesize.
- fpid, the kind of child.
- mode, nprocs, jpnull, job_hack.
- flags, fd[3], n (parse tree node), argv, path.

There are five kinds of child (ash.c:448-454):
- FS_OPENHERE: writes a heredoc into a pipe. Used only when the heredoc is larger than PIPE_BUF, which is 8192 (include/mingw.h:511, ash.c:6455-6493).
- FS_EVALBACKCMD: command substitution and process substitution (ash.c:7992-8018, 17106).
- FS_EVALSUBSHELL: `( ... )` and background `&` (ash.c:11347-11378, 17137).
- FS_EVALPIPE: each pipeline element (ash.c:11472-11509, 17154).
- FS_SHELLEXEC: every external command or applet that is not the final command under EV_EXIT (ash.c:12650-12664, 17178).

Parent side, spawn_forkshell (ash.c:17244-17276):
1. forkshell_prepare (ash.c:17971) runs two passes. forkshell_size (ash.c:17774) works out the exact byte count, split into a "funcblock" for structs and nodes and a "funcstring" for strings. Parse trees are sized with calcsize.
2. It calls CreateFileMapping(INVALID_HANDLE_VALUE) with an inheritable handle, sized for the data plus a relocation bitmap with one bit per pointer-sized word.
3. forkshell_copy (ash.c:17807) deep-copies everything into that region. Every pointer it writes goes through SAVE_PTR/MARK_PTR (ash.c:10668-10679), which sets that word's bit in the bitmap.
4. Copy routines use the SLIST_SIZE/COPY macros (ash.c:17308-17339):
   - var_copy covers all VTABSIZE=39 buckets and drops var_func.
   - tblentry_copy covers all CMDTABLESIZE=31 buckets and copies function bodies with copynode. Builtins are stored as an index into builtintab rather than a pointer.
   - globals_misc_copy covers curdir, physdir, arg0, commandname, minusc and the trap strings.
   - globals_var_copy covers funcname, positional parameters, redirlist and vartab.
   - atab_copy, history_copy and jobtab_copy run only when needed. Aliases and history are skipped for FS_SHELLEXEC; the job table is copied only for the "jobs" hack.
5. It spawns `busybox.exe sh --fs <hex handle>` through mingw_spawn_applet (process.c:230), which uses spawnveq and then the CRT's spawnve.
6. It closes and unmaps the view, then records the child's process HANDLE in the job (forkparent, ash.c:6361-6418).

Child side, forkshell_init (ash.c:18057-18179):
- ash_main checks for `argc==3 && argv[1]=="--fs"` before any other initialization (ash.c:16872).
- It maps the view and walks the bitmap, rebasing each marked pointer: `*ptr = fs + (*ptr - old_base)`.
- It restores builtin pointers from their indices, and calls reinitvar (ash.c:17220) to re-attach the special-variable callbacks.
- It allocates a new basepf input buffer, reseeds $RANDOM, increments shlvl and clears traps. A bare `trap` with no arguments is special-cased so it can still print the parent's traps.
- Background children ignore Ctrl-C via SetConsoleCtrlHandler(NULL,TRUE) and get stdin from /dev/null. Foreground children install ctrl_handler.
- It then dispatches through forkshell_child (ash.c:17189).
- The mapped block is "sticky": free() is #defined to sticky_free, which ignores pointers inside the block (ash.c:458-460, 18183).

File descriptors: fds 3 and up, such as pipe ends, reach the child only through MSVCRT's fd-table inheritance. spawnve passes the CRT fd table in STARTUPINFO lpReserved2, and every handle is inheritable. The child then dup2()s or closes them itself, for example in forkshell_evalpipe.

Costs:
- Every subshell, pipeline stage and command substitution pays for one CreateProcess of busybox.exe, a full deep copy of all variables, functions and aliases, plus up to the whole history (1024 lines by default in w64devkit), and a pointer-fixup pass. These are O(state) per fork.
- External commands are worse. Outside EV_EXIT, the parent spawns a forkshell child (FS_SHELLEXEC). That child calls shellexec, then mingw_execve, which spawns the real program and waits for it (wait_for_child, process.c:493). So each external command costs two process creations, and the intermediate busybox stays alive.
- `exec` cannot replace the process. It becomes "spawn, wait, exit with the child's code".
- Applets avoid the second process. NOFORK applets run inside the main shell (ash.c:12594-12643; disabled on ARM). Other applets and `#!` scripts whose interpreter is an applet run as NOEXEC inside the forkshell child (tryexec_applet ash.c:7057, tryexec ash.c:7142).
- Waiting does not use SIGCHLD. waitpid_child (ash.c:4955-5003) calls WaitForMultipleObjects in chunks of 64 handles with a 1 ms timeout when blocking, so it is a polling loop. w64devkit's wait-n-echild.patch fixes a 100% CPU spin there.

Takeaway for unabashed: CreateProcess directly from the main shell with std handles in STARTUPINFO, and run subshells and command substitutions in-process wherever possible, sweeping up with an arena reset. That removes both the serialization and the extra process.

======================================================================
2. SPAWNING EXTERNAL PROGRAMS
======================================================================

Lookup (find_command, Windows branch ash.c:9948-9976; shellexec ash.c:7246-7332):
- has_path(name) means the name contains / or \, or has a drive prefix X: (mingw.c:2634).
- unix_path(p) is true when dirname(p) is one of /bin, /usr/bin, /sbin or /usr/sbin (mingw.c:2621). For such paths, `/bin/foo` and `/usr/bin/env` are reduced to their basename and looked up as an applet, then a builtin, then in PATH.
- PATH search uses padvance with PATH_SEP ';' (include/libbb.h:2751). It reserves 4 extra bytes for an extension and accepts / or \ as a trailing separator (ash.c:3203-3258).

Extensions:
- add_win32_extension (mingw.c:2289) first tests the bare name. If that fails and the name has no known suffix or trailing dot, it tries suffixes in the fixed order com, exe, sh, bat, cmd (mingw.c:2240).
- PATHEXT is ignored.
- The "executable" bit is synthesized in do_lstat (mingw.c:822-827): a regular file is executable if it has one of those suffixes, or if has_exec_format (mingw.c:528) finds `#!`, a PE image that is not a DLL with subsystem 1, 2, 3 or 7, or an APE header ("MZqFpD"). The content sniff reads 1 KB and tries to preserve the file's access time.

Shebangs, parse_interpreter (process.c:67):
- It reads 100 bytes, requires `#!`, and splits on whitespace into path, basename and one options string (trimmed).
- A file ending in `.sh` with no `#!` is treated as `sh <file>`.
- ENOEXEC fallback is explicitly skipped (ash.c:7191). A script with no shebang and no .sh suffix therefore fails to execute; this differs from bash.
- mingw_spawn_interpreter (process.c:301) recurses at most 4 levels (ELOOP). For a unix_path interpreter that is an applet, it runs busybox with a fake argv[0] of "N:/name", where N is the index of the script within argv. appletlib.c:1358 decodes this. Otherwise it tries file_is_win32_exe(interp.path), then searches PATH for interp.name.
- Inside the shell, a script whose interpreter is an applet runs in-process (ash.c:7164-7187, set_interp).

Command-line quoting, quote_arg (process.c:120):
- Follows the MS CRT rules. Backslashes are doubled only when they precede a `"`; `"` becomes `\"`. The whole argument is quoted only if it contains a space or tab, or is empty.
- There is no cmd.exe metacharacter escaping for .bat/.cmd files, so the "BatBadBut" problem is present.
- spawnveq (process.c:160):
  - requires stat to show a regular file with the exec bit;
  - converts argv[0] slashes to backslashes, because spawnve mishandles relative .bat paths and cmd.exe with forward slashes;
  - treats an empty .bat as success;
  - appends "." to extensionless paths so spawnve will not add an extension;
  - maps EINVAL to E2BIG when the command line is too long.
- Special cases:
  - The libtool hack rewrites `cmd //c echo` to `/c` (ash.c:7148-7156; w64devkit issue 50).
  - ERROR_ELEVATION_REQUIRED falls back to ShellExecuteEx "runas" (process.c:515, 556-572).
- Environment: listvars(VEXPORT) is passed as envp. On UCRT, a CRT bug forces clearenv/putenv into the parent instead (ash.c:7257-7264).

Exit status, exit_code_to_wait_status_cmd (process.c:444):
- 0xC0000005 becomes SIGSEGV, and 0xC000013A becomes SIGINT.
- busybox's own convention is "signal << 24 with the low bits zero".
- Other NTSTATUS values above 0xFF print an "Error 0x..." message and the low byte is used, or 255 when the low byte is 0.

kill (process.c:966):
- Negative and zero pids enumerate process descendants through a Toolhelp snapshot.
- SIGKILL uses TerminateProcess. Other signals inject a remote thread that calls ExitProcess(sig<<24) (kill_signal_by_handle, process.c:905), which only works when the target has the same architecture.
- The ash `kill %job` path always passes " -pid", so the whole tree is signaled (ash.c:4838-4842).

======================================================================
3. PATHS, ENVIRONMENT, DEVICES
======================================================================

- busybox-w32 has no MSYS2-style /c/ translation. The documented model (README.md "Hints"; getpwd cases at ash.c:3389-3499):
  - C:/path and //host/share are absolute.
  - C:path is relative to the cwd of drive C:, resolved with GetFullPathName("C:") in get_drive_cwd (mingw.c:2543).
  - /path is relative to the root of the current drive or share.
  - path is relative to the cwd.
- PWD normalization:
  - converts to forward slashes;
  - strips trailing dots and spaces (strip_dot_space, mingw.c:2373);
  - fixes case with realpath and uppercases the drive letter or UNC host (fix_path_case, mingw.c:2554);
  - root_len and unc_root_len live at mingw.c:2469-2497.
  - mingw_chdir resolves symlinks and fixes case (mingw.c:1934). getcwd returns forward slashes (mingw.c:1218).
- Glob: expmeta skips a leading "C:" that is not followed by / (ash.c:9322). w64devkit's bare-drive.patch makes `C:` mean the drive's cwd in opendir and concat_path_file.
- Glob options: nocaseglob, nohiddenglob and nohidsysglob (ash.c:526-529, 9281, 9402).
- PATH, CDPATH and MANPATH: fix_pathvar (ash.c:2833) runs on every setvareq (ash.c:2901-2920) and rewrites `:` separators to `;`, skipping the colon in a drive prefix.
- Environment import, init() at ash.c:16624-16682, runs only if windows_env() sees mixed-case Path, ComSpec or ProgramData (mingw.c:2695). That test means "not launched by another busybox". When it runs:
  - All names are uppercased.
  - Names with invalid characters keep a copy of the original and are also added with the bad characters replaced by `_`.
  - Under mintty, HOME is unset.
  - USER, LOGNAME, HOME and SHELL are set if unset. HOME comes from GetUserProfileDirectoryA with forward slashes (mingw.c:1263).
  - Imported variables are flagged VIMPORT, and setwinxp (ash.c:3107) converts backslashes to slashes in their values, except COMSPEC and SYSTEMROOT.
  - The `-X` / `winxp` option (ash.c:520) leaves backslashes alone. The README says this is for "crufty old Windows code".
- getenv wrapper (env.c): TMPDIR falls back to TMP, then TEMP; HOME falls back to the profile directory. Setting an empty value needs a workaround (env.c:96-117).
- BB_* control variables, special in varinit (ash.c:2603-2607; include/libbb.h:2753-2759): BB_OVERRIDE_APPLETS, BB_SKIP_ANSI_EMULATION, BB_TERMINAL_MODE, BB_SYSTEMROOT and BB_CRITICAL_ERROR_DIALOGS. When exported they are pushed into the real CRT environment immediately (is_bb_var, ash.c:2872). BB_UMASK is exported by `umask` (ash.c:16471). BB_GLOBBING drives CRT argv wildcard expansion in _setargv (mingw.c:14).
- Device names (mingw.c:195-280): /dev/null maps to "nul" and /dev/tty to "con". /dev/stdin, /dev/stdout and /dev/stderr dup fds 0-2, and /dev/fd/N dups N. /dev/zero and /dev/urandom are emulated, and file attributes are faked for them. These only work inside busybox (redirects and applet arguments), not as arguments passed to other programs.
- Append redirection: the file is opened, then lseek(SEEK_END) (ash.c:6545).

CRLF tolerance:
- Script input strips CR from CRLF pairs, including a CR at a buffer boundary (nonblock_immune_wrapper, ash.c:12855).
- evalstring calls remove_cr (ash.c:15664).
- Command substitution drops a CR before each trailing newline it strips (ash.c:8123), and turns CRLF into LF inside IFS-split regions (ash.c:7508-7545).
- `read` handles CR LF and backslash-CR-LF (shell_common.c:313-342).

======================================================================
4. CONSOLE: LINE EDITING, VT, UTF-8, CTRL-C
======================================================================

Line editor: busybox libbb/lineedit.c, shared with hush. w64devkit's build has emacs keys plus vi mode (FEATURE_EDITING_VI), HISTFILE persistence, fancy PS1 and tab completion. It is far from readline: no inputrc, a limited binding set, and no SIGWINCH handling (EDITING_WINCH is off).

Windows changes in lineedit.c:
- Completion matches file names case-insensitively, keeps a "case-sensitive" byte after each match's NUL, and prefers the on-disk case (772-813, 1038, 1356-1545).
- Completion turns off the expensive exec-format sniff with BB_STAT_NO_HAS_EXEC_FORMAT (944, 1094).
- Ctrl-Z converts backslashes in the line to slashes (2869).
- Every keypress calls move_cursor_row(0) so the console scrolls back to the cursor (2803).
- When stdin is not a tty, input falls back to fgets plus remove_cr (2701-2719).
- IGNORE_CTRL_C flag (3134).
- w64devkit patches: disable-beep.patch, disable-empty-complete.patch (empty-prefix Tab would scan all of PATH, which takes "several seconds") and history-limit.patch (the cap rises to 16384; the w64devkit default is 1024).

Key input (libbb/read_key.c:115):
- If VT input is off, it calls windows_read_key (termios.c:41). That function uses ReadConsoleInput in raw mode (mode 0), maps VK codes to KEYCODE_* with Ctrl/Alt/Shift folded into bits, ignores Alt+numpad entry, and maps Alt-b/f/d/Backspace.
- Otherwise it parses VT escape sequences.
- termios emulation is minimal. tcgetattr and tcsetattr just get and set the CONIN$ console mode, plus the ECHO bit, VINTR=3 and VEOF=4 (termios.c:1-38).

Terminal mode, terminal_mode() (winansi.c:86):
- Bit 0 is VT_OUTPUT and bit 1 is VT_INPUT. Values 4 and 5 mean auto-detect; the build default is CONFIG_TERMINAL_MODE=5.
- ConEmu and Wine force 0.
- It repairs mode 0 left behind by SSH (defaults 3 for output and 0x1f7 for input) and clears DISABLE_NEWLINE_AUTO_RETURN.
- The shell calls terminal_mode(TRUE) before every interactive prompt (ash.c:15766) to undo console-mode damage done by children.

Output without VT:
- ansi_emulate_write (winansi.c:1035) and process_escape emulate cursor movement, erase, SGR including 256-color and 24-bit color mapped to 16 console colors (via a Lab color match), and the alternate screen buffer through CreateConsoleScreenBuffer (winansi.c:200).

UTF-8:
- The x64 build has a UTF-8 activeCodePage manifest (FEATURE_UTF8_MANIFEST). w64devkit turns off FAIL_IF_UTF8_MANIFEST_UNSUPPORTED so the binary still runs on old Windows, falling back to the ANSI code page.
- Input: readConsoleInput_utf8 (winansi.c:1358) uses ReadConsoleInputW and delivers UTF-8 one byte per event. It combines surrogate pairs by peeking 16 records ahead, and works around conhost sending key-up without key-down for pasted non-ASCII. Its comments say ReadConsoleInputA with CP 65001 crashes the console for code points at or above U+0800.
- Output: writeCon_utf8 (winansi.c:1461) decodes UTF-8 into a 4096-wchar buffer for WriteConsoleW, carrying partial sequences across calls. It is used when ACP is UTF-8 but the console output CP is not.
- LC_ALL/LC_CTYPE/LANG set to "C" disables conversion (winansi.c:1566).
- Without the manifest, text is converted between ANSI and OEM with charToConBuffA and conToCharBuffA.
- FEATURE_EURO switches CP 850 to 858.
- isatty() is false for NUL (mingw_isatty, winansi.c:1185).
- SIGPIPE is emulated: a write failing with ERROR_NO_DATA on a pipe, or ERROR_BROKEN_PIPE, exits 141 unless SIGPIPE is ignored (winansi.c:860-893).

Ctrl-C:
- ctrl_handler (ash.c:4933) sets bb_got_signal and pending_int. It raises an interrupt only if the shell is not the interactive root and no INT trap is set.
- Console children receive CTRL_C_EVENT themselves.
- The intermediate exec-waiter installs kill_child_ctrl_handler (process.c:406). If the child is not attached to the console (GetConsoleProcessList), it kills the child tree with kill(-pid,SIGINT); console children are left to handle Ctrl-C themselves.
- The prompt prints ^C through write_ctrl_c (ash.c:12954-12985).
- Traps: only INT is meaningful; may_have_traps is set only for a non-empty INT trap (ash.c:16061). Plus EXIT.
- `read -t` returns 142 on timeout, and ^C during `read` propagates (ash.c:16387).

Job control (JOBS_WIN32, ash.c:243-253, 4389-4428):
- There is no stopping, no fg/bg, no process groups and no tty ownership.
- `jobs`, `kill` and `wait` work, with "[n] pid" printed for background jobs.
- Exiting with jobs still running prints "You have background jobs." (ash.c:5558).

======================================================================
5. OTHER WINDOWS-SPECIFIC SHELL FEATURES
======================================================================

- Invocation:
  - The applets are ash, sh and lash; lash is MINGW-only and means a login shell (ash.c:216-222). login_sh is `applet_name[0]=='l'` (ash.c:16749), and -l or --login also work.
  - Undocumented flags (ash.c:13599): `-d DIR` (start directory), `-t TITLE` and `-N` ("press any key" on exit, SUW32).
- Profiles (ash.c:16973-16989): <system drive>/etc/profile, then the exe-relative profile, then $HOME/.profile.
  - w64devkit's system-profile.patch changes the exe-relative path to "../etc/profile", i.e. w64devkit/etc/profile. That file exports PATH_SEPARATOR=';', ac_executable_extensions=.exe and build_alias, so autotools does not mistake w64devkit for MSYS2.
  - w64devkit's login-no-chdir.patch stops the login shell from changing to $HOME.
- An interactive shell re-chdirs to getcwd() to normalize PWD (ash.c:16946).
- PROMPT_COMMAND is implemented, on the MINGW build only (ash.c:15770).
- `title` builtin (ash.c:16267); noconsole and noiconify options (ash.c:3133-3159). w64devkit's default-noiconify.patch makes noiconify the default.
- `cd -a` prints every drive's cwd (ash.c:12043-12072).
- `type` and `command -v` add the Windows extension and report "builtin applet" (ash.c:9683, 10326).
- stat st_ino is 0 unless EXTRA_FILE_DATA is enabled, so the $PWD import check (ash.c:16720-16729) is weak.

======================================================================
6. XP AND LEGACY-WINDOWS MEASURES
======================================================================

- lazyload.h: DECLARE_PROC_ADDR and INIT_PROC_ADDR, backed by get_proc_addr (mingw.c:2593). It uses LoadLibraryExA(LOAD_LIBRARY_SEARCH_SYSTEM32) and falls back to LoadLibrary with an explicit System32 path on Windows 7 systems lacking KB2533623.
- Lazily loaded APIs:
  - kernel32: GetSystemTimePreciseAsFileTime (Win8+, falls back to GetSystemTimeAsFileTime, mingw.c:2418), GetConsoleProcessList, Get/SetConsoleScreenBufferInfoEx, GetFinalPathNameByHandleA, CreateSymbolicLinkA, CreateHardLinkA, FindFirstFileNameW/FindNextFileNameW, ExitProcess (for remote-thread kill).
  - advapi32: SystemFunction036 (RtlGenRandom).
  - ntdll: RtlNtStatusToDosError, wine_get_version.
  - psapi: EnumProcessModules, GetModuleFileNameExA, GetProcessImageFileNameA, GetPerformanceInfo, EnumPageFilesA.
  - shell32: ShellExecuteExA. user32: ShowWindow. userenv: GetUserProfileDirectoryA.
- Runtime fallbacks:
  - PROCESS_QUERY_LIMITED_INFORMATION falls back to PROCESS_QUERY_INFORMATION (process.c:710-737). GetProcessTimes returning zeros on XP is handled.
  - FindExInfoBasic falls back to FindExInfoStandard (mingw.c:418-443).
  - The UTF-8 manifest is optional. Without UTF-8 input, readConsoleInput_utf8 degrades to '?' for non-ASCII characters (winansi.c:1430).
- w64devkit build: CONFIG_EXTRA_CFLAGS="-D_WIN32_WINNT=0x502" (Dockerfile:803). The x86 kit uses mingw32w_defconfig with no manifest, Pentium 4 and SSE2. The README says the x86 kit targets XP and later with "limited Unicode support".

======================================================================
7. TESTS
======================================================================

shell/ash_test/run-all runs each directory's run-* scripts. Each `X.tests` is run with $THIS_SH and its output compared with `X.right`. The harness compiles helper programs printenv.c, recho.c and zecho.c with gcc.

Counts of .tests files:

| Directory | Tests |
|---|---|
| ash-alias | 3 |
| ash-arith | 19 |
| ash-comm | 1 |
| ash-getopts | 6 |
| ash-glob | 8 |
| ash-heredoc | 28 |
| ash-invert | 1 |
| ash-misc | 106 |
| ash-parsing | 35 |
| ash-psubst | 10 |
| ash-quoting | 24 |
| ash-read | 12 |
| ash-redir | 28 |
| ash-signals | 20 |
| ash-standalone | 4 |
| ash-vars | 69 |
| ash-z_slow | 1 |

That is about 375 tests. None are Windows-specific; grep finds no mingw, win32 or windows mentions. ash-standalone tests the NOFORK/NOEXEC environment behaviour. Treat these as a small POSIX-subset corpus that may be useful; bash's own tests/ are the primary oracle.

======================================================================
8. KNOWN QUIRKS AND INCOMPATIBILITIES WITH BASH (MSYS2)
======================================================================

1. No /c/... paths, no mount table, no /tmp or /usr mapping. /x means the root of the current drive. An AI agent that writes `cd /c/Users/x` fails.
2. Imported environment names are uppercased. Imported values have backslashes turned into slashes, except COMSPEC and SYSTEMROOT; `-X` stops this.
3. PATH colons are silently rewritten to semicolons.
4. Extension probing uses .com/.exe/.sh/.bat/.cmd and ignores PATHEXT. The exec bit comes from content sniffing.
5. No ENOEXEC fallback. A shebang of /bin/X, /usr/bin/X, /sbin/X or /usr/sbin/X becomes a lookup of X as an applet, then in PATH.
6. Two processes per external command. `exec` does not replace the process. $! and pids refer to the intermediate busybox.
7. No job control (stop, fg, bg). Only INT and EXIT traps work. kill uses ExitProcess injection, and exit codes encode signals as sig<<24.
8. fds 3 and up reach only CRT-based children (MSVCRT lpReserved2). Non-CRT programs see only the std handles.
9. No cmd.exe-safe quoting for .bat files.
10. CRLF is accepted in scripts, command substitution and read.
11. The language is ash with bash-compat bits only: no arrays and only partial [[ ]]. w64devkit sets CONFIG_BASH_IS_NONE (src/busybox.config), so it has no `bash` command at all.

======================================================================
9. W64DEVKIT INTEGRATION AND WHAT A REPLACEMENT MUST LOOK LIKE
======================================================================

Applet stubs:
- Dockerfile:809-827 compiles src/busybox-alias.c (CRT-free, kernel32 only) into alias.exe. It copies it to bin/<name>.exe for a fixed list that includes `ash`, `lash` and `sh`, plus about 150 others such as ls, sed, awk and make-adjacent tools.
- The stub calls GetModuleFileNameW to find its directory, then CreateProcessW("<dir>\busybox.exe", GetCommandLineW() unchanged, inherit=TRUE). It ignores Ctrl-C in itself, waits, and returns the child's exit code. busybox dispatches on argv[0], so `sh.exe ...` becomes the "sh" applet.
- src/alias.c is the general version for non-busybox aliases such as cc, c99 and pkg-config. It takes compile-time EXE and CMD (the replacement argv[0] plus extra arguments).
- Applet choice inside busybox is prefer_applet (libbb/appletlib.c:296-343). BB_OVERRIDE_APPLETS holds a list separated by spaces, commas or semicolons; "-" disables all applets; "+" or entries after the first ';' mean "use an external program if one exists". Because SH_STANDALONE and PREFER_APPLETS are on, ash runs applets without touching PATH.

Launcher (src/w64devkit.c:588-683):
- Sets W64DEVKIT_HOME and W64DEVKIT, and HOME from w64devkit.ini with forward slashes.
- Builds PATH as `<dir>\bin` (optionally preceded by `<dir>\lib\ccache;`). It then appends the inherited PATH or a minimal one, or nothing in strict mode. Backslashes and ';' are used throughout.
- Sets the console title, changes to HOME (falling back to USERPROFILE), and runs `<dir>\bin\busybox.exe` with the literal command line "sh -l" (lines 665-672). This is hardcoded to busybox.exe, not bin/sh.exe.

A drop-in replacement for "sh"/"ash" needs to:
- Ship as bin/sh.exe (and ash.exe, lash.exe), removing those names from the alias list at Dockerfile:812-826.
- Change w64devkit.c to launch the new executable (or bin/sh.exe) with "sh -l".
- Choose behaviour by the argv[0] basename: strip .exe and compare case-insensitively. sh, ash and lash select native-path mode; lash or a leading '-' means login; bash would select MSYS2 mode.
- Honor -l, -c and -s, plus -d, -t, -X and -N if exact parity matters.
- Read <sysdrive>/etc/profile, then <exe dir>/../etc/profile, then ~/.profile.
- Not chdir on login.
- Reproduce the import normalizations above, or deliberately not. Applets and scripts may depend on them, for example uppercase PATH or HOME with forward slashes.
- Run sh/#!/bin/sh scripts with itself rather than busybox.
- Keep the libtool `cmd //c echo` workaround (the etc/profile comment ties the two together).
- Map /dev/null and /dev/tty in redirections.
- Lazy-load anything newer than XP; the x86 kit is built for _WIN32_WINNT 0x502.

Applet performance:
- Busybox ash runs applets in-process. A new shell spawning bin/ls.exe pays for the stub plus busybox.exe, i.e. two CreateProcess calls.
- Mitigation: recognize the stubs (same directory as busybox.exe, identical file or size, or a known list) and CreateProcess busybox.exe directly with argv[0] set to the applet name.
- Respect BB_OVERRIDE_APPLETS if parity is desired.
- GNU make in the kit runs `sh -c` from PATH, so -c handling and its quoting must be exact.
