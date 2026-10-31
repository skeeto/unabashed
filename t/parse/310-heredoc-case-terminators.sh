f() {
    case $x in
        a) cat <<A ;&
a
A
        b) cat <<B | wc ;;&
b
B
        c) cat <<C &
c
C
        ;;
    esac
}
declare -f
