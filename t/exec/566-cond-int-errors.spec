== stdout 72
start
space 1
or 0
not 0
rhs-skipped i=0
lhs-done i=1
and 1
else 1
next

== stderr 551
bash: line 2: [[: 1 2: arithmetic syntax error in expression (error token is "2")
bash: line 3: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
bash: line 4: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
bash: line 6: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
bash: line 7: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
bash: line 8: [[: 1/0: division by 0 (error token is "0")
bash: line 9: [[: 2#9: value too great for base (error token is "2#9")

== merged 623
start
bash: line 2: [[: 1 2: arithmetic syntax error in expression (error token is "2")
space 1
bash: line 3: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
or 0
bash: line 4: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
not 0
bash: line 6: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
rhs-skipped i=0
bash: line 7: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
lhs-done i=1
bash: line 8: [[: 1/0: division by 0 (error token is "0")
and 1
bash: line 9: [[: 2#9: value too great for base (error token is "2#9")
else 1
next

== status 0
