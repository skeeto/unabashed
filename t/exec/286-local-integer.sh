f() {
    local -i n=3+4
    echo "n=$n"
    n+=2
    echo "n=$n"
    n="n * 2"
    echo "n=$n"
    n=abc
    echo "n=$n"
    declare -p n
    local -i m
    m+=5
    echo "m=$m"
}
f
echo "n outside: [${n-unset}]"
