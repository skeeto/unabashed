# tools

## bash 5.3 oracle

Expected outputs come from real GNU bash 5.3, built in Docker from the
repository's `bash-5.3.tar.gz`. Only our own scripts and the recorded
outputs are committed.

### Image

Build or rebuild from the repository root. This works from Windows
(w64devkit, Git Bash, PowerShell) or WSL against the same Docker daemon:

    docker build -f tools/Dockerfile.bash53 -t unabashed/bash53 .
    docker run --rm unabashed/bash53 bash --version

The build checks the tarball's SHA-256. The image holds bash in
`/usr/local/bin/bash` (default `./configure`), the test helpers `recho`,
`zecho`, `printenv` and `xcase` from bash's `support/` beside it, and a
user `oracle` (uid 1000).

`/opt/fakes/bin` holds the fakes: `fakes.cpp` built with
`main_fakes.cpp` as one multi-call binary, `/opt/fakes/bin/fakes`, plus a
symbolic link to it for each program (`cat`, `sed`, `od`, ...; `fakes
--list` prints them). The vos runs the same code in process, so specs
recorded with the fakes on the PATH agree with vos runs by construction.
They build in a stage of their own, so editing `fakes.cpp` leaves the
cached bash build alone; rebuild the image after changing the fakes.
Debian's coreutils remain in `/usr/bin` for comparison, but are not on
the recording PATH. `t/unit/fakes_record.sh compare` runs the fakes'
unit cases against them. `FAKES_CASES` and `FAKES_GOLDEN` point that
script at another case list and golden file, and
`FAKES_PATH=/usr/local/bin:/usr/bin:/bin` records the real coreutils
instead, so that `fakes_test DIR` checks the fakes' fidelity on the vos
directly. Each case gets 30 seconds (status 124 after that), since real
tools can block (`sleep inf`) or run on forever (`seq -inf 1`).

### Recording

    tools/record t/foo.sh [t/bar.sh ...]

This writes `t/foo.spec` next to each script. Each script runs twice,
each time in a fresh container: once with stdout and stderr apart, and
once with `2>&1`. Both runs must exit with the same status. Each run
looks like this:

- `bash --norc --noprofile -c SCRIPT bash`, so `$0` is `bash` and
  diagnostics read `bash: line N: ...`
- stdin is `/dev/null`
- `env -i LANG=C TZ=UTC HOME=/home/oracle PATH=/usr/local/bin:/opt/fakes/bin`,
  so external commands are bash's helpers or the fakes, never coreutils
- uid 1000, hostname `oracle`, umask 022, `ulimit -n` 1024
- `--network none`, `--read-only`, all capabilities dropped
- `/tmp` and the workdir `/home/oracle` (also `$HOME`) are empty tmpfs
- under `--init`, so bash is not pid 1; its pid is stable (7)

Settings come from environment variables: `RECORD_IMAGE` (default
`unabashed/bash53`), `RECORD_PATH` (the PATH inside the container) and
`RECORD_TIMEOUT` (default 60 seconds per run). The exit status is
nonzero if any script failed to record. A failed script keeps its old
spec.

