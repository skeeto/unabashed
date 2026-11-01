# errors opening redirection targets; the command does not run
echo before
echo notrun </tmp/nosuch
echo status $?
echo notrun >/tmp/nodir/file
echo status $?
echo notrun >/tmp
echo status $?
echo notrun >>/tmp/nodir/x
echo status $?
read x </tmp/nosuch
echo status $? "x=$x"
f() { echo in f; }
f </tmp/nosuch
echo status $?
echo notrun 2>/tmp/nodir/e
echo status $?
echo after
