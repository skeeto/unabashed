# $? inside a command substitution sees the status before it
false
x=$(echo $?)
echo "x=$x"
(exit 6)
echo "$(echo $?) $?"
true
y=$(false; echo $?)
echo "y=$y"
(exit 3)
( echo "sub sees $?" )
(exit 4)
echo "${ echo funsub sees $?; }"
