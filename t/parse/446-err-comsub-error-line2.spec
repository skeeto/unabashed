== stdout 2
a

== stderr 130
bash: -c: line 2: syntax error near unexpected token `done' while looking for matching `)'
bash: -c: line 2: `echo $(done) never'

== merged 132
a
bash: -c: line 2: syntax error near unexpected token `done' while looking for matching `)'
bash: -c: line 2: `echo $(done) never'

== status 127
