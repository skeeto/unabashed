# A CHLD trap runs once for each child bash would fork and reap: a
# program, ( ), $( ), each pipeline stage (not one lastpipe keeps), a
# background job, a procsub, a coproc; right after the wait, inside its
# command's redirections; not again for children of its own action
exec 2>&1
trap 'echo CHLD' CHLD
echo 1; cat </dev/null; echo 2
( cat </dev/null ); echo 3
x=$(cat </dev/null); echo 4
cat </dev/null | cat; echo 5
( : ); echo 6
x=$(echo hi); echo 7
: & wait; echo 8
sleep 0.2 & echo 9a; sleep 0.5; echo 9b
cat </dev/null & cat </dev/null & wait; echo 10
nosuch; echo 11
cat <(echo x); echo 12
true | true | true; echo 13
shopt -s lastpipe; echo a | read v; echo "14 $v"
f() { cat </dev/null; }; f; echo 15
command cat </dev/null; echo 16
x=`echo q`; echo 17
coproc cat; exec {COPROC[1]}>&-; wait; echo 18
{ cat </dev/null; } > /dev/null; echo 19
trap 'cat </dev/null; echo inner' CHLD; ( : ); echo 20
trap - CHLD; ( : ); echo 21
