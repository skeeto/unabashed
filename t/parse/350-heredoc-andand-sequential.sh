f() {
    cat <<A &&
a
A
    cat <<B &&
b
B
    cat <<C
c
C
}
declare -f
