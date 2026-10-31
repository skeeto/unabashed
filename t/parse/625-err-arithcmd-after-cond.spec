== stdout 2
a

== stderr 108
bash: -c: line 2: syntax error near unexpected token `x = 1'
bash: -c: line 2: `f() { [[ a ]] ((x = 1)); }'

== merged 110
a
bash: -c: line 2: syntax error near unexpected token `x = 1'
bash: -c: line 2: `f() { [[ a ]] ((x = 1)); }'

== status 2
