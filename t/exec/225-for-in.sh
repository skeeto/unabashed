# for with an in-list
for w in a b 'c d' "e  f"; do echo "[$w]"; done
for w in; do echo never; done
echo "empty list: $?"
v='x y  z'
for w in $v; do echo "<$w>"; done
for w in "$v"; do echo "<$w>"; done
for w in pre$v'post'; do echo "{$w}"; done
e=
for w in $e; do echo never; done
for w in "$e"; do echo "quoted empty [$w]"; done
for w in $e "" $e; do echo "middle [$w]"; done
for w in a b
do
    echo "newline do $w"
done
for w in 1 2; { echo "brace $w"; }
