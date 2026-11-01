# break in a subshell or command substitution inside a loop
for i in 1 2; do
    (break; echo "subshell continues $i")
    echo "loop $i"
done
for i in 1 2; do
    (for j in a b; do break; done; echo "inner loop broke $i")
done
for i in 1 2; do
    x=$(continue; echo "cmdsub $i")
    echo "x=$x"
done
for i in 1 2; do
    ( exit 3 ) || echo "status $?"
done
