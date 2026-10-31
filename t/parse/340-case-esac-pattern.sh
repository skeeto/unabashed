f() {
    case x in (esac|a) ;; esac
    case x in a) ;; (esac) b;; esac
    case x in "esac") ;; esac
    case esac in (esac) ;; esac
    case x in (esac
    ) c;; esac
}
declare -f
