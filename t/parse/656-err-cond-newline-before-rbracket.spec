== stdout 2
a

== stderr 154
bash: -c: line 2: unexpected token `newline', conditional binary operator expected
bash: -c: line 2: syntax error near `a'
bash: -c: line 2: `f() { [[ a'

== merged 156
a
bash: -c: line 2: unexpected token `newline', conditional binary operator expected
bash: -c: line 2: syntax error near `a'
bash: -c: line 2: `f() { [[ a'

== status 2
