== stdout 7
before

== stderr 104
bash: -c: line 4: syntax error near unexpected token `;'
bash: -c: line 4: `f() { case x in a) ;; E; }'

== merged 111
before
bash: -c: line 4: syntax error near unexpected token `;'
bash: -c: line 4: `f() { case x in a) ;; E; }'

== status 2
