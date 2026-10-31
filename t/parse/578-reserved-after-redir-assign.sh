f() {
    for x
    in a b
    do echo; done
    case x
    in x) ;;
    esac
    for x
    do :; done
    for x; do :; done
    echo $(for y
in c
do :; done)
}
declare -f
