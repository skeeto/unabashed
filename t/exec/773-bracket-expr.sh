# Bracket expressions: classes, ranges, negation with ! and ^, a ]
# or - taken literally, escapes, and incomplete brackets.
m() {
    local p=$1; shift
    printf '%-16s' "$p"
    for s; do
        case $s in
        $p) printf ' %s' "$s" ;;
        esac
    done
    echo
}
set -- a b z A Z 0 9 - ] '[' '!' '^' _ . ' ' '\' x
m '[abc]'          "$@"
m '[a-c]'          "$@"
m '[!a-y]'         "$@"
m '[^a-y]'         "$@"
m '[]a]'           "$@"
m '[!]a]'          "$@"
m '[a-]'           "$@"
m '[-z]'           "$@"
m '[[:alpha:]]'    "$@"
m '[[:digit:]_]'   "$@"
m '[[:punct:]]'    "$@"
m '[[:space:][:upper:]]' "$@"
m '[![:alnum:]]'   "$@"
m '[[:xdigit:]]'   "$@"
m '[\]]'           "$@"
m '[\!]'           "$@"
m '[a\-z]'         "$@"
m '[[]'            "$@"
m '[\]'           "$@"
m '[[.-.]]'        "$@"
m '[[=a=]]'        "$@"
m '[Z-a]'          "$@"
m '[a-a]'          "$@"
m '[z-a]'          "$@"
m '[!]'            "$@"
m '[a'             "$@"
m 'x[a'            x 'x[a' xa
m '[[:bogus:]]'    "$@"
m '[[:alpha:]'     "$@" '[:alpha:'
m '[[:alpha:]]x'   ax bx 1x
m '[ab][cd]'       ac bd ad ca
