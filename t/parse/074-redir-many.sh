f() {
    a 3>&1 1>&2 2>&3 3>&-
    exec 4<> rw 5>&4 6<&4
}
declare -f
