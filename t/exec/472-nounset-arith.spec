== stdout 38
a=4
st=1
st=1
d ok
st=1
st=1
g=1
st=1

== stderr 171
bash: line 4: y: unbound variable
bash: line 5: y: unbound variable
bash: line 7: z: unbound variable
bash: line 8: w: unbound variable
bash: line 10: n: unbound variable

== merged 209
a=4
bash: line 4: y: unbound variable
st=1
bash: line 5: y: unbound variable
st=1
d ok
bash: line 7: z: unbound variable
st=1
bash: line 8: w: unbound variable
st=1
g=1
bash: line 10: n: unbound variable
st=1

== status 0
