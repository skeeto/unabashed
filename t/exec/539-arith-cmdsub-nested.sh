echo "$(( $(echo 3) * 2 ))"
echo "$(( $(( 1 + 1 )) ** 3 ))"
(( x = $(echo 4) + 1 )); echo "x=$x"
f() { echo 5; }
echo "$(( $(f) - 1 ))"
y=2
echo "$(( y$(echo 0) ))" 2>&1
y0=11
echo "$(( y$(echo 0) ))"
echo "$(( `echo 6` / 2 ))"
echo $(( 1 +
  2 +
  3 ))
(( z = 1 +
   2 )); echo "z=$z"
n=3
echo "$(( n${n:+0} ))"
echo "$[ 2 * 21 ]"
