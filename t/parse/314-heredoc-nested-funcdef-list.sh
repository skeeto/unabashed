f() {
    g() { cat; } <<A; echo x
a
A
    h() { cat; } <<B && echo y
b
B
    k() { cat <<C; }; echo z
c
C
}
declare -f
