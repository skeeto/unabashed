# A shell started with fd 1 or fd 2 closed: writes fail as on any
# closed descriptor, a dup of it fails, errors with nowhere to go are
# dropped (and the shell carries on), and it opens again like any other
bash -c '
echo hi; echo "echo $?" >&2
printf x; echo "printf $?" >&2
exec 4>&1; echo "dup $?" >&2
echo hi | cat; echo "pipeline ${PIPESTATUS[*]}" >&2
(echo hi); echo "subshell $?" >&2
x=$(echo inner); echo "cmdsub $x" >&2
[ -t 1 ]; echo "tty $?" >&2
exec >out.txt; echo "reopened $?"; cat out.txt >&2
' bash >&-
echo "status $?"
bash -c '
nosuch; echo "missing $?"
exec 5>&2; echo "dup $?"
echo hi >&2; echo "echo $?"
set -x; : traced; set +x
for i in 1 2 3; do nosuch$i; done; echo "loop $?"
trap "echo trapped \$?" ERR; false; trap - ERR
cat nosuch; echo "program $?"
exec 2>err.txt; nosuch; cat err.txt
' bash 2>&-
echo "status $?"
bash -c 'echo out; nosuch; read x; cat; exit 5' bash <&- >&- 2>&-
echo "all closed $?"
