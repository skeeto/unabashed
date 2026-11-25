== stdout 137
err
x
s=2
err
x
s=2
err
x
s=0
err
x
err
x
st=1
x
x
st=3
st=1
st=1 []
---
err
exit
e2
s=3
dbg
exit
e2
s=4
ret
exit
e2
s=3
err
exit
e2
s=1

== stderr 20
bash: line 19: u: x

== merged 157
err
x
s=2
err
x
s=2
err
x
s=0
err
x
err
x
st=1
x
x
st=3
st=1
st=1 []
---
err
exit
e2
s=3
dbg
exit
e2
s=4
ret
exit
e2
s=3
err
bash: line 19: u: x
exit
e2
s=1

== status 0
