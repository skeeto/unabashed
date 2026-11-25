# An array assignment cut off by the end of eval's text leaves bash's
# parser expecting more of it, until a later syntax error resets it:
# the next parse then ends at once, without the text it was given
eval "x=("; echo "1: $?"
eval "x=("; echo "2: $?"
eval "x=("; eval ":"; eval "x=("; echo "3: $?"
for i in 1 2; do eval "x=("; echo "4: $i $?"; done
y=$(eval "x=("; echo $?); echo "5: $y"
# In a subshell, the abandoned parse ends the subshell
( eval "x=("; echo "6: $?" ); echo "6: $?"
( for i in 1 2; do eval "x=("; echo "7: $?"; done ); echo "7: $?"
# A command substitution inside ${...} that does not end is reported as
# its own parse reports it, abandoning the command (# nofixedpoint:
# the function's text does not parse again)
x=a
f() { echo ${x+"$x$(: $(r+=( $'a\'b' )))"}; echo not reached; }
f
echo "8: $?"
