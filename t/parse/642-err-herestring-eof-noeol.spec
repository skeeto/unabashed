== stdout 2
a

== stderr 97
bash: -c: line 2: syntax error near unexpected token `newline'
bash: -c: line 2: `f() { cat <<<'

== merged 99
a
bash: -c: line 2: syntax error near unexpected token `newline'
bash: -c: line 2: `f() { cat <<<'

== status 2
