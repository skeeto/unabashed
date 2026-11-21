== stdout 341
1 12
1b 7
2 12
2b 7
12
3b 7
12
4b 7
f 12
5b 7
1
6 1
1
7 1
1
8 1
9 1
1c+=10
10 1 0
bash: line 20: R: readonly variable
11 0 0
bash: line 21: R: readonly variable
12 0 1
bash: line 22: R: readonly variable
0
13 0 0
+ c=0
+ Z=0
+ Z=1
+ : ''
bash: line 25: R: readonly variable
+ X=3
+ : ''
+ Y=a
+ Y=ab
+ printenv Y
ab
+ set +x
14 1 1
15 1 1 1

== stderr 0

== merged 341
1 12
1b 7
2 12
2b 7
12
3b 7
12
4b 7
f 12
5b 7
1
6 1
1
7 1
1
8 1
9 1
1c+=10
10 1 0
bash: line 20: R: readonly variable
11 0 0
bash: line 21: R: readonly variable
12 0 1
bash: line 22: R: readonly variable
0
13 0 0
+ c=0
+ Z=0
+ Z=1
+ : ''
bash: line 25: R: readonly variable
+ X=3
+ : ''
+ Y=a
+ Y=ab
+ printenv Y
ab
+ set +x
14 1 1
15 1 1 1

== status 0
