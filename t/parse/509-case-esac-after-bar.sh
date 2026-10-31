f() {
    case x in
        a) ;;
        b|esac) echo y;;
        (c|esac) echo z;;
    esac
}
declare -f
