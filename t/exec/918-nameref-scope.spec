== stdout 375
g: x=g-local r=g-local
f: x=via-ref r=via-ref
top via-ref
g2: x=g2 r=g2
f2: x=via-ref2
top via-ref
g3: x=unset
top via-ref
g4: x=g4 r=loc
f4 x=back
bash: line 20: typeset: zz: readonly variable
s=1
bash: line 20: zz: readonly variable
1 g
2 g
3 g
4 g
5 g
6 g0 A
7 n0 n1
8 n0 n1 more
declare -- x="g"
declare -a arr=([0]="n0" [1]="n1" [2]="more")
f: x=5 arr=f0 f1
k n=k
h n=5

== stderr 0

== merged 375
g: x=g-local r=g-local
f: x=via-ref r=via-ref
top via-ref
g2: x=g2 r=g2
f2: x=via-ref2
top via-ref
g3: x=unset
top via-ref
g4: x=g4 r=loc
f4 x=back
bash: line 20: typeset: zz: readonly variable
s=1
bash: line 20: zz: readonly variable
1 g
2 g
3 g
4 g
5 g
6 g0 A
7 n0 n1
8 n0 n1 more
declare -- x="g"
declare -a arr=([0]="n0" [1]="n1" [2]="more")
f: x=5 arr=f0 f1
k n=k
h n=5

== status 0
