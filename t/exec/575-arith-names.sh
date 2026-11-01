_x=3 x_1=4 X=5 xX9=6
echo "names $(( _x + x_1 + X + xX9 ))"
unset u
echo "dollar-unset-plus $(( $u + 1 ))"
echo "dollar-unset-alone $(( $u ))"
y=abc abc=12
echo "y->abc $(( y ))"
(( z = y )); echo "z=$z"
w='abc + 1'
echo "w $(( w ))"
echo "minus-dollar $(( -$X ))"
echo "dollar-brace $(( ${X} * ${x_1} ))"
echo "positional"
set -- 7 8
echo "pos $(( $1 * $2 )) $(( $# + 1 ))"
echo "dollar-unset-times $(( $u * 2 ))"
echo unreachable
