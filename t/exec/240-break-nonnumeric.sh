# a non-numeric break count in a loop is fatal in a script
(for i in 1 2; do break x; echo "sub after break"; done; echo "sub after loop")
echo "subshell status $?"
for i in 1 2; do
    echo "i=$i"
    break abc
    echo "after break"
done
echo "not reached $?"
