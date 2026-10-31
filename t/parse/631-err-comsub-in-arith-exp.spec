== stdout 2
a

== stderr 140
bash: -c: line 2: syntax error near unexpected token `fi' while looking for matching `)'
bash: -c: line 2: `f() { echo $(( $(fi) + 1 )); }'

== merged 142
a
bash: -c: line 2: syntax error near unexpected token `fi' while looking for matching `)'
bash: -c: line 2: `f() { echo $(( $(fi) + 1 )); }'

== status 127
