MSYS2 / Git-for-Windows bash behaviors a drop-in replacement must emulate

Facts come from four places:
- the installed Git for Windows at C:/Users/wellons/git, which has bash 5.3.015-1, msys2-runtime 3.6.9-2 and mingw-w64-x86_64-git 2.55.0.4 (versions from etc/package-versions.txt);
- msys2-runtime and MSYS2-packages sources on GitHub;
- the Cygwin user guide;
- read-only probes run in the Bash tool, which is this same Git Bash. Probes only used builtins, ls/cat/cygpath, and the native w64devkit echo.exe to show what a native child actually receives.

The bash source paths below are relative to the bash-5.3 tree in the scratchpad.

1. POSIX PATH NAMESPACE

Mount table. `/proc/mounts` on this machine reads:
```
C:/Users/wellons/git                  /     ntfs binary,noacl,auto
C:/Users/wellons/git/usr/bin          /bin  ntfs binary,noacl,auto
C:/Users/wellons/AppData/Local/Temp   /tmp  ntfs binary,noacl,posix=0,usertemp
C:                                    /c    ntfs binary,noacl,posix=0,user,noumount,auto
D:                                    /d   ...   (one entry per existing drive)
```
- C:/Users/wellons/git/etc/fstab has only two lines that matter:
  - `none / cygdrive binary,posix=0,noacl,user 0 0` sets the cygdrive prefix to "/", which gives /c instead of /cygdrive/c.
  - `none /tmp usertemp binary,posix=0,noacl 0 0` maps /tmp to the user's Windows %TEMP%.
- etc/mtab is a symlink to /proc/mounts.
- "/" is implicit: it is the install root, the parent of usr/bin, where msys-2.0.dll lives.
- /usr/bin is implicit too. In MSYS2 /bin is a mount of usr/bin.
- Cygwin's own default mounts are /usr/bin and /usr/lib.
- fstab format:
  - Field 1 is the Windows path with forward slashes and `\040` for a space. Field 2 is the mount point, field 3 the type ("cygdrive" or "usertemp" are special), field 4 the options. Fields 5 and 6 are ignored.
  - Options: binary/text, acl/noacl, posix=0/1, exec/notexec/cygexec, user, override, and others.
  - There can also be per-user files at /etc/fstab.d/$USER.
  - Source: https://cygwin.com/cygwin-ug-net/using.html#mount-table
