f() {
    case $1 in
        a) echo a ;&
        b) echo b ;;&
        c) echo c ;;
        d) echo d
    esac
}
declare -f
