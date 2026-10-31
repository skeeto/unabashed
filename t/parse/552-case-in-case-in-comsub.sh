f() {
    echo $(case a in a) case b in (b) echo $(case c in c) echo deep;; esac);; esac;; esac)
    x=$(case a in
        a)
            case b in
                b|c) echo one ;;
                (d) case e in e) echo two;; esac ;;
            esac
            ;;
    esac)
}
declare -f
