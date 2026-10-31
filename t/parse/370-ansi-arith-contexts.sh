f() {
    echo $(( $'1' + x )) "$(( $'2' ))" $[ $'3' ] "$(echo $'\t')" "`echo $'\t'`"
    (( y = $'4' ))
    for (( i = $'5'; i < 2; i++ )); do :; done
}
declare -f
