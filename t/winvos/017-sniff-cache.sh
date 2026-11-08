# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 7).
# The execute bit and kind of a file come from its contents through a
# cache keyed by path, mtime and size: a rewrite of the same size
# shows through once the mtime moves, and subshells share the cache.
# Without #! (and without an extension) it is not executable and is
# refused, as busybox-w32 refuses it.
printf '#!/bin/sh\necho one\n' >t
[ -x t ] && echo "x"
./t
printf '##/bin/sh\necho two\n' >t
touch -t 200101010000 t
[ -x t ] && echo "x" || echo "-"
./t; echo "status $?"
printf '#!/bin/sh\necho six\n' >t
touch -t 200201010000 t
( [ -x t ] && ./t )
cp C:/w64/bin/argv.exe t
./t seven | head -1
