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
user `oracle` (uid 1000). `/opt/fakes/bin` is empty for now; it is
reserved for the fakes multi-call binary.

### Recording

    tools/record t/foo.sh [t/bar.sh ...]

This writes `t/foo.spec` next to each script. Each script runs twice,
each time in a fresh container: once with stdout and stderr apart, and
once with `2>&1`. Both runs must exit with the same status. Each run
looks like this:

- `bash --norc --noprofile -c SCRIPT bash`, so `$0` is `bash` and
  diagnostics read `bash: line N: ...`
- stdin is `/dev/null`
- `env -i LANG=C TZ=UTC HOME=/home/oracle PATH=/usr/local/bin:/usr/bin:/bin`
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
