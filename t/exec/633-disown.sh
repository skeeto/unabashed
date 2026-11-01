# disown and its options, and fg, bg and suspend without job control
sleep 1 & sleep 2 & sleep 3 &
disown %2
jobs
disown -h %1
jobs
disown
jobs
disown %5; echo "s $?"
disown 12345; echo "s $?"
disown -z; echo "s $?"
sleep 1 & sleep 1 &
disown -a
jobs
disown; echo "none $?"
{ sleep 1; exit 1; } & sleep 3 &
sleep 2
disown -r
jobs
wait; echo "w $?"
fg; echo "fg $?"
bg %1; echo "bg $?"
suspend; echo "suspend $?"
suspend -z; echo "suspend $?"
sleep 3 & sleep 3 &
kill %1; wait %1; echo "kill %1 $?"
kill -s TERM %sleep; echo "one left $?"
kill %9; echo "no job $?"
sleep 3 & sleep 3 &
kill -s TERM %sleep; echo "ambiguous $?"
kill %%; wait; echo "end $?"
