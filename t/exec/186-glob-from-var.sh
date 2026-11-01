# Glob characters from unquoted expansions are active; from quoted
# expansions they are literal. Split happens before globbing.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > p1; : > p2; : > 'p *'
x='p?'
show $x
show "$x"
x='p*'
show $x
show "$x"
x='p* q'
show $x
x='p [12]'
show $x
IFS=:
x='p1:p?'
show $x
unset IFS
x=p
show $x?
show "$x"?
show ${x}*
show "${x}*"
show $(echo 'p?')
show "$(echo 'p?')"
y='[p]1'
show $y
