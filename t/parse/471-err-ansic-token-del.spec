== stdout 0

== stderr 114
bash: -c: line 1: syntax error near unexpected token `$'x\'\001\177\'y''
bash: -c: line 1: `f() { :; } x$'\177'y'

== merged 114
bash: -c: line 1: syntax error near unexpected token `$'x\'\001\177\'y''
bash: -c: line 1: `f() { :; } x$'\177'y'

== status 2
