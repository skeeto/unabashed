# associative array order (bash's hashlib) for every creation path, slices,
# negative subscripts and @K @k @a @A @Q transformations
keys() { local -n _a=$1; local i; for ((i=0;i<200;i++)); do _a[k$i]=$i; done; echo "${!_a[@]}"; }
declare -A a1; keys a1
s=x; declare -A s; keys s
declare -A a3=(); keys a3
f() { local -A a4; keys a4; }; f
g() { declare -A a5=([z]=1); keys a5; }; g
declare -A a6; a6=([q]=1); keys a6
a7=1; declare -A a7; a7=(); keys a7
declare -A a8; unset a8; declare -A a8; keys a8
declare -A a9=([a]=1 [b]=2 [c]=3 [d]=4 [e]=5); a9+=([f]=6); echo ${!a9[@]}
declare -A b=([x]=1); b=(); for i in {1..2100}; do b[$i]=$i; done; echo "${!b[@]}"
declare -A c; c=( $(for i in {1..50}; do echo k$i v$i; done) ); echo "${!c[@]}"
x=(1 2 3); echo "${x[@]:1:5} ${x[@]: -2} ${x[@]:0:-1}" 2>&1
x[10]=10; echo "${x[@]:2:2} ${x[-1]} ${x[@]: -1:1}"; echo "${!x[@]}"
declare -A m=([a]=1 [b]="x y"); echo "${m[@]@K}"; echo "${m[@]@k}"; echo "${m[@]@a}" "${x[@]@a}" "${x@a}"
printf '<%s>' "${m[@]@k}"; echo; echo "${x[@]@Q}"; echo "${x[@]@A}"; echo "${m[@]@A}"
x[-1]=last; echo ${x[@]}; x[-20]=bad
