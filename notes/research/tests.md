The suite is usable as an oracle, but only for part of the work. About 30 of the 86 tests need nothing but the shell, and nearly every test also starts the shell under test as a child process (`$THIS_SH`). A virtual OS that can run a fresh in-process instance of our shell, plus four helper programs and about 15 small coreutils clones, would cover about 80% of it. Everything was read in place; nothing in the tree was run or changed.

All paths are relative to `C:/Users/wellons/AppData/Local/Temp/claude/C--Users-wellons-src-unabashed/49667c1f-f31a-4608-a8d2-7135244c0f96/scratchpad/src/bash-5.3`.

## 1. What `tests/README` says

The whole README is two lines: type `sh run-all`, and see COPYRIGHT. `tests/COPYRIGHT` says the files are FSF copyright (1991-2011) under the COPYING file at the top of the tree, which is GPLv3+. One exception: `tests/ifs-posix.tests` is copyright 2005 Glen Fowler. The run-* scripts carry GPLv3+ headers.

## 2. How the suite works

**Size.** 83 `.tests` files, 451 `.sub` files, 87 `.right` files and 88 `run-*` scripts. That is 86 real tests plus the two drivers `run-all` and `run-minimal`. The `.tests` and `.sub` files total about 36,800 lines.

**Make targets** (`Makefile.in` lines 598-601 and 1079-1083):
- `make test`, `tests` and `check` build four helper programs (`TESTS_SUPPORT`) from `support/` and copy them into `tests/`.
- They then run `cd tests && BUILD_DIR=.. PATH=$(BUILD_DIR)/tests:$PATH THIS_SH=$(THIS_SH) $(SHELL) run-all`.
- `run-gprof` is the profiling variant (line 693). `asan-tests` and `lsan-tests` work the same way (lines 678-689).

**`tests/run-all`** does the following in order:
1. Sets `TMPDIR` to `/tmp` if unset and exports it.
2. Computes a suffix with `${THIS_SH} -c 'echo $(( $RANDOM + $BASHPID ))'`.
3. Exports `BASH_TSTOUT=${TMPDIR}/bashtst-$SUFFIX` and sets traps that remove it.
4. Sets `PATH=.:$PATH` so `recho`, `zecho` and `printenv` are found in `tests/`.
5. Unsets `BASH_ENV`. If the driver shell is itself bash, it runs `export -n SHELLOPTS`, `set +o posix`, `typeset -p SHELLOPTS` and `export -n BASHOPTS`.
6. Defaults `THIS_SH=../bash` and `BUILD_DIR=..`.
7. Runs `${THIS_SH} ./version`. This prints `BASH_VERSION`, `BASH_VERSINFO`, `HOSTTYPE`, `OSTYPE` and `MACHTYPE`, and warns if `UID==0`. Root breaks the permission tests.
8. Loops over `run-*`, skipping itself, `run-minimal`, `run-gprof`, `*.orig` and `*~`. For each it does `echo $x; sh $x; rm -f $BASH_TSTOUT`.
9. The rule is: any output other than the test names means a possible failure.

**Each `run-X` script** is about two lines:
```
${THIS_SH} ./X.tests > ${BASH_TSTOUT} 2>&1
diff ${BASH_TSTOUT} X.right && rm -f ${BASH_TSTOUT}
```
The pass/fail signal is just whether `diff` prints anything. The exit status of the shell under test is never checked directly; tests print `$?` themselves. Variations:
- **`grep -v '^expect'` filter:** attr, exp-tests, extglob, extglob2, invert, invocation, more-exp, new-exp, nquote, nquote1-3, nquote5, posix2, varenv. In these the `.tests` file defines `expect() { echo expect "$@"; }` as inline documentation (for example `tests/exp.tests:26`), and the filter strips it.
- **`| cat -v`:** run-printf.
- **stderr not captured:** extglob2, extglob3, glob-bracket, input-test, intl, posixpat, precedence, set-e, strip, tilde, tilde2. run-rhs-exp uses `2>&1 > file`, so stderr goes to the terminal and is not diffed.
- **`diff -a`** (only if the host diff supports it, for binary-ish output): glob-test, intl, nquote4.
- **Hard-coded `/tmp`:** run-dbg-support and run-dbg-support2 write `/tmp/<name>.check`, run with `< /dev/null`, and `exit $?`. run-set-x also uses `< /dev/null`.
- **`PATH=$PATH:`pwd``:** the extglob and glob runners.
- **Pre-test environment changes:** run-errors runs `unset OLDPWD`; run-test runs `unset GROUPS UID`.
- **Multiple pairs:** run-dirstack runs two (dstack, dstack2). run-dollars runs `./dollar-at-star` against `dollar.right`; run-array2 runs `./array-at-star` against `array2.right`; run-execscript runs `./execscript` against `exec.right`; run-input-test runs `${THIS_SH} < ./input-line.sh` against `input.right`.
- Many runners first print "warning: ..." lines to stderr explaining expected false failures: strerror text, signal numbering, missing locales, missing `/dev/fd`.

