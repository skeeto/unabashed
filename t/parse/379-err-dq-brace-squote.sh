echo before
f() { echo "${x:-'abc}"; }
echo after
declare -f
