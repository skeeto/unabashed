== stdout 13
before
after

== stderr 98
bash: -c: line 2: syntax error near unexpected token `;'
bash: -c: line 2: `a=(1 ; 2); echo same'

== merged 111
before
bash: -c: line 2: syntax error near unexpected token `;'
bash: -c: line 2: `a=(1 ; 2); echo same'
after

== status 0
