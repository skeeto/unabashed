f() {
    echo $(( $(echo 1) + ${#x} + ${a[$(echo 0)]} ))
    (( x = $(echo 2) * ${y:-3} ))
    for (( i=$(echo 0); i<${n:-3}; i++ )); do :; done
    echo $(( $( (echo 4) ) ))
    echo $(( `echo 5` + $((6)) ))
    a[$(echo 1)+1]=$(( $(cat <<E
7
E
) ))
    echo $[ $(echo 8) + 1 ]
}
declare -f
