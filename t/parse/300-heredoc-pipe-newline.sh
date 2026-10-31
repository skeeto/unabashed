f() {
    cat <<A |
alpha
A
    grep a
    echo next
}
g() {
    cat <<A |
one
A

    wc -l <<B
two
B
}
declare -f
