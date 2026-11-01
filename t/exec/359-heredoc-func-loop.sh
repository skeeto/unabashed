# here-documents inside functions and loops expand at run time
greet() {
    while read l; do echo "$l"; done <<EOF2
hi $1, n=$n
EOF2
}
n=1; greet ann
n=2; greet bob
for i in a b c; do
    read v <<EOF2
item-$i
EOF2
    echo "$v"
done
declare -f greet
