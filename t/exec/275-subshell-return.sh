# return inside a subshell inside a function only leaves the subshell
f() {
    for i in 1 2; do
        (return 4; echo never)
        echo "sub $i: $?"
    done
    (for j in a b; do return 5; done; echo never)
    echo "loop sub: $?"
    x=$(return 6; echo never)
    echo "cmdsub: $? [$x]"
    return 7
}
f
echo "f: $?"
