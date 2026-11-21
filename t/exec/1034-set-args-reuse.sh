# Replaced positional parameters are freed: these loops run in a
# bounded store. A set in a ${ cmd; } inside an expansion of them, or
# in a loop over them, leaves what is being expanded intact.
p() { printf '<%s>' "$@"; echo; }
i=0; while ((i < 40000)); do set -- aaaa bbbb cccc; ((i++)); done; p "$@"
i=0; while ((i < 40000)); do set -- dd ee ff; shift; ((i++)); done; p "$@"
f() { local i=0; while ((i < 40000)); do set -- gg hh "$1"; ((i++)); done; p "$@"; }
f xx; p "$@"
g() { set -- "$@" z; }
i=0; while ((i < 40000)); do g a b; ((i++)); done; p "$@"
echo 'set -- in1 in2' >s.sh
i=0; while ((i < 20000)); do source ./s.sh a b; ((i++)); done; p "$@"
h() { local i=0; while ((i < 20000)); do source ./s.sh a b; ((i++)); done; p "$@"; }
h hh; p "$@"
set -- abcdef ghi
p "$@${ set -- zz; }" "${1#${ set -- yy; echo a; }}" "$@"
set -- a b c
for x; do set -- "$@" "$x$x"; done; p "$@"
set -- a b c
for x in "$@"; do shift; set -- "$@" "<$x>"; done; p "$@"
