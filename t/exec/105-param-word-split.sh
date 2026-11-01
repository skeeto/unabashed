# word splitting of unquoted operator words
p() { printf '<%s>' "$@"; echo; }
unset u; v='1 2'
p ${u:-a b}
p ${u:-a "b c" d}
p ${u:-$v}
p ${u:-"$v"}
p ${u:-'x  y'}
p ${u:-a\ b}
IFS=:
p ${u:-a:b}
p ${u:-$v}
w=c:d
p ${u:-$w}
p ${u:-"$w"}
p ${u:+x:y} ${w:+p:q}
