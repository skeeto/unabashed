# A sourced file does not inherit DEBUG (without set -T): it starts with
# it unset and the caller's comes back after it, and its RETURN action,
# unless the file set its own DEBUG. A DEBUG action it sets runs in it,
# even one the same size or text as the caller's, and stays after it.
# ERR and RETURN are inherited.
printf '%s\n' "trap 'echo err2' ERR" "trap 'echo dbg2' DEBUG" "trap 'echo ret2' RETURN" "false" "echo in-s1" > s1.sh
trap 'echo err1' ERR
trap 'echo dbg1' DEBUG
trap 'echo ret1' RETURN
. ./s1.sh
trap - DEBUG
trap -p
trap - ERR RETURN
echo "-- the caller's, listed and run"
printf '%s\n' "trap -p" "false" "echo in-s2" > s2.sh
trap 'echo err1' ERR
trap 'echo ret1' RETURN
trap 'echo dbg1' DEBUG
. ./s2.sh
trap - DEBUG ERR RETURN
echo "-- the same text"
printf '%s\n' "trap 'echo dbg1' DEBUG" "echo in-s3" > s3.sh
trap 'echo dbg1' DEBUG
. ./s3.sh
echo top
trap - DEBUG
echo "-- unset in the file, and put back"
printf '%s\n' "trap - DEBUG" "trap - RETURN" "echo in-s4" > s4.sh
trap 'echo dbg1' DEBUG
trap 'echo ret1' RETURN
. ./s4.sh
echo top
trap - DEBUG
trap -p
echo "-- in a function"
printf '%s\n' "trap -p" "echo in-s5" > s5.sh
f() { trap 'echo retf' RETURN; trap 'echo dbgf' DEBUG; . ./s5.sh; echo f-after; trap - DEBUG; }
f
trap - RETURN
printf '%s\n' "trap 'echo \"d \$BASH_COMMAND\"' DEBUG" > s6.sh
f() { . ./s6.sh; echo in-f; }
f
echo top
trap - DEBUG
echo "-- set -T"
set -T
trap 'echo "d $BASH_COMMAND"' DEBUG
. ./s2.sh
trap - DEBUG
set +T
