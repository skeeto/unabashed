# braces and odd characters inside operator words
p() { printf '<%s>' "$@"; echo; }
unset u; s=v
p ${u:-{a,b}}
p "${u:-{}}" ${u:-x}y}
p "${u:-'}'}" "${u:-"}"}" "${u:-\}}"
p "${u:-${s}}" "${u:-$s}}"
p "${u:-a#b}" "${u:-a%b}" "${u:-a/b}"
p "${u:-:}" "${u-:-}" "${u:-?}" "${u:-=}"
p "${s:+a{b}c}"
