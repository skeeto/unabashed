# function definitions with redirections apply at each call
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
f() { echo "f says $1"; } >>/tmp/f
f one
f two
show /tmp/f
out=/tmp/g
g() { echo "g says $1"; } >$out
g a
out=/tmp/h
g b
show /tmp/g
show /tmp/h
h() { echo "h $1"; echo "h err" >&2; } 2>&1
h x >/tmp/k
show /tmp/k
declare -f f g
r() { read v; echo "read $v"; } </tmp/f
r
