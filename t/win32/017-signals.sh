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

trap 'echo top USR1' USR1
{ kill -USR1 $$; } &
wait
sleep 1
echo end
