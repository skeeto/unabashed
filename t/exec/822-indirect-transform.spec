== stdout 571
1 Z Z |
bash: line 6: x: invalid indirect expansion
bash: line 7: : invalid variable name
4 dflt
bash: line 9: nosuch[3]: invalid indirect expansion
6 |
7 0 'aaa' 0 1
bash: line 13: aaa bbb: invalid variable name
9 aaa bb a bbb
10 ||||
11 declare -i w|declare -i w|i|i|declare -i w
12 declare -a e|declare -a e|a|a|declare -a e
13 declare -A h|declare -A h|A|A|A
14 ar ar|ar|declare -ar ra
15 |||s='1'
16 declare -rl VAR1|rl
17 a declare -a n
bash: line 24: m: unbound variable
bash: line 25: k: unbound variable
bash: line 26: n: unbound variable
21 / -a x/a /
22 a xxa

== stderr 0

== merged 571
1 Z Z |
bash: line 6: x: invalid indirect expansion
bash: line 7: : invalid variable name
4 dflt
bash: line 9: nosuch[3]: invalid indirect expansion
6 |
7 0 'aaa' 0 1
bash: line 13: aaa bbb: invalid variable name
9 aaa bb a bbb
10 ||||
11 declare -i w|declare -i w|i|i|declare -i w
12 declare -a e|declare -a e|a|a|declare -a e
13 declare -A h|declare -A h|A|A|A
14 ar ar|ar|declare -ar ra
15 |||s='1'
16 declare -rl VAR1|rl
17 a declare -a n
bash: line 24: m: unbound variable
bash: line 25: k: unbound variable
bash: line 26: n: unbound variable
21 / -a x/a /
22 a xxa

== status 0
