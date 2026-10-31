== stdout 7
before

== stderr 94
bash: -c: line 2: syntax error near unexpected token `!'
bash: -c: line 2: `f() { a | ! b; }'

== merged 101
before
bash: -c: line 2: syntax error near unexpected token `!'
bash: -c: line 2: `f() { a | ! b; }'

== status 2
