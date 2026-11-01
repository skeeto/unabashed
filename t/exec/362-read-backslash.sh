# backslash handling: escapes, continuation, -r
printf '%s\n' 'a\b c\ d' >/tmp/f
read x y </tmp/f; echo "x=[$x] y=[$y]"
read -r x y </tmp/f; echo "x=[$x] y=[$y]"
printf 'first \\nsecond\nthird\n' >/tmp/g
read x </tmp/g; echo "x=[$x]"
read -r x </tmp/g; echo "x=[$x]"
printf 'end\' >/tmp/h
read x </tmp/h; echo "status $? x=[$x]"
printf 'tail\\n' >/tmp/h
read x </tmp/h; echo "status $? x=[$x]"
printf '%s\n' '\double' >/tmp/k
read x </tmp/k; echo "x=[$x]"
printf '%s\n' 'esc\ aped rest' >/tmp/k
read x y </tmp/k; echo "x=[$x] y=[$y]"
printf '%s\n' '  \  lead' >/tmp/k
read x </tmp/k; echo "x=[$x]"
