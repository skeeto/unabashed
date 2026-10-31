== stdout 13
before
after

== stderr 96
bash: -c: line 2: syntax error near unexpected token `('
bash: -c: line 2: `f() { a=( ( ) ); }'

== merged 109
before
bash: -c: line 2: syntax error near unexpected token `('
bash: -c: line 2: `f() { a=( ( ) ); }'
after

== status 0
