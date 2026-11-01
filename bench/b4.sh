# associative arrays: insert, look up, iterate keys, unset half
declare -A m
for ((i=0; i<200000; i++)); do m[k$i]=$i; done
s=0; for ((i=0; i<200000; i++)); do ((s += m[k$i])); done; echo $s
n=0; for k in "${!m[@]}"; do ((n++)); done; echo $n
for ((i=0; i<200000; i+=2)); do unset "m[k$i]"; done
echo ${#m[@]}
