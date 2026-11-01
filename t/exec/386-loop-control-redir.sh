# break, continue and return inside redirected loops restore descriptors
printf '%s\n' a b c d >/tmp/f
while read l; do
    [ "$l" = b ] && continue
    [ "$l" = c ] && break
    echo "loop $l"
done </tmp/f
read x || echo "stdin restored: status $?"
f() {
    while read l; do
        if [ "$l" = b ]; then return 7; fi
        echo "f $l"
    done </tmp/f
}
f
echo "f status $?"
read x || echo "stdin still restored"
g() { echo "g out"; return 3; } >/tmp/g
g; echo "g status $?"
echo visible
