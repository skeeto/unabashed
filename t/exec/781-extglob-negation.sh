# !(...) matches anything its alternatives do not, including the empty
# string, and composes with what follows it.
shopt -s extglob
m() {
    local p=$1; shift
    printf '%-14s' "$p"
    for s; do
        case $s in
        $p) printf ' %s' "${s:-''}" ;;
        esac
    done
    echo
}
set -- '' a b aa ab abc ba bb foo foo.c foo.h .c c
m '!()'          "$@"
m '!(*)'         "$@"
m '!(a)*'        "$@"
m '!(a)b'        "$@"
m 'a!(b)'        "$@"
m '!(a|b)'       "$@"
m '!(?)'         "$@"
m '!(??)'        "$@"
m '!(*.c|*.h)'   "$@"
m 'foo!(.c)'     "$@"
m '!(foo).c'     "$@"
m '!(!(a|b))'    "$@"
m '*!(b)'        "$@"
m '!(b)*'        "$@"
m '@(!(a)|a)'    "$@"
m '!(*(a))'      "$@"
m '!(+(a|b))'    "$@"
[[ abc == !(*b*) ]] && echo bad || echo neg ok
[[ xyz == !(*b*) ]] && echo neg ok2
