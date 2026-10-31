f() {
    if a; then cat <<A; fi | b
a
A
    while read l; do :; done <<B | cat
b
B
    echo end
}
declare -f
