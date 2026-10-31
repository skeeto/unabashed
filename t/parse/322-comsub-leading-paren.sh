f() {
    a=$( (x) | y)
    b=$( ( x ); y)
    c=$( (x) && y )
    d=<( (x) )
    e=$( ((x)) )
    echo >( ( y ) ) "$( (q) )"
}
declare -f
