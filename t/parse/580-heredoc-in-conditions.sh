f() {
    if cat <<E; then
x
E
        :
    fi
    while cat <<E
y
E
    do :; done
    until cat <<E; false
z
E
    do :; done
    case x in x) ;; esac <<E
w
E
    select s in a; do break; done <<E
v
E
    [[ -n $(cat <<E
u
E
) ]]
    (( $(cat <<E
1
E
) ))
}
declare -f
