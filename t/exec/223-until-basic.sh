# until loops run while the condition fails
i=0
until [ $i -ge 3 ]; do
    echo "i=$i"
    i=$((i+1))
done
echo "status: $?"
false
until true; do echo never; done
echo "never ran: $?"
i=0
until [ $i = 2 ]; do i=$((i+1)); (exit $((i+10))); done
echo "last body: $?"
i=0
until ! [ $i -lt 2 ]; do echo "neg $i"; i=$((i+1)); done
until (( i-- <= 0 )); do printf '[%s]' $i; done
echo
