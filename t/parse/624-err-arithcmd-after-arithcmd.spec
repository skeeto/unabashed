== stdout 2
a

== stderr 108
bash: -c: line 2: syntax error near unexpected token `3 + 4'
bash: -c: line 2: `f() { ((1+2)) ((3 + 4)); }'

== merged 110
a
bash: -c: line 2: syntax error near unexpected token `3 + 4'
bash: -c: line 2: `f() { ((1+2)) ((3 + 4)); }'

== status 2
