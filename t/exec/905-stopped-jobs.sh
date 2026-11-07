# Stopped jobs under set -m. A foreground program that stops becomes a
# stopped job (a script prints nothing) and the command's status is 128
# plus the signal; a stop by TSTP breaks out of the loops running, other
# stops do not, and a function goes on. jobs shows Stopped, and jobs -l
# the signal's description. A stopped job becomes current. fg continues
# and waits for it, bg continues it, kill %N continues it to take TERM,
# wait warns, disown warns. Every case kills what it leaves stopped.
bash -c 'set -m; bash -c "kill -STOP \$\$"; echo st=$?; jobs; jobs -l | sed "s/ *[0-9][0-9]* Stopped/ PID Stopped/"; echo "[$!]"; fg; echo fgst=$?' 2>&1
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; jobs; disown %1; echo st=$?; kill -9 $!' 2>&1 | sed "s/group [0-9]*/group N/"
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; disown -h %1; echo st=$?; disown -a; kill -9 $!' 2>&1 | sed "s/group [0-9]*/group N/"
bash -c 'set -m; sleep 0.3 >/dev/null 2>&1 & p=$!; kill -STOP $p; sleep 0.1; bg >/dev/null; echo st=$?; jobs; [ "$!" = "$p" ] && echo same; wait' 2>&1
bash -c 'set -m; sleep 0.3 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; bg 2>/dev/null; jobs; bg; echo st=$?; wait' 2>&1
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; kill -CONT %1; sleep 0.1; jobs; kill -TSTP %1; sleep 0.1; jobs -l | sed "s/ *[0-9][0-9]* Stopped/ PID Stopped/"; kill -TTIN %1; jobs -l | sed "s/ *[0-9][0-9]* Stopped/ PID Stopped/"; kill -9 %1; sleep 0.1; jobs' 2>&1
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; wait %1; echo st=$?; wait -n; echo st=$?; wait; echo st=$?; jobs; wait $!; echo st=$?; kill -9 %1' 2>&1 | sed "s/job 1\[[0-9]*\]/job 1[PID]/"
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & sleep 1 >/dev/null 2>&1 & sleep 1 >/dev/null 2>&1 & kill -STOP %2; sleep 0.1; jobs; kill -STOP %1; sleep .1; jobs; kill -CONT %1 %2; sleep .1; jobs; kill %1 %2 %3' 2>&1
bash -c 'set -m; sleep 0.3 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; fg; echo st=$?; jobs' 2>&1
bash -c 'set -m; sleep 0.3 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; %1; echo st=$?' 2>&1
bash -c 'set -m; suspend -x; echo $?' 2>&1
bash -c 'suspend; echo $?' 2>&1
bash -c 'set -m; f() { bash -c "kill -STOP \$\$"; echo in f $?; }; f; for i in 1 2; do bash -c "kill -TSTP \$\$"; echo $?; done; jobs; jobs -l | sed "s/ *[0-9][0-9]* Stopped/ PID Stopped/"; kill -9 %1 %2' 2>&1
bash -c 'set -m; for i in 1 2; do bash -c "kill -STOP \$\$"; echo $i $?; done; echo after $?; jobs; kill -9 %1 %2' 2>&1
bash -c 'set -m; i=0; while [ $i -lt 2 ]; do i=$((i+1)); for j in a; do bash -c "kill -TSTP \$\$"; echo $i $j $?; done; done; echo after $?; kill -9 %1' 2>&1
bash -c 'set -m; bash -c "kill -TSTP \$\$"; echo $?; { bash -c "kill -TSTP \$\$"; echo inbrace $?; }; bash -c "kill -TSTP \$\$" && echo and; bash -c "kill -STOP \$\$" || echo or; kill -9 %1 %2 %3 %4' 2>&1
bash -c 'set -m; f() { bash -c "kill -TSTP \$\$"; echo in f $?; }; f; echo after $?; kill -9 %1' 2>&1
bash -c 'set -m
for i in 1 2; do bash -c "kill -TTIN \$\$"; echo $i $?; done
echo next
kill -9 %1 %2' 2>&1
bash -c 'set -m; sleep 0.2 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; kill -CONT %1; sleep 0.3; echo x; jobs' 2>&1
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; kill -9 %1; sleep 0.1; jobs' 2>&1
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; kill -TERM $!; sleep 0.1; jobs; kill -9 $!; sleep .1; jobs' 2>&1
bash -c 'set -m; x=1 bash -c "kill -STOP \$\$" >/dev/null 2>&1; jobs; kill -9 %1' 2>&1
bash -c 'set -m; bash -c "kill -STOP \$\$"; bash -c "kill -STOP \$\$"; jobs; fg %1; jobs; kill -9 %2' 2>&1
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & kill -STOP $!; exit 3'; echo st=$?
bash -c 'set -m; trap "echo got TSTP" TSTP; kill -TSTP $$; echo after' 2>&1
bash -c 'set -m; trap "" TSTP; kill -TSTP $$; echo after' 2>&1
bash -c 'set -m; bash -c "kill -TTIN \$\$"; echo $?; jobs; fg; jobs' 2>&1
bash -c 'set -m; bash -c "kill -STOP \$\$"; %1; echo st=$?; bash -c "kill -STOP \$\$; exit 4"; fg; echo st=$?' 2>&1
bash -c 'set -m; bash -c "kill -STOP \$\$; kill -STOP \$\$; exit 5"; echo st=$?; fg; echo st=$?; jobs; fg; echo st=$?' 2>&1
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; jobs -s; echo -; jobs -r; echo -; kill -CONT %1; sleep 0.1; jobs -s; echo -; jobs -r; kill -9 %1' 2>&1
bash -c 'set -m; sleep 1 >/dev/null 2>&1 & kill -STOP $!; sleep 0.1; kill %1; sleep 0.1; jobs; jobs' 2>&1 | sed 's/^\[1\]./[1]?/'
