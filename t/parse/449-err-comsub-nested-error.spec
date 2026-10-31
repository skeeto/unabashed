== stdout 2
a

== stderr 134
bash: -c: line 2: syntax error near unexpected token `then' while looking for matching `)'
bash: -c: line 2: `echo "$(echo $(then))"'

== merged 136
a
bash: -c: line 2: syntax error near unexpected token `then' while looking for matching `)'
bash: -c: line 2: `echo "$(echo $(then))"'

== status 127
