# the loop variable after the loop, and changes inside the body
x=orig
for x in; do :; done
echo "after empty: $x"
for x in a b c; do :; done
echo "after: $x"
list='1 2 3'
for x in $list; do
    list='9 9 9 9'
    echo "x=$x"
    x=changed
done
echo "x=$x list=$list"
f() {
    local x=local
    for x in p q; do :; done
    echo "in f: $x"
}
x=global
f
echo "outside: $x"
for x in a b; do unset x; done
echo "unset: ${x-unset}"