**`.sub` files** are child scripts. They are almost always started as a new process, `${THIS_SH} ./name.sub`, and only occasionally sourced. Some heavy users of `THIS_SH`: errors (65 occurrences counting subs), execscript (54), posixexp (53), array (33), varenv (30), invocation (30), new-exp (28), builtins (28), set-e (24), history (21), exportfunc (21), printf (21). `tests/nameref.tests:131` finds its subs with a glob (`for testfile in ./nameref[0-9].sub ...`), so even that "pure" test needs a directory listing.

**`run-minimal`** is the curated set for `--enable-minimal-config` builds (no arrays and so on). It is a good first milestone list: dollars, execscript, func, getopts, heredoc, ifs-posix, input-test, invert, more-exp, nquote, posix2, posixpat, posixpipe, precedence, quote, read, rhs-exp, strip, tilde, dynvar, iquote, type, comsub-eof, comsub-posix. It also lists `run-ifs-tests`, which does not exist. Note that minimal config has no arrays, while our clone always will.

**Shared helpers.** `tests/test-glue-functions` defines `_intl_normalize_spaces` (`sed` that squeezes whitespace, used after `od`) and `_cut_leading_spaces` (`sed`, used after `wc`). They exist to hide formatting differences between od and wc implementations.

**Data files:** `history.list`, `mapfile.data`, `redir3.in1`, `redir3.in2`, `redir4.in1`, `redir13.in`, `varenv15.in`.

**`tests/misc/`** is not run by run-all. It holds old manual tests (dev-tcp, sigint-1..4, wait-bg, read-nchars, run-r2.sh, test-minus-e) and two perf scripts:
- `perftest`: a `case` loop over `/usr/lib/*/*`.
- `perf-script`: an arithmetic loop using `let`.

Both are useful seeds for our own performance comparison against bash.

## 3. Helper programs (`support/*.c`, built into `tests/`)

These are trivial to reimplement from the description below; read for behavior only.
- **`recho`** (`support/recho.c`): prints each argument as `argv[N] = <arg>`, one per line. Bytes below 0x20 print as `^` followed by the byte plus 64; 0x7F prints as `^?`. It is the main way the suite shows word splitting and quoting.
- **`zecho`** (`support/zecho.c`): prints its arguments joined by single spaces plus a newline. No option or escape handling.
- **`printenv`** (`support/printenv.c`): with no arguments, prints the environment. With one argument, prints the value of the first variable whose entry starts with that name followed by `=`, and exits 0; exits 1 if there is none.
- **`xcase`** (`support/xcase.c`): `[-lnu] [file]` copies stdin or a file to stdout. `-u` uppercases, `-l` lowercases, `-n` makes stdout unbuffered. Used by coproc.
- `support/printenv.sh` is a shell version of printenv.

## 4. Environment the suite assumes

