f() {
    if \
        a; \
    then b; fi
    for i \
        in 1 2; do :; done
    x=1\
2
}
declare -f
