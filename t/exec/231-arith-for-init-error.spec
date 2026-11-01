== stdout 18
status 1
status 1

== stderr 148
bash: line 2: ((: i = 1 +: arithmetic syntax error: operand expected (error token is "+")
bash: line 4: ((: 1/0: division by 0 (error token is "0")

== merged 166
bash: line 2: ((: i = 1 +: arithmetic syntax error: operand expected (error token is "+")
status 1
bash: line 4: ((: 1/0: division by 0 (error token is "0")
status 1

== status 0
