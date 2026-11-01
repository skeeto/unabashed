# break with too many arguments
for i in 1 2; do
    echo "i=$i"
    break 1 2
    echo "after break"
done
echo "after loop $?"
