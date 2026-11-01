== stdout 140
x=[]
y=[out
err]
z=[shown]
r=hidden
w=from-here
v=<one>
<two>
status 1 u=[bash: line 17: echo: write error: Bad file descriptor]
start
t=[]

== stderr 118
to-stderr
bash: line 21: echo: write error: Bad file descriptor
bash: line 22: echo: write error: Bad file descriptor

== merged 258
to-stderr
x=[]
y=[out
err]
z=[shown]
r=hidden
w=from-here
v=<one>
<two>
status 1 u=[bash: line 17: echo: write error: Bad file descriptor]
start
bash: line 21: echo: write error: Bad file descriptor
bash: line 22: echo: write error: Bad file descriptor
t=[]

== status 0
