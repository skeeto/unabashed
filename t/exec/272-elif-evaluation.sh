# conditions are evaluated in order until one succeeds
t() { echo "test $1"; return $2; }
if t 1 1; then echo b1
elif t 2 1; then echo b2
elif t 3 0; then echo b3
elif t 4 0; then echo b4
else echo b5
fi
if t a 0; then echo ba; elif t b 0; then echo bb; fi
if t x 1; t y 0; then echo "last of list decides"; fi
if t m 0; t n 1; then echo no; else echo "list false"; fi
