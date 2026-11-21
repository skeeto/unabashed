# A shell started with no standard input and no console (tools/stdrun
# -d): reads fail at once, in the shell and in its programs, which get
# no handle for it rather than their own hidden console's, and copies
# of it are as absent. Likewise a shell with no standard output. The
# programs' messages are dropped (busybox's or not).
S=$BIN/stdrun.exe
"$S" -d -t 20 - 1 1 -- "$U" -c '
cat 2>/dev/null; echo "cat $?"
cat 2>/dev/null | cat; echo "pipeline ${PIPESTATUS[*]}"
(cat 2>/dev/null); echo "subshell $?"
read x 2>/dev/null; echo "read $?"
cat </dev/null; echo "null $?"
exec 3<&0; cat <&3 2>/dev/null; echo "dup $?"
exec 0<&-; cat 2>/dev/null; echo "closed $?"
' sh
echo "status $?"
"$S" -d -t 20 0 - 1 -- "$U" -c 'echo hi; echo "echo $?" >&2
"$BB" echo hi 2>/dev/null; echo "program $?" >&2' sh
echo "status $?"
