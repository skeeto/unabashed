# wait: for everything, for pids and job specs, for any (-n), with -p,
# statuses kept after a job is gone, and the errors
{ sleep 1; exit 3; } &
p1=$!
{ sleep 2; exit 4; } &
p2=$!
wait $p2; echo "p2 $?"
wait $p1; echo "p1 $?"
wait $p1; echo "p1 again $?"
{ sleep 1; exit 7; } &
wait %1; echo "%1 $?"
wait %1; echo "%1 again $?"
sleep 2 & sleep 1 &
wait; echo "all $?"
jobs
{ sleep 2; exit 1; } & { sleep 1; exit 2; } & q=$! ; { sleep 3; exit 3; } &
wait -n; echo "n $?"
wait -n -p who; echo "n $? $([ "$who" = "$q" ] && echo first || echo other)"
wait -n %3 %2; echo "n2 $?"
wait -n; echo "none $?"
wait -n -p who; echo "none $? ${who-unset}"
{ sleep 1; exit 9; } & r=$!
wait -p who $r; echo "p $? $([ "$who" = "$r" ] && echo same)"
wait 12345; echo "x $?"
wait %8; echo "y $?"
wait foo; echo "z $?"
wait 1x; echo "w $?"
wait -x; echo "o $?"
wait -p; echo "q $?"
wait -p 1bad; echo "v $?"
readonly RO
wait -p RO; echo "r $?"
wait -n 12345 %9; echo "m $?"
exec 3< <(exit 6)
wait $!; echo "psub $?"
exec 3<&-
true &
wait -f $!; echo "f $?"
wait; echo "done $?"
