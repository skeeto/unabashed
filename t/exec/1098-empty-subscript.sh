# An empty subscript names no element (bash's valid_array_reference):
# a nameref cannot take a[], read, printf -v and wait -p call it not a
# valid identifier, unset without -v takes it for a function's name,
# ${a[]} is a bad substitution, [[ -v a[] ]] and ${!x} find no element,
# while an assignment or declare still says bad array subscript.
a=(x y); declare -A h=([k]=v)
declare -n r='a[]'; echo "declare -n $?"; declare -p r 2>&1
declare -n r='h[]'; echo "declare -n assoc $?"
declare -n r='r[]'; echo "declare -n self $?"
f() { local -n q='a[]'; echo "local -n $?"; }; f
declare -n e
{ e='a[]'; echo notreached; }
declare -p e
declare -n ar=a; declare -n ar+='[]'; echo "append $?"; declare -p ar
declare -n ar+='[]x'; echo "append $?"
declare -n ar+='[1]'; echo "append $?"; declare -p ar
declare -n fr; declare -n fr+='a[]'; echo "append new $?"
declare -n lr; for lr in a 'a[]' b; do echo "for $lr"; done; echo "for $?"
'a[]'() { echo fn; }
unset 'a[]'; echo "unset $?"; declare -F 'a[]'
unset 'h[]'; echo "unset assoc $?"
unset -v 'a[]'; echo "unset -v $?"
k=; unset a[$k]; echo "unset a[\$k] $?"
[[ -v a[] ]]; echo "-v $?"
test -v 'a[]'; echo "test -v $?"
read 'a[]' <<<z; echo "read $?"
read x 'h[]' <<<'z w'; echo "read second $?"
read -a 'a[]' <<<z; echo "read -a $?"
printf -v 'a[]' z; echo "printf -v $?"
wait -p 'a[]'; echo "wait -p $?"
shopt -s assoc_expand_once
read 'h[]' <<<z; echo "read once $?"
printf -v 'h[]' z; echo "printf -v once $?"
shopt -u assoc_expand_once
x='a[]'; { echo "${!x}"; echo notreached; }
{ echo "${a[]}"; echo notreached; }
{ echo "${#a[]}"; echo notreached; }
{ echo "${a[]:-d}"; echo notreached; }
{ a[]=1; echo notreached; }
declare 'a[]=1'; echo "declare $?"
declare -p a h
