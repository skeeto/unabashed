# quoting in operator words outside double quotes
p() { printf '<%s>' "$@"; echo; }
unset u; s=val
p ${u:-'a b'}
p ${u:-"a b"}
p ${u:-\"q\"}
p ${u:-\$s} ${u:-\\} ${u:-\a}
p ${u:-\}} ${u:-a\}b}
p ${u:-'$s'}
p ${u:-$'tab\tx'}
p ${u:-""} ${u:-''} ${u:-"$u"}
p ${u:-'}'}
p ${u:-"}"}
p ${u:-a\\\\b} ${u:-"\\"}
