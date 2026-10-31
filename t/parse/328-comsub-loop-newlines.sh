f() {
    a=$(for i in a; do b
c; done)
    b=$(for ((;;)); do
x
done)
    c=$(select s; do
x; y
z
done)
}
declare -f
