== stdout 77
outer
[inner1]
[inner2]
status 1
[three]
x=err
sub read inner1
parent inner1

== stderr 37
bash: line 7: 3: Bad file descriptor

== merged 114
outer
[inner1]
[inner2]
bash: line 7: 3: Bad file descriptor
status 1
[three]
x=err
sub read inner1
parent inner1

== status 0
