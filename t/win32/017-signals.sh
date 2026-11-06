# Signals between our own shells go through the signal channel (M5b):
# a background job's traps run, a level of another process gets its
# own, and a job's lone program, which bash would have exec'd, gets
# the signal itself
{ trap 'echo caught TERM; exit 3' TERM; echo r >ready.txt; while :; do sleep 1; done; } &
while [ ! -e ready.txt ]; do sleep 1; done
kill $!
wait $!
echo "trapped $?"

{ sleep 3; echo "unreached"; } &
sleep 1
kill $!
wait $!
echo "plain $?"

( p=$BASHPID; trap 'echo level got USR2' USR2; { kill -USR2 $p; } & wait $!; sleep 1; echo "level done" )

"$U" -c 'trap "echo kid TERM" TERM; echo r >kid.txt; sleep 2; echo kid end' &
while [ ! -e kid.txt ]; do sleep 1; done
kill $!
wait $!
echo "kid $?"

# TERM to a shell that exec'd one of ours goes on to that one
"$U" -c 'exec "$U" -c "trap \"echo exec TERM; exit 4\" TERM; echo r >ex.txt; sleep 3; echo exec end"' &
while [ ! -e ex.txt ]; do sleep 1; done
kill $!
wait $!
echo "exec $?"

# KILL to a job whose last program it exec'd (in a job that ends with
# the shell) ends that program too, as in bash where they are one
"$U" -c 'echo r >k9.txt; sleep 2; echo end >k9end.txt' &
while [ ! -e k9.txt ]; do sleep 1; done
kill -9 $!
{ wait $!; } 2>k9.err
echo "k9 $?"
"$BB" sed 's/: line [0-9]*: *[0-9]* Killed */: Killed /' k9.err
sleep 3
[ -e k9end.txt ] || echo "k9 program gone"

trap 'echo top USR1' USR1
{ kill -USR1 $$; } &
wait
sleep 1
echo end
