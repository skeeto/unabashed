# redirections combined with pipelines of builtins and functions
both() { echo out; echo err >&2; }
both 2>&1 | while read l; do echo "piped $l"; done
both 2>/dev/null | while read l; do echo "only $l"; done
echo hidden >/tmp/f | while read l; do echo "never $l"; done
echo "status $?"
read l </tmp/f; echo "f=$l"
printf '%s\n' a b | { read x; read y; echo "$y$x"; } >/tmp/g
read l </tmp/g; echo "g=$l"
while read l; do echo "$l"; done </tmp/f | while read m; do echo "[$m]"; done
