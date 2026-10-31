== stdout 2
a

== stderr 110
bash: -c: line 2: syntax error near unexpected token `('
bash: -c: line 2: `f() { case x in ((a)) ;; esac; }'

== merged 112
a
bash: -c: line 2: syntax error near unexpected token `('
bash: -c: line 2: `f() { case x in ((a)) ;; esac; }'

== status 2
