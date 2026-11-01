== stdout 501
0 <- test 1 -eq 1
1 <- test 1 -eq 2
0 <- test 1 -ne 2
0 <- test -3 -lt -2
0 <- test 5 -le 5
0 <- test 6 -gt 5
1 <- test 4 -ge 5
0 <- test 007 -eq 7
0 <- test +3 -eq 3
0 <- test -0 -eq 0
0 <- test  12  -eq 12
0 <- test 9223372036854775807 -gt 0
0 <- test -9223372036854775808 -lt 0
0 <- [ 10 -gt 9 ]
1 <- [ 10 > 9 ]
2 <- test 1 -eq x
2 <- test x -eq 1
2 <- test  -eq 0
2 <- test 3x -lt 4
2 <- test 0x10 -eq 16
2 <- test 9223372036854775808 -gt 0
2 <- test 1+1 -eq 2
2 <- test 1 -eq
2 <- test 1.5 -lt 2

== stderr 392
bash: line 1: test: x: integer expected
bash: line 1: test: x: integer expected
bash: line 1: test: : integer expected
bash: line 1: test: 3x: integer expected
bash: line 1: test: 0x10: integer expected
bash: line 1: test: 9223372036854775808: integer expected
bash: line 1: test: 1+1: integer expected
bash: line 1: test: 1: unary operator expected
bash: line 1: test: 1.5: integer expected

== merged 893
0 <- test 1 -eq 1
1 <- test 1 -eq 2
0 <- test 1 -ne 2
0 <- test -3 -lt -2
0 <- test 5 -le 5
0 <- test 6 -gt 5
1 <- test 4 -ge 5
0 <- test 007 -eq 7
0 <- test +3 -eq 3
0 <- test -0 -eq 0
0 <- test  12  -eq 12
0 <- test 9223372036854775807 -gt 0
0 <- test -9223372036854775808 -lt 0
0 <- [ 10 -gt 9 ]
1 <- [ 10 > 9 ]
bash: line 1: test: x: integer expected
2 <- test 1 -eq x
bash: line 1: test: x: integer expected
2 <- test x -eq 1
bash: line 1: test: : integer expected
2 <- test  -eq 0
bash: line 1: test: 3x: integer expected
2 <- test 3x -lt 4
bash: line 1: test: 0x10: integer expected
2 <- test 0x10 -eq 16
bash: line 1: test: 9223372036854775808: integer expected
2 <- test 9223372036854775808 -gt 0
bash: line 1: test: 1+1: integer expected
2 <- test 1+1 -eq 2
bash: line 1: test: 1: unary operator expected
2 <- test 1 -eq
bash: line 1: test: 1.5: integer expected
2 <- test 1.5 -lt 2

== status 0
