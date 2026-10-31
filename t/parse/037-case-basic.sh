f() {
    case $1 in
        a) echo a ;;
        b|c) echo bc ;;
        *) echo other ;;
    esac
}
declare -f
