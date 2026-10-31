f() {
    [[ a &&
       b ||
       c ]]
    [[ -n $x
    ]]
}
declare -f
