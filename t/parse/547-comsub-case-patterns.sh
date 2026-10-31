f() {
    echo $(case x in (a|b) echo ab;; c|d) echo cd;; (*\)*) echo paren;; esac)
    echo $(case x in
        ( a | b ) echo 1 ;;
        x|y) echo 2 ;&
        ")") echo 3 ;;&
        *) echo 4
    esac)
    echo "$(case $x in (a) echo "(";; esac)"
    echo $(case x in (esac) echo e;; in) echo i;; esac)
}
declare -f
