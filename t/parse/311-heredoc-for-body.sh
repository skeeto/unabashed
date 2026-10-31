f() {
    for i in a b; do
        cat <<A
$i
A
    done
    for ((i=0; i<2; i++)); do
        cat <<A; echo $i
x
A
    done
    select s in a; do cat <<S; done
s
S
}
declare -f
