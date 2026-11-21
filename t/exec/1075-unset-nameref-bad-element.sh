# unset through a nameref to an array element, as bash's unset_builtin:
# a bad subscript is reported but is not a failure (bash ignores
# unbind_array_element's result there); unset 'r[2]' finds no variable
# and does nothing, while unset -v 'r[2]' unsets what r names, its own
# subscript ignored. An empty associative key is a bad subscript.
exec 2>&1
a=(x y z)
declare -n r='a[-5]'; unset r; echo "1 $? ${a[*]}"
declare -n r='a[-5]'; unset -v r; echo "2 $? ${a[*]}"
declare -n r='a[-5]'; unset -n r; echo "3 $? ${r-unset}"
f() { local -n l='a[-9]'; unset l; echo "4 $?"; }; f
declare -n r='s[1]'; s=str; unset r; echo "5 $? $s"
declare -n r='a[0]'
unset 'r[2]'; echo "6 $? ${a[*]}"
unset -v 'r[2]'; echo "7 $? ${a[*]}"
a=(x y z); declare -n r='a[-9]'
unset -v 'r[2]'; echo "8 $? ${a[*]}"
declare -A A=([k]=v); e=
declare -n r='A[$e]'; unset r; echo "9 $? ${!A[*]}"
unset 'A[$e]'; echo "10 $?"
a=(x y z); declare -n r='a[-9]'; unset r 'a[-9]'; echo "11 $? ${a[*]}"
a=(x y z); declare -n r='a[1]'; unset r; echo "12 $? ${a[*]}"
