x=global
f() {
    local x=local
    unset x
    echo "after unset: [${x-unset}]"
    x=again
    echo "reassigned: $x"
}
f
echo "global: $x"
g() {
    local y=1
    unset y
    declare -p y
    echo "status $?"
}
y=gy
g
echo "y: $y"
