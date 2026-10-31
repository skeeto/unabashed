f() {
    ! a
    ! a | b
    ! ! a
    ! { a; }
    ! ( a )
    ! [[ x ]]
    a && ! b
}
declare -f
