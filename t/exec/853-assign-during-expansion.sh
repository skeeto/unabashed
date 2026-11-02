declare -A B=([x]=0); B+=(x 1 y ${B[x]}); declare -p B
declare -A D=([x]=0); D=(x 1 y ${D[x]}); declare -p D
declare -A F=([x]=0); F+=(x 1 y "${F[x]}" z "${F[y]}"); declare -p F
declare -A G=([x]=0); G+=(x $((n=1)) y "$n${G[x]}"); declare -p G
declare -A H=([x]=0); H+=(x 1 y "${H[x]}" "${H[x]}"); declare -p H
declare -A A=([x]=0); A+=([x]=1 [y]=${A[x]}); declare -p A
x=hellohellohellohellohellohellohellohello; echo "${x/l/${ unset x; y=zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz; }L}"
declare -A K=([k]=hellohellohellohellohellohellohello); echo "${K[k]/l/${ unset K; M=(zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz); }L}"
x=hellohellohellohellohellohellohellohello; echo "${x/l/$((x=5))}" $x
