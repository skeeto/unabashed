# subscripted temporary assignments are reported and skipped
f() { declare -p var 2>&1; echo "f: $?"; }
var[0]=X var[@]=Y f; echo "st=$?"
var[0]=X true; echo "st=$?"; declare -p var 2>&1
v[1]=1 printenv v; echo "st=$?"
var=(1 2) f
echo done
