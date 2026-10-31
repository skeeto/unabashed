f() {
    case $x in
        (a) echo a ;;
        (b | c) echo bc ;;
        ( d ) ;;
    esac
}
declare -f
