f() {
    select x; { echo $x; }
    select x in a b; { break; }
    select x
    do
        :
    done
}
declare -f
