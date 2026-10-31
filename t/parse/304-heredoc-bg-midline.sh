f() {
    cat <<A & echo x; echo y
a
A
    cat <<A & cat <<B &
a
A
b
B
    echo end
}
declare -f
