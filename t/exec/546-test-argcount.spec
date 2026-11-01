== stdout 432
1 <- test
0 <- test -n
0 <- test -z
0 <- test !
0 <- test (
0 <- test -a
0 <- test -o
0 <- test -x
1 <- test ! !
1 <- test ! -n
1 <- test ! -z
0 <- test -n -n
1 <- test -z -z
2 <- test = =
2 <- test x =
2 <- test = x
0 <- test ! = !
0 <- test -a -a -a
0 <- test -o -o -o
0 <- test ! ! !
0 <- test ! -z -z
0 <- test -n = -n
2 <- test = = = =
1 <- test ! ! ! !
0 <- test ! x = y
0 <- test ( -n )
1 <- test ! ( x )
2 <- test a b c d e

== stderr 219
bash: line 1: test: =: unary operator expected
bash: line 1: test: x: unary operator expected
bash: line 1: test: =: unary operator expected
bash: line 1: test: too many arguments
bash: line 1: test: too many arguments

== merged 651
1 <- test
0 <- test -n
0 <- test -z
0 <- test !
0 <- test (
0 <- test -a
0 <- test -o
0 <- test -x
1 <- test ! !
1 <- test ! -n
1 <- test ! -z
0 <- test -n -n
1 <- test -z -z
bash: line 1: test: =: unary operator expected
2 <- test = =
bash: line 1: test: x: unary operator expected
2 <- test x =
bash: line 1: test: =: unary operator expected
2 <- test = x
0 <- test ! = !
0 <- test -a -a -a
0 <- test -o -o -o
0 <- test ! ! !
0 <- test ! -z -z
0 <- test -n = -n
bash: line 1: test: too many arguments
2 <- test = = = =
1 <- test ! ! ! !
0 <- test ! x = y
0 <- test ( -n )
1 <- test ! ( x )
bash: line 1: test: too many arguments
2 <- test a b c d e

== status 0
