f() {
    case $x in
        a) cat <<A; echo after;;
a
A
        b) echo b; cat <<B ;;
b
B
        c) echo c ;;
    esac
}
declare -f
