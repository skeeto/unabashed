== stdout 2
a

== stderr 148
bash: -c: line 2: unexpected token 283 in conditional command
bash: -c: line 2: syntax error near `{fd}>'
bash: -c: line 2: `f() { [[ {fd}>x ]]; }'

== merged 150
a
bash: -c: line 2: unexpected token 283 in conditional command
bash: -c: line 2: syntax error near `{fd}>'
bash: -c: line 2: `f() { [[ {fd}>x ]]; }'

== status 2
