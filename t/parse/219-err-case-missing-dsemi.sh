f() {
    for x in a b; do
        echo $x
    done
}
g() {
    case $1 in
        a) echo a ;;
        b) echo b
        c) echo c ;;
    esac
}
