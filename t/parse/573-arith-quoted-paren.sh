f() {
    echo $(( ')' + ")" ))
    echo $(( ${x:-')'} ))
}
declare -f
