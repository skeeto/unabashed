f() {
    cat $(a;b) <<A "$(c
d)"
x
A
    echo $(cat <<B) tail
b
B
}
declare -f
