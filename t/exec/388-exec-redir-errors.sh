# exec with a failing redirection reports it and continues
exec 3</tmp/nosuch
echo "status $?"
exec 4>/tmp/nodir/x
echo "status $?"
exec 5>&8
echo "status $?"
exec >$empty
echo "status $?"
echo still running
