f() {
    cat <<A &&
a
A
    echo ok ||
    cat <<B
b
B
    echo last
}
declare -f
