== stdout 118
sub: local-x arg1 f
y=changed x=local-x
in sub: subshell
after sub: local-x
z=inner x=local-x
x=[unset]
q=1
q=[unset]

== stderr 0

== merged 118
sub: local-x arg1 f
y=changed x=local-x
in sub: subshell
after sub: local-x
z=inner x=local-x
x=[unset]
q=1
q=[unset]

== status 0
