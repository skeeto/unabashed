f() {
    for i in 1 2; do
        while a; do
            case $i in
                1) if b; then
                       { c; ( d | e ) && [[ x ]]; }
                   else
                       until g; do (( n++ )); done
                   fi ;;
            esac
        done
    done
}
declare -f
