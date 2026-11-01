== stdout 110
n=2+3
declare -- e="1"
u=lower
declare -l il="7"
q=B
declare -- n="2+3"
declare -- u="lower"
declare -- q="B"

== stderr 0

== merged 110
n=2+3
declare -- e="1"
u=lower
declare -l il="7"
q=B
declare -- n="2+3"
declare -- u="lower"
declare -- q="B"

== status 0
