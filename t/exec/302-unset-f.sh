f() { echo "f runs"; }
f
unset -f f
f
echo "status $?"
unset -f f
echo "unset missing status $?"
declare -F f
echo "declare -F status $?"
g() { echo g; }
g=var
unset -f g
echo "g var: $g"
g
echo "status $?"
