f() {
    time a
    time a | b
    time -p a
    time ! a
    ! time a
    time { a; b; }
    time
    time -p
}
declare -f
