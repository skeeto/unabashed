# A background program's group (kill -- -PID: its own job on Windows 8
# and later) ends with it. Once the program and everything it started
# are gone, the group is no such process, as in bash, rather than an
# empty job that a kill "succeeds" on (Windows reuses the pid at once).
# While something is left, the kill reaches it. cmd leads the groups:
# not one of our shells, whose signal channel takes a group's signal.
sleep 0.2 & p=$!
wait $p; echo "wait $?"
kill -- -$p 2>/dev/null; echo "ended $?"
kill -0 -- -$p 2>/dev/null; echo "probe $?"
cmd //c "sleep 5" & p=$!
sleep 0.5
kill -- -$p; echo "live $?"
wait $p; echo "killed $?"
sleep 0.3  # a job's processes end after TerminateJobObject returns
kill -- -$p 2>/dev/null; echo "after $?"
# Its program gone, the group lives on in what that started: it exists,
# and a kill ends it (Windows 8 and later; before, there is no group
# beyond the program, as Windows 7 is version 6.1 and XP 5.1).
nested=1
case "$(cmd //c ver)" in *n\ 5.*|*n\ 6.[01].*) nested= ;; esac
cmd //c "start /b sleep 3 >NUL 2>NUL <NUL" & p=$!
wait $p
if [ "$nested" ]; then
    kill -0 -- -$p; echo "kin $?"
    kill -- -$p; echo "kin killed $?"
    sleep 0.3
    kill -0 -- -$p 2>/dev/null; echo "kin gone $?"
else
    sleep 3  # the leftover sleep, which nothing can reach
    printf 'kin 0\nkin killed 0\nkin gone 1\n'
fi
