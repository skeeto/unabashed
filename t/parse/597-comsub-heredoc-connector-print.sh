f() {
    x=$(cat <<E | b; c
x
E
)
    x=$(cat <<E; cat <<F
x
E
y
F
)
    x=$(while a; do cat <<E
x
E
    done; b)
    x=$( { cat <<E; } | cat
x
E
)
    x=$(g() { a; b
    c; }; h)
}
declare -f
# nofixedpoint: bash drops the ; after a here-document in a comsub
# (done b, b c) and its output is not a fixed point under reparse
