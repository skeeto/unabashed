f() {
    while a; do b; done
    until a; do b; done
    while a; b; do c; d; done
    while
        a
    do
        b
    done
}
declare -f
