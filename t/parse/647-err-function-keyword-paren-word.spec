== stdout 2
a

== stderr 99
bash: -c: line 2: syntax error near unexpected token `{'
bash: -c: line 2: `function f (x) { :; }'

== merged 101
a
bash: -c: line 2: syntax error near unexpected token `{'
bash: -c: line 2: `function f (x) { :; }'

== status 2
