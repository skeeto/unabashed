== stdout 2
a

== stderr 130
bash: -c: line 2: syntax error near unexpected token `fi' while looking for matching `)'
bash: -c: line 2: `f() { [[ $(fi) ]]; }'

== merged 132
a
bash: -c: line 2: syntax error near unexpected token `fi' while looking for matching `)'
bash: -c: line 2: `f() { [[ $(fi) ]]; }'

== status 127
