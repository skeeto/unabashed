f() {
    a <<A | b <<B | c <<C
1
A
2
B
3
C
}
g() {
    a <<A | b <<B | c <<C; echo z
1
A
2
B
3
C
}
declare -f
