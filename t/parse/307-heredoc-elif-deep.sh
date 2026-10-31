f() {
    while a; do
        if b; then
            :
        elif cat <<A; then
a
A
            c
        elif cat <<B
b
B
        then
            d
        fi
    done
}
declare -f
