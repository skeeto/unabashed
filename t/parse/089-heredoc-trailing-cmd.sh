f() {
    cat <<EOF; echo after
body
EOF
    cat <<EOF | cat; echo two
x
EOF
}
declare -f
