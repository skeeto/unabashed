echo before
f() { exit 9; }
declare -f
exit 3
echo after
