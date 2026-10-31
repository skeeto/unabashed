f() {
    echo $'\cA\cz\c[' $'\c?' $'a\c@b' $'x\c' $'\c\\y' $'\c`z' $'\c1'
}
declare -f
