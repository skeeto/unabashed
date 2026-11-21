== stdout 329
1: st 3
2: EXIT 3 hBc
st 3
3: bash: line 1: u: oops
${u?oops}+ echo a
a
bash: line 1: u: oops
${u?oops}+ set +x
c
st 0
4: st 4
5: st 5
6: + echo a
a
+ set +x
b
after 0
st 0
7: after 3
EXIT 0
st 0
8: st 0
9: st 3
10: st 3
11: st 3
12: st 3
13: EXIT 1
st 1
14: bash: line 1: u: oops
EXIT 1 ehBc
st 1
15: + echo a
a
+ echo 0
0
st 0

== stderr 0

== merged 329
1: st 3
2: EXIT 3 hBc
st 3
3: bash: line 1: u: oops
${u?oops}+ echo a
a
bash: line 1: u: oops
${u?oops}+ set +x
c
st 0
4: st 4
5: st 5
6: + echo a
a
+ set +x
b
after 0
st 0
7: after 3
EXIT 0
st 0
8: st 0
9: st 3
10: st 3
11: st 3
12: st 3
13: EXIT 1
st 1
14: bash: line 1: u: oops
EXIT 1 ehBc
st 1
15: + echo a
a
+ echo 0
0
st 0

== status 0
