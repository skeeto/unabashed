# declare with namerefs: attributes go to the target, +n, listings
x=5
declare -n r=x
declare -i r; r=2+3; declare -p x
declare -u r; r=abc; declare -p x
declare +n r; declare -p r
declare -n r; declare -p r
declare -n 2>&1 | grep -v '^declare -n BASH'
foo=bar; declare -n foo; declare -p foo
declare +n foo=other; echo "$foo $bar"
declare -n n1='1x' 2>&1; echo "st=$?"
declare -n n2=n2 2>&1; echo "st=$?"
declare -a arr=(1); declare -n arr 2>&1; echo "st=$?"
declare -n n3=; echo "st=$?"
readonly x; declare -n rx=x; rx=1; echo "st=$?"
declare -n e1; declare -a e1; declare -p e1
declare -n e2; declare e2=42; echo "st=$?"; declare -p e2
export r
declare -nx xr=x; printenv xr
for xr in a b; do declare -p xr; done
declare -n l; for l in one two; do echo "$l ${!l}"; done
one=1 two=2; for l in one two; do echo "$l"; done
