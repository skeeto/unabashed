== stdout 13
one
st=1
two

== stderr 39
bash: line 2: ${x y}: bad substitution

== merged 52
one
bash: line 2: ${x y}: bad substitution
st=1
two

== status 0
