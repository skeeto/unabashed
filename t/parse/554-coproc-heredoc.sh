f() {
    coproc cat <<E
x
E
    coproc NAME { cat <<E; }
y
E
    coproc ( cat <<E
z
E
    )
    coproc W while read l; do echo $l; done <<E
w
E
}
declare -f
