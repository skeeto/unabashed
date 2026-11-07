# ${!a[@]:N} and ${!a[*]:N} take the elements as names: a missing array
# is an invalid indirection (abandoning the command), a declared empty
# one names nothing. read -t wraps its seconds at 2^32 as bash's timer
# does, and a timeout that wraps to nothing sets no timer at all.
exec 2>&1
echo "a:${!c[@]:1}:"; echo "not reached"
f() { echo "b:${!c[*]:0}:"; echo "not reached"; }; f; echo "not reached"
echo "status $?"
echo "keys:${!c[@]}:"
declare -a c; echo "empty:${!c[@]:1}:"
set --; echo "pos:${!@:1}:"
sleep 2 | read -t 4294967296.3 x; echo "wrapped $?"
sleep 1 | read -t 4294967296 x; echo "no timer $?"
read -t 9223372036854775808 x </dev/null; echo "overflow $?"
