# In POSIX mode wait forgets the saved status it returns, but in a
# command substitution only there: the shell around it still has it
set -o posix
(exit 3) & p1=$!
(exit 4) & p2=$!
sleep 0.3
jobs >/dev/null
x=$(wait $p2; echo "in comsub: $?"); echo "$x"
x=$(wait $p1; wait $p1 2>/dev/null; echo "twice: $?"); echo "$x"
x=$(wait; echo "plain: $?"); echo "$x"
wait $p2; echo "p2: $?"
wait $p1; echo "p1: $?"
wait $p1 2>&1 | sed 's/pid [0-9]*/pid N/'
