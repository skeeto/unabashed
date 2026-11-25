# A for loop's header runs the DEBUG trap and xtrace as decided at each
# iteration, so the body may turn either on for the next
for i in 1 2 3; do trap 'echo dbg $i' DEBUG; done
trap - DEBUG
for i in 1 2 3; do trap 'echo dbg $i' DEBUG; [ $i = 2 ] && trap - DEBUG; done
f() { for i in 1 2; do trap 'echo fdbg $i' DEBUG; done; }; f
trap - DEBUG
for i in 1 2; do set -x; done 2>&1
set +x
