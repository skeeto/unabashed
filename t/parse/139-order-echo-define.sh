echo first
f() { echo body; }
echo second
declare -f
echo third
