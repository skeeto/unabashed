== stdout 151
status 1 ro=value
status 1
status 1
plus r status 1
declare -r ro="value"
declare -r dr="1"
declare -r dr="1"
declare -r ro="value"
declare -r novalue

== stderr 216
bash: line 2: unset: ro: cannot unset: readonly variable
bash: line 4: unset: ro: cannot unset: readonly variable
bash: line 6: unset: ro: cannot unset: readonly variable
bash: line 9: declare: dr: readonly variable

== merged 367
bash: line 2: unset: ro: cannot unset: readonly variable
status 1 ro=value
bash: line 4: unset: ro: cannot unset: readonly variable
status 1
bash: line 6: unset: ro: cannot unset: readonly variable
status 1
bash: line 9: declare: dr: readonly variable
plus r status 1
declare -r ro="value"
declare -r dr="1"
declare -r dr="1"
declare -r ro="value"
declare -r novalue

== status 0
