f() {
    echo $(( $(echo ")") ))
    echo $(( `echo ")"` ))
    (( $(echo ")") ))
}
declare -f
