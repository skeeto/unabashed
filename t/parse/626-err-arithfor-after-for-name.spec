== stdout 2
a

== stderr 114
bash: -c: line 2: syntax error near unexpected token `('
bash: -c: line 2: `f() { for x ((a;b;c)); do :; done; }'

== merged 116
a
bash: -c: line 2: syntax error near unexpected token `('
bash: -c: line 2: `f() { for x ((a;b;c)); do :; done; }'

== status 2
