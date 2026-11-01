# Brace expansion of assignment-like words: prefix assignments are not
# brace expanded; declaration builtin arguments are.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
a={x,y}
show "$a"
declare b={x,y}
show "$b"
export c=p{1,2}q
show "$c"
d={1..3} show "$d"
show "$d"
show e={m,n}
