== stdout 6
after

== stderr 107
bash: -c: line 1: syntax error near unexpected token `&'
bash: -c: line 1: `f() { a=(1 & 2); }; echo same'

== merged 113
bash: -c: line 1: syntax error near unexpected token `&'
bash: -c: line 1: `f() { a=(1 & 2); }; echo same'
after

== status 0
