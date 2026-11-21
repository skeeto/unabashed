# Process substitution under a low ulimit -n. At 64 its descriptor, 63,
# is the last there is: the program it is given to once failed to start
# ("Invalid argument"), each way of starting it moving the descriptors
# it maps to above 63. Below 64 bash leaves it where pipe made it, as
# we do now, rather than at 63, beyond the limit.
for n in 64 20; do
    ( ulimit -n $n; exec bash -c '
p=<(:); [ "${p#/dev/fd/}" -lt '$n' ] && echo "below the limit"
cat <(echo spawn); echo "spawn $?"
(ulimit -t 1000; cat <(echo fork); echo "fork $?")
{ cat <(echo job); echo "job $?"; } & wait
echo out > >(cat); wait; echo "out $?"
' )
done
