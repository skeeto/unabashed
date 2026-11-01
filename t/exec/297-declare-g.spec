== stdout 148
in f: func-local global-from-f typeset-local
d=[unset] g=[global-from-f] t=[unset]
h sees: local
x=global-set
declare -i gi="5"
declare -x gx="exp"

== stderr 0

== merged 148
in f: func-local global-from-f typeset-local
d=[unset] g=[global-from-f] t=[unset]
h sees: local
x=global-set
declare -i gi="5"
declare -x gx="exp"

== status 0
