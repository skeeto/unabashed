# break, continue and return through eval
for i in 1 2 3; do
    eval '[ $i = 2 ] && continue'
    echo "i=$i"
    eval break
done
for i in 1 2; do
    for j in a b; do
        eval "break 2"
    done
    echo never
done
echo "after $i$j"
f() {
    for i in 1 2; do eval 'return 3'; done
    echo never
}
f; echo "f: $?"
for i in 1 2; do
    eval 'for j in x y; do break 2; done'
    echo never
done
echo "eval inner loop: $i$j"
