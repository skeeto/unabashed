f() {
    cat <<A
a
A
    case x in x) y;; esac
    cat <<B
b
B
    for i in 1; do :; done
    cat <<C
c
C
    [[ x ]]
    cat <<D
d
D
    coproc cat
}
declare -f
