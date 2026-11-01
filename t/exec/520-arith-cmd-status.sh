(( 0 )); echo "0 -> $?"
(( 1 )); echo "1 -> $?"
(( -1 )); echo "-1 -> $?"
(( 5 - 5 )); echo "5-5 -> $?"
(( x = 0 )); echo "x=0 -> $? x=$x"
(( y = 7 )); echo "y=7 -> $? y=$y"
(( "3" )); echo "quoted -> $?"
(( 2 > 1 )); echo "2>1 -> $?"
(( 1 > 2 )); echo "1>2 -> $?"
(( z++ )); echo "z++ -> $? z=$z"
(( z++ )); echo "z++ -> $? z=$z"
(( --z )); echo "--z -> $? z=$z"
if (( 3 )); then echo if-yes; fi
if (( 0 )); then echo if-yes; else echo if-no; fi
n=0
while (( n < 3 )); do echo "n=$n"; (( n++ )); done
until (( n == 0 )); do (( n-- )); done; echo "n=$n"
(( 1 )) && echo and-ok
(( 0 )) || echo or-ok
! (( 0 )); echo "bang -> $?"
((0)); echo "nospace -> $?"
((	9	)); echo "tabs -> $?"
