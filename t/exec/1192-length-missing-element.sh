# ${#name[sub]} is 0 when no variable name is set (missing, declared
# without a value, or unset), as bash's array_length_reference: the
# subscript is not evaluated, so its errors and substitutions do not
# happen. A set scalar or an empty array still evaluates it; set -u
# still reports the element unbound.
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c
echo \"after \$?\"" 2>&1
done <<'EOF2'
echo ${#map[a b]}
echo ${#x[-5]}
echo ${#x[$(echo hi)]}
x=1; unset x; echo ${#x[1/0]}
declare -a x; echo ${#x[a b]}
export x; echo ${#x[a b]}
declare -n r=q; echo ${#r[a b]}
x=; echo ${#x[a b]}
x=abc; echo ${#x[-5]}
x=(); echo ${#x[a b]}
set -u; echo ${#map[1]}
EOF2
