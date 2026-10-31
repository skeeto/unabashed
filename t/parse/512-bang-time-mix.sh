f() {
    time ! time a
    ! time ! a
    ! time -p a | b
    a && ! time b
}
declare -f
