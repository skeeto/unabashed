declare -A h=([one]=1 [two]=2 [three]=3 [four]=4 [five]=5 [six]=6 [seven]=7 [eight]=8 [nine]=9 [ten]=10)
echo "${!h[@]}"; echo "${h[@]}"
declare -A g; for i in {1..3000}; do g[k$i]=$i; done; echo "${#g[@]}"; s=; for k in "${!g[@]}"; do s+=$k,; done; echo ${s:0:200}
x=7; declare -A c; c[x]=1; declare -p c; f=str; declare -A f; f[a]=b; declare -p f
declare u; declare -A u; for i in {1..300}; do u[$i]=v; done; s=; for k in "${!u[@]}"; do s+=$k,; done; echo ${s:0:300}
unset 'g[k5]' 'g[k77]'; echo "${#g[@]}"; declare -A e=(); echo "${!e[@]}" "${#e[@]}"
declare -A q=([a]=1 [b]=2); q=([c]=3 [a]=4); declare -p q; q+=([d]=5 [c]+=x); declare -p q
declare -A t=(k1 v1 k2 v2 k3); declare -p t; declare -A t2=([$'\t']=tab ["*"]=star [@]=at ['~']=til [#x]=hash [x#]=h2 ['a"b']=q); declare -p t2
