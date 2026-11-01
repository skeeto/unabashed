== stdout 421
declare -i x="5"
declare -iu x="0"
declare -- r="x"
declare -n r="x"
declare -n r="x"
declare -n foo="bar"
bar other
bash: line 11: declare: `1x': invalid variable name for name reference
st=1
bash: line 12: declare: n2: nameref variable self references not allowed
st=1
bash: line 13: declare: arr: reference variable cannot be an array
st=1
st=1
declare -a e1
st=1
x
declare -nx xr="a"
declare -nx xr="b"
 one
 two
1
2

== stderr 178
bash: line 14: declare: `': not a valid identifier
bash: line 15: x: readonly variable
bash: line 17: declare: `42': not a valid identifier
bash: line 17: declare: e2: not found

== merged 599
declare -i x="5"
declare -iu x="0"
declare -- r="x"
declare -n r="x"
declare -n r="x"
declare -n foo="bar"
bar other
bash: line 11: declare: `1x': invalid variable name for name reference
st=1
bash: line 12: declare: n2: nameref variable self references not allowed
st=1
bash: line 13: declare: arr: reference variable cannot be an array
st=1
bash: line 14: declare: `': not a valid identifier
st=1
bash: line 15: x: readonly variable
declare -a e1
bash: line 17: declare: `42': not a valid identifier
st=1
bash: line 17: declare: e2: not found
x
declare -nx xr="a"
declare -nx xr="b"
 one
 two
1
2

== status 0
