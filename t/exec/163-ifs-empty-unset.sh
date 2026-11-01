# An empty IFS disables splitting; an unset IFS behaves as the default.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
x=' a  b:c '
IFS=
show $x
show $x$x
y=
show $y
show $y $y "$y"
unset IFS
show $x
IFS=' '
x=$'a\tb c'
show $x
IFS=$'\t'
show $x
x=$'\t\ta\t\tb\t'
show $x
