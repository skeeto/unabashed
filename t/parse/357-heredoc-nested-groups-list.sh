f() {
    { { cat <<A; } | b; } && c
a
A
    x; { y; cat <<B; }
b
B
    z
}
declare -f
