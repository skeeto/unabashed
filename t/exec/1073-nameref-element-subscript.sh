# A nameref to an array element reads as unset when it is itself
# subscripted, as bash's find_variable finds nothing by the name
# "a[0]": ${r[0]}, ${r[@]}, ${!r[@]}, and -v of r[0], while $r and
# [[ -v r ]] (unset too) are as before. -v on such names, on chains of
# them and on subscripts that run commands never runs out of memory.
a=(x y); x=s; declare -n r='a[0]' p=x
echo "<${r[0]}> <${r[@]}> <${!r[@]}> <${#r[@]}> <${#r[0]}> <${r[*]}> <$r>"
echo "<${p[0]}> <${p[@]}> <${!p[@]}> <${#p[@]}>"
[[ -v r[0] ]]; echo "1 $?"
[[ -v r[@] ]]; echo "2 $?"
[[ -v p[0] ]]; echo "3 $?"
test -v 'r[0]'; echo "4 $?"
declare -A A=([0]=v); declare -n q='A[0]'
echo "<${q[0]}> <${q[@]}> <${!q[@]}> <${q[0]:-unset}>"
[[ -v q[0] ]]; echo "5 $?"
test -v 'q[0]'; echo "6 $?"
declare -n rr=r w='a[$(echo 1)]'
[[ -v r ]]; echo "7 $?"
test -v rr; echo "8 $?"
[[ -v w ]]; echo "9 $?"
[[ -v a[1] ]]; echo "10 $?"
test -v 'a[$(echo 1)]'; echo "11 $?"
f() { local -n l=$1; [[ -v l ]]; echo "12 $?"; [[ -v l[0] ]]; echo "13 $?"; }
f 'a[1]'; f 'A[0]'
set -u
echo "<${r[0]}>"
echo not reached
