# M5a: Windows paths and spawning

Status: planning done; implementation in progress (see "Steps").

Scope and exit criteria: plan.md §6 and the M5a row of its milestone
table. The exact MSYS2 rules are in research/msys2.md; Win32 API
fallbacks in research/win32.md; busybox-w32 conventions in
research/busybox.md. Open items: the M5a section of issues.md.


## Where the code stood at the start (541c565)

- No path engine: `Shell.windows` is one flag (shell.cpp); `isabsolute`,
  `haspath`, `absolute` (expand.cpp) accept `C:` and `\`; `absolute()` is
  the chokepoint about 25 `os_*` call sites go through.
- `pathsearch` (exec.cpp) tries `"" .exe .com .bat .cmd` on a `;` PATH,
  matching neither msys2 (`""`, `.exe`) nor busybox (`.com .exe .sh .bat
  .cmd`); `stdpath` is `/usr/bin;/bin`.
- `initvars` hard-codes Linux identity (BASH, MACHTYPE, OSTYPE,
  BASH_VERSINFO).
- cd/pwd keep the native cwd and PWD as one string; CDPATH splits on `:`.
- wide.cpp (MSVCRT `splitcmdline`, `nativepath`, `quotearg`,
  `quotebatarg`) is only included by main_win32.cpp.
- main_win32 `os_spawn` builds the command line itself (ComSpec for
  .bat/.cmd, MSVCRT quoting otherwise), drops descriptors above 2, and
  `os_stat` adds exec bits by extension whatever the mode. Startup
  splits its own command line MSVCRT-style and imports only fds 0-2.
- A `#!` script fails CreateProcess and falls into the ENOEXEC
  in-process path, so `#!/usr/bin/env python` runs as shell code: a bug.
- The vos has no Windows personality (rejects non-`/` paths,
  case-sensitive, ELF-like fake magic, ignores command lines).
- `OsInfo` has no exe, raw command line or inherited fds; `Spawn` has no
  command line or CRT information.
- Procsubs expand to `/dev/fd/N`, which native programs cannot open.
- TZ is honoured only when exported; otherwise UTC.
- No CRLF handling and no `igncr`.
- Git for Windows on this host: `C:/Users/wellons/git` (5.3.15). The
  real shell is `usr/bin/bash.exe` (`bin/bash.exe` is the git-wrapper
  launcher). Tools take `GITBASH=`, defaulting to the former.


## Principles

1. All path semantics are pure core functions over a context struct, no
   Win32 calls: unit tests run anywhere, and the vos's Windows
   personality exercises them on Linux. The platform sees only
   canonical native paths (`C:/x`, `//srv/share/x`, `//./NUL`,
   `//./pipe/x`, or POSIX `/x`).
2. The core builds the Win32 command line (`Spawn.cmdline`) and decides
   the target kind; the platform passes it verbatim; the vos splits it
   with the target fake's own parser (MSVCRT or Cygwin).
3. Randomized and property tests only on pure functions or the vos.
   Real processes and files only through curated scripts in the
   sandbox, in a scratch WORKDIR outside the repo.
4. No GPL text. Git Bash and cygpath outputs are recorded as goldens.
   msys2-runtime's msys2_path_conv.cc (CC0) may be read for semantics,
   then reimplemented in house style.


## Steps

Waves: 1, 2, 3 in parallel; then 4; then 5; then 6 and 7 in parallel;
then 8 and 9 in parallel; then 10.

