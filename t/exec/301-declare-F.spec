== stdout 105
declare -f alpha
declare -f mid
declare -f zeta
alpha
status 0
status 1
alpha
zeta
status 1
status 1
mid

== stderr 0

== merged 105
declare -f alpha
declare -f mid
declare -f zeta
alpha
status 0
status 1
alpha
zeta
status 1
status 1
mid

== status 0
