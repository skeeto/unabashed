declare -a a; declare -p a; a=(); declare -p a; a=(x); declare -p a
declare -ar r=(1 2); r[0]=z; echo st=$?; r=(3); echo st=$?; unset r; echo st=$?; unset 'r[0]'; echo st=$?; declare -p r
declare -ax ex=(1 2); declare -p ex; env | grep ^ex= ; echo done
declare -al low=(A B); declare -p low; low+=(C); low[5]=DD; declare -p low
declare -Au up=([k]=v); up[x]+=w; declare -p up
declare -a s=str 2>&1; declare -p s
declare b[2]=x; declare -p b; declare c[3]; declare -p c
declare -a 'q=(1 2 $HOME)'; declare -p q; w='(1 2)'; declare -a v=$w; declare -p v
declare n='(1 2)'; declare -p n
declare -A A; declare -p A; declare A+=([x]=1); declare -p A
f() { local -A loc=([k]=v); local -a la; la+=(1); declare -p loc la; local; }; f
g() { declare -g -a ga=(1 2); declare -g gh; }; g; declare -p ga
typeset -a ty=(1); declare -p ty; declare +a ty; declare -p ty
echo "${a[@]@A}" "${ex[@]@A}"; declare -p a | cat
export -p | grep ex
