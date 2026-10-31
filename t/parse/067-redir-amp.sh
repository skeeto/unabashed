f() {
    a &> out
    a &>> out
    a >& out
    a >&out
    a >& 1
}
declare -f
