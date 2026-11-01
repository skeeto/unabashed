# Signals and waiting: a trapped signal interrupts wait, kill ends a
# process substitution's vproc (and so its program) or one blocked in
# read; SIGPIPE caught, ignored (also by the programs started) or fatal
trap 'echo got usr1' USR1
exec 3< <(sleep 1; kill -USR1 $$; sleep 2; echo late)
wait $!; echo "wait $?"
cat <&3
exec 3<&-
exec 4< <(sleep 5)
kill $!; wait $!; echo "killed $?"
exec 4<&-
exec 5< <(read x; echo "read $?")
kill -TERM $!; wait $!; echo "w $?"
exec 5<&-
{ trap 'echo caught >&2' PIPE; sleep 1; echo x; echo "st $?" >&2; } | true
echo "1 ${PIPESTATUS[*]}"
{ trap '' PIPE; sleep 1; echo x; echo "st $?" >&2; } | true
echo "2 ${PIPESTATUS[*]}"
{ sleep 1; echo x; echo "st $?" >&2; } | true
echo "3 ${PIPESTATUS[*]}"
trap '' PIPE
{ sleep 1; echo x; echo "st $?" >&2; } | true
echo "4 ${PIPESTATUS[*]}"
{ sleep 1; yes; echo "st $?" >&2; } | true
echo "5 ${PIPESTATUS[*]}"
