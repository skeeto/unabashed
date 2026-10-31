f() {
    cat <(cat <<E
x
E
) >(tr a b <<F
f
F
)
    diff <(cat <<A) <(cat <<B)
a
A
b
B
    cat < <(cat <<C
c
C
) <<D
d
D
}
declare -f