**Filesystem**
- `TMPDIR` (default `/tmp`) is used by about 45 test groups. Some hard-code `/tmp`: test.tests has 103 references, plus dstack (`pushd /tmp`), execscript (`ln -s $THIS_SH /tmp/bash`), and type (`hash -p /tmp/...`).
- Some tests write into the current directory, which is `tests/`. Examples: `read2.sub:62` does `mkfifo a.pipe`; `read9.sub` writes `read9.expout` and `read9.stdout`.
- It reads `/etc/passwd` (array.tests:114, redir.tests:62, coproc.tests:68).
- It uses `/dev/null` (256 occurrences), `/dev/stdin`, `/dev/stdout` and `/dev/stderr`, `/dev/fd/0`, `/dev/fd/1`, `/dev/fd/2` and `/dev/fd/6`, `/dev/tty` (21: errors, exportfunc, test, read2, read7, quotearray3, vredir8, errors1), and `/dev/udp` once.
- Some file names are illegal on Windows: `a?` (glob4.sub:21), `*abc.c` (glob6.sub:34), `a*b` and `a\*b` (glob8.sub:26).
- It assumes POSIX permission bits (`chmod`, `test -r/-w/-x/-u/-g/-k`), symlinks and hard links (test.tests:269 and 420, globstar3.sub, rsh2.sub), FIFOs (read2, source6, test1), and `ulimit` (builtins, procsub, redir10, vredir6).

**Build tree** (bash-specific, skip these):
- `glob-bracket.tests:38` greps `$BUILD_DIR/examples/loadables/Makefile`, compiles a loadable `strmatch.so` with gcc, and loads it with `enable -f`.
- `heredoc5.sub:35-41` cats `$BUILD_DIR/y.tab.c`, `config.h` and `version.h` through heredocs.

**External commands found at command position** (the probe was a regex; I hand-checked the less common hits):
- **Very common:** cat, rm (about 40 groups), grep, sed, mkdir, touch, chmod.
- **Moderate:** sleep (about 30 files, mostly jobs, trap, read, coproc, procsub), wc, ls, rmdir, cp, ln, kill, expr (arith, ifs-posix, more-exp, quotearray, set-e), env (exec, exportfunc, intl, nameref, varenv), od (case, glob, intl, nquote, posixexp, unicode), tr (braces, exp, trap, set-x).
- **Rare:**
  - awk: case3.sub:41, iquote.tests:119, coproc.tests:68.
  - sort -u: heredoc2.sub.
  - cmp: heredoc5, read9.
  - diff: read9; also shopt1.sub using `<(...)`.
  - mv: heredoc3.
  - mktemp: shopt1.
  - tee: lastpipe2.
  - mkfifo: as above.
  - hexdump -b: printf6.sub (Debian ships it in bsdextrautils).
  - date: dynvar, procsub, rsh; `command -p date` in rsh.tests:43.
  - perl / python: dynvar.tests:64-65, only as fallbacks for epoch time.
  - `locale -a`: glob2, intl2, unicode1, to detect installed locales.
  - id: exportfunc, inside CVE payload strings that should never run.
  - seq, ps, tail, cut: appear only inside strings or deliberately unterminated syntax (comsub-eof6.sub, complete.tests:25).
- **Self:** `$THIS_SH` everywhere. Also `/bin/sh` (`cp /bin/sh` in rsh1.sub and rsh2.sub; `exec -i /bin/sh` in errors.tests:113).
- The drivers themselves need `sh`, `diff`, `rm` and `grep -v`, and `cat -v` for printf.

**Locale**
- The default is whatever the environment has. Many tests force `LANG=C` (27) or `LC_ALL=C` (22) themselves.
- UTF-8 is required by intl (`intl.tests:14` exports `LC_ALL=en_US.UTF-8`), nquote4, printf2 and printf6, unicode2 and unicode3.
- Optional locales, each skipped with a warning if absent: de_DE.UTF-8 (intl2), fr_FR.ISO8859-1, zh_TW.big5 and ja_JP.SJIS (unicode1), zh_TW.big5 (glob2), ru_RU.CP1251 (unicode2:34), zh_HK.big5hkscs.
- `LC_COLLATE=C` is set in 8 places to pin glob and sort order.

