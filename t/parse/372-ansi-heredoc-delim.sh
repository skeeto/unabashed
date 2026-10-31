f() {
    cat <<$'E\x41'
body $x
EA
    cat <<$"Q"
two
Q
}
declare -f
