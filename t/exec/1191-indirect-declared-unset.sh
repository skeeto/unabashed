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
