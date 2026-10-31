f() {
    coproc cat <<A
a
A
    coproc C { cat; } <<B
b
B
    coproc ( cat <<D ); echo after
d
D
}
declare -f
