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


## Implementation

### Step 1: paths.cpp

API (all pure; results in the given arena):
- `initmounts(ctx, a, root, fstab)`: the root and /bin mounts, then
  fstab (`parsefstab`: `\040`, `posix=0` as nocase, two fields as a
  nocase mount, three ignored, `cygdrive`, `usertemp`, `bind`,
  `override`; unknown options reject the line).
- `topath(ctx, a, path) -> PathRes`: canonical native path for
  opening, or a virtual kind (PK_NULL, PK_TTY, PK_FD, PK_DEVICE,
  PK_DEV, PK_PROC, PK_CYGDRIVE), with `literal` for names Win32 would
  alter in msys mode.
- `toposix(ctx, a, path)`: cygpath -u. `towin(ctx, a, path, back)`:
  cygpath -w (back) or -m. `towinabs`: the same from the cwd.
- `toposixlist(ctx, a, list)`, `towinlist(ctx, a, list, back)`: -up,
  -wp, -mp. `envtoposix(ctx, a, name, value)`: startup import of
  conv_envvars (quotes and trailing backslashes stripped).
- `msysarg(ctx, a, arg, excl)`, `msysargv(ctx, a, argv, env)`: one
  argument, or an argv under the child environment's
  MSYS2_ARG_CONV_EXCL and MSYS_NO_PATHCONV.
- `msysenv(ctx, a, env)`: the environment block for a native child.
- `personaof(argv0, fstab) -> PathMode`.

Goldens: t/unit/paths_record.sh runs the curated t/unit/paths.cases
in Git Bash 5.3.15 (cygpath on fixed strings, plus an argv/environment
printer it compiles into an empty scratch directory) and writes
t/unit/paths.golden with the recording context (root, %TEMP%, cwd,
drives, fstab), which paths_test.cpp rebuilds. Cases whose answer
depends on the file system or open descriptors were left out (below),
and cygpath -w never sees a UNC path, which would hit the network.
paths_test.cpp: 553 golden checks, 146 inline (native mode, fstab,
virtual files, reserved names, POSIX mode, persona, startup import),
and about 920k property checks on random input (canonical and
idempotent topath, Windows and POSIX round trips through the mounts,
msysarg leaves slash-free arguments alone, sorted msysenv).

Found by recording (msys2.md corrected where it was wrong):
- TZ is dropped *if* it starts with ':' or holds anything but
  alphanumerics and `-+:`; an empty TZ is kept.
- A missing or empty PATH appends `PATH=<root>\usr`; `PATH=` itself
  is kept. MSYS_NO_PATHCONV with an empty value also disables the
  heuristic (not conv_envvars). MSYS2_ARG_CONV_EXCL only counts when
  exported.
- As arguments, /dev/null, zero, full, random, urandom and clipboard
  all become `/Device/Null`; /dev/std* become /proc/self/fd/N and
  /dev/fd/N becomes /proc/PID/fd/N; /dev itself and unknown /dev
  entries map under the root; `/dev/null/` is left alone.
- `/a:...` is an escaped Windows path: `/a:/b;` gives `a:/b;` and
  `--a=/b:/c` gives `--a=b:/c`. `PATH=C:/a` as an environment entry
  gives `C;A:\`.
- cygpath -wp turns empty elements into "."; -w of /dev/tty, /dev/ptmx
  and /proc entries prints them unchanged; a missing drive with a
  trailing slash gives `X://`; `\\.\C:\x` stays `//./C:/x` with -u.

Choices of ours: reserved DOS names are devices in both modes (classic
Win32 rule in native mode; bare names only in msys mode, where a
reserved name with an extension, or a trailing dot or space, opens
literally). In msys mode `C:` is the drive root and `..` in a relative
path resolves in the POSIX namespace; in native mode `C:x` uses the
drive's cwd. `..` resolves lexically (Cygwin checks the file system).
`envless` moved from vars.cpp to paths.cpp, `upcase` and `byteat` from
b_printf.cpp to base.cpp.


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
- igncr, completion_strip_exe and cygpath follow the path mode (the
  msyspath switch), not the persona (step 6).
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

Decided by the user (2026-11-04): the agent-batch gate is the shell's
own time, ≥5x Git Bash's. Per command, the shell's own time is the
end-to-end time minus the time of the same external programs run
directly, minus the process creation floor (a do-nothing program such
as argv.exe started the same way), all measured by step 3's method.
Step 3's numbers put Git Bash's at about 11 ms (50.8 - 21.3 - 18.3)
and unabashed's no-op at about 1 ms (19.4 - 18.3), so the gate is
decided by how cheaply we run the batch's shell work and start up.


## Exit criteria checks

1. t/msys/run with unabashed.exe and unabashed32.exe green against every
   recorded spec; `# vos` subset green in tests.exe on Windows and WSL.
2. tools/w64trial: configure, make and make check pass, and config.h
   plus config.status substitutions equal a busybox sh run.
3. Agent batch: the shell's own time (end to end minus the externals
   run directly, minus the process creation floor) ≥5x lower than Git Bash's (medians of 5, matching
   checksums).
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

### Step 3: Git Bash harness, corpus and baseline

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


### Step 4: the vos's Windows personality

API (api.cpp), the only core-facing change:
- `OsInfo.exe`: the shell's own image, native form (`C:/x/sh.exe`).
  main_win32.cpp fills it from GetModuleFileNameW (`\\?\` stripped,
  slashes forward), main_posix.cpp from /proc/self/exe (empty
  elsewhere).
- `OsInfo.cmdline`: the raw Windows command line in WTF-8, the input
  main_win32.cpp already split; empty on POSIX.
- `Spawn.cmdline`: when set, the platform passes it verbatim (step 7
  builds it in the core); empty keeps today's quoting of argv.
- `Spawn.crtslot`: 4 or 8, the lpReserved2 slot width for fds ≥3; 0
  passes only 0-2. Only the vos reads it so far.

Vos (vos.cpp, `Vos.win`, set by `vreset(..., win=1)`). Linux mode is
unchanged: the whole Linux suite passes byte for byte.
- Paths go through `vpath`: `/c/x` and `/dev/null` are accepted (the
  shell still produces them), everything else goes through paths.cpp's
  `winnorm` against `C:/`. Kinds: a file (internally `/C:/x` in the
  same node tree), the null device (`//./NUL` and a reserved DOS name
  as the last component in any directory, with or without an
  extension), a named pipe (`//./pipe/NAME`, a table of 64 VPipes), or
  nothing (any other `//` UNC path: ENOENT).
- Lookup folds ASCII case; listing sorts in NTFS order. `os_stat`
  synthesizes Windows modes: directories 0755, files 0644, read-only
  files lose the write bits, and `.exe .com .bat .cmd .sh` names or
  `MZ`/`#!` content add 0111 (the noacl rule plus `.sh`).
- Fakes are installed as synthetic PE images (`vpeimage`: the machine,
  one import, and for our own shell an `.unabash` section) that the
  real `pesniff` classifies, with a marker naming the fake; the image
  maps back to its fake, so a copied `.exe` still runs.
- `os_spawn` in win mode: the command line is `Spawn.cmdline`, or else
  argv quoted the way main_win32.cpp does (batch files through
  `%ComSpec% /d /e:on /v:off /s /c "..."`); over 32766 UTF-16 units is
  E2BIG. The child's argv is that command line split by the target's
  parser: `splitcmdline` with the CRT's variant for native images,
  `splitcygwin` for msys-2.0.dll ones (globbing against the vos, `~`,
  `@file` up to ten deep), the raw argv for our own shell. Fds ≥3 reach
  the child only with `crtslot`. A child sees the native cwd
  (`C:/work`), or `/c/work` if it is an msys program.

