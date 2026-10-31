== stdout 2
a

== stderr 99
bash: -c: line 2: syntax error near unexpected token `1 + 2'
bash: -c: line 2: `[[ a ]] ((1 + 2))'

== merged 101
a
bash: -c: line 2: syntax error near unexpected token `1 + 2'
bash: -c: line 2: `[[ a ]] ((1 + 2))'

== status 2
