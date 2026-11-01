set -u
x=3
echo "a=$((x + 1))"
(echo "b=$((y + 1))"; echo after-b); echo "st=$?"
(echo "c=$(($y + 1))"; echo after-c); echo "st=$?"
((x > 2)) && echo "d ok"
( (( z )); echo "after-e st=$?"); echo "st=$?"
(let "w = w + 1"; echo "after-f st=$?"); echo "st=$?"
(z=1; echo "g=$((z))")
(for ((i = 0; i < n; i++)); do :; done; echo after-h); echo "st=$?"