Windows tree (`vwintree`): `C:/w64/bin` holds `sh.exe` (our shell),
`argv.exe` (x64) and `argv32.exe` (i386, both msvcrt; output in
tools/argv.cpp's format, with ARGV_SHOW and ARGV_FD), `busybox.exe`
(msvcrt; the Linux fakes as applets) and one alias stub per Linux fake
(`ls.exe` runs `busybox ls`, as w64devkit's do). `C:/msys/usr/bin`
holds `bash.exe` (our shell) and the Linux fakes as MSYS2 programs;
`C:/msys/etc/fstab` is MSYS2's default and `C:/msys/tmp` exists.
`C:/Windows/System32/cmd.exe` (ucrt) is a small cmd model: `/d /s /c`
quote stripping, `%VAR%` (case-insensitive, `%cd%`, `:~N,M`), `^`,
echo/rem/set/exit [/b], batch files line by line, programs in the cwd
and PATH with .COM .EXE .BAT .CMD, 9009 when not found. Also `C:/work`
(the cwd), `D:` and `C:/Users/oracle/AppData/Local/Temp`.

Harness (main_test.cpp):
- `# personality: native|msys` runs a spec with Windows OsInfo: exe
  `C:/w64/bin/sh.exe` or `C:/msys/usr/bin/bash.exe`, cwd `C:/work`, a
  Windows environment (`=C:`, ComSpec, LANG, Path, SystemRoot, TEMP,
  TMP, TZ, USERPROFILE) and a command line built from argv. `--run
  [--personality NAME] [--spec] SCRIPT` does the same by hand.
- `./tests.exe t/msys/*.sh` runs the `# vos` scripts in the vos with
  t/msys's layout (C:/git for Git's usr/bin, C:/work/bin/argv.exe,
  C:/work/NAME as the script's directory, `# shell:` with $D $DW $DU)
  and tools/README.md's normalization in C, then reports `N t/msys vos
  specs, P passed, F failed (S not # vos)`; `--msys-diff` shows the
  differences. It is a separate invocation, so the default suites stay
  green while t/msys is red.
- `test_vwin` checks stat, case, modes, pesniff on the images, copied
  images, named pipes and each spawn kind.
- t/winvos/ (6 native specs, 1 msys): a hand-checked baseline of
  today's minimal Windows behavior, not oracle output. Each script
  says so; steps 5 and 7 change them on purpose. `.gitattributes`
  keeps their CRs.
- main_fuzz.cpp `-DFUZZ_WIN` runs any mode in the Windows vos, the
  shell as sh.exe in C:/work.

Results: tests.exe 144/0; t/exec 654/0, also with --chaos 20; t/parse
594/0; t/unit 10/0; t/winvos 7/0, also with --chaos 20; paths_test
923855/0; wide_test 100581/0; FUZZ_WIN run, chaos and isolate replay
t/exec and t/winvos (661 files) clean; unabashed.exe, unabashed32.exe
and the WSL POSIX build without warnings (tests.exe 144/0 there).
t/msys in the vos: 41 `# vos` specs, 1 passed (057), 40 failed, 20 not
`# vos`; the failures have step 3's baseline causes (no mount table,
`C:/\`, bare argv[0], CRs kept).

### Step 5: path modes in the shell

Shell state (shell.cpp):
- `Shell.pathmode` (PM_POSIX, PM_MSYS, PM_NATIVE) replaces
  `Shell.windows`; `sh->win32` keeps the platform fact (trap.cpp).
  `sh->paths` is the PathCtx (mounts, cygdrive, pid, temp), `sh->exe`
  the shell's image in native form.
- `sh->cwd` is now the native working directory the platform sees
  (empty for a virtual msys directory such as /proc), and `sh->wd` the
  logical one, bash's the_current_working_directory: cd's base, what
  pwd prints, `\w`, DIRSTACK and jobs' directories. $PWD is only
  exported state; assigning it moves nothing, as in bash.

Startup (unabashed.cpp):
- `pathsetup` runs before initvars. The install root is the shell's
  directory less `usr/bin` or `bin`; `root/etc/fstab`, read with
  os_open, gives the mounts (initmounts) and with personaof's argv[0]
  rule the persona: sh, ash and lash are native, anything else is msys
  (so unabashed.exe beside no fstab is msys). With no fstab the
  cygdrive prefix is `/`, as every MSYS2 install sets it, and a shell
  in `root/bin` (w64devkit's layout) mounts /bin on its own directory
  so that PATH survives a trip through msys form.
- Environment, msys: names in the renv list are uppercased, PATH and
  ORIGINAL_PATH convert as lists and HOME, SHELL, TMPDIR, TMP, TEMP as
  single paths (envtoposix), TERM=msys is dropped, `=X:` entries stay
  in badenv for children. HOME defaults from USERPROFILE, TERM to
  xterm-256color, PATH to /usr/bin. Native (busybox-w32's rules, only
  when Path, ComSpec or ProgramData show it is a real Windows
  environment): names uppercased; values slashified except COMSPEC and
  SYSTEMROOT; an invalid name goes to badenv as is and is imported
  with `_` for each invalid byte; HOME from USERPROFILE, unexported;
  USER and LOGNAME from USERNAME.
- Identity: native is `win32` and `x86_64-w64-mingw32` (i686 on 32
  bits), BASH and SHELL the image; msys follows the Git Bash
  recordings (`cygwin`, `x86_64-pc-cygwin`, BASH the POSIX image path
  without `.exe`, SHELL /usr/bin/bash by default). BASH_VERSINFO[5]
  is MACHTYPE.
- PWD: msys converts the native cwd with toposix; an inherited PWD
  naming the same file is kept (not in native mode).
- In msys mode `msysargv` re-splits `OsInfo.cmdline` with splitcygwin
  and does the caller's duties: `@file` (at most ten deep; the 11th is
  fatal, wording unverified), braces and globbing with the shell's own
  glob, `~` from HOME, an empty command line meaning the image's path.
  cygbraces and cygunquote moved from vos.cpp to wide.cpp for it.

Paths (expand.cpp, exec.cpp):
- `resolve` is the one door to paths.cpp: it builds a PathCtx with the
  mode and cwd (native mode fills a `C:x` path's drive directory from
  the `=C:` entry), joins a relative msys path onto `sh->wd`, calls
  topath, recognizes the /dev directory and prefixes literal names
  (trailing dot or space) with `//?/`. `absolute` uses it outside
  POSIX mode. isabsolute and haspath know `\` and drives.
- `pathstat` replaces os_stat(absolute) everywhere the shell tests a
  name (test, [[, glob, source, exec, redirections, noclobber): it
  answers for /dev and its devices, /proc/self, /proc/cygdrive and
  reserved DOS names, and applies the msys rule that `foo` finds
  `foo.exe`. `openwin` opens the virtual files: /dev/fd/N and
  /dev/std* dup, /dev/tty is CONIN$/CONOUT$, /dev/null and the sinks
  are NUL (sinks refuse reads), /proc/self/exename and winexename are
  served from memory, /proc/PID/comm is not there.
- glob lists mount points inside a directory, dev and proc at /, and
  /dev's devices (pathlist).
- PATH and CDPATH split on `;` in native mode, `:` otherwise; the
  search tries `""` and `.exe` in msys mode and all five extensions in
  native mode. Children start in `spawncwd`, the root's native path
  when the logical directory is virtual.

cd and pwd (builtins.cpp):
- msys: a non-`/` operand converts with toposix first; logical cd
  canonicalizes the POSIX path (keeping `//`), -P asks the file
  system. native: the operand resolves as typed (`/x` on the current
  drive, `C:` the drive's own directory) and PWD is the canonical
  native path in the case typed.
- setcwd updates OLDPWD, PWD, wd, cwd and the `=X:` entry for the new
  drive, which children inherit.
- `cd ''` is "null directory", status 1, in every mode (bash 5.3).
- pwd takes combined -L/-P/-W; -W (Windows modes only) prints the
  native form with `/`.
- The hidden shopt `msyspath` (-s, -u, -q, `shopt msyspath`, -O/+O)
  switches the mode at run time; it is absent from bare shopt,
  `shopt -p` and BASHOPTS. Switching re-renders PATH, PWD and the
  working directories and clears the command hash.
- `\w`, `\W`, jobs and dirs contract HOME ignoring case in the Windows
  modes.

Platform (main_win32.cpp): widepath passes a `\\?\` path through
untouched, since GetFullPathNameW strips the trailing dot that made it
literal.

Decision on paths.cpp's lexical items (issues.md, Accepted): the shell
stays lexical. It asks the file system only where the corpus shows
Git Bash depends on it: the `.exe` suffix rule and the virtual files.
Each listed item is a cygpath or child-argument corner no recorded
script reaches, and checking them would put os_* calls inside the
pure engine.

Tests:
- t/winvos baselines changed on purpose: native `cd /c/Windows` now
  fails (it is C:/c/Windows) and `cd c:/WORK` keeps the case typed
  (001); PATH and TEMP are imported with forward slashes while COMSPEC
  is not (002, 005); /dev/tty exists (004); 006's msys child path is
  spelled `C:/msys/...`; the msys persona prints /c/work, a POSIX PATH
  and /usr/bin/bash.exe (007).
- New t/winvos specs: 008-native-paths (`/x`, `C:x` by the drive's
  directory, `=C:` inherited, `;` PATH and CDPATH, pwd -W), 009-native-env
  (the import rules, an invalid name, identity, the hidden shopt and
  its re-rendering), 010-msys-paths (/dev, /proc, the suffix rule,
  HOME in any case, the root listing, the toggle). The personality
  environment gained `ProgramFiles(x86)` for the invalid-name rule, and
  an msys personality's command line is quoted the Cygwin way.
- main_test `test_pathmodes`: -O/+O msyspath and the msys re-split of
  a raw command line (globbing, quoted patterns, braces).
- t/win32/013-paths: cd/pwd both ways, `\\?\` paths, `2>nul`, reads of
  /dev/null and NUL, a trailing-dot name, the native persona as sh.exe.
  It found the two bugs fixed in 0a26827.

Results: tests.exe 149/0; t/exec 3413/0, with --chaos 20 42533/0;
t/parse 3045/0; t/unit 149/0; t/winvos 10 specs 198/0, with --chaos
20 798/0; paths_test 923855/0; wide_test 100581/0; t/win32 -x 26/0;
the WSL POSIX build without warnings (tests.exe, t/exec, t/parse,
t/winvos all 0 failed there); FUZZ_WIN replay of t/winvos, t/msys,
t/exec and t/win32 and the Linux FUZZ_CHAOS replay of the same 738
files clean (one expected out-of-memory exit, 608).

t/msys by topic, passed of total, before -> after (vos: the `# vos`
scripts under `./tests.exe t/msys/*.sh`; real: `t/msys/run`):

| topic                    | vos           | real          |
|--------------------------|---------------|---------------|
| cd, pwd (001-010)        | 0/9 -> 9/9    | 0/10 -> 9/10  |
| cygpath (011-015)        | 0/5 -> 0/5    | 0/5 -> 0/5    |
| home, tmp (016-019)      | 0/3 -> 2/3    | 0/4 -> 2/4    |
| argconv (020-028)        | 0/9 -> 0/9    | 0/9 -> 0/9    |
| envconv (029-034)        | 0/5 -> 0/5    | 0/6 -> 0/6    |
| msysq (035-037)          | -             | 0/3 -> 0/3    |
| type, exe (038-041)      | 0/3 -> 1/3    | 0/4 -> 1/4    |
| noacl, shebang (042-047) | -             | 0/6 -> 0/6    |
| crlf (048-051)           | 0/3 -> 0/3    | 0/4 -> 0/4    |
| options (052-053)        | 0/2 -> 0/2    | 0/2 -> 0/2    |
| dev, proc (054-057)      | 1/2 -> 2/2    | 1/4 -> 3/4    |
| claude (058-060)         | -             | 0/3 -> 0/3    |
| all                      | 1/41 -> 14/41 | 1/60 -> 15/60 |

The failures left belong to later steps: cygpath, CRLF and the
options to step 6; argument and environment conversion, argv[0], the
run-time suffix rule and `.exe` stripping in type to step 7 (017, 018,
034, 040, 055); 007 to the /proc listing (issues.md).

### Step 6: msys input and builtins

Input (parse.cpp's `dropcr`/`squeezecr`, applied in runsource and to
each line fillstdin reads, so every parse goes through it: scripts,
`-c`, eval, source, comsub and procsub bodies, traps, mapfile
callbacks):
- msys mode (`CR_ALL`) drops every CR, as Git Bash's parser does: in
  quotes, heredocs and `\` CR LF continuations too. `$'\r'` survives.
- native mode (`CR_CRLF`) drops only the CR of a CR LF pair, as
  busybox-w32's remove_cr; POSIX mode keeps everything.
- Command substitution (exec.cpp's capresult/trimnewlines) strips
  trailing `\n` and `\r\n` in both Windows modes, so `a\r\r\n` keeps
  one CR and a lone trailing CR stays. In native mode an unquoted
  comsub splitting on newlines also takes interior CR LF as one
  (expand.cpp's putcomsub).
- read: native mode ends a line at CR LF and continues one at
  backslash CR LF (with the default delimiter, not -N); msys keeps CRs
  (`read -r`, mapfile), as recorded.

Options (shell.cpp): `O_IGNCR` after history and
`S_COMPLETION_STRIP_EXE` after compat44, at Git Bash's positions.
`hasoption`/`hasshopt` hide them outside msys mode from set -o, shopt
(names, listings, -p, -o), SHELLOPTS, BASHOPTS, their import, `[[ -o
]]` and compgen; Linux and native keep bash 5.3's lists. `igncr(sh)`
is the option in msys mode. What Git Bash's igncr does, recorded in
t/msys/061: every CR leaves comsub output (before the trailing newline
trim) and what read stores (not counted by -n/-N, after the delimiter
test, and not an escaped one: `\` CR is a literal CR); mapfile keeps
them; `$-` gains no letter; SHELLOPTS lists it.

cygpath (paths.cpp's `cygpathopts`, `cygpathname`, `cygpathdir`,
`cygpathrun`, all pure; builtins.cpp's `b_cygpath`):
- -u (default), -w, -m (one form; two different ones are an error), -a,
  -p, -W, -S, -f FILE or `-f -`, -l and -s as no-ops, -h, `--`,
  clustered short options, `-fFILE`, and the long names (--unix,
  --windows, --mixed, --absolute, --path, --file[=F], --windir,
  --sysdir, --long-name, --short-name, --help). Usage and help text
  are our own.
- -a: "." and ".." name the directory without a trailing slash; -a
  of a relative name joins the working directory's POSIX form.
- An empty name prints "can't convert empty path" and stops, status 1.
  Git's cygpath buffers stdout until it exits except with -f, where
  each line is flushed: the builtin writes the error first or last to
  match the merged recordings.
- -f lines lose a final CR. -W and -S use SYSTEMROOT, else WINDIR,
  else C:/Windows.
- A `B_MSYS` builtin flag; `findbuiltin` and `compbuiltins` now take
  the shell, so cygpath exists only in msys mode (`type cygpath` says
  builtin, the accepted deviation). The shell started as cygpath (any
  directory, `.exe` or not, msys persona) runs the builtin over its
  own argv after the msys re-split and exits.

Merge note for step 7: findbuiltin's new Shell argument touches
exec.cpp (decl, assignments, simple commands, function definitions),
builtins.cpp (type, command, builtin, hash) and proc.cpp, one line each.

Tests:
- New t/msys recordings from Git Bash: 061-igncr (comsub, read, -n,
  -N, -d, mapfile, field splitting, with and without igncr), 062-crlf
  -corners (continuations, quotes, heredocs, `$'\r'`, eval, funsubs),
  063-cygpath-opts (-W/-S case-folded, -f FILE and -, CR LF input, -l,
  -s, `--`, -a, errors and statuses). All three `# vos`.
- t/winvos/011-native-crlf (hand-checked: the native CR rules, the
  5.3 option lists) and 012-msys-input (the msys rules through -c
  text, eval, traps, functions; igncr; the lists and SHELLOPTS/BASHOPTS;
  msyspath hiding them and cygpath; the long options; the shell copied
  as cygpath.exe).
- paths_test: every u/w/m/up/wp/mp golden record again through the
  builtin's parsing and run (287 more golden checks), 48 inline checks
  (options, -a, -p, -W/-S, -f, errors) and a property loop over random
  argument lists.

Results: tests.exe 149/0; t/exec 3413/0, with --chaos 20 42533/0;
t/parse 3045/0; t/unit 149/0; t/winvos 12 specs 208/0, with --chaos
20 928/0; paths_test 1124310/0; wide_test 100581/0 (--real 6/0);
t/win32 -x 26/0 (003-waits once missed a count under load, then
passed); unabashed.exe and unabashed32.exe without warnings; the WSL
POSIX build without warnings (tests.exe, t/exec, t/parse, t/unit,
t/winvos 0 failed); FUZZ_WIN replay of t/winvos, t/msys, t/exec and
t/win32 (743 files) and the Linux FUZZ_CHAOS replay of the same clean
(the expected out-of-memory exit, 608), FUZZ_WIN+FUZZ_CHAOS over
t/winvos and t/msys clean.

t/msys by topic, passed of total, before -> after:

| topic                    | vos           | real          |
|--------------------------|---------------|---------------|
| cygpath (011-015)        | 0/5 -> 5/5    | 0/5 -> 5/5    |
| crlf (048-051)           | 0/3 -> 3/3    | 0/4 -> 3/4    |
| options (052-053)        | 0/2 -> 2/2    | 0/2 -> 2/2    |
| new: 061-063             | 3/3           | 3/3           |
| all                      | 14/41 -> 27/44 | 15/60 -> 28/63 |

051 fails only on `cmd //c`, which needs step 7's argument conversion
(the CRs are right).

### Step 7: spawning by target kind

spawn.cpp (between glob.cpp and exec.cpp) decides how a program
starts on Windows; main_win32's os_spawn only runs CreateProcess with
the core's `Spawn.path` and `Spawn.cmdline`.

Search and the execute bit:
- `pathsearch` (moved from exec.cpp with `canexec` and `pathlookup`):
  msys mode tries `""` then `.exe`; native mode the bare name then
  `.com .exe .sh .bat .cmd` unless the name has one; POSIX unchanged.
  bash's file_to_lose_on rule holds: a non-executable match is
  remembered, found by type/command -v/-P and hashed, and running it
  says "Permission denied", 126 (checked in WSL).
- The platform's os_stat reports content bits only (`#!`, `MZ`,
  `:\n`; any `.exe` is taken to be MZ, unread); `winmode` adds the
  mode's rule: msys noacl is `.exe` or those contents, never
  `.com/.bat/.cmd` or empty files (recorded, 042-044); native is
  busybox-w32's do_lstat: an executable extension, `#!`, an APE
  header, or a PE image that is not a DLL with subsystem 1, 2, 3 or 7
  (a bare `MZ` is not).
- pathstat's msys `.exe` retry and `locate` (what to run): msys adds
  `.exe` to a missing name; native mode, as busybox's
  add_win32_extension does for explicit paths too, tries the
  extensions on `./mine` (checked against w64devkit's busybox sh,
  where `./mine` runs mine.exe with argv[0] `.\mine`).
- The sniff cache: `SniffCache`, a 256 KiB block from perm (off the
  journaled store, so subshells share it), a hash trie keyed by the
  ASCII-folded native path, valid while mtime and size match, started
  over when full. It holds the kind (script line, `:`, PE info, bare
  MZ, APE). bash's command hash is unchanged.

Targets (`wintarget`, by `pesniff` and the extension):
- Our own image (the `.unabash` section): its persona by the target's
  name and install (`personaof`, fstab at its root): Cygwin quoting
  for an msys persona, MSVCRT for a native one; the child environment
  of an msys child.
- MSYS2/Cygwin images: `quotecygwin` (quotes the empty word, blanks,
  quotes and the glob/brace characters, `@`/`~` first) and no argument
  conversion; the conv_envvars (PATH, ORIGINAL_PATH, LD_LIBRARY_PATH as
  lists; HOME, SHELL, TMPDIR, TMP, TEMP) in Windows form with
  backslashes, so the child imports them back.
- Native images from an msys shell: argv[0] the image's Windows path
  with backslashes (026), the rest through `msysargv` with the shell's
  environment (MSYS_NO_PATHCONV, MSYS2_ARG_CONV_EXCL), `msysenv` for
  the environment plus the `=X:` entries, SYSTEMROOT and WINDIR when
  absent (recorded: MSYSTEM is not added); MSYS2-style quoting.
- Native images from a native shell: MSVCRT quoting, argv[0] as typed
  with backslashes (busybox's spawnveq); the busybox alias stubs
  (CRT-free, at most 64 KiB, busybox.exe beside) are bypassed: busybox
  runs with the applet as argv[0], unless BB_OVERRIDE_APPLETS names it
  (`-` for all, `+` prefix, blanks/commas/semicolons); the libtool
  hack turns `cmd //c echo` into `/c`.
- The single-path conv_envvars of either kind of child get `.exe` when
  missing and the `.exe` is a file (recorded: SHELL=/usr/bin/bash
  reaches a child as `...\bash.exe`; arguments and other variables do
  not).
- `.bat`/`.cmd`: `%ComSpec%` (else `<windir>\System32\cmd.exe`) with
  `/d /e:on /v:off /s /c "..."`, arguments through `quotebatarg`
  (Rust's BatBadBut rules, now in core) after msysargv in msys mode;
  CR or LF in an argument is EINVAL.
- `crtslot`: a CRT image's handle width, kept in the Target for step 8
  (descriptors above 2 are not passed yet).

Shebangs (up to four nested levels, then ELOOP "bad interpreter: Too
many levels of symbolic links"): leading blanks skipped, the line ends
at CR or LF, the rest of the line is one argument. `/bin/sh`,
`/bin/bash`, `/usr/bin/sh`, `/usr/bin/bash` and `/usr/bin/env sh|bash`
self-spawn with `-O`/`+O msyspath` for the parent's mode; others are
found by path (mount table in msys mode, extensions in native), else
by basename on PATH; missing: "path: interp: bad interpreter: No such
file or directory", 126 (046). In native mode `/usr/bin/env PROG` runs
busybox's env stub (env searches PATH). A native `.sh` without `#!`
runs as `sh file` (self); other files without `#!` keep bash's
in-process ENOEXEC fallback (busybox would fail: accepted). `#!` alone
runs as shell code (045); `#!/usr/bin/env python` no longer does.

Errors and exit:
- E2BIG from a command line over 32766 UTF-16 units, or (XP) an
  environment block over 32767: "path: Argument list too long", 126.
  Exec errors now name the program's path, as bash does.
- `exitstatus` maps 0xC0000135 to 127 with WNOLIB; the core prints
  Cygwin's "IMAGE: error while loading shared libraries: DLL: cannot
  open shared object file: No such file or directory", the image's
  POSIX path in msys mode and the first import found in none of the
  image's directory, System32, SysWOW64, the Windows directory and
  PATH (recorded from Git Bash with a program importing a missing DLL).
- `exec prog`: os_exec spawns, waits and ExitProcess with the status.
  The program is in a kill-on-close job (Windows 8+: its own, also
  when it ignores Ctrl-C as a background job does; XP/7: one made here,
  with breakaway from the shell's job when allowed), cleared before the
  shell exits normally. CREATE_NO_WINDOW when the shell has no console.
- In msys mode a Windows-path argv[0] gives `$0` without `.exe`
  (`/usr/bin/bash`, 059).

type, command -v, hash: in msys mode the name found by the `.exe` rule
is shown without it (038-041); native shows the file found.

vos: Linux os_spawn enforces execve's limits (a string under 128 KiB,
all strings and pointers within 2 MiB) and a `getconf` fake reports
ARG_MAX, so execscript's E2BIG case passes (its trap order still
differs). The vos's Windows mode gives the platform's content-only
mode bits. The t/msys vos fixture spells C:\WINDOWS as the recording
machine did and has w64devkit's cc.

Tests:
- t/winvos baselines changed on purpose: `.bat` mode bits are the
  core's (unit), a bare `MZ` file is not executable and `./mine` runs
  mine.exe as `.\mine` (003), argv[0] backslashed (002, 006), type
  shows `/usr/bin/bash` (007).
- New hand-checked t/winvos specs: 013-msys-children (Cygwin quoting,
  conversions and exclusions, SYSTEMROOT/WINDIR, batch from msys),
  014-native-shebang, 015-msys-shebang (chains, rest-of-line argument,
  ELOOP, `.sh`, missing interpreters, `#!/usr/bin/env fake`),
  016-native-stubs (bypass, BB_OVERRIDE_APPLETS forms, `cmd //c`,
  E2BIG 126), 017-sniff-cache (rewrites of the same size).
- t/win32/014-spawn: argv.exe and argv32.exe from both shell widths,
  BatBadBut vectors through the real cmd.exe from both modes, shebangs
  to busybox (`#!BUSYBOX sh`, `#!/usr/bin/env argv`, `#!/bin/sh`,
  `.sh`), exec with kill-on-close (the shell killed by its Windows pid,
  the program dies), exec's status, a program importing nosuch.dll
  from both modes. The runner builds argv, argv32 and that program.
  MSYS2 programs cannot start under this sandbox's token (t/msys
  covers them with `-L`).

Spawn cost, 400 runs of `./argv.exe "a b" /c/x` in the sandbox, three
rounds each (pre-step-7 master d1a7e1c -> this step): msys mode
3.35 -> 3.49 ms per spawn (argument and environment conversion),
native 3.36 -> 3.37 ms.

Results: tests.exe 149/0; t/exec 3413/0, with --chaos 20 42533/0;
t/parse 3045/0; t/unit 149/0; t/winvos 17 specs 232/0, with --chaos 20
1252/0; paths_test 1124310/0; wide_test 100581/0; t/win32 -x 28/0, and
006, 008, 012, 014 twenty times each with -x, clean; unabashed.exe and
unabashed32.exe without warnings, importing KERNEL32.dll only; the WSL
POSIX build without warnings (tests.exe, t/exec, t/winvos, t/msys 0
failed); Linux FUZZ_CHAOS replay of t/winvos, t/msys, t/exec, t/win32
(749 files) clean but the expected out-of-memory exit (608); FUZZ_WIN
replay of the same clean but out-of-memory exits (016's 400 KB
command line, and 608, 633, 796 as before this step) and t/win32's
004 and 005, which loop until a Ctrl-C the vos never sends (as before;
the replay had stopped at 003 on the sniff cache's size, now sized to
the arena). The XP
laptop was unreachable (ssh timed out): unabashed32.exe's 014 subset
and XP/7's exec job path are unchecked there.

t/msys by topic, passed of total, before -> after:

| topic                    | vos            | real           |
|--------------------------|----------------|----------------|
| cd, pwd (001-010)        | 9/9            | 9/10           |
| home, tmp (016-019)      | 2/3 -> 3/3     | 2/4 -> 3/4     |
| argconv (020-028)        | 0/9 -> 9/9     | 0/9 -> 9/9     |
| envconv (029-034)        | 0/5 -> 5/5     | 0/6 -> 5/6     |
| msysq (035-037)          | -              | 0/3 -> 2/3     |
| type, exe (038-041)      | 1/3 -> 3/3     | 1/4 -> 4/4     |
| noacl, shebang (042-047) | -              | 0/6 -> 6/6     |
| crlf (048-051)           | 3/3            | 3/4 -> 4/4     |
| dev, proc (054-057)      | 2/2            | 3/4 -> 4/4     |
| claude (058-060)         | -              | 0/3 -> 3/3     |
| all                      | 27/44 -> 44/44 | 28/63 -> 59/63 |

(cygpath, options and 061-063 unchanged and green.) Left: 007 (the
/proc listing); 018, 034 and 037, where an MSYS2 child we start gets
its environment from the Windows block (Git Bash hands its children
POSIX values through the cygheap): `HOME='C:\Windows'` arrives
converted, and the child's /tmp is the real %TEMP% rather than the
test's TEMP (issues.md).

### Step 8: descriptors

**Inherited descriptors** (`OsInfo.inherit`, imported by sh_main as
F_OS files from 3 up, skipping any at 0-2, at FDMAX or up, or twice):
- POSIX (`inherited`, before initasync opens anything): one `poll`
  over 3 to min(RLIMIT_NOFILE, FDMAX), POLLNVAL marking the closed
  ones; F_GETFL for the access; then each moved with F_DUPFD_CLOEXEC
  to 10 and up, after all are found, so a duplicate cannot land on a
  number not yet probed.
- Win32: the CRT's lpReserved2 blob from GetStartupInfoW, decoded at
  our own pointer width (`decodecrtfds`). An entry from 3 up with
  FOPEN and a disk, char or pipe handle is taken; a handle that equals
  a standard or earlier one is duplicated, others are made
  non-inheritable. The blob gives no access mode, so both.
- vos: fake_bash passes its context's descriptors 3-63 (os_dup), with
  `vaccess` giving the mode.

**Passing them** (startproc): a CRT program (`Target.crtslot`, set for
MSVCRT/UCRT images, busybox's stub and our own shell, `selftarget`) gets
every descriptor below FDMAX in the blob: 0-2 closed there, so the CRT
sets them up from hStd* in text mode as without a blob, and 3 and up
binary (FOPEN, plus FPIPE/FDEV by GetFileType), as busybox-w32 passes
them. Verified with t/win32/015 and a probe of the raw blob:
- The slot width is the child's (PE32+ 8, PE32 4) from a 64-bit
  shell, but a 32-bit shell always writes 4-byte slots: for a 64-bit
  child WOW64 widens them itself, zero-extending each handle and
  keeping the bytes after them, so 8-byte slots came out 13 bytes
  apart (a 49-byte blob arrived as 69). Our 64-bit shell started by a
  32-bit parent thus decodes at 8.
- An MSYS2/Cygwin program gets no blob (crtslot 0) and no descriptors
  above 2: Cygwin passes those in its cygheap, and a Cygwin child
  ignores an MSVCRT blob (checked: busybox sh with fd 4 open starting
  Git's bash, `echo >&4` is "Bad file descriptor", `ls /proc/self/fd`
  shows 0-3).
- Descriptors above 2 go nowhere else on Windows: runprogram skips
  them for a non-CRT target, and always skips bridged ones (below).

**Named pipes**: `os_namedpipe(os, a, name, how)` makes an instance of
`//./pipe/unabashed-PID-HEX` (fresh, unguessable, FIRST_PIPE_INSTANCE)
or another instance of `name`; NP_OUT for the server writing, NP_NOBUF
for no buffer (a write completes only as the client reads it).
PIPE_REJECT_REMOTE_CLIENTS is dropped when XP rejects it. IO_CONNECT
waits for the client in a helper (H_CONNECT, ConnectNamedPipe on the
object's own handle; ERROR_PIPE_CONNECTED and ERROR_NO_DATA count as
connected). os_cancel ends that wait by becoming the client
(CreateFileW on the saved name; if that fails a real client came, and
the Io completes by itself). The vos has the same under `//./pipe/`:
instances in creation order, one client each, E_BUSY when all are
taken, E_ACCES for the wrong direction; it buffers NP_NOBUF writes.

**Bridges** (exec.cpp): for a native program (not our own shell, not an
MSYS2 program, not a spawnto stage), an argument that is `/dev/fd/N`
with N from 3, whole or after its last `=`, becomes `\\.\pipe\NAME`
(one bridge per descriptor, at most 16), and wintarget runs again on
the new argv. A pump serves the pipe: PUMP_SERVE copies what the
descriptor reads (an OS handle, a pipe's OS form, a materialized
string) to the client; PUMP_TAKE copies what a client writes into the
descriptor's pipe or handle. Decisions:
- Clients come in turn, two more instances listening ahead (NSPARE),
  since a program may open the path once only to look before it opens
  it to use. busybox diff opened it twice (with one instance its
  second open failed); Git's cat three times, in order: look, use, look
  again. Instances go to clients oldest first, on XP too (probed with
  a three-instance server).
- Serving instances are unbuffered, so a client that leaves without
  reading takes nothing: the write fails, and the next client gets the
  same bytes. A writing bridge drains, at its program's exit, the
  clients that connected by then (a deadline of now on its connect),
  so what a short-lived writer left in the pipe is not lost.
- The program's exit ends the bridge (endbridge): a serving bridge is
  cancelled (its spares closed first, so the cancelling client can
  only reach the instance waited on); a taking one drains as above.
  On an escape, unconnected bridges are cancelled and all orphaned.
- A bridged descriptor is not also passed in the blob: lending the
  pipe to the program aborted the pump's own read of it (E_INTR), and
  busybox cat then saw an empty second file about one run in four.
- MSYS2 programs are left unbridged. They do take `\\.\pipe\NAME`
  (`//./pipe/NAME` is "No such file or directory"), but with
  unbuffered instances Git's cat read nothing (it finds the pipe empty
  and stops), a probe server with buffered ones worked, yet with
  buffered serving instances diff and an empty procsub still hung at
  times, `tee >(...)` cannot open one for writing (EINVAL, truncation
  of a file), and head seeks back in it. A hang is worse than the
  error they get now (issues.md).

Tests: t/win32/015-fds (curated; inheritance to our shell at both
widths and from busybox, CRT programs of both widths from both shell
widths with argv's fd probe, procsubs to busybox and argv, a write
procsub, an unopened one, the native persona and the 32-bit shell);
main_test's vwin named-pipe test (instances, busy, access, reuse);
t/winvos/024-descriptors (hand-checked: the fd probe, bridges);
t/winvos/006 (argv sees fd 3). On the XP laptop 015 normalizes argv[0]
quoting (spaces in the directory) as 014 does.

015 hung in about half the runs on the XP laptop (one bridged
`busybox wc -l <(seq 100000) >file` in 50-100), never on Windows 11.
Traced: the bridge pump's writes to its client went through the private
duplicate (Wobj.own) of the procsub's OS pipe, so they looped back into
the pipe the pump read, its reads ended early, and wc's instance was
never closed. One Wobj served both handles: os_close drained the wake
queue after unregistering the last handle, a helper's operation ending
there released the object (closing = 2), and os_close's `closing |=
!refs` made it 3, defeating maybefree's never-twice check, so the
object went onto the free list twice. It needs a helper completion
pending at that close, which the single-core XP laptop's timing gives.
Fixed with a plain `freed` mark (reset by newobj). Not modelled in the
vos, which has no Wobj layer. After: 1200 such commands without a hang,
015 10 of 10 there.

| suite        | before | after |
|--------------|--------|-------|
| t/win32 (-x) | 28/28  | 30/30 |
| t/msys real  | 68/72  | 68/72 |

### Step 9: startup files and the time zone

Startup files (unabashed.cpp, "Startup files"), read after `$0` and the
positional parameters are bound, as in both bash and busybox:
- bash and msys modes follow bash: a login shell (`-l`, `--login`) reads
  /etc/profile and the first existing of ~/.bash_profile,
  ~/.bash_login, ~/.profile (a directory counts and says "is a
  directory"); a leading `-` in `$0` makes a login shell, whose files
  are read only when it is also interactive; an interactive non-login
  shell reads ~/.bashrc, or `--rcfile`/`--init-file`'s file, unless
  `--norc`; a non-interactive one `$BASH_ENV`. `--noprofile` skips the
  login files. Named sh: /etc/profile and ~/.profile for a login shell,
  `$ENV` for an interactive one (also with `--norc`), then POSIX mode;
  `--posix` reads no login files and only `$ENV`. In msys mode /etc
  goes through the mount table, and as in Git Bash an interactive shell
  reads /etc/bash.bashrc before ~/.bashrc (also with `--rcfile`, not
  with `--norc`). Recorded from bash 5.3 (t/exec/870-873) and Git Bash
  (t/msys/070-076).
- `return N` in a startup file leaves `$?` as it was before the return
  (bash's behaviour, recorded); a syntax error leaves 2 and the shell
  goes on; `exit` there exits. An interactive shell's errors are
  prefixed by the shell's name alone, as bash's.
- Logout: a login shell's `exit` (also inside a function, never in a
  subshell, not at the end of `-c`) reads ~/.bash_logout and in msys
  mode /etc/bash.bash_logout, before the EXIT trap; the status stays
  unless the file exits. `logout` outside a login shell: "logout: not
  login shell: use `exit'", 1. `shopt -s/-u login_shell` is ignored
  quietly (read-only).
- `bash -i` with no terminal on stdin or stderr prints bash's two
  job-control complaints, as the recordings show; a terminal on either
  stream stands in for bash's /dev/tty (t/win32/008 has the console on
  stdin and stderr in a file).
- Native mode follows busybox-w32 (hand-checked against w64devkit's
  busybox sh): a login shell (`-l`, `--login`, `$0` starting with `-`,
  or a name starting with `l`, as lash) reads `<system
  drive>/etc/profile`, `<exe dir>/../etc/profile` and `$HOME/.profile`
  (no tilde; none when HOME is unset), missing files and directories
  quietly; an interactive shell then reads `$ENV` (expanded); no
  BASH_ENV, no job-control messages. personaof now strips a leading
  `-`, so `-sh` is native too. Deviations: `--noprofile`/`--norc` are
  accepted and honoured (busybox: "bad option", 2) and `logout` exists;
  a syntax error in a profile is not fatal (busybox exits 2).

The system zone:
- `os_localtz` returns the system zone as a TZ value. Win32:
  GetDynamicTimeZoneInformation (resolved at run time, for
  DynamicDaylightTimeDisabled) else GetTimeZoneInformation (XP),
  rendered by the core's `wintz` (b_printf.cpp) as a POSIX TZ string:
  each name's capitals when there are at least three (Cygwin's rule:
  "Eastern Standard Time" is EST), else `<+hhmm>`; offsets from Bias
  plus StandardBias/DaylightBias; rules `Mm.w.d` for day-of-week
  dates, `Jn` for fixed ones, `/time` unless 02:00. POSIX:
  `:/etc/localtime` when it is a regular file (through links), else
  UTC0. The vos: `Vos.localtz`, set by a spec's `# localtz:` line,
  else UTC0.
- `shellzone` (exec.cpp) picks the zone for printf `%(...)T` and the
  prompt's time escapes: the exported TZ, else the system zone (which
  is never exported). Outside native mode a TZ naming a file
  (absolute, `:` prefix, or under TZDIR in POSIX mode, else
  /usr/share/zoneinfo) is read as a whole TZif file (`parsetzif`: v1
  or v2+, transitions up to the last, the footer after, glibc's
  first-standard-type rule before the first), so historical dates
  agree with glibc, which the old footer-only read got wrong.
  Unparsable: GMT in msys mode (Cygwin), UTC otherwise. Native mode
  has no zoneinfo; TZ is a POSIX string.
