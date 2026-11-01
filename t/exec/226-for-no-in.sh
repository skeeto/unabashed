# for without in iterates over the positional parameters
set -- a 'b c' d
for x; do echo "[$x]"; done
for x
do
    echo "nl [$x]"
done
for x do echo "nosemi [$x]"; done
f() {
    for y; do echo "f [$y]"; done
    echo "f status $?"
}
f 1 '2 3'
f
set --
false
for x; do echo never; done
echo "none: $?"
