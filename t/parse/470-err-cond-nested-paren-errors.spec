== stdout 2
a

== stderr 219
bash: -c: line 2: unexpected token `b', conditional binary operator expected
bash: -c: line 2: expected `)'
bash: -c: line 2: expected `)'
bash: -c: line 2: syntax error near `b'
bash: -c: line 2: `[[ ( ! ( a b ) ) ]]'

== merged 221
a
bash: -c: line 2: unexpected token `b', conditional binary operator expected
bash: -c: line 2: expected `)'
bash: -c: line 2: expected `)'
bash: -c: line 2: syntax error near `b'
bash: -c: line 2: `[[ ( ! ( a b ) ) ]]'

== status 2
