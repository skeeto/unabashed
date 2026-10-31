f() {
    select x in a b c; do echo $x; break; done
    select x; do :; done
    select x in
    do :
    done
}
declare -f
