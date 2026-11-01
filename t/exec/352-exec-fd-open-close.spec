== stdout 70
status 1
a=line1 b=line2
status 1 c=
status 1
> line1
> line2
> line3

== stderr 106
bash: line 6: 3: Bad file descriptor
bash: line 15: read: 3: invalid file descriptor: Bad file descriptor

== merged 176
bash: line 6: 3: Bad file descriptor
status 1
a=line1 b=line2
status 1 c=
bash: line 15: read: 3: invalid file descriptor: Bad file descriptor
status 1
> line1
> line2
> line3

== status 0
