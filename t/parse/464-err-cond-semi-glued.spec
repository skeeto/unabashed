== stdout 2
a

== stderr 148
bash: -c: line 2: unexpected argument `;' to conditional binary operator
bash: -c: line 2: syntax error near `;b'
bash: -c: line 2: `[[ a -eq;b ]]'

== merged 150
a
bash: -c: line 2: unexpected argument `;' to conditional binary operator
bash: -c: line 2: syntax error near `;b'
bash: -c: line 2: `[[ a -eq;b ]]'

== status 2
