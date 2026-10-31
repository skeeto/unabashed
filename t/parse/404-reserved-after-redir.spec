== stdout 4
mid

== stderr 112
bash: -c: line 7: syntax error near unexpected token `then'
bash: -c: line 7: `g() { >x if true; then :; fi; }'

== merged 116
mid
bash: -c: line 7: syntax error near unexpected token `then'
bash: -c: line 7: `g() { >x if true; then :; fi; }'

== status 2
