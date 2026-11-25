# ${!x} is an invalid indirect expansion only when no variable x
# exists: one declared but unset (export, declare, local, readonly,
# a local unset in its own function) names nothing and expands as
# unset, through its operators too, and set -u calls it `!x'. Assigning
# through such an indirection (${!x:=v}, ${!1=v}) is an invalid
# indirect expansion named `!x'. A nameref that is unset, or ends a
# chain of them unset, is one too, named by the first.
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c
echo \"after \$?\"" 2>&1
done <<'EOF'
export x; echo "[${!x}]"
declare x; echo "[${!x}]"
declare -a x; echo "[${!x}]"
readonly x; echo "[${!x}]"
f() { local x; echo "[${!x}]"; }; f
f() { local x=1; unset x; echo "[${!x}]"; }; f
x=1; unset x; echo "[${!x}]"
export x; echo "[${!x-d}] [${!x:+a}] [${!x@Q}] [${!x#a}]"
declare -i x; echo "[${!x:-d}]"
set -u; export x; echo "[${!x}]"
export x; echo "[${!x?}]"
export x; echo "[${!x:=a}]"
export x; echo "[${!x=a}]"
echo "[${!1:=a}]"
a=(); echo "[${!a[0]:=a}]"
x=y; echo "[${!x:=a}] $y"
declare -n r; echo "[${!r}]"
declare -n r; echo "[${!r-d}]"
declare -n r s=r; echo "[${!s}]"
declare -n r=x; echo "[${!r}]"
declare -n r=s s=t; echo "[${!r}]"
EOF

# In POSIX mode, of the errors bash reports with report_error only a
# bad substitution, an unbound element and a bad subscript to assign
# end the shell (expand_wdesc_fatal); an invalid indirection or
# variable name, a parameter that cannot be assigned, failglob's no
# match, a bad subscript for a length and a negative substring length
# abandon the command (DISCARD), as without it.
while IFS= read -r c; do
    echo "-- $c"
    bash -o posix -c "$c; echo \"same line\"
echo \"after \$?\"" 2>&1; echo "status $?"
done <<'EOF'
echo ${!r}
r=1a; echo ${!r}
declare -n r; echo ${!r}
echo ${!x:=v}
echo ${1=x}
shopt -s failglob; echo /nope*
x=(1); echo ${#x[-5]}
x=abc; echo ${x:1:-5}
echo ${x!}
echo ${arr[-1]:=a}
set -u; echo ${a[@]}${a[1]}
echo $((1/0))
EOF
