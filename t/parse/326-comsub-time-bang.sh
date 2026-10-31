f() {
    a=$(time -p ! x)
    b=$(! time x | y)
    c=$(time)
    d=$(! { x; })
    e=$(time ( x ) 2>&1)
}
declare -f
