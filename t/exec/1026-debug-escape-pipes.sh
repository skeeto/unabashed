# A DEBUG action that escapes a pipeline before its last stage starts
# (set -u's fatal error, exit) closes the shell's pipe ends: a started
# stage writing to a stage that never runs gets SIGPIPE, not a hang
x=$(exec 2>&1; set -u; trap '[[ $BASH_COMMAND == cat* ]] && : $nope' DEBUG; { sleep 0.3; echo late >&2; } | cat; echo notreached)
echo "1 $? [${x##*: }]"
x=$(exec 2>&1; trap '[[ $BASH_COMMAND == cat* ]] && exit 7' DEBUG; { yes | head -c 200000; echo "w $?" >&2; } | cat)
echo "2 $? [$x]"
x=$(exec 2>&1; trap '[[ $BASH_COMMAND == cat* ]] && exit 8' DEBUG; { i=0; while :; do echo $i; i=$((i+1)); done; } | cat)
echo "3 $? [${#x}]"
( trap '[[ $BASH_COMMAND == cat* ]] && exit 6' DEBUG; { sleep 0.3; echo lost; } | cat )
echo "4 $?"
# and the shell goes on with the same pipeline
{ sleep 0.3; echo once; } | cat
