== stdout 153
x in f: [unset] [empty]
declare -- x
x now: set
x after: global
u: [unset]
-v u: 1
x: []
declare -- x=""
declare -- v="1"
declare -- w
declare -- v2="2"

== stderr 0

== merged 153
x in f: [unset] [empty]
declare -- x
x now: set
x after: global
u: [unset]
-v u: 1
x: []
declare -- x=""
declare -- v="1"
declare -- w
declare -- v2="2"

== status 0
