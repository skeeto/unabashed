== stdout 582
## echo hi | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok
## { echo out; echo err >&2; } 2>&1 | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok
## echo hi | cat | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok
## printf "a\nb\n" | while read -r x; do echo "<$x>"; done | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok
## echo hi | { read -r x; echo "$x"; } | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok

== stderr 0

== merged 582
## echo hi | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok
## { echo out; echo err >&2; } 2>&1 | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok
## echo hi | cat | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok
## printf "a\nb\n" | while read -r x; do echo "<$x>"; done | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok
## echo hi | { read -r x; echo "$x"; } | cat
4 ok
5 ok
6 ok
7 ok
8 ok
9 ok
10 ok
11 ok
12 ok
13 ok
14 ok
16 ok
20 ok
24 ok

== status 0
