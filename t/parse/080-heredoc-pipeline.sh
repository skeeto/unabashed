f() {
    cat <<A | sed s/a/b/ | tr a b <<B
apple
A
banana
B
    cat <<EOF | while read l; do echo $l; done
one
EOF
}
declare -f
