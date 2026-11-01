== stdout 100
fd=10
fd2=11
status 1
[hello]
in=10
line=hello
temp
t=10
[grouped]
dup=12
via-dup
status 1
status 1

== stderr 116
bash: line 9: $fd: Bad file descriptor
bash: line 26: nv: ambiguous redirect
bash: line 29: bad: ambiguous redirect

== merged 216
fd=10
fd2=11
bash: line 9: $fd: Bad file descriptor
status 1
[hello]
in=10
line=hello
temp
t=10
[grouped]
dup=12
via-dup
bash: line 26: nv: ambiguous redirect
status 1
bash: line 29: bad: ambiguous redirect
status 1

== status 0
