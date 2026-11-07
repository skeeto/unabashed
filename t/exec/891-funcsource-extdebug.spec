== stdout 173
f: 1 [bash]
g: 1 [bash]
f: 2 [bash bash]
top: 0 []
k: [./lib.sh ./lib.sh]
k: [./lib.sh]
f
g
h
k
f 3 bash
g 5 bash
h 6 bash
k 2 ./lib.sh
status 1
declare -f f
declare -f g
k

== stderr 0

== merged 173
f: 1 [bash]
g: 1 [bash]
f: 2 [bash bash]
top: 0 []
k: [./lib.sh ./lib.sh]
k: [./lib.sh]
f
g
h
k
f 3 bash
g 5 bash
h 6 bash
k 2 ./lib.sh
status 1
declare -f f
declare -f g
k

== status 0
