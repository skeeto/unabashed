# Pathname expansion with *, ? and bracket expressions, sorted.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
for f in a1 a2 a10 b1 B2 ab abc _x 9z; do : > "$f"; done
show *
show a*
show a?
show ??
show ?
show *1
show a*c
show [ab]1
show [a-b]?
show [!a]*
show [^a]*
show *[0-9]
show [B]*
show a[12]
show /tmp/a?
show ./b*
show *z *x
