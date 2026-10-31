f() {
    a && ! { b; }
    time { a; } | b
    ! ( a ) || { b; } &
    { a; } & { b; }
}
declare -f
