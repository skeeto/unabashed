== stdout 2
a

== stderr 128
bash: -c: line 2: syntax error near unexpected token `fi' while looking for matching `)'
bash: -c: line 2: `f() { cat <(fi); }'

== merged 130
a
bash: -c: line 2: syntax error near unexpected token `fi' while looking for matching `)'
bash: -c: line 2: `f() { cat <(fi); }'

== status 127
