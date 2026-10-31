f() {
    exec {fd}> file
    exec {fd}< file
    exec {fd}>&-
    exec {fd}>&2
    a {x}>> app
    a {y}<> rw
    exec {fd}<&-
}
declare -f
