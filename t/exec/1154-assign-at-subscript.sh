# ${name[@]:=word} and ${name[*]=word} for an indexed array, unset or
# empty, cannot assign: bash's assign_array_element reports "name[@]:
# bad array subscript" and the command is abandoned. A positional or
# special parameter "cannot assign in this way"; an associative array
# takes @ as a key.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c; echo \"status \$?\""
done <<'EOF'
y="${map[@]:=x}"; echo "<$y>"
y=${arr[*]:=x}; echo "<$y>"
echo ${x[@]=x}
arr=(); echo ${arr[@]:=x}
arr=(''); echo "${arr[*]:=x}"
x=1; echo ${x[@]:=y}
declare -n r=arr; echo ${r[@]:=x}
echo ${1:=x}
echo ${@:=x}
echo ${1[@]:=x}
declare -A map; echo ${map[@]:=x}; declare -p map
echo ${arr[1]:=x}; declare -p arr
EOF
