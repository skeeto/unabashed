echo "empty $(( ))"
echo "blank $((   ))"
x=
echo "var-empty $(( x ))"
(( )); echo "cmd empty status $?"
((  )); echo "cmd blank status $?"
(( 
  1
  +
  1
)); echo "multiline status $?"
echo "tabs $((	3	*	3	))"
echo "nl $((
4
))"
v='
5
'
echo "var nl $(( v ))"
echo "dq $(( "1 + 1" ))"
echo "dq2 $(( "1" + "2" ))"
y=' '
echo "space var $(( y + 1 ))"
