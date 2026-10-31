== stdout 6
start

== stderr 233
bash: -c: line 3: syntax error near unexpected token `;' while looking for matching `)'
bash: -c: line 3: `    x=$(a=(;))'
bash: -c: line 4: syntax error near unexpected token `}' while looking for matching `)'
bash: -c: line 4: `}'

== merged 239
start
bash: -c: line 3: syntax error near unexpected token `;' while looking for matching `)'
bash: -c: line 3: `    x=$(a=(;))'
bash: -c: line 4: syntax error near unexpected token `}' while looking for matching `)'
bash: -c: line 4: `}'

== status 2
