== stdout 842
pipe: 1 0 3 (3)
true: 0 (1)
false: 1 (1)
group: 1 (1)
func: 3 (1)
assign: 0 (1)
assigncomsub: 3 (1)
subshell: 4 (1)
if: 1 (1)
if2: 2 0 (2)
dbrack: 1 (1)
arith: 1 (1)
for: 1 (1)
forempty: 1 0 (2)
while: 1 (1)
case: 1 0 (2)
bang: 1 0 (2)
bang2: 0 (1)
andand: 1 (1)
oror: 5 0 (2)
nosuch: 127 (1)
eval: 3 (1)
funcpipe: 0 (1)
funcpipe2: 4 (1)
userassign: 0 (1)
declare -a PIPESTATUS=([0]="1" [1]="0")
1 0
1 0
1 0
afterunset: 1 0 (2)
redir: 0 (1)
redironly: 0 (1)
exitpipe: 3 1 (2)
last: 0 1 (2)
bg: 1 0 (2)
funcdef: 1 0 (2)
grpredir: 1 0 (2)
simpredir: 1 (1)
arithpipe: 1 0 (2)
11
arith2: 0 (1)
3 1
x=8
same: 0 (1)
declare: 0 (1)
ro: 1 2 (2)
3 1 0
3b 0 0
0
4 0 0 1 0
5 1
6 1
7 0
8 1 0
9 1 0
10 1
11 0
12 3
13 1 2
14 1 0
14b 0
4 0 0 1 0
declare -ar PIPESTATUS=([0]="1" [1]="2")
0 "1" 1 "2"
'1' '2'
ar ar
2
3
f declare -a FUNCNAME=([0]="f" [1]="g")

== stderr 277
bash: line 42: /nonexistent/x: No such file or directory
bash: line 43: /nonexistent/x: No such file or directory
bash: line 54: unset: PIPESTATUS: cannot unset: readonly variable
bash: line 55: PIPESTATUS: readonly variable
bash: line 63: local: PIPESTATUS: readonly variable

== merged 1119
pipe: 1 0 3 (3)
true: 0 (1)
false: 1 (1)
group: 1 (1)
func: 3 (1)
assign: 0 (1)
assigncomsub: 3 (1)
subshell: 4 (1)
if: 1 (1)
if2: 2 0 (2)
dbrack: 1 (1)
arith: 1 (1)
for: 1 (1)
forempty: 1 0 (2)
while: 1 (1)
case: 1 0 (2)
bang: 1 0 (2)
bang2: 0 (1)
andand: 1 (1)
oror: 5 0 (2)
nosuch: 127 (1)
eval: 3 (1)
funcpipe: 0 (1)
funcpipe2: 4 (1)
userassign: 0 (1)
declare -a PIPESTATUS=([0]="1" [1]="0")
1 0
1 0
1 0
afterunset: 1 0 (2)
redir: 0 (1)
redironly: 0 (1)
exitpipe: 3 1 (2)
last: 0 1 (2)
bg: 1 0 (2)
funcdef: 1 0 (2)
bash: line 42: /nonexistent/x: No such file or directory
grpredir: 1 0 (2)
bash: line 43: /nonexistent/x: No such file or directory
simpredir: 1 (1)
arithpipe: 1 0 (2)
11
arith2: 0 (1)
3 1
x=8
same: 0 (1)
declare: 0 (1)
ro: 1 2 (2)
3 1 0
3b 0 0
0
4 0 0 1 0
5 1
bash: line 54: unset: PIPESTATUS: cannot unset: readonly variable
6 1
bash: line 55: PIPESTATUS: readonly variable
7 0
8 1 0
9 1 0
10 1
11 0
12 3
13 1 2
bash: line 63: local: PIPESTATUS: readonly variable
14 1 0
14b 0
4 0 0 1 0
declare -ar PIPESTATUS=([0]="1" [1]="2")
0 "1" 1 "2"
'1' '2'
ar ar
2
3
f declare -a FUNCNAME=([0]="f" [1]="g")

== status 0
