# indexed arrays: sequential fill and sum, LCG random reads, reverse walk
n=300000
for ((i=0; i<n; i++)); do a[i]=i; done
s=0; for ((i=0; i<n; i++)); do ((s += a[i])); done; echo $s
x=1 s=0
for ((k=0; k<100000; k++)); do ((x = (x*1103515245 + 12345) % 2147483648, s += a[x%n])); done; echo $s
s=0; for ((i=n-1; i>=0; i-=3)); do s=$((s + ${a[i]})); done; echo $s
echo ${#a[@]}
