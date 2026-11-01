# Literal text in an unquoted ${...} operator word is split with IFS,
# like the value of an expansion.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show ${u:-a b}
show ${u:=a  b}
show "$u"
show ${u+p q}
show ${u:+$u r}
IFS=:
show ${w-a:b::c}
show ${w-"a:b"c:d}
show "${w-a:b}"
w=1:2
show ${w+$w:3}
unset IFS
show ${w-x}${w+ y z}
show x${n-}y
show ${n-'a b'}' c d'
show ${n-a\ b}
show ${n-$'a b'}
