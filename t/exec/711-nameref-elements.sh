# namerefs to array elements and whole arrays
a=(zero one two)
declare -n e='a[1]'
echo "$e ${!e}"
e=uno; declare -p a
e+=X; echo "${a[1]}"
unset e; declare -p a
declare -n all='a[@]'
printf '<%s>' "$all"; echo
printf '<%s>' $all; echo
declare -n arr=a
arr+=(three); arr[7]=seven
echo "${arr[@]} ${#arr[@]} ${!arr[@]}"
echo "${arr[@]:1:2} ${arr[-1]}"
declare -A m=([k]=v)
declare -n mr=m
mr[j]=w; echo "${mr[k]} ${mr[j]} ${#mr[@]}"
declare -n me='m[k]'
me=V; echo "${m[k]}"
declare -n r; r[0]=foo; echo "st=$?"
declare -n q; q=(1 2); declare -p q
declare -n t='a[0]'; t=(1 2); echo notreached
