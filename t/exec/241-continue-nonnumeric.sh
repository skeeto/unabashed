# a non-numeric continue count in a loop
while :; do
    echo body
    continue 1x
    echo after
done
echo "not reached $?"