- Git Bash has no zoneinfo, so its zone names are GMT (recorded, 078),
  and with TZ unset it shows the GetTimeZoneInformation capitals
  (077 compares with `date`).
- On the XP laptop the registry's DaylightName is "Eastern Standard
  Time" (an XP quirk after the DST updates), so summer shows EST at
  -0400, as Cygwin's same rule would; its rule is the pre-2007 US one,
  which is XP's own data.

Tests:
- t/exec/870-874 (recorded from bash 5.3): login files and their order,
  dash login, `--noprofile`, `return` and errors in profiles;
  interactive rc files, `--rcfile`, `--norc`, BASH_ENV; sh-named and
  `--posix` rules with ENV; logout cases; TZif files with history,
  footers, TZDIR. The vos's Linux tree gains the effect of the oracle's
  /etc/profile (Debian's PATH).
- t/msys/070-078 (recorded from Git Bash, which passes them): login,
  login with `-c`, interactive, rcfile, logout, BASH_ENV, norc, the
  local zone, TZ strings. lib.sh gains NAME.d trees, a private install
  for tests with their own etc, and `# env:` lines (tools/README.md).
- t/winvos (hand-checked): 020-native-startup (busybox's three
  profiles, lash, dash, ENV, no BASH_ENV, return status, missing and
  directory profiles, unset HOME), 021-msys-startup (/etc through the
  mounts, bash.bashrc, rcfile, norc, logout files, sh-named),
  022-msys-localtz and 023-native-localtz (`# localtz:`, the GMT
  fallback, no export to children).
- printf_test: `wintz` table cases (DST and not, southern hemisphere,
  half hours, fixed dates, odd times, a bad month), each parsed back;
  `parsetzif` on synthetic TZif files (v1, v2, no transitions, bad
  footers, truncation, bad indices and order) and 100000 random
  mutations of them.

Results: tests.exe 149/0; t/exec 659 specs 3438/0, with --chaos 20
42858/0; t/parse 3045/0; t/winvos 21 specs 252/0, with --chaos 20
1512/0; t/msys vos 45/45; printf_test 837/0; paths_test 1124310/0;
t/win32 -x 28/0; unabashed.exe and unabashed32.exe without warnings,
importing KERNEL32.dll only; the WSL POSIX build (tests.exe, t/exec,
t/winvos, t/msys 0 failed; printf and the prompt agree with `date` for
/etc/localtime and a TZ name). XP laptop: unabashed32.exe shows the
system zone (EST, above) and reads ~/.profile for `-l`. (fakes_test
does not build, before this step too: vos.cpp needs paths.cpp.)

| t/msys            | vos   | real           |
|-------------------|-------|----------------|
| startup (070-076) | -     | 7/7            |
| tz (077-078)      | 1/1   | 2/2            |
| all               | 45/45 | 59/63 -> 68/72 |

### Step 10a: t/msys green

The four failures left after step 9 (007, 018, 034, 037), and the
32-bit run.

What Git Bash hands its MSYS2 children, found by probing Git's own
programs from busybox (curated commands, outside the sandbox):
- Its POSIX environment and its mount table, through the cygheap
  (child_info in lpReserved2, with the handle of the shared table).
  Neither can be built by a non-Cygwin parent.
- A child of a non-Cygwin parent instead imports the Windows block:
  HOME, TMP, TEMP, TMPDIR, SHELL, PATH and the other conv_envvars are
  converted from any Windows spelling (`C:\x`, `C:/x`, `C:x`, `\\?\C:\x`,
  `c:\x`; MSYS2_ENV_CONV_EXCL and MSYS_NO_PATHCONV do not stop it),
  a value starting with `/` is kept, other variables are kept as is.
- Its mount table is per install, keyed by its DLL's directory (a
  junction resolves to the target, so it shares the key; a hard-linked
  copy has its own) and per user, and the first live process of an
  install builds it: from `<root>/etc/fstab`, with the usertemp `/tmp`
  from that process's TMP, else TEMP, else USERPROFILE (TMP=/tmp makes
  it C:/tmp). Probed with a private copy: children with TMP C:\foo1,
  then C:\foo2, then C:\foo4 while one with C:\foo3 still ran, saw
  /tmp at foo1, foo2 and foo3.