**Expected-output conventions you must reproduce**
- **Error prefixes:**
  - Script errors look like `./name.tests: line N: ...` (about 880 lines across 51 `.right` files), so `LINENO` must be exact.
  - `-c` errors look like `bash: -c: line N: ...`. Tests pass `bash`, `bash5` or `posix-bash` as `$0`.
  - exec.right shows `/tmp/bash: notthere: No such file or directory` and `exec: .: cannot execute: Is a directory`.
- **Messages:** errno text must match glibc strerror wording. Signal names and numbers follow Linux (the trap, jobs and heredoc runners warn about this).
- **Version-specific:**
  - history2.sub echoes `${BASH_VERSION%.*}`, and `posix2syntax.sub:15` builds `bash${BASH_VERSION%%.*}`. We must report `5.3.x` to match.
  - shopt.tests prints the full `shopt` and `set -o` lists, so they must match 5.3 exactly.
  - array.tests:19 greps out `BASH_VERSINFO`, `PIPESTATUS` and `GROUPS` from `declare` output.
  - dbg-support and dbg-support2 require `extdebug`, DEBUG and RETURN traps, and `FUNCNAME`/`BASH_LINENO`.
  - cond.tests:197 has a Linux `ldd` transcript, but only as data.

## 5. Categorization of all 86 tests

Notation: `+S` means it starts `$THIS_SH` as a child; "R" means the recho/zecho/printenv helpers. A test is placed in its most demanding category.

**(a) Pure shell.** Needs only builtins, R, `+S`, and occasionally one trivially fakeable tool (named in parentheses).
- appendop (printenv)
- arith +S (expr)
- arith-for (sed)
- array2 (recho)
- attr +S
- braces (tr, zecho)
- casemod
- cprint (cat)
- dbg-support, dbg-support2 (the runner writes /tmp)
- dollars +S (recho)
- extglob2, extglob3
- getopts +S
- herestr (cat)
- ifs-posix (expr)
- input-test (script on stdin; tests exact input buffering and seek-back)
- invert (grep)
- more-exp (expr, R)
- nameref (R; needs a directory listing of `tests/`)
- nquote +S (od, R)
- nquote1, nquote2, nquote3, nquote5 (R; od and cat)
- parser +S
- posixexp2, posixpat
- posixpipe (wc; output of the `time` keyword)
- precedence
- quotearray +S (expr)
- rhs-exp (recho)
- strip
- tilde (references /tmp only as a string)
- tilde2 (cat, recho)
- iquote (wc, awk)
- comsub-eof +S (cat)
- comsub-posix +S (cat, ls, wc)
- case +S (od, awk)
- exp-tests +S (cat, sed, tr)
- complete (`compgen -d` reads the current directory)
- set-x (cat, tr; TMPDIR)
- set-e +S (cat, rm, expr)

