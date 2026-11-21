# A standard handle the shell was started without is a closed
# descriptor, as `<&-` leaves one for a POSIX bash: copying it fails as
# for any closed descriptor, it is no terminal, it opens again like any
# other, and a shell to read its commands from a missing stdin fails at
# once. Without standard error, errors go nowhere and the shell goes on.
S=$BIN/stdrun.exe
"$S" -t 20 - 1 1 -- "$U" -c '
exec 3<&0; echo "dup $?"
[ -t 0 ]; echo "tty $?"
read x; echo "read $?"
exec </dev/null; read x; echo "reopened $?"
exec <&-; exec 4<&0; echo "closed again $?"
' sh
echo "status $?"
"$S" -t 20 - 1 1 -- "$U" | sed 's/^.*: error/sh: error/'
echo "stdin ${PIPESTATUS[0]}"
"$S" -t 20 0 1 - -- "$U" -c '
nosuch; echo "missing $?"
exec 5>&2; echo "dup $?"
echo hi >&2; echo "echo $?"
for i in 1 2 3; do nosuch; done; echo "loop $?"
set -x; : traced; set +x
"$BB" cat nosuch; echo "program $?"
exec 2>err.txt; nosuch; cat err.txt
' sh
echo "status $?"
"$S" -t 20 0 - 1 -- "$U" -c 'exec 6>&1; echo "dup $?" >&2
[ -t 1 ]; echo "tty $?" >&2
exec >out.txt; echo "reopened $?"; cat out.txt >&2' sh
echo "status $?"
