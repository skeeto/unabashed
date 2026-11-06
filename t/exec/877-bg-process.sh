# A background job is a process of its own: $! is its BASHPID, $$ its
# parent's, a signal to $! reaches its traps, and a shell that starts
# one exits without waiting for it
{ echo "$BASHPID $$" >pids.txt; } &
wait $!
read -r b d <pids.txt
[ "$b" = "$!" ] && echo "BASHPID is \$!"
[ "$d" = "$$" ] && echo "\$\$ is the parent's"
[ "$b" != "$$" ] && echo "not the parent"

{ trap 'echo caught TERM; exit 3' TERM; echo ready >ready.txt; while :; do sleep 1; done; } &
while [ ! -e ready.txt ]; do sleep 1; done
kill $!
wait $!
echo "trapped $?"

{ trap '' TERM; sleep 2; echo "ignored TERM"; } &
sleep 1
kill $!
wait $!
echo "ignoring $?"

# levels of the job are reachable from the parent, and the parent's
# from the job
{ ( echo "$BASHPID" >sub.txt; trap 'echo sub got USR1; exit 4' USR1; sleep 2 ); echo "sub $?"; } &
j=$!
while [ ! -s sub.txt ]; do sleep 1; done
kill -USR1 "$(cat sub.txt)"
wait $j
( p=$BASHPID; trap 'echo level got USR2' USR2; { kill -USR2 $p; } & wait $!; sleep 1; echo "level done" )

bash -c '{ sleep 2; echo late >late.txt; } & echo started'
[ -e late.txt ] || echo "not yet"
sleep 3
cat late.txt
