# read -d sets the delimiter; -d '' means NUL
printf 'a,b,c' >/tmp/f
read -d , x </tmp/f; echo "status $? x=$x"
while read -d , x; do echo "got $x"; done </tmp/f
echo "left x=$x"
printf 'one\0two\0' >/tmp/g
while read -r -d '' x; do echo "nul $x"; done </tmp/g
printf 'line1\nline2;rest' >/tmp/h
read -d ';' x </tmp/h; echo "x=[$x]"
read -d ';' a b </tmp/h; echo "a=[$a] b=[$b]"
read -dX x <<<"abcXdef"; echo "x=$x"
read -d xyz x <<<"abxcd"; echo "x=$x"
