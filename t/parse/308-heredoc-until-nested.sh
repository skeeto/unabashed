f() {
    for i in 1; do
        case $i in
            1)
                until cat <<U; do
u
U
                    :
                done
                ;;
        esac
    done
}
declare -f
