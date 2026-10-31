f() {
    diff <(a) <(b)
    tee >(c) >(d | e) > /dev/null
    cat < <(cmd)
    a > >(b)
    echo <( { x; y; } ) >( (z) )
    while read l; do :; done < <(gen)
}
declare -f
