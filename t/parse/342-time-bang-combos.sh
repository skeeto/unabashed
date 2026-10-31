f() {
    time -p ! a | b
    ! time -p { a; }
    time ( a ) > out
    time -p -- a
    time -- b
    time -- -p c
    ! time -- d
    time --; time -p --
    time ! ! a
    ! ! time a
    time while a; do b; done
    if time; then ! ; fi
}
declare -f
