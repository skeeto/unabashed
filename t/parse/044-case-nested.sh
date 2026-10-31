f() {
    case $1 in
        a) case $2 in
               x) echo ax ;;
           esac ;;
        b) for i in 1; do case $i in 1) ;; esac; done ;;
    esac
}
declare -f
