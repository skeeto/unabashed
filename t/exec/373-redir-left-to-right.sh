# redirections apply left to right; earlier targets are still created
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
echo payload >/tmp/a >/tmp/b
echo "-- a"; show /tmp/a
echo "-- b"; show /tmp/b
echo x >/tmp/c </tmp/nosuch >/tmp/d
echo status $?
read l </tmp/c; echo "c status $? [$l]"
read l </tmp/d; echo "d status $?"
echo v >/tmp/e 2>&1 >/tmp/f
show /tmp/f
exec 3>/tmp/g 4>&3
echo via3 >&3
echo via4 >&4
exec 3>&- 4>&-
show /tmp/g
