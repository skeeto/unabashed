# read -u reads from a descriptor; bad descriptors are errors
printf '%s\n' l1 l2 l3 >/tmp/f
exec 4</tmp/f
read -u 4 a; read -u4 b
echo "a=$a b=$b"
read c <&4; echo "c=$c"
read -u 4 d; echo "status $? d=[$d]"
read -u 5 x; echo "status $?"
read -u abc x; echo "status $?"
read -u -1 x; echo "status $?"
read -u 99999 x; echo "status $?"
read -u "" x; echo "status $?"
exec 4<&-
read -u 4 x; echo "status $?"
exec 6>/tmp/w
read -u 6 x; echo "status $?"
