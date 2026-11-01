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
    sandbox [-m MIB] [-p COUNT] [-t SECS] DIR -- PROGRAM [ARGS...]

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
Windows `ping` fails inside the sandbox.
