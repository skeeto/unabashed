# ${#...} forms that mean $# with an operator
p() { printf '<%s>' "$@"; echo; }
set -- a b c
p "${#:-x}" "${#-x}" "${#+set}" "${#:+set}"
p "${#%3}" "${#/3/three}"
set --
p "${#:-x}" "${#-x}" "${#:+set}"
