== stdout 209
VAR=[x]
declare -x VAR="x"
after: [unset]
VAR=[tmp]
declare -x VAR="tmp"
after: orig
in mod: modified
after mod: orig
in loc: inner
after loc: orig
VAR=[orig]
declare -- VAR="orig"
A=1 B=2
A=[unset] B=[unset]

== stderr 0

== merged 209
VAR=[x]
declare -x VAR="x"
after: [unset]
VAR=[tmp]
declare -x VAR="tmp"
after: orig
in mod: modified
after mod: orig
in loc: inner
after loc: orig
VAR=[orig]
declare -- VAR="orig"
A=1 B=2
A=[unset] B=[unset]

== status 0
