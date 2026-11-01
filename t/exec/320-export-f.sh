f() { echo f; }
g() { echo g; }
export -f f
echo "status $?"
declare -fx
declare -Fx
export -f nosuch
echo "status $?"
export -nf f
echo "status $?"
declare -Fx
echo "end"
