f() {
    x=$(g() { cat <<E
x
E
}; g)
    y=$(h() { :; } <<E
y
E
)
    z=$(function k { cat; } <<E 2>&1
z
E
)
    cat <<<"$(cat <<E
w
E
)"
}
declare -f
