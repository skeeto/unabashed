f() {
    if a & then b & c & fi
    while a & do b; done
    for i in 1; do a & done
    until a; do { b & } done
    case x in x) a & b & ;; esac
    ( a & ) &
    echo a\& ; echo b
}
declare -f
