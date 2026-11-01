# ${x=w} and ${x:=w} assign and expand
p() { printf '<%s>' "$@"; echo; }
unset a b; c=
p "${a=one}" "$a"
p "${a=two}" "$a"
p "${b:=three}" "$b"
p "${c=four}" "[$c]"
p "${c:=five}" "$c"
unset d; : ${d:=x y z}; p "$d"
unset e; p ${e=1 2}; p "$e"
unset f; p "${f=}" "${f-unset}"
unset g; g2=${g:=inner}; p "$g" "$g2"
