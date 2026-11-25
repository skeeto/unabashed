== stdout 1690
-- ((arr[x]))
bash: line 1: arr: unbound variable
exit 127
-- x='a b'; ((arr[x]))
bash: line 1: arr: unbound variable
exit 127
-- ((map[k]++))
bash: line 1: map: unbound variable
exit 127
-- echo $((arr[1+y]))
bash: line 1: arr: unbound variable
exit 127
-- ((arr[x]+=1))
bash: line 1: arr: unbound variable
exit 127
-- declare -a arr; ((arr[x]))
bash: line 1: arr: unbound variable
exit 127
-- declare -n r=arr; ((r[x]))
bash: line 1: r: unbound variable
exit 127
-- f() { local -a arr; ((arr[x])); }; f
bash: line 1: arr: unbound variable
exit 127
-- x='arr[y]'; echo $((x))
bash: line 1: arr: unbound variable
exit 127
-- [[ arr[x] -eq 0 ]]
bash: line 1: arr: unbound variable
exit 127
-- echo $(( 0 && arr[x] ))
0
status 0
exit 0
-- ((arr[x]=1)); declare -p arr
bash: line 1: x: unbound variable
exit 127
-- arr=(1); ((arr[x]))
bash: line 1: x: unbound variable
exit 127
-- declare -A m=([a]=1); ((m[k])); echo "<$?>"
<1>
status 0
exit 0
-- echo ${#arr[x]}
bash: line 1: arr[x]: unbound variable
exit 127
-- echo ${#arr[$x]}
bash: line 1: arr[$x]: unbound variable
exit 127
-- echo "${#arr[0]}"
bash: line 1: arr[0]: unbound variable
exit 127
-- echo ${#arr[@]}
bash: line 1: arr: unbound variable
exit 1
-- x=abc; echo ${#x[0]}
bash: line 1: x[0]: unbound variable
exit 127
-- x=abc; echo ${#x[@]}
bash: line 1: x: unbound variable
exit 1
-- declare -a arr; echo ${#arr[x]}
bash: line 1: arr[x]: unbound variable
exit 127
-- declare -n r=arr; echo ${#r[x]}
bash: line 1: r[x]: unbound variable
exit 127
-- arr=(1); echo ${#arr[x]}
bash: line 1: x: unbound variable
exit 127
-- declare -A m=([a]=1); echo ${#m[k]}
0
status 0
exit 0
-- f() { echo ${#FUNCNAME[0]}; }; f
1
status 0
exit 0

== stderr 0

== merged 1690
-- ((arr[x]))
bash: line 1: arr: unbound variable
exit 127
-- x='a b'; ((arr[x]))
bash: line 1: arr: unbound variable
exit 127
-- ((map[k]++))
bash: line 1: map: unbound variable
exit 127
-- echo $((arr[1+y]))
bash: line 1: arr: unbound variable
exit 127
-- ((arr[x]+=1))
bash: line 1: arr: unbound variable
exit 127
-- declare -a arr; ((arr[x]))
bash: line 1: arr: unbound variable
exit 127
-- declare -n r=arr; ((r[x]))
bash: line 1: r: unbound variable
exit 127
-- f() { local -a arr; ((arr[x])); }; f
bash: line 1: arr: unbound variable
exit 127
-- x='arr[y]'; echo $((x))
bash: line 1: arr: unbound variable
exit 127
-- [[ arr[x] -eq 0 ]]
bash: line 1: arr: unbound variable
exit 127
-- echo $(( 0 && arr[x] ))
0
status 0
exit 0
-- ((arr[x]=1)); declare -p arr
bash: line 1: x: unbound variable
exit 127
-- arr=(1); ((arr[x]))
bash: line 1: x: unbound variable
exit 127
-- declare -A m=([a]=1); ((m[k])); echo "<$?>"
<1>
status 0
exit 0
-- echo ${#arr[x]}
bash: line 1: arr[x]: unbound variable
exit 127
-- echo ${#arr[$x]}
bash: line 1: arr[$x]: unbound variable
exit 127
-- echo "${#arr[0]}"
bash: line 1: arr[0]: unbound variable
exit 127
-- echo ${#arr[@]}
bash: line 1: arr: unbound variable
exit 1
-- x=abc; echo ${#x[0]}
bash: line 1: x[0]: unbound variable
exit 127
-- x=abc; echo ${#x[@]}
bash: line 1: x: unbound variable
exit 1
-- declare -a arr; echo ${#arr[x]}
bash: line 1: arr[x]: unbound variable
exit 127
-- declare -n r=arr; echo ${#r[x]}
bash: line 1: r[x]: unbound variable
exit 127
-- arr=(1); echo ${#arr[x]}
bash: line 1: x: unbound variable
exit 127
-- declare -A m=([a]=1); echo ${#m[k]}
0
status 0
exit 0
-- f() { echo ${#FUNCNAME[0]}; }; f
1
status 0
exit 0

== status 0
