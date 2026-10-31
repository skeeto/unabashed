== stdout 2
a

== stderr 182
bash: line 3: warning: here-document at line 2 delimited by end-of-file (wanted `A')
bash: -c: line 3: syntax error near unexpected token `fi'
bash: -c: line 3: `f() { :; } <<A; fi'

== merged 184
a
bash: line 3: warning: here-document at line 2 delimited by end-of-file (wanted `A')
bash: -c: line 3: syntax error near unexpected token `fi'
bash: -c: line 3: `f() { :; } <<A; fi'

== status 2
