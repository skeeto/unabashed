== stdout 531
0 <- test ! 
1 <- test ! x
0 <- test ! ! x
1 <- test ! ! 
1 <- test ! -n x
0 <- test ! -z x
0 <- test ! a = b
1 <- test ! a = a
0 <- test x -a y
1 <- test x -a 
0 <- test  -o y
1 <- test  -o 
0 <- test -n x -a -n y
1 <- test -n x -a -z y
0 <- test -z x -o -n y
0 <- test 1 -eq 1 -a 2 -eq 2 -a 3 -eq 3
0 <- test 1 -eq 1 -a 2 -eq 3 -o 4 -eq 4
1 <- test 1 -eq 2 -o 2 -eq 2 -a 3 -eq 4
0 <- test ! 1 -eq 2 -a ! 2 -eq 3
0 <- test !  -a ! 
1 <- test x -a y -a z -a 
0 <- test  -o  -o  -o z
0 <- [ a = a -a b = b ]
0 <- [ a = b -o b = b ]

== stderr 0

== merged 531
0 <- test ! 
1 <- test ! x
0 <- test ! ! x
1 <- test ! ! 
1 <- test ! -n x
0 <- test ! -z x
0 <- test ! a = b
1 <- test ! a = a
0 <- test x -a y
1 <- test x -a 
0 <- test  -o y
1 <- test  -o 
0 <- test -n x -a -n y
1 <- test -n x -a -z y
0 <- test -z x -o -n y
0 <- test 1 -eq 1 -a 2 -eq 2 -a 3 -eq 3
0 <- test 1 -eq 1 -a 2 -eq 3 -o 4 -eq 4
1 <- test 1 -eq 2 -o 2 -eq 2 -a 3 -eq 4
0 <- test ! 1 -eq 2 -a ! 2 -eq 3
0 <- test !  -a ! 
1 <- test x -a y -a z -a 
0 <- test  -o  -o  -o z
0 <- [ a = a -a b = b ]
0 <- [ a = b -o b = b ]

== status 0
