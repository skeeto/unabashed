== stdout 98
in: 2
out: 1
in: unset
out: 1
b: unset
declare -x a="1"
declare -- a="1"
a now 3
n=3
n=1+2
deep
3

== stderr 0

== merged 98
in: 2
out: 1
in: unset
out: 1
b: unset
declare -x a="1"
declare -- a="1"
a now 3
n=3
n=1+2
deep
3

== status 0
