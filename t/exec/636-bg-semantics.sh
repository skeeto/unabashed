# Background jobs and the rest of the shell: input from /dev/null,
# "a & b &" as two jobs, reports of jobs killed by a signal (pids
# hidden), jobs in subshells, and command substitutions waiting for
# their writers
echo input | { cat & wait; }
cat & wait
{ cat; } < <(echo redirected) & wait
sleep 2 & sleep 2 &
jobs
( jobs; echo "sub" )
echo "$(jobs)"
jobs | cat
{ jobs; } | cat
wait
bash -c '
f() { sleep 5; }
f & kill -9 $!; sleep 1
echo "after kill"
sleep 5 & kill -USR1 $!; sleep 1
echo "after usr1"
sleep 5 & kill $!; sleep 1
echo "after term"
trap "echo hup" HUP
sleep 5 & kill -HUP $!; sleep 1
echo "after hup"
sleep 5 & kill -QUIT $!; sleep 1
echo "end"
' 2>&1 | sed 's/line \([0-9]*\): *[0-9]* /line \1: PID /'
x=$( { sleep 1; echo bg; } & echo fg )
echo "$x"
( { sleep 1; echo orphan; } & )
echo "parent"
sleep 2
echo $( (sleep 1; echo inner) & )
{ echo first; sleep 1; echo second; } &
wait $!
echo "status $?"
sleep 2 & sleep 1; kill -INT $!; wait $!; echo "int ignored $?"
{ sleep 2; echo "group done"; } & sleep 1; kill -QUIT $!; wait $!; echo "quit ignored $?"
{ trap 'echo int trapped' INT; sleep 2; } & sleep 1; kill -INT $!; wait $!; echo "trap $?"
