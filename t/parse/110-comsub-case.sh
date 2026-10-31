f() {
    x=$(case $1 in a) echo a ;; b|c) echo bc ;; esac)
    y=$(case $1 in (a) echo a ;; esac)
    z="$(case x in x) echo ')' ;; esac)"
}
declare -f
