f() {
    cat <<E; a=(1
body
E
2)
    echo next
}
declare -f
