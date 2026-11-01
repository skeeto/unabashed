== stdout 97
count=0
all=[]
star=[]
zero=bash
st=1
st=1
one=a two=b
st=1
none
f1=only
st=1
now=0
atlen=0
st=1

== stderr 178
bash: line 7: $1: unbound variable
bash: line 8: 10: unbound variable
bash: line 11: $3: unbound variable
bash: line 13: $2: unbound variable
bash: line 18: $1: unbound variable

== merged 275
count=0
all=[]
star=[]
zero=bash
bash: line 7: $1: unbound variable
st=1
bash: line 8: 10: unbound variable
st=1
one=a two=b
bash: line 11: $3: unbound variable
st=1
none
f1=only
bash: line 13: $2: unbound variable
st=1
now=0
atlen=0
bash: line 18: $1: unbound variable
st=1

== status 0
