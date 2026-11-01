# Repeated slashes in patterns are kept as written in the results.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > r1; : > r2
show //tmp/r*
show ///tmp/r1*
show /tmp//r*
show /tmp///r1
show .//r*
show ./r?
show /tmp/./r1*
show //h*/oracle
show /home//*/