1. **paths.cpp, the pure path engine.** `PathMode` (POSIX, MSYS,
   NATIVE), `PathCtx` (mode, root, mounts, native cwd, per-drive cwds
   from `=X:`, temp). `topath` (accepts `C:/x`, `C:\x`, `/c/x`,
   `//srv/share`, `\\srv\share`, `\\?\`; msys longest-prefix mounts
   case-insensitive except `/dev` and `/proc`; native: `/` is the
   current drive's root, `C:x` uses that drive's cwd; virtual results
   for /dev/null, /dev/tty, /dev/fd/N, /dev/std*, /proc/self,
   /proc/PID/comm, /proc/cygdrive and reserved DOS names). `toposix`
   (cygpath -u), `towin` (-w, -m), path lists, fstab parser, `msysarg`
   (msys2.md §2 classifier with its misfires, MSYS2_ARG_CONV_EXCL),
   `msysenv` (conv_envvars, MSYS2_ENV_CONV_EXCL, MSYS_NO_PATHCONV),
   `personaof`. Tests: standalone t/unit/paths_test.cpp with goldens
   recorded from Git Bash's cygpath and tools/argv.exe on fixed
   strings, plus property loops (round trips) on random bytes.
2. **wide.cpp codecs and the PE sniffer.** Move the include into the
   core. `splitcygwin` (Cygwin build_argv: both quotes, `\\` and `\"`
   inside quotes, wildcard/`~` and `@file` flags for the caller),
   `quotecygwin`, splitcmdline variants (old msvcrt.dll, post-2008
   `""`), the lpReserved2 blob codec (slot width 4 or 8, FOPEN/FPIPE/
   FDEV, 65535-byte cap), `pesniff` (machine, subsystem, CRT kind,
   cygwin, ours) walking the import directory, and a section marker in
   our own image. Tests: quote/split round trips on random bytes,
   goldens from real parsers, `pesniff` on synthetic and real images.
