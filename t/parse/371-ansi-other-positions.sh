f() {
    case $'a\tb' in $'x\ty') ;; $'\'') ;; esac
    [[ $x == $'\t' && $'\n' != y ]]
    for i in $'a b' $"c"; do :; done
    a > $'o\tf' 2>> $"e"
    x=$'v\tw' y[$'k']=$'z'
}
declare -f
