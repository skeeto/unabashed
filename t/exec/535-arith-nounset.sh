set -u
x=1
echo "$(( x + 1 ))"
(( x++ )); echo "x=$x"
(( undefined_v + 1 )); echo "status $?"
echo "after"
