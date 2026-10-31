f() {
    while read l; do
        echo $l
    done <<EOF
a
b
EOF
    for i in 1 2; do
        cat <<END
iter $i
END
    done
}
declare -f
