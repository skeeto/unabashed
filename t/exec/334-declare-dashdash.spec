== stdout 67
declare -- dd="1"
di=4
ld=-x
declare -x ed="3"
dd=[unset]
status 1

== stderr 60
bash: line 11: declare: `-notopt=1': not a valid identifier

== merged 127
declare -- dd="1"
di=4
ld=-x
declare -x ed="3"
dd=[unset]
bash: line 11: declare: `-notopt=1': not a valid identifier
status 1

== status 0
