== stdout 46
a=first
b=second
c=third
status 1 d=
status 1

== stderr 56
bash: line 13: read: 0: read error: Bad file descriptor

== merged 102
a=first
b=second
c=third
status 1 d=
bash: line 13: read: 0: read error: Bad file descriptor
status 1

== status 0
