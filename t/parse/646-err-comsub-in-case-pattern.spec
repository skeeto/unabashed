== stdout 2
a

== stderr 147
bash: -c: line 2: syntax error near unexpected token `done' while looking for matching `)'
bash: -c: line 2: `f() { case x in $(done)) ;; esac; }'

== merged 149
a
bash: -c: line 2: syntax error near unexpected token `done' while looking for matching `)'
bash: -c: line 2: `f() { case x in $(done)) ;; esac; }'

== status 127
