== stdout 176
f: declared typeset 2
g sees: declared typeset 2
top: top [unset] [unset]
declare -- x="top"
declare -- x="l"
---
declare -- m1="1"
declare -- m2="two words"
declare -i m3="3"

== stderr 0

== merged 176
f: declared typeset 2
g sees: declared typeset 2
top: top [unset] [unset]
declare -- x="top"
declare -- x="l"
---
declare -- m1="1"
declare -- m2="two words"
declare -i m3="3"

== status 0