3. **Git Bash differential harness and first corpus.** tools/argv.cpp
   (normal CRT startup, x64 and i686; prints cmdline, argv, selected env
   vars, optional fd probe), sandbox `-E` for a deterministic env,
   tools/gbrecord writing t/msys/NAME.spec, t/msys/run modelled on
   t/win32/run with a scratch install tree and an fstab mounting Git's
   usr/bin and etc, path normalization (@WORK@, @ROOT@), about 50
   curated t/msys/*.sh (cd/pwd/-W, cygpath, ~, arg and env conversion,
   quoting to msys children, type/hash with .exe stripping, noacl -x,
   shebangs, CRLF, option listings, OSTYPE, /dev/null, /proc/self, the
   Claude Code `-c` wrapper); `# vos` marks scripts the vos can run.
   Baseline timing of Git Bash on a draft agent batch.
4. **The vos's Windows personality.** `Spawn.cmdline`, `Spawn.crtslot`,
   `OsInfo.exe`, `OsInfo.cmdline`; drive roots, case-insensitive lookup,
   reserved names, `//./pipe/`; synthetic PE images for fakes; a Windows
   tree (C:/msys install with fstab, C:/w64/bin native fakes, argv,
   busybox and stubs, a simulated cmd.exe, C:/work); `os_spawn` splits
   with the fake's parser; `# personality:` in specs, `--personality`,
   the C normalizer, `# vos` t/msys scripts in tests.exe; `-DFUZZ_WIN`.
   Baseline t/winvos/ native specs (hand-checked, marked as such).
5. **Path modes in the shell.** `Shell.pathmode` (plus `sh->win32` for
   platform facts), persona from argv[0] and the install layout, mount
   table from OsInfo.exe and fstab, `absolute()` through `topath()`,
   native cwd split from PWD (cd logical/physical, `cd C:`, CDPATH by
   mode, `pwd -W`, `=X:`), hidden shopt `msyspath`, environment import
   (msys and busybox rules), identity variables, virtual files,
   `os_procname`, the msys stat suffix rule, case-insensitive HOME in
   prompts and tilde, msys re-split of the raw command line.
6. **msys input and builtins.** Drop every `\r` from parser input in msys
   mode; comsub strips trailing `\r\n`; native mode follows busybox;
   `set -o igncr`; option lists with `igncr` and `completion_strip_exe`
   in msys mode; the `cygpath` builtin (also as argv[0]).
7. **Spawning by target kind (spawn.cpp).** PATH search and exec bit by
   mode, a PATH cache keyed by path/mtime/size, target kinds (native
   MSVCRT/UCRT/other, msys, batch, self, busybox stub), msysarg/msysenv
   for native targets, Cygwin quoting for msys ones, batch through
   ComSpec in the core, shebangs (self-spawn for sh/bash, mount table
   then PATH otherwise, `.sh` without `#!` in native mode), E2BIG 126,
   0xC0000135 as 127, `exec` as spawn+wait+exit under kill-on-close,
   CREATE_NO_WINDOW without a console, type/command -v/hash display.
   os_spawn shrinks to CreateProcess with `s->path` and `s->cmdline`.
8. **Descriptors.** `OsInfo.inherit` (POSIX probe, Win32 lpReserved2,
   vos fake_bash), pass every open fd ≥3 to CRT targets with the
   target's slot width, `os_namedpipe` with a ConnectNamedPipe helper
   op, `/dev/fd/N` in argv rewritten to a named pipe for native and
   msys targets.
9. **Startup files and the system time zone.** Startup files by persona
   (msys login and interactive, sh/ash/lash), `--norc`, `--noprofile`,
   `--rcfile`; `os_localtz` (GetTimeZoneInformation to a POSIX TZ
   string; TZif footer on POSIX; configurable in the vos); zone names
   from zoneinfo where it exists.
10. **Exit criteria and benchmarks.** bench/agent (200 lines, Claude
   Code's wrapper, a CRT-free batch driver), bench/configure, profiling
   (cached environment block, startup costs), tools/w64trial on a
   scratch copy of w64devkit, final notes.


## Decisions

Taken (recommendations, revisit if the user disagrees):
- Persona: basename `bash` is msys; `sh`, `ash`, `lash` are native,
  unless `<root>/etc/fstab` exists (Git/MSYS2 installs, where sh is
  bash). argv[0] `sh` still selects POSIX mode, as bash.
- Native-mode OSTYPE `win32`, MACHTYPE `x86_64-w64-mingw32` (or i686).
- Hidden shopt name: `msyspath`.
- Forward slashes in canonical native paths; case kept as typed.
- Our own children recognized by a PE section marker.
- `type cygpath` reports a builtin in msys mode (accepted deviation).
- Shebang interpreters that are ourselves self-spawn.
- Native mode applet-first only through busybox alias stubs.
- The exec job sets kill-on-close on the program's M3b job (Win8+).
- Every open fd ≥3 goes to CRT targets.

- Git Bash cannot start at Low integrity (step 3), so t/msys and
  bench/agent run under `sandbox -L`: restricted token, job, process
  cap and timeout, but the caller's integrity level. Curated scripts
  only, which write only inside their own directory (tools/README.md,
  corpus rules).

For the user, when they come up:
- Trial projects beyond local binutils-gdb libiberty and zlib (xz and
  GNU make tarballs need a download).
- The agent-batch gate: external programs and the agent's own spawn of
  the shell cap the ratio near 1.3x, at most 2.6x (step 3 results), so
  "≥5x Git Bash" needs redefining.


## Step 3: Git Bash harness, corpus and baseline

Tools (tools/README.md has the details and the normalization rules
that step 4 reimplements):
- tools/argv.cpp, `make argv.exe argv32.exe`: CRT startup on purpose;
  prints `cmdline = <...>`, `argv[i] = <...>`, `NAME=value` or `NAME
  unset` for $ARGV_SHOW, and with ARGV_FD=N `fd N open|closed`.
- tools/sandbox: `-E NAME=VALUE` (an exact environment) and `-L`.
- tools/gbrecord WORKDIR SCRIPT...: records t/msys/NAME.spec from Git
  Bash, each script twice, failing on any difference.
- t/msys/run [-s SHELL] [-x] WORKDIR [TEST...]; t/msys/lib.sh holds
  what both share. The shell under test is copied to
  WORKDIR/inst/usr/bin/bash.exe, whose fstab mounts Git's usr/bin on
  /usr/bin and /bin and Git's etc on /etc.
- bench/agent: the draft agent batch (below).

Sandbox findings for Git Bash:
- At Low integrity every MSYS2 program dies at startup:
  `NtCreateDirectoryObject(\BaseNamedObjects\msys-2.0S5-<key>):
  0xC0000022` (the session namespace denies Low the create), and
  CreateProcess reports 0xC0000142. A write-restricted token whose only
  write SID is the logon SID, granted on DIR, fails the same way (and
  also fails to set its token's default DACL). Hence `-L`.
- An executable with a Low label runs at Low integrity wherever it is
  started from, so a `sandbox.exe` inside a sandboxed DIR silently
  breaks every later -L run: tools stay outside DIR.
- msys-2.0.dll's installation key comes from the DLL's path, and the
  mount table and process list live in per-installation shared memory.
  An uncopied Git Bash shares them with any Git Bash the user runs (in
  bench/agent `/tmp` resolved to the user's real TEMP, set by an
  earlier process). The copied DLL in WORKDIR/inst isolates the corpus
  from that, and its children (Git's own msys-2.0.dll) still inherit
  the shell's mount table: Git's cygpath answers @ROOT@ for `/`.

What the recordings settled (Git Bash 5.3.15, msys2-runtime 3.6.9):
- `echo /*` in msys mode lists the root directory's own entries merged
  with the mount points and the virtual trees: in the scratch install
  `/bin /dev /etc /proc /tmp /usr` (inst holds only etc and usr; bin
  and tmp are mounts; dev and proc virtual). /.* matches nothing.
- `/proc/PID/comm` does not exist in this runtime (self, exename,
  winexename and cygdrive do), so the Claude Code snapshot's read of it
  fails quietly in Git Bash too.
- noacl -x: `.exe` and the `#!`, `MZ`, `:\n` contents count; `.com`,
  `.bat`, `.cmd` and an empty file do not (a `.bat` starting with `#!`
  does). A file that is not -x still runs as a script when named
  (`./plain`, and from PATH): only `[ -x ]` follows the rule.
- A native parent that exports PWD (busybox does, as `C:/x`) makes
  Git Bash keep that spelling for `pwd`; Claude Code exports none.
- Writing to /dev/stdout reopens the output file and truncates it, so
  the corpus does not.

Corpus (60 scripts, t/msys/NNN-*.sh; `# vos` on 41):

| Topic | Scripts |
|---|---|
| cd, pwd, PWD, `pwd -W` | 001 cd-drive, 002 cd-winpath, 003 pwd-W, 004 pwd-logical, 005 cd-case, 006 cd-mounts, 007 root-glob, 008 cd-errors, 009 cd-dotdot, 010 cdpath |
| cygpath | 011 -u, 012 -w/-m, 013 -p, 014 -a, 015 root |
| `~`, HOME, TEMP | 016 home-tilde, 017 home-set, 018 tmp-temp, 019 home-prompt |
| argument conversion | 020 paths, 021 unchanged, 022 options, 023 lists, 024 MSYS2_ARG_CONV_EXCL, 025 MSYS_NO_PATHCONV, 026 argv0, 027 quoting, 028 relative |
| environment conversion | 029 PATH, 030 vars, 031 MSYS2_ENV_CONV_EXCL, 032 MSYS_NO_PATHCONV, 033 TZ, 034 import |
| quoting to msys children | 035 printf, 036 glob/@file, 037 env |
| type, command -v, hash | 038 exe, 039 native, 040 exe-suffix, 041 hash-p |
| noacl -x | 042 noacl-x, 043 noacl-run |
| shebangs | 044 sh, 045 env, 046 parse, 047 native |
| CRLF | 048 source, 049 comsub, 050 script (CRLF file), 051 native |
| option lists | 052 set-o, 053 shopt |
| identity | 054 ostype |
| /dev, /proc | 055 dev-null, 056 proc-self, 057 dev-fd |
| Claude Code wrapper | 058 wrapper, 059 fail, 060 snapshot |

Self-check: `t/msys/run -s "$GITBASH"` passes 60 of 60, twice, in
two different WORKDIRs.

Baseline, unabashed.exe at 133dcee (`make unabashed.exe`): 1 passed,
59 failed. By topic: cd/pwd 0/10, cygpath 0/5, HOME/TEMP 0/4, argument
conversion 0/9, environment conversion 0/6, msys quoting 0/3, type
0/4, noacl 0/2, shebangs 0/4, CRLF 0/4, options 0/2, identity 0/1,
/dev and /proc 1/3 (057), wrapper 0/3. The causes are the expected
ones: no mount table or POSIX output form (`C:/\`, `@GIT@\usr\bin/ls.exe`),
no `pwd -W` or cygpath, no argument or environment conversion (argv[0]
is the bare name), CRs kept, Linux identity, no igncr or
completion_strip_exe, no shebang search on PATH.

Agent batch timing (bench/agent, 52 commands, medians of 5 rounds,
Windows 11, ms per command):

| | ms |
|---|---|
| Git Bash, full (what the agent waits for) | 50.8 |
| Git Bash, `-c WRAPPER(:)` (startup, snapshot, wrapper) | 29.3 |
| Git Bash, `-c exit` | 24.5 |
| externals alone (1.3 programs per command, from busybox) | 21.3 |
| floor: spawning a trivial CRT program (argv.exe) | 18.3 |
| unabashed.exe `-c exit` | 18.1 |
| unabashed.exe `-c WRAPPER(:)` | 19.4 |

Outside the sandbox the floor is the same (17.4 ms; Git Bash `-c exit`
21, an MSYS2 `true` 18.4, `git --version` 25). So process creation on
this machine, about 17 ms, dominates: Git Bash's own work is 6 ms of
startup above the floor plus 5 ms of snapshot and wrapper, and the
externals are mostly their spawns. Full ≈ noop + externals holds
(29.3 + 21.3 = 50.6).

Estimate: unabashed with the same externals ≈ 19.4 + 21.3 = 40.7 ms,
1.25x Git Bash. Even with free externals (all builtins or in-process
applets) the agent's spawn of the shell bounds it at 50.8/19.4 = 2.6x.
"≥5x Git Bash" on an end-to-end per-command batch is unreachable; it
could hold for the shell's own time (startup, snapshot, wrapper and
builtins: 11 ms vs about 1 ms), for a batch run inside one shell, or
in vos instruction counts. For the user to decide (Decisions).


## Exit criteria checks

1. t/msys/run with unabashed.exe and unabashed32.exe green against every
   recorded spec; `# vos` subset green in tests.exe on Windows and WSL.
2. tools/w64trial: configure, make and make check pass, and config.h
   plus config.status substitutions equal a busybox sh run.
3. Agent batch ≥5x Git Bash (medians of 5, matching checksums).
4. Configure workload vs busybox sh, tables recorded.
5. No regressions (t/parse, t/exec, chaos, oracle counts), t/win32 green
   with new 013-016, XP laptop runs t/win32, kernel32-only imports and
   PE 5.01, FUZZ_WIN campaigns clean, Claude Code's Bash tool works
   with CLAUDE_CODE_GIT_BASH_PATH pointing at a scratch install.
6. No M5a items left in issues.md.


## Implementation

### Step 2: wide.cpp codecs and the PE sniffer

wide.cpp is now part of the core (included by unabashed.cpp before
parse.cpp), so the vos and the POSIX build compile it too. New API:
- `splitcmdline(a, cmd, parser)`: `ARGV_SHELL32` (the default, as
  before), `ARGV_MSVCRT`, `ARGV_UCRT`.
- `splitcygwin(a, cmd, program)` returns `CygArg {arg, pat, file, end,
  flags}`; the caller globs `CYG_GLOB` words (`pat`, GLOB_NOCHECK
  giving `arg`) and reads `CYG_ATFILE` ones (`file`), re-splitting
  "contents + ' ' + cmd[end:]" without a program name.
- `quotecygwin(b, arg)`; `quoteprogram` now also quotes `'`, CR and LF
  (Cygwin's program name).
- `encodecrtfds(a, fds, n, width)`, `decodecrtfds(a, blob, width)` over
  `CrtFd {handle, flags}` with `CRTFD_OPEN/PIPE/DEV`.
- `pesniff(read, ctx, scratch)` returns `PeInfo {machine, subsystem,
  width, crt, posix, dll, ours, ok}`: `CRT_NONE/MSVCRT/UCRT`,
  `POSIX_CYGWIN/MSYS`. Our marker is a `.unabash` section holding
  "unabashed" (`[[gnu::used, gnu::section]]`, Windows builds only);
  it survives `-O2 -s` in unabashed.exe and unabashed32.exe.

Tests: t/unit/wide_test.cpp (build line in its header) replays
t/unit/wide.golden, round-trips 20000 random argument lists through
both quoters and every parser, checks the descriptor codec and pesniff
on synthetic, truncated and randomly damaged images; `--real [DIR]`
sniffs cmd.exe, Git's ls.exe, w64devkit's busybox.exe and echo.exe
stub, and unabashed{,32}.exe. t/unit/wide_record.sh (Windows only)
records the golden from wide.cases with throwaway programs built in a
scratch directory: a native printer that splits its own command line
with CommandLineToArgvW, msvcrt.dll's `__wgetmainargs` and ucrtbase's
`_configure_wide_argv`, and a driver that starts it and Git's msys
printf through CreateProcess with fixed command lines.

What the recording showed (Windows 11, msys2-runtime 3.6.9):
- System msvcrt.dll follows the pre-2008 rule: `""` inside quotes is a
  literal quote that ends the quoting, like CommandLineToArgvW. Only
  UCRT keeps the quoting (`"a""b c"` is `a"b c`, else `a"b`, `c`).
- The program name: CommandLineToArgvW takes up to the closing quote
  and starts the next argument right there (`"p q"r s` gives `p q`,
  `r`, `s`); both CRTs toggle quotes until an unquoted blank (`p qr`,
  `s`) with backslashes literal. A leading blank gives an empty
  program name in all three. An empty command line gives the
  program's own path (not recorded: machine-specific).
- Cygwin separators are space, tab, LF and CR (not VT, FF). Inside
  either quote `\` and `\<quote>` escape, even in a drive-letter word
  (contradicting the source reading that `C:` disables them); a drive
  word (`C:x` or `"C:`, no slash needed) only keeps its unquoted
  backslashes literal. Outside quotes a backslash escapes only when the
  word goes through glob (it has `?*["'(){}` or a leading `~`):
  `a\b` stays, `a\b(c)` gives `ab(c)`, `a\"b` gives `a\b`.
- Single quotes and the scanner disagree: the scanner ends `'a\'` at
  the second quote, but glob's unquoting reads `\'` as an escape, so
  `'a\'b' c` is the single argument `a'b c`.
- `@file`: the name's quotes are removed without escapes, and if the
  file cannot be read the word goes through glob unquoted (`@"*"` globs
  `@*`). The contents are split like the command line, CR and LF
  separating, followed by a space and the rest of the line, so an
  unterminated quote in the file swallows the rest. At most 10
  response files per command line, nested or not; the 11th kills the
  process ("Too many levels of nesting").
- glob: `{a,b}` and `a{b,c}d` expand even without matches, `{}` stays;
  `~` and `~/x` stay as typed when HOME does not exist; long quoted
  arguments (5000 bytes) are not truncated.
- cmd.exe on Windows 11 imports UCRT (api-ms-win-crt-*), not msvcrt;
  Git's msys tools import msys-2.0.dll and no CRT; w64devkit's
  busybox imports msvcrt.dll and its alias stubs only kernel32.
