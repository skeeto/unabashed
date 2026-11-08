# Output to regular files is coalesced: whatever looks next still sees
# it in order, and a file written now and then fills within 100 ms
exec 3>f
echo a >&3
echo b >>f
echo c >&3
cat f
echo x >g; [ -s g ] && echo nonempty
: >h; { echo one >>h; echo go; } | { read -r; cat h; }
printf '%s\n' 1 2 3 >k; wc -l <k
(echo sub >>k); tail -n 1 k
{ echo bg >>k; } & wait; tail -n 1 k
exec 4>>m; echo first >&4; read -r line <m; echo "read: $line"
"$U" -c 'echo child >>m'; echo parent >&4; cat m
# more than the buffer holds, then a little, at once
s=$(printf '%20000s' ''); { printf '%s' "$s"; echo tail; } >big; wc -c <big
# a busy loop writing a line now and then: a reader a second later
# sees lines from before it started
( "$U" -c 'sleep 1'; n=$(wc -l <log); [ "$n" -gt 0 ] && echo "seen early" ) &
end=$(( ${EPOCHSECONDS} + 3 ))
i=0; while [ $EPOCHSECONDS -lt $end ]; do i=$((i+1)); [ $((i % 2000)) = 0 ] && echo $i; done >log
wait
