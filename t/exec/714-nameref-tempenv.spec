== stdout 83
temp
declare -x var="temp"
declare -- var="foo"
f: fn fn
foo
f: foo foo
fd=10
st=1

== stderr 101
bash: line 13: exec: `11': not a valid identifier
bash: line 13: badfd: cannot assign fd to variable

== merged 184
temp
declare -x var="temp"
declare -- var="foo"
f: fn fn
foo
f: foo foo
fd=10
bash: line 13: exec: `11': not a valid identifier
bash: line 13: badfd: cannot assign fd to variable
st=1

== status 0
