== stdout 426
1 <- test -t 0
1 <- test -t 1
1 <- test -t 2
1 <- test -t 99
0 <- test -t
2 <- test -t x
1 <- test -t -1
1 <- test -t 0 -a x
0 <- test -t -a x
0 <- test -t -o x
127 <- [[ -t 0 ]]
0 <- test -O /tmp/f
0 <- test -G /tmp/f
1 <- test -N /tmp/f
1 <- test -u /tmp/f
0 <- test -k /tmp
0 <- test -c /dev/null
1 <- test -O /
1 <- test -w /
0 <- test -w /home/oracle
0 <- test -e /dev/stdin
0 <- test -e /dev/fd/0
1 <- test -e /dev/fd/7

== stderr 76
bash: line 1: test: x: integer expected
bash: line 1: [[: command not found

== merged 502
1 <- test -t 0
1 <- test -t 1
1 <- test -t 2
1 <- test -t 99
0 <- test -t
bash: line 1: test: x: integer expected
2 <- test -t x
1 <- test -t -1
1 <- test -t 0 -a x
0 <- test -t -a x
0 <- test -t -o x
bash: line 1: [[: command not found
127 <- [[ -t 0 ]]
0 <- test -O /tmp/f
0 <- test -G /tmp/f
1 <- test -N /tmp/f
1 <- test -u /tmp/f
0 <- test -k /tmp
0 <- test -c /dev/null
1 <- test -O /
1 <- test -w /
0 <- test -w /home/oracle
0 <- test -e /dev/stdin
0 <- test -e /dev/fd/0
1 <- test -e /dev/fd/7

== status 0
