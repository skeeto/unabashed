# break and continue with counts in nested loops
for i in 1 2 3; do
    for j in a b c; do
        [ $j = b ] && break
        echo "$i$j"
    done
done
echo --
for i in 1 2 3; do
    for j in a b c; do
        [ $j = b ] && break 2
        echo "$i$j"
    done
    echo never
done
echo "after break 2: $?"
for i in 1 2; do
    for j in a b; do
        [ $j = b ] && continue 2
        echo "$i$j"
    done
    echo never
done
echo --
for i in 1 2; do
    for j in a b; do
        break 99
    done
    echo never
done
echo "break 99: $? $i$j"
for i in 1 2; do
    for j in a b; do
        echo "$i$j"
        continue 99
    done
    echo never
done
echo "continue 99: $?"
for i in 1 2; do break 1; done; echo "break 1: $i"
for i in 1 2; do continue 1; echo no; done; echo "continue 1: $i"
