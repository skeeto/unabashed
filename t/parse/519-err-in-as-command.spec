== stdout 7
before

== stderr 99
bash: -c: line 2: syntax error near unexpected token `in'
bash: -c: line 2: `f() { echo in; in; }'

== merged 106
before
bash: -c: line 2: syntax error near unexpected token `in'
bash: -c: line 2: `f() { echo in; in; }'

== status 2
