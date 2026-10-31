f() {
    while a; do
        until b; do
            while c; do break 2; done
            continue
        done
    done
}
declare -f
