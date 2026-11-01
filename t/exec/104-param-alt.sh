# ${x+w} and ${x:+w}
p() { printf '<%s>' "$@"; echo; }
unset u; e=; s=val
p "${u+a}" "${u:+a}" "${e+a}" "${e:+a}" "${s+a}" "${s:+a}"
p ${u+a b} ${s+a b} ${s:+"a b"} "${s:+a b}"
p ${e:+x} ${e+} ${s:+}
p "${s:+$s-$s}" "${s+${u-inner}}"
set -- one two
p ${1+"$@"} ${3+"$@"}
p "${1+$@}"
