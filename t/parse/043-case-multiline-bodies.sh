f() {
    case $1 in
        a)
            echo one
            echo two
            ;;
        b)
            if x; then y; fi
            ;&
        *)
            ;;
    esac
}
declare -f
