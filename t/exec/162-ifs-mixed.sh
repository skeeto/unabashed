# IFS with both whitespace and non-whitespace: whitespace around a
# non-whitespace delimiter is absorbed into it.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
IFS=' :'
x='a : b';      show $x
x='a  ::  b';   show $x
x=' : a';       show $x
x='a : ';       show $x
x=' :: ';       show $x
x='a b:c';      show $x
x=':  :';       show $x
IFS=$' \t:'
x=$'a\t:\tb c';  show $x
IFS=': '
x='a: :b';      show $x
