# Output the shell gathers for a file goes out within a moment though
# the shell computes on with no system call, and before a signal kills
# it: a reader of the file sees what bash would have written. Once,
# nothing came until the loop ended, and a killed shell's never came.
: >out
( exec >out; echo one; echo two; s=$SECONDS; while ((SECONDS - s < 4)); do :; done ) &
pid=$!
for ((i = 0; i < 200; i++)); do
    seen=$(<out)
    [[ $seen == *two* ]] && break
    sleep 0.01
done
echo "while it runs: ${seen//$'\n'/ }"
kill $pid; wait $pid; echo "status $?"

: >out
( exec >out; echo one; echo two; s=$SECONDS; while ((SECONDS - s < 4)); do :; done ) &
pid=$!
until [[ -s out ]]; do sleep 0.01; done
sleep 0.03
kill $pid; wait $pid; echo "status $?"
seen=$(<out)
echo "once killed: ${seen//$'\n'/ }"
