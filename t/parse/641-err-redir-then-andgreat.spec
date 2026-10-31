== stdout 2
a

== stderr 91
bash: -c: line 2: syntax error near unexpected token `&>'
bash: -c: line 2: `echo b < &>c'

== merged 93
a
bash: -c: line 2: syntax error near unexpected token `&>'
bash: -c: line 2: `echo b < &>c'

== status 2
