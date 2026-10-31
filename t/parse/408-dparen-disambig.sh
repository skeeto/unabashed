f() {
    (( (a) ))
    ((a) )
    ( (a))
    ((echo a); echo b)
    (( a ) )
    ((a)|b)
}
declare -f
