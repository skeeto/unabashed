== stdout 99
1a status 1
1 2 status 1
dq status 1
at status 1
single quotes status 1
double quotes status 0
end

== stderr 421
bash: line 2: ((: 1a: value too great for base (error token is "1a")
bash: line 4: ((: 1 2: arithmetic syntax error in expression (error token is "2")
bash: line 6: ((: "3": arithmetic syntax error: operand expected (error token is ""3"")
bash: line 8: ((: @: arithmetic syntax error: operand expected (error token is "@")
bash: line 9: ((: '1' + 2 : arithmetic syntax error: operand expected (error token is "'1' + 2 ")

== merged 520
bash: line 2: ((: 1a: value too great for base (error token is "1a")
1a status 1
bash: line 4: ((: 1 2: arithmetic syntax error in expression (error token is "2")
1 2 status 1
bash: line 6: ((: "3": arithmetic syntax error: operand expected (error token is ""3"")
dq status 1
bash: line 8: ((: @: arithmetic syntax error: operand expected (error token is "@")
at status 1
bash: line 9: ((: '1' + 2 : arithmetic syntax error: operand expected (error token is "'1' + 2 ")
single quotes status 1
double quotes status 0
end

== status 0
