== stdout 4
one

== stderr 103
bash: -c: line 2: syntax error near unexpected token `fi'
bash: -c: line 2: `echo two; echo three; fi'

== merged 107
one
bash: -c: line 2: syntax error near unexpected token `fi'
bash: -c: line 2: `echo two; echo three; fi'

== status 2
