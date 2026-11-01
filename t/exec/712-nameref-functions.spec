== stdout 124
one two
in: set
out: outer
inside
[]
3
declare -n ref="var"
bash: line 15: declare: var: not found
declare -a foo=([0]="X")

== stderr 346
bash: line 6: typeset: warning: v: circular name reference
bash: line 6: warning: v: circular name reference
bash: line 6: warning: v: maximum nameref depth (8) exceeded
bash: line 9: warning: p: circular name reference
bash: line 10: warning: p: circular name reference
bash: line 12: declare: self: nameref variable self references not allowed

== merged 470
one two
in: set
out: outer
bash: line 6: typeset: warning: v: circular name reference
bash: line 6: warning: v: circular name reference
bash: line 6: warning: v: maximum nameref depth (8) exceeded
inside
bash: line 9: warning: p: circular name reference
bash: line 10: warning: p: circular name reference
[]
bash: line 12: declare: self: nameref variable self references not allowed
3
declare -n ref="var"
bash: line 15: declare: var: not found
declare -a foo=([0]="X")

== status 0
