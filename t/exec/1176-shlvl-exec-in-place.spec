== stdout 183
top 1
1
c2 2
c3 3
1
1
2
2
2
2
1
2
2
1
paren 0 0 1
redir 1 0
0
0
0
0
1
0
1
t
0
0
0
1
1
0
1
pipecs 1
-- exec
1
1
1
0 1
0
8
-- values
1
9
0
1
0
6
0
6
1
2
5
1
-- script
script 2
script 2

== stderr 106
bash: line 1: warning: shell level (1000) too high, resetting to 1
bash: line 1: SHLVL: readonly variable

== merged 289
top 1
1
c2 2
c3 3
1
1
2
2
2
2
1
2
2
1
paren 0 0 1
redir 1 0
0
0
0
0
1
0
1
t
0
0
0
1
1
0
1
pipecs 1
-- exec
1
1
1
0 1
0
8
-- values
1
9
0
1
0
6
0
6
bash: line 1: warning: shell level (1000) too high, resetting to 1
1
bash: line 1: SHLVL: readonly variable
2
5
1
-- script
script 2
script 2

== status 0
