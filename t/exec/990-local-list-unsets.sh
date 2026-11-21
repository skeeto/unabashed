# While a new local array's list expands, the variable it hides is the
# one seen; code there may unset or replace that variable, and the
# local then hides whatever is there once the list is done
arr=(g1 g2 g3)
f() { local -a arr=( ${ unset arr; } x y ); declare -p arr; }; f
declare -p arr
arr=(g1 g2 g3)
f() { local -a arr=( ${ arr=(n1 n2); } "${arr[@]}" ); declare -p arr; }; f
declare -p arr
h=old
f() { local -A h=( [k]=${ unset h; }v ); declare -p h; }; f
declare -p h
s=old
g() { local s=inner; f; declare -p s; }
f() { local -a s=( ${ unset s; } x ); declare -p s; }; g
declare -p s
for i in {1..300}; do eval "v$i=$i"; done
f() { local -a z=( ${ unset z; } x ); }; z=(a b); f; f; declare -p z
