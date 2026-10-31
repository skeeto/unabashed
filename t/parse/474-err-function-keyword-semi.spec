== stdout 2
a

== stderr 97
bash: -c: line 2: syntax error near unexpected token `;'
bash: -c: line 2: `function f ; { :; }'

== merged 99
a
bash: -c: line 2: syntax error near unexpected token `;'
bash: -c: line 2: `function f ; { :; }'

== status 2
