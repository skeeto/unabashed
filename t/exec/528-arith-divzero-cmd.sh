(( 1 / 0 )); echo "div status $?"
(( 5 % 0 )); echo "mod status $?"
x=3
(( x /= 0 )); echo "div-assign status $? x=$x"
(( y = 2, 1 / 0 )); echo "partial status $? y=$y"
if (( 1/0 )); then echo then; else echo "else $?"; fi
(( 1/0 )) || echo "or-branch"
echo "end"
