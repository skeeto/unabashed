== stdout 366
0 <- test ( x )
1 <- test (  )
0 <- test ( -n x )
1 <- test ( -z x )
0 <- test ( a = a )
1 <- test ( a = b )
0 <- test ! ( a = b )
0 <- test ( a = b ) -o ( c = c )
0 <- test (  -o x ) -a ( y )
2 <- test ( ( x ) )
0 <- test ( ( ( a = a ) ) )
0 <- test ( -n x -a -z  ) -a x
0 <- [ ( 1 -eq 1 ) ]
0 <- [ ! ( ! a ) ]
2 <- test ( )
0 <- test ( ( )
0 <- test )
0 <- test (

== stderr 94
bash: line 1: test: (: unary operator expected
bash: line 1: test: (: unary operator expected

== merged 460
0 <- test ( x )
1 <- test (  )
0 <- test ( -n x )
1 <- test ( -z x )
0 <- test ( a = a )
1 <- test ( a = b )
0 <- test ! ( a = b )
0 <- test ( a = b ) -o ( c = c )
0 <- test (  -o x ) -a ( y )
bash: line 1: test: (: unary operator expected
2 <- test ( ( x ) )
0 <- test ( ( ( a = a ) ) )
0 <- test ( -n x -a -z  ) -a x
0 <- [ ( 1 -eq 1 ) ]
0 <- [ ! ( ! a ) ]
bash: line 1: test: (: unary operator expected
2 <- test ( )
0 <- test ( ( )
0 <- test )
0 <- test (

== status 0
