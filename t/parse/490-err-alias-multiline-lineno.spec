== stdout 9
before
a

== stderr 88
bash: -c: line 8: syntax error near unexpected token `fi'
bash: -c: line 8: `echo a
f '

== merged 97
before
a
bash: -c: line 8: syntax error near unexpected token `fi'
bash: -c: line 8: `echo a
f '

== status 2
