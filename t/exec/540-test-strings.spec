== stdout 430
0 <- test -z 
1 <- test -z x
1 <- test -n 
0 <- test -n x
0 <- test abc
1 <- test 
0 <- test abc = abc
1 <- test abc = abd
0 <- test abc == abc
0 <- test abc != abd
1 <- test abc != abc
0 <- test  = 
0 <- test a < b
1 <- test b < a
0 <- test B < a
1 <- test abc > abd
0 <- test abcd > abc
0 <- [ x = x ]
1 <- [ x != x ]
0 <- [ a b = a b ]
0 <- [ * = * ]
1 <- [ * = x ]
1 <- [ -n  ]
0 <- [ -z  ]
0 <- [ = ]
0 <- [ -n ]
0 <- [ -z ]

== stderr 0

== merged 430
0 <- test -z 
1 <- test -z x
1 <- test -n 
0 <- test -n x
0 <- test abc
1 <- test 
0 <- test abc = abc
1 <- test abc = abd
0 <- test abc == abc
0 <- test abc != abd
1 <- test abc != abc
0 <- test  = 
0 <- test a < b
1 <- test b < a
0 <- test B < a
1 <- test abc > abd
0 <- test abcd > abc
0 <- [ x = x ]
1 <- [ x != x ]
0 <- [ a b = a b ]
0 <- [ * = * ]
1 <- [ * = x ]
1 <- [ -n  ]
0 <- [ -z  ]
0 <- [ = ]
0 <- [ -n ]
0 <- [ -z ]

== status 0
