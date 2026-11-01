# -r with -d, and backslash before the delimiter
printf 'a\,b,c' >/tmp/f
read -d , x </tmp/f; echo "x=[$x]"
read -r -d , x </tmp/f; echo "x=[$x]"
printf 'p\\nq;' >/tmp/g
read -d ';' x </tmp/g; echo "x=[$x]"
read -rd ';' x </tmp/g; echo "x=[$x]"
printf 'line\nnext' >/tmp/h
read -d '' x </tmp/h; echo "status $? x=[$x]"
read -r -d '' a b </tmp/h; echo "a=[$a] b=[$b]"
