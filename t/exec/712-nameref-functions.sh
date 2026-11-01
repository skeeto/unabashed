# namerefs in functions: local -n, dynamic scope, circular references
setvar() { local -n ref=$1; shift; ref="$*"; }
setvar g one two; echo "$g"
f() { local x=inner; local -n r=$1; r=set; echo "in: $x"; }
x=outer; f x; echo "out: $x"
loop() { typeset -n v=$1; v=inside; }
v=global; loop v; echo "$v"
declare -n p=q; declare -n q=p
p=1; echo "st=$?"
echo "[$p]"
unset -n p q
declare -n self=self
cnt() { local -n arr=$1; echo ${#arr[@]}; }
list=(a b c); cnt list
mk() { declare -n ref=var; declare -g ref=(X); declare -p ref var 2>&1; }
declare -n ref=foo
mk; declare -p foo
