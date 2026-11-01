# duplicating descriptors that are not open
echo a >&5
echo status $?
read x <&7
echo status $?
echo b 2>&3
echo status $?
echo c 3>&4
echo status $?
{ echo d; } >&6
echo status $?
echo e 1>&9 2>&1
echo status $?
echo done
echo self 4>&4
echo status $?
echo selfmove 4>&4-
echo status $?
read y 0<&0 <<<"in"
echo "status $? y=$y"
