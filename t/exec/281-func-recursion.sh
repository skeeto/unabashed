fact() {
    if (( $1 <= 1 )); then
        echo 1
    else
        local sub
        sub=$(fact $(( $1 - 1 )))
        echo $(( $1 * sub ))
    fi
}
fact 1; fact 5; fact 10
fib() {
    local n=$1
    if (( n < 2 )); then r=$n; return; fi
    fib $((n - 1)); local a=$r
    fib $((n - 2)); r=$((a + r))
}
fib 15; echo "fib 15 = $r"
depth=0
down() { depth=$((depth+1)); if [ $1 -gt 0 ]; then down $(($1 - 1)); fi; echo "unwind $1"; }
down 3
echo "depth=$depth"
