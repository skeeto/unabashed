f() { echo original; }
readonly -f f
f() { echo replaced; }
echo "status $?"
f
unset -f f
echo "unset status $?"
f
readonly -f nosuch
echo "status $?"
readonly -f
declare -F f
