== stdout 33
st=0 x=[in]
exit st=3
unset st=1

== stderr 217
bash: line 6: let: 1 ==: arithmetic syntax error: operand expected (error token is "==")
bash: line 2: let: 1 ==: arithmetic syntax error: operand expected (error token is "==")
bash: line 16: unset: unbound variable

== merged 250
st=0 x=[in]
bash: line 6: let: 1 ==: arithmetic syntax error: operand expected (error token is "==")
bash: line 2: let: 1 ==: arithmetic syntax error: operand expected (error token is "==")
exit st=3
bash: line 16: unset: unbound variable
unset st=1

== status 4
