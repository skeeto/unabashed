# Bracket expressions: ranges, negation, ] first, - first/last.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
for f in a b c d - ']' '!' x1 y1 z1; do : > "$f"; done
show [a-c]
show [!a-c]
show []a]
show [!]a]
show [-a]
show [a-]
show [c-a]
show [x-z]1
show [!!]
show [\!]
show [\]]
show [a\-c]
show [[]
show [*]
show [?]1
