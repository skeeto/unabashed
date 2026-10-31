== stdout 2
a

== stderr 105
bash: -c: line 2: syntax error near unexpected token `;;'
bash: -c: line 2: `f() { if a; then b;; fi; }'

== merged 107
a
bash: -c: line 2: syntax error near unexpected token `;;'
bash: -c: line 2: `f() { if a; then b;; fi; }'

== status 2
