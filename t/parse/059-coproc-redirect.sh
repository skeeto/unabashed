f() {
    coproc cmd 2>&1
    coproc C { cmd; } > out
    coproc cmd | other
}
declare -f
