# exec redirections in a subshell do not leak to the parent
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
( exec >/tmp/f; echo inner1; echo inner2 )
echo outer
show /tmp/f
( exec 3>/tmp/g; echo three >&3 )
echo leak >&3
echo status $?
show /tmp/g
x=$(exec 2>&1; echo err >&2)
echo "x=$x"
( exec </tmp/f; read a; echo "sub read $a" )
read b </tmp/f; echo "parent $b"
