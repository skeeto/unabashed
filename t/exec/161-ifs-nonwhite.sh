# Non-whitespace IFS characters each delimit a field: adjacent ones
# produce empty fields, a leading one produces an empty first field,
# but a trailing one does not produce an empty last field.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
IFS=:
x=a:b:c;   show $x
x=a::b;    show $x
x=:a;      show $x
x=a:;      show $x
x=a::;     show $x
x=::;      show $x
x=:;       show $x
x=' a : b '; show $x
x=;        show $x
show "$x"
x=a:b
show $x$x
show $x:$x
show "$x":$x
IFS=:,
x='a,b:c,,d'; show $x
