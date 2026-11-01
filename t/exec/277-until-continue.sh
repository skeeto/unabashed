# continue in until and while re-evaluates the condition
i=0
until [ $i -ge 4 ]; do
    i=$((i+1))
    [ $i = 2 ] && continue
    echo "until $i"
done
i=0
while [ $i -lt 4 ]; do
    i=$((i+1))
    [ $i = 3 ] && continue
    echo "while $i"
done
i=0
until [ $i -ge 3 ]; do
    i=$((i+1))
    for j in a b; do
        [ $j = b ] && continue 2
        echo "$i$j"
    done
    echo never
done
