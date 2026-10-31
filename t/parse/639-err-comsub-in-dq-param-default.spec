== stdout 2
a

== stderr 137
bash: -c: line 2: syntax error near unexpected token `fi' while looking for matching `)'
bash: -c: line 2: `f() { echo "${x:-$(fi)}"; }'

== merged 139
a
bash: -c: line 2: syntax error near unexpected token `fi' while looking for matching `)'
bash: -c: line 2: `f() { echo "${x:-$(fi)}"; }'

== status 127
