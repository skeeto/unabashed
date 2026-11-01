# Background jobs: $!, the jobs listing (running, done, exit statuses,
# signals), job specs, and what a job's end looks like afterwards
sleep 3 &
p=$!
f() { sleep 2; echo "f ran"; return 3; }
f &
{ sleep 2; exit 5; } &
jobs
case $p in [0-9]*) echo "pid ok";; esac
[ "$(jobs -p %1)" = "$p" ] && echo "jobs -p agrees"
jobs %1 %+ %-
jobs %% %sleep %?exit
jobs %f
jobs %3x; echo "status $?"
jobs %s; echo "status $?"
jobs 1; echo "status $?"
jobs -r
sleep 4
echo "-- after"
jobs -n
jobs
echo "-- again"
jobs
sleep 3 & sleep 1; kill $!; sleep 1
trap : USR2; sleep 3 & sleep 1; kill -USR2 $!; sleep 1
jobs; echo "-- end"
jobs
true & sleep 1; jobs -r; jobs -s; jobs
