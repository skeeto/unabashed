f() { cat; } <<A >out <<B
a
A
b
B
g() { cat <<C; } <<D
c
C
d
D
declare -f
