# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 10).
# busybox-w32 runs a missing /bin/X, /usr/bin/X, /sbin/X or /usr/sbin/X
# as X by name: sh, bash and ash are the shell itself (configure runs
# $SHELL as /bin/sh), anything else is found on PATH. Other missing
# absolute paths, and deeper ones, still fail.
/bin/sh -c 'echo sh "$@"' x 1
/usr/bin/bash -c 'echo bash $#' x 1 2
/bin/argv one
/usr/sbin/argv two
/bin/nosuch; echo "status $?"
/opt/argv; echo "status $?"
/usr/bin/x/argv; echo "status $?"
