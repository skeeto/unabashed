== stdout 2
a

== stderr 129
bash: -c: line 2: syntax error near unexpected token `;' while looking for matching `)'
bash: -c: line 2: `f() { echo $( ; ); }'

== merged 131
a
bash: -c: line 2: syntax error near unexpected token `;' while looking for matching `)'
bash: -c: line 2: `f() { echo $( ; ); }'

== status 127
