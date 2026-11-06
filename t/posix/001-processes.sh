# What only real processes show (POSIX build, tools/record check mode;
# the vos and Windows cannot fork): exec with work in flight keeps the
# shell's pid; a job's own trap; a signal to a level of another process
bash -c 'p=$$; exec bash -c "read x; echo \"\$x, pid kept \$(( \$\$ == $p ))\"; cat" < <(echo first; sleep 1; echo later)'
{ trap 'echo caught TERM; exit 3' TERM; echo r >r.txt; while :; do sleep 1; done; } &
while [ ! -e r.txt ]; do sleep 1; done
kill $!; wait $!; echo "trapped $?"
( p=$BASHPID; trap 'echo level got USR2' USR2; { kill -USR2 $p; } & wait $!; sleep 1; echo "level done" )
( exec sleep 5 ) & sleep 1; kill $!; wait $!; echo "exec'd job $?"
SECONDS=0
bash -c '{ sleep 2; echo late >late.txt; } >/dev/null 2>&1 & echo started'
[ $SECONDS -lt 2 ] && echo "returned at once"
[ -e late.txt ] || echo "not yet"
sleep 3; cat late.txt
