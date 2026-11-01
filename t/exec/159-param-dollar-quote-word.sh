# $'...' and $"..." in operator words and patterns
p() { printf '<%s>' "$@"; echo; }
unset u; x=$'a\tb'
p ${u:-$'x\ty'} "${u:-$'x\ty'}"
p "${x/$'\t'/T}" ${x/$'\t'/T} "${x#a$'\t'}"
p ${u:-$"msg"} "${u:-$"msg"}"
p ${u:-a$'\n'b}
