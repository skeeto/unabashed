# Under set -u, which name is unbound for name[sub] when name is no
# array. In arithmetic, as bash's expr_streval, an unset (or declared
# but unset) name is reported before its subscript is evaluated, by
# the name as written (a nameref's own). In ${#name[sub]}, as bash's
# array_length_reference, an unset name or a scalar is unbound as
# name[sub], the subscript unexpanded, or for @ and * as name, not
# fatally. A set array evaluates the subscript as before.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "set -u; $c; echo \"status \$?\""; echo "exit $?"
done <<'EOF'
((arr[x]))
x='a b'; ((arr[x]))
((map[k]++))
echo $((arr[1+y]))
((arr[x]+=1))
declare -a arr; ((arr[x]))
declare -n r=arr; ((r[x]))
f() { local -a arr; ((arr[x])); }; f
x='arr[y]'; echo $((x))
[[ arr[x] -eq 0 ]]
echo $(( 0 && arr[x] ))
((arr[x]=1)); declare -p arr
arr=(1); ((arr[x]))
declare -A m=([a]=1); ((m[k])); echo "<$?>"
echo ${#arr[x]}
echo ${#arr[$x]}
echo "${#arr[0]}"
echo ${#arr[@]}
x=abc; echo ${#x[0]}
x=abc; echo ${#x[@]}
declare -a arr; echo ${#arr[x]}
declare -n r=arr; echo ${#r[x]}
arr=(1); echo ${#arr[x]}
declare -A m=([a]=1); echo ${#m[k]}
f() { echo ${#FUNCNAME[0]}; }; f
EOF
