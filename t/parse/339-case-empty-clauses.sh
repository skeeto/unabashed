f() {
    case x in a) ;; b) ;& c) ;;& esac
    case x in a|b) esac
    case x in (a) esac
    case x in a) b;; c) esac
    case x in
        a)
    esac
}
declare -f
