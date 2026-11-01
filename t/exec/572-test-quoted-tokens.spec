== stdout 262
0 <- test ( = (
0 <- test ) = )
0 <- test ! = !
0 <- test -a = -a
0 <- test -o = -o
0 <- test -n = -n
1 <- test ! = x
0 <- test ( != )
0 <- test ] = ]
0 <- [ ] = ] ]
0 <- [ ( = ( ]
0 <- [ ! ]
0 <- [ ( ]
0 <- [ ) ]
0 <- [ -a ]
0 <- test -e -a -e
0 <- test ! -a !

== stderr 0

== merged 262
0 <- test ( = (
0 <- test ) = )
0 <- test ! = !
0 <- test -a = -a
0 <- test -o = -o
0 <- test -n = -n
1 <- test ! = x
0 <- test ( != )
0 <- test ] = ]
0 <- [ ] = ] ]
0 <- [ ( = ( ]
0 <- [ ! ]
0 <- [ ( ]
0 <- [ ) ]
0 <- [ -a ]
0 <- test -e -a -e
0 <- test ! -a !

== status 0
