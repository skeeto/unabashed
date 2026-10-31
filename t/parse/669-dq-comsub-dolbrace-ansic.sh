f() { echo "$(echo ${x-$'a b'})" "${ echo ${x:-$'c'}; }"; }
declare -f
