# kill reaches a nested shell blocked in read: TERM ends it, a trapped
# signal runs its action and the read goes on
exec 3< <(sleep 2 | bash -c 'echo $$; read x; echo "got $x"')
read pid <&3
kill $pid; echo "kill $?"
cat <&3; exec 3<&-
exec 3< <(sleep 2 | bash -c 'trap "echo trapped" USR1; echo $$; read x; echo "read $?"')
read pid <&3
kill -USR1 $pid; echo "kill $?"
cat <&3; exec 3<&-
kill -0 $pid 2>/dev/null; echo "gone $?"
trap 'echo t' USR1
read x < <(kill -USR1 $$; sleep 1; echo line); echo "$x $?"
trap - USR1
