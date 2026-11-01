# nesting different loop kinds
i=0
while [ $i -lt 2 ]; do
    for w in a b; do
        n=0
        until [ $n -ge 2 ]; do
            for ((k=0; k<2; k++)); do
                printf '%s%s%s%s ' $i $w $n $k
            done
            n=$((n+1))
        done
    done
    i=$((i+1))
done
echo
for a in 1 2 3; do
    for b in 1 2 3; do
        [ $b -gt $a ] && continue 2
        [ $a = 3 ] && break 2
        printf '%s,%s ' $a $b
    done
done
echo
for a in x y; do
    while :; do
        until false; do
            echo "deep $a"
            break 2
        done
        echo never
    done
    echo "after while $a"
done
