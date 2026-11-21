== stdout 103
w=[a b c d e f g h
more text]
w=[: ${ n=1+; }]
w=[: ${ true; }]
inner [echo "$x [$BASH_COMMAND]"]
st 0

== stderr 81
s.sh: line 2: 1+: arithmetic syntax error: operand expected (error token is "+")

== merged 184
s.sh: line 2: 1+: arithmetic syntax error: operand expected (error token is "+")
w=[a b c d e f g h
more text]
w=[: ${ n=1+; }]
w=[: ${ true; }]
inner [echo "$x [$BASH_COMMAND]"]
st 0

== status 0
