== stdout 14
one
two
three

== stderr 115
bash: -c: line 3: syntax error near unexpected token `)'
bash: -c: line 3: `echo four; ( echo five ) ) ; echo six'

== merged 129
one
two
three
bash: -c: line 3: syntax error near unexpected token `)'
bash: -c: line 3: `echo four; ( echo five ) ) ; echo six'

== status 2
