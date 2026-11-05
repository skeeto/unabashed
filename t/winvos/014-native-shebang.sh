# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 7).
# Shebangs in native mode: /bin/sh and /usr/bin/env sh run the shell
# itself, other interpreters are found by path (with an executable
# extension) or by name on PATH, the rest of the line is one argument,
# chains nest four levels deep, a .sh file without #! runs as a shell
# script, and a missing interpreter is a "bad interpreter" error.
# (#!/usr/bin/env PROG runs busybox env, whose vos fake has no PATH
# search with .exe: t/win32/014-spawn covers it.)
printf '#!/bin/sh\necho sh "$@"\n' >a
printf '#! /usr/bin/env sh\necho env-sh "$@"\n' >b
printf '#!C:/w64/bin/argv one  two\n' >d
printf '#!./d\n' >e1
printf '#!./e1\n' >e2
printf '#!./e2\n' >e3
printf '#!./e3\n' >e4
printf 'echo plain "$@"\n' >p.sh
printf '#!/no/such\n' >f
printf '#!/usr/bin/env nosuch\n' >g
./a 1
./b 2
./d 4
./e3 5
./e4 6; echo "status $?"
./p.sh 7
./f; echo "status $?"
./g; echo "status $?"
