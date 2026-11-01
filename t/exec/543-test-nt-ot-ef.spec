== stdout 554
0 <- test /tmp/a -nt /tmp/missing
1 <- test /tmp/missing -nt /tmp/a
1 <- test /tmp/a -ot /tmp/missing
0 <- test /tmp/missing -ot /tmp/a
1 <- test /tmp/missing -nt /tmp/missing2
1 <- test /tmp/missing -ot /tmp/missing2
1 <- test /tmp/a -nt /tmp/a
1 <- test /tmp/a -ot /tmp/a
0 <- test /tmp/a -ef /tmp/a
0 <- test /tmp/a -ef /tmp/./a
1 <- test /tmp/a -ef /tmp/missing
1 <- test /tmp/missing -ef /tmp/missing
0 <- test /tmp -ef /tmp/
0 <- test a -ef /tmp/a
0 <- [ a -nt missing ]
127 <- [[ a -nt missing ]]
127 <- [[ missing -ot a ]]
127 <- [[ a -ef ./a ]]

== stderr 108
bash: line 1: [[: command not found
bash: line 1: [[: command not found
bash: line 1: [[: command not found

== merged 662
0 <- test /tmp/a -nt /tmp/missing
1 <- test /tmp/missing -nt /tmp/a
1 <- test /tmp/a -ot /tmp/missing
0 <- test /tmp/missing -ot /tmp/a
1 <- test /tmp/missing -nt /tmp/missing2
1 <- test /tmp/missing -ot /tmp/missing2
1 <- test /tmp/a -nt /tmp/a
1 <- test /tmp/a -ot /tmp/a
0 <- test /tmp/a -ef /tmp/a
0 <- test /tmp/a -ef /tmp/./a
1 <- test /tmp/a -ef /tmp/missing
1 <- test /tmp/missing -ef /tmp/missing
0 <- test /tmp -ef /tmp/
0 <- test a -ef /tmp/a
0 <- [ a -nt missing ]
bash: line 1: [[: command not found
127 <- [[ a -nt missing ]]
bash: line 1: [[: command not found
127 <- [[ missing -ot a ]]
bash: line 1: [[: command not found
127 <- [[ a -ef ./a ]]

== status 0
