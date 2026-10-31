f() {
    if true; then
        while read a; do
            for i in 1 2; do
                until false; do
                    { echo $i; } >&2 2>/dev/null
                    case $i in
                        1) ( echo one ) >>log ;;
                    esac 3<in
                    select s in a; do break; done <&0
                done 4>out
            done <<E
heredoc
E
        done < input 2>&1
    elif false; then
        [[ -n x ]] >/dev/null; (( 1 )) 2>/dev/null
    else
        f2() { echo; } >f2out
    fi >outer 2>>err <<<"str"
}
declare -f
