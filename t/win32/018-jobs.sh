# Job control builtins on real processes, without a terminal (set -m):
# jobs and its options, kill with several signals and the states they
# leave, fg and bg of running jobs, %N &, disown, wait -n and -f, and
# killed jobs reported when wait collects them.
set -m
pids() { sed 's/  *[0-9][0-9]*/ N/g'; }
sleep 5 & sleep 5 & sleep 0.2 &
jobs
jobs -r | pids; jobs -p | sed 's/[0-9][0-9]*/N/'; jobs -l | pids
sleep 0.5; jobs; jobs
kill %1; kill -9 %2; kill -INT %3 2>&1; sleep 0.3; jobs
{ sleep 5 & kill -HUP %1; wait %1; echo "hup $?"; } 2>out; pids <out
{ sleep 5 & kill -s USR1 %+; wait; echo "usr1 $?"; } 2>out; pids <out
sleep 5 & kill -INT %1; wait; echo "int $?"
sleep 0.3 & fg %1; echo "fg $?"
sleep 0.3 & bg 2>&1; wait -f %1; echo "f $?"
sleep 0.2 & sleep 5 & disown %2; jobs; wait -n; echo "n $?"; kill $!
sleep 5 & %1 & kill %1; wait; jobs
echo end
