# while counting loops and their statuses
i=0
while [ $i -lt 3 ]; do
    echo "i=$i"
    i=$((i+1))
done
echo "status: $?"
i=0
while [ $i -lt 2 ]; do i=$((i+1)); false; done
echo "body false: $?"
false
while false; do echo never; done
echo "never ran: $?"
i=0
while i=$((i+1)); [ $i -le 3 ]; do printf '%s ' $i; done
echo
i=0
while (( i++ < 3 )); do printf '<%s>' $i; done
echo " i=$i"
