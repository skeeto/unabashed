# return from a function inside nested loops
f() {
    for i in 1 2 3; do
        while :; do
            [ $i = 2 ] && return 7
            echo "f $i"
            break
        done
    done
    echo never
}
f
echo "f: $?"
g() {
    local n=0
    until false; do
        n=$((n+1))
        case $n in 3) (exit 4); return;; esac
    done
}
g
echo "g: $?"
h() {
    for ((i=0; ; i++)); do
        if [ $i -ge 2 ]; then return $((i*10)); fi
    done
}
h
echo "h: $? i=$i"
outer() {
    for x in a b; do
        f >/dev/null
        echo "outer $x after f $?"
    done
}
outer
k() { for i in 1; do return; done; }
false
k
echo "k: $?"
