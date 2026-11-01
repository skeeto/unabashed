== stdout 176
1 x 1
2
23
4 x
declare -n r="x"
declare -n s="r"
x=unset
declare -n r="x"
bash: line 11: declare: r: not found
declare -n e="y"
y=5
dflt
declare -n z="dflt"
new
abxde bc ABCDE

== stderr 90
bash: line 13: `a b': not a valid identifier
bash: line 20: r: invalid indirect expansion

== merged 266
1 x 1
2
23
4 x
declare -n r="x"
declare -n s="r"
x=unset
declare -n r="x"
bash: line 11: declare: r: not found
declare -n e="y"
y=5
bash: line 13: `a b': not a valid identifier
dflt
declare -n z="dflt"
new
bash: line 20: r: invalid indirect expansion
abxde bc ABCDE

== status 0
