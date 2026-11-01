# Backslashes in unquoted expansions used as patterns.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > 'a*'; : > ab; : > 'c\d'
x='a\*'
show $x
x='a\b'
show $x
x='c\d'
show $x
x='c\\d'
show $x
x='c\*'
show $x
x='\a*'
show $x
x='a\'
show $x
