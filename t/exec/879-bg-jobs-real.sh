# Background jobs as processes: wait collects a killed one with its
# report, wait -n/-p/-f, jobs and disown work on their pids, and DEBUG
# runs in the parent before a background simple command
# (reports name pids, which differ from bash's: masked)
f() { sleep 5; }
{
f &
kill -9 $!
wait
echo "after wait $?" >&2
sleep 5 &
kill -9 $!
wait $!
echo "status $?" >&2
} 2>&1 | sed 's/: line [0-9]*: *[0-9]* Killed/: Killed/'
{ sleep 1; exit 3; } &
a=$!
{ sleep 2; exit 4; } &
b=$!
wait -n -p who
echo "first $? $([ "$who" = "$a" ] && echo a)"
wait -n -p who
echo "second $? $([ "$who" = "$b" ] && echo b)"
{ sleep 1; exit 5; } &
wait -f $!
echo "waited -f $?"
{ sleep 1; } &
jobs
disown
jobs
echo "jobs gone"
trap 'echo "DEBUG $BASH_COMMAND"' DEBUG
true &
wait
true | true &
wait
trap - DEBUG
echo end
