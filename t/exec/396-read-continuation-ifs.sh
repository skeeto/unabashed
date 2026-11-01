# line continuation inside fields, and multiple lines joined
printf 'a b\\nc d\n' >/tmp/f
read x y z </tmp/f; echo "x=[$x] y=[$y] z=[$z]"
printf 'one\\n\\ntwo\n' >/tmp/g
read x </tmp/g; echo "x=[$x]"
printf 'p\\n' >/tmp/h
read x </tmp/h; echo "status $? x=[$x]"
printf 'q \\n r\n' >/tmp/k
read x y </tmp/k; echo "x=[$x] y=[$y]"
