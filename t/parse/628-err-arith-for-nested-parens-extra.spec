== stdout 2
a

== stderr 134
bash: -c: line 2: syntax error near unexpected token `)'
bash: -c: line 2: `f() { for (( i = (1) ; (i < (3)) ; i++ ))) do :; done; }'

== merged 136
a
bash: -c: line 2: syntax error near unexpected token `)'
bash: -c: line 2: `f() { for (( i = (1) ; (i < (3)) ; i++ ))) do :; done; }'

== status 2
