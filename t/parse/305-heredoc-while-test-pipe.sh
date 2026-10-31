f() {
    while cat <<A | grep x; do
x
A
        :
    done
    until a && cat <<B; do
b
B
        :
    done
}
declare -f
