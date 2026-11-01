== stdout 41
status 1
[one]
[two]
a=line1
status 1 b=

== stderr 214
bash: line 7: 4: Bad file descriptor
bash: line 17: read: 6: invalid file descriptor: Bad file descriptor
bash: line 20: echo: write error: Bad file descriptor
bash: line 21: echo: write error: Bad file descriptor

== merged 255
bash: line 7: 4: Bad file descriptor
status 1
[one]
[two]
a=line1
bash: line 17: read: 6: invalid file descriptor: Bad file descriptor
status 1 b=
bash: line 20: echo: write error: Bad file descriptor
bash: line 21: echo: write error: Bad file descriptor

== status 1
