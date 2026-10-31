== stdout 13
before
after

== stderr 131
bash: -c: line 2: syntax error near unexpected token `;' while looking for matching `)'
bash: -c: line 2: `echo $(a=(1 ; 2)) same'

== merged 144
before
bash: -c: line 2: syntax error near unexpected token `;' while looking for matching `)'
bash: -c: line 2: `echo $(a=(1 ; 2)) same'
after

== status 0
