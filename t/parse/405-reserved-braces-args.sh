f() {
    { echo }; }
    { echo a}; }
    case x in (esac) a ;; esac
    case x in a|esac) b ;; esac
    echo ]] [[ ! { } in do
}
declare -f
