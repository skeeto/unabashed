== stdout 251
declare -i i="1"
declare -ir ir="2"
declare -ix ix="3"
declare -xl lx="lower"
declare -ru ur="UPPER"
declare -rx xr="4"
declare -t tr="5"
declare -ix all="6"
status 1
declare -i i="1"
declare -ir ir="2"
status 1
declare -- novalue
declare -i inovalue

== stderr 86
bash: line 10: declare: missing: not found
bash: line 12: declare: missing: not found

== merged 337
declare -i i="1"
declare -ir ir="2"
declare -ix ix="3"
declare -xl lx="lower"
declare -ru ur="UPPER"
declare -rx xr="4"
declare -t tr="5"
declare -ix all="6"
bash: line 10: declare: missing: not found
status 1
declare -i i="1"
bash: line 12: declare: missing: not found
declare -ir ir="2"
status 1
declare -- novalue
declare -i inovalue

== status 0
