f() {
    cat <<A | tr a b <<B | cat - <<C
a $(cat <<D
d1 $(cat <<E
e1
E
)
D
) a2
A
b1 `cat <<F
f1
F
`
B
c1
C
    echo done
}
declare -f
