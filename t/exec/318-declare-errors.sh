declare -Z x
echo "status $?"
declare 1abc=3
echo "status $?"
declare 'a b'
echo "status $?"
declare good=1 '=bad' also=2
echo "status $? good=$good also=$also"
typeset -q
echo "status $?"
readonly -z
echo "status $?"
export -q
echo "status $?"
declare -
echo "status $?"
