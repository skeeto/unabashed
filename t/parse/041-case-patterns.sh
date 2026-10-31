f() {
    case $x in
        *.c | *.h) : ;;
        [abc]*) : ;;
        ?) : ;;
        'quoted*') : ;;
        "dq $y") : ;;
        \*) : ;;
        $var) : ;;
        "") : ;;
        in|esac|do) : ;;
    esac
}
declare -f