- /cygdrive does not exist in this setup (`ls /cygdrive` gives ENOENT). `/proc/cygdrive` exists and always points at the current prefix. `cygpath -w /cygdrive/c` returns `C:\Users\wellons\git\cygdrive\c`, an ordinary path under the root.
- Drive mapping: `/c` → `C:/`, `/c/x` → `C:/x`. Any single letter counts: `cygpath -w /x` gives `X:\` even if there is no X: drive. The letter is case-insensitive, so `/C/Users` works.
- Case:
  - With Windows' default obcaseinsensitive, every file lookup is case-insensitive.
  - Mount points also matched case-insensitively here: `/TMP` resolves and converts to the Temp directory.
  - The virtual device trees are case-sensitive: `/Dev/Null` and `/PROC` both give ENOENT.
  - Case is never canonicalized. `cd /C/USERS/Wellons` leaves PWD and `pwd -P` as typed, and `realpath` does not fix it either.
- //server/share is UNC. `cd //c` fails because it looks for a server named "c". Bash has Cygwin-only code so that `~user` does not produce `//`: lib/tilde/tilde.c:235. Readline treats `//server` as a directory: lib/readline/complete.c:662.
- Windows path input is accepted everywhere. `cd 'C:\Users\wellons'`, `cd C:/Users` and `cd c:/users` all work. PWD becomes the POSIX form (/c/Users/wellons, or /c/users as typed). `cd C:` goes to /c, the drive root, not that drive's own cwd. Mechanism in bash:
  - general.h:260-275: ABSPATH accepts `X:` or a separator at the start, and ISDIRSEP accepts both `/` and `\`.
  - general.c:869: make_absolute runs cygwin_conv_path(CCP_WIN_A_TO_POSIX).
  - general.c:845: absolute_program treats `\` as a path separator under MSYS.
  - lib/glob/smatch.c:306: in globbing, `\` also counts as a directory separator.
- /dev (virtual, `ls -la /dev`):
  - null, full, random, ptmx, clipboard, dsp, mixer, sdX/scdX block devices, shm, mqueue.
  - fd → /proc/self/fd; stdin, stdout and stderr → /proc/self/fd/{0,1,2}.
  - /dev/tty.
- /proc: per-PID dirs, self, cpuinfo, meminfo, mounts, version, uptime, loadavg, registry, registry32, registry64, cygdrive, sys, and others. Claude Code's snapshot reads `/proc/${CLAUDE_PID}/comm`. (msys2-runtime 3.6.9 has no `comm` there, so that read fails; recorded in t/msys/056.)
- Bash also emulates /dev/fd/N, /dev/stdin/out/err and /dev/tcp|udp/host/port itself in redirections: redir.c:580-590 and lib/sh/eaccess.c:64.
- Bash warns at startup if /tmp is missing: shell.c:352-366 and 412.

2. ARGUMENT CONVERSION WHEN AN MSYS PROGRAM SPAWNS A NATIVE PROGRAM

Where it happens:
- msys2-runtime winsup/cygwin/spawn.cc, child_info_spawn::worker. Conversion runs only when the target is not a Cygwin/MSYS exe.
  - av::setup checks for an MZ header, then hook_or_detect_cygwin inspects the imports for msys-2.0.dll.
  - App-execution-alias reparse points count as native.
- Every argv[i] goes through arg_heuristic_with_exclusions, including argv[0].

MSYS2_ARG_CONV_EXCL (path.cc):
- The value is split on ';'.
- An entry that is exactly "*" disables all conversion.
- Otherwise an entry matches as a prefix of the whole argument (`strstr(arg, excl) == arg`). Empty entries are ignored.
- After conversion, ";;" is collapsed.

MSYS_NO_PATHCONV: if set to any value, conversion is skipped entirely. This check is in find_path_start_and_type and is now in upstream msys2.

Classifier (msys2_path_conv.cc, find_path_start_and_type; the file is CC0):
- Convert at all only if the argument contains '/' or '\' before any whitespace.
- Return NONE (no conversion) when:
  - the argument starts with `~` or `:`;
  - it contains any of `` ` ' * ? [ ] ``;
  - it contains `/~`, `::` (IPv6), `:./`, `:../`, or `@@`.
- Leading characters that are not alphanumeric and not one of `/ \ : - .` are skipped, and classification restarts after them. This is why `@/file` converts.
- `X:\…` and `X:/…` are already Windows paths and stay unchanged. With a later ';' they are a Windows path list, also unchanged.
- `./` and `../` are relative paths: only slashes are normalized.
- Leading '/':
  - `/X:` is an escaped Windows path, so the first '/' is dropped.
  - `//` followed by a '/' later on gives UNC, which is copied.
  - `//` with no further '/' is an escape: `//c` → `/c`. This is how `cmd //c`, `start`'s `"$COMSPEC" //c start` (usr/bin/start) and `/c`-style switches are written.
  - Otherwise ':' followed by `/`, `:` or `.` makes a POSIX path list. Any other ':', or a ';', makes a Windows list, left alone.
  - Otherwise it is a rooted path.
- `-X…` options: after `-` plus one letter, the remainder is classified again. `-I/usr/include` → `-IC:/…/usr/include`.
- `=` restarts classification after it, so `--dir=/foo` and `foo=/bar` both convert. `,` after a leading '-' also restarts.
- `scheme://` is a URL and is left alone.
- `/dev/null` is special-cased to `nul`.
- A POSIX list `/a:/b` becomes `C:\…\a;C:\…\b`: the separator becomes ';' and backslashes are used.
- A single path becomes forward-slash Windows form.

Observed results with native echo.exe:
```
/c/Users → C:/Users              /tmp/x → C:/Users/wellons/AppData/Local/Temp/x
/        → C:/Users/wellons/git/ /usr/bin → C:/Users/wellons/git/usr/bin
/c       → C:/                   //c → /c        //server/share → unchanged
/dev/null→ nul                   --dir=/foo:/bar → --dir=C:\…\foo;C:\…\bar
/c/x;/d/y→ unchanged             http://a/b, a/b, ./a/b, */x, 1:2, x:/y → unchanged
@/file   → @C:/Users/wellons/git/file
/a b     → C:/Users/wellons/git/a b
/foo,/bar→ C:/…/git/foo,/bar     (only the first element converts)
```
Known misfires:
- `-DX=/y` → `-DX=Y:/` and `-o/x` → `-oX:/`: a single-letter component is taken as a drive.
- `/x/..` → `X://..`.
- Virtual paths cannot be converted:
  - `<(true)` reaches a native program as `/proc/689/fd/63`;
  - `/dev/stdin` → `/proc/self/fd/0`, `/dev/fd/1` → `/proc/N/fd/1`;
  - `/dev/clipboard` → `/Device/Null`.
