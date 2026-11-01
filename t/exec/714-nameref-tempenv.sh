# temporary bindings through namerefs bind the target
var=foo
declare -n ref=var
ref=temp printenv var
ref=temp declare -p var
declare -p var
f() { echo "f: $var $ref"; }
ref=fn f
echo "$var"
declare -n u
u=/ f
exec {fd}>/dev/null; echo "fd=$fd"
declare -n badfd; exec {badfd}>/dev/null; echo "st=$?"
