f() {
    if a; then b; fi |& c
    ( a ) 2>x |& b
    while a; do b; done |& c |& d
    a <<A |& b
x
A
}
declare -f