So 018 and 034 failed on the harness, not the shell: Git's install had
a dozen Git Bash processes alive (the user's), whose table put /tmp at
the real %TEMP%, so Git's printenv, env and cat started by our shell
saw /tmp there (HOME and PATH under the WORKDIR in %TEMP% became
/tmp/claude/...), while the copied Git Bash under test had a table of
its own that its children inherited. Fix (t/msys/lib.sh): WORKDIR/git,
a clone of Git's install (usr/bin hard-linked, etc copied, rebuilt when
Git's files change), lends the programs, so each test's first MSYS2
child builds a fresh table from Git's fstab with the test's TMP. The
shell keeps passing the conv_envvars in Windows form: a child that
builds its own table converts them back to the POSIX forms Git Bash's
children see (TEMP=/tmp, HOME and PATH as in the shell), and passing
POSIX forms instead would break exactly that child (TMP=/tmp mounts
/tmp on C:/tmp) and send PATH to the loader in a form it cannot use.

037's `HOME='C:\Windows' printenv HOME` cannot match: C:\Windows
reaches a child only through a Cygwin parent. Accepted (issues.md),
and the line now assigns a Windows path to a variable the runtime does
not convert (re-recorded with gbrecord; the spec is unchanged, as both
print C:\Windows). A trampoline through the child install's env.exe
would pass it, at a process per such command; not done.

007 (glob.cpp's proclist, expand.cpp's procnames and proctype,
exec.cpp's pathstat): /proc lists Cygwin's fixed entries (codesets,
cpuinfo, cygdrive, devices, filesystems, loadavg, locales, meminfo,
misc, mounts, net, partitions, registry, registry32, registry64, self,
stat, swaps, sys, sysvipc, uptime, version, as Git Bash lists them)
and the shell's pid and BASHPID; stat knows which are directories;
/proc/self lists exename, fd and winexename. The fixed files have no
contents (a read is EACCES) and the subdirectories list nothing
(issues.md).

32-bit: 054 failed only on MACHTYPE, HOSTTYPE and BASH_VERSINFO[5]
naming i686, the build's architecture (step 5). t/msys/run now maps
i686 to x86_64 when the shell under test is a 32-bit image (the PE
header's machine), and the deviation is listed as Accepted.

Tests: t/winvos/025-msys-proc (hand-checked against Git Bash: the
listings by glob and from inside /proc, the types, the own pid, the
unreadable file). Git Bash itself still passes 72/72 with the clone
(`t/msys/run -s "$GITBASH"`).

Results: unabashed.exe, unabashed32.exe and tests.exe without
warnings; tests.exe 159/0; t/exec 659 specs 3448/0, with --chaos 20
42868/0; t/parse 3055/0; t/unit 159/0; t/winvos 23 specs 272/0, with
--chaos 20 1652/0; t/msys vos 45/45; t/win32 -x 30/0.

| t/msys            | vos   | real           | real, 32-bit   |
|-------------------|-------|----------------|----------------|
| all               | 45/45 | 68/72 -> 72/72 | 72/72          |

Exit criterion 1 (the Git Bash comparison) holds.


### Step 10b: the w64devkit trial

tools/w64trial (tools/README.md) builds autoconf projects with a
mirror of w64devkit whose sh.exe is the shell (kit-ub) and with one
whose sh.exe is the stock stub to busybox (kit-bb), each phase under
tools/sandbox, then compares config.h and config.status's
substitutions. The mirrors hard-link bin and junction the rest; both
shells get one, since gcc pays about 7 ms a run reaching lib and
libexec through the junctions. Sources: GNU make 4.4.1 and xz 5.6.4
(release tarballs, checksums as published).

Trial setup, not the shell:
- make 4.4.1's gnulib breaks under GCC 15's C23 (`getenv ()`), with
  either shell: CFLAGS=-std=gnu17.
- autoconf needs w64devkit's etc/profile (PATH_SEPARATOR=';',
  ac_executable_extensions, build_alias); each phase sources it, as a
  login shell does. Set through the environment, the import would
  uppercase their names.
- Path, ComSpec and SystemRoot are spelled as Windows does, or neither
  shell imports (slashes) the environment.
- busybox fails gnulib's POSIX shell test (`$(umask)` is a new process
  that always says 0002), so xz's configure stops with stock w64devkit;
  the busybox run gets gl_cv_posix_shell=/bin/sh. Ours passes the test.

Shell bugs found and fixed:
- `$SHELL` as `/bin/sh` (configure's config.sub calls, 127): in native
  mode a missing /bin/X, /usr/bin/X, /sbin/X or /usr/sbin/X is X by
  name, sh, bash and ash being the shell (busybox-w32's unix_path).
  t/winvos/026-native-unixpath.
- The alias-stub test (small, CRT-free, beside busybox.exe) took
  u-config and cc.exe (the alias for gcc) for busybox stubs, so
  pkg-config ran as an applet. Stubs are now known by their message
  naming busybox.exe in ASCII (their UTF-16 path is built in code, not
  stored whole). t/winvos/016 runs a CRT-free non-stub.
- main_win32.cpp renamed Path to PATH before the core saw it, so the
  native import (values slashed, only for a Windows parent's
  environment) never ran on real Windows: configure found backslashed
  MKDIR_P and the shell read the backslashes as escapes. t/win32/013.

Results: make and xz configure, build and pass make check with both
shells, with equal config.h and substitutions. Single runs (ms):

| phase     | make bb | make ub | xz bb | xz ub |
|-----------|---------|---------|-------|-------|
| configure | 27068   | 20538   | 37743 | 22362 |
| make -j8  | 3289    | 2904    | 42286 | 22026 |
| check -j8 | 752     | 540     | 9300  | 4160  |

The configure workload (exit criterion 4), `w64trial -n 5`, medians
of 5 with the shells alternating:

| configure | busybox sh | unabashed | ratio |
|-----------|------------|-----------|-------|
| make      | 24910      | 19302     | 1.29x |
| xz        | 31301      | 19011     | 1.65x |

Per operation outside the sandbox: a trivial program 16 ms from busybox,
3.4 ms from us (busybox's waits poll); a busybox applet 7.0 ms and
6.2 ms; `x=$(echo hi)` 6 ms and 0.05 ms; a gcc compile and link 83-92
ms and 72-74 ms. Configure spends most of its time in gcc.

Exit criterion 2 holds.


### Step 10c: the agent batch gate

bench/agent gained a `floor` mode (tools/argv.cpp, a do-nothing CRT
program, started as the shell is) and prints `own`, full - ext - floor
per command, the gate's measure. Its busybox driver turned out to skew
it: busybox's waits poll, so a program it starts costs about 16 ms
against 3.4 ms from us. `full` and `floor` share that and it cancels,
but `ext` (the externals alone, run by the driver) carries it once per
external, more than once per command, and with it our own time came out
negative (28.6 - 19.0 - 17.1 ms). `-d DRIVER` runs the batch under a
native-mode shell with exact waits instead, ours as sh.exe.

That run found a bug: after a few hundred distinct programs the shell
died out of memory. alloc wants room for one more object than it takes,
and the sniff cache's "start over when full" test allowed for one, so
between one and two entries' worth left it neither started over nor
fit. Fixed (spawn.cpp), with t/winvos/027-native-sniffcache.

Results (Windows 11, 52 commands, medians of 5 rounds, our driver, ms
per command):

| mode                       | Git Bash | unabashed |
|----------------------------|----------|-----------|
| full (what the agent waits)| 35.8     | 18.9      |
| noop (-c WRAPPER(:))       | 13.8     | 4.1       |
| exit (-c exit)             | 11.9     | 3.6       |
| floor                      | 3.4      | 3.5       |
| ext (externals alone)      | 11.6     | 11.6      |
| own (full - ext - floor)   | 20.7     | 3.7       |

Three such pairs gave own ratios of 5.55x, 5.19x and 5.53x, and full
1.86-1.89x. Exit criterion 3 holds, narrowly. Of our 3.7 ms, 0.6 is
startup and wrapper above the floor; the rest is that the first
CreateProcess in a fresh process costs Windows about 1.5-2 ms more than
later ones (measured with a standalone program: plain, thread-pool and
job waits cost the same), which each per-command shell pays and the
long-lived driver does not. Git Bash pays it too.

With the busybox driver, for comparison: Git Bash full 44.6, noop 25.1,
exit 22.3, floor 17.2, ext 19.0; ours full 28.6, noop 18.1, exit 17.0.
