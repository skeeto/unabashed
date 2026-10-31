f() {
    for i in 1 2; { echo $i; }
    for i; { echo $i; }
}
declare -f
