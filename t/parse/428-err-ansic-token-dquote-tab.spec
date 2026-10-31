== stdout 2
a

== stderr 102
bash: -c: line 2: syntax error near unexpected token `$'"a\tb"''
bash: -c: line 2: `f() { :; } "a	b"'

== merged 104
a
bash: -c: line 2: syntax error near unexpected token `$'"a\tb"''
bash: -c: line 2: `f() { :; } "a	b"'

== status 2
