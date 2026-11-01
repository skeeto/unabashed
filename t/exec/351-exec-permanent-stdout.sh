# exec with only redirections makes them permanent
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
exec 3>&1
exec >/tmp/f
echo to-file
echo also-to-file
exec >&3 3>&-
echo back
show /tmp/f
exec 2>/tmp/e
echo err1 >&2
echo nosuch </tmp/nosuch
exec 2>&1
show /tmp/e
echo status $?
exec 4>&1
{ exec >/tmp/x; echo in-group; } >/tmp/y
echo after-group
exec >&4 4>&-
echo "-- x"; show /tmp/x
echo "-- y"; show /tmp/y
f() { exec 5>/tmp/z; }
f
echo via-func >&5
exec 5>&-
show /tmp/z