### Specs on the POSIX build

    RECORD_SHELL=unabashed RECORD_CHECK=1 tools/record t/exec/*.sh

`RECORD_SHELL` runs a Linux build of our shell (`make unabashed`, in WSL
or Linux) in bash's place, mounted read-only over `/usr/local/bin/bash`
in the same container, so that scripts' own `bash -c` run it too;
`RECORD_CHECK` compares the result with each spec instead of writing
it, printing `PASS` or `FAIL` per script. Several instances can share
the work: `ls t/exec/*.sh | xargs -P 6 -n 25 env RECORD_SHELL=...
RECORD_CHECK=1 tools/record`.

### Spec format

A spec is a series of blocks in this order: `stdout`, `stderr`,
`merged`, `status`. `merged` comes from the `2>&1` run.

    == stdout 3
    hi
    
    == stderr 4
    err
    
    == merged 7
    hi
    err
    
    == status 3

A stream block's header is `== NAME LEN`. Exactly LEN bytes of raw
output follow, then one `\n` that is not part of the output. The count
keeps the format exact when output has no final newline or contains
`==` lines. `== status N` is a single line holding the exit status;
death by signal records 128+N.

## bash's test suite on the POSIX build (oracle-posix)

    tools/oracle-posix [-v] SHELL [NAME...|all]

Runs bash 5.3's run-NAME tests (category c of notes/research/tests.md
by default, the ones needing real processes; `all` for every one) as
run-all does, `sh run-NAME` in the suite's directory with `THIS_SH` the
given Linux build of our shell. A test passes if it prints nothing but
`warning:` lines; `-v` prints a failure's output. One container from
the oracle image, as for recording (`--network none`, read-only root,
tmpfs `/tmp` and workdir, uid 1000, no capabilities); the suite is
unpacked from `bash-5.3.tar.gz` afresh for each test. PATH has bash's
helpers and Debian's coreutils, not the fakes. `ORACLE_TIMEOUT` limits
each test (300 seconds). Real bash, given as SHELL (copy
`/usr/local/bin/bash` out of the image), fails a few here too (signal
listing order, no terminal, no en_US.UTF-8, ulimit): results are read
against its own.

## fuzz (WSL or Linux, AFL++)

    tools/fuzz MODE [MINUTES [CORES]]

Builds `main_fuzz.cpp` in one of its modes (`run`, `parse`, `chaos`,
`isolate`; see its header) with `afl-clang-fast++` from a snapshot of
the working tree in `~/fuzz-MODE`, seeds it with the small spec scripts
and runs AFL++ on CORES cores (default 60 minutes, 2 cores), then counts
the crashes. Every mode runs scripts only on the vos. A plain build of
the same mode replays files: `g++ -std=c++20 -g3 -DFUZZ_CHAOS -o replay
main_fuzz.cpp; ./replay crash...`.

## sandbox (Windows)

`sandbox.cpp` confines the curated Windows integration tests, which need
the real filesystem and CreateProcess. It is CRT-free and XP-capable:

    cc -nostartfiles -o sandbox.exe tools/sandbox.cpp -ladvapi32
    sandbox [-L] [-E NAME=VALUE]... [-m MIB] [-p COUNT] [-t SECS] \
            DIR -- PROGRAM [ARGS...]

It creates DIR (with parents) and labels it Low integrity, inherited by
its contents, then runs PROGRAM in DIR with a copy of the caller's token
lowered to Low integrity, with every privilege but SeChangeNotify removed
and Administrators deny-only. The child can read what the user can, but
can write only Low places: DIR, `AppData\LocalLow` and
`HKCU\Software\AppDataLow`. Not `%TEMP%`, so point TMP/TMPDIR into DIR
for tests that need temporary files.

The child runs in a job object with kill-on-close, so the whole tree
dies when PROGRAM exits, on timeout, or when the sandbox itself is
killed. `-m` caps the tree's committed memory, `-p` its live process
count, and `-t` its wall time. Crashes end the process instead of
opening an error dialog.

Everything after `--` passes through verbatim. PROGRAM is looked up
from the caller's directory and PATH, not DIR. Standard handles are
inherited. The status is PROGRAM's, or 124 on timeout, 125 on a sandbox
error, 126 if PROGRAM cannot run, 127 if it is not found.

On XP there are no integrity levels: the sandbox warns and relies on
the job object and the restricted token alone, which does not stop
writes to the user's own files. DIR must be on NTFS (Vista and later).
Low integrity also blocks some unrelated things, such as ICMP, so the
Windows `ping` fails inside the sandbox. A program started from a file
with a Low label runs at Low integrity even outside the sandbox, so
keep tools (such as `sandbox.exe` itself) out of DIR.

`-E NAME=VALUE`, repeatable, gives PROGRAM an environment made of
exactly those variables (sorted as Windows sorts them; for a repeated
name the last wins) instead of the caller's, for reproducible runs.

`-L` keeps the caller's integrity level: the token is still restricted
(no privileges, Administrators deny-only) and the job, its limits and
the timeout still apply, but writes are no longer confined to DIR. It
exists because MSYS2 programs, Git Bash included, cannot start at Low
integrity: msys-2.0.dll creates its shared-object directory
`\BaseNamedObjects\msys-2.0S5-<key>` at startup, which the session's
namespace denies to Low (`NtCreateDirectoryObject ... 0xC0000022`, the
process then fails with 0xC0000142). A write-restricted token (logon
SID only, granted on DIR) fails the same way. Use `-L` only for
curated scripts: t/msys and bench/agent.

## conrun (Windows)

`conrun.cpp` runs a program on a console of its own, so that tests can
type at it and send it Ctrl-C without a visible window. It is CRT-free
and XP-capable:

    cc -nostartfiles -o conrun.exe tools/conrun.cpp
    conrun [-t SECS] [ACTION...] -- PROGRAM [ARGS...]

It starts itself again with `CREATE_NO_WINDOW` (a new console without
a window), opens that console's input for PROGRAM's standard input,
passes its own standard output and error through, and performs the
actions in order: `sleep:MS`, `await:PATH` (until the program creates
PATH, the reliable way to synchronize), `type:TEXT` (key events through
`WriteConsoleInputW`; `\n` Enter, `\z` Ctrl-Z, `\uXXXX` a UTF-16 unit,
`\` a backslash), `ctrl-c` and `ctrl-break` (`GenerateConsoleCtrlEvent`
to the whole console). The status is PROGRAM's, 124 if it outlives the
actions by SECS (default 30) and is killed, 125 on an error.

An event from `GenerateConsoleCtrlEvent` is not a key typed at the
console: it does not end a console read in progress, which a typed
Ctrl-C does. Tests send Ctrl-C while the shell runs commands, and the
keyboard case is checked with winctl (notes/m3b.md).

## Windows integration tests (t/win32)

    t/win32/run [-s SHELL] [-x] [-u] WORKDIR [TEST.sh...]

Curated scripts only, never generated ones, on the real file system and
processes, each run by the shell under test (default `unabashed.exe`)
inside `sandbox` in `WORKDIR/NAME`, which must be outside the
repository (use the scratchpad). `sandbox.exe` and `conrun.exe` are
built into `WORKDIR/bin` from `tools/`. Output and errors are merged,
`status N` appended, and compared with `NAME.out` (`-u` rewrites the
expected outputs; check them by hand). `-x` runs every test a second
time with `UNABASHED_XPIO=1`, which makes the Win32 layer take its XP
code paths (no `CancelSynchronousIo`). A test's comments may hold
`# conrun: ACTION...` (run it under conrun, its input a hidden
console; words split as by the shell) and `# shell: ARGS` (run
`SHELL ARGS` instead of `SHELL test.sh`). `$U` is the shell's own path,
for tests that run it as a program. The suite runs under w64devkit's
`sh` on Windows 11 and on the XP laptop (`~/src/scratch`); there the
sandbox warns that XP has no integrity levels, which the runner
filters out.

## argv (Windows)

`argv.cpp` shows what a typical native program receives. Unlike the
other tools it keeps the normal CRT startup, so msvcrt.dll splits the
command line (mingw-w64 `__wgetmainargs`, no wildcard expansion):

    make argv.exe argv32.exe

It prints to standard output, in UTF-8, each line ending in a bare LF:

    cmdline = <RAW>    GetCommandLineW(), as received
    argv[I] = <ARG>    one line per CRT argument, I counting from 0
    NAME=VALUE         for each name in $ARGV_SHOW, in the order given
    NAME unset         ... for a name that is not set
    fd N open          with ARGV_FD=N, if CRT descriptor N is open,
                       after which "argv: fd N\n" is written to it
    fd N closed        ... if it is not

Text between `<` and `>` is raw, with no escaping (it may hold newlines
or `>`). ARGV_SHOW holds names separated by spaces or commas, looked up
as Windows does, ignoring case; the name prints as given. A lone UTF-16
surrogate prints as U+FFFD. The status is 0, or 1 if writing failed.
The vos's argv fake (M5a step 4) prints the same format.

## Git Bash differential tests (t/msys)

Curated scripts only, recorded from Git Bash and replayed against the
shell under test on the real file system, never generated or random
input. Both tools run under w64devkit's `sh`; `GITBASH` names Git
Bash's real shell (default `C:/Users/wellons/git/usr/bin/bash.exe`;
`bin/bash.exe` is only the git-wrapper launcher).

    tools/gbrecord WORKDIR t/msys/NAME.sh...     # writes t/msys/NAME.spec
    t/msys/run [-s SHELL] [-x] WORKDIR [TEST.sh...]

WORKDIR must be outside the repository and spelled `C:/...` (a `/c/...`
spelling would make w64devkit's sh create `C:/c/...`); use the
scratchpad. `-s` picks the shell under test (default `unabashed.exe` in
the repository root); `t/msys/run -s "$GITBASH"` checks the recordings
against Git Bash itself and must pass. `-x` runs each test again with
`UNABASHED_XPIO=1`. A failure prints the diff of the specs.

**Install tree.** WORKDIR/inst is rebuilt on every invocation: the
shell under test is copied to `inst/usr/bin/bash.exe` (with
`msys-2.0.dll` beside it when the shell's own directory has one, as Git
Bash's does: an MSYS2 program takes its root from that DLL), and
`inst/etc/fstab` reads

    GIT/usr/bin /usr/bin ntfs binary,noacl,auto 0 0
    GIT/usr/bin /bin ntfs binary,noacl,auto 0 0
    GIT/etc /etc ntfs binary,noacl,auto 0 0
    none / cygdrive binary,posix=0,noacl,user 0 0
    none /tmp usertemp binary,posix=0,noacl 0 0

where GIT is WORKDIR/git (spaces as `\040`), a clone of the Git
install (`GITBASH/../..`): its `usr/bin` hard-linked (copied when
links fail) and its `etc` copied, rebuilt when Git's files change.
So `/` is inst, the programs and configuration are Git's, and the copied
DLL keeps the shell's mount table and process list apart from any Git
Bash the user is running. The clone does the same for the MSYS2
children of a shell that is not an MSYS2 program: the runtime keys its
shared mount table by the DLL's directory (a junction resolves to the
same key), and the first live process of an install fixes `/tmp` from
its TMP, so children from Git's own directory would see the `/tmp` of
whatever Git Bash the user is running. With the clone, each test's
first MSYS2 child reads GIT/etc/fstab with the test's TMP. (Git Bash's
children inherit Git Bash's table either way.) WORKDIR/bin holds
`sandbox.exe`, `argv.exe` and `argv32.exe`, built from tools/.

**Runs.** Each NAME.sh is copied to WORKDIR/NAME/test.sh, a fresh
directory holding also empty `home` and `tmp`, and run twice, once with
stdout and stderr apart and once with `2>&1`, both with stdin
`/dev/null`, as

    sandbox -L -t 60 -p 64 -E ... WORKDIR/NAME -- inst/usr/bin/bash.exe ARGS

ARGS is `--norc --noprofile test.sh`, or the words of a `# shell: ARGS`
comment, split and unquoted by sh's `eval`, in which `$D`, `$DW` and
`$DU` are the test directory as `C:/x`, `C:\x` and `/c/x`. The
environment is exactly:

    PATH=GIT\usr\bin;W64\bin;SYSTEMROOT\System32;WORKDIR\bin
    SYSTEMROOT, COMSPEC=SYSTEMROOT\System32\cmd.exe  (SYSTEMROOT the caller's)
    HOME=WORKDIR\NAME\home  TEMP=TMP=WORKDIR\NAME\tmp  (backslashes)
    LANG=C  TZ=UTC  (and UNABASHED_XPIO=1 under -x)

W64 is the w64devkit root (the directory above `cc`). A `# env:
NAME=VALUE` comment adds a variable (one word, no blanks; `$D`, `$DW`
and `$DU` expanded), and `# env: -TZ` drops `TZ=UTC` so the system zone
applies. A directory NAME.d beside the script is copied over the test
directory first (`home/.profile`, `etc/profile`, ...). When the test
directory then has an `etc`, the test runs from a private install,
`WORKDIR/NAME/inst`, a copy of `inst/usr/bin` whose fstab mounts that
`etc` on `/etc`, so that neither shell reads Git's startup files and
the test's are read instead. Statuses must
agree between the two runs. The spec format is tools/record's, with the
paths normalized. `MSYS_TIMEOUT` sets the timeout in seconds.

`-L`: MSYS2 programs cannot run at Low integrity (see sandbox), and the
corpus runs them under either shell, so these runs are not
write-confined. Hence the corpus rules below.

**Recording.** `tools/gbrecord` builds the same tree with Git Bash and
records each script twice (four runs); the two specs must be identical
or the script is reported as nondeterministic and its spec is left
alone.

**Normalization.** Four directories are replaced by tokens:

    @ROOT@  WORKDIR/inst
    @WORK@  WORKDIR
    @GIT@   the Git install (its clone, WORKDIR/git)
    @W64@   the w64devkit root

Each directory is first made absolute as `X:R`: an uppercase drive
letter X, then R, the path from its leading `/`, with forward slashes,
no trailing slash, in the case `pwd` reports. With x the lowercase
drive letter and B being R with every `/` turned into `\`, it has six
spellings:

    X:R   x:R   X:B   x:B   /xR   /XR

For `C:/w/inst` these are `C:/w/inst`, `c:/w/inst`, `C:\w\inst`,
`c:\w\inst`, `/c/w/inst` and `/C/w/inst`. The 24
(spelling, token) pairs are ordered by the spelling's length, longest
first, ties by the bytes of "TOKEN SPELLING"; then, pair by pair in
that order, every occurrence of the spelling in the output is replaced
by the token, scanning left to right without overlaps. Matching is
byte-exact (no other case folding, no word boundaries); stdout, stderr
and the merged stream are normalized separately and otherwise kept
byte for byte, CRs included. Since @ROOT@'s spellings are the longest,
`WORKDIR/inst/x` becomes `@ROOT@/x`, not `@WORK@/inst/x`. When the
shell under test is a 32-bit image, every `i686` then becomes `x86_64`
(MACHTYPE and HOSTTYPE name the build's architecture; Git Bash is
64-bit).

**Corpus rules.** Scripts are hand-written and reviewed. They print no
pids, times, `$RANDOM`, user or machine names, or drive lists. They
write only through relative paths in their own directory, `$HOME` or
`$TMP`, never after a `cd` to an absolute path (a shell that misreads
`/x` would write elsewhere, outside the sandbox's protection). They do
not pass root-relative paths outside the mounts (`/`, `/foo`) to MSYS2
children: a child of Git Bash inherits its mount table (root inst), a
child of a native shell has Git's. Deliberate divergences stay out (see
issues.md, M5a, "Accepted"). `# vos` marks scripts that run only
builtins, `argv`/`argv32`, `cygpath` (a builtin in msys mode) and
programs fakes.cpp already has (`mkdir`), or merely look programs up,
so that the vos can run them; M5a step 4 settles the list.
`t/msys/*-crlf-*.sh` keep their CRLF line endings (.gitattributes).

## Agent batch (bench/agent)

    bench/agent/run [-n RUNS] [-d DRIVER] WORKDIR [SHELL]

times `bench/agent/batch.txt` (52 typical agent commands, each with
the external programs it runs) as Claude Code's Bash tool runs them:
one `SHELL -c WRAPPER` per command, WRAPPER being the exact string from
notes/research/msys2.md section 7 with the command in its `eval`,
sourcing `snapshot.sh`. A driver (`driver.sh`) runs inside `sandbox -L`
in WORKDIR/run with a scratch git repository and a fixed environment,
and the medians of RUNS rounds (5) are printed per mode: `full` (the
commands), `noop` (the wrapper around `:`), `exit` (`SHELL -c exit`),
`floor` (a do-nothing CRT program, tools/argv.cpp, started the same
way), `ext` (the external programs alone, run by the driver) and
`empty` (the loop), then `own`, full - ext - floor: the shell's own
time, which the M5a gate compares. The driver is busybox sh, or with
`-d` a native-mode shell such as unabashed.exe, copied as sh.exe; use
one with exact waits for `own`, since busybox's polling waits add about
12 ms to each program it starts, inflating `ext`. `BENCH_MODES` selects
modes; a shell without MSYS2 paths should run only `exit noop`. The
full run's output checksum must not change between rounds. Results: notes/m5a.md.

## w64devkit trial (tools/w64trial)

    tools/w64trial [-s SHELL] [-k KIT] [-j N] [-n RUNS] WORKDIR TARBALL...

Builds autoconf release tarballs (NAME.tar.gz extracting to NAME) the
way a w64devkit user would, once with the stock kit's busybox sh and
once with SHELL (default `unabashed.exe`) as the kit's sh.exe. KIT
defaults to the w64devkit running the script, and is never written:
WORKDIR/kit-bb and kit-ub mirror it (bin hard-linked, every other entry
a junction), differing only in sh.exe. Each phase (extract, configure,
`make -jN`, `make -jN check`) runs `sh -c` in tools/sandbox in
WORKDIR/bb or WORKDIR/ub, after sourcing the kit's etc/profile, with a
fixed Windows-style environment, and is timed. Then config.h (as
config.status names it) and config.status's substitutions must be
equal, the build directories and kits normalized. `-n RUNS` then reruns
configure RUNS times per shell, alternating, and prints the medians.
Logs are WORKDIR/{bb,ub}/NAME.PHASE.log. Results: notes/m5a.md.
