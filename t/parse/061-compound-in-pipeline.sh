f() {
    { a; } | ( b ) | while read; do :; done | if x; then y; fi
    for i in 1; do :; done && case x in *) ;; esac || [[ y ]]
}
declare -f
