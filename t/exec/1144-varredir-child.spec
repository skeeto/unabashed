== stdout 301
declare -- x="7"
1
declare -x e="1"
declare -i i="3"
declare -a a=([0]="1" [1]="2")
status 1
declare -- x="10"
in 10
out 4
10
1
declare -i i="10"
11
declare -i i="3"
declare -a a=([0]="1" [1]="10")
declare -a a=([0]="1" [1]="2")
declare -a b=([3]="10")
status 3
z=10
w=11
v=12
u=13
t2=14
k2=unset
six

== stderr 493
bash: line 7: declare: x: not found
bash: line 8: declare: x: not found
bash: line 9: declare: x: not found
bash: line 10: declare: x: not found
bash: line 11: nosuch: command not found
bash: line 11: declare: x: not found
bash: line 12: declare: x: not found
bash: line 17: declare: t: not found
bash: line 18: ro: readonly variable
bash: line 18: ro: cannot assign fd to variable
bash: line 20: declare: x: not found
bash: line 25: declare: b: not found
bash: line 27: declare: y: not found

== merged 794
bash: line 7: declare: x: not found
bash: line 8: declare: x: not found
bash: line 9: declare: x: not found
bash: line 10: declare: x: not found
bash: line 11: nosuch: command not found
bash: line 11: declare: x: not found
bash: line 12: declare: x: not found
declare -- x="7"
1
declare -x e="1"
declare -i i="3"
declare -a a=([0]="1" [1]="2")
bash: line 17: declare: t: not found
bash: line 18: ro: readonly variable
bash: line 18: ro: cannot assign fd to variable
status 1
declare -- x="10"
bash: line 20: declare: x: not found
in 10
out 4
10
1
declare -i i="10"
11
declare -i i="3"
declare -a a=([0]="1" [1]="10")
declare -a a=([0]="1" [1]="2")
declare -a b=([3]="10")
bash: line 25: declare: b: not found
status 3
bash: line 27: declare: y: not found
z=10
w=11
v=12
u=13
t2=14
k2=unset
six

== status 0
