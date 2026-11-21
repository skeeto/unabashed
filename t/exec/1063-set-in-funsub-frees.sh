# set in a ${ cmd; } frees the parameters it replaces once the expansion
# that ran it is done, so these loops run in a bounded store, and what
# the expansion was reading stays intact until then.
p() { printf '<%s>' "$@"; echo; }
i=0; while ((i < 40000)); do x=${ set -- aaaa bbbb cccc; }; ((i++)); done; p "$@"
f() { local i=0; while ((i < 40000)); do x=${ set -- dd "$1" ee; }; ((i++)); done; p "$@"; }
f q; p "$@"
g() { y=${ local i=0; while ((i < 20000)); do set -- ff gg; ((i++)); done; }; }
for k in 1 2 3; do g; done; p "$@"
set -- one two three
p "$@" "${ set -- four five; }" "$@" "${@/o/${ set -- six; echo 0; }}" "$@"
set -- abc def
p "${@:1:${ set -- g h i j; echo 3; }}" "${*%${ set -- k; echo [a-z]; }}" "$1"
set -- abc def
x=${ for a; do set -- "$@" "${ set -- z; }$a"; done; }; p "$@"
set -- abc def
p "${ set -- x${ set -- y; echo "$1"; }; echo "$1"; }" "$@"
set -- abc def
p "$1${ set -- uvw; }$1" "${2-${ set -- r s t; echo none; }}" "$#"
