== stdout 155
declare -- BASH_TRAPSIG="10"
unexported
in trap 5
declare -rx BASH_TRAPSIG="5"
exit 5
declare -rx BASH_TRAPSIG="5"
comsub 5
declare -rx BASH_TRAPSIG="5"
5

== stderr 0

== merged 155
declare -- BASH_TRAPSIG="10"
unexported
in trap 5
declare -rx BASH_TRAPSIG="5"
exit 5
declare -rx BASH_TRAPSIG="5"
comsub 5
declare -rx BASH_TRAPSIG="5"
5

== status 0
