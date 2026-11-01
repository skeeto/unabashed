exec 2>&1
set -x
p='a*' s='x y'
[[ abc == $p ]]
[[ abc == "$p" ]]
[[ $s == "x y" && -n $s ]]
[[ ! -z $s || 1 -eq 2 ]]
[[ 1+1 -eq 2 ]]
[[ ( a < b ) ]]
test -n "$s"
[ "$s" = 'x y' ]
set +x
