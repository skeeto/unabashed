== stdout 5
next

== stderr 185
bash: -c: line 1: syntax error near unexpected token `;'
bash: -c: line 1: `if a=(1 ; 2); then :; fi'
bash: -c: line 4: syntax error: unexpected end of file from `if' command on line 1

== merged 190
bash: -c: line 1: syntax error near unexpected token `;'
bash: -c: line 1: `if a=(1 ; 2); then :; fi'
next
bash: -c: line 4: syntax error: unexpected end of file from `if' command on line 1

== status 2
