# n>&m- moves a descriptor
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
exec 4>/tmp/f
echo one >&4
exec 5>&4-
echo two >&5
echo three >&4
echo status $?
exec 5>&-
show /tmp/f
echo line1 >/tmp/g
echo line2 >>/tmp/g
exec 6</tmp/g
exec 7<&6-
read -u 7 a
echo "a=$a"
read -u 6 b
echo status $? "b=$b"
exec 7<&-
echo moved 3>&1- 
echo status $?
