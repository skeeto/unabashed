# layout variations of case
case a in a) echo one ;; esac
case a
in
a)
echo two
;;
esac
case a in
    # comment before item
    a) # comment after pattern
        echo three ;; # comment after ;;
esac
case a in (a|b) echo four;; (c) echo no;; esac
case a in
    b | a ) echo five ;;
esac
case in in in) echo "word in";; esac
case esac in esac) echo "word esac";; esac
case a in a) echo six; esac; echo "after six"
case a in a) { echo seven; } ;; esac
case a in a) if true; then echo eight; fi;; esac
case a in a) case b in b) echo nine;; esac;; esac
