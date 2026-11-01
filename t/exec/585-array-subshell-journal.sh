# subshell isolation and journaling
a=(1 2 3); declare -A h=([x]=1)
( a[1]=X; a+=(4); h[y]=2; unset 'h[x]'; declare -p a h )
declare -p a h
v=$(a=(q); h=(); declare -p a h); echo "$v"
declare -p a h
f() { local -a a=(L); a[5]=M; declare -p a; }; f; declare -p a
g() { a[0]=G; h[z]=3; }; g; declare -p a h
( unset a; declare -p a ) 2>&1; declare -p a
for i in 1 2 3; do b[i]=$((i*i)); done; declare -p b
echo "${b[@]}" "${#b[*]}"
c=(); c[0]=x; c+=(y); echo ${c[*]}
x=(a b); x[1]+=c; x+=d; declare -p x
i=3; arr[i++]=v; arr[i]=w; declare -p arr i
(( z[2] = 5, z[1]++ )); declare -p z; echo $(( z[2] * 2 )) $((z[1]))
declare -A m; m[a]=1; (( m[a] += 4 )); (( m[b]++ )); declare -p m
k=key; m[$k]=v; echo "${m[$k]}" "${m[key]}"; m["a b"]=sp; echo "${m[a b]}"
