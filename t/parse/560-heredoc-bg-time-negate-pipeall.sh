f() {
    cat <<E &
a
E
    cat <<E & cat <<F
b
E
c
F
    time cat <<E | cat
d
E
    ! cat <<E |& cat
e
E
    time -p ! cat <<E
f
E
    cat <<E || cat <<F && cat <<G
g
E
h
F
i
G
}
declare -f
