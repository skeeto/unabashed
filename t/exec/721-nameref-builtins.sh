# namerefs through array forms, arithmetic, read, printf -v, getopts, tests
arr=(a b c); declare -n r=arr
echo "${#r[@]} ${r[@]:1} ${!r[@]} ${r[1]} ${r[-1]} ${#r} ${r@A}"
echo "$(( r[1]=5, r[1]+1 ))"; declare -p arr
read r <<< "x y"; declare -p arr
printf -v r '%s' fmt; declare -p arr
declare -n o=opt; getopts ab: o -b; echo "$o $opt $OPTARG"
[[ -v r ]] && echo set; [[ -v r[2] ]] && echo set2; [ -v r ] && echo tset
echo "${r+plus} ${nope+x}"
declare -n n2=1 2>&1; echo $?
upvar() { local -n _ret=$1; _ret=(1 2 3); }
caller() { local res; upvar res; declare -p res; }; caller
outer() { local v=out; inner v; echo "outer: $v"; }; inner() { local -n ref=$1; ref=changed; unset ref; echo "inner: ${ref-unset}"; }; outer
declare -n e1=a b1=c; a=1 c=2; echo "$e1 $b1"
declare -n m; declare -A mm=([k]=v); m=mm; echo "${m[k]} ${!m}"
x=5; declare -n xr=x; (( xr++ )); let xr*=2; echo $x
declare -n self2; self2=self2 2>&1; echo "st=$?"; declare -p self2
for v in 1 2; do :; done; declare -n lv; for lv in arr x; do echo "${lv}"; done
declare -n dr=undefvar; echo "[${dr:-dflt}] [${dr:=assigned}] $undefvar"
f() { declare -n gr=$1; declare -g gr=9; }; f gx; echo "gx=$gx"
declare -n ir=x; declare -i ir; ir=3*3; declare -p x
unset -n ir; declare -n ir=x; export ir; declare -p x
declare -n pr=x; declare -p pr x; declare +n pr; declare -p pr
