# break and continue with counts out of range inside loops
for i in 1 2; do
    for j in a b; do
        echo "$i$j"
        break 0
        echo "after break 0"
    done
    echo "outer $i"
done
echo "break 0: $?"
for i in 1 2; do
    while :; do
        continue -1
        echo "after continue -1"
    done
done
echo "continue -1: $?"
for i in 1; do break -- 0; done
echo "break -- 0: $?"
for i in 1 2; do break -- 2; done
echo "break -- 2: $? $i"
for i in 1 2; do echo $i; continue ' 1'; done
echo "spaces: $?"
