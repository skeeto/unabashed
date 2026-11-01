== stdout 92
status 0
read status 1
truncated status 1
x=1
status 1
status 1 y=2
status 1
z=sub
status 0

== stderr 150
bash: line 11: /tmp/nosuch: No such file or directory
bash: line 13: /tmp/nosuch: No such file or directory
bash: line 15: $empty: ambiguous redirect

== merged 242
status 0
read status 1
truncated status 1
x=1
bash: line 11: /tmp/nosuch: No such file or directory
status 1
bash: line 13: /tmp/nosuch: No such file or directory
status 1 y=2
bash: line 15: $empty: ambiguous redirect
status 1
z=sub
status 0

== status 0
