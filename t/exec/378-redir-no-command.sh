# redirections without a command name
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
>/tmp/f
echo status $?
read l </tmp/f; echo "read status $?"
echo data >/tmp/g
>/tmp/g
read l </tmp/g; echo "truncated status $?"
x=1 >/tmp/h
echo "x=$x"
</tmp/nosuch
echo status $?
y=2 </tmp/nosuch
echo "status $? y=$y"
>$empty
echo status $?
z=$(echo sub) >/tmp/k
echo "z=$z"
2>/tmp/e
echo status $?
