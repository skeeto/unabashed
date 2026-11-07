# A script's bare wait forgets the jobs it collects, $!'s too, but
# keeps the status of $!'s job when that one ended before the wait:
# wait -n and wait $! still find it, until jobs reports it.
exec 2>&1
for i in 1 2 3; do sleep 0.0$i & done
sleep 0.5
wait; echo "w=$?"
wait -n; echo "n=$?"
wait $!; echo "p=$?"
for i in 1 2 3; do sleep 0.0$i & done
sleep 0.5
wait; echo "w=$?"; jobs >/dev/null
wait -n; echo "n=$?"
for i in 1 2 3; do sleep 0.$i & done
wait; echo "w=$?"
wait -n; echo "n=$?"
wait $! 2>&1 | sed 's/pid [0-9]*/pid N/'
set -m
for i in 1 2 3; do sleep 0.0$i & done
sleep 0.5
wait; echo "w=$?"
wait -n; echo "n=$?"
