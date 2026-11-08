== stdout 73
$ echo one
one
$ echo two
two
$ exit
---
one
two
one
one
---
a
b
c
b
c
b

== stderr 0

== merged 73
$ echo one
one
$ echo two
two
$ exit
---
one
two
one
one
---
a
b
c
b
c
b

== status 0
