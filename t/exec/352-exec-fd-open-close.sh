# exec n>file, exec n<file, exec n>&-
exec 3>/tmp/f
echo line1 >&3
echo line2 >&3
exec 3>&-
echo after close >&3
echo status $?
exec 3</tmp/f
read -u 3 a
read b <&3
echo "a=$a b=$b"
read -u 3 c
echo status $? "c=$c"
exec 3<&-
read -u 3 d
echo status $?
exec 9>>/tmp/f
echo line3 >&9
exec 9>&-
while read l; do echo "> $l"; done </tmp/f
