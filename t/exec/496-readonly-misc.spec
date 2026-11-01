== stdout 135
a=unset b=2
st=1
declare -r a
declare -r b="2"
redef st=1
f
unsetf st=1
c st=1 c=3
arith st=1 b=2
let st=1 b=2
prefix-builtin st=0
end

== stderr 271
bash: line 3: a: readonly variable
bash: line 8: f: readonly function
bash: line 11: unset: f: cannot unset: readonly function
bash: line 14: c: readonly variable
bash: line 16: b: readonly variable
bash: line 18: b: readonly variable
bash: line 20: b: readonly variable

== merged 406
a=unset b=2
bash: line 3: a: readonly variable
st=1
declare -r a
declare -r b="2"
bash: line 8: f: readonly function
redef st=1
f
bash: line 11: unset: f: cannot unset: readonly function
unsetf st=1
bash: line 14: c: readonly variable
c st=1 c=3
bash: line 16: b: readonly variable
arith st=1 b=2
bash: line 18: b: readonly variable
let st=1 b=2
bash: line 20: b: readonly variable
prefix-builtin st=0
end

== status 0
