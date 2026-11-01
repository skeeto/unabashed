a='1 + 2'
echo "a*3 = $(( a * 3 ))"
echo "\$a*3 = $(( $a * 3 ))"
b=a
echo "b = $(( b ))"
c='b + 1'
echo "c = $(( c ))"
e=
echo "e+1 = $(( e + 1 ))"
unset u
echo "u+1 = $(( u + 1 ))"
s='  42  '
echo "s = $(( s ))"
n=010
echo "n = $(( n ))"
h=0x1F
echo "h = $(( h ))"
neg=-5
echo "-neg = $(( -neg )) neg*neg = $(( neg*neg ))"
echo "--neg = $(( - -neg ))"
p='2 > 1'
echo "p = $(( p ))"
q='x = 4'
echo "q = $(( q )) x=$x"
r='y += 2'
y=3
echo "r = $(( r )) y=$y"
(( v = a ))
echo "v=$v"
w='a'
echo "w*2 = $(( w * 2 ))"
