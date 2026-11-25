== stdout 811
-- y="${map[@]:=x}"; echo "<$y>"
bash: line 1: map[@]: bad array subscript
-- y=${arr[*]:=x}; echo "<$y>"
bash: line 1: arr[*]: bad array subscript
-- echo ${x[@]=x}
bash: line 1: x[@]: bad array subscript
-- arr=(); echo ${arr[@]:=x}
bash: line 1: arr[@]: bad array subscript
-- arr=(''); echo "${arr[*]:=x}"
bash: line 1: arr[*]: bad array subscript
-- x=1; echo ${x[@]:=y}
1
status 0
-- declare -n r=arr; echo ${r[@]:=x}
bash: line 1: r[@]: bad array subscript
-- echo ${1:=x}
bash: line 1: $1: cannot assign in this way
-- echo ${@:=x}
bash: line 1: $@: cannot assign in this way
-- echo ${1[@]:=x}
bash: line 1: ${1[@]:=x}: bad substitution
-- declare -A map; echo ${map[@]:=x}; declare -p map
x
declare -A map=(["@"]="x" )
status 0
-- echo ${arr[1]:=x}; declare -p arr
x
declare -a arr=([1]="x")
status 0

== stderr 0

== merged 811
-- y="${map[@]:=x}"; echo "<$y>"
bash: line 1: map[@]: bad array subscript
-- y=${arr[*]:=x}; echo "<$y>"
bash: line 1: arr[*]: bad array subscript
-- echo ${x[@]=x}
bash: line 1: x[@]: bad array subscript
-- arr=(); echo ${arr[@]:=x}
bash: line 1: arr[@]: bad array subscript
-- arr=(''); echo "${arr[*]:=x}"
bash: line 1: arr[*]: bad array subscript
-- x=1; echo ${x[@]:=y}
1
status 0
-- declare -n r=arr; echo ${r[@]:=x}
bash: line 1: r[@]: bad array subscript
-- echo ${1:=x}
bash: line 1: $1: cannot assign in this way
-- echo ${@:=x}
bash: line 1: $@: cannot assign in this way
-- echo ${1[@]:=x}
bash: line 1: ${1[@]:=x}: bad substitution
-- declare -A map; echo ${map[@]:=x}; declare -p map
x
declare -A map=(["@"]="x" )
status 0
-- echo ${arr[1]:=x}; declare -p arr
x
declare -a arr=([1]="x")
status 0

== status 0
