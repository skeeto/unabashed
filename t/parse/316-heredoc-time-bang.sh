f() {
    time cat <<A | wc
a
A
    ! cat <<B
b
B
    time -p ! { cat <<C; }
c
C
    echo end
}
declare -f
