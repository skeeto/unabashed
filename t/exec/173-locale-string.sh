# $"..." is a double-quoted string translated through the locale; in
# the C locale it is unchanged.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
x='a b'
show $"plain"
show $"$x"
show $"\$x"
show $"a"b$"c"
show $""
show "$"x
show "$"
show $
show a$
y=$"in $x"
show "$y"
