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

For the user, when they come up:
- Trial projects beyond local binutils-gdb libiberty and zlib (xz and
  GNU make tarballs need a download).
- If external programs cap the agent-batch ratio below 5x Git Bash,
  how to redefine that gate (step 3's baseline will tell).
- If Git Bash will not run under the Low-integrity sandbox: a `-L` mode
  (no integrity lowering, job and timeout kept) for recording only.


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
