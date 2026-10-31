f() {
    { cat <<A
a
A
    }
    echo one
    echo two
    ( cat <<B
b
B
    )
    echo three
}
declare -f
