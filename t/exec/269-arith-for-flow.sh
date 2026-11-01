# continue runs the step expression; break skips it
for ((i=0; i<5; i++)); do
    [ $i = 1 ] && continue
    [ $i = 3 ] && break
    echo "i=$i"
done
echo "after: i=$i"
n=0
for ((i=0; i<3; i++)); do
    for ((j=0; j<3; j++)); do
        n=$((n+1))
        [ $j = 1 ] && continue 2
    done
done
echo "n=$n i=$i j=$j"
for ((i=0; i<3; i++)); do
    i=$((i+1))
    echo "modified i=$i"
done
c=0
for ((i=0; i<10; i++, c++)); do [ $i = 2 ] && break; done
echo "c=$c"
