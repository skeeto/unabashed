f() {
    > out a b
    a > out b
    2>&1 > out a
    < in
    > out
    x=1 > out
    > out x=1 a
    a <in >out 2>err
}
declare -f
