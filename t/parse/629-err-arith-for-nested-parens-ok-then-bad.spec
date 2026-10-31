== stdout 2
a

== stderr 136
bash: -c: line 2: syntax error near unexpected token `fi'
bash: -c: line 2: `f() { for (( i = (1 + 2) * (3) ; ((i)) < 3 ; i++ )) fi; }'

== merged 138
a
bash: -c: line 2: syntax error near unexpected token `fi'
bash: -c: line 2: `f() { for (( i = (1 + 2) * (3) ; ((i)) < 3 ; i++ )) fi; }'

== status 2
