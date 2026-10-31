f() {
    coproc a && b
    coproc N { a; } 2>&1 | b
    coproc a & coproc b
    ! coproc c
    time coproc d
    coproc NAME cmd arg
}
declare -f