- So process substitution does not work with native programs in Git Bash.

Docs: https://www.msys2.org/docs/filesystem-paths/. Windows-form arguments given to MSYS programs (`ls C:/`) are not converted; they just work because the runtime accepts Windows paths.

Native parent → MSYS child command line (dcrt0.cc build_argv/globify). This is critical for Claude Code, a native process that runs `bash.exe -c "<script>"`:
- In single or double quotes, `\\` becomes `\` and `\"` becomes `"`. Other backslashes stay literal.
- Single quotes also act as quotes.
- Unquoted arguments containing `? * [ " ' ( ) { }`, or starting with `~`, are globbed with GLOB_NOCHECK.
- Unquoted `@file` arguments are expanded as response files, at most 10.
- Confirmed live: the Bash tool turned my `echo '\\a' "\\b" 'c\\\\d'` into `\a \b c\\d`.
- argv[0] containing `:` or `\` is converted to POSIX.

3. ENVIRONMENT CONVERSION

At MSYS process startup from a Windows environment (environ.cc):
- conv_envvars converts these Windows → POSIX:
  - as path lists, ';' → ':': PATH, ORIGINAL_PATH, LD_LIBRARY_PATH;
  - as single absolute paths: HOME, SHELL, TMPDIR, TMP, TEMP.
- Names forced to uppercase (renv_arr, matched case-insensitively): COMMONPROGRAMFILES, COMSPEC, PATH, PROGRAMFILES, SYSTEMDRIVE, SYSTEMROOT, TEMP, TMP, WINDIR, and MSYSTEM in the MSYS build. Observed: `CommonProgramW6432` keeps its case.
- `=C:`-style per-drive cwd entries are hidden as `!C:` and restored for children.
- TERM: if absent, it is set to xterm-256color when the console has 24-bit color, otherwise cygwin. TERM=msys is discarded.
- If PATH is missing, the runtime directory is added.
- Names that are not valid identifiers, such as `ProgramFiles(x86)`, are not bash variables but still pass through to children.

Spawning a native child (build_env with keep_posix false):
- conv_envvars are converted back to Windows form with backslashes. Observed: TMP=C:\Users\wellons\AppData\Local\Temp.
- Every other non-empty variable goes through the same arg heuristic: `FOO=/tmp/a:/usr` becomes `C:/…/Temp/a;C:/…/git/usr`.
- MSYS2_ENV_CONV_EXCL: ';'-separated prefixes matched against the whole "KEY=VALUE" string, or "*" for all.
- TZ is dropped unless it starts with ':' or is only alphanumerics and `-+:`.
- The block is sorted with strcmp, as Windows expects.
- keep_posix is also true for strace.

Between two MSYS processes nothing is converted: `HOME='C:\Users\x' bash -c 'echo $HOME'` prints `C:\Users\x`.

Bash-side MSYS patches (now upstream as #ifdef __MSYS__):
- variables.c:2554: set_if_not strips one trailing CR.
- variables.c:4860: the export-string cache is not used.
- variables.c:5790: sv_home rebuilds the exported environment when HOME changes.

Git for Windows startup:
- /etc/profile (C:/Users/wellons/git/etc/profile):
  - Builds PATH from MSYS2_PATH="/usr/local/bin:/usr/bin:/bin", plus `$MINGW_PREFIX/bin` for MINGW*, UCRT* and CLANG*, then ORIGINAL_PATH.
  - MSYS2_PATH_TYPE can be strict, inherit (the default) or minimal.
  - Saves ORIGINAL_TMP and ORIGINAL_TEMP.
  - If TMP or TEMP contain backslashes, rewrites them with `cygpath -m`.
  - Sets TMPDIR=$TMP.
  - Sources /etc/profile.d/*.sh. env.sh prepends `$HOME/bin`; aliases.sh defines ls and ll.
- /etc/msystem defaults MSYSTEM to MSYS and sources /etc/msystem.d/$MSYSTEM.
- /etc/bash.bashrc sets the MSYS2-style PS1 (`\u@\h $MSYSTEM \w`) and sources git-prompt.sh for non-login shells.
- nsswitch.conf has `db_home: env windows cygwin desc`.
- The launcher (bin/bash.exe, git-bash.exe, which is git-wrapper) sets MSYSTEM=MINGW64, EXEPATH and PLINK_PROTOCOL, prepends mingw64/bin and usr/bin, and sets HOME from USERPROFILE (or HOMEDRIVE+HOMEPATH) when unset. HOME was /c/Users/wellons here. This launcher detail is from memory of git-wrapper.c, so moderate confidence.
- Observed in the Bash tool: OSTYPE=cygwin and MACHTYPE=x86_64-pc-cygwin. Older MSYS2 builds reported OSTYPE=msys. Claude Code's snapshot tests `$OSTYPE` for msys, cygwin or win32.

4. EXECUTABLES, .exe, SCRIPTS, SYMLINKS, PERMISSIONS, CRLF

.exe suffix (Cygwin path.cc stat_suffixes: "", ".exe", then .lnk as a last resort):
- stat("foo") finds foo.exe, so `[ -e git-bash ]` is true and running foo runs foo.exe.
- If both foo and foo.exe exist, foo itself is found first.
- type, command -v and hash show the name without .exe: `cmd is /c/Windows/system32/cmd`. Typing `notepad.exe` explicitly gives `/c/Windows/system32/notepad.exe`.
- In this install, plain `notepad` hits the usr/bin/notepad script first.
- shopt completion_strip_exe (Cygwin bash patch, off by default) drops .exe from completion candidates only when it is the same file.
- .com gets no special handling. .bat, .cmd and .btm skip the content checks in av::setup.

Executable bit under noacl (all of these mounts are noacl):
- Executable if the name ends in .exe, .com or .lnk, or the content starts with `#!`, `MZ` or `:\n`.
- Observed: `-x` is true for git-bash.exe and usr/bin/egrep (`#!/bin/sh`), and false for .bat, .cmd, .py without a shebang, and etc/profile.
- Recorded in t/msys/042-043: an empty `.com` is not -x (only `.exe` among the suffixes tried), a `.bat` starting with `#!` is, and a file that is not -x still runs as a script when named or found on PATH.
- chmod +x does not stick. Only the w bit maps to the DOS readonly attribute.
- Emulating this faithfully means reading the first bytes of the file when deciding `-x` or doing PATH search.

Shebangs (spawn.cc av::setup):
- Leading whitespace is skipped. The line ends at CR or LF, and trailing blanks are trimmed.
- The first token is the interpreter. The rest of the line is a single argument.
- The interpreter is found with find_exec over PATH, and the loop repeats for nested interpreters.
- A file with no `#!` runs under /bin/sh only for exec*p/spawn*p. Bash's own ENOEXEC fallback covers the shell case.
- An empty file counts as a script.

Symlinks:
- The default is a WSL-compatible reparse point or a SYSTEM-attribute cookie file.
- winsymlinks:lnk, native or nativestrict are chosen through the MSYS or CYGWIN environment variable. MSYS was empty here.
- MSYS2's documented default is deepcopy, meaning `ln -s` copies (https://www.msys2.org/docs/symlinks/).
- Junctions are read as symlinks but never created.
- Cygwin bash's `pwd -P` uses realpath() (lib/sh/pathphys.c, Cygwin patch).

CRLF:
- MSYS bash drops every '\r' from parser input: parse.y:1609 yy_getc and parse.y:2596 shell_getc.
  - Observed: `x=a\rb` gives "ab", and `echo "q\r"` prints "q".
  - This is unconditional and separate from the Cygwin `igncr` option (`set -o igncr`, off here). igncr is the Cygwin-only option that skips CR in buffered input, string input, command substitution and `read`.
- Command substitution strips trailing "\r\n" as newlines: subst.c:6786 and stringlib.c:270.
  - Observed: `$(printf 'a\rb\r\nc\r\n\r\n')` gives `a\rb\r\nc`.
  - Interior CRs survive.
- `read -r` keeps CR: `b\r` stays `b\r`.
- fc and inputrc reading tolerate CRLF: builtins/fc.def:695, bashline.c:801, lib/readline/bind.c:1102.
- When the read builtin's input is not a tty, Cygwin treats it as a pipe: builtins/read.def:481.
- For command substitution, Cygwin moves the pipe fds to 3 or higher to avoid hangs with native children: subst.c:7214.

5. CYGPATH

Options:
- Output form: -u (default), -w, -m (mixed: C:/x), -d (8.3), -t TYPE.
- Conversion: -a absolute, -p path list, -l / -s long or short names, -r `\\?\` prefix, -U /proc/cygdrive, -C codepage.
- Folders: -W, -S, -D, -H, -O, -P, -F id, -A.
- Input: -f file or '-' for stdin.

Observed:
```
-u 'C:\Users'        → /c/Users          -u 'C:/…/git/usr/bin' → /usr/bin  (mount-aware, longest prefix)
-u '…\AppData\Local\Temp\x' → /tmp/x      -u 'C:' → /c    -u 'c:foo' → c:foo (unchanged)   -u 'a\b' → a/b
-w / → C:\Users\wellons\git\   -m /tmp → C:/Users/wellons/AppData/Local/Temp
-w /dev/null → \\.\NUL  (-m //./NUL)     -w /proc → /proc (unconvertible)
-wp '/c/a:/usr/bin:.' → C:\a;C:\Users\wellons\git\usr\bin;.     -up 'C:\a;D:\b' → /c/a:/d/b
```
/etc/profile itself calls `cygpath -Wu` and `cygpath -m`, and agents use `cygpath -w/-m/-u` all the time. The tool is not shipped in w64devkit.

Recommendation: provide cygpath as a builtin, also reachable when the program is invoked under the name cygpath. Implement at least -u, -w, -m, -a, -p, -W, -S and -f, using the same conversion engine as the shell, so the output always agrees with the shell's mount view. Also add `pwd -W` (MSYS2 patch 0006-bash-4.3-add-pwd-W-option.patch), which prints the forward-slash Windows cwd: `pwd -W` in / gives C:/Users/wellons/git.

6. CONSOLE AND TTY

mintty:
- mintty is a Cygwin pty. The slave side is a pair of named pipes (pattern `\msys-<hex>-ptyN-from-master` / `-to-master`, or `cygwin-…`). The line discipline lives in the Cygwin DLL.
- A native program sees plain pipes: isatty is false, and termios is impossible.
- Git's compat/winansi.c (detect_msys_tty) recognizes mintty by that pipe name. This is from memory, moderate confidence.
- Since Cygwin 3.1, a pseudo console (ConPTY) is created when a pty process spawns a native one. It is on by default and turned off with MSYS=disable_pcon (environ.cc parse_options).
- So a native shell started by mintty normally gets a real ConPTY console. Without pcon it must fall back to reading lines without editing.
- winpty (usr/bin/winpty.exe, winpty-agent.exe) is the legacy workaround. Older Git for Windows aliases.sh files aliased node, python and others through winpty under TERM=xterm*. This install's copy has only ls and ll.
- etc/minttyrc: Term=xterm-256color, 100x27.

conhost / Windows Terminal: Cygwin emulates an xterm on top of the console. Configuration lives in etc/inputrc:
- completion-ignore-case on, bell-style visible, completion-query-items 40;
- mark-directories and mark-symlinked-directories on;
- show-all-if-ambiguous off;
- key bindings: \e[1;5C/D for word motion, \e[1~ / \e[4~ for Home and End, \e[3~ for delete, \e[Z for completion, \e[2;2~ for paste-from-clipboard.

7. WHAT AI AGENTS RELY ON (observed from Claude Code's own invocation)

Bash is started as `/usr/bin/bash -c` with `$-` = hBc: not a login shell, and /etc/profile is not sourced. BASH_EXECUTION_STRING is:
```
source /c/Users/wellons/.claude/shell-snapshots/snapshot-bash-….sh 2>/dev/null || true &&
export TEMP='C:\Users\…\Temp' TMP='C:\Users\…\Temp' && shopt -u extglob 2>/dev/null || true &&
{ \builtin unalias -- 'unsetenv'; \builtin unset -f -- 'unsetenv'; } >/dev/null 2>&1 || true &&
eval '<command, with '"'"' quoting>' < /dev/null && pwd -P >| /c/Users/wellons/AppData/Local/Temp/claude-5468-cwd
```
- The harness reads the `pwd -P` file back as an MSYS path. stdout and stderr are files under /tmp/claude/…; stdin is /dev/null.
- The snapshot (C:/Users/wellons/.claude/shell-snapshots/*.sh) uses:
  - `unalias -a`, `shopt -s expand_aliases`, `hash rg`, `typeset -f`, `function name {`;
  - `[[ -x ]]`, `local -a`, `${1+"$@"}`, `${arr[@]+"${arr[@]}"}`;
  - `ARGV0=rg` as an environment prefix, `exec -a`;
  - `$OSTYPE` matching msys, cygwin or win32;
  - `/proc/$PID/comm`, `command pgrep`;
  - `export PATH='/c/…:/mingw64/bin:/usr/bin:…'`, so PATH is in MSYS form with ':' separators.
- Locating bash: CLAUDE_CODE_GIT_BASH_PATH overrides it, and recent versions fall back to PowerShell when bash is absent (anthropics/claude-code issues #53068 and #36562).
- Agents also commonly use:
  - `/c/...` paths, `cd /c/...`, `/tmp`, `/dev/null`, `2>&1`, `>|`;
  - `cygpath -w/-m`, `$HOME` = /c/Users/x;
  - single-quoted Windows paths with backslashes (`'C:\foo\bar'`), which bash accepts in cd, redirections and file tests.
- They also write `2>nul`. In Git Bash that creates a literal file named "nul". In a native shell CreateFile("nul") is the device.
- A drop-in must parse its own Windows command line exactly as Cygwin's build_argv does, including `\\` → `\` inside quotes, globbing of unquoted wildcards, and @file. Otherwise agents' scripts reach the shell with different bytes.

RECOMMENDED SPEC

Internal model: all OS calls take a canonical Windows path (WCHAR, forward or back slashes). The mode decides only four things:
- (a) what a drive-less absolute path means;
- (b) the output form of PWD, `pwd`, `~`, completion, `type`/`command -v`, `$PATH` and absolute glob results;
- (c) environment translation at startup and for children;
- (d) argument conversion for children.

Input forms: accept these in BOTH modes:
- `X:/…` and `X:\…`; bare `X:` means the drive root in msys mode and the drive cwd in native mode;
- `//server/share` and `\\server\share`;
- `\` as a separator in paths.

MSYS mode is the default when invoked as bash.exe. Switch with something like `shopt -s msyspath`, and take the initial setting from argv[0] or an environment override.
- Mount table:
  - "/" is the install root. Find it from the exe location (strip /usr/bin or /bin), or from an `etc/fstab` next to it; parse fstab lines with the format above.
  - Built-in mounts: /bin and /usr/bin → <root>/usr/bin; /tmp → %TEMP%; cygdrive prefix "/".
  - Optionally /cygdrive/X as an alias, and /proc/cygdrive.
  - Longest-prefix match. Match mount points case-insensitively, but virtual /dev and /proc case-sensitively.
- Virtual files:
  - /dev/null → NUL; /dev/tty → CONIN$/CONOUT$; /dev/stdin/out/err and /dev/fd/N handled internally;
  - /dev/clipboard (optional); /proc/self, /proc/PID/comm (minimal); /proc/cygdrive.
  - Process substitution: use named pipes, and pass `\\.\pipe\…` (or a temp FIFO path) to native children instead of /dev/fd/N.
- Output form is /c/… and /tmp/… with longest-mount reverse mapping, matching cygpath -u. Keep path case as typed.
- Environment at startup, Windows → POSIX:
  - PATH and ORIGINAL_PATH (';' → ':'); HOME, SHELL, TMPDIR, TMP, TEMP as single paths;
  - uppercase the renv_arr names; set HOME from USERPROFILE if unset; default TERM as described in section 3.
  - Keep `=X:` entries and non-identifier names for children.
  - Simplest is converting at the CreateProcess boundary only.
- Children:
  - Detect MSYS/Cygwin PE files by their msys-2.0.dll or cygwin1.dll import, cached by path and mtime.
  - Native targets: apply the msys2_path_conv heuristic, honoring MSYS2_ARG_CONV_EXCL, MSYS2_ENV_CONV_EXCL and MSYS_NO_PATHCONV. Build the command line with MSVCRT quoting.
  - MSYS targets: convert only the conv_envvars set, which round-trips, and leave other arguments and variables POSIX. Build the command line with Cygwin quoting: double-quote anything containing `?*[]"'(){}`, starting with `~` or `@`, or containing whitespace, and escape `\` and `"` inside.
  - The two quoting schemes conflict on `\\`, so this detection is required, not optional.
  - Reproduce the heuristic's misfires (`-DX=/y`) exactly. Agents and Makefiles have adapted to them.
- Exec lookup:
  - Try name, then name.exe (and .lnk if wanted). Report the name without .exe in type, hash and command -v.
  - Executability uses the noacl rule from section 4. Run shebangs ourselves: map `/bin/sh`, `/usr/bin/env` and `/bin/bash` through the mount table, or to ourselves when that is cheaper.
  - Provide completion_strip_exe (off by default).
- Parser: drop every '\r' on input, as __MSYS__ does unconditionally. Strip trailing CRLF in command substitution. Provide `set -o igncr` for Cygwin compatibility.
- OSTYPE: "cygwin" to match current Git Bash, which this one-machine probe saw, or "msys" for older MSYS2 builds. Tests usually accept both. MACHTYPE x86_64-pc-cygwin.
- Builtins: cygpath and `pwd -W`.

Native mode (busybox-w32 style) is the default when invoked as sh or ash:
- "/foo" is relative to the current drive's root.
- Output form is `C:/…` with forward slashes.
- PATH is ';'-separated with forward slashes. Observed: busybox sh shows PATH=C:/Users/wellons/w64devkit/bin;… and TMP with forward slashes.
- Map /dev/null → NUL. Do not translate /c/ or /tmp, since they collide with real directories on the drive.
- No argument conversion for children; only quoting.

Coexistence: input coexists fine. MSYS bash already accepts C:/ and C:\ input, so always accepting Windows forms costs nothing. What cannot coexist:
- the meaning of drive-less "/foo" (install root vs drive root);
- the PATH separator, ':' vs ';'. The ':' in `C:/` makes a ':'-separated PATH ambiguous, so native mode must use ';';
- the output form.
So make it one switch that flips all three at once. When toggled at runtime, re-render PATH and PWD.

Pitfalls:
1. The `//x` escape vs UNC.
2. Single-letter components read as drives.
3. Converting arguments destined for MSYS programs when they should not be, or failing to.
4. Cygwin argv globbing of an unquoted `*` when we start an MSYS child.
5. @file expansion in MSYS children.
6. `2>nul` and other reserved DOS names.
7. Paths that cannot be converted: /proc and /dev/fd passed to native programs.
8. Exported HOME and TMP holding backslashes. Agents export `TMP='C:\…'`.
9. Case-preserving but case-insensitive comparisons for `-ef`, `cd ..` and prompt `~` substitution (`\w` compares against $HOME).
10. Windows XP: no ConPTY and no VT console, so console editing needs the plain console API path.
11. Cost of the `-x` check: reading file headers during PATH search should be cached per command hash.

Sources:
- https://raw.githubusercontent.com/msys2/msys2-runtime/msys2-3.6.5/winsup/cygwin/msys2_path_conv.cc
- https://raw.githubusercontent.com/msys2/msys2-runtime/msys2-3.6.5/winsup/cygwin/path.cc
- https://raw.githubusercontent.com/msys2/msys2-runtime/msys2-3.6.5/winsup/cygwin/spawn.cc
- https://raw.githubusercontent.com/msys2/msys2-runtime/msys2-3.6.5/winsup/cygwin/environ.cc
- https://raw.githubusercontent.com/msys2/msys2-runtime/msys2-3.6.5/winsup/cygwin/dcrt0.cc
- https://github.com/msys2/MSYS2-packages/tree/master/bash (patches 0001, 0002, 0005, 0006)
- https://www.msys2.org/docs/filesystem-paths/
- https://www.msys2.org/docs/symlinks/ (cited from prior knowledge; not fetched this session)
- https://cygwin.com/cygwin-ug-net/using.html
- https://cygwin.com/cygwin-ug-net/cygpath.html
- https://github.com/anthropics/claude-code/issues/53068
- https://github.com/anthropics/claude-code/pull/36562

Local files: C:/Users/wellons/git/etc/{fstab,profile,bash.bashrc,msystem,inputrc,minttyrc,nsswitch.conf,profile.d/*.sh}, C:/Users/wellons/git/usr/bin/{start,notepad}, C:/Users/wellons/.claude/shell-snapshots/snapshot-bash-1793449599359-ajbe9t.sh
