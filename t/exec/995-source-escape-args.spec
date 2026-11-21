== stdout 111
after: 2 <orig1> <orig2>
orig1 orig2 more
top: orig1 orig2
no args: orig1 orig2
st 0
trap: [a b] [./x.sh]
st 3

== stderr 249
./g.sh: line 2: 1+: arithmetic syntax error: operand expected (error token is "+")
./g.sh: line 2: 1+: arithmetic syntax error: operand expected (error token is "+")
./g.sh: line 2: 1+: arithmetic syntax error: operand expected (error token is "+")

== merged 360
./g.sh: line 2: 1+: arithmetic syntax error: operand expected (error token is "+")
after: 2 <orig1> <orig2>
orig1 orig2 more
./g.sh: line 2: 1+: arithmetic syntax error: operand expected (error token is "+")
top: orig1 orig2
./g.sh: line 2: 1+: arithmetic syntax error: operand expected (error token is "+")
no args: orig1 orig2
st 0
trap: [a b] [./x.sh]
st 3

== status 0
