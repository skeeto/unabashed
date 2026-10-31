== stdout 2
a

== stderr 127
bash: -c: line 2: syntax error near unexpected token `)' while looking for matching `}'
bash: -c: line 2: `f() { x=${| ) }; }'

== merged 129
a
bash: -c: line 2: syntax error near unexpected token `)' while looking for matching `}'
bash: -c: line 2: `f() { x=${| ) }; }'

== status 127
