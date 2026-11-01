# Patterns starting at the root and with dot components.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > r1
show /*/oracle
show /h*/o*/
show /tmp/./r?
show /tmp/../tmp/r*
show //tmp/r1
show //tmp/r*
show /tmp/r[1]
show /tm[p]/r1
show /tmp/*/
