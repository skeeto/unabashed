== stdout 179
x=3 status=0
y=6 z=0
status zero-result 1
last zero 1
last nonzero 0
c=3 status=0
d=1
e=2
f=16 g=8 h=3
i=9
j=4 0
let 0 -> 1
let 1 -> 0
no args -> 1
error mid -> 1 k=1
syntax -> 1

== stderr 190
bash: line 14: let: expression expected
bash: line 15: let: 1/0: division by 0 (error token is "0")
bash: line 16: let: m = : arithmetic syntax error: operand expected (error token is "= ")

== merged 369
x=3 status=0
y=6 z=0
status zero-result 1
last zero 1
last nonzero 0
c=3 status=0
d=1
e=2
f=16 g=8 h=3
i=9
j=4 0
let 0 -> 1
let 1 -> 0
bash: line 14: let: expression expected
no args -> 1
bash: line 15: let: 1/0: division by 0 (error token is "0")
error mid -> 1 k=1
bash: line 16: let: m = : arithmetic syntax error: operand expected (error token is "= ")
syntax -> 1

== status 0
