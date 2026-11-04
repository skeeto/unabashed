== stdout 81
1
2
3
1 0
2 4
3 7
a
4 9
5 b 11
c
6
d
7
8 3 3
9 4 4
10 0 0
11 6 5 6
12 7 7
13 2 2

== stderr 283
t 0 true
t 0 true
t 0 true
t 0 echo "2 $n"
t 0 f
t 0 cat
t 0 echo "3 $n"
t 0 echo a
t 0 echo "4 $n"
t 0 x=$(echo b | cat)
t 0 echo "5 $x $n"
t 0 trap - DEBUG
s 0 echo c
s 1 cat
s 0 echo "6"
s 0 shopt -s lastpipe
s 0 echo d
s 0 cat
s 0 echo "7"
s 0 shopt -u lastpipe
s 0 trap - DEBUG

== merged 364
1
2
3
1 0
t 0 true
t 0 true
t 0 true
t 0 echo "2 $n"
2 4
t 0 f
t 0 cat
t 0 echo "3 $n"
3 7
t 0 echo a
a
t 0 echo "4 $n"
4 9
t 0 x=$(echo b | cat)
t 0 echo "5 $x $n"
5 b 11
t 0 trap - DEBUG
s 0 echo c
s 1 cat
c
s 0 echo "6"
6
s 0 shopt -s lastpipe
s 0 echo d
s 0 cat
d
s 0 echo "7"
7
s 0 shopt -u lastpipe
s 0 trap - DEBUG
8 3 3
9 4 4
10 0 0
11 6 5 6
12 7 7
13 2 2

== status 0
