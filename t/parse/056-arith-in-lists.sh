f() {
    (( a )) && (( b )) || ((c))
    ! (( a ))
    while (( i < 3 )); do (( i++ )); done
    if ((x)); then :; fi
}
declare -f
