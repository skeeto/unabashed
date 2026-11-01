# n>&- closes; writing to a closed descriptor fails
echo hi >&-
echo status $?
echo there 1>&-
echo status $?
{ echo out; echo err >&2; } 2>&-
echo status $?
read x <&-
echo status $? "x=$x"
printf 'x\n' >&-
echo status $?
echo closed-unopened 5>&- ok
echo status $?
