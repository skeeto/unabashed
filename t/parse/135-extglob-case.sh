shopt -s extglob
f() {
    case $x in
        @(a|b)) ;;
        !(c)) ;;
        +([0-9])) ;;
        *(x)?(y)) ;;
        @(*.c|*.h)) ;;
    esac
}
declare -f
