f() {
    ( cat <<A ) | b
a
A
    { cat <<B; } && c
b
B
    ( { cat <<C; } ) || d
c
C
    echo end
}
declare -f
