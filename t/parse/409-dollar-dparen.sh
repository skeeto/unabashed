f() {
    echo $( (a) ) $((1+2)) $( (b); c ) $((c) ) "$( (d) )"
    echo $((echo  x);(y)) "$((e)  |  f)"
}
declare -f
