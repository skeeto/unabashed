# ${#name[sub]} of a variable that is missing or declared without a
# value is 0, its subscript never read, as bash's
# array_length_reference; a scalar's or an array's subscript is read.
# ${!name} of a variable declared without a value names nothing and
# expands as unset; only a missing one is an invalid indirection.
# declare's message for a bad element name leaves out the + of +=.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c"
    echo "status $?"
done <<'EOF'
echo ${#u[a b]}
echo ${#u[1/0]} ${#u[-1]} ${#u[$(echo ran >&2)]}
declare -a e; echo ${#e[a b]}
declare -A m; echo ${#m[a b]}
u=1; echo ${#u[a b]}
u=1; echo ${#u[-5]}
a=(1); echo ${#a[1/0]}
set -u; echo ${#u[1/0]}
export x; echo "<${!x}>"
declare x; echo "<${!x}>"
f() { local x; echo "<${!x}>"; }; f
echo "<${!x}>"
set -u; declare x; echo "<${!x}>"
declare arr[]+=a; declare arr[]=a
declare +x u=1 arr[]+=a
EOF
