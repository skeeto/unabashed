# quoting in operator words inside double quotes
p() { printf '<%s>' "$@"; echo; }
unset u; s=val
p "${u:-'a b'}"
p "${u:-"a b"}"
p "${u:-\"q\"}"
p "${u:-\$s}" "${u:-\\}" "${u:-\a}"
p "${u:-\}}" "${u:-a\}b}"
p "${u:-'$s'}"
p "${s:+'$s'}"
p "${u:-$'tab\tx'}"
p "${u:-"$s"'x'}"
p "${u:-a\\\\b}" "${u:-\`}"
