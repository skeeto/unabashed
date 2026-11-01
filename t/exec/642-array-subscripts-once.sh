# Array subscripts in arithmetic, in [[ -v ]] and the arithmetic
# operators of [[ ]], and in unset name[sub], expand once: a key holding
# $(...) or brackets is used as it is, never run
declare -A A
key='x],b[$(echo ran >&2)'
(( A[$key]++ ))
(( A['$key']++ ))
(( A["$key"]++ ))
declare -p A
echo $(( A[$key] + 1 ))
[[ -v A[$key] ]]; echo "v $?"
[[ -v A['$key'] ]]; echo "v $?"
[[ -v A[nokey] ]]; echo "v $?"
[[ A[$key] -eq 2 ]]; echo "eq $?"
unset -v A["$key"]
declare -p A
k2='$(echo foo)'
A[$k2]=1; A[foo]=2
unset "A[$k2]"
declare -p A
A[$k2]=1; unset 'A[$k2]'; declare -p A
unset -v A[$k2]
declare -A B; B[$k2]=1
test -v B["$k2"]; echo "test $?"
shopt -s assoc_expand_once; shopt -p array_expand_once
test -v B["$k2"]; echo "test $?"
shopt -u array_expand_once
declare -a a
i=2
(( a[i]=5, a[$i+1]=6 ))
declare -p a
(( a[7<(4+2)] = 3 )); declare -p a
a=(['1']=x ["2"]=y [$'3']=z); declare -p a
BASH_COMPAT=51; unset a[@]; declare -p a 2>&1
