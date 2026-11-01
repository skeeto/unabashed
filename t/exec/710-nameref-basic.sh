# namerefs: reads, assignments, ${!ref}, chains, unset and unset -n
x=1
declare -n r=x
echo "$r ${!r} ${#r}"
r=2; echo "$x"
r+=3; echo "$x"
declare -n s=r
s=4; echo "$x ${!s}"
declare -p r s
unset r; echo "x=${x-unset}"; declare -p r
unset -n r; declare -p r 2>&1
declare -n e; e=y; declare -p e; e=5; echo "y=$y"
declare -n bad; bad='a b'; echo "st=$?"; declare -p bad
unset -n e bad s
declare -n z
echo "${z=dflt}"; declare -p z
unset -n z
declare -n w=undefined
w=new; echo "$undefined"
echo "${!r-none} ${!w}"
x=abcde; declare -n v=x; echo ${v//c/x} ${v:1:2} ${v^^}
