# undocumented ~ and ~~ case toggling
p() { printf '<%s>' "$@"; echo; }
x='Hello World'
p "${x~}" "${x~~}"
p "${x~~[lo]}"
