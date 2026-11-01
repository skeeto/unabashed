# <> opens for reading and writing without truncating
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
printf '%s\n' 12345 abcde >/tmp/f
exec 3<>/tmp/f
read -u 3 first
echo "first=$first"
echo XY >&3
exec 3>&-
show /tmp/f
echo new <>/tmp/g
show /tmp/g
printf 'hello\n' >/tmp/h
echo HI 1<>/tmp/h
show /tmp/h
read line <>/tmp/h
echo "line=$line"
echo x <>/tmp/nodir/y
echo status $?
