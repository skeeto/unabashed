cplx() {
    case $1 in
        a|b) echo ab ;;
        *) echo other ;;
    esac
    (( x = $1 + 1 ))
    [[ -n $x && $x != 0 ]] && echo nonzero
    for ((i = 0; i < 2; i++)); do echo $i; done
    until false; do break; done
    { echo grouped; } 2>&1
    local v="${1:-def}" w=$(echo sub)
}
declare -f cplx
