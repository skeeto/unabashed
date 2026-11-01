== stdout 24
one
st=1
got three
st=1

== stderr 90
bash: line 4: $(echo /tmp/a b): ambiguous redirect
bash: line 8: $(:): ambiguous redirect

== merged 114
one
bash: line 4: $(echo /tmp/a b): ambiguous redirect
st=1
got three
bash: line 8: $(:): ambiguous redirect
st=1

== status 0
