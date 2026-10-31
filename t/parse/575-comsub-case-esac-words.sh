f() {
    echo $(case x in x) echo esac;; esac)
    echo $(case x in x) esac)
    echo $(case x in x);; esac)
    echo $(case x in x) echo in esac
    esac)
    case "$(echo ")")" in $(echo b)|"$(echo ")")") echo c;; esac
    echo $(case x in x) cat <<E
body
E
;; esac)
}
declare -f
