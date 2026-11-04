== stdout 82
group
st=0
st=0
loop
st=0
st=1
st=0 x=[in]
st=1
x=[comsub]
outer
st=0
script
st=1

== stderr 150
bash: line 7: 4: Bad file descriptor
bash: line 9: 4: Bad file descriptor
bash: line 11: 3: Bad file descriptor
bash: line 14: 4: Bad file descriptor

== merged 232
group
st=0
st=0
loop
st=0
bash: line 7: 4: Bad file descriptor
st=1
st=0 x=[in]
bash: line 9: 4: Bad file descriptor
st=1
x=[comsub]
bash: line 11: 3: Bad file descriptor
outer
st=0
script
bash: line 14: 4: Bad file descriptor
st=1

== status 0
