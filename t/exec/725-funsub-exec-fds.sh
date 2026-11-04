# exec in ${ ...; } leaves its other descriptors on the substitution's
# output after it ends, where what they write is lost, while standard
# output comes back
trap 'echo trap out; echo trap err >&2' EXIT
x=${ exec 2>&1; echo in; }
echo "[$x]"
nosuch1
echo lost >&2
exec 2>&1
echo back >&2
f() { y=${ exec 2>&1; echo a; echo $((1/0)); echo b; }; echo "f [$y] $?"; }
f
nosuch2
exec 2>&1
z=${ exec >&2; echo to stderr; }
echo "[$z]"
v=${ exec 2>&1; { echo bg; echo bg2 >&2; } & wait; echo fg; }
echo "[$v]"
echo lost2 >&2
exec 2>&1
w=${ exec 2>&1; echo $((1/0)); }
echo "[$w]"
echo "${ exec 2>&1; echo $((1/0)); }" >&2
for i in 1 2 3; do : ${ exec 2>&1; echo $i; }; nosuch$i; exec 2>&1; done
echo "${ exec 2>&1; : ${u?unset}; echo no; }"
echo unreached
