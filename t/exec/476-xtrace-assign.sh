set -x
a=1 b=2
s='x y'
q="it's"
t=$'tab\there'
n=$((3 + 4))
c=$(echo sub)
a=5 echo prefix
a=6 b=7 : multi
export e=1
readonly r='a b'
declare -i i=3+4
local_test() { local l=v m; l=w; }
local_test
x+=more
echo "$a $b $s $q $n $c $i $x"
