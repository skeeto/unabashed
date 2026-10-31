f() {
    time -p -- a b
    time -- a
    time -p
    time -p ! a
    time -p -- -p
    a | time -p b
}
declare -f
