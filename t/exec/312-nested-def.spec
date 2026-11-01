== stdout 199
status 127
outer ran
inner defined by outer
first version
second version
generated a
generated b
declare -f gen_a
declare -f gen_b
declare -f inner
declare -f mk
declare -f outer
declare -f redefine

== stderr 39
bash: line 5: inner: command not found

== merged 238
bash: line 5: inner: command not found
status 127
outer ran
inner defined by outer
first version
second version
generated a
generated b
declare -f gen_a
declare -f gen_b
declare -f inner
declare -f mk
declare -f outer
declare -f redefine

== status 0
