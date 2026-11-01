== stdout 257
0 <- command test a = a
1 <- builtin test a = b
0 <- builtin [ x ]
1 <- command [  ]
0 <- [ 1 -lt 2 ]
1 <- eval [ 1 -gt 2 ]
redir 0
redir2 0
redir3 2
err: bash: line 11: [: 1: unary operator expected
bash: line 13: test: x: unary operator expected
redir4 2

== stderr 97
bash: -c: line 14: syntax error near unexpected token `('
bash: -c: line 14: `type test [ [[ (('

== merged 354
0 <- command test a = a
1 <- builtin test a = b
0 <- builtin [ x ]
1 <- command [  ]
0 <- [ 1 -lt 2 ]
1 <- eval [ 1 -gt 2 ]
redir 0
redir2 0
redir3 2
err: bash: line 11: [: 1: unary operator expected
bash: line 13: test: x: unary operator expected
redir4 2
bash: -c: line 14: syntax error near unexpected token `('
bash: -c: line 14: `type test [ [[ (('

== status 2
