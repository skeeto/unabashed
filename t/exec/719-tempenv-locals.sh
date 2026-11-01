# temporary bindings meet export, readonly, declare -I, local lists, set -k
var=60 export var; declare -p var
tv=foo declare -r tv; echo ${tv@A}
f1() { a=3 readonly a; echo f1:$a; }
a=7 f1; echo "global: ${a-unset}"
fn() { var=value declare -x var; declare -p var; }
var=one; fn; declare -p var
arr=(zero one two)
g() { local -a arr=( "${arr[@]}" ); arr+=(three); declare -p arr; }
g; declare -p arr
var=global
h() { local -I var; declare -p var; local var2; declare -p var; }
h
export VAR1=abc
k() { local VAR1; printenv VAR1; local -p VAR1 PATH 2>&1; }
k
set -k
echo a=1 b c=2
x=5 true y=6; echo "$x $y"
set +k
f2() { echo "f2: $x"; unset x; }
f3() { local x=local; f2; echo "after: ${x-unset}"; }
x=global; f3
shopt -s localvar_unset; f3; shopt -u localvar_unset
IGNOREEOF=0; shopt -o ignoreeof
f4() { local -; set +o ignoreeof; }
f4; shopt -o ignoreeof; echo "$IGNOREEOF"
