echo "$(( 1 ? 10 : 20 )) $(( 0 ? 10 : 20 ))"
x=5
echo "$(( x > 3 ? x * 2 : x / 2 ))"
echo "nested $(( x < 0 ? -1 : x == 0 ? 0 : 1 ))"
echo "nested2 $(( 1 ? 0 ? 3 : 4 : 5 ))"
i=0 j=0
echo "lazy $(( 1 ? i++ : j++ )) i=$i j=$j"
echo "lazy $(( 0 ? i++ : j++ )) i=$i j=$j"
echo "skip-div $(( 1 ? 2 : 1/0 )) $(( 0 ? 1/0 : 3 ))"
(( y = x ? 100 : 200 )); echo "y=$y"
echo "assign-in $(( 1 ? (z = 4) : 0 )) z=$z"
echo "with-comma $(( (1, 0) ? 7 : 8 ))"
echo "status:"; (( 0 ? 1 : 0 )); echo $?
v=$(( x ? x : -1 ))
echo "v=$v"
