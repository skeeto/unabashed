# Under command and builtin, a declaration builtin's arguments keep
# their own forms: a quoted 'name=(...)' after an unquoted name[sub]
# is a string, not a list, and name[sub] is no compound assignment.
p() { printf '<%s>' "$@"; echo; }
command declare a[1] 'x1=(1 2)'; declare -p x1
builtin declare a[1] 'x2=(1 2)'; declare -p x2
command typeset a[1] 'x3=(1 2)' a[2] 'x4=(3 4)'; declare -p x3 x4
command export a[0] 'x5=(1 2)' 2>&1; declare -p x5
builtin readonly a[0] 'x6=(1 2)' 2>&1; declare -p x6
f() {
    builtin local a[0] 'x7=(3 4)'; declare -p x7
    command local a[0] 'x8=(5 6)'; declare -p x8
}
f
command -p declare a[0] 'x9=(7 8)'; declare -p x9
command builtin declare a[0] 'x10=(9 0)'; declare -p x10
declare -r ro=(1 2)
command declare ro[1] 2>&1; declare -p ro
builtin declare -a ro[0] 2>&1; declare -p ro
declare -a b=(1 2 3)
command unset 'b[1]'; builtin unset b[2]; declare -p b
