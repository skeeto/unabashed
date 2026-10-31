== stdout 6
start

== stderr 110
bash: -c: line 3: syntax error near unexpected token `)'
bash: -c: line 3: `    echo $(case x in x) echo;; )'

== merged 116
start
bash: -c: line 3: syntax error near unexpected token `)'
bash: -c: line 3: `    echo $(case x in x) echo;; )'

== status 127
