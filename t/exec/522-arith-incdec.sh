i=5
echo "i++ = $(( i++ )) now $i"
echo "++i = $(( ++i )) now $i"
echo "i-- = $(( i-- )) now $i"
echo "--i = $(( --i )) now $i"
j=1
echo "j++ + j++ = $(( j++ + j++ )) now $j"
k=1
echo "++k * ++k = $(( ++k * ++k )) now $k"
unset m
echo "m++ = $(( m++ )) now $m"
unset m
echo "--m = $(( --m )) now $m"
s=' 7 '
echo "s++ = $(( s++ )) now $s"
e='3 + 4'
(( e++ ))
echo "e now $e"
x=10
echo "- -x = $(( - -x )) x=$x"
echo "-+x = $(( -+x ))"
echo "+ +x = $(( + +x ))"
echo "x--- = $(( x--- 1 )) x=$x"
z=2
echo "z+++z = $(( z+++z )) z=$z"