**(b) Needs a filesystem.** Files, dirs, perms, links or FIFOs, under TMPDIR or the current directory.
- alias (cat, rm, chmod)
- array (rm, grep, mkdir, touch, ls; /etc/passwd)
- assoc (cat, rm, grep, touch; sleep)
- comsub (cat, sed, grep, wc)
- comsub2 (cat, rm, mkdir, touch, sleep)
- cond (rm; cond-regexp files)
- dirstack (`pushd /tmp`)
- extglob (rm, mkdir, rmdir, touch)
- func (cat, rm, sleep, printenv)
- globstar (mkdir, touch, ls, ln -s)
- heredoc (cat, rm, sed, grep, touch, sort, head, cmp, mv; needs the build tree)
- histexpand and history (`history.list`, HISTFILE in TMPDIR; cat, rm, wc)
- ifs (rm, mkdir, touch)
- lastpipe (cat, rm, tee)
- mapfile (`mapfile.data`)
- new-exp (cat, rm, sed; process substitution)
- posix2 (cat, rm, sed, touch, chmod)
- quote (rm, sed, grep)
- redir (cat, rm, grep, mkdir, touch, chmod; /etc/passwd; ulimit)
- shopt (rm, touch, chmod, mktemp, diff with `<()`; `set -o emacs`)
- test (heavy /tmp; touch, chmod, ln, mkfifo, kill; `-t` on /dev/tty; root-sensitive)
- type (cat, rm, sed, grep, mkdir, touch, chmod, mv; `hash -p`)
- vredir (cat, rm, grep, ls; /dev/tty; ulimit)
- glob-test (rm, sed, grep, mkdir, rmdir, touch, chmod, sleep, od; file names with `* ? \`; zh_TW.big5)

**(c) Needs real external processes.** Exec semantics, PATH search, the environment, or `/dev/fd`.
- execscript (cat, rm, mkdir, rmdir, touch, ls, sleep, cp, ln, chmod, env, kill; executing non-executables, `/tmp/bash` symlink, `command -p`)
- invocation +S x30 (cat, rm, sed, grep, mkdir, chmod; command-line options)
- exportfunc (env -i; exported-function CVE payloads; printenv)
- varenv +S x30 (env, printenv, grep, rm)
- posixexp (cp `$THIS_SH` to `$TMPDIR/sh`; od, sed, grep)
- builtins (cat, rm, sed, grep, mkdir, rmdir, wc, sleep, cp, chmod, mkfifo, kill; `/dev/fd`; ulimit)
- procsub (cat, rm, ls, wc, sleep, date; FIFOs or `/dev/fd`)
- rsh (cp `/bin/sh`, ln, cat, rm; `command -p date`)
- dynvar (date, or perl/python as fallbacks; sleep; `SECONDS`, `EPOCHSECONDS`, `RANDOM`/`SRANDOM`)
- glob-bracket (gcc plus a loadable built from BUILD_DIR; bash-specific, skip)

**(d) Interactive, tty, job control, signals, timing**
- jobs (`set -m`, sleep, kill, cat)
- trap (sleep, kill, cat, tr; signal numbers)
- coproc (cat, rm, sed, grep, sleep, awk, kill, xcase)
- errors +S x65 (`set -m`, /dev/tty, `set -i` rejection, kill)
- read (`read -t` timeouts; read7 does `read -e` from /dev/tty, which exercises readline; mkfifo, cmp, diff)
- Also touched elsewhere: `set -m` in complete, comsub2, shopt, exp12, trap8, varenv21.

**(e) Locale and multibyte**
- intl (od, grep, env, `locale -a`; UTF-8 plus the optional locales above)
- nquote4 (UTF-8 `$'\u...'`)
- printf (date, hexdump -b; printf6 under `LC_ALL=en_US.UTF-8`; printf2 switches LANG)
- glob-test (zh_TW.big5 in glob2)
- Partly: case, nquote and nquote3 (od dumps of control characters).

**Platform-specific and bash-specific summary**
- **Skip outright:** glob-bracket (loadables), heredoc5's BUILD_DIR part, dbg-support's dependence on the bashdb machinery (keep it only if we implement extdebug).
- **Linux/POSIX only:** jobs, trap, coproc, procsub, test, rsh, execscript, plus the illegal file names in glob.
- **Exact version required:** history2, posix2syntax, shopt and version.

## 6. Fake programs the virtual OS needs

- **Tier 0:** run `THIS_SH` as a fresh in-process shell instance (a new arena and a new virtual process with its own fd table and environment). Accept `argv[0]` override, `-c`, a script path, and stdin scripts.
- **Tier 1 (helpers):** recho, zecho, printenv, xcase.
- **Tier 2 (covers most of a and b):**
  - cat (including `-v`), rm (`-f`, `-r`), mkdir (`-p`), rmdir, touch, ls (plain names, sorted), chmod (octal and `+x`), ln (`-s`), cp, mv
  - grep (`-v`, `-e`, `-E`, `-q`, `-c`, `-i`)
  - sed: just `s///g` with `|` delimiters, `-e`, `1q`, and the two glue expressions
  - tr (single-character sets, `\012`), wc (`-l`, `-c`), expr (`+` and the `:` regex match)
  - od (`-b`, `-c`, `-t a`), hexdump -b, sort -u, head, cmp, diff (equal or not plus a minimal unified output)
  - env (`-i`, `NAME=value`), tee, mktemp, mkfifo
  - sleep and date on a virtual clock
  - kill (virtual signals), `locale -a` (a fixed list), awk (only the three tiny programs used)
- **Device files:** `/dev/null`, `/dev/stdin`, `/dev/stdout`, `/dev/stderr`, `/dev/fd/N`, `/dev/tty` (absent by default), plus a synthetic `/etc/passwd` and `/tmp`.

Outputs only need to match after the suite's own normalization (`_cut_leading_spaces`, `_intl_normalize_spaces`), with GNU wording for errors.

## 7. Recommended use

1. **Keep the suite out of our repo.** It is GPLv3+. Reference it from outside instead:
   - The harness takes `BASH_SRC` or `BASH_TESTS` (a path to an extracted bash-5.3).
   - Optionally provide a fetch script that downloads `bash-5.3.tar.gz` from ftp.gnu.org, checks a pinned SHA-256, and extracts into a gitignored `third_party/` or scratch directory.
   - Commit only our own harness code and a manifest: test name, category, expected-pass flag, and a short reason. Test names are facts, not copied expression.
   - Never paste `.tests`, `.sub` or `.right` content, or snippets from them, into our tests. When a failure needs a regression test, write a fresh minimal script from the documented semantics and capture its expected output from real bash. Output generated by running real bash on our own scripts is ours to commit.
2. **Our own runner** reimplements the run-* contract rather than calling those scripts:
   - Per test, copy `tests/` into a throwaway working directory, because tests write into the current directory.
   - Set `THIS_SH`, `TMPDIR` (private), `PATH` (fake-tools dir first), `BUILD_DIR`, a fixed environment (`env -i` style; `LANG=C` unless the test needs UTF-8; `TZ=UTC`; `HOME`), and unset `OLDPWD`, `BASH_ENV`, `SHELLOPTS` and `BASHOPTS`.
   - Run the main script, apply the same filters (`2>&1`, `grep -v '^expect'`, `cat -v`), compare with the `.right` file, and report a diff.
   - Do this in two backends:
     - **Virtual OS** for everything in categories a and b plus the c tests that fake tools can satisfy. This backend is safe and deterministic, and it is the one to use for fuzzing.
     - **POSIX/Linux layer** in a disposable WSL user or container, never as root, for c, d and e.
3. **Order of milestones:**
   - the run-minimal list
   - the rest of category a (quoting, expansion, arith, parser, getopts, dollars, nquote*, posix*, precedence)
   - category b on the virtual filesystem
   - c and d on Linux
   - e once multibyte support lands
   - shopt and invocation last, since they need the full option tables
4. **Differential testing against real bash 5.3 in WSL Debian:**
   - Confirm the version first. I expect Debian 13 "trixie" ships 5.2.x and that 5.3 is only in unstable; I did not check this. If needed, build 5.3 from this same tarball in WSL with `./configure && make`.
   - Run `make tests` there to get a baseline of which tests already fail on real bash in that environment (locales, `/dev/tty`, root). Record it in the manifest.
   - **Corpus:** our own snippet files, plus (by path, not copied) each `.tests` and `.sub` file run standalone.
   - **Runs:** for each item, run `bash --norc --noprofile` and ours, both with `$0` forced to `bash` (`exec -a bash`), the same fixed environment, a fresh TMPDIR, and stdin from `/dev/null` unless the test needs input.
   - **Compare separately:** stdout, stderr and exit status, plus one merged `2>&1` run to check interleaving.
   - **Normalize:** TMPDIR paths, PIDs, `$RANDOM` and time values (or seed them; `RANDOM=n` is deterministic in bash).
   - Use this same rig for performance: time loops such as `tests/misc/perf-script` and `perftest` under both shells.
   - On Windows, differential runs against MSYS2 or Git-for-Windows bash (5.2) cover the MSYS2 path behavior. Do not use them as the semantic oracle.
5. **Safety.** The tests run `rm -rf $TESTDIR $WORK_DIR` (`glob-bracket.tests`), `cd` into TMPDIR, chmod files and kill processes. A buggy expansion, such as an empty variable, could become destructive. So:
   - Never run the real-filesystem backend on the Windows host or against a real home directory.
   - Fuzz only on the virtual OS, where "exec" and "fs" are in-memory fakes.
