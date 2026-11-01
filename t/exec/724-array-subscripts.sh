# array slices, negative and sparse subscripts, scalars as arrays
a=(1 2 3 4 5)
echo "${a[@]:1:2}" "${a[@]: -2:1}" "${a[*]:3}" "${a[@]:9}" "${#a[@]:1}" 2>&1
unset 'a[1]'; echo "${a[@]:1:2}" "${!a[@]}"
a[-1]=X; a+=([10]=t u); echo "${a[@]}" "${!a[@]}"
b=([5]=five [2]=two); echo "${b[@]}" "${b[-1]}" "${b[@]: -1}"
declare -a c=([3]=x); c[-1]+=y; echo "${c[@]}"
s=str; echo "${s[@]:1}" "${s[0]}" "${s[-1]}" "${#s[@]}"
declare -A m; m[a]=1; m[b]=2; unset 'm[a]'; m[c]=3; echo "${!m[@]}"
x=(a b); x[1]+=c; x+=(d); echo "${x[@]}"; declare -i n=(1+1 2*3); declare -p n
y=( [1]=a [1]+=b ); declare -p y
s=str
echo "1 ${u[-1]}"
echo "2 ${s[-1]}"
echo "3 ${#s[-1]}"
echo "4 ${s[-0]}"
unset s; echo "5 ${s[-1]}"
echo done
