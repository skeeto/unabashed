== stdout 2
a

== stderr 110
bash: -c: line 3: syntax error near `;'
bash: -c: line 3: `  for (( i = (1 + (2) ; i < 3; i++ )); do :; done'

== merged 112
a
bash: -c: line 3: syntax error near `;'
bash: -c: line 3: `  for (( i = (1 + (2) ; i < 3; i++ )); do :; done'

== status 2
