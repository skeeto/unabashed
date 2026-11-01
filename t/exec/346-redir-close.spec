== stdout 80
status 1
status 1
out
status 1
status 1 x=
status 1
closed-unopened ok
status 0

== stderr 217
bash: line 2: echo: write error: Bad file descriptor
bash: line 4: echo: write error: Bad file descriptor
bash: line 8: read: 0: read error: Bad file descriptor
bash: line 10: printf: write error: Bad file descriptor

== merged 297
bash: line 2: echo: write error: Bad file descriptor
status 1
bash: line 4: echo: write error: Bad file descriptor
status 1
out
status 1
bash: line 8: read: 0: read error: Bad file descriptor
status 1 x=
bash: line 10: printf: write error: Bad file descriptor
status 1
closed-unopened ok
status 0

== status 0
