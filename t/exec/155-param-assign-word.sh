# words assigned by ${x=w}: quoting and splitting
p() { printf '<%s>' "$@"; echo; }
set -- a 'b c'
unset v; p ${v="x y"}; p "$v"
unset v; p "${v="x y"}"; p "$v"
unset v; p "${v=$@}"; p "$v"
unset v; IFS=:; p ${v=$*}; p "$v"; unset IFS
unset v; p "${v='q'}"; p "$v"
unset v; p ${v='q'}; p "$v"
unset v; p "${v:=\$x}"; p "$v"
