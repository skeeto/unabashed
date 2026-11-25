# The read builtin on a directory reports the error the read gives,
# "Is a directory", in every mode; read -t 0 finds it ready, as
# bash's select does. A descriptor not open for reading is still a bad
# file descriptor.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c"
    echo "status $?"
done <<'EOF'
read x <.; echo $? "<$x>"
read -r -n 3 x <.; echo $?
read -d '' x </; echo $?
read -t 1 x <.; echo $?
read -t 0 x <.; echo $?
exec 3<.; read -u 3 x; echo $?
while read l; do :; done <.; echo $?
read -a arr <.; echo $?
read -N 1 x <.; echo $?
read x 0>/dev/null; echo $?
mapfile a <.; echo $? ${#a[@]}
EOF
