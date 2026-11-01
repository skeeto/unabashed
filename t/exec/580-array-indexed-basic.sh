a=(1 2 3); a[7]=x; declare -p a; declare -A h=([b]=2 [a]=1 [zz]="q r"); declare -p h
echo ${a[-1]} ${a[-8]}; echo ${a[-9]}; a[-9]=3; echo st=$?
b=x; b[2]=y; declare -p b; declare -a c; declare -p c; c=(); declare -p c
declare -A e; declare -p e; local x 2>/dev/null; f(){ local -a l=(1 '2 3'); declare -p l; local -A m; declare -p m; }; f
echo ${!a[@]} "${#a[@]}" ${#a[7]}
readonly -a r=(1 2); declare -p r; export -a ex=(1); declare -p ex
declare -ai ai=(1+1 3*3); declare -p ai; ai+=(4); declare -p ai; ai[0]+=5; declare -p ai
s=([0]=a [5]=b c); declare -p s; s+=([1]=x [5]+=y z); declare -p s
