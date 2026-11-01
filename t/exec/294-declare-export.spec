== stdout 160
declare -x ex="1"
declare -x e2="two"
declare -x e3="three"
declare -x e4
declare -- e2="two"
declare -- ex="1"
declare -- e5="five"
declare -x tx="t"
status 1

== stderr 60
bash: line 16: export: `bad name=1': not a valid identifier

== merged 220
declare -x ex="1"
declare -x e2="two"
declare -x e3="three"
declare -x e4
declare -- e2="two"
declare -- ex="1"
declare -- e5="five"
declare -x tx="t"
bash: line 16: export: `bad name=1': not a valid identifier
status 1

== status 0
