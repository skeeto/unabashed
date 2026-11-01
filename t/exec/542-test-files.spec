== stdout 601
0 <- test -e /tmp/empty
1 <- test -e /tmp/nope
0 <- test -a /tmp/full
0 <- test -f /tmp/empty
1 <- test -f /tmp
0 <- test -d /tmp
1 <- test -d /tmp/full
1 <- test -s /tmp/empty
0 <- test -s /tmp/full
1 <- test -s /tmp/nope
0 <- test -r /tmp/full
0 <- test -w /tmp/full
1 <- test -x /tmp/full
0 <- test -x /tmp
1 <- test -r /tmp/nope
1 <- test -L /tmp/full
1 <- test -h /tmp/full
1 <- test -e 
1 <- test -d 
1 <- test -f /tmp/
0 <- test -d /tmp/
0 <- test -d /tmp/./
1 <- test -e /tmp/full/
1 <- test -e /tmp/full/x
0 <- test -f full
0 <- test -d .
0 <- test -d ..
0 <- [ -f empty ]
0 <- [ ! -f nope ]

== stderr 0

== merged 601
0 <- test -e /tmp/empty
1 <- test -e /tmp/nope
0 <- test -a /tmp/full
0 <- test -f /tmp/empty
1 <- test -f /tmp
0 <- test -d /tmp
1 <- test -d /tmp/full
1 <- test -s /tmp/empty
0 <- test -s /tmp/full
1 <- test -s /tmp/nope
0 <- test -r /tmp/full
0 <- test -w /tmp/full
1 <- test -x /tmp/full
0 <- test -x /tmp
1 <- test -r /tmp/nope
1 <- test -L /tmp/full
1 <- test -h /tmp/full
1 <- test -e 
1 <- test -d 
1 <- test -f /tmp/
0 <- test -d /tmp/
0 <- test -d /tmp/./
1 <- test -e /tmp/full/
1 <- test -e /tmp/full/x
0 <- test -f full
0 <- test -d .
0 <- test -d ..
0 <- [ -f empty ]
0 <- [ ! -f nope ]

== status 0
