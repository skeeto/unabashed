# C-style for loops
for ((i=0; i<3; i++)); do echo "i=$i"; done
echo "after: i=$i status $?"
for ((i=0, j=10; i<j; i+=4, j--)); do echo "$i $j"; done
false
for ((i=5; i<3; i++)); do echo never; done
echo "never: $? i=$i"
for ((i=0; i<2; i++)); do (exit 4); done
echo "body: $?"
n=0
for ((;;)); do n=$((n+1)); [ $n -ge 3 ] && break; done
echo "infinite broken at $n"
for ((k=0; k<2;)); do echo "k=$k"; ((k++)); done
for ((; m<2; m++)); do echo "m=$m"; done
for (( x = 1 ; x <= 4 ; x *= 2 ))
do
    echo "x=$x"
done
for ((i=0; i<2; i++)) { echo "brace $i"; }
lim=2
for ((i=0; i<lim; i++)); do echo "var lim $i"; done
for ((i=0; i<$lim; i++)); do echo "dollar lim $i"; done
