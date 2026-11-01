# break in a function called from a loop does not reach the loop
f() { echo "f $1"; break; echo "f after $?"; }
g() { continue 2; echo "g after $?"; }
for i in 1 2; do
    f $i
    g
    echo "loop $i"
done
h() {
    for j in a b c; do
        [ $j = b ] && break
        echo "h $j"
    done
    echo "h done"
}
for i in 1 2; do h; done
