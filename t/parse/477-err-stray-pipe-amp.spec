== stdout 2
a

== stderr 98
bash: -c: line 2: syntax error near unexpected token `|&'
bash: -c: line 2: `echo b |& |& echo c'

== merged 100
a
bash: -c: line 2: syntax error near unexpected token `|&'
bash: -c: line 2: `echo b |& |& echo c'

== status 2
