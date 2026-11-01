# Quoted and unquoted parts of one word join; only the unquoted
# expansion parts split.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
x='1 2'
show "$x"$x
show $x"$x"
show '$x'$x
show a"b"'c'd
show "a b"$x'c d'
show ''$x''
show "$x""$x"
show "${x}"x${x}
e=
show "$e"$x"$e"
show $e$x$e
IFS=-
y=a-b
show "$y"-$y-"$y"
